/*
 * modbus_adapter_pdu.h
 *
 *  Created on: 15Jan.,2019
 *      Author: EN
 */

#ifndef MODBUS_ADAPTER_PDU_H_
#define MODBUS_ADAPTER_PDU_H_

#include "stdint.h"
#include "msserial.h"

typedef enum{

	MODBUS_PDU_ACCESS_NONE = 0,
	MODBUS_PDU_ACCESS_CUSTOMER,
	MODBUS_PDU_ACCESS_ENKO,
}ModbusPDUAccess_e;

// Modbus icin uygulama katmani fonksiyonlari/driverlari
int8_t ModbusPDUAdapterWrite(uint16_t address, int8_t *value, uint16_t len_register);
int16_t ModbusPDUAdapterRead(uint16_t param_adr);
int8_t ModbusPDUAdressCheck(uint16_t address, uint16_t FN_CODE_x);

void modbusPDUAccessTimeoutTickUpdate(uint32_t tick_step);
void setModbusPDUAccessTimeoutParam(uint32_t tick);
void setModbusPDUAccessLevel(ModbusPDUAccess_e MODBUS_PDU_ACCESS_x);
ModbusPDUAccess_e getModbusPDUAccessLevel();
void clearModbusPDUAccessTimeoutTick();

#endif /* MODBUS_ADAPTER_PDU_H_ */
