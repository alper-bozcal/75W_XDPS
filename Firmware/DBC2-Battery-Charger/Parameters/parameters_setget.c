/*
 * parameters.c
 *
 *  Created on: 14 Nis 2016
 *      Author: NAZIM YILDIZ
 */

#include "parameters_setget.h"
#include "Flash.h"

#define WRITE_TO_EXTERNAL

int16_t data[PARAM_MAX_COUNT];


extern const ParameterList_t ParamList[PARAM_LAST_INDEX];
extern HAL_StatusTypeDef Flash_writeReferencesToFlash(void);

int8_t setParamDataViaPN(uint16_t param_no, int16_t value, int8_t CALLBACK_X){

	static uint16_t buf_paramno;

	if(param_no >= PARAM_LAST_INDEX) return -1;

	if((ParamList[param_no - 1].min <= value) && (ParamList[param_no - 1].max >= value)){
		data[param_no - 1] = value;
	}
	if(getSaveActionViaPN(param_no) == SAVE_ENABLE){
		Flash_writeReferencesToFlash();
	}

	buf_paramno = param_no;

	if(CALLBACK_X == CALLBACK_ON) ParamList[param_no-1].FuncCallback(&buf_paramno);

	return 0;
}

int16_t getParamDataViaPN(uint16_t param_no){
	return data[param_no - 1];
}


int16_t *getParamDataAdr(uint16_t param_no){
	return &data[param_no - 1];
}

int16_t getParamMinViaPN(uint16_t param_no){

	return ParamList[param_no - 1].min;
}


int16_t getParamMaxViaPN(uint16_t param_no){

	return ParamList[param_no-1].max;
}


int16_t getParamDefViaPN(uint16_t param_no){
	return ParamList[param_no-1].def;
}

int16_t getParamCoefViaPN(uint16_t param_no){
	
	return ParamList[param_no-1].coef;	

}

Callback getParamCallbackFunc(uint16_t param_no){

	return ParamList[param_no - 1].FuncCallback;
}



int16_t checkAccessLevelRead(uint16_t param_no, uint8_t ACCESS_LEVELx){
	if((ParamList[param_no-1].access_level & 0xF0) <= ACCESS_LEVELx){
		return 1;
	}
	else
		return -1;
}

int16_t checkAccessLevelWrite(uint16_t param_no, uint8_t ACCESS_LEVELx){
	if((ParamList[param_no-1].access_level & 0x0F) <= ACCESS_LEVELx){
		return 1;
	}
	else
		return -1;
}

SaveDE_e getSaveActionViaPN(uint16_t param_no){
	return ParamList[param_no-1].save_type;
}



void loadDefaults(uint8_t ACCESS_LEVELx){

}

