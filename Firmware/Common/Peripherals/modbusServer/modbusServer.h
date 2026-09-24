/**
 * @file modbusServer.h
 * @author ENKO Electronics
 * @brief  Modbus Server header file.
 * @version 0.1
 * @date 2023-05-22
 * 
 * @copyright Copyright (c) 2023 @ ENKO Electronics
 * 
 */
#ifndef MODBUSSERVER_H
#define MODBUSSERVER_H

#include "stdint.h"
#include "string.h"
#include "stdlib.h"
#include "main.h"

// Registers are divided in to logical groups of this size.
#define GROUP_SIZE 64

/**
 * @brief Enum for access level parameters. This is for readability.
 *
 */
typedef enum ACCESS_LEVEL_s{
  LEVEL_0 = 0, // No Protection
  LEVEL_1 = 1, // User
  LEVEL_2 = 2, // Service
  LEVEL_3 = 3, // Factory
  LEVEL_4 = 4, // Enko
  LEVEL_5 = 5 // Read Only
}ACCESS_LEVEL_t;

/**
 * @brief Enum for save condition parameters. This is for readability.
 *
 */
typedef enum SAVE_STATUS_s{
  SAVE_DISABLE = 0,
  SAVE_ENABLE = 1,
}SAVE_STATUS_t;

/**
 * @brief Modbus Register struct to fill in.
 * 
 */
typedef struct modBusRegister_s{
  // Pointer to the value of the register
  int16_t*        value;
  //0 based modbus register address
  const uint16_t        mbAddress;
  // default value of the register
  const uint16_t        Default;
  //defined default value of this register
  const int16_t         MaxValue;
  //defined min value of this register
  //const uint16_t      Default;
  //defined max value of this register
  const int16_t         MinValue;
  //defined access level identifier
  const ACCESS_LEVEL_t  accessLevel;
  //Save action flag to call ISaveParam func
  const SAVE_STATUS_t   saveEnable;
  //Callback function pointer to call at the write action of this register
  void (*callbackFnPointer)(void);
}modBusRegister_t;

/**
 * @brief Server type is used for creating different server object at the same time
 * 
 */
typedef enum serverType_e{
  //Placeholder for switch case
  Server_ERR = 0,
  //Serial server type
  Server_RTU = 1,
  //ASCII server type
  Server_ASCII = 2,
  //TCP server type
  Server_TCP = 3,

}serverType_t; 

//Function pointer type for Interface save functions
typedef void (*saveParamF)(uint16_t indexAddr, uint16_t data);

/**
 * @brief modbus parameter group typedef
 * Each group contains 100 register if they are defined in the real register array.
 * 
 */
typedef struct group_s{
  uint16_t max;     // Max mbAddr % GROUP_SIZE inside of the group
  // max is required because there can be groups with only 0th address
  uint16_t size;    // Size of the group. It can be max 100
  uint16_t* ptr;    // Pointer to group indexes array
}group_t;


/**
 * @brief Modbus server main object that has related attributes and functions.
 * @note This objects should be created with modbusCreate function and should be filled properly to operate.
 * 
 */
typedef struct modbusServerObject_s{

  //MB protocol server address.
  //This can change in case of multiple servers on same physical line
  uint8_t sAddr;
  //Server type definition to decide how to parse and pack the mb packets
  serverType_t serverType;
  //Size of the request array as bytes.
  uint16_t reqPacketSize;
  //MB request packet from the client. This array gets filled with addRequest function
  uint8_t reqPacket[MB_REQ_SIZE];
  //response packet to fill in according to request
  uint8_t response[256]; //255 is the max response size supported by modbus
  //MB registers array that has related attributes in it
  const modBusRegister_t *modbusRegisters;
  // array of modbus register groups
  group_t *groups;
  //flag to indicate there is not handled request packet.
  uint8_t requestFlag;
  //mb register's total count related to this server
  uint16_t MB_REG_COUNT;
  // Access Level for modbus user on other end of device
  ACCESS_LEVEL_t userLevel;
  //Pointer function to save parameters
  saveParamF saveParam;
  // Address to MBReg Array index variable. Changes on every request
  uint16_t index;
  // Timer for loging out user from access level
  uint16_t loginTimer;
  // loginTimerMaxCount [ms] of inactivity on MB would trigger logout
  // Timer max value to compare 
  uint16_t loginTimerMaxCount;
}modbusServerObject_t;

/**
 * @brief Packet structure to parse incoming mb request
 * 
 */
typedef struct{
  //todo this struct should have different params for different server types. E.g. header, data?, trailer
  //server address on packet
  uint8_t sAddr;
  //function on code packet
  uint8_t fnCode;
  //modbus address on packet
  uint16_t mbAddr;
  //quantity requested on packet
  uint16_t quantity;
  //crc on packet
  uint16_t crc;
}modbusPacket_t;


/**
 * @brief Object creator function to fill and return server parameters
 * 
 * @note for now this only implements RTU type server properties.
 * @return modbusServerObject_t server object to use on modbus comms
 */
void modbusServerCreate(
    modbusServerObject_t *server, //Server object to fill
    uint8_t serverAddr,           // Server address for the master device
    serverType_t serverType, // Server Type for the server object. This determines how should request packet be handled.
    //?uint8_t* reqPacket, //Pointer for array of request packet. This packet could be RTU, ASCII, TCP?!?!?
    //uint16_t reqPacketSize, //Size of the request array as bytes.
    const modBusRegister_t* modbusRegisters, // user should fill this struct somewhere else and create object afterwards.
    uint16_t registerCount, //Total register count for modbusRegisters array
    saveParamF saveParamFunc, //Func pointer to implement for parameter permanent saves e.g. FRAM
    uint16_t loginTimerMaxCount
);

/**
 * @brief This function should be called periodically so server functions can run.
 * @note It could be called inside 10ms RTOS task but this haven't been tested yet.
 */
uint16_t modbusServerRun(
    modbusServerObject_t* server
);

/**
 * @brief This function fills request array when called inside some sort of interrupt.
 *          IT could be USB, uart etc. or DMA complt IT.
 */
uint8_t* modbusServerAddRequest(
    modbusServerObject_t* server,   // Server object to fill in. This should be created first.
    uint8_t reqPacket[MB_REQ_SIZE], // Incoming modbus request packet
    uint16_t size,                  // size of the request buffer
    uint16_t *responseLen           // Size of the response buffer
);

/**
 * @brief inline function to parse incoming packet
 * 
 * @return modbusPacket_t packet object that has required attributes.
 */
modbusPacket_t modbusObjectParse(
    const modbusServerObject_t* server //Server object to fill in. This should be created first.
);

/**
 * @brief Set the Param Val By Address object
 * @note This function does check boundary conditions.
 */
uint8_t setParamValByAddress(
    modbusServerObject_t* server, //Server object to fill in. This should be created first.
    uint16_t mbAddr, //modbus address to change actual value
    int16_t val //value to set
);

/**
 * @brief Sets the value of the given modbus register without boundary checking.
 * First consider not using it if possible.
 */
uint8_t setParamValByAddress_unsafe(
    modbusServerObject_t* server, //Server object to fill in. This should be created first.
    uint16_t mbAddr, //modbus address to change actual value
    int16_t val //value to set
);

/**
 * @brief Get the Param Val By Address object.
 * @note This function does not check for access level.
 */
int16_t getParamValByAddress(
    modbusServerObject_t* server, //Server object to fill in. This should be created first.
    uint16_t mbAddr // Modbus address to check for value.
);

/**
 * @brief Get the Param Min By Address object.
 * @note This function does not check for access level.
 */
int16_t getParamMinByAddress(
    modbusServerObject_t* server, //Server object to fill in. This should be created first.
    uint16_t mbAddr               // Modbus address to check for value.
);

/**
 * @brief Get the Param Max By Address object.
 * @note This function does not check for access level.
 */
int16_t getParamMaxByAddress(
    modbusServerObject_t* server, //Server object to fill in. This should be created first.
    uint16_t mbAddr               // Modbus address to check for value.
);

/**
 * @brief Get the Param default By Address object.
 * @note This function does not check for access level.
 * @param server  Server object to fill in. This should be created first.
 * @param mbAddr  Modbus address to check for value.
 */
int16_t getParamDefaultByAddress(modbusServerObject_t* server, uint16_t mbAddr);

/**
 * @brief calculates modbus CRC 16 for parameter array.
 * @note This function was taken from NY modbus lib.
 * 
 * @return uint16_t CRC16 for the array.
 */
uint16_t modbusGetCRC16(
    uint8_t *packet, //Array to calculate CRC for.
    uint32_t size //Array size to iterate through.
);

/**
 * @brief mapping function to convert register address to register index for size considerations
 */
uint16_t modbusServergetIndex(modbusServerObject_t *server, uint16_t mbAddr);

/**
 * @brief updates loginTimer of the modbus server object
 * @note should be called every ms
 * @param server modbus server object to update
 */
void modbusServerUpdateTimer(modbusServerObject_t *server);

/*----------------------------------Set-Get-Interface Functions----------------------------------*/
//todo Below funcs not implemented yet!.
void modbusServerDelete(modbusServerObject_t* server);
//get def by addr
//get min max by addr
//


/**
 * @brief Interface Function to init modbusServerObject_t pointer.
 * @note Should be called on system init
 *
 */
void ImodbusServerInit(
    modbusServerObject_t *server //Server object to fill in
);

/**
 * @brief Check ENKO signature and perform factory reset if mismatch
 * 
 * This function checks if the controller.enkoSignature parameter matches
 * the ENKO_SIGNATURE constant. If they don't match, it performs a factory
 * reset to restore all parameters to their default values.
 */
void ImodbusCheckEnkoSignature(void);
#endif //MODBUSSERVER_H
