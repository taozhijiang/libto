#ifndef _SCHEDULER_HPP_
#define _SCHEDULER_HPP_

#include "general.hpp"
#include <boost/noncopyable.hpp>
#include "task.hpp"

#include <deque>

namespace libto {

class Scheduler:public boost::noncopyable
{
public:
    static Scheduler& getInstance()
    {
        static Scheduler obj;
        return obj;
    }

    void CreateTask(TaskFunc const& func)
    {
        Task_Ptr p_task( new Task(func));
        task_list_.push_back(p_task);
        return;
    }

    Task_Ptr GetCurrentTask() const
    {
        return current_task_;
    }

    bool do_run_one()
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
            BOOST_LOG_T(debug) << "Job is finished with " << ptr->t_id_ << endl;
        }
        else{
            task_list_.push_back(ptr);
        }

        return true;
    }

    std::size_t RunTask()
    {
        std::size_t n = 0;
        bool ret = false;
        for (;;) {
            if( ret = do_run_one() )
                ++ n;

            if (task_list_.empty())
                ::sleep(1);
        }

        BOOST_LOG_T(info) << "Already run " << n << " serivces... " << endl;
        return n;
    }

    std::size_t RunUntilNoTask()
    {
        std::size_t n = 0;
        bool ret = false;
        for (;;) {
            if( ret = do_run_one() )
                ++ n;

            if (task_list_.empty())
               break;
        }
        
        BOOST_LOG_T(info) << "Already run " << n << " serivces... " << endl;
        return n;
    }

private:
    Scheduler() = default;
    std::deque<Task_Ptr> task_list_;
    Task_Ptr current_task_;
    Task_Ptr null_task_;  //单例，反正就一个
};

}



#endif // _SCHEDULER_HPP_
