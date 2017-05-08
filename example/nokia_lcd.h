/*
 * Nokia 3310/5110 LCD helper functions
 *
 * Copyright (C) 2017 Sven Gregori <sven@craplab.fi>
 * Released under MIT License
 *
 */
#ifndef _NOKIA_LCD_H_
#define _NOKIA_LCD_H_

#include <string.h>
#include "nokia_gfx.h"

#define LCD_START_LINE_ADDR (66-2)
#define LCD_X_RES 84
#define LCD_Y_RES 48
#define LCD_MEMORY_SIZE     ((LCD_X_RES * LCD_Y_RES) / 8)

/*
 * Two options are implemented for animation LCD update:
 *  1. write diffs into internal LCD memory and write the whole memory
 *     to the LCD, same way the initial keyframe (or any other full
 *     screen graphic) is sent to the LCD
 *  2. write diffs into internal LCD memory and at the same time send
 *     that particlar change to the LCD controller.
 *
 * The second option will add ~160 bytes to the firmware binary due to
 * additionally required x/y address calculations.
 *
 * Initial tests showed that the first option takes around 10ms while
 * the second option (naturally) varies, and values between 1.8ms and
 * 5.4ms could be recorded for diff counts varying between 21 and 62.
 * So while it can have performance improvements, it can be assumed
 * that at values >100 diffs/frame, the full memory write option will
 * actually be more efficient.
 *
 * From image "quality" point of view, no actual differences could be
 * observed between either option, including ghosting and flickering.
 *
 * For now, both options will remain, and defining/undefining the
 * NOKIA_GFX_ANIMATION_FULL_UPDATE flag will toggle between them
 */
#define NOKIA_GFX_ANIMATION_FULL_UPDATE

extern unsigned char nokia_lcd_memory[];

#define nokia_lcd_clear_memory() memset(&nokia_lcd_memory, 0, LCD_MEMORY_SIZE)
void nokia_lcd_reset(void);
void nokia_lcd_init(void);
void nokia_lcd_fullscreen(const uint8_t data[]);

#ifdef NOKIA_GFX_ANIMATION
#ifdef NOKIA_GFX_ANIMATION_FULL_UPDATE
void nokia_lcd_diff_frame(const struct nokia_gfx_frame *frame);
#else
void nokia_lcd_update_diff(const struct nokia_gfx_frame *frame);
#endif /* NOKIA_GFX_ANIMATION_FULL_UPDATE */
#endif /* NOKIA_GFX_ANIMATION */

void spi_init(void);

#endif /* _NOKIA_LCD_H_ */
