/*
 * SDL.h -- the part of SDL2 that OpenTyrian uses, answered by TRACE instead (src/sdl_trace.c).
 *
 * OpenTyrian is written against SDL2. There is no SDL in the sandbox, so this header declares just the types,
 * constants and calls the game's own files use, with SDL's own values where the game stores them (scancodes end up in
 * the config file, so they must be SDL's numbers). Anything else won't compile, which is deliberate: a new SDL call in
 * a future OpenTyrian shows up here rather than failing quietly at run time.
 *
 * video.c, video_scale*.c and file.c are not built at all; src/video_trace.c and src/file_trace.c replace them.
 */

#ifndef TYRTRACE_SDL_H
#define TYRTRACE_SDL_H

#include <math.h>          /* the real SDL_stdinc.h brings these in, and the game relies on it */
#include <stdarg.h>
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* ---- SDL_types.h / SDL_stdinc.h ---- */

typedef int8_t   Sint8;
typedef uint8_t  Uint8;
typedef int16_t  Sint16;
typedef uint16_t Uint16;
typedef int32_t  Sint32;
typedef uint32_t Uint32;
typedef int64_t  Sint64;
typedef uint64_t Uint64;

typedef enum { SDL_FALSE = 0, SDL_TRUE = 1 } SDL_bool;

#define SDL_VERSION_ATLEAST(x, y, z) 1          /* behave like a current SDL 2 */
#define SDL_PRINTF_FORMAT_STRING
#define SDL_PRINTF_VARARG_FUNC(n) __attribute__((format(printf, n, n + 1)))

#define SDL_free free
#define SDL_vsnprintf vsnprintf
size_t SDL_strlcpy(char *dst, const char *src, size_t size);

/* ---- SDL_endian.h: WebAssembly is little-endian ---- */

#define SDL_LIL_ENDIAN 1234
#define SDL_BIG_ENDIAN 4321
#define SDL_BYTEORDER  SDL_LIL_ENDIAN

#define SDL_SwapLE16(x) ((Uint16)(x))
#define SDL_SwapLE32(x) ((Uint32)(x))
#define SDL_SwapBE16(x) ((Uint16)(((Uint16)(x) << 8) | ((Uint16)(x) >> 8)))
#define SDL_SwapBE32(x) __builtin_bswap32((Uint32)(x))

/* ---- init, errors, logging ---- */

#define SDL_INIT_AUDIO    0x00000010u
#define SDL_INIT_VIDEO    0x00000020u
#define SDL_INIT_JOYSTICK 0x00000200u

int  SDL_Init(Uint32 flags);
int  SDL_InitSubSystem(Uint32 flags);
void SDL_QuitSubSystem(Uint32 flags);
void SDL_Quit(void);
const char *SDL_GetError(void);
int  SDL_SetHint(const char *name, const char *value);
#define SDL_HINT_MOUSE_RELATIVE_SYSTEM_SCALE "SDL_MOUSE_RELATIVE_SYSTEM_SCALE"

/* A module has no folder of its own: data and user files come from src/file_trace.c instead */
char *SDL_GetBasePath(void);

enum { SDL_LOG_CATEGORY_APPLICATION = 0 };
typedef enum
{
    SDL_LOG_PRIORITY_VERBOSE = 1, SDL_LOG_PRIORITY_DEBUG, SDL_LOG_PRIORITY_INFO, SDL_LOG_PRIORITY_WARN,
    SDL_LOG_PRIORITY_ERROR, SDL_LOG_PRIORITY_CRITICAL,
} SDL_LogPriority;
void SDL_LogSetPriority(int category, SDL_LogPriority priority);
void SDL_LogMessageV(int category, SDL_LogPriority priority, const char *fmt, va_list ap);
void SDL_Log(const char *fmt, ...) SDL_PRINTF_VARARG_FUNC(1);
void SDL_LogDebug(int category, const char *fmt, ...) SDL_PRINTF_VARARG_FUNC(2);
void SDL_LogWarn(int category, const char *fmt, ...) SDL_PRINTF_VARARG_FUNC(2);
void SDL_LogError(int category, const char *fmt, ...) SDL_PRINTF_VARARG_FUNC(2);

#define SDL_MESSAGEBOX_ERROR 0x10
typedef struct SDL_Window SDL_Window;
int SDL_ShowSimpleMessageBox(Uint32 flags, const char *title, const char *message, SDL_Window *window);

/* ---- time ---- */

Uint32 SDL_GetTicks(void);
void   SDL_Delay(Uint32 ms);

/* ---- surfaces: 8-bit, in memory, which is all the game draws into ---- */

typedef struct SDL_Color { Uint8 r, g, b, a; } SDL_Color;
typedef struct SDL_Rect { int x, y, w, h; } SDL_Rect;

typedef struct SDL_PixelFormat
{
    Uint32 format;
    Uint8  BitsPerPixel;
    Uint8  BytesPerPixel;
} SDL_PixelFormat;

typedef struct SDL_Surface
{
    Uint32 flags;
    SDL_PixelFormat *format;
    int w, h;
    int pitch;
    void *pixels;
} SDL_Surface;

#define SDL_MUSTLOCK(s) 0
#define SDL_PIXELFORMAT_RGB888 0x16161804u
#define SDL_PIXELFORMAT_RGB565 0x15151002u

SDL_Surface *SDL_CreateRGBSurface(Uint32 flags, int width, int height, int depth,
                                  Uint32 rmask, Uint32 gmask, Uint32 bmask, Uint32 amask);
void SDL_FreeSurface(SDL_Surface *surface);
int  SDL_FillRect(SDL_Surface *dst, const SDL_Rect *rect, Uint32 color);
int  SDL_BlitSurface(SDL_Surface *src, const SDL_Rect *srcrect, SDL_Surface *dst, SDL_Rect *dstrect);

/* TRACE's frames are BGRA (0xAARRGGBB), so that is what a palette entry maps to */
Uint32 SDL_MapRGB(const SDL_PixelFormat *format, Uint8 r, Uint8 g, Uint8 b);

/* ---- windows: there is one picture, TERMinator's, so these only have to exist ---- */

typedef struct SDL_Renderer SDL_Renderer;
typedef struct SDL_Texture SDL_Texture;

#define SDL_ENABLE  1
#define SDL_DISABLE 0
#define SDL_IGNORE  0
#define SDL_QUERY  -1
/* No displays to choose: the setup menu then offers only "Window", and TERMinator decides the rest */
static inline int SDL_GetNumVideoDisplays(void) { return 0; }
int  SDL_ShowCursor(int toggle);
int  SDL_SetRelativeMouseMode(SDL_bool enabled);

/* ---- keyboard ---- */

typedef enum
{
    SDL_SCANCODE_UNKNOWN = 0,

    SDL_SCANCODE_A = 4, SDL_SCANCODE_B, SDL_SCANCODE_C, SDL_SCANCODE_D, SDL_SCANCODE_E, SDL_SCANCODE_F,
    SDL_SCANCODE_G, SDL_SCANCODE_H, SDL_SCANCODE_I, SDL_SCANCODE_J, SDL_SCANCODE_K, SDL_SCANCODE_L,
    SDL_SCANCODE_M, SDL_SCANCODE_N, SDL_SCANCODE_O, SDL_SCANCODE_P, SDL_SCANCODE_Q, SDL_SCANCODE_R,
    SDL_SCANCODE_S, SDL_SCANCODE_T, SDL_SCANCODE_U, SDL_SCANCODE_V, SDL_SCANCODE_W, SDL_SCANCODE_X,
    SDL_SCANCODE_Y, SDL_SCANCODE_Z,

    SDL_SCANCODE_1 = 30, SDL_SCANCODE_2, SDL_SCANCODE_3, SDL_SCANCODE_4, SDL_SCANCODE_5,
    SDL_SCANCODE_6, SDL_SCANCODE_7, SDL_SCANCODE_8, SDL_SCANCODE_9, SDL_SCANCODE_0,

    SDL_SCANCODE_RETURN = 40, SDL_SCANCODE_ESCAPE = 41, SDL_SCANCODE_BACKSPACE = 42, SDL_SCANCODE_TAB = 43,
    SDL_SCANCODE_SPACE = 44, SDL_SCANCODE_MINUS = 45, SDL_SCANCODE_EQUALS = 46, SDL_SCANCODE_LEFTBRACKET = 47,
    SDL_SCANCODE_RIGHTBRACKET = 48, SDL_SCANCODE_BACKSLASH = 49, SDL_SCANCODE_NONUSHASH = 50,
    SDL_SCANCODE_SEMICOLON = 51, SDL_SCANCODE_APOSTROPHE = 52, SDL_SCANCODE_GRAVE = 53, SDL_SCANCODE_COMMA = 54,
    SDL_SCANCODE_PERIOD = 55, SDL_SCANCODE_SLASH = 56, SDL_SCANCODE_CAPSLOCK = 57,

    SDL_SCANCODE_F1 = 58, SDL_SCANCODE_F2, SDL_SCANCODE_F3, SDL_SCANCODE_F4, SDL_SCANCODE_F5, SDL_SCANCODE_F6,
    SDL_SCANCODE_F7, SDL_SCANCODE_F8, SDL_SCANCODE_F9, SDL_SCANCODE_F10, SDL_SCANCODE_F11, SDL_SCANCODE_F12,

    SDL_SCANCODE_PRINTSCREEN = 70, SDL_SCANCODE_SCROLLLOCK = 71, SDL_SCANCODE_PAUSE = 72, SDL_SCANCODE_INSERT = 73,
    SDL_SCANCODE_HOME = 74, SDL_SCANCODE_PAGEUP = 75, SDL_SCANCODE_DELETE = 76, SDL_SCANCODE_END = 77,
    SDL_SCANCODE_PAGEDOWN = 78, SDL_SCANCODE_RIGHT = 79, SDL_SCANCODE_LEFT = 80, SDL_SCANCODE_DOWN = 81,
    SDL_SCANCODE_UP = 82,

    SDL_SCANCODE_NUMLOCKCLEAR = 83, SDL_SCANCODE_KP_DIVIDE = 84, SDL_SCANCODE_KP_MULTIPLY = 85,
    SDL_SCANCODE_KP_MINUS = 86, SDL_SCANCODE_KP_PLUS = 87, SDL_SCANCODE_KP_ENTER = 88,
    SDL_SCANCODE_KP_1 = 89, SDL_SCANCODE_KP_2, SDL_SCANCODE_KP_3, SDL_SCANCODE_KP_4, SDL_SCANCODE_KP_5,
    SDL_SCANCODE_KP_6, SDL_SCANCODE_KP_7, SDL_SCANCODE_KP_8, SDL_SCANCODE_KP_9, SDL_SCANCODE_KP_0,
    SDL_SCANCODE_KP_PERIOD = 99, SDL_SCANCODE_NONUSBACKSLASH = 100,

    SDL_SCANCODE_LCTRL = 224, SDL_SCANCODE_LSHIFT = 225, SDL_SCANCODE_LALT = 226, SDL_SCANCODE_LGUI = 227,
    SDL_SCANCODE_RCTRL = 228, SDL_SCANCODE_RSHIFT = 229, SDL_SCANCODE_RALT = 230, SDL_SCANCODE_RGUI = 231,

    SDL_NUM_SCANCODES = 512
} SDL_Scancode;

typedef Sint32 SDL_Keycode;
#define SDLK_SCANCODE_MASK (1 << 30)
#define SDL_SCANCODE_TO_KEYCODE(x) ((SDL_Keycode)(x) | SDLK_SCANCODE_MASK)
enum
{
    SDLK_UNKNOWN = 0,
    SDLK_RIGHTBRACKET = ']',
    SDLK_a = 'a', SDLK_d = 'd', SDLK_g = 'g', SDLK_l = 'l', SDLK_o = 'o', SDLK_r = 'r', SDLK_s = 's',
};

typedef enum
{
    KMOD_NONE = 0x0000,
    KMOD_LSHIFT = 0x0001, KMOD_RSHIFT = 0x0002,
    KMOD_LCTRL = 0x0040, KMOD_RCTRL = 0x0080,
    KMOD_LALT = 0x0100, KMOD_RALT = 0x0200,
    KMOD_LGUI = 0x0400, KMOD_RGUI = 0x0800,
    KMOD_NUM = 0x1000, KMOD_CAPS = 0x2000,
    KMOD_CTRL = KMOD_LCTRL | KMOD_RCTRL,
    KMOD_SHIFT = KMOD_LSHIFT | KMOD_RSHIFT,
    KMOD_ALT = KMOD_LALT | KMOD_RALT,
    KMOD_GUI = KMOD_LGUI | KMOD_RGUI,
} SDL_Keymod;

typedef struct SDL_Keysym
{
    SDL_Scancode scancode;
    SDL_Keycode sym;
    Uint16 mod;
} SDL_Keysym;

const char  *SDL_GetScancodeName(SDL_Scancode scancode);
SDL_Scancode SDL_GetScancodeFromName(const char *name);
void SDL_StartTextInput(void);
void SDL_StopTextInput(void);

/* ---- events ---- */

#define SDL_RELEASED 0
#define SDL_PRESSED  1

typedef enum
{
    SDL_QUIT = 0x100,
    SDL_WINDOWEVENT = 0x200,
    SDL_KEYDOWN = 0x300, SDL_KEYUP, SDL_TEXTEDITING, SDL_TEXTINPUT,
    SDL_MOUSEMOTION = 0x400, SDL_MOUSEBUTTONDOWN, SDL_MOUSEBUTTONUP, SDL_MOUSEWHEEL,
} SDL_EventType;

enum
{
    SDL_WINDOWEVENT_RESIZED = 5,
    SDL_WINDOWEVENT_FOCUS_GAINED = 12,
    SDL_WINDOWEVENT_FOCUS_LOST = 13,
};

#define SDL_BUTTON(x)      (1 << ((x) - 1))
#define SDL_BUTTON_LEFT    1
#define SDL_BUTTON_MIDDLE  2
#define SDL_BUTTON_RIGHT   3
#define SDL_BUTTON_LMASK   SDL_BUTTON(SDL_BUTTON_LEFT)
#define SDL_BUTTON_MMASK   SDL_BUTTON(SDL_BUTTON_MIDDLE)
#define SDL_BUTTON_RMASK   SDL_BUTTON(SDL_BUTTON_RIGHT)

typedef struct SDL_KeyboardEvent { Uint32 type; Uint8 state; Uint8 repeat; SDL_Keysym keysym; } SDL_KeyboardEvent;
typedef struct SDL_TextInputEvent { Uint32 type; char text[32]; } SDL_TextInputEvent;
typedef struct SDL_WindowEvent { Uint32 type; Uint8 event; Sint32 data1, data2; } SDL_WindowEvent;
typedef struct SDL_MouseMotionEvent { Uint32 type; Uint32 state; Sint32 x, y, xrel, yrel; } SDL_MouseMotionEvent;
typedef struct SDL_MouseButtonEvent { Uint32 type; Uint8 button, state, clicks; Sint32 x, y; } SDL_MouseButtonEvent;

typedef union SDL_Event
{
    Uint32 type;
    SDL_KeyboardEvent key;
    SDL_TextInputEvent text;
    SDL_WindowEvent window;
    SDL_MouseMotionEvent motion;
    SDL_MouseButtonEvent button;
} SDL_Event;

int SDL_PollEvent(SDL_Event *event);
int SDL_PushEvent(SDL_Event *event);

/* ---- joysticks: TERMinator's gamepads (engine contract TE_IN_PAD_*, TRACE clients that answer pad=1) ----
 * Polled, never event-driven: OpenTyrian's joystick.c calls SDL_JoystickEventState(SDL_IGNORE) and then reads
 * the axes, buttons and hat every frame, so sdl_trace.c just reports the state tyrtrace.c is keeping.
 * A pad is the Xbox layout in SDL GameController order, so Tyrian's own defaults (axes 0/1 and hat 0 to move,
 * the first six buttons to fire) land on the left stick, the d-pad and A/B/X/Y without any configuring. */

typedef struct SDL_Joystick SDL_Joystick;
#define SDL_HAT_CENTERED 0x00
#define SDL_HAT_UP       0x01
#define SDL_HAT_RIGHT    0x02
#define SDL_HAT_DOWN     0x04
#define SDL_HAT_LEFT     0x08

int          SDL_NumJoysticks(void);
SDL_Joystick *SDL_JoystickOpen(int index);
void         SDL_JoystickClose(SDL_Joystick *j);
const char  *SDL_JoystickName(SDL_Joystick *j);
int          SDL_JoystickNumAxes(SDL_Joystick *j);
int          SDL_JoystickNumButtons(SDL_Joystick *j);
int          SDL_JoystickNumHats(SDL_Joystick *j);
Sint16       SDL_JoystickGetAxis(SDL_Joystick *j, int n);
Uint8        SDL_JoystickGetButton(SDL_Joystick *j, int n);
Uint8        SDL_JoystickGetHat(SDL_Joystick *j, int n);
void         SDL_JoystickUpdate(void);
int          SDL_JoystickEventState(int state);

/* ---- audio ----
 * One device, run by the game's own thread: src/sdl_trace.c calls the game's callback whenever TERMinator's queue
 * has room, from SDL_Delay, SDL_PollEvent and every presented frame, which between them run many times a frame. With
 * one thread there is nothing to lock. */

typedef Uint16 SDL_AudioFormat;
#define AUDIO_S8     0x8008
#define AUDIO_S16LSB 0x8010
#define AUDIO_S16SYS AUDIO_S16LSB
#define SDL_AUDIO_ALLOW_FREQUENCY_CHANGE 0x01
#define SDL_AUDIO_ALLOW_SAMPLES_CHANGE   0x08

typedef void (*SDL_AudioCallback)(void *userdata, Uint8 *stream, int len);
typedef Uint32 SDL_AudioDeviceID;

typedef struct SDL_AudioSpec
{
    int freq;
    SDL_AudioFormat format;
    Uint8 channels;
    Uint8 silence;
    Uint16 samples;
    Uint32 size;
    SDL_AudioCallback callback;
    void *userdata;
} SDL_AudioSpec;

SDL_AudioDeviceID SDL_OpenAudioDevice(const char *device, int iscapture, const SDL_AudioSpec *desired,
                                      SDL_AudioSpec *obtained, int allowed_changes);
void SDL_CloseAudioDevice(SDL_AudioDeviceID dev);
void SDL_PauseAudioDevice(SDL_AudioDeviceID dev, int pause_on);
static inline void SDL_LockAudioDevice(SDL_AudioDeviceID dev) { (void)dev; }
static inline void SDL_UnlockAudioDevice(SDL_AudioDeviceID dev) { (void)dev; }

/* Only the one conversion the game asks for: its 8-bit 11025 Hz samples to 16-bit at the output rate */
typedef struct SDL_AudioCVT
{
    int needed;
    SDL_AudioFormat src_format, dst_format;
    int src_rate, dst_rate;
    Uint8 *buf;
    int len;
    int len_cvt;
    int len_mult;
    double len_ratio;
} SDL_AudioCVT;

int SDL_BuildAudioCVT(SDL_AudioCVT *cvt, SDL_AudioFormat src_format, Uint8 src_channels, int src_rate,
                      SDL_AudioFormat dst_format, Uint8 dst_channels, int dst_rate);
int SDL_ConvertAudio(SDL_AudioCVT *cvt);

#endif /* TYRTRACE_SDL_H */
