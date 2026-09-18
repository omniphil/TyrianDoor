# TyrianDoor

The source of the Tyrian module that the TYRIAN BBS door sends to callers using
[TERMinator](https://deadmodemsociety.com/terminator/). The whole game runs on the caller's own PC inside TERMinator's
TRACE sandbox, as WebAssembly.

The game is [OpenTyrian](https://github.com/opentyrian/opentyrian) (GPL-2), included unmodified in
`third_party/opentyrian` (release v2.1.20260913). Everything this project adds is in `module/`: a small stand-in for the
SDL calls OpenTyrian makes, answered by TRACE instead (screen, keyboard, sound, clock, files). See `module/README.md`.

## Build

Needs [wasi-sdk](https://github.com/WebAssembly/wasi-sdk) 34 (`~/tools/wasi-sdk-34.0-x86_64-linux` by default):

```
cd module
make                      # or: make WASI_SDK=/path/to/wasi-sdk
```

That produces `tyrian.wasm`, the exact file the door sends. `module/trace/trace_api.h` is the TRACE module API it
builds against.

## Game data

Not included. The game plays the Tyrian 2.1 data files, which their author released as freeware; OpenTyrian's
`get_data.sh` fetches them. That data is not covered by the GPL.

## Licence

GPL-2, as OpenTyrian (`LICENSE`).
