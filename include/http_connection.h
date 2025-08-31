#ifndef HTTP_CONNECTION_H
#define HTTP_CONNECTION_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <string>
#include <memory>
#include <unordered_map>
#include <vector>

// HTTP请求方法
enum class HttpMethod {
    GET,
    POST,
    HEAD,
    PUT,
    DELETE,
    OPTIONS,
    TRACE,
    CONNECT,
    UNKNOWN
};

// HTTP请求解析状态
enum class HttpRequestParseState {
    REQUEST_LINE,
    HEADERS,
    BODY,
    FINISH
};

class HttpConnection : public std::enable_shared_from_this<HttpConnection> {
public:
    HttpConnection(int sockfd);
    ~HttpConnection();

    // 处理HTTP请求
    bool process();
    
    // 读取数据
    ssize_t read(int* savedErrno);
    
    // 获取响应头
    const std::string &getResponse() const { return responseHeader_; }
    
    // 写入数据
    ssize_t write(int* savedErrno);
    
    // 关闭连接
    void close();
    
    // 重置连接状态
    void reset();
    
    // 追加缓冲区数据
    void appendBuffer(const std::string& data) { readBuffer_ += data; }
    
    // 是否正在处理
    bool isProcessing() const { return isProcessing_; }

private:
    // 解析请求相关方法
    bool parseRequest();
    bool parseRequestLine(const std::string &line);
    bool parseHeaders(const std::string &line);
    bool parseBody(const std::string &body);
    
    // 生成响应相关方法
    void generateResponse();
    void generateErrorResponse(int statusCode, const std::string &message);

    int sockfd_;                             // 套接字描述符
    struct sockaddr_in addr_;                // 地址信息
    std::string readBuffer_;                 // 读取缓冲区
    bool isProcessing_;                      // 是否正在处理
    bool isClose_;                           // 是否关闭
    
    // 响应相关
    std::string responseHeader_;             // 响应头
    
    // HTTP请求解析相关
    HttpRequestParseState parseState_;       // 解析状态
    HttpMethod method_;                      // 请求方法
    std::string path_;                       // 请求路径
    std::string version_;                    // HTTP版本
    std::string body_;                       // 请求体
    std::unordered_map<std::string, std::string> headers_; // 请求头
    std::unordered_map<std::string, std::string> queryParams_; // 查询参数
};

#endif // HTTP_CONNECTION_H