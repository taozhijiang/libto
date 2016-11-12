#ifndef _SCHEDULER_HPP_
#define _SCHEDULER_HPP_

#include "general.hpp"
#include <boost/noncopyable.hpp>
#include "task.hpp"
#include "thread.hpp"

#include <boost/thread.hpp>

#include <deque>

namespace libto {

class Scheduler: public boost::noncopyable, public TaskOperation
{
public:
    static Scheduler& getInstance()
    {
        static Scheduler obj;
        return obj;
    }

    // Create coroutine task in main thread
    void CreateTask(TaskFunc const& func)
    {
        Task_Ptr p_task( new Task(func) );
        p_task->setTaskStat(TaskStat::TASK_RUNNING);
        task_list_.push_back(p_task);

        return;
    }

    // dispatch 从0开始的线程索引号
    void CreateTask(TaskFunc const& func, std::size_t dispatch)
    {
        if ( (dispatch + 1) > thread_list_.size() )
            thread_list_.resize(dispatch + 1);
        
        if (!thread_list_[dispatch])
        {
            Thread_Ptr th = std::make_shared<Thread>();
            thread_group_.create_thread(boost::bind(&Thread::RunTask, th));
            thread_list_[dispatch] = th;
            BOOST_LOG_T(info) << "Create Thread index: " << dispatch << endl;
        }

        thread_list_[dispatch]->CreateTask(func);
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
        for (;;) {
            if( (ret = do_run_one()) )
                ++ n;

            if (task_list_.empty())
                ::sleep(1);
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

            if (task_list_.empty())
               break;
        }
        
        BOOST_LOG_T(info) << "Already run " << n << " serivces... " << endl;
        return n;
    }

    void joinAllThreads() {
        thread_group_.join_all();
    }

private:
    Scheduler() = default;
    boost::thread_group    thread_group_;
    std::deque<Thread_Ptr> thread_list_; //默认在main thread中执行的协程
};

}



#endif // _SCHEDULER_HPP_
