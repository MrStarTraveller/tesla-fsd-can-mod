#pragma once

#include <cstdint>
#include "../can_frame_types.h"

enum class CanDriverError : uint8_t {
    None = 0,
    NoMessage,
    InitFailed,
    FilterConfigFailed,
    InterruptConfigFailed,
    ReadFailed,
    SendFailed,
    BusOff,
};

struct CanDriverDiagnostics {
    CanDriverError lastError = CanDriverError::None;
    uint32_t initFailures = 0;
    uint32_t filterFailures = 0;
    uint32_t interruptFailures = 0;
    uint32_t readFailures = 0;
    uint32_t noMessageReads = 0;
    uint32_t sendFailures = 0;
    uint32_t recoveries = 0;
};

struct CanDriver {
    virtual bool init() = 0;
    virtual void setFilters(const uint32_t* ids, uint8_t count) = 0;
    virtual bool enableInterrupt(void (*onReady)()) = 0;
    virtual bool read(CanFrame& frame) = 0;
    virtual void send(const CanFrame& frame) = 0;
    const CanDriverDiagnostics& diagnostics() const { return diagnostics_; }
    virtual ~CanDriver() = default;

protected:
    void recordInitFailure() {
        diagnostics_.initFailures++;
        diagnostics_.lastError = CanDriverError::InitFailed;
    }

    void recordFilterFailure() {
        diagnostics_.filterFailures++;
        diagnostics_.lastError = CanDriverError::FilterConfigFailed;
    }

    void recordInterruptFailure() {
        diagnostics_.interruptFailures++;
        diagnostics_.lastError = CanDriverError::InterruptConfigFailed;
    }

    void recordNoMessageRead() {
        diagnostics_.noMessageReads++;
        diagnostics_.lastError = CanDriverError::NoMessage;
    }

    void recordReadFailure(CanDriverError error = CanDriverError::ReadFailed) {
        diagnostics_.readFailures++;
        diagnostics_.lastError = error;
    }

    void recordSendFailure() {
        diagnostics_.sendFailures++;
        diagnostics_.lastError = CanDriverError::SendFailed;
    }

    void recordRecovery() {
        diagnostics_.recoveries++;
    }

    void setLastError(CanDriverError error) {
        diagnostics_.lastError = error;
    }

    void clearLastError() {
        diagnostics_.lastError = CanDriverError::None;
    }

    void resetDiagnostics() {
        diagnostics_ = {};
    }

private:
    CanDriverDiagnostics diagnostics_;
};
