/*
 * File      : drv_uart.c
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2013, RT-Thread Develop Team
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://openlab.rt-thread.com/license/LICENSE
 *
 * Change Logs:
 * Date           Author       Notes
 *
 */

#ifndef DRV_UART_H
#define DRV_UART_H

int uart_init(void);
int uart_putc(char c);
int uart_getc(void);

void uart_console_output(const char *str, int length);

#endif /* end of include guard: DRV_UART_H */
