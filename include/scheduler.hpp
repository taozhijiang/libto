#ifndef _SCHEDULER_HPP_
#define _SCHEDULER_HPP_

#include "general.hpp"
#include <boost/noncopyable.hpp>
#include "task.hpp"
#include "thread.hpp"
#include "epoll.hpp"

#include <boost/thread.hpp>
#include <boost/format.hpp>

#include <sys/timerfd.h>
#include <unistd.h>

#include <vector>

namespace libto {

class Scheduler: public boost::noncopyable, public TaskOperation, public Epoll
{
public:

    // 引入其他的重载版本
    using TaskOperation::createTask;
    using TaskOperation::createTimer;

    static Scheduler& getInstance()
    {
        static Scheduler obj;
        return obj;
    }

    void addTask(const Task_Ptr& p_task, TaskStat stat = TaskStat::TASK_RUNNING) override {
        assert(stat == TaskStat::TASK_RUNNING);

        p_task->setTaskStat(stat);
        task_list_.push_back(p_task);
        return;
    }

    // 基本都是链表的头部，所以效率应该不会差
    void removeTask(const Task_Ptr& p_task) override {
        task_list_.remove(p_task);
        return;
    }

    void blockTask(int fd, const Task_Ptr& p_task) override {
        p_task->setTaskStat(TaskStat::TASK_BLOCKING);
        task_list_.remove(p_task);
        task_blocking_list_[fd] = p_task;
    }

    void resumeTask(const Task_Ptr& p_task) override {
        p_task->setTaskStat(TaskStat::TASK_RUNNING);
        task_list_.push_back(p_task);
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
            Thread_Ptr th = std::make_shared<Thread>(dispatch);
            thread_group_.create_thread(boost::bind(&Thread::RunTask, th));
            thread_list_[dispatch] = th;
            BOOST_LOG_T(debug) << "Create Thread Index @: " << dispatch;
        }

        return thread_list_[dispatch]->createTask(func);
    }

    int createTimer(TaskFunc const& func, std::size_t dispatch,
                        std::size_t msec, bool forever = false) {
        if ( (dispatch + 1) > thread_list_.size() )
            thread_list_.resize(dispatch + 1);

        if (!thread_list_[dispatch])
        {
            Thread_Ptr th = std::make_shared<Thread>(dispatch);
            thread_group_.create_thread(boost::bind(&Thread::RunTask, th));
            thread_list_[dispatch] = th;
            BOOST_LOG_T(debug) << "Create Thread Index @: " << dispatch;
        }

        return thread_list_[dispatch]->createTimer(func, msec, forever);
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
        assert(ptr->getTaskStat() == TaskStat::TASK_RUNNING);

        current_task_ = ptr;
        ptr->swapIn();

        // 此处ptr被协程切换回来了
        current_task_ = null_task_;
        if (ptr->isFinished()) {
            // Task会被自动析构，后续考虑搞个对象缓存列表
            BOOST_LOG_T(debug) << "Job is finished with " << ptr->t_id_ ;
        }
        else
        {
            if (ptr->getTaskStat() == TaskStat::TASK_RUNNING) {
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

    /**
     * Runtask() 涉及到协程的调度和异步事件的响应，
     * 所以必须保证高效
     */
    std::size_t RunTask() override
    {
        BOOST_LOG_T(info) << "Main Thread RunTask() ...";
        std::size_t total = do_run_task(false);
        BOOST_LOG_T(info) << "Already run " << total << " serivces... " ;

        return total;
    }

    // RunUntilNoTask() 不会处理其他线程的事件，此处需要注意
    std::size_t RunUntilNoTask() override
    {
        BOOST_LOG_T(debug) << "Main Thread RunUntilNoTask() ...";
        if (!thread_list_.empty()) {
            BOOST_LOG_T(error) << "RunUntilNoTask will not handle thread events, do not use these!" << endl;
            ::abort();
        }

        std::size_t total = do_run_task(true);
        BOOST_LOG_T(info) << "Already run " << total << " serivces... " ;

        return total;
    }

    // scheduler主线程不会有竞争条件，所以不用考虑优化
    std::size_t traverseTaskEvents( int ms=50) override
    {
        std::vector<int> fd_coll;
        std::size_t ret = 0;

        if(traverseEvent(fd_coll, ms)) {

            assert(!fd_coll.empty());
            for (auto fd: fd_coll) {
                ++ ret;
                auto tk = task_blocking_list_[fd];
                task_blocking_list_.erase(fd);

                tk->setTaskStat(TaskStat::TASK_RUNNING);
                task_list_.push_back(task_blocking_list_[fd]);

                // 目前只以oneshot的方式使用(libco也是这么做的)
                delEvent(fd);
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

    void showStat(){
        BOOST_LOG_T(info) << "libto stat display: " ;
        BOOST_LOG_T(info) << "current coroutine uuid: " << Task::currentTaskUUID() ;
        std::cerr << "current coroutine uuid: " << Task::currentTaskUUID() << endl;
        BOOST_LOG_T(info) << boost::format("Main Thread -> TOTAL:%lu, BLOCKING:%lu")
            % task_list_.size() % task_blocking_list_.size();
        std::cerr << boost::format("Main Thread -> TOTAL:%lu, BLOCKING:%lu")
            % task_list_.size() % task_blocking_list_.size() << endl;
        for (auto &ithread: thread_list_) {
            if (! ithread){
                 BOOST_LOG_T(info) << " = Empty Thread";
                 std::cerr << " = Empty Thread" << endl;
                 continue;
            }

            BOOST_LOG_T(info) << boost::format(" = Worker Thread -> TOTAL:%lu, BLOCKING:%lu")
                % ithread->getTaskSize() % ithread->getBlockingSize();
            std::cerr << boost::format(" = Worker Thread -> TOTAL:%lu, BLOCKING:%lu")
                % ithread->getTaskSize() % ithread->getBlockingSize() << endl;;
        }
    }

private:
    size_t do_run_task(bool until_break) {

        std::size_t total = 0;
        std::size_t this_round = 0;
        bool real_do = false;

        for (;;) {
            real_do = false;

            if (until_break && task_list_.empty() && task_blocking_list_.empty())
               break;

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

    Scheduler(): Scheduler(0)
    {}

    explicit Scheduler(std::size_t max_event):
        TaskOperation(-1),
        Epoll(max_event)
    {
        current_task_ = nullptr;
        GetThreadInstance().thread_ = nullptr;
    }

    Task_Ptr                current_task_;
    boost::thread_group     thread_group_;
    std::vector<Thread_Ptr> thread_list_; //默认在main thread中执行的协程
};

}



#endif // _SCHEDULER_HPP_
