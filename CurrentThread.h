#pragma once 

#include<unistd.h>
#include<sys/syscall.h>

namespace CurrentThread{
    // __thread  thread_local    不加__thread  后面的变量是全局的，所有线程共享的
    extern thread_local int t_cachedTid;
    void cacheTid();

    inline int tid(){
        // __builtin_expect(t_cachedTid == 0, 0) 告诉编译器，t_cachedTid == 0 很少发生，
        // 从而优化代码，使得编译器为 t_cachedTid != 0 的情况生成更高效的代码。
        if(__builtin_expect(t_cachedTid == 0, 0)){
            cacheTid();
        }
        return t_cachedTid;
    }
}
