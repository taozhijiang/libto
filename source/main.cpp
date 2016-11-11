#include "general.hpp"

#include "scheduler.hpp"
#include "coroutine.hpp"

using coroutine = libto::Coroutine;
using scheduler = libto::Scheduler;

void echo()
{
    BOOST_LOG_T(info) << "TAOZJ" << endl;
}

int main(int argc, char* argv[])
{
	coroutine c;
    c.bind_proc(echo);

    scheduler::getInstance().RunTask();

    return 0;
}
