# WebServer

WebFileServer 是一款基于 Linux 环境开发的轻量级 HTTP 服务器，采用 C++11 语言实现。该服务器提供基本的 HTTP 请求处理能力，支持多线程并发，采用 Reactor 事件处理模型和 I/O 复用技术（epoll）实现高效的网络通信。

## 功能特性

### 核心功能
- **基本 HTTP 服务**：接收并处理 HTTP 请求，返回静态响应
- **并发处理**：使用线程池处理多个客户端连接
- **高效事件监听**：基于 epoll 实现的高效事件驱动机制
- **非阻塞 I/O**：提高服务器吞吐量和响应性能

### 技术特点
- **Reactor 事件处理模型**：分离事件监听和事件处理，提高系统并发能力
- **基于线程池的并发模型**：使用线程池处理客户端连接，避免频繁创建和销毁线程的开销
- **定时器功能**：支持定时任务调度
- **信号处理**：优雅处理系统信号

## 技术架构

- **开发语言**：C++11
- **运行环境**：Linux 系统
- **构建工具**：Makefile
- **网络模型**：Reactor 事件处理模型
- **并发模型**：基于线程池的并发处理
- **网络库**：使用系统调用（socket、epoll 等）
- **协议支持**：HTTP/1.1

## 安装和使用

### 环境要求
- Linux 系统
- g++ 编译器（支持 C++11 标准）
- make 工具

### 编译项目

1. 进入项目目录
```bash
cd WebFileServer
```

2. 执行编译命令
```bash
make
```

### 启动服务器

默认启动方式（端口 8888，使用 4 个工作线程）：
```bash
./main
```

### 访问方式

使用浏览器或 curl 工具访问服务器：
```
curl http://localhost:8888
```

服务器将返回 "Hello from WebFileServer!" 作为响应。

## 项目结构

项目采用模块化设计，各组件之间职责清晰，便于维护和扩展：

```
WebFileServer/
├── Makefile                 # 构建脚本
├── main.cpp                 # 程序入口
├── event_loop.h/.cpp        # 事件循环，核心调度模块
├── channel.h/.cpp           # 事件通道，负责文件描述符和事件的绑定
├── epoller.h/.cpp           # epoll 封装，实现 I/O 复用
├── inet_address.h/.cpp      # IP 地址封装
├── socket.h/.cpp            # Socket 封装
├── acceptor.h/.cpp          # 连接接收器
├── tcp_server.h/.cpp        # TCP 服务器实现
├── tcp_connection.h/.cpp    # TCP 连接实现
├── event_loop_thread_pool.h/.cpp # 事件循环线程池
├── timestamp.h/.cpp         # 时间戳工具
├── buffer.h/.cpp            # 缓冲区实现
└── utils.h/.cpp             # 通用工具函数
```

## 主要模块说明

### 1. EventLoop
事件循环是服务器的核心调度模块，负责监听文件描述符上的事件并分发给对应的处理函数。每个线程最多有一个 EventLoop 对象。

### 2. Channel
Channel 封装了文件描述符及其关注的事件，是 Reactor 模式中的事件处理器。它负责注册、更新和删除事件，并在事件发生时调用对应的回调函数。

### 3. Epoller
Epoller 封装了 Linux 的 epoll 系统调用，提供高效的 I/O 复用功能。它负责管理多个 Channel 对象，监听其上的事件，并在事件发生时通知对应的 Channel。

### 4. TcpServer
TcpServer 是服务器的主要类，负责管理 TCP 连接。它接收新的连接请求，并为每个连接创建一个 TcpConnection 对象。

### 5. TcpConnection
TcpConnection 封装了一个 TCP 连接，负责数据的读写和连接的管理。它包含 Socket、Channel 等成员，处理连接的建立、数据传输和关闭。

### 6. EventLoopThreadPool
EventLoopThreadPool 是一个事件循环线程池，用于处理客户端连接。它创建多个线程，每个线程都有一个 EventLoop 对象，可以并发处理多个连接。

## 工作流程

1. **服务器初始化**：创建 EventLoop、TcpServer 等对象，设置回调函数，启动线程池
2. **监听连接**：TcpServer 通过 Acceptor 监听客户端连接请求
3. **连接处理**：当有新连接请求时，Acceptor 接受连接并创建 TcpConnection 对象
4. **事件分发**：EventLoop 不断监听文件描述符上的事件，并分发给对应的处理函数
5. **数据读写**：TcpConnection 处理数据的读写，调用用户设置的回调函数
6. **连接关闭**：当连接关闭时，清理资源并从服务器中移除连接

## 注意事项

- 当前版本仅提供基本的 HTTP 响应功能，不支持文件上传、下载和删除等操作
- 服务器默认监听 8888 端口，使用 4 个工作线程
- 请确保运行环境中安装了 g++ 编译器（支持 C++11 标准）和 make 工具
- 如需修改服务器配置，请直接修改 main.cpp 文件中的相关参数