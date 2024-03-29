#include <gbdk/platform.h>

#include <stdint.h>

#include "ball.h"

#include "threads.h"
#include "ring.h"
#include "utils.h"

#include "globals.h"

#define  MIN_BALL_X 1
#define  MAX_BALL_X (((DEVICE_SCREEN_WIDTH - 1) << 3) - 1)
#define  MIN_BALL_Y 1
#define  MAX_BALL_Y (((DEVICE_SCREEN_HEIGHT - 1) << 3) - 1)

void ball_init_coords(ball_object_t * ball) {
    ball->x = 1; ball->dx = 1;
    ball->y = 1; ball->dy = -1;
    ball->state = BALL_STUCK;
}

void ball_threadproc(void * arg, void * ctx) OLDCALL {
    ring_t * queue = (ring_t *)(((context_t *)ctx)->queue);
    ball_object_t * ball = (ball_object_t *) arg;
    uint16_t last_tick = gettickcount(), now;
    uint16_t msg;
    uint8_t bat_x = 0, bat_y = 0;
    while(!is_thread_terminated(ctx)) {
        while (ring_get(queue, &msg)) {
            if ((msg & QUEUE_COMMAND) == QUEUE_COMMAND) {
                switch ((uint8_t)msg) {
                    case UNSTUCK_BALL:
                        ball->state = BALL_FLY;
                        break;
                }
            } else SPLIT_WORD(msg, bat_y, bat_x);
        }
        now = gettickcount();
        if (now != last_tick) {
            switch (ball->state) {
                case BALL_STUCK:
                    ball->x = bat_x + 8;
                    ball->y = bat_y - 8;
                    move_sprite(ball->idx, ball->x + DEVICE_SPRITE_PX_OFFSET_X, ball->y + DEVICE_SPRITE_PX_OFFSET_Y);
                    break;
                case BALL_FLY:
                    ball->x += ball->dx;
                    if ((ball->x <= MIN_BALL_X) || (ball->x >= MAX_BALL_X)) ball->dx = -ball->dx;

                    ball->y += ball->dy;
                    if (ball->dy > 0) {
                        if (ball->y >= MAX_BALL_Y) CRITICAL {
                            ring_put(&feedback_ring, MAKE_WORD(KILL_BALL, ((context_t *)ctx)->thread_id));
                        } else {
                            if ((ball->x > (bat_x - 6)) && (ball->x < (bat_x + ((3 << 3) + 2))) &&
                                (ball->y > (bat_y - 8)) && (ball->y < (bat_y - 6))) {
                                ball->dy = -ball->dy;
                            }
                        }
                    } else {
                        if (ball->y <= MIN_BALL_Y) ball->dy = -ball->dy;
                    }

                    move_sprite(ball->idx, ball->x + DEVICE_SPRITE_PX_OFFSET_X, ball->y + DEVICE_SPRITE_PX_OFFSET_Y);
                    break;
            }
            last_tick = now;
        }
        switch_to_thread();
    }
    hide_sprite(ball->idx);
}