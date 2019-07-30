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

#include "RC4_Encrypt.h"
#include "b64.h"

#include <stdint.h>

#include "critical.h"

typedef struct {
    unsigned char *buffer;  /* the buffer holding the data              */
    unsigned int size;      /* the size of the allocated buffer         */
    unsigned int in;        /* data is added at offset (in % size)      */
    unsigned int out;       /* data is extracted from off. (out % size) */
} kfifo;

#define UART_RING_BUFFER_SIZE  2048

#define min(a,b) (((a) < (b)) ? (a) : (b))

static unsigned char uart_recv_ring_buffer[UART_RING_BUFFER_SIZE] = {0};

static kfifo uart_kfifo;



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

void cjson_test(){
	
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
}

int scanNconnect_wifi(void){
	//scan w3
	int wifi_ret = 1;
	rda5981_scan_result *bss_list = NULL;
	int scan_res;
	printf("Start wifi_scan...\r\n");
	scan_res = wifi.scan(NULL, 0);
	printf("Start wifi_scan end...\r\n");
	bss_list = (rda5981_scan_result *)malloc(scan_res * sizeof(rda5981_scan_result));
	memset(bss_list, 0, scan_res * sizeof(rda5981_scan_result));
	if (bss_list == NULL) {
		printf("malloc buf fail\r\n");
		return -1;
	}
	int ret = wifi.scan_result(bss_list, scan_res);
	rda5981_scan_result *tmp_rda5981_scan_result=bss_list;
	for(int tmp_ret=ret;tmp_ret>0;tmp_ret--){
		printf("%d:%s\r\n",tmp_ret,(unsigned char *)((*(tmp_rda5981_scan_result++)).SSID));
		if(strstr((char *)((*(tmp_rda5981_scan_result++)).SSID),"CMW3")==(char *)((*(tmp_rda5981_scan_result++)).SSID)){
			//found ap
			unsigned char s[256] = {0};
			//yJyw9M9e8hA=
			char key[256] = {"TENBAY RC4 KEY"};
			char encode_str[]="CMW30002";
			strcpy(encode_str,(char *)((*(tmp_rda5981_scan_result++)).SSID));
			unsigned long len=strlen(encode_str);
			rc4_init(s,(unsigned char *)key, strlen(key));
			rc4_crypt(s,(unsigned char *)encode_str, strlen(encode_str));
			printf("encode_str: %s\r\n",encode_str);
			char * tmp_str=base64_encode(encode_str,len);
			printf("encode_str: %s\r\n",tmp_str);
			wifi_ret = wifi.connect((char *)((*(tmp_rda5981_scan_result++)).SSID), tmp_str, NULL, NSAPI_SECURITY_NONE);
			free(tmp_str);
		}
	}
	printf("##########scan return :%d\n", ret);
	free(bss_list);
	return wifi_ret;//1 err
}

//fifo///////////////////////////////////////////////////////////////////////
int kfifo_init(kfifo *fifo, unsigned char *buffer, unsigned int size)
{
	core_util_critical_section_enter();
    /* size must be a power of 2 */
    if((size & (size - 1)) != 0)
        return -1;

    if (!buffer)
        return -1;

    fifo->buffer = buffer;
    fifo->size = size;
    fifo->in = fifo->out = 0;
	core_util_critical_section_exit();

    return 0;
}

static inline void __kfifo_reset(kfifo *fifo)
{
    fifo->in = fifo->out = 0;
}

static inline void kfifo_reset(void)
{
	core_util_critical_section_enter();

    __kfifo_reset(&uart_kfifo);

	core_util_critical_section_exit();
}

unsigned int __kfifo_put(kfifo *fifo,
            unsigned char *buffer, unsigned int len)
{
    unsigned int l;

    len = min(len, fifo->size - fifo->in + fifo->out);

    /*
     * Ensure that we sample the fifo->out index -before- we
     * start putting bytes into the kfifo.
     */

    /* first put the data starting from fifo->in to buffer end */
    l = min(len, fifo->size - (fifo->in & (fifo->size - 1)));
    memcpy(fifo->buffer + (fifo->in & (fifo->size - 1)), buffer, l);

    /* then put the rest (if any) at the beginning of the buffer */
    memcpy(fifo->buffer, buffer + l, len - l);

    /*
     * Ensure that we add the bytes to the kfifo -before-
     * we update the fifo->in index.
     */

    fifo->in += len;

    return len;
}

unsigned int kfifo_put(unsigned char *buffer, unsigned int len)
{
    unsigned int ret;

	core_util_critical_section_enter();

    ret = __kfifo_put(&uart_kfifo, buffer, len);

	core_util_critical_section_exit();

    return ret;
}

static unsigned int __kfifo_get(kfifo *fifo,
            unsigned char *buffer, unsigned int len)
{
    unsigned int l;

    len = min(len, fifo->in - fifo->out);

    /*
     * Ensure that we sample the fifo->in index -before- we
     * start removing bytes from the kfifo.
     */

    /* first get the data from fifo->out until the end of the buffer */
    l = min(len, fifo->size - (fifo->out & (fifo->size - 1)));
    memcpy(buffer, fifo->buffer + (fifo->out & (fifo->size - 1)), l);

    /* then get the rest (if any) from the beginning of the buffer */
    memcpy(buffer + l, fifo->buffer, len - l);

    /*
     * Ensure that we remove the bytes from the kfifo -before-
     * we update the fifo->out index.
     */

    fifo->out += len;

    return len;
}

unsigned int kfifo_get(unsigned char *buffer, unsigned int len)
{
    unsigned int ret;
    kfifo *fifo;

	core_util_critical_section_enter();

    fifo = &uart_kfifo;

    ret = __kfifo_get(fifo, buffer, len);

    /*
     * optimization: if the FIFO is empty, set the indices to 0
     * so we don't wrap the next time
     */
    if (fifo->in == fifo->out)
        fifo->in = fifo->out = 0;

	core_util_critical_section_exit();

    return ret;
}

static inline unsigned int __kfifo_len(kfifo *fifo)
{
    return fifo->in - fifo->out;
}

unsigned int kfifo_len(void)
{
    unsigned int ret;
	
	core_util_critical_section_enter();

    ret = __kfifo_len(&uart_kfifo);

	core_util_critical_section_exit();

    return ret;
}
/*
	char * test_str[100];
	kfifo_init(&uart_kfifo, uart_recv_ring_buffer, UART_RING_BUFFER_SIZE);
	while(1){
		memset(test_str,0,100);
		gets(test_str);
		kfifo_put(test_str,strlen(test_str));
		int tmp_str_len = strlen(test_str);
		while(tmp_str_len--){
			char tmp_s;
			int ret = kfifo_get(&tmp_s,1);
			printf(",%c\r\n",tmp_s);
		}
		
	}
*/
//fifo end////////////////////////////////////////////////////////////////////

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
	
	printf("================================serial             test===================================\r\n");
	//test_serial();
	printf("================================            end        ===================================\r\n");
	
	printf("================================cjson              test===================================\r\n");
	cjson_test();
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
	int test_wifi_ret = wifi.connect("xiaohui", "qwertyui", NULL, NSAPI_SECURITY_NONE);
    if (test_wifi_ret==0) {
        printf("connect to success, ip %s\r\n", wifi.get_ip_address());
    } else {
        printf("connect to fail\r\n");
    }
	
	printf("================================            end        ===================================\r\n");
	
	printf("================================start http post test===================================\r\n");
	http_post();
	printf("================================            end        ===================================\r\n");

	printf("================================wifi.disconnect() test===================================\r\n");
	wifi.disconnect();
	printf("================================            end        ===================================\r\n");

	printf("************************************** start ************************************************\r\n");

	//it will connect wifi frist read wifi info from flash and connect this wifi,it will connect w3 if connect error which ssid read from the flash
	char ssid[32];
	char passwd[64];
	int wifi_ret;
	int r_flash_ret = rda5981_flash_read_sta_data(ssid, passwd);
    if (r_flash_ret==0 && strlen(ssid)) {//get ssid from flash
        printf("get ssid from flash: ssid:%s, pass:%s\r\n",ssid, passwd);
		wifi_ret = wifi.connect(ssid, passwd, NULL, NSAPI_SECURITY_NONE);
	}else{
		scanNconnect_wifi();
	}
	if(wifi_ret != 0){
		scanNconnect_wifi();
	}
	//start deal come code rev from uart
	
	printf("************************************************          end        ************************************************\r\n");

    while (true) {
        
    }
}