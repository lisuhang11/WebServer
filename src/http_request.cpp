#include "http_request.h"
#include <algorithm>
#include <iostream>

HttpRequest::HttpRequest() 
    : method_(HttpMethod::UNKNOWN),
      path_(""),
      version_(""),
      body_(""),
      state_(HttpRequestParseState::REQUEST_LINE),
      readBuffer_("") {
}

void HttpRequest::reset() {
    method_ = HttpMethod::UNKNOWN;
    path_ = "";
    version_ = "";
    body_ = "";
    state_ = HttpRequestParseState::REQUEST_LINE;
    queryParams_.clear();
    headers_.clear();
    post_.clear();
    readBuffer_.clear();
}

bool HttpRequest::parse(const std::shared_ptr<Buffer>& buffer) {
    if (buffer->readableBytes() <= 0) {
        return false;
    }

    // 将Buffer中的数据读取到readBuffer_
    readBuffer_.append(buffer->peek(), buffer->readableBytes());
    buffer->retrieveAll();

    const char* CRLF = Buffer::kCRLF;
    size_t hasRead = 0;
    size_t dataSize = readBuffer_.size();

    while (state_ != HttpRequestParseState::FINISH && hasRead < dataSize) {
        const char* lineEnd = std::search(
            readBuffer_.data() + hasRead,
            readBuffer_.data() + dataSize,
            CRLF,
            CRLF + 2
        );

        if (lineEnd == readBuffer_.data() + dataSize) {
            // 没有找到完整的行，退出循环
            break;
        }

        std::string line(readBuffer_.data() + hasRead, lineEnd);
        hasRead = static_cast<size_t>(lineEnd - readBuffer_.data()) + 2;

        switch (state_) {
            case HttpRequestParseState::REQUEST_LINE:
                if (!parseRequestLine(line)) {
                    return false;
                }
                break;
            case HttpRequestParseState::HEADERS:
                if (line.empty()) {
                    // 头部解析完成，检查是否有请求体
                    state_ = HttpRequestParseState::BODY;
                } else {
                    parseHeaders(line);
                }
                break;
            case HttpRequestParseState::BODY:
                parseBody(line);
                state_ = HttpRequestParseState::FINISH;
                break;
            default:
                break;
        }
    }

    // 处理请求体（如果有的话）
    if (state_ == HttpRequestParseState::BODY && hasRead < dataSize) {
        std::string body(readBuffer_.data() + hasRead, readBuffer_.data() + dataSize);
        parseBody(body);
        state_ = HttpRequestParseState::FINISH;
    }

    return state_ == HttpRequestParseState::FINISH;
}

bool HttpRequest::parseRequestLine(const std::string& line) {
    std::regex pattern("(GET|POST|PUT|DELETE)\\s+([^\\s]+)\\s+(HTTP/\\d\\.\\d)");
    std::smatch result;

    if (std::regex_match(line, result, pattern)) {
        std::string methodStr = result[1].str();
        if (methodStr == "GET") {
            method_ = HttpMethod::GET;
        } else if (methodStr == "POST") {
            method_ = HttpMethod::POST;
        } else if (methodStr == "PUT") {
            method_ = HttpMethod::PUT;
        } else if (methodStr == "DELETE") {
            method_ = HttpMethod::DELETE;
        }

        std::string fullPath = result[2].str();
        parsePath(fullPath);
        version_ = result[3].str();
        state_ = HttpRequestParseState::HEADERS;
        return true;
    }

    return false;
}

bool HttpRequest::parseHeaders(const std::string& line) {
    std::regex pattern("([^:]+):\\s*(.+)");
    std::smatch result;

    if (std::regex_match(line, result, pattern)) {
        std::string key = result[1].str();
        std::string value = result[2].str();
        headers_[key] = value;
        return true;
    }

    return false;
}

bool HttpRequest::parseBody(const std::string& body) {
    body_ = body;

    // 解析POST请求数据
    if (method_ == HttpMethod::POST) {
        auto it = headers_.find("Content-Type");
        if (it != headers_.end() && it->second.find("application/x-www-form-urlencoded") != std::string::npos) {
            // 解析表单数据
            size_t pos = 0;
            while (pos < body_.size()) {
                size_t ampPos = body_.find('&', pos);
                if (ampPos == std::string::npos) {
                    ampPos = body_.size();
                }
                size_t eqPos = body_.find('=', pos);
                if (eqPos != std::string::npos && eqPos < ampPos) {
                    std::string key = body_.substr(pos, eqPos - pos);
                    std::string value = body_.substr(eqPos + 1, ampPos - eqPos - 1);
                    post_[key] = value;
                }
                pos = ampPos + 1;
            }
        }
    }

    return true;
}

void HttpRequest::parsePath(const std::string& fullPath) {
    size_t queryPos = fullPath.find('?');
    if (queryPos != std::string::npos) {
        // 提取路径
        path_ = fullPath.substr(0, queryPos);
        // 解析查询参数
        std::string query = fullPath.substr(queryPos + 1);
        size_t pos = 0;
        while (pos < query.size()) {
            size_t ampPos = query.find('&', pos);
            if (ampPos == std::string::npos) {
                ampPos = query.size();
            }
            size_t eqPos = query.find('=', pos);
            if (eqPos != std::string::npos && eqPos < ampPos) {
                std::string key = query.substr(pos, eqPos - pos);
                std::string value = query.substr(eqPos + 1, ampPos - eqPos - 1);
                queryParams_[key] = value;
            }
            pos = ampPos + 1;
        }
    } else {
        path_ = fullPath;
    }
}