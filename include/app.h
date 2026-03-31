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

static volatile bool frameReady = true;
static void canISR() { frameReady = true; }

#ifndef NATIVE_BUILD
static void signalInitFault() {
    static unsigned long lastToggleMs = 0;
    static bool ledOn = false;
    const unsigned long now = millis();
    if (now - lastToggleMs >= 250) {
        ledOn = !ledOn;
        if (kStatusLedPin >= 0) {
            digitalWrite(kStatusLedPin, ledOn ? LOW : HIGH);
        }
        lastToggleMs = now;
    }
}
#endif

template<typename Driver>
static void appSetup(std::unique_ptr<Driver> drv, const char* readyMsg) {
    if (kStatusLedPin >= 0) {
        pinMode(kStatusLedPin, OUTPUT);
        digitalWrite(kStatusLedPin, HIGH);
    }

    appHandler = std::make_unique<SelectedHandler>();
    delay(1500);
    Serial.begin(115200);
    unsigned long t0 = millis();
    while (!Serial && millis() - t0 < 1000) {}

    appDriver = std::move(drv);
    appReady = appDriver->init();
    appInitFault = !appReady;
    if (!appReady) {
        Serial.println("CAN init failed");
        return;
    }

    appDriver->setFilters(appHandler->filterIds(), appHandler->filterIdCount());
    if constexpr (Driver::kSupportsISR) {
        appDriver->enableInterrupt(canISR);
    }

    Serial.println(readyMsg);
}

template<typename Driver>
static void appLoop() {
    if (!appReady) {
#ifndef NATIVE_BUILD
        if (appInitFault) {
            signalInitFault();
        }
#endif
        return;
    }

    if constexpr (Driver::kSupportsISR) {
        if (!frameReady) return;
        frameReady = false;
    }

    CanFrame frame;
    while (appDriver->read(frame)) {
        if (kStatusLedPin >= 0) {
            digitalWrite(kStatusLedPin, LOW);
        }
        appHandler->handleMessage(frame, *appDriver);
    }
    if (kStatusLedPin >= 0) {
        digitalWrite(kStatusLedPin, HIGH);
    }
}
