<p align="right">
  <strong>简体中文</strong> · <a href="README.md">English</a>
</p>

# BSP 硬件演示（AI Passport 参考示例）

专为 FoloToy AI Passport (ESP32-C3) 打造的外设硬件验证与功能演示工程。

## 功能特性

- **屏幕显示**：ST7789 240x320 彩色液晶屏、亮度调节、调色板测试与像素吉祥物动画。
- **按键与 ADC**：物理按键检测（上/下/确定）及电池 ADC 采样监测。
- **音频编解码**：ES8311 I2S 编解码器音频播放、音调测试与录音测试。
- **电池电量**：电池电量（SoC）百分比估算与充电状态指示。
- **Wi-Fi**：热点扫描与信号强度可视化列表。
- **蓝牙 BLE**：NimBLE 广播与连接测试。
- **低功耗模式**：浅度睡眠与深度睡眠模式验证。

## 编译与烧录

在仓库根目录下运行：

```bash
# 编译固件
idf.py -C projects/bsp-demo build

# 烧录到设备并查看日志
idf.py -C projects/bsp-demo flash monitor
```

## 运行主机测试

```bash
cc -std=c11 -Wall -Wextra -Werror -Iprojects/bsp-demo/main \
    projects/bsp-demo/tests/test_ui_pixel_math.c \
    projects/bsp-demo/main/ui_pixel_math.c \
    -o /tmp/test_ui_pixel_math && /tmp/test_ui_pixel_math
```
