#pragma once

#include <stdint.h>
#include "etl/string.h"
#include "etl/to_string.h"
#include "etl/to_arithmetic.h"

namespace console {

/**
 * @brief Функции, объявления, псевдонимы, конвертеры и прочее, использующееся в терминале.
 */

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
 * @brief Проверка, является ли значение в строке началом / концом массива.
 * @param [in] inpStr Входящая строка.
 * @return Результат: начало / конец массива / не массив / ошибка. 
 */
ArrayStatus isArray (char* inpStr);

/**
 * @brief Преобразование строки в числовое значение etl::variant<int, float>.
 * @param [in] inpStr Входящая строка.
 * @param [out] var Ссылка на переменную etl::variant
 * @return Результат чтения: true = корректно. 
 */
bool stringToVar (char* inpStr, termVariant& var);

/**
 * @brief Преобразование строки в числовое значение int.
 * @param [in] inpStr Входящая строка.
 * @param [out] var Ссылка на переменную int
 * @return Результат чтения: true = корректно. 
 */
bool stringToInt (char* inpStr, int32_t& var);

}   // namespace console