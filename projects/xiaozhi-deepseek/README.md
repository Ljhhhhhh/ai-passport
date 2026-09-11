<p align="right">
  <a href="README.zh_CN.md">简体中文</a> · <strong>English</strong>
</p>

# Xiaozhi AI Assistant (Built-in DeepSeek Edition)

Xiaozhi AI Companion client tailored for **FoloToy AI Passport** (ESP32-C3, 8 MB Flash, no PSRAM). This edition features **built-in DeepSeek model integration**, allowing developers to pre-configure DeepSeek as the direct LLM provider along with API keys during firmware compilation. Once flashed, the device runs out of the box without requiring external gateway servers, app bindings, or QR code setup.

## Features

- **Built-in DeepSeek Provider**: Direct HTTPS REST + SSE (Server-Sent Events) streaming connection to DeepSeek Chat Completions API (`https://api.deepseek.com/v1/chat/completions`). Supports `deepseek-chat` and `deepseek-reasoner` (DeepSeek-R1).
- **Compile-time Zero-Configuration**: Fill in `CONFIG_XIAOZHI_DEEPSEEK_API_KEY` and `CONFIG_XIAOZHI_WIFI_SSID` in `sdkconfig` or `menuconfig`. The firmware is ready immediately after flashing.
- **Expressive Emotion UI**: Xiaozhi-style emotional avatar with blinking eyes, dynamic audio sound waves, status badges, battery percentage, Wi-Fi status, and real-time streaming dialogue subtitles on the ST7789P3 240x320 LCD.
- **Audio Feedback & Voice Loop**: Synthesized harmonic chimes for wake, thinking, completion, click, and error states; full-duplex I2S audio via ES8311 codec (16 kHz 16-bit PCM).
- **Low SRAM Optimization**: Zero-copy SSE chunk parser and dynamic mbedTLS memory buffers specifically tuned for the 400 KB internal SRAM of ESP32-C3 without PSRAM.
- **Preset Topic Navigator**: Built-in prompt selector allowing users to browse and trigger diverse conversation topics via physical hardware buttons.

## Hardware Mapping (FoloToy AI Passport)

| Peripheral | Chip / Interface | Pins / Configuration |
|---|---|---|
| **MCU** | ESP32-C3 | Single-core RISC-V @ 160 MHz, 8 MB Flash, 400 KB SRAM |
| **Display** | ST7789P3 240x320 SPI | MOSI=9, SCLK=8, CS=1, DC=20, BL=21 (PWM) |
| **Audio Codec** | ES8311 (I2C + I2S) | I2C: SDA=10, SCL=7 (addr 0x18); I2S: MCLK=6, BCLK=5, WS=3, DOUT=2, DIN=4 |
| **Fuel Gauge** | CW2017 | I2C: SDA=10, SCL=7 (addr 0x63) |
| **Buttons** | 3-Key ADC Divider | GPIO0 (ADC1_CH0): UP (0 mV), DOWN (300 mV), OK (595 mV) |

## Configuration

Configure the built-in DeepSeek credentials and Wi-Fi settings in `projects/xiaozhi-deepseek/sdkconfig.defaults` or via menuconfig:

```bash
idf.py -C projects/xiaozhi-deepseek menuconfig
```

Navigate to `Xiaozhi DeepSeek AI Assistant`:
- `AI Model Configuration (Built-in DeepSeek)`:
  - `Model Provider Name`: `deepseek`
  - `DeepSeek API Key`: Enter your API key (`sk-...`)
  - `DeepSeek API Base URL`: `https://api.deepseek.com/v1`
  - `DeepSeek Model Name`: `deepseek-chat` (or `deepseek-reasoner`)
  - `System Prompt`: Customizable personality prompt
  - `Temperature`: `7` (0.7)
  - `Max Completion Tokens`: `512`
- `Wi-Fi Configuration`:
  - `Default Wi-Fi SSID`: Your Wi-Fi network name
  - `Default Wi-Fi Password`: Your Wi-Fi password

## Button Controls

- **UP Key (Short Press)**: Cycle to previous preset topic.
- **DOWN Key (Short Press)**: Cycle to next preset topic.
- **OK Key (Short Press)**:
  - In `IDLE` state: Send selected prompt to DeepSeek.
  - In `SPEAKING` / `THINKING` state: Interrupt current response and return to `IDLE`.
  - In `ERROR` state: Dismiss error and return to `IDLE`.
- **OK Key (Long Press)**: Enter push-to-talk voice recording mode.

## Build and Flash

```bash
# 1. Build firmware
idf.py -C projects/xiaozhi-deepseek build

# 2. Flash and monitor output
idf.py -C projects/xiaozhi-deepseek flash monitor

# 3. Run host tests
./tools/validate.sh --static
```
