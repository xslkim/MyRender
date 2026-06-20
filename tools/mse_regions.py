#!/usr/bin/env python3
"""Region-based MSE breakdown: split a 960x540 capture into a grid and report
per-cell MSE. Also writes a heat-map diff image.

Usage:
    python tools/mse_regions.py <ours.bmp> <reference.png> [grid_w grid_h] [diff_out.png]
"""
import sys
import numpy as np
from PIL import Image


def load_rgb(path):
    return np.asarray(Image.open(path).convert("RGB"), dtype=np.float32)


def main():
    ours_path = sys.argv[1]
    ref_path  = sys.argv[2]
    gw = int(sys.argv[3]) if len(sys.argv) > 3 else 6
    gh = int(sys.argv[4]) if len(sys.argv) > 4 else 4
    diff_out = sys.argv[5] if len(sys.argv) > 5 else "diff_heat.png"

    ours = load_rgb(ours_path)
    ref  = load_rgb(ref_path)
    if ours.shape != ref.shape:
        ref_img = Image.open(ref_path).convert("RGB").resize(
            (ours.shape[1], ours.shape[0]), Image.BILINEAR)
        ref = np.asarray(ref_img, dtype=np.float32)

    H, W = ours.shape[:2]
    print(f"image {W}x{H}, grid {gw}x{gh}")
    total = 0.0
    total_n = 0
    cell_w = W // gw
    cell_h = H // gh
    worst = []
    for gy in range(gh):
        row_str = ""
        for gx in range(gw):
            y0 = gy * cell_h
            x0 = gx * cell_w
            o = ours[y0:y0+cell_h, x0:x0+cell_w]
            r = ref[y0:y0+cell_h, x0:x0+cell_w]
            d = o - r
            mse = float(np.mean(d ** 2))
            n = o.shape[0] * o.shape[1]
            total += mse * n
            total_n += n
            worst.append((mse, gx, gy))
            row_str += f"{mse:7.1f} "
        print(f"  row y={gy}: {row_str}")
    print(f"overall MSE = {total/total_n:.2f}")
    print("\nworst cells (MSE, gx, gy):")
    for mse, gx, gy in sorted(worst, reverse=True)[:6]:
        print(f"  cell ({gx},{gy}): MSE={mse:.1f}")

    # Heat map: amplify squared error into a red-tinted overlay.
    sq = (ours - ref) ** 2
    err = np.mean(sq, axis=2)              # HxW
    err = np.clip(err * 0.5, 0, 255).astype(np.uint8)
    heat = np.zeros((H, W, 3), dtype=np.uint8)
    heat[..., 0] = err                     # red channel
    Image.fromarray(heat).save(diff_out)
    print(f"diff heat map -> {diff_out}")


if __name__ == "__main__":
    main()
