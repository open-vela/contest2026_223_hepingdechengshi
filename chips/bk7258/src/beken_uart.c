/****************************************************************************
 *
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/
#include "beken_irq.h"
#include "bk7236.h"
#include "bk_private/bk_uart.h"
#include "driver/hal/hal_uart_types.h"
#include "driver/uart.h"
#include "gpio_driver.h"
#include <nuttx/arch.h>
#include <nuttx/irq.h>
#include <nuttx/serial/serial.h>
#include <spinlock.h>
typedef struct bk_uart_priv {
  uart_id_t uart_id;
  int uart_irq;
} bk_uart_priv_t;
static bool s_printf_uart_init = false;

void arm_lowputc(char ch) {
  if (s_printf_uart_init == false) {
    return;
  }

  bk_uart_write_bytes(UART_ID_1, &ch, 1);
}

void up_putc(int ch) {
  /* Check for LF */
  if (ch == '\n') {
    /* Add CR */
    arm_lowputc('\r');
  }

  arm_lowputc(ch);
}

void show_uart_info(void) {
  const char *hello = "Hello Uart\r\n";
  bk_uart_write_bytes(UART_ID_1, hello, strlen(hello));
}

bk_err_t beken_uart_init(void) {
  bk_err_t ret;
  uart_config_t uart_config = {
      .baud_rate = UART_BAUD_RATE,
      .data_bits = UART_DATA_8_BITS,
      .parity = UART_PARITY_NONE,
      .stop_bits = UART_STOP_BITS_1,
      .flow_ctrl = UART_FLOWCTRL_DISABLE,
      .src_clk = UART_SCLK_XTAL_26M,
      .rx_dma_en = UART_DMA_DISABLE,
      .tx_dma_en = UART_DMA_DISABLE,
  };

  gpio_dev_unmap(GPIO_0);
  gpio_dev_map(GPIO_0, GPIO_DEV_UART1_TXD);
  gpio_dev_unmap(GPIO_1);
  gpio_dev_map(GPIO_1, GPIO_DEV_UART1_RXD);
  ret = bk_uart_init(UART_ID_1, &uart_config);
  if (ret == BK_OK) {
    s_printf_uart_init = true;
    show_uart_info();
  }
  return ret;
}

/* UART interrupt status bits (see bk7258/soc/soc/uart_struct.h, int_status).
 */
#define UART_INT_TX_FIFO_NEED_WRITE (1u << 0) /* tx_fifo_need_write */
#define UART_INT_RX_FIFO_NEED_READ (1u << 1)  /* rx_fifo_need_read  */
#define UART_INT_RX_FINISH (1u << 6)          /* rx_finish          */

static int beken_uart_interrupt(int irq, void *context, void *arg) {
  struct uart_dev_s *dev = (struct uart_dev_s *)arg;
  uint32_t int_status;
  uint32_t int_enable;
  uint32_t status;

  /* Read the pending interrupt status, mask off the interrupts that are not
   * enabled, then clear the pending bits.  This mirrors the sequence used by
   * the Beken uart_isr_common() and keeps the UART1 status register drained.
   */
  struct bk_uart_priv *priv = (struct bk_uart_priv *)dev->priv;
  int_status = uart_get_interrupt_status(priv->uart_id);
  int_enable = uart_get_int_enable_status(priv->uart_id);
  status = int_status & int_enable;
  uart_clear_interrupt_status(priv->uart_id, int_status);

  /* Receive data available (rx_fifo_need_read | rx_finish). */

  if (status & (UART_INT_RX_FIFO_NEED_READ | UART_INT_RX_FINISH)) {
    uart_recvchars(dev);
  }

  /* Transmit FIFO needs data (tx_fifo_need_write). */

  if (status & UART_INT_TX_FIFO_NEED_WRITE) {
    uart_xmitchars(dev);
  }

  return OK;
}
int console_uart_setup(FAR struct uart_dev_s *dev) {
  (void)dev;
  return OK;
}
void console_uart_shutdown(FAR struct uart_dev_s *dev) {}
int console_uart_attach(FAR struct uart_dev_s *dev) {
  struct bk_uart_priv *priv = dev->priv;
  irq_attach(priv->uart_irq, beken_uart_interrupt, dev);
  return 0;
}
void console_uart_detach(FAR struct uart_dev_s *dev) {
  struct bk_uart_priv *priv = dev->priv;
  irq_detach(priv->uart_irq);
}
int console_uart_ioctl(FAR struct file *filep, int cmd, unsigned long arg) {
  return 0;
}
int console_uart_receive(FAR struct uart_dev_s *dev, FAR unsigned int *status) {
  struct bk_uart_priv *priv = dev->priv;
  int ch = uart_read_byte(priv->uart_id);

  if (status != NULL) {
    *status = 0;
  }

  return ch;
}
void console_uart_send(FAR struct uart_dev_s *dev, int ch) {
  struct bk_uart_priv *priv = dev->priv;
  uart_write_byte(priv->uart_id, (uint8_t)ch);
}
void console_uart_rxint(FAR struct uart_dev_s *dev, bool enable) {
  struct bk_uart_priv *priv = dev->priv;
  if (enable) {
    bk_uart_enable_rx_interrupt(priv->uart_id);
  } else {
    bk_uart_disable_rx_interrupt(priv->uart_id);
  }
}
void console_uart_txint(FAR struct uart_dev_s *dev, bool enable) {
  struct bk_uart_priv *priv = dev->priv;
  if (enable) {
    bk_uart_enable_tx_interrupt(priv->uart_id);
  } else {
    bk_uart_disable_tx_interrupt(priv->uart_id);
  }
}
bool console_uart_rxavailable(FAR struct uart_dev_s *dev) {
  struct bk_uart_priv *priv = dev->priv;
  return uart_read_ready(priv->uart_id) == BK_OK;
}
bool console_uart_txempty(FAR struct uart_dev_s *dev) {
  struct bk_uart_priv *priv = dev->priv;
  return bk_uart_is_tx_over(priv->uart_id);
}

/* Call to release some resource about the device when device was close
 * and unregistered.
 */

int console_uart_release(FAR struct uart_dev_s *dev) { return 0; }

ssize_t console_uart_recvbuf(FAR struct uart_dev_s *dev, FAR void *buf,
                             size_t len) {
  uint8_t *p = (uint8_t *)buf;
  size_t n;

  /* Drain bytes straight out of the hardware RX FIFO.  The Beken sw-kfifo
   * path (bk_uart_read_bytes) is not used here because the glue-layer ISR
   * replaces uart_isr_common() and therefore never fills it.
   */

  for (n = 0; n < len; n++) {
    int ret = uart_read_byte_ex(UART_ID_1, &p[n]);
    if (ret == -1) {
      break;
    }
  }

  return (ssize_t)n;
}

/* This method will send multiple bytes.
 * Returns the actual number of characters sent.
 */

ssize_t console_uart_sendbuf(FAR struct uart_dev_s *dev, FAR const void *buf,
                             size_t len) {
  bk_uart_write_bytes(UART_ID_1, buf, len);
  return (ssize_t)len;
}
bool console_uart_txready(FAR struct uart_dev_s *dev) {
  return uart_write_ready(UART_ID_1) == BK_OK;
}

char console_uart_tx_buffer[4096];
char console_uart_rx_buffer[4096];
struct uart_ops_s g_console_uart_ops = {
    .setup = console_uart_setup,
    .shutdown = console_uart_shutdown,
    .attach = console_uart_attach,
    .detach = console_uart_detach,
    .ioctl = console_uart_ioctl,
    .receive = console_uart_receive,
    .send = console_uart_send,
    .rxint = console_uart_rxint,
    .txint = console_uart_txint,
    .rxavailable = console_uart_rxavailable,
    .txempty = console_uart_txempty,
    .txready = console_uart_txready,
    .release = console_uart_release,
    .recvbuf = console_uart_recvbuf,
    .sendbuf = console_uart_sendbuf,

};
bk_uart_priv_t uart_priv_array[3] = {
    {.uart_id = UART_ID_0, .uart_irq = 0xFFFFFFFF},
    {.uart_id = UART_ID_1, .uart_irq = IRQ_NORMAL(UART1_IRQn)},
    {.uart_id = UART_ID_2, .uart_irq = IRQ_NORMAL(UART2_IRQn)},

};
SPINLOCK_SECTION uart_dev_t g_console_uart_dev = {
    .ops = &g_console_uart_ops,
    .xmit =
        {
            .buffer = console_uart_tx_buffer,
            .size = sizeof(console_uart_tx_buffer),
            .head = 0,
            .tail = 0,
        },
    .recv =
        {
            .buffer = console_uart_rx_buffer,
            .size = sizeof(console_uart_rx_buffer),
            .head = 0,
            .tail = 0,
        },
    .isconsole = true,
    .priv = &uart_priv_array[1],

};

uart_dev_t *GET_CONSOLE_SERIAL(void) { return &g_console_uart_dev; }
