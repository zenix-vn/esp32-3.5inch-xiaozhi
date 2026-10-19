# ESP32 3.5" display - Zin100 / Xiaozhi

Custom firmware for the **ESP32-2432S035** board (ESP32, 3.5" 480x320 ST7796 display, XPT2046 resistive touch,
microSD). Replaces the stock demo firmware with a LVGL UI and Xiaozhi (xiaozhi.me) device activation.

## What it does
- **Home**: clock (NTP), weather (Open-Meteo), mascot face, Xiaozhi status. Shows the 6-digit pairing code until the
  device is linked at xiaozhi.me.
- **Swipe left**: payment QR page (VietQR), then the **Apps** launcher.
- **Apps**: Wi-Fi (scan, connect with on-screen keyboard), Settings (brightness, screen-off timer, time zone,
  touch recalibration, factory reset), Storage, About. Music / Games / Photos are placeholders.
- First boot: touch calibration (tap 4 targets). Wi-Fi can also be set via the `Zin100-Setup` hotspot
  (open `192.168.4.1`). Hold BOOT 3 s to forget Wi-Fi; hold BOOT while powering up to recalibrate touch.

## Build and flash
```
cd firmware
python -m pip install platformio
python -m platformio run -t upload --upload-port COMx
```
Libraries (LVGL 8.3, TFT_eSPI, ArduinoJson) are fetched by PlatformIO. Pin and driver setup is in
`firmware/platformio.ini`.

## Layout
| Path | Purpose |
|---|---|
| `firmware/src/main.cpp` | UI shell: home, QR page, launcher, tile navigation |
| `firmware/src/pages.h` | Settings, Wi-Fi, Storage, About pages |
| `firmware/src/touch.h` | XPT2046 driver + calibration wizard |
| `firmware/src/wifi_setup.*` | Wi-Fi connect, captive portal, NTP |
| `firmware/src/xiaozhi_activation.*` | Xiaozhi OTA / activation protocol |
| `firmware/src/weather.*` | Weather + IP geolocation |
| `tools/` | One-off helper scripts (QR decode, patches) |

## Reference library (not committed)
The official Xiaozhi firmware is used only as reading material for the protocol. Clone it next to the code:
```
git clone --depth 1 https://github.com/78/xiaozhi-esp32.git libs/xiaozhi-esp32
```

## Status
Voice chat is not implemented yet: the board has a speaker connector but no microphone.
The on-screen yellow touch diagnostic strip in `main.cpp` (`buildDebugStrip`) is temporary.
