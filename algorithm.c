/* =============================================================
 *  MODULE 3 — Plagiarism Detection Algorithms
 * ============================================================= */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "algorithm.h"
#include "image_processing.h"   /* MAX_PATH_LEN */

/* ═══════════════════════════════════════════════════════════════
 *  ALGORITHM 1 — Word Match
 * ═══════════════════════════════════════════════════════════════ */

static int split_words(char *text,
                       char  words[MAX_WORDS][MAX_WORD_LEN])
{
    int count = 0;
    char *tok = strtok(text, " \t\n\r");
    while (tok && count < MAX_WORDS) {
        strncpy(words[count], tok, MAX_WORD_LEN - 1);
        words[count][MAX_WORD_LEN - 1] = '\0';
        count++;
        tok = strtok(NULL, " \t\n\r");
    }
    return count;
}

float word_match_similarity(const char *text1, const char *text2)
{
    /* strtok modifies in-place — work on copies */
    char t1[MAX_TEXT_ALG], t2[MAX_TEXT_ALG];
    strncpy(t1, text1, MAX_TEXT_ALG - 1);  t1[MAX_TEXT_ALG - 1] = '\0';
    strncpy(t2, text2, MAX_TEXT_ALG - 1);  t2[MAX_TEXT_ALG - 1] = '\0';

    char w1[MAX_WORDS][MAX_WORD_LEN];
    char w2[MAX_WORDS][MAX_WORD_LEN];
    int  n1 = split_words(t1, w1);
    int  n2 = split_words(t2, w2);

    if (n1 == 0 || n2 == 0) return 0.0f;

    int matched = 0;
    for (int i = 0; i < n1; i++)
        for (int j = 0; j < n2; j++)
            if (strcmp(w1[i], w2[j]) == 0) { matched++; break; }

    int denom = (n1 > n2) ? n1 : n2;
    return ((float)matched / denom) * 100.0f;
}

/* ═══════════════════════════════════════════════════════════════
 *  ALGORITHM 2 — Longest Common Subsequence (LCS)
 * ═══════════════════════════════════════════════════════════════ */

static int lcs_len(const char *X, const char *Y)
{
    int m = (int)strlen(X);
    int n = (int)strlen(Y);

    int *dp = (int *)calloc((size_t)(m + 1) * (size_t)(n + 1), sizeof(int));
    if (!dp) return 0;

    for (int i = 1; i <= m; i++) {
        for (int j = 1; j <= n; j++) {
            int k = i * (n + 1) + j;
            if (X[i-1] == Y[j-1])
                dp[k] = dp[(i-1)*(n+1)+(j-1)] + 1;
            else {
                int up   = dp[(i-1)*(n+1)+j];
                int left = dp[i*(n+1)+(j-1)];
                dp[k] = (up > left) ? up : left;
            }
        }
    }
    int result = dp[m*(n+1)+n];
    free(dp);
    return result;
}

float lcs_similarity(const char *text1, const char *text2)
{
    /* Truncate to algorithm limit */
    char t1[MAX_TEXT_ALG], t2[MAX_TEXT_ALG];
    strncpy(t1, text1, MAX_TEXT_ALG - 1); t1[MAX_TEXT_ALG-1] = '\0';
    strncpy(t2, text2, MAX_TEXT_ALG - 1); t2[MAX_TEXT_ALG-1] = '\0';

    int lcs     = lcs_len(t1, t2);
    int max_len = (int)strlen(t1);
    int l2      = (int)strlen(t2);
    if (l2 > max_len) max_len = l2;
    if (max_len == 0) return 0.0f;

    return ((float)lcs / max_len) * 100.0f;
}

/* ═══════════════════════════════════════════════════════════════
 *  ALGORITHM 3 — Edit Distance (Levenshtein)
 * ═══════════════════════════════════════════════════════════════ */

static int min3(int a, int b, int c)
{
    int m = (a < b) ? a : b;
    return (m < c) ? m : c;
}

static int edit_dist(const char *s1, const char *s2)
{
    int m = (int)strlen(s1);
    int n = (int)strlen(s2);

    int *dp = (int *)malloc((size_t)(m+1) * (size_t)(n+1) * sizeof(int));
    if (!dp) return 0;

    for (int i = 0; i <= m; i++) dp[i*(n+1)]   = i;
    for (int j = 0; j <= n; j++) dp[j]          = j;

    for (int i = 1; i <= m; i++) {
        for (int j = 1; j <= n; j++) {
            int cost = (s1[i-1] == s2[j-1]) ? 0 : 1;
            dp[i*(n+1)+j] = min3(
                dp[(i-1)*(n+1)+j]      + 1,
                dp[i*(n+1)+(j-1)]      + 1,
                dp[(i-1)*(n+1)+(j-1)]  + cost
            );
        }
    }
    int result = dp[m*(n+1)+n];
    free(dp);
    return result;
}

float edit_similarity(const char *text1, const char *text2)
{
    char t1[MAX_TEXT_ALG], t2[MAX_TEXT_ALG];
    strncpy(t1, text1, MAX_TEXT_ALG - 1); t1[MAX_TEXT_ALG-1] = '\0';
    strncpy(t2, text2, MAX_TEXT_ALG - 1); t2[MAX_TEXT_ALG-1] = '\0';

    int dist    = edit_dist(t1, t2);
    int max_len = (int)strlen(t1);
    int l2      = (int)strlen(t2);
    if (l2 > max_len) max_len = l2;
    if (max_len == 0) return 0.0f;

    return (1.0f - ((float)dist / max_len)) * 100.0f;
}

/* ═══════════════════════════════════════════════════════════════
 *  ALGORITHM 4 — TF-IDF cosine (Python delegate)
 * ═══════════════════════════════════════════════════════════════ */

float tfidf_similarity_py(const char *text1, const char *text2)
{
    if (!text1 || !text2 ||
        strlen(text1) == 0 || strlen(text2) == 0) return 0.0f;

    /* Write texts to a temp file */
    const char *tmpfile = "tmp_tfidf_input.txt";
    FILE *f = fopen(tmpfile, "w");
    if (!f) return 0.0f;
    fprintf(f, "%s\n---SEPARATOR---\n%s\n", text1, text2);
    fclose(f);

    /* Build command */
    char cmd[MAX_PATH_LEN + 64];
    snprintf(cmd, sizeof(cmd),
             "python \"%s\" \"%s\" 2>nul",
             TFIDF_SCRIPT, tmpfile);
             
    FILE *pipe = popen(cmd, "r");
    if (!pipe) { remove(tmpfile); return 0.0f; }

    float score = 0.0f;
    fscanf(pipe, "%f", &score);
    pclose(pipe);
    remove(tmpfile);

    return score;   /* already 0–100 from the Python script */
}

/* ═══════════════════════════════════════════════════════════════
 *  Aggregate: compute_similarity
 * ═══════════════════════════════════════════════════════════════ */

SimilarityResult compute_similarity(const char *text1, const char *text2)
{
    SimilarityResult r;

    r.word_match = word_match_similarity(text1, text2);
    r.lcs        = lcs_similarity       (text1, text2);
    r.edit       = edit_similarity      (text1, text2);
    r.tfidf      = tfidf_similarity_py  (text1, text2);

    /* Weights match original Gradio app exactly */
    r.final_score = r.word_match * 0.20f
                  + r.lcs        * 0.20f
                  + r.edit       * 0.20f
                  + r.tfidf      * 0.40f;

    /* Identical-text shortcut (matches original Python logic) */
    if (strcmp(text1, text2) == 0 && strlen(text1) > 0)
        r.final_score = 100.0f;

    return r;
}

/* ── Classification helpers ────────────────────────────────── */

PlagiarismLevel classify_level(float score)
{
    if (score >= 75.0f) return LEVEL_HIGH;
    if (score >= 45.0f) return LEVEL_MEDIUM;
    return LEVEL_LOW;
}

const char *level_label(PlagiarismLevel level)
{
    switch (level) {
        case LEVEL_HIGH:   return "High";
        case LEVEL_MEDIUM: return "Medium";
        default:           return "Low";
    }
}