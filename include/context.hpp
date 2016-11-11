#ifndef _CONTEXT_HPP_
#define _CONTEXT_HPP_

#include "general.hpp"
#include <boost/context/all.hpp>

namespace ctx = boost::context;

namespace libto {

class Context {
public:
    Context(std::function<void()> func):
        yield_(nullptr),
        callin_(
            [=](boost::context::execution_context<void> yield) mutable -> boost::context::execution_context<void>
            {
                this->yield_ = &yield;
                func();
                return std::move(*this->yield_);
            })
    {}

    inline bool swapIn() {
        callin_ = callin_();
        return true;
    }

    inline bool swapOut() {
        assert(yield_);
        *yield_ = (*yield_)();
        return true;
    }

    ~Context() {}

private:
    //只有这个协程被调用的时候，yeild_才会有值
    ctx::execution_context<void>* yield_;
    ctx::execution_context<void>  callin_;
};

}



#endif // _CONTEXT_HPP_
