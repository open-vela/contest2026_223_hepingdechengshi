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
#include "driver/uart.h"
#include "gpio_driver.h"
#include <nuttx/arch.h>
#include <nuttx/serial/serial.h>
#include <spinlock.h>
/* BK7258 AP 没有物理 UART，串口输出通过 mailbox FIFO 发到 CP。
 * uart_ll_write_byte() / uart_ll_is_fifo_write_ready() 是 LL 层
 * 内联函数，直接写硬件寄存器（hw->fifo_port.v = data）。
 */

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

int console_uart_setup(FAR struct uart_dev_s *dev) { (void)dev; }
void console_uart_shutdown(FAR struct uart_dev_s *dev) {}
int console_uart_attach(FAR struct uart_dev_s *dev) { return 0; }
void console_uart_detach(FAR struct uart_dev_s *dev) {}
int console_uart_ioctl(FAR struct file *filep, int cmd, unsigned long arg) {
  return 0;
}
int console_uart_receive(FAR struct uart_dev_s *dev, FAR unsigned int *status) {

}
void console_uart_send(FAR struct uart_dev_s *dev, int ch) {}
void console_uart_rxint(FAR struct uart_dev_s *dev, bool enable) {}
void console_uart_txint(FAR struct uart_dev_s *dev, bool enable) {}
bool console_uart_rxavailable(FAR struct uart_dev_s *dev) { return false; }
bool console_uart_txempty(FAR struct uart_dev_s *dev) { return false; }

/* Call to release some resource about the device when device was close
 * and unregistered.
 */

int console_uart_release(FAR struct uart_dev_s *dev) { return 0; }

ssize_t console_uart_recvbuf(FAR struct uart_dev_s *dev, FAR void *buf,
                             size_t len) {
  return 0;
}

/* This method will send multiple bytes.
 * Returns the actual number of characters sent.
 */

ssize_t console_uart_sendbuf(FAR struct uart_dev_s *dev, FAR const void *buf,
                             size_t len) {
  return len;
}
bool console_uart_txready(FAR struct uart_dev_s *dev) { return true; }

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

};

uart_dev_t *GET_CONSOLE_SERIAL(void) { return &g_console_uart_dev; }