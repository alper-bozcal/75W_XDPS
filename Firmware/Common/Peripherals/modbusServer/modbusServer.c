/**
 * @file modbusServer.c
 * @author ENKO Electronics
 * @brief  Modbus Server Implementation file.
 * @version 0.2
 * @date 2023-09-22
 * 
 * @copyright Copyright (c) 2023 @ ENKO Electronics
 * 
 * @note Resources can be found below:
 * * https://www.simplymodbus.ca/exceptions.htm
 * * https://www.vtscada.com/help/Content/D_Tags/Dev_Modbus_ErrMsg.htm
 * *  https://www.se.com/us/en/faqs/FA168406/#:~:text=Modbus%20is%20a%20serial%20communication,programmable%20logic%20controllers%20(PLCs).
 * * https://crccalc.com/
 * * https://npulse.net/en/online-modbus
 * * https://www.modbustools.com/modbus.html
 */

#include "modbusServer.h" //Header file for Server
#include "modbus-table.h" //Header file for Modbus address definitions

// External reference to ENKO password array from ImodbusServer.c
extern int16_t ENKO[15];

// Backup variables to store actual password values (defined in ImodbusServer.c)
extern int16_t ENKO_PASSWORD_BACKUP;
extern int16_t FACTORY_PASSWORD_BACKUP;
extern int16_t SERVICE_PASSWORD_BACKUP;
extern int16_t USER_PASSWORD_BACKUP;

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
    const modBusRegister_t *modbusRegisters, // user should fill this struct somewhere else and create object afterwards.
    uint16_t registerCount, //Total register count for modbusRegisters array
    saveParamF saveParamFunc, //Func pointer to implement for parameter permanent saves e.g. FRAM
    uint16_t loginTimerMaxCount
){

  switch (serverType)
  {
  case Server_RTU:
    server->sAddr = serverAddr;
    server->serverType = serverType;
    server->reqPacketSize = 256;//reqPacketSize;
    // server->reqPacket
    // server->response
    server->modbusRegisters = modbusRegisters;
    server->requestFlag = 0;
    server->MB_REG_COUNT = registerCount;
    server->userLevel = LEVEL_0;

    server->index = 0;

    server->saveParam = saveParamFunc;

    server->loginTimer = 0;
    server->loginTimerMaxCount = loginTimerMaxCount;

    // Determine how many groups of GROUP_SIZE there are in the modbusRegisters array
    // This is used for double array's first brackets
    uint16_t max = 0;
    for(uint16_t i = 0; i < server->MB_REG_COUNT; i++){
      if(max < (server->modbusRegisters[i].mbAddress / GROUP_SIZE)) max = server->modbusRegisters[i].mbAddress / GROUP_SIZE;
    }
    // Do not forget to include last group too
    // note: You can also make <= or >= where you use max
    max++;
    // allocate memory to groups array
    server->groups = malloc(max * sizeof(group_t));
    if(server->groups == NULL){
      assert("Not enough heap for modbus :( ");
    }
    // Initialise groups array. This is especially required if using CCMRAM
    for(uint16_t i = 0; i < max; i++){
      server->groups[i].size = 0;
      server->groups[i].max = 0;
    }
    // Determine each groups required size
    // This is used for double array's second brackets
    // Determine each groups max address
    // This is used for allocating memory to group
    for(uint16_t i = 0; i < server->MB_REG_COUNT; i++){
      server->groups[server->modbusRegisters[i].mbAddress / GROUP_SIZE].size += 1;
      if(server->groups[server->modbusRegisters[i].mbAddress / GROUP_SIZE].max < server->modbusRegisters[i].mbAddress % GROUP_SIZE) server->groups[server->modbusRegisters[i].mbAddress / GROUP_SIZE].max =server->modbusRegisters[i].mbAddress % GROUP_SIZE;
    }
    // Iterate over all groups and allocate memory to each one
    // If groups is empty, initialise it with NULL
    for(uint16_t i = 0; i < max; i++){
      if(server->groups[i].size != 0){
        // Allocate memory to all group including blank spaces
        // Ideally it should not have any blank spaces inside the group
        server->groups[i].ptr = malloc(server->groups[i].max * sizeof(uint16_t));
        if(server->groups[i].ptr == NULL){
          assert("Not enough heap for modbus :( ");
        }
        memset(server->groups[i].ptr, 0xFFFF, (server->groups[i].max * sizeof(uint16_t)));
      }else{
        // Group is empty
        server->groups[i].ptr = NULL;
      }
    }
    // Iterate through all registers and fill respected group with index
    uint16_t a = 0, b = 0;
    for(uint16_t i = 0; i < server->MB_REG_COUNT; i++){
      a = server->modbusRegisters[i].mbAddress / GROUP_SIZE;
      b = server->modbusRegisters[i].mbAddress % GROUP_SIZE;
      *(server->groups[a].ptr + b) = i;
    }

    *(server->groups[0].ptr + 0) = 0;

    break;
  case Server_ASCII:
    break;
  case Server_TCP:
    break;
  case Server_ERR:
    break;
  default:
    break;
  }
}

/**
 * @brief This function fills request array when called inside some sort of interrupt.
 *          IT could be USB, uart etc. or DMA complt IT.
 */
uint8_t* modbusServerAddRequest(
    modbusServerObject_t* server, //Server object to fill in. This should be created first.
    uint8_t reqPacket[MB_REQ_SIZE], //Incoming modbus request packet
    uint16_t size, //size of the request buffer
    uint16_t *responseLen
){
  // If flag is set, just skip the request
  // If request packet is bigger than buffer, don't respond
  if((server->requestFlag == 0) && size < MB_REQ_SIZE){
    //Fill in the request
    memcpy(server->reqPacket, reqPacket, size);
    server->reqPacketSize = size;
    //Set the flag for Run functions
    server->requestFlag = 1;
    // Run the actual modbus protocol and get the response LEN if any
    *responseLen = modbusServerRun(server);
    // Return the response buffer inside the server for transmission
    return server->response;
  }
  return NULL;
}

/**
 * @brief This function should be called periodically so server functions can run.
 * @note It could be called inside 10ms RTOS task but this haven't been tested yet.
 */
uint16_t modbusServerRun(
    modbusServerObject_t* server //Server object to fill in. This should be created first.
){
  if(!server->requestFlag) return 0;

  server->requestFlag = 0;
  // Clear login Timer, there is activity
  server->loginTimer = 0;

  modbusPacket_t packet = modbusObjectParse(server);

  //If this slave is not the intended responder, just ignore the packet
  if(packet.sAddr != server->sAddr){

    // https://www.modbus.org/docs/PI_MBUS_300.pdf
    // Or report the slave ID
    if(packet.fnCode == 0x11){  // Report Slave ID

      server->response[0] = server->sAddr;
      server->response[1] = packet.fnCode;
      server->response[2] = 2; // 2 Bytes of data
      server->response[3] = server->sAddr; // Slave Addr
      server->response[4] = 0xFF; //Device is ON

      uint16_t crc = modbusGetCRC16(server->response, 5);
      server->response[5] = (uint8_t)(crc >> 8);
      server->response[6] = (uint8_t)(crc);

      // return the size of the response packet
      return 7;
    }
    return 0;
  }

  //If there is CRC error, Slave should not return anything
  //https://www.simplymodbus.ca/exceptions.htm
  if(modbusGetCRC16(server->reqPacket, server->reqPacketSize - 2) != packet.crc) return 0;

  server->index = modbusServergetIndex(server, packet.mbAddr);

  uint16_t size  = 0;
  uint16_t crc   = 0;
  uint16_t index = 0;

  switch(packet.fnCode)
  {
  case 0x03: //Read holding registers

    //Check for multiple read boundary check
    if(packet.mbAddr + packet.quantity > UINT16_MAX){
      server->response[0] = server->sAddr;
      server->response[1] = 128 + packet.fnCode;

      //Response error: Illegal data address
      server->response[2] = 0x02;

      uint16_t crc = modbusGetCRC16(server->response, 3);
      server->response[3] = (uint8_t)(crc >> 8);
      server->response[4] = (uint8_t)(crc);

      // return the size of the response packet
      return 5;
    }

    server->response[0] = server->sAddr;
    server->response[1] = packet.fnCode;
    server->response[2] = 2 * packet.quantity;
    size = 3;

    for(uint16_t i = 0, j = 0; j < packet.quantity; i += 2, j++){
      uint16_t dummy = getParamValByAddress(server, packet.mbAddr + j);
      server->response[3 + i]     = (uint8_t)(dummy >> 8);
      server->response[3 + i + 1] = (uint8_t)(dummy);
      size += 2;
    }

    crc = modbusGetCRC16(server->response, size);
    server->response[size + 1] = (uint8_t)(crc >> 8);
    server->response[size + 0] = (uint8_t)(crc);

    // return the size of the response packet
    return (size + 2);

    break;
  case 0x06: //Write single register

    index = modbusServergetIndex(server, packet.mbAddr);

    // Check for read access level of user
    if(index == UINT16_MAX || server->userLevel < server->modbusRegisters[index].accessLevel){
      server->response[0] = server->sAddr;
      server->response[1] = 128 + packet.fnCode;

      //Response error: Illegal function
      server->response[2] = 0x01;

      uint16_t crc = modbusGetCRC16(server->response, 3);
      server->response[3] = (uint8_t)(crc >> 8);
      server->response[4] = (uint8_t)(crc);

      // return the size of the response packet
      return 5;
    }

    //Boundary checks
    if(setParamValByAddress(server, packet.mbAddr, (server->reqPacket[4] << 8) | server->reqPacket[5])){
      server->response[0] = server->sAddr;
      server->response[1] = 128 + packet.fnCode;

      //Illegal Data Value
      server->response[2] = 0x03;

      crc = modbusGetCRC16(server->response, 3);
      server->response[4] = (uint8_t)(crc >> 8);
      server->response[3] = (uint8_t)(crc);

      // return the size of the response packet
      return 5;
    }

    // Save parameter to static memory
    if(server->modbusRegisters[index].saveEnable){
      server->saveParam(index, *server->modbusRegisters[index].value);
    }

    server->response[0] = server->sAddr;
    server->response[1] = packet.fnCode;
    server->response[2] = server->reqPacket[2];
    server->response[3] = server->reqPacket[3];

    server->response[4] = server->reqPacket[4];
    server->response[5] = server->reqPacket[5];

    crc = modbusGetCRC16(server->response, 6);
    server->response[7] = (uint8_t)(crc >> 8);
    server->response[6] = (uint8_t)(crc);

    // Try to reach callback function of that register
    if(server->modbusRegisters[index].callbackFnPointer != NULL){
        server->modbusRegisters[index].callbackFnPointer();
    }

    // Send write OK response
    // return the size of the response packet
    return 8;
    break;

  case 0x10: //Write multiple registers
    //
    //            response[0] = server->sAddr;
    //
    //            //Check for multiple write boundary check
    //            if(packet.mbAddr + packet.quantity > server->MB_REG_COUNT){
    //                server->response[1] = 128 + packet.fnCode;
    //
    //                //Response error: Illegal data address
    //                server->response[2] = 0x02;
    //
    //                uint16_t crc = modbusGetCRC16(response, 3);
    //                server->response[3] = (uint8_t)(crc >> 8);
    //                server->response[4] = (uint8_t)(crc);
    //
    //                // return the size of the response packet
    //                return 5;
    //            }
    //
    //            server->response[1] = packet.fnCode;
    //            server->response[2] = server->reqPacket[2];
    //            server->response[3] = server->reqPacket[3];
    //
    //            server->response[4] = server->reqPacket[4];
    //            server->response[5] = server->reqPacket[5];
    //
    //            crc = modbusGetCRC16(response, 6);
    //            server->response[7] = (uint8_t)(crc >> 8);
    //            server->response[6] = (uint8_t)(crc);
    //
    break;

  default:
    server->response[0] = server->sAddr;
    server->response[1] = 128 + packet.fnCode;

    //Illegal function
    server->response[2] = 0x01;

    crc = modbusGetCRC16(server->response, 3);
    server->response[4] = (uint8_t)(crc >> 8);
    server->response[3] = (uint8_t)(crc);

    // return the size of the response packet
    return 5;
    break;
  }
  return 0;
}

/**
 * @brief inline function to parse incoming packet
 * 
 * @return modbusPacket_t packet object that has required attributes.
 */
modbusPacket_t modbusObjectParse(
    const modbusServerObject_t* server //Server object to fill in. This should be created first.
){
  modbusPacket_t packet;
  switch(server->serverType)
  {
  case Server_RTU:
    packet.sAddr    = server->reqPacket[0];
    packet.fnCode   = server->reqPacket[1];
    packet.mbAddr   = ((uint16_t)(server->reqPacket[2] << 8) | server->reqPacket[3]);
    packet.quantity = ((uint16_t)(server->reqPacket[4] << 8) | server->reqPacket[5]);
    uint16_t crc = (server->reqPacket[7] << 8);
    packet.crc      = crc | server->reqPacket[6];
    break;
  case Server_ASCII:
    break;
  case Server_TCP:
    break;
  case Server_ERR:
    break;
  default:
    break;
  }
  return packet;
}

/**
 * @brief Set the Param Val By Address object
 * @note This function does check boundary conditions.
 * @return uint8_t returns 0 if write is successfull and 1 if not.
 */
uint8_t setParamValByAddress(
    modbusServerObject_t* server, //Server object to fill in. This should be created first.
    uint16_t mbAddr, //modbus address to change actual value
    int16_t val //value to set
){
  uint16_t index = modbusServergetIndex(server, mbAddr);
  if(index == UINT16_MAX) return 1;
  
//  // Password security check - only allow password changes from appropriate access levels
//  switch(mbAddr) {
//    case DEVICE_ENKO_PASSWORD:
//      if(server->userLevel < LEVEL_4) return 1; // Access denied
//      break;
//    case DEVICE_FACTORY_PASSWORD:
//      if(server->userLevel < LEVEL_3) return 1; // Access denied
//      break;
//    case DEVICE_SERVICE_PASSWORD:
//      if(server->userLevel < LEVEL_2) return 1; // Access denied
//      break;
//    case DEVICE_USER_PASSWORD:
//      if(server->userLevel < LEVEL_1) return 1; // Access denied
//      break;
//    default:
//      break;
//  }
  
  if(val <= server->modbusRegisters[index].MaxValue){
    if(val >= server->modbusRegisters[index].MinValue){
      if(server->modbusRegisters[index].value != NULL) {
        *server->modbusRegisters[index].value = val;
        return 0;
      }
    }
  }
  return 1;
}


/**
 * @brief Sets the value of the given modbus register without boundary checking.
 * First consider not using it if possible.
 */
uint8_t setParamValByAddress_unsafe(
    modbusServerObject_t* server, //Server object to fill in. This should be created first.
    uint16_t mbAddr, //modbus address to change actual value
    int16_t val //value to set
){
  uint16_t index = modbusServergetIndex(server, mbAddr);
  if(index == UINT16_MAX) return 1;
  if(server->modbusRegisters[index].value == NULL) return 1;
  *server->modbusRegisters[index].value = val;
  return 0;
}

/**
 * @brief Get the Param Val By Address object.
 * @note This function does not check for access level.
 */
int16_t getParamValByAddress(
    modbusServerObject_t* server, //Server object to fill in. This should be created first.
    uint16_t mbAddr // Modbus address to check for value.
){
  uint16_t index = modbusServergetIndex(server, mbAddr);
  if(index == UINT16_MAX) return INT16_MAX;
  if(server->modbusRegisters[index].value == NULL) return 1;
  
  // Password security check - hide password values based on user level
  switch(mbAddr) {
    case DEVICE_ENKO_PASSWORD:
      if(server->userLevel < LEVEL_4) return 0;
      break;
    case DEVICE_FACTORY_PASSWORD:
      if(server->userLevel < LEVEL_3) return 0;
      break;
    case DEVICE_SERVICE_PASSWORD:
      if(server->userLevel < LEVEL_2) return 0;
      break;
    case DEVICE_USER_PASSWORD:
      if(server->userLevel < LEVEL_1) return 0;
      break;
    default:
      break;
  }
  
  return *server->modbusRegisters[index].value;
}

/**
 * @brief Get the Param Min By Address object.
 * @note This function does not check for access level.
 */
int16_t getParamMinByAddress(
    modbusServerObject_t* server, //Server object to fill in. This should be created first.
    uint16_t mbAddr               // Modbus address to check for value.
){
  uint16_t index = modbusServergetIndex(server, mbAddr);
  if(index == UINT16_MAX) return INT16_MAX;
  return server->modbusRegisters[index].MinValue;
}

/**
 * @brief Get the Param Max By Address object.
 * @note This function does not check for access level.
 */
int16_t getParamMaxByAddress(
    modbusServerObject_t* server, //Server object to fill in. This should be created first.
    uint16_t mbAddr               // Modbus address to check for value.
){
  uint16_t index = modbusServergetIndex(server, mbAddr);
  if(index == UINT16_MAX) return INT16_MAX;
  return server->modbusRegisters[index].MaxValue;
}

/**
 * @brief Get the Param default By Address object.
 * @note This function does not check for access level.
 * @param server  Server object to fill in. This should be created first.
 * @param mbAddr  Modbus address to check for value.
 */
int16_t getParamDefaultByAddress(modbusServerObject_t* server, uint16_t mbAddr){
  uint16_t index = modbusServergetIndex(server, mbAddr);
  if(index == UINT16_MAX) return INT16_MAX;
  return server->modbusRegisters[index].Default;
}

/**
 * @brief inline function. Calculates modbus CRC 16 for parameter array.
 * @note This function was taken from NY modbus lib.
 * 
 * @param packet    Array to calculate CRC for
 * @param size      Array size to iterate through
 * @return uint16_t CRC16 for the array.
 */
uint16_t modbusGetCRC16(uint8_t *packet, uint32_t size){
  static const uint16_t wCRCTable[] = {
      0X0000, 0XC0C1, 0XC181, 0X0140, 0XC301, 0X03C0, 0X0280, 0XC241,
      0XC601, 0X06C0, 0X0780, 0XC741, 0X0500, 0XC5C1, 0XC481, 0X0440,
      0XCC01, 0X0CC0, 0X0D80, 0XCD41, 0X0F00, 0XCFC1, 0XCE81, 0X0E40,
      0X0A00, 0XCAC1, 0XCB81, 0X0B40, 0XC901, 0X09C0, 0X0880, 0XC841,
      0XD801, 0X18C0, 0X1980, 0XD941, 0X1B00, 0XDBC1, 0XDA81, 0X1A40,
      0X1E00, 0XDEC1, 0XDF81, 0X1F40, 0XDD01, 0X1DC0, 0X1C80, 0XDC41,
      0X1400, 0XD4C1, 0XD581, 0X1540, 0XD701, 0X17C0, 0X1680, 0XD641,
      0XD201, 0X12C0, 0X1380, 0XD341, 0X1100, 0XD1C1, 0XD081, 0X1040,
      0XF001, 0X30C0, 0X3180, 0XF141, 0X3300, 0XF3C1, 0XF281, 0X3240,
      0X3600, 0XF6C1, 0XF781, 0X3740, 0XF501, 0X35C0, 0X3480, 0XF441,
      0X3C00, 0XFCC1, 0XFD81, 0X3D40, 0XFF01, 0X3FC0, 0X3E80, 0XFE41,
      0XFA01, 0X3AC0, 0X3B80, 0XFB41, 0X3900, 0XF9C1, 0XF881, 0X3840,
      0X2800, 0XE8C1, 0XE981, 0X2940, 0XEB01, 0X2BC0, 0X2A80, 0XEA41,
      0XEE01, 0X2EC0, 0X2F80, 0XEF41, 0X2D00, 0XEDC1, 0XEC81, 0X2C40,
      0XE401, 0X24C0, 0X2580, 0XE541, 0X2700, 0XE7C1, 0XE681, 0X2640,
      0X2200, 0XE2C1, 0XE381, 0X2340, 0XE101, 0X21C0, 0X2080, 0XE041,
      0XA001, 0X60C0, 0X6180, 0XA141, 0X6300, 0XA3C1, 0XA281, 0X6240,
      0X6600, 0XA6C1, 0XA781, 0X6740, 0XA501, 0X65C0, 0X6480, 0XA441,
      0X6C00, 0XACC1, 0XAD81, 0X6D40, 0XAF01, 0X6FC0, 0X6E80, 0XAE41,
      0XAA01, 0X6AC0, 0X6B80, 0XAB41, 0X6900, 0XA9C1, 0XA881, 0X6840,
      0X7800, 0XB8C1, 0XB981, 0X7940, 0XBB01, 0X7BC0, 0X7A80, 0XBA41,
      0XBE01, 0X7EC0, 0X7F80, 0XBF41, 0X7D00, 0XBDC1, 0XBC81, 0X7C40,
      0XB401, 0X74C0, 0X7580, 0XB541, 0X7700, 0XB7C1, 0XB681, 0X7640,
      0X7200, 0XB2C1, 0XB381, 0X7340, 0XB101, 0X71C0, 0X7080, 0XB041,
      0X5000, 0X90C1, 0X9181, 0X5140, 0X9301, 0X53C0, 0X5280, 0X9241,
      0X9601, 0X56C0, 0X5780, 0X9741, 0X5500, 0X95C1, 0X9481, 0X5440,
      0X9C01, 0X5CC0, 0X5D80, 0X9D41, 0X5F00, 0X9FC1, 0X9E81, 0X5E40,
      0X5A00, 0X9AC1, 0X9B81, 0X5B40, 0X9901, 0X59C0, 0X5880, 0X9841,
      0X8801, 0X48C0, 0X4980, 0X8941, 0X4B00, 0X8BC1, 0X8A81, 0X4A40,
      0X4E00, 0X8EC1, 0X8F81, 0X4F40, 0X8D01, 0X4DC0, 0X4C80, 0X8C41,
      0X4400, 0X84C1, 0X8581, 0X4540, 0X8701, 0X47C0, 0X4680, 0X8641,
      0X8201, 0X42C0, 0X4380, 0X8341, 0X4100, 0X81C1, 0X8081, 0X4040 };

  uint8_t nTemp;
  uint16_t wCRCWord = 0xFFFF;
  while (size--){
    nTemp = *packet++ ^ wCRCWord;
    wCRCWord >>= 8;
    wCRCWord ^= wCRCTable[nTemp];
  }
  return wCRCWord;
}

/**
 * @brief mapping function to convert register address to register index for size considerations
 */
uint16_t modbusServergetIndex(modbusServerObject_t *server, uint16_t mbAddr){

  // If group is not present
  uint16_t a = mbAddr / GROUP_SIZE;
  if(server->groups[a].ptr == NULL) return UINT16_MAX;

  // If group is smaller then requested mbAddr
  uint16_t b = mbAddr % GROUP_SIZE;
  if(b > server->groups[a].max) return UINT16_MAX;

  return *(server->groups[a].ptr + b);
}

/**
 * @brief updates loginTimer of the modbus server object
 * @note should be called every ms
 * @param server modbus server object to update
 */
void modbusServerUpdateTimer(modbusServerObject_t *server){
  // Disable logout if set to 0
  if(server->loginTimerMaxCount == 0) return;
  // Increase timer to logout after inactivity
  server->loginTimer += 1;
    if(server->loginTimer >= server->loginTimerMaxCount){
      server->userLevel = LEVEL_0;
      server->loginTimer = 0;
      
      // Note: Password hiding is automatically handled in getParamValByAddress
      // based on the userLevel change to LEVEL_0
    }
}
