#pragma once
/**
 * Unified touch interface for CYD boards
 * Supports:
 *   - XPT2046  (resistive, SPI)       — CYD 2.4" / 3.2" / 3.5"  — build flag: TOUCH_RESISTIVE
 *   - XPT2046  (resistive, VSPI bus)  — CYD 2.8"                 — build flag: TOUCH_XPT2046_VSPI
 *   - CST820   (capacitive, I2C)      — CYD 2.4"                 — build flag: TOUCH_CAPACITIVE
 *   - GT911    (capacitive, I2C)      — CYD 3.2"/3.5"            — build flag: TOUCH_GT911
 *
 * Calibration:
 *   Resistive variants store calibration data in NVS ("bitaxemon" namespace).
 *   Call touch.runCalibrationIfNeeded(prefs) after touch.begin().
 *   If no saved data is found, an interactive calibration screen is shown.
 *   To force recalibration, clear the keys via the /recalibrate web route.
 */

#include <Arduino.h>
#include <Preferences.h>

struct TouchPoint {
    int x;
    int y;
    bool pressed;
};

// ===== XPT2046 Resistive Touch (via TFT_eSPI built-in) =====
#ifdef TOUCH_RESISTIVE

#include <TFT_eSPI.h>

extern TFT_eSPI tft;

// Default calibration (landscape rotation 1, 320x240).
// Overwritten when saved calibration is loaded or fresh cal is run.
static uint16_t calData[5] = {642, 2916, 525, 2911, 0};

class TouchInterface {
public:
    void begin() {
        tft.setTouch(calData);
        Serial.println("TOUCH: XPT2046 resistive initialized");
    }

    // Load saved calibration from prefs, or run interactive calibration if none saved.
    // Returns true if interactive calibration was freshly run.
    bool runCalibrationIfNeeded(Preferences& prefs) {
        if (prefs.isKey("touchCal") && prefs.getBytesLength("touchCal") == sizeof(calData)) {
            prefs.getBytes("touchCal", calData, sizeof(calData));
            tft.setTouch(calData);
            Serial.println("TOUCH: Loaded saved calibration");
            return false;
        }
        _showIntro();
        // TFT_eSPI draws crosshairs at each corner and records raw values
        tft.calibrateTouch(calData, 0xFF31 /*cream*/, 0x08A7 /*dark blue*/, 15);
        prefs.putBytes("touchCal", calData, sizeof(calData));
        Serial.printf("TOUCH CAL: saved %u bytes\n", sizeof(calData));
        _showDone();
        return true;
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

private:
    void _showIntro() {
        tft.fillScreen(0x08A7);
        tft.setTextColor(0xFF31);
        tft.setTextSize(2);
        // "TOUCH CALIBRATION" = 18 chars × 12px = 216px
        tft.setCursor(SCR_W / 2 - 108, SCR_H / 2 - 30);
        tft.print("TOUCH CALIBRATION");
        tft.setTextColor(0xDD00);
        tft.setTextSize(1);
        // "TAP EACH CROSSHAIR IN TURN" = 26 chars × 6px = 156px
        tft.setCursor(SCR_W / 2 - 78, SCR_H / 2);
        tft.print("TAP EACH CROSSHAIR IN TURN");
        tft.setTextColor(0xA320);
        tft.setCursor(SCR_W / 2 - 72, SCR_H / 2 + 14);
        tft.print("(appears in each corner)");
        delay(2500);
        tft.fillScreen(0x08A7);
    }

    void _showDone() {
        tft.fillScreen(0x08A7);
        tft.setTextColor(0xFE40);
        tft.setTextSize(2);
        // "CAL COMPLETE!" = 13 chars × 12px = 156px
        tft.setCursor(SCR_W / 2 - 78, SCR_H / 2 - 10);
        tft.print("CAL COMPLETE!");
        tft.setTextColor(0xDD00);
        tft.setTextSize(1);
        tft.setCursor(SCR_W / 2 - 48, SCR_H / 2 + 12);
        tft.print("Calibration saved.");
        delay(1500);
        tft.fillScreen(0x08A7);
    }
};

#endif // TOUCH_RESISTIVE


// ===== XPT2046 Resistive Touch (separate VSPI bus — CYD 2.8") =====
#ifdef TOUCH_XPT2046_VSPI

#include <SPI.h>
#include <XPT2046_Touchscreen.h>
#include <TFT_eSPI.h>

extern TFT_eSPI tft;

static XPT2046_Touchscreen ts(TOUCH_CS, TOUCH_IRQ_PIN);

class TouchInterface {
    // Raw ADC calibration values — screen X maps from raw p.y, screen Y from raw p.x (inverted)
    int _xmin = 200, _xmax = 3900, _ymin = 200, _ymax = 3900;

public:
    void begin() {
        SPI.begin(TOUCH_SPI_CLK, TOUCH_SPI_MISO, TOUCH_SPI_MOSI, TOUCH_CS);
        ts.begin();
        Serial.println("TOUCH: XPT2046 (VSPI) resistive initialized");
    }

    // Load saved calibration from prefs, or run interactive 4-corner calibration.
    // Returns true if interactive calibration was freshly run.
    bool runCalibrationIfNeeded(Preferences& prefs) {
        if (prefs.isKey("touchXmin")) {
            _xmin = prefs.getInt("touchXmin", 200);
            _xmax = prefs.getInt("touchXmax", 3900);
            _ymin = prefs.getInt("touchYmin", 200);
            _ymax = prefs.getInt("touchYmax", 3900);
            Serial.printf("TOUCH: Loaded VSPI cal xmin=%d xmax=%d ymin=%d ymax=%d\n",
                          _xmin, _xmax, _ymin, _ymax);
            return false;
        }
        _runCalibrationUI(prefs);
        return true;
    }

    TouchPoint read() {
        TouchPoint tp = {0, 0, false};
        if (ts.touched()) {
            TS_Point p = ts.getPoint();
            // screen X comes from raw p.y; screen Y from raw p.x (inverted)
            tp.x = map(p.y, _ymin, _ymax, 0, SCR_W - 1);
            tp.y = map(p.x, _xmax, _xmin, 0, SCR_H - 1);
            tp.x = constrain(tp.x, 0, SCR_W - 1);
            tp.y = constrain(tp.y, 0, SCR_H - 1);
            tp.pressed = true;
        }
        return tp;
    }

private:
    static const uint16_t _BG     = 0x08A7; // dark blue
    static const uint16_t _FG     = 0xFF31; // cream white
    static const uint16_t _ACCENT = 0xFE40; // bright gold
    static const uint16_t _DIM    = 0xDD00; // medium gold
    static const uint16_t _DIMMER = 0xA320; // dim gold

    void _drawCrosshair(int x, int y, uint16_t color) {
        const int ARM = 14;
        tft.drawFastHLine(x - ARM, y, ARM * 2 + 1, color);
        tft.drawFastVLine(x, y - ARM, ARM * 2 + 1, color);
        tft.drawCircle(x, y, 6, color);
        tft.drawCircle(x, y, 3, color);
    }

    // Wait for a stable press, average samples over 400ms, then wait for release.
    TS_Point _waitForTap() {
        // Wait for initial contact
        while (!ts.touched()) delay(10);

        long sumX = 0, sumY = 0;
        int  count = 0;
        unsigned long t = millis();
        while (millis() - t < 400) {
            if (ts.touched()) {
                TS_Point p = ts.getPoint();
                sumX += p.x;
                sumY += p.y;
                count++;
            }
            delay(10);
        }
        while (ts.touched()) delay(10); // wait for release
        delay(150);                      // debounce

        TS_Point avg = {0, 0, 0};
        if (count > 0) {
            avg.x = sumX / count;
            avg.y = sumY / count;
        }
        return avg;
    }

    void _runCalibrationUI(Preferences& prefs) {
        const int OFF = 20; // crosshair offset from each edge

        // Corner targets: {screen X, screen Y, label, label offset hint}
        struct Target { int sx; int sy; const char* name; };
        Target targets[4] = {
            { OFF,           OFF,           "TOP LEFT"     },
            { SCR_W - OFF,   OFF,           "TOP RIGHT"    },
            { OFF,           SCR_H - OFF,   "BOTTOM LEFT"  },
            { SCR_W - OFF,   SCR_H - OFF,   "BOTTOM RIGHT" },
        };

        // Intro screen
        tft.fillScreen(_BG);
        tft.setTextColor(_FG);
        tft.setTextSize(2);
        tft.setCursor(SCR_W / 2 - 108, SCR_H / 2 - 36);
        tft.print("TOUCH CALIBRATION");
        tft.setTextColor(_DIM);
        tft.setTextSize(1);
        tft.setCursor(SCR_W / 2 - 78, SCR_H / 2 - 2);
        tft.print("TAP EACH CROSSHAIR IN TURN");
        tft.setTextColor(_DIMMER);
        tft.setCursor(SCR_W / 2 - 54, SCR_H / 2 + 14);
        tft.print("4 corners, one at a time");
        delay(2500);

        TS_Point samples[4];

        for (int i = 0; i < 4; i++) {
            tft.fillScreen(_BG);

            // Step counter
            tft.setTextColor(_DIMMER);
            tft.setTextSize(1);
            char step[12];
            snprintf(step, sizeof(step), "STEP %d / 4", i + 1);
            tft.setCursor(SCR_W / 2 - strlen(step) * 3, 4);
            tft.print(step);

            // Target name centred on screen
            tft.setTextColor(_FG);
            tft.setTextSize(2);
            int nameW = strlen(targets[i].name) * 12;
            tft.setCursor(SCR_W / 2 - nameW / 2, SCR_H / 2 - 14);
            tft.print(targets[i].name);

            // "TAP +" sub-text
            tft.setTextColor(_DIM);
            tft.setTextSize(1);
            tft.setCursor(SCR_W / 2 - 12, SCR_H / 2 + 6);
            tft.print("TAP +");

            // Crosshair at the corner
            _drawCrosshair(targets[i].sx, targets[i].sy, _ACCENT);

            // Collect the tap
            samples[i] = _waitForTap();

            // Flash crosshair white to confirm receipt
            _drawCrosshair(targets[i].sx, targets[i].sy, _FG);
            delay(250);
        }

        // Derive calibration from the 4 corner samples.
        // Axis mapping: screen X ← raw p.y, screen Y ← raw p.x (inverted)
        //   Left  side (TL=0, BL=2): p.y is at its minimum  → YMIN
        //   Right side (TR=1, BR=3): p.y is at its maximum  → YMAX
        //   Top   side (TL=0, TR=1): p.x is at its maximum  → XMAX (inverted)
        //   Bottom side (BL=2,BR=3): p.x is at its minimum  → XMIN
        _ymin = (samples[0].y + samples[2].y) / 2;
        _ymax = (samples[1].y + samples[3].y) / 2;
        _xmax = (samples[0].x + samples[1].x) / 2;
        _xmin = (samples[2].x + samples[3].x) / 2;

        // Guard against degenerate results — fall back to defaults
        if (_ymax - _ymin < 500 || _xmax - _xmin < 500) {
            Serial.println("TOUCH CAL: suspicious values, keeping defaults");
            _xmin = 200; _xmax = 3900; _ymin = 200; _ymax = 3900;
        }

        prefs.putInt("touchXmin", _xmin);
        prefs.putInt("touchXmax", _xmax);
        prefs.putInt("touchYmin", _ymin);
        prefs.putInt("touchYmax", _ymax);
        Serial.printf("TOUCH CAL: xmin=%d xmax=%d ymin=%d ymax=%d\n",
                      _xmin, _xmax, _ymin, _ymax);

        // Done screen
        tft.fillScreen(_BG);
        tft.setTextColor(_ACCENT);
        tft.setTextSize(2);
        tft.setCursor(SCR_W / 2 - 78, SCR_H / 2 - 10);
        tft.print("CAL COMPLETE!");
        tft.setTextColor(_DIM);
        tft.setTextSize(1);
        tft.setCursor(SCR_W / 2 - 48, SCR_H / 2 + 12);
        tft.print("Calibration saved.");
        delay(1500);
        tft.fillScreen(_BG);
    }
};

#endif // TOUCH_XPT2046_VSPI


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

    // Capacitive touch reports absolute coordinates — no calibration needed.
    bool runCalibrationIfNeeded(Preferences& /*prefs*/) { return false; }

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

static TAMC_GT911 gt911(TOUCH_SDA_PIN, TOUCH_SCL_PIN, TOUCH_INT_PIN, TOUCH_RST_PIN, SCR_W, SCR_H);

class TouchInterface {
public:
    void begin() {
        Wire.begin(TOUCH_SDA_PIN, TOUCH_SCL_PIN);
        delay(100);
        gt911.begin();
        gt911.setRotation(ROTATION_LEFT);
        Serial.println("TOUCH: GT911 capacitive initialized");
    }

    // Capacitive touch reports absolute coordinates — no calibration needed.
    bool runCalibrationIfNeeded(Preferences& /*prefs*/) { return false; }

    TouchPoint read() {
        TouchPoint tp = {0, 0, false};
        gt911.read();
        if (gt911.isTouched) {
            tp.x = gt911.points[0].x;
            tp.y = gt911.points[0].y;
#ifdef TOUCH_INVERT_X
            tp.x = (SCR_W - 1) - tp.x;
#endif
#ifdef TOUCH_INVERT_Y
            tp.y = (SCR_H - 1) - tp.y;
#endif
            tp.x = constrain(tp.x, 0, SCR_W - 1);
            tp.y = constrain(tp.y, 0, SCR_H - 1);
            tp.pressed = true;
        }
        return tp;
    }
};

#endif // TOUCH_GT911
