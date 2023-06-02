#include <gbdk/platform.h>
#include <stdint.h>

#define THREAD_SMART_SWITCHING
#define CRT0_ISR_STACK_USAGE 4

#include "threads.h"

#if defined(GAMEBOY) || defined(ANALOGUEPOCKET) || defined(MEGADUCK)
    #define REGISTER_BLOB_SIZE 4
#elif defined(MASTERSYSTEM) || defined(GAMEGEAR) || defined(MSXDOS)
    #define REGISTER_BLOB_SIZE 6
#endif
#ifndef NORETURN
    #define NORETURN _Noreturn
#endif

#ifdef THREAD_SMART_SWITCHING
uint8_t switch_mutex = 0b11111111;
#endif
main_context_t main_context = {.task_sp = NULL, .next = NULL};  // this is a main task context
context_t * first_context = (context_t *)&main_context;         // start of a context chain
context_t * current_context = (context_t *)&main_context;       // current context pointer

static void __dummy(void) NONBANKED NAKED {
__asm
#if defined(GAMEBOY) || defined(ANALOGUEPOCKET) || defined(MEGADUCK)
_supervisor_ISR::
        push    AF
        push    HL
        push    BC
        push    DE

        add     SP, #-CRT0_ISR_STACK_USAGE
        jr      _supervisor

_switch_to_thread::
        di
        push    AF
        push    HL
        push    BC
        push    DE

#ifdef THREAD_SMART_SWITCHING
        ld      HL, #_switch_mutex
        ld      (HL), #0b11111101       ; release of the slice disables next scheduled switching of context
#endif
        add     SP, #-CRT0_ISR_STACK_USAGE
_supervisor::
#ifdef THREAD_SMART_SWITCHING
        ld      HL, #_switch_mutex      ; check whether we need to switch context this time or not
        sra     (HL)
        jr      nc, 3$                  ; carry is normally set here
#endif
                                        ; context switch is required
        ldhl    SP, #CRT0_ISR_STACK_USAGE       ; CRT0_ISR_STACK_USAGE bytes on the top of a stack are dropped by supervisor
                                                ; to make it compatible with the beginning of an interriupt routine in crt0
        ld      B, H
        ld      C, L                    ; BC = SP + CRT0_ISR_STACK_USAGE

        ld      HL, #_current_context
        ld      A, (HL+)
        ld      H, (HL)
        ld      L, A

        ld      (HL), C
        inc     HL
        ld      (HL), B
        inc     HL                      ; _current_context->task_sp = SP of the task

        ld      A, (HL+)
        ld      H, (HL)
        ld      L, A                    ; HL = context_t(_current_context)->next

        or      H
        jr      NZ, 1$

        ld      HL, #_first_context
        ld      A, (HL+)
        ld      H, (HL)
        ld      L, A                    ; if (!next) HL = _first_context
1$:
        ld      A, L
        ld      (#_current_context), A
        ld      A, H
        ld      (#_current_context + 1), A

        ld      A, (HL+)
        ld      H, (HL)
        ld      L, A

        ld      SP, HL                  ; switch stack and restore context
2$:
        pop     DE
        pop     BC
        pop     HL
#ifdef ENABLE_WAIT_STAT
4$:
        ldh     A, (#_STAT_REG)
        and     #0x02
        jr      NZ, 4$
#endif
        pop     AF
        reti
#ifdef THREAD_SMART_SWITCHING
3$:                                     ; switching is not required, context was switched by user during the previous slice
        add     SP, #CRT0_ISR_STACK_USAGE
        jr      2$
#endif
#elif defined(MASTERSYSTEM) || defined(GAMEGEAR) || defined(MSXDOS)
_supervisor_ISR::
        push    af
        push    bc
        push    de
        push    hl
        push    iy
        push    ix

        push    HL
        jp      _supervisor

_switch_to_thread::
        di
        push    af
        push    bc
        push    de
        push    hl
        push    iy
        push    ix

#ifdef THREAD_SMART_SWITCHING
        ld      hl, #_switch_mutex
        ld      (hl), #0b11111101       ; release of the slice disables next scheduled switching of context
#endif
        push    HL                      ; one word on the top of a stack is dropped by supervisor
                                        ; to make it compatible with the beginning of an interriupt routine in crt
_supervisor::
#ifdef THREAD_SMART_SWITCHING
        ld      HL, #_switch_mutex      ; check whether we need to switch context this time or not
        sra     (HL)
        jp      nc, 3$                  ; carry is normally set here
#endif
                                        ; context switch is required
        ld      HL, #2
        add     HL, SP
        ex      DE, HL

        ld      HL, (_current_context)

        ld      (HL), E
        inc     HL
        ld      (HL), D
        inc     HL                      ; _current_context->task_sp = SP of the task

        ld      A, (HL)
        inc     HL
        ld      H, (HL)
        ld      L, A                    ; HL = context_t(_current_context)->next

        or      H
        jp      NZ, 1$

        ld      HL, (_first_context)    ; if (!next) HL = _first_context
1$:
        ld      (_current_context), HL

        ld      A, (HL)
        inc     HL
        ld      H, (HL)
        ld      L, A

        ld      SP, HL                  ; switch stack and restore context
2$:
        pop     ix
        pop     iy
        pop     hl
        pop     de
        pop     bc
        pop     af
        ei
        reti
#ifdef THREAD_SMART_SWITCHING
3$:                                     ; switching is not required, context was switched by user during the previous slice
        pop     HL
        jp      2$
#endif
#endif
__endasm;
}

NORETURN void __trap_function(context_t * context) OLDCALL {
    context->finished = TRUE;
    while(TRUE) switch_to_thread();        // it is safe to dispose context when the thread execution is here
}

context_t * get_thread_by_id(uint8_t id) {
    context_t * ctx = first_context->next;
    while (ctx) {
        if (ctx->thread_id == id) return ctx;
        ctx = ctx->next;
    }
    return NULL;
}

inline uint8_t generate_thread_id(void) {
    static uint8_t id = 0;
    while (get_thread_by_id(++id));
    return id;
}

void create_thread(context_t * context, uint16_t stack_size, threadproc_t threadproc, void * arg) {
    if ((context) && (threadproc)) {
        context_t * last_context;

        // initialize the new context
        context->next = NULL;
        context->finished = context->terminated = FALSE;
        context->thread_id = generate_thread_id();

        // set stack for the new thread
        uint16_t * stack = context->stack + ((stack_size) ? stack_size >> 1 : CONTEXT_STACK_SIZE_IN_WORDS);
        *--stack = (uint16_t)context;                   // thread context
        *--stack = (uint16_t)arg;                       // threadproc argument
        *--stack = (uint16_t)__trap_function;           // fall there when threadproc exits
        *--stack = (uint16_t)threadproc;                // threadproc entry point
        context->task_sp = stack - REGISTER_BLOB_SIZE;  // space for registers (all registers are undefined on the threadproc entry)

        // get last context in the chain
        for (last_context = first_context; (last_context->next); last_context = last_context->next) ;
        // append new context
        last_context->next = context;
    }
}

void destroy_thread(context_t * context) {
    if (context) {
        context_t * prev_context = first_context;
        while ((prev_context) && (prev_context->next != context)) prev_context = prev_context->next;
        if (prev_context) prev_context->next = context->next;
    }
}

void terminate_thread(context_t * context) {
    if (context) context->terminated = TRUE;
}

void join_thread(context_t * context) {
    if (context) while (!context->finished) switch_to_thread();
}

#if defined(GAMEBOY) || defined(ANALOGUEPOCKET) || defined(MEGADUCK)
uint8_t mutex_try_lock(mutex_t * mutex) NAKED PRESERVES_REGS(b, c) {
    mutex;
__asm
        ld      h, d
        ld      l, e

        xor     a
        sra     (hl)
        rla

        ret
__endasm;
}
void mutex_lock(mutex_t * mutex) NAKED PRESERVES_REGS(b, c) {
    mutex;
__asm
        ld      h, d
        ld      l, e
2$:
        sra     (hl)
        jr      nc, 1$
        call    _switch_to_thread    ; preserves everything
        jr      2$
1$:
        ret
__endasm;
}
void mutex_unlock(mutex_t * mutex) NAKED PRESERVES_REGS(b, c) {
    mutex;
__asm
        ld      h, d
        ld      l, e
        res     0, (hl)
        ret
__endasm;
}
#elif defined(MASTERSYSTEM) || defined(GAMEGEAR) || defined(MSXDOS)
uint8_t mutex_try_lock(mutex_t * mutex) NAKED PRESERVES_REGS(b, c, d, e, iyh, iyl) {
    mutex;
__asm
        xor     a
        sra     (hl)
        rla

        ret
__endasm;
}
void mutex_lock(mutex_t * mutex) NAKED PRESERVES_REGS(b, c, d, e, iyh, iyl) {
    mutex;
__asm
2$:
        sra     (hl)
        jr      nc, 1$
        call    _switch_to_thread    ; preserves everything
        jr      2$
1$:
        ret
__endasm;
}
void mutex_unlock(mutex_t * mutex) NAKED PRESERVES_REGS(b, c, d, e, iyh, iyl) {
    mutex;
__asm
        res     0, (hl)
        ret
__endasm;
}
#endif