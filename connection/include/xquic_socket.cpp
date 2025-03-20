#include <xquic_socket.h>

/**
 * 创建socket 链接
 * @param user_conn
 * @param cfg
 * @return
 */
int client_create_socket(xqc_cli_user_conn_t *user_conn) {
    int size = 0;
    int fd = 0;
    int ret;

    // struct sockaddr *addr = (struct sockaddr *) &cfg->addr;
    // fd = socket(addr->sa_family, SOCK_DGRAM, 0);
    // if (fd < 0) {
    //     printf("create socket failed,erron:%d \n", errno);
    //     return -1;
    // }

    // if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1) {
    //     printf("set socket nonblock failed,errno:%d", errno);
    //     goto err;
    // }

    // size = 1 * 1024 * 1024;
    // if (setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &size, sizeof(int)) < 0) {
    //     printf("setsockopt failed,errno:%d", errno);
    //     goto err;
    // }

    // if (setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &size, sizeof(int)) < 0) {
    //     printf("setsockopt failed,errno:%d", errno);
    //     goto err;
    // }

    // //connect can use send（not sendto） to improve performance
    // if (connect(fd, (struct sockaddr *) addr, cfg->addr_len) < 0) {
    //     printf("connect socket failed, errno: %d\n", errno);
    //     goto err;
    // }

    return fd;

    err:
    close(fd);
    return -1;
}