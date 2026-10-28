// Copyright 2026 The LieppOS Contributors
// SPDX-License-Identifier: Apache-2.0
// Host tests: production core/policy with fake attributes, never Binder IPC.
#include <mtk_vibrator/Controller.h>
#include "VibratorPolicy.h"

#include <sys/resource.h>
#include <sys/wait.h>
#include <poll.h>
#include <signal.h>
#include <unistd.h>

#include <cerrno>
#include <climits>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <stdexcept>
#include <vector>

namespace fs = std::filesystem;
using namespace lieppos::vibrator;
using Kind = VibratorPolicy::Kind;
#define CHECK(x) do { if (!(x)) throw std::runtime_error(#x); } while (false)

struct State {
    std::mutex mutex;
    std::condition_variable changed;
    std::vector<bool> writes;
    int failOn = 0, failOff = 0, errors = 0;
    bool waitOff() {
        std::unique_lock lock(mutex);
        return changed.wait_for(lock, std::chrono::seconds(3), [&] { return !writes.empty() && !writes.back(); });
    }
};
struct FakeSwitch final : Switch {
    explicit FakeSwitch(std::shared_ptr<State> s) : state(std::move(s)) {}
    int set(bool value) override {
        std::lock_guard lock(state->mutex);
        auto& failure = value ? state->failOn : state->failOff;
        if (failure) { --failure; return EIO; }
        state->writes.push_back(value); state->changed.notify_all(); return 0;
    }
    std::shared_ptr<State> state;
};
std::shared_ptr<Controller> controller(const std::shared_ptr<State>& s) {
    return std::make_shared<Controller>(std::make_unique<FakeSwitch>(s));
}

struct Attributes {
    std::string trigger = "[none] oneshot";
    std::vector<std::pair<std::string, std::string>> writes;
    std::map<std::string, std::string> values;
    int attempts = 0, failAt = 0, failRead = 0;
    bool ignoreSelect = false;
};
struct FakeAttributes final : LedAttributes {
    explicit FakeAttributes(std::shared_ptr<Attributes> a) : state(std::move(a)) {}
    int read(const char*, std::string* result) override {
        if (state->failRead) return state->failRead;
        *result = state->trigger; return 0;
    }
    int write(const char* name, const std::string& value) override {
        if (++state->attempts == state->failAt) return EIO;
        state->writes.emplace_back(name, value); state->values[name] = value;
        if (std::string(name) == "brightness") state->trigger = "[none] oneshot";
        if (std::string(name) == "trigger" && !state->ignoreSelect) state->trigger = "none [oneshot]";
        if (std::string(name) == "shot") {
            CHECK(state->trigger == "none [oneshot]");
            CHECK(state->values["invert"] == "0\n");
            CHECK(state->values["delay_off"] == "1\n");
            CHECK(std::stoi(state->values["delay_on"]) > 0);
        }
        return 0;
    }
    std::shared_ptr<Attributes> state;
};
std::unique_ptr<Switch> timed(const std::shared_ptr<Attributes>& a, int* error) {
    return makeOneShotSwitch(std::make_unique<FakeAttributes>(a), error);
}
void put(const fs::path& p, const std::string& value) { std::ofstream(p) << value; }
std::string get(const fs::path& p) {
    std::ifstream in(p); return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}
fs::path led(const fs::path& root) {
    const auto entry = root / "dt-defined-label";
    fs::create_directories(entry / "device");
    fs::create_symlink("../../drivers/regulator_vibrator", entry / "device/driver");
    put(entry / "max_brightness", "255\n"); put(entry / "brightness", "0\n");
    return entry;
}
struct Child {
    pid_t pid;
    ~Child() {
        if (pid > 0) { kill(pid, SIGKILL); int status; while (waitpid(pid, &status, 0) < 0 && errno == EINTR) {} }
    }
};

int main(int argc, char** argv) {
    if (argc != 2) return 2;
    const fs::path base = argv[1];
    const rlimit noCore{0, 0};
    if (setrlimit(RLIMIT_CORE, &noCore)) return 2;
    int passed = 0;
    const auto test = [&](const char* name, const std::function<void()>& body) {
        body(); ++passed; std::cout << "PASS " << name << '\n';
    };
    try {
        test("truthful capabilities callback and amplitude policy", [&] {
            auto s = std::make_shared<State>(); VibratorPolicy p(controller(s));
            CHECK(!deploymentAllowed());
            CHECK(p.capabilities() == 0); CHECK(p.unsupported().kind == Kind::Unsupported);
            CHECK(p.on(1, true).kind == Kind::Unsupported); CHECK(s->writes.empty());
            for (float value : {0.0f, -1.0f, 1.1f, std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN()})
                CHECK(p.amplitude(value).kind == Kind::IllegalArgument);
            CHECK(p.amplitude(0.5f).kind == Kind::Unsupported); CHECK(p.amplitude(1.0f).kind == Kind::Unsupported);
        });
        test("duration bounds validate before I/O", [&] {
            auto s = std::make_shared<State>(); VibratorPolicy p(controller(s));
            for (int value : {INT_MIN, -1, 0}) CHECK(p.on(value, false).kind == Kind::IllegalArgument);
            CHECK(s->writes.empty()); CHECK(p.on(INT_MAX, false).kind == Kind::Ok); CHECK(p.off().kind == Kind::Ok);
        });
        test("backend failure preserved and reenable latched until explicit off", [&] {
            auto s = std::make_shared<State>(); s->failOn = 1; VibratorPolicy p(controller(s));
            auto result = p.on(1000, false); CHECK(result.kind == Kind::IoError && result.error == EIO);
            CHECK(s->waitOff()); CHECK(p.on(10, false).kind == Kind::IoError);
            CHECK(p.off().kind == Kind::Ok); CHECK(p.on(1000, false).kind == Kind::Ok);
        });
        test("failed standalone off arms retry even with no previous deadline", [&] {
            auto s = std::make_shared<State>(); s->failOff = 1; auto c = controller(s);
            CHECK(c->off() == EIO); CHECK(s->waitOff()); CHECK(c->on(10) == EIO);
            CHECK(c->off() == 0); CHECK(c->on(1000) == 0);
        });
        test("failed replacement stops now rather than at old long deadline", [&] {
            auto s = std::make_shared<State>(); auto c = controller(s); CHECK(c->on(200000) == 0);
            { std::lock_guard lock(s->mutex); s->failOn = 1; }
            CHECK(c->on(200000) == EIO); CHECK(s->waitOff());
        });
        test("timer error latches and reports without runaway retry", [&] {
            auto s = std::make_shared<State>(); s->failOff = 1000;
            Controller c(std::make_unique<FakeSwitch>(s), [s](int error) {
                CHECK(error == EIO); std::lock_guard lock(s->mutex); ++s->errors; s->changed.notify_all();
            });
            CHECK(c.on(1) == 0);
            { std::unique_lock lock(s->mutex); CHECK(s->changed.wait_for(lock, std::chrono::seconds(3), [&] { return s->errors > 0; })); }
            CHECK(c.on(1000) == EIO);
            { std::lock_guard lock(s->mutex); s->failOff = 0; }
            CHECK(c.off() == 0);
        });
        test("partial null initialization is ENODEV not worker crash", [&] {
            Controller c(nullptr); CHECK(c.on(1) == ENODEV); CHECK(c.off() == ENODEV);
            VibratorPolicy p(nullptr); CHECK(p.on(1, false).error == ENODEV); CHECK(p.off().error == ENODEV);
        });
        test("cancelled timer cannot stop a replacement", [&] {
            auto s = std::make_shared<State>(); auto c = controller(s);
            CHECK(c->on(10) == 0); CHECK(c->off() == 0); CHECK(c->on(200000) == 0);
            std::this_thread::sleep_for(std::chrono::milliseconds(40));
            { std::lock_guard lock(s->mutex); CHECK(s->writes.back()); }
            CHECK(c->off() == 0);
        });
        test("concurrent request policy uses serialized backend", [&] {
            auto s = std::make_shared<State>(); VibratorPolicy p(controller(s));
            std::vector<std::thread> threads;
            for (int i = 0; i < 4; ++i) threads.emplace_back([&] {
                for (int j = 0; j < 40; ++j) { CHECK(p.on(200000, false).kind == Kind::Ok); CHECK(p.off().kind == Kind::Ok); }
            });
            for (auto& t : threads) t.join();
            CHECK(p.off().kind == Kind::Ok);
        });
        test("graceful shutdown and repeated controller restart stop", [&] {
            auto s = std::make_shared<State>();
            for (int i = 0; i < 20; ++i) {
                { auto c = controller(s); CHECK(c->off() == 0); CHECK(c->on(200000) == 0); }
                CHECK(!s->writes.back());
            }
        });
        test("one shot is finite configured last and never brightness on", [&] {
            auto a = std::make_shared<Attributes>(); int error; auto d = timed(a, &error);
            CHECK(d && error == 0 && d->usesKernelTimerProtocol()); CHECK(d->set(true) == EOPNOTSUPP);
            CHECK(d->start(0) == EINVAL && a->writes.empty()); CHECK(d->start(123) == 0);
            const std::vector<std::pair<std::string, std::string>> expected{
                {"brightness", "0\n"}, {"trigger", "oneshot\n"}, {"invert", "0\n"},
                {"delay_on", "123\n"}, {"delay_off", "1\n"}, {"shot", "1\n"}};
            CHECK(a->writes == expected);
        });
        test("kernel protocol is not truncated by an earlier userspace deadline", [&] {
            auto a = std::make_shared<Attributes>(); int error;
            Controller c(timed(a, &error)); CHECK(error == 0); CHECK(c.on(5) == 0);
            std::this_thread::sleep_for(std::chrono::milliseconds(30));
            // Fake attributes do not implement a kernel. This tests only that
            // our software does not preempt real kernel expiry with OFF.
            CHECK(a->writes.size() == 6 && a->writes.back().first == "shot");
            CHECK(c.off() == 0 && a->writes.back().first == "brightness");
        });
        test("failure at each one shot transaction never fakes success", [&] {
            for (int stage = 1; stage <= 6; ++stage) {
                auto a = std::make_shared<Attributes>(); a->failAt = stage; int error; auto d = timed(a, &error);
                CHECK(d && d->start(100) == EIO); CHECK(a->writes.size() == static_cast<size_t>(stage - 1));
            }
            auto a = std::make_shared<Attributes>(); int error; auto d = timed(a, &error);
            a->failRead = ENOENT; CHECK(d->start(1) == ENOENT);
        });
        test("missing or unselected trigger never falls back", [&] {
            int error; auto a = std::make_shared<Attributes>(); a->trigger = "[none] not-oneshot oneshot-extra";
            CHECK(!timed(a, &error) && error == EOPNOTSUPP);
            a->trigger = "[none] oneshot"; a->ignoreSelect = true; auto d = timed(a, &error);
            CHECK(d && d->start(1) == EOPNOTSUPP); CHECK(a->writes.size() == 2);
            CHECK(!makeOneShotSwitch(nullptr, &error) && error == ENODEV);
        });
        test("repeated shot cancels before rearm instead of ignored overlap", [&] {
            auto a = std::make_shared<Attributes>(); int error; auto d = timed(a, &error);
            CHECK(d && d->start(10) == 0 && d->start(20) == 0);
            CHECK(a->writes.size() == 12 && a->writes[6].first == "brightness" && a->writes[11].first == "shot");
        });
        test("timed discovery rejects missing trigger and attribute symlinks", [&] {
            const auto root = base / "timed-discovery"; const auto entry = led(root); int error;
            CHECK(!discoverTimedRegulatorLed(root.string(), &error) && error == ENOENT);
            put(entry / "other", "[none] oneshot\n"); fs::create_symlink("other", entry / "trigger");
            CHECK(!discoverTimedRegulatorLed(root.string(), &error) && error == ELOOP);
            fs::remove(entry / "trigger"); put(entry / "trigger", "[none] oneshot\n");
            auto d = discoverTimedRegulatorLed(root.string(), &error); CHECK(d && error == 0);
            // A plain file is not a kernel trigger; no fabricated [oneshot] acknowledgment.
            CHECK(d->start(1) == EOPNOTSUPP); CHECK(get(entry / "brightness") == "0\n");
        });
        test("SIGTERM SIGKILL crash show legacy limit; restart initial off repairs", [&] {
            for (int sig : {SIGTERM, SIGKILL, SIGABRT}) {
                const auto root = base / ("kill-" + std::to_string(sig)); const auto entry = led(root);
                int pipefd[2]; CHECK(pipe(pipefd) == 0); std::cout.flush();
                Child child{fork()}; CHECK(child.pid >= 0);
                if (child.pid == 0) {
                    close(pipefd[0]); int error; auto d = discoverRegulatorLed(root.string(), &error);
                    if (!d) _exit(3);
                    Controller c(std::move(d)); if (c.on(200000)) _exit(4);
                    if (write(pipefd[1], "R", 1) != 1) _exit(5);
                    for (;;) pause();
                }
                close(pipefd[1]); pollfd ready{pipefd[0], POLLIN, 0};
                const int polled = poll(&ready, 1, 3000); char value = 0;
                const ssize_t count = polled > 0 ? read(pipefd[0], &value, 1) : -1; close(pipefd[0]);
                CHECK(count == 1 && value == 'R'); CHECK(kill(child.pid, sig) == 0);
                int status; CHECK(waitpid(child.pid, &status, 0) == child.pid); child.pid = -1;
                CHECK(WIFSIGNALED(status) && WTERMSIG(status) == sig);
                std::this_thread::sleep_for(std::chrono::milliseconds(30)); CHECK(get(entry / "brightness") == "1\n");
                int error; auto d = discoverRegulatorLed(root.string(), &error); CHECK(d);
                Controller restarted(std::move(d)); CHECK(restarted.off() == 0); CHECK(get(entry / "brightness") == "0\n");
            }
        });
    } catch (const std::exception& e) { std::cerr << "FAIL after " << passed << ": " << e.what() << '\n'; return 1; }
    std::cout << "TOTAL " << passed << "/17\n";
    return passed == 17 ? 0 : 1;
}
