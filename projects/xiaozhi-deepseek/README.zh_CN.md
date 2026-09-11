<p align="right">
  <strong>简体中文</strong> · <a href="README.md">English</a>
</p>

# 小智 AI 助手 (DeepSeek 内置模型版本)

专为 **FoloToy AI Passport** (ESP32-C3, 8 MB Flash, 无 PSRAM) 定制的小智 AI 语音伴侣固件。本项目采用**模型内置架构**，在固件编译开发阶段直接指定 DeepSeek 作为模型供应商并预置 API Key。烧录后无需通过外部服务器网关中转，无需手机 App 配对扫码绑定模型，开机连网即可直接与 DeepSeek 进行智能对话。

## 功能特性

- **DeepSeek 模型直连内置**：支持直接通过 HTTPS REST + SSE (Server-Sent Events) 流式协议连接 DeepSeek 官方 API (`https://api.deepseek.com/v1/chat/completions`)，支持 `deepseek-chat` 与 `deepseek-reasoner` (DeepSeek-R1 深度思考模式)。
- **开发期一键预置**：在 `sdkconfig` 或 `menuconfig` 中预先填入 `CONFIG_XIAOZHI_DEEPSEEK_API_KEY` 与 Wi-Fi 信息，烧录后即开即用，无需二次配置模型。
- **生动的情绪表达 UI**：小智经典萌趣表情界面，支持眨眼、动态声波条、状态气泡、电池电量计、Wi-Fi 状态图标以及实时打字机流式对话字幕，完美适配 ST7789P3 240x320 彩色液晶屏。
- **音频交互与提示音**：内置纯正弦波包络合成提示音（唤醒、思考、完成、按键音、错误提示）；支持 ES8311 编解码器 16 kHz 16-bit PCM 全双工 I2S 语音采集与播放。
- **超低内存占用优化**：针对 ESP32-C3 仅 400 KB 内部 SRAM（无 PSRAM）量身定制的零拷贝 SSE 流式解析器与 mbedTLS 动态内存分配策略，确保稳定流畅运行。
- **预置话题导航器**：内置多组趣味互动对话与知识提问，支持通过板载物理按键自由切换与发起提问。

## 硬件引脚映射 (FoloToy AI Passport)

| 外设模块 | 芯片 / 接口 | 引脚与参数说明 |
|---|---|---|
| **主控 MCU** | ESP32-C3 | 单核 RISC-V @ 160 MHz，8 MB Flash，400 KB SRAM |
| **彩色显示屏** | ST7789P3 240x320 SPI | MOSI=9, SCLK=8, CS=1, DC=20, BL=21 (PWM 调光) |
| **音频编解码器** | ES8311 (I2C + I2S) | I2C: SDA=10, SCL=7 (地址 0x18); I2S: MCLK=6, BCLK=5, WS=3, DOUT=2, DIN=4 |
| **电量计芯片** | CW2017 | I2C: SDA=10, SCL=7 (地址 0x63) |
| **实体按键** | 三键分压 ADC | GPIO0 (ADC1_CH0): 上键 (0 mV), 下键 (300 mV), 确定键 (595 mV) |

## 编译与配置

在 `projects/xiaozhi-deepseek/sdkconfig.defaults` 中或通过图形化配置菜单预设 DeepSeek API Key 与 Wi-Fi：

```bash
idf.py -C projects/xiaozhi-deepseek menuconfig
```

进入 `Xiaozhi DeepSeek AI Assistant` 菜单：
- `AI Model Configuration (Built-in DeepSeek)`:
  - `Model Provider Name`: `deepseek`
  - `DeepSeek API Key`: 填入你的 API Key (`sk-...`)
  - `DeepSeek API Base URL`: `https://api.deepseek.com/v1`
  - `DeepSeek Model Name`: `deepseek-chat`（或 `deepseek-reasoner`）
  - `System Prompt`: 自定义角色提示词
  - `Temperature`: `7` (代表 0.7)
  - `Max Completion Tokens`: `512`
- `Wi-Fi Configuration`:
  - `Default Wi-Fi SSID`: 预设 Wi-Fi 名称
  - `Default Wi-Fi Password`: 预设 Wi-Fi 密码

## 按键操作说明

- **上键（短按）**：切换至上一个预设话题。
- **下键（短按）**：切换至下一个预设话题。
- **确定键（短按）**：
  - 在 `IDLE`（就绪）状态下：向 DeepSeek 发起当前选中的话题提问。
  - 在 `SPEAKING` / `THINKING`（回答/思考）状态下：打断当前回答并返回就绪状态。
  - 在 `ERROR`（错误）状态下：清除错误提示并返回就绪状态。
- **确定键（长按）**：进入录音聆听模式。

## 编译与烧录

```bash
# 1. 编译固件
idf.py -C projects/xiaozhi-deepseek build

# 2. 烧录并打开串口监视器
idf.py -C projects/xiaozhi-deepseek flash monitor

# 3. 运行静态与主机单元测试
./tools/validate.sh --static
```
