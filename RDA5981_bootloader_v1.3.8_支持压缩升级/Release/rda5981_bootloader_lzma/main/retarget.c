#include <string.h>
#include <stdio.h>
#include <drv_uart.h>

#if defined (__CC_ARM)
#include <rt_sys.h>

#pragma import(__use_no_semihosting_swi)

/* TODO: Standard IO device handles. */
#define STDIN       1
#define STDOUT      2
#define STDERR      3

/* Standard IO device name defines. */
const char __stdin_name[]  = "STDIN";
const char __stdout_name[] = "STDOUT";
const char __stderr_name[] = "STDERR";

/**
 * write data
 *
 * @param fh - file handle
 * @param buf - data buffer
 * @param len - buffer length
 * @param mode - useless, for historical reasons
 * @return a positive number representing the number of characters not written.
 */
int _sys_write(FILEHANDLE fh, const unsigned char *buf, unsigned len, int mode)
{
    if ((fh == STDOUT) || (fh == STDERR)) {
        uart_console_output((const char*)buf, len);
        return 0;
    }

    if (fh == STDIN)
        return -1;

    return 0;
}


#ifdef __MICROLIB
#include <stdio.h>

int fputc(int c, FILE *f)
{
    char ch = c;

    uart_console_output(&ch, 1);
    return 1;
}

int fgetc(FILE *f)
{
    return -1;
}
#endif

#elif defined (__GNUC__)

#include <stdio.h>
#include <stdlib.h>

int _write(int fd, char *ptr, int len)
{
    /*
     * write "len" of char from "ptr" to file id "fd"
     * Return number of char written.
     *
    * Only work for STDOUT, STDIN, and STDERR
     */
    if (fd > 2)
    {
        return -1;
    }

    uart_console_output((const char*)ptr, len);

    return len;
}

#else
#error unknown complier
#endif

