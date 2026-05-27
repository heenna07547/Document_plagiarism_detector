#ifndef ALGORITHM_H
#define ALGORITHM_H

/* =============================================================
 *  MODULE 3 — Plagiarism Detection Algorithms
 *  include/algorithm.h
 *
 *  Pure-C implementations:
 *    • word_match_similarity()  — token overlap %
 *    • lcs_similarity()         — Longest Common Subsequence %
 *    • edit_similarity()        — Levenshtein edit-distance %
 *
 *  Python-delegated:
 *    • tfidf_similarity_py()    — TF-IDF cosine via scripts/tfidf.py
 *
 *  Aggregate:
 *    • compute_similarity()     — weighted final score
 *      Weights: WordMatch×0.20 + LCS×0.20 + Edit×0.20 + TFIDF×0.40
 * ============================================================= */

#define TFIDF_SCRIPT    "scripts\\tfidf.py"
#define MAX_WORDS       4096
#define MAX_WORD_LEN    64
#define MAX_TEXT_ALG    8192    /* truncation limit for DP algorithms */

/* ── similarity result ──────────────────────────────────────── */
typedef struct {
    float word_match;
    float lcs;
    float edit;
    float tfidf;
    float final_score;
} SimilarityResult;

/* ── plagiarism level ───────────────────────────────────────── */
typedef enum {
    LEVEL_LOW,
    LEVEL_MEDIUM,
    LEVEL_HIGH
} PlagiarismLevel;

/* ── individual algorithm functions ────────────────────────── */

/*
 * word_match_similarity()
 *   Token overlap percentage between two texts.
 *   Returns 0–100.
 */
float word_match_similarity(const char *text1, const char *text2);

/*
 * lcs_similarity()
 *   Longest Common Subsequence percentage.
 *   Returns 0–100.
 */
float lcs_similarity(const char *text1, const char *text2);

/*
 * edit_similarity()
 *   Levenshtein edit-distance similarity percentage.
 *   Returns 0–100.
 */
float edit_similarity(const char *text1, const char *text2);

/*
 * tfidf_similarity_py()
 *   TF-IDF cosine similarity via Python delegate (scripts/tfidf.py).
 *   Returns 0–100.
 */
float tfidf_similarity_py(const char *text1, const char *text2);

/* ── aggregate ──────────────────────────────────────────────── */

/*
 * compute_similarity()
 *   Runs all four algorithms and returns a weighted SimilarityResult.
 *   Weights: WordMatch×0.20 + LCS×0.20 + Edit×0.20 + TFIDF×0.40
 */
SimilarityResult compute_similarity(const char *text1, const char *text2);

/* ── classification helpers ─────────────────────────────────── */

PlagiarismLevel classify_level(float score);
const char     *level_label   (PlagiarismLevel level);

#endif /* ALGORITHM_H */