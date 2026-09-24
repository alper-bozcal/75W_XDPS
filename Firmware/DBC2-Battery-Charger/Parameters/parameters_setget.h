/*
 * parameters.h
 *
 *  Created on: 14 Nis 2016
 *      Author: NAZIM YILDIZ
 */

#ifndef PARAMETERS_SETGET_H_
#define PARAMETERS_SETGET_H_

#include <parameter_definitions.h>
#include <stdint.h>

typedef enum{SAVE_DISABLE, SAVE_ENABLE}SaveDE_e;
typedef void (*Callback)(void *param);

typedef struct {
	const uint16_t param_no;
	const int16_t min;
	const int16_t max;
	const int16_t def;
	const int16_t coef;
	const uint8_t access_level;
	const SaveDE_e save_type;
	//Callback...fonksiyonu eklencek
	Callback FuncCallback;
}ParameterList_t;


//Parametre erisim seviye tanimlamalari
#define ACCESS_LEVEL1_WRITE	0x01		//Kullanici
#define ACCESS_LEVEL2_WRITE	0x02		//Servis
#define ACCESS_LEVEL3_WRITE	0x03		//Fabrika
#define ACCESS_LEVEL4_WRITE	0x04		//Enko
#define ACCESS_LEVEL5_WRITE	0x05		//Yazma yapilamaz
#define ACCESS_LEVEL1_READ  0x10
#define ACCESS_LEVEL2_READ  0x20
#define ACCESS_LEVEL3_READ  0x30
#define ACCESS_LEVEL4_READ  0x40
#define ACCESS_LEVEL5_READ  0x50	//Okuma yapilamaz

#define CALLBACK_OFF	0
#define CALLBACK_ON		1

/*
 * @brief         :
 * @param[]       :
 * @return        :
 * @precondition  :
 * @postcondition :
 */
int8_t setParamDataViaPN(uint16_t param_no, int16_t value, int8_t CALLBACK_X);
void setParamDataMultiViaPN(uint16_t param_no, int16_t *buffer, uint16_t len);
void setParamDataMultiViaPN_V2(uint16_t param_no, int16_t *buffer, uint16_t len);
/*
 * @brief         :
 * @param[]       :
 * @return        :
 * @precondition  :
 * @postcondition :
 */
//void setParamDataViaADR(uint16_t param_adr, int16_t value);
/*
 * @brief         :
 * @param[]       :
 * @return        :
 * @precondition  :
 * @postcondition :
 */
int16_t getParamDataViaPN(uint16_t param_no);
/*
 * @brief         :
 * @param[]       :
 * @return        :
 * @precondition  :
 * @postcondition :
 */
//int16_t getParamDataViaADR(uint16_t param_adr);
/*
 * @brief         :
 * @param[]       :
 * @return        :
 * @precondition  :
 * @postcondition :
 */
int16_t *getParamDataAdr(uint16_t param_no);
/*
 * @brief         :
 * @param[]       :
 * @return        :
 * @precondition  :
 * @postcondition :
 */
int16_t getParamMinViaPN(uint16_t param_no);
/*
 * @brief         :
 * @param[]       :
 * @return        :
 * @precondition  :
 * @postcondition :
 */
//int16_t getParamMinViaADR(uint16_t param_adr);
/*
 * @brief         :
 * @param[]       :
 * @return        :
 * @precondition  :
 * @postcondition :
 */
int16_t getParamMaxViaPN(uint16_t param_no);
/*
 * @brief         :
 * @param[]       :
 * @return        :
 * @precondition  :
 * @postcondition :
 */
//int16_t getParamMaxViaADR(uint16_t param_adr);
/*
 * @brief         :
 * @param[]       :
 * @return        :
 * @precondition  :
 * @postcondition :
 */
int16_t getParamDefViaPN(uint16_t param_no);
/*
 * @brief         :
 * @param[]       :
 * @return        :
 * @precondition  :
 * @postcondition :
 */
int16_t getParamCoefViaPN(uint16_t param_no);
/*
 * @brief         :
 * @param[]       :
 * @return        :
 * @precondition  :
 * @postcondition :
 */
Callback getParamCallbackFunc(uint16_t param_no);

/*
 * @brief         :
 * @param[]       :
 * @return        :
 * @precondition  :
 * @postcondition :
 */
//int16_t getParamDefViaADR(uint16_t param_adr);
/*
 * @brief         :
 * @param[]       :
 * @return        :
 * @precondition  :
 * @postcondition :
 */
int16_t checkAccessLevelRead(uint16_t param_no, uint8_t ACCESS_LEVELx);
/*
 * @brief         :
 * @param[]       :
 * @return        :
 * @precondition  :
 * @postcondition :
 */
int16_t checkAccessLevelWrite(uint16_t param_no, uint8_t ACCESS_LEVELx);
/*
 * @brief         :
 * @param[]       :
 * @return        :
 * @precondition  :
 * @postcondition :
 */
SaveDE_e getSaveActionViaPN(uint16_t param_no);
/*
 * @brief         :
 * @param[]       :
 * @return        :
 * @precondition  :
 * @postcondition :
 */
//SaveDE_e getSaveActionViaADR(uint16_t param_adr);
/*
 * @brief         :
 * @param[]       :
 * @return        :
 * @precondition  :
 * @postcondition :
 */
void loadDefaults(uint8_t ACCESS_LEVELx);

#endif /* PARAMETERS_SETGET_H_ */
