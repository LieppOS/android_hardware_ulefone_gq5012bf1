// Copyright 2026 The LieppOS Contributors
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <mtk_vibrator/Controller.h>
#include <cerrno>
#include <cmath>
#include <cstdint>
#include <utility>

namespace lieppos::vibrator {
// Production request logic, independent of Binder transport. Host tests exercise
// this code, NOT counterfeit BnVibrator/ScopedAStatus classes or Binder IPC.
class VibratorPolicy final {
  public:
    enum class Kind { Ok, IllegalArgument, Unsupported, IoError };
    struct Result { Kind kind; int error = 0; };
    explicit VibratorPolicy(std::shared_ptr<Controller> controller) : controller_(std::move(controller)) {}
    static constexpr int32_t capabilities() { return 0; }
    static Result unsupported() { return {Kind::Unsupported}; }
    static Result illegalArgument() { return {Kind::IllegalArgument}; }
    Result on(int32_t durationMs, bool hasCallback) {
        if (durationMs <= 0) return illegalArgument();
        if (hasCallback) return unsupported();  // No callback retained or invoked.
        return io(controller_ ? controller_->on(durationMs) : ENODEV);
    }
    Result off() { return io(controller_ ? controller_->off() : ENODEV); }
    static Result amplitude(float value) {
        if (!std::isfinite(value) || value <= 0.0f || value > 1.0f) return illegalArgument();
        return unsupported();
    }
  private:
    static Result io(int error) { return error ? Result{Kind::IoError, error} : Result{Kind::Ok}; }
    std::shared_ptr<Controller> controller_;
};

// No property, command-line argument or init action can bypass this gate.
// Change only after real Android build, reviewed lower-layer config/lifetime,
// actual node policy and separately approved hardware/process-death validation.
inline bool deploymentAllowed() { return false; }
}  // namespace lieppos::vibrator
