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

// Can be reference by through Scheduler::getThreadInstance
class Thread;
struct ThreadLocalInfo
{
    Thread *thread_ = nullptr;
};
extern ThreadLocalInfo& GetThreadInstance();

class Thread : public boost::noncopyable, public TaskOperation, public Epoll {
public:

    Thread():
        Thread(0)
    {}

    explicit Thread(std::size_t max_event):
        Epoll(max_event)
    {
        sigignore(SIGPIPE);
        current_task_ = nullptr;
        GetThreadInstance().thread_ = this;
        BOOST_LOG_T(debug) << "Create New Thread: " << boost::this_thread::get_id() << endl;
    }

    void createTask(TaskFunc const& func) override
    {
        Task_Ptr p_task( new Task(func));
        addTask(p_task);
        return;
    }

    void addTask(const Task_Ptr& p_task, TaskStat stat = TaskStat::TASK_RUNNING) override {
        {
            boost::lock_guard<boost::mutex> task_lock(task_mutex_);
            p_task->setTaskStat(stat);
            task_list_.push_back(p_task);
        }
        if (stat == TaskStat::TASK_RUNNING) 
            task_notify_.notify_one(); 
        return;
    }

    void removeTask(const Task_Ptr& p_task) override {
        boost::lock_guard<boost::mutex> task_lock(task_mutex_);
        task_list_.remove(p_task);
        return;
    }

    void blockTask(int fd, const Task_Ptr& p_task) override {
        boost::lock_guard<boost::mutex> task_lock(task_mutex_);
        p_task->setTaskStat(TaskStat::TASK_BLOCKING);
        task_blocking_list_[fd] = p_task;
    }

    void resumeTask(const Task_Ptr& p_task) override {
        boost::lock_guard<boost::mutex> task_lock(task_mutex_);
        p_task->setTaskStat(TaskStat::TASK_RUNNING); 
    }

    void removeBlockingFd(int fd) override {
        task_blocking_list_.erase(fd);
    }

    Task_Ptr getCurrentTask() const override
    {
        return current_task_;
    }

    bool isInCoroutine() const override {
        return !!current_task_;
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
                    resumeTask(tk); 
                }

                // 目前只以oneshot的方式使用，后续待优化
                delEvent(fd);
                removeBlockingFd(fd);
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
    Task_Ptr      current_task_;
    boost::mutex  task_mutex_;
    boost::condition_variable_any  task_notify_;
    boost::mutex  task_blocking_mutex_;
};

using Thread_Ptr = std::shared_ptr<Thread>;

}

#endif //_THREAD_HPP_

