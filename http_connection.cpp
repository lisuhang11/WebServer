#include "http_connection.h"
#include <cstring>
#include <iostream>
#include <sstream>
#include <fcntl.h>
#include <unistd.h>
#include <vector>
#include <string>

// 构造函数
HttpConnection::HttpConnection(int sockfd)
    : sockfd_(sockfd), isClose_(false), isProcessing_(false),
      parseState_(HttpRequestParseState::REQUEST_LINE), method_(HttpMethod::UNKNOWN) {
    addr_.sin_family = AF_INET;
    addr_.sin_addr.s_addr = htonl(INADDR_ANY);
    addr_.sin_port = htons(0);
    
    // 设置非阻塞
    int flags = fcntl(sockfd_, F_GETFL, 0);
    fcntl(sockfd_, F_SETFL, flags | O_NONBLOCK);
}

// 析构函数
HttpConnection::~HttpConnection() {
    ::close(sockfd_);
}

// 处理HTTP请求
bool HttpConnection::process() {
    isProcessing_ = true;
    
    try {
        // 解析请求
        if (!parseRequest()) {
            generateErrorResponse(400, "Bad Request");
            isProcessing_ = false;
            return false;
        }
        
        // 生成响应
        generateResponse();
        
        isProcessing_ = false;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "HttpConnection::process exception: " << e.what() << std::endl;
        generateErrorResponse(500, "Internal Server Error");
        isProcessing_ = false;
        return false;
    } catch (...) {
        std::cerr << "HttpConnection::process unknown exception" << std::endl;
        generateErrorResponse(500, "Internal Server Error");
        isProcessing_ = false;
        return false;
    }
}

// 解析HTTP请求
bool HttpConnection::parseRequest() {
    std::istringstream requestStream(readBuffer_);
    std::string line;
    
    // 解析请求行
    if (parseState_ == HttpRequestParseState::REQUEST_LINE) {
        if (!std::getline(requestStream, line)) {
            return false;
        }
        // 去除行尾的
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (!parseRequestLine(line)) {
            return false;
        }
        parseState_ = HttpRequestParseState::HEADERS;
    }
    
    // 解析请求头
    if (parseState_ == HttpRequestParseState::HEADERS) {
        while (std::getline(requestStream, line)) {
            // 去除行尾的
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            // 遇到空行表示请求头结束
            if (line.empty()) {
                parseState_ = HttpRequestParseState::BODY;
                break;
            }
            if (!parseHeaders(line)) {
                return false;
            }
        }
    }
    
    // 解析请求体
    if (parseState_ == HttpRequestParseState::BODY) {
        // 读取剩余的所有数据作为请求体
        std::string restOfData;
        std::string tempLine;
        while (std::getline(requestStream, tempLine)) {
            if (!tempLine.empty() && tempLine.back() == '\r') {
                tempLine.pop_back();
            }
            restOfData += tempLine + '\n';
        }
        if (!restOfData.empty()) {
            restOfData.pop_back(); // 移除最后一个换行符
            if (!parseBody(restOfData)) {
                return false;
            }
        }
        parseState_ = HttpRequestParseState::FINISH;
    }
    
    return parseState_ == HttpRequestParseState::FINISH;
}

// 解析请求行
bool HttpConnection::parseRequestLine(const std::string &line) {
    std::istringstream lineStream(line);
    std::string methodStr, fullPath;
    
    if (!(lineStream >> methodStr >> fullPath >> version_)) {
        return false;
    }
    
    // 解析请求方法
    if (methodStr == "GET") {
        method_ = HttpMethod::GET;
    } else if (methodStr == "POST") {
        method_ = HttpMethod::POST;
    } else if (methodStr == "HEAD") {
        method_ = HttpMethod::HEAD;
    } else if (methodStr == "PUT") {
        method_ = HttpMethod::PUT;
    } else if (methodStr == "DELETE") {
        method_ = HttpMethod::DELETE;
    } else {
        method_ = HttpMethod::UNKNOWN;
    }
    
    // 解析路径和查询参数
    size_t queryPos = fullPath.find('?');
    if (queryPos != std::string::npos) {
        path_ = fullPath.substr(0, queryPos);
        std::string queryStr = fullPath.substr(queryPos + 1);
        
        // 解析查询参数
        size_t start = 0;
        size_t end = 0;
        while ((end = queryStr.find('&', start)) != std::string::npos) {
            std::string param = queryStr.substr(start, end - start);
            size_t eqPos = param.find('=');
            if (eqPos != std::string::npos) {
                std::string key = param.substr(0, eqPos);
                std::string value = param.substr(eqPos + 1);
                queryParams_[key] = value;
            }
            start = end + 1;
        }
        // 处理最后一个参数
        std::string param = queryStr.substr(start);
        size_t eqPos = param.find('=');
        if (eqPos != std::string::npos) {
            std::string key = param.substr(0, eqPos);
            std::string value = param.substr(eqPos + 1);
            queryParams_[key] = value;
        }
    } else {
        path_ = fullPath;
    }
    
    return true;
}

// 解析请求头
bool HttpConnection::parseHeaders(const std::string &line) {
    size_t colonPos = line.find(':');
    if (colonPos == std::string::npos) {
        return false;
    }
    
    std::string key = line.substr(0, colonPos);
    std::string value = line.substr(colonPos + 1);
    
    // 去除value前后的空格
    value.erase(0, value.find_first_not_of(" \t"));
    value.erase(value.find_last_not_of(" \t") + 1);
    
    headers_[key] = value;
    
    return true;
}

// 解析请求体
bool HttpConnection::parseBody(const std::string &body) {
    body_ = body;
    return true;
}

// 生成HTTP响应
void HttpConnection::generateResponse() {
    // 清空响应头
    responseHeader_.clear();
    
    // 根据路径处理不同的请求
    if (path_ == "/" || path_ == "/index.html") {
        // 返回简单的欢迎页面
        std::string html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>HTTP Server</title></head><body>";
        html += "<h1>Welcome to HTTP Server</h1>";
        html += "<p>This is a simple HTTP server based on Reactor pattern.</p>";
        html += "<p>Current path: " + path_ + "</p>";
        html += "</body></html>";
        
        responseHeader_ = "HTTP/1.1 200 OK\r\n";
        responseHeader_ += "Content-Type: text/html; charset=utf-8\r\n";
        responseHeader_ += "Content-Length: " + std::to_string(html.size()) + "\r\n";
        responseHeader_ += "Connection: keep-alive\r\n";
        responseHeader_ += "\r\n";
        responseHeader_ += html;
    } else {
        // 返回当前路径信息的简单页面
        std::string html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>HTTP Server</title></head><body>";
        html += "<h1>Path: " + path_ + "</h1>";
        html += "<p>Method: ";
        
        switch (method_) {
            case HttpMethod::GET: html += "GET";
break;
            case HttpMethod::POST: html += "POST";
break;
            case HttpMethod::HEAD: html += "HEAD";
break;
            case HttpMethod::PUT: html += "PUT";
break;
            case HttpMethod::DELETE: html += "DELETE";
break;
            default: html += "Unknown";
break;
        }
        
        html += "</p>";
        
        // 添加请求头信息
        if (!headers_.empty()) {
            html += "<h2>Headers:</h2><ul>";
            for (const auto& header : headers_) {
                html += "<li>" + header.first + ": " + header.second + "</li>";
            }
            html += "</ul>";
        }
        
        // 添加查询参数信息
        if (!queryParams_.empty()) {
            html += "<h2>Query Parameters:</h2><ul>";
            for (const auto& param : queryParams_) {
                html += "<li>" + param.first + "=" + param.second + "</li>";
            }
            html += "</ul>";
        }
        
        // 添加请求体信息
        if (!body_.empty()) {
            html += "<h2>Request Body:</h2><pre>" + body_ + "</pre>";
        }
        
        html += "</body></html>";
        
        responseHeader_ = "HTTP/1.1 200 OK\r\n";
        responseHeader_ += "Content-Type: text/html; charset=utf-8\r\n";
        responseHeader_ += "Content-Length: " + std::to_string(html.size()) + "\r\n";
        responseHeader_ += "Connection: keep-alive\r\n";
        responseHeader_ += "\r\n";
        responseHeader_ += html;
    }
}

// 生成错误响应
void HttpConnection::generateErrorResponse(int statusCode, const std::string &message) {
    std::string html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>Error</title></head><body>";
    html += "<h1>" + std::to_string(statusCode) + " " + message + "</h1>";
    html += "</body></html>";
    
    responseHeader_ = "HTTP/1.1 " + std::to_string(statusCode) + " " + message + "\r\n";
    responseHeader_ += "Content-Type: text/html; charset=utf-8\r\n";
    responseHeader_ += "Content-Length: " + std::to_string(html.size()) + "\r\n";
    responseHeader_ += "Connection: close\r\n";
    responseHeader_ += "\r\n";
    responseHeader_ += html;
}

// 重置连接状态
void HttpConnection::reset() {
    readBuffer_.clear();
    responseHeader_.clear();
    body_.clear();
    parseState_ = HttpRequestParseState::REQUEST_LINE;
    method_ = HttpMethod::UNKNOWN;
    path_.clear();
    version_.clear();
    headers_.clear();
    queryParams_.clear();
    isProcessing_ = false;
    isClose_ = false;
}

// 读取数据
ssize_t HttpConnection::read(int* savedErrno) {
    ssize_t len = 0;
    ssize_t totalRead = 0;
    char buffer[4096];
    
    while (true) {
        len = ::read(sockfd_, buffer, sizeof(buffer));
        if (len < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break; // 非阻塞IO，没有更多数据可读
            }
            *savedErrno = errno;
            return -1;
        } else if (len == 0) {
            break; // 对方关闭连接
        }
        totalRead += len;
        readBuffer_.append(buffer, len);
    }
    return totalRead;
}

// 关闭连接
void HttpConnection::close() {
    isClose_ = true;
}

// 写入数据
ssize_t HttpConnection::write(int* savedErrno) {
    ssize_t len = 0;
    
    // 普通响应
    if (!responseHeader_.empty()) {
        len = ::write(sockfd_, responseHeader_.data(), responseHeader_.size());
        if (len < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // 非阻塞IO，暂时无法写入
                return len;
            }
            *savedErrno = errno;
            return -1;
        }
    }
    
    return len;
}