// SPDX-FileCopyrightText: AnmiTaliDev <anmitalidev@nuros.org>
// SPDX-License-Identifier: GPL-3.0-only

#include "lexer.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

void lexer_init(Lexer *l, const char *src) {
    l->src = src;
    l->cur = src;
    l->line = 1;
}

static void skip_whitespace_and_comments(Lexer *l) {
    while (*l->cur) {
        unsigned char c = (unsigned char)*l->cur;
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            if (c == '\n') l->line++;
            l->cur++;
        } else if (l->cur[0] == '/' && l->cur[1] == '/') {
            while (*l->cur && *l->cur != '\n') l->cur++;
        } else {
            break;
        }
    }
}

static Token make_token(Lexer *l, TokenType type, const char *start) {
    Token t;
    t.type = type;
    t.start = start;
    t.len = (int)(l->cur - start);
    t.line = l->line;
    t.ival = 0;
    return t;
}

static Token lex_number(Lexer *l) {
    const char *start = l->cur;
    int is_real = 0;

    while (isdigit((unsigned char)*l->cur)) l->cur++;

    if (*l->cur == '.' && isdigit((unsigned char)l->cur[1])) {
        is_real = 1;
        l->cur++;
        while (isdigit((unsigned char)*l->cur)) l->cur++;
    }

    Token t = make_token(l, is_real ? TOK_REAL : TOK_INT, start);
    if (is_real) {
        char *end;
        t.rval = strtod(start, &end);
    } else {
        char *end;
        t.ival = strtoll(start, &end, 10);
    }
    return t;
}

static Token lex_ident_or_keyword(Lexer *l) {
    const char *start = l->cur;
    while (isalnum((unsigned char)*l->cur) || *l->cur == '_') l->cur++;

    Token t = make_token(l, TOK_IDENT, start);
    int len = t.len;

    if (len == 3 && strncmp(start, "let", 3) == 0) t.type = TOK_LET;
    else if (len == 2 && strncmp(start, "in", 2) == 0) t.type = TOK_IN;
    else if (len == 2 && strncmp(start, "fn", 2) == 0) t.type = TOK_FN;
    else if (len == 5 && strncmp(start, "print", 5) == 0) t.type = TOK_PRINT;

    return t;
}

Token lexer_next(Lexer *l) {
    skip_whitespace_and_comments(l);

    if (!*l->cur) {
        Token t;
        t.type = TOK_EOF;
        t.start = l->cur;
        t.len = 0;
        t.line = l->line;
        t.ival = 0;
        return t;
    }

    const char *start = l->cur;
    unsigned char c = (unsigned char)*l->cur;

    // × U+00D7 -> UTF-8: 0xC3 0x97
    if (c == 0xC3 && (unsigned char)l->cur[1] == 0x97) {
        l->cur += 2;
        return make_token(l, TOK_CROSS, start);
    }

    // ∩ U+2229 -> UTF-8: 0xE2 0x88 0xA9
    if (c == 0xE2 && (unsigned char)l->cur[1] == 0x88 && (unsigned char)l->cur[2] == 0xA9) {
        l->cur += 3;
        return make_token(l, TOK_INTERSECT, start);
    }

    if (isdigit(c)) return lex_number(l);
    if (isalpha(c) || c == '_') return lex_ident_or_keyword(l);

    l->cur++;

    switch ((char)c) {
        case '+': return make_token(l, TOK_PLUS, start);
        case '-': return make_token(l, TOK_MINUS, start);
        case '*': return make_token(l, TOK_STAR, start);
        case '/': return make_token(l, TOK_SLASH, start);
        case '^': return make_token(l, TOK_CARET, start);
        case '(': return make_token(l, TOK_LPAREN, start);
        case ')': return make_token(l, TOK_RPAREN, start);
        case '{': return make_token(l, TOK_LBRACE, start);
        case '}': return make_token(l, TOK_RBRACE, start);
        case '[': return make_token(l, TOK_LBRACKET, start);
        case ']': return make_token(l, TOK_RBRACKET, start);
        case ',': return make_token(l, TOK_COMMA, start);
        case '=':
            if (*l->cur == '>') {
                l->cur++;
                return make_token(l, TOK_ARROW, start);
            }
            return make_token(l, TOK_EQ, start);
        case '.':
            if (*l->cur == '.') {
                l->cur++;
                return make_token(l, TOK_DOTDOT, start);
            }
            break;
    }

    return make_token(l, TOK_ERROR, start);
}

Token lexer_peek(Lexer *l) {
    Lexer saved = *l;
    Token t = lexer_next(l);
    *l = saved;
    return t;
}

const char *token_type_name(TokenType t) {
    switch (t) {
        case TOK_EOF:       return "EOF";
        case TOK_INT:       return "INT";
        case TOK_REAL:      return "REAL";
        case TOK_IDENT:     return "IDENT";
        case TOK_PLUS:      return "+";
        case TOK_MINUS:     return "-";
        case TOK_STAR:      return "*";
        case TOK_SLASH:     return "/";
        case TOK_CARET:     return "^";
        case TOK_CROSS:     return "x";
        case TOK_INTERSECT: return "intersect";
        case TOK_LPAREN:    return "(";
        case TOK_RPAREN:    return ")";
        case TOK_LBRACE:    return "{";
        case TOK_RBRACE:    return "}";
        case TOK_LBRACKET:  return "[";
        case TOK_RBRACKET:  return "]";
        case TOK_COMMA:     return ",";
        case TOK_DOTDOT:    return "..";
        case TOK_EQ:        return "=";
        case TOK_ARROW:     return "=>";
        case TOK_LET:       return "let";
        case TOK_IN:        return "in";
        case TOK_FN:        return "fn";
        case TOK_PRINT:     return "print";
        case TOK_ERROR:     return "ERROR";
        default:            return "?";
    }
}
