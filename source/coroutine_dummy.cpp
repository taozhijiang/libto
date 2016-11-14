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

