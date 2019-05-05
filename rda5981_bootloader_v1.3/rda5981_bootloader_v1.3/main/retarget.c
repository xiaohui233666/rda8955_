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
 * required by fopen() and freopen().
 *
 * @param name - file name with path.
 * @param openmode - a bitmap hose bits mostly correspond directly to
 *                     the ISO mode specification.
 * @return  -1 if an error occurs.
 */
FILEHANDLE _sys_open(const char *name, int openmode)
{
    /* Register standard Input Output devices. */
    if (strcmp(name, __stdin_name) == 0)
        return (STDIN);
    if (strcmp(name, __stdout_name) == 0)
        return (STDOUT);
    if (strcmp(name, __stderr_name) == 0)
        return (STDERR);

    return -1;
}

int _sys_close(FILEHANDLE fh)
{
    return 0;
}

/**
 * read data
 *
 * @param fh - file handle
 * @param buf - buffer to save read data
 * @param len - max length of data buffer
 * @param mode - useless, for historical reasons
 * @return The number of bytes not read.
 */
int _sys_read(FILEHANDLE fh, unsigned char *buf, unsigned len, int mode)
{
    if (fh == STDIN) {
        /* TODO */
        return 0;
    }

    if ((fh == STDOUT) || (fh == STDERR))
        return -1;

    return 0;
}

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

/**
 * put he file pointer at offset pos from the beginning of the file.
 *
 * @param pos - offset
 * @return the current file position, or -1 on failed
 */
int _sys_seek(FILEHANDLE fh, long pos)
{
    if (fh < STDERR)
        return -1;

    return -1;
}

/**
 * used by tmpnam() or tmpfile()
 */
int _sys_tmpnam(char *name, int fileno, unsigned maxlength)
{
    return -1;
}

char *_sys_command_string(char *cmd, int len)
{
    /* no support */
    return cmd;
}

/* This function writes a character to the console. */
void _ttywrch(int ch)
{
#ifdef RT_USING_CONSOLE
    char c;

    c = (char)ch;
    uart_putc(c);
#endif
}

void _sys_exit(int return_code)
{
    /* TODO: perhaps exit the thread which is invoking this function */
    while (1);
}

/**
 * return current length of file.
 *
 * @param fh - file handle
 * @return file length, or -1 on failed
 */
long _sys_flen(FILEHANDLE fh)
{
    return -1;
}

int _sys_istty(FILEHANDLE fh)
{
    return 0;
}

int remove(const char *filename)
{
    return -1;
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

