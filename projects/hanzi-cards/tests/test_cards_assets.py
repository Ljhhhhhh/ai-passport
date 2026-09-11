#!/usr/bin/env python3
"""Validate the 50-card list, I4 images, fonts, and IMA ADPCM clips."""

from __future__ import annotations

import json
import re
import struct
from pathlib import Path

PROJECT = Path(__file__).resolve().parent.parent
MANIFEST = PROJECT / "assets" / "cards" / "cards.json"
I4_DIR = PROJECT / "main" / "assets" / "cards"
IMAGE_W, IMAGE_H = 200, 164
STRIDE = IMAGE_W // 2
PALETTE = 64
I4_LEN = PALETTE + STRIDE * IMAGE_H
MAGIC = 0x34414D49


def load_cards() -> list[dict]:
    cards = json.loads(MANIFEST.read_text(encoding="utf-8"))["cards"]
    assert len(cards) == 50
    assert len({c["id"] for c in cards}) == 50
    assert len({c["hanzi"] for c in cards}) == 50
    assert cards[0]["hanzi"] == "爸" and cards[0]["phrase"] == "爸爸"
    assert cards[-1]["hanzi"] == "跳" and cards[-1]["phrase"] == "跳一跳"
    return cards


def generated_chars(path: Path) -> set[str]:
    source = path.read_text(encoding="utf-8")
    range_match = re.search(r"\.range_start = 0x([0-9a-f]+)", source)
    list_match = re.search(
        r"static const uint16_t unicode_list_0\[\] = \{\s*(.*?)\s*\};",
        source,
        re.DOTALL,
    )
    assert range_match and list_match, f"missing sparse cmap metadata in {path}"
    range_start = int(range_match.group(1), 16)
    offsets = [int(value, 16) for value in re.findall(r"0x([0-9a-f]+)", list_match.group(1))]
    assert offsets == sorted(offsets)
    assert offsets and offsets[0] == 0
    return {chr(range_start + offset) for offset in offsets}


def check_i4(card: dict) -> None:
    blob = (I4_DIR / f"{card['id']}.i4").read_bytes()
    assert len(blob) == I4_LEN, f"{card['id']}: I4 length {len(blob)}"
    palette = blob[:PALETTE]
    pixels = blob[PALETTE:]
    assert len(pixels) == STRIDE * IMAGE_H
    for i in range(16):
        b, g, r, a = palette[i * 4 : i * 4 + 4]
        assert a == 255
        assert 0 <= b <= 255 and 0 <= g <= 255 and 0 <= r <= 255
    for byte in pixels:
        assert 0 <= byte <= 255


def check_ima(card: dict) -> None:
    blob = (I4_DIR / f"{card['id']}.ima").read_bytes()
    magic, sample_count, predictor, step_index, _reserved = struct.unpack_from("<IIhbb", blob)
    assert magic == MAGIC, f"{card['id']}: bad IMA magic"
    assert sample_count > 0
    assert predictor == 0
    assert 0 <= step_index <= 88
    packed = (sample_count + 1) // 2
    assert len(blob) >= 12 + packed


def main() -> None:
    cards = load_cards()
    catalog = (PROJECT / "main" / "cards_catalog.c").read_text(encoding="utf-8")
    large = generated_chars(PROJECT / "main" / "font_cards_88.c")
    phrase = generated_chars(PROJECT / "main" / "font_cards_22.c")
    hanzi = {c["hanzi"] for c in cards}
    phrases = set("".join(c["phrase"] for c in cards))
    assert large == hanzi
    assert phrases <= phrase
    for card in cards:
        assert f's_{card["id"]}_i4_start' in catalog
        assert f's_{card["id"]}_ima_start' in catalog
        check_i4(card)
        check_ima(card)
    assert (PROJECT / "assets" / "cards" / "reference-warm-picturebook.png").is_file()


if __name__ == "__main__":
    main()
