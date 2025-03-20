#pragma once
#ifndef XQUIC_COMMON_H
#define XQUIC_COMMON_H
#include <xquic/xquic.h>
#include <xquic/xquic_typedef.h>
#include <sys/time.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <fcntl.h>
using namespace std;

#define LOG_PATH "clog.log"
#define KEY_PATH "ckeys.log"
#define OUT_DIR  "."

/* definition for connection */
#define DEFAULT_SERVER_ADDR "127.0.0.1"
#define DEFAULT_SERVER_PORT 8443

#define CIPHER_SUIT_LEN     256
#define TLS_GROUPS_LEN      64

#define PATH_LEN            1024
#define RESOURCE_LEN        1024
#define AUTHORITY_LEN       128
#define URL_LEN             1024
#define XQC_INTEROP_TLS_GROUPS "X25519:P-256:P-384:P-521"
#define XQC_PACKET_TMP_BUF_LEN  1600
#define MAX_BUF_SIZE            (100*1024*1024)
#define MAX_PATH_CNT            2

extern const char *line_break;
extern size_t READ_FILE_BUF_LEN;

extern int get_sys_errno();

extern uint64_t xqc_now();

typedef enum xqc_cli_alpn_type_s {
    ALPN_HQ,
    ALPN_H3,
} xqc_cli_alpn_type_t;

/* request method */
typedef enum request_method_e {
    REQUEST_METHOD_GET,
    REQUEST_METHOD_POST,
} REQUEST_METHOD;

extern char method_s[][16];

/* the congestion control types */
typedef enum cc_type_s {
    CC_TYPE_BBR,
    CC_TYPE_CUBIC,
    CC_TYPE_RENO,
    CC_TYPE_COPA
} CC_TYPE;

/* the task mode types */
typedef enum xqc_cli_task_mode_s {
    /* send multi requests in single connection with multi streams */
    MODE_SCMR,

    /* serially send multi requests in multi connections, with one request each connection */
    MODE_SCSR_SERIAL,

    /* concurrently send multi requests in multi connections, with one request each connection */
    MODE_SCSR_CONCURRENT,
} xqc_cli_task_mode_t;

/* network arguments */
// typedef struct xqc_cli_net_config_s {

//     /* server addr info */
//     struct sockaddr_in6 addr;
//     int                 addr_len;
//     char                server_addr[64];
//     short               server_port;
//     // char                host[256];

//     /* ipv4 or ipv6 */
//     int                 ipv6;

//     /* congestion control algorithm */
//     CC_TYPE             cc;     /* congestion control algorithm */
//     int                 pacing; /* is pacing on */

//     /* idle persist timeout */
//     int                 conn_timeout;

//     xqc_cli_task_mode_t mode;

//     char iflist[MAX_PATH_CNT][128];     /* list of interfaces */
//     int ifcnt;
    
//     int multipath;

//     uint8_t rebind_p0;
//     uint8_t rebind_p1;

// } xqc_cli_net_config_t;

/**
 * ============================================================================
 * the quic config definition section
 * quic config is those arguments about quic connection
 * all configuration on network should be put under this section
 * ============================================================================
 */

/* definition for quic */
#define MAX_SESSION_TICKET_LEN      8192    /* session ticket len */
#define MAX_TRANSPORT_PARAMS_LEN    8192    /* transport parameter len */
#define XQC_MAX_TOKEN_LEN           8192     /* token len */

#define SESSION_TICKET_FILE         "session_ticket"
#define TRANSPORT_PARAMS_FILE       "transport_params"
#define TOKEN_FILE                  "token"

typedef struct xqc_cli_quic_config_s {
    /* alpn protocol of client */
    xqc_cli_alpn_type_t alpn_type;
    char alpn[16];

    /* 0-rtt config */
    int  st_len;                        /* session ticket len */
    char st[MAX_SESSION_TICKET_LEN];    /* session ticket buf */
    int  tp_len;                        /* transport params len */
    char tp[MAX_TRANSPORT_PARAMS_LEN];  /* transport params buf */
    int  token_len;                     /* token len */
    char token[XQC_MAX_TOKEN_LEN];      /* token buf */

    char *cipher_suites;                /* cipher suites */

    uint8_t use_0rtt;                   /* 0-rtt switch, default turned off */
    uint64_t keyupdate_pkt_threshold;   /* packet limit of a single 1-rtt key, 0 for unlimited */

    uint8_t mp_ack_on_any_path;

    char mp_sched[32];
    uint8_t mp_backup;

    uint64_t close_path;

    uint8_t no_encryption;

    uint64_t recv_rate;

    uint32_t reinjection;

    uint8_t mp_version;

    uint64_t init_max_path_id;

    /* support interop test */
    int is_interop_mode;

    uint8_t send_path_standby;
    xqc_msec_t path_status_timer_threshold;

    uint64_t least_available_cid_count;

    uint64_t idle_timeout;
    uint8_t  remove_path_flag;

    size_t max_pkt_sz;

    char co_str[XQC_CO_STR_MAX_LEN];

} xqc_cli_quic_config_t;

/* environment config */
typedef struct xqc_cli_env_config_s {

    /* log path */
    char    log_path[256];
    int     log_level;

    /* out file */
    char    out_file_dir[256];

    /* key export */
    int     key_output_flag;
    char    key_out_path[256];

    /* life cycle */
    int     life;
} xqc_cli_env_config_t;

/**
 * ============================================================================
 * the request config definition section
 * all configuration on request should be put under this section
 * ============================================================================
 */
#define MAX_REQUEST_CNT 20480    /* client might deal MAX_REQUEST_CNT requests once */
#define MAX_REQUEST_LEN 256     /* the max length of a request */
#define g_host ""
typedef struct xqc_cli_request_s {
    char            path[RESOURCE_LEN];         /* request path */
    char            scheme[8];                  /* request scheme, http/https */
    REQUEST_METHOD  method;
    char            auth[AUTHORITY_LEN];
    char            url[URL_LEN];               /* original url */
    // char            headers[MAX_HEADER][256];   /* field line of h3 */
} xqc_cli_request_t;

/* request bundle args */
typedef struct xqc_cli_requests_s {
    /* requests */
    int                     request_cnt;    /* requests cnt in urls */
    xqc_cli_request_t  reqs[MAX_REQUEST_CNT];

    /* do not save responses to files */
    int dummy_mode;

    /* delay X us to start reqs */
    uint64_t req_start_delay; 

    uint64_t idle_gap;

    /* serial requests */
    uint8_t serial;

    int throttled_req;

    int ext_reqn;
    int batch_cnt;

} xqc_cli_requests_t;

/**
 * ============================================================================
 * the client args definition section
 * client initial args
 * ============================================================================
 */
typedef struct xqc_cli_client_args_s {
    /* network args */
    // xqc_cli_net_config_t   net_cfg;

    /* quic args */
    xqc_cli_quic_config_t  quic_cfg;

    /* environment args */
    xqc_cli_env_config_t   env_cfg;

    /* request args */
    // xqc_cli_requests_t     req_cfg;
} xqc_cli_client_args_t;

typedef struct xqc_cli_ctx_s {
    /* xquic engine context */
    xqc_engine_t    *engine;

    /* libevent context */
    // struct event    *ev_engine;
    // struct event    *ev_task;
    // struct event    *ev_kill;
    // struct event_base *eb;  /* handle of libevent */

    /* log context */
    int             log_fd;
    char            log_path[256];

    /* key log context */
    int             keylog_fd;

    /* client context */
    xqc_cli_client_args_t  *args;
} xqc_cli_ctx_t;

typedef struct xqc_cli_user_conn_s {
    // quic 引擎
    xqc_engine_t *engine = nullptr;
    // 具体传输的socket fd
    int fd = 0;
    // 创建的单个connect
    xqc_cid_t *xqc_cid;
    // 发送数据的流
    xqc_stream_t *stream = nullptr;

    char server_addr_str[64] = {0};
    struct sockaddr_in *local_addr_v4 = nullptr;
    struct sockaddr_in6 *local_addr_v6 = nullptr;
    socklen_t local_addrlen = 0;

    struct sockaddr_in *peer_addr_v4 = nullptr;
    struct sockaddr_in6 *peer_addr_v6 = nullptr;
    socklen_t peer_addrlen = 0;

} xqc_cli_user_conn_t;
#endif