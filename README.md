<p align="right">
  <a href="README.zh_CN.md">简体中文</a> · <strong>English</strong>
</p>

# FoloToy AI Passport Project Collection

Welcome to the **FoloToy AI Passport Project Collection** — a multi-project workspace and firmware collection for the FoloToy AI Passport (ESP32-C3 wearable AI hardware).

The FoloToy AI Passport is an open-source wearable AI card hardware powered by the ESP32-C3. It features a crisp 240×320 color LCD, an ES8311 I2S audio codec with microphone and speaker, three tactile control buttons, a CW2017 high-precision battery fuel gauge, and BLE communication. It serves as an ambient desk companion for AI coding assistants and a platform for embedded AI prototyping.

This repository is structured as a monorepo: all projects share a common Board Support Package (`components/bsp`), hardware design standards, and validation utilities, while each application lives independently inside its own subdirectory under `projects/`.

---

## Projects in this Collection

| Project | Directory | Description | Use Case |
| :--- | :--- | :--- | :--- |
| **Codex Passport** | [`projects/codex-passport`](projects/codex-passport) | Real-time companion monitoring Codex tasks, input prompts, unread completion, and quotas with Mac BLE sync and soft sound alerts | Coding companion, ambient task status display |
| **Animal Hanzi Story** | [`projects/animal-hanzi-story`](projects/animal-hanzi-story) | Interactive children's Hanzi literacy adventure with audio narration and custom bitmap fonts | Children's educational game |
| **Hanzi Cards** | [`projects/hanzi-cards`](projects/hanzi-cards) | Interactive character flashcard reader with voice pronunciation and NVS progress saving | Flashcard learning, literacy training |
| **XiaoZhi DeepSeek** | [`projects/xiaozhi-deepseek`](projects/xiaozhi-deepseek) | Wearable AI voice assistant connecting directly to DeepSeek LLM for conversational interactions | Voice assistant, on-the-go AI queries |
| **BSP Hardware Demo** | [`projects/bsp-demo`](projects/bsp-demo) | Comprehensive peripheral validation suite and reference menu (LCD, audio, buttons, battery, Wi-Fi, BLE, low power) | Factory self-test, driver development reference |

---

## Beginner's Quick Start Guide

You can get started using either of the following paths:

### Method 1: Web Flasher (No Toolchain Required, Recommended for Beginners)

If you just want to run the pre-built applications on your device:

1. **Connect Hardware**: Plug the AI Passport into your computer using a USB Type-C cable that supports **data transfer** (not a charge-only cable).
2. **Open Web Flasher**: Open Chrome or Edge and visit the [ESP Web Flasher](https://espressif.github.io/esptool-js/).
3. **Select Firmware**: Download the ready-to-flash merged binary from the repository (located at `build/<project_name>-full.bin`, such as `build/codex-passport-full.bin`).
4. **Flash to Board**:
   - Baud Rate: `460800`
   - Flash Offset: `0x0`
   - Click **Connect**, select the serial device port, and click **Program**. The board will automatically reboot into the selected application once finished.

---

### Method 2: Local Source Build (For Developers)

To modify source code, customize the UI, or build new features:

#### 1. Setup Environment

Install and activate **ESP-IDF 5.5.3**:

```bash
# Activate ESP-IDF 5.5.3
get_idf553
# Or using the standard ESP-IDF export script
. $IDF_PATH/export.sh
```

#### 2. Compile a Project

From the repository root, use the `-C` argument to build any project:

```bash
# Build Codex Passport
idf.py -C projects/codex-passport build

# Or build Animal Hanzi Story
idf.py -C projects/animal-hanzi-story build
```

#### 3. Flash and Monitor

With the board connected via USB:

```bash
# Flash firmware and open serial monitor
idf.py -C projects/codex-passport flash monitor
```

> Press `Ctrl + ]` to exit the serial monitor.

---

## Hardware Navigation and Buttons

The AI Passport front panel features three tactile buttons operating on an ADC resistor ladder connected to GPIO0:

| Button | Action | Menu / Standard View | Messages View |
| :--- | :--- | :--- | :--- |
| **UP** | Click | Scroll up / Previous item | Scroll up / Previous page |
| **DOWN** | Click | Scroll down / Next item | Scroll down / Next page |
| **OK** | Click | Enter selected item / Toggle view | View next page of 3 messages |
| **OK** | Click while asleep | **Wake screen** (stays awake 30s; no click action) | **Wake screen** |

### Display Sleep and Power Management

- **Auto Sleep**: When there are no unread tasks and no button has been pressed for 30 seconds, the backlight smoothly turns off to preserve battery life.
- **Stay Awake**: If there are active unread completed tasks, pending questions, or if BLE is disconnected, the screen stays illuminated so you never miss an alert.
- **Notifications**: Whenever a new actionable task event occurs (such as an input prompt or completed task), a soft two-tone chime sounds once.

---

## Creating a New Project

The repository includes a project generator script to scaffold an isolated application wired to the shared BSP in seconds:

```bash
# Scaffold a new project boilerplate
python3 tools/create_project.py my-ai-app --title "My First App"

# Build the newly generated project
idf.py -C projects/my-ai-app build
```

The new application directory will be created at `projects/my-ai-app`, complete with build scripts, source templates, and documentation.

---

## Validation and Testing Gate

Before committing code or submitting pull requests, run the automated test suite:

```bash
# 1. Static checks: document conventions, links, code formatting, and host unit tests
./tools/validate.sh --static

# 2. Firmware compilation: build and verify binary offsets for a specific project
./tools/validate.sh --firmware codex-passport

# 3. Complete gate: run full static checks and firmware builds for all projects
./tools/validate.sh
```

---

## Hardware Specifications

- **Microcontroller**: ESP32-C3FH4 (32-bit RISC-V single-core, 160 MHz)
- **Memory**: 8 MB embedded SPI Flash, no PSRAM
- **Display**: ST7789 240×320 portrait RGB565 IPS LCD via SPI2 (40 MHz) with LEDC PWM backlight control
- **Audio Subsystem**: ES8311 full-duplex I2S audio codec with miniature speaker and electret microphone
- **Input**: 3-button precision resistor ladder (shared on GPIO0 / ADC1 Channel 0)
- **Power Management**: CW2017 I2C fuel gauge for battery SOC and voltage; Type-C port with charge management

For pinouts and hardware design details, see [`docs/hardware-design/AI_HARDWARE_DEVELOPMENT_GUIDE.md`](docs/hardware-design/AI_HARDWARE_DEVELOPMENT_GUIDE.md).

---

## Frequently Asked Questions (FAQ)

<details>
<summary><strong>Q: My computer does not detect the USB serial port?</strong></summary>

1. Ensure your USB-C cable has **data transfer lines**; many phone charging cables are power-only.
2. The ESP32-C3 features native USB-Serial-JTAG. It does not need external CH340 or CP2102 drivers on macOS or modern Linux distributions.
</details>

<details>
<summary><strong>Q: Why does the screen show missing glyphs or boxes?</strong></summary>

The latest firmware includes a bundled OFL font with 30,440 characters covering standard Chinese characters and symbols. Make sure to flash the latest merged binary (`build/<project_name>-full.bin`).
</details>

<details>
<summary><strong>Q: When does the audio alert chime play?</strong></summary>

The chime only plays when a **new user-input prompt**, a **new unread completed task**, or an **error** is detected. Periodic reminders, reconnects, and initial startup synchronization remain completely silent.
</details>

