#include <boost/test/unit_test.hpp>

#include "libto.hpp"

#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>


BOOST_AUTO_TEST_SUITE(sch_timer_test)

void echo1() {
    std::cerr << "This is the timer 5s echo ..." << endl;
}

void echo2() {
    std::cerr << "This is the timer 1s echo ..." << endl;
}

void echo3() {
    std::cerr << "This is the timer one shot echo ..." << endl;
}

BOOST_AUTO_TEST_CASE(test1)
{
	coroutine c1;
    c1.bind_timer(echo1, 5000, true);

    coroutine c2;
    c2.bind_timer(echo2, 1000, true);

    coroutine c3