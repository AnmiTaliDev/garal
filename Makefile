# SPDX-FileCopyrightText: AnmiTaliDev <anmitalidev@nuros.org>
# SPDX-License-Identifier: GPL-3.0-only

CC = gcc
FC = gfortran
CFLAGS = -Wall -Wextra -std=c11 -Iinclude -Isrc -g
FFLAGS = -Wall -std=f2018 -g
LDFLAGS = -lm

SRC_C = src/lexer/lexer.c \
        src/ast/ast.c \
        src/parser/parser.c \
        src/eval/eval.c \
        src/builtins/builtins.c \
        main.c

SRC_F = src/fortran/rational_ops.f90 \
        src/fortran/matrix_ops.f90

OBJ_C = $(SRC_C:.c=.o)
OBJ_F = $(SRC_F:.f90=.o)

LIB_OBJ_C = src/lexer/lexer.o \
            src/ast/ast.o \
            src/parser/parser.o \
            src/eval/eval.o \
            src/builtins/builtins.o

TEST_C = tests/test_lexer.c tests/test_eval.c
TEST_BIN = tests/test_lexer tests/test_eval

TARGET = garal

all: $(TARGET)

$(TARGET): $(OBJ_C) $(OBJ_F)
	$(CC) -o $@ $^ -lgfortran $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.f90
	$(FC) $(FFLAGS) -c -o $@ $<

tests/test_lexer: tests/test_lexer.c src/lexer/lexer.o
	$(CC) $(CFLAGS) -o $@ $^

tests/test_eval: tests/test_eval.c $(LIB_OBJ_C) $(OBJ_F)
	$(CC) $(CFLAGS) -o $@ $^ -lgfortran $(LDFLAGS)

test: $(TEST_BIN)
	@echo "Running lexer tests..."
	@./tests/test_lexer
	@echo "Running eval tests..."
	@./tests/test_eval

clean:
	rm -f $(OBJ_C) $(OBJ_F) $(TARGET) $(TEST_BIN)
	find . -name "*.mod" -delete

.PHONY: all clean test
