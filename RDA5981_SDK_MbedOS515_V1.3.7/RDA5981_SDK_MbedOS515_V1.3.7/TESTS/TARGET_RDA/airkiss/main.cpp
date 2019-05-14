#include "mbed.h"
#include "rtos.h"
#include "inet.h"
#include "WiFiStackInterface.h"
#include "rda5981_sniffer.h"
#include "airkiss.h"
#include "gpadckey.h"
//#include "utest.h"
//using namespace utest::v1;

WiFiStackInterface wifi;

char ssid[32];
char passwd[64];
uint8_t random;
Semaphore airkiss_done(0);

/* airkiss reset */
GpadcKey airkiss_reset_key(KEY_A5);
Timer airkiss_reset_timer;
unsigned int falltime;
void airkiss_reset_irq_fall()
{
    airkiss_reset_timer.start();
    falltime = airkiss_reset_timer.read_ms();
}
void airkiss_reset_irq_rise()
{
    unsigned int time_now = airkiss_reset_timer.read_ms();
    if((time_now - falltime) > 3000) {   //about 5s
        printf("clear ssid saved in flash\r\n");
        rda5981_flash_erase_sta_data();         //long press for recording
    }
    airkiss_reset_timer.stop();
}
void rda5991h_smartlink_irq_init()
{
    printf("%s\r\n", __func__);
    airkiss_reset_key.fall(&airkiss_reset_irq_fall);
    airkiss_reset_key.rise(&airkiss_reset_irq_rise);
}

/* airkiss data */
const airkiss_config_t akconf =
{
	(airkiss_memset_fn)&memset,
	(airkiss_memcpy_fn)&memcpy,
	(airkiss_memcmp_fn)&memcmp,
	(airkiss_printf_fn)&printf
};
airkiss_context_t akcontex;

uint8_t my_scan_channel[15];
int cur_channel = 0;
int to_ds = 1;
int from_ds = 0;
int mgm_frame = 0;
static void time_callback(void const *p)
{
    do {
        cur_channel++;
		printf("\r\ncur_channel:%d\r\n",cur_channel);
    	if (cur_channel > 13) {
    		cur_channel = 1;
            to_ds = to_ds?0:1;
            from_ds = from_ds?0:1;
    	}
    } while (my_scan_channel[cur_channel] == 0);
    rda5981_start_sniffer(cur_channel, to_ds, from_ds, mgm_frame, 0);
    airkiss_change_channel(&akcontex);
}

RtosTimer timer(time_callback, osTimerPeriodic, 0);

static int send_rsp_to_apk(uint8_t random)
{
    int i;
    int ret;
    int udp_broadcast = 1;
    UDPSocket udp_socket;
    ip_addr_t ip4_addr;
    char host_ip_addr[16];

    memset((u8_t *)(&ip4_addr), 0xff, 4);
    strcpy(host_ip_addr, inet_ntoa(ip4_addr));

    printf("send response to host:%s\n", host_ip_addr);

    ret = udp_socket.open(&wifi);
    if (ret) {
        printf("open socket error:%d\r\n", ret);
        return ret;
    }

    ret = udp_socket.setsockopt(0,NSAPI_UDP_BROADCAST,&udp_broadcast,sizeof(udp_broadcast));
    if (ret) {
        printf("setsockopt error:%d\r\n", ret);
        return ret;
    }

    for (i=0; i<30; ++i) {
        ret = udp_socket.sendto(host_ip_addr, 10000, &random, 1);
        if (ret <= 0)
            printf("send rsp fail%d\n", ret);
        wait_us(100 * 1000);
    }
    printf("send response to host done\n");
    return ret;
}

static void airkiss_finish(void)
{
	int err;
	airkiss_result_t result;
	err = airkiss_get_result(&akcontex, &result);
	if (err == 0)
	{
		printf("airkiss_get_result() ok!");
        printf("ssid = \"%s\", pwd = \"%s\", ssid_length = %d, \"pwd_length = %d, random = 0x%02x\r\n",
        result.ssid, result.pwd, result.ssid_length, result.pwd_length, result.random);
        rda5981_stop_sniffer();
        strcpy(ssid, result.ssid);
        strcpy(passwd, result.pwd);
        random = result.random;
        airkiss_done.release();
	}
	else
	{
		printf("airkiss_get_result() failed !\r\n");
	}
}

static void wifi_promiscuous_rx(uint8_t *buf, uint16_t len)
{
	int ret;
	printf("wifi_promiscuous_rx %d:%x\r\n",len,buf[0]);
	printf("\r\n");
	for(int tmp_i=0;tmp_i<len;tmp_i++){
		printf("%c",buf[tmp_i]);
//		if(tmp_i%16==0)
//		{
//			printf("\r\n");
//		}
	}
	printf("\r\n");
	ret = airkiss_recv(&akcontex, buf, len);
	printf("wifi_promiscuous_rx ret=%d\r\n",ret);
	if ( ret == AIRKISS_STATUS_CHANNEL_LOCKED){
		printf("ret == AIRKISS_STATUS_CHANNEL_LOCKED\r\n");
        timer.stop();
	}else if ( ret == AIRKISS_STATUS_COMPLETE )
	{
		printf("ret == AIRKISS_STATUS_COMPLETE\r\n");
		airkiss_finish();
	}
}

void start_airkiss(void)
{
    int ret;

    ret = airkiss_init(&akcontex, &akconf);
    if (ret < 0)
    {
        printf("Airkiss init failed!\r\n");
        return;
    }
}

int my_smartconfig_handler(unsigned short data_len, void *data)
{
    static unsigned short seq_num = 0, seq_tmp;

    char *frame = (char *)data;

    seq_tmp = (frame[22] | (frame[23]<<8))>>4;
    if (seq_tmp == seq_num)
        return 0;
    else
        seq_num = seq_tmp;
    wifi_promiscuous_rx((uint8_t*)data, data_len);
    return 0;
}

#define TCP_SERVER_PORT       80
#define BUF_LEN               1024

TCPSocket *client;
typedef struct{
	char * buffer;
	Thread *thread;
	SocketAddress client_addr;
	TCPSocket client;
	char isuse;
}mesh_tcp_socket_t;
#define mesh_tcp_size 10
mesh_tcp_socket_t mesh_tcp[mesh_tcp_size];
TCPServer server;
SocketAddress *client_addr;

Thread t2;Thread t3;

void tcp_deal_func(mesh_tcp_socket_t *mesh_tcp_sub);
void test_thread(void)
{
	unsigned int ti=0;
	while(1 || ti++ <= 10)
	{
		printf("\r\n!!!!!!!!!!!!!!!!!!!!!test-thread %d!!!!!!!!!!!!!!!\r\n",ti);
		printf("stack_size %d free_stack %d\r\n", t2.stack_size(),t2.free_stack());
		//mesh_tcp_sub->thread->yield();//
		osDelay(200);
	}
	
}
int main() {
    int tmp_i;
    int ret;
    int airkiss_send_rsp = 0;
    rda5981_scan_result scan_result[30];
	
	t2.start(test_thread);
	t3.start(test_thread);
	memset(my_scan_channel, 0, sizeof(my_scan_channel));
	for(unsigned int tmp_i=0;tmp_i<mesh_tcp_size;tmp_i++)
	{
		mesh_tcp[tmp_i].isuse=0;
	}
/*
	ret = wifi.start_ap("RRDD-8889", NULL ,11);
	rda5981_apsta_info *sta_list=(rda5981_apsta_info *)malloc(sizeof(rda5981_apsta_info)*100);
	unsigned int num;
	num=wifi.ap_join_info(sta_list,100);
	printf("get_ip_address:%s\r\n",wifi.get_ip_address());
	printf("get_ip_address_ap:%s ,get_dhcp_start_ap: %s\r\n",wifi.get_ip_address_ap(),wifi.get_dhcp_start_ap());
	printf("					joined sta num: %d\r\n",num);
*/
/*
    ret = wifi.scan(NULL, 0);//necessary
    ret = wifi.scan_result(scan_result, 30);
    printf("scan result:%d\n", ret);
    for(i=0; i<ret; ++i) {
        if (scan_result[i].channel<=13) {
            printf("i:%d, channel%d\n", i, scan_result[i].channel);
            my_scan_channel[scan_result[i].channel] = 1;
        }
    }

    //rda5991h_smartlink_irq_init();

    ret = rda5981_flash_read_sta_data(ssid, passwd);
    if (0 && (ret==0 && strlen(ssid))) {//get ssid from flash
        printf("get ssid from flash: ssid:%s, pass:%s\r\n",
            ssid, passwd);
    } else {//need to smartconfig
        printf("start airkiss, version:%s\n", airkiss_version());
        rda5981_enable_sniffer(my_smartconfig_handler);
		//airkiss_set_key(&akcontex, "123", 3);
        start_airkiss();

        timer.start(600);
		
        airkiss_done.wait();
        airkiss_send_rsp = 1;
        wait_us(500 * 1000);
    }
*/
    //ret = wifi.connect(ssid, passwd, NULL, NSAPI_SECURITY_NONE);
	ret = wifi.connect("Bee-WiFi(2.4G)", "Beephone", NULL, NSAPI_SECURITY_NONE);
    if (ret==0) {
        printf("connect to %s success, ip %s\r\n", ssid, wifi.get_ip_address());
    } else {
        printf("connect to %s fail\r\n", ssid);
    }
/*
    if (airkiss_send_rsp) {
        rda5981_flash_write_sta_data(ssid, passwd);
        send_rsp_to_apk(random);
		#ifdef LWIP_DEBUG
			printf("\r\n------------------LWIP_DEBUG-enable--------------\r\n");
		#endif
    }
*/
	server.open(&wifi);
    server.bind(wifi.get_ip_address(), TCP_SERVER_PORT);

    server.listen(30);
    printf("Server Listening\n");
	unsigned int i=0,tmp_find_state=0;
    while (true) {
        printf("\n^^^^^^^^^^^^^^^^^^^^^^^^^^^^^Wait for new connection...^^^^^^^^^^^^^^^^^^^^^^^^^^^^\r\n");
		
		//TCPSocket temp_client;
		//SocketAddress tmp_client_addr;
		i=0;tmp_find_state=0;
		printf("mesh_tcp addr %p\r\n",&mesh_tcp);
		
		for(;i<mesh_tcp_size;i++)
		{
			printf("mesh_tcp[%d].isuse %d mesh_tcp[%d].client %p\r\n",i,mesh_tcp[i].isuse,i,&(mesh_tcp[i].client));
			if(mesh_tcp[i].isuse==0)
			{
				mesh_tcp[i].isuse=1;
				client=&(mesh_tcp[i].client);
				client_addr = &(mesh_tcp[i].client_addr);
				printf("mesh_tcp[%d].client %p\r\n",i,&(mesh_tcp[i].client));
				mesh_tcp[tmp_i].buffer = (char *)malloc(BUF_LEN);
				//mesh_tcp[tmp_i].thread = new Thread(&mesh_tcp[tmp_i],tcp_deal_func, osPriorityNormal, 2048);
				mesh_tcp[tmp_i].thread = new Thread( osPriorityNormal, 512);
				tmp_find_state = 1;
				break;
			}
		}
		if(tmp_find_state == 0)
		{
			printf("server busy now ,please wait and retry!\r\n");
			osDelay(200);
		}else{
			//client = &temp_client;
			//client_addr = &tmp_client_addr;
			//TCPSocket *client;SocketAddress *client_addr;
			//client = (TCPSocket *)malloc(sizeof(TCPSocket));
			//client_addr = (SocketAddress *)malloc(sizeof(SocketAddress));
			server.accept(client, client_addr);
			printf("Connection from: %s:%d\r\n", (*client_addr).get_ip_address(), (*client_addr).get_port());
			printf("\r\n------------------------new thread--------------\r\n");
			//mesh_tcp[i].thread->join();
			//mesh_tcp[i].thread->start(callback(tcp_deal_func,&mesh_tcp[tmp_i]));
			mesh_tcp[i].thread->start(&mesh_tcp[tmp_i],tcp_deal_func);
			//tcp_deal_func(&mesh_tcp[tmp_i]);
			printf("\r\n**************************test*****************************\r\n");
			//while(true){
			//	osDelay(20000);
			//}
			//t1.start(tcp_deal_func);
			//tcp_deal_func();
			//tcp_deal_func((void *)client);
			//thread.start();
			//wait(5);
			//thread.join();
		}

        
    }
}

unsigned char send_content[] = "Hello World!\n";
void tcp_deal_func(mesh_tcp_socket_t *mesh_tcp_sub)
{
	printf("Enter tcp_deal_func\r\n");
	TCPSocket *cli = &(mesh_tcp_sub->client);
	SocketAddress *cli_addr = &(mesh_tcp_sub->client_addr);
	char *buffer=mesh_tcp_sub->buffer;
	printf("tcp_deal_func Connection from: %s:%d\r\n", (*cli_addr).get_ip_address(), (*cli_addr).get_port());
	
	//char buffer[BUF_LEN+1];
	printf("Enter tcp_deal_func-\r\n");
	while(true) {
		printf("rev\r\n");
		int n = (*cli).recv(buffer, BUF_LEN);
		printf("Recieved Data: %d\r\n%.*s\r\n", n, n, buffer);
		if(n <= 0)
			break;
		printf("stack_size %d free_stack %d\r\n", mesh_tcp_sub->thread->stack_size(),mesh_tcp_sub->thread->free_stack());
				
		//mesh_tcp_sub->thread->yield();
		//(*cli).send("test---------", sizeof("test---------"));
	}
	printf("close\r\n");
	(*cli).close();
	printf("delete thread\r\n");
	//delete &(mesh_tcp_sub->thread);
	printf("relase\r\n");
	printf("test mesh_tcp_sub->isuse %d\r\n",mesh_tcp_sub->isuse);
	*(&(mesh_tcp_sub->isuse))=0;
	//must free cli and cli addr before return
	//free(cli);
	printf("free buff\r\n");
	free(buffer);
	printf("tcp_deal_func end\r\n");
}


