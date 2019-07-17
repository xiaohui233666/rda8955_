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
extern void test_wifi(void);

#if defined(MBED_RTOS_SINGLE_THREAD)
  #error [NOT_SUPPORTED] test not supported
#endif

typedef struct {
    uint32_t find_node_state;   /* A find_node_state value               */
} message_t;

#define QUEUE_SIZE       16
#define QUEUE_PUT_DELAY  100

MemoryPool<message_t, QUEUE_SIZE> mpool;
Queue<message_t, QUEUE_SIZE> queue;
#include "Queue.h"

struct tcp_pcb *client_pcb;

WiFiStackInterface wifi;

#define TCP_SERVER_PORT       80

#define mesh_tcp_size 3
#define BUF_LEN               100

rtos::Mutex mesh_socket_lock;
TCPSocket *client;
typedef struct{
	char tmp_id;
	char buffer[BUF_LEN+1];
	//Thread *thread;
	void * thread;
	SocketAddress client_addr;
	TCPSocket client;
	char isuse;
}mesh_tcp_socket_t;

mesh_tcp_socket_t mesh_tcp[mesh_tcp_size];
TCPServer *server;
SocketAddress *client_addr;



// test machine run status
Thread t2;Thread t3;
//test queue thread
Thread tqtw;Thread tqtr;

void tcp_deal_func(void *arg);
// void tcp_deal_func(void const *arg);
// void tcp_deal_func(mesh_tcp_socket_t *arg);
void test_thread(void)
{
	unsigned int ti=0;
	while(1 || ti++ <= 10)
	{
		ti++;
		printf("\r\n!!!!!!!!!!!!!!!!!!!!!test-thread %d!!!!!!!!!!!!!!!\r\n",ti);
		printf("stack_size %d free_stack %d\r\n", t2.stack_size(),t2.free_stack());
		//mesh_tcp_sub->thread->yield();//
		osDelay(3000);
		if(ti == 10)
			ti = 0;
	}
	
}
// void test_queue_read_thread(void)
// {
// 	unsigned int ti=0;
// 	while(1 || ti++ <= 10)
// 	{
// 		ti++;
// 		printf("\r\n##################test_read_queue_thread###################\r\n");
// 		printf("stack_size %d free_stack %d\r\n", t2.stack_size(),t2.free_stack());
// 		//mesh_tcp_sub->thread->yield();//
// 		osEvent evt = queue.get();
//         if (evt.status == osEventMessage) {
// 			message_t *message = (message_t*)evt.value.p;
// 			printf("\r\n##################test_read_queue_thread %d ###################\r\n",message->counter);
// 			mpool.free(message);
// 		}
// 		osDelay(3000);
// 		if(ti == 30)
// 			ti = 0;
// 	}
	
// }

void find_mesh_dev(void * arg){
	int ret;
	const char *SSID = NULL;
	rda5981_scan_result *bss_list = NULL;
    int scan_res;
	printf("Start wifi_scan...\r\n");
	//mbed_lwip_init(NULL);
	mesh_socket_lock.lock();
    scan_res = wifi.scan(NULL, 0);
	printf("Start wifi_scan end...\r\n");
    bss_list = (rda5981_scan_result *)malloc(scan_res * sizeof(rda5981_scan_result));
    memset(bss_list, 0, scan_res * sizeof(rda5981_scan_result));
    if (bss_list == NULL) {
        printf("malloc buf fail\r\n");
        return ;
    }
    ret = wifi.scan_result(bss_list, scan_res);//rda5981_get_scan_result(bss_list, scan_res);
	rda5981_scan_result *tmp_rda5981_scan_result=bss_list;
	for(int tmp_ret=ret;tmp_ret>0;tmp_ret--){
		printf("%d:%s\r\n",tmp_ret,(unsigned char *)((*(tmp_rda5981_scan_result++)).SSID));
	}
    printf("##########scan return :%d\n", ret);
    free(bss_list);
	mesh_socket_lock.unlock();
	//Thread::wait(500);
}

// void test_queue_write_thread(void)
// {
// 	unsigned int ti=0;
// 	while(1 || ti++ <= 10)
// 	{
// 		ti++;
// 		printf("\r\n##################test_write_queue_thread %d###################\r\n",ti);
// 		printf("stack_size %d free_stack %d\r\n", t2.stack_size(),t2.free_stack());
// 		//mesh_tcp_sub->thread->yield();//
// 		message_t *message = mpool.alloc();
//         message->counter = ti;
// 		queue.put(message);
//         Thread::wait(QUEUE_PUT_DELAY);
// 		osDelay(1000);
// 		if(ti == 30)
// 			ti = 0;
// 	}
	
// }

TCPSocket mesh_tcp_client;
void test_mesh_tcp_client(void * arg){
	//connect last node,find the online node
	//while(true){
		find_mesh_dev(0);
		mesh_socket_lock.lock();
		if(server != NULL){
			delete server;
		}
		server = new TCPServer();
		(*server).open(&wifi);
		printf("wifi.get_ip_address() %s\r\n", wifi.get_ip_address());
		(*server).bind(wifi.get_ip_address(), TCP_SERVER_PORT);
printf("(*server).bind(wifi.get_ip_address(), TCP_SERVER_PORT)\r\n");
		(*server).listen(30);
printf("(*server).listen(30)\r\n");
		mesh_socket_lock.unlock();
		int ret = wifi.connect("Bee-WiFi(2.4G)", "Beephone", NULL, NSAPI_SECURITY_NONE);
		if (ret==0) {
			// message_t *message = mpool.alloc();
			// message->counter = 1;
			// queue.put(message);
			// Thread::wait(QUEUE_PUT_DELAY);
			printf("connect last node,find the online node [%s]\r\n",wifi.get_ip_address());
			printf("connect success, ip %s\r\n", wifi.get_ip_address());
		} else {
			printf("connect fail\r\n");
		}
		mesh_tcp_client.open(&wifi);
		if(mesh_tcp_client.connect("192.168.60.225",233) == 0){
			
			printf("[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[connect server success]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]\r\n");
			osDelay(100);
			while(true){

			}
			mesh_tcp_client.close();
		}else{
			printf("[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[connect server faile]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]\r\n");
		}
		wifi.disconnect();
		osDelay(1000);//while(true){};
	//}
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

int main() {
    int tmp_i;
    int ret;
	
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
	
	test_mesh_tcp_client(0);//find_mesh_dev(0);
	//rda_thread_new(NULL, test_mesh_tcp_client, (void *)0, 2048,osPriorityLow);
	//osDelay(1000);
	t2.start(test_thread);
	t3.start(test_thread);
	
	// tqtw.start(test_queue_write_thread);
	// tqtr.start(test_queue_read_thread);

	for(unsigned int tmp_i=0;tmp_i<mesh_tcp_size;tmp_i++)
	{
		mesh_tcp[tmp_i].isuse=0;
	}
    //test_wifi();
	mesh_socket_lock.lock();
	while(server == NULL){
		osDelay(1000);
	}
	mesh_socket_lock.unlock();
    printf("Server Listening\n");
	unsigned int i=0,tmp_find_state=0;
    while (true) {
        printf("\n^^^^^^^^^^^^^^^^^^^^^^^^^^^^^Wait for new connection...^^^^^^^^^^^^^^^^^^^^^^^^^^^^\r\n");
		
		//TCPSocket temp_client;
		//SocketAddress tmp_client_addr;
		i=0;tmp_find_state=0;
		printf("mesh_tcp addr %p\r\n",&mesh_tcp);
		
		for(i=0;i<mesh_tcp_size;i++)
		{
			printf("mesh_tcp[%d].isuse %d mesh_tcp[%d].client %p\r\n",i,mesh_tcp[i].isuse,i,&(mesh_tcp[i].client));
			if(mesh_tcp[i].isuse==0)
			{
				mesh_tcp[i].isuse=1;
				mesh_tcp[i].tmp_id = i;
				client=&(mesh_tcp[i].client);
				client_addr = &(mesh_tcp[i].client_addr);
				printf("mesh_tcp[%d].client %p\r\n",i,&(mesh_tcp[i].client));
				//mesh_tcp[i].buffer = (char *)malloc(BUF_LEN);
				//mesh_tcp[i].thread = new Thread((&mesh_tcp[i],tcp_deal_func, osPriorityNormal, 2048);
				//mesh_tcp[i].thread = new Thread(osPriorityNormal, 512);
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
			mesh_socket_lock.lock();
			(*server).accept(client, client_addr);
			mesh_socket_lock.unlock();
			printf("Connection from: %s:%d\r\n", (*client_addr).get_ip_address(), (*client_addr).get_port());
			printf("\r\n------------------------new thread--------------\r\n");
			//mesh_tcp[i].thread->join();
			//mesh_tcp[i].thread->start();
			//mesh_tcp[i].thread->start(callback(tcp_deal_func,&mesh_tcp[i]));

			// unsigned char send_content[] = "Hello World!\n";
			// while(true) {
			// 	printf("\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\ \r\n");
			// 	int n = (*client).recv((void *)mesh_tcp[i].buffer, BUF_LEN);
			// 	if(n <= 0)
			// 		break;
			// 	printf("Recieved Data: %d\r\n%.*s\r\n", n, mesh_tcp[i].buffer);
			// 	//(*client).send(send_content, sizeof(send_content) - 1);
			// }
			// (*client).close();

			//mesh_tcp[i].thread->start(&mesh_tcp[i],tcp_deal_func);

			//(*client).send("Hello World!\n", sizeof("Hello World!\n") - 1);
			mesh_tcp[i].thread = rda_thread_new(NULL, tcp_deal_func, (void *)&mesh_tcp[i], 2048,osPriorityNormal);

			//void *stack=malloc(2048);
			//rt_tsk_create((FUNCP)(tcp_deal_func), (U32)2048, (void *)stack, (void *)&mesh_tcp[tmp_i]);

			//static osThreadDef(tcp_deal_func, osPriorityNormal, 4*1024);
			//printf("tmp_i %d\r\n",i);
    		//osThreadCreate(osThread(tcp_deal_func), (void *)&mesh_tcp[i]);

			//tcp_deal_func(&mesh_tcp[i]);
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
//delete &server can close all the client socket,try delete the server and renew the server obj , blind the port again , listen the port again ,and let the node connect.
unsigned char send_content[] = "Hello World!\n";
// void tcp_deal_func(mesh_tcp_socket_t *arg) 
void tcp_deal_func(void *arg)
//void tcp_deal_func(void const *arg)
{
	mesh_tcp_socket_t *mesh_tcp_sub = (mesh_tcp_socket_t *)arg;
	printf("Enter tcp_deal_func\r\n");
	
	//TCPSocket *cli = &(mesh_tcp_sub->client);
	//SocketAddress *cli_addr = &(mesh_tcp_sub->client_addr);
	//char *buffer=mesh_tcp_sub->buffer;
	printf("tcp_deal_func Connection from: %s:%d\r\n", (*(&(mesh_tcp_sub->client_addr))).get_ip_address(), (*(&(mesh_tcp_sub->client_addr))).get_port());
	
	//char buffer[BUF_LEN+1];
	printf("Enter tcp_deal_func-\r\n");
	while(true) {
		mesh_socket_lock.lock();
		printf("client %d \r\n",mesh_tcp_sub->tmp_id);
		printf("mesh_tcp_sub->thread %p %d\r\n",mesh_tcp_sub->thread,mesh_tcp_sub->thread);
		//printf("rev mesh_tcp_sub->client %p\r\n",&(mesh_tcp_sub->client));
		int n = (*(&(mesh_tcp_sub->client))).recv((void *)mesh_tcp_sub->buffer, BUF_LEN);
		printf("client %d Recieved Data: %d\r\n%.*s\r\n",mesh_tcp_sub->tmp_id, n, n, mesh_tcp_sub->buffer);
		//printf("Recieved Data: %d\r\n", n);
		(*(&(mesh_tcp_sub->client))).send("Hello World!\n", sizeof("Hello World!\n") - 1);
		if(n <= 0){
			printf("client %d close\r\n");
			//free(mesh_tcp_sub->buffer);
			(*(&(mesh_tcp_sub->client))).close();
			printf("client %d close end\r\n");
			mesh_tcp_sub->isuse=0;
			printf("client %d mesh_tcp_sub->isuse=0;\r\n");
			return;//break;
		}
		mesh_socket_lock.unlock();
		//printf("mesh_tcp_sub->client %d stack_size %d free_stack %d\r\n",mesh_tcp_sub->tmp_id, mesh_tcp_sub->thread->stack_size(),mesh_tcp_sub->thread->free_stack());
		//osDelay(200);
		//Thread::wait(2000);
		//Thread::yield();
		//(*cli).send("test---------", sizeof("test---------"));
	}
	printf("close\r\n");
	//(*(&(mesh_tcp_sub->client))).close();
	printf("delete thread\r\n");
	//delete &(mesh_tcp_sub->thread);
	printf("relase\r\n");

	//must free cli and cli addr before return
	//free(cli);
	printf("free buff\r\n");
	free(mesh_tcp_sub->buffer);

	printf("test mesh_tcp_sub->isuse %d\r\n",mesh_tcp_sub->isuse);
	
	printf("test mesh_tcp_sub->isuse end %d\r\n",mesh_tcp_sub->isuse);

	printf("tcp_deal_func end\r\n");
}


