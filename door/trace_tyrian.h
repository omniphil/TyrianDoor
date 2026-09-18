/* trace_tyrian.h -- sending Tyrian to the player's terminal and starting it; see trace_tyrian.c. */

#ifndef TRACE_TYRIAN_H
#define TRACE_TYRIAN_H

#include <stdbool.h>
#include <stddef.h>

/* Reads tyrian.wasm and tyrian.pak from beside the door binary. False means the door isn't installed properly. */
bool trace_tyrian_load_files(void);

/* Does this terminal have TRACE, with everything Tyrian needs (its own module, assets, sound, sending back)? */
bool trace_tyrian_detect(void);

/* Makes sure the terminal has the game data, uploading it if this is the player's first game. */
bool trace_tyrian_send_data(void (*progress)(int));

/* Starts the game on the player's machine (uploading it the first time), with the player's own files. */
bool trace_tyrian_open(void);

/* Waits until the player quits, or their time runs out, keeping the files the game sends back. */
void trace_tyrian_wait(void);

/* Sends a message to the game: a line of text, and optionally a payload after it. */
void trace_tyrian_send(const char *head, const void *payload, size_t len);

/* Stops the game if it's still running. */
void trace_tyrian_close(void);

#endif
