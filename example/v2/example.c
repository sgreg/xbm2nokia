/*
 * Nokia 3310/5110 LCD example
 *
 * Copyright (C) 2017 Sven Gregori <sven@craplab.fi>
 * Released under MIT License
 *
 *
 * ATmega48/88/168/328 pinout:
 *
 *   1  /Reset
 *   2  PD0     (unused)
 *   3  PD1     (unused)
 *   4  PD2     (unused)
 *   5  PD3     (unused)
 *   6  PD4     (unused)
 *   7  VCC     -
 *   8  GND     -
 *   9  PB6     (unused)
 *  10  PB7     (unused)
 *  11  PD5     (unused)
 *  12  PD6     (unused)
 *  13  PD7     (unused)
 *  14  PB0     (unused)    (was LCD Reset)
 *
 *  15  PB1     O   LCD D/C
 *  16  PB2     (unused)    (was LCD /CE)
 *  17  PB3     O   LCD MOSI / SerProg MOSI
 *  18  PB4     I   SerProg MISO
 *  19  PB5     O   LCD SCK / SerProg SCK
 *  20  AVCC    -
 *  21  AREF    -
 *  22  GND     -
 *  23  PC0     (unused)
 *  24  PC1     (unused)
 *  25  PC2     (unused)
 *  26  PC3     (unused)
 *  27  PC4     (unused)
 *  28  PC5     (unused)
 *
 */
#include <string.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include "nokia_gfx.h"
#include "nokia_lcd.h"

#ifdef NOKIA_GFX_ANIMATION
struct nokia_gfx_frame const *frames[NOKIA_GFX_FRAME_COUNT];

void
gfx_init(void)
{
    frames[0] = &nokia_gfx_trans_x1_x2;
    frames[1] = &nokia_gfx_trans_x2_x3;
    frames[2] = &nokia_gfx_trans_x3_x4;
    frames[3] = &nokia_gfx_trans_x4_x5;
    frames[4] = &nokia_gfx_trans_x5_x6;
    frames[5] = &nokia_gfx_trans_x6_x7;
    frames[6] = &nokia_gfx_trans_x7_x8;
    frames[7] = &nokia_gfx_trans_x8_x9;
    frames[8] = &nokia_gfx_trans_x9_x1;
}

#else /* NOKIA_GFX_ANIMATION */
const uint8_t *frames[NOKIA_GFX_COUNT];

void
gfx_init(void)
{
    frames[0] = nokia_gfx_x1;
    frames[1] = nokia_gfx_x2;
    frames[2] = nokia_gfx_x3;
    frames[3] = nokia_gfx_x4;
    frames[4] = nokia_gfx_x5;
    frames[5] = nokia_gfx_x6;
    frames[6] = nokia_gfx_x7;
    frames[7] = nokia_gfx_x8;
    frames[8] = nokia_gfx_x9;
}

#endif /* NOKIA_GFX_ANIMATION */

uint8_t frame_cnt;

int
main(void) {

    // set SerProg MISO input, everything else output
    DDRB = (1 << DDB0) | (1 << DDB1) | (1 << DDB2) | (1 << DDB3) | (1 << DDB5);
    /* LCD /CS high, rest low, inputs all w/ pullup */
    PORTB = ~((1 << PB0) | (1 << PB1) | (1 << PB3) | (1 << PB5));

    // set PC5 output to toggle bit during display update for performance tests
    DDRC = (1 << DDC5);
    // set PC5 (and everything else) default low
    PORTC = 0x00;


    spi_init();
    nokia_lcd_init();
    gfx_init();

#ifdef NOKIA_GFX_ANIMATION
    nokia_lcd_fullscreen(nokia_gfx_keyframe);

    while (1) {
        _delay_ms(500);
        PORTC = (1 << PC5);
#ifdef NOKIA_GFX_ANIMATION_FULL_UPDATE
        nokia_lcd_diff_frame(frames[frame_cnt]);
#else
        nokia_lcd_update_diff(frames[frame_cnt]);
#endif /* NOKIA_GFX_ANIMATION_FULL_UPDATE */
        PORTC = 0x00;

        if (++frame_cnt == NOKIA_GFX_FRAME_COUNT) {
            frame_cnt = 0;
        }

    }

#else /* NOKIA_GFX_ANIMATION */

    while (1) {
        nokia_lcd_fullscreen(frames[frame_cnt]);
        if (++frame_cnt == NOKIA_GFX_COUNT) {
            frame_cnt = 0;
        }

        _delay_ms(500);
    }

#endif /* NOKIA_GFX_ANIMATION */
}

