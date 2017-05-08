/*
 * xbm2nokia raw data creator
 *
 * Copyright (C) 2017 Sven Gregori <sven@craplab.fi>
 * Released under MIT License
 *
 * General functionality to convert XBM graphic files into raw data used
 * directly by Nokia 3310/5110 LCD. Due to the nature of XBM itself, the
 * XBM file is included as regular C file in the header files generated
 * by the xbm2nokia.sh script.
 *
 * Depending on which template file was used to generate the header file,
 * this file will either create a standalone graphic char array, the
 * keyframe char array for an animation, or a frame transistion struct
 * for an animation. Common to every operation is to take the XBM file
 * data and rotate it 90 degree counter clockwise, flip it vertically,
 * and transform rows and columns to match the LCD's controller's memory
 * map easily. This way the resulting data can be simply memcpy'd into
 * the LCD's memory buffer without any further transformation required.
 *
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "xbm2nokia.h"

#define DIFF_REALLOC 16

struct diff {
    uint16_t addr;
    uint8_t data;
};

struct frame {
    uint16_t diffcnt;
    struct diff *diffs;
};


/**
 * Return the number of bytes required to store a given number.
 * This is basically performing a special case ceil() operation.
 *
 * @param value number to get byte count for
 * @return number of bytes required to store the given number
 */
int
bytes(int value)
{
    /*
     * Yes, there are lots of more leet ways to do this (shift, modulo,
     * bitwise ANDing, using ternary operator), but gcc will either way
     * transform it in identical code, so I went for readability instead.
     */
    int bytes = (int) (value / 8.0);

    if (bytes * 8 == value) {
        return bytes;
    }

    return bytes + 1;
}

/**
 * Allocate memory for diff structs inside a give frame.
 *
 * Whenever needed, a new chunk of memory for DIFF_REALLOC diff struct
 * elements is allocated for the given frame struct.
 *
 * @param frame frame struct to allocate diff struct memory for
 */
void
frame_alloc(struct frame *frame)
{
    if (frame->diffcnt % DIFF_REALLOC == 0) {
        frame->diffs = (struct diff *) realloc(
                frame->diffs,
                (frame->diffcnt + DIFF_REALLOC) * sizeof(struct diff));
    }
}

/**
 * Rotate and flip input buffer into given output buffer.
 *
 * @param data_in input buffer
 * @param out output buffer with rotated and flipped data
 */
void
rotate_flip(uint8_t *data_in, uint8_t *out) {
    int width_off  = bytes(xbm2nokia_frame_width);
    int height_off = bytes(xbm2nokia_frame_height);

    int width, height;
    int width_byte, width_bit;
    int height_byte, height_bit;

    int data_byte;
    int data_value;

    uint8_t out_byte;
    for (width = 0; width < xbm2nokia_frame_width; width++) {
        out_byte = 0;
        for (height = 0; height < xbm2nokia_frame_height; height++) {
            width_byte = width / 8; // byte offset inside width
            width_bit  = width - (width_byte * 8); // bit offset inside byte offsetted byte ..eh

            data_byte  = data_in[height * width_off + width_byte]; // current byte data (width)
            data_value = (data_byte >> width_bit) & 0x01; // width data bit value

            height_byte = height / 8; // byte offset in height
            height_bit  = height - (height_byte * 8); // bit offset in byte offset

            out_byte |= data_value << height_bit;

            if (height_bit == 7 && height < xbm2nokia_frame_height - 1) {
                out[width * height_off + height_byte] = out_byte;
                out_byte = 0;
            }
        }

        out[width * height_off + height_byte] = out_byte;
    }
}

/**
 * Arrange buffer for LCD memory by transforming data in the way of:
 *
 *  x0_0, x0_1, x0_2, ..., x0_m      x0_0, x1_0, x2_0, ..., xn_0
 *  x1_0, x1_1, x1_2, ..., x1_m      x0_1, x1_1, x2_1, ..., xn_1
 *  x2_0, x2_1, x2_2, ..., x2_m  ->  x0_2, x1_2, x3_1, ..., xn_2
 *  ...                              ...
 *  xn_0, xn_1, xn_2, ..., xn_m      x0_m, x1_m, x3_m, ..., xn_m
 *
 * with m being the frame width, and n being bytes(frame height).
 * There is probably a more elegant way to do the transformation..
 *
 * @param in input buffer (rotated and flipped image)
 * @param out output buffer with re-arranged LCD memory data
 * @param buflen output buffer size
 */
void
arrange_mem(uint8_t *in, uint8_t *out, size_t buflen)
{
    size_t idx;
    int row;
    int rotate_row = 0;
    int out_index = 0;
    const int row_count = bytes(xbm2nokia_frame_height);

    for (row = 0; row < row_count; row++) {
        for (idx = 0; idx < buflen; idx++) {
            if (rotate_row == row) {
                out[out_index++] = in[idx];
            }

            if (++rotate_row == row_count) {
                rotate_row = 0;
            }
        }
    }
}

/**
 * Print out keyframe C code.
 * Code for .c file is written to stdout, header file code to stderr.
 *
 *
 * @param buffer keyframe data
 * @param buflen size of keyframe data
 */
void
print_keyframe(uint8_t *buffer, size_t buflen)
{
    size_t i;

    fprintf(stderr, "extern const uint8_t %s[];\n", framename);
    fprintf(stdout, "const uint8_t %s[] PROGMEM = {", framename);
    for (i = 0; i < buflen; i++) {
        if (i % 8 == 0) {
            fprintf(stdout, "\n    ");
        }
        fprintf(stdout, "0x%02x, ", buffer[i]);
    }
    fprintf(stdout, "\n};\n\n");
}

/**
 * Print out frame transistion C code.
 * Code for .c file is written to stdout, header file code to stderr.
 *
 * @param frame frame transistion struct
 */
void
print_frame_transistion(struct frame *frame)
{
    int i;
    struct diff *diff;

    fprintf(stderr, "extern const struct nokia_gfx_frame %s;\n", framename);
    fprintf(stdout, "const struct nokia_gfx_frame %s PROGMEM = {\n", framename);
    fprintf(stdout, "    .diffcnt = %d,\n", frame->diffcnt);
    fprintf(stdout, "    .diffs = {");
    for (i = 0; i < frame->diffcnt; i++) {
        if (i % 4 == 0) {
            fprintf(stdout, "\n        ");
        }
        diff = &frame->diffs[i];
        fprintf(stdout, "{%3d, 0x%02x}, ", diff->addr, diff->data);
    }
    fprintf(stdout, "\n    }\n};\n");
}

/**
 * Create keyframe char array from xbm image.
 */
void
process_keyframe(void)
{
    size_t buflen = bytes(xbm2nokia_frame_height) * xbm2nokia_frame_width;
    uint8_t *rotbuf = calloc(1, buflen);
    uint8_t *outbuf = calloc(1, buflen);

    // rotate and flip
    rotate_flip(xbm2nokia_frame1_data, rotbuf);
    // re-arrange for LCD memory map
    arrange_mem(rotbuf, outbuf, buflen);
    // write out data
    print_keyframe(outbuf, buflen);

    free(outbuf);
    free(rotbuf);
}

/**
 * Create frame transition struct from xbm image
 */
void
process_frame_transistion(void)
{
    size_t i;
    size_t buflen = bytes(xbm2nokia_frame_height) * xbm2nokia_frame_width;
    uint8_t *rotbuf1 = calloc(1, buflen);
    uint8_t *rotbuf2 = calloc(1, buflen);
    uint8_t *outbuf1 = calloc(1, buflen);
    uint8_t *outbuf2 = calloc(1, buflen);
    struct frame frame;
    struct diff *diff;

    memset(&frame, 0, sizeof(frame));

    // rotate and flip
    rotate_flip(xbm2nokia_frame1_data, rotbuf1);
    rotate_flip(xbm2nokia_frame2_data, rotbuf2);
    // re-arrange for LCD memory map
    arrange_mem(rotbuf1, outbuf1, buflen);
    arrange_mem(rotbuf2, outbuf2, buflen);

    // create frame transistion diff
    for (i = 0; i < buflen; i++) {
        if (outbuf1[i] != outbuf2[i]) {
            frame_alloc(&frame);
            diff = (struct diff *) &frame.diffs[frame.diffcnt++];
            diff->addr = i;
            diff->data = outbuf2[i];
        }
    }

    // write out frame transistion diff
    print_frame_transistion(&frame);

    free(outbuf1);
    free(outbuf2);
    free(rotbuf1);
    free(rotbuf2);
}

int
main(void)
{
#ifdef XBM2NOKIA_FRAME
    process_keyframe();
#else
    process_frame_transistion();
#endif

    return 0;
}

