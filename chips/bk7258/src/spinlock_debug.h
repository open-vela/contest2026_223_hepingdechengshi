/****************************************************************************
 * vendor/beken/chips/bk7258/src/spinlock_debug.h
 *
 * Spinlock debugging support for BK7258 SMP
 ****************************************************************************/

#ifndef __VENDOR_BEKEN_CHIPS_BK7258_SRC_SPINLOCK_DEBUG_H
#define __VENDOR_BEKEN_CHIPS_BK7258_SRC_SPINLOCK_DEBUG_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdint.h>
#include <nuttx/config.h>
#include <nuttx/spinlock.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define SPINLOCK_DEBUG_SIZE 128  /* Circular buffer size */

/* Event types */
#define SPINLOCK_EVENT_ENTER  1
#define SPINLOCK_EVENT_LEAVE  2
#define SPINLOCK_EVENT_ASSERT 3

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* Single spinlock event record */
struct spinlock_debug_info
{
  uint32_t timestamp;      /* Time of event */
  uint32_t cpu_id;         /* CPU that triggered the event */
  uint32_t event_type;     /* ENTER/LEAVE/ASSERT */
  uint32_t lock_addr;      /* Address of the spinlock */
  uint32_t lock_owner;     /* lock->owner at the time */
  uint32_t lock_count;     /* lock->count at the time */
  uint32_t lr;             /* Link register (return address) */
};

/* Per-CPU state summary */
struct spinlock_cpu_state
{
  uint32_t last_enter_time;
  uint32_t last_leave_time;
  uint32_t last_lock_addr;
  uint32_t enter_count;
  uint32_t leave_count;
  uint32_t assert_count;
  uint32_t expected_owner;
  uint32_t actual_owner;
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* Circular buffer of spinlock events (viewable via JTAG) */
extern volatile struct spinlock_debug_info g_spinlock_debug[SPINLOCK_DEBUG_SIZE];
extern volatile uint32_t g_spinlock_debug_idx;

/* Per-CPU state (viewable via JTAG) */
extern volatile struct spinlock_cpu_state g_spinlock_cpu_state[CONFIG_SMP_NCPUS];

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

void spinlock_debug_record_enter(FAR rspinlock_t *lock, uint32_t lr);
void spinlock_debug_record_leave(FAR rspinlock_t *lock, uint32_t lr);
void spinlock_debug_record_assert(FAR rspinlock_t *lock, uint32_t expected_owner,
                                   uint32_t actual_owner, uint32_t lr);

/* Inline helper to get return address */
static inline uint32_t spinlock_get_lr(void)
{
  uint32_t lr;
  __asm__ volatile("mov %0, lr" : "=r"(lr));
  return lr;
}

#endif /* __VENDOR_BEKEN_CHIPS_BK7258_SRC_SPINLOCK_DEBUG_H */
