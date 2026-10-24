cmake_minimum_required(VERSION 3.15)
project(FileProcessorServer CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GRPC REQUIRED grpc++)
pkg_check_modules(PROTOBUF REQUIRED protobuf)

set(PROTO_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../proto")
set(PROTO_FILE "${PROTO_DIR}/file_processor.proto")

set(PROTO_SRC "${CMAKE_CURRENT_BINARY_DIR}/file_processor.pb.cc")
set(PROTO_HDR "${CMAKE_CURRENT_BINARY_DIR}/file_processor.pb.h")
set(GRPC_SRC  "${CMAKE_CURRENT_BINARY_DIR}/file_processor.grpc.pb.cc")
set(GRPC_HDR  "${CMAKE_CURRENT_BINARY_DIR}/file_processor.grpc.pb.h")

set(PROTOC_COMPILER "/usr/bin/protoc")
set(GRPC_CPP_PLUGIN "/usr/bin/grpc_cpp_plugin")

add_custom_command(
  OUTPUT ${PROTO_SRC} ${PROTO_HDR} ${GRPC_SRC} ${GRPC_HDR}
  COMMAND ${PROTOC_COMPILER}
  ARGS --grpc_out="${CMAKE_CURRENT_BINARY_DIR}"
       --cpp_out="${CMAKE_CURRENT_BINARY_DIR}"
       -I "${PROTO_DIR}"
       --plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN}
       "${PROTO_FILE}"
  DEPENDS "${PROTO_FILE}"
)

include_directories("${CMAKE_CURRENT_BINARY_DIR}")

add_executable(server server.cc ${PROTO_SRC} ${GRPC_SRC})

execute_process(COMMAND pkg-config --libs --static grpc++ OUTPUT_VARIABLE GRPC_ALL_LIBS OUTPUT_STRIP_TRAILING_WHITESPACE)
separate_arguments(GRPC_ALL_LIBS)
target_link_libraries(server ${GRPC_ALL_LIBS} protobuf pthread)
