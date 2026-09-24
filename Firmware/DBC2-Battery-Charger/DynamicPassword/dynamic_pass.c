#include "dynamic_pass.h"

static const char VERSION[5] = "1.0.1";

// Default dinamik sifre arayuz fonksiyonlari
static int8_t EncodePasswordDefault(DynamicPass_t *DynamicPassObj);
static int8_t DecodePasswordDefault(DynamicPass_t *DynamicPassObj);
static int8_t GenerateRandomValueDefault(DynamicPass_t *DynamicPassObj);
static int8_t UpdatePasswordDefault(DynamicPass_t *DynamicPassObj, uint16_t DYNAMIC_PASS_TYPE);

const char *getDynamisPasswordVersion(){

    return VERSION;
}

int8_t setDynamicPasswordSoftware(DynamicPass_t *DynamicPassObj, uint32_t *pass_list, uint32_t *pass_encode_list, uint16_t pass_count){

    // Default CCS3000 icin yazilmis olan dinamik sifreleme fonksiyonundaki key degeri atansin
    DynamicPassObj->key = 1543;
    DynamicPassObj->random_val = 0;

    DynamicPassObj->passwords = pass_list;
    DynamicPassObj->passwords_encoded = pass_encode_list;
    DynamicPassObj->pass_count = pass_count;

    setDynamicPasswordInterface(DynamicPassObj, EncodePasswordDefault, DecodePasswordDefault, GenerateRandomValueDefault, UpdatePasswordDefault);
}

DYNAMIS_PASS_STATUS_e runDynamicPassword(DynamicPass_t *DynamicPassObj){

    uint16_t i;

    if(DynamicPassObj->generateRandomValueFunc(DynamicPassObj)){
        return DYNAMIC_PASS_STATUS_RANDNUM_ERR;
    }

    if(DynamicPassObj->encodePassFunc(DynamicPassObj)){
        return DYNAMIC_PASS_STATUS_ENCODE_ERR;
    }

    if(DynamicPassObj->decodePassFunc(DynamicPassObj)){
        return DYNAMIC_PASS_STATUS_DECODE_ERR;
    }

    for (i = 0; i < DynamicPassObj->pass_count; i++){

        if(DynamicPassObj->updatePasswordFunc(DynamicPassObj, i)){
            return DYNAMIC_PASS_STATUS_UPDATE_ERR;
        }
    }
    
    return DYNAMIC_PASS_STATUS_NO_ERR;
}

int8_t setDynamicPasswordInterface(DynamicPass_t *DynamicPassObj, 
                                IEncodePassword encodePassFunc,
                                IDecodePassword decodePassFunc,
                                IGenerateRandomValue generateRandValFunc,
                                IUpdatePassword updatePasswordFunc){

    DynamicPassObj->encodePassFunc = encodePassFunc;
    DynamicPassObj->decodePassFunc = decodePassFunc;
    DynamicPassObj->generateRandomValueFunc = generateRandValFunc;
    DynamicPassObj->updatePasswordFunc = updatePasswordFunc;
}

int8_t setDynamicPasswordKeyValue(DynamicPass_t *DynamicPassObj, uint32_t key){

    DynamicPassObj->key = key;
}

uint32_t getDynamicPasswordKeyValue(DynamicPass_t *DynamicPassObj){

    return DynamicPassObj->key;
}

int8_t setDynamicPasswordRandomValue(DynamicPass_t *DynamicPassObj, uint32_t random_val){

    DynamicPassObj->random_val = random_val;
}

uint32_t getDynamicPasswordRandomValue(DynamicPass_t *DynamicPassObj){

    return DynamicPassObj->random_val;    
}

int8_t setDynamicPasswordEncodeValue(DynamicPass_t * DynamicPassObj, uint32_t value, int16_t DYNAMIC_PASS_TYPE){

    if(DYNAMIC_PASS_TYPE >= DynamicPassObj->pass_count){
        return -1;
    }

    *(DynamicPassObj->passwords_encoded + DYNAMIC_PASS_TYPE) = value;

    return 0;
}

uint32_t getDynamicPasswordEncodeValue(DynamicPass_t *DynamicPassObj, uint16_t DYNAMIC_PASS_TYPE){
    
    if(DYNAMIC_PASS_TYPE >= DynamicPassObj->pass_count){
        return 0xFFFFFFFF;
    }

    return *(DynamicPassObj->passwords_encoded + DYNAMIC_PASS_TYPE);
}

int8_t setDynamicPasswordDecodeValue(DynamicPass_t *DynamicPassObj, uint32_t value, uint16_t DYNAMIC_PASS_TYPE){

    if(DYNAMIC_PASS_TYPE >= DynamicPassObj->pass_count){
        return -1;
    }

    *(DynamicPassObj->passwords + DYNAMIC_PASS_TYPE) = value;

    return 0;
}

uint32_t getDynamicPasswordDecodeValue(DynamicPass_t *DynamicPassObj, uint16_t DYNAMIC_PASS_TYPE){

    if(DYNAMIC_PASS_TYPE >= DynamicPassObj->pass_count){
        return 0xFFFFFFFF;
    }

    return *(DynamicPassObj->passwords + DYNAMIC_PASS_TYPE);
}

// ********************************
// Private function implementations
// ********************************
int8_t EncodePasswordDefault(DynamicPass_t *DynamicPassObj){

    uint32_t pass_encoded;
    uint16_t i;

    for (i = 0; i < DynamicPassObj->pass_count; i++){

        pass_encoded = getDynamicPasswordRandomValue(DynamicPassObj) ^ getDynamicPasswordKeyValue(DynamicPassObj);
        setDynamicPasswordEncodeValue(DynamicPassObj, pass_encoded, i);
    }

    return 0;
}

int8_t DecodePasswordDefault(DynamicPass_t *DynamicPassObj){

    uint32_t pass;
    uint16_t i;

    for(i = 0; i < DynamicPassObj->pass_count; i++){

        pass = getDynamicPasswordEncodeValue(DynamicPassObj, i) ^ getDynamicPasswordKeyValue(DynamicPassObj);
        setDynamicPasswordDecodeValue(DynamicPassObj, pass, i);
    }

    return 0;
}

int8_t GenerateRandomValueDefault(DynamicPass_t *DynamicPassObj){

    static uint16_t rand_number = 1982;

    setDynamicPasswordRandomValue(DynamicPassObj, 1982);
    rand_number++;

    return 0;
}

int8_t UpdatePasswordDefault(DynamicPass_t *DynamicPassObj, uint16_t DYNAMIC_PASS_TYPE){
    
    return 0;
}
