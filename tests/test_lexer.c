// SPDX-FileCopyrightText: AnmiTaliDev <anmitalidev@nuros.org>
// SPDX-License-Identifier: GPL-3.0-only

#include "../src/lexer/lexer.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static int tests_run = 0;
static int tests_failed = 0;

static void assert_tok(const char *src, TokenType expected) {
    tests_run++;
    Lexer l;
    lexer_init(&l, src);
    Token t = lexer_next(&l);
    if (t.type != expected) {
        fprintf(stderr, "FAIL: lex('%s') -> %s, expected %s\n",
                src, token_type_name(t.type), token_type_name(expected));
        tests_failed++;
    }
}

static void assert_int(const char *src, long long expected) {
    tests_run++;
    Lexer l;
    lexer_init(&l, src);
    Token t = lexer_next(&l);
    if (t.type != TOK_INT || t.ival != expected) {
        fprintf(stderr, "FAIL: int('%s') -> type=%s val=%lld, expected %lld\n",
                src, token_type_name(t.type), t.ival, expected);
        tests_failed++;
    }
}

static void assert_real(const char *src, double expected) {
    tests_run++;
    Lexer l;
    lexer_init(&l, src);
    Token t = lexer_next(&l);
    double diff = t.rval - expected;
    if (t.type != TOK_REAL || (diff < -1e-10 || diff > 1e-10)) {
        fprintf(stderr, "FAIL: real('%s') -> type=%s val=%f, expected %f\n",
                src, token_type_name(t.type), t.rval, expected);
        tests_failed++;
    }
}

static void test_token_sequence(const char *src, TokenType *expected, int n) {
    tests_run++;
    Lexer l;
    lexer_init(&l, src);
    int ok = 1;
    for (int i = 0; i < n; i++) {
        Token t = lexer_next(&l);
        if (t.type != expected[i]) {
            fprintf(stderr, "FAIL: seq('%s')[%d] -> %s, expected %s\n",
                    src, i, token_type_name(t.type), token_type_name(expected[i]));
            ok = 0;
        }
    }
    if (!ok) tests_failed++;
}

int main(void) {
    // basic operators
    assert_tok("+", TOK_PLUS);
    assert_tok("-", TOK_MINUS);
    assert_tok("*", TOK_STAR);
    assert_tok("/", TOK_SLASH);
    assert_tok("^", TOK_CARET);
    assert_tok("=", TOK_EQ);
    assert_tok("=>", TOK_ARROW);
    assert_tok("..", TOK_DOTDOT);
    assert_tok(",", TOK_COMMA);
    assert_tok("(", TOK_LPAREN);
    assert_tok(")", TOK_RPAREN);
    assert_tok("{", TOK_LBRACE);
    assert_tok("}", TOK_RBRACE);
    assert_tok("[", TOK_LBRACKET);
    assert_tok("]", TOK_RBRACKET);

    // unicode operators
    assert_tok("\xc3\x97", TOK_CROSS);       // ×
    assert_tok("\xe2\x88\xa9", TOK_INTERSECT); // ∩

    // keywords
    assert_tok("let", TOK_LET);
    assert_tok("in", TOK_IN);
    assert_tok("fn", TOK_FN);
    assert_tok("print", TOK_PRINT);

    // identifier
    assert_tok("abc", TOK_IDENT);
    assert_tok("A1", TOK_IDENT);
    assert_tok("_x", TOK_IDENT);

    // numbers
    assert_int("42", 42);
    assert_int("0", 0);
    assert_int("1000", 1000);
    assert_real("3.14", 3.14);
    assert_real("0.5", 0.5);

    // comment skipping
    assert_tok("// comment\n42", TOK_INT);

    // whitespace skipping
    assert_tok("   +", TOK_PLUS);

    // sequence: A = 2 + 3
    {
        TokenType seq[] = { TOK_IDENT, TOK_EQ, TOK_INT, TOK_PLUS, TOK_INT, TOK_EOF };
        test_token_sequence("A = 2 + 3", seq, 6);
    }

    // sequence: {1..10}
    {
        TokenType seq[] = { TOK_LBRACE, TOK_INT, TOK_DOTDOT, TOK_INT, TOK_RBRACE, TOK_EOF };
        test_token_sequence("{1..10}", seq, 6);
    }

    printf("%d/%d tests passed\n", tests_run - tests_failed, tests_run);
    return tests_failed > 0 ? 1 : 0;
}
