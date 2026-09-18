/*
 * video_trace.c -- Tyrian's screen, drawn by TRACE instead of an SDL window. Replaces OpenTyrian's video.c and
 * video_scale*.c.
 *
 * The game draws into 320x200 8-bit surfaces, exactly as it did on a VGA card. Each shown frame is turned into the
 * BGRA one trace_present takes through the game's own palette, and handed to TERMinator with the 4:3 flag, so it
 * comes out with the tall pixels the original had on a CRT. Scaling and full screen are TERMinator's business, so
 * the game's own settings for them are kept (they're in its config file) but do nothing.
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "keyboard.h"
#include "logging.h"
#include "palette.h"
#include "video.h"
#include "video_scale.h"

#include "trace_api.h"
#include "tyrtrace.h"

const char *const scaling_mode_names[ScalingMode_MAX] = {
    "Center",
    "Integer",
    "Fit 8:5",
    "Fit 4:3",
};

int fullscreen_display = -1;
ScalingMode scaling_mode = SCALE_INTEGER;

SDL_Surface *VGAScreen, *VGAScreenSeg;
SDL_Surface *VGAScreen2;
SDL_Surface *game_screen;

SDL_Window *main_window = NULL;
static SDL_PixelFormat g_window_format = { SDL_PIXELFORMAT_RGB888, 32, 4 };
SDL_PixelFormat *main_window_tex_format = &g_window_format;   /* the palette is mapped through this: see SDL_MapRGB */

/* One "scaler": TERMinator does the scaling */
uint scaler = 0;
const struct Scalers scalers[] = {
    { vga_width, vga_height, NULL, NULL, "None" },
};
const uint scalers_count = COUNTOF(scalers);

static uint32_t g_frame[vga_width * vga_height];

void set_scaler_by_name(const char *name)
{
    (void)name;
}

void init_video(void)
{
    VGAScreen = VGAScreenSeg = SDL_CreateRGBSurface(0, vga_width, vga_height, 8, 0, 0, 0, 0);
    VGAScreen2 = SDL_CreateRGBSurface(0, vga_width, vga_height, 8, 0, 0, 0, 0);
    game_screen = SDL_CreateRGBSurface(0, vga_width, vga_height, 8, 0, 0, 0, 0);
    if (VGAScreen == NULL || VGAScreen2 == NULL || game_screen == NULL)
    {
        logFatal("Not enough memory for the screen");
        exit(EXIT_FAILURE);
    }
    JE_clr256(VGAScreen);
}

void deinit_video(void)
{
    SDL_FreeSurface(VGAScreenSeg);
    SDL_FreeSurface(VGAScreen2);
    SDL_FreeSurface(game_screen);
    VGAScreen = VGAScreenSeg = VGAScreen2 = game_screen = NULL;
}

void video_on_win_resize(void) { }
void reinit_fullscreen(int new_display) { fullscreen_display = new_display; }
void toggle_fullscreen(void) { }

bool init_scaler(unsigned int new_scaler)
{
    return new_scaler < scalers_count;
}

bool set_scaling_mode_by_name(const char *name)
{
    for (int i = 0; i < ScalingMode_MAX; ++i)
        if (strcmp(name, scaling_mode_names[i]) == 0)
        {
            scaling_mode = i;
            return true;
        }
    return false;
}

void JE_clr256(SDL_Surface *screen)
{
    SDL_FillRect(screen, NULL, 0);
}

void JE_showVGA(void)
{
    const Uint8 *src = (const Uint8 *)VGAScreen->pixels;

    for (int y = 0; y < vga_height; y++)
    {
        const Uint8 *row = src + (size_t)y * VGAScreen->pitch;
        uint32_t *out = g_frame + y * vga_width;
        for (int x = 0; x < vga_width; x++)
            out[x] = rgb_palette[row[x]];
    }
    trace_present(g_frame, vga_width, vga_height, TRACE_PRESENT_ASPECT_4_3);

    /* Sound and saves are kept going from here too, right after the picture */
    tyrtrace_pump();
}

/* No mouse yet (TERMinator doesn't capture one for modules), so screen and window are the same thing */
void mapScreenPointToWindow(Sint32 *inout_x, Sint32 *inout_y) { (void)inout_x; (void)inout_y; }
void mapWindowPointToScreen(Sint32 *inout_x, Sint32 *inout_y) { (void)inout_x; (void)inout_y; }
void scaleWindowDistanceToScreen(Sint32 *inout_x, Sint32 *inout_y) { (void)inout_x; (void)inout_y; }
