#pragma once

/*
    noncopyable被继承以后，派生类对象可以正常的构造和析构，但是派生类对象无法进行拷贝构造和赋值操作
*/

class noncopyable{
public:
    noncopyable(const noncopyable& other) = delete;
    noncopyable& operator=(const noncopyable&) = delete;
protected:
    noncopyable() = default;
    ~noncopyable() = default;
};

/**
 * 
 * 防止意外的拷贝，进行资源管理，保持对象的不变性，控制对象的声明周期
 * 
*/