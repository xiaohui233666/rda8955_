#include "mbed.h"
#include "rtos.h"
#include "inet.h"
#include "WiFiStackInterface.h"
#include "greentea-client/test_env.h"
#include "cmsis_os.h"

extern void test_wifi(void);

#if defined(MBED_RTOS_SINGLE_THREAD)
  #error [NOT_SUPPORTED] test not supported
#endif

typedef struct {
    uint32_t counter;   /* A counter value               */
} message_t;

#define QUEUE_SIZE       16
#define QUEUE_PUT_DELAY  100

MemoryPool<message_t, QUEUE_SIZE> mpool;
Queue<message_t, QUEUE_SIZE> queue;
#include "Queue.h"

WiFiStackInterface wifi;

#define TCP_SERVER_PORT       80
#define BUF_LEN               1024

TCPSocket *client;
typedef struct{
	char tmp_id;
	char * buffer;
	Thread *thread;
	SocketAddress client_addr;
	TCPSocket client;
	char isuse;
}mesh_tcp_socket_t;
#define mesh_tcp_size 30
mesh_tcp_socket_t mesh_tcp[mesh_tcp_size];
TCPServer server;
SocketAddress *client_addr;



// Queue test_queue();
Thread t2;Thread t3;Thread tqtw;Thread tqtr;

//void tcp_deal_func(void const *arg);
void tcp_deal_func(mesh_tcp_socket_t *arg);
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
void test_queue_read_thread(void)
{
	unsigned int ti=0;
	while(1 || ti++ <= 10)
	{
		ti++;
		printf("\r\n##################test_read_queue_thread###################\r\n");
		printf("stack_size %d free_stack %d\r\n", t2.stack_size(),t2.free_stack());
		//mesh_tcp_sub->thread->yield();//
		osEvent evt = queue.get();
        if (evt.status == osEventMessage) {
			message_t *message = (message_t*)evt.value.p;
			printf("\r\n##################test_read_queue_thread %d ###################\r\n",message->counter);
			mpool.free(message);
		}
		osDelay(3000);
		if(ti == 30)
			ti = 0;
	}
	
}
void test_queue_write_thread(void)
{
	unsigned int ti=0;
	while(1 || ti++ <= 10)
	{
		ti++;
		printf("\r\n##################test_write_queue_thread %d###################\r\n",ti);
		printf("stack_size %d free_stack %d\r\n", t2.stack_size(),t2.free_stack());
		//mesh_tcp_sub->thread->yield();//
		message_t *message = mpool.alloc();
        message->counter = ti;
		queue.put(message);
        Thread::wait(QUEUE_PUT_DELAY);
		osDelay(1000);
		if(ti == 30)
			ti = 0;
	}
	
}
int main() {
    int tmp_i;
    int ret;
    rda5981_scan_result scan_result[30];
	
	// t2.start(test_thread);
	// t3.start(test_thread);
	

	// tqtw.start(test_queue_write_thread);
	// tqtr.start(test_queue_read_thread);

	for(unsigned int tmp_i=0;tmp_i<mesh_tcp_size;tmp_i++)
	{
		mesh_tcp[tmp_i].isuse=0;
	}
    test_wifi();

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
				mesh_tcp[i].buffer = (char *)malloc(BUF_LEN);
				//mesh_tcp[i].thread = new Thread((&mesh_tcp[i],tcp_deal_func, osPriorityNormal, 2048);
				mesh_tcp[i].thread = new Thread(osPriorityNormal, 512);
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
			//mesh_tcp[i].thread->start();
			//mesh_tcp[i].thread->start(callback(tcp_deal_func,&mesh_tcp[i]));
			mesh_tcp[i].thread->start(&mesh_tcp[i],tcp_deal_func);
			void *stack=malloc(2048);
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

unsigned char send_content[] = "Hello World!\n";
void tcp_deal_func(mesh_tcp_socket_t *arg)
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
		printf("client %d",mesh_tcp_sub->tmp_id);
		//printf("rev mesh_tcp_sub->client %p\r\n",&(mesh_tcp_sub->client));
		int n = (*(&(mesh_tcp_sub->client))).recv(mesh_tcp_sub->buffer, BUF_LEN);
		printf("Recieved Data: %d\r\n%.*s\r\n", n, n, mesh_tcp_sub->buffer);
		if(n <= 0)
			break;
		//printf("mesh_tcp_sub->client %p stack_size %d free_stack %d\r\n",mesh_tcp_sub->client, mesh_tcp_sub->thread->stack_size(),mesh_tcp_sub->thread->free_stack());
		//osDelay(200);
		Thread::wait(200);
		//mesh_tcp_sub->thread->yield();
		//(*cli).send("test---------", sizeof("test---------"));
	}
	printf("close\r\n");
	(*(&(mesh_tcp_sub->client))).close();
	printf("delete thread\r\n");
	//delete &(mesh_tcp_sub->thread);
	printf("relase\r\n");

	//must free cli and cli addr before return
	//free(cli);
	printf("free buff\r\n");
	free(mesh_tcp_sub->buffer);

	printf("test mesh_tcp_sub->isuse %d\r\n",mesh_tcp_sub->isuse);
	mesh_tcp_sub->isuse=0;
	printf("test mesh_tcp_sub->isuse end %d\r\n",mesh_tcp_sub->isuse);

	printf("tcp_deal_func end\r\n");
}


