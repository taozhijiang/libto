#ifndef _LIBTO_HPP_
#define _LIBTO_HPP_

#include "general.hpp"

#include "scheduler.hpp"
#include "coroutine.hpp"
#include "task.hpp"

using coroutine = libto::Coroutine;
using scheduler = libto::Scheduler;

#define sch_yield do{ libto::Task_Ptr curr_ = libto::Scheduler::getInstance().GetCurrentTask(); \
                               if(curr_) curr_->swapOut(); \
                   } while(0);

#define RunTask          do{ libto::Scheduler::getInstance().RunTask(); } while(0);
#define RunUntilNoTask   do{ libto::Scheduler::getInstance().RunUntilNoTask(); } while(0);
#define JoinAllThreads   do{ libto::Scheduler::getInstance().joinAllThreads(); } while(0);

#endif // _LIBTO_HPP_
