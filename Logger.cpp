#include"Logger.h"
#include"Timestamp.h"

#include<iostream>
#include<ctime>

// 获取日志的唯一实例对象
Logger& Logger::instance(){
    static Logger logger;       // 单例模式，在汇编中自动加锁
    return logger;
}

// 设置日志级别
void Logger::setLogLevel(int level){
    logLevel_ = level;
}

// 写日志  time [级别信息] : msg
void Logger::log(std::string msg){
    std::cout<<Timestamp::now().toString()<<"  ";
    switch (logLevel_)
    {
    case INFO:
        std::cout<<"[INFO]"<<": ";
        break;
    case ERROR:
        std::cout<<"[ERROR]"<<": ";
        break;
    case FATAL:
        std::cout<<"[FATAL]"<<": ";
        break;
    case DEBUG:
        std::cout<<"[DEBUG]"<<": ";
        break;
    default:
        break;
    }
    std::cout<<msg<<std::endl;
}
