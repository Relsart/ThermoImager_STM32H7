#pragma once

#include <stdint.h>
#include "etl/string.h"
#include "etl/to_string.h"
#include "etl/to_arithmetic.h"

namespace console {

typedef etl::variant <int, float> termVariant;

enum TermVarIndex
{
    IntIndex = 0,
    FloatIndex = 1
};

enum class ArrayStatus
{
    NotArray,
    BeginArray, 
    EndArray, 
    Error
};

/**
 * @brief Check if a value in a string is the start/end of an array
 * @param [in] inpStr Input string
 * @return Result: Start of array / End of array / Not an array / Error 
 */
ArrayStatus isArray(char* inpStr);

/**
 * @brief Converting a string to a numeric value etl::variant<int, float>.
 * @param [in] inpStr Input string
 * @param [out] var Link to the etl::variant variable
 * @return Result: true = Ok 
 */
bool stringToVar(char* inpStr, termVariant& var);

/**
 * @brief Converting a string to a numeric value int.
 * @param [in] inpStr Input string
 * @param [out] var Link to the int variable
 * @return Result: true = Ok 
 */
bool stringToInt(char* inpStr, int32_t& var);

}   // namespace console
