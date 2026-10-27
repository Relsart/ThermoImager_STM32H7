#pragma once
#include "TermCmdInterface.h"
#include "Loger.h"

namespace console {

/**
 * @brief Command for RTOS tasks diagnostic information (memory limits)
 *        Command name: task
 */
class CmdTask : public CmdInterface
{
private:
public:
    /**
     * @brief Console command handler
     */
    void exec(uint32_t argc, char** arg) override;

    /**
     * @brief Constructor 
     */
    CmdTask() : CmdInterface("task", "RTOS tasks diagnostic")
    {}
};

extern CmdTask cmdTask;

}   // namespace console
