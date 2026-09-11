<p align="right">
  <a href="README.zh_CN.md">简体中文</a> · <strong>English</strong>
</p>

# BSP Hardware Demo (AI Passport Reference Demo)

Reference hardware peripheral verification demo and interactive menu for FoloToy AI Passport (ESP32-C3).

## Features

- **Display**: ST7789 240x320 color LCD, brightness control, palette test, and mascot animation.
- **Button / ADC**: Physical key detection (Up/Down/OK) and battery ADC voltage monitoring.
- **Audio**: ES8311 I2S codec audio playback, tones, and recording tests.
- **Battery**: Battery state-of-charge (SoC) estimation and charging indicator.
- **Wi-Fi**: AP scanning with visual signal strength indicator.
- **BLE**: NimBLE advertising and connection testing.
- **Low Power**: Light sleep and deep sleep mode verification.

## Building and Flashing

From the repository root:

```bash
# Build firmware
idf.py -C projects/bsp-demo build

# Flash to connected device and monitor
idf.py -C projects/bsp-demo flash monitor
```

## Running Host Tests

```bash
cc -std=c11 -Wall -Wextra -Werror -Iprojects/bsp-demo/main \
    projects/bsp-demo/tests/test_ui_pixel_math.c \
    projects/bsp-demo/main/ui_pixel_math.c \
    -o /tmp/test_ui_pixel_math && /tmp/test_ui_pixel_math
```
