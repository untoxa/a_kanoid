#ifndef __BALL_H_INCLUDE
#define __BALL_H_INCLUDE

#include <gb/gb.h>

#include "ring.h"

enum ball_state_t { BALL_STUCK, BALL_FLY };

typedef struct { 
    UBYTE idx; 
    UBYTE x, dx; 
    UBYTE y, dy; 
    UBYTE speed;
    enum ball_state_t state;
} ball_object_t;

typedef struct ball_t {
    struct  ball_t * next;
    ring_t ring;
    ball_object_t object;
} ball_t;

extern void ball_init_coords(ball_object_t * ball);
extern void ball_threadproc(void * arg, void * ctx);

#endif