#include<iostream>
#include<xquic_common.h>
#include<xquic/xquic.h>
#include<xquic_engine_callbacks.h>
#include<xquic_hq_callbacks.h>
#include<xquic_transport_callbacks.h>
#include<xquic_socket.h>
#include<cassert>
#include<thread>

using namespace std;

#define XQC_OK      0
#define XQC_ERROR   -1

#define XQC_PACKET_TMP_BUF_LEN  1600
#define MAX_BUF_SIZE            (100*1024*1024)
#define MAX_PATH_CNT            2

// #define XQC_ALPN_HQ_INTEROP         "hq-interop"  /* QUIC v1 */
// #define XQC_ALPN_HQ_INTEROP_LEN     10
// #define XQC_ALPN_HQ_29              "hq-29"       /* draft-29 */
// #define XQC_ALPN_HQ_29_LEN          5
const char XQC_ALPN_TRANSPORT[] = "transport";
const int XQC_ALPN_TRANSPORT_LEN = 9;

xqc_usec_t now_time;

//初始化ssl
void xqc_cli_init_engine_ssl_config(xqc_engine_ssl_config_t* cfg, xqc_cli_client_args_t *args)
{
    memset(cfg, 0, sizeof(xqc_engine_ssl_config_t));
    // if (args->quic_cfg.cipher_suites) {
    //     cfg->ciphers = args->quic_cfg.cipher_suites;

    // } else {
    //     cfg->ciphers = XQC_TLS_CIPHERS;
    // }

    // cfg->groups = XQC_INTEROP_TLS_GROUPS;
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

    ret = xqc_engine_register_alpn(ctx->engine, XQC_ALPN_TRANSPORT, XQC_ALPN_TRANSPORT_LEN, &ap_cbs, nullptr);
    if (ret != XQC_OK) {
        xqc_engine_unregister_alpn(ctx->engine, XQC_ALPN_TRANSPORT,XQC_ALPN_TRANSPORT_LEN);
        printf("engine register alpn error, ret:%d", ret);
        return XQC_ERROR;
    }else{
        printf("engine register alpn:%s,alpn_len:%d,ret:%d", XQC_ALPN_TRANSPORT, XQC_ALPN_TRANSPORT_LEN, ret);
    }

    // ret = xqc_engine_register_alpn(ctx->engine, XQC_ALPN_HQ_29, XQC_ALPN_HQ_29_LEN, &ap_cbs, nullptr);
    // if (ret != XQC_OK) {
    //     xqc_engine_unregister_alpn(ctx->engine, XQC_ALPN_HQ_29,XQC_ALPN_HQ_29_LEN);
    //     printf("engine register alpn error, ret:%d", ret);
    //     return XQC_ERROR;
    // }else{
    //     printf("engine register alpn:%s,alpn_len:%d,ret:%d", XQC_ALPN_HQ_29, XQC_ALPN_HQ_29_LEN, ret);
    // }

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
    // /* net cfg */
    // // args->net_cfg.conn_timeout = 60;
    // // strncpy(args->net_cfg.server_addr, "127.0.0.1", sizeof(args->net_cfg.server_addr));
    // // args->net_cfg.server_port = 8443;

    // /* env cfg */
    // args->env_cfg.log_level = XQC_LOG_DEBUG;
    // strncpy(args->env_cfg.log_path, LOG_PATH, sizeof(args->env_cfg.log_path));
    // strncpy(args->env_cfg.out_file_dir, OUT_DIR, sizeof(args->env_cfg.out_file_dir));
    // strncpy(args->env_cfg.key_out_path, KEY_PATH, sizeof(args->env_cfg.out_file_dir));

    // /* quic cfg */
    // args->quic_cfg.alpn_type = ALPN_HQ;
    // strncpy(args->quic_cfg.alpn, "hq-interop", sizeof(args->quic_cfg.alpn));
    // args->quic_cfg.keyupdate_pkt_threshold = UINT64_MAX;
    // /* default 10 */
    // args->quic_cfg.mp_version = XQC_MULTIPATH_10;
    // args->quic_cfg.init_max_path_id = 2;
    // args->quic_cfg.max_pkt_sz = 1200;

    // args->req_cfg.throttled_req = -1;

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

//初始化引擎上下文ctx以及参数默认配置
void xqc_cli_init_ctx(xqc_cli_ctx_t *pctx, xqc_cli_client_args_t *args)
{
    strncpy(pctx->log_path, args->env_cfg.log_path, sizeof(pctx->log_path) - 1);
    pctx->args = args;
    xqc_cli_open_log_file(pctx);
    xqc_cli_open_keylog_file(pctx);
}

//释放资源(引擎上下文资源)
void xqc_cli_free_ctx(xqc_cli_ctx_t *ctx){
    xqc_cli_close_keylog_file(ctx);
    xqc_cli_close_log_file(ctx);

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
    ~ConnCtx(){
        destory_conn();
    };

    /**
     * 解析服务器地址
     * @param cfg
     * @return
     */
    void client_parse_server_addr(xqc_cli_user_conn_t *user_conn, const std::string &server_addr, int server_port) {
        struct addrinfo hints = {0};
        memset(&hints, 0, sizeof(struct addrinfo));
        hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
        hints.ai_socktype = SOCK_DGRAM; /* Datagram socket */
        hints.ai_flags = AI_PASSIVE;    /* For wildcard IP address */
        hints.ai_protocol = 0;          /* Any protocol */
        hints.ai_canonname = NULL;
        hints.ai_addr = NULL;
        hints.ai_next = NULL;

        struct addrinfo *result = nullptr; // 删除new addrinfo，使用指针直接赋值
        int rv = getaddrinfo(server_addr.c_str(), std::to_string(server_port).c_str(), &hints, &result);
        if (rv != 0) {
            printf( "get addr info from hostname: %d", gai_strerror(rv));
            return;
          }
          user_conn->peer_addrlen = result->ai_addrlen;
          if (result->ai_family == AF_INET6) {
            user_conn->peer_addr_v6 =
                (sockaddr_in6 *)malloc(sizeof(struct sockaddr_in6));
            memcpy(user_conn->peer_addr_v6, result->ai_addr, result->ai_addrlen);
            inet_ntop(result->ai_family, &(user_conn->peer_addr_v6->sin6_addr),
            user_conn->server_addr_str, sizeof(user_conn->server_addr_str));
          } else if (result->ai_family == AF_INET) {
            user_conn->peer_addr_v4 =
                (sockaddr_in *)malloc(sizeof(struct sockaddr_in));
            memcpy(user_conn->peer_addr_v4, result->ai_addr, result->ai_addrlen);
            inet_ntop(result->ai_family, &(user_conn->peer_addr_v4->sin_addr),
            user_conn->server_addr_str, sizeof(user_conn->server_addr_str));
          } else {
            assert(false);
          }
          assert(user_conn->fd == 0);
          user_conn->fd = socket(result->ai_family, SOCK_DGRAM, 0);
          freeaddrinfo(result);
          cout << "ClientUserConnCreate server:"
            << server_addr << ", addr: " << user_conn->server_addr_str
            << ", port: " << server_port << " , fd:" << user_conn->fd << endl;

          if (user_conn->fd < 0) {
            user_conn->fd = 0;
            printf( "create socket failed, errno: %s", get_sys_errno());
            return;
          }
        printf("zyffff client_parse_server_addr end \n");
    }

    // 创建连接
    static std::unique_ptr<ConnCtx> create_connection(xqc_engine_t *engine, xqc_cli_client_args_t *args, const std::string &server_addr, 
        int server_port) {
        // 创建连接上下文
        std::unique_ptr<ConnCtx> conn_ctx(new ConnCtx);
        // create user_conn
        xqc_cli_user_conn_t *user_conn = new xqc_cli_user_conn_t;        
        memset(user_conn, 0, sizeof(xqc_cli_user_conn_t)); 
        // 解析ip、端口
        conn_ctx->client_parse_server_addr(user_conn, server_addr, server_port);
        if (user_conn->peer_addr_v4 == nullptr && user_conn->peer_addr_v6 == nullptr) { // 检查解析结果是否有效
            printf("Failed to parse server address\n");
            delete user_conn;
            return nullptr;
        }
        // 初始化连接配置
        xqc_conn_settings_t conn_settings;
        conn_ctx->init_connection_settings(&conn_settings, args);
        printf("init connection settings \n");

        /* init ssl config*/
        xqc_conn_ssl_config_t conn_ssl_config;
        conn_ctx->init_connection_ssl_config(&conn_ssl_config, args);
        printf("init init_connection_ssl_config \n");
        // todo  创建socket fd
        // user_conn->fd = client_create_socket(user_conn, &args->net_cfg);
        // printf("init client_create_socket \n");
        // if (user_conn->fd < 0) {
        //     printf("client_create_socket error\n");
        //     delete user_conn;
        //     return nullptr;
        // }
        printf("after init client_create_socket \n");
        sockaddr *local_addr;
        socklen_t local_len;
        if (conn_ctx->user_conn == nullptr)
        {
            printf(" conn_ctx->user_conn 为null \n");
        }else{
            printf(" conn_ctx->user_conn 不为null \n");
        }
        

        if (user_conn->peer_addr_v6) {
            printf(" conn_ctx->user_conn peer_addr_v6\n");
            user_conn->local_addr_v6 = (sockaddr_in6 *)malloc(sizeof(struct sockaddr_in6));
            if (user_conn->local_addr_v6 == nullptr) { // 检查内存分配是否成功
                printf("Memory allocation failed for local_addr_v6\n");
                close(user_conn->fd);
                free(user_conn);
                return nullptr;
            }
            local_len = sizeof(struct sockaddr_in6);
            local_addr = (sockaddr *)user_conn->local_addr_v6;
        } else if (user_conn->peer_addr_v4) {
            printf(" conn_ctx->user_conn peer_addr_v4\n");
            user_conn->local_addr_v4 = (sockaddr_in *)malloc(sizeof(struct sockaddr_in));
            if (user_conn->local_addr_v4 == nullptr) { // 检查内存分配是否成功
                printf("Memory allocation failed for local_addr_v4\n");
                close(user_conn->fd);
                free(user_conn);
                return nullptr;
            }
            local_len = sizeof(struct sockaddr_in);
            local_addr = (sockaddr *)user_conn->local_addr_v4;
        } else {
            assert(false);
            printf(" conn_ctx->user_conn else\n");
        }
        user_conn->local_addrlen = local_len;
        int ret = getsockname(user_conn->fd, local_addr, &local_len);
        printf(" init getsockname,ret:%d \n", ret);
        if (ret < 0) {
            cout << "getsockname error, errno: " << get_sys_errno() << endl;
            close(user_conn->fd);
            delete user_conn;
            return nullptr;
        }
        printf("Server Address: %s\n", user_conn->server_addr_str);
        printf("Generated CID: %d\n", (&user_conn->xqc_cid));
    
        printf("engine register alpn:%s,alpn_len:%d \n", XQC_ALPN_TRANSPORT, XQC_ALPN_TRANSPORT_LEN);
        printf(" xqc_connect start\n");  
        user_conn->engine = engine;
        // cid = xqc_connect(user_conn->engine, &conn_settings,
        //                 (const unsigned char *) args->quic_cfg.token, args->quic_cfg.token_len,
        //                 args->net_cfg.server_addr, false, &conn_ssl_config,
        //                 (struct sockaddr *) &args->net_cfg.addr, args->net_cfg.addr_len,
        //                 args->quic_cfg.alpn,
        //                 user_conn);

        struct sockaddr *addr = nullptr;

        if (user_conn->peer_addr_v4 != nullptr) {
          addr = (struct sockaddr *)user_conn->peer_addr_v4;
        } else if (user_conn->peer_addr_v6 != nullptr) {
          addr = (struct sockaddr *)user_conn->peer_addr_v6;
        } else {
          assert(false);
        }
        //测试
        const xqc_cid_t *cid = xqc_connect(
            engine, &conn_settings, (const unsigned char *) args->quic_cfg.token, args->quic_cfg.token_len, user_conn->server_addr_str, false,
            &conn_ssl_config, addr, user_conn->peer_addrlen, XQC_ALPN_TRANSPORT, user_conn);

        cout << "create cid(" << (void *)cid
            << ") success" << endl;
        printf(" xqc_connect end\n");             
        if (cid == NULL) {
            printf("xqc connect error alpn type=%s", XQC_ALPN_TRANSPORT);
            close(user_conn->fd);
            delete user_conn;
            return nullptr;
        }

        user_conn->xqc_cid = (xqc_cid_t *)malloc(sizeof(xqc_cid_t));
        memcpy(user_conn->xqc_cid, cid, sizeof(xqc_cid_t));
        // 保存用户连接
        conn_ctx->user_conn = user_conn;
        printf(" create_connection end\n");         
        return conn_ctx;
    }

    /**
     * 初始化链接设置
     * @param args
     */
    void init_connection_settings(xqc_conn_settings_t *settings, xqc_cli_client_args_t *args) {

        /* 拥塞控制*/
        xqc_cong_ctrl_callback_t cong_ctrl;
        cong_ctrl = xqc_bbr_cb;

        memset(settings, 0, sizeof(xqc_conn_settings_t));
        settings->keyupdate_pkt_threshold = args->quic_cfg.keyupdate_pkt_threshold;//单个 1-rtt 密钥的数据包限制，0 表示无限制

        settings->pacing_on = 0;
        settings->ping_on = 1;
        settings->proto_version = XQC_VERSION_V1; //协议版本
        settings->cong_ctrl_callback = cong_ctrl; //拥塞控制算法
        settings->cc_params.customize_on = 1;     //是否打开自定义
        settings->cc_params.init_cwnd = 32;       //拥塞窗口数
        settings->so_sndbuf = 1024 * 1024;//socket send  buf的大小
        settings->init_idle_time_out = 10 * 1000;//xquic default 10s
        settings->idle_time_out = 10 * 1000;//xquic default
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
        if (user_conn->stream != nullptr) {
            return;
          }
          user_conn->stream = xqc_stream_create(user_conn->engine, user_conn->xqc_cid, nullptr, this);
          if (nullptr == user_conn->stream) {
            printf("create transport stream error\n");
          }
          printf("create xqc stream create success, stream id: %d",xqc_stream_id(user_conn->stream));
    }

    //销毁流
    void dsestory_stream() {
        if (user_conn != nullptr && user_conn->stream == nullptr) {
            return;
        }
        printf("destory stream, id: " + xqc_stream_id(user_conn->stream));
          xqc_stream_close(user_conn->stream);
          user_conn->stream = nullptr;
    }
    //销毁连接
    void destory_conn() { // 修正拼写错误为destroy_conn()
        if (user_conn == nullptr) {
            return;
        }
        dsestory_stream();
        xqc_conn_close(user_conn->engine, user_conn->xqc_cid);
        xqc_engine_main_logic(user_conn->engine);

        if (user_conn->local_addr_v4) {
            free(user_conn->local_addr_v4);
            user_conn->local_addr_v4 = nullptr;
        }
        if (user_conn->local_addr_v6) {
            free(user_conn->local_addr_v6);
            user_conn->local_addr_v6 = nullptr;
        }
        if (user_conn->peer_addr_v4) {
            free(user_conn->peer_addr_v4);
            user_conn->peer_addr_v4 = nullptr;
        }
        if (user_conn->peer_addr_v6) {
            free(user_conn->peer_addr_v6);
            user_conn->peer_addr_v6 = nullptr;
        }
        free(user_conn->xqc_cid);
        user_conn->xqc_cid = nullptr;

        free(user_conn);
        user_conn = nullptr; 
    }

    //发送数据,不关闭fd
    ssize_t send_msg(unsigned char *msg, size_t len, int fin = 0) {
        if (user_conn == nullptr) {
            return -1;
        }
        if (user_conn->fd == 0 || user_conn->stream == nullptr) {
          printf("xquic send msg error");
          return -1;
        }
        ssize_t ret = xqc_stream_send(user_conn->stream, msg, len, fin);
        cout << "xqc_stream_send msg=" << (const char *)msg << ", ret: " << ret << endl;
        return ret;
    }

    //发送数据,关闭fd
    ssize_t send_msg_fin(unsigned char *msg, size_t len) {
        int fin = 1;
        return send_msg(msg, len, fin);
    }

    //接收数据
    void receive_msg() {
        ssize_t recv_size = 0;
        ssize_t recv_sum = 0;
        struct sockaddr *local_addr = nullptr;
        struct sockaddr *peer_addr = nullptr;
        socklen_t local_len = 0;
        socklen_t peer_len = 0;
        unsigned char packet_buf[XQC_PACKET_TMP_BUF_LEN];
        int i;

        if (user_conn->peer_addr_v4 != nullptr) {
            local_addr = (struct sockaddr *)user_conn->local_addr_v4;
            peer_addr = (struct sockaddr *)user_conn->peer_addr_v4;
        } else if (user_conn->peer_addr_v6 != nullptr) {
            peer_addr = (struct sockaddr *)user_conn->peer_addr_v6;
            local_addr = (struct sockaddr *)user_conn->local_addr_v6;
        } else {
            assert(false);
        }
        local_len = user_conn->local_addrlen;
        peer_len = user_conn->peer_addrlen;
        do {
            recv_size = recvfrom(user_conn->fd, packet_buf, sizeof(packet_buf), 0,
            peer_addr, &user_conn->peer_addrlen);
            if (recv_size < 0 && get_sys_errno() == EAGAIN) {
                break;
            }
    
            if (recv_size <= 0) {
                break;
            }
    
            recv_sum += recv_size;
            uint64_t recv_time = xqc_now();
            int ret = xqc_engine_packet_process(user_conn->engine, packet_buf, recv_size,
                local_addr, local_len, peer_addr, peer_len, (xqc_msec_t)recv_time, user_conn);
            if ( ret != XQC_OK){
                cout << "xqc_client_read_handler: packet process err, ret: " << ret << endl;
                return;
            }
        } while (recv_size > 0);
    
    finish_recv:
        xqc_engine_finish_recv(user_conn->engine);
    }
};

void testReceive(std::unique_ptr<ConnCtx>& conn_ctx_) {
    conn_ctx_->receive_msg();
}

int main() {
    /* init env if necessary */
   //  xqc_platform_init_env();
   
   //  /* get input client args */
    xqc_cli_client_args_t *args = new xqc_cli_client_args_t;
    memset(args, 0, sizeof(xqc_cli_client_args_t));
    //设置默认参数
    // xqc_cli_init_args(args);

   //  /* init client ctx */
    xqc_cli_ctx_t *ctx = new xqc_cli_ctx_t;
    memset(ctx, 0, sizeof(xqc_cli_ctx_t));
    xqc_cli_init_ctx(ctx, args);

   //  /* init engine */
   xqc_cli_init_xquic_engine(ctx, args);

   // 创建连接
   std::unique_ptr<ConnCtx> conn_ctx_;
   conn_ctx_ = ConnCtx::create_connection(ctx->engine, args, "127.0.0.1", 8443);

   //创建流
   conn_ctx_->create_stream();

   //接收数据
   std::thread t1(testReceive, std::ref(conn_ctx_));

   //发送数据
   string msg = "hello world";
   conn_ctx_->send_msg((unsigned char *)msg.data(), msg.size());

   //等待30s
   // 休眠1秒
   std::this_thread::sleep_for(std::chrono::seconds(30));
  
   //销毁连接
   if (conn_ctx_){
     conn_ctx_->destory_conn();
   }
   
   //释放资源
    xqc_engine_destroy(ctx->engine);
    xqc_cli_free_ctx(ctx);

   return 0;
}