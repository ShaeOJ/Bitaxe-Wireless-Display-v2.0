#pragma once
/**
 * Unified touch interface for ESP32-2432S024
 * Supports both XPT2046 (resistive, SPI) and CST820 (capacitive, I2C)
 * Selected via build flags: TOUCH_RESISTIVE or TOUCH_CAPACITIVE
 */

#include <Arduino.h>

struct TouchPoint {
    int x;
    int y;
    bool pressed;
};

// ===== XPT2046 Resistive Touch (via TFT_eSPI built-in) =====
#ifdef TOUCH_RESISTIVE

#include <TFT_eSPI.h>

// TFT_eSPI handles XPT2046 on the shared HSPI bus internally
// when TOUCH_CS is defined in build_flags.
// We just need a reference to the tft object from main.cpp.
extern TFT_eSPI tft;

// Calibration data for landscape rotation 1 (320x240)
// Obtained from TFT_eSPI calibrateTouch() on real hardware
static uint16_t calData[5] = {642, 2916, 525, 2911, 0};

class TouchInterface {
public:
    void begin() {
        tft.setTouch(calData);
        Serial.println("TOUCH: TFT_eSPI built-in XPT2046 initialized");
        Serial.printf("TOUCH: TOUCH_CS=%d\n", TOUCH_CS);
        // Quick raw read test
        uint16_t tx, ty;
        bool hit = tft.getTouch(&tx, &ty, 20);
        Serial.printf("TOUCH: initial probe hit=%d x=%d y=%d\n", hit, tx, ty);
    }

    TouchPoint read() {
        TouchPoint tp = {0, 0, false};
        uint16_t tx, ty;
        if (tft.getTouch(&tx, &ty, 15)) {
            tp.x = tx;
            tp.y = ty;
            tp.pressed = true;
        }
        return tp;
    }
};

#endif // TOUCH_RESISTIVE

// ===== CST820 Capacitive Touch (via bb_captouch) =====
#ifdef TOUCH_CAPACITIVE

#include <bb_captouch.h>

static BBCapTouch bbct;
static TOUCHINFO ti;

class TouchInterface {
public:
    void begin() {
        bbct.init(TOUCH_SDA_PIN, TOUCH_SCL_PIN, TOUCH_RST_PIN, TOUCH_INT_PIN);
    }

    TouchPoint read() {
        TouchPoint tp = {0, 0, false};
        if (bbct.getSamples(&ti)) {
            if (ti.count > 0) {
                // CST820 reports in native portrait; remap for landscape
                int rawX = ti.x[0];
                int rawY = ti.y[0];
                tp.x = rawY;
                tp.y = 239 - rawX;
                tp.x = constrain(tp.x, 0, 319);
                tp.y = constrain(tp.y, 0, 239);
                tp.pressed = true;
            }
        }
        return tp;
    }
};

#endif // TOUCH_CAPACITIVE
