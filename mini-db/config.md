# MiniDB 运行环境配置

## 操作系统
- Ubuntu 25.10 LTS (x86_64)

## 编译环境
- GCC 11.4.0
- CMake 3.22.1

## 依赖库
- 无第三方依赖（仅使用C++标准库和Linux系统调用）

## 编译命令
```bash
mkdir build
cd build
cmake ..
make -j4