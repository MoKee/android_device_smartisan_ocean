/*
 * Copyright (C) 2019 The Android Open Source Project
 * Copyright (C) 2019 The MoKee Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "VibratorService"

#include <android-base/logging.h>

#include "Vibrator.h"

namespace android {
namespace hardware {
namespace vibrator {
namespace V1_3 {
namespace implementation {

Vibrator::Vibrator(vibrator_device_t *device,
                   std::ofstream&& vibrator_state,
                   std::ofstream&& vibrator_duration,
                   std::ofstream&& vibrator_activate)
    : mDevice(device)
    , mVibratorState(std::move(vibrator_state))
    , mVibratorDuration(std::move(vibrator_duration))
    , mVibratorActivate(std::move(vibrator_activate))
    {
}

// Methods from ::android::hardware::vibrator::V1_0::IVibrator follow.

Return<Status> Vibrator::on(uint32_t timeoutMs) {
    if (!mVibratorState || !mVibratorDuration || !mVibratorActivate) {
        return Status::UNKNOWN_ERROR;
    }

    mVibratorState << 1 << std::endl;
    mVibratorDuration << timeoutMs << std::endl;
    mVibratorActivate << 1 << std::endl;

    return Status::OK;
}

Return<Status> Vibrator::off() {
    if (!mVibratorActivate) {
        return Status::UNKNOWN_ERROR;
    }

    mVibratorActivate << 0 << std::endl;

    return Status::OK;
}

Return<bool> Vibrator::supportsAmplitudeControl() {
    return false;
}

Return<Status> Vibrator::setAmplitude(uint8_t) {
    return Status::UNSUPPORTED_OPERATION;
}

Return<void> Vibrator::perform(V1_0::Effect effect, EffectStrength strength, perform_cb _hidl_cb) {
    return perform<decltype(effect)>(effect, strength, _hidl_cb);
}

// Methods from ::android::hardware::vibrator::V1_1::IVibrator follow.

Return<void> Vibrator::perform_1_1(V1_1::Effect_1_1 effect, EffectStrength strength,
                                   perform_cb _hidl_cb) {
    return perform<decltype(effect)>(effect, strength, _hidl_cb);
}

// Methods from ::android::hardware::vibrator::V1_2::IVibrator follow.

Return<void> Vibrator::perform_1_2(V1_2::Effect effect, EffectStrength strength,
                                   perform_cb _hidl_cb) {
    return perform<decltype(effect)>(effect, strength, _hidl_cb);
}

// Methods from ::android::hardware::vibrator::V1_3::IVibrator follow.

Return<bool> Vibrator::supportsExternalControl() {
    return false;
}

Return<Status> Vibrator::setExternalControl(bool) {
    return Status::UNSUPPORTED_OPERATION;
}

Return<void> Vibrator::perform_1_3(Effect effect, EffectStrength strength, perform_cb _hidl_cb) {
    return perform<decltype(effect)>(effect, strength, _hidl_cb);
}

// Private methods follow.

Return<void> Vibrator::perform(Effect effect, EffectStrength, perform_cb _hidl_cb) {
    VendorEffect id;
    uint32_t ms;
    Status status = Status::OK;

    id = effectToVendorId(effect, &status);
    if (status != Status::OK) {
        LOG(ERROR) << "Perform: Effect not supported: " << effectToName(effect);
        _hidl_cb(status, 0);
        return Void();
    }

    ms = mDevice->vibrator_haptic_feedback(mDevice, id);
    if (ms <= 0) {
        LOG(ERROR) << "Perform: Failed to perform feedback: " << ms;
        _hidl_cb(Status::UNKNOWN_ERROR, 0);
        return Void();
    }

    LOG(INFO) << "Perform: Effect " << effectToName(effect)
              << " => ID " << id << ", duration = " << ms;

    _hidl_cb(status, ms);

    return Void();
}

template <typename T>
Return<void> Vibrator::perform(T effect, EffectStrength strength, perform_cb _hidl_cb) {
    auto validRange = hidl_enum_range<T>();
    if (effect < *validRange.begin() || effect > *std::prev(validRange.end())) {
        _hidl_cb(Status::UNSUPPORTED_OPERATION, 0);
        return Void();
    }
    return perform(static_cast<Effect>(effect), strength, _hidl_cb);
}

const std::string Vibrator::effectToName(Effect effect) {
    return toString(effect);
}

VendorEffect Vibrator::effectToVendorId(Effect effect, Status* status) {
    switch (effect) {
        case Effect::CLICK:
            return VendorEffect::IMPACT_MEDIUM;
        case Effect::DOUBLE_CLICK:
            return VendorEffect::NOTIFICATION_WARNING;
        case Effect::TICK:
        case Effect::TEXTURE_TICK:
            return VendorEffect::SELECTION_LIGHT;
        case Effect::THUD:
            return VendorEffect::IMPACT_HEAVY;
        case Effect::POP:
            return VendorEffect::SELECTION_HEAVY;
        case Effect::HEAVY_CLICK:
            return VendorEffect::IMPACT_HEAVY;
        default:
            *status = Status::UNSUPPORTED_OPERATION;
            return VendorEffect::NONE;
    }
}

}  // namespace implementation
}  // namespace V1_3
}  // namespace vibrator
}  // namespace hardware
}  // namespace android
