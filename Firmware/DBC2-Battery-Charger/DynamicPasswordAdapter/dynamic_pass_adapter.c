#include "dynamic_pass_adapter.h"
#include "dynamic_pass.h"
#include "stm32f0xx_hal.h"
#include "parameters_setget.h"

static uint32_t pass_array[DYNAMIC_PASS_END];
static uint32_t pass_encode_array[DYNAMIC_PASS_END];

static int8_t EncodePassword(DynamicPass_t *DynamicPassObj);
static int8_t DecodePassword(DynamicPass_t *DynamicPassObj);
static int8_t GenerateRandomValue(DynamicPass_t *DynamicPassObj);
static int8_t UpdatePassword(DynamicPass_t *DynamicPassObj, uint16_t DYNAMIC_PASS_TYPE);

static DynamicPass_t DynamicPassObj;

void setDynamicPasswordSetup(DynamicPass_t *DynamicPassObj){

	setDynamicPasswordSoftware(DynamicPassObj, pass_array, pass_encode_array, DYNAMIC_PASS_END);
	setDynamicPasswordInterface(DynamicPassObj, EncodePassword, DecodePassword, GenerateRandomValue, UpdatePassword);
	setDynamicPasswordKeyValue(DynamicPassObj, 3006);
}

DynamicPass_t *getDynamicPasswordObj(){

	return &DynamicPassObj;
}

int8_t EncodePassword(DynamicPass_t *DynamicPassObj){

    uint32_t pass;
    uint16_t forth_dec;

    // Fabrika sifresini uretelim
    forth_dec = getDynamicPasswordRandomValue(DynamicPassObj) % 2 == 0 ? 4000 : 2000;
    pass = ((getDynamicPasswordRandomValue(DynamicPassObj) + forth_dec) * 3) ^ getDynamicPasswordKeyValue(DynamicPassObj);
    setDynamicPasswordEncodeValue(DynamicPassObj, pass, DYNAMIC_PASS_FACTORY);

    // Servis sifresi uretelim
    forth_dec = getDynamicPasswordRandomValue(DynamicPassObj) % 2 == 0 ? 8000 : 6000;
    pass = ((getDynamicPasswordRandomValue(DynamicPassObj) + forth_dec) * 3) ^ getDynamicPasswordKeyValue(DynamicPassObj);
    setDynamicPasswordEncodeValue(DynamicPassObj, pass, DYNAMIC_PASS_SERVICE);

    forth_dec = getDynamicPasswordRandomValue(DynamicPassObj) < 333 ? 1000 : 3000;
    pass = ((getDynamicPasswordRandomValue(DynamicPassObj) + forth_dec) * 3) ^ getDynamicPasswordKeyValue(DynamicPassObj);
    setDynamicPasswordEncodeValue(DynamicPassObj, pass, DYNAMIC_PASS_USER);

    return 0;
}

int8_t DecodePassword(DynamicPass_t *DynamicPassObj){

    uint32_t pass;

    pass = (getDynamicPasswordEncodeValue(DynamicPassObj, DYNAMIC_PASS_FACTORY) ^ getDynamicPasswordKeyValue(DynamicPassObj)) / 3;
    setDynamicPasswordDecodeValue(DynamicPassObj, pass, DYNAMIC_PASS_FACTORY);

    pass = (getDynamicPasswordEncodeValue(DynamicPassObj, DYNAMIC_PASS_SERVICE) ^ getDynamicPasswordKeyValue(DynamicPassObj)) / 3;
    setDynamicPasswordDecodeValue(DynamicPassObj, pass, DYNAMIC_PASS_SERVICE);

    pass = (getDynamicPasswordEncodeValue(DynamicPassObj, DYNAMIC_PASS_USER) ^ getDynamicPasswordKeyValue(DynamicPassObj)) / 3;
    setDynamicPasswordDecodeValue(DynamicPassObj, pass, DYNAMIC_PASS_USER);

    return 0;
}

int8_t GenerateRandomValue(DynamicPass_t *DynamicPassObj){

    int32_t rand_number;
    
    rand_number = HAL_GetTick() % 1000;
    setDynamicPasswordRandomValue(DynamicPassObj, rand_number);

    return 0;
}

int8_t UpdatePassword(DynamicPass_t *DynamicPassObj, uint16_t DYNAMIC_PASS_TYPE){

    // Uretilen sifreler modbus-registerlarina aktarilacak
	if(DYNAMIC_PASS_TYPE == DYNAMIC_PASS_FACTORY){

		setParamDataViaPN(DEVICE_DYNPASS_FACTORYCODE, getDynamicPasswordEncodeValue(getDynamicPasswordObj(), DYNAMIC_PASS_FACTORY), CALLBACK_OFF);
		setParamDataViaPN(DEVICE_DYNPASS_FACTORYPASS, getDynamicPasswordDecodeValue(getDynamicPasswordObj(), DYNAMIC_PASS_FACTORY), CALLBACK_OFF);
	}
	else if(DYNAMIC_PASS_TYPE == DYNAMIC_PASS_SERVICE){

		setParamDataViaPN(DEVICE_DYNPASS_SERVICECODE, getDynamicPasswordEncodeValue(getDynamicPasswordObj(), DYNAMIC_PASS_SERVICE), CALLBACK_OFF);
		setParamDataViaPN(DEVICE_DYNPASS_SERVICEPASS, getDynamicPasswordDecodeValue(getDynamicPasswordObj(), DYNAMIC_PASS_SERVICE), CALLBACK_OFF);
	}
	else if(DYNAMIC_PASS_TYPE == DYNAMIC_PASS_USER){

		setParamDataViaPN(DEVICE_DYNPASS_USERCODE, getDynamicPasswordEncodeValue(getDynamicPasswordObj(), DYNAMIC_PASS_USER), CALLBACK_OFF);
		setParamDataViaPN(DEVICE_DYNPASS_USERPASS, getDynamicPasswordDecodeValue(getDynamicPasswordObj(), DYNAMIC_PASS_USER), CALLBACK_OFF);
	}
	else{
		return -1;
	}

	return 0;
}
