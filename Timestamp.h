#pragma once

#include<iostream>
#include<string>

class Timestamp{
public:
    Timestamp();
    explicit Timestamp(int64_t microSecondsSinceEpoch);     // explict是为了反正隐式转换为Timestamp类对象，避免不必要的错误
    static Timestamp now();
    std::string toString() const;
private:
    int64_t microSecondsSinceEpoch_;
};




/**
 * 比如没有加explicit这个关键字    可以这样调用构造函数    Timestamp  Timestamp = 23;
 * 但如果有这个关键字只能显示的调用    Timestamp Timestamp(23);
 * 
 * 比如
 * 
 * 
class MyClass {
public:
	explicit MyClass(int x) : value(x) {}

private:
	int value;
};

void foo(MyClass obj) {
	std::cout << "MyClass version" << std::endl;
}

//void foo(int x) {
//	cout << "int version" << endl;
//}

int main() {
	foo(42);
	return 0;
}
 *          当构造函数声明为explicit时，42不能隐式的转换为MyClass类的对象
 *   注： 当编译器没有声明explicit时，传入42会优先调用int类型的（重载决策）
*/