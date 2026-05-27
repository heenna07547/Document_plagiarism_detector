#!/usr/bin/env python3
"""
scripts/tfidf.py  —  called by Module 3 (algorithm.c)

Usage (called internally by C via popen):
  python scripts/tfidf.py <tmpfile>

Input file format (written by algorithm.c):
  <text1>
  ---SEPARATOR---
  <text2>

Output:
  A single float (0.0 – 100.0) printed to stdout.
  C reads this with fscanf(pipe, "%f", &score).
"""

import sys
import os

# ── dependency check ─────────────────────────────────────────────
try:
    from sklearn.feature_extraction.text import TfidfVectorizer
    from sklearn.metrics.pairwise import cosine_similarity
except ImportError as e:
    sys.stderr.write(f"[tfidf] Missing dependency: {e}\n"
                     "Run: pip install scikit-learn\n")
    sys.exit(1)


# ── TF-IDF cosine similarity ─────────────────────────────────────
def tfidf_cosine(text1: str, text2: str) -> float:
    """
    Returns cosine similarity as a percentage (0.0 – 100.0).
    Falls back to 0.0 if either text is empty or vectorisation fails.
    """
    text1 = text1.strip()
    text2 = text2.strip()

    if not text1 or not text2:
        return 0.0

    # Identical texts → 100 %
    if text1 == text2:
        return 100.0

    try:
        vectorizer = TfidfVectorizer()
        tfidf_matrix = vectorizer.fit_transform([text1, text2])
        score = cosine_similarity(tfidf_matrix[0], tfidf_matrix[1])[0][0]
        return float(score) * 100.0
    except ValueError:
        # Happens when vocabulary is empty after stop-word stripping
        return 0.0


# ── entry point ──────────────────────────────────────────────────
if __name__ == "__main__":
    if len(sys.argv) < 2:
        sys.stderr.write("Usage: tfidf.py <input_file>\n")
        sys.exit(1)

    tmpfile = sys.argv[1]

    if not os.path.isfile(tmpfile):
        sys.stderr.write(f"[tfidf] File not found: {tmpfile}\n")
        print("0.0")
        sys.exit(0)

    try:
        with open(tmpfile, "r", encoding="utf-8", errors="ignore") as f:
            content = f.read()

        # Split on the sentinel written by algorithm.c
        parts = content.split("---SEPARATOR---")
        if len(parts) < 2:
            sys.stderr.write("[tfidf] Separator not found in input.\n")
            print("0.0")
            sys.exit(0)

        text1 = parts[0].strip()
        text2 = parts[1].strip()

        score = tfidf_cosine(text1, text2)
        # Print with 6 decimal places — fscanf("%f") handles this fine
        print(f"{score:.6f}")

    except Exception as e:
        sys.stderr.write(f"[tfidf] Error: {e}\n")
        print("0.0")
        sys.exit(0)