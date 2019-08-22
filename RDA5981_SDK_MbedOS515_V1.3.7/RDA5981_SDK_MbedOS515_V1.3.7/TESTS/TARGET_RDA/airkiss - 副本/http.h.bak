#ifndef HTTP__h
#define HTTP__h

typedef int socket_t;
typedef uint32_t http_req_t;
#ifndef poxis
typedef uint32_t size_t;
#endif

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

#define HTTP_REQ_GET      0
#define HTTP_REQ_HEAD     1
#define HTTP_REQ_POST     2
#define HTTP_REQ_PUT      3
#define HTTP_REQ_DELETE   4
#define HTTP_REQ_TRACE    5
#define HTTP_REQ_OPTIONS  6
#define HTTP_REQ_CONNECT  7
#define HTTP_REQ_PATCH    8

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

#ifndef poxis
char* URL_encode(char* const addr);
static int dissect_address(char* const address, char* host, const size_t max_host_length, char* resource, const size_t max_resource_length);
static int build_http_request(const char* host, const char* resource, const http_req_t http_req, char* request,const size_t max_req_size, char** header_lines, const size_t header_line_count, char* const body);
static char* _http_request(char* address, http_req_t http_req, http_ret_t* p_ret,uint32_t* resp_len, char** header_lines, size_t header_line_count, char* body);
void free_header(http_header_t* p_header);
static void word_to_string(const char* word, char** str);
static void dissect_header(char* data, http_response_t* p_resp);
http_response_t* http_request_w_body(char* const address, const http_req_t http_req, char** header_lines, size_t header_line_count, char* const body);
void http_post(void);
#endif

#endif
