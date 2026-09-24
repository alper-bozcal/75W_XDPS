#ifndef BOOTLOADER_H
#define BOOTLOADER_H

#include "usb_device.h"
#include "stm32f0xx_hal.h"
#include "usbd_cdc_if.h"

//User Definitions--------------------------------------------------------------------------
//start address of the APP code
#define USER_PROG_START_ADR 0x08004000

//signature address for APP code. It is hard coded check for "ENKO" string at the end of code area
#define USER_PROG_SIGN_ADR  0x800EFFC

//CRC address for BL + APP Code. CRC of this should be calculated with script and write to this specific address
#define USER_PROG_CRC_ADR 0x800F7F8

//wait time for cases there are no usb connection
//Should be higher than 700ms
#define TIMEOUT_MS        1000
//wait time for cases when there are usb connection but not communication
#define TIMEOUT_COMM_MS   1000

//Start page of the APP code
#define USER_PROG_START_PAGE  9
//end page of the APP code, if there is blank space at the end of APP it should not be included in this
#define USER_PROG_END_PAGE    29
//page size for fixed sized mcus
#define PAGE_SIZE         2048

//ID value for product. This is specific for every product
#define devID    12

// Debug Authentication Password Hash (SHA256)
#define PASSWORD_PROTECTED
#define OTP_START_ADDRESS     0x08FFF000

//TAG address for SCADA integration this is for where to look for tag readings, should be the last part of app code
#define USER_PROG_TAG_ADR  0x800FFC0

//TAG SIZE for SCADA integration this is for how much bytes should be read at the last section of app region
#define TAG_SIZE 64

//END User Definitions----------------------------------------------------------------------

//User Selections---------------------------------------------------------------------------

// If defined, Option Byte is not going to set.
#define DEBUG_MODE  1

//Led to indicate boot state
#define BOOT_LED 1
#ifdef  BOOT_LED
  #define BOOT_LED_PORT LED_B_GPIO_Port
  #define BOOT_LED_PIN  LED_B_Pin
  #define BOOT_LED_OFF_STATE GPIO_PIN_SET
  #define BOOT_LED_ON_STATE GPIO_PIN_RESET
#endif

//USB definitions for boot source
#define BOOT_USB
//#define BOOT_UART
#ifdef  BOOT_USB
  #define USB_PIN_1 GPIO_PIN_11
  #define USB_PIN_2 GPIO_PIN_12
  #define USB_PORT  GPIOA
//USB handler for HAL library
extern   USBD_HandleTypeDef hUsbDeviceFS;
#endif

//UART definitions for boot source
#ifdef  BOOT_UART
  #define UART_PIN_1  GPIO_PIN_2
  #define UART_PIN_2  GPIO_PIN_3
  #define UART_PORT GPIOA
extern UART_HandleTypeDef huart2;
  #define UART_NAME huart2
#endif
//END User Selections-----------------------------------------------------------------------

//Total packet size for communication
#define RX_BUFFER_SIZE  972

//Communication definitions, they are important for SCADA
#define ACKLOD 0x06
#define XON    0x11
#define XOFF   0x13

//Error definitions. They are important for SCADA
#define IMZA_HATASI         0x20
#define DEVID_HATASI        0x21
#define CRC_HATASI          0x22
#define COM_HATASI          0x24
#define BOOTLOADER_TIMEOUT  0x25

//Packet ID definitions, first byte of incoming packet
#define ID_EOF              0 //Last packet ID
#define ID_FLASH_DATA       1 //Normal transmission ID
#define ID_FLASH_CRC        2 //CRC packet ID
#define ID_HANDSHAKE        3 //HS packet ID

//Default IDs for ENKO
#define DevID_DEFAULT       1

//wrong packet try time for timeout
#define TRYTIMES 5

//default CRYPTO key for SCADA, shouldn't change
#define KEY 123456

//Jump function definition
typedef  void (*pFunction)(void);

//Status flags for bootloader app
typedef enum BOOTLOADER_STATUS{
  STATUS_SIGN_OK = 0,
  STATUS_SIGN_FAIL,

  STATUS_COMM_START_OK,
  STATUS_COMM_START_NOT,
  STATUS_COMM_END,

  STATUS_CRC_OK,
  STATUS_CRC_FAIL,

  STATUS_HANDSHAKE_OK,
  STATUS_HANDSHAKE_FAIL,
}BOOTLOADER_STATUS;

/**
 * @brief delay interface function
 * in future HAL_Delay can be replaced
 */
void Delay(uint32_t _d);

/**
 * @brief Keeps the USB pins down for specified amount of ms.
 * This way it resets on PC side
 * @param _d: time to delay as ms
 */
void USBPinDown(uint32_t _d);

/**
 * Actual bootloader state machine
 */
void BOOTLOADER_Init(void);

/**
 * @brief to check application signature
 * @param _addr signature address
 * @param _crc crc address for BL + APP code area
 */
BOOTLOADER_STATUS checkAppSign(uint32_t _sign, uint32_t _crc);

/**
 *@brief send 1 byte of data
 *@param byte data to sent
 */
void sendByte(uint8_t byte);

/**
 *@brief check CRC32 of the buffer
 *@param _data buffer pointer
 *@param _size buffer size as bytes
 */
BOOTLOADER_STATUS checkCRC(uint8_t* _data, uint32_t _size);

/**
 *@brief  get CRC32 of the buffer
 *@param  _data buffer pointer
 *@param  _size buffer size as bytes
 *@retval CRC32 value of the buffer
 */
uint32_t getCRC(uint8_t* _data, uint32_t _size);

/**
 *@brief writes supplied buf to flash with specified address
 *@param _data buffer pointer
 *@param _szie buffer size as bytes
 *@param _index flash address to write
 */
void writeBufftoFlash(uint8_t* _data, uint32_t _size, uint32_t _index);

/**

 *@brief reads specified data from flash to buffer
 *@param _data buffer pointer
 *@param _size read size as bytes
 *@param _index flash address to write
 */
void readBuffFromFlash(uint8_t* _data, uint32_t _size, uint32_t _index);

/**
 * @brief sends TAG packet created from last part of the Boot loader flash region
 */
void sendTAG(void);

/**
 * @brief Jumps to address specified by parameter _addr
 * @param _addr Jump address
 */
void jumpToApp(uint32_t _addr);

/**
 * @brief to get communication interface state, if it is configured, device would wait for TIMEOUT_COMM_MS
 */
uint8_t getState(void);
#if defined __STM32F4xx_HAL_H
/**
  * @brief  Gets the sector of a given address. This function has been taken from ST examples
  * @param  Address:
  * @retval The sector of a given address
  */
static uint32_t GetSector(uint32_t Address);

//Flash Changes For STM32F4 MCU's
/* Base address of the Flash sectors Bank 1 */
#define ADDR_FLASH_SECTOR_0     ((uint32_t)0x08000000) /* Base @ of Sector 0, 16 Kbytes */
#define ADDR_FLASH_SECTOR_1     ((uint32_t)0x08004000) /* Base @ of Sector 1, 16 Kbytes */
#define ADDR_FLASH_SECTOR_2     ((uint32_t)0x08008000) /* Base @ of Sector 2, 16 Kbytes */
#define ADDR_FLASH_SECTOR_3     ((uint32_t)0x0800C000) /* Base @ of Sector 3, 16 Kbytes */
#define ADDR_FLASH_SECTOR_4     ((uint32_t)0x08010000) /* Base @ of Sector 4, 64 Kbytes */
#define ADDR_FLASH_SECTOR_5     ((uint32_t)0x08020000) /* Base @ of Sector 5, 128 Kbytes */
#define ADDR_FLASH_SECTOR_6     ((uint32_t)0x08040000) /* Base @ of Sector 6, 128 Kbytes */
#define ADDR_FLASH_SECTOR_7     ((uint32_t)0x08060000) /* Base @ of Sector 7, 128 Kbytes */
#define ADDR_FLASH_SECTOR_8     ((uint32_t)0x08080000) /* Base @ of Sector 8, 128 Kbytes */
#define ADDR_FLASH_SECTOR_9     ((uint32_t)0x080A0000) /* Base @ of Sector 9, 128 Kbytes */
#define ADDR_FLASH_SECTOR_10    ((uint32_t)0x080C0000) /* Base @ of Sector 10, 128 Kbytes */
#define ADDR_FLASH_SECTOR_11    ((uint32_t)0x080E0000) /* Base @ of Sector 11, 128 Kbytes */

/* Base address of the Flash sectors Bank 2 */
#define ADDR_FLASH_SECTOR_12     ((uint32_t)0x08100000) /* Base @ of Sector 0, 16 Kbytes */
#define ADDR_FLASH_SECTOR_13     ((uint32_t)0x08104000) /* Base @ of Sector 1, 16 Kbytes */
#define ADDR_FLASH_SECTOR_14     ((uint32_t)0x08108000) /* Base @ of Sector 2, 16 Kbytes */
#define ADDR_FLASH_SECTOR_15     ((uint32_t)0x0810C000) /* Base @ of Sector 3, 16 Kbytes */
#define ADDR_FLASH_SECTOR_16     ((uint32_t)0x08110000) /* Base @ of Sector 4, 64 Kbytes */
#define ADDR_FLASH_SECTOR_17     ((uint32_t)0x08120000) /* Base @ of Sector 5, 128 Kbytes */
#define ADDR_FLASH_SECTOR_18     ((uint32_t)0x08140000) /* Base @ of Sector 6, 128 Kbytes */
#define ADDR_FLASH_SECTOR_19     ((uint32_t)0x08160000) /* Base @ of Sector 7, 128 Kbytes */
#define ADDR_FLASH_SECTOR_20     ((uint32_t)0x08180000) /* Base @ of Sector 8, 128 Kbytes  */
#define ADDR_FLASH_SECTOR_21     ((uint32_t)0x081A0000) /* Base @ of Sector 9, 128 Kbytes  */
#define ADDR_FLASH_SECTOR_22     ((uint32_t)0x081C0000) /* Base @ of Sector 10, 128 Kbytes */
#define ADDR_FLASH_SECTOR_23     ((uint32_t)0x081E0000) /* Base @ of Sector 11, 128 Kbytes */

#elif defined __STM32H5xx_HAL_H
uint32_t GetSector(uint32_t Address);
uint32_t GetBank(uint32_t Addr);
#endif

#if !defined (DEBUG_MODE)
  void _lockOB(void);
#endif

#endif  //BOOTLOADER_H
