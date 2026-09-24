#ifndef FLASH_H
#define FLASH_H

#include "main.h"

//This should be changed for other MCU's. This one works for STM32072
#define PAGESIZEASBYTES 2048
//This should be changed for other MCU's. This one works for STM32072
#define BASEADDR (uint32_t)0x08000000
//This is app specific. Determines the reserved size for flash application
#define USERBASEADDR (uint32_t)0x0800F800

uint32_t Flash_readWordFromAddr(__IO uint32_t* _addr);

HAL_StatusTypeDef Flash_ErasePageWithNumber(uint8_t _page);

HAL_StatusTypeDef Flash_writeDataChunktoAddr(uint16_t* _data, uint32_t _addr, uint16_t _size);

HAL_StatusTypeDef Flash_readDataChunkFromAddr(int16_t*, uint32_t, uint16_t);

HAL_StatusTypeDef Flash_writeWordtoAddr(uint32_t _data, uint32_t _addr);

HAL_StatusTypeDef Flash_writeWordChunktoAddr(uint32_t* _data, uint32_t _addr, uint16_t _size);

HAL_StatusTypeDef Flash_readWordChunkFromAddr(uint32_t* _data, uint32_t  _addr, uint16_t _size);
#endif
