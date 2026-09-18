# Builds tyrian.wasm: the whole of Tyrian (game, graphics, music and sound) as a TRACE module the door sends to
# TERMinator.
#
# OpenTyrian comes from ../third_party unmodified. The files that would talk to SDL and the operating system are
# replaced: video.c and video_scale*.c by src/video_trace.c, file.c by src/file_trace.c, and the SDL calls left in the
# game's own files are answered by include/SDL.h + src/sdl_trace.c. Built without networking. Run in WSL/Linux with
# wasi-sdk.
WASI_SDK ?= $(HOME)/tools/wasi-sdk-34.0-x86_64-linux
CC = $(WASI_SDK)/bin/clang

OPENTYRIAN = ../third_party/opentyrian
# trace_api.h: the client's own tree when building here, or the copy bundled with the published source, so the
# released source builds on its own without a checkout of TERMinator beside it.
TRACE_INCLUDE = $(firstword $(wildcard ../../../TERMinator-Windows/trace/include) trace)

# Shown on the title screen: the OpenTyrian release in ../third_party
OPENTYRIAN_VERSION ?= v2.1.20260913

CFLAGS = --target=wasm32-wasip1-threads -O2 -pthread -std=gnu11 -DNDEBUG \
         -Wall -Wno-unused-variable -Wno-unused-but-set-variable -Wno-unused-function \
         -D_WASI_EMULATED_SIGNAL -D_WASI_EMULATED_PROCESS_CLOCKS \
         -DOPENTYRIAN_VERSION='"$(OPENTYRIAN_VERSION)"' \
         -include tyrtrace_compat.h -I include -I src -I $(TRACE_INCLUDE) -I $(OPENTYRIAN)/src
LDFLAGS = -mexec-model=reactor -Wl,--import-memory,--export-memory,--shared-memory,--max-memory=268435456 \
          -Wl,--export=wasi_thread_start -Wl,--strip-all -Wl,-z,stack-size=1048576

# The game, less the files replaced here
REPLACED = video.c video_scale.c video_scale_hqNx.c file.c
GAME_SRC = $(filter-out $(addprefix $(OPENTYRIAN)/src/, $(REPLACED)), $(wildcard $(OPENTYRIAN)/src/*.c))

# Our platform layer
TRACE_SRC = $(wildcard src/*.c)

SRC = $(GAME_SRC) $(TRACE_SRC)
OBJ = $(patsubst %.c, obj/%.o, $(notdir $(SRC)))
VPATH = src:$(OPENTYRIAN)/src

tyrian.wasm: $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) $(LDFLAGS) -o $@

# -MMD writes each object's header dependencies, so editing a header rebuilds what uses it
obj/%.o: %.c | obj
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

-include $(OBJ:.o=.d)

# OpenTyrian's main() runs on the module's game thread (src/tyrtrace.c), under another name
obj/opentyr.o: $(OPENTYRIAN)/src/opentyr.c | obj
	$(CC) $(CFLAGS) -Dmain=opentyrian_main -MMD -MP -c $< -o $@

obj:
	mkdir -p obj

clean:
	rm -rf obj tyrian.wasm

.PHONY: clean
