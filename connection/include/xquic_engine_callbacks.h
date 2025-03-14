#ifndef XQUIC_ENGINE_CALLBACKS_H
#define XQUIC_ENGINE_CALLBACKS_H

#include<xquic_common.h>

void xqc_cli_write_log_file(xqc_log_level_t lvl, const void *buf, size_t size, void *engine_user_data);
void xqc_cli_set_event_timer(xqc_usec_t wake_after, void *eng_user_data);

#endif