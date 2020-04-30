#include "globals.h"

ring_t feedback_ring;

context_t thread_contexts[MAX_THREADS];
context_t * free_contexts = 0;

ball_t ball_objects[MAX_BALLS];
ball_t * free_balls = 0;

