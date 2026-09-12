#!/usr/bin/env python3
"""Generate Mandarin voice broadcast clips and convert them to IMA ADPCM for Codex Passport."""

from __future__ import annotations

import argparse
import os
import subprocess
import sys
import tempfile
from pathlib import Path

CLIPS = {
    "voice_done": "任务已完成",
    "voice_wait": "请确认，等待输入",
    "voice_error": "任务执行失败",
    "voice_new_msg": "收到新消息",
}

def run(cmd: list[str]) -> None:
    subprocess.run(cmd, check=True)

def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--voice", default="Tingting")
    parser.add_argument("--rate", type=int, default=175)
    parser.add_argument("--wav-dir", type=Path, default=Path("assets/voice"))
    parser.add_argument("--out-dir", type=Path, default=Path("main/assets/voice"))
    args = parser.parse_args()

    repo = Path(__file__).resolve().parent.parent
    converter = repo.parent.parent / "projects/animal-hanzi-story/tools/hanzi_wav_to_adpcm.py"
    wav_dir = repo / args.wav_dir
    out_dir = repo / args.out_dir
    wav_dir.mkdir(parents=True, exist_ok=True)
    out_dir.mkdir(parents=True, exist_ok=True)

    for name, text in CLIPS.items():
        fd, aiff_name = tempfile.mkstemp(suffix=".aiff")
        os.close(fd)
        aiff = Path(aiff_name)
        wav = wav_dir / f"{name}.wav"
        ima = out_dir / f"{name}.ima"
        wav.unlink(missing_ok=True)
        try:
            run(["say", "-v", args.voice, "-r", str(args.rate), "-o", str(aiff), text])
            run([
                "afconvert",
                "-f", "WAVE",
                "-d", "LEI16@16000",
                "-c", "1",
                str(aiff),
                str(wav),
            ])
        finally:
            aiff.unlink(missing_ok=True)
        run([sys.executable, str(converter), str(wav), str(ima)])
        print(f"{name}: wav={wav.stat().st_size} bytes, ima={ima.stat().st_size} bytes")

if __name__ == "__main__":
    main()

