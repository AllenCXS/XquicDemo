#ifndef XQUIC_HQ_CALLBACKS_H
#define XQUIC_HQ_CALLBACKS_H
#include<xquic_common.h>

int xqc_cli_hq_conn_create_notify(xqc_connection_t *conn, const xqc_cid_t *cid, void *user_data, void *conn_proto_data);

int xqc_cli_hq_conn_close_notify(xqc_connection_t *conn, const xqc_cid_t *cid, void *user_data, void *conn_proto_data);

void xqc_cli_hq_conn_handshake_finished(xqc_connection_t *conn, void *user_data, void *conn_proto_data);

void xqc_cli_hq_conn_ping_acked_notify(xqc_connection_t *conn, const xqc_cid_t *cid, void *ping_user_data, void *user_data,void *conn_proto_data);

int xqc_client_stream_read_notify(xqc_stream_t *stream, void *user_data);
  
int xqc_client_stream_write_notify(xqc_stream_t *stream, void *user_data);

int xqc_client_stream_close_notify(xqc_stream_t *stream, void *user_data); 

#endif