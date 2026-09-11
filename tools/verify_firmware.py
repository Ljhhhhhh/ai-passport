#!/usr/bin/env python3
"""Verify the merged ESP32-C3 firmware layout produced by idf.py merge-bin."""

from __future__ import annotations

import sys
from pathlib import Path


def main() -> int:
    build_dir = Path(sys.argv[1] if len(sys.argv) > 1 else "build").resolve()
    proj_name = sys.argv[2] if len(sys.argv) > 2 else None

    if proj_name:
        merged_path = build_dir / f"{proj_name}-full.bin"
        app_bin = f"{proj_name}.bin"
    else:
        candidates = list(build_dir.glob("*-full.bin"))
        if candidates:
            merged_path = candidates[0]
            app_bin = merged_path.name.replace("-full.bin", ".bin")
        else:
            merged_path = build_dir / "FoloToy-AI-Passport-full.bin"
            app_bin = "FoloToy-AI-Passport.bin"

    flash_args_path = build_dir / "flash_args"
    if not merged_path.is_file() or not flash_args_path.is_file():
        print(f"ERROR: merged firmware ({merged_path}) or flash_args ({flash_args_path}) is missing", file=sys.stderr)
        return 1

    flash_args = flash_args_path.read_text(encoding="utf-8")
    if "--flash_size 8MB" not in flash_args:
        print("ERROR: flash_args does not select the required 8 MB flash size", file=sys.stderr)
        return 1

    expected_images = (
        (0x0000, "bootloader/bootloader.bin"),
        (0x8000, "partition_table/partition-table.bin"),
        (0x10000, app_bin),
    )

    merged = merged_path.read_bytes()
    for offset, relative_name in expected_images:
        image_path = build_dir / relative_name
        if not image_path.is_file():
            print(f"ERROR: missing image {image_path}", file=sys.stderr)
            return 1
        image = image_path.read_bytes()
        if merged[offset : offset + len(image)] != image:
            print(f"ERROR: {relative_name} differs at merged offset 0x{offset:x}", file=sys.stderr)
            return 1
        print(f"Verified {relative_name}: {len(image)} bytes at 0x{offset:x}")

    if len(merged) > 8 * 1024 * 1024:
        print("ERROR: merged firmware exceeds 8 MB", file=sys.stderr)
        return 1

    print(f"Merged firmware: PASS ({len(merged)} bytes, flash at 0x0)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
