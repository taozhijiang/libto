#ifndef _THREAD_HPP_
#define _THREAD_HPP_

#include "general.hpp"
#include <boost/noncopyable.hpp>

#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/condition_variable.hpp>
#include <signal.h>

#include <algorithm>

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

    explicit Thread(int thread_idx):
        Thread(thread_idx, 0)
    {}

    Thread(int thread_idx, std::size_t max_event):
        TaskOperation(thread_idx),
        Epoll(max_event)
    {
        sigignore(SIGPIPE);
        current_task_ = nullptr;
        // Error!!! not for thread
        //GetThreadInstance().thread_ = this;
        BOOST_LOG_T(debug) << "Create New Thread: " << boost::this_thread::get_id();
    }

    /**
     * These xxxTask interface should be call by main thread with
     * lock externally, internel should directly manipulate data for
     * better performance
     */
    void addTask(const Task_Ptr& p_task, TaskStat stat = TaskStat::TASK_RUNNING) override {
        assert (stat == TaskStat::TASK_RUNNING);

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
        {
            boost::lock_guard<boost::mutex> task_lock(task_mutex_);
            p_task->setTaskStat(TaskStat::TASK_BLOCKING);
            task_list_.remove(p_task);
        }
        task_blocking_list_[fd] = p_task;

        return;
    }

    void resumeTask(const Task_Ptr& p_task) override {
        {
            boost::lock_guard<boost::mutex> task_lock(task_mutex_);
            p_task->setTaskStat(TaskStat::TASK_RUNNING);
            task_list_.push_back(p_task);
        }
        task_notify_.notify_one();
    }

    /* End xxxTask */

    void removeBlockingFd(int fd) override {
        task_blocking_list_.erase(fd);
    }

    std::size_t getTaskSize() {
        boost::lock_guard<boost::mutex> task_lock(task_mutex_);
        return task_list_.size();
    }

    std::size_t getBlockingSize() {
        return task_blocking_list_.size();
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

            assert(ptr->getTaskStat() == TaskStat::TASK_RUNNING);
        }

        current_task_ = ptr;
        ptr->swapIn();

        // 此处ptr被协程切换回来了
        current_task_ = null_task_;
        if (ptr->isFinished()) {
            // Task会被自动析构，后续考虑搞个对象缓存列表
            BOOST_LOG_T(debug) << "Job is finished with " << ptr->t_id_ ;
        }
        else{
            if (ptr->getTaskStat() == TaskStat::TASK_RUNNING) {
                boost::lock_guard<boost::mutex> task_lock(task_mutex_);
                task_list_.push_back(ptr);
            }
            // if blocking case, already added to task_blocking_list_
            else if (ptr->getTaskStat() == TaskStat::TASK_BLOCKING)
            {
                /* Noop */;
            }
            else{
                BOOST_LOG_T(error) << "Error task stat: " << static_cast<int>(ptr->getTaskStat()) << std::endl;
                ::abort();
            }
        }

        return true;
    }

    std::size_t RunTask() override
    {
        // ATTENTION !!!
        // VERY IMPORTANT !!!!
        GetThreadInstance().thread_ = this;

        BOOST_LOG_T(info) << "Worker Thread RunTask() ..." ;
        std::size_t total = do_run_task(false);
        BOOST_LOG_T(info) << "Already run " << total << " serivces... " ;

        return total;
    }

    std::size_t RunUntilNoTask() override
    {
        // ATTENTION !!!
        // VERY IMPORTANT !!!!
        GetThreadInstance().thread_ = this;

        BOOST_LOG_T(info) << "Worker Thread RunUntilNoTask() ..." ;
        std::size_t total = do_run_task(true);
        BOOST_LOG_T(info) << "Already run " << total << " serivces... " ;

        return total;
    }

    // will be called by main thread, watch out for mutex protect
    std::size_t traverseTaskEvents(int ms=0) override
    {
        std::vector<int> fd_coll;
        std::vector<Task_Ptr> ready;

        if(traverseEvent(fd_coll, ms))
        {
            assert(!fd_coll.empty());

            for (auto fd: fd_coll) {

                auto tk = task_blocking_list_[fd];
                tk->setTaskStat(TaskStat::TASK_RUNNING);
                ready.push_back(tk);
                task_blocking_list_.erase(fd);

                delEvent(fd);
                removeBlockingFd(fd);
            }

            {
                boost::lock_guard<boost::mutex> task_lock(task_mutex_);
                for (auto& item: ready){
                    task_list_.push_back(item);
                }
            }

            task_notify_.notify_one();
        }

        return ready.size();
    }


    virtual ~Thread() {
        BOOST_LOG_T(info) << "Boost Thread Exit... ";
    }

    Task_Ptr getCurrentTask() const override {
        return current_task_;
    }

    bool isInCoroutine() const override {
        return !!current_task_;
    }

private:
    size_t do_run_task(bool until_break) {

        std::size_t total = 0;
        std::size_t this_round = 0;
        bool real_do = false;

        for (;;) {
            real_do = false;

            {
                boost::lock_guard<boost::mutex> task_lock(task_mutex_);
                if (until_break && task_list_.empty() && task_blocking_list_.empty())
                   break;
            }

            this_round = 0;
            do {
                if (do_run_one()) {
                    ++ total;
                    real_do = true;
                }
            } while ( ++this_round < 20 );

            if(traverseTaskEvents(0))
                real_do = true;

            if (!real_do) {
                usleep(10*1000);
            }
        }

        return total;
    }

private:
    Task_Ptr      current_task_;
    // 所有的任务和定时器都是从主线程添加的，所以这个mutex是必须的
    boost::mutex  task_mutex_;
    boost::condition_variable_any  task_notify_;
};

using Thread_Ptr = std::shared_ptr<Thread>;

}

#endif //_THREAD_HPP_

