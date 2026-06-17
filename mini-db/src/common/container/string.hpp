#ifndef STRING_HPP
#define STRING_HPP

#include <cstddef>
#include <cstring>
#include <cctype>
#include <cstdio>
#include <stdexcept>
#include "vector.hpp"

// 字符串类，替代STL的std::string
class String {
private:
    char* data_;     // 指向字符数组的指针
    size_t size_;    // 字符串长度（不包含末尾的'\0'）
    size_t capacity_;// 内存容量（包含末尾的'\0'）

    // 重新分配内存
    void reallocate(size_t new_cap) {
        if (new_cap <= capacity_) return;
        char* new_data = new char[new_cap];
        // 拷贝原有字符串
        strncpy(new_data, data_, size_);
        new_data[size_] = '\0'; // 确保以'\0'结尾
        delete[] data_;
        data_ = new_data;
        capacity_ = new_cap;
    }

public:
    // 默认构造函数：创建空字符串
    String() : data_(new char[1]), size_(0), capacity_(1) { 
        data_[0] = '\0'; 
    }

    // 从C风格字符串构造
    String(const char* str) : size_(strlen(str)), capacity_(size_ + 1) {
        data_ = new char[capacity_];
        strcpy(data_, str);
    }

    // 从字符数组的指定长度构造
    String(const char* str, size_t len) : size_(len), capacity_(len + 1) {
        data_ = new char[capacity_];
        strncpy(data_, str, len);
        data_[size_] = '\0';
    }

    // 析构函数
    ~String() { delete[] data_; }

    // 拷贝构造函数
    String(const String& other) : size_(other.size_), capacity_(other.capacity_) {
        data_ = new char[capacity_];
        strcpy(data_, other.data_);
    }

    // 拷贝赋值运算符
    String& operator=(const String& other) {
        if (this == &other) return *this;
        delete[] data_;
        size_ = other.size_;
        capacity_ = other.capacity_;
        data_ = new char[capacity_];
        strcpy(data_, other.data_);
        return *this;
    }

    // 移动构造函数
    String(String&& other) noexcept 
        : data_(other.data_), size_(other.size_), capacity_(other.capacity_) {
        // 原对象置为空字符串
        other.data_ = new char[1];
        other.data_[0] = '\0';
        other.size_ = 0;
        other.capacity_ = 1;
    }

    // 移动赋值运算符
    String& operator=(String&& other) noexcept {
        if (this == &other) return *this;
        delete[] data_;
        data_ = other.data_;
        size_ = other.size_;
        capacity_ = other.capacity_;
        // 原对象置为空字符串
        other.data_ = new char[1];
        other.data_[0] = '\0';
        other.size_ = 0;
        other.capacity_ = 1;
        return *this;
    }

    // 获取C风格字符串指针
    const char* c_str() const { return data_; }

    // 获取字符串长度
    size_t length() const { return size_; }

    // 判断是否为空
    bool empty() const { return size_ == 0; }

    // 重载[]运算符，访问指定字符
    char operator[](size_t idx) const { 
        if (idx >= size_) throw std::out_of_range("String index out of range");
        return data_[idx]; 
    }

    // 字符串拼接
    String& operator+=(const String& other) {
        size_t new_size = size_ + other.size_;
        // 如果容量不够，扩容
        if (new_size + 1 > capacity_) 
            reallocate(new_size + 1);
        // 拼接字符串
        strcat(data_, other.data_);
        size_ = new_size;
        return *this;
    }

    // 比较运算符
    bool operator==(const String& other) const { 
        return strcmp(data_, other.data_) == 0; 
    }

    bool operator!=(const String& other) const { 
        return !(*this == other); 
    }

    bool operator<(const String& other) const { 
        return strcmp(data_, other.data_) < 0; 
    }

    bool operator>(const String& other) const { 
        return strcmp(data_, other.data_) > 0; 
    }

    // 截取子串
    String substr(size_t pos, size_t len) const {
        if (pos >= size_) throw std::out_of_range("Substring start out of range");
        if (pos + len > size_) len = size_ - pos;
        return String(data_ + pos, len);
    }

    // 按分隔符分割字符串
    Vector<String> split(char delimiter) const {
        Vector<String> parts;
        size_t start = 0;
        for (size_t i = 0; i <= size_; ++i) {
            // 遇到分隔符或字符串末尾
            if (i == size_ || data_[i] == delimiter) {
                if (i > start) {
                    parts.push_back(String(data_ + start, i - start));
                }
                start = i + 1;
            }
        }
        return parts;
    }

    // 去除首尾空白字符
    String trim() const {
        size_t start = 0, end = size_;
        // 跳过开头的空白
        while (start < end && isspace(static_cast<unsigned char>(data_[start]))) 
            start++;
        // 跳过结尾的空白
        while (end > start && isspace(static_cast<unsigned char>(data_[end - 1]))) 
            end--;
        return String(data_ + start, end - start);
    }

    // 转换为小写
    String to_lower() const {
        String res(*this);
        for (size_t i = 0; i < res.size_; ++i) 
            res.data_[i] = tolower(static_cast<unsigned char>(res.data_[i]));
        return res;
    }

    // 转换为大写
    String to_upper() const {
        String res(*this);
        for (size_t i = 0; i < res.size_; ++i) 
            res.data_[i] = toupper(static_cast<unsigned char>(res.data_[i]));
        return res;
    }

     friend std::ostream& operator<<(std::ostream& os, const String& str) {
        os << str.data_;
        return os;
    }
};

// 全局字符串拼接函数
inline String operator+(const String& a, const String& b) {
    String res(a);
    res += b;
    return res;
}



#endif // STRING_HPP