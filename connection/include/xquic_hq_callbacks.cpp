#include<xquic_hq_callbacks.h>

//xqc_connection_t *conn, const xqc_cid_t *cid, void *conn_user_data, void *conn_proto_data
int xqc_cli_hq_conn_create_notify(xqc_connection_t *conn, const xqc_cid_t *cid, void *user_data, void *conn_proto_data)
{
    xqc_cli_user_conn_t *user_conn = (xqc_cli_user_conn_t *)user_data;
    xqc_conn_set_alp_user_data(conn, user_conn);
    int ret = xqc_conn_is_ready_to_send_early_data(conn);
    printf("xqc_conn_is_ready_to_send_early_data:%d\n", ret);
    return 0;
}

int xqc_cli_hq_conn_close_notify(xqc_connection_t *conn, const xqc_cid_t *cid, void *user_data, void *conn_proto_data)
{
    xqc_cli_user_conn_t *user_conn = (xqc_cli_user_conn_t *) user_data;
    // xqc_demo_cli_on_task_finish(user_conn->ctx, user_conn->task);
    printf("ap_cbs.conn_cbs.conn_close_notify data:%p\n", user_data);
    free(user_conn);
    return 0;
}

void xqc_cli_hq_conn_handshake_finished(xqc_connection_t *conn, void *user_data, void *conn_proto_data) {
    xqc_cli_user_conn_t *user_conn = (xqc_cli_user_conn_t *) user_data;
    xqc_conn_stats_t stats = xqc_conn_get_stats(user_conn->engine, user_conn->xqc_cid);
    char ortt_info[100] = {0};
    char info[50] = "without 0-RTT";
    if (stats.early_data_flag == XQC_0RTT_ACCEPT) {
        strcpy(info, "0-RTT was accepted");
    } else if (stats.early_data_flag == XQC_0RTT_REJECT) {
        strcpy(info, "0-RTT was rejected");
    }
    sprintf(ortt_info, ">>>>>>>> 0rtt_flag:%d(%s)<<<<<<<<<", stats.early_data_flag, info);
    printf("ortt_info %s", ortt_info);
    //发送ping包
    xqc_int_t ret = xqc_conn_send_ping(user_conn->engine, user_conn->xqc_cid, nullptr);
    printf("send ping ret:%d\n", ret);
    cout << "datagram mss: "
      << xqc_datagram_get_mss(conn) << ", DCID: "
      << xqc_dcid_str_by_scid(user_conn->engine, user_conn->xqc_cid)
      << ", SCID: " << xqc_scid_str(user_conn->engine, user_conn->xqc_cid)
      << ", send ping: " << ret << endl;
}

void xqc_cli_hq_conn_ping_acked_notify(xqc_connection_t *conn, const xqc_cid_t *cid,
    void *ping_user_data, void *user_data, void *conn_proto_data) {
    // xqc_cli_user_conn_t *user_conn = (xqc_cli_user_conn_t *) user_data;
    // uint64_t recv_time = xqc_now();
    printf("ap_cbs.conn_cbs.conn_ping_acked data:%p\n", user_data);
}

// stream
int xqc_client_stream_read_notify(xqc_stream_t *stream, void *user_data) {
    printf("ap_cbs.stream_cbs.stream_read_notify \n");
    ssize_t read;
    ssize_t read_sum = 0;
  
    unsigned char buff[4096] = {0};
    size_t buff_size = 4096;
    unsigned char fin = 0;
    do {
      read = xqc_stream_recv(stream, buff, buff_size, &fin);
        cout << "[cxs]xqc_stream_recv read: " << read << ", fin: " << fin << ", buffer: " << buff << endl;
  
      if (read == -XQC_EAGAIN) {
        break;
  
      } else if (read < 0) {
        printf("xqc_stream_recv error %zd\n", read);
        return 0;
      }
      read_sum += read;
    } while (read > 0 && !fin);
    cout << "xqc_stream_recv read sum: " << read_sum << ", fin: " << fin << endl;
    return 0;
  }
  
  int xqc_client_stream_write_notify(xqc_stream_t *stream, void *user_data) {
    printf("ap_cbs.stream_cbs.stream_write_notify \n");
    return 0;
  }

  int xqc_client_stream_close_notify(xqc_stream_t *stream, void *user_data) {
    printf("ap_cbs.stream_cbs.stream_close_notify \n");
    return 0;
  }