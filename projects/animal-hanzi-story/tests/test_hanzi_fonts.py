#!/usr/bin/env python3
"""Verify generated LVGL sparse cmaps resolve every configured Hanzi glyph."""

from __future__ import annotations

import ast
import re
from pathlib import Path


PROJECT = Path(__file__).resolve().parent.parent
GENERATOR = PROJECT / "tools" / "hanzi_make_fonts.py"


def configured_chars(name: str) -> set[str]:
    tree = ast.parse(GENERATOR.read_text(encoding="utf-8"))
    for node in tree.body:
        if isinstance(node, ast.Assign):
            for target in node.targets:
                if isinstance(target, ast.Name) and target.id == name:
                    value = ast.literal_eval(node.value)
                    return set(value)
    raise AssertionError(f"missing {name} in {GENERATOR}")


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
    assert offsets == sorted(offsets), f"unsorted sparse cmap offsets in {path}"
    assert offsets and offsets[0] == 0, f"first sparse cmap offset must be zero in {path}"
    return {chr(range_start + offset) for offset in offsets}


def main() -> None:
    large = generated_chars(PROJECT / "main" / "font_hanzi_80.c")
    ui = generated_chars(PROJECT / "main" / "font_hanzi_24.c")
    assert large == configured_chars("LARGE_CHARS")
    assert ui == configured_chars("UI_CHARS")


if __name__ == "__main__":
    main()
