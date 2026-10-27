#include "CmdSD.h"
#include <cstring>

namespace console {

CmdSD cmdSD;

void CmdSD::showResult (FRESULT resu)
{
    switch (resu)
    {
    case FR_OK:
        Log () << Log::endl << "OK";
        break;
    case FR_DISK_ERR:
        Log () << Log::endl << "Low layer disk I/O error (SDMMC)";
        break;
    case FR_NO_FILESYSTEM:
        Log () << Log::endl << "No valid FAT volume";
        break;
    case FR_NOT_ENOUGH_CORE:
        Log () << Log::endl << "Working buffer could not be allocated. Check FF_USE_LFN in ffconf.c";
        break;
    default:
        Log () << Log::endl << "Other undefined error code";
        break;
    }
}


void CmdSD::exec (uint32_t argc, char** arg)
{
    if (strcmp (arg[0], "format") == 0) 
    {
        Log () << Log::endl << "Formatting...";
        Log::enable ();
        showResult (f_mkfs ("", 0, nullptr, FF_MAX_SS));
        return;
    }
    Log::enable ();
}

}   // namespace console
