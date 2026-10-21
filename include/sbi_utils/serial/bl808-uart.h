/* SPDX-License-Identifier: BSD-2-Clause */

#ifndef __SERIAL_BL808_UART_H__
#define __SERIAL_BL808_UART_H__

#include <sbi/sbi_types.h>

int bl808_uart_init(unsigned long base, u32 in_freq, u32 baudrate);

#endif
