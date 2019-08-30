#include <stdint.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include "at.h"
#include "inet.h"
#include "WiFiStackInterface.h"
#include "rda_sys_wrapper.h"
#include "rda5991h_wland.h"
#include "lwip_stack.h"
#include "console_cfunc.h"

#define SOCKET_NUM 4
#define SEND_LIMIT  1460
#define RECV_LIMIT  1460
#define IP_LENGTH 16

#define MAC_FORMAT                          "%02x:%02x:%02x:%02x:%02x:%02x"
#define MAC2STR(ea)                  (ea)[0], (ea)[1], (ea)[2], (ea)[3], (ea)[4], (ea)[5]

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
#define RDA_AT_PRINT
#define AT_PRINT
#define AT_RESP_OK
#define AT_RESP_OK_EQU
#define AT_RESP_ERROR
extern UDPSocket udpsocket;
extern void response(char *cmd,char *dat,char *err);

static char trans_index = 0xff;
static char ssid[32+1];
static char pw[64+1];
static char conn_flag = 0;
static char ap_flag = 0;
extern unsigned char rda_mac_addr[6];
extern WiFiStackInterface wifi;

typedef enum{
    TCP,
}socket_type_t;
typedef struct{
    void *socket;
    int type;
    char SERVER_ADDR[20];
    int SERVER_PORT;
    int LOCAL_PORT; //used for UDP
    int used;
    SocketAddress address;
    void *threadid;
}rda_socket_t;
static rda_socket_t rda_socket[SOCKET_NUM];

int do_wsmac( cmd_tbl_t *cmd, int argc, char *argv[])
{
    char *mdata, mac[6], i;
	char TmpStr[18];
    if(*argv[1] == '?'){
        rda_get_macaddr((unsigned char *)mac, 0);
        sprintf(TmpStr, "%02x:%02x:%02x:%02x:%02x:%02x",
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
		response("AT+WSMAC",TmpStr,"");
        return 0;
    }

    if(conn_flag == 1 || ap_flag == 1){
		response("AT+WSMAC","","arg error");
        //AT_RESP_ERROR(idx, ERROR_ABORT);
        return 0;
    }

    if(strlen(argv[1]) != 12){
		response("AT+WSMAC","","arg error");
        //AT_RESP_ERROR(idx, ERROR_ARG);
        return 0;
    }

    mdata = argv[1];
    for(i = 0; i < 12; i++){
        if(mdata[i] >= 0x41 && mdata[i] <= 0x46)// switch 'A' to 'a'
            mdata[i] += 0x20;
        if(mdata[i] >= 0x61 && mdata[i] <= 0x66)//switch "ab" to 0xab
            mac[i] = mdata[i] - 0x57;
        if(mdata[i] >= 0x30 && mdata[i] <= 0x39)
            mac[i] = mdata[i] - 0x30;
            if(i%2 == 1)
                mac[i/2] = mac[i-1] << 4 | mac[i];
    }
    if(!mac_is_valid(mac)){
        //AT_RESP_ERROR(idx, ERROR_ARG);
		response("AT+WSMAC","","arg error");
        return 0;
    }
    memcpy(rda_mac_addr, mac, 6);
    rda5981_flash_write_mac_addr(rda_mac_addr);
	response("AT+WSMAC","ok","");
    //AT_RESP_OK(idx);

    return 0;
}

int conn(char *ssid, char *pw)
{
    int ret;
    const char *ip_addr;
    unsigned int enable, ip, msk, gw;
    char ips[IP_LENGTH], msks[IP_LENGTH], gws[IP_LENGTH];

/*     if(fixip_flag == 0){
        ret = rda5981_flash_read_dhcp_data(&enable, &ip, &msk, &gw);
        if(ret == 0 && enable == 1){
            memcpy(ips, inet_ntoa(ip), IP_LENGTH);
            memcpy(msks, inet_ntoa(msk), IP_LENGTH);
            memcpy(gws, inet_ntoa(gw), IP_LENGTH);
            wifi.set_dhcp(0);
            wifi.set_network(ips, msks, gws);
        }else{
            wifi.set_dhcp(1);
        }
    }
    fixip_flag = 0; */
    ret = wifi.connect(ssid, pw, NULL, NSAPI_SECURITY_NONE, 0);
	udpsocket.close();
	udpsocket.open(&wifi);
	udpsocket.bind(0);
    if(ret != 0){
		conn_flag = 0;
		//response("AT+WSCONN","connect error","");
        //RESP_ERROR(ERROR_FAILE);
        return -1;
    }

    ip_addr = wifi.get_ip_address();

    if (ip_addr) {
		//response("AT+WSCONN","connected","");
        //RDA_AT_PRINT("Client IP Address is %s\r\n", ip_addr);
        conn_flag = 1;
        return 0;
    } else {
		conn_flag = 0;
		//response("AT+WSCONN","disconnect","");
        //RDA_AT_PRINT("No Client IP Address\r\n");
        return -1;
    }
}

int do_wsconn( cmd_tbl_t *cmd, int argc, char *argv[])
{
    int ret, flag;

    if (argc < 1) {
		response("AT+WSCONN","","arg error");
        //AT_RESP_ERROR(idx, ERROR_ARG);
        return 0;
    }

    if(*argv[1] == '?'){
        if(conn_flag == 1){
			response("AT+WSCONN","connected","");
            //wifi.update_rssi();
            //AT_RESP_OK_EQU(idx, "ssid:%s, RSSI:%ddb, ip:%s, BSSID: "MAC_FORMAT"\r\n", wifi.get_ssid(), wifi.get_rssi(), wifi.get_ip_address(), MAC2STR(wifi.get_BSSID()));
        }else{
			response("AT+WSCONN","disconnect","");
            //AT_RESP_OK_EQU(idx, "ssid:, RSSI:0db, ip:0.0.0.0\r\n");
        }
        return 0;
    }

    if(conn_flag == 1){
		response("AT+WSCONN","","connected");
        // AT_RESP_ERROR(idx, ERROR_ABORT);
        //AT_PRINT(idx, "error! Has been connected!");
        return 0;
    }

    if(argc == 1)
        flag = 0;
    else
        flag = atoi(argv[1]);

    memset(ssid, 0, sizeof(ssid));
    memset(pw, 0, sizeof(pw));

    if(argc > 2)
        memcpy(ssid, argv[2], strlen(argv[2]));

    if(argc > 3)
        memcpy(pw, argv[3], strlen(argv[3]));

    if (strlen(ssid) == 0) {
        ret = rda5981_flash_read_sta_data(ssid, pw);
        if (ret == 0 && strlen(ssid)) {
            //AT_PRINT(idx, "get ssid from flash: ssid:%s, pass:%s\r\n", ssid, pw);
        }else{
			response("AT+WSCONN","wifi info is NULL","");
            //AT_RESP_ERROR(idx, ERROR_ARG);
            return 0;
        }
    }

    //AT_PRINT(idx, "ssid %s pw %s\r\n", ssid, pw);

    ret = conn(ssid, pw);
    if(ret == 0){
        if(flag == 1)
            rda5981_flash_write_sta_data(ssid, pw);
		response("AT+WSCONN","ok","");
        //AT_RESP_OK(idx);
    }else{
		response("AT+WSCONN","","error");
        //AT_RESP_ERROR(idx, ERROR_FAILE);
    }
    return 0;
}
//tcp socket///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void do_recv_thread(void *argument)
{
    unsigned int index, size, res;
    char recv_buf[RECV_LIMIT+1];
    index = *(int *)argument;
	free(argument);
	char tmp_str[9];
    // use the bit31 of index/argument to indicate which console executes this at command 
	memset(tmp_str,0,sizeof(tmp_str));
	sprintf(tmp_str,"do_recv_thread%d",index);
	response(tmp_str,"","");
    while(1){
        if(rda_socket[index].type == TCP){
            size = ((TCPSocket*)(rda_socket[index].socket))->recv((void *)recv_buf, RECV_LIMIT);
            //AT_PRINT(idx, "do_recv_thread size %d\r\n", size);
            if(size <= 0){
                ((TCPSocket*)(rda_socket[index].socket))->close();
                delete (TCPSocket*)(rda_socket[index].socket);
/*                if(trans_index == index){
                     do{
                        TCPSocket* tcpsocket = new TCPSocket(&wifi);
                        rda_socket[index].socket = (void *)tcpsocket;
                        res = tcpsocket->connect(rda_socket[index].address);
                    }while(res != 0);
                }else{ */
                    memset(&rda_socket[index], 0, sizeof(rda_socket_t));
                    //AT_RESP(idx, "+LINKDOWN=%d\r\n", index);
                    return;
/*                 } */
            }
        }
        recv_buf[size] = 0;
		
		memset(tmp_str,0,sizeof(tmp_str));
        // if(trans_index == 0xff){
			sprintf(tmp_str,"AT+NREV%d",index);
			response(tmp_str,recv_buf,"");
            //AT_RESP(idx, "+IPD=%d,%d,%s,%d,", index, size, rda_socket[index].SERVER_ADDR, rda_socket[index].SERVER_PORT);
            //AT_RESP(idx, "%s\r\n",recv_buf);
/*         }else if(trans_index == index){
			response("AT+NREV",recv_buf,"net starus error");
            //AT_RESP(idx, "%s\r\n",recv_buf);
        }else {
            continue;
        } */
    }
}


int do_nstart(int index,char *IP,char *port)
{
    int res;
	int *tmp_index;
	tmp_index = (int *)malloc(sizeof(int));
	*tmp_index = index;
	if(wifi.get_ip_address() == NULL){
		response("AT+NSTART","","net starus error");
		return;
	}
    //for(index=0; index<SOCKET_NUM; index++)
    if(rda_socket[index].used != 0){
		response("AT+NSTART","","used");
		return 0;
	}

    if(index == SOCKET_NUM){
		response("AT+NSTART","","too mach connect");
        //AT_RESP_ERROR(idx, ERROR_FAILE);
        return 0;
    }

    // add @2017.08.15 use the bit31 to indicate which console execute this command: 1 means UART1_IDX 
    //index = index | (idx << 31);

    memcpy(rda_socket[index].SERVER_ADDR, IP, strlen(IP));
    rda_socket[index].SERVER_PORT = atoi(port);
    //AT_PRINT(idx, "ip %s port %d\r\n", rda_socket[index].SERVER_ADDR, rda_socket[index].SERVER_PORT);
    rda_socket[index].address = SocketAddress(rda_socket[index].SERVER_ADDR, rda_socket[index].SERVER_PORT);

	TCPSocket* tcpsocket = new TCPSocket(&wifi);
	rda_socket[index].type = TCP;
	rda_socket[index].socket = (void *)tcpsocket;
	res = tcpsocket->connect(rda_socket[index].address);
	if(res != 0){
		response("AT+NSTART","","connect error");
		//AT_PRINT(idx, "connect failed res = %d\r\n", res);
		//AT_RESP_ERROR(idx, ERROR_FAILE);
		return 0;
	}
	
    rda_socket[index].used = 1;

    rda_socket[index].threadid = rda_thread_new(NULL, do_recv_thread, (void *)tmp_index, 1024*4, osPriorityNormal);
    //AT_RESP_OK_EQU(idx, "%d\r\n", index);
	response("AT+NSTART","ok","");
    return 0;
}

int do_nsend(int index,unsigned char *buf,int total_len)
{
    unsigned int len, size;
	if(wifi.get_ip_address() == NULL){
		response("AT+NSEND","","net starus error");
		return 0;
	}
    if(rda_socket[index].used != 1){
		response("AT+NSEND","","not used");
		return 0;
        //AT_PRINT(idx, "Socket not in used!\r\n");
        //AT_RESP_ERROR(idx, ERROR_ABORT);
    }

    if(total_len > SEND_LIMIT){
		response("AT+NSEND","","data too big");
		return 0;
        //AT_PRINT(idx, "Send data len longger than %d!\r\n", SEND_LIMIT);
        //AT_RESP_ERROR(idx, ERROR_ARG);
    }

/*     buf = (unsigned char *)malloc(total_len);
    len = 0;

    while(len < total_len) {
        tmp_len = console_fifo_get(&buf[len], total_len-len, idx);
        len += tmp_len;
    } */

    if(rda_socket[index].type == TCP){
        size = ((TCPSocket*)(rda_socket[index].socket))->send((void *)buf, total_len);
        //AT_PRINT(idx, "tcp send size %d\r\n", size);
    }

//    free(buf);
    if(size == total_len){
		response("AT+NSEND","ok","");
        //AT_RESP_OK(idx);
    }else{
		response("AT+NSEND","error","");
        //AT_RESP_ERROR(idx, ERROR_FAILE);
	}
	
    return 0;
}

int do_nstop(int index)
{
    if(rda_socket[index].used != 1){
		response("AT+NSEND","","not used");
		return 0;
        //AT_PRINT(idx, "Socket not in used!\r\n");
        //AT_RESP_ERROR(idx, ERROR_ABORT);
    }
    if(rda_socket[index].type == TCP){
        ((TCPSocket*)(rda_socket[index].socket))->close();
    }
    rda_socket[index].used = 0;
	if(rda_socket[index].threadid != NULL){
		osThreadTerminate((osThreadId)(rda_socket[index].threadid));
	}
    delete rda_socket[index].socket;
    //AT_RESP_OK(idx);
    return 0;
	
	
} 




