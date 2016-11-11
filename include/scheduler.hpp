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

    void RunTask()
    {
        while (true)
        {
            if (!task_list_.empty())
            {
                Task_Ptr ptr = task_list_.front();
                task_list_.pop_front();
                ptr->swapIn();
            }
        }
    }

private:
    Scheduler() = default;
    std::deque<Task_Ptr> task_list_;
};

}



#endif // _SCHEDULER_HPP_
