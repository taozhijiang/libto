#ifndef _TASK_HPP_
#define _TASK_HPP_

#include "general.hpp"
#include "context.hpp"

#include <list>
#include <map>
#include <vector>

#include <sys/timerfd.h>

namespace libto {

class Task;
class TaskOperation;
using TaskFunc = std::function<void()>;
using Task_Ptr = std::shared_ptr<Task>;
using Task_Weak = std::weak_ptr<Task>;

extern void _sch_yield();
extern void _sch_read(int fd);
extern void _sch_write(int fd);
extern void _sch_rdwr(int fd);
extern int  _timer_prep(std::size_t msec, bool forever);

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
    Task(TaskFunc const& fn):
    t_id_(task_uuid++),
    task_stat_(TaskStat::TASK_INITIALIZE),
    func_(fn),
    context_([this] {Task_Callback();})
    {
        BOOST_LOG_T(info) << "Created Coroutine with task_id: " << t_id_ ;
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
        BOOST_LOG_T(info) << "Terminated Coroutine with task_id: " << t_id_ ;
    }

    static uint64_t currentTaskUUID() {
        return task_uuid;
    }

public:
    const uint64_t t_id_;

protected:
    static uint64_t task_uuid;

    TaskStat  task_stat_;
    TaskFunc  func_;
    Context   context_;
};


class TaskOperation {
public:
    TaskOperation():
    task_list_() {}

    void createTask(TaskFunc const& func) {
        Task_Ptr p_task( new Task(func) );
        addTask(p_task);
        return;
    }

    int createTimer(TaskFunc const& func, std::size_t msec, bool forever = false) {
        int timerfd;

        if ( (timerfd = _timer_prep(msec, forever)) == -1)
            return -1;

        if (forever) {
            Task_Ptr p_task( new Task([=] {
                for(;;){
                    char null_read[8];
                    _sch_read(timerfd);
                    read(timerfd, null_read, 8);
                    func();
                }
            }));

            addTask(p_task);
        }
        else {
            Task_Ptr p_task( new Task([=] {
                    char null_read[8];
                    _sch_read(timerfd);
                    read(timerfd, null_read, 8);
                    func();
            }));

            addTask(p_task);
        }

        return timerfd;
    }
    //virtual void removeTimer(int timerfd) = 0;

    virtual void addTask(const Task_Ptr &p_task, TaskStat stat = TaskStat::TASK_RUNNING) = 0;
    virtual void removeTask(const Task_Ptr& p_task) = 0;

    virtual void blockTask(int fd, const Task_Ptr& p_task) = 0;
    virtual void resumeTask(const Task_Ptr& p_task) = 0;
    virtual void removeBlockingFd(int fd) = 0;

    virtual bool do_run_one() = 0;
    virtual std::size_t RunUntilNoTask() = 0;
    virtual std::size_t RunTask() = 0;

    // ms = -1, forever
    virtual std::size_t traverseTaskEvents(std::vector<int>& fd_coll, int ms=0) = 0;

    virtual ~TaskOperation() = default;

    virtual Task_Ptr getCurrentTask() const = 0;
    virtual bool isInCoroutine() const = 0;

protected:
    std::list<Task_Ptr> task_list_;

    // IO 等待的列表，socket/fd
    std::map<int, Task_Weak> task_blocking_list_;

    // 空闲的Task对象缓存队列
    std::vector<Task_Ptr>    task_obj_cache_;

    static Task_Ptr null_task_;
};


}



#endif // _TASK_HPP_
