#ifndef __GLOBALS_H_INCLUDE
#define __GLOBALS_H_INCLUDE

#include "ring.h"
#include "threads.h"

#include "ball.h"

#define MAX_THREADS 10
#define MAX_BALLS 10

#define QUEUE_COMMAND 0xFF00

#define KILL_BALL 0
#define UNSTUCK_BALL 1

extern ring_t feedback_ring;

extern context_t thread_contexts[MAX_THREADS];
extern context_t * free_contexts;

extern ball_t ball_objects[MAX_BALLS];
extern ball_t * free_balls;

#endif