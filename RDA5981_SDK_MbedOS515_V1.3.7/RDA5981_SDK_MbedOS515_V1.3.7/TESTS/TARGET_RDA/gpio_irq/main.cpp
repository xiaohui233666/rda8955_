#include "mbed.h"
#include "rtos.h"

InterruptIn key(PC_0);  // PA_8(GPIO10) as gpio_irq pin

void key_up()
{
    mbed_error_printf("Key Up\r\n");
}

void key_down()
{
    mbed_error_printf("Key Down\r\n");
}

int main()
{
    printf("Start GPIO IRQ test...\r\n");
    key.rise(&key_up);
    key.fall(&key_down);

    while(true) {
		 printf("Start GPIO IRQ test...\r\n");
		 wait_us(500 * 1000);
    }
}
