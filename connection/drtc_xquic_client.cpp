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

const char XQC_ALPN_TRANSPORT[] = "transport";
const int XQC_ALPN_TRANSPORT_LEN = 9;

xqc_usec_t now_time;

//初始化ssl
void xqc_cli_init_engine_ssl_config(xqc_engine_ssl_config_t* ssl_cfg, xqc_cli_client_args_t *args)
{
    memset(ssl_cfg, 0, sizeof(xqc_engine_ssl_config_t));
    ssl_cfg->ciphers = XQC_TLS_CIPHERS;
    ssl_cfg->groups = XQC_TLS_GROUPS;
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
        // .conn_update_cid_notify = xqc_cli_conn_update_cid_notify
    };

    *cb = callback;
    *transport_cbs = tcb;
}
//注册engine
int xqc_cli_init_alpn_ctx(xqc_cli_ctx_t *ctx)
{
    int ret = 0;

    // xqc_app_proto_callbacks_t ap_cbs = {
    //     .conn_cbs = {
    //         .conn_create_notify = xqc_cli_hq_conn_create_notify,
    //         .conn_close_notify = xqc_cli_hq_conn_close_notify,
    //         .conn_handshake_finished = xqc_cli_hq_conn_handshake_finished,
    //         .conn_ping_acked = xqc_cli_hq_conn_ping_acked_notify,
    //     },
    //     .stream_cbs = {
    //         .stream_write_notify = xqc_client_stream_write_notify,
    //         .stream_read_notify = xqc_client_stream_read_notify,
    //         .stream_close_notify = xqc_client_stream_close_notify,
    //         .stream_create_notify = stream_create_notify_callback,
    //         .stream_closing_notify = stream_closing_notify_callback
    //     }
    // };

    /* register transport callbacks */
    xqc_app_proto_callbacks_t ap_cbs;
    {
      memset(&ap_cbs, 0, sizeof(xqc_app_proto_callbacks_t));

      // conn
      ap_cbs.conn_cbs.conn_create_notify = xqc_cli_hq_conn_create_notify;
      ap_cbs.conn_cbs.conn_close_notify = xqc_cli_hq_conn_close_notify;
      ap_cbs.conn_cbs.conn_handshake_finished =
      xqc_cli_hq_conn_handshake_finished;
      ap_cbs.conn_cbs.conn_ping_acked = xqc_cli_hq_conn_ping_acked_notify;
      // stream
      ap_cbs.stream_cbs.stream_read_notify = xqc_client_stream_write_notify;
      ap_cbs.stream_cbs.stream_write_notify = xqc_client_stream_write_notify;
      ap_cbs.stream_cbs.stream_create_notify = stream_create_notify_callback;
      ap_cbs.stream_cbs.stream_closing_notify = stream_closing_notify_callback;
      ap_cbs.stream_cbs.stream_close_notify = xqc_client_stream_close_notify;

      // dgram
      // ap_cbs.dgram_cbs.datagram_write_notify =
      // datagram_write_notify_callback; ap_cbs.dgram_cbs.datagram_read_notify =
      // datagram_read_notify_callback; ap_cbs.dgram_cbs.datagram_acked_notify =
      // datagram_acked_notify_callback; ap_cbs.dgram_cbs.datagram_lost_notify =
      // datagram_lost_notify_callback;
      // ap_cbs.dgram_cbs.datagram_mss_updated_notify =
      //     datagram_mss_updated_notify_callback;
    }

    ret = xqc_engine_register_alpn(ctx->engine, XQC_ALPN_TRANSPORT, 9, &ap_cbs, nullptr);
    if (ret != XQC_OK) {
        xqc_engine_unregister_alpn(ctx->engine, XQC_ALPN_TRANSPORT,9);
        printf("engine register alpn error, ret:%d \n", ret);
        return XQC_ERROR;
    }else{
        printf("engine register alpn:%s,alpn_len:%d,ret:%d \n", XQC_ALPN_TRANSPORT, 9, ret);
    }

    return XQC_OK;
}

//创建引擎
int xqc_cli_init_xquic_engine(xqc_cli_ctx_t *ctx, xqc_cli_client_args_t *args){
     now_time = xqc_now();
    /* init engine ssl config */
    xqc_engine_ssl_config_t engine_ssl_config = {0};   
    engine_ssl_config.ciphers = XQC_TLS_CIPHERS;
    engine_ssl_config.groups = XQC_TLS_GROUPS;
    
    xqc_transport_callbacks_t transport_cbs;
    // xqc_cli_init_engine_ssl_config(&engine_ssl_config, args);

    /* init engine callbacks */
    xqc_engine_callback_t callback;
    memset(&callback, 0, sizeof(xqc_engine_callback_t));
    xqc_cli_init_callback(&callback, &transport_cbs, args);

    xqc_config_t config;
    int ret = xqc_engine_get_default_config(&config, XQC_ENGINE_CLIENT);
    if (ret < 0) {
        printf("xqc_engine_get_default_config failed: %d \n", ret);
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
        printf("init alpn ctx error!\n" );
        return -1;
    }
    return XQC_OK;
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


 // 构造函数
ConnCtx::ConnCtx(){};
//析构函数
ConnCtx::~ConnCtx(){
    if (local_addr_v4) {
        free(local_addr_v4);
        local_addr_v4 = nullptr;
    }
    if (local_addr_v6) {
        free(local_addr_v6);
        local_addr_v6 = nullptr;
    }
    if (peer_addr_v4) {
        free(peer_addr_v4);
        peer_addr_v4 = nullptr;
    }
    if (peer_addr_v6) {
        free(peer_addr_v6);
        peer_addr_v6 = nullptr;
    }
    if (fd != 0) {
        close(fd);
        fd = 0;
    }
}

    /**
     * 解析服务器地址
     * @param cfg
     * @return
     */
void ConnCtx::client_parse_server_addr(const std::string &server_addr, int server_port) {
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
        this->peer_addrlen = result->ai_addrlen;
        if (result->ai_family == AF_INET6) {
        this->peer_addr_v6 =
            (sockaddr_in6 *)malloc(sizeof(struct sockaddr_in6));
        memcpy(this->peer_addr_v6, result->ai_addr, result->ai_addrlen);
        inet_ntop(result->ai_family, &(this->peer_addr_v6->sin6_addr),
        this->server_addr_str, sizeof(this->server_addr_str));
        } else if (result->ai_family == AF_INET) {
        this->peer_addr_v4 =
            (sockaddr_in *)malloc(sizeof(struct sockaddr_in));
        memcpy(this->peer_addr_v4, result->ai_addr, result->ai_addrlen);
        inet_ntop(result->ai_family, &(this->peer_addr_v4->sin_addr),
        this->server_addr_str, sizeof(this->server_addr_str));
        } else {
        assert(false);
        }
        assert(this->fd == 0);
        this->fd = socket(result->ai_family, SOCK_DGRAM, 0);
        freeaddrinfo(result);
        cout << "ClientUserConnCreate server:"
        << server_addr << ", addr: " << this->server_addr_str
        << ", port: " << server_port << " , fd:" << this->fd << endl;

        if (this->fd < 0) {
        this->fd = 0;
        printf( "create socket failed, errno: %s", get_sys_errno());
        return;
        }
    printf("zyffff client_parse_server_addr end \n");
}

    // 创建连接上下文
 std::unique_ptr<ConnCtx> ConnCtx::create_connCtx(xqc_engine_t *engine, xqc_cli_client_args_t *args, const std::string &server_addr, 
        int server_port) {
        // 创建连接上下文
        std::unique_ptr<ConnCtx> conn_ctx(new ConnCtx);
        conn_ctx->engine = engine;

        // 解析ip、端口
        conn_ctx->client_parse_server_addr(server_addr, server_port);
        if (conn_ctx->peer_addr_v4 == nullptr && conn_ctx->peer_addr_v6 == nullptr) { // 检查解析结果是否有效
            printf("Failed to parse server address\n");
            return nullptr;
        }
        printf("after init client_create_socket \n");
        sockaddr *local_addr;
        socklen_t local_len;

        if (conn_ctx->peer_addr_v6) {
        conn_ctx->local_addr_v6 =
            (sockaddr_in6 *)malloc(sizeof(struct sockaddr_in6));
        local_len = sizeof(struct sockaddr_in6);
        local_addr = (sockaddr *)conn_ctx->local_addr_v6;

        } else if (conn_ctx->peer_addr_v4) {
        conn_ctx->local_addr_v4 =
            (sockaddr_in *)malloc(sizeof(struct sockaddr_in));
        local_len = sizeof(struct sockaddr_in);
        local_addr = (sockaddr *)conn_ctx->local_addr_v4;
        } else {
        assert(false);
        }
        conn_ctx->local_addrlen = local_len;
        int ret = getsockname(conn_ctx->fd, local_addr, &local_len);
        if (ret < 0) {
            printf("getsockname error, errno: %s\n", get_sys_errno());
        }
    #ifdef WEBRTC_WINDOWS
        if (ioctlsocket(user_conn->fd, FIONBIO, &flags) == SOCKET_ERROR) {
        }
    #else
        if (fcntl(conn_ctx->fd, F_SETFL, O_NONBLOCK) == -1) {
          printf("set socket nonblock failed, errno: %s\n", get_sys_errno());
        }
    #endif
        int size = 1 * 1024 * 1024;
        if (setsockopt(conn_ctx->fd, SOL_SOCKET, SO_RCVBUF, &size, sizeof(int)) <
            0) {
                printf("setsockopt failed, errno: %s\n", get_sys_errno());
        }
    
        if (setsockopt(conn_ctx->fd, SOL_SOCKET, SO_SNDBUF, &size, sizeof(int)) <
            0) {
                printf("setsockopt failed, errno %s\n",get_sys_errno());
        }
        printf(" create_connection end\n");         
        return conn_ctx;
    }

    /**
     * 初始化链接设置
     * @param args
     */
void ConnCtx::init_connection_settings(xqc_conn_settings_t *settings) {

        /* 拥塞控制*/
        xqc_cong_ctrl_callback_t cong_ctrl;
        cong_ctrl = xqc_bbr_cb;

        memset(settings, 0, sizeof(xqc_conn_settings_t));
        settings->pacing_on = 0;
        settings->ping_on = 0;
        settings->proto_version = XQC_VERSION_V1;
        settings->cong_ctrl_callback = cong_ctrl;
        settings->cc_params.customize_on = 1;
        settings->cc_params.init_cwnd = 32;
        settings->cc_params.cc_optimization_flags = 0;
        settings->cc_params.copa_delta_ai_unit = 1.0;
        settings->cc_params.copa_delta_base = 0.05;
        settings->spurious_loss_detect_on = 0;
        settings->keyupdate_pkt_threshold = 0;
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
//创建连接
void ConnCtx::create_conn() {
    assert(engine && xqc_cid == nullptr);
        // 初始化连接配置
        xqc_conn_settings_t conn_settings;
        this->init_connection_settings(&conn_settings);
        printf("init connection settings \n");

        /* init ssl config*/
        unsigned char token[XQC_MAX_TOKEN_LEN] = {0};
        int token_len = XQC_MAX_TOKEN_LEN;
        xqc_conn_ssl_config_t conn_ssl_config;
        { memset(&conn_ssl_config, 0, sizeof(conn_ssl_config)); }
        
    struct sockaddr *addr = nullptr;

    if (peer_addr_v4 != nullptr) {
        addr = (struct sockaddr *)peer_addr_v4;
    } else if (peer_addr_v6 != nullptr) {
        addr = (struct sockaddr *)peer_addr_v6;
    } else {
        assert(false);
    }

    const xqc_cid_t *cid = xqc_connect(
        engine, &conn_settings, token, token_len, server_addr_str, false,
        &conn_ssl_config, addr, peer_addrlen, XQC_ALPN_TRANSPORT, this);
    if (cid == nullptr) {
        return;
    }
    xqc_cid = (xqc_cid_t *)malloc(sizeof(xqc_cid_t));
    memcpy(xqc_cid, cid, sizeof(xqc_cid_t));
    cout << "create cid(" << (void *)cid << ") success, user data: " << (void *)this << endl;
    }

//创建流
void ConnCtx::create_stream() {
        
    assert(engine && xqc_cid && stream == nullptr);
    if (stream != nullptr) {
        return;
    }
    stream = xqc_stream_create(engine, xqc_cid, nullptr, this);
    if (nullptr == stream) {
        printf("create transport stream error\n");
    }
    printf("create xqc stream create success, stream id: %d \n", xqc_stream_id(stream));
}

//销毁流
void ConnCtx::dsestory_stream() {
    if (stream == nullptr) {
        return;
    }
        printf("destory stream, id: %d \n", xqc_stream_id(stream));
        xqc_stream_close(stream);
        stream = nullptr;
}
//销毁连接
void ConnCtx::destory_conn() { // 修正拼写错误为destroy_conn()
    
    if (xqc_cid == nullptr) {
        printf("DestoryXqcCid skip\n");
        return;
    }
    dsestory_stream();
    printf("close conn\n");
    xqc_conn_close(engine, xqc_cid);
    xqc_engine_main_logic(engine);
    free(xqc_cid);
    xqc_cid = nullptr;
}

//发送数据,不关闭fd
ssize_t ConnCtx::send_msg(unsigned char *msg, size_t len, int fin) {
    if (fd == 0 || xqc_cid == nullptr || stream == nullptr) {
        printf("xquic send msg error\n");
        return -1;
        }
        ssize_t ret = xqc_stream_send(stream, msg, len, fin);
    cout << "xqc_stream_send msg=" << (const char *)msg << ", ret: " << ret << endl;
    return ret;
}

//发送数据,关闭fd
ssize_t ConnCtx::send_msg_fin(unsigned char *msg, size_t len) {
    int fin = 1;
    return send_msg(msg, len, fin);
}

//接收数据
void ConnCtx::receive_msg() {
    printf("receive_msg \n");
    if (this == nullptr || fd == 0) {
        return;
    }
    int sockfd = fd;
    fd_set read_fds;
    struct timeval timeout;
    
    FD_ZERO(&read_fds);
    FD_SET(sockfd, &read_fds);
    
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    
    int activity = select(sockfd + 1, &read_fds, nullptr, nullptr, &timeout);
    if (activity < 0) {
        printf("select error\n");
        return;
    } else if (activity == 0) {
        printf( "select time out\n");
        return;
    } else if (!FD_ISSET(sockfd, &read_fds)) {
        printf("not recv server msg\n");
        return;
    }
    
    unsigned char packet_buf[1500];
    struct sockaddr *local_addr = nullptr;
    struct sockaddr *peer_addr = nullptr;
    if (peer_addr_v4 != nullptr) {
        local_addr = (struct sockaddr *)local_addr_v4;
        peer_addr = (struct sockaddr *)peer_addr_v4;
    } else if (peer_addr_v6 != nullptr) {
        peer_addr = (struct sockaddr *)peer_addr_v6;
        local_addr = (struct sockaddr *)local_addr_v6;
    } else {
        assert(false);
    }
    ssize_t recv_sum = 0;
    ssize_t recv_size = 0;
    do {
        errno = 0;
        recv_size = recvfrom(sockfd, (void *)packet_buf, sizeof(packet_buf), 0,
                            peer_addr, &peer_addrlen);
        xqc_usec_t now_time = xqc_now();
        if (recv_size < 0 && get_sys_errno() == EAGAIN) {
        break;
        }
        if (recv_size < 0) {
        cout << "recv size < 0 size: " << recv_size << ", err: "
                                            << strerror(get_sys_errno()) << endl;
        break;
        }
        /* if recv_size is 0, break while loop, */
        if (recv_size == 0) {
        printf( "recv size is 0 break loop.\n");
        break;
        }
        printf("Socket is readable recv_size: %d \n" , recv_size);
        recv_sum += recv_size;
        xqc_int_t ret = xqc_engine_packet_process(
        engine, packet_buf, recv_size, local_addr, local_addrlen,
            peer_addr, peer_addrlen, now_time, nullptr);
            printf( "xqc_engine_packet_process: %d \n", ret);
        if (ret != XQC_OK) {
        printf( "xqc_client_read_handler: packet process err, ret: %d\n", ret);
        break;
        }
    } while (recv_size > 0);
    
    printf("xqc engine recv finish, socket recv sum size: %d \n", recv_sum);
    xqc_engine_finish_recv(engine);
    
}

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
    // xqc_cli_init_ctx(ctx, args);

   //  /* init engine */
   xqc_cli_init_xquic_engine(ctx, args);

   // 创建连接
   std::unique_ptr<ConnCtx> conn_ctx_;
   conn_ctx_ = ConnCtx::create_connCtx(ctx->engine, args, "43.231.160.216", 8443);
   conn_ctx_->create_conn();

   //创建流
   conn_ctx_->create_stream();

   //接收数据
   std::thread t1(testReceive, std::ref(conn_ctx_));

   //发送数据
   for (size_t i = 0; i < 5; i++)
   {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    string msg = "hello world";
    conn_ctx_->send_msg((unsigned char *)msg.data(), msg.size());
   }

   //等待30s
   std::this_thread::sleep_for(std::chrono::seconds(30));
  
   //销毁连接
   if (conn_ctx_){
     conn_ctx_->destory_conn();
   }
   
   //释放资源
    xqc_cli_free_ctx(ctx);

   return 0;
}