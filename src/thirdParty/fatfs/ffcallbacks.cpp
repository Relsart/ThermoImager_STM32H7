
#include "platform/Platform.h"
#include "fatfs/ff.h"
#include "diskio.h"

#include "driver/Dwt.h"
#include "driver/SysTimer.h"

/**
 * @brief DEBUG_SDMMC опция для мониторинга передаваемых данных на карту SD. Для диагностики "подглючивающих" карт.
 * @todo Убрать директиву когда будет консольная команда для управления категориями логов.
 */
#define DEBUG_SDMMC

#ifdef DEBUG_SDMMC
uint32_t totalReadBytes = 0;
uint32_t totalWriteBytes = 0;
#endif

static const uint32_t TimeoutMs = 5000;

extern "C" DRESULT MMC_disk_read (BYTE* buff, LBA_t sector, UINT count)
{
    //driver::sdmmc::Result result = platform::board.sdCard.readBlocks(buff, sector, count, 100);
    //Log(lmSystem, result == sdmmc::Result::Ok ? Info : Error) << "SD card reading result: " << result;

    //driver::sdmmc::Result result = platform::board.sdCard.readViaIDMA (buff, sector, count);

    //auto param = driver::DwtTimer::getInstance().timeoutInit(TimeoutMs);
    /*do
    {   if (((driver::getMsTicks () - startTime) > 5000) || (result != driver::sdmmc::Result::Ok))
        {
            Log (lmFatFs, LogLevel::Error) << "Disk reading error!";
            platform::board.sdCard.resetIdma ();
            return RES_ERROR;
        }

    } while (platform::board.sdCard.getIdmaState () != driver::SdCard::IdmaState::FinishOk);
    platform::board.sdCard.resetIdma ();*/

    #ifdef DEBUG_SDMMC
    totalReadBytes += static_cast<uint32_t>(count * 512);
    Log (lmFatFs, LogLevel::Info) << "Reading " << totalReadBytes << " bytes";
    #endif
    return RES_OK;
}


extern "C" DRESULT MMC_disk_write (const BYTE* buff, LBA_t sector, UINT count)
{
    //driver::sdmmc::Result result = platform::board.sdCard.writeBlocks(buff, sector, count, 100);
    //Log(lmSystem, result == sdmmc::Result::Ok ? Info : Error) << "SD card writing result: " << result;

   /* driver::sdmmc::Result result = platform::board.sdCard.writeViaIDMA (buff, sector, count);
    uint64_t startTime = driver::getMsTicks ();
    do
    {   if (((driver::getMsTicks () - startTime) > 5000) || (result != driver::sdmmc::Result::Ok))
        {
            Log (lmFatFs, LogLevel::Error) << "Disk writing error!";
            platform::board.sdCard.resetIdma ();
            return RES_ERROR;
        }

    } while (platform::board.sdCard.getIdmaState () != driver::SdCard::IdmaState::FinishOk);
    platform::board.sdCard.resetIdma ();*/
    #ifdef DEBUG_SDMMC
    totalWriteBytes += static_cast<uint32_t>(count * 512);
    Log (lmFatFs, LogLevel::Info) << "Writing " << totalWriteBytes << " bytes";
    #endif
    return RES_OK;
}

extern "C" DRESULT MMC_disk_ioctl (BYTE cmd, void* buff)
{
    switch (cmd)
    {
    case CTRL_SYNC:
        return RES_OK;
    
    case GET_SECTOR_COUNT:
    {
        // TODO: check for card initialisation here!
        //uint32_t bl = platform::board.sdCard.getBlocksNumber();
       // *(reinterpret_cast<uint32_t*>(buff)) = bl;

        return RES_OK;
    }
    case GET_SECTOR_SIZE:
    case GET_BLOCK_SIZE:
    {
        // TODO: check for card initialisation here!
        //uint32_t blen = platform::board.sdCard.getBlockLen();
        //*(reinterpret_cast<uint32_t*>(buff)) = blen;
        return RES_OK;
    }
    }
    return RES_ERROR;
}
