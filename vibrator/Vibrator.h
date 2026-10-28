// Copyright 2026 The LieppOS Contributors
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <aidl/android/hardware/vibrator/BnVibrator.h>
#include "VibratorPolicy.h"

namespace aidl::android::hardware::vibrator {
// Implements the genuine frozen V2 interface; no local AIDL redefinition.
class Vibrator final : public BnVibrator {
  public:
    explicit Vibrator(std::shared_ptr<::lieppos::vibrator::Controller> controller);
    ndk::ScopedAStatus getCapabilities(int32_t* result) override;
    ndk::ScopedAStatus off() override;
    ndk::ScopedAStatus on(int32_t timeoutMs, const std::shared_ptr<IVibratorCallback>& callback) override;
    ndk::ScopedAStatus perform(Effect, EffectStrength, const std::shared_ptr<IVibratorCallback>&, int32_t*) override;
    ndk::ScopedAStatus getSupportedEffects(std::vector<Effect>* result) override;
    ndk::ScopedAStatus setAmplitude(float amplitude) override;
    ndk::ScopedAStatus setExternalControl(bool) override;
    ndk::ScopedAStatus getCompositionDelayMax(int32_t*) override;
    ndk::ScopedAStatus getCompositionSizeMax(int32_t*) override;
    ndk::ScopedAStatus getSupportedPrimitives(std::vector<CompositePrimitive>*) override;
    ndk::ScopedAStatus getPrimitiveDuration(CompositePrimitive, int32_t*) override;
    ndk::ScopedAStatus compose(const std::vector<CompositeEffect>&, const std::shared_ptr<IVibratorCallback>&) override;
    ndk::ScopedAStatus getSupportedAlwaysOnEffects(std::vector<Effect>*) override;
    ndk::ScopedAStatus alwaysOnEnable(int32_t, Effect, EffectStrength) override;
    ndk::ScopedAStatus alwaysOnDisable(int32_t) override;
    ndk::ScopedAStatus getResonantFrequency(float*) override;
    ndk::ScopedAStatus getQFactor(float*) override;
    ndk::ScopedAStatus getFrequencyResolution(float*) override;
    ndk::ScopedAStatus getFrequencyMinimum(float*) override;
    ndk::ScopedAStatus getBandwidthAmplitudeMap(std::vector<float>*) override;
    ndk::ScopedAStatus getPwlePrimitiveDurationMax(int32_t*) override;
    ndk::ScopedAStatus getPwleCompositionSizeMax(int32_t*) override;
    ndk::ScopedAStatus getSupportedBraking(std::vector<Braking>*) override;
    ndk::ScopedAStatus composePwle(const std::vector<PrimitivePwle>&, const std::shared_ptr<IVibratorCallback>&) override;
  private:
    ::lieppos::vibrator::VibratorPolicy policy_;
};
}  // namespace aidl::android::hardware::vibrator
