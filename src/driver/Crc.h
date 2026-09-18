
#pragma once
#include <stdint.h>


/* Расчет CRC-8, 16, 32 на аппаратном модуле STM32 */

#define CRC_NORESET  false    

#define CRC_CONFIG_RESET    0x01    // Бит 0 - сброс перед предыдущим расчетом 
#define CRC_CONFIG_REVIN    0x02    // Бит 1 - побайтовый реверс на входе 
#define CRC_CONFIG_REVOUT   0x04    // Бит 2 - реверс на выходе 
#define CRC_CONFIG_EXOR     0x08    // Бит 3 - исключающее или на выходе

namespace driver
{

/**
 * @brief Clocking CRC by bus AHB4 enable 
 */
void CRCInit();

/**
* @brief Calculate CRC-8
* @param [in] data data
* @param [in] size size
* @param [in] init init value
* @param [in] poly polynom
* @param [in] config Бит 0 сброс, бит 1 реверс на входе, бит 2 реверс на выходе, бит 3 exor на выходе
* @return Checksum
*/
uint8_t getcrc8 (const uint8_t*data, uint32_t size, uint32_t init, uint32_t poly, uint8_t config);

/**
* @brief Расчёт контрольной суммы по алгоритму CRC-16/CCITT-FALSE
* @param [in] data Указатель на данные (uint16_t)
* @param [in] size Размер данных
* @param [in] init Значение инициализации
* @param [in] poly Полином
* @param [in] config Бит 0 сброс, бит 1 реверс на входе, бит 2 реверс на выходе, бит 3 exor на выходе
* @return uint16_t Контрольная сумма
*/
uint16_t getcrc16 (const uint8_t*data, uint32_t size, uint32_t init, uint32_t poly, uint8_t config);

/**s
* @brief Расчёт контрольной суммы по алгоритму CRC-32.
* @param [in] data Указатель на данные (uint32_t).
* @param [in] size Размер данных.
* @param [in] init Значение инициализации.
* @param [in] poly Полином.
* @param [in] config Бит 0 сброс, бит 1 реверс на входе, бит 2 реверс на выходе, бит 3 exor на выходе
* @return uint32_t Контрольная сумма
*/
uint32_t getcrc32 (const uint32_t*data, uint32_t size, uint32_t init, uint32_t poly, uint8_t config);

}   // namespace driver
