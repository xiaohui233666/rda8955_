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

extern void response(char *cmd,char *dat,char *err);

static char ssid[32+1];
static char pw[64+1];
static char conn_flag = 0;
static char ap_flag = 0;
extern unsigned char rda_mac_addr[6];
extern WiFiStackInterface wifi;
int conn(char *ssid, char *pw)
{
    int ret;
    const char *ip_addr;
    unsigned int ip, msk, gw;
    char ips[IP_LENGTH], msks[IP_LENGTH], gws[IP_LENGTH];

    ret = wifi.connect(ssid, pw, NULL, NSAPI_SECURITY_NONE, 0);
    if(ret != 0){
        //RESP_ERROR(ERROR_FAILE);
        return -1;
    }

    ip_addr = wifi.get_ip_address();

    if (ip_addr) {
        RDA_AT_PRINT("Client IP Address is %s\r\n", ip_addr);
        conn_flag = 1;
        return 0;
    } else {
        RDA_AT_PRINT("No Client IP Address\r\n");
        return -1;
    }
}
int do_wsmac( cmd_tbl_t *cmd, int argc, char *argv[], unsigned char idx)
{
    char *mdata, mac[6], i;

    if(*argv[1] == '?'){
        rda_get_macaddr((unsigned char *)mac, 0);
        AT_RESP_OK_EQU(idx, "%02x:%02x:%02x:%02x:%02x:%02x\r\n",
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        return 0;
    }

    if(conn_flag == 1 || ap_flag == 1){
        AT_RESP_ERROR(idx, ERROR_ABORT);
        return 0;
    }

    if(strlen(argv[1]) != 12){
        AT_RESP_ERROR(idx, ERROR_ARG);
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
        AT_RESP_ERROR(idx, ERROR_ARG);
        return 0;
    }
    memcpy(rda_mac_addr, mac, 6);
    rda5981_flash_write_mac_addr(rda_mac_addr);
    AT_RESP_OK(idx);

    return 0;
}
int do_wsconn( cmd_tbl_t *cmd, int argc, char *argv[], unsigned char idx)
{
    int ret, flag;

    if (argc < 1) {
        AT_RESP_ERROR(idx, ERROR_ARG);
        return 0;
    }

    if(*argv[1] == '?'){
        if(conn_flag == 1){
            wifi.update_rssi();
            AT_RESP_OK_EQU(idx, "ssid:%s, RSSI:%ddb, ip:%s, BSSID: "MAC_FORMAT"\r\n", wifi.get_ssid(), wifi.get_rssi(), wifi.get_ip_address(), MAC2STR(wifi.get_BSSID()));
        }else{
            AT_RESP_OK_EQU(idx, "ssid:, RSSI:0db, ip:0.0.0.0\r\n");
        }
        return 0;
    }

    if(conn_flag == 1){
        AT_RESP_ERROR(idx, ERROR_ABORT);
        AT_PRINT(idx, "error! Has been connected!");
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
            AT_PRINT(idx, "get ssid from flash: ssid:%s, pass:%s\r\n", ssid, pw);
        }else{
            AT_RESP_ERROR(idx, ERROR_ARG);
            return 0;
        }
    }

    AT_PRINT(idx, "ssid %s pw %s\r\n", ssid, pw);

    ret = conn(ssid, pw);
    if(ret == 0){
        if(flag == 1)
            rda5981_flash_write_sta_data(ssid, pw);
        AT_RESP_OK(idx);
    }else{
        AT_RESP_ERROR(idx, ERROR_FAILE);
    }
    return 0;
}

int do_disconn( cmd_tbl_t *cmd, int argc, char *argv[], unsigned char idx)
{
    if(conn_flag == 0){
        AT_RESP_ERROR(idx, ERROR_ABORT);
        return 0;
    }

    wifi.disconnect();
    while(wifi.get_ip_address() != NULL)
        Thread::wait(20);
    conn_flag = 0;
    AT_RESP_OK(idx);
    return 0;
}