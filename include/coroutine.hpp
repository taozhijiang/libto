#ifndef _COROUTINE_HPP_
#define _COROUTINE_HPP_

#include "general.hpp"
#include "scheduler.hpp"


namespace libto {

class Coroutine {

public:
    Coroutine() = default;

    template <typename Arg>
    void bind_proc(Arg const& arg)
    {
        Scheduler::getInstance().createTask(arg);
    }

    template <typename Arg>
    void bind_proc(Arg const& arg, std::size_t dispatch)
    {
        Scheduler::getInstance().createTask(arg, dispatch);
    }

    template <typename Arg>
    void bind_timer(Arg const& arg, std::size_t msec, bool forever = false)
    {
        Scheduler::getInstance().createTimer(arg, msec, forever);
    }

    template <typename Arg>
    void bind_timer(Arg const& arg, std::size_t dispatch,
                        std::size_t msec, bool forever = false)
    {
        Scheduler::getInstance().createTimer(arg, dispatch, msec, forever);
    }
};

}



#endif // _COROUTINE_HPP_
