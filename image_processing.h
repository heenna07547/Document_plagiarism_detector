#ifndef IMAGE_PROCESSING_H
#define IMAGE_PROCESSING_H

/* =============================================================
 *  MODULE 1 — Image Processing & Preprocessing
 *  include/image_processing.h
 *
 *  Mirrors the Python OpenCV pipeline:
 *    preprocess_image()  →  resize → grayscale → denoise →
 *                           sharpen → adaptive-threshold →
 *                           morphological close
 *
 *  Because true OpenCV bindings in pure C require libopencv,
 *  the actual pixel work is delegated to the Python helper
 *  scripts/preprocess.py via popen().  This header exposes
 *  the clean C API that the rest of the program calls.
 * ============================================================= */

#define PREPROCESS_SCRIPT  "scripts\\preprocess.py"
#define MAX_PATH_LEN        512

/* Supported image extensions */
static inline int is_image_file(const char *ext) {
    return (ext &&
            ( strcmp(ext, "png")  == 0 ||
              strcmp(ext, "jpg")  == 0 ||
              strcmp(ext, "jpeg") == 0 ));
}

/*
 * preprocess_image()
 *   Runs the Python preprocessing pipeline on `input_path`.
 *   Writes the cleaned image to `output_path`.
 *   Returns 0 on success, -1 on failure.
 */
int preprocess_image(const char *input_path, const char *output_path);

/*
 * convert_pdf_pages()
 *   Converts every page of a PDF to individual PNG files inside
 *   `out_dir` (e.g. "out_dir/page_0.png", "page_1.png" …).
 *   Fills `page_paths` array with the resulting paths (caller-alloc).
 *   Returns the number of pages written, or -1 on failure.
 */
int convert_pdf_pages(const char *pdf_path,
                      const char *out_dir,
                      char        page_paths[][MAX_PATH_LEN],
                      int         max_pages);

#endif /* IMAGE_PROCESSING_H */