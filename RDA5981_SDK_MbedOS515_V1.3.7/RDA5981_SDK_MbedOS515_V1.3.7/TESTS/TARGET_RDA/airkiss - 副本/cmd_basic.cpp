/*****************************************************************************
* @file     cmd_basic.c
* @date     26.07.2016
* @note
*
******************************************************************************/
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "console_cfunc.h"
#include "at.h"

static int isdigit(unsigned char c)
{
    return ((c >= '0') && (c <='9'));
}

static int isxdigit(unsigned char c)
{
    if ((c >= '0') && (c <='9'))
        return 1;
    if ((c >= 'a') && (c <='f'))
        return 1;
    if ((c >= 'A') && (c <='F'))
        return 1;
    return 0;
}

static int islower(unsigned char c)
{
    return ((c >= 'a') && (c <='z'));
}

static unsigned char toupper(unsigned char c)
{
    if (islower(c))
        c -= 'a'-'A';
    return c;
}

uint32_t simple_strtoul(const char *cp,char **endp,unsigned int base)
{
    uint32_t result = 0,value;

    if (*cp == '0') {
        cp++;
        if ((*cp == 'x') && isxdigit(cp[1])) {
            base = 16;
            cp++;
        }
        if (!base) {
            base = 8;
        }
    }
    if (!base) {
        base = 10;
    }
    while (isxdigit(*cp) && (value = isdigit(*cp) ? *cp-'0' : (islower(*cp)
            ? toupper(*cp) : *cp)-'A'+10) < base) {
        result = result*base + value;
        cp++;
    }
    if (endp)
        *endp = (char *)cp;
    return result;
}

int cmd_get_data_size(const char *arg, int default_size)
{
    /* Check for a size specification .b, .w or .l. */
    int len = strlen(arg);

    if((len > 2) && (arg[len - 2] == '.')) {
        switch(arg[len - 1]) {
            case 'b': return 1;
            case 'w': return 2;
            case 'l': return 4;
            case 's': return -2;
            default:  return -1;
        }
    }

    return default_size;
}

void show_cmd_usage(const cmd_tbl_t *cmd)
{
    printf("Usage:\r\n%s %d\r\n -%s\r\n", cmd->name, cmd->maxargs, cmd->usage);
}

int add_cmd_to_list(const cmd_tbl_t *cmd)
{
    cmd_tbl_t *tmp_cmd;

    if(CMD_LIST_COUNT <= cmd_cntr) {
        printf("No more cmds supported.\r\n");
        return -1;
    }

    tmp_cmd = &(cmd_list[cmd_cntr]);
    cmd_cntr++;
    memcpy((char *)tmp_cmd, (char *)cmd, sizeof(cmd_tbl_t));

    return 0;
}

int do_at( cmd_tbl_t *cmd, int argc, char *argv[])
{
    RESP_OK();
    return 0;
}



