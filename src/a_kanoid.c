#include <gbdk/platform.h>
#include <gbdk/font.h>
#include <gbdk/metasprites.h>

#include <stdint.h>

#include "threads.h"
#include "ring.h"

#include "utils.h"

#include "globals.h"
#include "ball.h"
#include "tile_data.h"

#define BAT_MIN_X 0
#define BAT_MAX_X ((DEVICE_SCREEN_WIDTH * 8) - (3 * 8))

void broadcast_message(uint16_t msg) {
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
        CRITICAL {
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

        CRITICAL {
            create_thread(ctx, DEFAULT_STACK_SIZE, &ball_threadproc, (void *)&ball->object);
            cancel_pending_interrupts();
        }
    }
}

#ifdef MSXDOS
#define SPRITE_ATTR 0x0f
#else
#define SPRITE_ATTR 0x00
#endif
const metasprite_t bat0[] = {
    METASPR_ITEM(DEVICE_SPRITE_PX_OFFSET_Y, DEVICE_SPRITE_PX_OFFSET_X, 0, SPRITE_ATTR), METASPR_ITEM(0, 8, 1, SPRITE_ATTR),  METASPR_ITEM(0, 8, 2, SPRITE_ATTR), METASPR_TERM
};
const metasprite_t * const bat[] = { bat0 };


const unsigned char const bat_tile_map[3] = {0, 1, 2};

uint16_t last_tick = 0, now;
uint8_t joy = 0, old_joy = 0;
uint8_t old_pad_x = 1, pad_x = 0, old_pad_y = 0, pad_y = ((DEVICE_SCREEN_HEIGHT - 1) * 8);
uint16_t msg;
uint8_t msg_h, msg_l;

void main () {
#if defined(MSXDOS)
    _current_1bpp_colors = 0xf0;
    set_bkg_1bpp_data(0, 1, empty_tile);
    set_bkg_1bpp_data(256, 1, empty_tile);
    set_bkg_1bpp_data(512, 1, empty_tile);
#else
    set_bkg_1bpp_data(0, 1, empty_tile);
#endif

    set_sprite_1bpp_data(0, 3, bat_tiles);
    set_sprite_1bpp_data(3, 1, ball);

    fill_bkg_rect(DEVICE_SCREEN_X_OFFSET, DEVICE_SCREEN_Y_OFFSET, DEVICE_SCREEN_WIDTH, DEVICE_SCREEN_HEIGHT, 0x00);

    ring_init(&feedback_ring, 0);                           // initialize a feedback ring

    // create pool of threads
    for (uint8_t i = 0; i < MAX_THREADS; i++) {
        thread_contexts[i].next = free_contexts;
        free_contexts = &thread_contexts[i];
    }

    // create pool of balls
    for (uint8_t i = 0; i < MAX_BALLS; i++) {
        ball_objects[i].next = free_balls;
        set_sprite_tile(i + 3, 3);
        set_sprite_prop(i + 3, SPRITE_ATTR);
        ball_objects[i].object.idx = i + 3;
        ball_init_coords(&ball_objects[i].object);
        free_balls = &ball_objects[i];
    }

    execute_ball_thread();

#if defined(NINTENDO)
    add_TIM(&supervisor);
    TMA_REG = 0xE0U; TAC_REG = 0x04U;
    set_interrupts(VBL_IFLAG | TIM_IFLAG);
#elif defined(SMS) || defined(MSXDOS)
    add_VBL(&supervisor);
    set_interrupts(VBL_IFLAG);
#endif

    SHOW_SPRITES;

    broadcast_message(MAKE_WORD(pad_y, pad_x));             // broadcast position of a bat

    while (TRUE) {
        now = gettickcount();
        if (now != last_tick) {                             // at least 1 VBL
            old_joy = joy, joy = joypad();
            if (joy & J_LEFT) {
                if (pad_x) pad_x--;
            } else if (joy & J_RIGHT) {
                if (pad_x < BAT_MAX_X) pad_x++;
            } else if ((joy & ~old_joy) & J_A) {
                broadcast_message(QUEUE_COMMAND | UNSTUCK_BALL);
            } else if ((joy & ~old_joy) & J_B) {
                execute_ball_thread();
            }

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