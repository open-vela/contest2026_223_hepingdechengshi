#ifndef BEKEN_PORT_H
#define BEKEN_PORT_H

#ifdef __cplusplus
extern "C" {
#endif

/* newlib-compatible pseudo-random number generator.
 *
 * Unlike NuttX's rand()/srand() (which store the seed in task_info_s and are
 * therefore per-task and re-entrant), these use a single process-global seed
 * shared by every task and CPU.  This mirrors newlib's rand(), which is not
 * required to be thread-safe.
 */
void native_srand(unsigned int seed);
int  native_rand(void);

#ifdef __cplusplus
}
#endif

#endif /* BEKEN_PORT_H */
