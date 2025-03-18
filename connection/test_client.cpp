#include <iostream>
#include "xquic_common.h"

using namespace std;


int main() {
   //初始化引擎
   //创建连接
   //创建流
   //发送数据

   //销毁流
   //销毁连接
   //销毁引擎

     /* init env if necessary */
    //  xqc_platform_init_env();
    
    //  /* get input client args */
     xqc_cli_client_args_t *args;
     memset(args, 1, sizeof(xqc_cli_client_args_t));
     //设置默认参数
     xqc_cli_init_args(args);
     //设置信令访问地址、端口
     client_parse_server_addr(args->net_cfg,"127.0.0.1",8188);
 
    //  /* init client ctx */
     xqc_cli_ctx_t *ctx;
     memset(ctx, 1, sizeof(xqc_cli_client_args_t));
     xqc_cli_init_ctx(ctx, args);
 
    //  /* init engine */
    xqc_cli_init_xquic_engine(ctx, args);
 
    // 创建连接
    create_connection(ctx->, args)
    //  event_base_dispatch(ctx->eb);

    //释放资源
     xqc_engine_destroy(ctx->engine);
     xqc_cli_free_ctx(ctx);



    return 0;
}