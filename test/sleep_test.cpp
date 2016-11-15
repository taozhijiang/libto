#include <boost/test/unit_test.hpp>

#include "libto.hpp"


BOOST_AUTO_TEST_SUITE(sch_sleep_test)

void echo() {
    std::cerr << "sleep 500ms " << endl;
    sch_sleep_ms(500);
    std::cerr << "sleep 1000ms " << endl;
    sch_sleep_ms(1000);
    std::cerr << "sleep 1500ms " << endl;
    sch_sleep_ms(1500);
    std::cerr << "sleep 3000ms " << endl;
    sch_sleep_ms(3000);
    std::cerr << "sleep 5000ms " << endl;
    sch_sleep_ms(5000);
    std::cerr << "Done!" << endl;
}


BOOST_AUTO_TEST_CASE(test1)
{
	coroutine c1;
    c1.bind_proc(echo);

    coroutine c2;
    c2.bind_proc(echo, 2);

    RunTask;

    return;
}

BOOST_AUTO_TEST_SUITE_END()
