#include<iostream>
#include<xquic_common.h>
#include<xquic/xquic.h>
#include<xquic_engine_callbacks.h>
#include<xquic_hq_callbacks.h>
#include<xquic_transport_callbacks.h>

using namespace std;

#define XQC_OK      0
#define XQC_ERROR   -1

#define XQC_PACKET_TMP_BUF_LEN  1600
#define MAX_BUF_SIZE            (100*1024*1024)
#define XQC_INTEROP_TLS_GROUPS  "X25519:P-256:P-384:P-521"
#define MAX_PATH_CNT            2

#define XQC_ALPN_HQ_INTEROP         "hq-interop"  /* QUIC v1 */
#define XQC_ALPN_HQ_INTEROP_LEN     10
#define XQC_ALPN_HQ_29              "hq-29"       /* draft-29 */
#define XQC_ALPN_HQ_29_LEN          5

xqc_usec_t now_time;

//初始化ssl
void xqc_cli_init_engine_ssl_config(xqc_engine_ssl_config_t* cfg, xqc_cli_client_args_t *args)
{
    memset(cfg, 0, sizeof(xqc_engine_ssl_config_t));
    if (args->quic_cfg.cipher_suites) {
        cfg->ciphers = args->quic_cfg.cipher_suites;

    } else {
        cfg->ciphers = XQC_TLS_CIPHERS;
    }

    cfg->groups = XQC_INTEROP_TLS_GROUPS;
}

//初始化引擎回调函数
void xqc_cli_init_callback(xqc_engine_callback_t *cb, xqc_transport_callbacks_t *transport_cbs,
    xqc_cli_client_args_t* args){
    static xqc_engine_callback_t callback = {
        .log_callbacks = {
            .xqc_log_write_err = xqc_cli_write_log_file,
            .xqc_log_write_stat = xqc_cli_write_log_file
        },
        .set_event_timer = xqc_cli_set_event_timer,
    };

    static xqc_transport_callbacks_t tcb = {
        .write_socket = xqc_cli_write_socket,
        .write_socket_ex = xqc_cli_write_socket_ex,
        .save_token = xqc_cli_save_token, /* save token */
        .save_session_cb = xqc_cli_save_session_cb,
        .save_tp_cb = xqc_cli_save_tp_cb,
        .conn_update_cid_notify = xqc_cli_conn_update_cid_notify
    };

    *cb = callback;
    *transport_cbs = tcb;
}
//注册engine
int xqc_cli_init_alpn_ctx(xqc_cli_ctx_t *ctx)
{
    int ret = 0;

    xqc_app_proto_callbacks_t ap_cbs = {
        .conn_cbs = {
            .conn_create_notify = xqc_cli_hq_conn_create_notify,
            .conn_close_notify = xqc_cli_hq_conn_close_notify,
            .conn_handshake_finished = xqc_cli_hq_conn_handshake_finished,
            .conn_ping_acked = xqc_cli_hq_conn_ping_acked_notify,
        },
        .stream_cbs = {
            .stream_write_notify = xqc_client_stream_write_notify,
            .stream_read_notify = xqc_client_stream_read_notify,
            .stream_close_notify = xqc_client_stream_close_notify,
        }
    };

    ret = xqc_engine_register_alpn(ctx->engine, XQC_ALPN_HQ_INTEROP, XQC_ALPN_HQ_INTEROP_LEN, &ap_cbs, nullptr);
    if (ret != XQC_OK) {
        xqc_engine_unregister_alpn(ctx->engine, XQC_ALPN_HQ_INTEROP,XQC_ALPN_HQ_INTEROP_LEN);
        printf("engine register alpn error, ret:%d", ret);
        return XQC_ERROR;
    }else{
        printf("engine register alpn:%s,alpn_len:%d,ret:%d", XQC_ALPN_HQ_INTEROP, XQC_ALPN_HQ_INTEROP_LEN, ret);
    }

    ret = xqc_engine_register_alpn(ctx->engine, XQC_ALPN_HQ_29, XQC_ALPN_HQ_29_LEN, &ap_cbs, nullptr);
    if (ret != XQC_OK) {
        xqc_engine_unregister_alpn(ctx->engine, XQC_ALPN_HQ_29,XQC_ALPN_HQ_29_LEN);
        printf("engine register alpn error, ret:%d", ret);
        return XQC_ERROR;
    }else{
        printf("engine register alpn:%s,alpn_len:%d,ret:%d", XQC_ALPN_HQ_29, XQC_ALPN_HQ_29_LEN, ret);
    }

    return XQC_OK;
}

//创建引擎
int xqc_cli_init_xquic_engine(xqc_cli_ctx_t *ctx, xqc_cli_client_args_t *args){
    now_time = xqc_now();
    /* init engine ssl config */
    xqc_engine_ssl_config_t engine_ssl_config;
    xqc_transport_callbacks_t transport_cbs;
    xqc_cli_init_engine_ssl_config(&engine_ssl_config, args);

    /* init engine callbacks */
    xqc_engine_callback_t callback;
    xqc_cli_init_callback(&callback, &transport_cbs, args);

    xqc_config_t config;
    if (xqc_engine_get_default_config(&config, XQC_ENGINE_CLIENT) < 0) {
        return XQC_ERROR;
    }

    switch (args->env_cfg.log_level) {
    case 'd':
        config.cfg_log_level = XQC_LOG_DEBUG;
        break;
    case 'i':
        config.cfg_log_level = XQC_LOG_INFO;
        break;
    case 'w':
        config.cfg_log_level = XQC_LOG_WARN;
        break;
    case 'e':
        config.cfg_log_level = XQC_LOG_ERROR;
        break;
    default:
        config.cfg_log_level = XQC_LOG_DEBUG;
        break;
    }
    //创建引擎
    ctx->engine = xqc_engine_create(XQC_ENGINE_CLIENT, &config, 
                                     &engine_ssl_config, &callback, &transport_cbs, ctx);
    if (ctx->engine == NULL) {
        cout << "xqc_engine_create error" << endl;
        return XQC_ERROR;
    }
     //注册alpn
    if (xqc_cli_init_alpn_ctx(ctx) < 0) {
        cout << "init alpn ctx error!" << endl;
        return -1;
    }
    return XQC_OK;
}

//创建连接
/**
 * 解析服务器地址
 * @param cfg
 * @return
 */
void client_parse_server_addr(xqc_cli_net_config_t *net_cfg, const std::string &server_addr, int server_port) {
    /* set hit for hostname resolve */
    struct addrinfo hints = {0};
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;        /* allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_DGRAM;     /* datagram socket */
    hints.ai_flags = AI_PASSIVE;        /* For wildcard IP address */
    hints.ai_protocol = 0;              /* Any protocol */
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

    /* resolve server's ip from hostname */
    struct addrinfo *result = NULL;
    int rv = getaddrinfo(server_addr.c_str(),std::to_string(server_port).c_str(), &hints, &result);
    if (rv != 0) {
        char err_msg[1024];
        sprintf(err_msg, "get addr info from hostname:%s.", gai_strerror(rv));
        printf("%s\n", err_msg);
    }
    memcpy(&net_cfg->addr, result->ai_addr, result->ai_addrlen);
    net_cfg->addr_len = result->ai_addrlen;

    /* convert to string */
    if (result->ai_family == AF_INET6) {
        inet_ntop(result->ai_family, &(((struct sockaddr_in6 *) result->ai_addr)->sin6_addr),
        net_cfg->server_addr, sizeof(net_cfg->server_addr));
    } else {
        inet_ntop(result->ai_family, &(((struct sockaddr_in *) result->ai_addr)->sin_addr),
        net_cfg->server_addr, sizeof(net_cfg->server_addr));
    }

    printf("client parse server addr server[%s] addr:%s:%d", net_cfg->host, net_cfg->server_addr,
        net_cfg->server_port);
    freeaddrinfo(result);
}

//创建流

//发送数据

//接收数据

//关闭流

//关闭连接

//关闭引擎