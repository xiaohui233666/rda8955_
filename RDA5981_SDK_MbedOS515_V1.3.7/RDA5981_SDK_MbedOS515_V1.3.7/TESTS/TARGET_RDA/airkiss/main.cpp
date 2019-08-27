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

#define UART_RING_BUFFER_SIZE  2048

#define min(a,b) (((a) < (b)) ? (a) : (b))

#if defined(TOOLCHAIN_GCC_ARM)
//#include <malloc.h>
#endif

extern void test_wifi(void);
void response(char *cmd,char *dat,char *err);
void *wifi_main_msgQ;

#if defined(MBED_RTOS_SINGLE_THREAD)
  #error [NOT_SUPPORTED] test not supported
#endif


rda_console_t p_rda_console;
rtos::Mutex fifo_lock;

void tzset(void);
WiFiStackInterface wifi;
UDPSocket udpsocket;
//#define core_util_critical_section_enter_def() fifo_lock.lock()
//#define core_util_critical_section_exit_def() fifo_lock.unlock()
#define core_util_critical_section_enter_def() 
//core_util_critical_section_enter()
#define core_util_critical_section_exit_def() 
//core_util_critical_section_exit()

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

char* host_name = "time.windows.com";

//fifo////////////////////////////////////////////////////////////////////////////////////////////////////
int NodeLen;
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

__STATIC_INLINE int IsEmptyDbLinkList(pNODE pHead);

__STATIC_INLINE int GetLengthDbLinkList(pNODE pHead);

__STATIC_INLINE int InsertEleDbLinkList(pNODE pHead, int pos, unsigned char data);

__STATIC_INLINE int DeleteEleDbLinkList(pNODE pHead, int pos);

__STATIC_INLINE unsigned char GetEleDbLinkList(pNODE pHead, int pos);

void FreeMemory(pNODE *ppHead);

static NODE UART_REVBUF[2048];

pNODE CreateDbLinkList(int BufSize)
{
	int i, data = 0;
	pNODE pTail = 0, p_new = 0;
	memset(UART_REVBUF,0,sizeof(UART_REVBUF));
	//pNODE pHead = (pNODE)malloc(sizeof(NODE));
	pNODE pHead = UART_REVBUF;
	UART_REVBUF[0].isuse = 1;
	if (0 == pHead)
	{
		//printf("err\n");
	}
	NodeLen = 0;
	pHead->data = 0;
	pHead->pPre = 0;
	pHead->pNext = 0;
	pTail = pHead;
	return pHead;
}

void TraverseDbLinkList(pNODE pHead)
{
	pNODE pt = pHead->pNext;
}

__STATIC_INLINE int IsEmptyDbLinkList(pNODE pHead)
{
	pNODE pt = pHead->pNext;
 
	if (pt == 0){
		return 1;
	}else{
		return 0;
	}
}

__STATIC_INLINE int GetLengthDbLinkList(pNODE pHead)
{
	core_util_critical_section_enter();
	int length = 0;
	length = NodeLen;
	core_util_critical_section_exit();
	return length;
}

__STATIC_INLINE int InsertEleDbLinkList(pNODE pHead, int pos, unsigned char data)
{
	core_util_critical_section_enter();
	int ret;
	pNODE pt = 0, p_new = 0;
	pNODE tmp_pHead = pHead;
	tmp_pHead = pHead;
	if (pos > 0 && pos < NodeLen +2)
	{
		//p_new = (pNODE)malloc(sizeof(NODE));
		int tmp_i;
		for(tmp_i=0;UART_REVBUF[tmp_i].isuse == 1;tmp_i++);
		p_new = UART_REVBUF+tmp_i;
		UART_REVBUF[tmp_i].isuse = 1;
		if (0 == p_new)
		{
			//printf("alloc cmem err！\n");
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
		ret = 1;
	}
	else{
		ret = 0;
	}
	core_util_critical_section_exit();
	return ret;
}

__STATIC_INLINE int DeleteEleDbLinkList(pNODE pHead, int pos)
{
	core_util_critical_section_enter();
	int ret;
	pNODE pt = 0;
	pNODE tmp_pHead = pHead;
	tmp_pHead = pHead;
	if (pos > 0 && pos < GetLengthDbLinkList(pHead) + 1)
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
		unsigned int tmp_i = 0;
		for(tmp_i=0;&UART_REVBUF[tmp_i] != pHead->pNext;tmp_i++);
		UART_REVBUF[tmp_i].isuse = 0;
		UART_REVBUF[tmp_i].data = 0;
		pHead->pNext = pt;
		if (0 != pt)
			pt->pPre = pHead;
		NodeLen--;
		ret = 1;
	}
	else{
		ret = 0;
	}
	core_util_critical_section_exit();
	return ret;
}

__STATIC_INLINE unsigned char GetEleDbLinkList(pNODE pHead, int pos)
{
	core_util_critical_section_enter();
	pNODE pt = 0;
	unsigned char ret;
	if (pos > 0 && pos < GetLengthDbLinkList(pHead) + 1)
	{
		while (1)
		{
			pos--;
			if (0 == pos)
				break;
			pHead = pHead->pNext;
		}
		ret = pHead->pNext->data;
	}
	else{
		ret = 0;
	}
	core_util_critical_section_exit();
	return ret;
}

void FreeMemory(pNODE *ppHead)
{
	pNODE pt = 0;
 
	while (*ppHead != 0)
	{
		pt = (*ppHead)->pNext;
		free(*ppHead);
		if (0 != pt)
			pt->pPre = 0;
		*ppHead = pt;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////

/* UART Serial Pins: tx, rx */
pNODE head = 0;
extern serial_t stdio_uart;
mbed::RawSerial serial(UART0_TX, UART0_RX);
static void console_irq_handler(uint32_t id, SerialIrq event);

void deal_searial_input(const void *arg);
Thread uart_thread;

int test_serial()
{
	head = CreateDbLinkList(UART_RING_BUFFER_SIZE);
	int flag = IsEmptyDbLinkList(head);
	if (flag){
		//printf("NULL\n");
	}else
	{
		int length = GetLengthDbLinkList(head);
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
}
void Print(char *s){
	while(*s)
		serial_putc(&stdio_uart, *s++);
}
char buffer[80];
int do_ntp(void){
	long rLen=0;
	if(wifi.get_ip_address() == NULL)
		return;
	// Create and zero out the packet. All 48 bytes worth.
	ntp_packet packet = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	memset( &packet, 0, sizeof( ntp_packet ) );
	// Set the first byte's bits to 00,011,011 for li = 0, vn = 3, and mode = 3. The rest will be left set to zero.
	*( ( char * ) &packet + 0 ) = 0x1b; // Represents 27 in base 10 or 00011011 in base 2.
	
	ip_addr_t server;
	int ret = netconn_gethostbyname(host_name, &server);
	if(ret != 0){
		response("AT+Time","","dns error");
		return 0;
	}

	SocketAddress addr(inet_ntoa(server), 123);
	if (!addr) {
		response("AT+Time","","address error");
		return 0;
	}
	
	udpsocket.open(&wifi);
	udpsocket.bind(0);
	rLen = udpsocket.sendto(inet_ntoa(server), 123, ( char* ) &packet, sizeof( ntp_packet ));
	rLen = udpsocket.recvfrom(&addr,( void* ) &packet, sizeof( ntp_packet ));
	//printf("recv %d Bytes \n", rLen);
	if (rLen <= 0) {
		//printf("ERROR: UDP recv error, len is %d £¡\r\n", rLen);
		response("AT+Time","","rev error");
		return 0;
	}
	
	//struct _reent reent;
	//_putenv_r(&reent,"TZ=PST8PDT");
	//setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 3);
	//_tzset_r(&reent);
	
	packet.txTm_s = ntohl( packet.txTm_s ); // Time-stamp seconds.
	packet.txTm_f = ntohl( packet.txTm_f ); // Time-stamp fraction of a second.
	time_t txTm = ( time_t ) ( packet.txTm_s - NTP_TIMESTAMP_DELTA );
	struct tm *info;
	
	udpsocket.close();
	info = localtime(&txTm);
	info->tm_hour += 8;
	memset(buffer,0,sizeof(buffer));
	strftime(buffer, 80, "%Y-%m-%d %H:%M:%S", info);
	response("AT+Time",buffer,"");
	//RESP_OK();
	//printf("\tNTP timestamp is %s", ctime(&txTm));
	//printf("udp recv test end \r\n");
}

void response(char *cmd,char *dat,char *err){
	Print((char *)'<');
	cJSON *root = NULL;
	char *out = NULL;
	unsigned int TmpLen = 0;
	unsigned int TmpDataCrc;
	char *TmpStrP;
	root = cJSON_CreateObject();
	cJSON_AddItemToObject(root, "cmd", cJSON_CreateString(cmd));
	cJSON_AddItemToObject(root, "data", cJSON_CreateString(dat));
	cJSON_AddItemToObject(root, "err", cJSON_CreateString(err));
	out=cJSON_Print(root);
	TmpLen = strlen(out);
	serial_putc(&stdio_uart, (TmpLen>>8)&0xff);
	serial_putc(&stdio_uart, TmpLen&0xff);
	serial_putc(&stdio_uart, (((TmpLen>>8)&0xff)+(TmpLen&0xff))&0xff);
	TmpDataCrc = 0;
	TmpStrP = out;
	while(*TmpStrP){
		if(TmpDataCrc < 0xff)
			TmpDataCrc+=*TmpStrP;
		serial_putc(&stdio_uart,*TmpStrP);
	}
	free(out);
	cJSON_Delete(root);
	serial_putc(&stdio_uart,(char)(TmpDataCrc&0xff));
	Print((char *)'>');
}

int do_at_ntp(cmd_tbl_t *cmd, int argc, char *argv[]){
	//printf("argc:%d argv:%s\r\n",argc,argv[1]);
	do_ntp();
	return 0;
}

unsigned char json_buf[600];

/* void deal_searial_input(const void *arg){
	while(1){
		GetLengthDbLinkList(head);
		GetEleDbLinkList(head, 1);
		DeleteEleDbLinkList(head,GetLengthDbLinkList(head));
	}
} */

static void console_irq_handler(uint32_t id, SerialIrq event)
{
	unsigned char tmp_uart_char;
	if(RxIrq == event)
	while(serial_readable(&stdio_uart))
    {
		tmp_uart_char=serial_getc(&stdio_uart);//serial.getc();
		InsertEleDbLinkList(head, GetLengthDbLinkList(head)+1, tmp_uart_char);
		//printf("read_c:%c\r\n",tmp_uart_char);
		Print("read\r\n");
        //serial.putc(serial.getc());
    }
	//NodeLen+=TmpLen;
}

void feed_dog_thread(void *arg){
	//int cnt = 0;
	wdt_t *obj_p;
	obj_p = (wdt_t *)arg;
	while(true) {
		//printf("alive\r\n");
        Thread::wait(1500);
        //cnt++;
        rda_wdt_feed(obj_p);
    }
}

cJSON *root = NULL;
char *out = NULL;
inline void analyze_json(unsigned char *str){
	
	root = cJSON_CreateObject();
	root = cJSON_Parse((char *)str);
	out = cJSON_Print(root);
	Print("json_str_analyze:");Print(out);Print("\r\n");
	
	cJSON *Type = cJSON_GetObjectItem(root,"type");
	Print("type:");Print(Type->valuestring);Print("\r\n");
	if(strcmp(Type->valuestring,"tcp0")==0){
	
		cJSON *Ip = cJSON_GetObjectItem(root,"Ip");
		Print("Ip:");Print(Ip->valuestring);Print("\r\n");
		
		cJSON *Port = cJSON_GetObjectItem(root,"Port");
		Print("Port:");Print(Port->valuestring);Print("\r\n");
		
		cJSON *Data = cJSON_GetObjectItem(root,"Data");
		Print("Data:");Print(Data->valuestring);Print("\r\n");
		//socket tcp? short? or long?
		
	}else if(strcmp(Type->valuestring,"tcp1")==0){
	
		cJSON *Ip = cJSON_GetObjectItem(root,"Ip");
		Print("Ip:");Print(Ip->valuestring);Print("\r\n");
		
		cJSON *Port = cJSON_GetObjectItem(root,"Port");
		Print("Port:");Print(Port->valuestring);Print("\r\n");
		
		cJSON *Data = cJSON_GetObjectItem(root,"Data");
		Print("Data:");Print(Data->valuestring);Print("\r\n");
		//socket tcp? short? or long?
		
	}else if(strcmp(Type->valuestring,"tcp2")==0){
	
		cJSON *Ip = cJSON_GetObjectItem(root,"Ip");
		Print("Ip:");Print(Ip->valuestring);Print("\r\n");
		
		cJSON *Port = cJSON_GetObjectItem(root,"Port");
		Print("Port:");Print(Port->valuestring);Print("\r\n");
		
		cJSON *Data = cJSON_GetObjectItem(root,"Data");
		Print("Data:");Print(Data->valuestring);Print("\r\n");
		//socket tcp? short? or long?
		
	}else if(strcmp(Type->valuestring,"tcp2")==0){
	
		cJSON *Ip = cJSON_GetObjectItem(root,"Ip");
		Print("Ip:");Print(Ip->valuestring);Print("\r\n");
		
		cJSON *Port = cJSON_GetObjectItem(root,"Port");
		Print("Port:");Print(Port->valuestring);Print("\r\n");
		
		cJSON *Data = cJSON_GetObjectItem(root,"Data");
		Print("Data:");Print(Data->valuestring);Print("\r\n");
		//socket tcp? short? or long?
		
	}else if(strcmp(Type->valuestring,"at")==0){
		cJSON *Code = cJSON_GetObjectItem(root,"Code");
		Print("Code:");Print(Code->valuestring);Print("\r\n");
		
		if(run_command(Code->valuestring) < 0) {
			//Print("command fail\r\n");
			response(Type->valuestring,"","cmd error");
		}

	}
	free(out);
	cJSON_Delete(root);
}

void deal_searial_input(const void *arg){
	unsigned int tmp_pkg_len;
	unsigned int tmp_pkg_len_i;
	int tmp_pos = 1;
	int tmp_Dat_pos;
	unsigned char tmp_check;
	while(1){
		tmp_pos = 1;
		tmp_check = 0;
		tmp_pkg_len = 0;
	
		//wait hdr
		//printf("wait hdr\r\n");
		while(1){
			while(tmp_pos > GetLengthDbLinkList(head));//{printf("cur buf len:%d\r\n",GetLengthDbLinkList(head));}
			//printf("read_fifo:%c[%x,%d]\r\n",GetEleDbLinkList(head, tmp_pos),GetEleDbLinkList(head, tmp_pos),GetEleDbLinkList(head, tmp_pos));
			if(GetEleDbLinkList(head, tmp_pos) == '<'){
				DeleteEleDbLinkList(head,tmp_pos);
				break;
			}
			DeleteEleDbLinkList(head,tmp_pos);
		}
		//wait pkg len
		//printf("wait pkg len\r\n");
		while(GetLengthDbLinkList(head) < tmp_pos);//{printf("cur buf len:%d\r\n",GetLengthDbLinkList(head));}
		tmp_pkg_len = GetEleDbLinkList(head, tmp_pos);
		tmp_check = GetEleDbLinkList(head, tmp_pos);
		//printf("wait pkg len:%d\r\n",GetEleDbLinkList(head, tmp_pos));
		
		tmp_pos++;
		while(GetLengthDbLinkList(head) < tmp_pos);//{printf("cur buf len:%d\r\n",GetLengthDbLinkList(head));}
		tmp_pkg_len = tmp_pkg_len << 8;
		tmp_pkg_len += GetEleDbLinkList(head, tmp_pos);
		tmp_check += GetEleDbLinkList(head, tmp_pos);
		//printf("wait pkg len:%d\r\n",GetEleDbLinkList(head, tmp_pos));
		
		
		//rev len crc
		//printf("rev len crc : %x\r\n",tmp_check);
		unsigned char len_crc_check;
		tmp_pos++;
		while(GetLengthDbLinkList(head) < tmp_pos);//{printf("cur buf len:%d\r\n",GetLengthDbLinkList(head));}
		len_crc_check = GetEleDbLinkList(head, tmp_pos);
		
		//check len crc
		//printf("check crc check:%x,tmp_check:%x\r\n",len_crc_check,tmp_check);
		if(len_crc_check != tmp_check){
			//len crc err
			//printf("len crc err check:%x,tmp_check:%x\r\n",len_crc_check,tmp_check);
			tmp_pos = 1;
			continue;
		}
		
		tmp_pkg_len_i = tmp_pkg_len;
		tmp_Dat_pos = tmp_pos + 1;
		//rev data
		//printf("rev data tmp_pkg_len_i:%d\r\n",tmp_pkg_len_i);
		while(tmp_pkg_len_i--){
			tmp_pos++;
			while(GetLengthDbLinkList(head) < tmp_pos);
			tmp_check += GetEleDbLinkList(head, tmp_pos);
			//printf("rev data[%d]=%c\r\n",tmp_pos,GetEleDbLinkList(head, tmp_pos));
		}
		//rev crc
		//printf("rev crc : %x\r\n",tmp_check);
		unsigned char check;
		tmp_pos++;
		while(GetLengthDbLinkList(head) < tmp_pos);
		check = GetEleDbLinkList(head, tmp_pos);
		
		tmp_pos++;
		while(GetLengthDbLinkList(head) < tmp_pos);
		if(GetEleDbLinkList(head, tmp_pos) != '>'){
			//pkg end err
			//printf("pkg end err:%c,%x\r\n",GetEleDbLinkList(head, tmp_pos),GetEleDbLinkList(head, tmp_pos));
			tmp_pos = 1;
			continue;
		}
		
		//check crc
		//printf("check crc check:%d,tmp_check:%d\r\n",check,tmp_check);
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
			//printf("data:%s\r\n",json_buf);
			Print("data:");Print((char *)json_buf);Print("\r\n");
			analyze_json(json_buf);
			//continue;
		}else{
			//crc err
			//printf("crc err check:%d,tmp_check:%d\r\n",check,tmp_check);
			tmp_pos = 1;
			continue;
		}
/* 		unsigned int tmp_i;
		for(tmp_i = 0;tmp_i<100;tmp_i++){
			//printf("%d isuse: %d len:%d\r\n",tmp_i,UART_REVBUF[tmp_i].isuse,tmp_i,NodeLen);
		} */
		
	}
	
}

int main() {
	
	wdt_t obj;
    
	rda_wdt_init(&obj, 2);
    rda_wdt_start(&obj);
	rda_thread_new(NULL, feed_dog_thread, (void *)&obj, 1024,osPriorityNormal);
	serial_init(&stdio_uart, UART0_TX, UART0_RX);
	
    serial_baud(&stdio_uart,115200);

 	wifi_main_msgQ = rda_msgQ_create(5);
	wifi.set_msg_queue(wifi_main_msgQ);
	printf("************************************************          start        ************************************************\r\n");
	Print("start");
/* 	int test_wifi_ret = wifi.connect("xiaohui", "qwertyui", NULL, NSAPI_SECURITY_NONE);//"ChaoMeng_03F", "cm88889999"
    if (test_wifi_ret==0) {
        printf("connect to success, ip %s\r\n", wifi.get_ip_address());
    } else {
        printf("connect to fail\r\n");
    } */
	
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
	
	//printf("************************************************          end        ************************************************\r\n");
	unsigned int msg;
    int ret, cnt = 1;
    while (true) {
        rda_msg_get(wifi_main_msgQ, &msg, osWaitForever);
        switch(msg)
        {
            case MAIN_RECONNECT:
                //printf("wifi disconnect!\r\n");
                ret = wifi.disconnect();
                if(ret != 0){
                    //printf("disconnect failed!\r\n");
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
            case MAIN_STOP_AP:
                wifi.stop_ap(1);
                break;
            default:
                //printf("unknown msg\r\n");
                break;
        }
    }
}