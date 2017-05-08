/* Automatically created by xbm2nokia.sh */
#ifndef _NOKIA_GFX_H_
#define _NOKIA_GFX_H_

#define NOKIA_GFX_ANIMATION

struct nokia_gfx_diff {
    uint16_t addr;
    uint8_t data;
};

struct nokia_gfx_frame {
    uint16_t delay;
    uint16_t diffcnt;
    struct nokia_gfx_diff diffs[];
};

#define NOKIA_GFX_FRAME_COUNT 9

extern const uint8_t nokia_gfx_keyframe[];
extern const struct nokia_gfx_frame nokia_gfx_trans_x1_x2;
extern const struct nokia_gfx_frame nokia_gfx_trans_x2_x3;
extern const struct nokia_gfx_frame nokia_gfx_trans_x3_x4;
extern const struct nokia_gfx_frame nokia_gfx_trans_x4_x5;
extern const struct nokia_gfx_frame nokia_gfx_trans_x5_x6;
extern const struct nokia_gfx_frame nokia_gfx_trans_x6_x7;
extern const struct nokia_gfx_frame nokia_gfx_trans_x7_x8;
extern const struct nokia_gfx_frame nokia_gfx_trans_x8_x9;
extern const struct nokia_gfx_frame nokia_gfx_trans_x9_x1;

#endif
