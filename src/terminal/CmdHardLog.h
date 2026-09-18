#pragma once
#include "TermCmdInterface.h"
#include "Loger.h"

namespace console {

class CmdHardFaultLog : public CmdInterface
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
    CmdHardFaultLog() : CmdInterface("faultlog", "Reads log of last Hard Fault")
    {}
};

extern CmdHardFaultLog cmdFaultLog;

}   // namespace console