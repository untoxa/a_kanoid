#ifndef __UTILS_H_INCLUDE
#define __UTILS_H_INCLUDE

#include <gbdk/platform.h>
#include <stdint.h>

#define abs(a) (((int)(a) & 0x8000)?((int)(!(a))+1):(a))

#define MAKE_WORD(h, l) (((h) << 8) | (l))
#define SPLIT_WORD(w, h, l) (h=(uint8_t)((w)>>8),l=(uint8_t)(w))

#define LIST_PUSH(LIST, ITM) ((ITM)->next=(LIST),(LIST)=(ITM))

extern uint16_t gettickcount();

#endif
