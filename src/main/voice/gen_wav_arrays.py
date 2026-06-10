#!/usr/bin/env python3
"""
Regenerate sit_up_wav.c and thank_you_good_work_wav.c from WAV files.
Run from repo root: python3 src/main/voice/gen_wav_arrays.py
"""
import os

WAV_DIR = os.path.join(os.path.dirname(__file__))

CLIPS = [
    ("sit_up.wav",              "sit_up_wav",              "sit_up_wav.c"),
    ("thank_you_good_work.wav", "thank_you_good_work_wav", "thank_you_good_work_wav.c"),
]

COLS = 16

for wav_file, sym, out_file in CLIPS:
    wav_path = os.path.join(WAV_DIR, wav_file)
    out_path = os.path.join(WAV_DIR, out_file)

    with open(wav_path, "rb") as f:
        data = f.read()

    lines = ["#include <stdint.h>", "#include <stddef.h>", ""]
    lines.append(f"// Auto-generated from voice/{wav_file} — do not edit manually")
    lines.append(f"const uint8_t {sym}_data[] = {{")

    for i in range(0, len(data), COLS):
        chunk = data[i:i + COLS]
        hex_vals = ", ".join(f"0x{b:02x}" for b in chunk)
        lines.append(f"    {hex_vals},")

    lines.append("};")
    lines.append(f"const uint32_t {sym}_len = {len(data)}u;")
    lines.append("")

    with open(out_path, "w") as f:
        f.write("\n".join(lines))

    print(f"wrote {out_file}  ({len(data)} bytes from {wav_file})")
