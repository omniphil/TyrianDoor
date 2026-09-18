/* tyrtrace.h -- what the TRACE platform layer's files share with each other (see tyrtrace.c). */

#ifndef TYRTRACE_H
#define TYRTRACE_H

#include <stddef.h>
#include <stdint.h>

/* One event from TERMinator (engine contract TE_IN_*), queued for the game thread */
typedef struct
{
    int32_t type, flags, a, b, c;
} tyrtrace_event_t;

#define TE_IN_KEY   1   /* flags bit0 pressed, bit1 extended (E0); a = set-1 scancode */
#define TE_IN_TEXT  2   /* a = UTF-16 code unit of a typed character */
#define TE_IN_FOCUS 5   /* flags bit0 focused */
#define TE_IN_QUIT  6   /* the player closed the picture, or the door went away */

void tyrtrace_log(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

/* Events, oldest first; 0 when there are none */
int  tyrtrace_next_event(tyrtrace_event_t *out);

/* The game's data: one packed asset the door sent (tools/mkpak.py). NULL when there is no such file. */
const uint8_t *tyrtrace_data_file(const char *name, size_t *size);

/* The player's files (saves and settings), which live on the BBS: what the door sent at the start, and changes sent
 * back up (file_trace.c). */
void tyrtrace_user_file_received(const char *name, size_t off, size_t total, const uint8_t *data, size_t len);
const uint8_t *tyrtrace_user_file(const char *name, size_t *size);
void tyrtrace_user_file_written(const char *name, const uint8_t *data, size_t size);
int  tyrtrace_user_files_pump(void);   /* sends what it can; returns 1 while anything is still waiting to go */

/* Everything that has to happen regularly on the game thread: sound, and saves going up to the BBS. Called from
 * SDL_Delay, SDL_PollEvent and every frame, so it runs whatever the game is waiting for. */
void tyrtrace_pump(void);
void tyrtrace_pump_audio(void);

/* The game is done: saves are made sure of, then TERMinator is told to close the picture. Does not return. */
void tyrtrace_exit(int code) __attribute__((noreturn));

#endif
