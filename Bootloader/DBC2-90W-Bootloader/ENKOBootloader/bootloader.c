#include "bootloader.h"

// BOOTLOADER Version
__attribute__((section(".VERSION.SGN"))) volatile const uint16_t VERSION = 3002;

#ifdef __STM32F0xx_HAL_H
__attribute__((section(".My_Vector_Table_Section.table")))  uint32_t VectorTable[50];
#endif //__STM32F0xx_HAL_H

//size of incoming data buffer as bytes
volatile uint32_t size_rxBuffer = 0;
//enumarator to keep status of bootloader
volatile BOOTLOADER_STATUS hsStatus, currentStatus = STATUS_COMM_START_NOT;
//holds flash erase state
volatile uint8_t flashFlag = 0;
//holds systick value when timeout count starts
volatile uint32_t prev_tick;
//holds retry count
volatile uint8_t tryCount = 0;
//Incoming data buffer
uint8_t rxBuffer[RX_BUFFER_SIZE];
//partitioned KEY values for crypto op.
uint32_t KEY8_1 = ( (uint32_t)KEY & 0x000000FF);
uint32_t KEY8_2 = ( (uint32_t)KEY & 0x0000FF00) >> 8;
uint32_t KEY8_3 = ( (uint32_t)KEY & 0x00FF0000) >> 16;
uint32_t KEY8_4 = ( (uint32_t)KEY & 0xFF000000) >> 24;
//Incoming data buffer for UART transmission
#ifdef BOOT_UART
uint8_t uartBuff[RX_BUFFER_SIZE];
#endif

 void BOOTLOADER_Init(void){

  #if !defined (DEBUG_MODE)
   _lockOB();
  #endif

   if(VERSION != 0) __NOP();

  //Init USB
  #ifdef BOOT_USB
  USBPinDown(100);
  MX_USB_DEVICE_Init();
  #endif

  //Init UART
  #ifdef BOOT_UART
    HAL_UART_Receive_IT(&huart2, uartBuff, RX_BUFFER_SIZE);
    //hUsbDeviceFS.dev_state = USBD_STATE_CONFIGURED;
  #endif

  do{
    //Init led
    #ifdef BOOT_LED
    HAL_GPIO_WritePin(BOOT_LED_PORT, BOOT_LED_PIN, BOOT_LED_ON_STATE);
    #endif

    //Init variables
    tryCount = 0;
    currentStatus = STATUS_COMM_START_NOT;

    //Wait for USB connection
    prev_tick = HAL_GetTick();
    while(HAL_GetTick() - prev_tick <= TIMEOUT_MS){
      //If there is USB connection
      if(getState()) continue;
      if(currentStatus == STATUS_COMM_END) break;

      //Wait for Communication
      prev_tick = HAL_GetTick();
      while(HAL_GetTick() - prev_tick <= TIMEOUT_COMM_MS){
        //Start Comm Byte
        sendByte(ACKLOD);
        Delay(100);
        if(currentStatus == STATUS_COMM_START_OK) break;
      }

      while(currentStatus == STATUS_COMM_START_OK){
        if(tryCount >= TRYTIMES){
          sendByte(COM_HATASI);
          currentStatus = STATUS_COMM_END;
          break;
        }

        //Wait till receive buffer is full or timed out of communication
        while( (size_rxBuffer < (RX_BUFFER_SIZE))  && (HAL_GetTick() - prev_tick <= TIMEOUT_COMM_MS));
        //stop sender
        sendByte(XOFF);

        //Solve the crypto
        for(uint16_t i = 0; i < RX_BUFFER_SIZE - 1; i = i+4){
          rxBuffer[i]     ^= KEY8_1;
          rxBuffer[i + 1] ^= KEY8_2;
          rxBuffer[i + 2] ^= KEY8_3;
          rxBuffer[i + 3] ^= KEY8_4;
        }

        //Buffer CRC check
        BOOTLOADER_STATUS tmp = checkCRC(rxBuffer, size_rxBuffer);
        if(tmp == STATUS_CRC_FAIL){
          sendByte(CRC_HATASI);
          tryCount++;
          size_rxBuffer = 0;
          continue;
        }

        //if it is the first one change status
        if(rxBuffer[0] == ID_HANDSHAKE){
          //Check for HW_ID and SW_ID
          //If it is wrong
          if(( (uint16_t)((uint16_t)rxBuffer[1] | (uint16_t)((uint16_t)rxBuffer[2] << 8)) != devID )){

            if( (uint32_t)rxBuffer[1] != DevID_DEFAULT ){
              sendByte(DEVID_HATASI);
              tryCount++;
              size_rxBuffer = 0;
              prev_tick = 0;
              continue;
            }else{
              sendByte(CRC_HATASI);
              tryCount++;
              size_rxBuffer = 0;
              prev_tick = 0;
              continue;
            }
          }
          HAL_FLASH_Unlock();

          KEY8_1 = rxBuffer[6];
          KEY8_2 = rxBuffer[7];
          KEY8_3 = rxBuffer[8];
          KEY8_4 = rxBuffer[9];
          hsStatus = STATUS_HANDSHAKE_OK;
          sendTAG();
          size_rxBuffer = 0;
        }
        //If it is the last one, change status
        else if(rxBuffer[0] == ID_EOF){
          currentStatus = STATUS_COMM_END;
          #ifdef BOOT_LED
          HAL_GPIO_WritePin(BOOT_LED_PORT, BOOT_LED_PIN, BOOT_LED_ON_STATE);
          #endif

          HAL_FLASH_Lock();
          sendTAG();
          Delay(1000);
          continue;
        }
        //if it is data, write it to flash
        else if(rxBuffer[0] == ID_FLASH_DATA){
          //if it is not validated yet
          if(hsStatus != STATUS_HANDSHAKE_OK){
            tryCount++;
            size_rxBuffer = 0;
            continue;
          }

          //If everything right, write to flash
          uint16_t pDataSize = ((rxBuffer[2] << 8) | rxBuffer[1]) - 4;
          writeBufftoFlash( (rxBuffer + 7), pDataSize, *(uint32_t*)&rxBuffer[3]);
          readBuffFromFlash((rxBuffer + 7), pDataSize, *(uint32_t*)&rxBuffer[3]);
          if( checkCRC(rxBuffer, size_rxBuffer) != STATUS_CRC_OK){
            sendByte(CRC_HATASI);
            tryCount++;
            size_rxBuffer = 0;
            continue;
          }
          #ifdef BOOT_LED
          HAL_GPIO_TogglePin(BOOT_LED_PORT, BOOT_LED_PIN);
          #endif
        }
        size_rxBuffer = 0;
        prev_tick = HAL_GetTick();
        //Start for next packet
        sendByte(XON);
      }
    }
    //If no connection
    //or comms times out
    //or firmware upgrade is done
    //Check and Jump to app code
  }while(checkAppSign(USER_PROG_SIGN_ADR, USER_PROG_CRC_ADR) != STATUS_SIGN_OK);


  #ifdef BOOT_LED
    HAL_GPIO_WritePin(BOOT_LED_PORT, BOOT_LED_PIN, BOOT_LED_OFF_STATE);
  #endif


  sendByte(BOOTLOADER_TIMEOUT);
  jumpToApp(USER_PROG_START_ADR);
}

void Delay(uint32_t _d){
  HAL_Delay(_d);
}

void USBPinDown(uint32_t _d){
#ifdef BOOT_USB
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  __HAL_RCC_GPIOA_CLK_ENABLE();
  HAL_GPIO_DeInit(USB_PORT, (USB_PIN_1|USB_PIN_2));
  GPIO_InitStruct.Pin = USB_PIN_1|USB_PIN_2;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(USB_PORT, &GPIO_InitStruct);
  HAL_GPIO_WritePin(USB_PORT, (USB_PIN_1|USB_PIN_2), GPIO_PIN_RESET);
  Delay(_d);
  HAL_GPIO_DeInit(USB_PORT, (USB_PIN_1|USB_PIN_2));
#endif
}

BOOTLOADER_STATUS checkAppSign(uint32_t _sign, uint32_t _crc){

  uint8_t* signature = (uint8_t*)_sign;
  if((signature[0]=='E') && (signature[1]=='N') && (signature[2]=='K') && (signature[3]=='O')){
      return STATUS_SIGN_OK;
  }

  sendByte(IMZA_HATASI);
  HAL_NVIC_SystemReset();
  return STATUS_SIGN_FAIL;
}

void sendByte(uint8_t byte){
#ifdef BOOT_UART
  HAL_UART_Transmit(&UART_NAME, &byte, 1, 100);
#endif
#ifdef  BOOT_USB
  uint32_t i = 0;
  if(hUsbDeviceFS.dev_state != USBD_STATE_CONFIGURED) return;
  while(CDC_Transmit_FS(&byte, 1) != USBD_OK){
    i++;
    if(i > 65535) break;
  }
#endif
}

BOOTLOADER_STATUS checkCRC(uint8_t* _data, uint32_t _size){
  volatile uint32_t crc = getCRC(_data, _size);

  if( *(uint32_t *) (_data + _size) != crc){
    return STATUS_CRC_OK;
  }
  sendByte(CRC_HATASI);
  return STATUS_CRC_FAIL;
}

uint32_t getCRC(uint8_t* _data, uint32_t _size){
  //CRC32
  //Polynomials chosen from https://users.ece.cmu.edu/~koopman/crc/
  volatile uint32_t crc = 0xf8c9140a;
  volatile uint32_t rout = 0;

  for(uint32_t i = 0; i < _size; i++){
    crc = ((uint16_t)( *(_data + i) )) ^ crc;

    for(uint16_t j = 0; j < 8; j++){

      rout = crc & 1;
      crc = crc >> 1;
      if(rout){
        crc = crc ^ 0xA0000001;
      }
    }
  }
  return crc;
}

void ISR_USBData(uint8_t* Buf, uint32_t *Len){

  //If transmission started now
  if(currentStatus == STATUS_COMM_START_NOT)  currentStatus = STATUS_COMM_START_OK;

  //If there is enough space in buffer
  if( (size_rxBuffer + *Len) <= RX_BUFFER_SIZE){
    uint32_t i;
    for(i = 0; i < *Len; i++){
      rxBuffer[size_rxBuffer + i] = Buf[i];
    }
    size_rxBuffer += i;

  //If there is not enough space in the buffer
  }else{
    __NOP();
  }
}

void writeBufftoFlash(uint8_t* _data, uint32_t _size, uint32_t _index){
  HAL_FLASH_Unlock();

  //If it is the first write
  if(flashFlag == 0){
    flashFlag = 1;
    //Clear the App Flash Area
    static FLASH_EraseInitTypeDef EraseInitStruct;
    uint32_t PageError = 0;

    #if defined __STM32H5xx_HAL_H
    uint32_t FirstSector = GetSector(USER_PROG_START_ADR);
    uint32_t NbOfSectors = GetSector(0x08010000 - 1) - FirstSector + 1;
    uint32_t BankNumber  = GetBank(USER_PROG_START_ADR);
    EraseInitStruct.TypeErase     = FLASH_TYPEERASE_SECTORS;
    EraseInitStruct.Banks         = BankNumber;
    EraseInitStruct.Sector        = FirstSector;
    EraseInitStruct.NbSectors     = NbOfSectors;

    HAL_FLASHEx_Erase(&EraseInitStruct, &PageError);
    FLASH_WaitForLastOperation(500);
    //! This is not the right way to do this, but it works :/
    PageError                     = 0;
    FirstSector                   = GetSector(0x08010000);
    NbOfSectors                   = GetSector(USER_PROG_SIGN_ADR) - FirstSector + 1;
    BankNumber                    = GetBank(USER_PROG_SIGN_ADR);
    EraseInitStruct.TypeErase     = FLASH_TYPEERASE_SECTORS;
    EraseInitStruct.Banks         = BankNumber;
    EraseInitStruct.Sector        = FirstSector;
    EraseInitStruct.NbSectors     = NbOfSectors;

    #elif defined __STM32F4xx_HAL_H
    uint32_t FirstSector = GetSector(USER_PROG_START_ADR);
    uint32_t NbOfSectors = GetSector(USER_PROG_SIGN_ADR) - FirstSector + 1;
    EraseInitStruct.TypeErase = FLASH_TYPEERASE_SECTORS;
    EraseInitStruct.VoltageRange  = FLASH_VOLTAGE_RANGE_3;
    EraseInitStruct.Sector        = FirstSector;
    EraseInitStruct.NbSectors     = NbOfSectors;
    #else
    EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
    EraseInitStruct.PageAddress = USER_PROG_START_ADR;
    EraseInitStruct.NbPages = USER_PROG_END_PAGE - USER_PROG_START_PAGE + 1;
    #endif

    HAL_FLASHEx_Erase(&EraseInitStruct, &PageError);
    FLASH_WaitForLastOperation(500);
  }

  uint32_t data32[4] = {0, 0, 0, 0};
  //Rearrange the incoming data packet
  for(uint32_t i = 0; i < _size; i+=16){

    data32[0] = data32[0] | _data[i + 0] << 0;
    data32[0] = data32[0] | _data[i + 1] << 8;
    data32[0] = data32[0] | _data[i + 2] << 16;
    data32[0] = data32[0] | _data[i + 3] << 24;

    data32[1] = data32[1] | _data[i + 4] << 0;
    data32[1] = data32[1] | _data[i + 5] << 8;
    data32[1] = data32[1] | _data[i + 6] << 16;
    data32[1] = data32[1] | _data[i + 7] << 24;

    data32[2] = data32[2] | _data[i + 8] << 0;
    data32[2] = data32[2] | _data[i + 9] << 8;
    data32[2] = data32[2] | _data[i + 10] << 16;
    data32[2] = data32[2] | _data[i + 11] << 24;

    data32[3] = data32[3] | _data[i + 12] << 0;
    data32[3] = data32[3] | _data[i + 13] << 8;
    data32[3] = data32[3] | _data[i + 14] << 16;
    data32[3] = data32[3] | _data[i + 15] << 24;

    FLASH_WaitForLastOperation(500);
    #if defined __STM32H5xx_HAL_H
    if((_index + i) >= USER_PROG_START_ADR && (_index + i) <= USER_PROG_SIGN_ADR){
      HAL_FLASH_Program(FLASH_TYPEPROGRAM_QUADWORD, _index + i, (uint32_t)data32);
    }
    data32[0] = 0;data32[1] = 0;data32[2] = 0;data32[3] = 0;
    #else
    uint32_t data32 = 0;
    //Rearrange the incoming data packet
    for(uint32_t i = 0; i < _size; i+=4){

      data32 = data32 | _data[i + 0] << 0;
      data32 = data32 | _data[i + 1] << 8;
      data32 = data32 | _data[i + 2] << 16;
      data32 = data32 | _data[i + 3] << 24;

      FLASH_WaitForLastOperation(500);
      if((_index + i) >= USER_PROG_START_ADR && (_index + i) <= USER_PROG_SIGN_ADR){
        HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, _index + i, data32);
      }
      data32 = 0;
    #endif

  }

  //HAL_FLASH_Lock();
}
}

void readBuffFromFlash(uint8_t* _data, uint32_t _size, uint32_t _index){

  for(uint32_t i = 0; i < _size; i += 4){
    if((_index + i) >= USER_PROG_START_ADR && (_index + i) <= USER_PROG_SIGN_ADR){
      _data[i] = *(__IO uint32_t*)(_index + i);
    }
  }
}

void sendTAG(void){
  //First byte is ID
  sendByte(ID_HANDSHAKE);
  //Sends TAG_SIZE bytes from just before USER_PROG_START_ADR
  uint32_t* tag = (uint32_t*)(USER_PROG_TAG_ADR);
  for(uint8_t i = 0; i < TAG_SIZE / 4; i++){
    sendByte(tag[i] >> 0);
    sendByte(tag[i] >> 8);
    sendByte(tag[i] >> 16);
    sendByte(tag[i] >> 24);
  }
  //Last byte is continue
  sendByte(XON);
}

void jumpToApp(uint32_t _addr){

  pFunction Jump_To_Application;
  uint32_t JumpAddress;

  //Disable USB
  #ifdef BOOT_USB
  USBD_DeInit(&hUsbDeviceFS);
  USBPinDown(100);
  #endif

  //Disable interrupts
  //__disable_irq();

  //define jump address
  JumpAddress = *(__IO uint32_t*)(_addr + 4);

  //define jump function
  Jump_To_Application = (pFunction) JumpAddress;

  //set stack pointer
  __set_MSP(*(__IO uint32_t*) _addr);

    //reenable irq
   // __enable_irq();
   HAL_DeInit();
   HAL_RCC_DeInit();
   SysTick->CTRL = 0;
  //make jump

  Jump_To_Application();
  __NOP();
}

#if !defined (DEBUG_MODE)
void _lockOB(void){
  #if defined (__STM32H5xx_HAL_H)
  // Debug Authentication Password Hash (SHA256)
  #if defined PASSWORD_PROTECTED
  // Hash of Password "0x00 0x01 0x02 0x03 0x04 0x05 0x06 0x07 0x08 0x09 0x0A 0x0B"
  uint16_t passwordHash[32] = {0xF3FF, 0xBCA9, 0x37DD, 0x3D36, 0x3C70, 0x4F1C, 0x1295, 0x3653,
                               0x1586, 0x6878, 0xD4F0, 0x6AF1, 0x020F, 0xF1D0, 0x24DA, 0xA2F9,
                               0x5077, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
                 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000};
  #else
    #error "Do not forget to define 16 Bytes Debug Authentication password and its 32 Bytes Hash!"
  #endif
  FLASH_OBProgramInitTypeDef OBInit;
  // Get current status of the OB
  HAL_FLASHEx_OBGetConfig(&OBInit);
  // If Product State is Open Flash can be accessed
  // So it should be closed for data safety
  if(OBInit.ProductState == OB_PROD_STATE_OPEN){
    // Unlock OB
    HAL_FLASH_Unlock();
    HAL_FLASH_OB_Unlock();
    // Change Product State (life cycle) to PROVISIONING
    OBInit.ProductState = OB_PROD_STATE_PROVISIONING;
    HAL_FLASHEx_OBProgram(&OBInit);
    // Launch means restart MCU
    HAL_FLASH_OB_Launch();
    // Lock OB
    HAL_FLASH_OB_Lock();
    HAL_FLASH_Lock();
    //Unlock OB
    HAL_FLASH_Unlock();
    HAL_FLASH_OB_Unlock();
    // In the Provisioning state Write One Time Programmable Debug Authentication Password
    for(uint8_t i = 0; i < sizeof(passwordHash) / sizeof(passwordHash[0]); ++i){
      HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD_OTP,
                          (OTP_START_ADDRESS + (i*sizeof(uint16_t))), (uint32_t)&passwordHash[i]);
        FLASH_WaitForLastOperation(100);
    }
    // If Product State is Provisioning Flash can be accessed
    // So it should be closed for data safety
    OBInit.ProductState = OB_PROD_STATE_CLOSED;
    HAL_FLASHEx_OBProgram(&OBInit);
    // Launch means restart MCU
    HAL_FLASH_OB_Launch();
    // Lock OB
    HAL_FLASH_OB_Lock();
    HAL_FLASH_Lock();

    // Redundant state check for guaranteeing data safe operation
  }else if(OBInit.ProductState == OB_PROD_STATE_PROVISIONING){
    // Unlock OB
    HAL_FLASH_Unlock();
    HAL_FLASH_OB_Unlock();
    // In the Provisioning state Write One Time Programmable Debug Authentication Password
    for(uint8_t i = 0; i < sizeof(passwordHash) / sizeof(passwordHash[0]); ++i){
      HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD_OTP,
                          (OTP_START_ADDRESS + (i*sizeof(uint16_t))), (uint32_t)&passwordHash[i]);
        FLASH_WaitForLastOperation(100);
    }
    // If Product State is Provisioning Flash can be accessed
    // So it should be closed for data safety
    OBInit.ProductState = OB_PROD_STATE_CLOSED;
    HAL_FLASHEx_OBProgram(&OBInit);
    // Launch means restart MCU
    HAL_FLASH_OB_Launch();
    // Lock OB
    HAL_FLASH_OB_Lock();
    HAL_FLASH_Lock();
  }
  HAL_FLASH_OB_Lock();
  HAL_FLASH_Lock();
  #else
  FLASH_OBProgramInitTypeDef OBInit;
  // Get current status of the OB
  HAL_FLASHEx_OBGetConfig(&OBInit);
  //If it is not LEVEL 1 protected, set it. Else just ignore.
  if(OBInit.RDPLevel != OB_RDP_LEVEL_1){
    //Unlock OB
    HAL_FLASH_Unlock();
    HAL_FLASH_OB_Unlock();
    // Set Protection Level 1. This Level means no read operation but OB can be change in exchange of mass erase.
    // LEVEL 2 means no change on OB
    OBInit.OptionType = OPTIONBYTE_RDP;
    OBInit.RDPLevel = OB_RDP_LEVEL_1;
    // Change OB
    HAL_FLASHEx_OBProgram(&OBInit);
    // Launch means restart MCU
    HAL_FLASH_OB_Launch();
  }
  //Lock OB
  HAL_FLASH_OB_Lock();
  HAL_FLASH_Lock();
  #endif
}
#endif

uint8_t getState(void){
  #ifdef BOOT_USB
    if(hUsbDeviceFS.dev_state != USBD_STATE_CONFIGURED){
    #ifdef BOOT_UART
      return 0;
    #endif
    return 1;
    }
  #endif
  return 0;
}

#if defined __STM32F4xx_HAL_H
static uint32_t GetSector(uint32_t Address){
  uint32_t sector = 0;

  if((Address < ADDR_FLASH_SECTOR_1) && (Address >= ADDR_FLASH_SECTOR_0))
  {
    sector = FLASH_SECTOR_0;
  }
  else if((Address < ADDR_FLASH_SECTOR_2) && (Address >= ADDR_FLASH_SECTOR_1))
  {
    sector = FLASH_SECTOR_1;
  }
  else if((Address < ADDR_FLASH_SECTOR_3) && (Address >= ADDR_FLASH_SECTOR_2))
  {
    sector = FLASH_SECTOR_2;
  }
  else if((Address < ADDR_FLASH_SECTOR_4) && (Address >= ADDR_FLASH_SECTOR_3))
  {
    sector = FLASH_SECTOR_3;
  }
  else if((Address < ADDR_FLASH_SECTOR_5) && (Address >= ADDR_FLASH_SECTOR_4))
  {
    sector = FLASH_SECTOR_4;
  }
  else if((Address < ADDR_FLASH_SECTOR_6) && (Address >= ADDR_FLASH_SECTOR_5))
  {
    sector = FLASH_SECTOR_5;
  }
  else if((Address < ADDR_FLASH_SECTOR_7) && (Address >= ADDR_FLASH_SECTOR_6))
  {
    sector = FLASH_SECTOR_6;
  }
  else if((Address < ADDR_FLASH_SECTOR_8) && (Address >= ADDR_FLASH_SECTOR_7))
  {
    sector = FLASH_SECTOR_7;
  }
  else if((Address < ADDR_FLASH_SECTOR_9) && (Address >= ADDR_FLASH_SECTOR_8))
  {
    sector = FLASH_SECTOR_8;
  }
  else if((Address < ADDR_FLASH_SECTOR_10) && (Address >= ADDR_FLASH_SECTOR_9))
  {
    sector = FLASH_SECTOR_9;
  }
  else if((Address < ADDR_FLASH_SECTOR_11) && (Address >= ADDR_FLASH_SECTOR_10))
  {
    sector = FLASH_SECTOR_10;
  }
  else if((Address < ADDR_FLASH_SECTOR_12) && (Address >= ADDR_FLASH_SECTOR_11))
  {
    sector = FLASH_SECTOR_11;
  }
  else if((Address < ADDR_FLASH_SECTOR_13) && (Address >= ADDR_FLASH_SECTOR_12))
  {
    sector = FLASH_SECTOR_12;
  }
  else if((Address < ADDR_FLASH_SECTOR_14) && (Address >= ADDR_FLASH_SECTOR_13))
  {
    sector = FLASH_SECTOR_13;
  }
  else if((Address < ADDR_FLASH_SECTOR_15) && (Address >= ADDR_FLASH_SECTOR_14))
  {
    sector = FLASH_SECTOR_14;
  }
  else if((Address < ADDR_FLASH_SECTOR_16) && (Address >= ADDR_FLASH_SECTOR_15))
  {
    sector = FLASH_SECTOR_15;
  }
  else if((Address < ADDR_FLASH_SECTOR_17) && (Address >= ADDR_FLASH_SECTOR_16))
  {
    sector = FLASH_SECTOR_16;
  }
  else if((Address < ADDR_FLASH_SECTOR_18) && (Address >= ADDR_FLASH_SECTOR_17))
  {
    sector = FLASH_SECTOR_17;
  }
  else if((Address < ADDR_FLASH_SECTOR_19) && (Address >= ADDR_FLASH_SECTOR_18))
  {
    sector = FLASH_SECTOR_18;
  }
  else if((Address < ADDR_FLASH_SECTOR_20) && (Address >= ADDR_FLASH_SECTOR_19))
  {
    sector = FLASH_SECTOR_19;
  }
  else if((Address < ADDR_FLASH_SECTOR_21) && (Address >= ADDR_FLASH_SECTOR_20))
  {
    sector = FLASH_SECTOR_20;
  }
  else if((Address < ADDR_FLASH_SECTOR_22) && (Address >= ADDR_FLASH_SECTOR_21))
  {
    sector = FLASH_SECTOR_21;
  }
  else if((Address < ADDR_FLASH_SECTOR_23) && (Address >= ADDR_FLASH_SECTOR_22))
  {
    sector = FLASH_SECTOR_22;
  }
  else /* (Address < FLASH_END_ADDR) && (Address >= ADDR_FLASH_SECTOR_23) */
  {
    sector = FLASH_SECTOR_23;
  }
  return sector;
}

#elif defined __STM32H5xx_HAL_H
//! These two functions was decleared static, might cause problems. Test them!
uint32_t GetSector(uint32_t Address)
{
  uint32_t sector = 0;

  if((Address >= FLASH_BASE) && (Address < FLASH_BASE + FLASH_BANK_SIZE))
  {
    sector = (Address & ~FLASH_BASE) / FLASH_SECTOR_SIZE;
  }
  else if ((Address >= FLASH_BASE + FLASH_BANK_SIZE) && (Address < FLASH_BASE + FLASH_SIZE))
  {
    sector = ((Address & ~FLASH_BASE) - FLASH_BANK_SIZE) / FLASH_SECTOR_SIZE;
  }
  else
  {
    sector = 0xFFFFFFFF; /* Address out of range */
  }

  return sector;
}
uint32_t GetBank(uint32_t Addr)
{
  uint32_t bank = 0;

  if (READ_BIT(FLASH->OPTSR_CUR, FLASH_OPTSR_SWAP_BANK) == 0)
  {
    /* No Bank swap */
    if (Addr < (FLASH_BASE + FLASH_BANK_SIZE))
    {
      bank = FLASH_BANK_1;
    }
    else
    {
      bank = FLASH_BANK_2;
    }
  }
  else
  {
    /* Bank swap */
    if (Addr < (FLASH_BASE + FLASH_BANK_SIZE))
    {
      bank = FLASH_BANK_2;
    }
    else
    {
      bank = FLASH_BANK_1;
    }
  }

  return bank;
}
#endif

#ifdef BOOT_UART
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart){
  /**
   * UART Boot process callback function for filling rxBuffer
   */
  uint32_t ptr = RX_BUFFER_SIZE;
  ISR_USBData(uartBuff, &ptr);
  HAL_UART_Receive_IT(&huart2, uartBuff, RX_BUFFER_SIZE);

}
#endif
