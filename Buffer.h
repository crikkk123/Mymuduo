#pragma once


#include <vector>
#include <stdlib.h>
#include <string>
#include <algorithm>

// 网络库底层的缓冲器类型定义
class Buffer{
public:
    static const size_t kCheapPrepend = 8;
    static const size_t KInitialSize = 1024;

    explicit Buffer(size_t initialSize = KInitialSize):buffer_(kCheapPrepend+KInitialSize),readerIndex_(kCheapPrepend),writerIndex_(kCheapPrepend){}

    size_t readableBytes() const {return writerIndex_ - readerIndex_;}
    size_t writeableBytes() const {return buffer_.size() - writerIndex_;}
    size_t prependableBytes() const {return readerIndex_;}

    const char* peek() const{ return begin()+readerIndex_; }       // 返回缓存区中可读数据的起始地址

    void retrieve(size_t len){
        if(len<readableBytes()){
            readerIndex_ += len;    // 应用只读取了可读缓冲区数据的一部分，就是len，还剩下readerIndex_ += len -》writerIndex_
        }
        else{
            retrieveAll();
        }
    }

    void retrieveAll(){
        readerIndex_ = writerIndex_ = kCheapPrepend;
    }

    // 把onMessage函数上报的Buffer数据，转成string类型的数据返回
    std::string retrieveAllAsString(){return retrieveAsString(readableBytes());}

    std::string retrieveAsString(size_t len){
        std::string result(peek(),len);
        retrieve(len);
        return result;
    }

    void ensureWrioteableBytes(size_t len){
        if(writeableBytes()<len){
            makeSpace(len);
        }
    }

    // 把[data,data+len]内存上的数据，添加到writable缓冲区当中
    void append(const char* data,size_t len){
        ensureWrioteableBytes(len);
        // beginWrite: &*buffer_.begin()
        std::copy(data,data+len,beginWrite());
        writerIndex_ += len;
    }

    char* beginWrite(){
        return begin()+writerIndex_;
    }
    const char* beginWrite() const{
        return begin()+writerIndex_;
    }

    // 从fd上读取数据   利用栈上的空间
    ssize_t readFd(int fd,int* saveErrno);
    // 通过fd发送数据
    ssize_t writeFd(int fd,int* saveErrno);
private:
    char* begin(){
        return &*buffer_.begin();
    }
    const char* begin() const{
        return &*buffer_.begin();
    }

    void makeSpace(size_t len){
        // 如果buffer的大小减去write（剩余可写的空间）+ 前半部分已经读完的空间  <   需要写的空间加上空的8个字节   需要扩容
        // buffer的扩容有许多优点，刚开始的空间并不会很大，随着使用空间扩容，扩容后的空间并不会减少到刚开始的大小
        // 这样有很多好处，避免了频繁的创建和缩小空间
        if(writeableBytes()+prependableBytes()<len+kCheapPrepend){
            buffer_.resize(writerIndex_+len);
        }
        else{       // 否则的话将数据前移，把可写的空间移动到最后面，这样空间就足够了
            size_t readable = readableBytes();  // 需要赋值的大小
            // copy第一个参数是复制的起始地址，第二个参数是数据复制到哪，第三个参数是复制到vector的哪个位置开始
            std::copy(begin()+readerIndex_,begin()+writerIndex_,begin()+kCheapPrepend);
            readerIndex_ = kCheapPrepend;   // 复制到kCheapPrepend下标
            writerIndex_ = readerIndex_ + readable;     // 写的下标加上需要复制的大小
        }
    }

    std::vector<char> buffer_;      // buffer用的vector
    size_t readerIndex_;            // 读下标
    size_t writerIndex_;            // 写下标
};
