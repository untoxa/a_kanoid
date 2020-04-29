#ifndef __SPRITE_UTILS_H_INCLUDE
#define __SPRITE_UTILS_H_INCLUDE

#include <gb/gb.h>

typedef struct {
    UBYTE y, x;
} sprite_offset_t;

typedef struct {
    UBYTE y, min_y, max_y, x, min_x, max_x;
} sprite_offset_limit_t;


extern void multiple_clear_sprite_tiledata(UBYTE start, UBYTE count);
extern void multiple_set_sprite_prop(UBYTE start, UBYTE count, UBYTE prop);
extern void multiple_set_sprite_tiles(UBYTE start, UBYTE count, const unsigned char * tiles);
extern void multiple_move_sprites(UBYTE start, UBYTE count, UBYTE x, UBYTE y, sprite_offset_t * offsets);
extern void multiple_move_sprites_limits(UBYTE start, UBYTE count, UBYTE x, UBYTE y, sprite_offset_limit_t * offsets, UBYTE dx, UBYTE dy);

#endif