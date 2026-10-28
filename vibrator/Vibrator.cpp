// Copyright 2026 The LieppOS Contributors
// SPDX-License-Identifier: Apache-2.0
#include "Vibrator.h"

#include <utility>

namespace aidl::android::hardware::vibrator {
namespace {
using Policy = ::lieppos::vibrator::VibratorPolicy;
ndk::ScopedAStatus status(Policy::Result result) {
    switch (result.kind) {
        case Policy::Kind::Ok: return ndk::ScopedAStatus::ok();
        case Policy::Kind::IllegalArgument:
            return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
        case Policy::Kind::Unsupported:
            return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
        case Policy::Kind::IoError:
            return ndk::ScopedAStatus::fromServiceSpecificError(result.error);
    }
    return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
}
ndk::ScopedAStatus unsupported() { return status(Policy::unsupported()); }
}  // namespace

Vibrator::Vibrator(std::shared_ptr<::lieppos::vibrator::Controller> controller)
    : policy_(std::move(controller)) {}

ndk::ScopedAStatus Vibrator::getCapabilities(int32_t* result) {
    if (!result) return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    *result = Policy::capabilities();  // No unverified callbacks/amplitude/effects.
    return ndk::ScopedAStatus::ok();
}
ndk::ScopedAStatus Vibrator::off() { return status(policy_.off()); }
ndk::ScopedAStatus Vibrator::on(int32_t timeoutMs, const std::shared_ptr<IVibratorCallback>& callback) {
    return status(policy_.on(timeoutMs, static_cast<bool>(callback)));
}
ndk::ScopedAStatus Vibrator::getSupportedEffects(std::vector<Effect>* result) {
    if (!result) return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    result->clear();
    return ndk::ScopedAStatus::ok();
}
ndk::ScopedAStatus Vibrator::setAmplitude(float amplitude) {
    return status(Policy::amplitude(amplitude));
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
