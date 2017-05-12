/*
 * Nokia 3310/5110 LCD helper functions
 *
 * Copyright (C) 2017 Sven Gregori <sven@craplab.fi>
 * Released under MIT License
 *
 */
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include "nokia_lcd.h"

/* everything SPI */
#define SPI_TYPE_COMMAND = 0;
#define SPI_TYPE_DATA    = 1;

#define spi_cs_high()   do { PORTB |=  (1 << PB2); } while (0)
#define spi_cs_low()    do { PORTB &= ~(1 << PB2); } while (0)
#define spi_dc_high()   do { PORTB |=  (1 << PB1); } while (0)
#define spi_dc_low()    do { PORTB &= ~(1 << PB1); } while (0)
#define lcd_rst_high()  do { PORTB |=  (1 << PB0); } while (0)
#define lcd_rst_low()   do { PORTB &= ~(1 << PB0); } while (0)


/**
 * Initialize SPI.
 */
void
spi_init(void)
{
    /* master, SPI mode 0, MSB first */
    SPCR = (1 << SPE) | (1 << MSTR) | (1 << SPR0) | (0 << DORD);
}

/**
 * Send LCD command via SPI.
 *
 * @param command command to send via SPI
 */
static void
spi_send_command(uint8_t command) {
    spi_cs_low();
    spi_dc_low();

    SPDR = command;
    while (!(SPSR & (1 << SPIF))) {
        /* wait */
    }

    spi_cs_high();
}

/**
 * Send LCD data via SPI.
 *
 * @param data data to send via SPI
 */
static void
spi_send_data(uint8_t data) {
    spi_cs_low();
    spi_dc_high();

    SPDR = data;
    while (!(SPSR & (1 << SPIF))) {
        /* wait */
    }

    spi_cs_high();
}

/* everything SPI - the end */


/**
 * Reset Nokia LCD.
 *
 * TLS8204 datashet states a >3us low pulse is requred on the
 * LCD reset line. We'll pulse it for 100us for no real reason,
 * to be on the safe side.
 *
 * Note, if running out of I/O pins, the LCD reset pin can be
 * connected via RC circuit to delay the high level on power up.
 * However, this will require a power cycle to hard reset the
 * LCD when resetting the uC.
 */
void
nokia_lcd_reset(void)
{
    lcd_rst_low();
    _delay_us(100);
    lcd_rst_high();
}

/**
 * Initialize Nokia 3310/5110 LCD.
 *
 * Values are initially taken from Olimex 3310 LCD Arduino example.
 * For more information, check TLS8204 datasheet's command table and
 * register description page 17ff.
 */
void
nokia_lcd_init(void)
{
    spi_send_command(0x21); // function set, H1H0 = 01
    spi_send_command(0xc8); // set EVR, EV[6:0] = 1001000
    // set start line S6 (start line register = S[6:0])
    spi_send_command(0x04 | !!(LCD_START_LINE_ADDR & (1u << 6)));
    // set start line S[5:0]
    spi_send_command(0x40 | (LCD_START_LINE_ADDR & ((1u << 6) -1)));
    spi_send_command(0x12); // system bias set, 1:68
    spi_send_command(0x20); // function set, H1H0 = 00
    spi_send_command(0x08); // display control, display off
    spi_send_command(0x0c); // display control, normal display
}

/**
 * Display fullscreen image on the LCD.
 * Writes given data straight to the LCD via SPI.
 *
 * Note, data is expected to be stored in PROGMEM.
 *
 * @param data full screen PROGMEM data to display
 */
void
nokia_lcd_fullscreen(const uint8_t data[])
{
    uint8_t x;
    uint8_t y;
                   
    for (y = 0; y < LCD_Y_RES / 8; y++) {
        spi_send_command(0x80);     // set X addr to 0x00
        spi_send_command(0x40 | y); // set Y addr to y
        for (x = 0; x < LCD_X_RES; x++) {
            /* read data straight from PROGMEM variable and send it */
            spi_send_data(pgm_read_byte(&(data[y * LCD_X_RES + x])));
        }
    }
}


#ifdef NOKIA_GFX_ANIMATION
#ifdef NOKIA_GFX_ANIMATION_FULL_UPDATE
/**
 * Display animation frame diff.
 *
 * Updates internal LCD memory buffer with animation frame diff
 * and send the full buffer to the LCD via SPI afterwards.
 *
 * @param frame frame transition data
 */
void
nokia_lcd_diff_frame(const struct nokia_gfx_frame *frame)
{
    uint16_t i;
    uint16_t cnt = pgm_read_word(&(frame->diffcnt));
    struct nokia_gfx_diff diff;

    for (i = 0; i < cnt; i++) {
        diff.addr = pgm_read_word(&(frame->diffs[i].addr));
        diff.data = pgm_read_byte(&(frame->diffs[i].data));
        //nokia_lcd_memory[diff.addr] = diff.data;
    }
    //nokia_lcd_update();
}

#else /* NOKIA_GFX_ANIMATION_FULL_UPDATE */

/**
 * Display animation frame diff.
 *
 * Updates internal LCD memory buffer with animation frame diff
 * and send the changes pixel by pixel to the LCD via SPI on the fly.
 *
 * @param frame frame transition data
 */
void
nokia_lcd_update_diff(const struct nokia_gfx_frame *frame)
{
    uint16_t i;
    uint16_t cnt = pgm_read_word(&(frame->diffcnt));
    struct nokia_gfx_diff diff;
    uint8_t x, y;

    for (i = 0; i < cnt; i++) {
        diff.addr = pgm_read_word(&(frame->diffs[i].addr));
        diff.data = pgm_read_byte(&(frame->diffs[i].data));
        //nokia_lcd_memory[diff.addr] = diff.data;

        y = diff.addr / 84;
        x = diff.addr - ((uint16_t) (y * 84));
        spi_send_command(0x80 | x); // set X addr to x
        spi_send_command(0x40 | y); // set Y addr to y
        spi_send_data(diff.data);
    }
}
#endif /* NOKIA_GFX_ANIMATION_FULL_UPDATE */
#endif /* NOKIA_GFX_ANIMATION */

