/*
 * sdl_trace.c -- the SDL2 calls OpenTyrian makes (include/SDL.h), answered by TRACE.
 *
 * Time comes from trace_time_ms, keys from TERMinator's events, sound goes to TERMinator's mixer, and log messages to
 * its debug output. Surfaces are plain memory. The screen itself is video_trace.c.
 */

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <time.h>

#include "SDL.h"
#include "trace_api.h"
#include "tyrtrace.h"

/* ---- the little things ---- */

size_t SDL_strlcpy(char *dst, const char *src, size_t size)
{
    size_t len = strlen(src);
    if (size > 0)
    {
        size_t n = len < size - 1 ? len : size - 1;
        memcpy(dst, src, n);
        dst[n] = '\0';
    }
    return len;
}

int  SDL_Init(Uint32 flags) { (void)flags; return 0; }
int  SDL_InitSubSystem(Uint32 flags) { (void)flags; return 0; }
void SDL_QuitSubSystem(Uint32 flags) { (void)flags; }
void SDL_Quit(void) { }
const char *SDL_GetError(void) { return "not available"; }
int  SDL_SetHint(const char *name, const char *value) { (void)name; (void)value; return 1; }
char *SDL_GetBasePath(void) { return NULL; }
int  SDL_ShowCursor(int toggle) { (void)toggle; return 0; }
int  SDL_SetRelativeMouseMode(SDL_bool enabled) { (void)enabled; return 0; }

/* ---- logging: to TERMinator's debug output, since a module has no console ---- */

void SDL_LogSetPriority(int category, SDL_LogPriority priority) { (void)category; (void)priority; }

void SDL_LogMessageV(int category, SDL_LogPriority priority, const char *fmt, va_list ap)
{
    static const char *const level[] = { "", "", "debug: ", "", "warning: ", "error: ", "FATAL: " };
    char text[512];
    va_list copy;

    (void)category;
    va_copy(copy, ap);
    vsnprintf(text, sizeof(text), fmt, copy);
    va_end(copy);
    if (text[0])
        tyrtrace_log("tyrian: %s%s", level[priority <= SDL_LOG_PRIORITY_CRITICAL ? priority : 0], text);
}

#define LOG_WITH(priority, fmt) do { va_list ap; va_start(ap, fmt); \
    SDL_LogMessageV(0, priority, fmt, ap); va_end(ap); } while (0)

void SDL_Log(const char *fmt, ...) { LOG_WITH(SDL_LOG_PRIORITY_INFO, fmt); }
void SDL_LogDebug(int category, const char *fmt, ...) { (void)category; LOG_WITH(SDL_LOG_PRIORITY_DEBUG, fmt); }
void SDL_LogWarn(int category, const char *fmt, ...) { (void)category; LOG_WITH(SDL_LOG_PRIORITY_WARN, fmt); }
void SDL_LogError(int category, const char *fmt, ...) { (void)category; LOG_WITH(SDL_LOG_PRIORITY_ERROR, fmt); }

int SDL_ShowSimpleMessageBox(Uint32 flags, const char *title, const char *message, SDL_Window *window)
{
    (void)flags; (void)window;
    tyrtrace_log("tyrian: %s: %s", title, message);   /* the fatal message has already been logged; this is its box */
    return 0;
}

/* ---- time ---- */

Uint32 SDL_GetTicks(void)
{
    return (Uint32)trace_time_ms();
}

/*
 * Every wait in the game ends up here, so this is where sound is kept flowing and saves go up to the BBS. Long
 * waits are cut into short sleeps so neither runs dry while the game sits on a menu.
 */
void SDL_Delay(Uint32 ms)
{
    Uint32 end = SDL_GetTicks() + ms;

    for (;;)
    {
        Sint32 left;
        struct timespec ts;

        tyrtrace_pump();
        left = (Sint32)(end - SDL_GetTicks());
        if (left <= 0)
            break;
        if (left > 5)
            left = 5;
        ts.tv_sec = 0;
        ts.tv_nsec = (long)left * 1000000L;
        nanosleep(&ts, NULL);
    }
}

/* ---- surfaces ---- */

static SDL_PixelFormat g_format8 = { 0, 8, 1 };
static SDL_PixelFormat g_format32 = { SDL_PIXELFORMAT_RGB888, 32, 4 };

SDL_Surface *SDL_CreateRGBSurface(Uint32 flags, int width, int height, int depth,
                                  Uint32 rmask, Uint32 gmask, Uint32 bmask, Uint32 amask)
{
    SDL_Surface *s;
    (void)flags; (void)rmask; (void)gmask; (void)bmask; (void)amask;

    if (depth != 8 && depth != 32)
        return NULL;
    s = (SDL_Surface *)calloc(1, sizeof(*s));
    if (s == NULL)
        return NULL;
    s->format = depth == 8 ? &g_format8 : &g_format32;
    s->w = width;
    s->h = height;
    s->pitch = width * s->format->BytesPerPixel;
    s->pixels = calloc((size_t)s->pitch, (size_t)height);
    if (s->pixels == NULL)
    {
        free(s);
        return NULL;
    }
    return s;
}

void SDL_FreeSurface(SDL_Surface *surface)
{
    if (surface != NULL)
    {
        free(surface->pixels);
        free(surface);
    }
}

/* Clip a rectangle to a surface; false when nothing is left */
static int clip(const SDL_Surface *s, SDL_Rect *r)
{
    int x2 = r->x + r->w, y2 = r->y + r->h;
    if (r->x < 0) r->x = 0;
    if (r->y < 0) r->y = 0;
    if (x2 > s->w) x2 = s->w;
    if (y2 > s->h) y2 = s->h;
    r->w = x2 - r->x;
    r->h = y2 - r->y;
    return r->w > 0 && r->h > 0;
}

int SDL_FillRect(SDL_Surface *dst, const SDL_Rect *rect, Uint32 color)
{
    SDL_Rect r = rect != NULL ? *rect : (SDL_Rect){ 0, 0, dst->w, dst->h };

    if (!clip(dst, &r))
        return 0;
    for (int y = r.y; y < r.y + r.h; y++)
    {
        Uint8 *row = (Uint8 *)dst->pixels + (size_t)y * dst->pitch;
        if (dst->format->BytesPerPixel == 1)
            memset(row + r.x, (int)(color & 0xFF), (size_t)r.w);
        else
            for (int x = r.x; x < r.x + r.w; x++)
                ((Uint32 *)row)[x] = color;
    }
    return 0;
}

/* Only ever used by the game to copy one whole 8-bit screen to another */
int SDL_BlitSurface(SDL_Surface *src, const SDL_Rect *srcrect, SDL_Surface *dst, SDL_Rect *dstrect)
{
    SDL_Rect s = srcrect != NULL ? *srcrect : (SDL_Rect){ 0, 0, src->w, src->h };
    int dx = dstrect != NULL ? dstrect->x : 0, dy = dstrect != NULL ? dstrect->y : 0;
    int bpp = src->format->BytesPerPixel;

    if (bpp != dst->format->BytesPerPixel || !clip(src, &s))
        return -1;
    for (int y = 0; y < s.h; y++)
    {
        int ty = dy + y;
        int tx = dx, w = s.w, sx = s.x;
        if (ty < 0 || ty >= dst->h)
            continue;
        if (tx < 0) { sx -= tx; w += tx; tx = 0; }
        if (tx + w > dst->w) w = dst->w - tx;
        if (w <= 0)
            continue;
        memcpy((Uint8 *)dst->pixels + (size_t)ty * dst->pitch + (size_t)tx * bpp,
               (Uint8 *)src->pixels + (size_t)(s.y + y) * src->pitch + (size_t)sx * bpp, (size_t)w * bpp);
    }
    return 0;
}

Uint32 SDL_MapRGB(const SDL_PixelFormat *format, Uint8 r, Uint8 g, Uint8 b)
{
    (void)format;
    return 0xFF000000u | ((Uint32)r << 16) | ((Uint32)g << 8) | b;
}

/* ---- keyboard ----
 *
 * TERMinator sends physical keys as set-1 scancodes, with a flag for the E0-prefixed ones (the arrow block, right
 * Ctrl/Alt, keypad Enter). They become SDL's scancodes, which is what the game keeps in its key settings.
 */

static const Uint8 g_set1[128] =
{
    [0x01] = SDL_SCANCODE_ESCAPE,
    [0x02] = SDL_SCANCODE_1, [0x03] = SDL_SCANCODE_2, [0x04] = SDL_SCANCODE_3, [0x05] = SDL_SCANCODE_4,
    [0x06] = SDL_SCANCODE_5, [0x07] = SDL_SCANCODE_6, [0x08] = SDL_SCANCODE_7, [0x09] = SDL_SCANCODE_8,
    [0x0A] = SDL_SCANCODE_9, [0x0B] = SDL_SCANCODE_0, [0x0C] = SDL_SCANCODE_MINUS, [0x0D] = SDL_SCANCODE_EQUALS,
    [0x0E] = SDL_SCANCODE_BACKSPACE, [0x0F] = SDL_SCANCODE_TAB,
    [0x10] = SDL_SCANCODE_Q, [0x11] = SDL_SCANCODE_W, [0x12] = SDL_SCANCODE_E, [0x13] = SDL_SCANCODE_R,
    [0x14] = SDL_SCANCODE_T, [0x15] = SDL_SCANCODE_Y, [0x16] = SDL_SCANCODE_U, [0x17] = SDL_SCANCODE_I,
    [0x18] = SDL_SCANCODE_O, [0x19] = SDL_SCANCODE_P, [0x1A] = SDL_SCANCODE_LEFTBRACKET,
    [0x1B] = SDL_SCANCODE_RIGHTBRACKET, [0x1C] = SDL_SCANCODE_RETURN, [0x1D] = SDL_SCANCODE_LCTRL,
    [0x1E] = SDL_SCANCODE_A, [0x1F] = SDL_SCANCODE_S, [0x20] = SDL_SCANCODE_D, [0x21] = SDL_SCANCODE_F,
    [0x22] = SDL_SCANCODE_G, [0x23] = SDL_SCANCODE_H, [0x24] = SDL_SCANCODE_J, [0x25] = SDL_SCANCODE_K,
    [0x26] = SDL_SCANCODE_L, [0x27] = SDL_SCANCODE_SEMICOLON, [0x28] = SDL_SCANCODE_APOSTROPHE,
    [0x29] = SDL_SCANCODE_GRAVE, [0x2A] = SDL_SCANCODE_LSHIFT, [0x2B] = SDL_SCANCODE_BACKSLASH,
    [0x2C] = SDL_SCANCODE_Z, [0x2D] = SDL_SCANCODE_X, [0x2E] = SDL_SCANCODE_C, [0x2F] = SDL_SCANCODE_V,
    [0x30] = SDL_SCANCODE_B, [0x31] = SDL_SCANCODE_N, [0x32] = SDL_SCANCODE_M, [0x33] = SDL_SCANCODE_COMMA,
    [0x34] = SDL_SCANCODE_PERIOD, [0x35] = SDL_SCANCODE_SLASH, [0x36] = SDL_SCANCODE_RSHIFT,
    [0x37] = SDL_SCANCODE_KP_MULTIPLY, [0x38] = SDL_SCANCODE_LALT, [0x39] = SDL_SCANCODE_SPACE,
    [0x3A] = SDL_SCANCODE_CAPSLOCK,
    [0x3B] = SDL_SCANCODE_F1, [0x3C] = SDL_SCANCODE_F2, [0x3D] = SDL_SCANCODE_F3, [0x3E] = SDL_SCANCODE_F4,
    [0x3F] = SDL_SCANCODE_F5, [0x40] = SDL_SCANCODE_F6, [0x41] = SDL_SCANCODE_F7, [0x42] = SDL_SCANCODE_F8,
    [0x43] = SDL_SCANCODE_F9, [0x44] = SDL_SCANCODE_F10, [0x45] = SDL_SCANCODE_NUMLOCKCLEAR,
    [0x46] = SDL_SCANCODE_SCROLLLOCK,
    [0x47] = SDL_SCANCODE_KP_7, [0x48] = SDL_SCANCODE_KP_8, [0x49] = SDL_SCANCODE_KP_9,
    [0x4A] = SDL_SCANCODE_KP_MINUS, [0x4B] = SDL_SCANCODE_KP_4, [0x4C] = SDL_SCANCODE_KP_5,
    [0x4D] = SDL_SCANCODE_KP_6, [0x4E] = SDL_SCANCODE_KP_PLUS, [0x4F] = SDL_SCANCODE_KP_1,
    [0x50] = SDL_SCANCODE_KP_2, [0x51] = SDL_SCANCODE_KP_3, [0x52] = SDL_SCANCODE_KP_0,
    [0x53] = SDL_SCANCODE_KP_PERIOD, [0x56] = SDL_SCANCODE_NONUSBACKSLASH,
    [0x57] = SDL_SCANCODE_F11, [0x58] = SDL_SCANCODE_F12,
};

/* The E0 keys: the same scancodes, but a different key */
static SDL_Scancode extended_key(int set1)
{
    switch (set1)
    {
    case 0x1C: return SDL_SCANCODE_KP_ENTER;
    case 0x1D: return SDL_SCANCODE_RCTRL;
    case 0x35: return SDL_SCANCODE_KP_DIVIDE;
    case 0x37: return SDL_SCANCODE_PRINTSCREEN;
    case 0x38: return SDL_SCANCODE_RALT;
    case 0x47: return SDL_SCANCODE_HOME;
    case 0x48: return SDL_SCANCODE_UP;
    case 0x49: return SDL_SCANCODE_PAGEUP;
    case 0x4B: return SDL_SCANCODE_LEFT;
    case 0x4D: return SDL_SCANCODE_RIGHT;
    case 0x4F: return SDL_SCANCODE_END;
    case 0x50: return SDL_SCANCODE_DOWN;
    case 0x51: return SDL_SCANCODE_PAGEDOWN;
    case 0x52: return SDL_SCANCODE_INSERT;
    case 0x53: return SDL_SCANCODE_DELETE;
    case 0x5B: return SDL_SCANCODE_LGUI;
    case 0x5C: return SDL_SCANCODE_RGUI;
    default:   return SDL_SCANCODE_UNKNOWN;
    }
}

/* The key's character on a US keyboard, which is what SDL's keycodes are for the typing keys */
static SDL_Keycode keycode_for(SDL_Scancode sc)
{
    static const char punct[] = { '\r', 27, 8, '\t', ' ', '-', '=', '[', ']', '\\', '#', ';', '\'', '`', ',', '.', '/' };

    if (sc >= SDL_SCANCODE_A && sc <= SDL_SCANCODE_Z)
        return 'a' + (sc - SDL_SCANCODE_A);
    if (sc >= SDL_SCANCODE_1 && sc <= SDL_SCANCODE_9)
        return '1' + (sc - SDL_SCANCODE_1);
    if (sc == SDL_SCANCODE_0)
        return '0';
    if (sc >= SDL_SCANCODE_RETURN && sc <= SDL_SCANCODE_SLASH)
        return punct[sc - SDL_SCANCODE_RETURN];
    if (sc == SDL_SCANCODE_DELETE)
        return 127;
    return SDL_SCANCODE_TO_KEYCODE(sc);
}

static Uint16 modifier_for(SDL_Scancode sc)
{
    switch (sc)
    {
    case SDL_SCANCODE_LSHIFT: return KMOD_LSHIFT;
    case SDL_SCANCODE_RSHIFT: return KMOD_RSHIFT;
    case SDL_SCANCODE_LCTRL:  return KMOD_LCTRL;
    case SDL_SCANCODE_RCTRL:  return KMOD_RCTRL;
    case SDL_SCANCODE_LALT:   return KMOD_LALT;
    case SDL_SCANCODE_RALT:   return KMOD_RALT;
    case SDL_SCANCODE_LGUI:   return KMOD_LGUI;
    case SDL_SCANCODE_RGUI:   return KMOD_RGUI;
    default:                  return 0;
    }
}

static const char *const g_key_names[SDL_NUM_SCANCODES] =
{
    [SDL_SCANCODE_A] = "A", [SDL_SCANCODE_B] = "B", [SDL_SCANCODE_C] = "C", [SDL_SCANCODE_D] = "D",
    [SDL_SCANCODE_E] = "E", [SDL_SCANCODE_F] = "F", [SDL_SCANCODE_G] = "G", [SDL_SCANCODE_H] = "H",
    [SDL_SCANCODE_I] = "I", [SDL_SCANCODE_J] = "J", [SDL_SCANCODE_K] = "K", [SDL_SCANCODE_L] = "L",
    [SDL_SCANCODE_M] = "M", [SDL_SCANCODE_N] = "N", [SDL_SCANCODE_O] = "O", [SDL_SCANCODE_P] = "P",
    [SDL_SCANCODE_Q] = "Q", [SDL_SCANCODE_R] = "R", [SDL_SCANCODE_S] = "S", [SDL_SCANCODE_T] = "T",
    [SDL_SCANCODE_U] = "U", [SDL_SCANCODE_V] = "V", [SDL_SCANCODE_W] = "W", [SDL_SCANCODE_X] = "X",
    [SDL_SCANCODE_Y] = "Y", [SDL_SCANCODE_Z] = "Z",
    [SDL_SCANCODE_1] = "1", [SDL_SCANCODE_2] = "2", [SDL_SCANCODE_3] = "3", [SDL_SCANCODE_4] = "4",
    [SDL_SCANCODE_5] = "5", [SDL_SCANCODE_6] = "6", [SDL_SCANCODE_7] = "7", [SDL_SCANCODE_8] = "8",
    [SDL_SCANCODE_9] = "9", [SDL_SCANCODE_0] = "0",
    [SDL_SCANCODE_RETURN] = "Return", [SDL_SCANCODE_ESCAPE] = "Escape", [SDL_SCANCODE_BACKSPACE] = "Backspace",
    [SDL_SCANCODE_TAB] = "Tab", [SDL_SCANCODE_SPACE] = "Space", [SDL_SCANCODE_MINUS] = "-",
    [SDL_SCANCODE_EQUALS] = "=", [SDL_SCANCODE_LEFTBRACKET] = "[", [SDL_SCANCODE_RIGHTBRACKET] = "]",
    [SDL_SCANCODE_BACKSLASH] = "\\", [SDL_SCANCODE_NONUSHASH] = "#", [SDL_SCANCODE_SEMICOLON] = ";",
    [SDL_SCANCODE_APOSTROPHE] = "'", [SDL_SCANCODE_GRAVE] = "`", [SDL_SCANCODE_COMMA] = ",",
    [SDL_SCANCODE_PERIOD] = ".", [SDL_SCANCODE_SLASH] = "/", [SDL_SCANCODE_CAPSLOCK] = "CapsLock",
    [SDL_SCANCODE_F1] = "F1", [SDL_SCANCODE_F2] = "F2", [SDL_SCANCODE_F3] = "F3", [SDL_SCANCODE_F4] = "F4",
    [SDL_SCANCODE_F5] = "F5", [SDL_SCANCODE_F6] = "F6", [SDL_SCANCODE_F7] = "F7", [SDL_SCANCODE_F8] = "F8",
    [SDL_SCANCODE_F9] = "F9", [SDL_SCANCODE_F10] = "F10", [SDL_SCANCODE_F11] = "F11", [SDL_SCANCODE_F12] = "F12",
    [SDL_SCANCODE_PRINTSCREEN] = "PrintScreen", [SDL_SCANCODE_SCROLLLOCK] = "ScrollLock",
    [SDL_SCANCODE_PAUSE] = "Pause", [SDL_SCANCODE_INSERT] = "Insert", [SDL_SCANCODE_HOME] = "Home",
    [SDL_SCANCODE_PAGEUP] = "PageUp", [SDL_SCANCODE_DELETE] = "Delete", [SDL_SCANCODE_END] = "End",
    [SDL_SCANCODE_PAGEDOWN] = "PageDown", [SDL_SCANCODE_RIGHT] = "Right", [SDL_SCANCODE_LEFT] = "Left",
    [SDL_SCANCODE_DOWN] = "Down", [SDL_SCANCODE_UP] = "Up", [SDL_SCANCODE_NUMLOCKCLEAR] = "Numlock",
    [SDL_SCANCODE_KP_DIVIDE] = "Keypad /", [SDL_SCANCODE_KP_MULTIPLY] = "Keypad *",
    [SDL_SCANCODE_KP_MINUS] = "Keypad -", [SDL_SCANCODE_KP_PLUS] = "Keypad +",
    [SDL_SCANCODE_KP_ENTER] = "Keypad Enter", [SDL_SCANCODE_KP_1] = "Keypad 1", [SDL_SCANCODE_KP_2] = "Keypad 2",
    [SDL_SCANCODE_KP_3] = "Keypad 3", [SDL_SCANCODE_KP_4] = "Keypad 4", [SDL_SCANCODE_KP_5] = "Keypad 5",
    [SDL_SCANCODE_KP_6] = "Keypad 6", [SDL_SCANCODE_KP_7] = "Keypad 7", [SDL_SCANCODE_KP_8] = "Keypad 8",
    [SDL_SCANCODE_KP_9] = "Keypad 9", [SDL_SCANCODE_KP_0] = "Keypad 0", [SDL_SCANCODE_KP_PERIOD] = "Keypad .",
    [SDL_SCANCODE_NONUSBACKSLASH] = "\\",
    [SDL_SCANCODE_LCTRL] = "Left Ctrl", [SDL_SCANCODE_LSHIFT] = "Left Shift", [SDL_SCANCODE_LALT] = "Left Alt",
    [SDL_SCANCODE_LGUI] = "Left GUI", [SDL_SCANCODE_RCTRL] = "Right Ctrl", [SDL_SCANCODE_RSHIFT] = "Right Shift",
    [SDL_SCANCODE_RALT] = "Right Alt", [SDL_SCANCODE_RGUI] = "Right GUI",
};

const char *SDL_GetScancodeName(SDL_Scancode scancode)
{
    if ((unsigned)scancode < SDL_NUM_SCANCODES && g_key_names[scancode] != NULL)
        return g_key_names[scancode];
    return "";
}

SDL_Scancode SDL_GetScancodeFromName(const char *name)
{
    if (name == NULL || !name[0])
        return SDL_SCANCODE_UNKNOWN;
    for (int i = 0; i < SDL_NUM_SCANCODES; i++)
        if (g_key_names[i] != NULL && strcasecmp(g_key_names[i], name) == 0)
            return (SDL_Scancode)i;
    return SDL_SCANCODE_UNKNOWN;
}

static int g_text_input;
void SDL_StartTextInput(void) { g_text_input = 1; }
void SDL_StopTextInput(void) { g_text_input = 0; }

/* ---- events ---- */

#define PUSHED_MAX 8
static SDL_Event g_pushed[PUSHED_MAX];   /* what the game pushed itself (the joystick code does), read first */
static int       g_pushed_count;
static Uint8     g_down[SDL_NUM_SCANCODES];
static Uint16    g_mod;

int SDL_PushEvent(SDL_Event *event)
{
    if (g_pushed_count == PUSHED_MAX)
        return 0;
    g_pushed[g_pushed_count++] = *event;
    return 1;
}

/* A UTF-16 code unit to UTF-8. Surrogates are dropped: the game only has code page 437 to show anyway. */
static int utf8(unsigned cp, char *out)
{
    if (cp < 0x80) { out[0] = (char)cp; return 1; }
    if (cp < 0x800) { out[0] = (char)(0xC0 | (cp >> 6)); out[1] = (char)(0x80 | (cp & 0x3F)); return 2; }
    if (cp >= 0xD800 && cp < 0xE000) return 0;
    out[0] = (char)(0xE0 | (cp >> 12));
    out[1] = (char)(0x80 | ((cp >> 6) & 0x3F));
    out[2] = (char)(0x80 | (cp & 0x3F));
    return 3;
}

int SDL_PollEvent(SDL_Event *event)
{
    tyrtrace_event_t in;

    /* A real window reports focus when it opens, and the game pauses itself until it hears it. TERMinator only
     * reports changes, and the picture has the keyboard from the moment it opens, so say so once here. */
    static int g_focus_reported;
    if (!g_focus_reported)
    {
        g_focus_reported = 1;
        memset(event, 0, sizeof(*event));
        event->type = SDL_WINDOWEVENT;
        event->window.event = SDL_WINDOWEVENT_FOCUS_GAINED;
        return 1;
    }

    if (g_pushed_count > 0)
    {
        *event = g_pushed[0];
        memmove(g_pushed, g_pushed + 1, sizeof(g_pushed[0]) * (size_t)--g_pushed_count);
        return 1;
    }

    tyrtrace_pump();

    while (tyrtrace_next_event(&in))
    {
        memset(event, 0, sizeof(*event));
        switch (in.type)
        {
        case TE_IN_KEY:
        {
            SDL_Scancode sc = (in.flags & 2) ? extended_key(in.a) : ((unsigned)in.a < 128 ? g_set1[in.a] : 0);
            int pressed = in.flags & 1;
            Uint16 mod = modifier_for(sc);

            if (sc == SDL_SCANCODE_UNKNOWN)
                continue;
            if (pressed)
                g_mod |= mod;
            else
                g_mod &= (Uint16)~mod;

            event->type = pressed ? SDL_KEYDOWN : SDL_KEYUP;
            event->key.state = pressed ? SDL_PRESSED : SDL_RELEASED;
            event->key.repeat = (Uint8)(pressed && g_down[sc]);   /* a held key auto-repeating */
            event->key.keysym.scancode = sc;
            event->key.keysym.sym = keycode_for(sc);
            event->key.keysym.mod = g_mod;
            g_down[sc] = (Uint8)pressed;
            return 1;
        }

        case TE_IN_TEXT:
        {
            int n;
            /* Typed characters only matter while the game asks for text (a save's name); the key itself has
             * already arrived as a key. Control characters come as keys too. */
            if (!g_text_input || in.a < 32 || in.a == 127)
                continue;
            n = utf8((unsigned)in.a, event->text.text);
            if (n == 0)
                continue;
            event->text.text[n] = '\0';
            event->type = SDL_TEXTINPUT;
            return 1;
        }

        case TE_IN_FOCUS:
            event->type = SDL_WINDOWEVENT;
            event->window.event = (in.flags & 1) ? SDL_WINDOWEVENT_FOCUS_GAINED : SDL_WINDOWEVENT_FOCUS_LOST;
            if (!(in.flags & 1))
            {
                /* Keys held while the picture lost focus will never see their release */
                memset(g_down, 0, sizeof(g_down));
                g_mod = 0;
            }
            return 1;

        case TE_IN_QUIT:
            event->type = SDL_QUIT;
            return 1;

        default:
            continue;
        }
    }
    return 0;
}

/* ---- sound ----
 *
 * The game mixes its own music (an emulated OPL chip) and samples in a callback, mono at 44.1 kHz. Here that callback
 * is run on the game's own thread whenever TERMinator's queue has room, and each sample goes out on both channels.
 */

#define MIX_FRAMES 1024

static SDL_AudioCallback g_callback;
static void             *g_userdata;
static int               g_audio_open, g_audio_paused = 1;

SDL_AudioDeviceID SDL_OpenAudioDevice(const char *device, int iscapture, const SDL_AudioSpec *desired,
                                      SDL_AudioSpec *obtained, int allowed_changes)
{
    (void)device; (void)iscapture; (void)allowed_changes;
    if (desired->callback == NULL)
        return 0;
    *obtained = *desired;
    obtained->freq = 44100;            /* TE_AUDIO_RATE */
    obtained->format = AUDIO_S16SYS;
    obtained->channels = 1;
    obtained->samples = MIX_FRAMES;
    obtained->size = MIX_FRAMES * 2;
    g_callback = desired->callback;
    g_userdata = desired->userdata;
    g_audio_open = 1;
    return 1;
}

void SDL_CloseAudioDevice(SDL_AudioDeviceID dev)
{
    (void)dev;
    g_audio_open = 0;
}

void SDL_PauseAudioDevice(SDL_AudioDeviceID dev, int pause_on)
{
    (void)dev;
    g_audio_paused = pause_on;
}

void tyrtrace_pump_audio(void)
{
    static Sint16 mono[MIX_FRAMES];
    static Sint16 stereo[MIX_FRAMES * 2];
    static int busy;

    if (!g_audio_open || g_audio_paused || busy)
        return;
    busy = 1;       /* the game's callback never waits, but be sure it can't find itself here again */
    while (trace_audio_room() >= MIX_FRAMES)
    {
        g_callback(g_userdata, (Uint8 *)mono, (int)sizeof(mono));
        for (int i = 0; i < MIX_FRAMES; i++)
            stereo[2 * i] = stereo[2 * i + 1] = mono[i];
        if (trace_audio_write(stereo, MIX_FRAMES) <= 0)
            break;
    }
    busy = 0;
}

/* The game's samples are 8-bit signed at 11025 Hz; this makes them 16-bit at the output rate, linearly
 * interpolated. It is the only conversion the game asks for. */
int SDL_BuildAudioCVT(SDL_AudioCVT *cvt, SDL_AudioFormat src_format, Uint8 src_channels, int src_rate,
                      SDL_AudioFormat dst_format, Uint8 dst_channels, int dst_rate)
{
    if (src_format != AUDIO_S8 || dst_format != AUDIO_S16SYS || src_channels != 1 || dst_channels != 1 ||
        src_rate <= 0 || dst_rate < src_rate)
        return -1;
    memset(cvt, 0, sizeof(*cvt));
    cvt->needed = 1;
    cvt->src_format = src_format;
    cvt->dst_format = dst_format;
    cvt->src_rate = src_rate;
    cvt->dst_rate = dst_rate;
    cvt->len_mult = 2 * ((dst_rate + src_rate - 1) / src_rate) + 2;
    cvt->len_ratio = 2.0 * dst_rate / src_rate;
    return 1;
}

int SDL_ConvertAudio(SDL_AudioCVT *cvt)
{
    int in = cvt->len;
    int out = (int)((long long)in * cvt->dst_rate / cvt->src_rate);
    Sint8 *src;
    Sint16 *dst = (Sint16 *)cvt->buf;

    if (in <= 0)
    {
        cvt->len_cvt = 0;
        return 0;
    }
    /* Work backwards through the same buffer is awkward with interpolation, so take a copy of the input */
    src = (Sint8 *)malloc((size_t)in);
    if (src == NULL)
        return -1;
    memcpy(src, cvt->buf, (size_t)in);
    for (int i = 0; i < out; i++)
    {
        long long pos = (long long)i * cvt->src_rate * 256 / cvt->dst_rate;   /* 8.8 fixed point */
        int j = (int)(pos >> 8), frac = (int)(pos & 255);
        int a = src[j], b = j + 1 < in ? src[j + 1] : a;
        dst[i] = (Sint16)(((a * (256 - frac) + b * frac) * 256) / 256);
    }
    free(src);
    cvt->len_cvt = out * 2;
    return 0;
}
