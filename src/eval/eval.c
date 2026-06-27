// SPDX-FileCopyrightText: AnmiTaliDev <anmitalidev@nuros.org>
// SPDX-License-Identifier: GPL-3.0-only

#define _POSIX_C_SOURCE 200809L
#include "eval.h"
#include "builtins/builtins.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

extern long long rat_gcd(long long a, long long b);
extern void rat_add(long long an, long long ad, long long bn, long long bd, long long *cn, long long *cd);
extern void rat_sub(long long an, long long ad, long long bn, long long bd, long long *cn, long long *cd);
extern void rat_mul(long long an, long long ad, long long bn, long long bd, long long *cn, long long *cd);
extern void rat_div(long long an, long long ad, long long bn, long long bd, long long *cn, long long *cd);

extern void mat_mul(const double *a, const double *b, double *c, int m, int n, int k);

Value val_int(long long n) {
    Value v;
    v.kind = VAL_INT;
    v.ival = n;
    return v;
}

Value val_real(double r) {
    Value v;
    v.kind = VAL_REAL;
    v.rval = r;
    return v;
}

Value val_rational(long long num, long long den) {
    Value v;
    v.kind = VAL_RATIONAL;
    long long g = rat_gcd(num < 0 ? -num : num, den < 0 ? -den : den);
    if (g == 0) { v.rat.num = 0; v.rat.den = 1; return v; }
    v.rat.num = num / g;
    v.rat.den = den / g;
    if (v.rat.den < 0) { v.rat.num = -v.rat.num; v.rat.den = -v.rat.den; }
    return v;
}

void value_free(Value *v) {
    if (!v) return;
    if (v->kind == VAL_SET) {
        for (size_t i = 0; i < v->set.len; i++)
            value_free(&v->set.elems[i]);
        free(v->set.elems);
        v->set.elems = NULL;
        v->set.len = v->set.cap = 0;
    } else if (v->kind == VAL_MATRIX) {
        free(v->mat.data);
        v->mat.data = NULL;
    }
}

Value value_copy(const Value *v) {
    Value out = *v;
    if (v->kind == VAL_SET) {
        out.set.elems = malloc(v->set.len * sizeof(Value));
        for (size_t i = 0; i < v->set.len; i++)
            out.set.elems[i] = value_copy(&v->set.elems[i]);
        out.set.cap = v->set.len;
    } else if (v->kind == VAL_MATRIX) {
        size_t sz = (size_t)(v->mat.rows * v->mat.cols) * sizeof(double);
        out.mat.data = malloc(sz);
        memcpy(out.mat.data, v->mat.data, sz);
    }
    return out;
}

static double val_to_double(const Value *v) {
    switch (v->kind) {
        case VAL_INT: return (double)v->ival;
        case VAL_RATIONAL: return (double)v->rat.num / (double)v->rat.den;
        case VAL_REAL: return v->rval;
        default: return 0.0;
    }
}

static int val_compare(const Value *a, const Value *b) {
    double da = val_to_double(a);
    double db = val_to_double(b);
    if (da < db) return -1;
    if (da > db) return 1;
    return 0;
}

static int val_equal(const Value *a, const Value *b) {
    return val_compare(a, b) == 0;
}

void value_print(const Value *v) {
    switch (v->kind) {
        case VAL_INT:
            printf("%lld", v->ival);
            break;
        case VAL_RATIONAL:
            if (v->rat.den == 1) printf("%lld", v->rat.num);
            else printf("%lld/%lld", v->rat.num, v->rat.den);
            break;
        case VAL_REAL: {
            char buf[64];
            snprintf(buf, sizeof(buf), "%.10g", v->rval);
            printf("%s", buf);
            break;
        }
        case VAL_SET:
            printf("{");
            for (size_t i = 0; i < v->set.len; i++) {
                if (i > 0) printf(", ");
                value_print(&v->set.elems[i]);
            }
            printf("}");
            break;
        case VAL_MATRIX:
            printf("[");
            for (int r = 0; r < v->mat.rows; r++) {
                if (r > 0) printf(", ");
                printf("[");
                for (int c = 0; c < v->mat.cols; c++) {
                    if (c > 0) printf(", ");
                    char buf[64];
                    double val = v->mat.data[r * v->mat.cols + c];
                    long long ival = (long long)round(val);
                    if (fabs(val - ival) < 1e-9) printf("%lld", ival);
                    else { snprintf(buf, sizeof(buf), "%.10g", val); printf("%s", buf); }
                }
                printf("]");
            }
            printf("]");
            break;
    }
}


Env *env_new(Env *parent) {
    Env *e = calloc(1, sizeof(Env));
    e->parent = parent;
    return e;
}

void env_free(Env *e) {
    if (!e) return;
    Binding *b = e->head;
    while (b) {
        Binding *next = b->next;
        free(b->name);
        value_free(&b->val);
        free(b);
        b = next;
    }
    free(e);
}

void env_set(Env *e, const char *name, Value v) {
    // update existing binding in this scope
    for (Binding *b = e->head; b; b = b->next) {
        if (strcmp(b->name, name) == 0) {
            value_free(&b->val);
            b->val = v;
            return;
        }
    }
    Binding *b = calloc(1, sizeof(Binding));
    b->name = strdup(name);
    b->val = v;
    b->next = e->head;
    e->head = b;
}

Value *env_get(Env *e, const char *name) {
    for (Env *cur = e; cur; cur = cur->parent) {
        for (Binding *b = cur->head; b; b = b->next) {
            if (strcmp(b->name, name) == 0) return &b->val;
        }
    }
    return NULL;
}


static Value scalar_add(const Value *a, const Value *b) {
    if (a->kind == VAL_REAL || b->kind == VAL_REAL)
        return val_real(val_to_double(a) + val_to_double(b));
    if (a->kind == VAL_INT && b->kind == VAL_INT)
        return val_int(a->ival + b->ival);
    long long an = a->kind == VAL_INT ? a->ival : a->rat.num;
    long long ad = a->kind == VAL_INT ? 1 : a->rat.den;
    long long bn = b->kind == VAL_INT ? b->ival : b->rat.num;
    long long bd = b->kind == VAL_INT ? 1 : b->rat.den;
    long long cn, cd;
    rat_add(an, ad, bn, bd, &cn, &cd);
    if (cd == 1) return val_int(cn);
    return val_rational(cn, cd);
}

static Value scalar_sub(const Value *a, const Value *b) {
    if (a->kind == VAL_REAL || b->kind == VAL_REAL)
        return val_real(val_to_double(a) - val_to_double(b));
    if (a->kind == VAL_INT && b->kind == VAL_INT)
        return val_int(a->ival - b->ival);
    long long an = a->kind == VAL_INT ? a->ival : a->rat.num;
    long long ad = a->kind == VAL_INT ? 1 : a->rat.den;
    long long bn = b->kind == VAL_INT ? b->ival : b->rat.num;
    long long bd = b->kind == VAL_INT ? 1 : b->rat.den;
    long long cn, cd;
    rat_sub(an, ad, bn, bd, &cn, &cd);
    if (cd == 1) return val_int(cn);
    return val_rational(cn, cd);
}

static Value scalar_mul(const Value *a, const Value *b) {
    if (a->kind == VAL_REAL || b->kind == VAL_REAL)
        return val_real(val_to_double(a) * val_to_double(b));
    if (a->kind == VAL_INT && b->kind == VAL_INT)
        return val_int(a->ival * b->ival);
    long long an = a->kind == VAL_INT ? a->ival : a->rat.num;
    long long ad = a->kind == VAL_INT ? 1 : a->rat.den;
    long long bn = b->kind == VAL_INT ? b->ival : b->rat.num;
    long long bd = b->kind == VAL_INT ? 1 : b->rat.den;
    long long cn, cd;
    rat_mul(an, ad, bn, bd, &cn, &cd);
    if (cd == 1) return val_int(cn);
    return val_rational(cn, cd);
}

static Value scalar_div(const Value *a, const Value *b) {
    if (a->kind == VAL_REAL || b->kind == VAL_REAL)
        return val_real(val_to_double(a) / val_to_double(b));
    long long an = a->kind == VAL_INT ? a->ival : a->rat.num;
    long long ad = a->kind == VAL_INT ? 1 : a->rat.den;
    long long bn = b->kind == VAL_INT ? b->ival : b->rat.num;
    long long bd = b->kind == VAL_INT ? 1 : b->rat.den;
    if (bn == 0) {
        fprintf(stderr, "division by zero\n");
        return val_int(0);
    }
    long long cn, cd;
    rat_div(an, ad, bn, bd, &cn, &cd);
    if (cd == 1) return val_int(cn);
    return val_rational(cn, cd);
}

static Value scalar_pow(const Value *base, const Value *exp) {
    if (exp->kind == VAL_INT && exp->ival >= 0) {
        long long n = exp->ival;
        if (base->kind == VAL_INT) {
            long long result = 1;
            long long b = base->ival;
            for (long long i = 0; i < n; i++) result *= b;
            return val_int(result);
        }
    }
    return val_real(pow(val_to_double(base), val_to_double(exp)));
}


static int cmp_values(const void *a, const void *b) {
    return val_compare((const Value *)a, (const Value *)b);
}

static Value make_set(Value *elems, size_t len) {
    if (len > 1) qsort(elems, len, sizeof(Value), cmp_values);
    size_t j = 0;
    for (size_t i = 0; i < len; i++) {
        if (j == 0 || !val_equal(&elems[j-1], &elems[i])) {
            elems[j++] = elems[i];
        } else {
            value_free(&elems[i]);
        }
    }
    Value v;
    v.kind = VAL_SET;
    v.set.elems = elems;
    v.set.len = j;
    v.set.cap = len;
    return v;
}

static Value set_union(const Value *a, const Value *b) {
    size_t total = a->set.len + b->set.len;
    Value *elems = malloc(total * sizeof(Value));
    for (size_t i = 0; i < a->set.len; i++) elems[i] = value_copy(&a->set.elems[i]);
    for (size_t i = 0; i < b->set.len; i++) elems[a->set.len + i] = value_copy(&b->set.elems[i]);
    return make_set(elems, total);
}

static Value set_difference(const Value *a, const Value *b) {
    Value *elems = malloc(a->set.len * sizeof(Value));
    size_t len = 0;
    for (size_t i = 0; i < a->set.len; i++) {
        int found = 0;
        for (size_t j = 0; j < b->set.len; j++) {
            if (val_equal(&a->set.elems[i], &b->set.elems[j])) { found = 1; break; }
        }
        if (!found) elems[len++] = value_copy(&a->set.elems[i]);
    }
    return make_set(elems, len);
}

static Value set_intersection(const Value *a, const Value *b) {
    Value *elems = malloc(a->set.len * sizeof(Value));
    size_t len = 0;
    for (size_t i = 0; i < a->set.len; i++) {
        for (size_t j = 0; j < b->set.len; j++) {
            if (val_equal(&a->set.elems[i], &b->set.elems[j])) {
                elems[len++] = value_copy(&a->set.elems[i]);
                break;
            }
        }
    }
    return make_set(elems, len);
}

static Value set_cartesian(const Value *a, const Value *b) {
    size_t total = a->set.len * b->set.len;
    Value *elems = malloc(total * sizeof(Value));
    size_t k = 0;
    for (size_t i = 0; i < a->set.len; i++) {
        for (size_t j = 0; j < b->set.len; j++) {
            Value pair;
            pair.kind = VAL_SET;
            pair.set.len = 2;
            pair.set.cap = 2;
            pair.set.elems = malloc(2 * sizeof(Value));
            pair.set.elems[0] = value_copy(&a->set.elems[i]);
            pair.set.elems[1] = value_copy(&b->set.elems[j]);
            elems[k++] = pair;
        }
    }
    Value v;
    v.kind = VAL_SET;
    v.set.elems = elems;
    v.set.len = total;
    v.set.cap = total;
    return v;
}

static Value scalar_mul_set(const Value *scalar, const Value *set) {
    Value *elems = malloc(set->set.len * sizeof(Value));
    for (size_t i = 0; i < set->set.len; i++)
        elems[i] = scalar_mul(scalar, &set->set.elems[i]);
    return make_set(elems, set->set.len);
}


static Value matrix_elemwise(const Value *a, const Value *b, BinOp op) {
    if (a->mat.rows != b->mat.rows || a->mat.cols != b->mat.cols) {
        fprintf(stderr, "matrix dimension mismatch for elementwise op\n");
        return val_int(0);
    }
    int sz = a->mat.rows * a->mat.cols;
    double *data = malloc(sz * sizeof(double));
    for (int i = 0; i < sz; i++) {
        switch (op) {
            case OP_ADD: data[i] = a->mat.data[i] + b->mat.data[i]; break;
            case OP_SUB: data[i] = a->mat.data[i] - b->mat.data[i]; break;
            default: data[i] = 0.0; break;
        }
    }
    Value v;
    v.kind = VAL_MATRIX;
    v.mat.data = data;
    v.mat.rows = a->mat.rows;
    v.mat.cols = a->mat.cols;
    return v;
}

static Value matrix_multiply(const Value *a, const Value *b) {
    if (a->mat.cols != b->mat.rows) {
        fprintf(stderr, "matrix multiply: incompatible dimensions (%dx%d) * (%dx%d)\n",
                a->mat.rows, a->mat.cols, b->mat.rows, b->mat.cols);
        return val_int(0);
    }
    int m = a->mat.rows, n = a->mat.cols, k = b->mat.cols;
    double *data = calloc(m * k, sizeof(double));
    mat_mul(a->mat.data, b->mat.data, data, m, n, k);
    Value v;
    v.kind = VAL_MATRIX;
    v.mat.data = data;
    v.mat.rows = m;
    v.mat.cols = k;
    return v;
}

static Value matrix_pow(const Value *base, long long n) {
    if (base->mat.rows != base->mat.cols) {
        fprintf(stderr, "matrix power: matrix must be square\n");
        return val_int(0);
    }
    int sz = base->mat.rows;
    // start with identity
    double *result = calloc(sz * sz, sizeof(double));
    for (int i = 0; i < sz; i++) result[i * sz + i] = 1.0;

    Value cur = value_copy(base);
    while (n > 0) {
        if (n & 1) {
            double *tmp = calloc(sz * sz, sizeof(double));
            mat_mul(result, cur.mat.data, tmp, sz, sz, sz);
            free(result);
            result = tmp;
        }
        n >>= 1;
        if (n > 0) {
            double *tmp = calloc(sz * sz, sizeof(double));
            mat_mul(cur.mat.data, cur.mat.data, tmp, sz, sz, sz);
            free(cur.mat.data);
            cur.mat.data = tmp;
        }
    }
    value_free(&cur);

    Value v;
    v.kind = VAL_MATRIX;
    v.mat.data = result;
    v.mat.rows = sz;
    v.mat.cols = sz;
    return v;
}


Value eval_expr(Evaluator *ev, Env *env, AstNode *node) {
    switch (node->kind) {
        case EXPR_INT: return val_int(node->ival);
        case EXPR_REAL: return val_real(node->rval);

        case EXPR_IDENT: {
            Value *v = env_get(env, node->ident);
            if (!v) {
                fprintf(stderr, "line %d: undefined variable '%s'\n", node->line, node->ident);
                ev->had_error = 1;
                return val_int(0);
            }
            return value_copy(v);
        }

        case EXPR_NEG: {
            Value operand = eval_expr(ev, env, node->neg.operand);
            switch (operand.kind) {
                case VAL_INT: operand.ival = -operand.ival; return operand;
                case VAL_RATIONAL: operand.rat.num = -operand.rat.num; return operand;
                case VAL_REAL: operand.rval = -operand.rval; return operand;
                default:
                    fprintf(stderr, "line %d: cannot negate this type\n", node->line);
                    ev->had_error = 1;
                    value_free(&operand);
                    return val_int(0);
            }
        }

        case EXPR_BINOP: {
            Value left = eval_expr(ev, env, node->binop.left);
            Value right = eval_expr(ev, env, node->binop.right);
            BinOp op = node->binop.op;
            Value result;

            if (left.kind == VAL_MATRIX && right.kind == VAL_MATRIX) {
                switch (op) {
                    case OP_ADD: result = matrix_elemwise(&left, &right, OP_ADD); break;
                    case OP_SUB: result = matrix_elemwise(&left, &right, OP_SUB); break;
                    case OP_MUL: result = matrix_multiply(&left, &right); break;
                    case OP_POW:
                        if (right.kind == VAL_INT) result = matrix_pow(&left, right.ival);
                        else { fprintf(stderr, "matrix power exponent must be integer\n"); ev->had_error = 1; result = val_int(0); }
                        break;
                    default:
                        fprintf(stderr, "line %d: unsupported operation on matrices\n", node->line);
                        ev->had_error = 1;
                        result = val_int(0);
                }
                value_free(&left); value_free(&right);
                return result;
            }

            if (left.kind == VAL_SET && right.kind == VAL_SET) {
                switch (op) {
                    case OP_ADD: result = set_union(&left, &right); break;
                    case OP_SUB: result = set_difference(&left, &right); break;
                    case OP_CROSS: result = set_cartesian(&left, &right); break;
                    case OP_INTERSECT: result = set_intersection(&left, &right); break;
                    default:
                        fprintf(stderr, "line %d: unsupported operation on sets\n", node->line);
                        ev->had_error = 1;
                        result = val_int(0);
                }
                value_free(&left); value_free(&right);
                return result;
            }

            if (left.kind == VAL_SET && (right.kind == VAL_INT || right.kind == VAL_RATIONAL || right.kind == VAL_REAL)) {
                if (op == OP_MUL) { result = scalar_mul_set(&right, &left); value_free(&left); value_free(&right); return result; }
            }
            if (right.kind == VAL_SET && (left.kind == VAL_INT || left.kind == VAL_RATIONAL || left.kind == VAL_REAL)) {
                if (op == OP_MUL) { result = scalar_mul_set(&left, &right); value_free(&left); value_free(&right); return result; }
            }

            if (left.kind == VAL_MATRIX && op == OP_POW) {
                if (right.kind == VAL_INT) result = matrix_pow(&left, right.ival);
                else { fprintf(stderr, "matrix power exponent must be integer\n"); ev->had_error = 1; result = val_int(0); }
                value_free(&left); value_free(&right);
                return result;
            }

            switch (op) {
                case OP_ADD: result = scalar_add(&left, &right); break;
                case OP_SUB: result = scalar_sub(&left, &right); break;
                case OP_MUL: result = scalar_mul(&left, &right); break;
                case OP_DIV: result = scalar_div(&left, &right); break;
                case OP_POW: result = scalar_pow(&left, &right); break;
                default:
                    fprintf(stderr, "line %d: operator not applicable here\n", node->line);
                    ev->had_error = 1;
                    result = val_int(0);
            }
            value_free(&left); value_free(&right);
            return result;
        }

        case EXPR_SET_LITERAL: {
            Value *elems = malloc(node->set_literal.len * sizeof(Value));
            for (size_t i = 0; i < node->set_literal.len; i++)
                elems[i] = eval_expr(ev, env, node->set_literal.elems[i]);
            return make_set(elems, node->set_literal.len);
        }

        case EXPR_SET_RANGE: {
            Value from = eval_expr(ev, env, node->set_range.from);
            Value to = eval_expr(ev, env, node->set_range.to);
            if (from.kind != VAL_INT || to.kind != VAL_INT) {
                fprintf(stderr, "line %d: set range bounds must be integers\n", node->line);
                ev->had_error = 1;
                value_free(&from); value_free(&to);
                return val_int(0);
            }
            long long a = from.ival, b = to.ival;
            if (b < a) {
                Value v; v.kind = VAL_SET; v.set.elems = NULL; v.set.len = 0; v.set.cap = 0;
                return v;
            }
            size_t len = (size_t)(b - a + 1);
            Value *elems = malloc(len * sizeof(Value));
            for (size_t i = 0; i < len; i++) elems[i] = val_int(a + (long long)i);
            Value v; v.kind = VAL_SET; v.set.elems = elems; v.set.len = len; v.set.cap = len;
            return v;
        }

        case EXPR_MATRIX: {
            int rows = node->matrix.rows, cols = node->matrix.cols;
            double *data = malloc(rows * cols * sizeof(double));
            for (int i = 0; i < rows * cols; i++) {
                Value elem = eval_expr(ev, env, node->matrix.elems[i]);
                switch (elem.kind) {
                    case VAL_INT: data[i] = (double)elem.ival; break;
                    case VAL_RATIONAL: data[i] = (double)elem.rat.num / (double)elem.rat.den; break;
                    case VAL_REAL: data[i] = elem.rval; break;
                    default:
                        fprintf(stderr, "line %d: matrix elements must be scalars\n", node->line);
                        ev->had_error = 1;
                        data[i] = 0.0;
                }
                value_free(&elem);
            }
            Value v;
            v.kind = VAL_MATRIX;
            v.mat.data = data;
            v.mat.rows = rows;
            v.mat.cols = cols;
            return v;
        }

        case EXPR_CALL: {
            const char *name = node->call.name;
            Value *args = malloc(node->call.nargs * sizeof(Value));
            for (size_t i = 0; i < node->call.nargs; i++)
                args[i] = eval_expr(ev, env, node->call.args[i]);

            Value result;

            if (builtin_exists(name)) {
                result = builtin_call(ev, name, args, node->call.nargs, node->line);
            } else {
                FuncEntry *fe = NULL;
                for (FuncEntry *f = ev->funcs; f; f = f->next) {
                    if (strcmp(f->name, name) == 0) { fe = f; break; }
                }
                if (!fe) {
                    fprintf(stderr, "line %d: unknown function '%s'\n", node->line, name);
                    ev->had_error = 1;
                    for (size_t i = 0; i < node->call.nargs; i++) value_free(&args[i]);
                    free(args);
                    return val_int(0);
                }
                if ((int)node->call.nargs != fe->body.nparams) {
                    fprintf(stderr, "line %d: function '%s' expects %d args, got %zu\n",
                            node->line, name, fe->body.nparams, node->call.nargs);
                    ev->had_error = 1;
                    for (size_t i = 0; i < node->call.nargs; i++) value_free(&args[i]);
                    free(args);
                    return val_int(0);
                }
                Env *fenv = env_new(ev->global);
                for (int i = 0; i < fe->body.nparams; i++)
                    env_set(fenv, fe->body.params[i], args[i]);
                for (size_t i = 0; i < fe->body.nassigns; i++) {
                    Value v = eval_expr(ev, fenv, fe->body.assign_exprs[i]);
                    env_set(fenv, fe->body.assign_names[i], v);
                }
                result = eval_expr(ev, fenv, fe->body.result);
                env_free(fenv);
            }

            for (size_t i = 0; i < node->call.nargs; i++) value_free(&args[i]);
            free(args);
            return result;
        }

        case EXPR_LET_IN: {
            Env *local = env_new(env);
            for (size_t i = 0; i < node->let_in.nassigns; i++) {
                Value v = eval_expr(ev, local, node->let_in.exprs[i]);
                env_set(local, node->let_in.names[i], v);
            }
            Value result = eval_expr(ev, local, node->let_in.body);
            env_free(local);
            return result;
        }
    }

    return val_int(0);
}

void eval_stmt(Evaluator *ev, Stmt *s) {
    switch (s->kind) {
        case STMT_ASSIGN: {
            Value v = eval_expr(ev, ev->global, s->assign.expr);
            env_set(ev->global, s->assign.name, v);
            break;
        }
        case STMT_PRINT: {
            Value v = eval_expr(ev, ev->global, s->print.expr);
            value_print(&v);
            printf("\n");
            value_free(&v);
            break;
        }
        case STMT_FN_DEF: {
            FuncEntry *fe = calloc(1, sizeof(FuncEntry));
            fe->name = strdup(s->fn_def.name);
            fe->body.nparams = s->fn_def.body.nparams;
            fe->body.params = malloc(fe->body.nparams * sizeof(char *));
            for (int i = 0; i < fe->body.nparams; i++)
                fe->body.params[i] = strdup(s->fn_def.body.params[i]);
            fe->body.nassigns = s->fn_def.body.nassigns;
            if (fe->body.nassigns) {
                fe->body.assign_names = malloc(fe->body.nassigns * sizeof(char *));
                fe->body.assign_exprs = malloc(fe->body.nassigns * sizeof(AstNode *));
                for (size_t i = 0; i < fe->body.nassigns; i++) {
                    fe->body.assign_names[i] = strdup(s->fn_def.body.assign_names[i]);
                    fe->body.assign_exprs[i] = s->fn_def.body.assign_exprs[i];
                }
            }
            fe->body.result = s->fn_def.body.result;
            fe->next = ev->funcs;
            ev->funcs = fe;
            break;
        }
        case STMT_EXPR: {
            Value v = eval_expr(ev, ev->global, s->expr.expr);
            value_print(&v);
            printf("\n");
            value_free(&v);
            break;
        }
    }
}

void eval_program(Evaluator *ev, Program *prog) {
    for (size_t i = 0; i < prog->len && !ev->had_error; i++)
        eval_stmt(ev, &prog->stmts[i]);
}

void eval_init(Evaluator *ev) {
    ev->global = env_new(NULL);
    ev->funcs = NULL;
    ev->had_error = 0;
}

void eval_free(Evaluator *ev) {
    env_free(ev->global);
    ev->global = NULL;
    FuncEntry *fe = ev->funcs;
    while (fe) {
        FuncEntry *next = fe->next;
        free(fe->name);
        for (int i = 0; i < fe->body.nparams; i++) free(fe->body.params[i]);
        free(fe->body.params);
        for (size_t i = 0; i < fe->body.nassigns; i++) free(fe->body.assign_names[i]);
        free(fe->body.assign_names);
        free(fe->body.assign_exprs);
        free(fe);
        fe = next;
    }
    ev->funcs = NULL;
}
