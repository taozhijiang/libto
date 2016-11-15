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
    extern TaskOperation* GetCurrentTaskOperation();
    extern Epoll* GetCurrentEpoll();

    extern void _sch_yield();
    extern int _timer_prep(std::size_t msec, bool forever);
    extern bool _sch_sleep_ms(std::size_t msec);
    extern void _sch_read(int fd);
    extern void _sch_write(int fd);
    extern void _sch_rdwr(int fd);

}

#define sch_yield do{  libto::Task_Ptr curr_ = libto::GetCurrentTaskOperation()->getCurrentTask(); \
                        if(curr_) curr_->swapOut(); \
                   } while(0);

#define RunTask          do{ libto::GetCurrentTaskOperation()->RunTask(); } while(0);
#define RunUntilNoTask   do{ libto::GetCurrentTaskOperation()->RunUntilNoTask(); } while(0);
#define JoinAllThreads   do{ libto::Scheduler::getInstance().joinAllThreads(); } while(0);

#define sch_read(fd)  libto::_sch_read(fd)
#define sch_write(fd) libto::_sch_write(fd)
#define sch_rdwr(fd)  libto::_sch_rdwr(fd)

#define sch_sleep_ms(msec) libto::_sch_sleep_ms(msec)

#endif // _LIBTO_HPP_
