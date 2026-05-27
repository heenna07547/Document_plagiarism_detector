#!/usr/bin/env python3
"""
scripts/preprocess.py  —  called by Module 1 (image_processing.c)

Usage (called internally by C via popen):
  python scripts/preprocess.py preprocess <input_img> <output_img>
  python scripts/preprocess.py pdf2png    <pdf_path>  <out_dir>

Implements the exact OpenCV pipeline from the original Colab notebook:
  resize ×2  →  grayscale  →  fastNlMeansDenoising
  sharpen kernel  →  adaptiveThreshold  →  morphologyEx CLOSE
"""

import sys
import os

try:
    import cv2
    import numpy as np
    from pdf2image import convert_from_path
except ImportError as e:
    sys.stderr.write(f"[preprocess] Missing dep: {e}\n"
                     "Run: pip install opencv-python pdf2image pillow\n")
    sys.exit(1)


# ── OpenCV pipeline (mirrors Colab exactly) ──────────────────────

def preprocess(input_path: str, output_path: str) -> None:
    img = cv2.imread(input_path)
    if img is None:
        raise ValueError(f"Cannot read: {input_path}")

    # 1. Resize ×2 (INTER_CUBIC)
    img = cv2.resize(img, None, fx=2.0, fy=2.0,
                     interpolation=cv2.INTER_CUBIC)

    # 2. Grayscale
    gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)

    # 3. Denoise  (h=30, template=7, search=21)
    denoised = cv2.fastNlMeansDenoising(gray, None, 30, 7, 21)

    # 4. Sharpen kernel  [[0,-1,0],[-1,5,-1],[0,-1,0]]
    kernel    = np.array([[0, -1, 0], [-1, 5, -1], [0, -1, 0]])
    sharpened = cv2.filter2D(denoised, -1, kernel)

    # 5. Adaptive threshold (Gaussian, block=31, C=11)
    thresh = cv2.adaptiveThreshold(
        sharpened, 255,
        cv2.ADAPTIVE_THRESH_GAUSSIAN_C,
        cv2.THRESH_BINARY,
        31, 11
    )

    # 6. Morphological close (1×1 kernel)
    k2      = np.ones((1, 1), np.uint8)
    cleaned = cv2.morphologyEx(thresh, cv2.MORPH_CLOSE, k2)

    cv2.imwrite(output_path, cleaned)


# ── PDF → per-page PNG ───────────────────────────────────────────

def pdf_to_png(pdf_path: str, out_dir: str) -> None:
    os.makedirs(out_dir, exist_ok=True)
    pages = convert_from_path(pdf_path)
    for i, page in enumerate(pages):
        out = os.path.join(out_dir, f"page_{i}.png")
        page.save(out, "PNG")
        print(out)   # C reads these paths from stdout


# ── entry point ──────────────────────────────────────────────────

if __name__ == "__main__":
    if len(sys.argv) < 4:
        sys.stderr.write("Usage: preprocess.py <mode> <arg1> <arg2>\n")
        sys.exit(1)

    mode = sys.argv[1]

    try:
        if mode == "preprocess":
            preprocess(sys.argv[2], sys.argv[3])
        elif mode == "pdf2png":
            pdf_to_png(sys.argv[2], sys.argv[3])
        else:
            sys.stderr.write(f"Unknown mode: {mode}\n")
            sys.exit(1)
    except Exception as e:
        sys.stderr.write(f"[preprocess] Error: {e}\n")
        sys.exit(1)