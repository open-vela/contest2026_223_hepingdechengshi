#ifndef BEKEN_ARCH_H
#define BEKEN_ARCH_H
#include <stdint.h>

#define BK_NUTTX_SMP_CMD_SCHED 0x10u /* sent by up_send_smp_sched */
#define BK_NUTTX_SMP_CMD_CALL 0x11u  /* sent by up_send_smp_call  */
#define BK_NUTTX_SMP_CMD_TEST 0xF1u  /* sent by up_send_smp_call  */
int bk_cross_core_send(uint32_t cmd);

#endif