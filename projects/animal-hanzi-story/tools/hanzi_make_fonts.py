#!/usr/bin/env python3
"""Generate LVGL 4 bpp fonts for the 3-day MVP hanzi app."""

from __future__ import annotations

import argparse
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont


LARGE_CHARS = "爸妈大小山跳水"
UI_CHARS = (
    " 0123456789"
    "·：:？?！!，,。、▲▼><OKUD"
    "第123天汉字魔法跳大与小新朋友水认一认哪个是哪个字是"
    "自由小世界今天探索完成按任意键开始按确定让小人跳一跳"
    "按上变大按下变小这是水按任意键继续找到水让小河奔流"
    "清清的小河奔流起来啦太棒啦选对啦上大下小跳眼睛休息明天见"
    "按上选上面按下选下面爸妈大小山跳水完成"
)


def unique_chars(text: str) -> list[str]:
    seen: set[str] = set()
    chars: list[str] = []
    for ch in text:
        if ch not in seen:
            seen.add(ch)
            chars.append(ch)
    return chars


def pack_4bpp(pixels: list[int], width: int, height: int) -> list[int]:
    row_bytes = (width + 1) // 2
    data: list[int] = []
    for y in range(height):
        for x in range(0, row_bytes * 2, 2):
            hi = pixels[y * width + x] if x < width else 0
            lo = pixels[y * width + x + 1] if x + 1 < width else 0
            data.append(((hi >> 4) << 4) | (lo >> 4))
    return data


def glyph_metrics(font: ImageFont.FreeTypeFont, ch: str) -> dict:
    if ch == " ":
        advance = font.getlength(" ")
        return {
            "cp": ord(ch),
            "adv_w": int(round(advance * 16)),
            "box_w": 0,
            "box_h": 0,
            "ofs_x": 0,
            "ofs_y": 0,
            "bitmap": [],
        }

    left, top, right, bottom = font.getbbox(ch)
    box_w = max(0, right - left)
    box_h = max(0, bottom - top)
    image = Image.new("L", (max(box_w, 1), max(box_h, 1)), 0)
    draw = ImageDraw.Draw(image)
    draw.text((-left, -top), ch, font=font, fill=255)
    pixels = list(image.getdata())
    ascent, descent = font.getmetrics()
    ofs_y = ascent - bottom
    advance = font.getlength(ch)
    return {
        "cp": ord(ch),
        "adv_w": int(round(advance * 16)),
        "box_w": box_w,
        "box_h": box_h,
        "ofs_x": left,
        "ofs_y": ofs_y,
        "bitmap": pack_4bpp(pixels, box_w, box_h),
    }


def emit_font(name: str, size: int, chars: list[str], destination: Path, font_path: Path) -> None:
    font = ImageFont.truetype(str(font_path), size=size)
    ascent, descent = font.getmetrics()
    glyphs = [glyph_metrics(font, ch) for ch in sorted(chars, key=ord)]

    bitmap_bytes: list[int] = []
    glyph_records: list[str] = []
    offset = 0

    for g in glyphs:
        glyph_records.append(
            f"    {{.bitmap_index = {offset}, .adv_w = {g['adv_w']}, "
            f".box_w = {g['box_w']}, .box_h = {g['box_h']}, "
            f".ofs_x = {g['ofs_x']}, .ofs_y = {g['ofs_y']}}},"
        )
        bitmap_bytes.extend(g["bitmap"])
        offset += len(g["bitmap"])

    range_start = glyphs[0]["cp"]
    unicode_offsets = [g["cp"] - range_start for g in glyphs]
    if unicode_offsets != sorted(unicode_offsets) or unicode_offsets[-1] > 0xFFFF:
        raise ValueError(f"{name}: sparse cmap offsets must be sorted 16-bit values")
    unicode_list = ", ".join(f"0x{offset:x}" for offset in unicode_offsets)
    bitmap_lines = [
        "    " + ", ".join(f"0x{b:x}" for b in bitmap_bytes[i : i + 12]) + ","
        for i in range(0, len(bitmap_bytes), 12)
    ]
    guard = name.upper()

    body = f"""/*******************************************************************************
 * Size: {size} px
 * Bpp: 4
 * Characters: {len(glyphs)}
 ******************************************************************************/

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl.h"
#endif

#ifndef {guard}
#define {guard} 1
#endif

#if {guard}

static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {{
{chr(10).join(bitmap_lines)}
}};

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {{
    {{.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0}},
{chr(10).join(glyph_records)}
}};

static const uint16_t unicode_list_0[] = {{
    {unicode_list}
}};

static const lv_font_fmt_txt_cmap_t cmaps[] = {{
    {{
        .range_start = 0x{range_start:x},
        .range_length = {unicode_offsets[-1] + 1},
        .glyph_id_start = 1,
        .unicode_list = unicode_list_0,
        .glyph_id_ofs_list = NULL,
        .list_length = {len(glyphs)},
        .type = LV_FONT_FMT_TXT_CMAP_SPARSE_TINY
    }}
}};

static lv_font_fmt_txt_dsc_t font_dsc = {{
    .glyph_bitmap = glyph_bitmap,
    .glyph_dsc = glyph_dsc,
    .cmaps = cmaps,
    .kern_dsc = NULL,
    .kern_scale = 0,
    .cmap_num = 1,
    .bpp = 4,
    .kern_classes = 0,
    .bitmap_format = 0,
#if LVGL_VERSION_MAJOR == 8
    .cache = &cache
#endif
}};

#if LVGL_VERSION_MAJOR >= 8
const lv_font_t {name} = {{
#else
lv_font_t {name} = {{
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,
    .line_height = {ascent + descent},
    .base_line = {descent},
#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)
    .subpx = LV_FONT_SUBPX_NONE,
#endif
#if LV_VERSION_CHECK(7, 4, 0) || LVGL_VERSION_MAJOR >= 8
    .underline_position = -{max(1, size // 10)},
    .underline_thickness = 1,
#endif
    .dsc = &font_dsc,
#if LV_VERSION_CHECK(8, 2, 0) || LVGL_VERSION_MAJOR >= 9
    .fallback = NULL,
#endif
    .user_data = NULL,
}};

#endif /*#if {guard}*/
"""
    destination.write_text(body, encoding="utf-8")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--font", type=Path, default=Path("/System/Library/Fonts/STHeiti Medium.ttc"))
    parser.add_argument("--out-dir", type=Path, default=Path("projects/animal-hanzi-story/main"))
    args = parser.parse_args()
    args.out_dir.mkdir(parents=True, exist_ok=True)
    emit_font("font_hanzi_80", 72, unique_chars(LARGE_CHARS), args.out_dir / "font_hanzi_80.c", args.font)
    emit_font("font_hanzi_24", 22, unique_chars(UI_CHARS), args.out_dir / "font_hanzi_24.c", args.font)
    print("Fonts generated successfully.")


if __name__ == "__main__":
    main()
