#ifndef __RING_H_INCLUDE
#define __RING_H_INCLUDE

#include <gbdk/platform.h>
#include <stdint.h>

#define RING_SIZE 128
#define RING_SIZE_IN_WORDS (RING_SIZE >> 1)

typedef struct {                        // ring buffer
    uint8_t size;
    uint8_t head, tail;                 // head and tail of the buffer
    uint16_t ring[RING_SIZE_IN_WORDS];  // ring buffer
} ring_t;

extern void ring_init(ring_t * ring, int size) ;
extern uint8_t ring_put(ring_t * ring, uint16_t data);
extern uint8_t ring_get(ring_t * ring, uint16_t * res);

#endif
