/*****************************************************************************
* @file     console.cpp
* @date     26.07.2016
* @note
*
*****************************************************************************/
#include "console.h"
#include "at.h"
#include  <stdio.h>

static void console_rxisr_Callback(void) {
    char str[100];
    // while(serial_readable(&stdio_uart))
    while(1)
    {
        // unsigned char c = serial_getc(&stdio_uart);
           memset(str,0,sizeof(str));
           unsigned char *c = gets(str);
           str[strlen(str)+1]='\n';
           if(run_command(str) < 0) {
                printf("command fail\r\n");
            }
    }
}

int main(){
    // if(0 != console_cmd_add(&cmd_test)) {
    //     printf("Add cmd failed\r\n");
    // }
    console_rxisr_Callback();

}


