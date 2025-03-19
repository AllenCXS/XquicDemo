BASE_PATH=`pwd`
OS=mac

compile_client() {
    libname=client
    echo "编译: ${libname}"

    path=${BASE_PATH}
    if [ ! -d "$path" ]; then
        echo "目录不存在: $path"
        exit 1
    fi
    build_path=${BASE_PATH}/build_${OS}/build_${libname}
    rm -rf ${build_path}
    mkdir -p ${build_path}

    cd ${path}
    rm -rf build
    mkdir -p build && cd build
    
    cmake -DCMAKE_INSTALL_PREFIX=${build_path}      \
      -DBUILD_SHARED_LIBS=0                     \
      -DCMAKE_C_FLAGS="-fPIC -fsanitize=address" \
      -DCMAKE_CXX_FLAGS="-fPIC -fsanitize=address" \
      -DCMAKE_CXX_STANDARD_LIBRARY=libc++       \
      -DPLATFORM=mac                            \
      -DCMAKE_SYSTEM_PROCESSOR=arm64            \
      -DCMAKE_OSX_ARCHITECTURES=arm64           \
      -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0        \
      -DCMAKE_PREFIX_PATH=/Users/lizhi/Documents/workSpace/XquicDemo/third_party/libxquic/mac/arm64 \
      ..
          ..
    make
    make install
    if [ $? -ne 0 ]; then
        echo "编译 ${libname} 失败"
        exit 1
    fi
}

compile_client