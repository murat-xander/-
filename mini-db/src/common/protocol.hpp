#ifndef PROTOCOL_HPP
#define PROTOCOL_HPP

#include "container/string.hpp"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdexcept>

// 简单的TCP通信协议：先发送4字节长度（网络字节序），再发送数据
struct Protocol {
    // 从套接字读取数据
    static String read(int sockfd) {
        // 第一步：读取4字节的长度字段
        uint32_t len;
        ssize_t n = recv(sockfd, &len, sizeof(len), 0);
        if (n <= 0) throw std::runtime_error("Connection closed");
        // 将网络字节序转换为主机字节序
        len = ntohl(len);
        
        // 第二步：根据长度读取实际数据
        char* buf = new char[len + 1];
        ssize_t total = 0;
        // 循环读取，确保读取到完整数据
        while (total < len) {
            n = recv(sockfd, buf + total, len - total, 0);
            if (n <= 0) {
                delete[] buf;
                throw std::runtime_error("Connection closed");
            }
            total += n;
        }
        // 添加字符串结束符
        buf[len] = '\0';
        String res(buf);
        delete[] buf;
        return res;
    }

    // 向套接字写入数据
    static void write(int sockfd, const String& data) {
        // 第一步：将长度转换为网络字节序并发送
        uint32_t len = htonl(data.length());
        send(sockfd, &len, sizeof(len), 0);
        // 第二步：发送实际数据
        send(sockfd, data.c_str(), data.length(), 0);
    }
};

#endif // PROTOCOL_HPP