#pragma once

#include "can_frame_types.h"

constexpr uint8_t kMuxMask = 0x07;
constexpr uint8_t kFsdUiMask = 0x40;
constexpr uint8_t kHw3ProfileMask = 0x06;

inline uint8_t readMuxID(const CanFrame& frame) {
    return frame.data[0] & kMuxMask;
}

inline bool isFSDSelectedInUI(const CanFrame& frame) {
    return (frame.data[4] & kFsdUiMask) != 0;
}

inline void setSpeedProfileV12V13(CanFrame& frame, int profile) {
    frame.data[6] &= static_cast<uint8_t>(~kHw3ProfileMask);
    frame.data[6] |= (profile << 1);
}

inline void setBit(CanFrame& frame, int bit, bool value) {
    if (bit < 0 || bit >= 64) {
        return;
    }
    int byteIndex = bit / 8;
    int bitIndex = bit % 8;
    uint8_t mask = static_cast<uint8_t>(1U << bitIndex);
    if (value) {
        frame.data[byteIndex] |= mask;
    } else {
        frame.data[byteIndex] &= static_cast<uint8_t>(~mask);
    }
}
