#include "Buffer.h"

#include <errno.h>
#include <sys/uio.h>
#include <unistd.h>

/**
 * 从fd上读取数据存储到buffer中   Poller工作在LT模式    利用栈上的空间
 * Buffer缓冲区是有大小的！  但是从fd上读取数据的时候，缺不知道TCP数据的最终大小
 * 
 * readv的第三个参数是iovcnt，如果为1，表示只用vec[0]，如果为2,表示先用vec[0]的空间不够再用vec[1]的空间，所以说
 * append的第三个参数是n-writable
 */
ssize_t Buffer::readFd(int fd,int* saveErrno){
    char extrabuf[65536] = {0};     // 栈上的内存空间   64k

    struct iovec vec[2];

    const size_t writable = writeableBytes();   // 这是buffer底层缓冲区剩余的可写空间大小
    vec[0].iov_base = begin() + writerIndex_;
    vec[0].iov_len = writable;

    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof(extrabuf);

    // 选取剩余空间与栈上的空间中更大的     
    const int iovcnt = (writable<sizeof(extrabuf))?2:1;
    const ssize_t n = ::readv(fd,vec,iovcnt);
    if(n<0){
        *saveErrno = errno;
    }
    else if(n<=writable){       // Buffer的可写缓冲区已经够存储读出来的数据了   这时存储到buffer的缓冲区上了
        writerIndex_ += n;
    }
    else{   // extrabuf里面也写入了数据
        writerIndex_ = buffer_.size();
        // append里有ensureWrioteableBytes调用，确保空间足够，这时空间是不够的，需要扩容大小为需要空间的大小（利用栈上的空间存储，不够时扩容）
        // 第二个参数是n-writable的原因是，可能读取到的数据很多剩余的空间不够，只需要扩容读取的数据大小减去剩余空间的大小
        // （先扩容）从栈的缓冲区读取buffer缓冲区存不下的数据
        append(extrabuf,n-writable);    // writerIndex_开始写，n-writable大小的数据
    }
    return n;
}

// 往fd里写
ssize_t Buffer::writeFd(int fd,int* saveErrno){
    // 往fd里写  peek()是读下标   写数据的大小
    ssize_t n = ::write(fd,peek(),readableBytes());
    if(n<0){
        *saveErrno = errno;
    }
    return n;
}
