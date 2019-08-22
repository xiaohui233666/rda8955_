#include <stdint.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include "console_cfunc.h"
#include "at.h"

#if  !defined ( __STATIC_INLINE )
#if   defined ( __CC_ARM )
  #define __STATIC_INLINE  static __inline
#elif defined ( __GNUC__ )
  #define __STATIC_INLINE  static inline
#elif defined ( __ICCARM__ )
  #define __STATIC_INLINE  static inline
#else
  #error "Unknown compiler"
#endif
#endif

static char erase_seq[] = "\b \b";       /* erase sequence         */
static char   tab_seq[] = "    ";        /* used to expand TABs    */
char console_buffer[CMD_CBSIZE];
char lastcommand[CMD_CBSIZE] = { 0, };
unsigned int echo_flag = 1;
extern int do_at( cmd_tbl_t *cmd, int argc, char *argv[]);
extern int do_at_ntp( cmd_tbl_t *cmd, int argc, char *argv[]);
cmd_tbl_t cmd_list[CMD_LIST_COUNT] = {
	{
		"AT",               1,   do_at,
		"AT                 - AT mode"
	},
	{
		"AT+Time",          2,   do_at_ntp,
		"AT+Time            - AT Time"
	},
};
unsigned int cmd_cntr = 2;
#define min(a,b) (((a) < (b)) ? (a) : (b))

static int parse_line (char *line, char *argv[])
{
    int nargs = 0;

    while (nargs < CMD_MAXARGS) {
        /* skip any white space */
        while ((*line == ' ') || (*line == '\t') || (*line == ',')) {
            ++line;
        }

        if (*line == '\0') {    /* end of line, no more args    */
            argv[nargs] = 0;
            return (nargs);
        }

        /** miaodefang modified for RDA5991 VerG Begin */
        /* Argument include space should be bracketed by quotation mark */
        if(*line == '\"') {
            /* Skip quotation mark */
            line++;

            /* Begin of argument string */
            argv[nargs++] = line;

            /* Until end of argument */
            while(*line && (*line != '\"')) {
                ++line;
            }
        } else {
            argv[nargs++] = line;    /* begin of argument string    */

            /* find end of string */
            while(*line && (*line != ',') && (*line != '=')) {
                ++line;
            }
        }
        /** miaodefang modified for RDA5991 VerG End */

        if (*line == '\0') {    /* end of line, no more args    */
            argv[nargs] = 0;
            return (nargs);
        }

        *line++ = '\0';         /* terminate current arg     */
    }

    RESP_ERROR(ERROR_ARG);

    return (nargs);
}

static cmd_tbl_t *find_cmd (const char *cmd)
{
    cmd_tbl_t *cmdtp;
    cmd_tbl_t *cmdtp_temp = &cmd_list[0];    /* Init value */
    uint32_t len;
    int n_found = 0;
    int i;

    len = strlen(cmd);

    for (i = 0;i < (int)cmd_cntr;i++) {
        cmdtp = &cmd_list[i];
		printf("cmd:%s cmdtp->name:%s\r\n",cmd,cmdtp->name);
        if (strncmp(cmd, cmdtp->name, len) == 0) {
            if (len == strlen(cmdtp->name))
                return cmdtp;      /* full match */

            cmdtp_temp = cmdtp;    /* abbreviated command ? */
            n_found++;
        }
    }
    if (n_found == 1) {  /* exactly one match */
        return cmdtp_temp;
    }

    return 0;   /* not found or ambiguous command */
}


int run_command(char *cmd)
{
    cmd_tbl_t *cmdtp;
    char *argv[CMD_MAXARGS + 1];    /* NULL terminated    */
    int argc;

    /* Extract arguments */
    if ((argc = parse_line(cmd, argv)) == 0) {
        return -1;
    }

    /* Look up command in command table */
    if ((cmdtp = find_cmd(argv[0])) == 0) {
        printf("command %s no found\r\n",argv[0]);
        RESP_ERROR(ERROR_CMD);
        return -1;
    }

    /* found - check max args */
    if (argc > cmdtp->maxargs) {
        RESP_ERROR(ERROR_ARG);
        return -1;
    }

    /* OK - call function to do the command */
    if ((cmdtp->cmd) (cmdtp, argc, argv) != 0) {
        return -1;
    }

    return 0;
}

static char *delete_char(char *buffer, char *p, int *colp, int *np, int plen)
{
    char *s;

    if (*np == 0) {
        return (p);
    }

    if (*(--p) == '\t') {            /* will retype the whole line    */
        while (*colp > plen) {
            // console_puts(erase_seq);
            (*colp)--;
        }
        for (s=buffer; s<p; ++s) {
            if (*s == '\t') {
                // console_puts(tab_seq+((*colp) & 07));
                *colp += 8 - ((*colp) & 07);
            } else {
                ++(*colp);
                // console_putc(*s);
            }
        }
    } else {
        // console_puts(erase_seq);
        (*colp)--;
    }
    (*np)--;
    return (p);
}


int handle_char(const char c, char *prompt) {
    static char   *p   = console_buffer;
    static int    n    = 0;              /* buffer index        */
    static int    plen = 0;           /* prompt length    */
    static int    col;                /* output column cnt    */

    if (prompt) {
        plen = strlen(prompt);
        if(plen == 1 && prompt[0] == 'r')
            plen = 0;
        else
            // console_puts(prompt);
        p = console_buffer;
        n = 0;
        return 0;
    }
    col = plen;

    /* Special character handling */
    switch (c) {
        case '\r':                /* Enter        */
        case '\n':
            *p = '\0';
            // console_puts("\r\n");
            return (p - console_buffer);

        case '\0':                /* nul            */
            return -1;

        case 0x03:                /* ^C - break        */
            console_buffer[0] = '\0';    /* discard input */
            return 0;

        case 0x15:                /* ^U - erase line    */
            while (col > plen) {
                // console_puts(erase_seq);
                --col;
            }
            p = console_buffer;
            n = 0;
            return -1;

        case 0x17:                /* ^W - erase word     */
            p=delete_char(console_buffer, p, &col, &n, plen);
            while ((n > 0) && (*p != ' ')) {
                p=delete_char(console_buffer, p, &col, &n, plen);
            }
            return -1;

        case 0x08:                /* ^H  - backspace    */
        case 0x7F:                /* DEL - backspace    */
            p=delete_char(console_buffer, p, &col, &n, plen);
            return -1;

        default:
         /*  Must be a normal character then  */
            if (n < CMD_CBSIZE-2) {
                if (c == '\t') {    /* expand TABs        */
                    // console_puts(tab_seq+(col&07));
                    col += 8 - (col&07);
                } else {
                    if(echo_flag == 1){
                        ++col;         /* echo input        */
                        // console_putc(c);
                    }
                }
                *p++ = c;
                ++n;
            } else {          /* Buffer full        */
                // console_putc('\a');
            }
    }

    return -1;
}



