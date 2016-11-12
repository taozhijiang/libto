#include "general.hpp"

#include "coroutine.hpp"
#include "task.hpp"

namespace libto {

    uint64_t Task::task_uuid = 0;

    Task_Ptr TaskOperation::null_task_;

}

