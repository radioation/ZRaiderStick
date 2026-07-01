#include <atari.h>
#include <string.h>
#include <stdint.h>
#include <peekpoke.h>
#include <conio.h>

#include "pmg.h"

#define PADDL0_ADDR 624
#define PADDL1_ADDR 625
#define CH_ADDR     764

#define MAX_PADDLE  180

#define PM_X_MIN    48
#define PM_X_MAX    208

#define PM_Y_MIN    32
#define PM_Y_MAX    223

static uint8_t *player0;

static uint8_t old_y;
static uint8_t old_valid;

static uint8_t clamp_paddle(uint8_t v)
{
    return (v > MAX_PADDLE) ? MAX_PADDLE : v;
}

static uint8_t map_paddle_to_hpos(uint8_t p)
{
    p = clamp_paddle(p);

    return (uint8_t)(PM_X_MIN + (((uint16_t)p * (PM_X_MAX - PM_X_MIN)) / MAX_PADDLE));
}

static uint8_t map_paddle_to_y(uint8_t p)
{
      uint16_t v;

    p = clamp_paddle(p);

    v = PM_Y_MAX - (((uint16_t)p * (PM_Y_MAX - PM_Y_MIN)) / MAX_PADDLE);

    return (uint8_t)v;
}

static void erase_dot(uint8_t y)
{
    player0[y - 2] = 0x00;
    player0[y - 1] = 0x00;
    player0[y]     = 0x00;
    player0[y + 1] = 0x00;
    player0[y + 2] = 0x00;
}

static void draw_dot(uint8_t y)
{
    player0[y - 2] = 0x18;
    player0[y - 1] = 0x3C;
    player0[y]     = 0x7E;
    player0[y + 1] = 0x3C;
    player0[y + 2] = 0x18;
}

int main(void)
{
    uint16_t p = (uint16_t)pmgMem;

    uint8_t px;
    uint8_t py;
    uint8_t x;
    uint8_t y;

    clrscr();

    ANTIC.pmbase = p >> 8;
    memset(pmgMem, 0, 2048);

    OS.sdmctl = 62; // single line PM

    GTIA_WRITE.gractl = 0x03;
    GTIA_WRITE.sizep0 = 0x00;

    OS.color2 = 0x98;
    OS.color4 = 0x98;
    OS.gprior = 1;
    OS.pcolr0 = 0x7a;

    player0 = pmgMem + 1024;// In single-line PM mode, player 0 starts at PM base + 1024.

    old_y = 0;
    old_valid = 0;

    POKE(CH_ADDR, 255);

    while (1) {
        px = PEEK(PADDL0_ADDR);
        py = PEEK(PADDL1_ADDR);

        x = map_paddle_to_hpos(px);
        y = map_paddle_to_y(py);

        if (y < 2) {
            y = 2;
        } else if (y > 252) {
            y = 252;
        }

        if (!old_valid || y != old_y) {
            if (old_valid) {
                erase_dot(old_y);
            }

            draw_dot(y);
            old_y = y;
            old_valid = 1;
        }

        GTIA_WRITE.hposp0 = x;
    }

    // nevah!
    return 0;
}
