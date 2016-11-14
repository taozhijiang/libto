#ifndef _LIBTO_HPP_
#define _LIBTO_HPP_

#include "general.hpp"

#include "scheduler.hpp"
#include "coroutine.hpp"
#include "task.hpp"
#include "epoll.hpp"
#include "thread.hpp"

using coroutine = libto::Coroutine;
using scheduler = libto::Scheduler;

namespace libto {
inline TaskOperation* GetCurrentTaskOperation() {
    Thread *p_thread = GetThreadInstance().thread_;
    if (!p_thread)
    {
        std::cout << "Main Thread" << endl;
        return &Scheduler::getInstance();
    }
    else
    {
        std::cout << "Worker Thread" << endl;
        return p_thread;
    }
}

inline Epoll* GetCurrentEpoll() {
    Thread *p_thread = GetThreadInstance().thread_;
    if (!p_thread)
        return &Scheduler::getInstance();
    else
        return p_thread;
}

}

#define sch_yield do{  libto::Task_Ptr curr_ = libto::GetCurrentTaskOperation()->getCurrentTask(); \
                        if(curr_) curr_->swapOut(); \
                   } while(0);

#define RunTask          do{ libto::GetCurrentTaskOperation()->RunTask(); } while(0);
#define RunUntilNoTask   do{ libto::GetCurrentTaskOperation()->RunUntilNoTask(); } while(0);
#define JoinAllThreads   do{ libto::Scheduler::getInstance().joinAllThreads(); } while(0);

#define sch_read(fd)  do{ \
                        libto::Task_Ptr curr_ = libto::GetCurrentTaskOperation()->getCurrentTask(); \
                        libto::GetCurrentEpoll()->addEvent(fd,  EPOLLIN | EPOLLERR); \
                        libto::GetCurrentTaskOperation()->blockTask(fd, curr_); \
                        curr_->swapOut(); \
                        } while(0);

#define sch_write(fd) do{ \
                        libto::Task_Ptr curr_ = libto::GetCurrentTaskOperation()->getCurrentTask(); \
                        libto::GetCurrentEpoll()->addEvent(fd,  EPOLLOUT | EPOLLERR); \
                        libto::GetCurrentTaskOperation()->blockTask(fd, curr_); \
                        curr_->swapOut(); \
                        } while(0);

#define sch_rw(fd)    do{ \
                        libto::Task_Ptr curr_ = libto::GetCurrentTaskOperation()->getCurrentTask(); \
                        libto::GetCurrentEpoll()->addEvent(fd,  EPOLLIN | EPOLLOUT | EPOLLERR); \
                        libto::GetCurrentTaskOperation()->blockTask(fd, curr_); \
                        curr_->swapOut(); \
                        } while(0);

#endif // _LIBTO_HPP_
