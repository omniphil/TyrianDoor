# The TYRIAN door. It doesn't run the game: it sends tyrian.wasm and tyrian.pak to TERMinator, which plays them on
# the player's own machine (see ../module). Built for the BBS box; the same C builds anywhere POSIX.
CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2
LDFLAGS =

OBJ = main.o door.o trace_tyrian.o trace_door.o files.o sha256.o

tyriandoor: $(OBJ)
	$(CC) $(OBJ) -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Everything the sysop needs in one folder: the door, the game and its data.
#   tyrian.wasm  ships in this folder; ../module rebuilds it (needs wasi-sdk) if it's missing
#   tyrian.pak   the freeware Tyrian 2.1 data, downloaded (about 4 MB) and packed the first time; never shipped here
install: tyriandoor
	@if [ -f tyrian.wasm ]; then echo "tyrian.wasm: already here"; \
	elif [ -f ../module/tyrian.wasm ]; then cp ../module/tyrian.wasm . && echo "tyrian.wasm: copied"; \
	else echo "tyrian.wasm: MISSING -- run make in ../module (needs wasi-sdk), then make install again"; fi
	@if [ -f tyrian.pak ]; then echo "tyrian.pak: already here"; \
	else \
	    if [ ! -f ../data/tyrian1.lvl ]; then bash ../third_party/opentyrian/get_data.sh ../data || exit 1; fi; \
	    python3 ../tools/mkpak.py ../data tyrian.pak; \
	fi

clean:
	rm -f $(OBJ) tyriandoor

.PHONY: clean install
