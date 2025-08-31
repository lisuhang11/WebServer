#include "http_connection.h"
#include <cstring>
#include <iostream>
#include <sstream>
#include <fcntl.h>
#include <unistd.h>
#include <vector>
#include <string>
#include "timestamp.h"

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

// 获取MIME类型
bool HttpConnection::getFileExtension(const std::string& path, std::string& extension) {
    size_t dotPos = path.find_last_of('.');
    if (dotPos == std::string::npos || dotPos == path.size() - 1) {
        return false;
    }
    extension = path.substr(dotPos + 1);
    return true;
}

// 获取MIME类型
std::string HttpConnection::getMimeType(const std::string& extension) {
    if (extension == "html" || extension == "htm") return "text/html; charset=utf-8";
    if (extension == "css") return "text/css; charset=utf-8";
    if (extension == "js") return "application/javascript; charset=utf-8";
    if (extension == "json") return "application/json; charset=utf-8";
    if (extension == "png") return "image/png";
    if (extension == "jpg" || extension == "jpeg") return "image/jpeg";
    if (extension == "gif") return "image/gif";
    if (extension == "svg") return "image/svg+xml";
    if (extension == "pdf") return "application/pdf";
    if (extension == "txt") return "text/plain; charset=utf-8";
    if (extension == "xml") return "application/xml";
    if (extension == "zip") return "application/zip";
    if (extension == "mp3") return "audio/mpeg";
    if (extension == "mp4") return "video/mp4";
    // 默认MIME类型
    return "application/octet-stream";
}

// 读取文件内容
bool HttpConnection::readFile(const std::string& filePath, std::string& content) {
    FILE* file = fopen(filePath.c_str(), "rb");
    if (!file) {
        return false;
    }
    
    // 获取文件大小
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    // 读取文件内容
    content.resize(fileSize);
    size_t bytesRead = fread(&content[0], 1, fileSize, file);
    fclose(file);
    
    return bytesRead == fileSize;
}

// 处理静态文件请求
bool HttpConnection::handleStaticFile() {
    // 文件路径映射 - 将URI映射到服务器本地文件系统路径
    std::string filePath = "/home/WebFileServer/public";
    
    if (path_ == "/") {
        filePath += "/index.html";
    } else {
        filePath += path_;
    }
    
    // 检查文件是否存在并可读
    std::string fileContent;
    if (!readFile(filePath, fileContent)) {
        return false;
    }
    
    // 获取文件扩展名和MIME类型
    std::string extension;
    std::string mimeType = "application/octet-stream";
    if (getFileExtension(filePath, extension)) {
        mimeType = getMimeType(extension);
    }
    
    // 生成响应
    responseHeader_ = "HTTP/1.1 200 OK\r\n";
    responseHeader_ += "Content-Type: " + mimeType + "\r\n";
    responseHeader_ += "Content-Length: " + std::to_string(fileContent.size()) + "\r\n";
    responseHeader_ += "Connection: keep-alive\r\n";
    responseHeader_ += "\r\n";
    responseHeader_ += fileContent;
    
    return true;
}

// 处理API请求
bool HttpConnection::handleApiRequest() {
    // 处理表单提交
    if (path_ == "/api/submit" && method_ == HttpMethod::POST) {
        // 解析表单数据
        std::unordered_map<std::string, std::string> formData;
        
        // 假设表单数据是x-www-form-urlencoded格式
        size_t start = 0;
        size_t end = 0;
        while ((end = body_.find('&', start)) != std::string::npos) {
            std::string param = body_.substr(start, end - start);
            size_t eqPos = param.find('=');
            if (eqPos != std::string::npos) {
                std::string key = param.substr(0, eqPos);
                std::string value = param.substr(eqPos + 1);
                formData[key] = value;
            }
            start = end + 1;
        }
        // 处理最后一个参数
        std::string param = body_.substr(start);
        size_t eqPos = param.find('=');
        if (eqPos != std::string::npos) {
            std::string key = param.substr(0, eqPos);
            std::string value = param.substr(eqPos + 1);
            formData[key] = value;
        }
        
        // 生成JSON响应
        std::string jsonResponse = "{";
        jsonResponse += "\"status\": \"success\",";
        jsonResponse += "\"message\": \"表单提交成功\",";
        jsonResponse += "\"data\": {";
        
        bool first = true;
        for (const auto& pair : formData) {
            if (!first) jsonResponse += ",";
            jsonResponse += "\"" + pair.first + "\": \"" + pair.second + "\"";
            first = false;
        }
        
        jsonResponse += "}}";
        
        responseHeader_ = "HTTP/1.1 200 OK\r\n";
        responseHeader_ += "Content-Type: application/json; charset=utf-8\r\n";
        responseHeader_ += "Content-Length: " + std::to_string(jsonResponse.size()) + "\r\n";
        responseHeader_ += "Connection: keep-alive\r\n";
        responseHeader_ += "\r\n";
        responseHeader_ += jsonResponse;
        
        return true;
    }
    
    // 处理其他API请求
    else if (path_.substr(0, 5) == "/api/") {
        std::string jsonResponse;
        
        // 特定处理/api/test端点
        if (path_ == "/api/test") {
            jsonResponse = "{";
            jsonResponse += "\"status\": \"success\",";
            jsonResponse += "\"message\": \"API测试成功\",";
            jsonResponse += "\"server_info\": {";
            jsonResponse += "\"name\": \"WebFileServer\",";
            jsonResponse += "\"version\": \"1.0.0\",";
            jsonResponse += "\"time\": \"" + Timestamp::now().toString() + "\"";
            jsonResponse += "}}";
        }
        // 返回API帮助信息
        else {
            jsonResponse = "{";
            jsonResponse += "\"status\": \"success\",";
            jsonResponse += "\"message\": \"WebFileServer API\",";
            jsonResponse += "\"available_endpoints\": [";
            jsonResponse += "{\"path\": \"/api/submit\", \"method\": \"POST\", \"description\": \"处理表单提交\"},";
            jsonResponse += "{\"path\": \"/api/test\", \"method\": \"GET\", \"description\": \"API测试端点\"}]";
            jsonResponse += "}";
        }
        
        responseHeader_ = "HTTP/1.1 200 OK\r\n";
        responseHeader_ += "Content-Type: application/json; charset=utf-8\r\n";
        responseHeader_ += "Content-Length: " + std::to_string(jsonResponse.size()) + "\r\n";
        responseHeader_ += "Connection: keep-alive\r\n";
        responseHeader_ += "\r\n";
        responseHeader_ += jsonResponse;
        
        return true;
    }

    return false;
}

// 生成HTTP响应
void HttpConnection::generateResponse() {
    // 清空响应头
    responseHeader_.clear();
    
    // 首先尝试处理API请求
    if (handleApiRequest()) {
        return;
    }
    
    // 然后尝试处理静态文件请求
    if (handleStaticFile()) {
        return;
    }
    
    // 如果都不是，返回404错误
    generateErrorResponse(404, "Not Found");
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