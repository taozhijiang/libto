#include "general.hpp"

#include "coroutine.hpp"
#include "task.hpp"
#include "thread.hpp"

namespace libto {

    uint64_t Task::task_uuid = 0;
    Task_Ptr TaskOperation::null_task_ = nullptr;

    int st_make_nonblock(int socket)
    {
        int flags = 0;

        flags = fcntl (socket, F_GETFL, 0);
    	flags |= O_NONBLOCK;
        fcntl (socket, F_SETFL, flags);

        return 0;
    }


    ThreadLocalInfo& GetThreadInstance()
    {
        static __thread ThreadLocalInfo *info = nullptr;

        if (!info)
            info = new ThreadLocalInfo();

        return *info;
    }

    TaskOperation* GetCurrentTaskOperation() {
        Thread *p_thread = GetThreadInstance().thread_;

        if (!p_thread)
            return &Scheduler::getInstance();
        else
            return p_thread;
    }

    Epoll* GetCurrentEpoll() {
        Thread *p_thread = GetThreadInstance().thread_;
        if (!p_thread)
            return &Scheduler::getInstance();
        else
            return p_thread;
    }


    void _sch_yield() {
        Task_Ptr curr_ = GetCurrentTaskOperation()->getCurrentTask();
        if(curr_)
            curr_->swapOut();
    }

    // return timerfd, -1 for error
    int _timer_prep(std::size_t msec, struct itimerspec& itv, bool forever) {
        int timerfd = -1;
        struct timespec   now;

        if (msec == 0)
            return -1;

        if( (timerfd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK)) == -1)
            return -1;

        clock_gettime(CLOCK_MONOTONIC, &now);
        memset(&itv, 0, sizeof(struct itimerspec));
        itv.it_value.tv_sec = now.tv_sec + msec/1000;
        itv.it_value.tv_nsec = now.tv_nsec + (msec%1000)*1000*1000;

        if (forever) {
            itv.it_interval.tv_sec = msec / 1000;
            itv.it_interval.tv_nsec = (msec%1000)*1000*1000;
        }

        if( timerfd_settime(timerfd, TFD_TIMER_ABSTIME, &itv, NULL) == -1){
            close(timerfd);
            return -1;
        }

        return timerfd;
    }
#if 0
    void _sch_sleep_ms(std::size_t msec) {
        Task_Ptr curr_ = GetCurrentTaskOperation()->getCurrentTask();
        if(curr_)
            curr_->swapOut();
    }
#endif
    void _sch_read(int fd) {
        Task_Ptr curr_ = GetCurrentTaskOperation()->getCurrentTask();
        GetCurrentEpoll()->addEvent(fd,  EPOLLIN | EPOLLERR);
        GetCurrentTaskOperation()->blockTask(fd, curr_);
        curr_->swapOut();
    }

    void _sch_write(int fd) {
        Task_Ptr curr_ = GetCurrentTaskOperation()->getCurrentTask();
        GetCurrentEpoll()->addEvent(fd,  EPOLLIN | EPOLLERR);
        GetCurrentTaskOperation()->blockTask(fd, curr_);
        curr_->swapOut();
    }

    void _sch_rdwr(int fd) {
        Task_Ptr curr_ = GetCurrentTaskOperation()->getCurrentTask();
        GetCurrentEpoll()->addEvent(fd,  EPOLLIN | EPOLLERR);
        GetCurrentTaskOperation()->blockTask(fd, curr_);
        curr_->swapOut();
    }

}

