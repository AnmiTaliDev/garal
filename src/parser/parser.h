// SPDX-FileCopyrightText: AnmiTaliDev <anmitalidev@nuros.org>
// SPDX-License-Identifier: GPL-3.0-only

#ifndef GARAL_PARSER_H
#define GARAL_PARSER_H

#include "lexer/lexer.h"
#include "ast/ast.h"

typedef struct {
    Lexer lexer;
    Token cur;
    int had_error;
} Parser;

void parser_init(Parser *p, const char *src);
int parser_parse_program(Parser *p, Program *prog);
AstNode *parser_parse_expr(Parser *p);

#endif
