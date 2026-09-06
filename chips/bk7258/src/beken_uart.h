#pragma once
#include "os/os.h"
#include <nuttx/serial/serial.h>
bk_err_t beken_uart_init(void);

uart_dev_t *GET_CONSOLE_SERIAL(void);

