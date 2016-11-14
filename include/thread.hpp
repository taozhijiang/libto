#ifndef _THREAD_HPP_
#define _THREAD_HPP_

#include "general.hpp"
#include <boost/noncopyable.hpp>

#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/condition_variable.hpp>
#include <signal.h>

#include "epoll.hpp"

namespace libto {

class Thread : public boost::noncopyable, public TaskOperation, private Epoll {
public:

    Thread():
        Thread(0)
    {}

    explicit Thread(std::size_t max_event):
        Epoll(max_event)
    {
        sigignore(SIGPIPE);

        BOOST_LOG_T(debug) << "New Thread: " << boost::this_thread::get_id() << endl;
    }

    void CreateTask(TaskFunc const& func)
    {
        Task_Ptr p_task( new Task(func, this));
        p_task->setTaskStat(TaskStat::TASK_RUNNING);
        {
            boost::lock_guard<boost::mutex> task_lock(task_mutex_);
            task_list_.push_back(p_task);
        }
        task_notify_.notify_one();
        return;
    }

    bool do_run_one() override
    {
        Task_Ptr ptr;

        {
            boost::lock_guard<boost::mutex> task_lock(task_mutex_);
            if (task_list_.empty())
                return false;

            ptr = task_list_.front();
            task_list_.pop_front();
        }

        current_task_ = ptr;
        ptr->swapIn();

        // 此处ptr被协程切换回来了
        current_task_ = null_task_;
        if (ptr->isFinished()) {
            // Task会被自动析构，后续考虑搞个对象缓存列表
            BOOST_LOG_T(debug) << "Job is finished with " << ptr->t_id_ << endl;
        }
        else{
            boost::lock_guard<boost::mutex> task_lock(task_mutex_);
            task_list_.push_back(ptr);
        }

        return true;
    }

    std::size_t RunTask() override
    {
        std::size_t n = 0;
        bool ret = false;

        for (;;) {
            if( (ret = do_run_one()) )
                ++ n;

            {
                boost::unique_lock<boost::mutex> task_lock(task_mutex_);
                while (task_list_.empty()) {
                    task_notify_.wait(task_lock);
                }
            }
        }

        BOOST_LOG_T(info) << "Already run " << n << " serivces... " << endl;
        return n;
    }

    std::size_t RunUntilNoTask() override
    {
        std::size_t n = 0;
        bool ret = false;

        for (;;) {
            if( (ret = do_run_one()) )
                ++ n;
            {
                boost::lock_guard<boost::mutex> task_lock(task_mutex_);
                if (task_list_.empty())
                   break;
            }
        }

        BOOST_LOG_T(info) << "Already run " << n << " serivces... " << endl;
        return n;
    }

    std::size_t traverseTaskEvents(std::vector<int>& fd_coll, int ms=0) override
    {
        std::size_t ret = 0;

        if(traverseEvent(fd_coll, ms) && !fd_coll.empty())
        {
            boost::lock_guard<boost::mutex> task_lock(task_mutex_);
            for (auto fd: fd_coll){
                if (auto tk = task_blocking_list_[fd].lock())
                {
                    ++ ret;
                    task_list_.emplace_front(tk);
                }

                // 目前只以oneshot的方式使用，后续待优化
                delEvent(fd);
                task_blocking_list_.erase(fd);
            }

            if (ret > 0)
                task_notify_.notify_one();
        }

        return ret;
    }


    virtual ~Thread() {
        BOOST_LOG_T(info) << "Boost Thread Exit... " << endl;
    }

private:
    boost::mutex  task_mutex_;
    boost::condition_variable_any  task_notify_;
};

using Thread_Ptr = std::shared_ptr<Thread>;

}

#endif //_THREAD_HPP_

