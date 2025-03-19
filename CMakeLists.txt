# 设置项目名称
cmake_minimum_required(VERSION 3.10)
project(WebSocketClient)

# 设置C++标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_VERBOSE_MAKEFILE on)


# 包含xquic的头文件路径
include_directories("/Users/lizhi/Documents/workSpace/XquicDemo/third_party/libxquic/mac/arm64/include/" 
"/Users/lizhi/Documents/workSpace/XquicDemo/connection/include/")

# 添加可执行文件
add_executable(xquic_client connection/drtc_xquic_client.cpp
    connection/include/xquic_engine_callbacks.cpp 
    connection/include/xquic_hq_callbacks.cpp
    connection/include/xquic_socket.cpp
    connection/include/xquic_transport_callbacks.cpp
    connection/include/xquic_common.cpp)

# 链接xquic库
target_link_libraries(xquic_client "/Users/lizhi/Documents/workSpace/XquicDemo/third_party/libxquic/mac/arm64/lib/libxquic.dylib")