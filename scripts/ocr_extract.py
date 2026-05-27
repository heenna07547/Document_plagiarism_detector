#!/usr/bin/env python3
"""
scripts/ocr_extract.py  —  called by Module 2 (text_extraction.c)

Usage (called internally by C via popen):
  python scripts/ocr_extract.py <filepath>

Supports:
  .txt        → read directly
  .png/.jpg   → preprocess with OpenCV → Tesseract OCR
  .pdf        → pdf2image → per-page OCR → concatenate

Output:
  Cleaned, stop-word-free text printed to stdout.
  C reads this via popen().
"""

import sys
import os

# ── dependency check ─────────────────────────────────────────────
try:
    import cv2
    import numpy as np
    import pytesseract
    from PIL import Image
    from pdf2image import convert_from_path
    import nltk
    from nltk.tokenize import word_tokenize
    from nltk.corpus import stopwords
except ImportError as e:
    sys.stderr.write(f"[ocr_extract] Missing dependency: {e}\n"
                     "Run: pip install opencv-python pytesseract pdf2image "
                     "pillow nltk\n")
    sys.exit(1)

# ── Tesseract path (Windows) ─────────────────────────────────────
pytesseract.pytesseract.tesseract_cmd = (
    r"C:\Program Files\Tesseract-OCR\tesseract.exe"
)

# ── NLTK data check ──────────────────────────────────────────────
for pkg, path in [("punkt_tab", "tokenizers/punkt_tab"),
                  ("stopwords", "corpora/stopwords")]:
    try:
        nltk.data.find(path)
    except LookupError:
        nltk.download(pkg, quiet=True)

STOP_WORDS = set(stopwords.words("english"))


# ── OpenCV preprocessing (same pipeline as preprocess.py) ────────
def preprocess_for_ocr(img_bgr):
    """Resize → grayscale → denoise → sharpen → threshold → morph."""
    img = cv2.resize(img_bgr, None, fx=2.0, fy=2.0,
                     interpolation=cv2.INTER_CUBIC)
    gray     = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
    denoised = cv2.fastNlMeansDenoising(gray, None, 30, 7, 21)
    kernel   = np.array([[0, -1, 0], [-1, 5, -1], [0, -1, 0]])
    sharp    = cv2.filter2D(denoised, -1, kernel)
    thresh   = cv2.adaptiveThreshold(
        sharp, 255,
        cv2.ADAPTIVE_THRESH_GAUSSIAN_C,
        cv2.THRESH_BINARY, 31, 11
    )
    k2      = np.ones((1, 1), np.uint8)
    cleaned = cv2.morphologyEx(thresh, cv2.MORPH_CLOSE, k2)
    return cleaned


# ── OCR on a single OpenCV image ─────────────────────────────────
def ocr_image_array(img_bgr):
    processed = preprocess_for_ocr(img_bgr)
    text = pytesseract.image_to_string(
        processed,
        config="--oem 3 --psm 6"
    )
    return text


# ── stop-word removal ────────────────────────────────────────────
def remove_stopwords(text):
    tokens = word_tokenize(text.lower())
    filtered = [w for w in tokens if w.isalpha() and w not in STOP_WORDS]
    return " ".join(filtered)


# ── handlers per file type ───────────────────────────────────────
def extract_txt(filepath):
    with open(filepath, "r", encoding="utf-8", errors="ignore") as f:
        return f.read()


def extract_image(filepath):
    img = cv2.imread(filepath)
    if img is None:
        raise ValueError(f"Cannot read image: {filepath}")
    return ocr_image_array(img)


def extract_pdf(filepath):
    pages = convert_from_path(filepath)
    all_text = []
    for page in pages:
        # Convert PIL image → OpenCV array
        img_bgr = cv2.cvtColor(np.array(page), cv2.COLOR_RGB2BGR)
        all_text.append(ocr_image_array(img_bgr))
    return "\n".join(all_text)


# ── entry point ──────────────────────────────────────────────────
if __name__ == "__main__":
    if len(sys.argv) < 2:
        sys.stderr.write("Usage: ocr_extract.py <filepath>\n")
        sys.exit(1)

    filepath = sys.argv[1]

    if not os.path.isfile(filepath):
        sys.stderr.write(f"[ocr_extract] File not found: {filepath}\n")
        sys.exit(1)

    ext = os.path.splitext(filepath)[1].lower()

    try:
        if ext == ".txt":
            raw = extract_txt(filepath)
        elif ext in (".png", ".jpg", ".jpeg"):
            raw = extract_image(filepath)
        elif ext == ".pdf":
            raw = extract_pdf(filepath)
        else:
            sys.stderr.write(f"[ocr_extract] Unsupported type: {ext}\n")
            sys.exit(1)

        # Remove stop words and print — C reads this from stdout
        result = remove_stopwords(raw)
        print(result)

    except Exception as e:
        sys.stderr.write(f"[ocr_extract] Error: {e}\n")
        sys.exit(1)