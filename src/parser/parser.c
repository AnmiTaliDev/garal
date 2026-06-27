// SPDX-FileCopyrightText: AnmiTaliDev <anmitalidev@nuros.org>
// SPDX-License-Identifier: GPL-3.0-only

#define _POSIX_C_SOURCE 200809L
#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void parser_init(Parser *p, const char *src) {
    lexer_init(&p->lexer, src);
    p->cur = lexer_next(&p->lexer);
    p->had_error = 0;
}

static Token peek(Parser *p) {
    return p->cur;
}

static Token advance(Parser *p) {
    Token prev = p->cur;
    p->cur = lexer_next(&p->lexer);
    return prev;
}

static Token expect(Parser *p, TokenType type) {
    if (p->cur.type != type) {
        fprintf(stderr, "line %d: expected '%s', got '%s'\n",
                p->cur.line, token_type_name(type), token_type_name(p->cur.type));
        p->had_error = 1;
        Token t = p->cur;
        return t;
    }
    return advance(p);
}

static Token peek2(Parser *p) {
    Lexer saved = p->lexer;
    Token t = lexer_next(&p->lexer);
    p->lexer = saved;
    return t;
}

static int at_assignment(Parser *p) {
    if (p->cur.type != TOK_IDENT) return 0;
    Token next = peek2(p);
    return next.type == TOK_EQ;
}

static AstNode *parse_expr(Parser *p);

static AstNode *parse_primary(Parser *p) {
    Token t = peek(p);

    if (t.type == TOK_INT) {
        advance(p);
        return ast_int(t.ival, t.line);
    }

    if (t.type == TOK_REAL) {
        advance(p);
        return ast_real(t.rval, t.line);
    }

    if (t.type == TOK_IDENT) {
        Token name = advance(p);
        if (peek(p).type == TOK_LPAREN) {
            advance(p); // consume '('
            AstNode **args = NULL;
            size_t nargs = 0;
            size_t cap = 0;
            if (peek(p).type != TOK_RPAREN) {
                while (1) {
                    if (nargs == cap) {
                        cap = cap ? cap * 2 : 4;
                        args = realloc(args, cap * sizeof(AstNode *));
                    }
                    args[nargs++] = parse_expr(p);
                    if (peek(p).type != TOK_COMMA) break;
                    advance(p); // consume ','
                }
            }
            expect(p, TOK_RPAREN);
            return ast_call(name.start, name.len, args, nargs, name.line);
        }
        return ast_ident(name.start, name.len, name.line);
    }

    if (t.type == TOK_LPAREN) {
        advance(p);
        AstNode *inner = parse_expr(p);
        expect(p, TOK_RPAREN);
        return inner;
    }

    if (t.type == TOK_LBRACE) {
        advance(p); // consume '{'
        int line = t.line;
        AstNode *first = parse_expr(p);
        if (peek(p).type == TOK_DOTDOT) {
            // set range: { from .. to }
            advance(p); // consume '..'
            AstNode *to = parse_expr(p);
            expect(p, TOK_RBRACE);
            return ast_set_range(first, to, line);
        }
        AstNode **elems = NULL;
        size_t len = 0, cap = 0;
        cap = 4;
        elems = malloc(cap * sizeof(AstNode *));
        elems[len++] = first;
        while (peek(p).type == TOK_COMMA) {
            advance(p); // consume ','
            if (len == cap) {
                cap *= 2;
                elems = realloc(elems, cap * sizeof(AstNode *));
            }
            elems[len++] = parse_expr(p);
        }
        expect(p, TOK_RBRACE);
        return ast_set_literal(elems, len, line);
    }

    if (t.type == TOK_LBRACKET) {
        advance(p); // consume outer '['
        int line = t.line;
        AstNode **all_elems = NULL;
        int rows = 0;
        int cols = -1;
        size_t total = 0, cap = 0;

        while (peek(p).type == TOK_LBRACKET) {
            advance(p); // consume '['
            int row_cols = 0;
            while (peek(p).type != TOK_RBRACKET && peek(p).type != TOK_EOF) {
                if (total == cap) {
                    cap = cap ? cap * 2 : 8;
                    all_elems = realloc(all_elems, cap * sizeof(AstNode *));
                }
                all_elems[total++] = parse_expr(p);
                row_cols++;
                if (peek(p).type != TOK_COMMA) break;
                advance(p); // consume ','
            }
            expect(p, TOK_RBRACKET); // consume ']'
            if (cols == -1) {
                cols = row_cols;
            } else if (cols != row_cols) {
                fprintf(stderr, "line %d: inconsistent matrix row lengths\n", line);
                p->had_error = 1;
            }
            rows++;
            if (peek(p).type != TOK_COMMA) break;
            advance(p); // consume ','
        }
        expect(p, TOK_RBRACKET); // consume outer ']'
        if (cols == -1) cols = 0;
        return ast_matrix(all_elems, rows, cols, line);
    }

    if (t.type == TOK_LET) {
        advance(p); // consume 'let'
        int line = t.line;

        char **names = NULL;
        AstNode **exprs = NULL;
        size_t nassigns = 0, cap = 0;

        while (peek(p).type != TOK_IN && peek(p).type != TOK_EOF) {
            if (!at_assignment(p)) {
                fprintf(stderr, "line %d: expected assignment in let block\n", peek(p).line);
                p->had_error = 1;
                break;
            }
            Token name_tok = advance(p); // IDENT
            advance(p); // '='
            AstNode *val = parse_expr(p);

            if (nassigns == cap) {
                cap = cap ? cap * 2 : 4;
                names = realloc(names, cap * sizeof(char *));
                exprs = realloc(exprs, cap * sizeof(AstNode *));
            }
            names[nassigns] = strndup(name_tok.start, name_tok.len);
            exprs[nassigns] = val;
            nassigns++;
        }
        expect(p, TOK_IN);
        AstNode *body = parse_expr(p);
        return ast_let_in(names, exprs, nassigns, body, line);
    }

    fprintf(stderr, "line %d: unexpected token '%s'\n", t.line, token_type_name(t.type));
    p->had_error = 1;
    advance(p);
    return ast_int(0, t.line);
}

static AstNode *parse_unary(Parser *p) {
    Token t = peek(p);
    if (t.type == TOK_MINUS) {
        advance(p);
        AstNode *operand = parse_unary(p);
        return ast_neg(operand, t.line);
    }
    return parse_primary(p);
}

static AstNode *parse_power(Parser *p) {
    AstNode *base = parse_unary(p);
    if (peek(p).type == TOK_CARET) {
        Token op = advance(p);
        AstNode *exp = parse_power(p);
        return ast_binop(OP_POW, base, exp, op.line);
    }
    return base;
}

static AstNode *parse_mul(Parser *p) {
    AstNode *left = parse_power(p);
    while (1) {
        Token t = peek(p);
        BinOp op;
        if (t.type == TOK_STAR)       op = OP_MUL;
        else if (t.type == TOK_SLASH) op = OP_DIV;
        else if (t.type == TOK_CROSS) op = OP_CROSS;
        else if (t.type == TOK_INTERSECT) op = OP_INTERSECT;
        else break;
        advance(p);
        AstNode *right = parse_power(p);
        left = ast_binop(op, left, right, t.line);
    }
    return left;
}

static AstNode *parse_add(Parser *p) {
    AstNode *left = parse_mul(p);
    while (1) {
        Token t = peek(p);
        BinOp op;
        if (t.type == TOK_PLUS)  op = OP_ADD;
        else if (t.type == TOK_MINUS) op = OP_SUB;
        else break;
        advance(p);
        AstNode *right = parse_mul(p);
        left = ast_binop(op, left, right, t.line);
    }
    return left;
}

AstNode *parser_parse_expr(Parser *p) {
    return parse_add(p);
}

static AstNode *parse_expr(Parser *p) {
    return parse_add(p);
}

static void parse_fn_body(Parser *p, FnBody *body) {
    body->assign_names = NULL;
    body->assign_exprs = NULL;
    body->nassigns = 0;
    size_t cap = 0;

    while (at_assignment(p)) {
        Token name_tok = advance(p); // IDENT
        advance(p); // '='
        AstNode *val = parse_expr(p);

        if (body->nassigns == cap) {
            cap = cap ? cap * 2 : 4;
            body->assign_names = realloc(body->assign_names, cap * sizeof(char *));
            body->assign_exprs = realloc(body->assign_exprs, cap * sizeof(AstNode *));
        }
        body->assign_names[body->nassigns] = strndup(name_tok.start, name_tok.len);
        body->assign_exprs[body->nassigns] = val;
        body->nassigns++;
    }

    body->result = parse_expr(p);
}

static Stmt parse_stmt(Parser *p) {
    Token t = peek(p);
    Stmt s;
    s.line = t.line;

    if (t.type == TOK_FN) {
        advance(p); // consume 'fn'
        Token name = expect(p, TOK_IDENT);
        expect(p, TOK_LPAREN);

        char **params = NULL;
        int nparams = 0;
        int pcap = 0;

        if (peek(p).type != TOK_RPAREN) {
            while (1) {
                Token param = expect(p, TOK_IDENT);
                if (nparams == pcap) {
                    pcap = pcap ? pcap * 2 : 4;
                    params = realloc(params, pcap * sizeof(char *));
                }
                params[nparams++] = strndup(param.start, param.len);
                if (peek(p).type != TOK_COMMA) break;
                advance(p);
            }
        }
        expect(p, TOK_RPAREN);
        expect(p, TOK_ARROW);

        s.kind = STMT_FN_DEF;
        s.fn_def.name = strndup(name.start, name.len);
        s.fn_def.body.params = params;
        s.fn_def.body.nparams = nparams;
        parse_fn_body(p, &s.fn_def.body);
        return s;
    }

    if (t.type == TOK_PRINT) {
        advance(p);
        s.kind = STMT_PRINT;
        s.print.expr = parse_expr(p);
        return s;
    }

    if (at_assignment(p)) {
        Token name = advance(p); // IDENT
        advance(p); // '='
        s.kind = STMT_ASSIGN;
        s.assign.name = strndup(name.start, name.len);
        s.assign.expr = parse_expr(p);
        return s;
    }

    s.kind = STMT_EXPR;
    s.expr.expr = parse_expr(p);
    return s;
}

int parser_parse_program(Parser *p, Program *prog) {
    program_init(prog);
    while (peek(p).type != TOK_EOF && !p->had_error) {
        Stmt s = parse_stmt(p);
        program_push(prog, s);
    }
    return p->had_error ? -1 : 0;
}
