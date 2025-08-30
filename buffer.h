#ifndef BUFFER_H
#define BUFFER_H

#include <vector>
#include <string>
#include <algorithm>

class Buffer {
public:
    static const size_t kCheapPrepend = 8;
    static const size_t kInitialSize = 1024;

    explicit Buffer(size_t initialSize = kInitialSize)
        : buffer_(kCheapPrepend + initialSize),
          readerIndex_(kCheapPrepend),
          writerIndex_(kCheapPrepend) {
    }

    // 可读字节数
    size_t readableBytes() const {
        return writerIndex_ - readerIndex_;
    }

    // 可写字节数
    size_t writableBytes() const {
        return buffer_.size() - writerIndex_;
    }

    // prependable字节数
    size_t prependableBytes() const {
        return readerIndex_;
    }

    // 获取读指针
    const char* peek() const {
        return begin() + readerIndex_;
    }

    // 查找换行符
    const char* findCRLF() const {
        const char* crlf = std::search(peek(), beginWrite(), kCRLF, kCRLF+2);
        return crlf == beginWrite() ? nullptr : crlf;
    }

    // 查找换行符（从指定位置开始）
    const char* findCRLF(const char* start) const {
        const char* crlf = std::search(start, beginWrite(), kCRLF, kCRLF+2);
        return crlf == beginWrite() ? nullptr : crlf;
    }

    // 更新读指针
    void retrieve(size_t len) {
        if (len < readableBytes()) {
            readerIndex_ += len;
        } else {
            retrieveAll();
        }
    }

    // 更新读指针到指定位置
    void retrieveUntil(const char* end) {
        retrieve(end - peek());
    }

    // 重置缓冲区
    void retrieveAll() {
        readerIndex_ = kCheapPrepend;
        writerIndex_ = kCheapPrepend;
    }

    // 获取数据并重置缓冲区
    std::string retrieveAllAsString() {
        return retrieveAsString(readableBytes());
    }

    // 获取指定长度的数据
    std::string retrieveAsString(size_t len) {
        std::string result(peek(), len);
        retrieve(len);
        return result;
    }

    // 添加数据到缓冲区
    void append(const char* data, size_t len) {
        ensureWritableBytes(len);
        std::copy(data, data+len, beginWrite());
        hasWritten(len);
    }

    void append(const std::string& str) {
        append(str.data(), str.size());
    }

    // 确保有足够的可写空间
    void ensureWritableBytes(size_t len) {
        if (writableBytes() < len) {
            makeSpace(len);
        }
    }

    // 获取写指针
    char* beginWrite() {
        return begin() + writerIndex_;
    }

    const char* beginWrite() const {
        return begin() + writerIndex_;
    }

    // 更新写指针
    void hasWritten(size_t len) {
        writerIndex_ += len;
    }

    // prepend数据
    void prepend(const void* data, size_t len) {
        readerIndex_ -= len;
        const char* d = static_cast<const char*>(data);
        std::copy(d, d+len, begin()+readerIndex_);
    }

    // 收缩缓冲区
    void shrink(size_t reserve) {
        std::vector<char> buf(kCheapPrepend + readableBytes() + reserve);
        std::copy(peek(), peek()+readableBytes(), buf.begin()+kCheapPrepend);
        buffer_.swap(buf);
    }

    // 从文件描述符读取数据
    ssize_t readFd(int fd, int* savedErrno);

private:
    char* begin() {
        return &*buffer_.begin();
    }

    const char* begin() const {
        return &*buffer_.begin();
    }

    // 扩展缓冲区
    void makeSpace(size_t len) {
        if (writableBytes() + prependableBytes() < len + kCheapPrepend) {
            buffer_.resize(writerIndex_ + len);
        } else {
            size_t readable = readableBytes();
            std::copy(begin()+readerIndex_, begin()+writerIndex_, begin()+kCheapPrepend);
            readerIndex_ = kCheapPrepend;
            writerIndex_ = readerIndex_ + readable;
        }
    }

    std::vector<char> buffer_;
    size_t readerIndex_;
    size_t writerIndex_;

    static const char kCRLF[]; // "\r\n"
};

#endif // BUFFER_H