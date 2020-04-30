#include "utils.h"

UWORD gettickcount() __critical { return sys_time; }
