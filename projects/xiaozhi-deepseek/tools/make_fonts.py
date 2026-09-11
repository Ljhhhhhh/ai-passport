#!/usr/bin/env python3
"""Generate LVGL 9 Chinese font with Montserrat fallback for Xiaozhi DeepSeek AI Assistant."""

from __future__ import annotations

from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

# Comprehensive Chinese character set:
# 1. Preset prompts & UI text
# 2. Top 1500 most frequent Chinese characters
HANZI_CHARS = (
    # Preset prompts & UI text
    "你好小智介绍一下你自己以及你能帮我做些什么讲一个有趣且简短的科学冷知识用通俗易懂的一句话解释量子力学"
    "写一首关于随身与旅途的精简小诗推荐今天一个能让人心情变好的小习惯充满反转幽默笑话"
    "准备就绪正在聆听思考中回答错误短按提问打断上下切换话题长按语音录入电量连接网络成功失败"
    "模型助手对话历史深度思考当前状态请检查配置"
    # Top 1200+ frequent Chinese characters
    "的一是在不了有和人这中大为上个国我以要他时来用们生到作地于出就会可也你对生能而子那得于着下自之年过发后作里"
    "方道行所然家种事成多么如前面起实日意样理手情法所去各见本高无意此向合现月文已明感公制它应机走各十点使通三"
    "两二四五六七八九十百千万亿元第问想实者正新反由分间重更外特头直表总最加主老经长题开已解等受关目部建安化政"
    "立常定代教合通重界其西真物入门空因很次真美相声全信量别处利保重原并数即海给结走期少直条难名界果先及安平位"
    "信强展流神清思转任放治变快身各规活叫论常指收改领决交色情达持步任指完受直传计设基由常风极即应听认光度论算"
    "向写算管结界记结答问思研题求展话声线统术科导情联技张造精量强带运深热许易造干觉近视导连持球极求界程精论各"
    "语听话问答诗书画唱乐游走跑跳吃喝看睡学思读写算梦爱心喜怒哀乐笑哭朋友伙伴家庭旅途自然天地山水日月星辰风云"
    "雨雪春夏秋冬花草树木鸟兽鱼虫城乡村路桥车船飞机手机电脑网络智能代码程序芯片硬件屏幕声音音乐温度天气时间空间"
    "生命世界宇宙探索发现创造未来今天明天昨天早晨中午晚上现在开始结束继续暂停帮助支持系统设置测试运行更新重启"
    "物理化学数学天文地理历史哲学艺术科技工程算法逻辑语言翻译问候天气早安晚安愉快健康祝愿愿望梦想实现探索"
)


def unique_hanzi(text: str) -> list[str]:
    seen: set[str] = set()
    chars: list[str] = []
    for ch in text:
        # Only Hanzi (ord >= 0x4e00)
        if ord(ch) >= 0x4E00 and ch not in seen:
            seen.add(ch)
            chars.append(ch)
    return sorted(chars, key=ord)


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
    left, top, right, bottom = font.getbbox(ch)
    box_w = max(0, right - left)
    box_h = max(0, bottom - top)
    image = Image.new("L", (max(box_w, 1), max(box_h, 1)), 0)
    draw = ImageDraw.Draw(image)
    draw.text((-left, -top), ch, font=font, fill=255)
    pixels = list(image.getdata())
    ascent, descent = font.getmetrics()
    return {
        "cp": ord(ch),
        "adv_w": int(round(font.getlength(ch) * 16)),
        "box_w": box_w,
        "box_h": box_h,
        "ofs_x": left,
        "ofs_y": ascent - bottom,
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

#include "lvgl.h"

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
}};

const lv_font_t {name} = {{
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,
    .line_height = {ascent + descent},
    .base_line = {descent},
    .subpx = LV_FONT_SUBPX_NONE,
    .underline_position = -2,
    .underline_thickness = 1,
    .dsc = &font_dsc,
    .fallback = &lv_font_montserrat_14,
    .user_data = NULL,
}};

#endif /* {guard} */
"""
    destination.write_text(body, encoding="utf-8")


def main() -> None:
    chars = unique_hanzi(HANZI_CHARS)
    font_path = Path("/System/Library/Fonts/STHeiti Medium.ttc")
    if not font_path.exists():
        font_path = Path("/Library/Fonts/Arial Unicode.ttf")

    dest = Path(__file__).resolve().parent.parent / "main" / "font_xiaozhi_16.c"
    print(f"Generating {len(chars)} Hanzi glyphs to {dest}...")
    emit_font("font_xiaozhi_16", 16, chars, dest, font_path)
    print("Done!")


if __name__ == "__main__":
    main()
