#ifndef FRONTEND_BACKEND_H
#define FRONTEND_BACKEND_H

/* =============================================================
 *  MODULE 4 — Frontend & Backend (CLI UI + HTML Report)
 *  include/frontend_backend.h
 *
 *  Frontend  → interactive terminal UI (banner, menus, progress,
 *               colour-coded results table)
 *  Backend   → orchestrates the full pipeline:
 *               collect files → extract → clean → compare → report
 *  Report    → generates a self-contained HTML report file
 * ============================================================= */

#include "algorithm.h"
#include "text_extraction.h"

#define MAX_FILES        20
#define MAX_FILENAME     256
#define HTML_REPORT      "report.html"

/* ── per-document record ────────────────────────────────────── */
typedef struct {
    char filepath[MAX_FILENAME];
    char name    [MAX_FILENAME];   /* basename only             */
    char raw_text[MAX_TEXT_BUF];
    char clean_text[MAX_TEXT_BUF];
    int  extraction_ok;            /* 1 = success, 0 = failed   */
} DocumentRecord;

/* ── pairwise result record ─────────────────────────────────── */
typedef struct {
    int              doc_a;
    int              doc_b;
    SimilarityResult result;
} PairResult;

/* ── frontend (terminal UI) ─────────────────────────────────── */
void ui_print_banner      (void);
void ui_print_usage       (const char *prog);
void ui_print_progress    (int current, int total, const char *filename);
void ui_print_pair_result (const char *name_a, const char *name_b,
                           SimilarityResult r);
void ui_print_summary     (PairResult *pairs, int pair_count,
                           DocumentRecord *docs);

/* ── backend (pipeline) ─────────────────────────────────────── */

/*
 * backend_run()
 *   Full pipeline given a list of file paths.
 *   Populates `docs` and `pairs` arrays (caller-allocated).
 *   Returns the number of pairs processed, or -1 on error.
 */
int backend_run(char       filepaths[][MAX_FILENAME],
                int        file_count,
                DocumentRecord *docs,     /* [file_count]        */
                PairResult     *pairs,    /* [file_count*(file_count-1)/2] */
                int        max_pairs);

/* ── HTML report ────────────────────────────────────────────── */

/*
 * generate_html_report()
 *   Writes a self-contained HTML report to `out_path`.
 *   Returns 0 on success, -1 on failure.
 */
int generate_html_report(const char     *out_path,
                         DocumentRecord *docs,   int doc_count,
                         PairResult     *pairs,  int pair_count);

#endif /* FRONTEND_BACKEND_H */