// SPDX-FileCopyrightText: AnmiTaliDev <anmitalidev@nuros.org>
// SPDX-License-Identifier: GPL-3.0-only

#ifndef GARAL_BUILTINS_H
#define GARAL_BUILTINS_H

#include "eval/eval.h"

// Returns 1 if name is a built-in function name.
int builtin_exists(const char *name);

// Call a built-in function. Returns the result value.
// ev is passed for error reporting.
Value builtin_call(Evaluator *ev, const char *name, Value *args, size_t nargs, int line);

#endif
