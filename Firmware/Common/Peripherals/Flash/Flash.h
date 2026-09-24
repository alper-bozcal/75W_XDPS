#ifndef FLASH_H
#define FLASH_H

#include "stdint.h"
#include "defaults.h"

void Flash_readWords(volatile uint32_t* arr, uint32_t startAddr, uint32_t size);

uint8_t Flash_writeWords(volatile uint32_t* arr, uint32_t startAddr, uint32_t size);

/**
 * @brief read size many bytes from startAddr of flash in to arr
 */
void Flash_readBytes(volatile uint8_t* arr, uint32_t startAddr, uint32_t size);

/**
 * @brief erase specified page of flash using hal functions
 */
uint8_t Flash_erasePage(uint16_t pageNum);

/**
 * @brief Write size of bytes to the startAddr of Flash from arr
 */
uint8_t Flash_writeBytes(volatile uint8_t* arr, uint32_t startAddr, uint32_t size);

/**
 * @brief Return single 16 bit parameter from Flash using index
 */
uint16_t Flash_readParameter(uint32_t startAddr, uint16_t index);

/**
 * @brief Write singe 16 bit parameter to Flash using index
 * @note Because how Flash works, we need to delete all of params and rewrite them
 */
uint16_t Flash_writeParameter(uint32_t startAddr, uint16_t index, uint16_t val, uint16_t total);

/**
 * @brief Write TAG array to user params page of Flash
 */
uint16_t Flash_writeTAG(uint8_t *arr, uint16_t size, uint16_t total);

/**
 * @brief Write all parameter to Flash
 * @note Because how Flash works, we need to delete all of params and rewrite them
 */
uint16_t Flash_writeAllParameter(uint32_t startAddr, int16_t *val, uint16_t total);
#endif
