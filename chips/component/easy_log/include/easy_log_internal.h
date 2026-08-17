#ifndef EASY_LOG_INTERNAL_H
#define EASY_LOG_INTERNAL_H
#include <stdarg.h>
#include <stdint.h>
#include <unistd.h>

#define EASY_LOG_MAX_BUFFER_SIZE    512

typedef enum {
    EASY_LOG_LEVEL_ERROR = 0,
    EASY_LOG_LEVEL_WARNING,
    EASY_LOG_LEVEL_INFO,
    EASY_LOG_LEVEL_DEBUG,
} easy_log_level_t;

typedef enum {
    EASY_LOG_LINK_MB_UART = 0,  /**< Log output via mailbox UART (MB_UART) */
    EASY_LOG_LINK_UART,         /**< Log output via physical UART (UART_ID_1) */
} easy_log_link_t;

int easy_log_output(const char *buffer, size_t length);

int easy_log_printf(const char *format, ...);

int easy_log_get_core_id(void);

int easy_log_level_printf(easy_log_level_t level, const char *func, int line,
                          const char *format, ...);

#endif
