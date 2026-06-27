// SPDX-FileCopyrightText: AnmiTaliDev <anmitalidev@nuros.org>
// SPDX-License-Identifier: GPL-3.0-only

#define _POSIX_C_SOURCE 200809L
#include "ast.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static AstNode *node_alloc(ExprKind kind, int line) {
    AstNode *n = calloc(1, sizeof(AstNode));
    if (!n) {
        fprintf(stderr, "out of memory\n");
        exit(1);
    }
    n->kind = kind;
    n->line = line;
    return n;
}

AstNode *ast_int(long long val, int line) {
    AstNode *n = node_alloc(EXPR_INT, line);
    n->ival = val;
    return n;
}

AstNode *ast_real(double val, int line) {
    AstNode *n = node_alloc(EXPR_REAL, line);
    n->rval = val;
    return n;
}

AstNode *ast_ident(const char *name, int len, int line) {
    AstNode *n = node_alloc(EXPR_IDENT, line);
    n->ident = strndup(name, len);
    return n;
}

AstNode *ast_binop(BinOp op, AstNode *left, AstNode *right, int line) {
    AstNode *n = node_alloc(EXPR_BINOP, line);
    n->binop.op = op;
    n->binop.left = left;
    n->binop.right = right;
    return n;
}

AstNode *ast_neg(AstNode *operand, int line) {
    AstNode *n = node_alloc(EXPR_NEG, line);
    n->neg.operand = operand;
    return n;
}

AstNode *ast_set_literal(AstNode **elems, size_t len, int line) {
    AstNode *n = node_alloc(EXPR_SET_LITERAL, line);
    n->set_literal.elems = elems;
    n->set_literal.len = len;
    return n;
}

AstNode *ast_set_range(AstNode *from, AstNode *to, int line) {
    AstNode *n = node_alloc(EXPR_SET_RANGE, line);
    n->set_range.from = from;
    n->set_range.to = to;
    return n;
}

AstNode *ast_matrix(AstNode **elems, int rows, int cols, int line) {
    AstNode *n = node_alloc(EXPR_MATRIX, line);
    n->matrix.elems = elems;
    n->matrix.rows = rows;
    n->matrix.cols = cols;
    return n;
}

AstNode *ast_call(const char *name, int nlen, AstNode **args, size_t nargs, int line) {
    AstNode *n = node_alloc(EXPR_CALL, line);
    n->call.name = strndup(name, nlen);
    n->call.args = args;
    n->call.nargs = nargs;
    return n;
}

AstNode *ast_let_in(char **names, AstNode **exprs, size_t nassigns, AstNode *body, int line) {
    AstNode *n = node_alloc(EXPR_LET_IN, line);
    n->let_in.names = names;
    n->let_in.exprs = exprs;
    n->let_in.nassigns = nassigns;
    n->let_in.body = body;
    return n;
}

void ast_node_free(AstNode *node) {
    if (!node) return;
    switch (node->kind) {
        case EXPR_INT:
        case EXPR_REAL:
            break;
        case EXPR_IDENT:
            free(node->ident);
            break;
        case EXPR_BINOP:
            ast_node_free(node->binop.left);
            ast_node_free(node->binop.right);
            break;
        case EXPR_NEG:
            ast_node_free(node->neg.operand);
            break;
        case EXPR_SET_LITERAL:
            for (size_t i = 0; i < node->set_literal.len; i++)
                ast_node_free(node->set_literal.elems[i]);
            free(node->set_literal.elems);
            break;
        case EXPR_SET_RANGE:
            ast_node_free(node->set_range.from);
            ast_node_free(node->set_range.to);
            break;
        case EXPR_MATRIX:
            for (int i = 0; i < node->matrix.rows * node->matrix.cols; i++)
                ast_node_free(node->matrix.elems[i]);
            free(node->matrix.elems);
            break;
        case EXPR_CALL:
            free(node->call.name);
            for (size_t i = 0; i < node->call.nargs; i++)
                ast_node_free(node->call.args[i]);
            free(node->call.args);
            break;
        case EXPR_LET_IN:
            for (size_t i = 0; i < node->let_in.nassigns; i++) {
                free(node->let_in.names[i]);
                ast_node_free(node->let_in.exprs[i]);
            }
            free(node->let_in.names);
            free(node->let_in.exprs);
            ast_node_free(node->let_in.body);
            break;
    }
    free(node);
}

void program_init(Program *p) {
    p->stmts = NULL;
    p->len = 0;
    p->cap = 0;
}

void program_push(Program *p, Stmt s) {
    if (p->len == p->cap) {
        p->cap = p->cap ? p->cap * 2 : 8;
        p->stmts = realloc(p->stmts, p->cap * sizeof(Stmt));
        if (!p->stmts) {
            fprintf(stderr, "out of memory\n");
            exit(1);
        }
    }
    p->stmts[p->len++] = s;
}

static void fn_body_free(FnBody *b) {
    for (int i = 0; i < b->nparams; i++) free(b->params[i]);
    free(b->params);
    for (size_t i = 0; i < b->nassigns; i++) {
        free(b->assign_names[i]);
        ast_node_free(b->assign_exprs[i]);
    }
    free(b->assign_names);
    free(b->assign_exprs);
    ast_node_free(b->result);
}

void program_free(Program *p) {
    for (size_t i = 0; i < p->len; i++) {
        Stmt *s = &p->stmts[i];
        switch (s->kind) {
            case STMT_ASSIGN:
                free(s->assign.name);
                ast_node_free(s->assign.expr);
                break;
            case STMT_PRINT:
                ast_node_free(s->print.expr);
                break;
            case STMT_FN_DEF:
                free(s->fn_def.name);
                fn_body_free(&s->fn_def.body);
                break;
            case STMT_EXPR:
                ast_node_free(s->expr.expr);
                break;
        }
    }
    free(p->stmts);
    p->stmts = NULL;
    p->len = p->cap = 0;
}
