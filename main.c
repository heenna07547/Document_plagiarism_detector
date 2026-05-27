/* =============================================================
 *  main.c  —  Plagiarism Detector Entry Point
 *
 *  Wires all 4 modules together:
 *    Module 1 → image_processing.h  (OpenCV preprocessing)
 *    Module 2 → text_extraction.h   (OCR + text cleaning)
 *    Module 3 → algorithm.h         (similarity algorithms)
 *    Module 4 → frontend_backend.h  (UI + pipeline + HTML report)
 *
 *  Usage:
 *    ./plagiarism_detector file1 file2 [file3 ...]
 *
 *  Supported: .txt  .png  .jpg  .jpeg  .pdf
 * ============================================================= */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "frontend_backend.h"   /* Module 4 — also pulls in algorithm.h */
#include "text_extraction.h"    /* Module 2 */
#include "image_processing.h"   /* Module 1 */

int main(int argc, char *argv[])
{
    /* ── Module 4 Frontend: banner ─────────────────────────── */
    ui_print_banner();

    if (argc < 3) {
        ui_print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    int file_count = argc - 1;
    if (file_count > MAX_FILES) {
        fprintf(stderr, "[main] Too many files — max %d.\n", MAX_FILES);
        return EXIT_FAILURE;
    }

    /* ── Copy argv paths into a flat array ─────────────────── */
    char filepaths[MAX_FILES][MAX_FILENAME];
    for (int i = 0; i < file_count; i++) {
        strncpy(filepaths[i], argv[i + 1], MAX_FILENAME - 1);
        filepaths[i][MAX_FILENAME - 1] = '\0';
    }

    /* ── Allocate result buffers ────────────────────────────── */
    DocumentRecord *docs = (DocumentRecord *)
        calloc((size_t)file_count, sizeof(DocumentRecord));

    int max_pairs = file_count * (file_count - 1) / 2;
    PairResult *pairs = (PairResult *)
        calloc((size_t)max_pairs, sizeof(PairResult));

    if (!docs || !pairs) {
        perror("[main] calloc");
        free(docs); free(pairs);
        return EXIT_FAILURE;
    }

    /* ── Module 4 Backend: run full pipeline ───────────────── */
    int pair_count = backend_run(filepaths, file_count,
                                 docs, pairs, max_pairs);

    if (pair_count < 0) {
        fprintf(stderr, "[main] Pipeline failed.\n");
        free(docs); free(pairs);
        return EXIT_FAILURE;
    }

    /* ── Module 4 Frontend: summary ────────────────────────── */
    ui_print_summary(pairs, pair_count, docs);

    /* ── Module 4: generate HTML report ────────────────────── */
    generate_html_report(HTML_REPORT, docs, file_count,
                         pairs, pair_count);

    free(docs);
    free(pairs);
    return EXIT_SUCCESS;
}