```
╔══════════════════════════════════════════════════════════════════════════════╗
║                                                                              ║
║    ██╗   ██╗ █████╗ ██╗   ██╗██╗  ████████╗    ████████╗███████╗ ██████╗     ║
║    ██║   ██║██╔══██╗██║   ██║██║  ╚══██╔══╝    ╚══██╔══╝██╔════╝██╔════╝     ║
║    ██║   ██║███████║██║   ██║██║     ██║          ██║   █████╗  ██║          ║
║    ╚██╗ ██╔╝██╔══██║██║   ██║██║     ██║          ██║   ██╔══╝  ██║          ║
║     ╚████╔╝ ██║  ██║╚██████╔╝███████╗██║          ██║   ███████╗╚██████╗     ║
║      ╚═══╝  ╚═╝  ╚═╝ ╚═════╝ ╚══════╝╚═╝          ╚═╝   ╚══════╝ ╚═════╝     ║
║                                                                              ║
║              B I T A X E   W I R E L E S S   D I S P L A Y                   ║
║                          ══ v2.0 ══                                          ║
║                                                                              ║
║          "Preparing for the future — one hash at a time."                    ║
║                                                                              ║
╚══════════════════════════════════════════════════════════════════════════════╝
```

> **VAULT-TEC INDUSTRIES** — *Mining Operations Division*
> *CLASSIFIED: Vault Dweller Eyes Only*

---

## `[TERMINAL ACCESS GRANTED]`

```
> ACCESSING VAULT-TEC MAINFRAME...
> LOADING MODULE: BITAXE WIRELESS DISPLAY v2.0
> STATUS: ██████████████████████████████ 100%
> WELCOME, OVERSEER.
```

A **steampunk-themed multi-device BitAxe monitor** built for the ESP32 Cheap Yellow Display (CYD 2.4"). Track up to **8 BitAxe miners** in real-time with a retro amber CRT aesthetic. Monitor hashrates, temperatures, power draw, shares, and more — all from a compact touchscreen in the comfort of your vault.

---

## `> FEATURES.DAT`

```
╔════════════════════════════════════════════════════════════════╗
║  VAULT-TEC BITAXE MONITOR — SYSTEM CAPABILITIES              ║
╠════════════════════════════════════════════════════════════════╣
║                                                                ║
║  [■] Multi-Device Monitoring .... Up to 8 BitAxe units        ║
║  [■] Real-Time Hashrate ........ GH/s & TH/s arc gauges       ║
║  [■] Temperature Tracking ...... Color-coded alerts            ║
║  [■] Power & Efficiency ........ Watts + J/TH calculations    ║
║  [■] Share Statistics .......... Accepted / Rejected / Rate    ║
║  [■] Remote Device Control ..... Freq / Voltage / Fan / RST   ║
║  [■] Multi-Coin Support ........ BTC, BCH, DGB, BTC2, XEC     ║
║  [■] Live Price Feed ........... CoinGecko API + 24h change   ║
║  [■] Network Difficulty ........ Mempool.space integration     ║
║  [■] Electricity Cost Calc ..... Configurable $/kWh rate      ║
║  [■] WiFi Config Portal ........ Web-based setup, no reflash  ║
║  [■] Steampunk CRT Theme ...... Amber/gold retro aesthetic    ║
║  [■] LED Status Indicators ..... RGB breathing + flash alerts  ║
║  [■] Touch Navigation ......... Swipe between screens         ║
║  [■] Non-Volatile Storage ...... Settings persist on reboot   ║
║                                                                ║
╚════════════════════════════════════════════════════════════════╝
```

---

## `> REQUIRED_HARDWARE.INV`

```
┌──────────────────────────────────────────────────────────┐
│  VAULT-TEC REQUISITION FORM — REQUIRED COMPONENTS        │
├──────────────────────────────────────────────────────────┤
│                                                          │
│  ITEM                          QTY    STATUS             │
│  ─────────────────────────     ───    ──────             │
│  ESP32-2432S024 (CYD 2.4")    1      MANDATORY          │
│  ├─ ILI9341 320x240 TFT LCD                             │
│  ├─ Capacitive (CST820) OR                               │
│  └─ Resistive (XPT2046) touch                            │
│                                                          │
│  BitAxe Miner(s)              1-8    AT LEAST ONE        │
│  USB-C Cable                  1      FOR POWER/FLASH     │
│  WiFi Network                 1      2.4 GHz             │
│                                                          │
└──────────────────────────────────────────────────────────┘
```

---

## `> INSTALLATION_PROTOCOL.DOC`

### Prerequisites

- [PlatformIO](https://platformio.org/) (CLI or VS Code extension)

### Step 1 — Clone the Vault

```bash
git clone https://github.com/ShaeOJ/Bitaxe-Wireless-Display-v2.0.git
cd Bitaxe-Wireless-Display-v2.0
```

### Step 2 — Select Your Build Variant

| Environment | Touch Type | Driver | Command |
|---|---|---|---|
| `cyd24c` | Capacitive | CST820 (I2C) | `pio run -e cyd24c` |
| `cyd24r` | Resistive | XPT2046 (SPI) | `pio run -e cyd24r` |

> **Overseer's Note:** Most CYD 2.4" boards ship with **capacitive** touch. Check your board before flashing.

### Step 3 — Flash the Firmware

```bash
# Capacitive touch (recommended)
pio run -e cyd24c -t upload

# Resistive touch
pio run -e cyd24r -t upload
```

### Step 4 — Configure Your Vault

```
┌─────────────────────────────────────────────────────────────┐
│                   WIFI SETUP PROTOCOL                        │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  1. Power on the display                                     │
│  2. Connect to WiFi AP:                                      │
│     ├─ SSID: BitAxe-Monitor                                  │
│     └─ PASS: bitaxe123                                       │
│  3. Navigate to: http://192.168.4.1                          │
│  4. Select your WiFi network & enter password                │
│  5. Enter BitAxe IP addresses (comma-separated)              │
│     Example: 192.168.1.100,192.168.1.101,192.168.1.102       │
│  6. Set your electricity rate ($/kWh)                        │
│  7. Save — the device will reboot and connect                │
│                                                              │
│  TIMEOUT: 180 seconds — device restarts if not configured    │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

---

## `> SCREEN_LAYOUT.MAP`

The display cycles through multiple screens via **swipe navigation**:

```
┌─────────┐    ┌─────────┐    ┌─────────┐         ┌─────────┐
│SCREEN 0 │───>│SCREEN 1 │───>│SCREEN 2 │───> ... │SCREEN N │
│Dashboard│<───│Pool Info│<───│Device 1 │<───     │Device 8 │
└─────────┘    └─────────┘    └─────────┘         └─────────┘
   Swipe ◄────────────────────────────────────────────► Swipe
```

| Screen | Content |
|---|---|
| **Dashboard** | Aggregate stats — total hashrate, power, efficiency, temp, shares across all devices |
| **Pool / Coin Info** | BTC price, 24h change, network difficulty, daily electricity cost, coin selector |
| **Device 1-8** | Individual miner stats with arc gauges + control buttons (FRQ, mV, FAN, RST) |

---

## `> DEVICE_CONTROL.CMD`

Each device screen provides **direct hardware control** via touch buttons:

```
┌────────────────────────────────────────────────────────┐
│  CONTROL PANEL — DEVICE SCREEN                          │
├────────────────────────────────────────────────────────┤
│                                                         │
│  [RST]   Restart device      (double-tap to confirm)   │
│  [FRQ+]  Increase frequency  (+25 MHz)                  │
│  [FRQ-]  Decrease frequency  (-25 MHz)                  │
│  [mV+]   Increase voltage    (+25 mV, max 1400 mV)     │
│  [mV-]   Decrease voltage    (-25 mV, min 1000 mV)     │
│  [FAN+]  Increase fan speed  (+5%, 0-100%)              │
│                                                         │
│  ⚠ WARNING: Adjusting voltage/frequency may affect      │
│  device stability. Vault-Tec is not responsible for     │
│  any fried ASICs. Proceed at your own risk, Dweller.    │
│                                                         │
└────────────────────────────────────────────────────────┘
```

---

## `> LED_STATUS.LOG`

```
STATUS              LED BEHAVIOR            MEANING
──────              ────────────            ───────
ALL SYSTEMS GO      Green (breathing)       WiFi + all devices online
PARTIAL             Green+Red (breathing)   WiFi OK, waiting for devices
DISCONNECTED        Red (breathing)         WiFi connection lost
SHARE ACCEPTED      Green (flash 200ms)     New share found!
```

---

## `> DEPENDENCIES.LIB`

| Library | Version | Purpose |
|---|---|---|
| [TFT_eSPI](https://github.com/Bodmer/TFT_eSPI) | 2.5.43 | Display driver & graphics rendering |
| [ArduinoJson](https://github.com/bblanchon/ArduinoJson) | 6.21.5 | JSON parsing for API responses |
| [WiFiManager](https://github.com/tzapu/WiFiManager) | Latest | Web-based WiFi configuration portal |
| [bb_captouch](https://github.com/bitbank2/bb_captouch) | Latest | CST820 capacitive touch driver |

---

## `> API_ENDPOINTS.NET`

```
BITAXE DEVICE (Local Network — HTTP)
├── GET  /api/system/info      → Device status & metrics
├── PATCH /api/system           → Set frequency, voltage, fan speed
└── POST /api/system/restart    → Restart device

COINGECKO (Internet — HTTPS)
└── GET /api/v3/simple/price   → Crypto price + 24h change

MEMPOOL.SPACE (Internet — HTTPS)
└── GET /api/v1/mining/hashrate/1m → Bitcoin network difficulty
```

---

## `> PROJECT_STRUCTURE.DIR`

```
Bitaxe-Wireless-Display-v2.0/
├── platformio.ini           # Build config (two environments)
├── src/
│   ├── main.cpp             # Application firmware (~1,840 lines)
│   └── touch_interface.h    # Touch driver abstraction layer
├── .gitignore
└── README.md                # You are here, Vault Dweller.
```

---

## `> TROUBLESHOOTING.HLP`

```
╔══════════════════════════════════════════════════════════════╗
║  PROBLEM                         SOLUTION                    ║
╠══════════════════════════════════════════════════════════════╣
║  Display is blank                Check USB power & backlight ║
║  WiFi portal won't appear        Hold reset, wait 10 sec    ║
║  Device shows "offline"          Verify BitAxe IP & network  ║
║  Touch not responding            Check build variant (c/r)   ║
║  Price not updating              CoinGecko rate limit (wait) ║
║  Config portal timeout           Restart & try within 180s   ║
╚══════════════════════════════════════════════════════════════╝
```

---

## `> LICENSE.TXT`

This project is open source. Use it, mod it, share it with your fellow Vault Dwellers.

---

```
╔══════════════════════════════════════════════════════════════════════════════╗
║                                                                              ║
║     "In the event of total nuclear annihilation, at least your hashrate      ║
║      will be monitored." — Vault-Tec Mining Operations Manual, Ch. 7        ║
║                                                                              ║
║                    ⚛  VAULT-TEC INDUSTRIES  ⚛                               ║
║              "Building a Better Tomorrow, Underground."                      ║
║                                                                              ║
╚══════════════════════════════════════════════════════════════════════════════╝
```
