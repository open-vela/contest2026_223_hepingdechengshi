#include <stdint.h>
#include "smp.h"

int up_cpu_index(void) {
    return cpu_get_core_id();
}