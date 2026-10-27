#pragma once
#include <stdint.h>

/**
 * @brief Groups of Logs Messages
 * @note ACHTUNG! lmEnd must be matched with moduleNames size!
 */
enum LogGroup 
{
    lmSystem = 0,
    lmTask,
    lmExtLibs,
    lmSDMMC,
    lmFatFs,
    lmThermoArray,
    lmParser,
    lmEnd
};

/**
 * @brief Module names for messages titles
 */
static constexpr const char* moduleNames [lmEnd] =
{
    "System: ",
    "Task: ",
    "Library: ",
    "SDMMC: ", 
    "FAT file system: ", 
    "Thermo sensor: ",
    "Data parsing: "
};
