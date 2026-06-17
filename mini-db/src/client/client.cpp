#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "../common/protocol.hpp"

int main() {
    // 1. 创建TCP套接字
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        std::cerr << "Failed to create socket" << std::endl;
        return 1;
    }

    // 2. 设置服务器地址
    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8888); // 服务器端口
    
    // 服务器IP地址（本地测试用127.0.0.1）
    if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0) {
        std::cerr << "Invalid address" << std::endl;
        return 1;
    }

    // 3. 连接到服务器
    if (connect(sockfd, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Connection failed" << std::endl;
        return 1;
    }

    // 4. 欢迎信息
    std::cout << "Welcome to MiniDB!" << std::endl;
    std::cout << "Type 'exit' to quit" << std::endl;

    // 5. 交互循环
    char buf[1024];
    while (true) {
        // 显示提示符
        std::cout << "minidb> ";
        // 读取用户输入
        std::cin.getline(buf, sizeof(buf));
        String sql(buf);
        
        if (sql.empty()) continue;

        // 发送SQL命令到服务器
        Protocol::write(sockfd, sql);
        // 接收服务器返回的结果
        String result = Protocol::read(sockfd);
        
        // 如果是exit命令，退出循环
        if (result == "exit") {
            break;
        }
        
        // 显示结果
        std::cout << result << std::endl;
    }

    // 6. 关闭套接字
    close(sockfd);
    return 0;
}