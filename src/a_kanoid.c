#include <gbdk/platform.h>
#include <gbdk/font.h>
#include <gbdk/metasprites.h>

#include "threads.h"
#include "ring.h"

#include "utils.h"

#include "globals.h"
#include "ball.h"
#include "tile_data.h"

#define BAT_MIN_X 0
#define BAT_MAX_X ((DEVICE_SCREEN_WIDTH * 8) - (3 * 8))

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

void execute_ball_thread() {
    if ((free_contexts) && (free_balls)) {
        ball_t * ball = free_balls;
        free_balls = ball->next;
        
        ball_init_coords(&ball->object);
        
        context_t * ctx = free_contexts;
        free_contexts = ctx->next;

        ring_init(&ball->ring, 0);
        ctx->queue = (void *)(ball->ring);
        ctx->userdata = (void *)(ball);

        __critical { 
            create_thread(ctx, DEFAULT_STACK_SIZE, &ball_threadproc, (void *)&ball->object);
        }
    }
}

//const sprite_offset_t const bat_offsets[3] = {{0x10, 0x08}, {0x10, 0x10}, {0x10, 0x18}};
const metasprite_t bat0[] = {
    {DEVICE_SPRITE_OFFSET_Y, DEVICE_SPRITE_OFFSET_X, 0, 0}, {0, 8, 1, 0},  {0, 8, 2, 0}, {metasprite_end}
};
const metasprite_t * const bat[] = { bat0 };


const unsigned char const bat_tile_map[3] = {0, 1, 2};

UWORD last_tick = 0, now;
UBYTE joy, j_a_dn, j_b_dn;
UBYTE old_pad_x = 1, pad_x = 0, old_pad_y = 0, pad_y = ((DEVICE_SCREEN_HEIGHT - 1) * 8);
UWORD msg;
UBYTE msg_h, msg_l;

void main () {
    set_sprite_data(0, 3, bat_tiles); 
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
        set_sprite_tile(i + 3, 3);
        ball_objects[i].object.idx = i + 3;
        ball_init_coords(&ball_objects[i].object);
        free_balls = &ball_objects[i];
    }    
      
    execute_ball_thread();
    
#if defined(__TARGET_gb) || defined(__TARGET_ap)
    add_TIM(&supervisor);    
    TMA_REG = 0xE0U; TAC_REG = 0x04U;
    set_interrupts(VBL_IFLAG | TIM_IFLAG);
#elif defined(__TARGET_sms) || defined(__TARGET_gg)
    add_VBL(&supervisor);
    set_interrupts(VBL_IFLAG); 
#endif

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
                j_a_dn = 1;
            } else if ((joy & J_B) && (!j_b_dn)) {
                execute_ball_thread();
                j_b_dn = 1;
            }
            if (!(joy & J_A)) j_a_dn = 0;                   // A button UP
            if (!(joy & J_B)) j_b_dn = 0;                   // B button UP

            if ((old_pad_x != pad_x) || (old_pad_y != pad_y)) {

                move_metasprite(bat[0], 0, 0, pad_x, pad_y);
                old_pad_x = pad_x;
                broadcast_message(MAKE_WORD(pad_y, pad_x));
            }
            last_tick = now;
        }
        while (ring_get(&feedback_ring, &msg)) {
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
        switch_to_thread(); 
        wait_vbl_done();        // main thread is always last in the list, so we may halt here  
    }
}