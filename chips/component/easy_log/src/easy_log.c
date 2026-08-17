#include "easy_log_internal.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include <common/bk_err.h>
#include <driver/mb_uart_driver.h>
#include <driver/uart.h>
#include <os/os.h>
#include "gpio_driver.h"
#ifndef CONFIG_EASY_LOG_UART_ID
#define CONFIG_EASY_LOG_UART_ID    MB_UART0
#endif

static u8 g_easy_log_uart_id = CONFIG_EASY_LOG_UART_ID;
static easy_log_link_t g_easy_log_link = EASY_LOG_LINK_MB_UART;
static bool g_easy_log_inited = false;

static const char *g_easy_log_level_string[] = {
    "E",    /* EASY_LOG_LEVEL_ERROR   */
    "W",    /* EASY_LOG_LEVEL_WARNING */
    "I",    /* EASY_LOG_LEVEL_INFO    */
    "D",    /* EASY_LOG_LEVEL_DEBUG   */
};

int easy_log_init(easy_log_link_t link)
{
    bk_err_t ret;

    g_easy_log_link = link;

    if (link == EASY_LOG_LINK_UART)
    {
        uart_config_t uart_config = {
            .baud_rate = UART_BAUD_RATE,
            .data_bits = UART_DATA_8_BITS,
            .parity    = UART_PARITY_NONE,
            .stop_bits = UART_STOP_BITS_1,
            .flow_ctrl = UART_FLOWCTRL_DISABLE,
            .src_clk   = UART_SCLK_XTAL_26M,
            .rx_dma_en = UART_DMA_DISABLE,
            .tx_dma_en = UART_DMA_DISABLE,
        };


		gpio_dev_unmap(GPIO_0);
		gpio_dev_map(GPIO_0, GPIO_DEV_UART1_TXD);
		gpio_dev_unmap(GPIO_1);
		gpio_dev_map(GPIO_1, GPIO_DEV_UART1_RXD);
        ret = bk_uart_init(UART_ID_1, &uart_config);
        if (ret != BK_OK)
        {
            return -1;
        }
    }
    else
    {
        ret = bk_mb_uart_dev_init(g_easy_log_uart_id, 1);
        if (ret != BK_OK)
        {
            return -1;
        }
    }

    g_easy_log_inited = true;
    return 0;
}

int easy_log_output(const char *buffer, size_t length)
{
    size_t sent_total = 0;
    int retry = 0;
    u16 sent;
    u16 ready;
    u16 chunk;

    if (!g_easy_log_inited)
    {
        return -1;
    }

    if (buffer == NULL || length == 0)
    {
        return 0;
    }

    if (length > 0xFFFF)
    {
        length = 0xFFFF;
    }

    if (g_easy_log_link == EASY_LOG_LINK_UART)
    {
        return (bk_uart_write_bytes(UART_ID_1, buffer, (uint32_t)length) == BK_OK) ? 0 : -1;
    }

    while (sent_total < length)
    {
        ready = bk_mb_uart_write_ready(g_easy_log_uart_id);
        if (ready == 0)
        {
            /* TX FIFO is full, wait for drain.
             * The mb_uart TX complete ISR will flush the FIFO
             * and bk_mb_uart_write_ready will eventually report free space.
             */
            if (++retry > 10000)
            {
                break;
            }
            continue;
        }

        chunk = (u16)(length - sent_total);
        if (chunk > ready)
        {
            chunk = ready;
        }

        sent = bk_mb_uart_write(g_easy_log_uart_id,
                                (u8 *)buffer + sent_total, chunk);
        if (sent == 0)
        {
            if (++retry > 10000)
            {
                break;
            }
            continue;
        }

        sent_total += sent;
        retry = 0;
    }

    return (sent_total == length) ? 0 : -1;
}

int easy_log_get_core_id(void)
{
    return rtos_get_core_id();
}

int easy_log_printf(const char *format, ...)
{
    char buffer[EASY_LOG_MAX_BUFFER_SIZE];
    va_list ap;
    int ret;
    size_t len;

    va_start(ap, format);
    ret = vsnprintf(buffer, sizeof(buffer), format, ap);
    va_end(ap);

    /* Ensure null-terminated if truncated */
    buffer[sizeof(buffer) - 1] = '\0';

    len = strlen(buffer);
    easy_log_output(buffer, len);

    return ret;
}

int easy_log_level_printf(easy_log_level_t level, const char *func, int line,
                          const char *format, ...)
{
    char buffer[EASY_LOG_MAX_BUFFER_SIZE];
    va_list ap;
    int ret = 0;
    size_t len;
    int offset;

    /* Format level prefix: [cpu_id][L] func(line):  */
    offset = snprintf(buffer, sizeof(buffer), "[%d][%s] %s(%d): ",
                      easy_log_get_core_id(),
                      g_easy_log_level_string[level], func, line);
    if (offset < 0 || (size_t)offset >= sizeof(buffer))
    {
        /* Prefix truncated, output what we can */
        buffer[sizeof(buffer) - 1] = '\0';
        goto output;
    }

    /* Format user message */
    va_start(ap, format);
    ret = vsnprintf(buffer + offset, sizeof(buffer) - offset, format, ap);
    va_end(ap);

    /* Append newline if there is room */
    len = strlen(buffer);
    if (len + 2 < sizeof(buffer))
    {
        buffer[len++] = '\r';
        buffer[len++] = '\n';
    }
    buffer[len] = '\0';

output:
    len = strlen(buffer);
    easy_log_output(buffer, len);

    return ret;
}
