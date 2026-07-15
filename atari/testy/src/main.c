#include <atari.h>
#include <string.h>
#include <stdint.h>
#include <peekpoke.h>
#include <conio.h>

#include "pmg.h"

#define PADDL0_ADDR 624
#define PADDL1_ADDR 625
#define PADDL2_ADDR 626
#define PADDL3_ADDR 627
#define CH_ADDR     764

#define MAX_PADDLE  180

#define PM_X_MIN    48
#define PM_X_MAX    208

#define PM_Y_MIN    32
#define PM_Y_MAX    223

static uint8_t *player0;
static uint8_t *player1;


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

static void erase_dot(uint8_t* player, uint8_t y)
{
    player[y - 2] = 0x00;
    player[y - 1] = 0x00;
    player[y]     = 0x00;
    player[y + 1] = 0x00;
    player[y + 2] = 0x00;
}

static void draw_dot(uint8_t* player, uint8_t y)
{
    player[y - 2] = 0x18;
    player[y - 1] = 0x18;
    player[y]     = 0x7E;
    player[y + 1] = 0x18;
    player[y + 2] = 0x18;
}

int main(void)
{
    uint16_t p = (uint16_t)pmgMem;

    uint8_t p0x;
    uint8_t p0y;
    uint8_t x0;
    uint8_t y0;
    uint8_t old_y0;
    uint8_t p1x;
    uint8_t p1y;
    uint8_t x1;
    uint8_t y2;
    uint8_t old_y2;

    uint8_t stick0;
    uint8_t stick1;
    clrscr();

    ANTIC.pmbase = p >> 8;
    memset(pmgMem, 0, 2048);

    OS.sdmctl = 62; // single line PM

    GTIA_WRITE.gractl = 0x03;
    GTIA_WRITE.sizep0 = 0x00;

    OS.color1 = 0x0F; // text
    OS.color2 = 0x98; // bg
    OS.color4 = 0x98;
    OS.gprior = 1;
    OS.pcolr0 = 0x42;
    OS.pcolr1 = 0xC2;

    player0 = pmgMem + 1024;// In single-line PM mode, player 0 starts at PM base + 1024.
    player1 = pmgMem + 1280;// player 1 starts at PM base + 1280.


    POKE(CH_ADDR, 255);

    gotoxy(0,0);
    cprintf("A8 Raider Stick Test");

    while (1) {
      // paddles
        p0x = PEEK(PADDL0_ADDR);
        p0y = PEEK(PADDL1_ADDR);
        p1x = PEEK(PADDL2_ADDR);
        p1y = PEEK(PADDL3_ADDR);

        x0 = map_paddle_to_hpos(p0x);
        y0 = map_paddle_to_y(p0y);
        x1 = map_paddle_to_hpos(p1x);
        y2 = map_paddle_to_y(p1y);


        erase_dot(player0, old_y0);
        draw_dot(player0, y0);
        old_y0 = y0;

        erase_dot(player1, old_y2);
        draw_dot(player1, y2);
        old_y2 = y2;

        GTIA_WRITE.hposp0 = x0;
        GTIA_WRITE.hposp1 = x1;

        gotoxy( 0, 20);
        cprintf( "X1:%3u Y1:%3u", x0, y0);
        gotoxy( 26, 20);
        cprintf( "X2:%3u Y2:%3u", x1, y2);

        // sticks
        stick0 = PEEK(632);
        gotoxy( 9,10 );
        cputc('U'+ ( ~stick0 & 0x01 ?  128 : 0));
        gotoxy( 9,14 );                      
        cputc('D'+ ( ~stick0 & 0x02 ?  128 : 0));
        gotoxy( 7,12 );                      
        cputc('L'+ ( ~stick0 & 0x04 ?  128 : 0));
        gotoxy( 11,12 );                     
        cputc('R'+ ( ~stick0 & 0x08 ?  128 : 0));
        gotoxy( 13,10 );
        cputc('F'+ (PEEK(644) ? 0 : 128));
        // sticks
        stick1 = PEEK(633);
        gotoxy( 29,10 );
        cputc('U'+ ( ~stick1 & 0x01 ?  128 : 0));
        gotoxy( 29,14 );
        cputc('D'+ ( ~stick1 & 0x02 ?  128 : 0));
        gotoxy( 27,12 );
        cputc('L'+ ( ~stick1 & 0x04 ?  128 : 0));
        gotoxy( 31,12 );
        cputc('R'+ ( ~stick1 & 0x08 ?  128 : 0));
        gotoxy( 33,10 );
        cputc('F'+ (PEEK(645) ? 0 : 128));
    }

    // nevah!
    return 0;
}
