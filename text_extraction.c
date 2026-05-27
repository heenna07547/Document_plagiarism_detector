/* =============================================================
 *  MODULE 2 — OCR & Text Extraction
 * ============================================================= */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "text_extraction.h"
#include "image_processing.h"   /* MAX_PATH_LEN */

static const char *file_ext(const char *path)
{
    const char *dot = strrchr(path, '.');
    if (!dot || dot == path) return NULL;
    return dot + 1;
}

static void ext_lower(const char *ext, char *buf, int buf_size)
{
    int i = 0;
    while (ext[i] && i < buf_size - 1) {
        buf[i] = (char)tolower((unsigned char)ext[i]);
        i++;
    }
    buf[i] = '\0';
}

FileType detect_file_type(const char *filepath)
{
    const char *ext = file_ext(filepath);
    if (!ext) return FTYPE_UNKNOWN;

    char lo[16];
    ext_lower(ext, lo, sizeof(lo));

    if (strcmp(lo, "txt")  == 0) return FTYPE_TXT;
    if (strcmp(lo, "png")  == 0 ||
        strcmp(lo, "jpg")  == 0 ||
        strcmp(lo, "jpeg") == 0) return FTYPE_IMAGE;
    if (strcmp(lo, "pdf")  == 0) return FTYPE_PDF;

    return FTYPE_UNKNOWN;
}

/* ── MODULE 2 — read a plain text file directly in C ───────── */
static int read_txt(const char *filepath, char *out_buf, int buf_size)
{
    FILE *f = fopen(filepath, "r");
    if (!f) {
        fprintf(stderr, "[Module2] Cannot open: %s\n", filepath);
        return -1;
    }
    int n = 0, c;
    while ((c = fgetc(f)) != EOF && n < buf_size - 1)
        out_buf[n++] = (char)c;
    out_buf[n] = '\0';
    fclose(f);
    return 0;
}

static int run_ocr(const char *filepath, char *out_buf, int buf_size)
{
    char cmd[MAX_PATH_LEN + 64];
    snprintf(cmd, sizeof(cmd),
             "python \"%s\" \"%s\" 2>nul",
             OCR_SCRIPT, filepath);

    FILE *pipe = popen(cmd, "r");
    if (!pipe) {
        fprintf(stderr, "[Module2] popen failed for: %s\n", filepath);
        return -1;
    }

    int n = 0, c;
    while ((c = fgetc(pipe)) != EOF && n < buf_size - 1)
        out_buf[n++] = (char)c;
    out_buf[n] = '\0';

    pclose(pipe);
    return 0;
}

/* ── MODULE 2 — extract_text (public) ───────────────────────── */
int extract_text(const char *filepath, char *out_buf, int buf_size)
{
    if (!filepath || !out_buf || buf_size <= 0) return -1;
    out_buf[0] = '\0';

    switch (detect_file_type(filepath)) {
        case FTYPE_TXT:   return read_txt(filepath, out_buf, buf_size);
        case FTYPE_IMAGE:
        case FTYPE_PDF:   return run_ocr (filepath, out_buf, buf_size);
        default:
            fprintf(stderr, "[Module2] Unsupported type: %s\n", filepath);
            return -1;
    }
}

static void str_replace_inplace(char *buf, int buf_size,
                                 const char *old, const char *rep)
{
    int old_len = (int)strlen(old);
    int rep_len = (int)strlen(rep);
    if (old_len == 0) return;

    char *tmp = (char *)malloc((size_t)buf_size);
    if (!tmp) return;

    char *src = buf, *dst = tmp;
    int left = buf_size - 1;

    while (*src && left > 0) {
        if (strncmp(src, old, (size_t)old_len) == 0) {
            int n = rep_len < left ? rep_len : left;
            memcpy(dst, rep, (size_t)n);
            dst += n; left -= n;
            src += old_len;
        } else {
            *dst++ = *src++;
            left--;
        }
    }
    *dst = '\0';
    strncpy(buf, tmp, (size_t)(buf_size - 1));
    buf[buf_size - 1] = '\0';
    free(tmp);
}

void fix_ocr_errors(char *buf, int buf_size)
{
    /* Matches Python corrections dict exactly */
    static const char *FIXES[][2] = {
        { "openlv",                          "opencv"              },
        { "braarizahon",                     "binarization"        },
        { "bnage",                           "image"               },
        { "mages",                           "images"              },
        { "umage",                           "image"               },
        { "responsibilrhes",                 "responsibilities"    },
        { "responsibl",                      "responsible"         },
        { "exact on",                        "extraction"          },
        { "reecgnitia",                      "recognition"         },
        { "machme",                          "machine"             },
        { "frocessing",                      "processing"          },
        { "leaplement",                      "implement"           },
        { "normal zation",                   "normalization"       },
        { "rrodule",                         "module"              },
        { "cleared",                         "cleaned"             },
        { "handturitien",                    "handwritten"         },
        { "ocr & text exact on specialist",
          "ocr & text extraction specialist"                       },
        { "team member z",                   "team member 2"       },
        { "bnage frocessing",                "image processing"    },
        { NULL, NULL }
    };

    for (int i = 0; FIXES[i][0]; i++)
        str_replace_inplace(buf, buf_size, FIXES[i][0], FIXES[i][1]);
}
void clean_text(char *buf, int buf_size)
{
    if (!buf || buf_size <= 0) return;

    /* 1. OCR error fixes */
    fix_ocr_errors(buf, buf_size);

    /* 2. Lowercase */
    for (char *p = buf; *p; p++)
        *p = (char)tolower((unsigned char)*p);

    /* 3 & 4. Strip non-alpha, collapse spaces */
    char *src = buf;
    char *dst = buf;
    int   was_space = 1;

    while (*src) {
        if (isalpha((unsigned char)*src)) {
            *dst++ = *src;
            was_space = 0;
        } else {
            if (!was_space) { *dst++ = ' '; was_space = 1; }
        }
        src++;
    }
    /* Trim trailing space */
    if (dst > buf && *(dst - 1) == ' ') dst--;
    *dst = '\0';
}