#pragma once

#include "CmdTask.h"
#include "CmdHardLog.h"

namespace console {

/**
 * @brief Links for all terminal commands:
 */
static constexpr CmdInterface* listCmd[]=
{
    #ifdef WITH_RTOS
    &cmdTask,
    #endif
    &cmdFaultLog,
};

}    // namespace console
