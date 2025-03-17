#ifndef XQUIC_TRANSPORT_CALLBACKS_H
#define XQUIC_TRANSPORT_CALLBACKS_H

#include<xquic_common.h>

#define SESSION_TICKET_FILE         "session_ticket"
#define TRANSPORT_PARAMS_FILE       "transport_params"
#define TOKEN_FILE                  "token"

ssize_t
xqc_cli_write_socket(const unsigned char *buf, size_t size, const struct sockaddr *peer_addr,
    socklen_t peer_addrlen, void *conn_user_data);

ssize_t
xqc_cli_write_socket_ex(uint64_t path_id, const unsigned char *buf, size_t size,
    const struct sockaddr *peer_addr, socklen_t peer_addrlen, void *conn_user_data);    

void xqc_cli_save_token(const unsigned char *token, uint32_t token_len, void *conn_user_data);
void xqc_cli_save_session_cb(const char *data, size_t data_len, void *conn_user_data);
    
void xqc_cli_save_tp_cb(const char *data, size_t data_len, void *conn_user_data);
    
void xqc_cli_conn_update_cid_notify(xqc_connection_t *conn, const xqc_cid_t *retire_cid,
        const xqc_cid_t *new_cid, void *user_data);


#endif