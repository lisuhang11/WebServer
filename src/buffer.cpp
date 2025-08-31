#include "buffer.h"
#include <unistd.h>
#include <sys/uio.h>

const char Buffer::kCRLF[] = "\r\n";

// 从文件描述符读取数据
ssize_t Buffer::readFd(int fd, int* savedErrno) {
    // 创建一个栈上的临时缓冲区
    char extrabuf[65536]; // 64KB
    struct iovec vec[2];
    const size_t writable = writableBytes();
    
    // 首先尝试写入buffer的可写空间
    vec[0].iov_base = beginWrite();
    vec[0].iov_len = writable;
    
    // 如果可写空间不够，使用临时缓冲区
    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof(extrabuf);
    
    // 最多读取writable + sizeof(extrabuf)字节
    const int iovcnt = (writable < sizeof(extrabuf)) ? 2 : 1;
    const ssize_t n = ::readv(fd, vec, iovcnt);
    
    if (n < 0) {
        // 发生错误
        *savedErrno = errno;
    } else if (static_cast<size_t>(n) <= writable) {
        // 数据都写入了buffer的可写空间
        writerIndex_ += n;
    } else {
        // 部分数据写入了临时缓冲区
        writerIndex_ = buffer_.size();
        // 将临时缓冲区中的数据添加到buffer
        append(extrabuf, n - writable);
    }
    
    return n;
}