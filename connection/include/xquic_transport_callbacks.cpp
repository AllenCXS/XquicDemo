#include<xquic_transport_callbacks.h>
#include<fcntl.h>


ssize_t
xqc_cli_write_socket(const unsigned char *buf, size_t size, const struct sockaddr *peer_addr,
    socklen_t peer_addrlen, void *conn_user_data){
    printf("transport_cbs.write_socket\n");

    ConnCtx *THIS = (ConnCtx *)conn_user_data;
    int fd = THIS->fd;
    ssize_t res = -1;
    if (fd < 0) {
        printf("write socket fd is error.\n");
        return res;
    }
    do {
        errno = 0;
        res = ::sendto(fd, buf, size, 0, peer_addr, peer_addrlen);

        if (res < 0) {
        if (errno == EAGAIN) {
            res = XQC_SOCKET_EAGAIN;
        } else {
            res = XQC_SOCKET_ERROR;
        }
        if (errno == EMSGSIZE) {
            res = size;
        }
        } else {
            printf( "Socket is writable, size: %d, res: %d", size, res);
        }
    } while ((res < 0) && (errno == EINTR));

    return res;
}

ssize_t
xqc_cli_write_socket_ex(uint64_t path_id, const unsigned char *buf, size_t size,
    const struct sockaddr *peer_addr, socklen_t peer_addrlen, void *conn_user_data)
{
    return xqc_cli_write_socket(buf, size, peer_addr, peer_addrlen,
        conn_user_data);
}

//修改
void xqc_cli_save_token(const unsigned char *token, uint32_t token_len, void *conn_user_data){
    // ConnCtx *user_conn = (ConnCtx *)conn_user_data;

    // int fd = open(TOKEN_FILE, O_TRUNC | O_CREAT | O_WRONLY, 0666);
    // if (fd < 0) {
    //     return;
    // }

    // ssize_t n = write(fd, token, token_len);
    // if (n < token_len) {
    //     close(fd);
    //     return;
    // }
    // close(fd);
}

 void
 xqc_cli_save_session_cb(const char *data, size_t data_len, void *conn_user_data)
 {
    // ConnCtx *user_conn = (ConnCtx *)conn_user_data;
 
    //  FILE * fp  = fopen(SESSION_TICKET_FILE, "wb");
    //  int write_size = fwrite(data, 1, data_len, fp);
    //  if (data_len != write_size) {
    //      printf("save _session_cb error\n");
    //      fclose(fp);
    //      return;
    //  }
    //  fclose(fp);
    //  return;
 }

 void
xqc_cli_save_tp_cb(const char *data, size_t data_len, void *conn_user_data)
{
    // ConnCtx *user_conn = (ConnCtx *)conn_user_data;
    // FILE * fp = fopen(TRANSPORT_PARAMS_FILE, "wb");
    // if (NULL == fp) {
    //     printf("open file for transport parameter error\n");
    //     return;
    // }

    // int write_size = fwrite(data, 1, data_len, fp);
    // if (data_len != write_size) {
    //     fclose(fp);
    //     return;
    // }
    // fclose(fp);
    // return;
}


void
xqc_cli_conn_update_cid_notify(xqc_connection_t *conn, const xqc_cid_t *retire_cid,
    const xqc_cid_t *new_cid, void *user_data)
{
    ConnCtx *user_conn = (ConnCtx *)user_data;
    memcpy(&user_conn->xqc_cid, new_cid, sizeof(*new_cid));
}
