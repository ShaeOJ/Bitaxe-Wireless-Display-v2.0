#pragma once
/**
 * Unified touch interface for CYD boards
 * Supports:
 *   - XPT2046  (resistive, SPI)  — CYD 2.4" — build flag: TOUCH_RESISTIVE
 *   - CST820   (capacitive, I2C) — CYD 2.4" — build flag: TOUCH_CAPACITIVE
 *   - GT911    (capacitive, I2C) — CYD 3.5" — build flag: TOUCH_GT911
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

extern TFT_eSPI tft;

// Calibration data for landscape rotation 1 (320x240)
static uint16_t calData[5] = {642, 2916, 525, 2911, 0};

class TouchInterface {
public:
    void begin() {
        tft.setTouch(calData);
        Serial.println("TOUCH: XPT2046 resistive initialized");
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
        Serial.println("TOUCH: CST820 capacitive initialized");
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

// ===== GT911 Capacitive Touch (via TAMC_GT911) =====
#ifdef TOUCH_GT911

#include <Wire.h>
#include <TAMC_GT911.h>

static TAMC_GT911 gt911(TOUCH_SDA_PIN, TOUCH_SCL_PIN, TOUCH_INT_PIN, TOUCH_RST_PIN, 480, 320);

class TouchInterface {
public:
    void begin() {
        Wire.begin(TOUCH_SDA_PIN, TOUCH_SCL_PIN);
        delay(100);
        gt911.begin();
        gt911.setRotation(ROTATION_LEFT);
        Serial.println("TOUCH: GT911 capacitive initialized");
    }

    TouchPoint read() {
        TouchPoint tp = {0, 0, false};
        gt911.read();
        if (gt911.isTouched) {
            tp.x = gt911.points[0].x;
            tp.y = gt911.points[0].y;
            tp.x = constrain(tp.x, 0, SCR_W - 1);
            tp.y = constrain(tp.y, 0, SCR_H - 1);
            tp.pressed = true;
        }
        return tp;
    }
};

#endif // TOUCH_GT911
