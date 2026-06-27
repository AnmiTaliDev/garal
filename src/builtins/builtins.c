// SPDX-FileCopyrightText: AnmiTaliDev <anmitalidev@nuros.org>
// SPDX-License-Identifier: GPL-3.0-only

#include "builtins.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

extern long long rat_gcd(long long a, long long b);
extern void mat_transpose(const double *a, double *b, int m, int n);
extern double mat_det(const double *a, int n);
extern int mat_rank(const double *a, int m, int n);

static const char *BUILTINS[] = {
    "det", "transpose", "rank", "len", "abs", "floor", "ceil",
    "sqrt", "gcd", "lcm", NULL
};

int builtin_exists(const char *name) {
    for (int i = 0; BUILTINS[i]; i++)
        if (strcmp(BUILTINS[i], name) == 0) return 1;
    return 0;
}

static double val_to_double(const Value *v) {
    switch (v->kind) {
        case VAL_INT: return (double)v->ival;
        case VAL_RATIONAL: return (double)v->rat.num / (double)v->rat.den;
        case VAL_REAL: return v->rval;
        default: return 0.0;
    }
}

static Value scalar_abs(const Value *v) {
    switch (v->kind) {
        case VAL_INT: return val_int(v->ival < 0 ? -v->ival : v->ival);
        case VAL_RATIONAL: {
            Value r = *v;
            if (r.rat.num < 0) r.rat.num = -r.rat.num;
            return r;
        }
        case VAL_REAL: return val_real(fabs(v->rval));
        default: {
            fprintf(stderr, "abs: not a scalar\n");
            return val_int(0);
        }
    }
}

Value builtin_call(Evaluator *ev, const char *name, Value *args, size_t nargs, int line) {
    (void)line;

    if (strcmp(name, "abs") == 0) {
        if (nargs != 1) { fprintf(stderr, "abs: expected 1 argument\n"); ev->had_error = 1; return val_int(0); }
        return scalar_abs(&args[0]);
    }

    if (strcmp(name, "floor") == 0) {
        if (nargs != 1) { fprintf(stderr, "floor: expected 1 argument\n"); ev->had_error = 1; return val_int(0); }
        double x = val_to_double(&args[0]);
        return val_int((long long)floor(x));
    }

    if (strcmp(name, "ceil") == 0) {
        if (nargs != 1) { fprintf(stderr, "ceil: expected 1 argument\n"); ev->had_error = 1; return val_int(0); }
        double x = val_to_double(&args[0]);
        return val_int((long long)ceil(x));
    }

    if (strcmp(name, "sqrt") == 0) {
        if (nargs != 1) { fprintf(stderr, "sqrt: expected 1 argument\n"); ev->had_error = 1; return val_int(0); }
        double x = val_to_double(&args[0]);
        return val_real(sqrt(x));
    }

    if (strcmp(name, "gcd") == 0) {
        if (nargs != 2) { fprintf(stderr, "gcd: expected 2 arguments\n"); ev->had_error = 1; return val_int(0); }
        if (args[0].kind != VAL_INT || args[1].kind != VAL_INT) {
            fprintf(stderr, "gcd: arguments must be integers\n"); ev->had_error = 1; return val_int(0);
        }
        long long g = rat_gcd(args[0].ival < 0 ? -args[0].ival : args[0].ival,
                               args[1].ival < 0 ? -args[1].ival : args[1].ival);
        return val_int(g);
    }

    if (strcmp(name, "lcm") == 0) {
        if (nargs != 2) { fprintf(stderr, "lcm: expected 2 arguments\n"); ev->had_error = 1; return val_int(0); }
        if (args[0].kind != VAL_INT || args[1].kind != VAL_INT) {
            fprintf(stderr, "lcm: arguments must be integers\n"); ev->had_error = 1; return val_int(0);
        }
        long long a = args[0].ival < 0 ? -args[0].ival : args[0].ival;
        long long b = args[1].ival < 0 ? -args[1].ival : args[1].ival;
        long long g = rat_gcd(a, b);
        return val_int(g == 0 ? 0 : a / g * b);
    }

    if (strcmp(name, "len") == 0) {
        if (nargs != 1) { fprintf(stderr, "len: expected 1 argument\n"); ev->had_error = 1; return val_int(0); }
        if (args[0].kind == VAL_SET) return val_int((long long)args[0].set.len);
        if (args[0].kind == VAL_MATRIX) return val_int((long long)(args[0].mat.rows * args[0].mat.cols));
        fprintf(stderr, "len: expected set or matrix\n"); ev->had_error = 1; return val_int(0);
    }

    if (strcmp(name, "transpose") == 0) {
        if (nargs != 1 || args[0].kind != VAL_MATRIX) {
            fprintf(stderr, "transpose: expected one matrix argument\n"); ev->had_error = 1; return val_int(0);
        }
        int m = args[0].mat.rows, n = args[0].mat.cols;
        double *out = malloc(n * m * sizeof(double));
        mat_transpose(args[0].mat.data, out, m, n);
        Value v;
        v.kind = VAL_MATRIX;
        v.mat.data = out;
        v.mat.rows = n;
        v.mat.cols = m;
        return v;
    }

    if (strcmp(name, "det") == 0) {
        if (nargs != 1 || args[0].kind != VAL_MATRIX) {
            fprintf(stderr, "det: expected one matrix argument\n"); ev->had_error = 1; return val_int(0);
        }
        if (args[0].mat.rows != args[0].mat.cols) {
            fprintf(stderr, "det: matrix must be square\n"); ev->had_error = 1; return val_int(0);
        }
        double d = mat_det(args[0].mat.data, args[0].mat.rows);
        // Return as integer if it's very close to an integer
        long long di = (long long)round(d);
        if (fabs(d - di) < 1e-9) return val_int(di);
        return val_real(d);
    }

    if (strcmp(name, "rank") == 0) {
        if (nargs != 1 || args[0].kind != VAL_MATRIX) {
            fprintf(stderr, "rank: expected one matrix argument\n"); ev->had_error = 1; return val_int(0);
        }
        int r = mat_rank(args[0].mat.data, args[0].mat.rows, args[0].mat.cols);
        return val_int((long long)r);
    }

    fprintf(stderr, "unknown built-in: %s\n", name);
    ev->had_error = 1;
    return val_int(0);
}
