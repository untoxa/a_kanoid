#include <stdint.h>

#include "ring.h"

void ring_init(ring_t * ring, int size) {
    if (ring) {
        if (!size) ring->size = RING_SIZE_IN_WORDS; else ring->size = size >> 1;
        ring->head = ring->tail = 0;
    }
}

uint8_t ring_put(ring_t * ring, uint16_t data) {
    uint8_t h = ring->head;
    if (++h >= ring->size) h = 0;
    if (h == ring->tail) return 0;
    ring->ring[ring->head] = data;
    ring->head = h;
    return 1;
}

uint8_t ring_get(ring_t * ring, uint16_t * res) {
    uint8_t t = ring->tail;
    if (t == ring->head) return 0;
    *res = ring->ring[ring->tail];
    if (++t >= ring->size) t = 0;
    ring->tail = t;
    return 1;
}