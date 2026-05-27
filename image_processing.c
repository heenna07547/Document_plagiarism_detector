/* =============================================================
 *  MODULE 1 — Image Processing & Preprocessing
 * ============================================================= */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "image_processing.h"

static int run_cmd(const char *cmd)
{
    int ret = system(cmd);
#if defined(_WIN32)
    return ret;           /* system() returns exit code on Windows */
#else
    if (WIFEXITED(ret))   /* POSIX: extract true exit code        */
        return WEXITSTATUS(ret);
    return -1;
#endif
}
int preprocess_image(const char *input_path, const char *output_path)
{
    if (!input_path || !output_path) return -1;

    char cmd[MAX_PATH_LEN * 2 + 64];
    snprintf(cmd, sizeof(cmd),
             "python \"%s\" preprocess \"%s\" \"%s\" 2>nul",
             PREPROCESS_SCRIPT, input_path, output_path);

    int rc = run_cmd(cmd);
    if (rc != 0) {
        fprintf(stderr,
                "[Module1] preprocess_image FAILED (exit %d): %s\n",
                rc, input_path);
        return -1;
    }
    return 0;
}

int convert_pdf_pages(const char *pdf_path,
                      const char *out_dir,
                      char        page_paths[][MAX_PATH_LEN],
                      int         max_pages)
{
    if (!pdf_path || !out_dir || !page_paths || max_pages <= 0) return -1;

    char cmd[MAX_PATH_LEN * 2 + 64];
    snprintf(cmd, sizeof(cmd),
             "python \"%s\" pdf2png \"%s\" \"%s\" 2>nul",
             PREPROCESS_SCRIPT, pdf_path, out_dir);

    FILE *pipe = popen(cmd, "r");
    if (!pipe) {
        fprintf(stderr, "[Module1] convert_pdf_pages: popen failed\n");
        return -1;
    }

    int count = 0;
    while (count < max_pages &&
           fgets(page_paths[count], MAX_PATH_LEN, pipe)) {
        /* Strip trailing newline */
        char *nl = strchr(page_paths[count], '\n');
        if (nl) *nl = '\0';
        if (strlen(page_paths[count]) > 0)
            count++;
    }

    int status = pclose(pipe);
    (void)status;   /* non-zero just means 0 pages — handled above */

    return count;
}