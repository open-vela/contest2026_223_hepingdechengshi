#include "os/os.h"
#include "os/mem.h"
#include <nuttx/irq.h>
#include <nuttx/spinlock.h>
#include <nuttx/spinlock_type.h>
#include <nuttx/mutex.h>
#include <nuttx/semaphore.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "cmsis_gcc.h"
#include "components/system.h"
#include <unistd.h>
#include "easy_log.h"
#include <mqueue.h>
#include <fcntl.h>
#include <time.h>
#include <stdlib.h>
static SPINLOCK_SECTION volatile spinlock_t rtos_spin_lock = SP_UNLOCKED;
uint32_t rtos_enter_critical(void) {
    return enter_critical_section();
}

void rtos_exit_critical(uint32_t flags) {
    leave_critical_section(flags);
}
void bk_printf(const char *fmt, ...)
{
    char buf[EASY_LOG_MAX_BUFFER_SIZE];
    va_list args;

    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    buf[sizeof(buf) - 1] = '\0';
    easy_log_output(buf, strlen(buf));
}

static const char g_level_char[] = { 'E', 'W', 'I', 'D' };

void bk_printf_ext(int level, char *tag, const char *fmt, ...)
{
    char buf[EASY_LOG_MAX_BUFFER_SIZE];
    int el = level - 1;   /* BK_LOG_ERROR(1) → 0, BK_LOG_WARN(2) → 1, ... */
    int offset;
    va_list args;

    /* Clamp to valid easy_log level range */
    if (el < 0)
        el = EASY_LOG_LEVEL_INFO;
    else if (el > EASY_LOG_LEVEL_DEBUG)
        el = EASY_LOG_LEVEL_DEBUG;

    /* Format prefix: [cpu_id][L] tag: */
    offset = snprintf(buf, sizeof(buf), "[%d][%c] %s: ",
                      easy_log_get_core_id(),
                      g_level_char[el],
                      tag ? tag : "");

    if (offset < 0 || (size_t)offset >= sizeof(buf))
      {
        buf[sizeof(buf) - 1] = '\0';
        goto output;
      }

    va_start(args, fmt);
    vsnprintf(buf + offset, sizeof(buf) - offset, fmt, args);
    va_end(args);

    buf[sizeof(buf) - 1] = '\0';

    /* Append CRLF, matching easy_log_level_printf output convention */
    size_t len = strlen(buf);
    if (len + 2 < sizeof(buf))
      {
        buf[len++] = '\r';
        buf[len++] = '\n';
      }
    buf[sizeof(buf) - 1] = '\0';

output:
    easy_log_output(buf, strlen(buf));
}
int port_disable_interrupts_flag(void)
{
    uint32_t primask_val = __get_PRIMASK();
    __disable_irq();
    return primask_val;
}

void port_enable_interrupts_flag(int val)
{
    __set_PRIMASK(val);
}

uint32_t rtos_disable_int(void)
{
    return port_disable_interrupts_flag();
}

void rtos_enable_int(uint32_t int_level)
{
    port_enable_interrupts_flag(int_level);
}

void *os_memset(void *b, int c, UINT32 len)
{
	return memset(b, c, len);
}

void *os_memcpy(void *out, const void *in, UINT32 n)
{
	return memcpy(out, in, n);
}

void *os_memmove(void *out, const void *in, UINT32 n)
{
	return memmove(out, in, n);
}

INT32 os_memcmp(const void *s1, const void *s2, UINT32 n)
{
	return memcmp(s1, s2, n);
}

int os_memcmp_const(const void *a, const void *b, size_t len)
{
	return memcmp(a, b, len);
}

void *os_malloc(size_t size)
{
	return malloc(size);
}

void *os_zalloc(size_t size)
{
	void *ptr = malloc(size);

	if (ptr)
		memset(ptr, 0, size);

	return ptr;
}

void *os_realloc(void *ptr, size_t size)
{
	return realloc(ptr, size);
}

void os_free(void *ptr)
{
	free(ptr);
}

bk_err_t rtos_delay_milliseconds( uint32_t num_ms )
{
    // uint32_t ticks;

    // ticks = num_ms / bk_get_ms_per_tick();
	// if (ticks == 0)
    //     ticks = 1;

	// RTOS_ASSERT_INT_ENABLED();

    usleep(num_ms * 100);

    return kNoErr;
}

struct beken_mqueue
{
    mqd_t  mqd;
    char   name[32];
    size_t msgsize;
};

static int g_queue_counter;

bk_err_t rtos_init_queue(beken_queue_t *queue, const char *name,
                          uint32_t message_size, uint32_t number_of_messages)
{
    struct beken_mqueue *mq;
    struct mq_attr attr;
    int flags;

    mq = (struct beken_mqueue *)malloc(sizeof(*mq));
    if (!mq)
        return kGeneralErr;

    if (name && name[0])
        snprintf(mq->name, sizeof(mq->name), "/%s_%d", name,
                 g_queue_counter++);
    else
        snprintf(mq->name, sizeof(mq->name), "/beken_q%d",
                 g_queue_counter++);

    mq->msgsize = message_size;

    attr.mq_maxmsg  = number_of_messages;
    attr.mq_msgsize = message_size;
    attr.mq_flags   = 0;

    flags = O_CREAT | O_RDWR;
    mq->mqd = mq_open(mq->name, flags, 0666, &attr);
    if (mq->mqd < 0)
    {
        free(mq);
        return kGeneralErr;
    }

    *queue = (void *)mq;
    return kNoErr;
}

bk_err_t rtos_push_to_queue(beken_queue_t *queue, void *message,
                             uint32_t timeout_ms)
{
    struct beken_mqueue *mq = (struct beken_mqueue *)(*queue);
    int ret;

    if (timeout_ms == BEKEN_WAIT_FOREVER ||
        timeout_ms == BEKEN_NEVER_TIMEOUT)
    {
        ret = mq_send(mq->mqd, (const char *)message, mq->msgsize, 0);
    }
    else if (timeout_ms == 0)
    {
        struct mq_attr attr;
        if (mq_getattr(mq->mqd, &attr) < 0)
            return kGeneralErr;
        if (attr.mq_curmsgs >= attr.mq_maxmsg)
            return kGeneralErr;
        ret = mq_send(mq->mqd, (const char *)message, mq->msgsize, 0);
    }
    else
    {
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec  += timeout_ms / 1000;
        ts.tv_nsec += (timeout_ms % 1000) * 1000000L;
        if (ts.tv_nsec >= 1000000000L)
        {
            ts.tv_sec++;
            ts.tv_nsec -= 1000000000L;
        }
        ret = mq_timedsend(mq->mqd, (const char *)message, mq->msgsize, 0,
                           &ts);
    }

    return (ret == 0) ? kNoErr : kGeneralErr;
}

bk_err_t rtos_push_to_queue_front(beken_queue_t *queue, void *message,
                                   uint32_t timeout_ms)
{
    struct beken_mqueue *mq = (struct beken_mqueue *)(*queue);
    int ret;

    if (timeout_ms == BEKEN_WAIT_FOREVER ||
        timeout_ms == BEKEN_NEVER_TIMEOUT)
    {
        ret = mq_send(mq->mqd, (const char *)message, mq->msgsize,
                      MQ_PRIO_MAX);
    }
    else if (timeout_ms == 0)
    {
        struct mq_attr attr;
        if (mq_getattr(mq->mqd, &attr) < 0)
            return kGeneralErr;
        if (attr.mq_curmsgs >= attr.mq_maxmsg)
            return kGeneralErr;
        ret = mq_send(mq->mqd, (const char *)message, mq->msgsize,
                      MQ_PRIO_MAX);
    }
    else
    {
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec  += timeout_ms / 1000;
        ts.tv_nsec += (timeout_ms % 1000) * 1000000L;
        if (ts.tv_nsec >= 1000000000L)
        {
            ts.tv_sec++;
            ts.tv_nsec -= 1000000000L;
        }
        ret = mq_timedsend(mq->mqd, (const char *)message, mq->msgsize,
                           MQ_PRIO_MAX, &ts);
    }

    return (ret == 0) ? kNoErr : kGeneralErr;
}

bk_err_t rtos_pop_from_queue(beken_queue_t *queue, void *message,
                              uint32_t timeout_ms)
{
    struct beken_mqueue *mq = (struct beken_mqueue *)(*queue);
    ssize_t ret;

    if (timeout_ms == BEKEN_WAIT_FOREVER ||
        timeout_ms == BEKEN_NEVER_TIMEOUT)
    {
        ret = mq_receive(mq->mqd, (char *)message, mq->msgsize, NULL);
    }
    else if (timeout_ms == 0)
    {
        struct timespec ts = {0, 0};
        ret = mq_timedreceive(mq->mqd, (char *)message, mq->msgsize,
                              NULL, &ts);
    }
    else
    {
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec  += timeout_ms / 1000;
        ts.tv_nsec += (timeout_ms % 1000) * 1000000L;
        if (ts.tv_nsec >= 1000000000L)
        {
            ts.tv_sec++;
            ts.tv_nsec -= 1000000000L;
        }
        ret = mq_timedreceive(mq->mqd, (char *)message, mq->msgsize,
                              NULL, &ts);
    }

    return (ret >= 0) ? kNoErr : kGeneralErr;
}

bk_err_t rtos_deinit_queue(beken_queue_t *queue)
{
    struct beken_mqueue *mq = (struct beken_mqueue *)(*queue);

    mq_close(mq->mqd);
    mq_unlink(mq->name);
    free(mq);
    *queue = NULL;

    return kNoErr;
}

bool rtos_is_queue_empty(beken_queue_t *queue)
{
    struct beken_mqueue *mq = (struct beken_mqueue *)(*queue);
    struct mq_attr attr;

    if (mq_getattr(mq->mqd, &attr) < 0)
        return true;

    return (attr.mq_curmsgs == 0);
}

bool rtos_is_queue_full(beken_queue_t *queue)
{
    struct beken_mqueue *mq = (struct beken_mqueue *)(*queue);
    struct mq_attr attr;

    if (mq_getattr(mq->mqd, &attr) < 0)
        return true;

    return (attr.mq_curmsgs >= attr.mq_maxmsg);
}

bool rtos_reset_queue(beken_queue_t *queue)
{
    struct beken_mqueue *mq = (struct beken_mqueue *)(*queue);
    struct mq_attr attr;
    char dummy;

    while (mq_getattr(mq->mqd, &attr) == 0 && attr.mq_curmsgs > 0)
        mq_receive(mq->mqd, &dummy, 1, NULL);

    return true;
}

bk_err_t rtos_init_semaphore(beken_semaphore_t *semaphore, int max_count)
{
    return rtos_init_semaphore_ex(semaphore, max_count, 0);
}

bk_err_t rtos_init_semaphore_ex(beken_semaphore_t *semaphore, int max_count,
                                 int init_count)
{
    sem_t *sem;

    if (semaphore == NULL)
        return kParamErr;

    sem = (sem_t *)malloc(sizeof(sem_t));
    if (sem == NULL)
        return kGeneralErr;

    if (nxsem_init(sem, 0, (uint32_t)init_count) < 0)
    {
        free(sem);
        return kGeneralErr;
    }

    *semaphore = (void *)sem;
    return kNoErr;
}

bk_err_t rtos_set_semaphore(beken_semaphore_t *semaphore)
{
    sem_t *sem;

    if (semaphore == NULL || *semaphore == NULL)
        return kParamErr;

    sem = (sem_t *)(*semaphore);
    return (nxsem_post(sem) < 0) ? kGeneralErr : kNoErr;
}

bk_err_t rtos_get_semaphore(beken_semaphore_t *semaphore, uint32_t timeout_ms)
{
    sem_t *sem;
    int ret;

    if (semaphore == NULL || *semaphore == NULL)
        return kParamErr;

    sem = (sem_t *)(*semaphore);

    if (timeout_ms == BEKEN_WAIT_FOREVER ||
        timeout_ms == BEKEN_NEVER_TIMEOUT)
    {
        ret = nxsem_wait(sem);
    }
    else if (timeout_ms == 0)
    {
        ret = nxsem_trywait(sem);
    }
    else
    {
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec  += timeout_ms / 1000;
        ts.tv_nsec += (timeout_ms % 1000) * 1000000L;
        if (ts.tv_nsec >= 1000000000L)
        {
            ts.tv_sec++;
            ts.tv_nsec -= 1000000000L;
        }
        ret = nxsem_timedwait(sem, &ts);
    }

    if (ret == OK)
        return kNoErr;
    if (ret == -ETIMEDOUT)
        return kTimeoutErr;

    return kGeneralErr;
}

int rtos_get_semaphore_count(beken_semaphore_t *semaphore)
{
    sem_t *sem;
    int count = 0;

    if (semaphore == NULL || *semaphore == NULL)
        return 0;

    sem = (sem_t *)(*semaphore);
    nxsem_get_value(sem, &count);

    /* NuttX stores a negative semcount to represent the number of tasks
     * waiting on an exhausted semaphore.  Beken's uxSemaphoreGetCount()
     * semantics return the currently available count (0 when empty), so
     * clamp the negative waiter count back to 0.
     */
    return (count < 0) ? 0 : count;
}

bk_err_t rtos_deinit_semaphore(beken_semaphore_t *semaphore)
{
    sem_t *sem;

    if (semaphore == NULL || *semaphore == NULL)
        return kParamErr;

    sem = (sem_t *)(*semaphore);
    nxsem_destroy(sem);
    free(sem);
    *semaphore = NULL;

    return kNoErr;
}

bk_err_t rtos_init_mutex(beken_mutex_t *mutex)
{
    mutex_t *m;

    if (mutex == NULL)
        return kParamErr;

    m = (mutex_t *)malloc(sizeof(mutex_t));
    if (m == NULL)
        return kGeneralErr;

    if (nxmutex_init(m) < 0)
    {
        free(m);
        return kGeneralErr;
    }

    *mutex = (void *)m;
    return kNoErr;
}

bk_err_t rtos_trylock_mutex(beken_mutex_t *mutex)
{
    mutex_t *m;

    if (mutex == NULL || *mutex == NULL)
        return kParamErr;

    m = (mutex_t *)(*mutex);
    return (nxmutex_trylock(m) < 0) ? kGeneralErr : kNoErr;
}

bk_err_t rtos_lock_mutex(beken_mutex_t *mutex)
{
    mutex_t *m;

    if (mutex == NULL || *mutex == NULL)
        return kParamErr;

    m = (mutex_t *)(*mutex);
    return (nxmutex_lock(m) < 0) ? kGeneralErr : kNoErr;
}

bk_err_t rtos_lock_mutex_timeout(beken_mutex_t *mutex, uint32_t timeout_ms)
{
    mutex_t *m;
    int ret;

    if (mutex == NULL || *mutex == NULL)
        return kParamErr;

    m = (mutex_t *)(*mutex);

    if (timeout_ms == BEKEN_WAIT_FOREVER ||
        timeout_ms == BEKEN_NEVER_TIMEOUT)
    {
        ret = nxmutex_lock(m);
    }
    else if (timeout_ms == 0)
    {
        ret = nxmutex_trylock(m);
    }
    else
    {
        ret = nxmutex_timedlock(m, timeout_ms);
    }

    if (ret == OK)
        return kNoErr;
    if (ret == -ETIMEDOUT)
        return kTimeoutErr;

    return kGeneralErr;
}

bk_err_t rtos_unlock_mutex(beken_mutex_t *mutex)
{
    mutex_t *m;

    if (mutex == NULL || *mutex == NULL)
        return kParamErr;

    m = (mutex_t *)(*mutex);
    return (nxmutex_unlock(m) < 0) ? kGeneralErr : kNoErr;
}

bk_err_t rtos_deinit_mutex(beken_mutex_t *mutex)
{
    mutex_t *m;

    if (mutex == NULL || *mutex == NULL)
        return kParamErr;

    m = (mutex_t *)(*mutex);
    nxmutex_destroy(m);
    free(m);
    *mutex = NULL;

    return kNoErr;
}

bk_err_t rtos_init_recursive_mutex(beken_mutex_t *mutex)
{
    rmutex_t *m;

    if (mutex == NULL)
        return kParamErr;

    m = (rmutex_t *)malloc(sizeof(rmutex_t));
    if (m == NULL)
        return kGeneralErr;

    if (nxrmutex_init(m) < 0)
    {
        free(m);
        return kGeneralErr;
    }

    *mutex = (void *)m;
    return kNoErr;
}

bk_err_t rtos_lock_recursive_mutex(beken_mutex_t *mutex)
{
    rmutex_t *m;

    if (mutex == NULL || *mutex == NULL)
        return kParamErr;

    m = (rmutex_t *)(*mutex);
    return (nxrmutex_lock(m) < 0) ? kGeneralErr : kNoErr;
}

bk_err_t rtos_unlock_recursive_mutex(beken_mutex_t *mutex)
{
    rmutex_t *m;

    if (mutex == NULL || *mutex == NULL)
        return kParamErr;

    m = (rmutex_t *)(*mutex);
    return (nxrmutex_unlock(m) < 0) ? kGeneralErr : kNoErr;
}

bk_err_t rtos_deinit_recursive_mutex(beken_mutex_t *mutex)
{
    rmutex_t *m;

    if (mutex == NULL || *mutex == NULL)
        return kParamErr;

    m = (rmutex_t *)(*mutex);
    nxrmutex_destroy(m);
    free(m);
    *mutex = NULL;

    return kNoErr;
}
static bool s_is_started_scheduler = false;
void rtos_start_scheduler(void)
{
	s_is_started_scheduler = true;
	nx_start();
}

bool rtos_is_scheduler_started(void)
{
	return s_is_started_scheduler;
}

bool rtos_is_in_interrupt_context() {
    return up_interrupt_context();
}