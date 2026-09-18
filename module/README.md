# tyrian.wasm — the whole of Tyrian as a TRACE module

OpenTyrian from `../third_party`, compiled to WebAssembly and run by TERMinator's sandbox (`gamesandbox`) on the
player's own machine. **The game's own code is untouched**: everything replaced is the part that would talk to SDL or
the operating system.

## Build

```
make            # needs wasi-sdk in ~/tools (same as the DOOM module)
```

Headless, without a BBS or a window (the pack goes in the assets folder named by its SHA-256):

```
gamesandbox_probe gamesandbox tyrian.wasm -seconds 40 -assets <dir with <sha>.bin> -data pak=<sha> \
    -keys 28@8000,28@9500,28@11000,28@12500,28@14000,28@15500,28@17000,1@18500,336@19500,336@20000,336@20500,\
336@21000,28@22000,336@23500,336@24000,336@24500,336@25000,28@26000,57@32000:8000 -shot game.bmp
```

`-keys` takes set-1 scancodes; 256 + a code is the extended (E0) key (328 Up, 336 Down, 331 Left, 333 Right), and
`:ms` holds it down. The script above goes Start New Game → the game menu → Play Next Level, and fires for 8 s.

## What's in here

| File | Replaces | What it does |
|---|---|---|
| `src/tyrtrace.c` | `main` | The TRACE entry points. OpenTyrian runs on its own thread, so none of its code had to be restructured. Loads the data pack the door names (`pak=<sha256>`), receives the player's files, and makes `exit()` save and close TERMinator's picture. |
| `src/sdl_trace.c` | SDL | The SDL calls the game makes: clock, `SDL_Delay`, keys (set-1 scancodes → SDL scancodes), 8-bit surfaces, the audio callback (run on the game thread whenever TERMinator's queue has room), the 11 kHz → 44.1 kHz sample conversion, log messages. |
| `src/video_trace.c` | `video.c`, `video_scale*.c` | Each shown frame through the game's palette to BGRA, presented at 4:3. Scaling/full screen are TERMinator's. |
| `src/file_trace.c` | `file.c` | Data files from the pack (`fmemopen`), the player's three files in memory, written with `open_memstream` and sent to the BBS when closed. Real stdio streams, because `config_file.c` uses stdio directly. |
| `include/SDL.h` | SDL headers | Only what the game uses, with SDL's own scancode values (they end up in the config file). Anything else won't compile, on purpose. |
| `include/tyrtrace_compat.h` | — | Forced into every file: `exit()` → `tyrtrace_exit()`. |

## Door messages

Down (door → game), a text line then an optional payload: `file name=<n> off=<o> total=<t>\n<bytes>` (the player's
files, before the game starts), `pak=<sha256>` (starts it), `quit` (time's up: save and close).
Up (game → door): `put name=<n> off=<o> total=<t>\n<bytes>`, pieces of 3000 bytes (`door/files.h` FILES_CHUNK).

## Known gaps

- No mouse or joystick (TERMinator doesn't pass them to modules yet); keyboard only.
- No network play (built without SDL_net). Two-player on one keyboard works as in the original.
- Recording demos does nothing (there's nowhere to keep them).
- Loading the player's files from the door is tested only on the door side (`door/test_door.py`); the headless probe
  can't send them, so the first real call is the first end-to-end test of a save coming back.
