#include "libto.hpp"

#include <sys/types.h>          /* See NOTES */
#include <string>

#include <boost/context/stack_traits.hpp>

static std::size_t thread_idx = 0;
using boost::context::stack_traits;

void respon_func(int sock){
    char buf[512];
    int count;
    std::string msg_200("HTTP/1.1 200 OK\r\nServer: nginx/1.8.0\r\nDate: Tue, 15 Nov 2016 03:10:22 GMT\r\nContent-Length: 2\r\nConnection: keep-alive\r\n\r\nOK");

    libto::st_make_nonblock(sock);

    #if 0  /* non-hook edition */

    sch_read(sock);
	while (true) {
        count = read (sock, buf, 512);
        if (count < 0) {
            if ( count == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                /*Do your things*/
                sch_write(sock);
                write(sock, msg_200.c_str(), msg_200.size());
                break;
            }
            else {
                BOOST_LOG_T(error) << "RECV ERROR for socket:" << sock ;
                break;
            }
        }
        else if (count == 0)    //stream socket peer has performed an orderly shutdown
        {
			break;
        }
    }
    #else

    count = read (sock, buf, 512);
    if (count < 0) { //errno == EAGAIN || errno == EWOULDBLOCK
        std::cerr << "Hook Read Error!" << std::endl;
    }
    else { // OK
        write(sock, msg_200.c_str(), msg_200.size());
    }
    #endif

    close(sock);

    // while (true) {
    //    sch_yield_stat(taskStat::TASK_STOPPED);
    // }

    return;
}

void server()
{
    int lsocket = socket(AF_INET, SOCK_STREAM, 0);
    int flag = 1;
	setsockopt(lsocket, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));

    struct sockaddr_in srvaddr;
	srvaddr.sin_family = AF_INET;
	srvaddr.sin_port = htons(7999);
	srvaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if(bind(lsocket, (struct sockaddr *)&srvaddr, sizeof(struct sockaddr_in))) {
		std::cerr << "Socket Bind Error!" ;
        close(lsocket);
		return;
	}

    if(listen(lsocket, 200)){
		std::cerr << "Socket Listen Error!" ;
        close(lsocket);
		return;
	}

    std::cerr << "Server Listen at: 0.0.0.0: " << ntohs(srvaddr.sin_port) << std::endl;

    libto::st_make_nonblock(lsocket);

    struct sockaddr in_addr;
    socklen_t in_len = sizeof(struct sockaddr);
    int accept_fd;
    coroutine cs;

    while (true) {
        accept_fd = accept(lsocket, &in_addr, &in_len); //非阻塞的Socket
        if (accept_fd == -1) {
            std::cerr << "Socket Accept Error!" ;
            continue;
        }

        cs.bind_proc(std::bind(respon_func, accept_fd), 1 + (++thread_idx %10) );
    }

    return;
}

namespace libto {

extern bool libto_init();

}

void stack_info(){
    std::cout << "!!! stack_traits info !!!" << std::endl;

    std::cout << "is_unbounded: " << stack_traits::is_unbounded() << std::endl;
    std::cout << "page_size: " << stack_traits::page_size() << std::endl;
    std::cout << "default_size: " << stack_traits::default_size() << std::endl;
    std::cout << "minimum_size: " << stack_traits::minimum_size() << std::endl;
    if (!stack_traits::is_unbounded())
        std::cout << "maximum_size: " << stack_traits::maximum_size() << std::endl;

    return;
}

int main(int argc, char* argv[])
{
    libto::libto_init();
    stack_info();

	coroutine c;
    c.bind_proc(server, 0);

    RunTask;
    JoinAllThreads;

    return 0;
}

