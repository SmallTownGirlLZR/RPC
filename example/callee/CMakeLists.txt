file(GLOB SERVER_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/*.cc)

file(GLOB PROTO_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/../*.pb.cc)

add_executable(server ${SERVER_SRCS} ${PROTO_SRCS})

target_link_library(server PRIVATE krpc_core ${LIBS})

target_compile_options(server PRIVATE -std=c++11 =Wall)

set_target_properties(server PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)1

