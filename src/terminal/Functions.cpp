#include "Functions.h"

namespace console {

const uint32_t notFoundInString = 0xFFFFFFFF;   // "Not found" result for etl::string.find()
const uint32_t lineSize = 100;

ArrayStatus isArray(char* inpStr)
{
    if (!inpStr)
    {
        return ArrayStatus::Error;
    }

    etl::string<lineSize> buifstr = inpStr;

    // Spaces removing:
    while (buifstr.find(" ") != notFoundInString)
    {
        buifstr.erase (buifstr.find(" "), 1);
    }

    if (buifstr.find("[") != notFoundInString)
        return ArrayStatus::BeginArray;
    else if (buifstr.find("]") != notFoundInString)
        return ArrayStatus::EndArray;
    else
        return ArrayStatus::NotArray;
}

void prepareStringVar (etl::istring *buifstr)
{
    while (buifstr->find(" ") != notFoundInString)
    {
        buifstr->erase(buifstr->find(" "), 1);
    }
    if (buifstr->find("[") != notFoundInString)
    {
        buifstr->erase(buifstr->find("["), 1);
    }
    if (buifstr->find("]") != notFoundInString)
    {
        buifstr->erase(buifstr->find("]"), 1);
    }
    if (buifstr->find(",") != notFoundInString)
    {
        buifstr->erase(buifstr->find(","), 1);
    }
}

uint8_t getNumBase (etl::istring *buifstr)
{
    int hexPrefixPos1 = buifstr->find("0x");
    int hexPrefixPos2 = buifstr->find("0X");
    if (hexPrefixPos1 >= 0)   
    {
        buifstr->erase(hexPrefixPos1, 2);
        return 16;
    }
    else if (hexPrefixPos2 >= 0)   
    {
        buifstr->erase(hexPrefixPos2, 2);
        return 16;
    }
    return 10;
}

bool stringToVar(char* inpStr, termVariant& var)
{
    if (!inpStr)
        return false;
    
    etl::string<lineSize> buifstr = inpStr;
    bool floatType = false;
    
    // 1. Removing spaces, commas, and array brackets:
    prepareStringVar(&buifstr);

    // 2. Defining int/float by point:
    if (buifstr.find(".") != notFoundInString)
    {
        floatType = true;
    }

    // 3. Defining base (hex/dec):
    uint8_t base = getNumBase(&buifstr);

    // 4. Convertation:
    if (floatType)
    {
        etl::to_arithmetic_result<float> float_result = etl::to_arithmetic<float>(buifstr);
        if (float_result.error() == etl::to_arithmetic_status::Valid)
        {
            var.emplace<float>(float_result.value());
            return true;
        }
        else
        {
            return false;
        }
    }
    else
    {
        etl::to_arithmetic_result<int> int_result = etl::to_arithmetic<int>(buifstr, base);
        if (int_result.error() == etl::to_arithmetic_status::Valid)
        {
            var.emplace<int>(int_result.value());
            return true;
        }
        else
        {
            return false;
        }
    }
    return false;    
}

bool stringToInt(char* inpStr, int32_t& var)
{
    if (!inpStr)
        return false;

    etl::string<lineSize> buifstr = inpStr;
    
    // 1. Removing extra chars: 
    prepareStringVar(&buifstr);

    // 2. Определение hex/dec:
    uint8_t base = getNumBase(&buifstr);

    // 3. Конвертация:
    etl::to_arithmetic_result<int32_t> int_result = etl::to_arithmetic<int32_t> (buifstr, base);
    if (int_result.error() == etl::to_arithmetic_status::Valid)
    {
        var = int_result.value();
        return true;
    }

    return false;
}

}   // namespace console
