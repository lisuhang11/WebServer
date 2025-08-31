#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>
#include <sys/types.h>

// 分割字符串函数
std::vector<std::string> split(const std::string &s, char delimiter);

// 获取文件大小
off_t getFileSize(const std::string &filename);

// 检查路径是否为目录
bool isDirectory(const std::string &path);

// 检查文件是否存在
bool fileExists(const std::string &filename);

// 规范化路径，处理相对路径组件，防止路径遍历攻击
std::string normalizePath(const std::string &path);

// 安全组合根目录与用户路径，确保不会跳出根目录
bool safePathJoin(const std::string &rootPath, const std::string &userPath, std::string &resultPath);

// 读取目录下的所有文件
std::vector<std::string> readDirectory(const std::string &path);

// 获取文件的MIME类型
std::string getMimeType(const std::string &filename);

// 生成文件列表HTML页面
std::string generateFileListHtml(const std::string &path);

// 格式化文件大小
std::string formatFileSize(off_t size);

#endif // UTILS_H