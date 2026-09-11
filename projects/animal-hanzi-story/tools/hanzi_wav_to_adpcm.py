#!/usr/bin/env python3
"""Convert 16 kHz mono WAV files into IMA ADPCM clips for the story app."""

from __future__ import annotations

import argparse
import struct
import wave
from pathlib import Path


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

    if nibble & 8:
        predictor -= predicted
    else:
        predictor += predicted
    predictor = clamp_i16(predictor)

    step_index += INDEX_TABLE[nibble]
    step_index = max(0, min(88, step_index))
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
    header = struct.pack("<IIhbb", MAGIC, len(samples), 0, 0, 0)
    return header + bytes(packed)


def read_wav(path: Path) -> list[int]:
    with wave.open(str(path), "rb") as wav_file:
        if wav_file.getnchannels() != 1 or wav_file.getsampwidth() != 2 or wav_file.getframerate() != 16000:
            raise SystemExit(f"{path} must be 16 kHz 16-bit mono")
        frames = wav_file.readframes(wav_file.getnframes())
    return list(struct.unpack("<" + "h" * (len(frames) // 2), frames))


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("wav", type=Path)
    parser.add_argument("out", type=Path)
    args = parser.parse_args()
    args.out.write_bytes(encode_pcm(read_wav(args.wav)))


if __name__ == "__main__":
    main()
