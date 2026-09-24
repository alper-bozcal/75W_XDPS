/*
 * Controller.h
 *
 *  Created on: 6 Eki 2022
 *      Author: ceyhun.uysal
 */
#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <parameter_definitions.h>
#include "main.h"
#include "Flash.h"
#include "parameters_setget.h"

//Power derator check values
#define POWER_DERATE_START_TEMP	93
#define POWER_DERATE_HIST_TEMP  90
#define POWER_DERATE_MAX_TEMP	105
//over voltage tick gets incremented on outputController. So, period of calling it is important for count value
// 32 * 128 = 4096ms
#define OVER_VOLTAGE_COUNT 32


//Fixed addresses of groups that should be keeped in memory
#define FLASH_POINTER_1		*(uint32_t*)0x0800F800
#define FLASH_POINTER_2		*(uint32_t*)0x0800F804
#define FLASH_POINTER_3		*(uint32_t*)0x0800F808
#define FLASH_POINTER_4		*(uint32_t*)0x0800F80C
#define FLASH_POINTER_5		*(uint32_t*)0x0800F810
#define FLASH_POINTER_6		*(uint32_t*)0x0800F814
#define FLASH_POINTER_7		*(uint32_t*)0x0800F818
#define FLASH_POINTER_8		*(uint32_t*)0x0800F81C
#define FLASH_POINTER_9		*(uint32_t*)0x0800F820
#define FLASH_POINTER_10	*(uint32_t*)0x0800F824
#define FLASH_POINTER_11	*(uint32_t*)0x0800F828
#define FLASH_POINTER_12	*(uint32_t*)0x0800F82C
#define FLASH_POINTER_13	*(uint32_t*)0x0800F830
#define FLASH_POINTER_14	*(uint32_t*)0x0800F834
#define FLASH_POINTER_15	*(uint32_t*)0x0800F838
#define FLASH_POINTER_16	*(uint32_t*)0x0800F83C


/**
 * @brief struct to used for controller. It is updated from modbus callbacks.
 * 
 */
typedef struct Controllerparam_s{
	//Reference voltage and current values multiplied by 100. It is used in outputcontroller.
	//int32_t REFVoltage;
	int32_t REFCurrent;
	//Reference pwm values they are used for monitoring
	int32_t dbgVPWM;
	int32_t dbgIPWM;
	//boost voltage reference
	int32_t boostVoltage;
	//It's been used to track non parallel mode voltage.
 	volatile int32_t nonParallelREFVoltage;
 	//to hold non-derated / non-compansated refvoltage
 	//It's been used to track latest voltage before boost mode.//! should I add this functionality too?
 	volatile int32_t settedREFVoltage;
 	//not derated ref current. Gets updated inside callback or powerDerator
 	volatile int32_t settedREFCurr;
}ControllerParam_t;

//errorHandler function input states, errorHandler should updated after update on here
typedef enum{
	ADCOpen,
	ADCOpen_false,

	outputReverse,
	outputReverse_false,

	overTempWarn,
	overTempWarn_false,

	overTempError,
	overTempError_false,

	PT100Connection,
	PT100Connection_false,

	overTempPT100,
	overTempPT100_false,

	overVoltageError,
	overVoltageError_false,

	wrongBattWarn,
	wrongBattWarn_false,
}errorType;

//Charging algorithm states. These are defined in Param list excel
typedef enum{
	S_PSU,
	S_CHARGER,
}ALG_STATES;

//Charger states are different then algorithm states
typedef enum{
	//On boot, OUTPUT_CHARGE_MODE params starts with value 0,
	//so if here gets changed, it should be updated too.
	BOOST = 0,
	EQUALIZATION = 1,
	FLOAT = 2,
}CHARGE_STATES;

//relay controller states
typedef enum{
	//off means relay is conducting.
	RELAY_CONDUCTING = 0,
	//On means relay is not conducting.
	RELAY_NOT = 1
}RELAY_STATE;

//return states for algorithm functions
typedef enum{
  STAT_FALSE = 0,
  STAT_TRUE  = 1,
  STAT_OK	 = 2,
}AlgorithmStatus;

//LED Color Controls
typedef enum{
	Red 	  = LED_R_Pin,
	Green 	= LED_G_Pin,
	Blue 	  = LED_B_Pin,
	White 	= LED_R_Pin | LED_G_Pin | LED_B_Pin,
	Purple	= LED_R_Pin | LED_B_Pin,
	Yellow	= LED_R_Pin | LED_G_Pin,
	Cyan	  = LED_G_Pin | LED_B_Pin,
	Black	  = 0,
}LEDColorType;
typedef enum{
	LED_CONS = 1,
	LED_NCONS = 0
}LEDStatus;

//control params for CUT OFF MOSFET
typedef enum{
	NOT_CONDUCTING = 0,
	CONDUCTING = 1,
}CUT_OFF_STATE;
/**
 * doesn't need typedef LedState
 * Because led state is controlled by just 2 uint32_t types 
 * and they live in main.c 
 */

void outputController(const MEASURED_t _measured);
void setVoltagePWM(const int16_t value);
void setCurrentPWM(const int16_t value);
uint8_t powerDerator(const MEASURED_t _measured);
void setVoltagePWMtoRef(const ControllerParam_t* _references);
void setCurrentPWMtoRef(const ControllerParam_t* _references);
void updateVPWMLimits(void);
void updateIPWMLimits(void);
void UpdateVoltageREFtoOld(void);
void UpdateVoltageREFToBoost(void);
void updateCurrentPWMtoPowerLimit(void);
void updateVoltagePWMtoPowerLimit(void);
void parallelWorking(const MEASURED_t _measured);
void setRelayOutput(const RELAY_STATE _stat);
RELAY_STATE getRelayState(void);
AlgorithmStatus isOutputReverse(const MEASURED_t _measured);
AlgorithmStatus isADCOpen(const MEASURED_t _measured);
AlgorithmStatus isOverVoltage(const MEASURED_t _measured);
AlgorithmStatus isOverCurrent(const MEASURED_t _measured);
AlgorithmStatus errorHandler(errorType _err);
AlgorithmStatus isPT100Connected(const MEASURED_t _measuredd);
void Charging(MEASURED_t _measured, uint8_t _boostLock, int16_t _PT100Comp);
uint32_t setLedColor(LEDColorType pin, LEDStatus _constant);
void clearLED(void);
void controlLEDbyParam(uint8_t _set);
void USBPinDown(uint32_t _d);
void RCC_APB2PeriphResetCmd(uint32_t RCC_APB2Periph, FunctionalState NewState);
void SYSCFG_MemoryRemapConfig(uint32_t SYSCFG_MemoryRemap);
void USBIncomingCallback(uint8_t* Buf, uint32_t *Len);
#endif //CONTROLLER_H
