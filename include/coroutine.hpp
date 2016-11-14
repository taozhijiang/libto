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
};

}



#endif // _COROUTINE_HPP_
