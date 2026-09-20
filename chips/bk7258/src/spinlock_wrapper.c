/****************************************************************************
 * vendor/beken/chips/bk7258/src/spinlock_wrapper.c
 *
 * Spinlock wrapper for debugging BK7258 SMP issues
 ****************************************************************************/

#include <nuttx/config.h>
#include <nuttx/spinlock.h>
#include <nuttx/irq.h>
#include "spinlock_debug.h"

/* Forward declarations from NuttX internal */
extern rspinlock_t g_schedlock;

/****************************************************************************
 * Name: enter_critical_section_debug
 *
 * Description:
 *   Wrapper around enter_critical_section with debug tracking
 ****************************************************************************/

irqstate_t enter_critical_section_debug(uint32_t lr)
{
  irqstate_t flags;
  int cpu_before = up_cpu_index();

  /* Record state BEFORE entering */
  spinlock_debug_record_enter(&g_schedlock, lr);

  /* Call the real enter_critical_section */
  flags = enter_critical_section();

  /* Verify consistency after acquiring lock */
  int cpu_after = up_cpu_index();
  if (cpu_before != cpu_after)
    {
      /* CPU changed during lock acquisition - this should never happen! */
      spinlock_debug_record_assert(&g_schedlock,
                                    cpu_before + 1,
                                    cpu_after + 1,
                                    lr);
    }

  return flags;
}

/****************************************************************************
 * Name: leave_critical_section_debug
 *
 * Description:
 *   Wrapper around leave_critical_section with debug tracking
 ****************************************************************************/

void leave_critical_section_debug(irqstate_t flags, uint32_t lr)
{
  int expected_owner = up_cpu_index() + 1;
  int actual_owner = g_schedlock.owner;

  /* Record state BEFORE leaving and check for mismatch */
  if (expected_owner != actual_owner)
    {
      /* MISMATCH DETECTED! Record it before the assert fires */
      spinlock_debug_record_assert(&g_schedlock,
                                    expected_owner,
                                    actual_owner,
                                    lr);
    }

  spinlock_debug_record_leave(&g_schedlock, lr);

  /* Call the real leave_critical_section (may assert) */
  leave_critical_section(flags);
}
