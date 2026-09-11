<p align="right">
  <a href="README.zh_CN.md">简体中文</a> · <strong>English</strong>
</p>

# Fonts

Store reusable font files and generated font sources here.

- Use descriptive names that include the family, weight, size, and format when relevant.
- Document the source, license, character range, conversion command, and expected destination.
- Check Flash and internal-RAM impact before adding a font; the ESP32-C3 has no PSRAM.
- Do not commit fonts whose license does not permit redistribution.

## Animal Hanzi Story

`main/font_hanzi_80.c` and `main/font_hanzi_24.c` are generated bitmap
subsets of Noto Sans SC under the SIL Open Font License 1.1. The source font is
not vendored. Download `NotoSansSC[wght].ttf` from the Google Fonts repository,
then regenerate the subsets:

```bash
python3 tools/hanzi_make_fonts.py \
  --font /path/to/NotoSansSC-wght.ttf --out-dir main
```

See [OFL-1.1.txt](OFL-1.1.txt) for the font license.
