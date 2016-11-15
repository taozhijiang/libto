#include "general.hpp"

#include "coroutine.hpp"
#include "task.hpp"
#include "thread.hpp"

namespace libto {

    uint64_t Task::task_uuid = 0;
    Task_Ptr TaskOperation::null_task_ = nullptr;

    void _sch_yield();
    int _timer_prep(std::size_t msec, bool forever);
    bool _sch_sleep_ms(std::size_t msec);
    void _sch_read(int fd);
    void _sch_write(int fd);
    void _sch_rdwr(int fd);

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

    bool IsMainThread() {
        return  ! GetThreadInstance().thread_;
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

    // 依据定时器的原理实现
    bool _sch_sleep_ms(std::size_t msec) {
        int timerfd;
        // char null_read[8];

        // 主线程的主协程一定不能够被阻塞出去，否则永远无法切换回来
        // 虽然用户无法在主协程中添加东西，但是开发者要注意
        assert (! (IsMainThread() && !GetCurrentTaskOperation()->isInCoroutine()) );

        Task_Ptr curr_ = GetCurrentTaskOperation()->getCurrentTask();

        if ( msec == 0 || (timerfd = _timer_prep(msec, false)) == -1)
            return false;

        _sch_read(timerfd);
        // read(timerfd, null_read, 8);

        GetCurrentEpoll()->delEvent(timerfd);
        close(timerfd);

        return true;
    }

    // return timerfd, -1 for error
    int _timer_prep(std::size_t msec, bool forever) {
        int timerfd = -1;
        struct timespec now;
        struct itimerspec itv;

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

    void _sch_read(int fd) {

        assert (! (IsMainThread() && !GetCurrentTaskOperation()->isInCoroutine()) );

        Task_Ptr curr_ = GetCurrentTaskOperation()->getCurrentTask();
        GetCurrentEpoll()->addEvent(fd,  EPOLLIN | EPOLLERR);
        GetCurrentTaskOperation()->blockTask(fd, curr_);
        curr_->swapOut();
    }

    void _sch_write(int fd) {

        assert (! (IsMainThread() && !GetCurrentTaskOperation()->isInCoroutine()) );

        Task_Ptr curr_ = GetCurrentTaskOperation()->getCurrentTask();
        GetCurrentEpoll()->addEvent(fd,  EPOLLOUT | EPOLLERR);
        GetCurrentTaskOperation()->blockTask(fd, curr_);
        curr_->swapOut();
    }

    void _sch_rdwr(int fd) {

        assert (! (IsMainThread() && !GetCurrentTaskOperation()->isInCoroutine()) );

        Task_Ptr curr_ = GetCurrentTaskOperation()->getCurrentTask();
        GetCurrentEpoll()->addEvent(fd,  EPOLLIN | EPOLLOUT | EPOLLERR);
        GetCurrentTaskOperation()->blockTask(fd, curr_);
        curr_->swapOut();
    }

}

