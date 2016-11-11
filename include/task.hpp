#ifndef _TASK_HPP_
#define _TASK_HPP_

#include "general.hpp"
#include "context.hpp"

namespace libto {

using TaskFunc = std::function<void()>;

class Task {
public:
    Task(TaskFunc const& fn):
    t_id_(task_uuid++),
    func_(fn),
    context_([this] {Task_Callback();})
    {
        BOOST_LOG_T(info) << "Created Coroutine with task_id: " << t_id_ << std::endl;
    }

    void Task_Callback() {
        if (func_)
            func_();

        func_ = TaskFunc(); // destruct std::func object
    }

    inline bool swapIn() {
        return context_.swapIn();
    }

    inline bool swapOut() {
        return context_.swapOut();
    }

    ~Task() { 
        BOOST_LOG_T(info) << "Terminated Coroutine with task_id: " << t_id_ << std::endl;
    }
private:
    static uint64_t task_uuid;

    uint64_t t_id_;
    TaskFunc func_;
    Context  context_;
};

using Task_Ptr = boost::shared_ptr<Task>;

}



#endif // _TASK_HPP_
