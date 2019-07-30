#include <stdint.h>



#include <ctype.h>
#define BUFFER_CHUNK_SIZE 30
#define HTTP_PORT         80
#define HTTP_1_1_STR      "HTTP/1.1"
#define HTTP_HDR_MAXLEN 160



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

#else

	#include <stdio.h>
	#include "mbed.h"
	#include "rtos.h"

	#include "WiFiStackInterface.h"
	#include "rda_wdt_api.h"
	#include "rda_sleep_api.h"
	#include "Thread.h"
	#include "cmsis_os.h"
	#include "inet.h"
	#include "lwip/api.h"
	#include "rda_sys_wrapper.h"
	extern WiFiStackInterface wifi;
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

#include "http.h"

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
 printf("t-build\r\n");
  sprintf(request, "%s %s %s\r\nHost: %s\r\nReferer: http://%s%s\r\n", 
      HTTP_REQUESTS[http_req],
      resource,
      HTTP_1_1_STR,
      host,
      host,
      resource);
  printf("\r\ntest\r\n");
  for (unsigned int i = 0; i < header_line_count; ++i)
  {
    sprintf(request, "%s%s\r\n", request, (char *)(header_lines)+i*HTTP_HDR_MAXLEN);
    printf("hdr:%s\r\n",(char *)(header_lines)+i*HTTP_HDR_MAXLEN);
  }
  printf("\r\ntest2\r\n");
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
	TCPSocket http_tcp_client;
	http_tcp_client.open(&wifi);
	if(http_tcp_client.connect(inet_ntoa(server),portno) != 0)
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
    http_req_size += strlen((char *)(header_lines)+i*HTTP_HDR_MAXLEN);
    printf("hdr_l:%d\r\n",strlen((char *)(header_lines)+i*HTTP_HDR_MAXLEN));
  }

  if (body != NULL)
    http_req_size += strlen(body);

  char* http_req_str = (char*) malloc(http_req_size); 

  build_http_request(host_addr, resource_addr, http_req, http_req_str, http_req_size, header_lines, header_line_count, body);

  /* send http request */
//  int len = write(sock, http_req_str, strlen(http_req_str));
#ifndef poxis
	int len = http_tcp_client.send(http_req_str,strlen(http_req_str));
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
  int32_t tmp_pkg_len = 0;
  char rev_chunk_len_tmp_buff = 0;
  int tmp_i=0;
  /* Read response from host */
  
  char http_respon1[]="\r\n\r\n";
	do
	  {
		//len = recv(sock, &resp_str[tot_len], buffer_len - tot_len, 0);
		if(!(tmp_i<sizeof(http_respon1)-1)){
			printf("\r\nnew buf(%d): %s\r\n",tot_len,resp_str);
				for(int tmp_r_i = 0; tmp_r_i < tot_len ; tmp_r_i ++)
				{
					printf("%c",*(resp_str+tmp_r_i));
				}
			printf("\r\n");
			
			printf("header end\r\n");
			break;
		}
		
		
		#ifndef poxis
		len = http_tcp_client.recv(&resp_str[tot_len], 1);
		#else
		len = read(nFd,&resp_str[tot_len], 1);
		#endif
		if (len <= 0 && cycles >= 0){
			*p_ret = HTTP_ERR_READING;
			free(resp_str);
			return NULL;
		}
		

	printf("\r\nnew buf(%d): %s\r\n",len,resp_str+tot_len);
		for(int tmp_r_i = 0; tmp_r_i < len ; tmp_r_i ++)
		{
			printf("%c,",*(resp_str+tot_len+tmp_r_i));
		}
	printf("\r\n");

		printf("rev rev %c dest %c\n",resp_str[tot_len],http_respon1[tmp_i]);
		if(resp_str[tot_len]==http_respon1[tmp_i]){//note: here have bug when the arrays are zero and the conn closed now
			tmp_i++;
		}else{
			if(tmp_i>0){
				tmp_i--;
			}
		}

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
 
 
while(1){
		tmp_pkg_len = 0; 
		while(len > 0){
			#ifndef poxis
				len = http_tcp_client.recv(&rev_chunk_len_tmp_buff, 1 );
			#else
				len = read(nFd,&rev_chunk_len_tmp_buff, 1);
			#endif
			
			if(len == 0){
			  #ifndef poxis
				http_tcp_client.close();
			  #else
				close(nFd);
			  #endif
			  *p_ret = HTTP_ERR_READING;
			  return NULL;
			}
			printf("rev_chunk_len_tmp_buff:%x(%c)\r\n",rev_chunk_len_tmp_buff,rev_chunk_len_tmp_buff);
			// rev chunk len
			if(rev_chunk_len_tmp_buff!='\r'){
				tmp_pkg_len<<=4;
				if(rev_chunk_len_tmp_buff >0x29 && rev_chunk_len_tmp_buff <0x3b)
					rev_chunk_len_tmp_buff-=0x30;
				else if(rev_chunk_len_tmp_buff >64 && rev_chunk_len_tmp_buff <91)
					rev_chunk_len_tmp_buff-=55;
				else if(rev_chunk_len_tmp_buff >96 && rev_chunk_len_tmp_buff <123)
					rev_chunk_len_tmp_buff-=87;
				else
				{
					printf("\r\ntcp ERR-ERR-ERR-ERR-ERR-ERR-ERR-ERR-ERR-ERR-ERR-ERR-ERR-ERR-ERR-ERR-ERR-ERR-ERR-ERR-ERR-!\r\n");
					*p_ret = HTTP_ERR_READING;
					free(resp_str);
					return NULL;
				}
				tmp_pkg_len+=rev_chunk_len_tmp_buff;
			}else{
				break;
			}
		}
		printf("next chunk len:%d\r\n",tmp_pkg_len);
		if(tmp_pkg_len==0)
			break;
	  
	  // read \n
		#ifndef poxis
			len = http_tcp_client.recv(&rev_chunk_len_tmp_buff, 1 );
		#else
			len = read(nFd,&rev_chunk_len_tmp_buff, 1);
		#endif

		if(len == 0){
			#ifndef poxis
			http_tcp_client.close();
			#else
			close(nFd);
			#endif
			*p_ret = HTTP_ERR_READING;
			return NULL;
		}
	   //////////////////////
	  
	  do
	  {
		//len = recv(sock, &resp_str[tot_len], buffer_len - tot_len, 0);

		
		#ifndef poxis
		len = http_tcp_client.recv(&resp_str[tot_len], (buffer_len - tot_len) > tmp_pkg_len ? tmp_pkg_len:(buffer_len - tot_len));
		#else
		len = read(nFd,&resp_str[tot_len], (buffer_len - tot_len) > tmp_pkg_len ? tmp_pkg_len:(buffer_len - tot_len));
		#endif
		if (len <= 0 && cycles >= 0){
			*p_ret = HTTP_ERR_READING;
			free(resp_str);
			return NULL;
		}
		

	printf("\r\nnew buf(%d): %s\r\n",len,resp_str+tot_len);
		for(int tmp_r_i = 0; tmp_r_i < len ; tmp_r_i ++)
		{
			printf("%c",*(resp_str+tot_len+tmp_r_i));
		}
	printf("\r\n");

		tot_len += len;
		tmp_pkg_len -= len;
		if(tmp_pkg_len == 0){
			printf("rev chunk end\r\n");
			break;
		}
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
}

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
	http_tcp_client.close();
  #else
	close(nFd);
  #endif
  return resp_str;

}
void free_header(http_header_t* p_header)
{
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

static void word_to_string(const char* word, char** str)
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

static void dissect_header(char* data, http_response_t* p_resp)
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

char tmp_hdr[2][HTTP_HDR_MAXLEN];//={"Content-Type:application/x-www-form-urlencoded","Content-Length:95"};

void http_post(void)
{
	printf("hdr_size:%d\r\n",sizeof(tmp_hdr));
	memset(tmp_hdr,0,sizeof(tmp_hdr));
	printf("test1\r\n");
	strcpy(((char *)(tmp_hdr)),"Content-Type:application/x-www-form-urlencoded");
	printf("test2:%s\r\n",&tmp_hdr[0]);
	
	sprintf((char *)&(tmp_hdr[1]),"Content-Length:%d",strlen("sign=38419a28bf6dbd073e59e28c6061a000&time=1552271732&openid=6000001&sv=0.0.1&sn=40011000000027"));
	
	printf("test:%s\r\n",&tmp_hdr[1]);
	printf("test t:%s\r\n",&((char **)tmp_hdr)[0]);
	http_response_t* p_resp = http_request_w_body("http://wy.cmfspay.com/hardware/devicestatus", HTTP_REQ_POST, (char**)tmp_hdr, 2, "sign=38419a28bf6dbd073e59e28c6061a000&time=1552271732&openid=6000001&sv=0.0.1&sn=40011000000027");
	printf("\r\nRESPONSE FROM :\n%s\n", p_resp->contents);
}

#ifdef poxis
	int main(){
		http_post();
	}
#endif
