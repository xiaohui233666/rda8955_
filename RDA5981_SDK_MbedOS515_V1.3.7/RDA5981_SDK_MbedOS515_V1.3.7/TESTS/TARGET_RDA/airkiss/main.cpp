#include "mbed.h"
#include "rtos.h"
#include "inet.h"
#include "WiFiStackInterface.h"
#include "greentea-client/test_env.h"
#include "cmsis_os.h"
#include "rda_sys_wrapper.h"
#include "rda5991h_wland.h"
#include "lwip_stack.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cJSON.h"

#include "lwip/api.h"
#include "http.h"


extern void test_wifi(void);

void *wifi_main_msgQ;

#if defined(MBED_RTOS_SINGLE_THREAD)
  #error [NOT_SUPPORTED] test not supported
#endif


WiFiStackInterface wifi;

void wifi_status_thread(){
	int ret, cnt = 1;
	unsigned int msg;
	while (true) {
        rda_msg_get(wifi_main_msgQ, &msg, osWaitForever);
        switch(msg)
        {
            case MAIN_RECONNECT: {
                printf("wifi disconnect!\r\n");
                ret = wifi.disconnect();
                if(ret != 0){
                    printf("disconnect failed!\r\n");
                    break;
                }
                ret = wifi.reconnect();
                while(ret != 0){
                    if(++cnt>5)
                        break;
                    osDelay(cnt*2*1000);
                    ret = wifi.reconnect();
                };
                cnt = 1;
                break;
            }
            case MAIN_STOP_AP:
                wifi.stop_ap(1);
                break;
            default:
                printf("unknown msg\r\n");
                break;
        }
    }
}

/*
*
 * function: read user data
 * @data: data to read
 * @len: length of data in byte
 * @flag: user data flag
 * return: 0:success, else:fail
 *
int rda5981_read_user_data(unsigned char *data, unsigned short len, unsigned int flag);

*
 * function: write user data
 * @data: data to write
 * @len: length of data in byte
 * @flag: user data flag
 * return: 0:success, else:fail
 *
int rda5981_write_user_data(unsigned char *data, unsigned short len, unsigned int flag);

*
 * function: erase user data
 * @flag: user data flag
 * return: 0:success, else:fail
 *
int rda5981_erase_user_data(unsigned int flag);
*/

void test_userdata_rw(){
	//test flag is 0,all data must be limt at 4KB
	//esare user data
	unsigned char test_buff[100]={0};
	printf("esare userdata status %d\r\n",rda5981_erase_user_data(1));
	printf("read userdata status %d\r\n",rda5981_read_user_data(test_buff, sizeof(test_buff), 1));
	int tmp_i = 0;
	for(tmp_i = 0;tmp_i < sizeof(test_buff);tmp_i++)
	{
		printf(" %x,",test_buff[tmp_i]);
		if(tmp_i % 6 == 0){
			printf("\r\n");
		}
	}
	
	memset(test_buff,0x55,sizeof(test_buff));
	printf("write userdata status %d\r\n",rda5981_write_user_data(test_buff, sizeof(test_buff), 1));
	
	printf("the read userdata status %d after write data\r\n",rda5981_read_user_data(test_buff, sizeof(test_buff), 1));
	for(tmp_i = 0;tmp_i < sizeof(test_buff);tmp_i++)
	{
		printf(" %x,",test_buff[tmp_i]);
		if(tmp_i % 6 == 0){
			printf("\r\n");
		}
	}
}

/* UART Serial Pins: tx, rx */
/*
mbed::RawSerial serial(UART0_TX, UART0_RX);

void rx_irq_handler(void)
{
    while(serial.readable()) {
		
		message_t *message = mpool.alloc();
        message->c = ti;
        queue.put(message);
        Thread::wait(QUEUE_PUT_DELAY);
        osDelay(1000);
		
        serial.putc(serial.getc());
    }
}

int test_serial()
{
    serial.baud(921600);
    serial.format(8, SerialBase::None, 1);
    serial.printf("Start Serial test...\r\n");
    serial.attach(rx_irq_handler, SerialBase::RxIrq);
    serial.printf("Wait forever\r\n");
    //Thread::wait(osWaitForever);
}

static unsigned char tmp_buff[2048] = {0};
static unsigned int tmp_buff_offset = 0;
typedef rev_status_t enum{
	WaitHeader = 0,
	WaitLengthRevFin = 1,
	RevicingData = 2,
	WaitEndFlag = 3,
}
void deal_searial_input(unsigned char * input_char){
	tmp_buff[tmp_buff_offset] = *input_char;
	rev_status_t rev_status=WaitHeader;
	switch (rev_status)
	{
	case WaitHeader:
		if(tmp_buff[tmp_buff_offset] =="<" )
		break;
	case WaitLengthRevFin:
		
		break;
	case 
	case WaitEndFlag:
		
		break;
	default:
		//error
		break;
	}
}
*/

int main() {
    int tmp_i;
    int ret;
	
	printf("================================flash user data rw test===================================\r\n");
	//test_serial();
	printf("================================            end        ===================================\r\n");
	
	printf("================================flash user data rw test===================================\r\n");
	cJSON *root = NULL;
	char *out = NULL;
	root = cJSON_CreateObject();
	cJSON_AddItemToObject(root, "name", cJSON_CreateString("Jack (\"Bee\") Nimble"));
	out=cJSON_Print(root);
	printf("json_str:%s\r\n",out);
	free(out);
	cJSON_Delete(root);
	root = cJSON_Parse("{\"name\":	\"testname\"}");
	out = cJSON_Print(root);
	printf("json_str_analyze:%s\r\n",out);
	free(out);
	cJSON_Delete(root);
	printf("================================            end        ===================================\r\n");
	
	
	printf("================================flash user data rw test===================================\r\n");
	test_userdata_rw();
	printf("================================            end        ===================================\r\n");
	
	printf("================================start serial rw test===================================\r\n");
	test_userdata_rw();
	printf("================================            end        ===================================\r\n");
	
	printf("================================start connect wifi test===================================\r\n");
	wifi_main_msgQ = rda_msgQ_create(5);
	wifi.set_msg_queue(wifi_main_msgQ);
	int wifi_ret = wifi.connect("xiaohui", "qwertyui", NULL, NSAPI_SECURITY_NONE);
    if (wifi_ret==0) {
        printf("connect to success, ip %s\r\n", wifi.get_ip_address());
    } else {
        printf("connect to fail\r\n");
    }
	printf("================================            end        ===================================\r\n");
	
	
	
	printf("================================start http post test===================================\r\n");
	http_post();
	printf("================================            end        ===================================\r\n");

    while (true) {
        
    }
}