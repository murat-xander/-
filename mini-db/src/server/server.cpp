#include <iostream>
#include <thread>
#include <mutex>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "../engine/engine.hpp"
#include "../common/protocol.hpp"

// 全局互斥锁：保护引擎的并发访问
std::mutex engine_mutex;
// 全局引擎实例
Engine engine;

// 处理单个客户端连接的函数
void handle_client(int client_fd) {
    std::cout << "Client connected" << std::endl;
    try {
        while (true) {
            // 读取客户端发送的SQL命令
            String sql = Protocol::read(client_fd);
            // 加锁执行SQL（保证线程安全）
            std::lock_guard<std::mutex> lock(engine_mutex);
            String result = engine.execute(sql);
            // 将结果发送回客户端
            Protocol::write(client_fd, result);
            
            // 如果是exit命令，退出循环
            if (result == "exit") {
                break;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Client error: " << e.what() << std::endl;
    }
    // 关闭客户端套接字
    close(client_fd);
    std::cout << "Client disconnected" << std::endl;
}

int main() {
    // 1. 创建TCP套接字
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        std::cerr << "Failed to create socket" << std::endl;
        return 1;
    }

    // 2. 设置套接字选项：允许地址重用
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        std::cerr << "Failed to set socket options" << std::endl;
        return 1;
    }

    // 3. 绑定地址和端口
    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY; // 监听所有网卡
    address.sin_port = htons(8888);       // 端口8888

    if (bind(server_fd, (sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "Failed to bind socket" << std::endl;
        return 1;
    }

    // 4. 开始监听
    if (listen(server_fd, 5) < 0) {
        std::cerr << "Failed to listen" << std::endl;
        return 1;
    }

    std::cout << "MiniDB server started on port 8888" << std::endl;
    std::cout << "Waiting for connections..." << std::endl;

    // 5. 循环接受客户端连接
    while (true) {
        sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) {
            std::cerr << "Failed to accept connection" << std::endl;
            continue;
        }

        // 为每个客户端创建一个独立的线程
        std::thread client_thread(handle_client, client_fd);
        // 分离线程，不需要等待其结束
        client_thread.detach();
    }

    // 关闭服务器套接字（理论上不会执行到这里）
    close(server_fd);
    return 0;
}