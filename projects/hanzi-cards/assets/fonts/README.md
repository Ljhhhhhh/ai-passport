<p align="right">
  <a href="README.zh_CN.md">简体中文</a> · <strong>English</strong>
</p>

# Fonts

`main/font_cards_88.c` and `main/font_cards_22.c` are generated 4 bpp subsets of Noto Sans SC under the SIL Open Font License 1.1. The source TTF/OTF is not vendored. Download `NotoSansSC-Regular.otf` from the [Noto CJK](https://github.com/notofonts/noto-cjk) repository, then regenerate:

```bash
python3 tools/make_cards.py --font /path/to/NotoSansSC-Regular.otf
```

If `--font` is omitted, the generator looks for that file under `assets/fonts/` and then for a macOS CJK system font. See [OFL-1.1.txt](OFL-1.1.txt).
