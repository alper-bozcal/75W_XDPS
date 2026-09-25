/**
 * @file Interfaces.c
 * @author C.U.
 * @brief This C/H files contains interface functions to peripherals
 *        Like __initEEPROM(), __initMODBUS() etc.
 *        This way all DBC2 but not 90-150 etc. spesific code can live in it
 * 
 */
/*----------------------------------------------------------------------------------------------------------------*/

#include "Interfaces.h"

/*----------------------------------------------------------------------------------------------------------------*/

__attribute__((section(".SIGNSECTION.SGN"))) volatile const uint8_t signature[4] ={'E','N','K','O'};
__attribute__((section(".SIGNATURESECTION.SGN"))) volatile const uint8_t signature2[4] ={'E','N','K','O'};
__attribute__((section(".V_TABLE_SECTION.table")))  uint32_t VectorTable[50];

/*----------------------------------------------------------------------------------------------------------------*/

extern measurement_t Vout, Iout, Intc, Ontc, Tpt, Vbat;
MEASURED_t measuredValues;
controller_t controller = {0}; // Controller singleton object
uint32_t statsCurrents[20] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
uint32_t statsTemps[20]    = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

modbusServerObject_t server;
volatile uint16_t ADCValues[DMA_SIZE];
uint16_t ledTick = 0,ledReverseTick = 0;
uint8_t ledToggle = 0;

static volatile uint8_t boostEdgeLastState = 0;
static volatile uint8_t boostDebounceState = 0;
static volatile uint8_t boostDebounceLastRead = 0;
static volatile uint16_t boostDebounceCounter = 0;

static uint8_t isFloatOperationMode = 0;
static uint8_t isBoostOperationMode = 0;

static int16_t derateFaultState = 0;

_timer_t boostTimer;
_timer_t ledTimer;
_timer_t batteryTimer;
_timer_t derateTimer;
_timer_t controllerTimer;
_timer_t statTimer;

protection_t protectionOverVolt;
protection_t protectionOverCurrent;
protection_t protectionOutputReverse;
protection_t protectionOverTemp;
protection_t protectionTripTemp;
protection_t protectionPT100Broken;
protection_t protectionOverTempPT100;
protection_t protectionUnderTempPT100;
protection_t protectionBatteryDisconnect;

// PID Controller objects
PID_t voltagePID = {0};
PID_t currentPID = {0};

// Battery Connection Detection
batConnDetection_t batteryConnectionDetection = {0};
int16_t batteryConnectionStatus = 0, pt100ConnectionStatus = 0; // 0 = connected, 1 = disconnected

/*----------------------------------------------------------------------------------------------------------------*/

void _initVectorTable(void){
  // Vektor tablosu tasıma
  for(uint8_t i = 0; i < 48; i++){
		VectorTable[i] = *(__IO uint32_t*)(USER_PROG_START_ADR + (i<<2));
	}
	// Enable the SYSCFG peripheral clock//
	RCC_APB2PeriphResetCmd(RCC_APB2ENR_SYSCFGEN, ENABLE);
	// Remap SRAM at 0x00000000 //
	SYSCFG_MemoryRemapConfig(((uint8_t)0x03));
	if(signature[0]==1)  // optimize etmemesi için eklendi.
		if(signature2[0]==1)  // optimize etmemesi için eklendi.
	__NOP();
}

void _lockOB(void){
  FLASH_OBProgramInitTypeDef OBInit;
  // Get current status of the OB
  HAL_FLASHEx_OBGetConfig(&OBInit);
  //If it is not LEVEL 1 protected, set it. Else just ignore.
  if(OBInit.RDPLevel != OB_RDP_LEVEL_1){
    //Unlock OB
    HAL_FLASH_Unlock();
    HAL_FLASH_OB_Unlock();
    // Set Protection Level 1. This Level means no read operation but OB can be change in exchange of mass erase.
    // LEVEL 2 means no change on OB
    OBInit.OptionType = OPTIONBYTE_RDP;
    OBInit.RDPLevel = OB_RDP_LEVEL_1;
    // Change OB
    HAL_FLASHEx_OBProgram(&OBInit);
    // Launch means restart MCU
    HAL_FLASH_OB_Launch();
  }
  //Lock OB
  HAL_FLASH_OB_Lock();
  HAL_FLASH_Lock();
}

void _initADC(void){

  //Start ADC and DMA peripherals
  extern ADC_HandleTypeDef hadc;
  HAL_ADCEx_Calibration_Start(&hadc);
  HAL_ADC_Start_DMA(&hadc, (uint32_t*)ADCValues, DMA_SIZE);
}

void _initSystem(void){

  // Load all parameters from Flash or to their defaults
  _loadParameters();

  // Initialize output protections
  initProtection(&protectionOverVolt,      &measuredValues.Vout, controller.boostVoltage + 100, OV_DEFAULT_DELAY, OV_DEFAULT_MODE);
  initProtection(&protectionOutputReverse, &measuredValues.Vbat, OR_DEFAULT_TARGET, OR_DEFAULT_DELAY, OR_DEFAULT_MODE);
  initProtection(&protectionOverCurrent,   &measuredValues.Iout, OC_DEFAULT_TARGET, OC_DEFAULT_DELAY, OC_DEFAULT_MODE);
  initProtection(&protectionOverTemp,      &derateFaultState, 0, OT_DEFAULT_DELAY, OT_DEFAULT_MODE);
  initProtection(&protectionTripTemp,      &measuredValues.CPT100, controller.pt100HighTripTemp, OT_DEFAULT_DELAY, MODE_OVER);
  initProtection(&protectionOverTempPT100, &measuredValues.CPT100, controller.Pt100AlarmTemp, OT_DEFAULT_DELAY, OT_DEFAULT_MODE);
  initProtection(&protectionUnderTempPT100, &measuredValues.CPT100, controller.pt100LowAlarmTemp, OT_DEFAULT_DELAY, MODE_UNDER);
  initProtection(&protectionBatteryDisconnect, &batteryConnectionStatus, 0, 1000, MODE_OVER); // Trigger when status >= 0 , delay 1 second
  initProtection(&protectionPT100Broken,   &pt100ConnectionStatus, 0, 1000, MODE_OVER); // Trigger when status >= 0 , delay 1 second

  // Init Output PID Controller Objects
  initPID(&voltagePID, V_PID_KP, V_PID_KI, V_PID_KD, V_PID_KA, PWM_MIN, PWM_VMAX);
  initPID(&currentPID, I_PID_KP, I_PID_KI, I_PID_KD, I_PID_KA, PWM_MIN, PWM_MAX);

  // Initialize Battery Connection Detection
  initBatteryConnectionDetection(&batteryConnectionDetection);
  // Enable detection (can be disabled via modbus or user setting if needed)
  enableBatteryConnectionDetection(&batteryConnectionDetection, 1);
}

void _initPWM(void){
  
  // Initialize PWM duty cycles to zero for new frequency
  V_PWM_DUTY = 0;
  I_PWM_DUTY = 0;
#ifdef V_PWM_APPLY
  V_PWM_APPLY();
#endif

  HAL_TIM_PWM_Start(&TIMER_PWM, I_PWM_CH);
#ifdef TIMER_V_PWM
  HAL_TIM_PWM_Start(&TIMER_V_PWM, V_PWM_CH);
#else
  HAL_TIM_PWM_Start(&TIMER_PWM, V_PWM_CH);
#endif
}

void _initTimer(void){

  // Start TIM14 1ms Timer Interrupt
  HAL_TIM_Base_Start_IT(&TIMER_1MS);

  HAL_TIM_Base_Start_IT(&htim3);

  // Start boost input timer (works with tim14)
  initTimer(&boostTimer);

  // Start LED Timer
  initTimer(&ledTimer);

  setTimer(&ledTimer, LED_DEFAULT_BLINK_TIME);

  // Start batteryTimer for 1min
  initTimer(&batteryTimer);
  batteryTimer.state = TIMER_COMPLETE;
//  setTimer(&batteryTimer, DEF_BATT_TIMER);

  // Start derateTimer for 1min
  initTimer(&derateTimer);
  setTimer(&derateTimer, DEF_BATT_TIMER);
  // To fix ref current 0 for 1 min bug
  derateTimer.state = TIMER_COMPLETE;

  // Start controllerTimer for 10ms
  initTimer(&controllerTimer);
  setTimer(&controllerTimer, DEF_CONTROL_TIMER);

  setTimer(&boostTimer, DEBOUNCE_TIME_BOOST_PIN);

  // Start statTimer for 8 Hours
  initTimer(&statTimer);
  setTimer(&statTimer, DEF_STAT_TIMER);
}

void _loadSystem(void){

  // Algorithm starts

  //Toggle the LEDs to initial state
  controller.LEDBlink = 0;
  controller.LEDColor = Red;
  controlLEDbyParam(controller.LEDColor, LED_ON);
  LL_IWDG_ReloadCounter(IWDG);
  HAL_Delay(LED_DEFAULT_BLINK_TIME);
  LL_IWDG_ReloadCounter(IWDG);
  // Set device model according to output voltage param
  CallbackModel();

  ImodbusBackupPasswords();

  // Start with Float, if there is no auto boost, stay in float
  controller.chargeState =  FLOAT;

  // Open the output MOSFET
  controller.flagCutOff = CONDUCTING;

  // Initialize boost pin debounce variables
  boostEdgeLastState = READ_BOOST_PIN;
}

void _initModbus(void){

  ImodbusServerInit(&server);
}


void _loadParameters(void){
  // Iterate over all param list
  for(uint16_t index = 0; index < server.MB_REG_COUNT; index++){
    // Init params with defaults
    if(server.modbusRegisters[index].value != NULL){
      *(server.modbusRegisters[index].value) = server.modbusRegisters[index].Default;
    }
    // If parameter is save enabled:
    if(server.modbusRegisters[index].saveEnable == SAVE_ENABLE){
      // Read parameter from flash and try to update modbus
      // If it fails, modbus will be default because we set it to default before
      setParamValByAddress(&server, server.modbusRegisters[index].mbAddress,
          Flash_readParameter(USER_PARAM_START_ADDR, index));
    }
  }
  
  // Check ENKO signature and perform factory reset if needed
  // This must be done after parameters are loaded from flash
  ImodbusCheckEnkoSignature();
  
  // Load measurement objects from modbus params
  setMeasurements();
  
  // Update PID parameters after loading from flash
  CallbackUpdateVoltagePID();
  CallbackUpdateCurrentPID();
  
  //* Test Jig must return to default params for the first time
}

void _checkProtections(void){

  // Check for reverse polarity
  if(protectionOutputReverse.triggered){
    //RED LED Constantly ON
    controller.flagERR = controller.flagERR | 0x01; // 1st Error bit is OutputReverse

  }else{
    controller.flagERR = controller.flagERR & ~(0x01); // 1st Error bit is OutputReverse

  }

  // Check for temperature
  if(protectionOverTemp.triggered){
    //RED LED Flashes 1Hz
    controller.flagERR = controller.flagERR | 0x02; // 2nd Error bit is overTempError

  }else{
    controller.flagERR = controller.flagERR & ~(0x02); // 2nd Error bit is overTempError

  }

  // Check for over voltage situation
  if(protectionOverVolt.triggered){
    //RED LED Flashes 2Hz
    controller.flagERR = controller.flagERR | 0x04; // 3rd Error bit is OverVolt

  }else{
    controller.flagERR = controller.flagERR & ~(0x04); // 3rd Error bit is OverVolt

  }

  // over current can happen on current limit so no blocking but set warning
//  if(protectionOverCurrent.triggered){
//    //controller.flagWARN = controller.flagWARN | 0x02; // 2nd Warning bit is OverCurrent
//  }else{
//    //controller.flagWARN = controller.flagWARN & ~(0x02); // 2nd Warning bit is OverCurrent
//  }

  // PT100 Trip Temperature (High Temperature Trip)
  if(controller.flagPt100 && protectionTripTemp.triggered && protectionPT100Broken.triggered == 0){
    controller.flagERR = controller.flagERR | 0x10; // 5th Error bit is PT100TripTemp

  }else{
    controller.flagERR = controller.flagERR & ~(0x10); // 5th Error bit is PT100TripTemp
  }

  // PT100 Over Temp Alarm
  if(controller.flagPt100 && protectionOverTempPT100.triggered && protectionPT100Broken.triggered == 0){
    controller.flagERR = controller.flagERR | 0x20; // 6th Error bit is PT100OverTemp

  }else{
    controller.flagERR = controller.flagERR & ~(0x20); // 6th Error bit is PT100OverTemp
  }

  // PT100 Under Temp Alarm
  if(controller.flagPt100 && protectionUnderTempPT100.triggered && protectionPT100Broken.triggered == 0){
    controller.flagERR = controller.flagERR | 0x08; // 4th Error bit is PT100UnderTemp

  }else{
    controller.flagERR = controller.flagERR & ~(0x08); // 4th Error bit is PT100UnderTemp
  }

  // PT100 Over Temp Warning
  if(controller.flagPt100 && (measuredValues.CPT100 > controller.Pt100WarningTemp) && protectionPT100Broken.triggered == 0){
    controller.flagWARN = controller.flagWARN | 0x10; // 5th Warning bit is PT100WarningTemp

  }else{
    controller.flagWARN = controller.flagWARN & ~(0x10); // 5th Warning bit is PT100WarningTemp
  }

  // PT100 Under Temp Warning
  if(controller.flagPt100 && (measuredValues.CPT100 < controller.pt100LowWarningTemp) && protectionPT100Broken.triggered == 0){
    controller.flagWARN = controller.flagWARN | 0x40; // 7th Warning bit is PT100UnderTempWarning

  }else{
    controller.flagWARN = controller.flagWARN & ~(0x40); // 7th Warning bit is PT100UnderTempWarning
  }

  // Check for battery disconnect protection
  if(protectionBatteryDisconnect.triggered){
    //RED LED Flashes 7Hz
    controller.flagERR = controller.flagERR | 0x80; // 8th Error bit is BatteryDisconnect
  }else{
    controller.flagERR = controller.flagERR & ~(0x80); // 8th Error bit is BatteryDisconnect
  }

  // PT100 Connection Status
  if(controller.flagPt100 && protectionPT100Broken.triggered){
    controller.flagERR = controller.flagERR | 0x40; // 6th Error bit

  }else{
    controller.flagERR = controller.flagERR & ~(0x40); // 6th Error bit
  }

  //Uyari kaldirildi.
//  // Check for open connections on ADC
//  if(isADCOpen(measuredValues) == STAT_TRUE){
//    controller.flagWARN = controller.flagWARN | 0x01; // 1st Warning bit is ADCOpen
//  }else{
//    controller.flagWARN = controller.flagWARN & 0xFE; // 1st Warning bit is ADCOpen
//  }

  if((measuredValues.ntcC/10) > POWER_DERATE_START_TEMP){
    controller.flagWARN = controller.flagWARN | 0x02; // 2nd Warning bit is board over temp
  }else{
    controller.flagWARN = controller.flagWARN & ~(0x02); // 2nd Warning bit is board over temp
  }
}

void updateStats(void){

  // Update current range buckets
  uint8_t current = ((measuredValues.Iout * 5) >> 9) % 16;
  if(current > 10) current = 10;
  statsCurrents[current]++;

  int16_t temperature = (measuredValues.INTC * 51) >> 9;
  if(temperature < 0)   temperature = 0;
  if(temperature > 100) temperature = 100;
  // TODO possible optimization below
  temperature /= 10;
  statsTemps[temperature]++;

}

void saveStats(void){

  // Read STAT section
  volatile uint32_t tmpStats[40]; // 40 = 20 Current + 20 Temperature range with redundants

  Flash_readWords(tmpStats, STAT_PAGE_START_ADDR, 40);

  // If it is not FF, increment each stat
  //If it is FF erase it and update it with new val
  for(uint8_t i = 0; i < 20; ++i){
    if(tmpStats[i] == 0xFFFF) tmpStats[i]  = statsCurrents[i];
    else                      tmpStats[i] += statsCurrents[i];
    statsCurrents[i] = 0;
  }

  for(uint8_t i = 0; i < 20; ++i){
    if(tmpStats[20 + i] == 0xFFFF) tmpStats[20 + i]  = statsTemps[i];
    else                           tmpStats[20 + i] += statsTemps[i];
    statsTemps[i] = 0;
  }

  // Erase STATs
  Flash_erasePage(STAT_PAGE_NO);

  // Write STATs
  Flash_writeWords(tmpStats, STAT_PAGE_START_ADDR, 40);

}

void sendStats(void){

  // Read STAT section
  volatile uint32_t tmpStats[40]; // 40 = 20 Current + 20 Temperature range with redundants

  Flash_readWords(tmpStats, STAT_PAGE_START_ADDR, 40);

  setParamValByAddress(&server, STAT_CURR_RANGE_1_1, (uint16_t)(tmpStats[0] >> 16));
  setParamValByAddress(&server, STAT_CURR_RANGE_1_2, (uint16_t) tmpStats[0]);
  setParamValByAddress(&server, STAT_CURR_RANGE_2_1, (uint16_t)(tmpStats[1] >> 16));
  setParamValByAddress(&server, STAT_CURR_RANGE_2_2, (uint16_t) tmpStats[1]);
  setParamValByAddress(&server, STAT_CURR_RANGE_3_1, (uint16_t)(tmpStats[2] >> 16));
  setParamValByAddress(&server, STAT_CURR_RANGE_3_2, (uint16_t) tmpStats[2]);
  setParamValByAddress(&server, STAT_CURR_RANGE_4_1, (uint16_t)(tmpStats[3] >> 16));
  setParamValByAddress(&server, STAT_CURR_RANGE_4_2, (uint16_t) tmpStats[3]);
  setParamValByAddress(&server, STAT_CURR_RANGE_5_1, (uint16_t)(tmpStats[4] >> 16));
  setParamValByAddress(&server, STAT_CURR_RANGE_5_2, (uint16_t) tmpStats[4]);
  setParamValByAddress(&server, STAT_CURR_RANGE_6_1, (uint16_t)(tmpStats[5] >> 16));
  setParamValByAddress(&server, STAT_CURR_RANGE_6_2, (uint16_t) tmpStats[5]);
  setParamValByAddress(&server, STAT_CURR_RANGE_7_1, (uint16_t)(tmpStats[6] >> 16));
  setParamValByAddress(&server, STAT_CURR_RANGE_7_2, (uint16_t) tmpStats[6]);
  setParamValByAddress(&server, STAT_CURR_RANGE_8_1, (uint16_t)(tmpStats[7] >> 16));
  setParamValByAddress(&server, STAT_CURR_RANGE_8_2, (uint16_t) tmpStats[7]);
  setParamValByAddress(&server, STAT_CURR_RANGE_9_1, (uint16_t)(tmpStats[8] >> 16));
  setParamValByAddress(&server, STAT_CURR_RANGE_9_2, (uint16_t) tmpStats[8]);
  setParamValByAddress(&server, STAT_CURR_RANGE_10_1,(uint16_t)(tmpStats[9] >> 16));
  setParamValByAddress(&server, STAT_CURR_RANGE_10_2,(uint16_t) tmpStats[9]);

  setParamValByAddress(&server, STAT_TEMP_RANGE_1_1, (uint16_t)(tmpStats[20 + 0] >> 16));
  setParamValByAddress(&server, STAT_TEMP_RANGE_1_2, (uint16_t) tmpStats[20 + 0]);
  setParamValByAddress(&server, STAT_TEMP_RANGE_2_1, (uint16_t)(tmpStats[20 + 1] >> 16));
  setParamValByAddress(&server, STAT_TEMP_RANGE_2_2, (uint16_t) tmpStats[20 + 1]);
  setParamValByAddress(&server, STAT_TEMP_RANGE_3_1, (uint16_t)(tmpStats[20 + 2] >> 16));
  setParamValByAddress(&server, STAT_TEMP_RANGE_3_2, (uint16_t) tmpStats[20 + 2]);
  setParamValByAddress(&server, STAT_TEMP_RANGE_4_1, (uint16_t)(tmpStats[20 + 3] >> 16));
  setParamValByAddress(&server, STAT_TEMP_RANGE_4_2, (uint16_t) tmpStats[20 + 3]);
  setParamValByAddress(&server, STAT_TEMP_RANGE_5_1, (uint16_t)(tmpStats[20 + 4] >> 16));
  setParamValByAddress(&server, STAT_TEMP_RANGE_5_2, (uint16_t) tmpStats[20 + 4]);
  setParamValByAddress(&server, STAT_TEMP_RANGE_6_1, (uint16_t)(tmpStats[20 + 5] >> 16));
  setParamValByAddress(&server, STAT_TEMP_RANGE_6_2, (uint16_t) tmpStats[20 + 5]);
  setParamValByAddress(&server, STAT_TEMP_RANGE_7_1, (uint16_t)(tmpStats[20 + 6] >> 16));
  setParamValByAddress(&server, STAT_TEMP_RANGE_7_2, (uint16_t) tmpStats[20 + 6]);
  setParamValByAddress(&server, STAT_TEMP_RANGE_8_1, (uint16_t)(tmpStats[20 + 7] >> 16));
  setParamValByAddress(&server, STAT_TEMP_RANGE_8_2, (uint16_t) tmpStats[20 + 7]);
  setParamValByAddress(&server, STAT_TEMP_RANGE_9_1, (uint16_t)(tmpStats[20 + 8] >> 16));
  setParamValByAddress(&server, STAT_TEMP_RANGE_9_2, (uint16_t) tmpStats[20 + 8]);
  setParamValByAddress(&server, STAT_TEMP_RANGE_10_1,(uint16_t)(tmpStats[20 + 9] >> 16));
  setParamValByAddress(&server, STAT_TEMP_RANGE_10_2,(uint16_t) tmpStats[20 + 9]);

}

void _mainSM(void){

  // Main SM determines if device is PSU or Charger
  // Charging() function works on charging states
  switch(controller.flagModePSU){

  case S_PSU:{
    // power supply mode
    // in this mode, Vout and Iout is directly controlled by params

    controller.LEDBlink = 0;

    if(controller.psuReferenceVoltage >= 1800){
      controller.LEDColor = Cyan;
    }else if(controller.psuReferenceVoltage <= 1600){
      controller.LEDColor = Green;
    }else{
      controller.LEDColor = Purple;
    }

    break;
}
  case S_CHARGER:{
    // Battery Charger mode. It can do Auto Battery Detect and Auto Boost with min boost time.

    // If enabled, check for battery type.
    if(controller.flagModeAuto == ENABLED){

      // Detect and set output voltage and current for battery
      //outputAutoDetect(&measuredValues);

    }else if(controller.flagModeAuto == DISABLED
          && controller.chargeState == FLOAT){

      controller.LEDBlink = 0;

      // If auto detect disabled, then check against output voltage parameter
      if(controller.chargeVoltage >= 1800){
        controller.LEDColor = Cyan;
        if(measuredValues.Vout >= 1800){
          controller.flagWARN = controller.flagWARN & 0xDF; // 6th Warning bit is wrongBattWarn
          return;
        }
      }else if(controller.chargeVoltage <= 1600 && controller.chargeVoltage >= 1201){
        controller.LEDColor = Green;
        if(measuredValues.Vout <= 1600){
          controller.flagWARN = controller.flagWARN & 0xDF; // 6th Warning bit is wrongBattWarn
          return;
        }
      }else{
        controller.LEDColor = Purple;
        controller.flagWARN = controller.flagWARN | 0x20; // 6th Warning bit is wrongBattWarn
      }
    }
    break;
  }
  default:{
    // There must be memory error or debug going on
    controller.LEDColor = Yellow;
    controller.LEDBlink = 0;
    break;
  }
  }
}

void _corePostInit(void){

  _initADC();

  _initModbus();

  _initSystem(); // load parameters

  _initPWM();

  _initTimer();

  _loadSystem();

  // Init the CANbus handle and J1939Module
  __initCAN(&CAN_HANDLE);

  // Disable HW OVER_VOLT to start operating normal
  HAL_GPIO_WritePin(OVER_VOLT_GPIO_Port, OVER_VOLT_Pin, GPIO_PIN_RESET);
}

void _coreLoop(void){
  
  static uint8_t prevReverseVal = 0;
  uint16_t choosenReferenceCurrent = 0;

  //* Reload IWDG to not cause reset of the MCU
  LL_IWDG_ReloadCounter(IWDG);

  // Convert and Filter ADC values to useable values
  updateMeasurements(&measuredValues);

  _checkProtections();

  _mainSM();

  // Update RELAY output
  if(protectionOutputReverse.triggered || protectionOverCurrent.triggered
  || protectionOverTemp.triggered || protectionOverVolt.triggered
  || (protectionOverTempPT100.triggered && controller.flagPt100 && protectionPT100Broken.triggered == 0)
  || (protectionUnderTempPT100.triggered && controller.flagPt100 && protectionPT100Broken.triggered == 0)
  || (protectionTripTemp.triggered && controller.flagPt100 && protectionPT100Broken.triggered == 0)
  || (protectionPT100Broken.triggered && controller.flagPt100)
  || protectionBatteryDisconnect.triggered){

    if(getRelayState() != RELAY_WAITING){
      setRelayState(RELAY_ERROR);
    }
  }else{
    if(getRelayState() != RELAY_WAITING) setRelayState(RELAY_CONDUCTING);
  }
  updateRelay();

  // Update main controller in every 10ms
  if(controllerTimer.state == TIMER_COMPLETE){
    setTimer(&controllerTimer, DEF_CONTROL_TIMER);

    updateMeasurementPT100(&Tpt, &measuredValues.CPT100);
//    // Use PT100 filter for immunity test noise rejection
//    updateMeasurementPT100WithFilter(&Tpt, &measuredValues.CPT100);

    // Calculate cable drop compensation voltage needed
    // This cable drop is used regardless of the device mode: PSU or CHARGER
    // [100mA] * [mV/A] / 1024 = [100mV]
    volatile int32_t cableDrop = ( measuredValues.Iout * controller.cableDrop) >> 10;
    if(cableDrop > MAX_ALLOWED_EXT_DROP) cableDrop = MAX_ALLOWED_EXT_DROP;
    if(cableDrop < 0) cableDrop = 0;

    // Calculate cable drop compensation voltage needed
    // This cable drop is used regardless of the device mode: PSU or CHARGER
    // [100mA] * [mV/A] / 1024 = [100mV]
    volatile int32_t cableDropInterval = ( measuredValues.Iout * controller.cableDropInternal) >> 10;
    if(cableDropInterval > MAX_ALLOWED_INT_DROP) cableDropInterval = MAX_ALLOWED_INT_DROP;
    if(cableDropInterval < 0) cableDropInterval = 0;

    choosenReferenceCurrent = controller.effectiveCurrent;
    // Reset operation mode flags
    isFloatOperationMode = 0;
    isBoostOperationMode = 0;

    if(controller.flagModePSU == S_PSU){
      
      if(controller.flagParallel == ENABLED){
        uint16_t parallel = measuredValues.Iout * controller.parallelCoef >> 7;
        controller.effectiveVoltage = controller.psuReferenceVoltage - parallel + cableDrop + cableDropInterval;
      }else{
        controller.effectiveVoltage = controller.psuReferenceVoltage + cableDrop + cableDropInterval;
      }

    }else if(controller.flagModePSU == S_CHARGER){

      uint16_t choosenReference = 0;

      // Update Effective Voltage Reference
      // If there is no boost but there is parallel connection
      if(controller.flagParallel == ENABLED
      && controller.chargeState == FLOAT){
        uint16_t parallel = measuredValues.Iout * controller.parallelCoef >> 7;
        //parallelWorking(MEASURED);
        choosenReference = controller.chargeVoltage - parallel;

      // if there is boost and parallel connection
      }else if(controller.flagParallel == ENABLED
            && controller.chargeState != FLOAT){
        uint16_t parallel = measuredValues.Iout * controller.parallelCoef >> 7;
        choosenReference = controller.boostVoltage - parallel;

      // If there is no parallel connection but boost
      }else if(controller.flagParallel == DISABLED
          && controller.chargeState != FLOAT){
        
        if(controller.derateStatus == 0){
        	choosenReferenceCurrent = controller.boostMaxCurrent;
        }
        choosenReference = controller.boostVoltage;
        // Set boost operation mode flag
        isBoostOperationMode = 1;
      // If there is no boost and no parallel connection and in FLOAT mode
      }else if(controller.flagParallel == DISABLED
            && controller.chargeState == FLOAT){
        
        // Float current monitoring logic using static variables
        static uint8_t floatMonitoringActive = 0;
        static uint32_t floatMonitoringCounter = 0;
        
        if(floatMonitoringActive == 0){
          // Start monitoring
          floatMonitoringActive = 1;
          floatMonitoringCounter = 0;
          choosenReference = controller.chargeVoltage;
        }else{
          // Check if current exceeds float threshold
          if(measuredValues.Iout > controller.floatCurrentThreshold){
            // Current exceeded threshold, reset monitoring and use charge voltage
            floatMonitoringActive = 0;
            floatMonitoringCounter = 0;
            choosenReference = controller.chargeVoltage;
          }else{
            // Current is below threshold, increment counter
            floatMonitoringCounter++;
            uint32_t floatDurationTime = controller.floatDuration * 6000; // in 10ms increments
            
            // Check if float duration has been reached
            // floatDuration is in minutes, controllerTimer runs every 100ms (10 times per second)
            // So: floatDuration * 60 seconds * 10 increments per second = floatDuration * 600
            if(floatMonitoringCounter >= floatDurationTime){
              // Float duration completed, use float voltage
              // Set float operation mode flag
              isFloatOperationMode = 1;
              choosenReference = controller.floatVoltage;
            }else{
              // Still monitoring, use charge voltage
              choosenReference = controller.chargeVoltage;
            }
          }
        }
        
    }else{
      choosenReference = controller.chargeVoltage;
      }

      // After knowing which voltage to choose for effective reference, take PT100 Compensation into account
      controller.effectiveVoltage = choosenReference + measuredValues.PT100Comp + cableDrop + cableDropInterval;

      if(controller.effectiveVoltage > controller.voltageLimitMax){
    	  controller.effectiveVoltage = controller.voltageLimitMax;
      }
      else if(controller.effectiveVoltage < controller.voltageLimitMin){
    	  controller.effectiveVoltage = controller.voltageLimitMin;
      }

      //if(pt100Flag == 1 && controller.flagPt100 == 0) output_voltage = outputReferences.setted;

    }

    // Check for PT100 trip temperature
    if(protectionTripTemp.triggered 
        && controller.flagModePSU == S_CHARGER 
        && protectionPT100Broken.triggered == 0
        && controller.flagPt100){
      // Set output cutoff to NOT_CONDUCTING when PT100 exceeds trip temperature
    	controller.effectiveVoltage = controller.safetyOutputVoltage;
    	controller.flagERR = controller.flagERR | 0x20; // 6th Error bit is PT100TripTemp
    }else{

      controller.flagERR = controller.flagERR & ~(0x20); // 6th Error bit is PT100TripTemp
    }

    // Update effective current reference this does not change according to S_PSU state
    if(derateTimer.state == TIMER_COMPLETE){
      setTimer(&derateTimer, DEF_BATT_TIMER);
      // ONTC adcRatio can be set to 0 for disabling derating function

      int16_t derateRefVoltage, derateRefCurrent;

      if(controller.flagModePSU == S_CHARGER){
    	  derateRefVoltage = controller.chargeVoltage;
    	  derateRefCurrent = controller.chargeCurrent;
      }
      else{
    	  derateRefVoltage = controller.psuReferenceVoltage;
    	  derateRefCurrent = controller.psuReferenceCurrent;
      }

      controller.derateStatus = updateCurrentByDerate(measuredValues.ntcC, derateRefVoltage, derateRefCurrent);

      if(controller.derateStatus && controller.flagCutOff == NOT_CONDUCTING){
        derateFaultState = 1;
      }
      else{
        derateFaultState = 0;
      }
    }

    // Update battery connection detection every 10ms
    updateBatteryConnectionDetection(&batteryConnectionDetection, &measuredValues);
    // Update battery connection status for protection system
    batteryConnectionStatus = isBatteryDisconnected(&batteryConnectionDetection);
    // Update battery connection status for protection system
    pt100ConnectionStatus = isPT100Broken(measuredValues);

    // Update PID controllers
    updatePID(&currentPID, choosenReferenceCurrent, measuredValues.Iout);
    if(controller.flagPWM == 0) I_PWM_DUTY = currentPID.output;
    //Ornegin 13.8V set degerinde akim modunda calisirken cikis 12V a set etmeye calisinca set edilemedi.
    //Manuel voltaj setti disinda derate gibi fonksiyonlarin yanlis calismasina neden olacaktir. Dolayisiyla voltaj pid her kosulda calismali.
//    if(currentPID.flagAntiWindup == ENABLED) updatePID(&voltagePID, controller.effectiveVoltage, measuredValues.Vout);
    updatePID(&voltagePID, controller.effectiveVoltage, measuredValues.Vout);
    if(controller.flagPWM == 0) V_PWM_DUTY = voltagePID.output;
#ifdef V_PWM_APPLY
    V_PWM_APPLY(); // PID veya Modbus (flagPWM=1) degerini timer'a yaz
#endif

    // Update operating mode for Modbus
    getOperatingMode(&measuredValues);

    // Update monitor voltage for Modbus (voltage without cable drop compensation)
    controller.monitorEffectVolt = controller.effectiveVoltage - cableDropInterval;
  }

  // Boost pin debounce processing - called every millisecond
  if(boostTimer.state == TIMER_COMPLETE){

	setTimer(&boostTimer, DEBOUNCE_TIME_BOOST_PIN);

	if(measuredValues.Vout > controller.floatVoltage){

	    // Read current pin state
	    uint8_t currentPinState = READ_BOOST_PIN;

	    // Debounce logic for boost pin
	    if (currentPinState != boostDebounceLastRead){
	      // Pin state changed, reset debounce counter
	      boostDebounceLastRead = currentPinState;
	      boostDebounceCounter = 0;
	    }
	    else{
	      // Pin state stable, increment counter
	      if (boostDebounceCounter < BOOST_DEBOUNCE_MS){
	        boostDebounceCounter++;
	      }

	      // Check if debounce period completed and state actually changed
	      if (boostDebounceCounter >= BOOST_DEBOUNCE_MS && boostDebounceState != boostDebounceLastRead){
	        boostDebounceState = boostDebounceLastRead;

	        // Check for edge detection after successful debounce
	        uint8_t edgeStateChanged = (boostDebounceState != boostEdgeLastState);

	        if (edgeStateChanged){
	          // Check if the detected edge matches the configured edge selection
	          uint8_t validEdge = 0;
	          if (boostDebounceState == GPIO_PIN_RESET && controller.boostEdgeSelection == FALLING_EDGE){
	            validEdge = 1; // Falling edge detected and configured (FALLING_EDGE = 0)
	          }
	          else if (boostDebounceState == GPIO_PIN_SET && controller.boostEdgeSelection == RISING_EDGE){
	            validEdge = 1; // Rising edge detected and configured (RISING_EDGE = 1)
	          }

	          if (validEdge){
	            controller.flagBoostTriggered = ENABLED;
	            batteryTimer.state = TIMER_COMPLETE;
	          }
	        }
	        boostEdgeLastState = boostDebounceState;
	      }
	    }
	}
	else{
		boostDebounceState = 0;
		boostDebounceLastRead = 0;
		boostDebounceCounter = 0;
		boostEdgeLastState = READ_BOOST_PIN;
	}
  }

  // This function should be called every minutes or so
  // It does timing and measuring battery
  if(batteryTimer.state == TIMER_COMPLETE){

	setTimer(&batteryTimer, DEF_BATT_TIMER);

    if(controller.flagModePSU == S_CHARGER) Charging(&measuredValues);

    if(controller.flagModePSU == S_CHARGER) _checkTemperature(&measuredValues);

    // Update the max MCU temperature variable
    if(measuredValues.INTC > controller.maxTempMCU){
      controller.maxTempMCU = measuredValues.INTC;
      server.saveParam(modbusServergetIndex(&server, TEMP_MAX_MCU), measuredValues.INTC);
    }

    updateStats();

    sendStats();

    controller.flagBoostTriggered = DISABLED;
  }

  // Update LEDs
  if(ledTimer.state == TIMER_COMPLETE){

    setTimer(&ledTimer, LED_DEFAULT_BLINK_TIME);

    if(protectionOverVolt.triggered){
      controller.LEDColor = Red;
      controller.LEDBlink = 2;
    }
    else if(protectionBatteryDisconnect.triggered){
      controller.LEDColor = Red;
      controller.LEDBlink = 3;
    }
    else if(protectionPT100Broken.triggered && controller.flagPt100){
      controller.LEDColor = Red;
      controller.LEDBlink = 4;
    }
    else if(protectionOverTemp.triggered){
      controller.LEDColor = Red;
      controller.LEDBlink = 5;
    }
    else if(protectionTripTemp.triggered && controller.flagPt100 && protectionPT100Broken.triggered == 0){
      controller.LEDColor = Red;
      controller.LEDBlink = 6;
    }
    else if(protectionOverTempPT100.triggered && controller.flagPt100 && protectionPT100Broken.triggered == 0){
      controller.LEDColor = Red;
      controller.LEDBlink = 7;
    }
    else if(protectionUnderTempPT100.triggered && controller.flagPt100 && protectionPT100Broken.triggered == 0){
      controller.LEDColor = Red;
      controller.LEDBlink = 8;
    }
    else if(protectionOverCurrent.triggered){
      controller.LEDColor = Red;
      controller.LEDBlink = 9;
    }

    if(controller.LEDBlink == 0 && protectionOutputReverse.triggered == 0){
    	ledTick = 0;
    	ledToggle = 0;
    	ledReverseTick = 0;
    }
    else if(protectionOutputReverse.triggered){

    	controller.flagCutOff = NOT_CONDUCTING;
        controller.LEDColor = Red;
        ledTick = 0;

		if(ledReverseTick < 5){

			ledReverseTick = 0;
		  if(ledToggle) ledToggle = 0;
		  else          ledToggle = 1;
		}
		else{
			ledReverseTick++;
		}
    }
    else{
    	ledReverseTick = 0;

		if(ledTick < controller.LEDBlink * 2){
		  if(ledToggle) ledToggle = 0;
		  else          ledToggle = 1;
		}

		if(ledTick > (LED_DEFAULT_TIME + controller.LEDBlink * 2)){
			ledTick = 0;
			ledToggle = 0;
		}
		else{
	    	ledTick++;
		}
    }

    if(ledToggle){
      controlLEDbyParam(controller.LEDColor, LED_OFF);
    }else{
      controlLEDbyParam(controller.LEDColor, LED_ON);
    }
  }

  // Check for output reverse protection state change to reset cut-off
  if(prevReverseVal != protectionOutputReverse.triggered && protectionOutputReverse.triggered == 0){
	  controller.flagCutOff = CONDUCTING;
  }
  prevReverseVal = protectionOutputReverse.triggered;

  //dbc2-75w icin test edilen kartlarin eski bootloader ile uyumlu olmasi icin .ld dosyasina bir imza daha eklenmisti.
  //Eklenen imza ile status alanlari calistigi icin imzanin silindigi farkedildi. bu yuzden fonksiyon disable edildi.
//  if(statTimer.state == TIMER_COMPLETE){
//    setTimer(&statTimer, DEF_STAT_TIMER);
//    saveStats();
//  }
}

/*----------------------------------------------------------------------------------------------------------------*/

void Callback1ms(void){

  updateProtection(&protectionOverVolt);
  updateProtection(&protectionOverCurrent);
  updateProtection(&protectionOutputReverse);
  updateProtection(&protectionOverTemp);
  updateProtection(&protectionTripTemp);
  updateProtection(&protectionOverTempPT100);
  updateProtection(&protectionUnderTempPT100);
  updateProtection(&protectionBatteryDisconnect);
  updateProtection(&protectionPT100Broken);

  updateTimer(&batteryTimer);

  updateTimer(&derateTimer);

  updateTimer(&boostTimer);

  updateTimer(&controllerTimer);

  updateTimer(&ledTimer);

  updateTimer(&statTimer);

  // Update Modbus objects' timers for login activity
  modbusServerUpdateTimer(&server);

  // Run the CANbus J1939Module every 1ms for periodic sent functions
  __runCAN();

  if(controller.flagCutOff == CONDUCTING){
    // Disable HW OVER_VOLT to start operating normal
    HAL_GPIO_WritePin(OVER_VOLT_GPIO_Port, OVER_VOLT_Pin, GPIO_PIN_RESET);

#ifdef PCB_VERSION_V7
    HAL_GPIO_WritePin(OUTPUT_HEALTY_GPIO_Port, OUTPUT_HEALTY_Pin, GPIO_PIN_RESET);
#else
    // Update CUT OFF MOSFET GPIO STATE
    HAL_GPIO_TogglePin(OUTPUT_HEALTY_GPIO_Port, OUTPUT_HEALTY_Pin);
#endif

  }else{

#ifdef PCB_VERSION_V7
	  HAL_GPIO_WritePin(OUTPUT_HEALTY_GPIO_Port, OUTPUT_HEALTY_Pin, GPIO_PIN_SET);
#else
    // Update CUT OFF MOSFET GPIO STATE
	  HAL_GPIO_WritePin(OUTPUT_HEALTY_GPIO_Port, OUTPUT_HEALTY_Pin, GPIO_PIN_RESET);
#endif

    // Disable Toggling of Cutoff (???????????????????????????????????????????????????)
//    HAL_GPIO_WritePin(OVER_VOLT_GPIO_Port, OVER_VOLT_Pin, GPIO_PIN_RESET);
    // Enable HW OVER_VOLT Protection
    HAL_GPIO_WritePin(OVER_VOLT_GPIO_Port, OVER_VOLT_Pin, GPIO_PIN_SET);
  }
}

/**
 * @brief callback for gpio perip. It's been used for Boost input pin
 *
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin){

  // Trigger if falling edge detected
  // And start boost input timer
//  if(GPIO_Pin == BOOST_INP){
//    setTimer(&boostTimer, DEBOUNCE_TIME_BOOST_PIN);
//  }
}

/**
 * @brief Determines the current operating mode of the device
 * @param measured Pointer to measured values structure
 * Operating mode value:
 *         0 = PSU Mode
 *         1 = Bulk Mode
 *         2 = Absorption Mode  
 *         3 = Float Mode
 *         4 = Boost Mode
 */
void getOperatingMode(MEASURED_t *measured) {
  // Check if PSU mode is selected
  if (controller.flagModePSU == 1) {
	  measured->operationMode = 0; // PSU Mode
	  return;
  }
  
  // Check if operating in Boost Mode
  if (isBoostOperationMode) {
	  measured->operationMode = 4; // Boost Mode
	  return;
  }
  
  // Check if operating in Float Mode
  if (isFloatOperationMode) {
	  measured->operationMode = 3; // Float Mode
	  return;
  }
  
  // For Bulk and Absorption modes, we need to check voltage and current conditions
  int16_t measurementVout = measured->Vout;
  int16_t measurementIout = measured->Iout;
  int16_t chargeVolt = controller.chargeVoltage;
  int16_t chargeCurrent = controller.chargeCurrent;
  
  // Calculate ±5% tolerance for voltage (chargeVolt ± 5%)
  int16_t voltageToleranceLow = (chargeVolt * 95) / 100;   // 95% of chargeVolt
  int16_t voltageToleranceHigh = (chargeVolt * 105) / 100; // 105% of chargeVolt
  
  // Calculate ±5% tolerance for current (chargeCurrent ± 5%)
  int16_t currentToleranceLow = (chargeCurrent * 95) / 100;   // 95% of chargeCurrent
  int16_t currentToleranceHigh = (chargeCurrent * 105) / 100; // 105% of chargeCurrent
  
  // Check for Absorption Mode:
  // measurementVout is within ±5% of chargeVolt AND measurementIout < chargeCurrent
  if ((measurementVout >= voltageToleranceLow && measurementVout <= voltageToleranceHigh) &&
      (measurementIout < chargeCurrent)) {
	  measured->operationMode = 2; // Absorption Mode
	  return;
  }
  
  // Check for Bulk Mode:
  // measurementVout < chargeVolt AND measurementIout is within ±5% of chargeCurrent
  if ((measurementVout < chargeVolt) &&
      (measurementIout >= currentToleranceLow && measurementIout <= currentToleranceHigh)) {
	  measured->operationMode = 1; // Bulk Mode
	  return;
  }
  
  // Default case - if none of the above conditions are met, return Absorption Mode
  measured->operationMode = 2; // Absorption Mode (default)
}

/*----------------------------------------------------------------------------------------------------------------*/

