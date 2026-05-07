#include <ctype.h>
#include <stdbool.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>

#define DEFAULT_WIDTH 80
#define MAX_CMD_LEN 4096
#define CROP 8192

typedef struct {
    char *title;
    char *description;
    char *extract;
} Article;

static int terminal_width(void) {
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 20) {
        return ws.ws_col;
    }
    return DEFAULT_WIDTH;
}

static void print_line(char ch, int width) {
    for (int i = 0; i < width; i++) {
        putchar(ch);
    }
    putchar('\n');
}

static void print_center(const char *text, int width) {
    int len = (int)strlen(text);
    if (len >= width) {
        printf("%s\n", text);
        return;
    }
    int left = (width - len) / 2;
    for (int i = 0; i < left; i++) {
        putchar(' ');
    }
    printf("%s\n", text);
}

static void print_wrapped(const char *text, int width, int indent) {
    if (!text) {
        return;
    }

    const char *p = text;
    while (*p) {
        int newline_count = 0;
        while (*p == '\n') {
            newline_count++;
            p++;
        }
        if (newline_count > 0) {
            putchar('\n');
        }
        if (!*p) {
            break;
        }

        const char *line_end = p;
        while (*line_end && *line_end != '\n') {
            line_end++;
        }

        int col = indent;
        for (int i = 0; i < indent; i++) {
            putchar(' ');
        }

        const char *cursor = p;
        while (cursor < line_end) {
            while (cursor < line_end && (*cursor == ' ' || *cursor == '\t')) {
                cursor++;
            }
            if (cursor >= line_end) {
                break;
            }

            const char *w = cursor;
            while (cursor < line_end && *cursor != ' ' && *cursor != '\t') {
                cursor++;
            }
            int wlen = (int)(cursor - w);

            if (col > indent && col + 1 + wlen > width) {
                putchar('\n');
                for (int i = 0; i < indent; i++) {
                    putchar(' ');
                }
                fwrite(w, 1, (size_t)wlen, stdout);
                col = indent + wlen;
            } else {
                if (col > indent) {
                    putchar(' ');
                    col++;
                }
                fwrite(w, 1, (size_t)wlen, stdout);
                col += wlen;
            }
        }
        putchar('\n');
        p = line_end;
    }
}

static void print_wrapped_flat(const char *text, int width, int indent) {
    if (!text) {
        return;
    }

    int col = indent;
    for (int i = 0; i < indent; i++) {
        putchar(' ');
    }

    const char *p = text;
    while (*p) {
        while (*p == ' ' || *p == '\t' || *p == '\n') {
            p++;
        }
        if (!*p) {
            break;
        }

        const char *w = p;
        while (*p && *p != ' ' && *p != '\t' && *p != '\n') {
            p++;
        }
        int wlen = (int)(p - w);

        if (col > indent && col + 1 + wlen > width) {
            putchar('\n');
            for (int i = 0; i < indent; i++) {
                putchar(' ');
            }
            fwrite(w, 1, (size_t)wlen, stdout);
            col = indent + wlen;
        } else {
            if (col > indent) {
                putchar(' ');
                col++;
            }
            fwrite(w, 1, (size_t)wlen, stdout);
            col += wlen;
        }
    }
    putchar('\n');
}

static char *url_encode_title(const char *input) {
    size_t n = strlen(input);
    char *out = malloc(n * 3 + 1);
    if (!out) {
        return NULL;
    }
    char *q = out;
    for (size_t i = 0; i < n; i++) {
        unsigned char c = (unsigned char)input[i];
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            *q++ = (char)c;
        } else if (c == ' ') {
            *q++ = '_';
        } else {
            sprintf(q, "%%%02X", c);
            q += 3;
        }
    }
    *q = '\0';
    return out;
}

static char *read_pipe_output(const char *cmd) {
    FILE *fp = popen(cmd, "r");
    if (!fp) {
        return NULL;
    }

    size_t cap = 8192;
    size_t len = 0;
    char *buf = malloc(cap);
    if (!buf) {
        pclose(fp);
        return NULL;
    }

    int ch;
    while ((ch = fgetc(fp)) != EOF) {
        if (len + 1 >= cap) {
            cap *= 2;
            char *tmp = realloc(buf, cap);
            if (!tmp) {
                free(buf);
                pclose(fp);
                return NULL;
            }
            buf = tmp;
        }
        buf[len++] = (char)ch;
    }
    buf[len] = '\0';
    pclose(fp);
    return buf;
}

static char *json_extract_string(const char *json, const char *key) {
    char needle[128];
    snprintf(needle, sizeof(needle), "\"%s\":\"", key);
    const char *p = strstr(json, needle);
    if (!p) {
        return NULL;
    }
    p += strlen(needle);
    const char *start = p;
    char *out = malloc(strlen(p) + 1);
    if (!out) {
        return NULL;
    }
    char *q = out;

    while (*p) {
        if (*p == '"' && p > start && p[-1] != '\\') {
            break;
        }
        if (*p == '\\') {
            p++;
            if (!*p) {
                break;
            }
            switch (*p) {
                case 'n':
                    *q++ = '\n';
                    break;
                case 't':
                case 'r':
                    *q++ = ' ';
                    break;
                case '\\':
                case '"':
                case '/':
                    *q++ = *p;
                    break;
                case 'u':
                    /* Skip unicode escape payload (e.g. \u2013) to avoid raw digits in output. */
                    for (int i = 0; i < 4 && p[1] && isxdigit((unsigned char)p[1]); i++) {
                        p++;
                    }
                    *q++ = ' ';
                    break;
                default:
                    break;
            }
        } else {
            *q++ = *p;
        }
        p++;
    }
    *q = '\0';
    return out;
}

static void free_article(Article *a) {
    free(a->title);
    free(a->description);
    free(a->extract);
}

static int fetch_article(const char *query, Article *article) {
    char *encoded = url_encode_title(query);
    if (!encoded) {
        return -1;
    }

    char cmd[MAX_CMD_LEN];
    snprintf(
        cmd,
        sizeof(cmd),
        "curl -sL --max-time 10 \"https://en.wikipedia.org/api/rest_v1/page/summary/%s\"",
        encoded
    );
    free(encoded);

    char *json = read_pipe_output(cmd);
    if (!json) {
        return -1;
    }

    if (strstr(json, "\"type\":\"https://mediawiki.org/wiki/HyperSwitch/errors/not_found\"")) {
        free(json);
        return 1;
    }

    article->title = json_extract_string(json, "title");
    article->description = json_extract_string(json, "description");
    article->extract = json_extract_string(json, "extract");
    free(json);

    if (!article->title) {
        return -1;
    }
    if (!article->description) {
        article->description = strdup("No short description available");
    }
    if (!article->extract) {
        article->extract = strdup("No article extract available.");
    }
    if (!article->description || !article->extract) {
        return -1;
    }
    return 0;
}

static char *fetch_full_extract(const char *query) {
    char *encoded = url_encode_title(query);
    if (!encoded) {
        return NULL;
    }

    char cmd[MAX_CMD_LEN];
    snprintf(
        cmd,
        sizeof(cmd),
        "curl -sL --max-time 15 \"https://en.wikipedia.org/w/api.php?action=query&format=json&prop=extracts&explaintext=1&redirects=1&titles=%s\"",
        encoded
    );
    free(encoded);

    char *json = read_pipe_output(cmd);
    if (!json) {
        return NULL;
    }

    char *extract = json_extract_string(json, "extract");
    free(json);
    return extract;
}

static void print_man_layout(const Article *a, const char *argv0) {
    int width = terminal_width();
    if (width > 140) {
        width = 140;
    }

    char header[256];
    snprintf(header, sizeof(header), "%s(1)  Terminal Wiki Manual  %s(1)", argv0, argv0);
    print_center(header, width);
    print_line('-', width);

    puts("NAME");
    char name_line[CROP];
    snprintf(name_line, sizeof(name_line), "%s - %s", a->title, a->description);
    print_wrapped_flat(name_line, width, 4);
    putchar('\n');

    puts("ARTICLE");
    print_wrapped(a->extract, width, 4);

    char date_buf[64];
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    strftime(date_buf, sizeof(date_buf), "%Y-%m-%d", tm_info);

    print_line('-', width);
    char footer[256];
    snprintf(footer, sizeof(footer), "%s  %s", argv0, date_buf);
    print_center(footer, width);
}

int main(int argc, char **argv) {
    bool always_more = false;
    int first_arg = 1;
    if (argc >= 2 && (strcmp(argv[1], "--more") == 0 || strcmp(argv[1], "-m") == 0)) {
        always_more = true;
        first_arg = 2;
    }

    if (argc < first_arg + 1) {
        fprintf(stderr, "Usage: %s [--more|-m] <wikipedia article title>\n", argv[0]);
        fprintf(stderr, "Example: %s \"Alan Turing\"\n", argv[0]);
        return 1;
    }

    char query[1024] = {0};
    for (int i = first_arg; i < argc; i++) {
        strncat(query, argv[i], sizeof(query) - strlen(query) - 1);
        if (i != argc - 1) {
            strncat(query, " ", sizeof(query) - strlen(query) - 1);
        }
    }

    Article a = {0};
    int rc = fetch_article(query, &a);
    if (rc == 1) {
        fprintf(stderr, "No Wikipedia page found for \"%s\".\n", query);
        return 2;
    }
    if (rc != 0) {
        fprintf(stderr, "Failed to fetch Wikipedia page. Ensure `curl` is installed and internet is available.\n");
        return 3;
    }

    print_man_layout(&a, "manwiki");

    bool want_more = always_more;
    if (!want_more && isatty(STDIN_FILENO)) {
        char answer[16] = {0};
        printf("\nShow more from this article? [y/N]: ");
        if (fgets(answer, sizeof(answer), stdin) && (answer[0] == 'y' || answer[0] == 'Y')) {
            want_more = true;
        }
    }

    if (want_more) {
        char *full = fetch_full_extract(query);
        if (full && strcmp(full, a.extract) != 0) {
            puts("\nMORE");
            print_wrapped(full, terminal_width() > 140 ? 140 : terminal_width(), 4);
        } else {
            puts("\nMORE");
            print_wrapped("No additional article text available.", terminal_width() > 140 ? 140 : terminal_width(), 4);
        }
        free(full);
    }

    free_article(&a);
    return 0;
}
