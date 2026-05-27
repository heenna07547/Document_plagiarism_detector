#ifndef TEXT_EXTRACTION_H
#define TEXT_EXTRACTION_H

/* =============================================================
 *  MODULE 2 — OCR & Text Extraction
 *  include/text_extraction.h
 *
 *  Wraps Tesseract OCR (via Python helper) and provides:
 *    • File-type detection
 *    • Raw text extraction   (image / PDF / plain-text)
 *    • OCR-error correction  (fix_ocr_errors)
 *    • Full text cleaning    (clean_text)
 * ============================================================= */

#define OCR_SCRIPT      "scripts\\ocr_extract.py"
#define MAX_TEXT_BUF    65536      /* 64 KB per document          */
#define MIN_CLEAN_LEN   10         /* fallback threshold          */

/* ── file type ─────────────────────────────────────────────── */
typedef enum {
    FTYPE_TXT,
    FTYPE_IMAGE,
    FTYPE_PDF,
    FTYPE_UNKNOWN
} FileType;

FileType detect_file_type(const char *filepath);

/* ── raw extraction ─────────────────────────────────────────── */

/*
 * extract_text()
 *   Fills `out_buf` with extracted text from `filepath`.
 *   TXT  → read directly in C.
 *   Image/PDF → popen() the Python OCR script.
 *   Returns 0 on success, -1 on failure.
 */
int extract_text(const char *filepath, char *out_buf, int buf_size);

/* ── cleaning pipeline ──────────────────────────────────────── */

/* Fix known OCR substitution errors in-place */
void fix_ocr_errors(char *buf, int buf_size);

/*
 * clean_text()
 *   Applies the full cleaning pipeline in-place:
 *     1. fix_ocr_errors()
 *     2. lowercase
 *     3. strip non-alpha characters → single spaces
 *     4. collapse whitespace
 *
 *  Note: stop-word removal and tokenization are handled
 *        in the Python OCR helper for Unicode correctness.
 */
void clean_text(char *buf, int buf_size);

#endif /* TEXT_EXTRACTION_H */