# TyrianDoor

Tyrian, the 1995 vertical shooter, as a BBS door for callers using [TERMinator](https://deadmodemsociety.com/terminator/).
The whole game runs on the caller's own PC inside TERMinator's TRACE sandbox, as WebAssembly, with its music and
sound; the BBS sends it once per caller and keeps each player's saves.

The game is [OpenTyrian](https://github.com/opentyrian/opentyrian) (GPL-2), included unmodified in
`third_party/opentyrian` (release v2.1.20260913).

## Running it on your BBS

On a Linux BBS box with `gcc`, `make`, `python3`, `curl` and `unzip`:

```
git clone https://github.com/omniphil/TyrianDoor
cd TyrianDoor/door
make              # builds the door, tyriandoor
make install      # downloads the freeware Tyrian 2.1 data and packs it into tyrian.pak
```

Then add a door entry that runs `tyriandoor <folder holding door32.sys>`. The details, including Mystic's settings
and where each player's saves are kept, are in [`door/INSTALL.md`](door/INSTALL.md). Callers need TERMinator 1.1.2
or newer; anyone else is told so and sent back to the BBS.

`door/tyrian.wasm` is prebuilt from `module/` at the same commit, so no WebAssembly toolchain is needed to run the
door.

| Folder | What |
|---|---|
| `door/` | the BBS side: detects TERMinator, sends the game and data by hash (once per caller), keeps saves per player |
| `module/` | the game side: a small stand-in for the SDL calls OpenTyrian makes, answered by TRACE (see `module/README.md`) |
| `tools/mkpak.py` | packs the Tyrian data files into the one asset the door sends |
| `third_party/opentyrian` | OpenTyrian, unmodified |

## Building the module

Only needed if you change it. Needs [wasi-sdk](https://github.com/WebAssembly/wasi-sdk) 34
(`~/tools/wasi-sdk-34.0-x86_64-linux` by default):

```
cd module
make                      # or: make WASI_SDK=/path/to/wasi-sdk
cp tyrian.wasm ../door/
```

`module/trace/trace_api.h` is the TRACE module API it builds against. If you change the module, point
`TYRIAN_SOURCE_URL` in `door/main.c` at your own copy of the source, since that's where the door tells players to find
it.

## Game data

Not included. The game plays the Tyrian 2.1 data files, which their author released as freeware; `make install`
fetches them with OpenTyrian's `get_data.sh`. That data is not covered by the GPL.

## Licence

GPL-2, as OpenTyrian (`LICENSE`).
