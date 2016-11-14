#ifndef _SCHEDULER_HPP_
#define _SCHEDULER_HPP_

#include "general.hpp"
#include <boost/noncopyable.hpp>
#include "task.hpp"
#include "thread.hpp"
#include "epoll.hpp"

#include <boost/thread.hpp>

#include <sys/timerfd.h>

#include <vector>

namespace libto {

class Scheduler: public boost::noncopyable, public TaskOperation, public Epoll
{
public:
    static Scheduler& getInstance()
    {
        static Scheduler obj;
        return obj;
    }

    void addTask(const Task_Ptr& p_task, TaskStat stat = TaskStat::TASK_RUNNING) override {
        p_task->setTaskStat(stat);
        task_list_.push_back(p_task);
        return;
    }

    void removeTask(const Task_Ptr& p_task) override {
        task_list_.remove(p_task);
        return;
    }

    void blockTask(int fd, const Task_Ptr& p_task) override {
        p_task->setTaskStat(TaskStat::TASK_BLOCKING);
        task_blocking_list_[fd] = p_task;
    }

    void resumeTask(const Task_Ptr& p_task) override {
        p_task->setTaskStat(TaskStat::TASK_RUNNING);
    }

    void removeBlockingFd(int fd) override {
        task_blocking_list_.erase(fd);
    }


    // dispatch 从0开始的线程索引号
    void createTask(TaskFunc const& func, std::size_t dispatch)
    {
        if ( (dispatch + 1) > thread_list_.size() )
            thread_list_.resize(dispatch + 1);

        if (!thread_list_[dispatch])
        {
            Thread_Ptr th = std::make_shared<Thread>();
            thread_group_.create_thread(boost::bind(&Thread::RunTask, th));
            thread_list_[dispatch] = th;
            BOOST_LOG_T(debug) << "Create Thread Index @: " << dispatch << endl;
        }

        thread_list_[dispatch]->createTask(func);
        return;
    }

    /**
     * Scheduler中不考虑锁的保护操作
     */
    bool do_run_one() override
    {
        if (task_list_.empty())
            return false;

        Task_Ptr ptr = task_list_.front();
        task_list_.pop_front();

        // move front to tail
        if (ptr->getTaskStat() != TaskStat::TASK_RUNNING) {
            task_list_.push_back(ptr);
            return false;
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
            task_list_.push_back(ptr);
        }

        return true;
    }

    std::size_t RunTask() override
    {
        std::size_t n = 0;
        bool ret = false;
        std::vector<int> fd_coll;

        BOOST_LOG_T(log) << "Main Thread RunTask() ..." << endl;

        for (;;) {
            if( (ret = do_run_one()) )
                ++ n;

            // Main Thread Check
            traverseTaskEvents(fd_coll, 50);
            ::sleep(1);

            // Other Thread Check
            for (auto& ithread: thread_list_){
                if (ithread){
                    // immediately
                    ithread->traverseTaskEvents(fd_coll);
                }
            }
        }

        BOOST_LOG_T(info) << "Already run " << n << " serivces... " << endl;
        return n;
    }

    // RunUntilNoTask() 不会处理其他线程的事件，此处需要注意
    std::size_t RunUntilNoTask() override
    {
        std::size_t n = 0;
        bool ret = false;
        std::vector<int> fd_coll;

        BOOST_LOG_T(log) << "Main Thread RunUntilNoTask() ..." << endl;

        if (!thread_list_.empty()) {
            BOOST_LOG_T(log) << "RunUntilNoTask will not handle thread events, do not use these!" << endl;
            ::abort();
        }

        for (;;) {
            if( (ret = do_run_one()) )
                ++ n;

            // Main Thread Check
            traverseTaskEvents(fd_coll, 50);

            if (task_list_.empty())
               break;
        }

        BOOST_LOG_T(info) << "Already run " << n << " serivces... " << endl;
        return n;
    }

    std::size_t traverseTaskEvents(std::vector<int>& fd_coll, int ms=50) override
    {
        std::size_t ret = 0;

        if(traverseEvent(fd_coll, ms) && !fd_coll.empty())
        {
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
        }

        return ret;
    }

    void joinAllThreads() {
        thread_group_.join_all();
    }

    Task_Ptr getCurrentTask() const override {
        return current_task_;
    }

    bool isInCoroutine() const override {
        return !!current_task_;
    }

private:
    Scheduler(): Scheduler(0)
    {}

    Scheduler(std::size_t max_event):
        Epoll(max_event)
    {
        current_task_ = nullptr;
        GetThreadInstance().thread_ = nullptr;
    }

    Task_Ptr               current_task_;
    boost::thread_group    thread_group_;
    std::vector<Thread_Ptr> thread_list_; //默认在main thread中执行的协程
};

}



#endif // _SCHEDULER_HPP_
