/*
 * modbus_adapter_pdu.c
 *
 *  Created on: 15Jan.,2019
 *      Author: EN
 */

#include "modbus_adapter_pdu.h"
#include "parameters_setget.h"

#include <stdlib.h>


// Private variables
static uint32_t timeout_param;
static uint32_t tick_timeout;
static ModbusPDUAccess_e modbus_access_level;


#define QUEUE_MBS_ITEM_LEN			2
#define QUEUE_MBS_ITEM_PARAM_LEN	123

typedef enum{
	QUEUE_MBS_RET_NO = 0,
	QUEUE_MBS_RET_YES
}QueueMbsRet;

typedef enum{

	QUEUE_MBS_STATE_AVAIBLE = 0,
	QUEUE_MBS_STATE_DOWNLOADING,
	QUEUE_MBS_STATE_INUSE
}QueueMbsState_e;

typedef struct{
	uint16_t addr;
	int16_t param_val[QUEUE_MBS_ITEM_PARAM_LEN];
	uint16_t reg_len;
	uint16_t param_index;		//

	QueueMbsState_e state;
}QueueMbsItem_t;

typedef struct{

	uint16_t head;	// Bir sonraki bos alani gosterir
	uint16_t tail;	// Gonderilmesi gereken alani gosterir
	QueueMbsItem_t msgPackages[QUEUE_MBS_ITEM_LEN];
}QueueMbs_t;

// Private variables
QueueMbs_t QueueMbsObj;

// Private/helper functions
static uint16_t addressToParamNo(uint32_t param_adr);
static QueueMbsRet isQueueMbsAvaible(QueueMbs_t *QueueMbsObj);
static int8_t addQueueMbsItem(QueueMbs_t *QueueObj, uint16_t addr, uint16_t len);
static int8_t addQueueMbsItemParamValues(QueueMbsItem_t *QueueItemObj, int16_t param_val);
static void setQueueMbsItemState(QueueMbs_t *QueueObj, QueueMbsState_e state);
static QueueMbsItem_t *fetchQueueMbsItem(QueueMbs_t *QueueObj);
static uint16_t getQueueMbsItemParamWrittenLen(QueueMbs_t *QueueObj);

static int8_t CallbackQueueReleasae(void *param);


int8_t ModbusPDUAdapterWrite(uint16_t address, int8_t *value, uint16_t len_register){
	uint16_t param_no;
	int16_t reg_val;
	int16_t addr;
	int8_t res;
	uint16_t i;
	QueueMbsItem_t *queueMbsItem;

	res = 0;
	addr = address;

	if(len_register == 1){	// SingleWrite

		// Modbus paketlerinde ilk once MSB geliyor
		reg_val = value[1] & 0x00FF;
		reg_val |= ((int16_t)value[0] << 8) & 0xFF00;

		param_no = addressToParamNo(addr);

		/*if(param_no >= INVERTER_CURRNOM && param_no <= INVERTER_POWER){

			if(getCompState(&compressor1) != COMP_STATE_STANDS){
				// Kompresor calisiyorsa bu parametrelere yazima izin verilmeyecek...
				return -2;
			}
		}*/

		res = setParamDataViaPN(param_no, reg_val, CALLBACK_ON);

		return res;
	}

	// QueueMbsObj yer var mi?
	if(isQueueMbsAvaible(&QueueMbsObj) == QUEUE_MBS_RET_YES){

		if(addQueueMbsItem(&QueueMbsObj, address, len_register)){
			return  -2;
		}
		// Modbus verilerini int16_t haline getirip Queue yapisina alalim
		for (i = 0; i < len_register; ++i) {

			reg_val = value[2 * i + 1] & 0x00FF;
			reg_val |= ((int16_t)value[2 * i] << 8) & 0xFF00;

			// Min max kontrolu
			param_no = addressToParamNo(addr);
			if(reg_val >= getParamMinViaPN(param_no) && reg_val <= getParamMaxViaPN(param_no)){

				addQueueMbsItemParamValues(&QueueMbsObj.msgPackages[QueueMbsObj.head], reg_val);
			}
			else{

				res = -1;	// DataLimit Hatasi
				if(getQueueMbsItemParamWrittenLen(&QueueMbsObj) <= 0){

					setQueueMbsItemState(&QueueMbsObj, QUEUE_MBS_STATE_AVAIBLE);
					return res;
				}

				break;
			}

			addr++;
		}

		// Suana kadar limit sorunu olmayan parametreler yazilacak
		setQueueMbsItemState(&QueueMbsObj, QUEUE_MBS_STATE_INUSE);

		// Sorunsuz bir sekilde biterse gerekli parametreler icin CALLBACK cagrisi yapilsin....
		queueMbsItem = fetchQueueMbsItem(&QueueMbsObj);
		if(queueMbsItem == NULL){

			// Hata
			res = -3;	// Device Failure
		}
		else{
			CallbackQueueReleasae(fetchQueueMbsItem(&QueueMbsObj));
		}
	}
	else{

		res = -2;	// Device_BUSY
	}

	return res;
}


int16_t ModbusPDUAdapterRead(uint16_t param_adr){
	uint16_t param_no;
	int16_t val;

	param_no = addressToParamNo(param_adr);

	val = getParamDataViaPN(param_no);
	
	return val;
}

int8_t ModbusPDUAdressCheck(uint16_t address, uint16_t FN_CODE_x){
//In here if you return 0 access will be granted
//if you return -1 access denied

	//Checking for dynamic password timeout
//	if(tick_timeout >= timeout_param){
//		setModbusPDUAccessLevel(MODBUS_PDU_ACCESS_NONE);
//	}

	//Should be deleted to activate Dynamic Password Functionality, it is blocking the rest
	if(address >= 0 && address <= (PARAM_USERS_LAST_INDEX - 1)){
		return 0;
	}else{
		return -1;
	}

	//If no password has entered yet, should be only read IDs and passwords
	if(getModbusPDUAccessLevel() == MODBUS_PDU_ACCESS_NONE){

		// Sadece LOGIN_SECURITY ve IDENKO registerlarina erisim izni verilecek.
		if(address >= 9965 && address <= 9966){
			// 49966, 49967 -> LOGIN_SECURITY_CUSTOMER
			return 0;
		}
		else if(address >= 9969 && address <= 9970){
			// 49970, 49971 -> LOGIN_SECURITY_ENKO
			return 0;
		}
		else if((address >= 9986 && address <= 9988) && (FN_CODE_x == FN_CODE_READ_HOLDING)){
			// 49987, 49989 -> IDENKO
			return 0;
		}
		else if((address >= 9963 && address <= 9964) && (FN_CODE_x != FN_CODE_READ_HOLDING)){
			// 49964, 49965 -> LOGIN_PASSWORD_CUSTOMER_REGs
			return 0;
		}
		else if((address >= 9967 && address <= 9968) && (FN_CODE_x != FN_CODE_READ_HOLDING)){
			// 49968, 49969 -> LOGIN_PASSWORD_ENKO_REGs
			return 0;
		}
		else{
			return -1;
		}
	}
	//If customer has login, they should access device informations but not params
	else if(getModbusPDUAccessLevel() == MODBUS_PDU_ACCESS_CUSTOMER){

		// IDENKO, MODEL_NAME ve IDCPU alanlarina erisim izni verilmeyecek
		if(address >= 9976 && address <= 9985){
			// IDCPU -> 49977 - 49986
			return -1;
		}
		else if((address >= 9986 && address <= 9988) &&
				(FN_CODE_x == FN_CODE_WRITE_MULTIPLE || FN_CODE_x == FN_CODE_WRITE_SINGLE)){
			// IDENKO -> 49987 - 49989
			// Bu alana MUSTERININ yazmasina izin verilmiyor
			return -1;
		}
		else if(address >= 9989 && address <= 9998){
			// MODEL_NAME -> 49990 - 49999
			return -1;
		}
		else if(address == 9971){
			// OPTIONAL_MODULE -> 49972
			return -1;
		}
		else if(address >= 9972 && address <= 9973){
			// HARDWARE_VER -> 49973 - 49974
			return -1;
		}
		else{}
	}
	//If access level is higher, for now should access all params so continue
	else{
	}

	//Clear dynamic passwword flag
	clearModbusPDUAccessTimeoutTick();

	//rest of the params should be accesable
	if(address >= 0 && address <= (PARAM_USERS_LAST_INDEX - 1)){
		return 0;
	}else{
		return -1;
	}
}

void modbusPDUAccessTimeoutTickUpdate(uint32_t tick_step){

	if(tick_timeout < timeout_param){

		tick_timeout += tick_step;
	}
}

void setModbusPDUAccessTimeoutParam(uint32_t tick){

	timeout_param = tick;
}

void setModbusPDUAccessLevel(ModbusPDUAccess_e MODBUS_PDU_ACCESS_x){

	modbus_access_level = MODBUS_PDU_ACCESS_x;
}

ModbusPDUAccess_e getModbusPDUAccessLevel(){

	return modbus_access_level;
}

void clearModbusPDUAccessTimeoutTick(){

	tick_timeout = 0;
}

void modbusQueuItemSoftSetup(){

	uint16_t i;
	QueueMbsObj.head = 0;
	QueueMbsObj.tail = 0;

	for (i = 0; i < QUEUE_MBS_ITEM_LEN; ++i) {
		QueueMbsObj.msgPackages[i].addr = 0;
		QueueMbsObj.msgPackages[i].reg_len = 0;
		QueueMbsObj.msgPackages[i].state = QUEUE_MBS_STATE_AVAIBLE;
	}
}

/*
 * @brief         		: Adres bilgisine karsi gelen parametre numarasini verir.
 * @param[param_adr]	: Parametre adresi
 * @return        		: Parametre numarasini geri dondurur
 * @precondition  		: Girilen adres gecerli mi kontrolu
 * @postcondition 		: Parametre numarasi limitler icinde mi kontrolu
 */
uint16_t addressToParamNo(uint32_t param_adr){
	// Cihaz parametreleri
//	if(param_adr >= MIN_OF_THIS_GROUP - 1)
//		if(param_adr <= MAX_THIS_GROUP)
//	return param_adr + 1 - MIN_OF_THIS_GROUP + (IMPLEMENTED_MAX_OF PREVIOUS - POSSIBLE_MIN_OF_PREVIOUS + 1) + (IMPLEMENTED_MIN_OF_2_PREVIOUS - POSSIBLE_MIN_2PREVIOUS + 1);
//	Any new group should added at the top. Because it doesn't check recursively

	if(param_adr >= 9921 && param_adr <= 9999 ){
		param_adr -= 9852;
	}

	if(param_adr >= G10_MIN - 1){ //MONITORING GROUP
			if(param_adr <= G10_MAX){
				return param_adr + 1 - G10_MIN + G_MONITORING_OFF + (G8_IMP_MAX - G8_MIN + 1) + (G7_IMP_MAX - G7_MIN + 1) + (G6_IMP_MAX - G6_MIN + 1) + (G5_IMP_MAX - G5_MIN + 1) + (G4_IMP_MAX - G4_MIN + 1) + (G3_IMP_MAX - G3_MIN + 1) + (G2_IMP_MAX - G2_MIN + 1) + (G1_IMP_MAX - G1_MIN + 1);
				//monitoring should not include the last group
			}
		}
	else if(param_adr >= G9_MIN - 1){
				if(param_adr <= G9_MAX){
					return param_adr + 1 - G9_MIN + (G8_IMP_MAX - G8_MIN + 1) + (G7_IMP_MAX - G7_MIN + 1) + (G6_IMP_MAX - G6_MIN + 1) + (G5_IMP_MAX - G5_MIN + 1) + (G4_IMP_MAX - G4_MIN + 1) + (G3_IMP_MAX - G3_MIN + 1) + (G2_IMP_MAX - G2_MIN + 1) + (G1_IMP_MAX - G1_MIN + 1);
				}
			}
	else if(param_adr >= G8_MIN - 1){
			if(param_adr <= G8_MAX){
				return param_adr + 1 - G8_MIN + (G7_IMP_MAX - G7_MIN + 1) + (G6_IMP_MAX - G6_MIN + 1) + (G5_IMP_MAX - G5_MIN + 1) + (G4_IMP_MAX - G4_MIN + 1) + (G3_IMP_MAX - G3_MIN + 1) + (G2_IMP_MAX - G2_MIN + 1) + (G1_IMP_MAX - G1_MIN + 1);
			}
		}
	else if(param_adr >= G7_MIN - 1){
	if(param_adr <= G7_MAX){
		return param_adr + 1 - G7_MIN + (G6_IMP_MAX - G6_MIN + 1) + (G5_IMP_MAX - G5_MIN + 1) + (G4_IMP_MAX - G4_MIN + 1) + (G3_IMP_MAX - G3_MIN + 1) + (G2_IMP_MAX - G2_MIN + 1) + (G1_IMP_MAX - G1_MIN + 1);
	}
	}
	else if(param_adr >= G6_MIN - 1){
		if(param_adr <= G6_MAX){
			return param_adr + 1 - G6_MIN + (G5_IMP_MAX - G5_MIN + 1) + (G4_IMP_MAX - G4_MIN + 1) + (G3_IMP_MAX - G3_MIN + 1) + (G2_IMP_MAX - G2_MIN + 1) + (G1_IMP_MAX - G1_MIN + 1);
		}
	}
	else if(param_adr >= G5_MIN - 1){
		if(param_adr <= G5_MAX){
			return param_adr + 1 - G5_MIN + (G4_IMP_MAX - G4_MIN + 1) + (G3_IMP_MAX - G3_MIN + 1) + (G2_IMP_MAX - G2_MIN + 1) + (G1_IMP_MAX - G1_MIN + 1);
		}
	}
	else if(param_adr >= G4_MIN - 1){
		if(param_adr <= G4_MAX){
			return param_adr + 1 - G4_MIN + (G3_IMP_MAX - G3_MIN + 1) + (G2_IMP_MAX - G2_MIN + 1) + (G1_IMP_MAX - G1_MIN + 1);
		}

	}
	else if(param_adr >= G3_MIN - 1){
		if(param_adr <= G3_MAX){
			return param_adr + 1 - G3_MIN + (G2_IMP_MAX - G2_MIN + 1) + (G1_IMP_MAX - G1_MIN + 1);
		}
	}
	else if(param_adr >= G2_MIN - 1){
		if(param_adr <= G2_MAX){
			return param_adr + 1 - G2_MIN + (G1_IMP_MAX - G1_MIN + 1);
		}
	}
	else if(param_adr >= G1_MIN){
		if(param_adr <= G1_MAX){
			return param_adr + 1 - G1_MIN;
		}
	}else{
		return 0;
	}
	return 0;


/*
	if((param_adr >= 0) && param_adr <= (0 + PARAM_USERS_LAST_INDEX - 1)){
		return (param_adr + 1);
	}
	else{
		return 0;
	}
*/
}

QueueMbsRet isQueueMbsAvaible(QueueMbs_t *QueueObj){


	if(QueueObj->head < QUEUE_MBS_ITEM_LEN){

		return QUEUE_MBS_RET_YES;
	}
	else if(QueueObj->head == QUEUE_MBS_ITEM_LEN && QueueObj->tail == QUEUE_MBS_ITEM_LEN){

		// Head ve tail QUEUE_MBS_ITEM_LEN degerine ulasmis
		QueueObj->head = 0;
		QueueObj->tail = 0;
		return QUEUE_MBS_RET_YES;
	}
	else{

		// Queue'da bekleyen islemlerin bitmesi icin beklenmeli
		return QUEUE_MBS_RET_NO;
	}
}

int8_t addQueueMbsItem(QueueMbs_t *QueueObj, uint16_t addr, uint16_t len){

	if(QueueObj->msgPackages[QueueObj->head].state != QUEUE_MBS_STATE_AVAIBLE){

		return -2;
	}

	QueueObj->msgPackages[QueueObj->head].addr = addr;
	QueueObj->msgPackages[QueueObj->head].reg_len = len;
	QueueObj->msgPackages[QueueObj->head].state = QUEUE_MBS_STATE_DOWNLOADING;
	QueueObj->msgPackages[QueueObj->head].param_index = 0;

	return 0;
}

int8_t addQueueMbsItemParamValues(QueueMbsItem_t *QueueItemObj, int16_t param_val){

	if(QueueItemObj->param_index >= QUEUE_MBS_ITEM_PARAM_LEN){
		return -1;
	}

	if(QueueItemObj->state == QUEUE_MBS_STATE_DOWNLOADING){
		QueueItemObj->param_val[QueueItemObj->param_index] = param_val;
		QueueItemObj->param_index++;
	}

	return 0;
}

void setQueueMbsItemState(QueueMbs_t *QueueObj, QueueMbsState_e state){

	if(state == QUEUE_MBS_STATE_INUSE){
		if(QueueObj->msgPackages[QueueObj->head].state == QUEUE_MBS_STATE_DOWNLOADING){

			QueueObj->msgPackages[QueueObj->head].state = QUEUE_MBS_STATE_INUSE;
			QueueObj->head++;
		}
	}
	else if(state == QUEUE_MBS_STATE_DOWNLOADING){

		QueueObj->msgPackages[QueueObj->head].state = state;
	}
	else if(state == QUEUE_MBS_STATE_AVAIBLE){

		QueueObj->msgPackages[QueueObj->head].addr = 0;
		QueueObj->msgPackages[QueueObj->head].param_index = 0;
		QueueObj->msgPackages[QueueObj->head].reg_len = 0;
		QueueObj->msgPackages[QueueObj->head].state = QUEUE_MBS_STATE_AVAIBLE;
	}
}

QueueMbsItem_t *fetchQueueMbsItem(QueueMbs_t *QueueObj){

	QueueMbsItem_t *queueMbsItemObj;

	queueMbsItemObj = NULL;
	if(QueueObj->tail < QUEUE_MBS_ITEM_LEN){
		queueMbsItemObj = &QueueObj->msgPackages[QueueObj->tail];
	}

	QueueObj->tail++;
	return queueMbsItemObj;
}

uint16_t getQueueMbsItemParamWrittenLen(QueueMbs_t *QueueObj){

	// Downloading asamasinda degilse 0 donulsun seklinde eklenti yapilabilir, ekstra kontrol
	return QueueObj->msgPackages[QueueObj->head].param_index;
}

int8_t CallbackQueueReleasae(void *param){

	QueueMbsItem_t *QueueMbsItemObj;

	QueueMbsItemObj = (QueueMbsItem_t *)param;

	QueueMbsItemObj->state = QUEUE_MBS_STATE_AVAIBLE;

	return 0;
}
