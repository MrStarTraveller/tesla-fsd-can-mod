#pragma once

#include <memory>
#include <algorithm>
#include "can_frame_types.h"
#include "drivers/can_driver.h"
#include "can_helpers.h"

#ifndef NATIVE_BUILD
#include <Arduino.h>
#endif

constexpr bool enableEmergencyVehicleDetection = true;
constexpr bool DEFAULT_ISA_SPEED_CHIME_SUPPRESS = false;

namespace handler_detail {
constexpr uint32_t kLegacyStwActnRqId = 69;
constexpr uint32_t kLegacyAutopilotId = 1006;
constexpr uint32_t kUiDriverAssistControlId = 1016;
constexpr uint32_t kUiAutopilotControlId = 1021;
constexpr uint32_t kDasStatusId = 921;

constexpr int kNagBit = 19;
constexpr int kFsdEnableBit = 46;
constexpr int kHw4NagAcknowledgeBit = 47;
constexpr int kEmergencyVehicleBit = 59;
constexpr int kFsdV14Bit = 60;

inline int mapLegacyStalkToProfile(uint8_t stalkPosition) {
    if (stalkPosition <= 1) return 2;
    if (stalkPosition == 2) return 1;
    return 0;
}

inline int mapHW3FollowDistanceToProfile(uint8_t followDistance, int currentProfile) {
    switch (followDistance) {
        case 1: return 2;
        case 2: return 1;
        case 3: return 0;
        default: return currentProfile;
    }
}

inline int mapHW4FollowDistanceToProfile(uint8_t followDistance, int currentProfile) {
    switch (followDistance) {
        case 1: return 3;
        case 2: return 2;
        case 3: return 1;
        case 4: return 0;
        case 5: return 4;
        default: return currentProfile;
    }
}

inline int profileFromOffset(int offset, int currentProfile) {
    switch (offset) {
        case 2: return 2;
        case 1: return 1;
        case 0: return 0;
        default: return currentProfile;
    }
}

inline uint8_t readFollowDistance(const CanFrame& frame) {
    return static_cast<uint8_t>((frame.data[5] & 0b11100000) >> 5);
}

inline uint8_t readSpeedOffsetSource(const CanFrame& frame) {
    return static_cast<uint8_t>((frame.data[3] >> 1) & 0x3F);
}

inline uint8_t clampSpeedOffset(uint8_t offsetSource) {
    int offset = (static_cast<int>(offsetSource) - 30) * 5;
    return static_cast<uint8_t>(std::clamp(offset, 0, 100));
}

inline int signedProfileOffset(uint8_t offsetSource) {
    return static_cast<int>(offsetSource) - 30;
}

inline uint8_t calculateDasStatusChecksum(const CanFrame& frame) {
    uint16_t sum = 0;
    for (int i = 0; i < 7; i++) {
        sum += frame.data[i];
    }
    sum += (kDasStatusId & 0xFF) + (kDasStatusId >> 8);
    return static_cast<uint8_t>(sum & 0xFF);
}
}

struct CarManagerBase {
    int speedProfile = 1;
    bool FSDEnabled = false;
    bool enablePrint = true;
    virtual void handleMessage(CanFrame& frame, CanDriver& driver) = 0;
    virtual const uint32_t* filterIds() const = 0;
    virtual uint8_t filterIdCount() const = 0;
    virtual ~CarManagerBase() = default;
};

struct LegacyHandler : public CarManagerBase {
    const uint32_t* filterIds() const override {
        static constexpr uint32_t ids[] = {handler_detail::kLegacyStwActnRqId, handler_detail::kLegacyAutopilotId};
        return ids;
    }
    uint8_t filterIdCount() const override { return 2; }

    void handleMessage(CanFrame& frame, CanDriver& driver) override {
        // STW_ACTN_RQ (0x045 = 69): Follow-Distance-Stalk as Source for Profile Mapping
        // byte[1]: 0x00=Pos1, 0x21=Pos2, 0x42=Pos3, 0x64=Pos4, 0x85=Pos5, 0xA6=Pos6, 0xC8=Pos7
        if (frame.id == handler_detail::kLegacyStwActnRqId) {
            uint8_t pos = frame.data[1] >> 5;
            speedProfile = handler_detail::mapLegacyStalkToProfile(pos);
            return;
        }
        if (frame.id == handler_detail::kLegacyAutopilotId) {
            auto index = readMuxID(frame);
            if (index == 0) FSDEnabled = isFSDSelectedInUI(frame);
            if (index == 0 && FSDEnabled) {
                setBit(frame, handler_detail::kFsdEnableBit, true);
                setSpeedProfileV12V13(frame, speedProfile);
                driver.send(frame);
            }
            if (index == 1) {
                setBit(frame, handler_detail::kNagBit, false);
                driver.send(frame);
            }
#ifndef NATIVE_BUILD
            if (index == 0 && enablePrint) {
                Serial.printf("LegacyHandler: FSD: %d, Profile: %d\n", FSDEnabled, speedProfile);
            }
#endif
        }
    }
};

struct HW3Handler : public CarManagerBase {
    int speedOffset = 0;

    const uint32_t* filterIds() const override {
        static constexpr uint32_t ids[] = {handler_detail::kUiDriverAssistControlId, handler_detail::kUiAutopilotControlId};
        return ids;
    }
    uint8_t filterIdCount() const override { return 2; }

    void handleMessage(CanFrame& frame, CanDriver& driver) override {
        if (frame.id == handler_detail::kUiDriverAssistControlId) {
            speedProfile = handler_detail::mapHW3FollowDistanceToProfile(
                handler_detail::readFollowDistance(frame), speedProfile);
            return;
        }
        if (frame.id == handler_detail::kUiAutopilotControlId) {
            auto index = readMuxID(frame);
            if (index == 0) FSDEnabled = isFSDSelectedInUI(frame);
            if (index == 0 && FSDEnabled) {
                const uint8_t offsetSource = handler_detail::readSpeedOffsetSource(frame);
                speedOffset = handler_detail::clampSpeedOffset(offsetSource);
                speedProfile = handler_detail::profileFromOffset(
                    handler_detail::signedProfileOffset(offsetSource), speedProfile);
                setBit(frame, handler_detail::kFsdEnableBit, true);
                setSpeedProfileV12V13(frame, speedProfile);
                driver.send(frame);
            }
            if (index == 1) {
                setBit(frame, handler_detail::kNagBit, false);
                driver.send(frame);
            }
            if (index == 2 && FSDEnabled) {
                frame.data[0] &= ~(0b11000000);
                frame.data[1] &= ~(0b00111111);
                frame.data[0] |= (speedOffset & 0x03) << 6;
                frame.data[1] |= (speedOffset >> 2);
                driver.send(frame);
            }
#ifndef NATIVE_BUILD
            if (index == 0 && enablePrint) {
                Serial.printf("HW3Handler: FSD: %d, Profile: %d, Offset: %d\n", FSDEnabled, speedProfile, speedOffset);
            }
#endif
        }
    }
};

struct HW4Handler : public CarManagerBase {
    bool isaSpeedChimeSuppress = DEFAULT_ISA_SPEED_CHIME_SUPPRESS; // suppresses ISA speed chime; speed limit sign will be empty while driving

    const uint32_t* filterIds() const override {
        static constexpr uint32_t ids[] = {
            handler_detail::kDasStatusId,
            handler_detail::kUiDriverAssistControlId,
            handler_detail::kUiAutopilotControlId,
        };
        return ids;
    }
    uint8_t filterIdCount() const override { return 3; }

    void handleMessage(CanFrame& frame, CanDriver& driver) override {
        if (isaSpeedChimeSuppress && frame.id == handler_detail::kDasStatusId) {
            frame.data[1] |= 0x20;
            frame.data[7] = handler_detail::calculateDasStatusChecksum(frame);
            driver.send(frame);
            return;
        }
        if (frame.id == handler_detail::kUiDriverAssistControlId) {
            speedProfile = handler_detail::mapHW4FollowDistanceToProfile(
                handler_detail::readFollowDistance(frame), speedProfile);
            return;
        }
        if (frame.id == handler_detail::kUiAutopilotControlId) {
            auto index = readMuxID(frame);
            if (index == 0) FSDEnabled = isFSDSelectedInUI(frame);
            if (index == 0 && FSDEnabled) {
                setBit(frame, handler_detail::kFsdEnableBit, true);
                setBit(frame, handler_detail::kFsdV14Bit, true);
                if (enableEmergencyVehicleDetection) {
                    setBit(frame, handler_detail::kEmergencyVehicleBit, true);
                }
                driver.send(frame);
            }
            if (index == 1) {
                setBit(frame, handler_detail::kNagBit, false);
                setBit(frame, handler_detail::kHw4NagAcknowledgeBit, true);
                driver.send(frame);
            }
            if (index == 2) {
                frame.data[7] &= ~(0x07 << 4);
                frame.data[7] |= (speedProfile & 0x07) << 4;
                driver.send(frame);
            }
#ifndef NATIVE_BUILD
            if (index == 0 && enablePrint) {
                Serial.printf("HW4Handler: FSD: %d, profile: %d\n", FSDEnabled, speedProfile);
            }
#endif
        }
    }
};
