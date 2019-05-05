#include "board.h"
#include "drv_uart.h"

/**
 * This function will initialize board.
 */
void board_init()
{
#if defined (__CC_ARM)
    __disable_irq();
#elif defined (__GNUC__)
    __asm__ __volatile__ ("cpsid i");
#elif
#error unknown complier
#endif
    uart_init();
}
