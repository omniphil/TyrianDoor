# Installing the TYRIAN door on the BBS

The door doesn't run the game. It sends it: TERMinator runs the whole of Tyrian on the player's own machine, so the
BBS only pays for one upload per player, ever. A player on any other terminal gets a polite screen explaining that.

## What the BBS needs

A Linux BBS box (the door finds its own folder the Linux way) with `gcc`, `make`, `python3`, `curl` and `unzip`, and
BBS software that writes a `door32.sys` drop file (tested with Mystic).

| File | Where it comes from | Size |
|---|---|---|
| `tyriandoor` | built here with `make` | ~40 KB |
| `tyrian.wasm` | ships in this folder (or `make` in `../module`, which needs wasi-sdk) | ~400 KB |
| `tyrian.pak` | the freeware Tyrian 2.1 data, downloaded and packed by `make install` | 10.2 MB |

The door looks for `tyrian.wasm` and `tyrian.pak` beside its own binary.

## Steps

1. **Get the whole repository onto the BBS box** (`git clone https://github.com/omniphil/TyrianDoor`), not just this
   folder: `make install` uses `../tools` and `../third_party` to fetch and pack the game data.
2. In `door/`:

   ```
   make            # builds tyriandoor
   make install    # checks tyrian.wasm is here; downloads the Tyrian 2.1 data and packs it into tyrian.pak
   ```

   After that, `door/` holds everything the door needs; the folder can be moved or copied into your BBS's doors
   directory on its own.
3. **Add a door entry** that runs `tyriandoor` with the folder holding `door32.sys` as its one argument. In Mystic:
   `(D3) Exec DOOR32 program` with Data

   ```
   ./doors/tyrian/tyriandoor /path/to/mystic/temp%3
   ```

   (`%3` is the node number.) Without a drop file (Mystic's `(DD) Exec external program`, for one) the door falls
   back to local mode and **every caller shares one `saves/player/` folder**. The door prints "Saved games for:
   <name>" as it starts; `player` there means the drop file isn't reaching it.

Callers need [TERMinator](https://deadmodemsociety.com/terminator/) 1.1.2 or newer with TRACE graphics. Anyone on
another terminal gets a screen saying so, and goes back to the BBS.

## Checking it works

Before letting players in, run the test that impersonates TERMinator. It needs no BBS and no terminal:

```
python3 test_door.py            # the full exchange: the game and data arrive intact, saves go both ways
python3 test_door.py --plain    # a terminal with no TRACE: the door should bow out politely
rm -rf saves                    # the test leaves a saves/player folder behind
```

## Saved games

Tyrian keeps three small files, and they're kept **per player** in `saves/<handle>-<user number>/` beside the door:

| File | What's in it |
|---|---|
| `tyrian.sav` | every saved game, and the high score tables |
| `tyrian.cfg` | the original game's settings (sound, difficulty) |
| `opentyrian.cfg` | OpenTyrian's settings, including the keys |

The door sends all three when the game starts; the game sends a file back whenever it writes it (saving a game,
changing a setting, quitting). A file only replaces the old one once it has arrived whole, and the old one is kept as
`<name>.bak`. If the player's time on the BBS runs out mid-game, the door asks the game to quit, which saves on the way
out, before it closes it.

## Licences

- The game is **OpenTyrian**, GPL-2 (`COPYING-opentyrian`). The door tells players where the source is as they leave
  (https://github.com/omniphil/TyrianDoor). If you change and rebuild `tyrian.wasm`, point `TYRIAN_SOURCE_URL` in
  `main.c` at your own copy of the source.
- The data is **Tyrian 2.1**, which its author released as freeware. The files are sent unchanged. The zip still
  carries the original 1995 commercial licence text (`license.doc`), which predates the freeware release; OpenTyrian's
  own release builds ship the same data on the same basis.
