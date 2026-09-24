/*
 * modbus_adapter_dma.h
 *
 *  Created on: 23 Jan 2017
 *      Author: Nazim Yildiz
 */

#ifndef MODBUS_ADAPTER_DMA_H_
#define MODBUS_ADAPTER_DMA_H_

#include "stdint.h"


/*
 * @brief         :
 * @param[]       :
 * @return        :
 * @precondition  :
 * @postcondition :
 */
void ModbusAdapterUartInit();
/*
 * @brief         :
 * @param[]       :
 * @return        :
 * @precondition  :
 * @postcondition :
 */
int8_t ModbusAdapterTransmitterDMA(int8_t *buffer, uint16_t size);
/*
 * @brief         :
 * @param[]       :
 * @return        :
 * @precondition  :
 * @postcondition :
 */
uint16_t ModbusAdapterReceiverDMA(int8_t *buffer, uint16_t size);
/*
 * @brief         :
 * @param[]       :
 * @return        :
 * @precondition  :
 * @postcondition :
 */
int8_t ModbusAdapterReceiverStopDMA();
/*
 * @brief         :
 * @param[]       :
 * @return        :
 * @precondition  :
 * @postcondition :
 */
int8_t isModbusAdapterTransmitDone();

#endif /* MODBUS_ADAPTER_DMA_H_ */
