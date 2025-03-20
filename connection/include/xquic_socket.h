#ifndef XQUIC_SOCKET_H
#define XQUIC_SOCKET_H
#include<xquic_common.h>

/**
 * 创建socket 链接
 * @param user_conn
 * @param cfg
 * @return
 */
int client_create_socket(xqc_cli_user_conn_t *user_conn);

#endif