/*
 * parameter_callback.h
 *
 *  Created on: 2 Jan 2017
 *      Author: EN
 */

#ifndef PARAMETER_CALLBACK_H_
#define PARAMETER_CALLBACK_H_



/*
 * @brief         : Bu fonksiyon Framde yedeklenen degerler cihaza tekrar yukelencegi zaman cagrilmali
 *
 * @param[]       :
 * @return        :
 * @precondition  :
 * @postcondition :
 */
void CallbackInit();
/*
 * @brief         :
 * @param[]       :
 * @return        :
 * @precondition  :
 * @postcondition :
 */
void CallbackNull(void *param);

/**
 * @brief 
 * 
 * @param param 
 */
void callBackUpdateVoltageREF(void *param);
void callBackUpdateCurrentREF(void *param);
void CallbackUpdateVPWM(void *param);
void CallbackUpdateIPWM(void *param);
void CallbackDeviceReset(void* param);
void CallbackUpdateVoltageBoost(void *param);
void CallbackDebugParam(void *param);
void CallbackTestMode(void* param);
void CallbackParallelWorking(void* param);
void CallbackUpdateParallelWorkingCoeff(void* param);
void CallbackRelay(void* param);
void CallbackLoginCustomer(void *arg);
void CallbackLoginEnko(void *arg);
void CallbackOptionModule(void *arg);
void CallbackCalibration(void *arg);
void CallbackDeviceUpdate(void *arg);
void CallbackLEDControl(void *arg);
void CallbackTempControl(void *arg);
void CallbackReverse(void *param);
void CallbackSaveTag(void *param);
void CallbackFactoryReset(void* param);
void CallbackModel(void* param);
void CallbackDynamicPass();
void CallbackCutoff(void* param);
#endif
