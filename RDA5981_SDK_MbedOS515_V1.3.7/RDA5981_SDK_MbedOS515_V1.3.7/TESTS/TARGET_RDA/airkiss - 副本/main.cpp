#include "mbed.h"
#include "rtos.h"
#include "inet.h"
#include "WiFiStackInterface.h"
#include "greentea-client/test_env.h"
#include "cmsis_os.h"
#include "rda_sys_wrapper.h"
#include "rda5991h_wland.h"
#include "lwip_stack.h"
#include "at.h"

#include "rda_wdt_api.h"
#include "rda_ccfg_api.h"

#include <stdio.h>
#include <time.h>

#include <stdlib.h>
#include <string.h>
#include "cJSON.h"


#include "lwip/api.h"
//#include "http.h"

#include "RC4_Encrypt.h"
#include "console_cfunc.h"
#include "b64.h"

#include <stdint.h>

#include "critical.h"

#include "serial_api.h"
#include "myconsole.h"

#define UART_RING_BUFFER_SIZE  2048

#define min(a,b) (((a) < (b)) ? (a) : (b))

#if defined(TOOLCHAIN_GCC_ARM)
#include <malloc.h>
#endif

extern void test_wifi(void);

void *wifi_main_msgQ;

#if defined(MBED_RTOS_SINGLE_THREAD)
  #error [NOT_SUPPORTED] test not supported
#endif


rda_console_t p_rda_console;
rtos::Mutex fifo_lock;

void tzset(void);
WiFiStackInterface wifi;

//#define core_util_critical_section_enter() fifo_lock.lock()
//#define core_util_critical_section_exit() fifo_lock.unlock()
#define core_util_critical_section_enter()
#define core_util_critical_section_exit()
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
			break;
		}
	}
	printf("##########scan return :%d\n", ret);
	free(bss_list);
	return wifi_ret;//1 err
}

//fifo////////////////////////////////////////////////////////////////////////////////////////////////////
unsigned int NodeLen;
typedef struct Node
{
	
	unsigned char isuse;
	unsigned char data;
	struct Node *pNext;
	struct Node *pPre;
}NODE, *pNODE;

#define EXIT_FAILURE 0

pNODE CreateDbLinkList(int BufSize);

void TraverseDbLinkList(pNODE pHead);

int IsEmptyDbLinkList(pNODE pHead);

int GetLengthDbLinkList(pNODE pHead);

int InsertEleDbLinkList(pNODE pHead, int pos, unsigned char data);

int DeleteEleDbLinkList(pNODE pHead, int pos);

unsigned char GetEleDbLinkList(pNODE pHead, int pos);

void FreeMemory(pNODE *ppHead);

NODE UART_REVBUF[1024];

pNODE CreateDbLinkList(int BufSize)
{
	core_util_critical_section_enter();
	int i, data = 0;
	pNODE pTail = 0, p_new = 0;
	memset(UART_REVBUF,0,sizeof(UART_REVBUF));
	//pNODE pHead = (pNODE)malloc(sizeof(NODE));
	pNODE pHead = UART_REVBUF;
	UART_REVBUF[0].isuse = 1;
	
	if (0 == pHead)
	{
		//printf("内存分配失败！\n");
		//exit(EXIT_FAILURE);
	}
	
	NodeLen = 0;
	pHead->data = 0;
	pHead->pPre = 0;
	pHead->pNext = 0;
	pTail = pHead;
 /*
	for (i=1; i<BufSize+1; i++)
	{
		p_new = (pNODE)malloc(sizeof(NODE));
 
		if (0 == p_new)
		{
			printf("内存分配失败！\n");
			return 0;
		}
 
		printf("请输入第%d个元素的值：", i);
		scanf("%d", &data);
 
		p_new->data = data;
		p_new->pNext = 0;
		p_new->pPre = pTail;
		pTail->pNext = p_new;
		pTail = p_new;
	}
 */
 core_util_critical_section_exit();
	return pHead;
}

void TraverseDbLinkList(pNODE pHead)
{
	core_util_critical_section_enter();
	pNODE pt = pHead->pNext;
 
/* 	printf("打印链表如：");
	while (pt != 0)
	{
		printf("%d ", pt->data);
		pt = pt->pNext;
	}
	putchar('\n'); */
	core_util_critical_section_exit();
}

int IsEmptyDbLinkList(pNODE pHead)
{
	core_util_critical_section_enter();
	pNODE pt = pHead->pNext;
 
	if (pt == 0){
		core_util_critical_section_exit();
		return 1;
	}else{
		core_util_critical_section_exit();
		return 0;
	}
}

int GetLengthDbLinkList(pNODE pHead)
{
	int length = 0;
	//core_util_critical_section_enter();
	
	length = NodeLen;
	//pNODE pt = pHead->pNext;
 /*
	while (pt != 0)
	{
		length++;
		pt = pt->pNext;
	}
*/
	//core_util_critical_section_exit();
	return length;
}

int GetLengthDbLinkList2(pNODE pHead)
{
	int length = 0;
	
	length = NodeLen;
	//pNODE pt = pHead->pNext;
 /*
	while (pt != 0)
	{
		length++;
		pt = pt->pNext;
	}
*/
	return length;
}

inline int InsertEleDbLinkList(pNODE pHead, int pos, unsigned char data)
{
	//printf("InsertEleDbLinkList\r\n");
	core_util_critical_section_enter();
	//printf("InsertEleDbLinkList1\r\n");
	pNODE pt = 0, p_new = 0;
 
	pNODE tmp_pHead = pHead;
	tmp_pHead = pHead;
	//printf("enter InsertEleDbLinkList\r\n");
	
	if (pos > 0 && pos < GetLengthDbLinkList2(pHead)+2)
	{
		//printf("InsertEleDbLinkList2\r\n");
		//printf("enter InsertEleDbLinkList (pNODE)malloc(sizeof(NODE))\r\n");
		//p_new = (pNODE)malloc(sizeof(NODE));
		int tmp_i;
		for(tmp_i=0;UART_REVBUF[tmp_i].isuse == 1;tmp_i++);
		p_new = UART_REVBUF+tmp_i;
		UART_REVBUF[tmp_i].isuse = 1;
		//printf("InsertEleDbLinkList3\r\n");
		if (0 == p_new)
		{
			printf("allo cmem err！\n");
			//exit(EXIT_FAILURE);
		}
 
		while (1)
		{
			pos--;
			if (0 == pos)
				break;
			pHead = pHead->pNext;
		}
			
		pt = pHead->pNext;
		p_new->data = data;
		p_new->pNext = pt;
		if (0 != pt)
			pt->pPre = p_new;
		p_new->pPre = pHead;
		pHead->pNext = p_new;
		
		NodeLen++;
		
		//printf("tmp_pHead->Len:%d\r\n",tmp_pHead->Len);
		core_util_critical_section_exit();
		//printf("InsertEleDbLinkList end1 \r\n");
		return 1;
	}
	else{
		core_util_critical_section_exit();
		//printf("InsertEleDbLinkList end2 \r\n");
		return 0;
	}
}

int DeleteEleDbLinkList(pNODE pHead, int pos)
{
	core_util_critical_section_enter();
	pNODE pt = 0;
	pNODE tmp_pHead = pHead;
	tmp_pHead = pHead;
	if (pos > 0 && pos < GetLengthDbLinkList2(pHead) + 1)
	{
		while (1)
		{
			pos--;
			if (0 == pos)
				break;
			pHead = pHead->pNext;
		}
		
		
		pt = pHead->pNext->pNext;
		//free(pHead->pNext);
		//UART_REVBUF[((pHead->pNext)-UART_REVBUF)/sizeof(NODE)].isuse = 0;
		unsigned int tmp_i = 0;
		for(tmp_i=0;&UART_REVBUF[tmp_i] != pHead->pNext;tmp_i++);
		UART_REVBUF[tmp_i].isuse = 0;
		pHead->pNext = pt;
		if (0 != pt)
			pt->pPre = pHead;
 
		NodeLen--;
		//printf("tmp_pHead->Len:%d\r\n",tmp_pHead->Len);
		core_util_critical_section_exit();
		return 1;
	}
	else{
		core_util_critical_section_exit();
		return 0;
	}
}

unsigned char GetEleDbLinkList(pNODE pHead, int pos)
{
	core_util_critical_section_enter();
	pNODE pt = 0;
 
	if (pos > 0 && pos < GetLengthDbLinkList2(pHead) + 1)
	{
		while (1)
		{
			pos--;
			if (0 == pos)
				break;
			pHead = pHead->pNext;
		}
		return pHead->pNext->data;
		core_util_critical_section_exit();
		return 1;
	}
	else{
		core_util_critical_section_exit();
		return 0;
	}
}

void FreeMemory(pNODE *ppHead)
{
	core_util_critical_section_enter();
	pNODE pt = 0;
 
	while (*ppHead != 0)
	{
		pt = (*ppHead)->pNext;
		free(*ppHead);
		if (0 != pt)
			pt->pPre = 0;
		*ppHead = pt;
	}
	core_util_critical_section_exit();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////

/* UART Serial Pins: tx, rx */
pNODE head = 0;
extern serial_t stdio_uart;
mbed::RawSerial serial(UART0_TX, UART0_RX);
unsigned char tmp_uart_char;
//void rx_irq_handler(void)
static void console_irq_handler(uint32_t id, SerialIrq event)
{
	unsigned int temp_len = 0;
	temp_len = 0;
	if(RxIrq == event)
	while(serial_readable(&stdio_uart))
    {
//    while(serial.readable()) {
		
		// message_t *message = mpool.alloc();
        // message->c = ti;
        // queue.put(message);
        // Thread::wait(QUEUE_PUT_DELAY);
        // osDelay(1000);
		tmp_uart_char=serial_getc(&stdio_uart);//serial.getc();
		InsertEleDbLinkList(head, GetLengthDbLinkList(head)+1, tmp_uart_char);
		
		//DeleteEleDbLinkList(head,GetLengthDbLinkList(head)+1);
		//printf("read_c:%c\r\n",tmp_uart_char);
		temp_len++;
        //serial.putc(serial.getc());
    }
	//core_util_critical_section_enter();
	//head->Len+=temp_len;
	//core_util_critical_section_exit();
	//osSemaphoreRelease(p_rda_console.console_sema.sema_id);
}

void deal_searial_input(const void *arg);
Thread uart_thread;

int test_serial()
{
	head = CreateDbLinkList(UART_RING_BUFFER_SIZE);
	
	int flag = IsEmptyDbLinkList(head);
	if (flag)
		printf("双向链表为空！\n");
	else
	{
		int length = GetLengthDbLinkList(head);
		//printf("双向链表的长度为：%d\n", length);
		TraverseDbLinkList(head);
	}
	
	p_rda_console.cmd_cnt = 0;

	#ifdef CMSIS_OS_RTX
		memset(p_rda_console.console_sema.data, 0, sizeof(p_rda_console.console_sema.data));
		p_rda_console.console_sema.def.semaphore = p_rda_console.console_sema.data;
	#endif
    p_rda_console.console_sema.sema_id = osSemaphoreCreate(&p_rda_console.console_sema.def, 0);
	
	serial_init(&stdio_uart, UART0_TX, UART0_RX);
	
    serial_baud(&stdio_uart,115200);
    serial_clear(&stdio_uart);
    stdio_uart.uart->IER |= 1 << 0;
	
	serial_irq_handler(&stdio_uart, console_irq_handler, (uint32_t)(&p_rda_console));
    serial_irq_set(&stdio_uart, RxIrq, 1);
	
	static osThreadDef(deal_searial_input, osPriorityNormal, 2*1024);
    p_rda_console.thread_id = osThreadCreate(osThread(deal_searial_input), NULL);
	
/*     serial.baud(115200);
    serial.format(8, SerialBase::None, 1);
    serial.printf("Start Serial test...\r\n");
    serial.attach(rx_irq_handler, SerialBase::RxIrq);
    serial.printf("Wait\r\n"); */
	
	//uart_thread.start(deal_searial_input);
	
    //Thread::wait(osWaitForever);
}

// static unsigned char tmp_buff[2048] = {0};
// static unsigned int tmp_buff_offset = 0;
// typedef rev_status_t enum{
	// WaitHeader = 0,
	// WaitLengthRevFin = 1,
	// RevicingData = 2,
	// WaitEndFlag = 3,
// }
//#define HTTP_HDR_MAXLEN 160
//char tmp_hdr[2][HTTP_HDR_MAXLEN];
cJSON *root = NULL;
char *out = NULL;
inline void analyze_json(unsigned char *str){
	

	root = cJSON_CreateObject();
	root = cJSON_Parse((char *)str);
	out = cJSON_Print(root);
	printf("json_str_analyze:%s\r\n",out);
	
	cJSON *Type = cJSON_GetObjectItem(root,"type");
	printf("type:%s\n",Type->valuestring);
/* 	if(strcmp(Type->valuestring,"http")==0){
		
		cJSON *Url = cJSON_GetObjectItem(root,"Url");
		printf("Url:%s\n",Url->valuestring);
		
		cJSON *Mothed = cJSON_GetObjectItem(root,"Mothed");
		printf("Mothed:%s\n",Mothed->valuestring);
		
		cJSON *Data = cJSON_GetObjectItem(root,"Data");
		printf("Data:%s\n",Data->valuestring);
		
		printf("hdr_size:%d\r\n",sizeof(tmp_hdr));
		memset(tmp_hdr,0,sizeof(tmp_hdr));
		printf("test1\r\n");
		strcpy(((char *)(tmp_hdr)),"Content-Type:application/x-www-form-urlencoded");
		printf("test2:%s\r\n",&tmp_hdr[0]);
		
		sprintf((char *)&(tmp_hdr[1]),"Content-Length:%d",strlen(Data->valuestring));
		
		printf("test:%s\r\n",&tmp_hdr[1]);
		printf("test t:%s\r\n",&((char **)tmp_hdr)[0]);
		http_response_t* p_resp = http_request_w_body(Url->valuestring, HTTP_REQ_POST, (char**)tmp_hdr, 2, Data->valuestring);
		printf("\r\nRESPONSE FROM :\n%s\n", p_resp->contents);
		
	}else */ 
	if(strcmp(Type->valuestring,"socket")==0){
	
		cJSON *Ip = cJSON_GetObjectItem(root,"Ip");
		printf("Ip:%s\n",Ip->valuestring);
		
		cJSON *Port = cJSON_GetObjectItem(root,"Port");
		printf("Port:%s\n",Port->valuestring);
		
		cJSON *Data = cJSON_GetObjectItem(root,"Data");
		printf("Data:%s\n",Data->valuestring);
		
		//socket tcp? short? or long?
		
	}else if(strcmp(Type->valuestring,"at")==0){
		cJSON *Code = cJSON_GetObjectItem(root,"Code");
		printf("Code:%s\n",Code->valuestring);

		if(run_command(Code->valuestring) < 0) {
			printf("command fail\r\n");
		}

	}
	
	free(out);
	cJSON_Delete(root);
}

  typedef struct
  {

    uint8_t li_vn_mode;      // Eight bits. li, vn, and mode.
                             // li.   Two bits.   Leap indicator.
                             // vn.   Three bits. Version number of the protocol.
                             // mode. Three bits. Client will pick mode 3 for client.

    uint8_t stratum;         // Eight bits. Stratum level of the local clock.
    uint8_t poll;            // Eight bits. Maximum interval between successive messages.
    uint8_t precision;       // Eight bits. Precision of the local clock.

    uint32_t rootDelay;      // 32 bits. Total round trip delay time.
    uint32_t rootDispersion; // 32 bits. Max error aloud from primary clock source.
    uint32_t refId;          // 32 bits. Reference clock identifier.

    uint32_t refTm_s;        // 32 bits. Reference time-stamp seconds.
    uint32_t refTm_f;        // 32 bits. Reference time-stamp fraction of a second.

    uint32_t origTm_s;       // 32 bits. Originate time-stamp seconds.
    uint32_t origTm_f;       // 32 bits. Originate time-stamp fraction of a second.

    uint32_t rxTm_s;         // 32 bits. Received time-stamp seconds.
    uint32_t rxTm_f;         // 32 bits. Received time-stamp fraction of a second.

    uint32_t txTm_s;         // 32 bits and the most important field the client cares about. Transmit time-stamp seconds.
    uint32_t txTm_f;         // 32 bits. Transmit time-stamp fraction of a second.

  } ntp_packet;              // Total: 384 bits or 48 bytes.
#define NTP_TIMESTAMP_DELTA 2208988800ull
UDPSocket udpsocket;
char* host_name = "time.windows.com";
void test_ntp(void){
	long rLen=0;
	
	// Create and zero out the packet. All 48 bytes worth.
	ntp_packet packet = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	memset( &packet, 0, sizeof( ntp_packet ) );
	// Set the first byte's bits to 00,011,011 for li = 0, vn = 3, and mode = 3. The rest will be left set to zero.
	*( ( char * ) &packet + 0 ) = 0x1b; // Represents 27 in base 10 or 00011011 in base 2.
	
	ip_addr_t server;
	int ret = netconn_gethostbyname(host_name, &server);
	if(ret != 0){
		printf("dns error\r\n");
	}

	SocketAddress addr(inet_ntoa(server), 123);
	if (!addr) {
		printf("address error\r\n");
		return;
	}

	udpsocket.open(&wifi);
	udpsocket.bind(0);

	rLen = udpsocket.sendto(inet_ntoa(server), 123, ( char* ) &packet, sizeof( ntp_packet ));

	printf("udp recv test begin \r\n");
	rLen = udpsocket.recvfrom(&addr,( void* ) &packet, sizeof( ntp_packet ));
	//printf("recv %d Bytes \n", rLen);
	if (rLen <= 0) {
		printf("ERROR: UDP recv error, len is %d £¡\r\n", rLen);
		return;
	}
	
	//struct _reent reent;
	//_putenv_r(&reent,"TZ=PST8PDT");
	//setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 3);
	//_tzset_r(&reent);
	
	packet.txTm_s = ntohl( packet.txTm_s ); // Time-stamp seconds.
	packet.txTm_f = ntohl( packet.txTm_f ); // Time-stamp fraction of a second.
	time_t txTm = ( time_t ) ( packet.txTm_s - NTP_TIMESTAMP_DELTA );
	struct tm *info;
	char buffer[80];
	info = localtime(&txTm);
	info->tm_hour += 8;
	strftime(buffer, 80, "%Y-%m-%d %H:%M:%S", info);
	printf("time:%s\r\n",buffer);
	
	//RESP_OK();
	printf("\tNTP timestamp is %s", ctime(&txTm));
	printf("udp recv test end \r\n");
}

int do_at_ntp( cmd_tbl_t *cmd, int argc, char *argv[]){
	printf("argc:%d argv:%s\r\n",argc,argv[1]);
	test_ntp();
	RESP_OK();
	return 0;
}

unsigned char json_buf[600];

/* void deal_searial_input(void){
	while(1){
		printf("GetLengthDbLinkList(head)=%d\r\n",GetLengthDbLinkList(head));
		DeleteEleDbLinkList(head,GetLengthDbLinkList(head));
		//printf("GetLengthDbLinkList(head)\r\n");
		//wait_us(100 * 1000);
	}
} */
#define wait_time 100
void deal_searial_input(const void *arg){
	unsigned int tmp_pkg_len;
	unsigned int tmp_pkg_len_i;
	int tmp_pos = 1;
	int tmp_Dat_pos;
	unsigned char tmp_check;
	while(1){
		//osSemaphoreRelease(p_rda_console.console_sema.sema_id);
		tmp_pos = 1;
		tmp_check = 0;
		tmp_pkg_len = 0;
	
		//wait hdr
		printf("wait hdr\r\n");
		while(1){
			while(tmp_pos > GetLengthDbLinkList(head)){printf("cur buf len:%d\r\n",GetLengthDbLinkList(head));Thread::wait(wait_time);}
			printf("read_fifo:%c[%x,%d]\r\n",GetEleDbLinkList(head, tmp_pos),GetEleDbLinkList(head, tmp_pos),GetEleDbLinkList(head, tmp_pos));
			if(GetEleDbLinkList(head, tmp_pos) == '<'){
				DeleteEleDbLinkList(head,tmp_pos);
				break;
			}
			DeleteEleDbLinkList(head,tmp_pos);
		}
		//wait pkg len
		printf("wait pkg len\r\n");
		while(GetLengthDbLinkList(head) < tmp_pos){printf("cur buf len:%d\r\n",GetLengthDbLinkList(head));Thread::wait(wait_time);}
		tmp_pkg_len = GetEleDbLinkList(head, tmp_pos);
		tmp_check = GetEleDbLinkList(head, tmp_pos);
		printf("wait pkg len:%d\r\n",GetEleDbLinkList(head, tmp_pos));
		
		tmp_pos++;
		while(GetLengthDbLinkList(head) < tmp_pos){printf("cur buf len:%d\r\n",GetLengthDbLinkList(head));Thread::wait(wait_time);}
		tmp_pkg_len = tmp_pkg_len << 8;
		tmp_pkg_len += GetEleDbLinkList(head, tmp_pos);
		tmp_check += GetEleDbLinkList(head, tmp_pos);
		printf("wait pkg len:%d\r\n",GetEleDbLinkList(head, tmp_pos));
		
		
		//rev len crc
		printf("rev len crc : %x\r\n",tmp_check);
		unsigned char len_crc_check;
		tmp_pos++;
		while(GetLengthDbLinkList(head) < tmp_pos){printf("cur buf len:%d\r\n",GetLengthDbLinkList(head));Thread::wait(wait_time);}
		len_crc_check = GetEleDbLinkList(head, tmp_pos);
		
		//check len crc
		printf("check crc check:%x,tmp_check:%x\r\n",len_crc_check,tmp_check);
		if(len_crc_check != tmp_check){
			//len crc err
			printf("len crc err check:%x,tmp_check:%x\r\n",len_crc_check,tmp_check);
			tmp_pos = 1;
			continue;
		}
		
		tmp_pkg_len_i = tmp_pkg_len;
		tmp_Dat_pos = tmp_pos + 1;
		//rev data
		printf("rev data tmp_pkg_len_i:%d\r\n",tmp_pkg_len_i);
		while(tmp_pkg_len_i--){
			tmp_pos++;
			while(GetLengthDbLinkList(head) < tmp_pos){printf("cur buf len:%d\r\n",GetLengthDbLinkList(head));Thread::wait(wait_time);}
			tmp_check += GetEleDbLinkList(head, tmp_pos);
			printf("rev data[%d]=%c\r\n",tmp_pos,GetEleDbLinkList(head, tmp_pos));
		}
		//rev crc
		printf("rev crc : %x\r\n",tmp_check);
		unsigned char check;
		tmp_pos++;
		while(GetLengthDbLinkList(head) < tmp_pos){printf("cur buf len:%d\r\n",GetLengthDbLinkList(head));Thread::wait(wait_time);}
		check = GetEleDbLinkList(head, tmp_pos);
		
		tmp_pos++;
		while(GetLengthDbLinkList(head) < tmp_pos){printf("cur buf len:%d\r\n",GetLengthDbLinkList(head));Thread::wait(wait_time);}
		if(GetEleDbLinkList(head, tmp_pos) != '>'){
			//pkg end err
			printf("pkg end err:%c,%x\r\n",GetEleDbLinkList(head, tmp_pos),GetEleDbLinkList(head, tmp_pos));
			tmp_pos = 1;
			continue;
		}
		
		//check crc
		printf("check crc check:%d,tmp_check:%d\r\n",check,tmp_check);
		if(check == tmp_check){
			//rev data
			memset(json_buf,0,sizeof(json_buf));
			tmp_pkg_len_i = tmp_pkg_len;
			unsigned char *tmp_json_buf_p;
			tmp_json_buf_p = json_buf;
			DeleteEleDbLinkList(head,1);
			DeleteEleDbLinkList(head,1);
			DeleteEleDbLinkList(head,1);
			while(tmp_pkg_len_i--){
				*(tmp_json_buf_p++) = GetEleDbLinkList(head, 1);
				tmp_Dat_pos++;
				DeleteEleDbLinkList(head,1);
			}
			DeleteEleDbLinkList(head,1);
			DeleteEleDbLinkList(head,1);
			printf("data:%s\r\n",json_buf);
			analyze_json(json_buf);
			//continue;
		}else{
			//crc err
			printf("crc err check:%d,tmp_check:%d\r\n",check,tmp_check);
			tmp_pos = 1;
			continue;
		}
		unsigned int tmp_i;
		for(tmp_i = 0;tmp_i<100;tmp_i++){
			printf("%d isuse: %d len:%d\r\n",tmp_i,UART_REVBUF[tmp_i].isuse,tmp_i,NodeLen);
		}
		
	}
	
}

void dump_heap_info(void)
{
#if defined(TOOLCHAIN_ARM)
    __heapstats((__heapprt)fprintf,stderr);
    __heapvalid((__heapprt) fprintf, stderr, 1);
#elif defined(TOOLCHAIN_GCC_ARM)
#if 0
// declared in <malloc.h>
struct mallinfo {
    size_t arena;    /* total space allocated from system */
    size_t ordblks;  /* number of non-inuse chunks */
    size_t smblks;   /* unused -- always zero */
    size_t hblks;    /* number of mmapped regions */
    size_t hblkhd;   /* total space in mmapped regions */
    size_t usmblks;  /* unused -- always zero */
    size_t fsmblks;  /* unused -- always zero */
    size_t uordblks; /* total allocated space */
    size_t fordblks; /* total non-inuse space */
    size_t keepcost; /* top-most, releasable (via malloc_trim) space */
};
#endif
    struct mallinfo info = {0,};
    //malloc_stats();
    info = mallinfo();
    printf("total system space   : %d\r\n",info.arena);
    printf("total allocated space: %d\r\n",info.uordblks);
    printf("total non-inuse space: %d\r\n",info.fordblks);
    printf("number of non-inuse chunks   : %d\r\n",info.ordblks);
    printf("biggest non-inuse chunk space: %d\r\n",info.keepcost);
#else
#error "Not Support!"
#endif
}
Thread heapinfo_thread;
void print_heap_info(void){
	while(1){
		dump_heap_info();
		wait_us(1000 * 1000);
	}
}


void feed_dog_thread(void *arg){
	int cnt = 0;
	wdt_t *obj_p;
	obj_p = (wdt_t *)arg;
	while(true) {
		printf("alive\r\n");
        Thread::wait(1500);
        cnt++;
        rda_wdt_feed(obj_p);
    }
}

int main() {
	
	wdt_t obj;
    
	rda_wdt_init(&obj, 2);
    rda_wdt_start(&obj);
	rda_thread_new(NULL, feed_dog_thread, (void *)&obj, 1024,osPriorityNormal);
	serial_init(&stdio_uart, UART0_TX, UART0_RX);
	
    serial_baud(&stdio_uart,115200);

	//heapinfo_thread.start(print_heap_info);
	printf("================================serial             test===================================\r\n");
	//test_serial();
	printf("================================            end        ===================================\r\n");
	
	printf("================================cjson              test===================================\r\n");
	//cjson_test();
	printf("================================            end        ===================================\r\n");
	
	printf("================================flash user data rw test===================================\r\n");
	//test_userdata_rw();
	printf("================================            end        ===================================\r\n");
	
	printf("================================start serial rw test===================================\r\n");
	//test_userdata_rw();
	printf("================================            end        ===================================\r\n");
	
	printf("================================start connect wifi test===================================\r\n");
/* 	wifi_main_msgQ = rda_msgQ_create(5);
	wifi.set_msg_queue(wifi_main_msgQ);
	int test_wifi_ret = wifi.connect("xiaohui", "qwertyui", NULL, NSAPI_SECURITY_NONE);//"ChaoMeng_03F", "cm88889999"
    if (test_wifi_ret==0) {
        printf("connect to success, ip %s\r\n", wifi.get_ip_address());
    } else {
        printf("connect to fail\r\n");
    } 
	rda_thread_new(NULL, wifi_status_thread, (void *)&obj, 1024,osPriorityRealtime); */
	
	printf("================================            end        ===================================\r\n");
	
	printf("================================start http post test===================================\r\n");
	//http_post();
	printf("================================            end        ===================================\r\n");
	
	printf("================================start     ntp   test===================================\r\n");
	//test_ntp();
	printf("================================            end        ===================================\r\n");

	printf("================================wifi.disconnect() test===================================\r\n");
	//wifi.disconnect();
	printf("================================            end        ===================================\r\n");

	printf("************************************** start ************************************************\r\n");
	test_serial();
	//发现当串口中断在扫描wifi的时候发生就会导致hard flat
	//it will connect wifi frist read wifi info from flash and connect this wifi,it will connect w3 if connect error which ssid read from the flash
/* 	char ssid[32];
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
	} */
	//start deal come code rev from uart
	
	printf("************************************************          end        ************************************************\r\n");


    while (true) {
		//loop
		printf("main loop\r\n");
		Thread::wait(1500);
        
    }
}