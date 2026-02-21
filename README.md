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

## `> WEB_FLASH_PROTOCOL.DOC`

> **NO PLATFORMIO? NO PROBLEM.**
> Vault-Tec has partnered with Espressif to provide a **browser-based flashing solution**.
> No software installation required — just Chrome and a USB cable.

### Option A — Flash a Pre-Built `.bin` (Easiest)

Download a pre-built `firmware_merged.bin` from the [Releases](https://github.com/ShaeOJ/Bitaxe-Wireless-Display-v2.0/releases) page and skip to **Step 3** below.

### Option B — Build Your Own `.bin`

If you prefer to compile from source, the build system automatically creates a merged binary.

#### Step 1 — Build the Firmware

```bash
# Clone and build (capacitive touch)
git clone https://github.com/ShaeOJ/Bitaxe-Wireless-Display-v2.0.git
cd Bitaxe-Wireless-Display-v2.0
pio run -e cyd24c
```

After a successful build, you'll see:

```
=============================================
  Merging firmware into single flashable .bin
  Output: .pio/build/cyd24c/firmware_merged.bin
=============================================
```

Your flashable file is at:

```
.pio/build/cyd24c/firmware_merged.bin    (capacitive touch)
.pio/build/cyd24r/firmware_merged.bin    (resistive touch)
```

#### Step 2 — Understand What's Inside

```
┌─────────────────────────────────────────────────────────────────┐
│  VAULT-TEC FIRMWARE MANIFEST — MERGED BINARY CONTENTS           │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  COMPONENT            FLASH ADDRESS    DESCRIPTION               │
│  ──────────           ─────────────    ───────────               │
│  Bootloader           0x1000           ESP32 second-stage boot   │
│  Partition Table      0x8000           Flash memory layout       │
│  OTA Selector         0xE000           Boot app state manager    │
│  Application          0x10000          BitAxe Monitor firmware   │
│                                                                  │
│  The merge script combines all 4 components into one file.       │
│  The merged .bin starts at address 0x0 with 0xFF padding.        │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

### Step 3 — Flash Using the Espressif Web Tool

```
╔══════════════════════════════════════════════════════════════════╗
║                                                                  ║
║   VAULT-TEC FIELD MANUAL — BROWSER-BASED FIRMWARE DEPLOYMENT     ║
║                                                                  ║
╠══════════════════════════════════════════════════════════════════╣
║                                                                  ║
║   REQUIREMENTS:                                                  ║
║   [■] Google Chrome, Microsoft Edge, or Chromium-based browser   ║
║   [■] USB-C cable connected to CYD board                         ║
║   [■] firmware_merged.bin file (from build or Releases page)     ║
║                                                                  ║
║   ⚠  Safari and Firefox are NOT supported (no WebSerial API)     ║
║                                                                  ║
╚══════════════════════════════════════════════════════════════════╝
```

**Follow these steps:**

**3.1** — Open the Espressif Web Flash Tool in Chrome/Edge:

> **https://espressif.github.io/esptool-js/**

**3.2** — Connect your CYD board via USB-C.

**3.3** — In the web tool, set the **Baud Rate** to `460800` (faster) or leave at `115200` (safer).

**3.4** — Click **"Connect"** and select your board's COM/serial port from the popup.

```
┌─────────────────────────────────────────┐
│  SELECT SERIAL PORT                      │
├─────────────────────────────────────────┤
│                                          │
│  > COM3 - CP2102 USB to UART Bridge      │  <-- Usually this one
│    COM1 - Communications Port            │
│                                          │
│  If you don't see the port, install the  │
│  CP2102 or CH340 USB driver for your     │
│  board and try again.                    │
│                                          │
└─────────────────────────────────────────┘
```

> **Overseer's Tip:** If no port appears, you may need to install the USB-UART driver:
> - **CP2102 boards:** [Silicon Labs CP2102 Driver](https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers)
> - **CH340 boards:** [CH340 Driver](http://www.wch-ic.com/downloads/CH341SER_ZIP.html)

**3.5** — Once connected, click **"Choose File"** and select your `firmware_merged.bin`.

**3.6** — Set the **Flash Address** to:

```
╔═══════════════════════════════════════════════╗
║                                               ║
║   FLASH ADDRESS:  0x0                         ║
║                                               ║
║   ⚠  THIS MUST BE 0x0 FOR MERGED BINARIES    ║
║      Using 0x1000 or 0x10000 WILL FAIL        ║
║                                               ║
╚═══════════════════════════════════════════════╝
```

**3.7** — Click **"Program"** and wait for the flashing to complete.

```
> ERASING FLASH...
> WRITING AT 0x00000000... (1%)
> WRITING AT 0x00010000... (25%)
> WRITING AT 0x00020000... (50%)
> WRITING AT 0x00030000... (75%)
> WRITING AT 0x00040000... (100%)
> VERIFYING...
>
> ✓ FIRMWARE DEPLOYMENT COMPLETE
> REBOOT DEVICE TO ACTIVATE
```

**3.8** — Press the **RST** (reset) button on your CYD board, or unplug and replug USB.

**3.9** — The display will boot up and launch the WiFi configuration portal. Proceed to **Step 4** in the Installation Protocol above to configure your devices.

### Flashing Individual Files (Advanced)

If you prefer to flash the 4 components separately (or if the merged bin isn't available), you can add each file individually in the web tool:

| File | Flash Address | Location |
|---|---|---|
| `bootloader.bin` | `0x1000` | `.pio/build/cyd24c/` |
| `partitions.bin` | `0x8000` | `.pio/build/cyd24c/` |
| `boot_app0.bin` | `0xE000` | `~/.platformio/packages/framework-arduinoespressif32/tools/partitions/` |
| `firmware.bin` | `0x10000` | `.pio/build/cyd24c/` |

> In the web tool, click **"Add File"** for each row and set the corresponding address.

```
┌────────────────────────────────────────────────────────────┐
│  ⚠ VAULT-TEC SAFETY ADVISORY                               │
├────────────────────────────────────────────────────────────┤
│                                                             │
│  • If your board won't boot after flashing, use the web     │
│    tool's "Erase Flash" button to wipe everything, then     │
│    re-flash the merged .bin at 0x0.                         │
│                                                             │
│  • Always use the correct build variant for your board:     │
│    cyd24c = capacitive touch, cyd24r = resistive touch.     │
│    Wrong variant = touch won't respond.                     │
│                                                             │
│  • First time flashing? You may need to hold the BOOT       │
│    button on the CYD while pressing RST to enter flash      │
│    mode. Release BOOT after the tool connects.              │
│                                                             │
└────────────────────────────────────────────────────────────┘
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
├── merge_firmware.py        # Post-build script — creates merged .bin
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
