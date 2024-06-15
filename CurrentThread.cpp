#include "CurrentThread.h"

namespace CurrentThread{
    thread_local int t_cachedTid = 0;

    void cacheTid(){
        if(t_cachedTid==0){
            // 通过Linux系统调用，获取当前线程的tid值
            t_cachedTid = static_cast<pid_t>(::syscall(SYS_gettid));
        }
    }
}
