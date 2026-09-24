/*
 * Controller.c
 *
 *  Created on: 6 Eki 2022
 *      Author: ceyhun.uysal
 */

 /*-------------------------------------- Includes --------------------------------------*/

#include "Controller.h"

/*-------------------------------------- Private Definitions --------------------------------------*/

extern modbusServerObject_t server;

extern controller_t controller;

// Relay state global variable
RELAY_STATE relayState = RELAY_NOT;

// Timer variables for boost mode
uint32_t t1 = 0, t2 = 0, boostCounter = 0;

// Objects to hold ADC related data and filter accumulator
measurement_t Vout, Iout, Intc, Ontc, Tpt, Vbat;

/*--------------------------------------Function Implementations --------------------------------------*/

void _checkTemperature(MEASURED_t *_measured){

  // If PT100 is not connected
  if(Tpt.flagOpen == ENABLED){

    // But PT100 is activated, we should raise a flag
    if(controller.flagPt100 == ENABLED){
      controller.flagWARN = controller.flagWARN | 0x04; // 3rd Warning bit is PT100Connection
    }else{
      // If it is not activated just ignore it
      controller.flagWARN = controller.flagWARN & ~(0x04); // 3rd Warning bit is PT100Connection
    }

  }else{

    //Activate PT100 in case it is connected
    //controller.flagPt100 = ENABLED;
    controller.flagWARN = controller.flagWARN & ~(0x04); // 3rd Warning bit is PT100Connection

    //if dvdc set
    //modify reference voltage accordingly
    //If battery temp drops below 25C then output should be increased
    //but if it gets hotter we should drop the output voltage
    //so CPT100 - 25 produces negative coef if batt is cold and gets multiplied with DVDT and multiplied with "-"
    //controller.referenceVoltage, outputReferences.settedREFVoltage + (controller.Pt100DVDT * (CPT100 - 25)), CALLBACK_OFF);
    _measured->PT100Comp = -1 * (controller.Pt100DVDT * ( ((_measured->CPT100 - 250) * 51) >> 9));
  }
}

/**
 * @brief return relayState global function for the updateRelay func.
 */
RELAY_STATE getRelayState(void){
  return relayState;
}

/**
 * @brief set relayState global function for the updateRelay func.
 */
void setRelayState(RELAY_STATE r){
  relayState = r;
}

/**
 * @brief Periodic update function for relay state.
 * It checks for current status and updates relay accordingly.
 */
void updateRelay(void){

  controller.flagRelay = RELAY_VALUE;

  switch(relayState){
  case RELAY_CONDUCTING:
    RELAY_ON;
    break;
  case RELAY_NOT:
    RELAY_OFF;
    break;
  case RELAY_WAITING:
    break;
  case RELAY_ERROR:
    RELAY_OFF;
  default:
    break;
  }
}

/**
 * @brief checks for ADC pin possible values returns STAT_TRUE if it is abnormal"
 * 
 * @param _measured 
 * @return AlgorithmStatus 
 */
AlgorithmStatus isADCOpen(const MEASURED_t _measured){
	if(_measured.Vout == 0) return STAT_TRUE;
	if(_measured.CPT100 <= -400 || _measured.CPT100 >= 1500) return STAT_TRUE;
	if(_measured.ntcC == 150) return STAT_TRUE;
	//if(_measured.Iout == 0) return STAT_TRUE;
	//if(_measured.Vbat == 0) return STAT_TRUE;
	//if(_measured.INTC == 0) return STAT_TRUE;
	return STAT_FALSE;
}

/**
 * @brief checks for ADC pin possible values returns STAT_TRUE if it is abnormal"
 *
 * @param _measured
 * @return AlgorithmStatus
 */
AlgorithmStatus isPT100Broken(const MEASURED_t _measured){
	if(_measured.CPT100 <= -400 || _measured.CPT100 >= 1500) return STAT_TRUE;
	return STAT_FALSE;
}

void outputAutoDetect(MEASURED_t *_measured){

  //Check for connected battery voltage
  //if it is out of range, we can't auto detect
  if(_measured->Vout >= 1800){
    controller.chargeVoltage = 2760;
    controller.boostVoltage = 2800;
    if(_measured->Vout >= 1800){
      controller.flagWARN = controller.flagWARN & 0xDF; // 6th Warning bit is wrongBattWarn
    }
  }else if(_measured->Vout <= 1600){
    controller.chargeVoltage = 1380;
    controller.boostVoltage = 1450;
    if(_measured->Vout <= 1600){
      controller.flagWARN = controller.flagWARN & 0xDF; // 6th Warning bit is wrongBattWarn
    }
  }else{
    // If auto mode can't detect batt type
    controller.chargeVoltage = 1200;
    controller.boostVoltage = 1450;
    controller.flagWARN = controller.flagWARN | 0x20; // 6th Warning bit is wrongBattWarn

  }

  controller.flagModeAuto = 0;
  callBackChargerUpdateVoltageREF();
  CallbackUpdateVoltageBoost();
}

void Charging(MEASURED_t *_measured){

  // If External Boost Input is triggered, we should stay in the boost mode until it is removed
	if(controller.flagBoostTriggered == ENABLED){
	  controller.chargeState = BOOST;
	  boostCounter = 0;
    t1 = 0;
    t2 = 0;
	}

	switch (controller.chargeState){

	case BOOST:
		boostCounter++;
		t1++;

		controller.LEDColor = Blue;
    controller.LEDBlink = 2;

		//this multiplier comes from %95 constraint *0.95 ~= 243/256
		if((_measured->Iout <= ((controller.effectiveCurrent * 243 ) >> 8))
				|| (t1 >= controller.boostDuration)){
		  controller.chargeState = EQUALIZATION;
		}

		break;
	case EQUALIZATION:

		boostCounter++;
		t2++;

    controller.LEDColor = Blue;
    controller.LEDBlink = 2;

		if(t2 >= t1 / 2){
			if(t2 >= controller.equalizationTime){
			  controller.chargeState = FLOAT;
			}
		}

		break;
	case FLOAT:
		
	boostCounter++;

	if(boostCounter >= 2 * (t1 + t2)){
		if(boostCounter >= controller.autoStartBoostTime){

			if(controller.flagAutoBoost == ENABLED && controller.flagParallel == DISABLED){

			  controller.chargeState = BOOST;
				boostCounter = 0;
				t1 = 0;
				t2 = 0;
			}
		}
	}

	break;
	default:		

	  // there must be memory error or debug going on
	  controller.LEDColor = Yellow;
    controller.LEDBlink = 2;

		break;
	}
}

void controlLEDbyParam(LEDColorType led, ledStatus_t _set){

  HAL_GPIO_WritePin(LED_R_GPIO_Port, LED_R_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(LED_G_GPIO_Port, LED_G_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(LED_B_GPIO_Port, LED_B_Pin, GPIO_PIN_SET);

  if(_set == LED_OFF) return;

  switch(led){
    case Black:
      break;

    case White:
      HAL_GPIO_WritePin(LED_R_GPIO_Port, LED_R_Pin, GPIO_PIN_RESET);
      HAL_GPIO_WritePin(LED_G_GPIO_Port, LED_G_Pin, GPIO_PIN_RESET);
      HAL_GPIO_WritePin(LED_B_GPIO_Port, LED_B_Pin, GPIO_PIN_RESET);
      break;

    case Red:
      HAL_GPIO_WritePin(LED_R_GPIO_Port, LED_R_Pin, GPIO_PIN_RESET);
      break;

    case Green:
      HAL_GPIO_WritePin(LED_G_GPIO_Port, LED_G_Pin, GPIO_PIN_RESET);
      break;

    case Blue:
      HAL_GPIO_WritePin(LED_B_GPIO_Port, LED_B_Pin, GPIO_PIN_RESET);
      break;

    case Purple:
      HAL_GPIO_WritePin(LED_R_GPIO_Port, LED_R_Pin, GPIO_PIN_RESET);
      HAL_GPIO_WritePin(LED_B_GPIO_Port, LED_B_Pin, GPIO_PIN_RESET);
      break;

    case Yellow:
      HAL_GPIO_WritePin(LED_R_GPIO_Port, LED_R_Pin, GPIO_PIN_RESET);
      HAL_GPIO_WritePin(LED_G_GPIO_Port, LED_G_Pin, GPIO_PIN_RESET);
      break;

    case Cyan:
      HAL_GPIO_WritePin(LED_G_GPIO_Port, LED_G_Pin, GPIO_PIN_RESET);
      HAL_GPIO_WritePin(LED_B_GPIO_Port, LED_B_Pin, GPIO_PIN_RESET);
      break;

    default:
      break;
  }
}

/**
 * @brief  : This function initalises the timer struct
 * @note   : void
 * @retval : void
 */
void initTimer(_timer_t *timer)
{
  timer->count = 0;
  timer->state = TIMER_OFF;
}

/**
 * @brief  : This function sets the timer and starts counting
 * @note   : void
 * @retval : void
 */
void setTimer(_timer_t *timer, long time)
{
  // If delay is non-zero, load the counter and start the timer
  if(time > 0)
  {
    timer->count = time;
    timer->state = TIMER_RUNNING;
  }
  // Otherwise, indicate the zero-time request has elapsed
  else
  {
    timer->state = TIMER_COMPLETE;
  }
}

/**
 * @brief  : This function updates the timer, if running
 * @note   : Must be run from a 1ms TIMX_ISR
 * @retval : void
 */
void updateTimer(_timer_t *timer)
{
  // Check for timer completion
  //*TIMER_RUNNING check added because without it, this function
  //  sets timerLOS.state to TIMER_COMPLETE and this causes misfire on LOS
  if((timer->count == 0) && (timer->state == TIMER_RUNNING))
  {
    timer->state = TIMER_COMPLETE;
  }
  // Decrement the timer if it is running
  if(timer->state == TIMER_RUNNING)
  {
    timer->count--;
  }
}

/**
 * @brief Initializes the protection
 */
void initProtection(protection_t *prot, int16_t *input, int16_t threshold, uint16_t delay, uint8_t mode){
  prot->delay = delay;
  prot->mode = mode;
  prot->input = input;
  prot->threshold = threshold;
  prot->triggered = 0;
  initTimer(&prot->timer);
}

/**
 * @brief Clears the error flag
 */
void resetProtection(protection_t* prot){

  prot->triggered = 0;
  initTimer(&prot->timer);
}

/**
 * @brief Update the error flag of protection according to input and threshold
 */
void updateProtection(protection_t *prot){

  // Check for the protection condition, based on the mode
  if(((prot->mode == MODE_UNDER) && (*(prot->input) < prot->threshold)) // Under mode
  || ((prot->mode == MODE_OVER ) && (*(prot->input) > prot->threshold)) // Over mode
  ){
    // Start the delayTimer if it is off
    if(prot->timer.state == TIMER_OFF){

      setTimer(&prot->timer, prot->delay);
    }

    // If the timer is started running then update the timer
    if(prot->timer.state == TIMER_RUNNING){

      updateTimer(&prot->timer);
    }

    // Set the flag after delayTime has elapsed
    if(prot->timer.state == TIMER_COMPLETE){

      prot->triggered = 1;
    }
  }else{

    // Reset the protection
    resetProtection(prot);
  }
}

/**
 * @brief Function to update measurement struct's avgValue according to related fields
 */
void updateMeasurement(measurement_t* m, int16_t *val){

  // To update measurement object, first do the ADC operations
  //   of offseting  and calibrating
  int32_t dummy = *(m->adcValue);
  dummy -= m->adcOffset;
  dummy *= m->adcRatio;
  // After that normalise the ADC value to 12Bit value
  dummy = dummy >> m->adcShift;
  // Check against negative offset. 0x7FFFFFFF is biggest positive number on 32Bit register.
  if(dummy > 0x7FFFFFFF) dummy = 0;

  // Now apply the IIR filter to measurement object
  // Exponential Weigthed Moving Average Filter
  // First increase quantisation to q10b, then run the 16-sample IIR filter
  m->filterCounter = ((31 * m->filterCounter) + (dummy << 6)) >> 5;
  m->avgValue = m->filterCounter >> 6; // taking out the q6b resolution improvement

  // Update the user value with new measurement
  *val = m->avgValue;

}

/**
 * @brief Initialise DBC2 measurement objects
 */
void initMeasurements(void){
  int16_t INTC_CAL1 = 0, INTC_CAL2  = 0;
  //ADC Calibration for internal NTC Temp
  INTC_CAL1 = *(int16_t*)0x1FFFF7B8;
  INTC_CAL2 = *(int16_t*)0x1FFFF7C2;

  Intc.adcValue = &ADC_VAL_INTC;
  Intc.adcOffset = INTC_CAL1;
  Intc.adcRatio = ((110.0 - 30.0) / (INTC_CAL2 - INTC_CAL1)) * 1024;

  Vout.adcValue = &ADC_VAL_VOUT;
  Iout.adcValue = &ADC_VAL_IOUT;
  Intc.adcValue = &ADC_VAL_INTC;
  Ontc.adcValue = &ADC_VAL_ONTC;
  Tpt.adcValue  = &ADC_VAL_PT100;
  Vbat.adcValue = &ADC_VAL_VBAT;
  Tpt.flagOpen = ENABLED; // Should be enabled by default
}


/**
 * @brief Function to update PT100 measurement struct's avgValue with q8b filter
 */
void updateMeasurementPT100(measurement_t* m, int16_t *val){

  // To update measurement object, first do the ADC operations
  //   of offseting  and calibrating
  int32_t dummy = *(m->adcValue);
  dummy -= m->adcOffset;
  dummy *= m->adcRatio;
  // After that normalise the ADC value to 12Bit value
  dummy = dummy >> m->adcShift;
  // Check against negative offset. 0x7FFFFFFF is biggest positive number on 32Bit register.
  if(dummy > 0x7FFFFFFF) dummy = 0;

  // Now apply the IIR filter to measurement object
  // Exponential Weigthed Moving Average Filter
  // First increase quantisation to q8b for PT100, then run the 16-sample IIR filter
  m->filterCounter = ((127 * m->filterCounter) + (dummy << 8)) >> 7;
  m->avgValue = m->filterCounter >> 8; // taking out the q8b resolution improvement

  // Update the user value with new measurement
  *val = m->avgValue;
}


/**
 * @brief Function to update PT100 measurement with noise filtering
 *
 * This function addresses PT100 immunity test noise issues by implementing:
 * 1. Median filter (5 samples) - Removes impulse noise spikes
 * 2. Aggressive low-pass IIR filter - Smooths remaining noise
 * 3. Outlier detection - Rejects readings that change >50°C instantly
 *
 * Problem solved: During immunity tests, PT100 shows wrong values (-4, 0, -3°C)
 * instead of normal 25°C due to noise. This filter uses integer-only math
 * to provide robust temperature reading even during high noise conditions.
 * @param m Pointer to measurement object
 * @param val Pointer to store the filtered temperature value
 */
void updateMeasurementPT100WithFilter(measurement_t* m, int16_t *val){

  static int16_t filterBuffer[5] = {0, 0, 0, 0, 0};  // Median filter buffer
  static uint8_t bufferIndex = 0;                    // Buffer index
  static int32_t aggressiveFilter = 0;               // Aggressive IIR filter accumulator
  static uint8_t filterInitialized = 0;             // Initialization flag

  // To update measurement object, first do the ADC operations
  //   of offseting  and calibrating
  int32_t dummy = *(m->adcValue);
  dummy -= m->adcOffset;
  dummy *= m->adcRatio;
  // After that normalise the ADC value to 12Bit value
  dummy = dummy >> m->adcShift;
  // Check against negative offset. 0x7FFFFFFF is biggest positive number on 32Bit register.
  if(dummy > 0x7FFFFFFF) dummy = 0;

  // Store current reading in circular buffer for median filtering
  filterBuffer[bufferIndex] = (int16_t)dummy;
  bufferIndex = (bufferIndex + 1) % 5;

  // Calculate median of last 5 samples (simple sorting for 5 elements)
  int16_t sortedBuffer[5];
  for(uint8_t i = 0; i < 5; i++){
    sortedBuffer[i] = filterBuffer[i];
  }

  // Simple bubble sort for 5 elements (optimized for small size)
  for(uint8_t i = 0; i < 4; i++){
    for(uint8_t j = 0; j < 4 - i; j++){
      if(sortedBuffer[j] > sortedBuffer[j + 1]){
        int16_t temp = sortedBuffer[j];
        sortedBuffer[j] = sortedBuffer[j + 1];
        sortedBuffer[j + 1] = temp;
      }
    }
  }

  int16_t medianValue = sortedBuffer[2]; // Middle value is median

  // Initialize aggressive filter on first run
  if(!filterInitialized){
    aggressiveFilter = medianValue << 10; // q10b for more precision
    filterInitialized = 1;
  }

  // Apply aggressive low-pass filter: y[n] = 0.96875 * y[n-1] + 0.03125 * x[n]
  // Using integer math: (31/32) * old + (1/32) * new
  aggressiveFilter = ((31 * aggressiveFilter) + (medianValue << 10)) >> 5;

  // Extract filtered value
  int16_t filteredValue = aggressiveFilter >> 10;

  // Additional outlier detection and rejection
  static int16_t lastValidValue = 250; // Start with 25C as default
  int16_t valueDiff = filteredValue - lastValidValue;

  // If change is too large (more than 50C in one reading), it's likely noise
  if(valueDiff > 500 || valueDiff < -500){
    filteredValue = lastValidValue; // Keep previous valid value
  } else {
    lastValidValue = filteredValue; // Update last valid value
  }

  // Update measurement object with filtered value
  m->avgValue = filteredValue;

  // Update the user value with new measurement
  *val = m->avgValue;
}




//void lowPassFilterLvl5(filterCalculate* obj, int inputVal){
//  obj->data    = inputVal;
//  obj->filter  = ((31 * obj->filter) + (obj->data << 6)) >> 5;
//  obj->avg     = obj->filter >> 6;
//}
//
//void lowPassFilterLvl6(filterCalculate* obj, int inputVal){
//  obj->data    = inputVal;
//  obj->filter  = ((63 * obj->filter) + (obj->data << 7)) >> 6;
//  obj->avg     = obj->filter >> 7;
//}
//
//void lowPassFilterLvl7(filterCalculate* obj, int inputVal){
//  obj->data    = inputVal;
//  obj->filter  = ((127 * obj->filter) + (obj->data << 8)) >> 7;
//  obj->avg     = obj->filter >> 8;
//}
//
//void lowPassFilterLvl8(filterCalculate* obj, int inputVal){
//  obj->data    = inputVal;
//  obj->filter  = ((255 * obj->filter) + (obj->data << 9)) >> 8;
//  obj->avg     = obj->filter >> 9;
//}


/**
 * @brief load calibrations from modbus if there are any calibrations saved before
 */
void setMeasurements(void){
  int16_t INTC_CAL1 = 0, INTC_CAL2  = 0;
  //ADC Calibration for internal NTC Temp
  INTC_CAL1 = *(int16_t*)0x1FFFF7B8;
  INTC_CAL2 = *(int16_t*)0x1FFFF7C2;

  Intc.adcValue = &ADC_VAL_INTC;
  Intc.adcOffset = INTC_CAL1;
  Intc.adcRatio = ((110.0 - 30.0) / (INTC_CAL2 - INTC_CAL1)) * 1024;
  Intc.adcShift = DEF_SHIFT_INTC;

  Vout.adcValue = &ADC_VAL_VOUT;
  Iout.adcValue = &ADC_VAL_IOUT;
  Ontc.adcValue = &ADC_VAL_ONTC;
  Tpt.adcValue  = &ADC_VAL_PT100;
  Vbat.adcValue = &ADC_VAL_VBAT;
}

/**
 * @brief Update measured struct according to DBC2 measurements objects
 */
void updateMeasurements(MEASURED_t *m){

  // Update ADC monitoring parameters too

  // First update measurement object, then set m pointer with its value
  // TODO MEASURED_t should be replaced by measurement_t
  updateMeasurement(&Vout, &m->Vout);
  updateMeasurement(&Iout, &m->Iout);

  // 6k8 18k yapildi. kazanc artinca opamp cikisinda giriste akim olmasa da deger veriyor.
  // 600mA'in altini okuma
  if(m->Iout <= 60) {
	  m->Iout = 0;
  }

  updateMeasurement(&Vbat, &m->Vbat);

//  if(*Intc.adcValue < Intc.adcOffset){
//    updateMeasurement(&Intc, &m->INTC);
//  }else{
//    m->INTC = 30;
//  }

  m->INTC = __LL_ADC_CALC_TEMPERATURE(3300, *Intc.adcValue, LL_ADC_RESOLUTION_12B);

  Ontc.avgValue = m->ntcC = getNTC(*Ontc.adcValue, Ontc.adcOffset, Ontc.adcRatio);

  if(*Tpt.adcValue < DEF_ADCMIN_PT100 || *Tpt.adcValue > DEF_ADCMAX_PT100 ){
    Tpt.flagOpen = 1;
  }else{
    Tpt.flagOpen = 0;
  }

}

/**
 * @brief Function to calculate current based on derate
 */
uint8_t updateCurrentByDerate(int16_t temperature, uint16_t vOutSet, uint16_t iOutSet) {

  uint8_t derateCondition = 0;
  // Normalize temperature measurement to degrees from degrees x10
  temperature /= 10;

  // Constants
  const int16_t slope = -375;         // Slope (375W/°C)
  const uint16_t yIntercept = 35625;  // Y-intercept value (b) (42375W)
  uint16_t iEffect = 0;
  static uint8_t isDerated = 0;
  // Derating state machine
  if(temperature <= POWER_DERATE_START_TEMP) {
    isDerated = 0;
    controller.effectiveCurrent = iOutSet;
//    controller.flagWARN = controller.flagWARN & 0xFD; // 2nd Warning bit is overtempwarn
    controller.flagCutOff = CONDUCTING;
    derateCondition = 0;
  }else if(temperature > POWER_DERATE_START_TEMP && temperature <= POWER_DERATE_END_TEMP && isDerated == 0) {
//    controller.flagWARN = controller.flagWARN | 0x02; // 2nd Warning bit is overtempwarn
    // Calculate power: P = m * T + b
    // Since integer operations are used, we need to ensure correct calculation.
    int32_t power = (slope * temperature + yIntercept) * 100;
    // If the power is negative, set it to zero, because power cannot be negative
    if (power < 0) {power = 0;}
    iEffect = power / vOutSet;

    if(iEffect < 0){
      controller.effectiveCurrent = 0;
    }else if(iEffect >= 0 && iEffect <= iOutSet){
      controller.effectiveCurrent = iEffect;
    }else if(iEffect > iOutSet){
      controller.effectiveCurrent = iOutSet;
    }
    derateCondition = 1;
  }else {
    isDerated = 1;
//    controller.flagWARN = controller.flagWARN | 0x02; // 2nd Warning bit is overtempwarn
    controller.flagCutOff = NOT_CONDUCTING;
    derateCondition = 1;
  }

  return derateCondition;
}

/**
 * @brief Get temperature from ADC value using a 64-point lookup table with integer-only math
 * For reversed NTC circuit (NTC to VCC, series resistor to GND)
 *
 * @param adc_raw: Raw ADC value from the MCU
 * @param adc_offset: Calibration offset to be added to raw ADC value
 * @param adc_ratio: Calibration multiplier in 0.1% (1024 = 100%)
 * @return: Temperature in Celsius × 10 (e.g., 100 means 10.0°C)
 */
int16_t getNTC(uint16_t adc, int16_t offset, uint16_t ratio) {
  // Apply calibration to ADC value
  adc = ((adc - offset) * ratio) >> 10;

  // Clamp the ADC value to valid range
  if(adc >= NTC_Lookup[0].adc) {
      return NTC_Lookup[0].temp;  // Highest ADC is lowest temperature
  }else if(adc <= NTC_Lookup[63].adc) {
      return NTC_Lookup[63].temp; // Lowest ADC is highest temperature
  }else {
    // Binary search to find the closest entries in the lookup table
    uint8_t left = 0;
    uint8_t right = 63;
    uint8_t mid;

    while (right - left > 1) {
      mid = (left + right) >> 1; // Divide by 2 using bit shift
      if (NTC_Lookup[mid].adc < adc) {
        right = mid;  // Key change: reversed comparison
      }else {
        left = mid;
      }
    }

    // Linear interpolation between the two closest points
    // Using integer math: result = y1 + (y2-y1) * (x-x1) / (x2-x1)
    uint16_t adc_high   = NTC_Lookup[left].adc;  // Changed from adc_low
    uint16_t adc_low    = NTC_Lookup[right].adc; // Changed from adc_high
    int16_t  temp_high  = NTC_Lookup[left].temp; // Changed from temp_low
    int16_t  temp_low   = NTC_Lookup[right].temp; // Changed from temp_high

    // Calculate the interpolated temperature using integer math
    // Use 32-bit integers for the calculation to avoid overflow
    // Note the reversed interpolation due to inverse relationship
    int32_t temp = temp_high -
        ((int32_t)(temp_high - temp_low) * (int32_t)(adc_high - adc)) /
        (int32_t)(adc_high - adc_low);

    return (int16_t)temp;
  }
}

/*-------------------------------------- Battery Connection Detection --------------------------------------*/

/**
 * @brief Initialize battery connection detection system
 * 
 * @param ctx Pointer to battery connection detection context
 */
void initBatteryConnectionDetection(batConnDetection_t *ctx) {
  if (ctx == NULL) return;
  
  ctx->state = BAT_CONN_IDLE;
  initTimer(&ctx->intervalTimer);
  initTimer(&ctx->monitorTimer);
  ctx->storedVoltage = 0;
  ctx->storedCurrent = 0;
  ctx->measuredVoltage = 0;
  ctx->measuredCurrent = 0;
  ctx->failedCheckCount = 0;
  ctx->firstFailTimestamp = 0;
  ctx->batteryNotConnected = 0;
  ctx->detectionEnabled = 0;
  
  // Start the interval timer for first check
  setTimer(&ctx->intervalTimer, BAT_CONN_CHECK_INTERVAL_MS);
}

/**
 * @brief Reset battery connection detection state
 * 
 * @param ctx Pointer to battery connection detection context
 */
void resetBatteryConnectionDetection(batConnDetection_t *ctx) {
  if (ctx == NULL) return;
  
  ctx->failedCheckCount = 0;
  ctx->firstFailTimestamp = 0;
  ctx->batteryNotConnected = 0;
  ctx->state = BAT_CONN_IDLE;
  initTimer(&ctx->intervalTimer);
  setTimer(&ctx->intervalTimer, BAT_CONN_CHECK_INTERVAL_MS);
}

/**
 * @brief Enable or disable battery connection detection
 * 
 * @param ctx Pointer to battery connection detection context
 * @param enable 1 to enable, 0 to disable
 */
void enableBatteryConnectionDetection(batConnDetection_t *ctx, uint8_t enable) {
  if (ctx == NULL) return;
  
  ctx->detectionEnabled = enable ? 1 : 0;
  
  if (enable) {
    resetBatteryConnectionDetection(ctx);
  }
}

/**
 * @brief Get battery connection status
 * 
 * @param ctx Pointer to battery connection detection context
 * @return uint8_t 1 if battery not connected, 0 if connected
 */
uint8_t isBatteryDisconnected(const batConnDetection_t *ctx) {
  if (ctx == NULL) return 0;
  return ctx->batteryNotConnected;
}

volatile int32_t voltageRatio, normmallyCurrent;
/**
 * @brief Helper function to evaluate battery connection based on measurements
 * 
 * @param ctx Pointer to battery connection detection context
 * @return BAT_CONN_RESULT Detection result
 */
static BAT_CONN_RESULT _evaluateBatteryConnection(batConnDetection_t *ctx) {
  
  // Check if voltage reached target (12V ± 100mV)
  if (ctx->measuredVoltage > (1200 + BAT_CONN_VOLTAGE_THRESHOLD_MV)) {
    // Voltage didn't reach target - battery not connected
    return BAT_CONN_RESULT_CONNECTED;
  }

  voltageRatio = ((ctx->storedVoltage * 10) / ctx->storedCurrent);
  normmallyCurrent = ((ctx->measuredVoltage / voltageRatio) * 95) / 10;
  
  // If ratios are similar (within tolerance), no battery
  if (ctx->measuredCurrent > normmallyCurrent
		  || (ctx->storedCurrent < BAT_CONN_MIN_CURRENT_TRESHOLD && ctx->measuredCurrent < BAT_CONN_MIN_CURRENT_TRESHOLD)) {  // 100mA
    return BAT_CONN_RESULT_NOT_CONNECTED;
  }
  // If current ratio is significantly higher, battery is connected
  else{
    return BAT_CONN_RESULT_CONNECTED;
  }

}

/**
 * @brief Main battery connection detection function
 * Must be called periodically (e.g., every 1ms from timer interrupt)
 * 
 * @param ctx Pointer to battery connection detection context
 * @param measured Pointer to current measurements
 * @return BAT_CONN_RESULT Current detection result
 */
BAT_CONN_RESULT updateBatteryConnectionDetection(batConnDetection_t *ctx, MEASURED_t *measured) {
  static uint32_t systemTimestamp = 0;
  BAT_CONN_RESULT result = BAT_CONN_RESULT_INDETERMINATE;
  extern controller_t controller;

  
  // Null pointer check
  if (ctx == NULL || measured == NULL) {
    return BAT_CONN_RESULT_INDETERMINATE;
  }
  
  // If detection is disabled, return connected status
  if (ctx->detectionEnabled == 0) {
    return BAT_CONN_RESULT_CONNECTED;
  }
  
  // Increment system timestamp (called every 10ms)
  systemTimestamp++;
  
  // Update interval timers
  updateTimer(&ctx->intervalTimer);
  
  // State machine for battery connection detection
  switch (ctx->state) {
    case BAT_CONN_IDLE:

        // Wait for interval timer to complete
        if(ctx->intervalTimer.state == TIMER_COMPLETE) {
          // Check if conditions are met for detection
		  if ((measured->Vout >= (controller.effectiveVoltage - BAT_CONN_VOLTAGE_CONDITION_MV)) && controller.flagModePSU == S_CHARGER) {
          // Start detection process
          ctx->state = BAT_CONN_STORE_VALUES;
        }
		else{
        	systemTimestamp = 0;
        	resetBatteryConnectionDetection(ctx);
          // Conditions not met, restart interval timer
          setTimer(&ctx->intervalTimer, BAT_CONN_CHECK_INTERVAL_MS);
        }
      }
      break;
      
    case BAT_CONN_STORE_VALUES:
      // Store current voltage and current values
      ctx->storedVoltage = measured->Vout;
      ctx->storedCurrent = measured->Iout;
      
      // Move to next state
      ctx->state = BAT_CONN_APPLY_VOLTAGE;
      break;
      
    case BAT_CONN_APPLY_VOLTAGE:
      // Set charger output to test voltage (12V)
      // Note: This assumes there's a function to set the reference voltage
      // You may need to modify this based on your actual implementation
      ctx->storedEffectiveVoltage = controller.effectiveVoltage;
      controller.effectiveVoltage = BAT_CONN_TEST_VOLTAGE_MV;
      // Start monitoring timer
      setTimer(&ctx->monitorTimer, BAT_CONN_CHECK_DURATION_MS);
      
      // Move to monitoring state
      ctx->state = BAT_CONN_MONITORING;
      break;
      
    case BAT_CONN_MONITORING:
      // Update monitor timer
      updateTimer(&ctx->monitorTimer);
      controller.effectiveVoltage = BAT_CONN_TEST_VOLTAGE_MV;

      // Wait for monitoring duration to complete
      if (ctx->monitorTimer.state == TIMER_COMPLETE) {
        // Store measured values after test period
        ctx->measuredVoltage = measured->Vout;
        ctx->measuredCurrent = measured->Iout;

        controller.effectiveVoltage = ctx->storedEffectiveVoltage;

        // Move to evaluation state
        ctx->state = BAT_CONN_EVALUATE;

      }
      break;
      
    case BAT_CONN_EVALUATE:
      // Evaluate the battery connection
      result = _evaluateBatteryConnection(ctx);
      
      // Handle result
      if (result == BAT_CONN_RESULT_NOT_CONNECTED) {
        // Increment failed check counter
        ctx->failedCheckCount++;
        
        // If this is the first failed check, record timestamp
        if (ctx->failedCheckCount == 1) {
          ctx->firstFailTimestamp = systemTimestamp;
        }
        
        // Check if we've exceeded the time window (1 hour)
        if ((systemTimestamp - ctx->firstFailTimestamp) > BAT_CONN_CHECK_WINDOW_MS) {
          // Time window exceeded, reset counter
          ctx->failedCheckCount = 1;
          ctx->firstFailTimestamp = systemTimestamp;
        }
        
        // Check if we've reached the maximum failed checks
        if (ctx->failedCheckCount >= BAT_CONN_MAX_FAILED_CHECKS) {
          // Report battery not connected
          ctx->batteryNotConnected = 1;
          ctx->failedCheckCount = 0;
          ctx->firstFailTimestamp = 0;
          systemTimestamp = 0;
        }
      } else if (result == BAT_CONN_RESULT_CONNECTED) {
        // Battery is connected, reset counters
        ctx->failedCheckCount = 0;
        ctx->firstFailTimestamp = 0;
        ctx->batteryNotConnected = 0;
        systemTimestamp = 0;
        resetBatteryConnectionDetection(ctx);
      }
      // If indeterminate, don't change counters
      
      // Return to idle state and restart interval timer
      ctx->state = BAT_CONN_IDLE;
      setTimer(&ctx->intervalTimer, BAT_CONN_CHECK_INTERVAL_MS);
      break;
      
    default:
      // Invalid state, reset to idle
      ctx->state = BAT_CONN_IDLE;
      setTimer(&ctx->intervalTimer, BAT_CONN_CHECK_INTERVAL_MS);
      break;
  }
  
  // Return current battery connection status
  if (ctx->batteryNotConnected && controller.flagModePSU == S_CHARGER) {
    return BAT_CONN_RESULT_NOT_CONNECTED;
  } else {
    return BAT_CONN_RESULT_CONNECTED;
  }
}

/**
  * @brief  Configures the memory mapping at address 0x00000000.
  * @param  SYSCFG_MemoryRemap: selects the memory remapping.
  *          This parameter can be one of the following values:
  *            @arg SYSCFG_MemoryRemap_Flash: Main Flash memory mapped at 0x00000000
  *            @arg SYSCFG_MemoryRemap_SystemMemory: System Flash memory mapped at 0x00000000
  *            @arg SYSCFG_MemoryRemap_SRAM: Embedded SRAM mapped at 0x00000000
  * @retval None
  */
void SYSCFG_MemoryRemapConfig(uint32_t SYSCFG_MemoryRemap)
{
  uint32_t tmpctrl = 0;

  /* Check the parameter */
  assert_param(IS_SYSCFG_MEMORY_REMAP(SYSCFG_MemoryRemap));

  /* Get CFGR1 register value */
  tmpctrl = SYSCFG->CFGR1;

  /* Clear MEM_MODE bits */
  tmpctrl &= (uint32_t) (~SYSCFG_CFGR1_MEM_MODE);

  /* Set the new MEM_MODE bits value */
  tmpctrl |= (uint32_t) SYSCFG_MemoryRemap;

  /* Set CFGR1 register with the new memory remap configuration */
  SYSCFG->CFGR1 = tmpctrl;
}

/**
  * @brief  Forces or releases High Speed APB (APB2) peripheral reset.
  * @param  RCC_APB2Periph: specifies the APB2 peripheral to reset.
  *          This parameter can be any combination of the following values:
  *             @arg RCC_APB2Periph_SYSCFG: SYSCFG clock
  *             @arg RCC_APB2Periph_ADC1:   ADC1 clock
  *             @arg RCC_APB2Periph_TIM1:   TIM1 clock
  *             @arg RCC_APB2Periph_SPI1:   SPI1 clock
  *             @arg RCC_APB2Periph_USART1: USART1 clock
  *             @arg RCC_APB2Periph_TIM15:  TIM15 clock
  *             @arg RCC_APB2Periph_TIM16:  TIM16 clock
  *             @arg RCC_APB2Periph_TIM17:  TIM17 clock
  *             @arg RCC_APB2Periph_DBGMCU: DBGMCU clock
  * @param  NewState: new state of the specified peripheral reset.
  *          This parameter can be: ENABLE or DISABLE.
  * @retval None
  */
void RCC_APB2PeriphResetCmd(uint32_t RCC_APB2Periph, FunctionalState NewState)
{
  /* Check the parameters */
  assert_param(IS_RCC_APB2_PERIPH(RCC_APB2Periph));
  assert_param(IS_FUNCTIONAL_STATE(NewState));

  if (NewState != DISABLE)
  {
    RCC->APB2RSTR |= RCC_APB2Periph;
  }
  else
  {
    RCC->APB2RSTR &= ~RCC_APB2Periph;
  }
}
