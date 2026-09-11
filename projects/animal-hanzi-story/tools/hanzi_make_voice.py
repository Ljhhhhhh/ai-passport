#!/usr/bin/env python3
"""Generate prototype Mandarin clips and convert them to IMA ADPCM for 3-day MVP."""

from __future__ import annotations

import argparse
import os
import subprocess
import sys
import tempfile
from pathlib import Path


CLIPS = {
    "day1_intro": "欢迎来到汉字小世界！我们来试试汉字魔法。",
    "day1_jump_prompt": "按确定键，让小人跳一跳！",
    "day1_scale_prompt": "按上变大，按下变小！",
    "day1_choice_prompt": "哪个字是【大】？按上选上面，按下选下面。",
    "day1_complete": "太棒了！今天的汉字魔法完成啦！",
    "day2_intro": "小世界里有一座山，但是山谷里还没有水。",
    "day2_learn_water": "这是【水】。水可以变成一条清清的小河。",
    "day2_choice_prompt": "找到【水】，让小河奔流起来吧！",
    "day2_water_flow": "找到了！小河奔流起来啦！",
    "day2_complete": "小河留在了小世界里！明天见！",
    "day3_intro": "我们来看看，你还记得这个字吗？",
    "day3_test_prompt": "哪个是【水】？按上选上面，按下选下面。",
    "day3_test_correct": "太棒啦！你真的认识【水】了！",
    "day3_sandbox_intro": "现在可以在小世界里自由施展魔法啦！",
    "day3_complete": "今天的探索结束啦，眼睛该休息了，明天再玩！",
    "char_da": "大。",
    "char_xiao": "小。",
    "char_tiao": "跳。",
    "char_shan": "山。",
    "char_shui": "水。",
    "char_ba": "爸。",
    "char_ma": "妈。",
    "magic_big": "变大！",
    "magic_small": "变小！",
    "magic_jump": "跳！",
    "magic_water": "哗啦啦，水来啦！",
    "again": "再听一次。",
    "hint_up_down": "按上选上面的字，按下选下面的字。",
}


def run(cmd: list[str]) -> None:
    subprocess.run(cmd, check=True)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--voice", default="Tingting")
    parser.add_argument("--rate", type=int, default=165)
    parser.add_argument("--wav-dir", type=Path, default=Path("assets/music/animal-hanzi"))
    parser.add_argument("--out-dir", type=Path, default=Path("main/assets/hanzi"))
    args = parser.parse_args()

    repo = Path(__file__).resolve().parent.parent
    converter = repo / "tools" / "hanzi_wav_to_adpcm.py"
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
        print(f"{name}: {ima.stat().st_size} bytes")


if __name__ == "__main__":
    main()
