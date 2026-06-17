# 指定CMake最低版本要求
cmake_minimum_required(VERSION 3.16)
# 项目名称
project(mini-db)

# 设置C++标准为C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# 设置编译选项：开启所有警告，支持多线程
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pthread")

# 添加头文件目录
include_directories(src)

# 生成服务器可执行文件
add_executable(minidb-server
    src/server/server.cpp
)

# 生成客户端可执行文件
add_executable(minidb-client
    src/client/client.cpp
)

# 生成单元测试可执行文件
add_executable(test-container
    tests/test_container.cpp
)

add_executable(test-storage
    tests/test_storage.cpp
)

add_executable(test-parser
    tests/test_parser.cpp
)

add_executable(test-dml
    tests/test_dml.cpp
)