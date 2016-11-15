#include "libto.hpp"

#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string>

static std::size_t thread_idx = 0;

void respon_func(int sock){
    char buf[512];
    int count;
    std::string msg_200("HTTP/1.1 200 OK\r\nServer: nginx/1.8.0\r\nDate: Tue, 15 Nov 2016 03:10:22 GMT\r\nContent-Length: 2\r\nConnection: keep-alive\r\n\r\nOK");

    libto::st_make_nonblock(sock);
	sch_read(sock);

	while (true) {
        count = recv (sock, buf, 512, 0);

        if (count < 0) {
            if ( count == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                /*Do your things*/
                sch_write(sock);
                write(sock, msg_200.c_str(), msg_200.size());
                break;
            }
            else {
                BOOST_LOG_T(error) << "RECV ERROR for socket:" << sock << endl;
                break;
            }
        }
        else if (count == 0)    //stream socket peer has performed an orderly shutdown
        {
			break;
        }
    }

    close(sock);
    return;
}

void server()
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

    struct sockaddr in_addr;
    socklen_t in_len = sizeof(struct sockaddr);
    int accept_fd;
    coroutine cs;

    while (true) {
        sch_read(lsocket);
        accept_fd = accept(lsocket, &in_addr, &in_len); //非阻塞的Socket
        if (accept_fd == -1) {
            std::cerr << "Socket Accept Error!" << endl;
            continue;
        }

        cs.bind_proc(std::bind(respon_func, accept_fd), 1 + (++thread_idx %10) );
    }

    return;
}


int main(int argc, char* argv[])
{
	coroutine c;
    c.bind_proc(server, 0);

    RunTask;
    JoinAllThreads;

    return 0;
}

