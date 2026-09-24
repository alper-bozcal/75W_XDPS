/**
 * @file modbusServer.c
 * @author ENKO Electronics
 * @brief  Modbus Server Interface header file. This file should be filled with app specific details.
 * @version 0.1
 * @date 2023-05-22
 * 
 * @copyright Copyright (c) 2023 @ ENKO Electronics
 * 
 */
#ifndef I_MODBUS_CALLBACK_H
#define I_MODBUS_CALLBACK_H

void callBackPsuUpdateVoltageREF(void);
void callBackPsuUpdateCurrentREF(void);
void callBackChargerUpdateVoltageREF(void);
void callBackChargerUpdateCurrentREF(void);
void CallbackUpdateVPWM(void);
void CallbackUpdateIPWM(void);
void CallbackDeviceReset(void);
void CallbackUpdateVoltageBoost(void);
void CallbackTestMode(void);
void CallbackRelay(void);
void CallbackDeviceUpdate(void);
void CallbackLEDControl(void);
void CallbackLEDBlink(void);
void CallbackSaveTag(void);
void CallbackFactoryReset(void);
void CallbackModel(void);

void CallbackUserPasswordEntry(void);
void CallbackEnkoPasswordEntry(void);
void CallbackLoginEnko(void);
void CallbackUpdateUserLevel(void);
void CallbackPt100TripTemp(void);
void CallbackPt100OverTemp(void);
void CallbackPt100UnderTemp(void);
void CallbackUpdateVoltagePID(void);
void CallbackUpdateCurrentPID(void);
#endif
