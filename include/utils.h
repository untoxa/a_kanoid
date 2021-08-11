#ifndef __UTILS_H_INCLUDE
#define __UTILS_H_INCLUDE

#include <gb/gb.h>

#define abs(a) (((int)(a) & 0x8000)?((int)(!(a))+1):(a)) 

#define MAKE_WORD(h, l) (((h) << 8) | (l))
#define SPLIT_WORD(w, h, l) (h=(UBYTE)((w)>>8),l=(UBYTE)(w))

extern UWORD gettickcount();

#endif
