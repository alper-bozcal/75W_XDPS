#include "Flash.h"

/**
 * @brief this func reads 32bit value from flash
 * 
 * @param _addr this is the address of the value in the flash
 * @return uint32_t 
 */
uint32_t Flash_readWordFromAddr(__IO uint32_t* _addr){

	//This function added for forward compatibility
	return *_addr;
}

/**
 * @brief Erase the page using page number. This number can change in other MCUs
 * Becareful this function uses BASEADDR and PAGESIZEASBYTES macros
 * 
 * @param _page page number from flash table you can get this info from build analyzer in cubeide
 * @return HAL_StatusTypeDef 
 */
HAL_StatusTypeDef Flash_ErasePageWithNumber(uint8_t _page){
	
	HAL_StatusTypeDef stat;	
	
	stat = HAL_FLASH_Unlock();
	if(stat != HAL_OK) return stat;

	static FLASH_EraseInitTypeDef EraseInitStruct;
	EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
	EraseInitStruct.PageAddress = BASEADDR + (PAGESIZEASBYTES * _page);
	EraseInitStruct.NbPages = 1;
	uint32_t PageError = 0;
	
	stat = HAL_FLASHEx_Erase(&EraseInitStruct, &PageError);
	if(stat != HAL_OK) return stat;

	stat = HAL_FLASH_Lock();
	if(stat != HAL_OK) return stat;

	return HAL_OK;
}

/**
 * @brief writes data specified with pointer to the flash address word by word
 * Should be called after erase functions
 * 
 * @param _data pointer to the data it should be increments of half word
 * @param _size size of the data as bytes
 * @param _addr flash address to be write that will be incremented
 * @return HAL_StatusTypeDef 
 */
HAL_StatusTypeDef Flash_writeDataChunktoAddr(uint16_t* _data, uint32_t _addr, uint16_t _size){

	if(_addr < 0x080F800 || _addr > 0x0800FFFF) return HAL_ERROR;
	HAL_StatusTypeDef stat;

	stat = HAL_FLASH_Unlock();
	if(stat != HAL_OK) return stat;

	for(int i = 0; i < _size / 2; i++){
		stat = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, _addr, *(_data + i));
		_addr += 4;
		if(stat != HAL_OK) return stat;
	}

	stat = HAL_FLASH_Lock();
	if(stat != HAL_OK) return stat;

	return HAL_OK;
}

/**
 * @brief reads _size bytes from flash address to the _data
 * 
 * @param _data pointer to the read data
 * @param _addr flash start address of data
 * @param _size size as words to read
 * @return HAL_StatusTypeDef 
 */
HAL_StatusTypeDef Flash_readDataChunkFromAddr(int16_t* _data, uint32_t  _addr, uint16_t _size){

	if(_addr < 0x080F800 || _addr > 0x0800FFFF) return HAL_ERROR;
	uint16_t i = 0, j = 0;
	for(i = 0; i < _size / 2; i++){
//		_data[i] = Flash_readWordFromAddr( (uint32_t*)(_addr + (i * 4)) );
//		*(uint32_t*)(_data + i) = Flash_readWordFromAddr( (uint32_t*)(_addr + (i * 4)) );
		*(uint32_t*)(_data + j) = Flash_readWordFromAddr( (uint32_t*)(_addr + (i * 4)) );
		j += 2;
		if(j > (_size/2) - 2) break;

	}
	__NOP();
	return HAL_OK;
}

/**
 * @brief writes word param to the specified address by reading whole page and re-writing it
 * Use it with caution!
 * @param _data data to be stored in flash
 * @param _addr address on flash
 * @return HAL_StatusTypeDef 
 */
HAL_StatusTypeDef Flash_writeWordtoAddr(uint32_t _data, uint32_t _addr){
	//read whole page
	uint32_t _page[PAGESIZEASBYTES / 4];
	Flash_readWordChunkFromAddr(_page, USERBASEADDR, PAGESIZEASBYTES / 4);

	//edit page
	_page[(_addr - USERBASEADDR) / 4] = _data;

	//erase page from flash
	Flash_ErasePageWithNumber(31);

	//write back
	Flash_writeWordChunktoAddr(_page, USERBASEADDR,PAGESIZEASBYTES / 4);

	return HAL_OK;
}

/**
 * @brief Writes array of words to the specified address
 * 
 * @param _data pointer to the 32 bit words
 * @param _addr flash address to be written
 * @param _size size as bytes
 * @return HAL_StatusTypeDef 
 */
HAL_StatusTypeDef Flash_writeWordChunktoAddr(uint32_t* _data, uint32_t _addr, uint16_t _size){

	HAL_StatusTypeDef stat;

	stat = HAL_FLASH_Unlock();
	if(stat != HAL_OK) return stat;

	for(int i = 0; i < _size / 4; i++){
		stat = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, _addr, *(_data + i));
		_addr += 4;
		if(stat != HAL_OK) return stat;
	}

	stat = HAL_FLASH_Lock();
	if(stat != HAL_OK) return stat;

	return HAL_OK;
}

/**
 * @brief read word array from flash
 * 
 * @param _data pointer to the output array
 * @param _addr flash address to be read
 * @param _size size as bytes
 * @return HAL_StatusTypeDef 
 */
HAL_StatusTypeDef Flash_readWordChunkFromAddr(uint32_t* _data, uint32_t  _addr, uint16_t _size){

	for(uint16_t i = 0; i < _size / 4; i++){
		_data[i] = Flash_readWordFromAddr( (uint32_t*)(_addr + (i * 4)) );
	}
	return HAL_OK;
}
