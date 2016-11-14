#ifndef _EPOLL_HPP_
#define _EPOLL_HPP_

#include <sys/epoll.h>

namespace libto {

class Epoll {
public:
    explicit Epoll(std::size_t max_events):
    max_events_(max_events),
    event_fd_(-1),
    events_(NULL)
    {
        // 不支持异步操作
        if(max_events_ == 0)
            return;

        events_ = (struct epoll_event*)calloc(max_events_, sizeof(struct epoll_event));
        assert(events_);
        memset(events_, 0, max_events_ * sizeof(struct epoll_event));
        event_fd_ = epoll_create(max_events_);
        assert(event_fd_ != -1);
    }

    bool addEvent(int fd, int events){
        return ctlEvent(fd, EPOLL_CTL_ADD, events);
    }

    bool delEvent(int fd) {
        return ctlEvent(fd, EPOLL_CTL_DEL, 0);
    }

    // EPOLL_CTL_ADD, EPOLL_CTL_MOD, EPOLL_CTL_DEL
    // EPOLLIN|EPOLLOUT|EPOLLERR
    bool ctlEvent(int fd, int op, int events) {
        struct epoll_event event;
        int ret = 0;

        st_make_nonblock(fd);
        event.data.fd = fd;
        event.events = events | EPOLLET; //edge tridge
        ret = epoll_ctl (event_fd_, op, fd, &event);

        return ret == -1 ? false : true;
    }

    // -1 block for ever
    bool traverseEvent(std::vector<int>& fds, int ms){
        int ret = 0;

        ret = epoll_wait(event_fd_, events_, max_events_, ms);
        if (ret == -1)
            return false;

        fds.clear();
        if (ret == 0)
            return true;

        for (int i=0; i<ret; ++i)
            fds.push_back(events_[i].data.fd);

        return true;
    }

    ~Epoll(){
        if (event_fd_ != -1)
            close(event_fd_);

        free(events_);
    }

private:
    std::size_t max_events_;
    int event_fd_;
    struct epoll_event* events_;

};

}


#endif //_EPOLL_HPP_
