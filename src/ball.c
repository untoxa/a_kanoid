#include "ball.h"

#include "threads.h"
#include "ring.h"
#include "sprite_utils.h"
#include "utils.h"

#include "globals.h"

#define  MIN_BALL_X 0
#define  MAX_BALL_X ((20 - 1) * 8)
#define  MIN_BALL_Y 0
#define  MAX_BALL_Y ((18 - 1) * 8)

const sprite_offset_t const ball_offset = {0x10, 0x08};

void ball_init_coords(ball_object_t * ball) {
    ball->x = 1; ball->dx = 1; 
    ball->y = 1; ball->dy = 0; 
    ball->speed = 0;
    ball->state = BALL_STUCK;
}

void ball_threadproc(void * arg, void * ctx) {
    ring_t * queue = (ring_t *)(((context_t *)ctx)->queue);
    ball_object_t * ball = (ball_object_t *) arg;
    UWORD last_tick = gettickcount(), now;
    UWORD msg;
    UBYTE bat_x = 0, bat_y = 0;
    while(!((context_t *)ctx)->terminated) {
        if (ring_get(queue, &msg)) {
            if ((msg & QUEUE_COMMAND) == QUEUE_COMMAND) {
                switch ((UBYTE)msg) {
                    case UNSTUCK_BALL:
                        ball->state = BALL_FLY;
                        break;
                }
            } else SPLIT_WORD(msg, bat_y, bat_x);
        }        
        now = gettickcount();
        if (abs(now - last_tick) > ball->speed) {
            switch (ball->state) {
                case BALL_STUCK:
                    ball->x = bat_x + 8;
                    ball->y = bat_y - 8;
                    multiple_move_sprites(ball->idx, 1, ball->x, ball->y, &ball_offset);
                    break;
                case BALL_FLY: 
                    if (ball->dx) { 
                        ball->x++; if (ball->x >= MAX_BALL_X) ball->dx = 0; 
                    } else {
                        if (ball->x) ball->x--;
                        if (ball->x == MIN_BALL_X) ball->dx = 1;             
                    }
                    if (ball->dy) { 
                        if ((ball->y) >= MAX_BALL_Y) __critical {
                            ring_put(&feedback_ring, MAKE_WORD(KILL_BALL, ((context_t *)ctx)->thread_id));
                        } else {
                            ball->y++;
                            if ((ball->x > bat_x - 6) && (ball->x < bat_x + ((3 * 8) + 2)))
                                if ((ball->y > bat_y - 8) && (ball->y < bat_y - 6)) 
                                    ball->dy = 0;
                        }
                    } else { 
                        if (ball->y) ball->y--; 
                        if (ball->y == MIN_BALL_Y) ball->dy = 1;             
                    }                    
                    multiple_move_sprites(ball->idx, 1, ball->x, ball->y, &ball_offset);
                    break;
            }
            last_tick = now;
        }
        switch_to_thread();
    }
    move_sprite(ball->idx, 0, 0);
}