#include <gb/gb.h>
#include <gb/font.h>
#include <stdio.h>

#include "threads.h"
#include "ring.h"
#include "sprite_utils.h"

#include "utils.h"

#include "globals.h"
#include "ball.h"
#include "tile_data.h"

#define BAT_MIN_X 0
#define BAT_MAX_X ((20 * 8) - (3 * 8))

#define MAX_THREADS 10
#define MAX_BALLS 10

void broadcast_message(UWORD msg) {
    context_t * ctx = first_context->next;
    while (ctx) {
        if (ctx->queue) 
            ring_put((ring_t *)ctx->queue, msg);
        ctx = ctx->next;
    }    
}

void terminate_and_destroy_thread(context_t * thread) {
    if (thread) {
        terminate_thread(thread);
        join_thread(thread);
        __critical {
            destroy_thread(thread);
        }
    }
}

context_t thread_contexts[MAX_THREADS];
context_t * free_contexts = 0;

ball_t ball_objects[MAX_BALLS];
ball_t * free_balls = 0;

void init_ball_coords(ball_object_t * ball) {
    ball->x = 1; ball->dx = 1; 
    ball->y = 1; ball->dy = 0; 
    ball->speed = 0;
    ball->state = BALL_STUCK;
}

void init_ball(ball_object_t * ball, UBYTE no) {
    set_sprite_tile(3 + no, 3);
    ball->idx = 3 + no;
    init_ball_coords(ball);
}

void execute_ball_thread() {
    if ((free_contexts) && (free_balls)) {
        ball_t * ball = free_balls;
        free_balls = ball->next;
        
        init_ball_coords(&ball->object);
        
        context_t * ctx = free_contexts;
        free_contexts = ctx->next;

        ring_init(&ball->ring, 0);
        ctx->queue = (void *)(ball->ring);
        ctx->userdata = (void *)(ball);

        __critical { 
            create_thread(ctx, 0, &ball_threadproc, (void *)&ball->object);
        }
    }
}

const sprite_offset_t const bat_offsets[3] = {{0x10, 0x08}, {0x10, 0x10}, {0x10, 0x18}};
const unsigned char const bat_tile_map[3] = {0, 1, 2};

UWORD last_tick, now;
UBYTE joy, j_b_dn;
UBYTE old_pad_x = 1, pad_x = 0, old_pad_y, pad_y = (17 * 8);
UWORD msg;
UBYTE msg_h, msg_l;
void main () {
    font_init();
    font_set(font_load(font_spect));

    set_sprite_data(0, 3, bat_tiles); 
    multiple_set_sprite_tiles(0, 3, bat_tile_map);          // bat
    
    set_sprite_data(3, 1, ball); 
    
    ring_init(&feedback_ring, 0);                           // initialize a feedback ring
  
    // create pool of threads
    for (UBYTE i = 0; i < MAX_THREADS; i++) {
        thread_contexts[i].next = free_contexts;
        free_contexts = &thread_contexts[i];
    }
  
    // create pool of balls
    for (UBYTE i = 0; i < MAX_BALLS; i++) {
        ball_objects[i].next = free_balls;
        init_ball(&ball_objects[i].object, i);
        free_balls = &ball_objects[i];
    }    
      
    execute_ball_thread();
    
    add_TIM(&supervisor);    
    TMA_REG = 0xE0U; TAC_REG = 0x04U;
    set_interrupts(VBL_IFLAG | TIM_IFLAG);
    
    SHOW_SPRITES;
        
    broadcast_message(MAKE_WORD(pad_y, pad_x));             // broadcast position of a bat    
        
    while (1) {
        now = gettickcount();
        if (now != last_tick) {                             // at least 1 VBL
            joy = joypad();
            if (joy & J_LEFT) {
                if (pad_x) pad_x--;                
            } else if (joy & J_RIGHT) {
                if (pad_x < BAT_MAX_X) pad_x++;
            } else if (joy & J_A) {
                broadcast_message(QUEUE_COMMAND | UNSTUCK_BALL);
            } else if ((joy & J_B) && (!j_b_dn)) {
                execute_ball_thread();
                j_b_dn = 1;
            }
            if (!(joy & J_B)) j_b_dn = 0;                     // B button UP

            if ((old_pad_x != pad_x) || (old_pad_y != pad_y)) {
                multiple_move_sprites(0, 3, pad_x, pad_y, &bat_offsets[0]);
                old_pad_x = pad_x;
                broadcast_message(MAKE_WORD(pad_y, pad_x));
            }
            last_tick = now;
        }
        if (ring_get(&feedback_ring, &msg)) {
            SPLIT_WORD(msg, msg_h, msg_l);
            switch (msg_h) {
                case KILL_BALL: {
                    context_t * ctx = get_thread_by_id(msg_l);
                    if (ctx) {
                        terminate_and_destroy_thread(ctx);
                        ctx->next = free_contexts;
                        free_contexts = ctx;
                        ball_t * ball = (ball_t *)(ctx->userdata);
                        if (ball) {
                            ball->next = free_balls;
                            free_balls = ball;
                        }
                    }
                    break;
                }
            }
        }
    }
}