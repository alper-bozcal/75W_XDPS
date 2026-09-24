/**
 * @file modbusServer.c
 * @author ENKO Electronics
 * @brief  Modbus Server Interface source file. This file should be filled with app specific details.
 * @version 0.1
 * @date 2023-05-22
 * 
 * @copyright Copyright (c) 2023 @ ENKO Electronics
 * 
 */
#include "ImodbusCallback.h"
#include "Controller.h"
#include "defaults.h"
#include "modbus-table.h"
#include "main.h"
#include "Flash.h"

extern modbusServerObject_t server;
extern controller_t controller;

extern int16_t ENKO_PASSWORD_BACKUP;
extern int16_t FACTORY_PASSWORD_BACKUP;
extern int16_t SERVICE_PASSWORD_BACKUP;
extern int16_t USER_PASSWORD_BACKUP;

void CallbackNull(void){}

void CallbackInit(){}

void callBackPsuUpdateVoltageREF(void){
  controller.flagPWM = 0;

  if(controller.psuReferenceVoltage * controller.psuReferenceCurrent > POWER){
    controller.psuReferenceCurrent = NOM_CURRENT;
  }

  Flash_writeParameter(USER_PARAM_START_ADDR, modbusServergetIndex(&server, PWMFLAG), 0, server.MB_REG_COUNT);

  CallbackModel();
}

void callBackPsuUpdateCurrentREF(void){

  if(controller.psuReferenceCurrent > MAX_CURRENT){
    controller.psuReferenceCurrent = MAX_CURRENT;
  }

  controller.flagPWM = 0;

  if(controller.psuReferenceVoltage * controller.psuReferenceCurrent > POWER){
    controller.psuReferenceVoltage = MIN_VOLTAGE;
  }

  extern protection_t protectionOverCurrent;
  protectionOverCurrent.threshold = controller.psuReferenceCurrent;

  controller.effectiveCurrent = controller.psuReferenceCurrent;

  Flash_writeParameter(USER_PARAM_START_ADDR, modbusServergetIndex(&server, PWMFLAG), 0, server.MB_REG_COUNT);

  CallbackModel();
}

void callBackChargerUpdateVoltageREF(void){
  controller.flagPWM = 0;

  if(controller.chargeVoltage * controller.chargeCurrent > POWER){
    controller.chargeCurrent = NOM_CURRENT;
  }

  extern protection_t protectionOverVolt;
  protectionOverVolt.threshold = controller.boostVoltage + 100;

  Flash_writeParameter(USER_PARAM_START_ADDR, modbusServergetIndex(&server, PWMFLAG), 0, server.MB_REG_COUNT);

  CallbackModel();
}

void callBackChargerUpdateCurrentREF(void){

  if(controller.chargeCurrent > MAX_CURRENT){
    controller.chargeCurrent = MAX_CURRENT;
  }

  controller.flagPWM = 0;

  if(controller.chargeVoltage * controller.chargeCurrent > POWER){
    controller.chargeVoltage = MIN_VOLTAGE;
  }

  extern protection_t protectionOverCurrent;
  protectionOverCurrent.threshold = controller.chargeCurrent;

  controller.effectiveCurrent = controller.chargeCurrent;

  Flash_writeParameter(USER_PARAM_START_ADDR, modbusServergetIndex(&server, PWMFLAG), 0, server.MB_REG_COUNT);

  CallbackModel();
}


void CallbackModel(void){

    //Check device type and set for analog

    int16_t modelRefVoltage, modelRefCurrent;

    if(controller.flagModePSU == S_CHARGER){
  	  modelRefVoltage = controller.chargeVoltage;
  	  modelRefCurrent = controller.chargeCurrent;
    }
    else{
  	  modelRefVoltage = controller.psuReferenceVoltage;
  	  modelRefCurrent = controller.psuReferenceCurrent;
    }

    if(modelRefVoltage <= 1600){

      #ifdef DBC_2_150W
      //This GPIO controls the output regulation zeners directly.
      HAL_GPIO_WritePin(OVER_VOLT_GPIO_Port, OVER_VOLT_Pin, GPIO_PIN_SET);
      #endif

      if(modelRefCurrent <= 500){
        controller.outputModel = 1205;

      }else if(modelRefCurrent > 500  && modelRefCurrent <= 1000){
        controller.outputModel = 1210;

      }
      else{
        controller.outputModel = 1200;

      }

    }else if(modelRefVoltage > 1600 && modelRefVoltage <= 3000){

      if(modelRefCurrent <= 500){
        controller.outputModel = 2405;

      }else if(modelRefCurrent > 500 && modelRefCurrent <= 1000){
        controller.outputModel = 2410;

      }else{
        controller.outputModel = 2400;

      }
    }else{
      controller.outputModel = 0000;
    }
}

void CallbackUpdateVPWM(void){

  controller.flagPWM = 1;
  Flash_writeParameter(USER_PARAM_START_ADDR, modbusServergetIndex(&server, PWMFLAG), 1, server.MB_REG_COUNT);
}

void CallbackUpdateIPWM(void){

  controller.flagPWM = 1;
  Flash_writeParameter(USER_PARAM_START_ADDR, modbusServergetIndex(&server, PWMFLAG), 1, server.MB_REG_COUNT);
}

void CallbackDeviceReset(void){
  HAL_NVIC_SystemReset();
}

void CallbackUpdateVoltageBoost(void){

  if( MIN_VOLTAGE > controller.boostVoltage || controller.boostVoltage > MAX_VOLTAGE){
    controller.boostVoltage = MIN_VOLTAGE;
  }

  if(controller.boostVoltage * controller.chargeCurrent > POWER){
    controller.chargeCurrent = NOM_CURRENT;
  }

  extern protection_t protectionOverVolt;
  protectionOverVolt.threshold = controller.boostVoltage + 100;

  controller.flagPWM = 0;
}

void CallbackTestMode(void){
  //Till test mode starts, jig should read monitor params from modbus and calculate the calib and offset params
  //When jig triggers test mode it should be ready to send calib and offset params
  //maybe serial number checking should be implemented in test mode, or jig just recognizes from qr?
}

void CallbackRelay(void){

  if(controller.flagRelay == DISABLED){
    RELAY_OFF;
    setRelayState(RELAY_WAITING);

  }else if(controller.flagRelay == ENABLED){
    RELAY_ON;
    setRelayState(RELAY_WAITING);
  }
}

void CallbackLEDControl(void){

  extern _timer_t ledTimer;
  initTimer(&ledTimer);

  controlLEDbyParam(controller.LEDColor, LED_ON);
}


void CallbackLEDBlink(void){
  extern _timer_t ledTimer;
    setTimer(&ledTimer, LED_DEFAULT_BLINK_TIME);
}

uint8_t tagCount = 0;
int16_t wtf[TAG_SIZE];
void CallbackSaveTag(void){

  wtf[tagCount] = getParamValByAddress(&server, DEBUG_TAG);
  tagCount++;
  if(tagCount == TAG_SIZE){
    //Save routine
    Flash_writeTAG((uint8_t*)wtf, 2 * tagCount, server.MB_REG_COUNT);
    tagCount = 0;
  }
}

void CallbackFactoryReset(void){

  extern int16_t ENKO[15];
  // Create a temp array
  int16_t* values = malloc(server.MB_REG_COUNT);
  if(values == NULL) return;
  // Iterate over all param list
  for(uint16_t index = 0; index < server.MB_REG_COUNT; index++){
    // Update the temp array for default values

	if((index>=MEASURE_OFFSET_VOUT && index<=MEASURE_SHIFT_PT100)
			&& ENKO[13] == RETURN_USER_DEFAULT){

	    values[index] = *(server.modbusRegisters[index].value);
	}
	else{
	    values[index] = server.modbusRegisters[index].Default;
	}

  }
  // Update Flash with defaults
  Flash_writeAllParameter(USER_PARAM_START_ADDR, values, server.MB_REG_COUNT);
  // Reset the device to apply changes
  CallbackDeviceReset();
}

/*------------------ Password Protection Callbacks ------------------*/

void CallbackEnkoPasswordEntry(void){
  uint16_t enkoEntry = getParamValByAddress(&server, DEVICE_ENKO_PASS_ENTRY);
  setParamValByAddress(&server, DEVICE_USER_PASS_ENTRY, LEVEL_0);

  // First restore password values for comparison (using backup values)
  ImodbusRestorePasswords();

  if(enkoEntry == ENKO_PASSWORD_BACKUP/*systems.PASS_ENKO*/){
    server.userLevel = LEVEL_4;
  }else{
    server.userLevel = LEVEL_0;
  }

  // After level change, hide passwords that are above current user level
  ImodbusHidePasswordsBasedOnLevel();

  setParamValByAddress(&server, DEVICE_ENKO_PASS_ENTRY, server.userLevel);
}

void CallbackUserPasswordEntry(void){
  uint16_t userEntry = getParamValByAddress(&server, DEVICE_USER_PASS_ENTRY);
  setParamValByAddress(&server, DEVICE_ENKO_PASS_ENTRY, LEVEL_0);

  // First restore password values for comparison (using backup values)
  ImodbusRestorePasswords();

  if(userEntry == FACTORY_PASSWORD_BACKUP/*systems.PASS_FACT*/){
    server.userLevel = LEVEL_3;
  }else if(userEntry == SERVICE_PASSWORD_BACKUP/*systems.PASS_SERV*/){
    server.userLevel = LEVEL_2;
  }else if(userEntry == USER_PASSWORD_BACKUP/*systems.PASS_USER*/){
    server.userLevel = LEVEL_1;
  }else{
    server.userLevel = LEVEL_0;
  }

  // After level change, hide passwords that are above current user level
  ImodbusHidePasswordsBasedOnLevel();

  setParamValByAddress(&server, DEVICE_USER_PASS_ENTRY, server.userLevel);
}

void CallbackLoginEnko(void){

  int16_t security1, security2;
  int16_t pass1, pass2;

  security1 = getParamValByAddress(&server, LOGIN_SECURITY_ENKO_REG1);
  security2 = getParamValByAddress(&server, LOGIN_SECURITY_ENKO_REG2);

  pass1 = security1 ^ (getParamValByAddress(&server, DEVICE_IDENKO_REG1) + getParamValByAddress(&server, DEVICE_IDENKO_REG3));
  pass2 = security2 ^ (getParamValByAddress(&server, DEVICE_IDENKO_REG2) + getParamValByAddress(&server, DEVICE_IDENKO_REG3));

  if(pass1 == getParamValByAddress(&server, LOGIN_PASSWORD_ENTRY_ENKO_REG1) &&
      pass2 == getParamValByAddress(&server, LOGIN_PASSWORD_ENTRY_ENKO_REG2)){

    server.userLevel = LEVEL_4;
    setParamValByAddress(&server, LOGIN_PASSWORD_ENTRY_ENKO_REG1, 0);
    setParamValByAddress(&server, LOGIN_PASSWORD_ENTRY_ENKO_REG2, 0);
  }
  else{

    server.userLevel = LEVEL_0;
  }
}

void CallbackUpdateUserLevel(void){
  // When password is changed, update the backup values
  // This function is called when password registers are written to
  ImodbusBackupPasswords();
}

void CallbackPt100TripTemp(void){
  extern protection_t protectionTripTemp;
  protectionTripTemp.threshold = controller.pt100HighTripTemp;
}

void CallbackPt100OverTemp(void){
  extern protection_t protectionOverTempPT100;
  protectionOverTempPT100.threshold = controller.Pt100AlarmTemp;
}

void CallbackPt100UnderTemp(void){
  extern protection_t protectionUnderTempPT100;
  protectionUnderTempPT100.threshold = controller.pt100LowAlarmTemp;
}

void CallbackUpdateVoltagePID(void){
  // Update voltage PID parameters from controller struct
  extern PID_t voltagePID;
  voltagePID.Kp = controller.voltagePidKp;
  voltagePID.Ki = controller.voltagePidKi;
  voltagePID.Kd = controller.voltagePidKd;
}

void CallbackUpdateCurrentPID(void){
  // Update current PID parameters from controller struct
  extern PID_t currentPID;
  currentPID.Kp = controller.currentPidKp;
  currentPID.Ki = controller.currentPidKi;
  currentPID.Kd = controller.currentPidKd;
}

/*-------------------------------------------------------------------*/
