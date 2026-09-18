/*
 * tyrtrace_compat.h -- forced into every file with -include, so OpenTyrian's sources build unchanged.
 *
 * OpenTyrian leaves with exit(): after saving from its own Quit, on a fatal error, or when the window closes. In the
 * sandbox that has to close TERMinator's picture and tell the door as well, and a fatal error's exit code (-1 in a
 * few places) isn't one WebAssembly accepts, so every exit goes to tyrtrace_exit instead (src/tyrtrace.c).
 */

#ifndef TYRTRACE_COMPAT_H
#define TYRTRACE_COMPAT_H

#include <stdlib.h>

void tyrtrace_exit(int code) __attribute__((noreturn));
#define exit(code) tyrtrace_exit(code)

#endif
