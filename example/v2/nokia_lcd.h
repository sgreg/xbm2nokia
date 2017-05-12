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

void nokia_lcd_init(void);
void nokia_lcd_fullscreen(const uint8_t data[]);

#ifdef NOKIA_GFX_ANIMATION
void nokia_lcd_update_diff(const struct nokia_gfx_frame *frame);
#endif /* NOKIA_GFX_ANIMATION */

void spi_init(void);

#endif /* _NOKIA_LCD_H_ */
