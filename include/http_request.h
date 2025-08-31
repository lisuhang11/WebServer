#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H 

#include <string>
#include <regex>
#include <unordered_map>
#include "buffer.h"

class HttpRequest {
public:
    enum class HttpMethod {
        GET,
        POST,
        PUT,
        DELETE,
        UNKNOWN
    };
    enum class HttpRequestParseState {
        REQUEST_LINE,
        HEADERS,
        BODY,
        FINISH
    };

    HttpRequest();
    ~HttpRequest() = default;

    bool parse(const std::shared_ptr<Buffer>& buffer);  // 解析HttpRequest

    // 重置请求
    void reset();

    // Getter方法
    HttpMethod getMethod() const { return method_; }
    const std::string& getPath() const { return path_; }
    const std::string& getVersion() const { return version_; }
    const std::string& getBody() const { return body_; }
    const std::unordered_map<std::string, std::string>& getHeaders() const { return headers_; }
    const std::unordered_map<std::string, std::string>& getQueryParams() const { return queryParams_; }
    const std::unordered_map<std::string, std::string>& getPostData() const { return post_; }

private:
    // 解析函数
    bool parseRequestLine(const std::string& line);  // 解析请求行
    bool parseHeaders(const std::string& line);      // 解析请求头
    bool parseBody(const std::string& body);         // 解析请求体
    void parsePath(const std::string& fullPath);     // 解析路径

private:
    HttpMethod method_;                              // 请求方法
    std::string path_;                               // 请求路径
    std::string version_;                            // HTTP版本
    std::string body_;                               // 原始请求体
    HttpRequestParseState state_;                    // 当前解析状态
    std::unordered_map<std::string, std::string> queryParams_;  // URL查询参数
    std::unordered_map<std::string, std::string> headers_;      // 请求头
    std::unordered_map<std::string, std::string> post_;         // POST请求数据
    std::string readBuffer_;                         // 读取的缓冲区内容
};

#endif