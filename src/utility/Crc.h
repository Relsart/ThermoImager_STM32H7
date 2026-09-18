#pragma once
#include <stdint.h>
#include "driver/Crc.h"

/**
 * @brief Расчёт crc по алгоритму CRC-16-CCITT с использованием аппаратного узла расчета CRC STM32
 * @param [in] pcBlock Указатель на данные (1 байт)
 * @param [in] len Размер данных
 * @param [in] reset Флаг сброса аппаратного модуля расчета CRC. Сбросить в 0 если нужно продолжать расчет.
 * @return uint16_t Контрольная сумма
 */
uint16_t crc16Ccitt (const uint8_t* pcBlock, uint16_t len, bool reset = true);

/**
 * @brief Расчёт crc по алгоритму CRC-8 с использованием аппаратного узла расчета CRC STM32
 * @param [in] pcBlock Указатель на данные (1 байт)
 * @param [in] len Размер данных
 * @param [in] reset Флаг сброса аппаратного модуля расчета CRC. Сбросить в 0 если нужно продолжать расчет.
 * @return uint8_t Контрольная сумма
 */
uint8_t crc8 (const uint8_t* pcBlock, uint16_t len, bool reset = true);
