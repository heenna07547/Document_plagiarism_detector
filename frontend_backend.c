/* =============================================================
 *  MODULE 4 — Frontend & Backend
 *  src/module4_ui/frontend_backend.c
 *
 *  FRONTEND  — terminal UI (banner, progress, colour table)
 *  BACKEND   — full pipeline orchestration
 *  REPORT    — self-contained HTML output (mirrors Gradio UI)
 * ============================================================= */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "frontend_backend.h"
#include "text_extraction.h"
#include "image_processing.h"

/* ANSI colour codes (disabled automatically on Windows cmd) */
#ifdef _WIN32
  #define COL_RESET  ""
  #define COL_CYAN   ""
  #define COL_GREEN  ""
  #define COL_YELLOW ""
  #define COL_RED    ""
  #define COL_BOLD   ""
#else
  #define COL_RESET  "\033[0m"
  #define COL_CYAN   "\033[96m"
  #define COL_GREEN  "\033[92m"
  #define COL_YELLOW "\033[93m"
  #define COL_RED    "\033[91m"
  #define COL_BOLD   "\033[1m"
#endif

/* ═══════════════════════════════════════════════════════════════
 *  FRONTEND — Terminal UI
 * ═══════════════════════════════════════════════════════════════ */

void ui_print_banner(void)
{
    printf("\n");
    fputs(COL_CYAN COL_BOLD, stdout);
    printf("  +==================================================+\n");
    printf("  |   Handwritten Document Plagiarism Detector        |\n");
    printf("  +==================================================+\n");
    fputs(COL_RESET "\n", stdout);
}

void ui_print_usage(const char *prog)
{
    fprintf(stderr,
        "Usage:\n"
        "  %s file1 file2 [file3 ... file%d]\n\n"
        "Supported types: .txt  .png  .jpg  .jpeg  .pdf\n\n"
        "OCR (images/PDFs) requires:\n"
        "  pip install pytesseract pdf2image opencv-python pillow nltk\n"
        "  sudo apt install tesseract-ocr poppler-utils\n\n"
        "Output:\n"
        "  - Results printed to terminal\n"
        "  - HTML report saved to: %s\n",
        prog, MAX_FILES, HTML_REPORT);
}

void ui_print_progress(int current, int total, const char *filename)
{
    printf("  " COL_CYAN "[%d/%d]" COL_RESET " Extracting: %s\n",
           current, total, filename);
}

void ui_print_pair_result(const char *name_a, const char *name_b,
                          SimilarityResult r)
{
    PlagiarismLevel lvl  = classify_level(r.final_score);
    const char     *lbl  = level_label(lvl);
    const char     *col  = (lvl == LEVEL_HIGH)   ? COL_RED    :
                           (lvl == LEVEL_MEDIUM)  ? COL_YELLOW :
                                                    COL_GREEN;

    printf("\n");
    printf("  +--------------------------------------------------+\n");
    printf("  | %-22s  vs  %-18s |\n", name_a, name_b);
    printf("  +--------------------------------------------------+\n");
    printf("  |  Word Match   : %6.2f%%                          |\n", r.word_match);
    printf("  |  LCS          : %6.2f%%                          |\n", r.lcs);
    printf("  |  Edit Dist    : %6.2f%%                          |\n", r.edit);
    printf("  |  TF-IDF       : %6.2f%%                          |\n", r.tfidf);
    printf("  |  Final Score  : " COL_BOLD "%6.2f %%" COL_RESET
           "  [%s%s%s]%*s|\n",
           r.final_score, col, lbl, COL_RESET,
           (int)(16 - (int)strlen(lbl)), " ");
    printf("  +--------------------------------------------------+\n");
}

void ui_print_summary(PairResult *pairs, int pair_count,
                      DocumentRecord *docs)
{
    printf("\n" COL_BOLD
           "  +=================================================+\n"
           "  |  SUMMARY -- %2d pair(s) compared                  |\n"
           "  +=================================================+" COL_RESET "\n",
           pair_count);

    /* Find highest-scoring pair */
    float max_score = -1.0f;
    int   max_idx   = 0;
    for (int i = 0; i < pair_count; i++) {
        if (pairs[i].result.final_score > max_score) {
            max_score = pairs[i].result.final_score;
            max_idx   = i;
        }
    }

    if (pair_count > 0) {
        printf("  Most similar: \"%s\" vs \"%s\" (%.2f %%)\n",
               docs[pairs[max_idx].doc_a].name,
               docs[pairs[max_idx].doc_b].name,
               max_score);
    }

    printf("  HTML report : %s\n\n", HTML_REPORT);
}

/* ═══════════════════════════════════════════════════════════════
 *  BACKEND — Pipeline Orchestration
 * ═══════════════════════════════════════════════════════════════ */

/* Return the basename of a path */
static const char *basename_of(const char *path)
{
    const char *s = strrchr(path, '/');
#ifdef _WIN32
    const char *s2 = strrchr(path, '\\');
    if (s2 > s) s = s2;
#endif
    return s ? s + 1 : path;
}

int backend_run(char       filepaths[][MAX_FILENAME],
                int        file_count,
                DocumentRecord *docs,
                PairResult     *pairs,
                int        max_pairs)
{
    if (!filepaths || !docs || !pairs) return -1;
    if (file_count < 2 || file_count > MAX_FILES) return -1;

    /* ── Step 1: Extract + clean text from every file ──────── */
    printf("\n  Extracting text from %d file(s)...\n\n", file_count);

    for (int i = 0; i < file_count; i++) {
        DocumentRecord *d = &docs[i];

        strncpy(d->filepath, filepaths[i], MAX_FILENAME - 1);
        d->filepath[MAX_FILENAME - 1] = '\0';

        strncpy(d->name, basename_of(filepaths[i]), MAX_FILENAME - 1);
        d->name[MAX_FILENAME - 1] = '\0';

        d->raw_text[0]   = '\0';
        d->clean_text[0] = '\0';
        d->extraction_ok = 0;

        ui_print_progress(i + 1, file_count, d->name);

        /* Module 2: extract */
        if (extract_text(d->filepath, d->raw_text,
                         (int)sizeof(d->raw_text)) == 0) {
            d->extraction_ok = 1;

            /* Module 2: clean (copy raw → clean, then clean in-place) */
            strncpy(d->clean_text, d->raw_text,
                    sizeof(d->clean_text) - 1);
            d->clean_text[sizeof(d->clean_text) - 1] = '\0';
            clean_text(d->clean_text, (int)sizeof(d->clean_text));

            /* Fallback: if cleaning wiped everything, use lowercased raw */
            if ((int)strlen(d->clean_text) < MIN_CLEAN_LEN) {
                strncpy(d->clean_text, d->raw_text,
                        sizeof(d->clean_text) - 1);
                for (char *p = d->clean_text; *p; p++)
                    if (*p >= 'A' && *p <= 'Z') *p += 32;
            }

            /* Preview */
            int prev = (int)strlen(d->clean_text);
            if (prev > 70) prev = 70;
            printf("    -> \"%-.*s%s\"\n\n",
                   prev, d->clean_text,
                   (int)strlen(d->clean_text) > 70 ? "..." : "");
        } else {
            fprintf(stderr, "    -> WARNING: extraction failed.\n\n");
        }
    }

    /* ── Step 2: Pairwise comparison (Module 3) ────────────── */
    printf("  Running similarity analysis...\n");

    int pair_count = 0;
    for (int i = 0; i < file_count && pair_count < max_pairs; i++) {
        for (int j = i + 1; j < file_count && pair_count < max_pairs; j++) {

            PairResult *pr = &pairs[pair_count];
            pr->doc_a = i;
            pr->doc_b = j;

            pr->result = compute_similarity(docs[i].clean_text,
                                            docs[j].clean_text);

            ui_print_pair_result(docs[i].name, docs[j].name,
                                 pr->result);
            pair_count++;
        }
    }

    return pair_count;
}

/* ═══════════════════════════════════════════════════════════════
 *  HTML REPORT — mirrors the Gradio dark-theme UI
 * ═══════════════════════════════════════════════════════════════ */

/* Score → CSS colour string */
static const char *score_color(float score)
{
    if (score >= 75.0f) return "#ef4444";   /* red    */
    if (score >= 45.0f) return "#f59e0b";   /* amber  */
    return "#22c55e";                        /* green  */
}

/* Escape < > & for safe HTML embedding */
static void html_escape(const char *in, char *out, int out_size)
{
    int i = 0;
    while (*in && i < out_size - 6) {
        if      (*in == '<') { strcpy(out + i, "&lt;");  i += 4; }
        else if (*in == '>') { strcpy(out + i, "&gt;");  i += 4; }
        else if (*in == '&') { strcpy(out + i, "&amp;"); i += 5; }
        else                 { out[i++] = *in; }
        in++;
    }
    out[i] = '\0';
}

int generate_html_report(const char     *out_path,
                         DocumentRecord *docs,   int doc_count,
                         PairResult     *pairs,  int pair_count)
{
    FILE *f = fopen(out_path, "w");
    if (!f) {
        fprintf(stderr, "[Module4] Cannot write report: %s\n", out_path);
        return -1;
    }

    /* ── HTML head ─────────────────────────────────────────── */
   fputs(
        "<!DOCTYPE html>\n<html lang='en'>\n<head>\n"
        "<meta charset='UTF-8'>\n"
        "<meta name='viewport' content='width=device-width,initial-scale=1'>\n"
        "<title>Plagiarism Detection Report</title>\n"
        "<style>\n"
        "  @import url('https://fonts.googleapis.com/css2?"
        "family=Inter:wght@400;600;700&family=JetBrains+Mono&display=swap');\n"
        "  :root{--bg:#000814;--surf:#0b1736;--card:#1f2d4a;"
        "--accent:#00b4d8;--text:#e2e8f0;--muted:#94a3b8;}\n"
        "  *{box-sizing:border-box;margin:0;padding:0}\n"
        "  body{background:linear-gradient(135deg,var(--bg),#001d3d);"
        "color:var(--text);font-family:Inter,sans-serif;"
        "min-height:100vh;padding:0 0 60px}\n"
        "  header{text-align:center;padding:48px 20px 32px;"
        "border-bottom:1px solid rgba(255,255,255,.08)}\n"
        "  header h1{font-size:2rem;font-weight:700;color:var(--accent)}\n"
        "  header p{color:var(--muted);margin-top:8px}\n"
        "  .wrap{max-width:1000px;margin:0 auto;padding:0 20px}\n"
        "  h2{color:var(--accent);font-size:1.1rem;margin:36px 0 14px}\n"
        "  .tbl-wrap{overflow-x:auto;border-radius:12px}\n"
        "  table{width:100%;border-collapse:collapse;font-size:.88rem}\n"
        "  th{background:var(--surf);padding:11px 14px;text-align:left;"
        "color:var(--muted);font-weight:600;white-space:nowrap;"
        "border-bottom:1px solid rgba(255,255,255,.06)}\n"
        "  td{padding:10px 14px;border-bottom:1px solid rgba(255,255,255,.04);"
        "background:rgba(11,23,54,.55);font-family:'JetBrains Mono',monospace;"
        "font-size:.82rem}\n"
        "  tr:last-child td{border-bottom:none}\n"
        "  .badge{display:inline-block;padding:3px 10px;border-radius:999px;"
        "font-size:.75rem;font-weight:700;font-family:Inter,sans-serif}\n"
        "  .high{background:rgba(239,68,68,.18);color:#ef4444}\n"
        "  .medium{background:rgba(245,158,11,.18);color:#f59e0b}\n"
        "  .low{background:rgba(34,197,94,.18);color:#22c55e}\n"
        "  .bar-wrap{display:flex;align-items:center;gap:8px}\n"
        "  .bar{flex:1;height:6px;background:rgba(255,255,255,.08);"
        "border-radius:999px;overflow:hidden;min-width:60px}\n"
        "  .bar-fill{height:100%;border-radius:999px}\n"
        "  .bar-val{min-width:40px;text-align:right}\n"
        "  .doc-card{background:var(--surf);border-radius:14px;"
        "padding:20px;margin-bottom:16px;"
        "border:1px solid rgba(255,255,255,.06)}\n"
        "  .doc-card h3{font-size:1rem;margin-bottom:14px;color:var(--accent)}\n"
        "  .doc-card h4{font-size:.78rem;color:var(--muted);"
        "text-transform:uppercase;letter-spacing:.05em;margin-bottom:6px}\n"
        "  .textbox{background:var(--card);border-radius:10px;"
        "padding:12px 14px;max-height:200px;overflow-y:auto;"
        "white-space:pre-wrap;font-family:'JetBrains Mono',monospace;"
        "font-size:.78rem;color:#c7d4e8;line-height:1.6;margin-bottom:14px}\n"
        "</style>\n</head>\n<body>\n"
        "<header>\n"
        "  <h1>&#128218; Plagiarism Detection Report</h1>\n"
        "  <p>Handwritten document comparison &mdash; "
        "WordMatch | LCS | Edit Distance | TF-IDF</p>\n"
        "</header>\n"
        "<div class='wrap'>\n",
        f
    );
    /* ── Results table ─────────────────────────────────────── */
    fprintf(f,
        "<h2>&#128202; Similarity Results</h2>\n"
        "<div class='tbl-wrap'>\n"
        "<table>\n"
        "<thead><tr>"
        "<th>File 1</th><th>File 2</th>"
        "<th>WordMatch %%</th><th>LCS %%</th>"
        "<th>Edit %%</th><th>TF-IDF %%</th>"
        "<th>Final Score</th><th>Level</th>"
        "</tr></thead>\n<tbody>\n"
    );

    for (int i = 0; i < pair_count; i++) {
        PairResult     *pr  = &pairs[i];
        SimilarityResult r  = pr->result;
        const char     *lvl = level_label(classify_level(r.final_score));
        const char     *col = score_color(r.final_score);
        const char     *cls = (r.final_score >= 75.0f) ? "high" :
                              (r.final_score >= 45.0f) ? "medium" : "low";

        fprintf(f,
            "<tr>"
            "<td>%s</td><td>%s</td>"
            "<td>%.2f</td><td>%.2f</td>"
            "<td>%.2f</td><td>%.2f</td>"
            "<td>"
              "<div class='bar-wrap'>"
                "<div class='bar'>"
                  "<div class='bar-fill' style='width:%.0f%%;background:%s'></div>"
                "</div>"
                "<span class='bar-val'>%.2f%%</span>"
              "</div>"
            "</td>"
            "<td><span class='badge %s'>%s</span></td>"
            "</tr>\n",
            docs[pr->doc_a].name, docs[pr->doc_b].name,
            r.word_match, r.lcs, r.edit, r.tfidf,
            r.final_score, col, r.final_score,
            cls, lvl
        );
    }

    fprintf(f, "</tbody>\n</table>\n</div>\n");

    /* ── OCR detail cards ──────────────────────────────────── */
    fprintf(f, "<h2>&#128196; OCR + Cleaned Text Details</h2>\n");

    char esc_buf[4096];

    for (int i = 0; i < doc_count; i++) {
        DocumentRecord *d = &docs[i];

        fprintf(f, "<div class='doc-card'>\n");
        fprintf(f, "<h3>&#128196; %s</h3>\n", d->name);

        fprintf(f, "<h4>Extracted OCR Text</h4>\n<div class='textbox'>");
        /* Emit first 2000 chars of raw text, HTML-escaped */
        char raw_preview[2001];
        strncpy(raw_preview, d->raw_text, 2000);
        raw_preview[2000] = '\0';
        html_escape(raw_preview, esc_buf, sizeof(esc_buf));
        fprintf(f, "%s", esc_buf);
        if (strlen(d->raw_text) > 2000) fprintf(f, "\n[... truncated ...]");
        fprintf(f, "</div>\n");

        fprintf(f, "<h4>Cleaned Text</h4>\n<div class='textbox'>");
        html_escape(d->clean_text, esc_buf, sizeof(esc_buf));
        fprintf(f, "%s", esc_buf);
        fprintf(f, "</div>\n");

        fprintf(f, "</div>\n");
    }

    fprintf(f, "</div>\n</body>\n</html>\n");
    fclose(f);

    printf("  HTML report written to: %s\n", out_path);
    return 0;
}