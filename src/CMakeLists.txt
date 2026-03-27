# 生成SRC_FILES
file(GLOB SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cc)

# 生成PROTO_SRC
file(GLOB PROTO_SRC ${CMAKE_CURRENT_SOURCE_DIR}/*.pb.cc )

# 编译出krpc_core 静态库 
add_library(krpc_core STATIC ${SRC_FILES} ${PROTO_SRC})

# 链接外部依赖库 
# PUBLIC : 未来任何依赖krpc_code的其他可执行程序 都会自动继承链接 protobuf  无需重复手写

target_link_libraries(krpc_core PUBLIC protobuf)
# 告诉编译器 在编译krpc_code的时 在include下查找头文件
target_include_directories(krpc_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# 设置编译选项 -Wall : 开启编译警告
target_compile_options(krpc_core PRIVATE -std=c++11 -Wall)
