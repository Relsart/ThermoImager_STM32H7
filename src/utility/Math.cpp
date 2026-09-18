#include "Math.h"

namespace math {

uint32_t roundToUint(float input)
{
    if (input < 0)
        return 0;   // Negative!

    uint32_t intpart = static_cast<uint32_t>(input);
    return (input - static_cast<float>(intpart) < 0.5) ? intpart : intpart + 1;
}

}   // namespace math