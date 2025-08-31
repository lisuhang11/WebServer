#include "http_response.h"
#include <sstream>
#include <fstream>
#include <iostream>
#include "utils.h"

HttpResponse::HttpResponse()
    : statusCode_(HttpStatusCode::OK), statusMessage_("OK") {
}

// 设置状态码
void HttpResponse::setStatusCode(HttpStatusCode code) {
    statusCode_ = code;
    
    // 根据状态码设置默认状态消息
    switch (code) {
        case HttpStatusCode::OK:
            statusMessage_ = "OK";
            break;
        case HttpStatusCode::BadRequest:
            statusMessage_ = "Bad Request";
            break;
        case HttpStatusCode::NotFound:
            statusMessage_ = "Not Found";
            break;
        case HttpStatusCode::Forbidden:
            statusMessage_ = "Forbidden";
            break;
        case HttpStatusCode::InternalServerError:
            statusMessage_ = "Internal Server Error";
            break;
        default:
            statusMessage_ = "Unknown Status";
            break;
    }
}

// 设置状态消息
void HttpResponse::setStatusMessage(const std::string &message) {
    statusMessage_ = message;
}

// 设置响应头
void HttpResponse::setHeader(const std::string &key, const std::string &value) {
    headers_[key] = value;
}

// 设置响应体
void HttpResponse::setBody(const std::string &body) {
    body_ = body;
    
    // 更新Content-Length头
    std::stringstream ss;
    ss << body_.size();
    headers_["Content-Length"] = ss.str();
}

// 设置文件信息（用于文件下载）
void HttpResponse::setFileInfo(std::shared_ptr<FileInfo> fileInfo) {
    fileInfo_ = fileInfo;
    
    if (fileInfo_) {
        // 设置文件大小
        std::stringstream ss;
        ss << fileInfo_->size;
        headers_["Content-Length"] = ss.str();
        
        // 设置文件MIME类型
        std::string mimeType = getMimeType(fileInfo_->path);
        headers_["Content-Type"] = mimeType;
        
        // 设置Content-Disposition头
        headers_["Content-Disposition"] = "attachment; filename=\"" + 
            fileInfo_->path.substr(fileInfo_->path.find_last_of('/') + 1) + "\"";
    }
}

// 生成完整的响应数据
std::string HttpResponse::generateResponseString() const {
    std::stringstream responseStream;
    
    // 写入状态行
    responseStream << "HTTP/1.1 " << static_cast<int>(statusCode_) << " " << statusMessage_ << "\r\n";
    
    // 写入响应头
    for (const auto &header : headers_) {
        responseStream << header.first << ": " << header.second << "\r\n";
    }
    
    // 写入空行
    responseStream << "\r\n";
    
    // 如果有响应体，写入响应体
    if (!body_.empty()) {
        responseStream << body_;
    }
    
    return responseStream.str();
}

// 生成错误响应
void HttpResponse::generateErrorResponse(HttpStatusCode statusCode, const std::string &message) {
    setStatusCode(statusCode);
    setStatusMessage(message);
    
    // 设置响应头
    headers_.clear();
    headers_["Content-Type"] = "text/html; charset=utf-8";
    
    // 构建错误页面HTML
    std::stringstream htmlStream;
    htmlStream << "<!DOCTYPE html>" << std::endl;
    htmlStream << "<html>" << std::endl;
    htmlStream << "<head>" << std::endl;
    htmlStream << "<title>Error " << static_cast<int>(statusCode) << "</title>" << std::endl;
    htmlStream << "<style>" << std::endl;
    htmlStream << "    body { font-family: Arial, sans-serif; margin: 40px; }" << std::endl;
    htmlStream << "    h1 { color: #333; }" << std::endl;
    htmlStream << "    p { color: #666; }" << std::endl;
    htmlStream << "</style>" << std::endl;
    htmlStream << "</head>" << std::endl;
    htmlStream << "<body>" << std::endl;
    htmlStream << "<h1>Error " << static_cast<int>(statusCode) << ": " << message << "</h1>" << std::endl;
    htmlStream << "<p>The requested operation could not be completed.</p>" << std::endl;
    htmlStream << "</body>" << std::endl;
    htmlStream << "</html>" << std::endl;
    
    setBody(htmlStream.str());
}

// 获取文件信息
const std::shared_ptr<FileInfo> HttpResponse::getFileInfo() const {
    return fileInfo_;
}

// 重置响应状态
void HttpResponse::reset() {
    statusCode_ = HttpStatusCode::OK;
    statusMessage_ = "OK";
    headers_.clear();
    body_.clear();
    fileInfo_.reset();
}