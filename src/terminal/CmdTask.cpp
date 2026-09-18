#include "CmdTask.h"
#include <cstring>
#include "task/TaskData.h"

namespace console {

CmdTask cmdTask;
using namespace task;

void CmdTask::exec (uint32_t argc, char** arg)
{
    Log() << Log::endl;
    for (int i = 0; i < Tasks::TasksEnd; i++)
    {
        if (tasksHandlers[i])
        {
            // For configUSE_TRACE_FACILITY option:
            #if (configUSE_TRACE_FACILITY == 1)
            TaskStatus_t xTaskDetails;
            vTaskGetInfo(tasksHandlers[i], &xTaskDetails, pdTRUE, eReady);
            Log() << Log::Base::Dec << "Task <" << xTaskDetails.pcTaskName << "> allocated mem size (bytes): " << StackSizes[i] * sizeof(StackType_t) << 
                    " remaining bytes in stack: " << xTaskDetails.usStackHighWaterMark * sizeof(StackType_t) << Log::endl;
            #else
            UBaseType_t remainingWords = uxTaskGetStackHighWaterMark(tasksHandlers[i]);
            BaseType_t remainingBytes = remainingWords * sizeof(StackType_t);
            Log() << Log::Base::Dec << "Task <" << pcTaskGetName(tasksHandlers[i]) << ">: remaining bytes in stack: "  << Log::Base::Dec << remainingBytes << Log::endl;
            #endif
        }
    }

    Log::enable ();
}

}   // namespace console
