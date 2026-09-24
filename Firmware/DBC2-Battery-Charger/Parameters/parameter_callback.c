/*
 * parameter_callback.c
 *
 *  Created on: 2 Jan 2017
 *      Author: Nazim Yildiz
 */


#include "parameters_setget.h"

#include "parameter_callback.h"
#include "main.h"
#include "Controller.h"
#include "modbus_adapter_pdu.h"
#include "../DynamicPasswordAdapter/dynamic_pass_adapter.h"

extern ControllerParam_t outputReferences;
extern uint32_t ledTimeBase;
extern uint16_t ledColorPin;
extern int16_t parallelCurrentCoefficient;
extern HAL_StatusTypeDef Flash_writeReferencesToFlash(void);
extern uint32_t LED_OK2;
void clearLED(void);
void CallbackNull(void *param){}

void CallbackInit(){}

void callBackUpdateVoltageREF(void *param){

	if(MIN_VOLTAGE > getParamDataViaPN(OUTPUT_VOLTAGE) || getParamDataViaPN(OUTPUT_VOLTAGE) > MAX_VOLTAGE ){
		setParamDataViaPN(OUTPUT_VOLTAGE, MIN_VOLTAGE, CALLBACK_OFF);
		setParamDataViaPN(OUTPUT_MODEL, 0000, CALLBACK_ON);
		return;
	}
	outputReferences.settedREFVoltage = outputReferences.nonParallelREFVoltage = getParamDataViaPN(OUTPUT_VOLTAGE);
	setVoltagePWMtoRef(&outputReferences);
	setParamDataViaPN(PWMFLAG, 0, CALLBACK_OFF);

	if(getParamDataViaPN(OUTPUT_VOLTAGE) * getParamDataViaPN(OUTPUT_CURRENT) > POWER){
		setParamDataViaPN(OUTPUT_CURRENT, 500, CALLBACK_OFF);
		outputReferences.REFCurrent = getParamDataViaPN(OUTPUT_CURRENT);
		setCurrentPWMtoRef(&outputReferences);
	}
	LED_OK2 = 1;

	setParamDataViaPN(OUTPUT_MODEL, 0000, CALLBACK_ON);
}

void callBackUpdateCurrentREF(void *param){

	if(getParamDataViaPN(OUTPUT_CURRENT) > MAX_CURRENT){
		setParamDataViaPN(OUTPUT_CURRENT, outputReferences.REFCurrent, CALLBACK_OFF);
		setParamDataViaPN(OUTPUT_MODEL, 0000, CALLBACK_ON);
		return;
	}
	outputReferences.REFCurrent = getParamDataViaPN(OUTPUT_CURRENT);
	setCurrentPWMtoRef(&outputReferences);
	setParamDataViaPN(PWMFLAG, 0, CALLBACK_OFF);

	if(getParamDataViaPN(OUTPUT_VOLTAGE) * getParamDataViaPN(OUTPUT_CURRENT) > POWER){
		setParamDataViaPN(OUTPUT_VOLTAGE, MIN_VOLTAGE, CALLBACK_OFF);
		setVoltagePWMtoRef(&outputReferences);
	}

	setParamDataViaPN(OUTPUT_MODEL, 0000, CALLBACK_ON);
}

void CallbackModel(void* param){

    //Check device type and set for analog

    if(getParamDataViaPN(OUTPUT_VOLTAGE) <= 1600){

#ifdef DBC_2_150W
    	//This GPIO controls the output regulation zeners directly.
    	HAL_GPIO_WritePin(OVER_VOLT_GPIO_Port, OVER_VOLT_Pin, GPIO_PIN_SET);
#endif
    	if(getParamDataViaPN(OUTPUT_CURRENT) <= 500){
    		setParamDataViaPN(OUTPUT_MODEL, 1205, CALLBACK_OFF);
    	}else if(getParamDataViaPN(OUTPUT_CURRENT) > 500 && getParamDataViaPN(OUTPUT_CURRENT) <= 1000){
    		setParamDataViaPN(OUTPUT_MODEL, 1210, CALLBACK_OFF);
    	}else{
    		setParamDataViaPN(OUTPUT_MODEL, 1200, CALLBACK_OFF);
    	}
    }else if(getParamDataViaPN(OUTPUT_VOLTAGE) > 1600 && getParamDataViaPN(OUTPUT_VOLTAGE) <= 3000){
    	if(getParamDataViaPN(OUTPUT_CURRENT) <= 500){
    		setParamDataViaPN(OUTPUT_MODEL, 2405, CALLBACK_OFF);
    	}else if(getParamDataViaPN(OUTPUT_CURRENT) > 500 && getParamDataViaPN(OUTPUT_CURRENT) <= 1000){
    		setParamDataViaPN(OUTPUT_MODEL, 2410, CALLBACK_OFF);
    	}else{
    		setParamDataViaPN(OUTPUT_MODEL, 2400, CALLBACK_OFF);
    	}
    }
}

void CallbackUpdateVPWM(void *param){

	Voltage_PWM = getParamDataViaPN(VOUTPWM);
	updateVPWMLimits();
	setParamDataViaPN(PWMFLAG, 1, CALLBACK_OFF);
	Flash_writeReferencesToFlash();

}

void CallbackUpdateIPWM(void *param){

	Current_PWM = getParamDataViaPN(IOUTPWM);
	updateIPWMLimits();
	setParamDataViaPN(PWMFLAG, 1, CALLBACK_OFF);
	Flash_writeReferencesToFlash();
}

void CallbackDeviceReset(void* param){
	HAL_NVIC_SystemReset();
}

void CallbackUpdateVoltageBoost(void *param){
	if( MIN_VOLTAGE > getParamDataViaPN(OUTPUT_BOOST_VOLTAGE) || getParamDataViaPN(OUTPUT_BOOST_VOLTAGE) > MAX_VOLTAGE){
		setParamDataViaPN(OUTPUT_BOOST_VOLTAGE, MIN_VOLTAGE, CALLBACK_OFF);
		return;
	}
	outputReferences.boostVoltage = getParamDataViaPN(OUTPUT_BOOST_VOLTAGE);
	setParamDataViaPN(PWMFLAG, 0, CALLBACK_OFF);
}

void CallbackDebugParam(void *param){
}

void CallbackTestMode(void* param){
	//Till test mode starts, jig should read monitor params from modbus and calculate the calib and offset params
	//When jig triggers test mode it should be ready to send calib and offset params
	//maybe serial number checking should be implemented in test mode, or jig just recognizes from qr?
//	if(getParamDataViaPN(TEST_MODE_ENABLE) == 1){}
}

void CallbackParallelWorking(void* param){

	if(getParamDataViaPN(PARALLEL_WORKING) == 1){
		outputReferences.nonParallelREFVoltage = getParamDataViaPN(OUTPUT_VOLTAGE);
	}else if(getParamDataViaPN(PARALLEL_WORKING) == 0){
		setParamDataViaPN(OUTPUT_VOLTAGE, outputReferences.nonParallelREFVoltage, CALLBACK_OFF);
	}
//	Flash_writeReferencesToFlash();
}

void CallbackUpdateParallelWorkingCoeff(void* param){
	//parallelCurrentCoefficient = getParamDataViaPN(PARALLEL_CURRENT_COEFF);
}

void CallbackRelay(void* param){
	if(getParamDataViaPN(OUTPUT_RELAY) == 0){
		setRelayOutput(RELAY_NOT);
	}else if(getParamDataViaPN(OUTPUT_RELAY) == 1){
		setRelayOutput(RELAY_CONDUCTING);
	}
}

void CallbackCalibration(void *arg){

/*
	int16_t tmp = getParamDataViaPN(*(uint16_t*)arg);
	switch(*(uint16_t*)arg){
	case MEASURE_CALIB_VOUT:
		CALIBRATION.Vout = tmp;
		break;
	case MEASURE_CALIB_IOUT:
		CALIBRATION.Iout = tmp;
		break;
	case MEASURE_CALIB_PT100:
		CALIBRATION.RPT100 = tmp;
		break;
	case MEASURE_CALIB_NTC:
		CALIBRATION.ntcC = tmp;
		break;
	case MEASURE_CALIB_VBAT:
		CALIBRATION.Vbat = tmp;
		break;
	case MEASURE_OFF_VOUT:
		OFFSET.Vout = tmp;
		break;
	case MEASURE_OFF_IOUT:
		OFFSET.Iout = tmp;
		break;
	case MEASURE_OFF_PT100:
		OFFSET.RPT100 = tmp;
		break;
	case MEASURE_OFF_NTC:
		OFFSET.ntcC = tmp;
		break;
	case MEASURE_OFF_VBAT:
		OFFSET.Vbat = tmp;
		break;
	default:
		break;
	}
	*/
//	Flash_writeReferencesToFlash();
}

void CallbackLEDControl(void *arg){
	clearLED();
	LED_OK2 = 0;
}

void CallbackTempControl(void *arg){
	if(*(uint16_t*)arg == TEMP_DERATE_NTC){
		//ntcDerateTemp = getParamDataViaPN(TEMP_DERATE_NTC);
	}else if(*(uint16_t*)arg == TEMP_DERATE_PT100){
		//pt100DerateTemp = getParamDataViaPN(TEMP_DERATE_PT100);
	}else{}
}

void CallbackDeviceUpdate(void *arg){
//	Flash_writeReferencesToFlash();
}

void CallbackReverse(void *param){

}

extern uint8_t tagCount;
extern int16_t wtf[TAG_SIZE];
void CallbackSaveTag(void *param){

	wtf[tagCount] = getParamDataViaPN(DEBUG_TAG);
	tagCount++;
	if(tagCount == TAG_SIZE){
		//Save routine
		Flash_writeReferencesToFlash();
		tagCount = 0;
	}
}

void CallbackFactoryReset(void* param){
	if(getParamDataViaPN(DEBUG_RETURN_FACTORY) != 1)return;

	for(uint16_t i = 0; i < PARAM_LAST_INDEX; i++){
		*getParamDataAdr(i) = getParamDefViaPN(i);
	}
	Flash_writeReferencesToFlash();
	setParamDataViaPN(DEVICE_RESET, 1, CALLBACK_ON);
}

void CallbackLoginEnko(void *arg){

	int16_t security1, security2;
	int16_t pass1, pass2;

	security1 = getParamDataViaPN(LOGIN_SECURITY_ENKO_REG1);
	security2 = getParamDataViaPN(LOGIN_SECURITY_ENKO_REG2);

	pass1 = security1 ^ (getParamDataViaPN(DEVICE_IDENKO_REG1) + getParamDataViaPN(DEVICE_IDENKO_REG3));
	pass2 = security2 ^ (getParamDataViaPN(DEVICE_IDENKO_REG2) + getParamDataViaPN(DEVICE_IDENKO_REG3));

	if(pass1 == getParamDataViaPN(LOGIN_PASSWORD_ENTRY_ENKO_REG1) &&
			pass2 == getParamDataViaPN(LOGIN_PASSWORD_ENTRY_ENKO_REG2)){

		setModbusPDUAccessLevel(MODBUS_PDU_ACCESS_ENKO);
		clearModbusPDUAccessTimeoutTick();

		// Login icin girilmis password alanlarini sifirlayalim,
		// boylece her iki password alaninin tekrar yazilmasi zorunlu olacak, GUVENLIK acigi giderilmis oldu.
		setParamDataViaPN(LOGIN_PASSWORD_ENTRY_ENKO_REG1, 0, CALLBACK_OFF);
		setParamDataViaPN(LOGIN_PASSWORD_ENTRY_ENKO_REG2, 0, CALLBACK_OFF);

		// Dinamik sifre uretilsin
		runDynamicPassword(getDynamicPasswordObj());
	}
	else{

		setModbusPDUAccessLevel(MODBUS_PDU_ACCESS_NONE);
	}
}

void CallbackOptionModule(void *arg){
}

void CallbackLoginCustomer(void *arg){

	int16_t security1, security2;
	int16_t pass1, pass2;

	security1 = getParamDataViaPN(LOGIN_SECURITY_CUSTOMER_REG1);
	security2 = getParamDataViaPN(LOGIN_SECURITY_CUSTOMER_REG2);

	pass1 = security1 ^ (getParamDataViaPN(DEVICE_IDENKO_REG1) - getParamDataViaPN(DEVICE_IDENKO_REG3));
	pass2 = security2 ^ (getParamDataViaPN(DEVICE_IDENKO_REG2) - getParamDataViaPN(DEVICE_IDENKO_REG3));

	if(pass1 == getParamDataViaPN(LOGIN_PASSWORD_ENTRY_CUSTOMER_REG1) &&
			pass2 == getParamDataViaPN(LOGIN_PASSWORD_ENTRY_CUSTOMER_REG2)){

		setModbusPDUAccessLevel(MODBUS_PDU_ACCESS_CUSTOMER);
		clearModbusPDUAccessTimeoutTick();

		// Login icin girilmis password alanlarini sifirlayalim,
		// boylece her iki password alaninin tekrar yazilmasi zorunlu olacak, GUVENLIK acigi giderilmis oldu.
		setParamDataViaPN(LOGIN_PASSWORD_ENTRY_CUSTOMER_REG1, 0, CALLBACK_OFF);
		setParamDataViaPN(LOGIN_PASSWORD_ENTRY_CUSTOMER_REG2, 0, CALLBACK_OFF);

		// Dinamik sifre uretilsin
		//runDynamicPassword(getDynamicPasswordObj());
	}
	else{

		setModbusPDUAccessLevel(MODBUS_PDU_ACCESS_NONE);
	}
}

void CallbackDynamicPass(){

	if(getParamDataViaPN(GENERAL_DYNAMIC_PASS_TYPE) == 1){

		setDynamicPasswordKeyValue(getDynamicPasswordObj(), 2783);
	}
	else if(getParamDataViaPN(GENERAL_DYNAMIC_PASS_TYPE) == 2){

		setDynamicPasswordKeyValue(getDynamicPasswordObj(), 1543);
	}
	else if(getParamDataViaPN(GENERAL_DYNAMIC_PASS_TYPE) == 3){

		setDynamicPasswordKeyValue(getDynamicPasswordObj(), 1564);
	}
	else if(getParamDataViaPN(GENERAL_DYNAMIC_PASS_TYPE) == 4){

		setDynamicPasswordKeyValue(getDynamicPasswordObj(), 1571);
	}
	else if(getParamDataViaPN(GENERAL_DYNAMIC_PASS_TYPE) == 5){

		setDynamicPasswordKeyValue(getDynamicPasswordObj(), 1643);
	}
	else if(getParamDataViaPN(GENERAL_DYNAMIC_PASS_TYPE) == 6){

		setDynamicPasswordKeyValue(getDynamicPasswordObj(), 1777);
	}
	else if(getParamDataViaPN(GENERAL_DYNAMIC_PASS_TYPE) == 7){

		setDynamicPasswordKeyValue(getDynamicPasswordObj(), 1775);
	}
	else if(getParamDataViaPN(GENERAL_DYNAMIC_PASS_TYPE) == 8){

		setDynamicPasswordKeyValue(getDynamicPasswordObj(), 1804);
	}
	else if(getParamDataViaPN(GENERAL_DYNAMIC_PASS_TYPE) == 9){

		setDynamicPasswordKeyValue(getDynamicPasswordObj(), 1791);
	}
	else if(getParamDataViaPN(GENERAL_DYNAMIC_PASS_TYPE) == 10){

		setDynamicPasswordKeyValue(getDynamicPasswordObj(), 1861);
	}
	else{

		setDynamicPasswordKeyValue(getDynamicPasswordObj(), 3006);
	}
}

void CallbackCutoff(void* param){
	//Should be called for every set of CUT OFF Mosfet
	if(getParamDataViaPN(OUTPUT_CUT_OFF) == CONDUCTING){//MOSFET on Conduction
		if(HAL_GPIO_ReadPin(CUT_OFF_OUTPUT_GPIO_Port, CUT_OFF_OUTPUT_Pin) == GPIO_PIN_SET) HAL_GPIO_WritePin(CUT_OFF_OUTPUT_GPIO_Port, CUT_OFF_OUTPUT_Pin, GPIO_PIN_RESET);
		//HAL_GPIO_WritePin(CUT_OFF_OUTPUT_GPIO_Port, CUT_OFF_OUTPUT_Pin, GPIO_PIN_RESET);

	}else if(getParamDataViaPN(OUTPUT_CUT_OFF) == NOT_CONDUCTING){//MOSFET not conducting
		if(HAL_GPIO_ReadPin(CUT_OFF_OUTPUT_GPIO_Port, CUT_OFF_OUTPUT_Pin) == GPIO_PIN_SET) HAL_GPIO_WritePin(CUT_OFF_OUTPUT_GPIO_Port, CUT_OFF_OUTPUT_Pin, GPIO_PIN_RESET);
		//HAL_GPIO_WritePin(CUT_OFF_OUTPUT_GPIO_Port, CUT_OFF_OUTPUT_Pin, GPIO_PIN_SET);

	}else{

	}
}



