// Copyright 2026 The LieppOS Contributors
// SPDX-License-Identifier: Apache-2.0
#include "Vibrator.h"

#include <cerrno>
#include <cmath>
#include <utility>

namespace aidl::android::hardware::vibrator {
namespace {
ndk::ScopedAStatus unsupported() {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}
ndk::ScopedAStatus status(int error) {
    if (!error) return ndk::ScopedAStatus::ok();
    // Client validation is performed before I/O. Preserve backend errno even
    // when the kernel rejects a write with EINVAL.
    return ndk::ScopedAStatus::fromServiceSpecificError(error);
}
}  // namespace

Vibrator::Vibrator(std::shared_ptr<::lieppos::vibrator::Controller> controller)
    : controller_(std::move(controller)) {}

ndk::ScopedAStatus Vibrator::getCapabilities(int32_t* result) {
    if (!result) return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    *result = 0;  // Timed on/off only. No unverified callbacks/amplitude/effects.
    return ndk::ScopedAStatus::ok();
}
ndk::ScopedAStatus Vibrator::off() { return status(controller_->off()); }
ndk::ScopedAStatus Vibrator::on(int32_t timeoutMs, const std::shared_ptr<IVibratorCallback>& callback) {
    if (timeoutMs <= 0) return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    if (callback) return unsupported();
    return status(controller_->on(timeoutMs));
}
ndk::ScopedAStatus Vibrator::getSupportedEffects(std::vector<Effect>* result) {
    if (!result) return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    result->clear();
    return ndk::ScopedAStatus::ok();
}
ndk::ScopedAStatus Vibrator::setAmplitude(float amplitude) {
    if (!std::isfinite(amplitude) || amplitude <= 0.0f || amplitude > 1.0f)
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    return unsupported();
}
ndk::ScopedAStatus Vibrator::perform(Effect, EffectStrength, const std::shared_ptr<IVibratorCallback>&, int32_t*) { return unsupported(); }
ndk::ScopedAStatus Vibrator::setExternalControl(bool) { return unsupported(); }
ndk::ScopedAStatus Vibrator::getCompositionDelayMax(int32_t*) { return unsupported(); }
ndk::ScopedAStatus Vibrator::getCompositionSizeMax(int32_t*) { return unsupported(); }
ndk::ScopedAStatus Vibrator::getSupportedPrimitives(std::vector<CompositePrimitive>*) { return unsupported(); }
ndk::ScopedAStatus Vibrator::getPrimitiveDuration(CompositePrimitive, int32_t*) { return unsupported(); }
ndk::ScopedAStatus Vibrator::compose(const std::vector<CompositeEffect>&, const std::shared_ptr<IVibratorCallback>&) { return unsupported(); }
ndk::ScopedAStatus Vibrator::getSupportedAlwaysOnEffects(std::vector<Effect>*) { return unsupported(); }
ndk::ScopedAStatus Vibrator::alwaysOnEnable(int32_t, Effect, EffectStrength) { return unsupported(); }
ndk::ScopedAStatus Vibrator::alwaysOnDisable(int32_t) { return unsupported(); }
ndk::ScopedAStatus Vibrator::getResonantFrequency(float*) { return unsupported(); }
ndk::ScopedAStatus Vibrator::getQFactor(float*) { return unsupported(); }
ndk::ScopedAStatus Vibrator::getFrequencyResolution(float*) { return unsupported(); }
ndk::ScopedAStatus Vibrator::getFrequencyMinimum(float*) { return unsupported(); }
ndk::ScopedAStatus Vibrator::getBandwidthAmplitudeMap(std::vector<float>*) { return unsupported(); }
ndk::ScopedAStatus Vibrator::getPwlePrimitiveDurationMax(int32_t*) { return unsupported(); }
ndk::ScopedAStatus Vibrator::getPwleCompositionSizeMax(int32_t*) { return unsupported(); }
ndk::ScopedAStatus Vibrator::getSupportedBraking(std::vector<Braking>*) { return unsupported(); }
ndk::ScopedAStatus Vibrator::composePwle(const std::vector<PrimitivePwle>&, const std::shared_ptr<IVibratorCallback>&) { return unsupported(); }
}  // namespace aidl::android::hardware::vibrator
