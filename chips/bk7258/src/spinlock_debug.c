/****************************************************************************
 * vendor/beken/chips/bk7258/src/spinlock_debug.c
 *
 * Spinlock debugging support for BK7258 SMP
 ****************************************************************************/

#include <stdint.h>
#include <nuttx/config.h>
#include <nuttx/spinlock.h>
#include <nuttx/irq.h>
#include <nuttx/clock.h>
#include "spinlock_debug.h"

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* Circular buffer to track enter/leave critical section events */
volatile struct spinlock_debug_info g_spinlock_debug[SPINLOCK_DEBUG_SIZE];
volatile uint32_t g_spinlock_debug_idx = 0;

/* Per-CPU state snapshot */
volatile struct spinlock_cpu_state g_spinlock_cpu_state[CONFIG_SMP_NCPUS];

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void spinlock_debug_record_enter(FAR rspinlock_t *lock, uint32_t lr)
{
  uint32_t idx = g_spinlock_debug_idx % SPINLOCK_DEBUG_SIZE;
  int cpu = up_cpu_index();

  g_spinlock_debug[idx].timestamp = clock_systime_ticks();
  g_spinlock_debug[idx].cpu_id = cpu;
  g_spinlock_debug[idx].event_type = SPINLOCK_EVENT_ENTER;
  g_spinlock_debug[idx].lock_addr = (uint32_t)lock;
  g_spinlock_debug[idx].lock_owner = lock->owner;
  g_spinlock_debug[idx].lock_count = lock->count;
  g_spinlock_debug[idx].lr = lr;

  /* Update per-CPU state */
  g_spinlock_cpu_state[cpu].last_enter_time = g_spinlock_debug[idx].timestamp;
  g_spinlock_cpu_state[cpu].last_lock_addr = (uint32_t)lock;
  g_spinlock_cpu_state[cpu].enter_count++;

  g_spinlock_debug_idx++;
}

void spinlock_debug_record_leave(FAR rspinlock_t *lock, uint32_t lr)
{
  uint32_t idx = g_spinlock_debug_idx % SPINLOCK_DEBUG_SIZE;
  int cpu = up_cpu_index();

  g_spinlock_debug[idx].timestamp = clock_systime_ticks();
  g_spinlock_debug[idx].cpu_id = cpu;
  g_spinlock_debug[idx].event_type = SPINLOCK_EVENT_LEAVE;
  g_spinlock_debug[idx].lock_addr = (uint32_t)lock;
  g_spinlock_debug[idx].lock_owner = lock->owner;
  g_spinlock_debug[idx].lock_count = lock->count;
  g_spinlock_debug[idx].lr = lr;

  /* Update per-CPU state */
  g_spinlock_cpu_state[cpu].last_leave_time = g_spinlock_debug[idx].timestamp;
  g_spinlock_cpu_state[cpu].leave_count++;

  g_spinlock_debug_idx++;
}

void spinlock_debug_record_assert(FAR rspinlock_t *lock, uint32_t expected_owner,
                                   uint32_t actual_owner, uint32_t lr)
{
  uint32_t idx = g_spinlock_debug_idx % SPINLOCK_DEBUG_SIZE;
  int cpu = up_cpu_index();

  g_spinlock_debug[idx].timestamp = clock_systime_ticks();
  g_spinlock_debug[idx].cpu_id = cpu;
  g_spinlock_debug[idx].event_type = SPINLOCK_EVENT_ASSERT;
  g_spinlock_debug[idx].lock_addr = (uint32_t)lock;
  g_spinlock_debug[idx].lock_owner = actual_owner;
  g_spinlock_debug[idx].lock_count = lock->count;
  g_spinlock_debug[idx].lr = lr;

  /* Update per-CPU state */
  g_spinlock_cpu_state[cpu].assert_count++;
  g_spinlock_cpu_state[cpu].expected_owner = expected_owner;
  g_spinlock_cpu_state[cpu].actual_owner = actual_owner;

  g_spinlock_debug_idx++;
}

/* Hook function called from modified spinlock.h */
void spinlock_debug_check_before_unlock(FAR rspinlock_t *lock)
{
  int expected_owner = up_cpu_index() + 1;
  int actual_owner = lock->owner;

  /* Record if there's a mismatch BEFORE the assert fires */
  if (expected_owner != actual_owner)
    {
      spinlock_debug_record_assert(lock, expected_owner, actual_owner,
                                    spinlock_get_lr());
    }
}

