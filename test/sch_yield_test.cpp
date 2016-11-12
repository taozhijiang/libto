#include <boost/test/unit_test.hpp>

#include "libto.hpp"

BOOST_AUTO_TEST_SUITE(sch_yield_test)

void echo()
{
    BOOST_LOG_T(info) << "echo output1" << endl;
    sch_yield;
    BOOST_LOG_T(info) << "echo output2" << endl;
    sch_yield;
    BOOST_LOG_T(info) << "echo output3" << endl;
    sch_yield;
    BOOST_LOG_T(info) << "echo output4" << endl;
}

void echo2()
{
    BOOST_LOG_T(info) << "echo2 output1" << endl;
    sch_yield;
    BOOST_LOG_T(info) << "echo2 output2" << endl;
    sch_yield;
    BOOST_LOG_T(info) << "echo2 output3" << endl;
    sch_yield;
    BOOST_LOG_T(info) << "echo2 output4" << endl;
    sch_yield;
    BOOST_LOG_T(info) << "echo2 output5" << endl;
    sch_yield;
    BOOST_LOG_T(info) << "echo2 output6" << endl;
    sch_yield;
    BOOST_LOG_T(info) << "echo2 output7" << endl;
}



BOOST_AUTO_TEST_CASE(test1)
{
	coroutine c;
    c.bind_proc(echo);

    coroutine c2;
    c2.bind_proc(echo2);


    RunUntilNoTask;


    return;
}


BOOST_AUTO_TEST_SUITE_END()
