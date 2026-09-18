#pragma once

#include "CmdTask.h"
#include "CmdHardLog.h"

namespace console {

static constexpr CmdInterface* listCmd[]=
{
    #ifdef WITH_RTOS
    &cmdTask,
    #endif
    &cmdFaultLog
};

}    // namespace console
