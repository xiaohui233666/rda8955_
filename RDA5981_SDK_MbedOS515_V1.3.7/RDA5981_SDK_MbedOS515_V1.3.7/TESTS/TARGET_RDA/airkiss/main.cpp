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

extern void test_wifi(void);

#if defined(MBED_RTOS_SINGLE_THREAD)
  #error [NOT_SUPPORTED] test not supported
#endif

/*
typedef struct {
    uint32_t find_node_state;   // A find_node_state value
} message_t;
*/

typedef struct {
    unsigned char c;
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

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include <stdio.h>


#include <ctype.h>
#define BUFFER_CHUNK_SIZE 10240
#define HTTP_PORT         80
#define HTTP_1_1_STR      "HTTP/1.1"

//#define poxis

#ifdef poxis
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <ctype.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#endif

#define HTTP_REQ_GET      0
#define HTTP_REQ_HEAD     1
#define HTTP_REQ_POST     2
#define HTTP_REQ_PUT      3
#define HTTP_REQ_DELETE   4
#define HTTP_REQ_TRACE    5
#define HTTP_REQ_OPTIONS  6
#define HTTP_REQ_CONNECT  7
#define HTTP_REQ_PATCH    8


typedef int socket_t;
typedef uint32_t http_req_t;

typedef enum
{
  HTTP_SUCCESS = 0,
  HTTP_EMPTY_BODY,
  HTTP_ERR_OPENING_SOCKET,
  HTTP_ERR_DISSECT_ADDR,
  HTTP_ERR_NO_SUCH_HOST,
  HTTP_ERR_CONNECTING,
  HTTP_ERR_WRITING,
  HTTP_ERR_READING,
  HTTP_ERR_OUT_OF_MEM,
  HTTP_ERR_BAD_HEADER,
  HTTP_ERR_TOO_MANY_REDIRECTS,
  HTTP_ERR_IS_HTTPS
} http_ret_t;


typedef struct 
{
  char* content_type;
  char* encoding;
  uint16_t status_code;
  char* status_text;
  char* redirect_addr;
} http_header_t;

typedef struct
{
  http_header_t* p_header;
  uint32_t length;
  char* contents;
  http_ret_t status;
} http_response_t;

static const char* HTTP_REQUESTS[] =
{
  "GET",
  "HEAD",
  "POST",
  "PUT",
  "DELETE",
  "TRACE",
  "OPTIONS",
  "CONNECT",
  "PATCH"
};

static char* URL_encode(char* const addr)
{
  unsigned int special_chars_count = 0;
  static const char reserved_chars[] = "!*\'();:@&=+$,/=#[]-_.~?";

  /* do one round to find the size */
  for (char* c = &addr[0]; *c != '\0'; ++c)
  {
    if (!isalpha(*c) && !isdigit(*c) && strchr(reserved_chars, *c) == NULL)
      ++special_chars_count;
  }

  char* enc_addr = (char*) malloc(strlen(addr) + 2 * special_chars_count + 1);
  char* new_p = &enc_addr[0];
  for (char* old = &addr[0]; *old != '\0';)
  {
    if (!isalpha(*old) && !isdigit(*old) && strchr(reserved_chars, *old) == NULL)
    {
      new_p += sprintf(new_p, "%%%X", *old++);
    }      
    else
      *new_p++ = *old++;
  }

  *new_p = '\0';

  return enc_addr;
}

static int dissect_address(char* const address, char* host, const size_t max_host_length, char* resource, const size_t max_resource_length)
{
  char* encoded_addr = URL_encode(address);

  /* remove any protocol headers (f.e. "http://") before we search for first '/' */
  char* start_pos = strstr(encoded_addr, "://");
  start_pos = ((start_pos == NULL) ? (char*) encoded_addr : start_pos + 3);

  char* slash_pos = strchr(start_pos, '/');

  /* no resource, send request to index.html*/
  if (slash_pos == NULL || slash_pos[1] == '\0')
  {
    strcpy(host, start_pos);

    /* remove slash from host if any */
    if (slash_pos != NULL)
      strchr(host, '/')[0] = '\0';
    
    //strcpy(resource, "/index.html");
    strcpy(resource, "/");
    free(encoded_addr);
    return 1;
  }

  char* addr_end = strchr(slash_pos, '\0');

  if (start_pos >= slash_pos - 1)
  {
    free(encoded_addr);
    return 0;
  }

  if (slash_pos - start_pos > max_host_length)
  {
    free(encoded_addr);
    return 0;
  }

  if (addr_end - slash_pos > max_resource_length)
  {
    free(encoded_addr);
    return 0;
  }

  strcpy(host, start_pos);
  host[slash_pos - start_pos] = '\0';
  strcpy(resource, slash_pos);
  resource[addr_end - slash_pos] = '\0';

  free(encoded_addr);
  return 1;
}

static int build_http_request(const char* host, const char* resource, const http_req_t http_req, char* request, 
    const size_t max_req_size, char** header_lines, const size_t header_line_count, char* const body)
{
  sprintf(request, "%s %s %s\r\nHost: %s\r\nReferer: http://%s%s\r\n", 
      HTTP_REQUESTS[http_req],
      resource,
      HTTP_1_1_STR,
      host,
      host,
      resource);
  for (unsigned int i = 0; i < header_line_count; ++i)
  {
    sprintf(request, "%s%s\r\n", request, header_lines[i]);
    //printf(header_lines[i]);
  }
  strcat(request, "\r\n");
  if (body != NULL)
    strcat(request, body);

  printf("request:\r\n%s",request);

  return 1;
}

static char* _http_request(char* address, http_req_t http_req, http_ret_t* p_ret, 
    uint32_t* resp_len, char** header_lines, size_t header_line_count, char* body)
{
  int portno = HTTP_PORT;

  if (strstr(address, "https://") != NULL)
  {
    *p_ret = HTTP_ERR_IS_HTTPS;
    return NULL;
  }

#ifndef poxis
  ip_addr_t server;
#else
  struct sockaddr_in serverAddr;
#endif

  char host_addr[256];
  char resource_addr[256];

  if (!dissect_address(address, host_addr, 256, resource_addr, 256))
  {
    *p_ret = HTTP_ERR_DISSECT_ADDR;
    return NULL;
  }

  #ifndef poxis
  /* do DNS lookup */
  int ret = netconn_gethostbyname(host_addr, &server);
  if(ret != 0){
    *p_ret = HTTP_ERR_NO_SUCH_HOST;
    return NULL;
  }
  #else
	  struct hostent *hp = gethostbyname(host_addr);
  #endif
  /* open socket to host */
  //socket_t sock = socket_open(server, portno);
  
  
  #ifndef poxis
  TCPSocket mesh_tcp_client;
  mesh_tcp_client.open(&wifi);
  if(mesh_tcp_client.connect(inet_ntoa(server),portno) != 0)
  {
    *p_ret = HTTP_ERR_OPENING_SOCKET;
    return NULL;
  }
  #else
	int nFd = socket(AF_INET,SOCK_STREAM,0);
	if (-1 == nFd)
	{
		printf("socket:");
		return -1;
	}

	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = inet_addr(inet_ntoa(*(struct in_addr*)hp->h_addr_list[0]));
	serverAddr.sin_port = htons(80);
    int nRet = connect(nFd,(struct sockaddr*)&serverAddr,sizeof(serverAddr));
    if (nRet == -1)
    {
        printf("connect:");
        return -1;
    }
  #endif


  /* Default timeout is too long */
  //no support//socket_set_timeout(sock, 1, 0);

  size_t http_req_size = 256 + strlen(address);
  for (size_t i = 0; i < header_line_count; ++i)
  {
    http_req_size += strlen(header_lines[i]);
  }

  if (body != NULL)
    http_req_size += strlen(body);

  char* http_req_str = (char*) malloc(http_req_size); 

  build_http_request(host_addr, resource_addr, http_req, http_req_str, http_req_size, header_lines, header_line_count, body);

  /* send http request */
//  int len = write(sock, http_req_str, strlen(http_req_str));
#ifndef poxis
  int len = mesh_tcp_client.send(http_req_str,strlen(http_req_str));
  free(http_req_str);

  if (len < 0)
  {
    *p_ret = HTTP_ERR_WRITING;
    return NULL;
  }
#else
	int len = write(nFd,http_req_str,strlen(http_req_str));
	free(http_req_str);

	if (len < 0)
	{
	*p_ret = HTTP_ERR_WRITING;
	return NULL;
	}
#endif
  uint32_t buffer_len = BUFFER_CHUNK_SIZE;
  char* resp_str = (char*) malloc(buffer_len);
  memset(resp_str, 0, buffer_len);
  int32_t tot_len = 0;
  uint32_t cycles = 0;
  
  /* Read response from host */
  do
  {
    //len = recv(sock, &resp_str[tot_len], buffer_len - tot_len, 0);
	#ifndef poxis
	len = mesh_tcp_client.recv(&resp_str[tot_len], buffer_len - tot_len);
	#else
	len = read(nFd,&resp_str[tot_len], buffer_len - tot_len);
	#endif
    if (len <= 0 && cycles >= 0)
      break;

    tot_len += len;

    if (tot_len >= buffer_len)
    {
      buffer_len += BUFFER_CHUNK_SIZE;
      char* newbuf = (char*) realloc(resp_str, buffer_len);

      if (newbuf == NULL)
      {
        *p_ret = HTTP_ERR_OUT_OF_MEM;
        free(resp_str);
        return NULL;
      }

      resp_str = newbuf;
      memset(&resp_str[buffer_len - BUFFER_CHUNK_SIZE], 0, BUFFER_CHUNK_SIZE);
    }
    ++cycles;

  } while (1);

  if (tot_len <= 0)
  {
    *p_ret = HTTP_ERR_READING;
    free(resp_str);
    return NULL;
  }
  /* shave buffer */
  char* new_str = (char*) realloc(resp_str, tot_len);
  
  if (new_str == NULL)
  {
    *p_ret = HTTP_ERR_OUT_OF_MEM;
    free(resp_str);
    return NULL;
  }
  resp_str = new_str;
  *resp_len = (tot_len);
  
  //socket_close(sock);
  #ifndef poxis
  mesh_tcp_client.close();
  #else
  close(nFd);
  #endif
  return resp_str;

}

void free_header(http_header_t* p_header){
  if (p_header->content_type != NULL)
    free(p_header->content_type);
  if (p_header->encoding != NULL)
    free(p_header->encoding);
  if (p_header->status_text != NULL)
    free(p_header->status_text);
  if (p_header->redirect_addr != NULL)
    free(p_header->redirect_addr);

  free(p_header);
}

static void word_to_string(const char* const word, char** str)
{
  uint32_t i = 0;
  while (word[i] == ' ' || word[i] == '\r' || word[i] == '\n' || word[i] == '\0')
    ++i;

  uint32_t start = i;

  while (word[i] != ' ' && word[i] != '\r' && word[i] != '\n')
    ++i;
  
  *str = (char*) malloc(i - start + 1);
  (*str)[i - start] = '\0';
  memcpy(*str, &word[start], i - start);
}

void dissect_header(char* data, http_response_t* p_resp)
{
  if (strlen(data) == 0)
  {
    p_resp->p_header = NULL;
    return;
  }
  p_resp->p_header = (http_header_t*) malloc(sizeof(http_header_t));

  char* content = data;
  char* header_end = strstr(content, "\r\n\r\n");
  if (header_end == NULL)
  {
    header_end = content + strlen(content);
  }
  else
  {
    /* explicitly null terminate header, to satisfy strstr() */
    header_end[0] = '\0';
  }

  /* handle first line. FORMAT:[HTTP/1.1 <STATUS_CODE> <STATUS_TEXT>] */
  content = strchr(data, ' ');
  if (content == NULL)
    return;
  
  p_resp->p_header->status_code = atoi(content);
  content = strchr(content + 1, ' ') + 1;

  size_t status_text_len = strchr(content, '\r') - content;
  
  p_resp->p_header->status_text = (char*) malloc(status_text_len);
  memcpy(p_resp->p_header->status_text, content, status_text_len);

  p_resp->p_header->content_type = NULL;
  p_resp->p_header->encoding = NULL;
  p_resp->p_header->redirect_addr = NULL;

  while (content <= header_end)
  {
    if (content == NULL)
      return;
    content = strstr(content, "\r\n");
    if (content == NULL)
      return;
    content += 2;

    if (strstr(content, "Content-Type:") == content)
    {
      content = strchr(content, ' ');
      word_to_string(content, &p_resp->p_header->content_type);
    }
    else if (strstr(content, "Content-Encoding:") == content)
    {
      content = strchr(content, ' ');
      word_to_string(content, &p_resp->p_header->encoding);
    }
    else if (strstr(content, "Location:") == content || strstr(content, "location:") == content)
    {
      content = strchr(content, ' ');
      word_to_string(content, &p_resp->p_header->redirect_addr);
    }
  }
}


http_response_t* http_request_w_body(char* const address, const http_req_t http_req, char** header_lines, size_t header_line_count, char* const body)
{
  http_response_t* p_resp = (http_response_t*) malloc(sizeof(http_response_t));
  memset(p_resp, 0, sizeof(http_response_t));
  
  uint32_t tot_len, header_len, body_len;
  unsigned char *body_pos, *resp_str;

  char* address_copy = address;

  /* loop until a non-redirect page is found (up to 5 times, as per spec recommendation) */
  for (unsigned char redirects = 0; redirects < 8; ++redirects)
  {
    resp_str = (unsigned char*)_http_request(address_copy, http_req, &p_resp->status, &tot_len, header_lines, header_line_count, body);
    
    if (resp_str == NULL)
    {
      return p_resp;
    }

    /* header ends with an empty line */
    body_pos = (unsigned char *)(strstr((char *)resp_str, (char *)"\r\n\r\n"));
	body_pos += 4;
    header_len = (body_pos - resp_str);
    body_len = (tot_len - header_len);

    /* place data in response header */
    dissect_header((char*)resp_str, p_resp);

    if (p_resp->p_header == NULL)
    {
      p_resp->status = HTTP_ERR_BAD_HEADER;
      return p_resp;
    }
    
    if (p_resp->p_header->status_code >= 300 && p_resp->p_header->status_code < 400)
    {
      if (p_resp->p_header->redirect_addr == NULL)
      {
        p_resp->status = HTTP_ERR_BAD_HEADER;
        return p_resp;
      }

      if (address_copy != address)
        free(address_copy);

      address_copy = (char*) malloc(strlen(p_resp->p_header->redirect_addr));
      strcpy(address_copy, p_resp->p_header->redirect_addr);

      free_header(p_resp->p_header);
      p_resp->p_header = NULL;

      free(resp_str);
      resp_str = NULL;
    }
    else
    {
      break;
    }
  }

  if (p_resp->p_header == NULL)
  {
    p_resp->status = HTTP_ERR_TOO_MANY_REDIRECTS;
    return p_resp;
  }

 
  /* if contents are compressed, uncompress it before placing it in the struct */
  if (p_resp->p_header->encoding != NULL && strstr(p_resp->p_header->encoding, "gzip") != NULL)
  {
    printf("err:gzip not support\r\n");
    p_resp->status = HTTP_ERR_OUT_OF_MEM;
    return p_resp;
    /* content length is always stored in the 4 last bytes */
    //uint32_t content_len = 0;
    //memcpy(&content_len, resp_str + tot_len - 4, 4);

    /* safe guard against insane sizes */
    //content_len = ((content_len < 30 * body_len) ? content_len : 30 * body_len);

    //p_resp->contents = (char*) malloc(content_len);

    //if (p_resp->contents == NULL)
    //{
    //  p_resp->status = HTTP_ERR_OUT_OF_MEM;
    //  return p_resp;
    //}

    /* zlib decompression (gzip) */
    /*z_stream infstream;
    memset(&infstream, 0, sizeof(z_stream));
    infstream.avail_in = (uInt)(body_len);
    infstream.next_in = (Bytef *)body_pos; 
    infstream.avail_out = (uInt)content_len;
    infstream.next_out = (Bytef *)p_resp->contents;

    inflateInit2(&infstream, 16 + MAX_WBITS);
    inflate(&infstream, Z_NO_FLUSH);
    inflateEnd(&infstream);

    p_resp->length = content_len;
    */
  }
  else if (body_len > 0)
  {
    p_resp->contents = (char*) malloc(body_len + 1);
    p_resp->contents[body_len] = '\0';
    memcpy(p_resp->contents, body_pos, body_len);
    p_resp->length = body_len;
  }
  else
  {
    /* response without body */
    free(resp_str);
    p_resp->contents = NULL;
    p_resp->status = HTTP_EMPTY_BODY;
    return p_resp;
  }

  free(resp_str);
  return p_resp;
}

char* tmp_hdr[2][100]={"Content-Type:application/x-www-form-urlencoded","Content-Length:95"};

void http_post(void)
{
	http_response_t* p_resp = http_request_w_body("http://wy.cmfspay.com/hardware/devicestatus", HTTP_REQ_POST, (char**)&tmp_hdr, 2, "sign=38419a28bf6dbd073e59e28c6061a000&time=1552271732&openid=6000001&sv=0.0.1&sn=40011000000027");
	printf("\r\nRESPONSE FROM :\n%s\n", p_resp->contents);
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

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
//         if (evt.status == osEretventMessage) {
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
	
	printf("================================start http post test===================================\r\n");
	http_post();
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


