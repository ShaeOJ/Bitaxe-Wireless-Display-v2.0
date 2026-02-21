/**
 * BitAxe Monitor - Steampunk Edition
 * Generic multi-BitAxe device monitor
 * Hardware: ESP32-2432S024 (2.4" 320x240 ILI9341)
 *
 * Monitors up to 8 individual BitAxe devices via their REST API.
 * Each device gets its own stats screen with controls.
 */

#include <SPI.h>
#include <Wire.h>
#include <TFT_eSPI.h>
#include "touch_interface.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <WiFiManager.h>

// Screen dimensions (landscape)
#define SCR_W 320
#define SCR_H 240

// Display
TFT_eSPI tft = TFT_eSPI();

// Touch
TouchInterface touch;

// === STEAMPUNK AMBER/GOLD PALETTE ===
#define CRT_BG        0x0000   // Pure black
#define CRT_SCANLINE  0x1040   // Very dim amber scanlines
#define CRT_GLOW      0x2080   // Amber glow underlay
#define CRT_DIM       0x4940   // Dim bronze labels/borders
#define CRT_MID       0x8240   // Medium amber secondary text
#define CRT_BRIGHT    0xCBC0   // Bright amber primary accent
#define CRT_WHITE     0xFEE5   // Bright gold for values
#define CRT_RED       0xF800   // Red (alerts)
#define CRT_YELLOW    0xFFE0   // Yellow (warnings)
#define CRT_ORANGE    0xFD20   // Orange (Bitcoin accent)
#define CRT_RED_DARK  0x8000   // Dark red (danger buttons)
#define PANEL_FILL    0x1860   // Dark brown panel fill
#define PANEL_BORDER  0x69E0   // Bronze panel border

// Button styles
enum ButtonStyle { BTN_PRIMARY, BTN_DANGER, BTN_GHOST };

// Touch state
bool touchPressed = false;
unsigned long touchStartTime = 0;
int touchStartX = 0;
int touchStartY = 0;
int currentScreen = 0;
const int SWIPE_THRESHOLD = 35;

// BTC price update
unsigned long lastBtcUpdate = 0;
const unsigned long BTC_UPDATE_INTERVAL = 60000;

// RGB LED pins (active LOW on CYD)
#define LED_RED   4
#define LED_GREEN 16
#define LED_BLUE  17

unsigned long lastLedToggle = 0;
bool ledState = false;
const unsigned long LED_BLINK_INTERVAL = 1000;

// Share flash tracking
int lastTotalShares = 0;
unsigned long shareFlashTime = 0;
const unsigned long SHARE_FLASH_DURATION = 200;

// Data storage
Preferences prefs;

// Update interval
unsigned long lastUpdate = 0;
const unsigned long UPDATE_INTERVAL = 5000;
int deviceFetchIndex = 0;

// === DEVICE DATA ===
struct DeviceInfo {
    bool valid = false;
    char ip[40] = "";
    String hostname = "";
    String deviceModel = "";
    String asicModel = "";
    float hashRate = 0;
    float hashRate_1h = 0;
    float temperature = 0;
    float vrTemp = 0;
    float power = 0;
    float voltage = 0;
    int coreVoltage = 1200;
    int frequency = 0;
    int fanSpeed = 0;
    int fanRpm = 0;
    int sharesAccepted = 0;
    int sharesRejected = 0;
    double bestDiff = 0;
    double bestSessionDiff = 0;
    String stratumURL = "";
    int stratumPort = 0;
    String stratumUser = "";
    int uptimeSeconds = 0;
    int wifiRSSI = 0;
};

DeviceInfo devices[8];
int deviceCount = 0;

// Pool/price info (global, not per-device)
struct PoolInfo {
    bool valid = false;
    float btcPrice = 0;
    float priceChange24h = 0;
    double networkDifficulty = 0;
};
PoolInfo pool;

// Coin definitions
struct CoinInfo {
    const char* apiId;
    const char* ticker;
    const char* name;
    uint16_t color;
};

const CoinInfo coins[] = {
    {"bitcoin",      "BTC",  "Bitcoin",      CRT_ORANGE},
    {"bitcoin-cash", "BCH",  "Bitcoin Cash", CRT_BRIGHT},
    {"digibyte",     "DGB",  "DigiByte",     CRT_MID},
    {"bitcoin-2",    "BTC2", "Bitcoin II",   CRT_YELLOW},
    {"ecash",        "XEC",  "eCash",        CRT_MID}
};
const int COIN_COUNT = 5;
int selectedCoin = 0;
float electricityRate = 0.12;

// Consecutive API failure tracking per device
int deviceFailCount[8] = {0};
const int MAX_FAIL_BEFORE_INVALID = 3;

// Double-tap confirmation
unsigned long lastRestartTap = 0;
int restartTapDevice = -1;
bool restartConfirmPending = false;

// POST feedback
unsigned long postFeedbackTime = 0;
bool postFeedbackSuccess = false;
String postFeedbackMsg = "";

// Button area struct
struct ButtonArea {
    int x, y, w, h;
};
bool checkButtonPress(ButtonArea &btn, int x, int y);

// Forward declarations
void drawBootScreen();
void drawSetupScreen();
void drawFailedScreen();
void drawMainUI();
void handleTouch();
void updateLed(unsigned long now);
void fetchBtcPrice();
void fetchNetworkDifficulty();
void fetchDeviceData(int index);
void updateDisplay();
void updatePoolScreen();
void updateDeviceScreen(int devIndex);
void drawPoolScreen();
void drawDeviceScreen(int devIndex);
void redrawCurrentScreen();
void drawScanlines();
void drawTouchRipple(int x, int y);
void scanlineWipeTransition();
void parseDeviceIPs(const char* ipList);
bool postDeviceSetting(int deviceIndex, const char* jsonBody);
bool postDeviceRestart(int deviceIndex);

// UI helpers
void drawScreenFrame(const char* title);
void drawPanel(int x, int y, int w, int h, const char* title = nullptr);
void drawArcGauge(int cx, int cy, int R, int r, float value, float minVal, float maxVal,
                   const char* label, const char* valueStr, const char* unit, uint16_t color);
void drawHBar(int x, int y, int w, int h, float value, float maxVal, uint16_t color,
              const char* valueStr);
void drawStatusDot(int x, int y, int status);
void drawGlowText(int x, int y, const char* text, int size, uint16_t color);
void drawNavBar(int screenIndex, int totalScreens);
void drawCoinIcon24(int x, int y, uint16_t color, const char* ticker);
void formatPrice(char* buf, int bufSize, float price);
void drawButton(ButtonArea &btn, const char* label, ButtonStyle style);
int getTotalScreens();
uint16_t tempColor(float temp);

// Button areas - forward declarations
extern ButtonArea btnNextCoin;
extern ButtonArea btnRateMinus;
extern ButtonArea btnRatePlus;
extern ButtonArea btnDevRestart;
extern ButtonArea btnDevFreqPlus;
extern ButtonArea btnDevFreqMinus;
extern ButtonArea btnDevVoltPlus;
extern ButtonArea btnDevVoltMinus;
extern ButtonArea btnDevFanPlus;

// ===== ICONS 24x24 =====

void drawIconPickaxe(int x, int y, uint16_t color) {
    for (int i = 0; i < 3; i++) {
        tft.drawLine(x+3+i, y+21, x+21+i, y+3, color);
    }
    for (int i = 0; i < 3; i++) {
        tft.drawLine(x+15, y+3+i, x+21, y+9+i, color);
    }
}

void drawIconBolt(int x, int y, uint16_t color) {
    tft.fillTriangle(x+12, y+2, x+4, y+12, x+13, y+12, color);
    tft.fillTriangle(x+10, y+11, x+18, y+11, x+8, y+22, color);
}

void drawIconGauge(int x, int y, uint16_t color) {
    tft.drawArc(x+12, y+14, 10, 8, 180, 360, color, CRT_BG);
    tft.drawArc(x+12, y+14, 11, 9, 180, 360, color, CRT_BG);
    for (int i = 0; i < 2; i++) {
        tft.drawLine(x+12, y+14, x+18+i, y+6, color);
    }
    tft.fillCircle(x+12, y+14, 3, color);
}

void drawIconWorker(int x, int y, uint16_t color) {
    tft.fillCircle(x+12, y+5, 5, color);
    tft.fillRect(x+8, y+11, 8, 8, color);
    tft.fillRect(x+4, y+12, 4, 6, color);
    tft.fillRect(x+16, y+12, 4, 6, color);
    tft.fillRect(x+8, y+19, 3, 5, color);
    tft.fillRect(x+13, y+19, 3, 5, color);
}

void drawIconCheck(int x, int y, uint16_t color) {
    for (int i = 0; i < 4; i++) {
        tft.drawLine(x+3, y+12+i, x+9, y+18+i, color);
        tft.drawLine(x+9, y+18+i, x+21, y+6+i, color);
    }
}

void drawIconX(int x, int y, uint16_t color) {
    for (int i = 0; i < 4; i++) {
        tft.drawLine(x+4+i, y+4, x+20+i, y+20, color);
        tft.drawLine(x+20-i, y+4, x+4-i, y+20, color);
    }
}

void drawIconBitcoin(int x, int y, uint16_t color) {
    tft.fillRoundRect(x+6, y+2, 12, 20, 2, color);
    tft.fillRect(x+8, y+4, 8, 16, CRT_BG);
    tft.fillRect(x+8, y+4, 4, 16, color);
    tft.fillCircle(x+14, y+8, 3, color);
    tft.fillCircle(x+14, y+16, 4, color);
    tft.fillRect(x+9, y, 2, 4, color);
    tft.fillRect(x+13, y, 2, 4, color);
    tft.fillRect(x+9, y+20, 2, 4, color);
    tft.fillRect(x+13, y+20, 2, 4, color);
}

void drawIconThermo(int x, int y, uint16_t color) {
    tft.drawRect(x+9, y+2, 6, 14, color);
    tft.drawRect(x+10, y+3, 4, 12, color);
    tft.fillCircle(x+12, y+19, 5, color);
    tft.fillRect(x+10, y+10, 4, 7, color);
}

void drawIconFan(int x, int y, uint16_t color) {
    tft.fillCircle(x+12, y+12, 3, color);
    tft.fillTriangle(x+12, y+9, x+6, y+2, x+12, y+2, color);
    tft.fillTriangle(x+15, y+12, x+22, y+8, x+22, y+16, color);
    tft.fillTriangle(x+12, y+15, x+18, y+22, x+12, y+22, color);
    tft.fillTriangle(x+9, y+12, x+2, y+8, x+2, y+16, color);
}

void drawIconChip(int x, int y, uint16_t color) {
    tft.drawRect(x+6, y+6, 12, 12, color);
    tft.drawRect(x+7, y+7, 10, 10, color);
    for (int i = 0; i < 3; i++) {
        tft.fillRect(x+8+i*3, y+2, 2, 4, color);
        tft.fillRect(x+8+i*3, y+18, 2, 4, color);
        tft.fillRect(x+2, y+8+i*3, 4, 2, color);
        tft.fillRect(x+18, y+8+i*3, 4, 2, color);
    }
}

void drawIconNetwork(int x, int y, uint16_t color) {
    tft.fillCircle(x+12, y+18, 4, color);
    tft.fillCircle(x+4, y+6, 3, color);
    tft.fillCircle(x+20, y+6, 3, color);
    for (int i = 0; i < 2; i++) {
        tft.drawLine(x+12, y+14+i, x+6, y+8+i, color);
        tft.drawLine(x+12, y+14+i, x+18, y+8+i, color);
    }
}

void drawIconVoltage(int x, int y, uint16_t color) {
    tft.drawCircle(x+12, y+12, 8, color);
    tft.drawCircle(x+12, y+12, 7, color);
    tft.drawLine(x+8, y+8, x+16, y+16, color);
    tft.drawLine(x+9, y+8, x+17, y+16, color);
}

void drawCoinIcon24(int x, int y, uint16_t color, const char* ticker) {
    tft.drawSmoothCircle(x+12, y+12, 11, color, CRT_BG);
    tft.drawSmoothCircle(x+12, y+12, 10, color, CRT_BG);
    tft.setTextColor(color);
    tft.setTextSize(1);
    tft.setCursor(x + 8, y + 8);
    tft.print(ticker[0]);
}

// ===== UI HELPER FUNCTIONS =====

void drawScanlines() {
    for (int y = 0; y < SCR_H; y += 3) {
        tft.drawFastHLine(0, y, SCR_W, CRT_SCANLINE);
    }
}

void drawGlowText(int x, int y, const char* text, int size, uint16_t color) {
    tft.setTextSize(size);
    tft.setTextColor(CRT_GLOW);
    tft.setCursor(x + 1, y + 1);
    tft.print(text);
    tft.setTextColor(color);
    tft.setCursor(x, y);
    tft.print(text);
}

void drawScreenFrame(const char* title) {
    tft.fillScreen(CRT_BG);
    drawScanlines();
    tft.drawRect(2, 2, 316, 236, CRT_DIM);
    tft.drawRect(3, 3, 314, 234, CRT_BRIGHT);
    tft.fillRect(6, 6, 308, 22, PANEL_FILL);
    tft.drawFastHLine(6, 28, 308, CRT_BRIGHT);
    tft.drawFastHLine(6, 29, 308, CRT_DIM);

    // "BitAxe" logo in ornate serif (left)
    tft.setFreeFont(&FreeSerifBoldItalic12pt7b);
    tft.setTextColor(CRT_WHITE);
    tft.setTextSize(1);
    tft.setCursor(10, 23);  // GFX fonts use baseline for y
    tft.print("BitAxe");
    tft.setFreeFont(NULL);  // Back to GLCD

    // Screen title (right-aligned)
    tft.setTextColor(CRT_BRIGHT);
    tft.setTextSize(1);
    int titleW = strlen(title) * 6;
    tft.setCursor(310 - titleW, 12);
    tft.print(title);
}

void drawPanel(int x, int y, int w, int h, const char* title) {
    tft.fillRoundRect(x, y, w, h, 4, PANEL_FILL);
    tft.drawRoundRect(x, y, w, h, 4, PANEL_BORDER);
    if (title) {
        tft.setTextColor(CRT_DIM);
        tft.setTextSize(1);
        tft.setCursor(x + 4, y + 3);
        tft.print(title);
        tft.drawFastHLine(x + 3, y + 12, w - 6, CRT_DIM);
    }
}

void drawArcGauge(int cx, int cy, int R, int r, float value, float minVal, float maxVal,
                   const char* label, const char* valueStr, const char* unit, uint16_t color) {
    tft.drawSmoothArc(cx, cy, R, r, 240, 120, CRT_DIM, CRT_BG, false);
    float pct = 0;
    if (maxVal > minVal) pct = constrain((value - minVal) / (maxVal - minVal), 0.0f, 1.0f);
    int sweep = (int)(240.0f * pct);
    if (sweep > 1) {
        int endAngle = (240 + sweep) % 360;
        tft.drawSmoothArc(cx, cy, R, r, 240, endAngle, color, CRT_BG, false);
    }
    int valW = strlen(valueStr) * 6;
    tft.setTextColor(CRT_WHITE);
    tft.setTextSize(1);
    tft.setCursor(cx - valW / 2, cy - 6);
    tft.print(valueStr);
    int unitW = strlen(unit) * 6;
    tft.setTextColor(CRT_MID);
    tft.setTextSize(1);
    tft.setCursor(cx - unitW / 2, cy + 4);
    tft.print(unit);
    int labelW = strlen(label) * 6;
    tft.setTextColor(CRT_DIM);
    tft.setTextSize(1);
    tft.setCursor(cx - labelW / 2, cy + R + 4);
    tft.print(label);
}

void drawHBar(int x, int y, int w, int h, float value, float maxVal, uint16_t color,
              const char* valueStr) {
    tft.drawRoundRect(x, y, w, h, 3, CRT_DIM);
    float pct = (maxVal > 0) ? constrain(value / maxVal, 0.0f, 1.0f) : 0;
    int fillW = (int)((w - 4) * pct);
    if (fillW > 0) {
        tft.fillRoundRect(x + 2, y + 2, fillW, h - 4, 2, color);
    }
    if (valueStr) {
        int textW = strlen(valueStr) * 6;
        tft.setTextColor(CRT_WHITE);
        tft.setTextSize(1);
        tft.setCursor(x + w / 2 - textW / 2, y + (h - 8) / 2);
        tft.print(valueStr);
    }
}

void drawStatusDot(int x, int y, int status) {
    uint16_t color;
    switch (status) {
        case 0: color = CRT_BRIGHT; break;
        case 1: color = CRT_YELLOW; break;
        case 2: color = CRT_RED; break;
        default: color = CRT_DIM; break;
    }
    tft.fillCircle(x, y, 4, color);
    tft.drawCircle(x, y, 5, color);
}

void drawNavBar(int screenIndex, int totalScreens) {
    int y = 226;
    int dotSpacing = 10;
    int totalWidth = (totalScreens - 1) * dotSpacing;
    int startX = 160 - totalWidth / 2;
    tft.setTextColor(screenIndex > 0 ? CRT_MID : CRT_DIM);
    tft.setTextSize(1);
    tft.setCursor(startX - 18, y - 2);
    tft.print("<");
    for (int i = 0; i < totalScreens; i++) {
        int dx = startX + i * dotSpacing;
        if (i == screenIndex) {
            tft.fillCircle(dx, y + 2, 3, CRT_BRIGHT);
        } else {
            tft.drawCircle(dx, y + 2, 2, CRT_DIM);
        }
    }
    tft.setTextColor(screenIndex < totalScreens - 1 ? CRT_MID : CRT_DIM);
    tft.setCursor(startX + totalWidth + 10, y - 2);
    tft.print(">");
}

void drawButton(ButtonArea &btn, const char* label, ButtonStyle style) {
    uint16_t fill, border, textColor;
    switch (style) {
        case BTN_DANGER: fill = CRT_RED_DARK; border = CRT_RED; textColor = CRT_WHITE; break;
        case BTN_GHOST:  fill = CRT_BG; border = CRT_DIM; textColor = CRT_DIM; break;
        default:         fill = PANEL_FILL; border = CRT_BRIGHT; textColor = CRT_BRIGHT; break;
    }
    tft.fillRoundRect(btn.x, btn.y, btn.w, btn.h, 3, fill);
    tft.drawRoundRect(btn.x, btn.y, btn.w, btn.h, 3, border);
    tft.setTextColor(textColor);
    tft.setTextSize(1);
    int textW = strlen(label) * 6;
    tft.setCursor(btn.x + (btn.w - textW) / 2, btn.y + (btn.h - 8) / 2);
    tft.print(label);
}

int getTotalScreens() {
    return 2 + deviceCount;
}

uint16_t tempColor(float temp) {
    if (temp > 65) return CRT_RED;
    if (temp > 55) return CRT_YELLOW;
    return CRT_BRIGHT;
}

void formatDiff(char* buf, int bufSize, double val) {
    if (val > 1000000000) snprintf(buf, bufSize, "%.2fG", val / 1000000000.0);
    else if (val > 1000000) snprintf(buf, bufSize, "%.2fM", val / 1000000.0);
    else if (val > 1000) snprintf(buf, bufSize, "%.2fK", val / 1000.0);
    else snprintf(buf, bufSize, "%.0f", val);
}

void formatShares(char* buf, int bufSize, int val) {
    if (val >= 1000000) snprintf(buf, bufSize, "%.1fM", val / 1000000.0);
    else if (val >= 1000) snprintf(buf, bufSize, "%.1fK", val / 1000.0);
    else snprintf(buf, bufSize, "%d", val);
}

void formatPrice(char* buf, int bufSize, float price) {
    if (price >= 10000)     snprintf(buf, bufSize, "$%.0f", price);
    else if (price >= 100)  snprintf(buf, bufSize, "$%.1f", price);
    else if (price >= 1)    snprintf(buf, bufSize, "$%.2f", price);
    else if (price >= 0.01) snprintf(buf, bufSize, "$%.4f", price);
    else                    snprintf(buf, bufSize, "$%.6f", price);
}

void formatUptime(char* buf, int bufSize, int seconds) {
    int days = seconds / 86400;
    int hours = (seconds % 86400) / 3600;
    int mins = (seconds % 3600) / 60;
    if (days > 0) snprintf(buf, bufSize, "%dd%dh", days, hours);
    else if (hours > 0) snprintf(buf, bufSize, "%dh%dm", hours, mins);
    else snprintf(buf, bufSize, "%dm", mins);
}

// ===== AGGREGATE HELPERS =====

float getTotalHashrate() {
    float total = 0;
    for (int i = 0; i < deviceCount; i++) {
        if (devices[i].valid) total += devices[i].hashRate;
    }
    return total;
}

float getTotalPower() {
    float total = 0;
    for (int i = 0; i < deviceCount; i++) {
        if (devices[i].valid) total += devices[i].power;
    }
    return total;
}

int getValidDeviceCount() {
    int count = 0;
    for (int i = 0; i < deviceCount; i++) {
        if (devices[i].valid) count++;
    }
    return count;
}

int getTotalSharesAccepted() {
    int total = 0;
    for (int i = 0; i < deviceCount; i++) {
        if (devices[i].valid) total += devices[i].sharesAccepted;
    }
    return total;
}

double getMaxBestSessionDiff() {
    double maxDiff = 0;
    for (int i = 0; i < deviceCount; i++) {
        if (devices[i].valid && devices[i].bestSessionDiff > maxDiff)
            maxDiff = devices[i].bestSessionDiff;
    }
    return maxDiff;
}

double getMaxBestDiff() {
    double maxDiff = 0;
    for (int i = 0; i < deviceCount; i++) {
        if (devices[i].valid && devices[i].bestDiff > maxDiff)
            maxDiff = devices[i].bestDiff;
    }
    return maxDiff;
}

// ===== TOUCH EFFECTS =====

void drawTouchRipple(int x, int y) {
    x = constrain(x, 15, SCR_W - 15);
    y = constrain(y, 15, SCR_H - 15);
    uint16_t colors[] = {CRT_BRIGHT, CRT_MID, CRT_DIM, CRT_GLOW, CRT_SCANLINE};
    int radii[] = {6, 12, 18, 24, 30};
    for (int i = 0; i < 5; i++) {
        tft.drawCircle(x, y, radii[i], colors[i]);
        tft.drawCircle(x, y, radii[i] - 1, colors[i]);
        delay(15);
    }
    for (int i = 4; i >= 0; i--) {
        tft.drawCircle(x, y, radii[i], CRT_BG);
        tft.drawCircle(x, y, radii[i] - 1, CRT_BG);
    }
}

void scanlineWipeTransition() {
    int bandHeight = 6;
    for (int y = 0; y < SCR_H; y += bandHeight) {
        tft.drawFastHLine(0, y, SCR_W, CRT_BRIGHT);
        if (y > 0) tft.fillRect(0, y - bandHeight, SCR_W, bandHeight, CRT_BG);
        delay(3);
    }
    tft.fillRect(0, SCR_H - bandHeight, SCR_W, bandHeight, CRT_BG);
    delay(30);
    for (int i = 0; i < 30; i++) {
        tft.drawPixel(random(0, SCR_W), random(0, SCR_H), CRT_DIM);
    }
    delay(20);
    tft.fillScreen(CRT_BG);
}

// ===== IP PARSING =====

void parseDeviceIPs(const char* ipList) {
    deviceCount = 0;
    char buf[320];
    strncpy(buf, ipList, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';
    char* tok = strtok(buf, ",");
    while (tok && deviceCount < 8) {
        while (*tok == ' ') tok++;
        // Trim trailing spaces
        char* end = tok + strlen(tok) - 1;
        while (end > tok && *end == ' ') { *end = '\0'; end--; }
        if (strlen(tok) > 0) {
            strncpy(devices[deviceCount].ip, tok, 39);
            devices[deviceCount].ip[39] = '\0';
            devices[deviceCount].valid = false;
            deviceCount++;
        }
        tok = strtok(NULL, ",");
    }
}

// ===== BOOT / SETUP / FAILED SCREENS =====

void drawBootScreen() {
    drawScreenFrame("MONITOR");
    drawGlowText(56, 70, "STEAMPUNK MONITOR", 1, CRT_BRIGHT);
    tft.setTextColor(CRT_MID);
    tft.setTextSize(1);
    tft.setCursor(66, 95);
    tft.print("FIRING UP THE BOILERS...");
    drawPanel(40, 120, 240, 24);
    for (int i = 0; i < 224; i += 4) {
        tft.fillRect(48, 126, i, 12, CRT_BRIGHT);
        delay(3);
    }
    delay(300);
}

void drawSetupScreen() {
    drawScreenFrame("WIFI SETUP");
    drawGlowText(88, 38, "CONNECTING...", 1, CRT_BRIGHT);
    drawPanel(10, 56, 300, 130, "IF NOT CONNECTED:");
    tft.setTextSize(1);
    tft.setTextColor(CRT_BRIGHT);
    tft.setCursor(18, 72);
    tft.print("1. CONNECT TO WIFI:");
    tft.setTextColor(CRT_WHITE);
    tft.setCursor(28, 86);
    tft.print("BitAxe-Monitor");
    tft.setTextColor(CRT_BRIGHT);
    tft.setCursor(18, 100);
    tft.print("2. PASSWORD:");
    tft.setTextColor(CRT_WHITE);
    tft.setCursor(100, 100);
    tft.print("bitaxe123");
    tft.setTextColor(CRT_BRIGHT);
    tft.setCursor(18, 114);
    tft.print("3. OPEN: 192.168.4.1");
    tft.setTextColor(CRT_BRIGHT);
    tft.setCursor(18, 128);
    tft.print("4. ENTER BITAXE IP(s)");
    tft.setTextSize(1);
    tft.setTextColor(CRT_DIM);
    tft.setCursor(48, 198);
    tft.print("TIMEOUT: 180s - AUTO RESTART");
    tft.setTextColor(CRT_MID);
    tft.setCursor(80, 214);
    tft.print("BITAXE INDUSTRIES");
}

void drawFailedScreen() {
    tft.fillScreen(CRT_BG);
    drawScanlines();
    tft.drawRect(2, 2, 316, 236, CRT_RED);
    tft.drawRect(3, 3, 314, 234, CRT_RED);
    drawGlowText(68, 60, "CONNECTION", 2, CRT_RED);
    drawGlowText(98, 90, "FAILED", 2, CRT_RED);
    drawGlowText(32, 140, "RESTARTING SYSTEM...", 1, CRT_BRIGHT);
    tft.setTextSize(1);
    tft.setTextColor(CRT_MID);
    tft.setCursor(80, 200);
    tft.print("BITAXE INDUSTRIES");
}

// ===== SCREEN 0: SUMMARY DASHBOARD =====

void drawMainUI() {
    drawScreenFrame("MONITOR");

    int validCount = getValidDeviceCount();
    if (validCount == 0 && deviceCount > 0) {
        drawGlowText(60, 100, "WAITING FOR DATA...", 1, CRT_DIM);
        drawNavBar(0, getTotalScreens());
        return;
    }
    if (deviceCount == 0) {
        drawGlowText(52, 100, "NO DEVICES CONFIGURED", 1, CRT_DIM);
        drawNavBar(0, getTotalScreens());
        return;
    }

    float totalHash = getTotalHashrate();
    float totalPower = getTotalPower();
    // Convert GH/s to TH/s for display
    float totalHashTH = totalHash / 1000.0;
    float efficiency = (totalHashTH > 0) ? (totalPower / (totalHashTH * 1000.0)) : 0;

    // 3 Arc Gauges
    char buf[16];
    if (totalHash >= 1000) {
        snprintf(buf, sizeof(buf), "%.2f", totalHashTH);
        drawArcGauge(53, 62, 28, 22, totalHashTH, 0, 10.0,
                     "HASHRATE", buf, "TH/s", CRT_BRIGHT);
    } else {
        snprintf(buf, sizeof(buf), "%.0f", totalHash);
        drawArcGauge(53, 62, 28, 22, totalHash, 0, 5000,
                     "HASHRATE", buf, "GH/s", CRT_BRIGHT);
    }

    snprintf(buf, sizeof(buf), "%.0f", totalPower);
    drawArcGauge(160, 62, 28, 22, totalPower, 0, 500,
                 "POWER", buf, "W", CRT_BRIGHT);

    snprintf(buf, sizeof(buf), "%.1f", efficiency);
    drawArcGauge(267, 62, 28, 22, efficiency, 0, 30,
                 "EFF", buf, "J/TH", CRT_BRIGHT);

    // Devices panel
    drawPanel(8, 100, 100, 62, "DEVICES");
    drawIconWorker(14, 114, CRT_BRIGHT);
    tft.setTextColor(CRT_WHITE);
    tft.setTextSize(2);
    tft.setCursor(42, 116);
    tft.printf("%d/%d", validCount, deviceCount);
    tft.setTextColor(CRT_DIM);
    tft.setTextSize(1);
    tft.setCursor(42, 140);
    tft.print("ONLINE");

    // Shares panel
    drawPanel(112, 100, 100, 62, "SHARES");
    drawIconCheck(118, 114, CRT_BRIGHT);
    int totalShares = getTotalSharesAccepted();
    char sharesBuf[16];
    formatShares(sharesBuf, sizeof(sharesBuf), totalShares);
    tft.setTextColor(CRT_WHITE);
    tft.setTextSize(1);
    tft.setCursor(146, 118);
    tft.print(sharesBuf);
    tft.setTextColor(CRT_DIM);
    tft.setTextSize(1);
    tft.setCursor(146, 140);
    tft.print("accepted");

    // Best Diff panel
    drawPanel(216, 100, 96, 62, "BEST DIFF");
    double maxDiff = getMaxBestSessionDiff();
    char diffBuf[16];
    formatDiff(diffBuf, sizeof(diffBuf), maxDiff);
    tft.setTextColor(CRT_WHITE);
    tft.setTextSize(1);
    tft.setCursor(222, 118);
    tft.print(diffBuf);
    tft.setTextColor(CRT_DIM);
    tft.setTextSize(1);
    tft.setCursor(222, 140);
    tft.print("session");

    // Status bar
    drawPanel(8, 170, 304, 22);
    int dotStatus = (validCount > 0) ? 0 : 2;
    drawStatusDot(20, 181, dotStatus);
    tft.setTextColor(CRT_BRIGHT);
    tft.setTextSize(1);
    tft.setCursor(30, 177);
    tft.print(validCount > 0 ? "OK" : "ERR");
    tft.setTextColor(CRT_DIM);
    tft.setCursor(48, 177);
    tft.print("|");
    tft.setTextColor(CRT_MID);
    tft.setCursor(56, 177);
    tft.print(WiFi.localIP().toString());
    tft.setTextColor(CRT_DIM);
    tft.setCursor(164, 177);
    tft.print("|");
    unsigned long secs = millis() / 1000;
    int hrs = secs / 3600;
    int mins = (secs % 3600) / 60;
    tft.setTextColor(CRT_MID);
    tft.setCursor(172, 177);
    tft.printf("%dh%dm", hrs, mins);

    drawNavBar(0, getTotalScreens());
}

void updateDisplay() {
    int validCount = getValidDeviceCount();
    if (validCount == 0) {
        tft.fillRect(50, 90, 220, 20, CRT_BG);
        tft.setTextColor(CRT_RED);
        tft.setTextSize(1);
        tft.setCursor(60, 100);
        tft.print("NO DEVICES RESPONDING");
        return;
    }

    float totalHash = getTotalHashrate();
    float totalPower = getTotalPower();
    float totalHashTH = totalHash / 1000.0;
    float efficiency = (totalHashTH > 0) ? (totalPower / (totalHashTH * 1000.0)) : 0;

    char buf[16];

    // Clear gauge centers
    tft.fillCircle(53, 62, 20, CRT_BG);
    tft.fillCircle(160, 62, 20, CRT_BG);
    tft.fillCircle(267, 62, 20, CRT_BG);

    if (totalHash >= 1000) {
        snprintf(buf, sizeof(buf), "%.2f", totalHashTH);
        drawArcGauge(53, 62, 28, 22, totalHashTH, 0, 10.0,
                     "HASHRATE", buf, "TH/s", CRT_BRIGHT);
    } else {
        snprintf(buf, sizeof(buf), "%.0f", totalHash);
        drawArcGauge(53, 62, 28, 22, totalHash, 0, 5000,
                     "HASHRATE", buf, "GH/s", CRT_BRIGHT);
    }

    snprintf(buf, sizeof(buf), "%.0f", totalPower);
    drawArcGauge(160, 62, 28, 22, totalPower, 0, 500,
                 "POWER", buf, "W", CRT_BRIGHT);

    snprintf(buf, sizeof(buf), "%.1f", efficiency);
    drawArcGauge(267, 62, 28, 22, efficiency, 0, 30,
                 "EFF", buf, "J/TH", CRT_BRIGHT);

    // Update devices count
    tft.fillRect(40, 112, 60, 20, PANEL_FILL);
    tft.setTextColor(CRT_WHITE);
    tft.setTextSize(2);
    tft.setCursor(42, 116);
    tft.printf("%d/%d", validCount, deviceCount);

    // Update shares
    tft.fillRect(144, 114, 60, 28, PANEL_FILL);
    int totalShares = getTotalSharesAccepted();
    char sharesBuf[16];
    formatShares(sharesBuf, sizeof(sharesBuf), totalShares);
    tft.setTextColor(CRT_WHITE);
    tft.setTextSize(1);
    tft.setCursor(146, 118);
    tft.print(sharesBuf);

    // Update best diff
    tft.fillRect(220, 114, 86, 28, PANEL_FILL);
    double maxDiff = getMaxBestSessionDiff();
    char diffBuf[16];
    formatDiff(diffBuf, sizeof(diffBuf), maxDiff);
    tft.setTextColor(CRT_WHITE);
    tft.setTextSize(1);
    tft.setCursor(222, 118);
    tft.print(diffBuf);

    // Update status bar
    tft.fillRect(28, 174, 280, 14, PANEL_FILL);
    int dotStatus = (validCount > 0) ? 0 : 2;
    drawStatusDot(20, 181, dotStatus);
    tft.setTextColor(CRT_BRIGHT);
    tft.setTextSize(1);
    tft.setCursor(30, 177);
    tft.print(validCount > 0 ? "OK" : "ERR");
    tft.setTextColor(CRT_DIM);
    tft.setCursor(48, 177);
    tft.print("|");
    tft.setTextColor(CRT_MID);
    tft.setCursor(56, 177);
    tft.print(WiFi.localIP().toString());
    tft.setTextColor(CRT_DIM);
    tft.setCursor(164, 177);
    tft.print("|");
    unsigned long secs = millis() / 1000;
    int hrs = secs / 3600;
    int mins = (secs % 3600) / 60;
    tft.setTextColor(CRT_MID);
    tft.setCursor(172, 177);
    tft.printf("%dh%dm", hrs, mins);
}

// ===== SCREEN 1: BITCOIN/PRICE PAGE =====

void drawPoolScreen() {
    drawScreenFrame("COIN / NETWORK");

    uint16_t coinColor = coins[selectedCoin].color;

    // Price panel
    drawPanel(8, 34, 304, 48);
    drawCoinIcon24(12, 38, coinColor, coins[selectedCoin].ticker);
    char priceBuf[24];
    if (pool.btcPrice > 0) {
        formatPrice(priceBuf, sizeof(priceBuf), pool.btcPrice);
    } else {
        snprintf(priceBuf, sizeof(priceBuf), "Loading...");
    }
    drawGlowText(38, 38, priceBuf, 2, coinColor);

    // 24h change
    tft.setTextSize(1);
    if (pool.priceChange24h >= 0) {
        tft.setTextColor(CRT_BRIGHT);
        tft.setCursor(38, 58);
        tft.printf("+%.1f%%", pool.priceChange24h);
    } else {
        tft.setTextColor(CRT_RED);
        tft.setCursor(38, 58);
        tft.printf("%.1f%%", pool.priceChange24h);
    }

    // Coin name + pool URL from first device
    tft.setTextColor(CRT_DIM);
    tft.setTextSize(1);
    tft.setCursor(110, 58);
    tft.print(coins[selectedCoin].name);
    tft.setCursor(38, 70);
    if (deviceCount > 0 && devices[0].valid && devices[0].stratumURL.length() > 0) {
        tft.printf("POOL: %s:%d", devices[0].stratumURL.c_str(), devices[0].stratumPort);
    } else {
        tft.print("POOL: --");
    }

    // Next coin button
    drawButton(btnNextCoin, "NEXT>", BTN_GHOST);

    // Best Diff panels
    drawPanel(8, 86, 152, 40, "BEST DIFF ALL-TIME");
    double bestAll = getMaxBestDiff();
    char diffBuf[16];
    formatDiff(diffBuf, sizeof(diffBuf), bestAll);
    tft.setTextColor(CRT_WHITE);
    tft.setTextSize(1);
    tft.setCursor(14, 102);
    tft.print(diffBuf);
    drawHBar(14, 114, 140, 6, 1.0, 1.0, CRT_MID, NULL);

    drawPanel(164, 86, 148, 40, "BEST DIFF SESSION");
    double bestSess = getMaxBestSessionDiff();
    formatDiff(diffBuf, sizeof(diffBuf), bestSess);
    tft.setTextColor(CRT_WHITE);
    tft.setTextSize(1);
    tft.setCursor(170, 102);
    tft.print(diffBuf);
    float sessionPct = (bestAll > 0) ? (float)(bestSess / bestAll) : 0;
    drawHBar(170, 114, 136, 6, sessionPct, 1.0, CRT_MID, NULL);

    // Bottom row: Error Rate | Net Diff | Daily Cost
    // Error rate from first device or aggregate
    float errPct = 0;
    int totalAcc = getTotalSharesAccepted();
    int totalRej = 0;
    for (int i = 0; i < deviceCount; i++) {
        if (devices[i].valid) totalRej += devices[i].sharesRejected;
    }
    if (totalAcc + totalRej > 0) errPct = (float)totalRej / (float)(totalAcc + totalRej) * 100.0;

    drawPanel(8, 130, 100, 60, "ERROR RATE");
    uint16_t errColor = CRT_BRIGHT;
    if (errPct > 5) errColor = CRT_RED;
    else if (errPct > 1) errColor = CRT_YELLOW;
    char errBuf[16];
    snprintf(errBuf, sizeof(errBuf), "%.2f%%", errPct);
    tft.setTextColor(errColor);
    tft.setTextSize(1);
    tft.setCursor(14, 146);
    tft.print(errBuf);
    drawHBar(14, 158, 88, 6, errPct, 10.0, errColor, NULL);

    drawPanel(112, 130, 100, 60, "NET DIFF");
    tft.setTextColor(CRT_WHITE);
    tft.setTextSize(1);
    tft.setCursor(118, 150);
    if (pool.networkDifficulty > 0) {
        tft.printf("%.2fT", pool.networkDifficulty / 1000000000000.0);
    } else {
        tft.print("Loading...");
    }

    drawPanel(216, 130, 96, 60, "DAILY COST");
    float totalPower = getTotalPower();
    float dailyKwh = totalPower * 24.0 / 1000.0;
    float dailyCost = dailyKwh * electricityRate;
    tft.setTextColor(CRT_MID);
    tft.setTextSize(1);
    tft.setCursor(222, 146);
    tft.printf("%.0fW", totalPower);
    char costBuf[16];
    snprintf(costBuf, sizeof(costBuf), "$%.2f/d", dailyCost);
    tft.setTextColor(CRT_WHITE);
    tft.setTextSize(1);
    tft.setCursor(222, 158);
    tft.print(costBuf);
    char rateBuf[16];
    snprintf(rateBuf, sizeof(rateBuf), "@$%.2f", electricityRate);
    tft.setTextColor(CRT_DIM);
    tft.setTextSize(1);
    tft.setCursor(222, 172);
    tft.print(rateBuf);
    drawButton(btnRateMinus, "-", BTN_GHOST);
    drawButton(btnRatePlus, "+", BTN_GHOST);

    drawNavBar(1, getTotalScreens());
}

void updatePoolScreen() {
    uint16_t coinColor = coins[selectedCoin].color;

    tft.fillRect(36, 36, 240, 18, PANEL_FILL);
    char priceBuf[24];
    if (pool.btcPrice > 0) {
        formatPrice(priceBuf, sizeof(priceBuf), pool.btcPrice);
    } else {
        snprintf(priceBuf, sizeof(priceBuf), "Loading...");
    }
    drawGlowText(38, 38, priceBuf, 2, coinColor);

    tft.fillRect(36, 56, 70, 10, PANEL_FILL);
    tft.setTextSize(1);
    if (pool.priceChange24h >= 0) {
        tft.setTextColor(CRT_BRIGHT);
        tft.setCursor(38, 58);
        tft.printf("+%.1f%%", pool.priceChange24h);
    } else {
        tft.setTextColor(CRT_RED);
        tft.setCursor(38, 58);
        tft.printf("%.1f%%", pool.priceChange24h);
    }

    tft.fillRect(36, 68, 240, 10, PANEL_FILL);
    tft.setTextColor(CRT_DIM);
    tft.setTextSize(1);
    tft.setCursor(38, 70);
    if (deviceCount > 0 && devices[0].valid && devices[0].stratumURL.length() > 0) {
        tft.printf("POOL: %s:%d", devices[0].stratumURL.c_str(), devices[0].stratumPort);
    } else {
        tft.print("POOL: --");
    }

    // Best diff all-time
    tft.fillRect(12, 98, 140, 12, PANEL_FILL);
    double bestAll = getMaxBestDiff();
    char diffBuf[16];
    formatDiff(diffBuf, sizeof(diffBuf), bestAll);
    tft.setTextColor(CRT_WHITE);
    tft.setTextSize(1);
    tft.setCursor(14, 102);
    tft.print(diffBuf);

    // Best diff session
    tft.fillRect(168, 98, 136, 12, PANEL_FILL);
    double bestSess = getMaxBestSessionDiff();
    formatDiff(diffBuf, sizeof(diffBuf), bestSess);
    tft.setTextColor(CRT_WHITE);
    tft.setTextSize(1);
    tft.setCursor(170, 102);
    tft.print(diffBuf);
    tft.fillRect(170, 114, 136, 6, PANEL_FILL);
    float sessionPct = (bestAll > 0) ? (float)(bestSess / bestAll) : 0;
    drawHBar(170, 114, 136, 6, sessionPct, 1.0, CRT_MID, NULL);

    // Error rate
    tft.fillRect(12, 142, 90, 24, PANEL_FILL);
    float errPct = 0;
    int totalAcc = getTotalSharesAccepted();
    int totalRej = 0;
    for (int i = 0; i < deviceCount; i++) {
        if (devices[i].valid) totalRej += devices[i].sharesRejected;
    }
    if (totalAcc + totalRej > 0) errPct = (float)totalRej / (float)(totalAcc + totalRej) * 100.0;
    uint16_t errColor = CRT_BRIGHT;
    if (errPct > 5) errColor = CRT_RED;
    else if (errPct > 1) errColor = CRT_YELLOW;
    char errBuf[16];
    snprintf(errBuf, sizeof(errBuf), "%.2f%%", errPct);
    tft.setTextColor(errColor);
    tft.setTextSize(1);
    tft.setCursor(14, 146);
    tft.print(errBuf);
    tft.fillRect(14, 158, 88, 6, PANEL_FILL);
    drawHBar(14, 158, 88, 6, errPct, 10.0, errColor, NULL);

    // Network difficulty
    tft.fillRect(116, 146, 90, 14, PANEL_FILL);
    tft.setTextColor(CRT_WHITE);
    tft.setTextSize(1);
    tft.setCursor(118, 150);
    if (pool.networkDifficulty > 0) {
        tft.printf("%.2fT", pool.networkDifficulty / 1000000000000.0);
    } else {
        tft.print("Loading...");
    }

    // Daily cost
    tft.fillRect(220, 142, 86, 40, PANEL_FILL);
    float totalPower = getTotalPower();
    float dailyKwh = totalPower * 24.0 / 1000.0;
    float dailyCost = dailyKwh * electricityRate;
    tft.setTextColor(CRT_MID);
    tft.setTextSize(1);
    tft.setCursor(222, 146);
    tft.printf("%.0fW", totalPower);
    char costBuf[16];
    snprintf(costBuf, sizeof(costBuf), "$%.2f/d", dailyCost);
    tft.setTextColor(CRT_WHITE);
    tft.setTextSize(1);
    tft.setCursor(222, 158);
    tft.print(costBuf);
    char rateBuf[16];
    snprintf(rateBuf, sizeof(rateBuf), "@$%.2f", electricityRate);
    tft.setTextColor(CRT_DIM);
    tft.setTextSize(1);
    tft.setCursor(222, 172);
    tft.print(rateBuf);
    drawButton(btnRateMinus, "-", BTN_GHOST);
    drawButton(btnRatePlus, "+", BTN_GHOST);
}

// ===== SCREEN 2+: INDIVIDUAL DEVICE SCREENS =====

void drawDeviceScreen(int devIndex) {
    if (devIndex >= deviceCount) return;
    DeviceInfo &dev = devices[devIndex];

    char title[40];
    if (dev.valid && dev.hostname.length() > 0) {
        snprintf(title, sizeof(title), "WORKER: %s", dev.hostname.c_str());
    } else {
        snprintf(title, sizeof(title), "WORKER: %s", dev.ip);
    }
    drawScreenFrame(title);

    // Page indicator
    tft.setTextColor(CRT_DIM);
    tft.setTextSize(1);
    tft.setCursor(278, 12);
    tft.printf("[%d/%d]", devIndex + 1, deviceCount);

    if (!dev.valid) {
        drawGlowText(60, 80, "DEVICE OFFLINE", 1, CRT_RED);
        tft.setTextColor(CRT_DIM);
        tft.setTextSize(1);
        tft.setCursor(60, 100);
        tft.printf("IP: %s", dev.ip);
        drawNavBar(2 + devIndex, getTotalScreens());
        return;
    }

    // Device model subtitle
    if (dev.deviceModel.length() > 0) {
        tft.setTextColor(CRT_DIM);
        tft.setTextSize(1);
        tft.setCursor(8, 12);
        tft.print(dev.deviceModel);
    }

    // Hashrate arc gauge
    char hashBuf[16];
    const char* hashUnit;
    if (dev.hashRate >= 1000) {
        snprintf(hashBuf, sizeof(hashBuf), "%.2f", dev.hashRate / 1000.0);
        hashUnit = "TH/s";
    } else {
        snprintf(hashBuf, sizeof(hashBuf), "%.0f", dev.hashRate);
        hashUnit = "GH/s";
    }
    drawArcGauge(80, 60, 28, 22, dev.hashRate, 0, 1000,
                 "HASHRATE", hashBuf, hashUnit, CRT_BRIGHT);

    // Temperature arc gauge
    char tempBuf[16];
    snprintf(tempBuf, sizeof(tempBuf), "%.1f", dev.temperature);
    drawArcGauge(240, 60, 28, 22, dev.temperature, 0, 80,
                 "TEMP", tempBuf, "C", tempColor(dev.temperature));

    // Info line
    tft.setTextColor(CRT_MID);
    tft.setTextSize(1);
    tft.setCursor(10, 96);
    tft.printf("IP:%s", dev.ip);
    tft.setTextColor(CRT_BRIGHT);
    tft.setCursor(148, 96);
    tft.printf("CORE:%dmV", dev.coreVoltage);
    tft.setTextColor(CRT_MID);
    tft.setCursor(240, 96);
    if (dev.fanRpm > 0) {
        tft.printf("FAN:%d", dev.fanRpm);
    } else {
        tft.printf("RSSI:%d", dev.wifiRSSI);
    }

    // Performance panel
    drawPanel(8, 108, 304, 68, "PERFORMANCE");
    int barX = 80;
    int barW = 224;

    // Power
    tft.setTextColor(CRT_DIM);
    tft.setTextSize(1);
    tft.setCursor(14, 124);
    tft.print("PWR");
    tft.setTextColor(CRT_MID);
    tft.setCursor(38, 124);
    tft.printf("%.1fW", dev.power);
    drawHBar(barX, 122, barW, 10, dev.power, 30.0, CRT_BRIGHT, NULL);

    // Frequency
    tft.setTextColor(CRT_DIM);
    tft.setCursor(14, 136);
    tft.print("FRQ");
    tft.setTextColor(CRT_MID);
    tft.setCursor(38, 136);
    tft.printf("%dM", dev.frequency);
    drawHBar(barX, 134, barW, 10, (float)dev.frequency, 1200.0, CRT_BRIGHT, NULL);

    // Input voltage
    tft.setTextColor(CRT_DIM);
    tft.setCursor(14, 148);
    tft.print("VIN");
    tft.setTextColor(CRT_MID);
    tft.setCursor(38, 148);
    tft.printf("%.2fV", dev.voltage);
    uint16_t vinColor = (dev.voltage < 4.9) ? CRT_RED : CRT_BRIGHT;
    drawHBar(barX, 146, barW, 10, dev.voltage, 5.5, vinColor, NULL);

    // Shares
    tft.setTextColor(CRT_DIM);
    tft.setCursor(14, 160);
    tft.print("SHR");
    char sBuf[16];
    formatShares(sBuf, sizeof(sBuf), dev.sharesAccepted);
    tft.setTextColor(CRT_MID);
    tft.setCursor(38, 160);
    tft.print(sBuf);
    drawHBar(barX, 158, barW, 10, (float)dev.sharesAccepted, (float)max(1, dev.sharesAccepted) * 1.2f, CRT_BRIGHT, NULL);

    // 6 Control buttons
    drawButton(btnDevRestart, "RST", BTN_DANGER);
    drawButton(btnDevFreqPlus, "FRQ+", BTN_PRIMARY);
    drawButton(btnDevFreqMinus, "FRQ-", BTN_PRIMARY);
    drawButton(btnDevVoltPlus, "mV+", BTN_PRIMARY);
    drawButton(btnDevVoltMinus, "mV-", BTN_PRIMARY);
    drawButton(btnDevFanPlus, "FAN+", BTN_PRIMARY);

    drawNavBar(2 + devIndex, getTotalScreens());
}

void updateDeviceScreen(int devIndex) {
    if (devIndex >= deviceCount) return;
    DeviceInfo &dev = devices[devIndex];

    if (!dev.valid) return;

    // Clear gauge centers
    tft.fillCircle(80, 60, 20, CRT_BG);
    tft.fillCircle(240, 60, 20, CRT_BG);

    // Hashrate gauge
    char hashBuf[16];
    const char* hashUnit;
    if (dev.hashRate >= 1000) {
        snprintf(hashBuf, sizeof(hashBuf), "%.2f", dev.hashRate / 1000.0);
        hashUnit = "TH/s";
    } else {
        snprintf(hashBuf, sizeof(hashBuf), "%.0f", dev.hashRate);
        hashUnit = "GH/s";
    }
    drawArcGauge(80, 60, 28, 22, dev.hashRate, 0, 1000,
                 "HASHRATE", hashBuf, hashUnit, CRT_BRIGHT);

    // Temperature gauge
    char tempBuf[16];
    snprintf(tempBuf, sizeof(tempBuf), "%.1f", dev.temperature);
    drawArcGauge(240, 60, 28, 22, dev.temperature, 0, 80,
                 "TEMP", tempBuf, "C", tempColor(dev.temperature));

    // Info line
    tft.fillRect(8, 94, 304, 10, CRT_BG);
    tft.setTextColor(CRT_MID);
    tft.setTextSize(1);
    tft.setCursor(10, 96);
    tft.printf("IP:%s", dev.ip);
    tft.setTextColor(CRT_BRIGHT);
    tft.setCursor(148, 96);
    tft.printf("CORE:%dmV", dev.coreVoltage);
    tft.setTextColor(CRT_MID);
    tft.setCursor(240, 96);
    if (dev.fanRpm > 0) {
        tft.printf("FAN:%d", dev.fanRpm);
    } else {
        tft.printf("RSSI:%d", dev.wifiRSSI);
    }

    // Performance bars
    int barX = 80;
    int barW = 224;

    // Power
    tft.fillRect(36, 121, 40, 10, PANEL_FILL);
    tft.fillRect(barX, 122, barW, 10, PANEL_FILL);
    tft.setTextColor(CRT_MID);
    tft.setTextSize(1);
    tft.setCursor(38, 124);
    tft.printf("%.1fW", dev.power);
    drawHBar(barX, 122, barW, 10, dev.power, 30.0, CRT_BRIGHT, NULL);

    // Frequency
    tft.fillRect(36, 133, 40, 10, PANEL_FILL);
    tft.fillRect(barX, 134, barW, 10, PANEL_FILL);
    tft.setTextColor(CRT_MID);
    tft.setCursor(38, 136);
    tft.printf("%dM", dev.frequency);
    drawHBar(barX, 134, barW, 10, (float)dev.frequency, 1200.0, CRT_BRIGHT, NULL);

    // Voltage
    tft.fillRect(36, 145, 40, 10, PANEL_FILL);
    tft.fillRect(barX, 146, barW, 10, PANEL_FILL);
    tft.setTextColor(CRT_MID);
    tft.setCursor(38, 148);
    tft.printf("%.2fV", dev.voltage);
    uint16_t vinColor = (dev.voltage < 4.9) ? CRT_RED : CRT_BRIGHT;
    drawHBar(barX, 146, barW, 10, dev.voltage, 5.5, vinColor, NULL);

    // Shares
    tft.fillRect(36, 157, 40, 10, PANEL_FILL);
    tft.fillRect(barX, 158, barW, 10, PANEL_FILL);
    char sBuf[16];
    formatShares(sBuf, sizeof(sBuf), dev.sharesAccepted);
    tft.setTextColor(CRT_MID);
    tft.setCursor(38, 160);
    tft.print(sBuf);
    drawHBar(barX, 158, barW, 10, (float)dev.sharesAccepted, (float)max(1, dev.sharesAccepted) * 1.2f, CRT_BRIGHT, NULL);
}

// ===== NETWORK FETCH FUNCTIONS =====

void fetchDeviceData(int index) {
    if (index >= deviceCount) return;
    if (WiFi.status() != WL_CONNECTED) return;
    if (strlen(devices[index].ip) == 0) return;

    String url = "http://" + String(devices[index].ip) + "/api/system/info";
    HTTPClient http;
    http.setTimeout(5000);
    http.begin(url);
    int httpCode = http.GET();

    if (httpCode == 200) {
        String payload = http.getString();
        DynamicJsonDocument doc(4096);
        DeserializationError error = deserializeJson(doc, payload);

        if (!error) {
            deviceFailCount[index] = 0;
            DeviceInfo &dev = devices[index];
            dev.valid = true;
            dev.hashRate = doc["hashRate"] | 0.0f;
            dev.hashRate_1h = doc["hashRate_1h"] | 0.0f;
            dev.temperature = doc["temp"] | 0.0f;
            dev.vrTemp = doc["vrTemp"] | 0.0f;
            dev.power = doc["power"] | 0.0f;
            dev.voltage = (doc["voltage"] | 0.0f) / 1000.0f;
            dev.coreVoltage = doc["coreVoltage"] | 1200;
            dev.frequency = doc["frequency"] | 0;
            dev.fanRpm = doc["fanrpm"] | 0;
            dev.fanSpeed = doc["fanspeed"] | 0;
            dev.sharesAccepted = doc["sharesAccepted"] | 0;
            dev.sharesRejected = doc["sharesRejected"] | 0;
            dev.bestDiff = doc["bestDiff"] | 0.0;
            dev.bestSessionDiff = doc["bestSessionDiff"] | 0.0;
            dev.hostname = doc["hostname"] | "";
            dev.deviceModel = doc["deviceModel"] | "";
            dev.asicModel = doc["ASICModel"] | "";
            dev.stratumURL = doc["stratumURL"] | "";
            dev.stratumPort = doc["stratumPort"] | 0;
            dev.stratumUser = doc["stratumUser"] | "";
            dev.uptimeSeconds = doc["uptimeSeconds"] | 0;
            dev.wifiRSSI = doc["wifiRSSI"] | 0;
        } else {
            deviceFailCount[index]++;
            if (deviceFailCount[index] >= MAX_FAIL_BEFORE_INVALID) {
                devices[index].valid = false;
            }
        }
    } else {
        deviceFailCount[index]++;
        if (deviceFailCount[index] >= MAX_FAIL_BEFORE_INVALID) {
            devices[index].valid = false;
        }
    }
    http.end();
}

void fetchBtcPrice() {
    if (WiFi.status() != WL_CONNECTED) return;

    String url = "https://api.coingecko.com/api/v3/simple/price?ids="
        + String(coins[selectedCoin].apiId)
        + "&vs_currencies=usd&include_24hr_change=true";

    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;
    http.setTimeout(10000);
    http.begin(client, url);
    int httpCode = http.GET();

    if (httpCode == 200) {
        String payload = http.getString();
        DynamicJsonDocument doc(512);
        DeserializationError error = deserializeJson(doc, payload);
        if (!error) {
            pool.btcPrice = doc[coins[selectedCoin].apiId]["usd"].as<float>();
            pool.priceChange24h = doc[coins[selectedCoin].apiId]["usd_24h_change"].as<float>();
        }
    }
    http.end();
}

void fetchNetworkDifficulty() {
    if (WiFi.status() != WL_CONNECTED) return;

    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;
    http.setTimeout(10000);
    http.begin(client, "https://mempool.space/api/v1/mining/hashrate/1m");
    int httpCode = http.GET();

    if (httpCode == 200) {
        String payload = http.getString();
        DynamicJsonDocument doc(4096);
        DeserializationError error = deserializeJson(doc, payload);
        if (!error) {
            pool.networkDifficulty = doc["currentDifficulty"].as<double>();
        }
    }
    http.end();
}

// ===== POST FUNCTIONS =====

bool postDeviceSetting(int deviceIndex, const char* jsonBody) {
    if (deviceIndex >= deviceCount) return false;
    if (WiFi.status() != WL_CONNECTED) return false;
    if (strlen(devices[deviceIndex].ip) == 0) return false;

    String url = "http://" + String(devices[deviceIndex].ip) + "/api/system";
    HTTPClient http;
    http.setTimeout(5000);
    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    int httpCode = http.sendRequest("PATCH", jsonBody);
    http.end();
    return (httpCode == 200);
}

bool postDeviceRestart(int deviceIndex) {
    if (deviceIndex >= deviceCount) return false;
    if (WiFi.status() != WL_CONNECTED) return false;
    if (strlen(devices[deviceIndex].ip) == 0) return false;

    String url = "http://" + String(devices[deviceIndex].ip) + "/api/system/restart";
    HTTPClient http;
    http.setTimeout(5000);
    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    int httpCode = http.POST("");
    http.end();
    return (httpCode == 200);
}

// ===== TOUCH HANDLING =====

unsigned long lastTouchDebug = 0;

void handleTouch() {
    TouchPoint tp = touch.read();

    if (millis() - lastTouchDebug > 200) {
        lastTouchDebug = millis();
        if (tp.pressed) {
            Serial.printf("TOUCH: x=%d y=%d pressed=YES state=%s\n",
                tp.x, tp.y, touchPressed ? "HELD" : "IDLE");
        }
    }

    static int lastTouchX = 0;
    static int lastTouchY = 0;

    if (tp.pressed && !touchPressed) {
        touchPressed = true;
        touchStartX = tp.x;
        touchStartY = tp.y;
        lastTouchX = tp.x;
        lastTouchY = tp.y;
        touchStartTime = millis();
    } else if (tp.pressed && touchPressed) {
        lastTouchX = tp.x;
        lastTouchY = tp.y;
    } else if (!tp.pressed && touchPressed) {
        int endX = lastTouchX;
        int endY = lastTouchY;
        int swipeDist = endX - touchStartX;

        if (millis() - touchStartTime > 30) {
            if (abs(swipeDist) < SWIPE_THRESHOLD) {
                // === Screen 1: Pool/Bitcoin page buttons ===
                if (currentScreen == 1) {
                    if (checkButtonPress(btnNextCoin, touchStartX, touchStartY)) {
                        drawTouchRipple(touchStartX, touchStartY);
                        selectedCoin = (selectedCoin + 1) % COIN_COUNT;
                        prefs.putInt("coin", selectedCoin);
                        fetchBtcPrice();
                        drawPoolScreen();
                    }
                    if (checkButtonPress(btnRateMinus, touchStartX, touchStartY)) {
                        electricityRate = max(0.01f, electricityRate - 0.01f);
                        prefs.putFloat("elecRate", electricityRate);
                        updatePoolScreen();
                    }
                    if (checkButtonPress(btnRatePlus, touchStartX, touchStartY)) {
                        electricityRate = min(1.00f, electricityRate + 0.01f);
                        prefs.putFloat("elecRate", electricityRate);
                        updatePoolScreen();
                    }
                }

                // === Screen 2+: Device control buttons ===
                if (currentScreen >= 2) {
                    int devIndex = currentScreen - 2;
                    if (devIndex < deviceCount && devices[devIndex].valid) {
                        // RST button with double-tap confirm
                        if (checkButtonPress(btnDevRestart, touchStartX, touchStartY)) {
                            unsigned long now = millis();
                            if (restartConfirmPending && restartTapDevice == devIndex && (now - lastRestartTap < 2000)) {
                                drawTouchRipple(touchStartX, touchStartY);
                                postDeviceRestart(devIndex);
                                restartConfirmPending = false;
                                restartTapDevice = -1;
                                drawDeviceScreen(devIndex);
                            } else {
                                restartConfirmPending = true;
                                restartTapDevice = devIndex;
                                lastRestartTap = now;
                                tft.fillRoundRect(btnDevRestart.x, btnDevRestart.y, btnDevRestart.w, btnDevRestart.h, 3, CRT_RED_DARK);
                                tft.drawRoundRect(btnDevRestart.x, btnDevRestart.y, btnDevRestart.w, btnDevRestart.h, 3, CRT_YELLOW);
                                tft.setTextColor(CRT_YELLOW);
                                tft.setTextSize(1);
                                tft.setCursor(btnDevRestart.x + 4, btnDevRestart.y + 6);
                                tft.print("AGAIN?");
                            }
                        }
                        // FRQ+
                        if (checkButtonPress(btnDevFreqPlus, touchStartX, touchStartY)) {
                            int newFreq = devices[devIndex].frequency + 25;
                            drawTouchRipple(touchStartX, touchStartY);
                            char body[32];
                            snprintf(body, sizeof(body), "{\"frequency\":%d}", newFreq);
                            postDeviceSetting(devIndex, body);
                        }
                        // FRQ-
                        if (checkButtonPress(btnDevFreqMinus, touchStartX, touchStartY)) {
                            int newFreq = max(100, devices[devIndex].frequency - 25);
                            drawTouchRipple(touchStartX, touchStartY);
                            char body[32];
                            snprintf(body, sizeof(body), "{\"frequency\":%d}", newFreq);
                            postDeviceSetting(devIndex, body);
                        }
                        // mV+
                        if (checkButtonPress(btnDevVoltPlus, touchStartX, touchStartY)) {
                            int newVolt = min(1400, devices[devIndex].coreVoltage + 25);
                            drawTouchRipple(touchStartX, touchStartY);
                            char body[32];
                            snprintf(body, sizeof(body), "{\"coreVoltage\":%d}", newVolt);
                            postDeviceSetting(devIndex, body);
                        }
                        // mV-
                        if (checkButtonPress(btnDevVoltMinus, touchStartX, touchStartY)) {
                            int newVolt = max(1000, devices[devIndex].coreVoltage - 25);
                            drawTouchRipple(touchStartX, touchStartY);
                            char body[32];
                            snprintf(body, sizeof(body), "{\"coreVoltage\":%d}", newVolt);
                            postDeviceSetting(devIndex, body);
                        }
                        // FAN+
                        if (checkButtonPress(btnDevFanPlus, touchStartX, touchStartY)) {
                            int newFan = min(100, devices[devIndex].fanSpeed + 5);
                            drawTouchRipple(touchStartX, touchStartY);
                            char body[32];
                            snprintf(body, sizeof(body), "{\"fanspeed\":%d}", newFan);
                            postDeviceSetting(devIndex, body);
                        }
                    }
                }
            }
            // Swipe navigation
            else if (abs(swipeDist) > SWIPE_THRESHOLD) {
                int maxScreen = 1 + deviceCount;
                if (swipeDist < 0) {
                    if (currentScreen < maxScreen) {
                        currentScreen++;
                        redrawCurrentScreen();
                    }
                } else {
                    if (currentScreen > 0) {
                        currentScreen--;
                        redrawCurrentScreen();
                    }
                }
            }
        }
        touchPressed = false;
    }
}

void redrawCurrentScreen() {
    scanlineWipeTransition();
    if (currentScreen == 0) {
        drawMainUI();
        updateDisplay();
    } else if (currentScreen == 1) {
        drawPoolScreen();
    } else {
        drawDeviceScreen(currentScreen - 2);
    }
}

// ===== LED =====

void updateLed(unsigned long now) {
    if (shareFlashTime > 0 && (now - shareFlashTime < SHARE_FLASH_DURATION)) {
        digitalWrite(LED_GREEN, LOW);
        digitalWrite(LED_RED, HIGH);
        digitalWrite(LED_BLUE, HIGH);
        return;
    }

    if (now - lastLedToggle >= LED_BLINK_INTERVAL) {
        lastLedToggle = now;
        ledState = !ledState;
        int validCount = getValidDeviceCount();
        if (validCount > 0 && WiFi.status() == WL_CONNECTED) {
            digitalWrite(LED_GREEN, ledState ? LOW : HIGH);
            digitalWrite(LED_RED, HIGH);
            digitalWrite(LED_BLUE, HIGH);
        } else if (WiFi.status() == WL_CONNECTED) {
            digitalWrite(LED_GREEN, ledState ? LOW : HIGH);
            digitalWrite(LED_RED, ledState ? LOW : HIGH);
            digitalWrite(LED_BLUE, HIGH);
        } else {
            digitalWrite(LED_RED, ledState ? LOW : HIGH);
            digitalWrite(LED_GREEN, HIGH);
            digitalWrite(LED_BLUE, HIGH);
        }
    }
}

// ===== BUTTON AREAS (320x240 layout) =====

// Pool page
ButtonArea btnNextCoin  = {268, 36, 42, 18};
ButtonArea btnRateMinus = {270, 170, 20, 14};
ButtonArea btnRatePlus  = {294, 170, 20, 14};

// Device screen (6 buttons across, ~50px each)
ButtonArea btnDevRestart   = {8, 182, 48, 22};
ButtonArea btnDevFreqPlus  = {60, 182, 48, 22};
ButtonArea btnDevFreqMinus = {112, 182, 48, 22};
ButtonArea btnDevVoltPlus  = {164, 182, 48, 22};
ButtonArea btnDevVoltMinus = {216, 182, 48, 22};
ButtonArea btnDevFanPlus   = {268, 182, 48, 22};

bool checkButtonPress(ButtonArea &btn, int x, int y) {
    return (x >= btn.x && x <= btn.x + btn.w && y >= btn.y && y <= btn.y + btn.h);
}

// ===== SETUP & LOOP =====

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("\n=== BitAxe Monitor - 2.4\" Steampunk Edition ===");

    // Backlight on early (GPIO 27 on the 2.4" CYD)
    pinMode(27, OUTPUT);
    digitalWrite(27, HIGH);

    tft.init();
    tft.setRotation(1);
    tft.fillScreen(CRT_BG);

    pinMode(LED_RED, OUTPUT);
    pinMode(LED_GREEN, OUTPUT);
    pinMode(LED_BLUE, OUTPUT);
    digitalWrite(LED_RED, HIGH);
    digitalWrite(LED_GREEN, HIGH);
    digitalWrite(LED_BLUE, HIGH);

    drawBootScreen();

    prefs.begin("bitaxemon", false);
    selectedCoin = prefs.getInt("coin", 0);
    electricityRate = prefs.getFloat("elecRate", 0.12);
    if (selectedCoin < 0 || selectedCoin >= COIN_COUNT) selectedCoin = 0;
    String savedIPs = prefs.getString("ips", "");
    if (savedIPs.length() > 0) {
        parseDeviceIPs(savedIPs.c_str());
    }

    // WiFiManager - scoped to free memory
    char ipListBuf[320] = "";
    if (savedIPs.length() > 0) {
        savedIPs.toCharArray(ipListBuf, sizeof(ipListBuf));
    }

    {
    WiFiManager wm;
    wm.setDebugOutput(true);

    const char* steampunkCSS = R"(
    <style>
        body { background-color: #000000 !important; color: #FFB000 !important; font-family: 'Courier New', monospace !important; }
        .wrap { background-color: #000000 !important; }
        h1, h2, h3 { color: #FFB000 !important; text-transform: uppercase; letter-spacing: 2px; text-shadow: 0 0 10px #FFB000; }
        input, select { background-color: #1A0C00 !important; color: #FFB000 !important; border: 2px solid #FFB000 !important; padding: 10px !important; font-family: 'Courier New', monospace !important; }
        input:focus { box-shadow: 0 0 10px #FFB000 !important; }
        button, input[type='submit'], input[type='button'] { background-color: #332200 !important; color: #FFB000 !important; border: 2px solid #FFB000 !important; padding: 12px 24px !important; font-family: 'Courier New', monospace !important; text-transform: uppercase; cursor: pointer; }
        button:hover, input[type='submit']:hover { background-color: #FFB000 !important; color: #000000 !important; box-shadow: 0 0 20px #FFB000; }
        a { color: #FFB000 !important; }
        .q { color: #FFB000 !important; }
        .msg { background-color: #1A0C00 !important; border: 1px solid #FFB000 !important; color: #FFB000 !important; }
        #s, #p { background-color: #1A0C00 !important; }
    </style>
    <title>BITAXE NETWORK CONFIG</title>
    )";
    wm.setCustomHeadElement(steampunkCSS);
    wm.setTitle("BITAXE SYSTEMS");

    WiFiManagerParameter custom_ips("ips", "BitAxe IPs (comma-separated)", ipListBuf, 320);
    wm.addParameter(&custom_ips);
    wm.setConfigPortalTimeout(180);

    wm.setSaveParamsCallback([&custom_ips, &ipListBuf]() {
        strncpy(ipListBuf, custom_ips.getValue(), sizeof(ipListBuf) - 1);
        ipListBuf[sizeof(ipListBuf) - 1] = '\0';
        Preferences p;
        p.begin("bitaxemon", false);
        p.putString("ips", String(ipListBuf));
        p.end();
    });

    drawSetupScreen();

    bool connected;
    if (deviceCount == 0) {
        connected = wm.startConfigPortal("BitAxe-Monitor", "bitaxe123");
    } else {
        connected = wm.autoConnect("BitAxe-Monitor", "bitaxe123");
    }

    if (!connected) {
        drawFailedScreen();
        delay(3000);
        ESP.restart();
    }
    }

    // Re-parse IPs in case they were updated via WiFiManager
    if (strlen(ipListBuf) > 0) {
        parseDeviceIPs(ipListBuf);
    }

    Serial.printf("Free heap after WiFiManager: %d bytes\n", ESP.getFreeHeap());
    Serial.println("WiFi connected: " + WiFi.localIP().toString());
    Serial.printf("Device count: %d\n", deviceCount);
    for (int i = 0; i < deviceCount; i++) {
        Serial.printf("  Device %d: %s\n", i, devices[i].ip);
    }

    // Initialize touch
    delay(200);
    touch.begin();

    // Initial data fetch - all devices
    for (int i = 0; i < deviceCount; i++) {
        fetchDeviceData(i);
    }
    fetchBtcPrice();
    fetchNetworkDifficulty();

    currentScreen = 0;
    drawMainUI();
}

void loop() {
    unsigned long now = millis();
    handleTouch();
    updateLed(now);

    // BTC price + network difficulty every 60s
    if (now - lastBtcUpdate >= BTC_UPDATE_INTERVAL) {
        lastBtcUpdate = now;
        fetchBtcPrice();
        fetchNetworkDifficulty();
    }

    // Fetch one device per cycle (round-robin) every 5s
    if (now - lastUpdate >= UPDATE_INTERVAL) {
        lastUpdate = now;

        if (deviceCount > 0) {
            fetchDeviceData(deviceFetchIndex);
            deviceFetchIndex = (deviceFetchIndex + 1) % deviceCount;
        }

        // Track share flashes
        int totalShares = getTotalSharesAccepted();
        if (totalShares > lastTotalShares) {
            if (lastTotalShares > 0) {
                shareFlashTime = now;
            }
            lastTotalShares = totalShares;
        }

        // Update current screen
        if (currentScreen == 0) updateDisplay();
        else if (currentScreen == 1) updatePoolScreen();
        else updateDeviceScreen(currentScreen - 2);
    }

    delay(50);
}
