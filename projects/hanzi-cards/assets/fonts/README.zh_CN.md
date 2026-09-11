<p align="right">
  <strong>简体中文</strong> · <a href="README.md">English</a>
</p>

# 字体

`main/font_cards_88.c` 与 `main/font_cards_22.c` 是 Noto Sans SC 的 4 bpp 子集，遵循 SIL Open Font License 1.1。源 TTF/OTF 不入库。从 [Noto CJK](https://github.com/notofonts/noto-cjk) 下载 `NotoSansSC-Regular.otf` 后重新生成：

```bash
python3 tools/make_cards.py --font /path/to/NotoSansSC-Regular.otf
```

未指定 `--font` 时，生成器会先找 `assets/fonts/` 下的该文件，再回退到 macOS 中文字体。许可证见 [OFL-1.1.txt](OFL-1.1.txt)。
