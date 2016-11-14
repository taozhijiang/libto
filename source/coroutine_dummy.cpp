#include "general.hpp"

#include "coroutine.hpp"
#include "task.hpp"

namespace libto {

    uint64_t Task::task_uuid = 0;
    Task_Ptr TaskOperation::null_task_ = nullptr; 

    int st_make_nonblock(int socket)
    {
        int flags = 0;

        flags = fcntl (socket, F_GETFL, 0);
    	flags |= O_NONBLOCK;
        fcntl (socket, F_SETFL, flags);

        return 0;
    }
}

