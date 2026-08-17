#include "beken_port.h"
#include <stdbool.h>
#include "assert.h"
/* Global PRNG state, mirroring newlib's rand()/srand().
 *
 * NuttX's rand()/srand() keep the seed in task_info_s (ta_randint1/2/3), which
 * makes them per-task and re-entrant but also "task-bound".  These native_*
 * functions drop that binding and keep a single global seed instead, matching
 * newlib's behavior.
 *
 * NOTE: the 64-bit read-modify-write below is NOT atomic, so concurrent calls
 * from the two AP cores (SMP) can tear and produce duplicate/overlapping
 * sequences.  This is the same guarantee newlib provides — rand() is not
 * required to be thread-safe (use rand_r() for a re-entrant version).
 */

#define NATIVE_RAND_MULTIPLIER 6364136223846793005ULL
#define NATIVE_RAND_INCREMENT  1ULL

static unsigned long long g_native_seed = 1;

void native_srand(unsigned int seed)
{
  g_native_seed = seed;
}

int native_rand(void)
{
  g_native_seed = g_native_seed * NATIVE_RAND_MULTIPLIER + NATIVE_RAND_INCREMENT;

  /* newlib RAND_MAX is 0x7fffffff on 32-bit-int targets (see sys/config.h),
   * so return the top 31 bits of the 64-bit state. */
  return (int)((g_native_seed >> 33) & 0x7fffffff);
}
void _assert(const char *filename, int linenum, const char *msg, void *regs,
             bool irq) {
  while (1)
    ;
}
void __assert(FAR const char *filename, int linenum, FAR const char *msg) {
  while (1)
    ;
}
