#pragma once

#include <string>

#include "noncopyable.h"

// LOG_INFO("%s %d",arg1,arg2)      在大型项目中。宏好几行代码的时候为了防止造成意想不到的错误用个do while(0)
#define LOG_INFO(logmsgFormat,...) \
    do \
    {   \
        Logger &logger = Logger::instance(); \
        logger.setLogLevel(INFO); \
        char buf[1024] = {0}; \
        snprintf(buf,1024,logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf);    \
    } while(0);

#define LOG_ERROR(logmsgFormat,...) \
    do \
    {   \
        Logger &logger = Logger::instance(); \
        logger.setLogLevel(ERROR); \
        char buf[1024] = {0}; \
        snprintf(buf,1024,logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf);    \
    } while(0);

#define LOG_FATAL(logmsgFormat,...) \
    do \
    {   \
        Logger &logger = Logger::instance(); \
        logger.setLogLevel(FATAL); \
        char buf[1024] = {0}; \
        snprintf(buf,1024,logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf);    \
        exit(-1);       \
    } while(0);

// DEBUG信息非常多 通过宏开启和关闭
#ifdef MUDUO_BUG
#define LOG_DEBUG(logmsgFormat,...) \
    do \
    {   \
        Logger &logger = Logger::instance(); \
        logger.setLogLevel(DEGUG); \
        char buf[1024] = {0}; \
        snprintf(buf,1024,logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf);    \
    } while(0);
#else
    #define LOG_DEBUG(logmsgFormat,...)
#endif

// 定义日志的级别  INFO  ERROR（不影响程序的运行不用退出）  FATAL（毁灭性的，需要退出）  DEGUG（通过一个宏来开启/关闭DEBUG的输出）
enum LogLevel{
    INFO,   // 普通信息
    ERROR,  // 错误信息
    FATAL,  // core信息
    DEBUG,  // 调试信息
};

// 输出一个日志类
class Logger:noncopyable{       // 禁用拷贝构造与赋值运算符，避免资源竞争和重复，不禁用可能有多个实例同时修改日志，导致资源竞争与错误
public:
    // 获取日志的唯一实例对象      像日志数据库这种模块只需要获取一个实例
    static Logger& instance();
    // 设置日志级别
    void setLogLevel(int level);
    // 写日志
    void log(std::string msg);
private:
    int logLevel_;     // 系统的变量通常是以_开头，陈硕在成员变量的后面加个_
    Logger(){}       // 单例模式把构造函数私有化
};
