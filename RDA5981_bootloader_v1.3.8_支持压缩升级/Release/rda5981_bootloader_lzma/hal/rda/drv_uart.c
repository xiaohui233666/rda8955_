/*
 * File      : drv_uart.c
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2006 - 2016, RT-Thread Development Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Change Logs:
 * Date           Author       Notes
 * 2006-03-12     Bernard      first version
 */

#include <board.h>
#include <drv_uart.h>
#include <rda_api.h>

#ifdef TARGET_RDA5981
#include <rda5981_bootrom_api.h>
#endif

int uart_init(void)
{
    return 0;
}

int uart_putc(char c)
{
    uart_send_byte(c);
    return 1;
}

int uart_getc(void)
{
    return uart_recv_byte();
}

void uart_console_output(const char *str, int length)
{
    int index;

    for (index = 0; index < length; index ++)
    {
        if (*str == '\n')
            uart_putc('\r');

        uart_putc(*str);
        str++;
    }
}
