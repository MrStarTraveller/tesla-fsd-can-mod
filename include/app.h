#pragma once

#include <memory>
#include "can_frame_types.h"
#include "drivers/can_driver.h"
#include "can_helpers.h"
#include "handlers.h"

#ifndef NATIVE_BUILD
#include <Arduino.h>
#endif

#if !defined(NATIVE_BUILD)
#if defined(PIN_LED)
constexpr int kStatusLedPin = PIN_LED;
#elif defined(LED_BUILTIN)
constexpr int kStatusLedPin = LED_BUILTIN;
#else
constexpr int kStatusLedPin = -1;
#endif
#endif

namespace app_detail {
enum class AppRuntimeState : uint8_t {
    Starting = 0,
    Idle,
    Processing,
    InitFault,
    RuntimeFault,
};

inline bool hasDriverFaultDelta(const CanDriverDiagnostics& previous, const CanDriverDiagnostics& current) {
    return current.initFailures > previous.initFailures ||
           current.filterFailures > previous.filterFailures ||
           current.interruptFailures > previous.interruptFailures ||
           current.readFailures > previous.readFailures ||
           current.sendFailures > previous.sendFailures ||
           current.recoveries > previous.recoveries;
}

inline AppRuntimeState nextRuntimeState(
    bool ready,
    bool initFault,
    const CanDriverDiagnostics& previous,
    const CanDriverDiagnostics& current,
    bool processedFrame
) {
    if (!ready) {
        return initFault ? AppRuntimeState::InitFault : AppRuntimeState::Starting;
    }
    if (hasDriverFaultDelta(previous, current)) {
        return AppRuntimeState::RuntimeFault;
    }
    if (processedFrame) {
        return AppRuntimeState::Processing;
    }
    return AppRuntimeState::Idle;
}
}

#if defined(HW4)
using SelectedHandler = HW4Handler;
#elif defined(HW3)
using SelectedHandler = HW3Handler;
#elif defined(LEGACY)
using SelectedHandler = LegacyHandler;
#else
#error "Define HW4, HW3 or LEGACY in build_flags"
#endif

static std::unique_ptr<CanDriver> appDriver;
static std::unique_ptr<CarManagerBase> appHandler;
static bool appReady = false;
static bool appInitFault = false;
static app_detail::AppRuntimeState appState = app_detail::AppRuntimeState::Starting;
static CanDriverDiagnostics appLastDiagnostics = {};

static volatile bool frameReady = true;
static void canISR() { frameReady = true; }

#ifndef NATIVE_BUILD
static void writeStatusLed(bool active) {
    if (kStatusLedPin >= 0) {
        digitalWrite(kStatusLedPin, active ? LOW : HIGH);
    }
}

static void signalBlinkFault(unsigned long intervalMs) {
    static unsigned long lastToggleMs = 0;
    static bool ledActive = false;
    const unsigned long now = millis();
    if (now - lastToggleMs >= intervalMs) {
        ledActive = !ledActive;
        writeStatusLed(ledActive);
        lastToggleMs = now;
    }
}

static void renderRuntimeState() {
    switch (appState) {
        case app_detail::AppRuntimeState::Processing:
            writeStatusLed(true);
            break;
        case app_detail::AppRuntimeState::InitFault:
            signalBlinkFault(250);
            break;
        case app_detail::AppRuntimeState::RuntimeFault:
            signalBlinkFault(500);
            break;
        case app_detail::AppRuntimeState::Starting:
        case app_detail::AppRuntimeState::Idle:
        default:
            writeStatusLed(false);
            break;
    }
}

static void reportDriverDiagnosticsDelta(
    const CanDriverDiagnostics& previous,
    const CanDriverDiagnostics& current,
    bool enablePrint
) {
    if (!enablePrint || !app_detail::hasDriverFaultDelta(previous, current)) {
        return;
    }

    Serial.printf(
        "Driver diagnostics: last=%u init=%lu filter=%lu irq=%lu read=%lu nomsg=%lu send=%lu recoveries=%lu\n",
        static_cast<unsigned>(current.lastError),
        static_cast<unsigned long>(current.initFailures),
        static_cast<unsigned long>(current.filterFailures),
        static_cast<unsigned long>(current.interruptFailures),
        static_cast<unsigned long>(current.readFailures),
        static_cast<unsigned long>(current.noMessageReads),
        static_cast<unsigned long>(current.sendFailures),
        static_cast<unsigned long>(current.recoveries)
    );
}
#endif

template<typename Driver>
static void appSetup(std::unique_ptr<Driver> drv, const char* readyMsg) {
#ifndef NATIVE_BUILD
    if (kStatusLedPin >= 0) {
        pinMode(kStatusLedPin, OUTPUT);
        writeStatusLed(false);
    }
#endif

    appHandler = std::make_unique<SelectedHandler>();

#ifndef NATIVE_BUILD
    delay(1500);
    Serial.begin(115200);
    unsigned long t0 = millis();
    while (!Serial && millis() - t0 < 1000) {}
#endif

    appDriver = std::move(drv);
    appReady = appDriver->init();
    appInitFault = !appReady;
    appLastDiagnostics = appDriver->diagnostics();
    appState = app_detail::nextRuntimeState(appReady, appInitFault, {}, appLastDiagnostics, false);

    if (!appReady) {
#ifndef NATIVE_BUILD
        Serial.println("CAN init failed");
        renderRuntimeState();
#endif
        return;
    }

    appDriver->setFilters(appHandler->filterIds(), appHandler->filterIdCount());
    if constexpr (Driver::kSupportsISR) {
        appDriver->enableInterrupt(canISR);
    }

    appLastDiagnostics = appDriver->diagnostics();
    appState = app_detail::nextRuntimeState(appReady, appInitFault, {}, appLastDiagnostics, false);

#ifndef NATIVE_BUILD
    Serial.println(readyMsg);
    reportDriverDiagnosticsDelta({}, appLastDiagnostics, appHandler->enablePrint);
    renderRuntimeState();
#endif
}

template<typename Driver>
static void appLoop() {
    if (!appReady) {
        appState = app_detail::nextRuntimeState(appReady, appInitFault, appLastDiagnostics, appLastDiagnostics, false);
#ifndef NATIVE_BUILD
        renderRuntimeState();
#endif
        return;
    }

    if constexpr (Driver::kSupportsISR) {
        if (!frameReady) {
            return;
        }
        frameReady = false;
    }

    CanFrame frame;
    bool processedFrame = false;
    while (appDriver->read(frame)) {
        processedFrame = true;
        appState = app_detail::AppRuntimeState::Processing;
#ifndef NATIVE_BUILD
        renderRuntimeState();
#endif
        appHandler->handleMessage(frame, *appDriver);
    }

    const CanDriverDiagnostics currentDiagnostics = appDriver->diagnostics();
    appState = app_detail::nextRuntimeState(appReady, appInitFault, appLastDiagnostics, currentDiagnostics, processedFrame);
#ifndef NATIVE_BUILD
    reportDriverDiagnosticsDelta(appLastDiagnostics, currentDiagnostics, appHandler->enablePrint);
    renderRuntimeState();
#endif
    appLastDiagnostics = currentDiagnostics;
}
