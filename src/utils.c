#include "utils.h"

uint16_t gettickcount(void) CRITICAL { return sys_time; }
