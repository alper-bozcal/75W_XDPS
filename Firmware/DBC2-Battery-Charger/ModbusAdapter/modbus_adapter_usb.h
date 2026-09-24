/*
 * modbus_adapter_usb.h
 *
 *  Created on: 30 Jan 2017
 *      Author: Nazim Yildiz
 */

#ifndef MODBUS_ADAPTER_USB_H_
#define MODBUS_ADAPTER_USB_H_

#include "stdint.h"
//#include "usbd_cdc.h"
//#include "usbd_cdc_if.h"

void ModbusAdapterUsbInit();
int8_t ModbusAdapterTransmitterUsb(int8_t *buffer, uint16_t size);
uint16_t ModbusAdapterReceiverUsb(int8_t *buffer, uint16_t size);
int8_t ModbusAdapterReceiverStopUsb();
int8_t isModbusAdapterTransmitUSBDone();

#endif /* MODBUS_ADAPTER_USB_H_ */
