<p align="right">
  <strong>简体中文</strong> · <a href="README.md">English</a>
</p>

# FoloToy AI Passport 项目集

欢迎来到 **FoloToy AI Passport 项目集** —— 专为 FoloToy AI Passport（ESP32-C3 掌上可穿戴 AI 硬件）打造的多工程与固件合集代码仓库。

FoloToy AI Passport 是一款精巧便携的嵌入式 AI 硬件卡片，配备 240×320 高清彩色液晶屏、ES8311 音频编解码芯片（带扬声器与麦克风）、3 枚物理按键、CW2017 高精度电池电量计以及蓝牙 BLE 通信能力。它既能作为桌面端的个人 AI 伴侣，也是探索嵌入式与大语言模型结合的理想平台。

本仓库采用多工程（Monorepo）架构：所有项目共享底层的板级支持包（`components/bsp`）、硬件规范与工程工具，而各个独立的应用项目则放置在 `projects/` 目录下，互不干扰、各自独立构建。

---

## 包含项目一览

| 项目名称 | 路径 | 功能简介 | 适用场景 |
| :--- | :--- | :--- | :--- |
| **Codex 伴侣护照** | [`projects/codex-passport`](projects/codex-passport) | 实时监控 Codex Agent 任务、待回复提问、未读完成通知与账号额度，支持 Mac 自动 BLE 同步与声光提醒 | 极客开发、多任务监控、桌面智能副屏 |
| **动物汉字故事** | [`projects/animal-hanzi-story`](projects/animal-hanzi-story) | 专为儿童打造的识字冒险交互故事，包含真人语音发音与定制点阵字库 | 儿童互动早教、识字游戏 |
| **汉字卡片** | [`projects/hanzi-cards`](projects/hanzi-cards) | 互动字卡点读与学习应用，支持本地音频发音与 NVS 学习进度断电保存 | 汉字点读、卡片认字 |
| **小智 DeepSeek 助手** | [`projects/xiaozhi-deepseek`](projects/xiaozhi-deepseek) | 连接 DeepSeek 大语言模型的随身 AI 语音助手，支持流式对话与显示 | 随身智能问答、语音助理 |
| **BSP 硬件演示** | [`projects/bsp-demo`](projects/bsp-demo) | 完整的外设硬件测试套件与参考菜单（屏幕、音频、按键、电池、Wi-Fi、BLE 与低功耗） | 硬件出厂自检、驱动开发参考 |

---

## 新手零基础上手指南

你可以选择以下两种方式之一将应用安装到你的 AI Passport 设备上：

### 方式一：网页一键烧录（免安装开发环境，最推荐新手）

如果你只想体验现有固件，无需配置复杂的开发环境：

1. **连接硬件**：使用一根具备**数据传输功能**的 USB Type-C 数据线（请勿使用仅能供电的纯充电线），将 AI Passport 插入电脑。
2. **打开网页烧录工具**：使用 Chrome 或 Edge 浏览器打开 [ESP Web Flasher 网页烧录工具](https://espressif.github.io/esptool-js/)。
3. **选择固件文件**：获取对应项目编译生成的合并固件文件（位于 `build/<项目名>-full.bin`，例如 `build/codex-passport-full.bin`）。
4. **配置参数并烧录**：
   - 波特率选择：`460800`
   - 烧录起始地址填写：`0x0`
   - 点击 **Connect** 按钮并选中连接的 USB 串口，然后点击 **Program** 开始写入。写入完成后设备将自动重启并运行。

---

### 方式二：本地源码编译与烧录（适合开发者）

如果你需要修改代码、调整界面或添加新功能：

#### 1. 准备开发环境

请安装并激活 **ESP-IDF 5.5.3** 环境（其他版本可能存在兼容性差异）：

```bash
# 激活 ESP-IDF 5.5.3
get_idf553
# 或通过官方 export 脚本激活
. $IDF_PATH/export.sh
```

#### 2. 编译指定项目

在仓库根目录下，通过 `-C` 参数指定要编译的目标项目子目录：

```bash
# 编译 Codex 伴侣护照
idf.py -C projects/codex-passport build

# 或编译动物汉字故事
idf.py -C projects/animal-hanzi-story build
```

#### 3. 烧录固件与打开串口监控

将设备通过 USB 连接电脑后执行：

```bash
# 烧录并实时查看串口日志输出
idf.py -C projects/codex-passport flash monitor
```

> 退出串口监视器请按快捷键 `Ctrl + ]`。

---

## 硬件交互与按键说明

AI Passport 正面配备三枚轻触按键，采用单引脚（GPIO0）电阻分压检测方案：

| 按键位置 | 基础操作 | 菜单/常规页面 | 消息/列表页面 |
| :--- | :--- | :--- | :--- |
| **上键 (UP)** | 短按 | 向上翻页 / 切换上一项 | 向上滚动或切换页面 |
| **下键 (DOWN)** | 短按 | 向下翻页 / 切换下一项 | 向下滚动或切换页面 |
| **确定键 (OK)** | 短按 | 进入选中的功能 / 切换视图 | 查看下一组三条消息 |
| **确定键 (OK)** | 熄屏时单击 | **唤醒屏幕**（亮屏 30 秒，不触发点击动作） | **唤醒屏幕** |

### 屏幕休眠与节能逻辑

- **自动熄屏**：当设备没有未读的待处理事项，且持续 30 秒无按键操作时，背光将自动平滑熄灭以节约电量。
- **消息常亮**：当存在未读已完成任务、待回复提问或蓝牙处于未连接状态时，设备屏幕将保持常亮，确保重要状态不被遗漏。
- **声光提醒**：当检测到新的待回复提问、新完成的任务或执行失败时，设备会发出柔和的短促双音提示，既醒目又不会造成打扰。

---

## 创建属于你的新项目

仓库提供了完善的项目生成脚本，只需一行命令即可生成配置好底层驱动的新工程：

```bash
# 使用脚手架一键生成工程模板
python3 tools/create_project.py my-ai-app --title "我的第一个应用"

# 立即进入工程并进行编译
idf.py -C projects/my-ai-app build
```

生成的新项目位于 `projects/my-ai-app`，自带独立的 `main/`、构建脚本以及完整的项目文档骨架。

---

## 工程质量与验证门禁

在提交代码或合并更新前，可通过统一脚本进行全方位的质量检查：

```bash
# 1. 快速检查：文档规范、链接有效性、代码格式与主机单元测试（无需真实硬件）
./tools/validate.sh --static

# 2. 固件编译：单独编译并验证指定项目的固件与分区表布局
./tools/validate.sh --firmware codex-passport

# 3. 完整门禁：运行全部静态测试与所有项目的编译校验
./tools/validate.sh
```

---

## 硬件规格速查

- **主控芯片**：ESP32-C3FH4（32 位 RISC-V 单核处理器，主频 160 MHz）
- **板载存储**：8 MB SPI Flash，无外挂 PSRAM
- **显示屏**：ST7789 240×320 竖屏 RGB565 IPS 液晶屏（SPI2 40 MHz，LEDC 硬件 PWM 背光调节）
- **音频架构**：ES8311 全双工 I2S 音频编解码芯片，集成高保真微型扬声器与驻极体麦克风
- **按键设计**：3 键高精度电阻分压梯形网络（共用 GPIO0 / ADC1 通道 0）
- **电源管理**：CW2017 I2C 库仑计芯片，实时监测电量百分比与电压；标准 Type-C 接口支持充放电

详细硬件接口引脚分配与电路设计规范可参阅 [`docs/hardware-design/AI_HARDWARE_DEVELOPMENT_GUIDE.zh_CN.md`](docs/hardware-design/AI_HARDWARE_DEVELOPMENT_GUIDE.zh_CN.md)。

---

## 常见问题与排查 (FAQ)

<details>
<summary><strong>Q: 电脑无法识别到设备串口？</strong></summary>

1. 确认使用的是具有**数据传输能力**的 USB Type-C 数据线，很多手机随附的仅是纯充电线。
2. ESP32-C3 内置原生 USB-Serial-JTAG 控制器，无需额外安装 CH340 或 CP2102 驱动。在 macOS / Linux 下插入即可直接识别出设备节点（例如 `/dev/cu.usbmodem*`）。
</details>

<details>
<summary><strong>Q: 屏幕文字出现方框或缺字乱码？</strong></summary>

最新的固件已内置包含 30,440 个字符的全量中文字库（基于思源黑体 OFL 许可），支持完整常用汉字与符号。如遇到字库问题，请确保使用最新的 `build/<项目名>-full.bin` 完整合并镜像重新烧录。
</details>

<details>
<summary><strong>Q: 声音什么时候会响？会打扰工作吗？</strong></summary>

设备采用事件触发式提醒机制：只有当有**新的待用户回复提问**、**新完成且未读的任务**或**任务异常报错**时，才会发出一次轻柔的 C5/E5 双音。启动时的旧消息同步、翻页、蓝牙重连均保持完全静默。
</details>

