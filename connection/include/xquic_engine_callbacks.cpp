#include<xquic_engine_callbacks.h>

void xqc_cli_write_log_file(xqc_log_level_t lvl, const void *buf, size_t size, void *engine_user_data){
    printf("xqc_engine_callback.log_callbacks.xqc_log_write_err: %s\n", buf);
}

void xqc_cli_set_event_timer(xqc_usec_t wake_after, void *eng_user_data){
    xqc_cli_ctx_t *ctx = (xqc_cli_ctx_t *) eng_user_data;
    printf("xqc_engine_wakeup_after %llu us, now %llu\n", wake_after, xqc_now());

    struct timeval tv;
    tv.tv_sec = wake_after / 1000000;
    tv.tv_usec = wake_after % 1000000;
    //TODO 
    // event_add(ctx->ev_engine, &tv);

}