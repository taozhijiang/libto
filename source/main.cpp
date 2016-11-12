#include "libto.hpp"

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


int main(int argc, char* argv[])
{
	coroutine c;
    c.bind_proc(echo, 1);

    coroutine c2;
    c2.bind_proc(echo2, 1);


    JoinAllThreads;

    return 0;
}
