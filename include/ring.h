#ifndef __RING_H_INCLUDE
#define __RING_H_INCLUDE

#include <gbdk/platform.h>

#define RING_SIZE 128
#define RING_SIZE_IN_WORDS (RING_SIZE >> 1)

typedef struct {                        // ring buffer
    UBYTE size;
    UBYTE head, tail;                   // head and tail of the buffer
    UINT16 ring[RING_SIZE_IN_WORDS];    // ring buffer
} ring_t;

extern void ring_init(ring_t * ring, int size) ;
extern UBYTE ring_put(ring_t * ring, UWORD data);
extern UBYTE ring_get(ring_t * ring, UWORD * res);

#endif
