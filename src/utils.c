#include <gb/gb.h>

UWORD gettickcount() __critical { return sys_time; }
