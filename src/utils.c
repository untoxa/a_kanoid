#include "utils.h"

uint16_t gettickcount() CRITICAL { return sys_time; }
