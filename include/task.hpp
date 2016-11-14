#ifndef _TASK_HPP_
#define _TASK_HPP_

#include "general.hpp"
#include "context.hpp"

#include <deque>
#include <map>
#include <vector>

namespace libto {

class Task;
class TaskOperation;
using TaskFunc = std::function<void()>;
using Task_Ptr = std::shared_ptr<Task>;
using Task_Weak = std::weak_ptr<Task>;

class Thread;

enum class TaskStat
{
    TASK_INITIALIZE,
    TASK_RUNNING,
    TASK_BLOCKING,
    TASK_STOPPED,
};

class Task {
    friend class TaskOperation;

public:
    Task(TaskFunc const& fn, Thread* p_thread):
    t_id_(task_uuid++),
    task_stat_(TaskStat::TASK_INITIALIZE),
    func_(fn),
    context_([this] {Task_Callback();}),
    p_thread_(p_thread)
    {
        BOOST_LOG_T(info) << "Created Coroutine with task_id: " << t_id_ << std::endl;
    }

    void Task_Callback() {
        if (func_)
            func_();

        func_ = TaskFunc(); // destruct std::func object
    }

    bool swapIn() {
        return context_.swapIn();
    }

    bool swapOut() {
        return context_.swapOut();
    }

    bool isFinished(){
        return !func_;
    }

    void setTaskStat(TaskStat stat) {
        task_stat_ = stat;
    }

    TaskStat getTaskStat() {
        return task_stat_;
    }

    ~Task() {
        BOOST_LOG_T(info) << "Terminated Coroutine with task_id: " << t_id_ << std::endl;
    }

public:
    const uint64_t t_id_;

private:
    static uint64_t task_uuid;

    TaskStat  task_stat_;
    TaskFunc  func_;
    Context   context_;
    Thread*   p_thread_;
};


class TaskOperation {
public:
    TaskOperation():
    task_list_(),current_task_(nullptr) {}

    Task_Ptr GetCurrentTask() const
    {
        return current_task_;
    }

    bool InCoroutine() const {
        return !!current_task_;
    }

    // 主线程中会返回nullptr
    Thread* GetCurrentThead() const
    {
        if (!current_task_)
            return nullptr;

        return current_task_->p_thread_;
    }

    virtual bool do_run_one() = 0;
    virtual std::size_t RunUntilNoTask() = 0;
    virtual std::size_t RunTask() = 0;

    // ms = -1, forever
    virtual std::size_t traverseTaskEvents(std::vector<int>& fd_coll, int ms=0) = 0;

    virtual ~TaskOperation() = default;

protected:
    std::deque<Task_Ptr> task_list_;
    Task_Ptr current_task_;

    // IO 等待的列表，socket/fd
    std::map<int, Task_Weak> task_blocking_list_;

    // 空闲的Task对象缓存队列
    std::vector<Task_Ptr> task_obj_cache_;

    static Task_Ptr null_task_;
};


}



#endif // _TASK_HPP_
