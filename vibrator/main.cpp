// Copyright 2026 The LieppOS Contributors
// SPDX-License-Identifier: Apache-2.0
#include "Vibrator.h"

#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>

#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <utility>

int main(int argc, char** argv) {
    (void)argc;
    android::base::InitLogging(argv, android::base::LogdLogger(android::base::SYSTEM));
    if (!lieppos::vibrator::deploymentAllowed()) {
        LOG(ERROR) << "Deployment CLOSED: target build, kernel expiry/lifetime and hardware validation required";
        return EXIT_FAILURE;  // Before device discovery, writes or registration.
    }
    int error = 0;
    // GQ5012BF1 live DT root node /regulator-vibrator, label "vibrator"; the
    // stock vendor genfscon labels exactly this path sysfs_vibrator. Opening it
    // directly avoids enumerating every LED (which needs broad LED access).
    auto device = lieppos::vibrator::openDriverTimedRegulatorLed(
            "/sys/devices/platform/regulator-vibrator", "vibrator", &error);
    if (!device || !device->usesKernelTimerProtocol()) {
        LOG(ERROR) << "Cannot open timed regulator_vibrator LED: " << std::strerror(error);
        return EXIT_FAILURE;
    }
    auto controller = std::make_shared<lieppos::vibrator::Controller>(std::move(device), [](int err) {
        LOG(ERROR) << "Vibrator safety-off write failed: " << std::strerror(err);
    });
    error = controller->off();
    if (error) {
        LOG(ERROR) << "Vibrator initial off failed: " << std::strerror(error);
        return EXIT_FAILURE;
    }
    using aidl::android::hardware::vibrator::Vibrator;
    auto service = ndk::SharedRefBase::make<Vibrator>(controller);
    const std::string instance = std::string(Vibrator::descriptor) + "/default";
    ABinderProcess_setThreadPoolMaxThreadCount(0);
    const binder_status_t registered = AServiceManager_addService(service->asBinder().get(), instance.c_str());
    if (registered != STATUS_OK) {
        LOG(ERROR) << "Cannot register " << instance << ": " << registered;
        return EXIT_FAILURE;
    }
    ABinderProcess_joinThreadPool();
    LOG(ERROR) << "Binder thread pool returned unexpectedly";
    return EXIT_FAILURE;
}
