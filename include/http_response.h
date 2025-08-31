#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include <string>
#include <unordered_map>
#include <memory>

// HTTP响应状态码
enum class HttpStatusCode {
    OK = 200,
    BadRequest = 400,
    NotFound = 404,
    Forbidden = 403,
    InternalServerError = 500
};

// 文件信息结构体
struct FileInfo {
    std::string path;
    off_t size;
};

class HttpResponse {
public:
    HttpResponse();
    ~HttpResponse() = default;

    // 设置状态码
    void setStatusCode(HttpStatusCode code);
    
    // 设置状态消息
    void setStatusMessage(const std::string &message);
    
    // 设置响应头
    void setHeader(const std::string &key, const std::string &value);
    
    // 设置响应体
    void setBody(const std::string &body);
    
    // 设置文件信息（用于文件下载）
    void setFileInfo(std::shared_ptr<FileInfo> fileInfo);
    
    // 生成完整的响应数据
    std::string generateResponseString() const;
    
    // 生成错误响应
    void generateErrorResponse(HttpStatusCode statusCode, const std::string &message);
    
    // 获取文件信息
    const std::shared_ptr<FileInfo> getFileInfo() const;
    
    // 重置响应状态
    void reset();

private:
    HttpStatusCode statusCode_;                     // 状态码
    std::string statusMessage_;                     // 状态消息
    std::unordered_map<std::string, std::string> headers_; // 响应头
    std::string body_;                              // 响应体
    std::shared_ptr<FileInfo> fileInfo_;            // 文件信息（用于下载）
};

#endif // HTTP_RESPONSE_H