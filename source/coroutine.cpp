#include "general.hpp"

#include "coroutine.hpp"
#include "task.hpp"
#include "thread.hpp"

#include <dlfcn.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>

namespace libto {

    uint64_t Task::task_uuid = 0;
    Task_Ptr TaskOperation::null_task_ = nullptr;

    void _sch_yield();
    int _timer_prep(std::size_t msec, bool forever);
    bool _sch_sleep_ms(std::size_t msec);
    void _sch_read(int fd);
    void _sch_write(int fd);
    void _sch_rdwr(int fd);


    ThreadLocalInfo& GetThreadInstance()
    {
        static __thread ThreadLocalInfo *info = nullptr;

        if (!info)
            info = new ThreadLocalInfo();

        return *info;
    }

    bool IsMainThread() {
        return  ! GetThreadInstance().thread_;
    }

    int GetThreadIdx() {
        return GetThreadInstance().thread_->thread_idx_;
    }

    TaskOperation* GetCurrentTaskOperation() {
        Thread *p_thread = GetThreadInstance().thread_;

        if (!p_thread)
            return &Scheduler::getInstance();
        else
            return p_thread;
    }

    Epoll* GetCurrentEpoll() {
        Thread *p_thread = GetThreadInstance().thread_;

        if (!p_thread)
            return &Scheduler::getInstance();
        else
            return p_thread;
    }


    void _sch_yield() {
        Task_Ptr curr_ = GetCurrentTaskOperation()->getCurrentTask();
        if(curr_)
            curr_->swapOut();
    }

    void _sch_yield_stat(TaskStat stat) {
        Task_Ptr curr_ = GetCurrentTaskOperation()->getCurrentTask();
        if(curr_) {
            curr_->setTaskStat(stat);
            curr_->swapOut();
        }
    }

    // 依据定时器的原理实现
    bool _sch_sleep_ms(std::size_t msec) {
        int timerfd;
        // char null_read[8];

        // 主线程的主协程一定不能够被阻塞出去，否则永远无法切换回来
        // 虽然用户无法在主协程中添加东西，但是开发者要注意
        assert (! (IsMainThread() && !GetCurrentTaskOperation()->isInCoroutine()) );

        if ( msec == 0 || (timerfd = _timer_prep(msec, false)) == -1)
            return false;

        _sch_read(timerfd);
        // read(timerfd, null_read, 8);

        GetCurrentEpoll()->delEvent(timerfd);
        close(timerfd);

        return true;
    }

    // return timerfd, -1 for error
    int _timer_prep(std::size_t msec, bool forever) {
        int timerfd = -1;
        struct timespec now;
        struct itimerspec itv;

        if (msec == 0)
            return -1;

        if( (timerfd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK)) == -1)
            return -1;

        clock_gettime(CLOCK_MONOTONIC, &now);
        memset(&itv, 0, sizeof(struct itimerspec));
        itv.it_value.tv_sec = now.tv_sec + msec/1000;
        itv.it_value.tv_nsec = now.tv_nsec + (msec%1000)*1000*1000;

        if (forever) {
            itv.it_interval.tv_sec = msec / 1000;
            itv.it_interval.tv_nsec = (msec%1000)*1000*1000;
        }

        if( timerfd_settime(timerfd, TFD_TIMER_ABSTIME, &itv, NULL) == -1){
            close(timerfd);
            return -1;
        }

        return timerfd;
    }

    void _sch_read(int fd) {

        if ( IsMainThread() || (!GetCurrentTaskOperation()->isInCoroutine()) )
            return;

        Task_Ptr curr_ = GetCurrentTaskOperation()->getCurrentTask();
        GetCurrentEpoll()->addEvent(fd,  EPOLLIN | EPOLLERR);
        GetCurrentTaskOperation()->blockTask(fd, curr_);
        curr_->swapOut();
    }

    void _sch_write(int fd) {

        if ( IsMainThread() || (!GetCurrentTaskOperation()->isInCoroutine()) )
            return;

        Task_Ptr curr_ = GetCurrentTaskOperation()->getCurrentTask();
        GetCurrentEpoll()->addEvent(fd,  EPOLLOUT | EPOLLERR);
        GetCurrentTaskOperation()->blockTask(fd, curr_);
        curr_->swapOut();
    }

    void _sch_rdwr(int fd) {

        if ( IsMainThread() || (!GetCurrentTaskOperation()->isInCoroutine()) )
            return;

        Task_Ptr curr_ = GetCurrentTaskOperation()->getCurrentTask();
        GetCurrentEpoll()->addEvent(fd,  EPOLLIN | EPOLLOUT | EPOLLERR);
        GetCurrentTaskOperation()->blockTask(fd, curr_);
        curr_->swapOut();
    }

}

// global hook namespace

typedef int (*accept_pfn_t)(int sockfd, struct sockaddr *addr, socklen_t *addrlen);

typedef ssize_t (*read_pfn_t)(int fildes, void *buf, size_t nbyte);
typedef ssize_t (*write_pfn_t)(int fildes, const void *buf, size_t nbyte);

typedef ssize_t (*send_pfn_t)(int socket, const void *buffer, size_t length, int flags);
typedef ssize_t (*recv_pfn_t)(int socket, void *buffer, size_t length, int flags);

typedef ssize_t (*sendto_pfn_t)(int socket, const void *message, size_t length,
	                 int flags, const struct sockaddr *dest_addr, socklen_t dest_len);
typedef ssize_t (*recvfrom_pfn_t)(int socket, void *buffer, size_t length,
	                 int flags, struct sockaddr *address, socklen_t *address_len);



static accept_pfn_t g_sys_accept_func 	= (accept_pfn_t)dlsym(RTLD_NEXT,"accept");

static read_pfn_t g_sys_read_func 		= (read_pfn_t)dlsym(RTLD_NEXT,"read");
static write_pfn_t g_sys_write_func 	= (write_pfn_t)dlsym(RTLD_NEXT,"write");

static sendto_pfn_t g_sys_sendto_func 	= (sendto_pfn_t)dlsym(RTLD_NEXT,"sendto");
static recvfrom_pfn_t g_sys_recvfrom_func = (recvfrom_pfn_t)dlsym(RTLD_NEXT,"recvfrom");

// TCP
static send_pfn_t g_sys_send_func 		= (send_pfn_t)dlsym(RTLD_NEXT,"send");
static recv_pfn_t g_sys_recv_func 		= (recv_pfn_t)dlsym(RTLD_NEXT,"recv");


ssize_t read ( int fd, void *buf, size_t nbyte ) {

    libto::_sch_read(fd);
    ssize_t ret = g_sys_read_func( fd, (char*)buf, nbyte );

    return ret;
}

ssize_t write ( int fd, const void *buf, size_t nbyte ) {
    ssize_t ret = 0;
    ssize_t wroten = 0;

    libto::_sch_write(fd);

    do {
        ret = g_sys_write_func( fd, (char*)buf, nbyte );
        wroten += ret;
    } while( (ret > 0) && ((size_t)wroten < nbyte) );

    return wroten;
}


#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>

int accept (int sockfd, struct sockaddr *addr, socklen_t *addrlen) {

    libto::_sch_read(sockfd);
    return g_sys_accept_func( sockfd, addr, addrlen );
}

ssize_t send (int socket, const void *buffer, size_t length, int flags){
    ssize_t ret = 0;
    ssize_t wroten = 0;

    libto::_sch_write(socket);

    do {
        ret = g_sys_send_func( socket, (char*)buffer, length, flags );
        wroten += ret;
    } while( (ret > 0) && ((size_t)wroten < length) );

    return wroten;
}

ssize_t recv (int socket, void *buffer, size_t length, int flags){

    libto::_sch_read(socket);
    ssize_t ret = g_sys_recv_func( socket, (char*)buffer, length, flags );

    return ret;
}


ssize_t sendto (int socket, const void *message, size_t length,
	                 int flags, const struct sockaddr *dest_addr, socklen_t dest_len){

    libto::_sch_write(socket);
    ssize_t ret = g_sys_sendto_func( socket, message, length, flags, dest_addr, dest_len );

    return ret;
}

ssize_t recvfrom (int socket, void *buffer, size_t length,
	                 int flags, struct sockaddr *address, socklen_t *address_len){

    libto::_sch_read(socket);
	ssize_t ret = g_sys_recvfrom_func( socket, buffer, length, flags, address, address_len );

	return ret;
}
