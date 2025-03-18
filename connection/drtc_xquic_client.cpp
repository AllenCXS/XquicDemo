#include<iostream>
#include<xquic_common.h>
#include<xquic/xquic.h>
#include<xquic_engine_callbacks.h>
#include<xquic_hq_callbacks.h>
#include<xquic_transport_callbacks.h>
#include<xquic_socket.h>

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

/* set client args to default values */
void xqc_cli_init_args(xqc_cli_client_args_t *args){
    memset(args, 0, sizeof(xqc_cli_client_args_t));

    /* net cfg */
    args->net_cfg.conn_timeout = 60;
    strncpy(args->net_cfg.server_addr, "127.0.0.1", sizeof(args->net_cfg.server_addr));
    args->net_cfg.server_port = 8443;

    /* env cfg */
    args->env_cfg.log_level = XQC_LOG_DEBUG;
    strncpy(args->env_cfg.log_path, LOG_PATH, sizeof(args->env_cfg.log_path));
    strncpy(args->env_cfg.out_file_dir, OUT_DIR, sizeof(args->env_cfg.out_file_dir));
    strncpy(args->env_cfg.key_out_path, KEY_PATH, sizeof(args->env_cfg.out_file_dir));

    /* quic cfg */
    args->quic_cfg.alpn_type = ALPN_HQ;
    strncpy(args->quic_cfg.alpn, "hq-interop", sizeof(args->quic_cfg.alpn));
    args->quic_cfg.keyupdate_pkt_threshold = UINT64_MAX;
    /* default 10 */
    args->quic_cfg.mp_version = XQC_MULTIPATH_10;
    args->quic_cfg.init_max_path_id = 2;
    args->quic_cfg.max_pkt_sz = 1200;

    args->req_cfg.throttled_req = -1;

}

//初始化引擎上下文ctx以及参数默认配置
void xqc_cli_init_ctx(xqc_cli_ctx_t *pctx, xqc_cli_client_args_t *args)
{
    strncpy(pctx->log_path, args->env_cfg.log_path, sizeof(pctx->log_path) - 1);
    pctx->args = args;
    xqc_cli_open_log_file(pctx);
    xqc_cli_open_keylog_file(pctx);
}
//日志打印
int xqc_cli_open_log_file(xqc_cli_ctx_t *ctx)
{
    ctx->log_fd = open(ctx->log_path, (O_WRONLY | O_APPEND | O_CREAT), 0644);
    if (ctx->log_fd <= 0) {
        return -1;
    }
    return 0;
}
int xqc_cli_close_log_file(xqc_cli_ctx_t *ctx)
{
    if (ctx->log_fd <= 0) {
        return -1;
    }
    close(ctx->log_fd);
    return 0;
}
int xqc_cli_open_keylog_file(xqc_cli_ctx_t *ctx)
{
    if (ctx->args->env_cfg.key_output_flag) {
        ctx->keylog_fd = open(ctx->args->env_cfg.key_out_path, (O_WRONLY | O_APPEND | O_CREAT), 0644);
        if (ctx->keylog_fd <= 0) {
            return -1;
        }
    }

    return 0;
}
int xqc_cli_close_keylog_file(xqc_cli_ctx_t *ctx)
{
    if (ctx->keylog_fd <= 0) {
        return -1;
    }
    close(ctx->keylog_fd);
    ctx->keylog_fd = 0;
    return 0;
}
//释放资源(引擎上下文资源)
void xqc_cli_free_ctx(xqc_cli_ctx_t *ctx){
    xqc_cli_close_keylog_file(ctx);
    xqc_cli_close_log_file(ctx);

    if (ctx->args) {
        free(ctx->args);
        ctx->args = NULL;
    }

    if (ctx->args) {
        free(ctx->args);
        ctx->args = NULL;
    }
    free(ctx);
    ctx = nullptr;
}

//创建连接 -start###########################
class ConnCtx
{
private:
    /* data */
public:
    // xquic connection
    xqc_cli_user_conn_t *user_conn;

    // 构造函数
    ConnCtx(){};
    //析构函数
    ~ConnCtx(){};

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

        printf("client parse server addr addr:%s:%d", net_cfg->server_addr, net_cfg->server_port);
        freeaddrinfo(result);
    }
    //创建连接
    static std::unique_ptr<ConnCtx> create_connection(xqc_engine_t *engine, xqc_cli_client_args_t *args) {
        // 创建连接上下文
        std::unique_ptr<ConnCtx> conn_ctx(new ConnCtx);
        //create user_conn
        xqc_cli_user_conn_t *user_conn;        
        memset(user_conn, 0, sizeof(xqc_cli_user_conn_t)); 
        

        //初始化连接配置
        xqc_conn_settings_t conn_settings;
        conn_ctx->init_connection_settings(&conn_settings, args);

        /* init ssl config*/
        xqc_conn_ssl_config_t conn_ssl_config;
        conn_ctx->init_connection_ssl_config(&conn_ssl_config, args);
        //创建socket fd
        user_conn->fd = client_create_socket(user_conn, &args->net_cfg);
        if (user_conn->fd < 0) {
            printf("client_create_socket error\n");
            return nullptr;
        }

        const xqc_cid_t *cid;
        printf("engine register alpn:%s,alpn_len:%d", args->quic_cfg.alpn, sizeof(args->quic_cfg.alpn));
        cid = xqc_connect(user_conn->engine, &conn_settings,
                            (const unsigned char *) args->quic_cfg.token, args->quic_cfg.token_len,
                            args->net_cfg.server_addr, false,&conn_ssl_config,
                            (struct sockaddr *) &args->net_cfg.addr, args->net_cfg.addr_len,
                            args->quic_cfg.alpn,
                            user_conn);
        if (cid == NULL) {
            printf("xqc connect error alpn type=%d", args->quic_cfg.alpn_type);
            return nullptr;
        }

        memcpy(&user_conn->xqc_cid, cid, sizeof(xqc_cid_t));
        //保存用户连接
        conn_ctx->user_conn = user_conn;
        return conn_ctx;
    }

    /**
     * 初始化链接设置
     * @param args
     */
    void init_connection_settings(xqc_conn_settings_t *settings, xqc_cli_client_args_t *args) {

        /* 拥塞控制*/
        xqc_cong_ctrl_callback_t cong_ctrl;
        switch (args->net_cfg.cc) {
            case CC_TYPE_BBR:
                cong_ctrl = xqc_bbr_cb;
                printf("cong_ctrl type xqc_bbr_cb");
                break;
            case CC_TYPE_CUBIC:
                cong_ctrl = xqc_cubic_cb;
                printf("cong_ctrl type xqc_cubic_cb");
                break;
            // case CC_TYPE_RENO:
            //     cong_ctrl = xqc_reno_cb;
            //     printf("cong_ctrl type xqc_reno_cb");
            //     break;
            default:
                cong_ctrl = xqc_bbr_cb;
                printf("default cong_ctrl type xqc_bbr_cb");
                break;
        }

        memset(settings, 0, sizeof(xqc_conn_settings_t));
        settings->keyupdate_pkt_threshold = args->quic_cfg.keyupdate_pkt_threshold;//单个 1-rtt 密钥的数据包限制，0 表示无限制

        settings->pacing_on = 0;
        settings->ping_on = 1;
        settings->proto_version = XQC_VERSION_V1; //协议版本
        settings->cong_ctrl_callback = cong_ctrl; //拥塞控制算法
        settings->cc_params.customize_on = 1;     //是否打开自定义
        settings->cc_params.init_cwnd = 32;       //拥塞窗口数
        settings->so_sndbuf = 1024 * 1024;//socket send  buf的大小
        settings->init_idle_time_out = (args->net_cfg.conn_timeout) * 1000;//xquic default 10s
        settings->idle_time_out = (args->net_cfg.conn_timeout) * 1000;//xquic default
        settings->cc_params.cc_optimization_flags = 0;
        settings->cc_params.copa_delta_ai_unit = 1.0;
        settings->cc_params.copa_delta_base = 0.05;
        settings->spurious_loss_detect_on = 1;  //散列丢失检测
        settings->keyupdate_pkt_threshold = 0;  //单个 1-rtt 密钥的数据包限制，0 表示无限制
        settings->max_datagram_frame_size = 0;
        settings->enable_multipath = 0;
        settings->enable_encode_fec = 0;
        settings->enable_decode_fec = 0;
        settings->multipath_version = XQC_MULTIPATH_10;
        settings->marking_reinjection = 1;
        settings->mp_ping_on = 0;
        settings->recv_rate_bytes_per_sec = 0;
        settings->close_dgram_redundancy = XQC_RED_NOT_USE;
        char conn_options[XQC_CO_STR_MAX_LEN] = {0};
        strncpy(settings->conn_option_str, conn_options, XQC_CO_STR_MAX_LEN);
    }

    /**
     * 初始化连接ssl配置
     * @param args
     */
    void init_connection_ssl_config(xqc_conn_ssl_config_t *conn_ssl_config, xqc_cli_client_args_t *args) {
        memset(conn_ssl_config, 0, sizeof(xqc_conn_ssl_config_t));

        /*set session ticket and transport parameter args */
        if (args->quic_cfg.st_len <= 0) {
            conn_ssl_config->session_ticket_data = NULL;
        } else {
            conn_ssl_config->session_ticket_data = args->quic_cfg.st;
            conn_ssl_config->session_ticket_len = args->quic_cfg.st_len;
        }

        if (args->quic_cfg.tp_len <= 0) {
            conn_ssl_config->transport_parameter_data = NULL;
        } else {
            conn_ssl_config->transport_parameter_data = args->quic_cfg.tp;
            conn_ssl_config->transport_parameter_data_len = args->quic_cfg.tp_len;
        }
    }
    //创建流
    void create_stream() {
       //todo
      }
    //销毁流
    void dsestory_stream() {
        //todo
    }
    //销毁连接
    void destory_conn() {
        //todo
        dsestory_stream();
        xqc_conn_close(user_conn->engine, &user_conn->xqc_cid);
        xqc_engine_main_logic(user_conn->engine);
        free(user_conn);
        user_conn = nullptr; 
    }

    //发送数据

    //接收数据
};

