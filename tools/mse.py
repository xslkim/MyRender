#!/usr/bin/env python3
"""Compare a MyRender capture (.bmp) against the Unity reference image.

Usage:
    python tools/mse.py <ours.bmp> <reference.png>
    python tools/mse.py <ours.bmp> <reference.png> --region x y w h

Outputs MSE, PSNR, and per-channel errors over the 960x540 (or resized) image.
"""
import sys
import numpy as np
from PIL import Image


def load_rgb(path):
    img = Image.open(path).convert("RGB")
    return np.asarray(img, dtype=np.float32)


def main():
    if len(sys.argv) < 3:
        print(__doc__)
        sys.exit(1)

    ours_path = sys.argv[1]
    ref_path  = sys.argv[2]

    ours = load_rgb(ours_path)
    ref  = load_rgb(ref_path)

    # Match dimensions: resize reference down/up to ours if needed.
    if ours.shape != ref.shape:
        print(f"[info] resizing reference {ref.shape[1]}x{ref.shape[0]} -> "
              f"{ours.shape[1]}x{ours.shape[0]}")
        ref_img = Image.open(ref_path).convert("RGB").resize(
            (ours.shape[1], ours.shape[0]), Image.BILINEAR)
        ref = np.asarray(ref_img, dtype=np.float32)

    # Optional region: --region x y w h
    x = y = 0
    w = ours.shape[1]
    h = ours.shape[0]
    if "--region" in sys.argv:
        i = sys.argv.index("--region")
        x, y, w, h = (int(v) for v in sys.argv[i + 1:i + 5])

    x2 = min(x + w, ours.shape[1])
    y2 = min(y + h, ours.shape[0])
    ours_r = ours[y:y2, x:x2]
    ref_r  = ref[y:y2, x:x2]

    diff = ours_r - ref_r
    mse  = float(np.mean(diff ** 2))
    psnr = 10.0 * np.log10(255.0 ** 2 / mse) if mse > 1e-9 else float("inf")

    me = np.mean(diff, axis=(0, 1))
    print(f"region: x={x} y={y} w={x2-x} h={y2-y}")
    print(f"MSE  = {mse:.2f}")
    print(f"PSNR = {psnr:.2f} dB")
    print(f"mean error per channel (ours-ref) R={me[0]:+.2f} "
          f"G={me[1]:+.2f} B={me[2]:+.2f}")


if __name__ == "__main__":
    main()
