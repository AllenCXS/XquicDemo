#include<iostream>
#include<xquic_common.h>

using namespace std;

#define XQC_OK      0
#define XQC_ERROR   -1

#define XQC_PACKET_TMP_BUF_LEN  1600
#define MAX_BUF_SIZE            (100*1024*1024)
#define XQC_INTEROP_TLS_GROUPS  "X25519:P-256:P-384:P-521"
#define MAX_PATH_CNT            2

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
    xqc_demo_cli_client_args_t* args){
    static xqc_engine_callback_t callback = {
        .log_callbacks = {
            .xqc_log_write_err = xqc_demo_cli_write_log_file,
            .xqc_log_write_stat = xqc_demo_cli_write_log_file,
            .xqc_qlog_event_write = xqc_demo_cli_write_qlog_file,
        },
        .keylog_cb = xqc_demo_cli_keylog_cb,
        .set_event_timer = xqc_demo_cli_set_event_timer,
    };

    static xqc_transport_callbacks_t tcb = {
        .write_socket = xqc_demo_cli_write_socket,
        .write_socket_ex = xqc_demo_cli_write_socket_ex,
        .save_token = xqc_demo_cli_save_token, /* save token */
        .save_session_cb = xqc_demo_cli_save_session_cb,
        .save_tp_cb = xqc_demo_cli_save_tp_cb,
        .conn_update_cid_notify = xqc_demo_cli_conn_update_cid_notify,
        .ready_to_create_path_notify = xqc_demo_cli_conn_create_path,
        .path_removed_notify = xqc_demo_cli_path_removed,
    };

    *cb = callback;
    *transport_cbs = tcb;
}
//
int xqc_cli_init_alpn_ctx(xqc_cli_ctx_t *ctx)
{
    int ret = 0;

    xqc_hq_callbacks_t hq_cbs = {
        .hqc_cbs = {
            .conn_create_notify = xqc_demo_cli_hq_conn_create_notify,
            .conn_close_notify = xqc_demo_cli_hq_conn_close_notify,
        },
        .hqr_cbs = {
            .req_close_notify = xqc_demo_cli_hq_req_close_notify,
            .req_read_notify = xqc_demo_cli_hq_req_read_notify,
            .req_write_notify = xqc_demo_cli_hq_req_write_notify,
        }
    };

    /* init hq context */
    ret = xqc_hq_ctx_init(ctx->engine, &hq_cbs);
    if (ret != XQC_OK) {
        printf("init hq context error, ret: %d\n", ret);
        return ret;
    }

    xqc_h3_callbacks_t h3_cbs = {
        .h3c_cbs = {
            .h3_conn_create_notify = xqc_demo_cli_h3_conn_create_notify,
            .h3_conn_close_notify = xqc_demo_cli_h3_conn_close_notify,
            .h3_conn_handshake_finished = xqc_demo_cli_h3_conn_handshake_finished,
        },
        .h3r_cbs = {
            .h3_request_create_notify = xqc_demo_cli_h3_request_create_notify,
            .h3_request_close_notify = xqc_demo_cli_h3_request_close_notify,
            .h3_request_read_notify = xqc_demo_cli_h3_request_read_notify,
            .h3_request_write_notify = xqc_demo_cli_h3_request_write_notify,
        }
    };

    /* init http3 context */
    ret = xqc_h3_ctx_init(ctx->engine, &h3_cbs);
    if (ret != XQC_OK) {
        printf("init h3 context error, ret: %d\n", ret);
        return ret;
    }

    return ret;
}

//创建引擎
int xqc_cli_init_xquic_engine(xqc_cli_ctx_t *ctx, xqc_cli_client_args_t *args){
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

    ctx->engine = xqc_engine_create(XQC_ENGINE_CLIENT, &config, 
                                     &engine_ssl_config, &callback, &transport_cbs, ctx);
    if (ctx->engine == NULL) {
        cout << "xqc_engine_create error" << endl;
        return XQC_ERROR;
    }

    if (xqc_cli_init_alpn_ctx(ctx) < 0) {
        cout << "init alpn ctx error!" << endl;
        return -1;
    }
    return XQC_OK;
}

//创建连接

//创建流

//发送数据

//接收数据

//关闭流

//关闭连接

//关闭引擎