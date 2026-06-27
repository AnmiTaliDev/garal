// SPDX-FileCopyrightText: AnmiTaliDev <anmitalidev@nuros.org>
// SPDX-License-Identifier: GPL-3.0-only

#include "src/parser/parser.h"
#include "src/eval/eval.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define REPL_BUF 4096

static char *read_file(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) { perror(path); return NULL; }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = malloc(size + 1);
    if (!buf) { fclose(f); return NULL; }
    size_t n = fread(buf, 1, size, f);
    buf[n] = '\0';
    fclose(f);
    return buf;
}

static void run_source(Evaluator *ev, const char *src) {
    Parser p;
    parser_init(&p, src);
    Program prog;
    if (parser_parse_program(&p, &prog) == 0)
        eval_program(ev, &prog);
    program_free(&prog);
}

static void run_repl(void) {
    Evaluator ev;
    eval_init(&ev);

    char buf[REPL_BUF];
    printf("Garal 0.1\n");
    while (1) {
        printf("> ");
        fflush(stdout);
        if (!fgets(buf, sizeof(buf), stdin)) break;

        // strip trailing newline
        size_t len = strlen(buf);
        if (len > 0 && buf[len-1] == '\n') buf[len-1] = '\0';
        if (buf[0] == '\0') continue;

        ev.had_error = 0;
        run_source(&ev, buf);
    }
    eval_free(&ev);
}

static void run_script(const char *path) {
    char *src = read_file(path);
    if (!src) { exit(1); }
    Evaluator ev;
    eval_init(&ev);
    run_source(&ev, src);
    int err = ev.had_error;
    eval_free(&ev);
    free(src);
    if (err) exit(1);
}

int main(int argc, char **argv) {
    if (argc == 1) {
        run_repl();
    } else if (argc == 2) {
        run_script(argv[1]);
    } else {
        fprintf(stderr, "usage: garal [script.grl]\n");
        return 1;
    }
    return 0;
}
