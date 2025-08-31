#include "utils.h"
#include <cstring>
#include <fstream>
#include <sstream>
#include <dirent.h>
#include <sys/stat.h>
#include <algorithm>
#include <vector>

// 字符串分割函数
std::vector<std::string> split(const std::string &s, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);
    
    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    
    return tokens;
}

// 规范化路径，处理相对路径组件，防止路径遍历攻击
std::string normalizePath(const std::string &path) {
    std::vector<std::string> components;
    std::string normalizedPath;
    
    // 处理路径中的每个组件
    std::vector<std::string> pathComponents = split(path, '/');
    
    for (const auto &component : pathComponents) {
        if (component.empty() || component == ".") {
            // 跳过空组件和当前目录
            continue;
        } else if (component == "..") {
            // 回到上一级目录
            if (!components.empty()) {
                components.pop_back();
            }
        } else {
            // 添加有效的目录组件
            components.push_back(component);
        }
    }
    
    // 构建规范化的路径
    for (size_t i = 0; i < components.size(); ++i) {
        normalizedPath += components[i];
        if (i < components.size() - 1) {
            normalizedPath += '/';
        }
    }
    
    return normalizedPath;
}

// 规范化根目录与用户路径的组合，防止路径遍历攻击
std::string normalizePathWithRoot(const std::string &rootPath, const std::string &userPath) {
    std::vector<std::string> components;
    std::string normalizedPath = rootPath;
    
    // 确保根目录以/结尾
    if (!normalizedPath.empty() && normalizedPath.back() != '/') {
        normalizedPath += '/';
    }
    
    // 处理用户路径中的每个组件
    std::vector<std::string> pathComponents = split(userPath, '/');
    
    for (const auto &component : pathComponents) {
        if (component.empty() || component == ".") {
            // 跳过空组件和当前目录
            continue;
        } else if (component == "..") {
            // 回到上一级目录，但不能超出根目录
            if (!components.empty()) {
                components.pop_back();
            }
        } else {
            // 添加有效的目录组件
            components.push_back(component);
        }
    }
    
    // 构建规范化的路径
    for (const auto &component : components) {
        normalizedPath += component + '/';
    }
    
    // 如果路径不为空且不是以/结尾，则添加/
    if (!normalizedPath.empty() && normalizedPath.back() != '/') {
        normalizedPath += '/';
    }
    
    return normalizedPath;
}

// 安全地组合根目录和用户提供的路径，防止路径遍历攻击
bool safePathJoin(const std::string &rootPath, const std::string &userPath, std::string &resultPath) {
    // 确保根目录以/结尾
    std::string normalizedRootPath = rootPath;
    if (!normalizedRootPath.empty() && normalizedRootPath.back() != '/') {
        normalizedRootPath += '/';
    }
    
    // 规范化用户路径
    std::string normalizedUserPath = normalizePath(userPath);
    
    // 组合路径
    resultPath = normalizedRootPath + normalizedUserPath;
    
    // 检查结果路径是否仍然在根目录下（防止../跳出根目录）
    // 这是一个额外的安全检查
    if (resultPath.compare(0, normalizedRootPath.size(), normalizedRootPath) != 0) {
        return false;
    }
    
    return true;
}

// 获取文件大小
off_t getFileSize(const std::string &filename) {
    struct stat statBuf;
    if (stat(filename.c_str(), &statBuf) < 0) {
        return -1;
    }
    return statBuf.st_size;
}

// 检查路径是否为目录
bool isDirectory(const std::string &path) {
    struct stat statBuf;
    if (stat(path.c_str(), &statBuf) < 0) {
        return false;
    }
    return S_ISDIR(statBuf.st_mode);
}

// 检查文件是否存在
bool fileExists(const std::string &filename) {
    struct stat statBuf;
    return (stat(filename.c_str(), &statBuf) == 0);
}

// 读取目录下的所有文件
std::vector<std::string> readDirectory(const std::string &path) {
    std::vector<std::string> files;
    DIR *dir;
    struct dirent *entry;
    
    if ((dir = opendir(path.c_str())) == nullptr) {
        return files;
    }
    
    while ((entry = readdir(dir)) != nullptr) {
        // 跳过 . 和 ..
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        files.push_back(entry->d_name);
    }
    
    closedir(dir);
    return files;
}

// 获取文件的MIME类型
std::string getMimeType(const std::string &filename) {
    size_t dotPos = filename.find_last_of('.');
    if (dotPos == std::string::npos) {
        return "application/octet-stream";
    }
    
    std::string extension = filename.substr(dotPos + 1);
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    
    if (extension == "html" || extension == "htm") {
        return "text/html";
    } else if (extension == "css") {
        return "text/css";
    } else if (extension == "js") {
        return "application/javascript";
    } else if (extension == "json") {
        return "application/json";
    } else if (extension == "xml") {
        return "application/xml";
    } else if (extension == "txt" || extension == "log") {
        return "text/plain";
    } else if (extension == "jpg" || extension == "jpeg") {
        return "image/jpeg";
    } else if (extension == "png") {
        return "image/png";
    } else if (extension == "gif") {
        return "image/gif";
    } else if (extension == "bmp") {
        return "image/bmp";
    } else if (extension == "svg") {
        return "image/svg+xml";
    } else if (extension == "pdf") {
        return "application/pdf";
    } else if (extension == "zip") {
        return "application/zip";
    } else if (extension == "rar") {
        return "application/x-rar-compressed";
    } else if (extension == "7z") {
        return "application/x-7z-compressed";
    } else if (extension == "tar") {
        return "application/x-tar";
    } else if (extension == "gz") {
        return "application/gzip";
    } else if (extension == "mp3") {
        return "audio/mpeg";
    } else if (extension == "wav") {
        return "audio/wav";
    } else if (extension == "mp4") {
        return "video/mp4";
    } else if (extension == "avi") {
        return "video/x-msvideo";
    } else if (extension == "mov") {
        return "video/quicktime";
    } else if (extension == "doc" || extension == "docx") {
        return "application/msword";
    } else if (extension == "xls" || extension == "xlsx") {
        return "application/vnd.ms-excel";
    } else if (extension == "ppt" || extension == "pptx") {
        return "application/vnd.ms-powerpoint";
    }
    
    return "application/octet-stream";
}

// 生成文件列表HTML页面
std::string generateFileListHtml(const std::string &rootPath) {
    std::vector<std::string> files = readDirectory(rootPath);
    std::string html;
    const int MAX_FILES_TO_DISPLAY = 100; // 限制显示的文件数量
    
    // 构建基础HTML结构，但使用更简洁的内容
    html += "<!DOCTYPE html>\n";
    html += "<html>\n";
    html += "<head>\n";
    html += "    <meta charset='UTF-8'>\n";
    html += "    <title>文件列表</title>\n";
    html += "    <style>\n";
    html += "        body { font-family: Arial, sans-serif; margin: 0; padding: 20px; }\n";
    html += "        h1 { color: #333; }\n";
    html += "        ul { list-style-type: none; padding: 0; }\n";
    html += "        li { margin: 5px 0; }\n";
    html += "        a { color: #0066cc; }\n";
    html += "    </style>\n";
    html += "</head>\n";
    html += "<body>\n";
    html += "    <h1>文件列表</h1>\n";
    
    // 文件上传表单（简化版）
    html += "    <form action='/upload' method='post' enctype='multipart/form-data'>\n";
    html += "        <input type='file' name='file' required>\n";
    html += "        <input type='submit' value='上传'>\n";
    html += "    </form>\n";
    html += "    <hr>\n";
    
    // 文件列表
    html += "    <ul>\n";
    
    int fileCount = 0;
    for (const auto &file : files) {
        if (fileCount >= MAX_FILES_TO_DISPLAY) {
            html += "        <li>... 还有更多文件 ...</li>\n";
            break;
        }
        
        // 检查文件是否是目录
        std::string fullPath = rootPath + file;
        if (isDirectory(fullPath)) {
            continue; // 不显示目录
        }
        
        // 文件列表项，包含文件名、下载按钮和删除链接
        html += "        <li style='display: flex; align-items: center; justify-content: space-between; padding: 8px; border-bottom: 1px solid #eee;'>";
        html += "            <span style='flex-grow: 1;'>" + file + "</span>";
        html += "            <div style='display: flex; gap: 10px;'>";
        html += "                <a href='/download?file=" + file + "' style='padding: 5px 10px; background-color: #4CAF50; color: white; text-decoration: none; border-radius: 4px;'>下载</a>";
        html += "                <a href='/delete?file=" + file + "' onclick='return confirm(\"确定要删除吗？\")' style='padding: 5px 10px; background-color: #f44336; color: white; text-decoration: none; border-radius: 4px;'>删除</a>";
        html += "            </div>";
        html += "        </li>\n";
        fileCount++;
    }
    
    html += "    </ul>\n";
    html += "</body>\n";
    html += "</html>\n";
    
    return html;
}

// 格式化文件大小
std::string formatFileSize(off_t size) {
    if (size < 0) {
        return "未知大小";
    }
    
    const char *units[] = {"B", "KB", "MB", "GB", "TB"};
    int unitIndex = 0;
    double doubleSize = size;
    
    while (doubleSize >= 1024 && unitIndex < 4) {
        doubleSize /= 1024;
        unitIndex++;
    }
    
    char buffer[20];
    snprintf(buffer, sizeof(buffer), "%.2f %s", doubleSize, units[unitIndex]);
    return buffer;
}