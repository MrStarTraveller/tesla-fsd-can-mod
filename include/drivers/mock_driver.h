#pragma once

#include <deque>
#include <vector>
#include "../can_frame_types.h"
#include "can_driver.h"

class MockDriver : public CanDriver {
public:
    static constexpr bool kSupportsISR = false;

    std::vector<CanFrame> sent;
    std::deque<CanFrame> received;
    bool initResult = true;
    bool sendResult = true;
    CanDriverError idleReadError = CanDriverError::NoMessage;

    bool init() override {
        resetDiagnostics();
        if (!initResult) {
            recordInitFailure();
            return false;
        }
        clearLastError();
        return true;
    }
    void setFilters(const uint32_t* /*ids*/, uint8_t /*count*/) override {}
    bool enableInterrupt(void (* /*onReady*/)()) override { return false; }

    bool read(CanFrame& frame) override {
        if (received.empty()) {
            if (idleReadError == CanDriverError::NoMessage) {
                recordNoMessageRead();
            } else if (idleReadError != CanDriverError::None) {
                recordReadFailure(idleReadError);
            } else {
                clearLastError();
            }
            return false;
        }

        frame = received.front();
        received.pop_front();
        clearLastError();
        return true;
    }

    void send(const CanFrame& frame) override {
        if (!sendResult) {
            recordSendFailure();
            return;
        }
        sent.push_back(frame);
        clearLastError();
    }

    void enqueueRead(const CanFrame& frame) {
        received.push_back(frame);
    }

    void reset() {
        sent.clear();
        received.clear();
        initResult = true;
        sendResult = true;
        idleReadError = CanDriverError::NoMessage;
        resetDiagnostics();
    }
};
