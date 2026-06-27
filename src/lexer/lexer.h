// SPDX-FileCopyrightText: AnmiTaliDev <anmitalidev@nuros.org>
// SPDX-License-Identifier: GPL-3.0-only

#ifndef GARAL_LEXER_H
#define GARAL_LEXER_H

typedef enum {
    TOK_EOF = 0,

    TOK_INT,
    TOK_REAL,
    TOK_IDENT,

    TOK_PLUS,
    TOK_MINUS,
    TOK_STAR,
    TOK_SLASH,
    TOK_CARET,
    TOK_CROSS,      // × U+00D7
    TOK_INTERSECT,  // ∩ U+2229

    TOK_LPAREN,
    TOK_RPAREN,
    TOK_LBRACE,
    TOK_RBRACE,
    TOK_LBRACKET,
    TOK_RBRACKET,
    TOK_COMMA,
    TOK_DOTDOT,
    TOK_EQ,
    TOK_ARROW,      // =>

    TOK_LET,
    TOK_IN,
    TOK_FN,
    TOK_PRINT,

    TOK_ERROR,
} TokenType;

typedef struct {
    TokenType type;
    const char *start;
    int len;
    int line;
    union {
        long long ival;
        double rval;
    };
} Token;

typedef struct {
    const char *src;
    const char *cur;
    int line;
} Lexer;

void lexer_init(Lexer *l, const char *src);
Token lexer_next(Lexer *l);
Token lexer_peek(Lexer *l);
const char *token_type_name(TokenType t);

#endif
