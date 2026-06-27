// SPDX-FileCopyrightText: AnmiTaliDev <anmitalidev@nuros.org>
// SPDX-License-Identifier: GPL-3.0-only

#include "../src/parser/parser.h"
#include "../src/eval/eval.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

static int tests_run = 0;
static int tests_failed = 0;

static Value eval_str(Evaluator *ev, const char *src) {
    Parser p;
    parser_init(&p, src);
    AstNode *node = parser_parse_expr(&p);
    Value v = eval_expr(ev, ev->global, node);
    ast_node_free(node);
    return v;
}

static void check_int(const char *src, long long expected) {
    tests_run++;
    Evaluator ev;
    eval_init(&ev);
    Value v = eval_str(&ev, src);
    if (v.kind != VAL_INT || v.ival != expected) {
        fprintf(stderr, "FAIL: eval('%s') -> kind=%d val=%lld, expected INT %lld\n",
                src, v.kind, v.kind == VAL_INT ? v.ival : -1, expected);
        tests_failed++;
    }
    value_free(&v);
    eval_free(&ev);
}

static void check_real(const char *src, double expected, double tol) {
    tests_run++;
    Evaluator ev;
    eval_init(&ev);
    Value v = eval_str(&ev, src);
    double got = 0.0;
    if (v.kind == VAL_REAL) got = v.rval;
    else if (v.kind == VAL_INT) got = (double)v.ival;
    double diff = got - expected;
    if (diff < 0) diff = -diff;
    if (diff > tol) {
        fprintf(stderr, "FAIL: eval('%s') -> %f, expected %f\n", src, got, expected);
        tests_failed++;
    }
    value_free(&v);
    eval_free(&ev);
}

static void check_rat(const char *src, long long num, long long den) {
    tests_run++;
    Evaluator ev;
    eval_init(&ev);
    Value v = eval_str(&ev, src);
    if (v.kind == VAL_INT && den == 1 && v.ival == num) {
        value_free(&v); eval_free(&ev); return;
    }
    if (v.kind != VAL_RATIONAL || v.rat.num != num || v.rat.den != den) {
        fprintf(stderr, "FAIL: eval('%s') -> kind=%d num=%lld den=%lld, expected %lld/%lld\n",
                src, v.kind,
                v.kind == VAL_RATIONAL ? v.rat.num : v.ival,
                v.kind == VAL_RATIONAL ? v.rat.den : 1LL,
                num, den);
        tests_failed++;
    }
    value_free(&v);
    eval_free(&ev);
}

static void check_set_len(const char *src, size_t expected_len) {
    tests_run++;
    Evaluator ev;
    eval_init(&ev);
    Value v = eval_str(&ev, src);
    if (v.kind != VAL_SET || v.set.len != expected_len) {
        fprintf(stderr, "FAIL: eval('%s') set len -> %zu, expected %zu\n",
                src, v.kind == VAL_SET ? v.set.len : (size_t)-1, expected_len);
        tests_failed++;
    }
    value_free(&v);
    eval_free(&ev);
}

int main(void) {
    // scalar arithmetic
    check_int("2 + 2", 4);
    check_int("10 - 3", 7);
    check_int("3 * 4", 12);
    check_int("2 ^ 8", 256);
    check_int("-(5)", -5);
    check_int("2 + 3 * 4", 14);      // precedence
    check_int("(2 + 3) * 4", 20);

    // rational arithmetic
    check_rat("1 / 2 + 1 / 2", 1, 1);  // simplifies to INT 1
    check_rat("1 / 3 + 1 / 6", 1, 2);
    check_rat("2 / 4", 1, 2);

    // real arithmetic
    check_real("3.14 * 2.0", 6.28, 1e-9);
    check_real("sqrt(4.0)", 2.0, 1e-9);
    check_real("sqrt(2.0)", 1.4142135623730951, 1e-9);

    // set range
    check_set_len("{1..5}", 5);
    check_set_len("{1..1}", 1);
    check_set_len("{5..4}", 0);

    // set literal
    check_set_len("{1, 2, 3}", 3);
    check_set_len("{1, 2, 2, 3}", 3);  // deduplication

    // let-in
    check_int("let A = 3 B = A * 2 in B + 1", 7);

    printf("%d/%d tests passed\n", tests_run - tests_failed, tests_run);
    return tests_failed > 0 ? 1 : 0;
}
