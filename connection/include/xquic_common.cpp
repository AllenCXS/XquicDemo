#include "xquic_common.h"

const char *line_break = "\n";
size_t READ_FILE_BUF_LEN = 2 *1024 * 1024;

int get_sys_errno() {
    int err = 0;
#ifdef WEBRTC_WINDOWS
    err = WSAGetLastError();
#else
    err = errno;
#endif
    return err;
}

uint64_t xqc_now() {
    /* get microsecond unit time */
    struct timeval tv;
    gettimeofday(&tv, NULL);
    xqc_usec_t ul = tv.tv_sec * (xqc_usec_t)1000000 + tv.tv_usec;
    return ul;
}

char method_s[][16] = {
    {"GET"}, 
    {"POST"}
};