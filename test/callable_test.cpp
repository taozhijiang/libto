#include <boost/test/unit_test.hpp>

#include "general.hpp"

#include "scheduler.hpp"
#include "coroutine.hpp"

using coroutine = libto::Coroutine;
using scheduler = libto::Scheduler;

BOOST_AUTO_TEST_SUITE(callable_test)

void echo()
{
    BOOST_LOG_T(info) << "echo output" << endl;
}

void echo3(const string msg)
{
    BOOST_LOG_T(info) << "std::bind echo output with: " << msg << endl;
}

BOOST_AUTO_TEST_CASE(test1)
{
	coroutine c;
    c.bind_proc(echo);

    coroutine c2;
    c2.bind_proc([]{
        std::cout << "Lambda Echo!" << std::endl;
    });

    coroutine c3;
    c3.bind_proc(std::bind(echo3, "tzj"));

    scheduler::getInstance().RunTask();

    return;
}


BOOST_AUTO_TEST_SUITE_END()
