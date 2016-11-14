#include <boost/test/unit_test.hpp>

#include "libto.hpp"

BOOST_AUTO_TEST_SUITE(sch_yield_test)

#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

void echo()
{
    int lsocket = socket(AF_INET, SOCK_STREAM, 0);
    int flag = 1;
	setsockopt(lsocket, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));

    struct sockaddr_in svraddr;
	svraddr.sin_family = AF_INET;
	svraddr.sin_port = htons(7599);
	svraddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if(bind(lsocket, (struct sockaddr *)&svraddr, sizeof(struct sockaddr_in))) {
		std::cerr << "Socket Bind Error!" << endl;
        close(lsocket);
		return;
	}

    if(listen(lsocket, 20)){
		std::cerr << "Socket Listen Error!" << endl;
        close(lsocket);
		return;
	}

    libto::st_make_nonblock(lsocket);
    std::cerr << "Attempt to schedule out for read!" << endl;

    struct sockaddr in_addr;
    socklen_t in_len;
    int infd;
    char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];

    in_len = sizeof(struct sockaddr);
    int round = 0;

    while (round < 3)
    {
        sch_read(lsocket);
        infd = accept(lsocket, &in_addr, &in_len); //非阻塞的Socket
        if (infd == -1)
        {
            std::cerr << "Socket Accept Error!" << endl;
            return;
        }

        if( getnameinfo (&in_addr, in_len,
                           hbuf, sizeof(hbuf),
                           sbuf, sizeof(sbuf),
                           NI_NUMERICHOST | NI_NUMERICSERV) == 0)
        {
            std::cerr <<"Accept NEW Connect:" << hbuf << ", " << sbuf <<endl;
            std::string ret_str = "Hello from server for " + std::string(hbuf) + ":" + std::string(sbuf) + "\n";
            write(infd, ret_str.c_str(), ret_str.size());
        }
    }

    return;
}


BOOST_AUTO_TEST_CASE(test1)
{
	coroutine c;
    c.bind_proc(echo);

    RunTask;

    return;
}

BOOST_AUTO_TEST_SUITE_END()