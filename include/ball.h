#ifndef __BALL_H_INCLUDE
#define __BALL_H_INCLUDE

#include <gbdk/platform.h>

#include <stdint.h>

#include "ring.h"

enum ball_state_t { BALL_STUCK, BALL_FLY };

typedef struct {
    uint8_t idx;
    uint8_t x, y;
    int8_t dx, dy;
    enum ball_state_t state;
} ball_object_t;

typedef struct ball_t {
    struct  ball_t * next;
    ring_t ring;
    ball_object_t object;
} ball_t;

extern void ball_init_coords(ball_object_t * ball);
extern void ball_threadproc(void * arg, void * ctx) OLDCALL;

#endif