<p align="right">
  <strong>简体中文</strong> · <a href="README.md">English</a>
</p>

# 字库资源（Fonts）

本目录存放项目可复用的字库资源。每个字库子目录或单个字库文件，应附说明。

## 如何使用

- 字库文件（如 `.ttf`、`.otf`、LVGL 使用的 C 数组字库等）复制到本目录，并在本项目 `README.md` 记录字名、字号、支持字符集与版权信息。
- 若需集成到 ESP-IDF 固件，参考 [`components/bsp/include/bsp_display.h`](../../../../components/bsp/include/bsp_display.h) 与 LVGL 字体接口，将字库转换为对应格式并放入正确资源目录。
- 字库占用 Flash 与内存，需在集成前评估 ESP32-C3 无 PSRAM 的限制（详见 `docs/hardware-design/AI_HARDWARE_DEVELOPMENT_GUIDE.md`）。

## 动物字乐园

`main/font_hanzi_80.c` 与 `main/font_hanzi_24.c` 是 Noto Sans SC 的位图
子集，遵循 SIL Open Font License 1.1；仓库不保存源字体。请从 Google Fonts
仓库下载 `NotoSansSC[wght].ttf`，再生成子集：

```bash
python3 tools/hanzi_make_fonts.py \
  --font /path/to/NotoSansSC-wght.ttf --out-dir main
```

字体许可证见 [OFL-1.1.txt](OFL-1.1.txt)。
