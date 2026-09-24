/*
 * Controller.c
 *
 *  Created on: 6 Eki 2022
 *      Author: ceyhun.uysal
 */
#include "Controller.h"

ControllerParam_t outputReferences;

//LED Toggle Control flag
uint32_t LED_OK2 = 1, LED_OK = 1;
//It's been used to count how much time hs been passed since we reached low end of voltage pwm
volatile uint32_t overVoltageTick = 0;
//max and min values for pwm limits. They are used for limiting the instant changing of output
volatile uint32_t VPWMHighLimit = 959, VPWMLowLimit = 0;
volatile uint32_t IPWMHighLimit = 959, IPWMLowLimit = 0;

volatile int64_t boostCounter = 0;
volatile int64_t t1 = 0, t2 = 0;
volatile int16_t oldV = 0;
uint8_t tempFlag = 0;

//To keep track temperature derate status. Doesn't need to be saved on flash
volatile uint32_t isDerated = 0;
//derate debounce flag. Doesn't need to be saved or shared outside. Only used in derator()
//volatile static uint32_t flag = 0;



/**
 * @brief main controller function for output. It runs every ms and checks for the outputreferences.REF values.
 * 
 */
void outputController(const MEASURED_t _measured){

	if(_measured.Vout - getParamDataViaPN(OUTPUT_VOLTAGE) > 10){ //İstenenden 100mV fazla
		if(Voltage_PWM > VPWMLowLimit){
			Voltage_PWM -= 1;
			overVoltageTick = 0;
		}else{ //This state means we can't lower the output below, so it might be on overVoltageError
			overVoltageTick++;
		}
	}
	else if( _measured.Vout - getParamDataViaPN(OUTPUT_VOLTAGE)  < -10){ //İstenenden 100mV az
		if(Voltage_PWM < VPWMHighLimit){
			Voltage_PWM += 1;
			overVoltageTick = 0;
		}
	}else{
		overVoltageTick = 0;
	}
	/*
	//Bu fonksiyon eklendiğinde fişten sökülürse akım pwmi kısılıyor
	//Kısıkken fişe takılırsa gerilim doğru olduğu için akım limiti değişmiyor 
	//Bu kullanıcının istediği şey olmayabilir
	else{
		//when desired current can be drawn with this voltage don't touch current pwm
		//if voltage is not right you should check if current is right
		//It could be on current limit
		return;
	}*/

	//when power derated, control should stop checking current
	//if(isDerated == 1) return;

	if(_measured.Iout - outputReferences.REFCurrent > 10){ //0.1A more than reference
		if(Current_PWM < IPWMHighLimit){
			Current_PWM += 10; //It works reversed
		}
	}
	else if(_measured.Iout - outputReferences.REFCurrent < -10){ //0.1A less than reference
		if(Current_PWM > IPWMLowLimit){
			Current_PWM -= 1; //It works reversed
		}
	}
}

/**
 * @brief Set the output voltage PWM value according to input param
 * 
 * @param _voltage expected voltage value * 100
 */
void setVoltagePWM(const int16_t _voltage){

	uint32_t calc = _voltage * VOLTAGE_X;
	if(calc > VOLTAGE_100Y){
		calc -= VOLTAGE_100Y;
	}else{
		calc = 0;
	}
	Voltage_PWM = calc;
	updateVPWMLimits();
}

/**
 * @brief Set the output voltage PWM value according to input param
 * 
 * @param value desired output current * 100
 */
void setCurrentPWM(const int16_t value){

	uint32_t calc;
	calc = value / 100.0 * CURRENT_X;
	if(calc < CURRENT_100Y){
		calc = CURRENT_100Y - calc;
	}else{
		calc = 0;
	}
	Current_PWM = calc;
	updateIPWMLimits();
}

/**
 * @brief Set the Voltage pwm to the param voltage
 * 
 */
void setVoltagePWMtoRef(const ControllerParam_t* _references){
	if(getParamDataViaPN(OUTPUT_VOLTAGE) < MIN_VOLTAGE || getParamDataViaPN(OUTPUT_VOLTAGE) > MAX_VOLTAGE) return;
	setVoltagePWM(getParamDataViaPN(OUTPUT_VOLTAGE));
}

/**
 * @brief Set the Current Pwm to outputreferences.REFCurrent
 * 
 */
void setCurrentPWMtoRef(const ControllerParam_t* _references){
	if(_references->REFCurrent > MAX_CURRENT) return;
	setCurrentPWM(_references->REFCurrent);
}

/**
 * @brief Power control function to limit output power according to ntc temperature. It should be called every second or so.
 * 
 */
uint8_t powerDerator(const MEASURED_t _measured){

	//controlling updated references doesn't needed in here because every update callback checks the power.
	//controlling output power right at the moment doesn't needed because voltage and current limits on TSM1011 will hold output if configured properly
	//Only control on the temperature derate should be implemented here

	//!Not Derating
	if(_measured.ntcC <= POWER_DERATE_START_TEMP){
		if(tempFlag == 1){//It is derated previously and there is already setted refCurrent
			 if(outputReferences.REFCurrent + 100 <= outputReferences.settedREFCurr){//Increase the output till reaching previous point
				 if(_measured.ntcC > POWER_DERATE_HIST_TEMP) return isDerated;
				 setParamDataViaPN(OUTPUT_CURRENT, getParamDataViaPN(OUTPUT_CURRENT) + 100, CALLBACK_ON);
			 }else{ tempFlag = 0; }//When output gets previous point, release the settedRefCurr because it can get updated by user.
		}

		errorHandler(overTempWarn_false);
		errorHandler(overTempError_false);
		isDerated = 0;
		return isDerated;
	}

	//!Just derating
	isDerated = 1;
	if(tempFlag == 0){//If it gets derated first time ref current should be saved somewhere (a.k.a settedRefCurr)
		tempFlag = 1;
		//store for later use
		outputReferences.settedREFCurr = outputReferences.REFCurrent;
	}

	/*
	if(_measured.ntcC == POWER_DERATE_START_TEMP + 1){
		setParamDataViaPN(OUTPUT_CURRENT, getParamDataViaPN(OUTPUT_CURRENT) - 100, CALLBACK_ON);
	}else if(_measured.ntcC == POWER_DERATE_START_TEMP + 2){
		setParamDataViaPN(OUTPUT_CURRENT, getParamDataViaPN(OUTPUT_CURRENT) - 200, CALLBACK_ON);
	}else if(_measured.ntcC == POWER_DERATE_START_TEMP + 3){
		setParamDataViaPN(OUTPUT_CURRENT, getParamDataViaPN(OUTPUT_CURRENT) - 300, CALLBACK_ON);
	}else if(_measured.ntcC == POWER_DERATE_START_TEMP + 4){
		setParamDataViaPN(OUTPUT_CURRENT, getParamDataViaPN(OUTPUT_CURRENT) - 400, CALLBACK_ON);
	}else if(_measured.ntcC == POWER_DERATE_START_TEMP + 5){
		setParamDataViaPN(OUTPUT_CURRENT, getParamDataViaPN(OUTPUT_CURRENT) - 500, CALLBACK_ON);
	}*/

	if(getParamDataViaPN(OUTPUT_CURRENT) - 100 >= 100){
		setParamDataViaPN(OUTPUT_CURRENT, getParamDataViaPN(OUTPUT_CURRENT) - 100, CALLBACK_ON);
	}
	errorHandler(overTempWarn);

	//!Completely derated
	if(_measured.ntcC <= POWER_DERATE_MAX_TEMP) return isDerated;
	if(_measured.INTC >= POWER_DERATE_START_TEMP){
		errorHandler(overTempError);
		return isDerated;
	}
	return isDerated;
}

/**
 * @brief Updates voltage pwm limits.
 * Limits used for stop the jumping on the output.
 * 
 */
void updateVPWMLimits(void){
	VPWMHighLimit = Voltage_PWM + 40;
	if(Voltage_PWM > 40){
		VPWMLowLimit = Voltage_PWM - 40;
	}
	else{
		VPWMLowLimit = 0;
	}
}

/**
 * @brief Updates current pwm limits.
 * Limits used for stop the jumping on the output.
 * 
 */
void updateIPWMLimits(void){
		IPWMHighLimit = Current_PWM + 100;
	if(Current_PWM > 100){
		IPWMLowLimit  = Current_PWM - 100;
	}
	else{
		IPWMLowLimit = 0;
	}
}

/**
 * @brief Set the Relay Output object to conducting or nonconducting state
 * 
 * @param _stat 
 */
void setRelayOutput(const RELAY_STATE _stat){
	if(_stat == RELAY_NOT){
		HAL_GPIO_WritePin(RELAY_OP_GPIO_Port, RELAY_OP_Pin, GPIO_PIN_RESET);
	}else if(_stat == RELAY_CONDUCTING){
		HAL_GPIO_WritePin(RELAY_OP_GPIO_Port, RELAY_OP_Pin, GPIO_PIN_SET);
	}
}

/**
 * @brief Get the Relay State object
 * 
 * @return RELAY_STATE 
 */
RELAY_STATE getRelayState(void){
	if(HAL_GPIO_ReadPin(RELAY_OP_GPIO_Port, RELAY_OP_Pin) == GPIO_PIN_SET) return RELAY_CONDUCTING;
	if(HAL_GPIO_ReadPin(RELAY_OP_GPIO_Port, RELAY_OP_Pin) == GPIO_PIN_RESET) return RELAY_NOT;
	return RELAY_NOT;
}

/**
 * @brief return STAT_TRUE if reverse voltage exceeds param
 * 
 * @param _measured measured output values
 * @return AlgorithmStatus 
 */
AlgorithmStatus isOutputReverse(const MEASURED_t _measured){
	if(_measured.Vbat > getParamDataViaPN(OUTPUT_REVERSE_VOLTAGE) ) return STAT_TRUE;
	return STAT_FALSE; 
}

/**
 * @brief checks for ADC pin possible values returns STAT_TRUE if it is abnormal"
 * 
 * @param _measured 
 * @return AlgorithmStatus 
 */
AlgorithmStatus isADCOpen(const MEASURED_t _measured){
	if(_measured.Vout == 0) return STAT_TRUE;
	if(_measured.RPT100 <= 8000 || _measured.RPT100 >= 20000) return STAT_TRUE;
	if(_measured.ntcC == 150) return STAT_TRUE;
	//if(_measured.Iout == 0) return STAT_TRUE;
	//if(_measured.Vbat == 0) return STAT_TRUE;
	//if(_measured.INTC == 0) return STAT_TRUE;
	return STAT_FALSE;
}

/**
 * @brief checks for how many times we reached voltage pwm below limit
 * 
 * @param _measured 
 * @return AlgorithmStatus 
 */
AlgorithmStatus isOverVoltage(const MEASURED_t _measured){
	//overVoltageTick gets incremented at outputController if we can't control output V.
	//If there is over voltage and we are the current source this means we lost control.
	//OV can also happen on bigger battery voltage.
	if(overVoltageTick >= OVER_VOLTAGE_COUNT &&_measured.Iout >= 100) return STAT_TRUE;
	return STAT_FALSE;
}

/**
 * @brief checks for current limit upper boundary
 * 
 * @param _measured 
 * @return AlgorithmStatus 
 */
AlgorithmStatus isOverCurrent(const MEASURED_t _measured){
	if(_measured.Iout >= MAX_CURRENT) return STAT_TRUE;
	return STAT_FALSE;
}

/**
 * @brief checks pt100 possible R values
 * 
 * @return AlgorithmStatus 
 */
AlgorithmStatus isPT100Connected(const MEASURED_t _measured){
	//If it is 7487 ADC must be 0,	  it goes to there in short circuit
	//If it is 205K ADC must be 4095, it goes to there in open circuit
	if(_measured.RPT100 <= 8000 || _measured.RPT100 >= 20000) return STAT_FALSE;
	return STAT_TRUE;
}

AlgorithmStatus errorHandler(errorType _err){

	uint8_t state = 0;
	uint8_t shift = 0;
	uint8_t fatal_clear = 0;
	switch(_err){
	case ADCOpen:
		if(getParamDataViaPN(WARN_ADC) == 1) return STAT_FALSE;
		setParamDataViaPN(WARN_ADC, 1, CALLBACK_OFF);
		shift = 1;
		state = 3;//Warning
		break;
	case ADCOpen_false:
		if(getParamDataViaPN(WARN_ADC) == 0) return STAT_FALSE;
		setParamDataViaPN(WARN_ADC, 0, CALLBACK_OFF);
		shift = 1;
		state = 0;//No Err
		break;

	case outputReverse:
		if(getParamDataViaPN(ERR_REVERSE) == 1) return STAT_FALSE;
		setParamDataViaPN(ERR_REVERSE, 1, CALLBACK_OFF);
		//RED LED Constantly ON
		setParamDataViaPN(LED_CONSTANT, 1, CALLBACK_OFF);
		shift = 1;
		state = 1;//Fatal
		break;
	case outputReverse_false:
		if(getParamDataViaPN(ERR_REVERSE) == 0) return STAT_FALSE;
		setParamDataViaPN(ERR_REVERSE, 0, CALLBACK_OFF);
		shift = 1;
		state = 0;//No Err
		fatal_clear = 1;
		break;

	case overTempWarn:
		if(getParamDataViaPN(WARN_OVER_TEMP) == 1) return STAT_FALSE;
		setParamDataViaPN(WARN_OVER_TEMP, 1, CALLBACK_OFF);
		shift = 2;
		state = 3;//Warning
		break;
	case overTempWarn_false:
		if(getParamDataViaPN(WARN_OVER_TEMP) == 0) return STAT_FALSE;
		setParamDataViaPN(WARN_OVER_TEMP, 0, CALLBACK_OFF);
		shift = 2;
		state = 0;//No Err
		break;

	case overTempError:
		if(getParamDataViaPN(ERR_OVER_TEMP) == 1) return STAT_FALSE;
		setParamDataViaPN(ERR_OVER_TEMP, 1, CALLBACK_OFF);
		//RED LED Flashes 1Hz
		setParamDataViaPN(LED_FLASH_TIME, 1024, CALLBACK_OFF);
		setParamDataViaPN(LED_BLINK, 1, CALLBACK_OFF);
		setParamDataViaPN(LED_CONSTANT, 0, CALLBACK_OFF);
		shift = 2;
		state = 1;//Fatal
		break;
	case overTempError_false:
		if(getParamDataViaPN(ERR_OVER_TEMP) == 0) return STAT_FALSE;
		setParamDataViaPN(ERR_OVER_TEMP, 0, CALLBACK_OFF);
		shift = 2;
		state = 0;//No Err
		fatal_clear = 1;
		break;

	case PT100Connection:
		if(getParamDataViaPN(WARN_PT100_CONN) == 1) return STAT_FALSE;
		setParamDataViaPN(WARN_PT100_CONN, 1, CALLBACK_OFF);
		shift = 3;
		state = 3;//Warning
		break;
	case PT100Connection_false:
		if(getParamDataViaPN(WARN_PT100_CONN) == 0) return STAT_FALSE;
		setParamDataViaPN(WARN_PT100_CONN, 0, CALLBACK_OFF);
		shift = 3;
		state = 0;//No Err
		break;

	case overTempPT100:
		if(getParamDataViaPN(WARN_PT100_TEMP) == 1) return STAT_FALSE;
		setParamDataViaPN(WARN_PT100_TEMP, 1, CALLBACK_OFF);
		shift = 4;
		state = 3;//Warning
		break;
	case overTempPT100_false:
		if(getParamDataViaPN(WARN_PT100_TEMP) == 0) return STAT_FALSE;
		setParamDataViaPN(WARN_PT100_TEMP, 0, CALLBACK_OFF);
		shift = 4;
		state = 0;//No Err
		break;

	case overVoltageError:
		if(getParamDataViaPN(ERR_OVER_VOLT) == 1) return STAT_FALSE;
		setParamDataViaPN(ERR_OVER_VOLT, 1, CALLBACK_OFF);
		//RED LED Flashes 2Hz
		setParamDataViaPN(LED_FLASH_TIME, 1024, CALLBACK_OFF);
		setParamDataViaPN(LED_BLINK, 2, CALLBACK_OFF);
		setParamDataViaPN(LED_CONSTANT, 0, CALLBACK_OFF);
		shift = 3;
		state = 1;//Fatal
		break;
	case overVoltageError_false:
		if(getParamDataViaPN(ERR_OVER_VOLT) == 0) return STAT_FALSE;
		setParamDataViaPN(ERR_OVER_VOLT, 0, CALLBACK_OFF);
		shift = 3;
		state = 0;//No Err
		fatal_clear = 1;
		break;

	case wrongBattWarn:
		if(getParamDataViaPN(WARN_BATT_VOLT) == 1) return STAT_FALSE;
		setParamDataViaPN(WARN_BATT_VOLT, 1, CALLBACK_OFF);
		shift = 6;
		state = 3;//Warning
		break;
	case wrongBattWarn_false:
		if(getParamDataViaPN(WARN_BATT_VOLT) == 0) return STAT_FALSE;
		setParamDataViaPN(WARN_BATT_VOLT, 0, CALLBACK_OFF);
		shift = 6;
		state = 0;//No Err
		break;


	default:
		break;
	}

	//state--> 0 No Err, 1 Fatal, 2 Temp, 3 Warning...
	switch(state){
	case 0:
		if(fatal_clear == 1){
			LED_OK = 1;
			*getParamDataAdr(ERR_FLAG) &= ~(0x1 << shift);
		}else{
			*getParamDataAdr(WARN_FLAG) &= ~(0x1 << shift);
		}
		//if(HAL_GPIO_ReadPin(CUT_OFF_OUTPUT_GPIO_Port, CUT_OFF_OUTPUT_Pin) == GPIO_PIN_SET) HAL_GPIO_WritePin(CUT_OFF_OUTPUT_GPIO_Port, CUT_OFF_OUTPUT_Pin, GPIO_PIN_RESET);
		setParamDataViaPN(OUTPUT_CUT_OFF, CONDUCTING, CALLBACK_ON);
		if(getRelayState() != RELAY_CONDUCTING) setRelayOutput(RELAY_CONDUCTING);
		break;
	case 1:
		setLedColor(((uint16_t)0x0040U), getParamDataViaPN(LED_CONSTANT));//RED
		LED_OK = 0;
		*getParamDataAdr(ERR_FLAG) |= 0x1 << shift;

		//! todo DISABLED FOR DEIF TESTS
//		if(HAL_GPIO_ReadPin(CUT_OFF_OUTPUT_GPIO_Port, CUT_OFF_OUTPUT_Pin) == GPIO_PIN_RESET) HAL_GPIO_WritePin(CUT_OFF_OUTPUT_GPIO_Port, CUT_OFF_OUTPUT_Pin, GPIO_PIN_SET);
		//setParamDataViaPN(OUTPUT_CUT_OFF, NOT_CONDUCTING, CALLBACK_ON);
		if(getRelayState() != RELAY_NOT) setRelayOutput(RELAY_NOT);
		__NOP();
		break;
	case 2:
		break;
	case 3:
		*getParamDataAdr(WARN_FLAG) |= 0x1 << shift;
		break;
	default:
		break;
	}

	return STAT_OK;
}

void Charging(const MEASURED_t _measured, uint8_t _boostLock, int16_t _PT100Comp){
	CHARGE_STATES charge = getParamDataViaPN(OUTPUT_CHARGE_MODE);
	if(_boostLock == 1) charge = BOOST;
	switch (charge)
	{
	case BOOST:
		boostCounter++;
		t1++;
		setParamDataViaPN(OUTPUT_VOLTAGE, outputReferences.boostVoltage, CALLBACK_OFF);
//		if(getParamDataViaPN(OUTPUT_VOLTAGE) != oldV){
//			setParamDataViaPN(OUTPUT_VOLTAGE, outputReferences.boostVoltage, CALLBACK_OFF);
//			oldV = getParamDataViaPN(OUTPUT_VOLTAGE);
//		}

		setLedColor(Blue, LED_NCONS);//todo Green-Cyan alternating

		if(getRelayState() != RELAY_CONDUCTING) setRelayOutput(RELAY_CONDUCTING);

		//this multiplier comes from %95 constraint
		if(_measured.Iout >= getParamDataViaPN(OUTPUT_CURRENT) * 0.95){
			return;
		}

		//If external boost pin is low, then we should stay in boost mode till it resets
		if(_boostLock == 1) return;

		setParamDataViaPN(OUTPUT_CHARGE_MODE, EQUALIZATION, CALLBACK_OFF);
		break;

	case EQUALIZATION:

		boostCounter++;
		t2++;
		setParamDataViaPN(OUTPUT_VOLTAGE, outputReferences.boostVoltage, CALLBACK_OFF);
//		if(getParamDataViaPN(OUTPUT_VOLTAGE) != oldV){
//			setParamDataViaPN(OUTPUT_VOLTAGE, outputReferences.boostVoltage, CALLBACK_OFF);
//			oldV = getParamDataViaPN(OUTPUT_VOLTAGE);
//		}

		setLedColor(Blue, LED_NCONS);//todo Green-Cyan alternating

		if(getRelayState() != RELAY_CONDUCTING) setRelayOutput(RELAY_CONDUCTING);

		if(t2 >= t1 / 2){
			if(t2 >= getParamDataViaPN(OUTPUT_CONS_VOLTAGE_TIME)){
				setParamDataViaPN(OUTPUT_CHARGE_MODE, FLOAT, CALLBACK_OFF);
			}
		}

		break;
	case FLOAT:
		
	setParamDataViaPN(OUTPUT_VOLTAGE, outputReferences.settedREFVoltage, CALLBACK_OFF);
	boostCounter++;
//	if(getParamDataViaPN(OUTPUT_VOLTAGE) != oldV){
//		setParamDataViaPN(OUTPUT_VOLTAGE, outputReferences.settedREFVoltage, CALLBACK_OFF);
//		oldV = getParamDataViaPN(OUTPUT_VOLTAGE);
//	}
	if(getRelayState() != RELAY_CONDUCTING) setRelayOutput(RELAY_CONDUCTING);

	if(boostCounter >= 2 * (t1 + t2)){
		if(boostCounter >= (getParamDataViaPN(OUTPUT_BOOST_TIME_MINUTES)) ){
			if(getParamDataViaPN(OUTPUT_AUTO_BOOST) == 1){
				setParamDataViaPN(OUTPUT_CHARGE_MODE, BOOST, CALLBACK_OFF);
				outputReferences.settedREFVoltage = getParamDataViaPN(OUTPUT_VOLTAGE);
				boostCounter = 0;
				t1 = 0;
				t2 = 0;
			}
		}
	}

	break;
	default:		
	//there must be memory error or debug going on
		setLedColor(Yellow, LED_NCONS);
		setParamDataViaPN(LED_BLINK, 2, CALLBACK_OFF);
		setParamDataViaPN(LED_FLASH_TIME, 1024, CALLBACK_OFF);
		break;
	}
	_PT100Comp += getParamDataViaPN(OUTPUT_VOLTAGE);
	setParamDataViaPN(OUTPUT_VOLTAGE, _PT100Comp, CALLBACK_OFF);
}

/**
 * @brief closes all led pins and sets ledColorPin var.
 */
uint32_t setLedColor(LEDColorType pin, LEDStatus _constant){

	if(pin == getParamDataViaPN(LED_COLOR)) return 0;
	if(LED_OK == 0) return 0;
	if(LED_OK2 == 0) return 0;
	setParamDataViaPN(LED_CONSTANT, _constant, CALLBACK_OFF);
	clearLED();
	setParamDataViaPN(LED_COLOR, pin, CALLBACK_OFF);
	return pin;
}

/**
 *@brief closes all LED pins
 */
void clearLED(void){
	HAL_GPIO_WritePin(LED_R_GPIO_Port, LED_R_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(LED_G_GPIO_Port, LED_G_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(LED_B_GPIO_Port, LED_B_Pin, GPIO_PIN_SET);
}

void controlLEDbyParam(uint8_t _set){
  switch(getParamDataViaPN(LED_COLOR)){
    case Black:
      clearLED();
      break;
    case White:
      if(_set == GPIO_PIN_SET){
        clearLED();
      }else if(_set == GPIO_PIN_RESET){
        HAL_GPIO_WritePin(LED_R_GPIO_Port, LED_R_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(LED_G_GPIO_Port, LED_G_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(LED_B_GPIO_Port, LED_B_Pin, GPIO_PIN_RESET);
      }
      break;
    case Red:
      if(_set == GPIO_PIN_SET){
        HAL_GPIO_WritePin(LED_R_GPIO_Port, LED_R_Pin, GPIO_PIN_SET);
      }else if(_set == GPIO_PIN_RESET){
        HAL_GPIO_WritePin(LED_R_GPIO_Port, LED_R_Pin, GPIO_PIN_RESET);
      }else{
        HAL_GPIO_TogglePin(LED_R_GPIO_Port, LED_R_Pin);
      }
      break;
    case Green:
      if(_set == GPIO_PIN_SET){
        HAL_GPIO_WritePin(LED_G_GPIO_Port, LED_G_Pin, GPIO_PIN_SET);
      }else if(_set == GPIO_PIN_RESET){
        HAL_GPIO_WritePin(LED_G_GPIO_Port, LED_G_Pin, GPIO_PIN_RESET);
      }else{
        HAL_GPIO_TogglePin(LED_G_GPIO_Port, LED_G_Pin);
      }
      break;
    case Blue:
      if(_set == GPIO_PIN_SET){
        HAL_GPIO_WritePin(LED_B_GPIO_Port, LED_B_Pin, GPIO_PIN_SET);
      }else if(_set == GPIO_PIN_RESET){
        HAL_GPIO_WritePin(LED_B_GPIO_Port, LED_B_Pin, GPIO_PIN_RESET);
      }else{
        HAL_GPIO_TogglePin(LED_B_GPIO_Port, LED_B_Pin);
      }
      break;
    case Purple:
      if(_set == GPIO_PIN_SET){
        HAL_GPIO_WritePin(LED_R_GPIO_Port, LED_R_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(LED_B_GPIO_Port, LED_B_Pin, GPIO_PIN_SET);
      }else if(_set == GPIO_PIN_RESET){
        HAL_GPIO_WritePin(LED_R_GPIO_Port, LED_R_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(LED_B_GPIO_Port, LED_B_Pin, GPIO_PIN_RESET);
      }else{
        HAL_GPIO_TogglePin(LED_R_GPIO_Port, LED_R_Pin);
        HAL_GPIO_TogglePin(LED_B_GPIO_Port, LED_B_Pin);
      }
      break;
    case Yellow:
      if(_set == GPIO_PIN_SET){
        HAL_GPIO_WritePin(LED_R_GPIO_Port, LED_R_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(LED_G_GPIO_Port, LED_G_Pin, GPIO_PIN_SET);
      }else if(_set == GPIO_PIN_RESET){
        HAL_GPIO_WritePin(LED_R_GPIO_Port, LED_R_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(LED_G_GPIO_Port, LED_G_Pin, GPIO_PIN_RESET);
      }else{
        HAL_GPIO_TogglePin(LED_R_GPIO_Port, LED_R_Pin);
        HAL_GPIO_TogglePin(LED_G_GPIO_Port, LED_G_Pin);
      }
      break;
    case Cyan:
      if(_set == GPIO_PIN_SET){
        HAL_GPIO_WritePin(LED_G_GPIO_Port, LED_G_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(LED_B_GPIO_Port, LED_B_Pin, GPIO_PIN_SET);
      }else if(_set == GPIO_PIN_RESET){
        HAL_GPIO_WritePin(LED_G_GPIO_Port, LED_G_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(LED_B_GPIO_Port, LED_B_Pin, GPIO_PIN_RESET);
      }else{
        HAL_GPIO_TogglePin(LED_G_GPIO_Port, LED_G_Pin);
        HAL_GPIO_TogglePin(LED_B_GPIO_Port, LED_B_Pin);
      }
      break;
  }
}

/**
 * @brief resets USB pin connection
 * @param _d: delay time
 */
void USBPinDown(uint32_t _d){
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	__HAL_RCC_GPIOA_CLK_ENABLE();
	HAL_GPIO_DeInit(GPIOA, (GPIO_PIN_11|GPIO_PIN_12));
	GPIO_InitStruct.Pin = GPIO_PIN_11|GPIO_PIN_12;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
	HAL_GPIO_WritePin(GPIOA, (GPIO_PIN_11|GPIO_PIN_12), GPIO_PIN_RESET);
	HAL_Delay(_d);
	HAL_GPIO_DeInit(GPIOA, (GPIO_PIN_11|GPIO_PIN_12));
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

/**
 * @brief called inside cdc_receive. used for test mode
 *
 * @param Buf usb incoming data buffer
 * @param Len size of data buffer
 */
void USBIncomingCallback(uint8_t* Buf, uint32_t *Len){
	//buraya gelmişse ilk 4 byte ENKOdur
	__NOP();
}
