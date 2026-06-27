// SPDX-FileCopyrightText: AnmiTaliDev <anmitalidev@nuros.org>
// SPDX-License-Identifier: GPL-3.0-only

#ifndef GARAL_AST_H
#define GARAL_AST_H

#include <stddef.h>

typedef enum {
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_POW,
    OP_CROSS,
    OP_INTERSECT,
} BinOp;

typedef enum {
    EXPR_INT,
    EXPR_REAL,
    EXPR_IDENT,
    EXPR_BINOP,
    EXPR_NEG,
    EXPR_SET_LITERAL,
    EXPR_SET_RANGE,
    EXPR_MATRIX,
    EXPR_CALL,
    EXPR_LET_IN,
} ExprKind;

typedef struct AstNode AstNode;

struct AstNode {
    ExprKind kind;
    int line;
    union {
        long long ival;
        double rval;
        char *ident;
        struct {
            BinOp op;
            AstNode *left;
            AstNode *right;
        } binop;
        struct {
            AstNode *operand;
        } neg;
        struct {
            AstNode **elems;
            size_t len;
        } set_literal;
        struct {
            AstNode *from;
            AstNode *to;
        } set_range;
        struct {
            AstNode **elems;  // flat row-major
            int rows;
            int cols;
        } matrix;
        struct {
            char *name;
            AstNode **args;
            size_t nargs;
        } call;
        struct {
            char **names;
            AstNode **exprs;
            size_t nassigns;
            AstNode *body;
        } let_in;
    };
};

typedef struct {
    char **params;
    int nparams;
    char **assign_names;
    AstNode **assign_exprs;
    size_t nassigns;
    AstNode *result;
} FnBody;

typedef enum {
    STMT_ASSIGN,
    STMT_PRINT,
    STMT_FN_DEF,
    STMT_EXPR,
} StmtKind;

typedef struct {
    StmtKind kind;
    int line;
    union {
        struct {
            char *name;
            AstNode *expr;
        } assign;
        struct {
            AstNode *expr;
        } print;
        struct {
            char *name;
            FnBody body;
        } fn_def;
        struct {
            AstNode *expr;
        } expr;
    };
} Stmt;

typedef struct {
    Stmt *stmts;
    size_t len;
    size_t cap;
} Program;

AstNode *ast_int(long long val, int line);
AstNode *ast_real(double val, int line);
AstNode *ast_ident(const char *name, int len, int line);
AstNode *ast_binop(BinOp op, AstNode *left, AstNode *right, int line);
AstNode *ast_neg(AstNode *operand, int line);
AstNode *ast_set_literal(AstNode **elems, size_t len, int line);
AstNode *ast_set_range(AstNode *from, AstNode *to, int line);
AstNode *ast_matrix(AstNode **elems, int rows, int cols, int line);
AstNode *ast_call(const char *name, int nlen, AstNode **args, size_t nargs, int line);
AstNode *ast_let_in(char **names, AstNode **exprs, size_t nassigns, AstNode *body, int line);

void ast_node_free(AstNode *node);

void program_init(Program *p);
void program_push(Program *p, Stmt s);
void program_free(Program *p);

#endif
