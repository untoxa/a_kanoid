#include "ring.h"

void ring_init(ring_t * ring, int size) {
    if (ring) {
        if (!size) ring->size = RING_SIZE_IN_WORDS; else ring->size = size >> 1;
        ring->head = ring->tail = 0;
    }
}

UBYTE ring_put(ring_t * ring, UWORD data) {
    UBYTE h = ring->head;
    h++; if (h >= ring->size) h = 0;
    if (h != ring->tail) {
        ring->ring[ring->head] = data;
        ring->head = h;
        return 1;
    }
    return 0;
}

UBYTE ring_get(ring_t * ring, UWORD * res) {
    UBYTE t = ring->tail;
    if (t != ring->head) {
        *res = ring->ring[ring->tail];
        t++; if (t >= ring->size) t = 0;
        ring->tail = t;
        return 1;
    }
    return 0;
}