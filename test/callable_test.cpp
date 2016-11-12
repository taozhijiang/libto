#include <boost/test/unit_test.hpp>

#include "libto.hpp"

BOOST_AUTO_TEST_SUITE(callable_test)

void echo()
{
    BOOST_LOG_T(info) << "echo output" << endl;
}

void echo3(string msg)
{
    BOOST_LOG_T(info) << "std::bind echo output with: " << msg << endl;
}

struct callobject {
    void operator()() {
        BOOST_LOG_T(info) << "struct callable test" << endl;
    }
    void memfunc(string msg){
        BOOST_LOG_T(info) << "struct callable test2 with: " << msg << endl;
    }
};

void echo6(string msg)
{
    BOOST_LOG_T(info) << "std::function echo output with: " << msg << endl;
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

    coroutine c4, c5;
    c4.bind_proc(callobject());
    callobject obj;
    c5.bind_proc(std::bind(&callobject::memfunc, &obj, "tzj006"));

    c.bind_proc([] {
        std::cout << "reuse coroutine object!" << endl;
    });

    
    RunUntilNoTask;


    return;
}


BOOST_AUTO_TEST_SUITE_END()
