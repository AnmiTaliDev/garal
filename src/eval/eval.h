// SPDX-FileCopyrightText: AnmiTaliDev <anmitalidev@nuros.org>
// SPDX-License-Identifier: GPL-3.0-only

#ifndef GARAL_EVAL_H
#define GARAL_EVAL_H

#include "ast/ast.h"
#include <stddef.h>

typedef enum {
    VAL_INT,
    VAL_RATIONAL,
    VAL_REAL,
    VAL_SET,
    VAL_MATRIX,
} ValKind;

typedef struct Value Value;
struct Value {
    ValKind kind;
    union {
        long long ival;
        struct { long long num, den; } rat;
        double rval;
        struct {
            Value *elems;
            size_t len;
            size_t cap;
        } set;
        struct {
            double *data;  // row-major
            int rows, cols;
        } mat;
    };
};

typedef struct Binding Binding;
struct Binding {
    char *name;
    Value val;
    Binding *next;
};

typedef struct Env Env;
struct Env {
    Binding *head;
    Env *parent;
};

typedef struct FuncEntry FuncEntry;
struct FuncEntry {
    char *name;
    FnBody body;
    FuncEntry *next;
};

typedef struct {
    Env *global;
    FuncEntry *funcs;
    int had_error;
} Evaluator;

void eval_init(Evaluator *ev);
void eval_free(Evaluator *ev);

Value eval_expr(Evaluator *ev, Env *env, AstNode *node);
void eval_stmt(Evaluator *ev, Stmt *s);
void eval_program(Evaluator *ev, Program *prog);

void value_free(Value *v);
Value value_copy(const Value *v);
void value_print(const Value *v);

// Create values
Value val_int(long long n);
Value val_real(double r);
Value val_rational(long long num, long long den);

Env *env_new(Env *parent);
void env_free(Env *e);
void env_set(Env *e, const char *name, Value v);
Value *env_get(Env *e, const char *name);

#endif
