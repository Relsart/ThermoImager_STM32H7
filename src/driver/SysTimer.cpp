#include "SysTimer.h"

namespace driver {


void sysDelay (uint32_t ms)
{
    uint64_t endTicks = msTicks + ms;
    while (msTicks < endTicks);
}

uint64_t getMsTicks ()
{
    return msTicks;
}

void setMsTicks (uint64_t ms)
{
    msTicks = ms;
}

}   // namespace driver