#!/usr/bin/env python3
"""Validate the card list and generate fonts, I4 art, IMA voice, and the C catalog."""

from __future__ import annotations

import argparse
import json
import os
import struct
import subprocess
import sys
import tempfile
import wave
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parent.parent
MANIFEST = ROOT / "assets" / "cards" / "cards.json"
PNG_DIR = ROOT / "assets" / "cards" / "png"
I4_DIR = ROOT / "main" / "assets" / "cards"
IMA_DIR = I4_DIR
FONT_OUT = ROOT / "main"
REF_PATH = ROOT / "assets" / "cards" / "reference-warm-picturebook.png"
IMAGE_W, IMAGE_H = 200, 164
STRIDE = IMAGE_W // 2
PALETTE_BYTES = 16 * 4

CREAM = (246, 235, 216)
INK = (60, 47, 35)
SKIN = (232, 196, 164)
HAIR = (42, 32, 28)
PEACH = (232, 154, 122)
YELLOW = (240, 196, 96)
RED = (214, 90, 74)
GREEN = (122, 168, 98)
BLUE = (122, 168, 196)
BROWN = (150, 102, 70)
WOOD = (186, 138, 90)
WHITE = (250, 246, 238)
ORANGE = (232, 140, 72)
PINK = (236, 176, 176)
SKY = (186, 214, 224)
NIGHT = (92, 110, 148)
GRASS = (142, 176, 98)

MAGIC = 0x34414D49
STEP_TABLE = [
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
    50, 55, 60, 66, 73, 80, 88, 97, 107, 118, 130, 143, 157, 173, 190, 209, 230,
    253, 279, 307, 337, 371, 408, 449, 494, 544, 598, 658, 724, 796, 876, 963,
    1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024, 3327,
    3660, 4026, 4428, 4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442,
    11487, 12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794,
    32767,
]
INDEX_TABLE = [-1, -1, -1, -1, 2, 4, 6, 8, -1, -1, -1, -1, 2, 4, 6, 8]


def load_cards() -> list[dict]:
    data = json.loads(MANIFEST.read_text(encoding="utf-8"))
    cards = data["cards"]
    if len(cards) != 50:
        raise SystemExit(f"cards.json must have 50 items, got {len(cards)}")
    ids = [c["id"] for c in cards]
    hanzi = [c["hanzi"] for c in cards]
    if len(set(ids)) != 50 or len(set(hanzi)) != 50:
        raise SystemExit("card id and hanzi must be unique")
    for card in cards:
        if not card["phrase"] or not card["prompt"]:
            raise SystemExit(f"{card['id']}: phrase and prompt required")
        if len(card["hanzi"]) != 1:
            raise SystemExit(f"{card['id']}: hanzi must be one character")
    return cards


def paper() -> tuple[Image.Image, ImageDraw.ImageDraw]:
    img = Image.new("RGB", (IMAGE_W, IMAGE_H), CREAM)
    return img, ImageDraw.Draw(img)


def person(draw: ImageDraw.ImageDraw, cx: int, cy: int, scale: float, shirt, hair=HAIR, facing=1) -> None:
    s = scale
    head_r = max(8, int(14 * s))
    head_cy = cy - int(50 * s)
    draw.ellipse((cx - head_r, head_cy - head_r - int(6 * s), cx + head_r, head_cy + int(2 * s)), fill=hair)
    draw.ellipse((cx - head_r, head_cy - head_r, cx + head_r, head_cy + head_r), fill=SKIN)
    eye = max(2, int(2 * s))
    draw.ellipse((cx - int(6 * s) - eye, head_cy - eye, cx - int(6 * s) + eye, head_cy + eye), fill=INK)
    draw.ellipse((cx + int(6 * s) - eye, head_cy - eye, cx + int(6 * s) + eye, head_cy + eye), fill=INK)
    draw.rounded_rectangle((cx - int(12 * s), cy - int(36 * s), cx + int(12 * s), cy - int(10 * s)), radius=6, fill=shirt)
    left = facing < 0
    arm_y0, arm_y1 = cy - int(32 * s), cy - int(16 * s)
    draw.rounded_rectangle((cx - int(20 * s), arm_y0, cx - int(12 * s), arm_y1 - (4 if left else 0)), radius=4, fill=SKIN)
    draw.rounded_rectangle((cx + int(12 * s), arm_y0, cx + int(20 * s), arm_y1 - (4 if not left else 0)), radius=4, fill=SKIN)
    draw.rectangle((cx - int(9 * s), cy - int(10 * s), cx - int(3 * s), cy), fill=BROWN)
    draw.rectangle((cx + int(3 * s), cy - int(10 * s), cx + int(9 * s), cy), fill=BROWN)


def ground(draw: ImageDraw.ImageDraw, color=GRASS, y=140) -> None:
    draw.rectangle((0, y, IMAGE_W, IMAGE_H), fill=color)


def draw_card(card_id: str) -> Image.Image:
    img, d = paper()
    if card_id == "ba":
        ground(d, WOOD, 150)
        person(d, 100, 150, 1.3, BLUE)
    elif card_id == "ma":
        ground(d, WOOD, 150)
        person(d, 100, 150, 1.25, PEACH)
    elif card_id == "bao":
        ground(d, WOOD, 142)
        person(d, 90, 142, 0.72, YELLOW)
        d.ellipse((118, 108, 152, 142), fill=PEACH)
    elif card_id == "ren":
        ground(d, WOOD, 142)
        person(d, 58, 142, 1.0, BLUE)
        person(d, 100, 142, 0.7, YELLOW)
        person(d, 142, 142, 1.0, PEACH)
    elif card_id == "shou":
        d.ellipse((36, 48, 96, 120), fill=SKIN)
        d.ellipse((108, 48, 168, 120), fill=SKIN)
    elif card_id == "yan":
        d.ellipse((50, 28, 150, 150), fill=SKIN)
        d.ellipse((70, 70, 98, 98), fill=WHITE)
        d.ellipse((106, 70, 134, 98), fill=WHITE)
        d.ellipse((80, 80, 92, 92), fill=INK)
        d.ellipse((116, 80, 128, 92), fill=INK)
    elif card_id == "er":
        d.ellipse((70, 24, 160, 150), fill=SKIN)
        d.ellipse((48, 70, 86, 118), fill=SKIN)
        d.ellipse((58, 82, 78, 108), fill=PEACH)
    elif card_id == "tou":
        d.ellipse((55, 40, 145, 140), fill=HAIR)
        d.ellipse((70, 70, 130, 140), fill=SKIN)
    elif card_id == "jiao":
        ground(d, WOOD, 90)
        d.ellipse((48, 92, 96, 132), fill=SKIN)
        d.ellipse((108, 92, 156, 132), fill=SKIN)
    elif card_id == "ya":
        d.ellipse((50, 28, 150, 150), fill=SKIN)
        d.arc((70, 80, 130, 130), 20, 160, fill=INK, width=4)
        d.rectangle((88, 96, 98, 110), fill=WHITE)
        d.rectangle((102, 96, 112, 110), fill=WHITE)
    elif card_id == "men":
        ground(d, GREEN, 130)
        d.rounded_rectangle((60, 28, 140, 140), radius=8, fill=WOOD)
        d.ellipse((96, 80, 112, 96), fill=YELLOW)
        d.rectangle((88, 36, 112, 56), fill=SKY)
    elif card_id == "deng":
        d.rectangle((0, 0, IMAGE_W, IMAGE_H), fill=NIGHT)
        d.polygon([(100, 20), (70, 70), (130, 70)], fill=YELLOW)
        d.rectangle((94, 70, 106, 130), fill=WOOD)
        d.ellipse((78, 62, 122, 86), fill=(255, 230, 150))
    elif card_id == "chuang":
        ground(d, WOOD, 130)
        d.rounded_rectangle((40, 70, 160, 130), radius=10, fill=WOOD)
        d.rounded_rectangle((48, 78, 152, 108), fill=PEACH, radius=8)
        d.ellipse((70, 82, 110, 104), fill=WHITE)
    elif card_id == "bei":
        ground(d, WOOD, 120)
        d.rounded_rectangle((78, 50, 122, 118), radius=16, fill=BLUE)
        d.rectangle((86, 58, 114, 90), fill=SKY)
    elif card_id == "shu":
        ground(d, WOOD, 126)
        d.polygon([(70, 50), (130, 50), (140, 118), (60, 118)], fill=WHITE)
        d.rectangle((64, 58, 136, 66), fill=PEACH)
        d.rectangle((64, 74, 120, 80), fill=BROWN)
    elif card_id == "che":
        ground(d, (210, 196, 170), 110)
        d.rounded_rectangle((40, 70, 160, 110), radius=12, fill=RED)
        d.polygon([(70, 48), (120, 48), (140, 70), (55, 70)], fill=SKY)
        d.ellipse((55, 96, 85, 126), fill=INK)
        d.ellipse((120, 96, 150, 126), fill=INK)
    elif card_id == "qiu":
        ground(d, GRASS, 118)
        d.ellipse((64, 46, 136, 118), fill=RED)
        d.arc((64, 46, 136, 118), 200, 340, fill=WHITE, width=3)
    elif card_id == "xie":
        ground(d, WOOD, 120)
        d.rounded_rectangle((36, 80, 96, 118), radius=16, fill=YELLOW)
        d.rounded_rectangle((108, 80, 168, 118), radius=16, fill=YELLOW)
        d.ellipse((44, 70, 70, 92), fill=YELLOW)
        d.ellipse((116, 70, 142, 92), fill=YELLOW)
    elif card_id == "fan":
        ground(d, WOOD, 120)
        d.ellipse((60, 70, 140, 130), fill=WHITE)
        d.ellipse((72, 82, 128, 118), fill=(240, 230, 200))
        d.line((150, 50, 150, 120), fill=BROWN, width=4)
    elif card_id == "nai":
        ground(d, WOOD, 120)
        d.polygon([(80, 46), (120, 46), (130, 118), (70, 118)], fill=WHITE)
        d.rectangle((88, 54, 112, 70), fill=CREAM)
    elif card_id == "mao":
        ground(d, WOOD, 148)
        d.ellipse((70, 58, 140, 128), fill=ORANGE)
        d.polygon([(78, 62), (70, 28), (96, 58)], fill=ORANGE)
        d.polygon([(114, 58), (140, 28), (132, 62)], fill=ORANGE)
        d.ellipse((88, 78, 100, 90), fill=INK)
        d.ellipse((112, 78, 124, 90), fill=INK)
    elif card_id == "gou":
        ground(d, GRASS, 148)
        d.ellipse((50, 80, 140, 132), fill=BROWN)
        d.ellipse((120, 58, 164, 104), fill=BROWN)
        d.ellipse((140, 72, 164, 104), fill=PEACH)
        d.ellipse((132, 72, 142, 84), fill=INK)
    elif card_id == "yu":
        d.rectangle((0, 0, IMAGE_W, IMAGE_H), fill=SKY)
        d.polygon([(50, 82), (130, 50), (130, 114)], fill=ORANGE)
        d.ellipse((118, 70, 162, 102), fill=ORANGE)
        d.ellipse((148, 80, 156, 88), fill=INK)
        d.polygon([(50, 82), (30, 60), (30, 104)], fill=PEACH)
    elif card_id == "niao":
        d.rectangle((0, 0, IMAGE_W, 110), fill=SKY)
        ground(d, GREEN, 110)
        d.ellipse((80, 58, 140, 100), fill=(90, 140, 170))
        d.polygon([(140, 74), (164, 82), (140, 90)], fill=ORANGE)
        d.ellipse((118, 70, 128, 80), fill=INK)
        d.line((40, 110, 160, 110), fill=BROWN, width=4)
    elif card_id == "chong":
        ground(d, GREEN, 90)
        d.ellipse((40, 70, 70, 100), fill=(110, 168, 90))
        d.ellipse((66, 66, 98, 98), fill=(110, 168, 90))
        d.ellipse((94, 70, 124, 100), fill=(110, 168, 90))
        d.line((48, 70, 40, 50), fill=INK, width=2)
        d.line((62, 70, 70, 50), fill=INK, width=2)
    elif card_id == "ma_animal":
        ground(d, GRASS, 120)
        d.ellipse((50, 70, 140, 120), fill=BROWN)
        d.rectangle((128, 48, 148, 90), fill=BROWN)
        d.ellipse((138, 40, 168, 70), fill=BROWN)
        d.rectangle((62, 112, 74, 140), fill=INK)
        d.rectangle((110, 112, 122, 140), fill=INK)
    elif card_id == "niu":
        ground(d, GRASS, 120)
        d.ellipse((48, 70, 150, 124), fill=(196, 160, 110))
        d.ellipse((140, 56, 176, 92), fill=(196, 160, 110))
        d.polygon([(148, 52), (140, 36), (156, 56)], fill=(196, 160, 110))
        d.polygon([(168, 52), (180, 36), (172, 58)], fill=(196, 160, 110))
    elif card_id == "yang":
        ground(d, GRASS, 120)
        d.ellipse((55, 70, 145, 124), fill=WHITE)
        d.ellipse((130, 58, 162, 92), fill=WHITE)
        d.ellipse((148, 70, 156, 78), fill=INK)
        d.ellipse((70, 96, 90, 116), fill=PINK)
    elif card_id == "tu":
        ground(d, GRASS, 126)
        d.ellipse((70, 80, 140, 130), fill=WHITE)
        d.ellipse((118, 56, 148, 92), fill=WHITE)
        d.ellipse((86, 40, 102, 88), fill=WHITE)
        d.ellipse((108, 36, 124, 88), fill=WHITE)
        d.ellipse((92, 48, 98, 78), fill=PINK)
        d.ellipse((114, 46, 120, 76), fill=PINK)
    elif card_id == "xiong":
        ground(d, GRASS, 126)
        d.ellipse((60, 70, 150, 130), fill=BROWN)
        d.ellipse((80, 48, 130, 92), fill=BROWN)
        d.ellipse((78, 40, 96, 58), fill=BROWN)
        d.ellipse((114, 40, 132, 58), fill=BROWN)
        d.ellipse((96, 70, 114, 86), fill=INK)
    elif card_id == "shan":
        d.rectangle((0, 0, IMAGE_W, IMAGE_H), fill=SKY)
        d.polygon([(0, 150), (70, 40), (140, 150)], fill=GREEN)
        d.polygon([(70, 150), (140, 30), (200, 150)], fill=(90, 140, 90))
        ground(d, GRASS, 148)
    elif card_id == "shui":
        ground(d, WOOD, 140)
        person(d, 86, 140, 0.85, YELLOW)
        d.rounded_rectangle((118, 88, 150, 130), radius=12, fill=BLUE)
    elif card_id == "tian":
        d.rectangle((0, 0, IMAGE_W, IMAGE_H), fill=BLUE)
        d.ellipse((30, 30, 80, 60), fill=WHITE)
        d.ellipse((110, 50, 170, 86), fill=WHITE)
    elif card_id == "yue":
        d.rectangle((0, 0, IMAGE_W, IMAGE_H), fill=NIGHT)
        d.ellipse((70, 36, 140, 106), fill=YELLOW)
        d.ellipse((92, 36, 162, 96), fill=NIGHT)
    elif card_id == "yu_rain":
        d.rectangle((0, 0, IMAGE_W, IMAGE_H), fill=NIGHT)
        ground(d, GREEN, 120)
        d.rounded_rectangle((70, 70, 130, 120), radius=6, fill=WOOD)
        for x in range(20, 190, 16):
            d.line((x, 10, x - 8, 50), fill=SKY, width=2)
    elif card_id == "hua":
        ground(d, GRASS, 126)
        d.line((100, 70, 100, 130), fill=GREEN, width=4)
        for box in [(70, 40, 110, 80), (90, 40, 130, 80), (70, 60, 110, 100), (90, 60, 130, 100)]:
            d.ellipse(box, fill=PEACH)
        d.ellipse((92, 62, 108, 78), fill=YELLOW)
    elif card_id == "shu_tree":
        d.rectangle((0, 0, IMAGE_W, 130), fill=SKY)
        ground(d, GRASS, 126)
        d.rectangle((92, 80, 108, 140), fill=BROWN)
        d.ellipse((50, 20, 150, 110), fill=GREEN)
    elif card_id == "hong":
        ground(d, WOOD, 126)
        d.ellipse((70, 46, 130, 114), fill=RED)
        d.line((100, 40, 100, 52), fill=BROWN, width=3)
        d.ellipse((100, 34, 118, 50), fill=GREEN)
    elif card_id == "huang":
        ground(d, WOOD, 126)
        d.polygon([(40, 90), (150, 50), (160, 70), (50, 110)], fill=YELLOW)
        d.ellipse((150, 48, 168, 66), fill=BROWN)
    elif card_id == "lan":
        d.rectangle((0, 0, IMAGE_W, IMAGE_H), fill=BLUE)
        d.ellipse((20, 24, 70, 54), fill=WHITE)
        d.ellipse((130, 70, 150, 86), fill=(90, 140, 170))
    elif card_id in ("da", "xiao"):
        ground(d, GRASS, 118)
        r = 54 if card_id == "da" else 16
        cx, cy = 100, 90
        d.ellipse((cx - r, cy - r, cx + r, cy + r), fill=RED)
    elif card_id in ("shang", "xia"):
        ground(d, WOOD, 140)
        for i, y in enumerate((110, 86, 62, 38)):
            d.rectangle((40 + i * 18, y, 180, y + 18), fill=(210, 170, 120))
        y = 70 if card_id == "shang" else 108
        person(d, 70 if card_id == "shang" else 140, y, 0.55, YELLOW)
    elif card_id in ("kai", "guan"):
        ground(d, GREEN, 130)
        d.rounded_rectangle((60, 28, 140, 140), radius=8, fill=WOOD)
        if card_id == "kai":
            d.polygon([(140, 28), (180, 44), (180, 140), (140, 140)], fill=(210, 170, 120))
            d.rectangle((70, 48, 110, 120), fill=YELLOW)
        else:
            d.ellipse((96, 80, 112, 96), fill=YELLOW)
    elif card_id in ("chi", "he"):
        ground(d, WOOD, 140)
        person(d, 80, 140, 0.85, YELLOW)
        if card_id == "chi":
            d.ellipse((118, 104, 158, 136), fill=WHITE)
            d.ellipse((126, 112, 150, 130), fill=(240, 230, 200))
        else:
            d.rounded_rectangle((118, 88, 150, 130), radius=12, fill=BLUE)
    elif card_id in ("pao", "tiao"):
        d.rectangle((0, 0, IMAGE_W, 140), fill=SKY)
        ground(d, GRASS, 140)
        y = 118 if card_id == "tiao" else 140
        person(d, 100, y, 0.9, RED, facing=1 if card_id == "pao" else 0)
        if card_id == "tiao":
            d.ellipse((88, 148, 112, 158), fill=(180, 200, 140))
    else:
        raise SystemExit(f"missing illustration for {card_id}")
    return img


def to_i4(img: Image.Image) -> bytes:
    img = img.convert("RGB").resize((IMAGE_W, IMAGE_H), Image.Resampling.NEAREST)
    pal = img.quantize(colors=16, method=Image.Quantize.MEDIANCUT, dither=Image.Dither.NONE)
    raw = pal.getpalette() or []
    pixels = list(pal.getdata())
    out = bytearray()
    for i in range(16):
        if (i * 3 + 2) < len(raw):
            r, g, b = raw[i * 3], raw[i * 3 + 1], raw[i * 3 + 2]
        else:
            r = g = b = 0
        out.extend((b, g, r, 255))
    for y in range(IMAGE_H):
        row = pixels[y * IMAGE_W : (y + 1) * IMAGE_W]
        for x in range(0, IMAGE_W, 2):
            hi = row[x] & 0x0F
            lo = row[x + 1] & 0x0F
            out.append((hi << 4) | lo)
    expected = PALETTE_BYTES + STRIDE * IMAGE_H
    if len(out) != expected:
        raise SystemExit(f"I4 size {len(out)} != {expected}")
    return bytes(out)


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
        return {"cp": ord(ch), "adv_w": int(round(advance * 16)), "box_w": 0, "box_h": 0, "ofs_x": 0, "ofs_y": 0, "bitmap": []}
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
    .underline_position = -{max(1, size // 10)},
    .underline_thickness = 1,
    .dsc = &font_dsc,
    .fallback = NULL,
    .user_data = NULL,
}};

#endif
"""
    destination.write_text(body, encoding="utf-8")


def clamp_i16(value: int) -> int:
    return max(-32768, min(32767, value))


def encode_nibble(predictor: int, step_index: int, sample: int) -> tuple[int, int, int]:
    step = STEP_TABLE[step_index]
    diff = sample - predictor
    nibble = 0
    if diff < 0:
        nibble = 8
        diff = -diff
    predicted = step >> 3
    if diff >= step:
        nibble |= 4
        predicted += step
        diff -= step
    if diff >= (step >> 1):
        nibble |= 2
        predicted += step >> 1
        diff -= step >> 1
    if diff >= (step >> 2):
        nibble |= 1
        predicted += step >> 2
    predictor = clamp_i16(predictor - predicted if nibble & 8 else predictor + predicted)
    step_index = max(0, min(88, step_index + INDEX_TABLE[nibble]))
    return nibble, predictor, step_index


def encode_pcm(samples: list[int]) -> bytes:
    packed = bytearray((len(samples) + 1) // 2)
    predictor = 0
    step_index = 0
    for index, sample in enumerate(samples):
        nibble, predictor, step_index = encode_nibble(predictor, step_index, sample)
        if index % 2 == 0:
            packed[index // 2] = nibble
        else:
            packed[index // 2] |= nibble << 4
    return struct.pack("<IIhbb", MAGIC, len(samples), 0, 0, 0) + bytes(packed)


def read_wav(path: Path) -> list[int]:
    with wave.open(str(path), "rb") as wav_file:
        if wav_file.getnchannels() != 1 or wav_file.getsampwidth() != 2 or wav_file.getframerate() != 16000:
            raise SystemExit(f"{path} must be 16 kHz 16-bit mono")
        frames = wav_file.readframes(wav_file.getnframes())
    return list(struct.unpack("<" + "h" * (len(frames) // 2), frames))


def find_font(explicit: Path | None) -> Path:
    candidates = []
    if explicit:
        candidates.append(explicit)
    candidates.extend(
        [
            ROOT / "assets" / "fonts" / "NotoSansSC-Regular.otf",
            ROOT / "assets" / "fonts" / "NotoSansSC-Regular.ttf",
            Path("/Library/Fonts/NotoSansSC-Regular.otf"),
            Path("/System/Library/Fonts/STHeiti Medium.ttc"),
            Path("/System/Library/Fonts/Hiragino Sans GB.ttc"),
        ]
    )
    for path in candidates:
        if path.is_file():
            return path
    raise SystemExit("Noto Sans SC or a CJK system font is required; pass --font")


def write_catalog(cards: list[dict]) -> None:
    decls = []
    entries = []
    for card in cards:
        ident = card["id"]
        image_len = (I4_DIR / f"{ident}.i4").stat().st_size
        voice_len = (IMA_DIR / f"{ident}.ima").stat().st_size
        decls.append(
            f"extern const uint8_t s_{ident}_i4_start[] asm(\"_binary_{ident}_i4_start\");\n"
            f"extern const uint8_t s_{ident}_ima_start[] asm(\"_binary_{ident}_ima_start\");"
        )
        entries.append(
            "    {\n"
            f"        .id = \"{ident}\",\n"
            f"        .hanzi = \"{card['hanzi']}\",\n"
            f"        .phrase = \"{card['phrase']}\",\n"
            f"        .image = s_{ident}_i4_start,\n"
            f"        .image_len = {image_len}U,\n"
            f"        .voice = s_{ident}_ima_start,\n"
            f"        .voice_len = {voice_len}U\n"
            "    }"
        )
    body = (
        '#include "cards_catalog.h"\n\n'
        + "\n".join(decls)
        + "\n\nconst cards_entry_t cards_catalog[CARDS_COUNT] = {\n"
        + ",\n".join(entries)
        + "\n};\n\n"
        + "int cards_catalog_get(uint8_t index, const cards_entry_t **entry)\n"
        + "{\n"
        + "    if (!entry || index >= CARDS_COUNT) {\n"
        + "        return 0;\n"
        + "    }\n"
        + "    *entry = &cards_catalog[index];\n"
        + "    return 1;\n"
        + "}\n"
    )
    (FONT_OUT / "cards_catalog.c").write_text(body, encoding="utf-8")


def make_voice(cards: list[dict], voice: str, rate: int) -> None:
    IMA_DIR.mkdir(parents=True, exist_ok=True)
    for card in cards:
        text = f"{card['hanzi']}。{card['phrase']}。"
        fd, aiff_name = tempfile.mkstemp(suffix=".aiff")
        os.close(fd)
        aiff = Path(aiff_name)
        wav_fd, wav_name = tempfile.mkstemp(suffix=".wav")
        os.close(wav_fd)
        wav = Path(wav_name)
        ima = IMA_DIR / f"{card['id']}.ima"
        try:
            subprocess.run(["say", "-v", voice, "-r", str(rate), "-o", str(aiff), text], check=True)
            subprocess.run(
                ["afconvert", "-f", "WAVE", "-d", "LEI16@16000", "-c", "1", str(aiff), str(wav)],
                check=True,
            )
            ima.write_bytes(encode_pcm(read_wav(wav)))
        finally:
            aiff.unlink(missing_ok=True)
            wav.unlink(missing_ok=True)
        print(f"{card['id']}: {ima.stat().st_size} bytes")


def make_reference(cards: list[dict], font_path: Path) -> None:
    screen = Image.new("RGB", (240, 320), CREAM)
    art = draw_card(cards[0]["id"]).resize((IMAGE_W, IMAGE_H), Image.Resampling.NEAREST)
    screen.paste(art, (20, 16))
    draw = ImageDraw.Draw(screen)
    large = ImageFont.truetype(str(font_path), size=88)
    small = ImageFont.truetype(str(font_path), size=22)
    draw.text((120, 188), cards[0]["hanzi"], font=large, fill=INK, anchor="mt")
    draw.text((120, 284), cards[0]["phrase"], font=small, fill=(107, 83, 68), anchor="mt")
    REF_PATH.parent.mkdir(parents=True, exist_ok=True)
    screen.save(REF_PATH)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--font", type=Path)
    parser.add_argument("--voice", default="Tingting")
    parser.add_argument("--rate", type=int, default=150)
    parser.add_argument("--skip-voice", action="store_true")
    parser.add_argument("--skip-fonts", action="store_true")
    args = parser.parse_args()
    cards = load_cards()
    font_path = find_font(args.font)

    PNG_DIR.mkdir(parents=True, exist_ok=True)
    I4_DIR.mkdir(parents=True, exist_ok=True)
    for card in cards:
        img = draw_card(card["id"])
        img.save(PNG_DIR / f"{card['id']}.png")
        (I4_DIR / f"{card['id']}.i4").write_bytes(to_i4(img))

    if not args.skip_fonts:
        hanzi_chars = unique_chars("".join(c["hanzi"] for c in cards))
        phrase_chars = unique_chars("".join(c["phrase"] for c in cards))
        emit_font("font_cards_88", 88, hanzi_chars, FONT_OUT / "font_cards_88.c", font_path)
        emit_font("font_cards_22", 22, phrase_chars, FONT_OUT / "font_cards_22.c", font_path)
    write_catalog(cards)
    make_reference(cards, font_path)
    if not args.skip_voice:
        make_voice(cards, args.voice, args.rate)
    print("Card assets generated.")


if __name__ == "__main__":
    main()
