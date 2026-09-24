#include "Flash.h"

void Flash_readBytes(volatile uint8_t* arr, uint32_t startAddr, uint32_t size){
  for(uint32_t i = 0; i < size; i += 2){
    arr[i] = *(uint8_t*)( startAddr + i );
    arr[i + 1] = *(uint8_t*)( startAddr + i + 1);
  }
}

void Flash_readWords(volatile uint32_t* arr, uint32_t startAddr, uint32_t size){

  for(uint32_t i = 0; i < size; ++i){
    arr[i] = *(uint32_t*)( startAddr + (i * sizeof(uint32_t)) );
  }
}

uint8_t Flash_writeWords(volatile uint32_t* arr, uint32_t startAddr, uint32_t size){

  // Check for address
  if(*(uint32_t*)startAddr != 0xFFFFFFFF) return 1;

  HAL_FLASH_Unlock();

  for(int i = 0; i < size; ++i){
    HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, (startAddr + (i * sizeof(uint32_t))), *(arr + i));
    FLASH_WaitForLastOperation(100);
  }

  HAL_FLASH_Lock();
  return 0;
}

uint8_t Flash_erasePage(uint16_t pageNum){
  uint8_t stat;
	
	stat = HAL_FLASH_Unlock();
	if(stat != 0) return stat;

	static FLASH_EraseInitTypeDef EraseInitStruct;
	EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
	EraseInitStruct.PageAddress = BASEADDR + (PAGESIZEASBYTES * pageNum);
	EraseInitStruct.NbPages = 1;
	uint32_t PageError = 0;
	
	stat = HAL_FLASHEx_Erase(&EraseInitStruct, &PageError);
	if(stat != 0) return stat;

	stat = HAL_FLASH_Lock();
	if(stat != 0) return stat;

	return 0;
}

uint8_t Flash_writeBytes(volatile uint8_t* arr, uint32_t startAddr, uint32_t size){

  // Check for address
  if(*(uint32_t*)startAddr != 0xFFFFFFFF) return 1;

	HAL_FLASH_Unlock();

	// check if arr + 4 is not buffer overflow
	if(size % 4 != 0){
	  size += 4 - (size % 4);
	}

	// Using uint8_t arr[] fill dummy with every 4 bytes and write it to flash
	for(int i = 0; i < size; i += 4){

    volatile uint32_t dummy = 0;

    dummy |= (arr[i + 0] << 0);
    dummy |= (arr[i + 1] << 8);
    dummy |= (arr[i + 2] << 16);
    dummy |= (arr[i + 3] << 24);

    FLASH_WaitForLastOperation(100);
		HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, (startAddr + i), dummy);
	}

	HAL_FLASH_Lock();
	return 0;
}

uint16_t Flash_readParameter(uint32_t startAddr, uint16_t index){

  return *(uint16_t*)(startAddr + (index * sizeof(uint16_t)) );
}

uint16_t Flash_writeParameter(uint32_t startAddr, uint16_t index, uint16_t val, uint16_t total){

  // Read user params page
  // Erase user params page
  // Update related param
  // Write user params page
  // Verify updated param
  // Write TAG info
  // Verify TAG info

  volatile uint8_t tmpParams[2 * total], TAG[2 * TAG_SIZE];

  Flash_readBytes(tmpParams, USER_PARAM_START_ADDR, 2 * total);

  Flash_readBytes(TAG, USER_TAG_START_ADDR, 2 * TAG_SIZE);

  Flash_erasePage(USER_PARAM_PAGE_NO);

  tmpParams[2 * index + 1] = val >> 8;
  tmpParams[2 * index + 0] = val >> 0;

  Flash_writeBytes(tmpParams, USER_PARAM_START_ADDR, 2 * total);

  Flash_writeBytes(TAG, USER_TAG_START_ADDR, 2 * TAG_SIZE);

  return 0;
}

uint16_t Flash_writeTAG(uint8_t *arr, uint16_t size, uint16_t total){

  // Read user params page
  // Erase user params page
  // Write user params page
  // Verify updated param
  // Write TAG info
  // Verify TAG info

  volatile uint8_t tmpParams[2 * total];

  Flash_readBytes(tmpParams, USER_PARAM_START_ADDR, 2 * total);

  Flash_erasePage(USER_PARAM_PAGE_NO);

  Flash_writeBytes(tmpParams, USER_PARAM_START_ADDR, 2 * total);

  Flash_writeBytes(arr, USER_TAG_START_ADDR, size);

  return 0;
}

uint16_t Flash_writeAllParameter(uint32_t startAddr, int16_t *val, uint16_t total){

  // Read user params page
  // Erase user params page
  // Update related param
  // Write user params page
  // Verify updated param
  // Write TAG info
  // Verify TAG info

  volatile uint8_t tmpParams[2 * total], TAG[2 * TAG_SIZE];

  Flash_readBytes(tmpParams, USER_PARAM_START_ADDR, 2 * total);

  Flash_readBytes(TAG, USER_TAG_START_ADDR, 2 * TAG_SIZE);

  for(uint16_t i = 0; i < total; i++){
    tmpParams[2 * i + 1] = *(val + i) >> 8;
    tmpParams[2 * i + 0] = *(val + i) >> 0;
  }

  Flash_erasePage(USER_PARAM_PAGE_NO);

  Flash_writeBytes(tmpParams, USER_PARAM_START_ADDR, 2 * total);

  Flash_writeBytes(TAG, USER_TAG_START_ADDR, 2 * TAG_SIZE);

  return 0;
}
