#pragma once
#include "TermCmdInterface.h"
#include "Loger.h"

#include "fatfs/ff.h"
#include "fatfs/diskio.h"

namespace console {

class CmdSD : public CmdInterface
{
private:
    FATFS fs;

    /**
     * @brief Result of file system operation output
     */
    void showResult(FRESULT resu);
public:

    /**
     * @brief Console command handler
     */
    void exec(uint32_t argc, char** arg) override;

    /**
     * @brief Constructor 
     */
    CmdSD() : CmdInterface("sd", "TestCommand")
    {}
};

extern CmdSD cmdSD;

}   // namespace console
