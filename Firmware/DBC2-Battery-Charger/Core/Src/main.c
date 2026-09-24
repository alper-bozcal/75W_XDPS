/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  * DBC2 Battery Charger Project Developed by ENKO Control Systems
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "can.h"
#include "dma.h"
#include "iwdg.h"
#include "tim.h"
#include "usb_device.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "usbd_cdc_if.h"
#include "Controller.h"
#include "stm32f0xx_hal.h"

#include "ModbusServerSerial/msserial.h"
#include "ModbusClientSerial/mcserial.h"
#include "ModbusPDU/modbus_pdu.h"
#include "modbus_adapter_dma.h"
#include "modbus_adapter_usb.h"
#include "modbus_adapter_pdu.h"
#include "parameters_setget.h"
#include "parameter_callback.h"
#include "dynamic_pass_adapter.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
//bootloader defines
#define USER_PROG_BASLANGIC_ADR 0x08004800
#define SYSCFG_MemoryRemap_SRAM ((uint8_t)0x03)
//#define  RCC_APB2ENR_SYSCFGEN   ((uint32_t)0x00000001)        /*!< SYSCFG clock enable */
#define RCC_APB2Periph_SYSCFG   RCC_APB2ENR_SYSCFGEN
//DMA conv complete buffer size
#define DMA_BUFFER_SIZE         6
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

__attribute__((section(".SIGNATURESECTION.SGN"))) volatile const uint8_t signature[4] ={'E','N','K','O'};
__attribute__((section(".My_Vector_Table_Section.table")))  uint32_t VectorTable[50];
//volatile uint32_t *VectorTable = (uint32_t *)0x20000000; // Carve space here via linker script, or attributes

MEASURED_t MEASURED;
extern int16_t data[PARAM_MAX_COUNT];

volatile int64_t tick = 0, tempTick = 0;

ModbusServerSerial_t MbServerUsbObj;
ModbusPDU_t MbServerUsbObj_PDU;

uint32_t ledToggleCount = 0, ledOffset = 0, ledOffset_2 = 0;

uint8_t  boostLock = 0, pt100Flag = 0;
GPIO_PinState boostPinState = GPIO_PIN_SET;
uint32_t boostPinCounter = 0;

int16_t INTC_CAL1 = 0, INTC_CAL2  = 0;
int16_t PT100Comp = 0;
int16_t CalibINTC = 0;

volatile uint16_t ADC_Buffer[6] = {0, 0, 0, 0, 0, 0};
volatile uint16_t DMA_Buffer[DMA_BUFFER_SIZE];

extern ControllerParam_t outputReferences;

ALG_STATES mode = S_PSU;

uint8_t tagCount = 0;

extern USBD_HandleTypeDef hUsbDeviceFS;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

void sysTickCallback(void);
HAL_StatusTypeDef Flash_writeReferencesToFlash(void);
HAL_StatusTypeDef Flash_readReferencesFromFlash(void);
void setDeviceParams(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
	// Vektor tablosu tasıma
  for(uint8_t i = 0; i < 48; i++){
		VectorTable[i] = *(__IO uint32_t*)(USER_PROG_BASLANGIC_ADR + (i<<2));
	}
	// Enable the SYSCFG peripheral clock//
	RCC_APB2PeriphResetCmd(RCC_APB2Periph_SYSCFG, ENABLE);
	// Remap SRAM at 0x00000000 //
	SYSCFG_MemoryRemapConfig(SYSCFG_MemoryRemap_SRAM);
	if(signature[0]==1)  // optimize etmemesi için eklendi.
	__NOP();
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_ADC_Init();
  MX_USB_DEVICE_Init();
  MX_TIM14_Init();
  MX_CAN_Init();
//  MX_TIM1_Init();
//  MX_TIM2_Init();
  MX_TIM3_Init();
  MX_IWDG_Init();
  /* USER CODE BEGIN 2 */
//  HAL_GPIO_WritePin(CUT_OFF_OUTPUT_GPIO_Port, CUT_OFF_OUTPUT_Pin, GPIO_PIN_RESET);
//  HAL_GPIO_WritePin(LED_R_GPIO_Port,LED_R_Pin, GPIO_PIN_RESET);
//  while(1);

  #ifdef DBC_2_150W
  MX_TIM3_Init();
  #endif
#ifdef DBC_2_150W_2
MX_TIM3_Init();
#endif
  #ifdef DBC_2_300W
  MX_TIM1_Init();
  MX_TIM2_Init();
  #endif

  #ifndef DEBUG_MODE_OPTIONS
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
  #endif
	//Start ADC and DMA peripherals
	HAL_ADCEx_Calibration_Start(&hadc);
	HAL_ADC_Start_DMA(&hadc, (uint32_t*)DMA_Buffer, DMA_BUFFER_SIZE);

  	// USB uzerinden MultiWrite fonksiyonu maksimum 27register icin gerceklesebilir.
	// USB Composite kurulumu yapilmaktadir. USB-CDC ve USB-MSC
	ModbusAdapterUsbInit();
	modbusServerSerialSoftInit(&MbServerUsbObj, &MbServerUsbObj_PDU);
	modbusServerSerialLLInit(&MbServerUsbObj, ModbusAdapterTransmitterUsb,
			ModbusAdapterReceiverUsb, ModbusAdapterReceiverStopUsb, isModbusAdapterTransmitUSBDone);
	ModbusPDUInit(&MbServerUsbObj_PDU, ModbusPDUAdapterWrite, ModbusPDUAdapterRead, ModbusPDUAdressCheck);
	setModbusServerSerialSlaveAddress(&MbServerUsbObj, 1);
	setModbusServerSerialControlInterval(&MbServerUsbObj, 1 * 2);
	setModbusServerSerialWait(&MbServerUsbObj, 10);

	//Init dynamic password
	setDynamicPasswordSetup(getDynamicPasswordObj());

	//load default parameters from ROM. Used till Flash read
	for(uint16_t i = 1; i < PARAM_MAX_COUNT; i++){
		data[i - 1] = getParamDefViaPN(i);
	}
	//Read from Flash and decide to use fixed PWM or controller
//	Flash_writeReferencesToFlash();
    Flash_readReferencesFromFlash();
    outputReferences.REFCurrent = getParamDataViaPN(OUTPUT_CURRENT);
	if(getParamDataViaPN(PWMFLAG) == 0){
	  setCurrentPWMtoRef(&outputReferences);
	  setVoltagePWMtoRef(&outputReferences);
	}else if(getParamDataViaPN(PWMFLAG) == 1){
	Voltage_PWM = getParamDataViaPN(VOUTPWM);
    updateVPWMLimits();
	Current_PWM = getParamDataViaPN(IOUTPWM);
    updateIPWMLimits();
	}

	setParamDataViaPN(DEVICE_SOFTWARE_VER, 1090, CALLBACK_OFF);

  // Set ENKO Constant MB params for this device
  setDeviceParams();
#ifdef DBC_2_90W
  //Start TIM3 and PWM outputs according to references above
  TIM3->ARR  = 959;
  TIM3->PSC  = 0;
  HAL_TIM_PWM_Start(&htim3, I_PWM_CH); //I_PWM
  HAL_TIM_PWM_Start(&htim3, V_PWM_CH); //V_PWM
#endif
#ifdef DBC_2_150W
	//Start TIM3 and PWM outputs according to references above
	TIM3->ARR  = 959;
	TIM3->PSC  = 0;
	HAL_TIM_PWM_Start(&htim3, I_PWM_CH); //I_PWM
	HAL_TIM_PWM_Start(&htim3, V_PWM_CH); //V_PWM
#endif
#ifdef DBC_2_150W_2
  //Start TIM3 and PWM outputs according to references above
  TIM3->ARR  = 959;
  TIM3->PSC  = 0;
  HAL_TIM_PWM_Start(&htim3, I_PWM_CH); //I_PWM
  HAL_TIM_PWM_Start(&htim3, V_PWM_CH); //V_PWM
#endif
#ifdef DBC_2_300W
	TIM2->ARR  = 959;
	TIM2->PSC  = 0;
	HAL_TIM_PWM_Start(&htim2, I_PWM_CH); //I_PWM
	HAL_TIM_PWM_Start(&htim2, V_PWM_CH); //V_PWM
	HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_3);
#endif

	//Start TIM14 interrupt to control LEDs
	TIM14->ARR = 59999;
	TIM14->PSC = 7;
	HAL_TIM_Base_Start_IT(&htim14);

	//ADC Calibration for internal NTC Temp
	INTC_CAL1 = *(int16_t*)0x1FFFF7B8;
	INTC_CAL2 = *(int16_t*)0x1FFFF7C2;
	//When I used int16_t instead of float for CalibINTC, flash size usage shrink 2KB!
	//https://www.st.com/resource/en/application_note/cd00211314-how-to-get-the-best-adc-accuracy-in-stm32-microcontrollers-stmicroelectronics.pdf
	CalibINTC = ((110.0 - 30.0) / (INTC_CAL2 - INTC_CAL1)) * 1024;

	//Toggle the LEDs to initial state
	setLedColor(Black, LED_CONS);
	setLedColor(Red, LED_CONS);
	controlLEDbyParam(GPIO_PIN_RESET);
	HAL_Delay(500);
	setLedColor(Blue, LED_CONS);
	controlLEDbyParam(GPIO_PIN_RESET);
	HAL_Delay(500);
	setLedColor(Green, LED_CONS);
	controlLEDbyParam(GPIO_PIN_RESET);
	HAL_Delay(500);

	//Init the flag variables
	outputReferences.settedREFVoltage = outputReferences.nonParallelREFVoltage = getParamDataViaPN(OUTPUT_VOLTAGE);
	boostPinState = HAL_GPIO_ReadPin(BOOST_INP_GPIO_Port, BOOST_INP_Pin);
    *getParamDataAdr(PARALLEL_CURRENT_COEFF) = getParamDefViaPN(PARALLEL_CURRENT_COEFF);//parallelCurrentCoefficient = 3;
    setParamDataViaPN(ERR_FLAG, 0, CALLBACK_OFF);
    setParamDataViaPN(WARN_FLAG, 0, CALLBACK_OFF);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
    //Algorithm starts

    //Check device type and set for analog
    //if(MAX_VOLTAGE <= 1600)HAL_GPIO_WritePin(OVER_VOLT_GPIO_Port, OVER_VOLT_Pin, GPIO_PIN_SET);
    setParamDataViaPN(OUTPUT_MODEL, 0000, CALLBACK_ON);

    //check for open connections on ADC
    if(isADCOpen(MEASURED) == STAT_TRUE){
    	errorHandler(ADCOpen);
    }else{
    	errorHandler(ADCOpen_false);
    }

    //Open the output MOSFET
	setParamDataViaPN(OUTPUT_CUT_OFF, CONDUCTING, CALLBACK_ON);
uint64_t mbPrevTick = 0;
	while (1)
	{
		//if this loop becomes faster than 1ms, tick related things will be stuck because it can't be incremented before new loop starts
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	  //* Reload IWDG to not cause reset of the MCU
    #ifndef DEBUG_MODE_OPTIONS
	  __HAL_IWDG_RELOAD_COUNTER(&hiwdg);
    #endif

	//update monitor parameters on modbus
		*getParamDataAdr(MONITORING_VOLTAGE) = MEASURED.Vout;
		*getParamDataAdr(MONITORING_CURRENT) = MEASURED.Iout;
	//I thought updating *getParamDataAdr(OUTPUT_VOLTAGE AND CURRENT) in here, would update the scada values.
	//But this will also effect voltage update callback

		// Moved this line to CPT100 calculation for SCADA
		// *getParamDataAdr(MONITORING_PT100)   = MEASURED.RPT100;

		*getParamDataAdr(MONITORING_NTC)     = MEASURED.ntcC;
		*getParamDataAdr(MONITORING_VBAT)    = MEASURED.Vbat;
		*getParamDataAdr(MONITORING_INTC) 	 = MEASURED.INTC;

		//To monitor actual pwm values over SCADA
		if(getParamDataViaPN(PWMFLAG) == 0){
			*getParamDataAdr(VOUTPWM) =  outputReferences.dbgVPWM = Voltage_PWM;
			*getParamDataAdr(IOUTPWM) =  outputReferences.dbgIPWM = Current_PWM;
		}

		//To monitor algorithm states over SCADA
		setParamDataViaPN(DEBUG_MODE, getParamDataViaPN(OUTPUT_CHARGE_MODE), CALLBACK_OFF);

    	//modbus function to respond calls from usb
		if(tick != mbPrevTick){
			mbPrevTick = tick;
			modbusServerSerialRun(&MbServerUsbObj);
		}

		//Control the LEDs
		if(getParamDataViaPN(LED_CONSTANT) == 0){
			if(HAL_GetTick() > ledOffset ){
				ledOffset = HAL_GetTick() + getParamDataViaPN(LED_FLASH_TIME);
				if(ledOffset < HAL_GetTick() ){
					ledOffset = getParamDataViaPN(LED_FLASH_TIME);
				}
				ledToggleCount = 0;
				controlLEDbyParam(GPIO_PIN_SET);
			}else{
				if(HAL_GetTick() > ledOffset_2){
					ledOffset_2 = HAL_GetTick() + 128;
					if(ledOffset_2 < HAL_GetTick()) ledOffset_2 = 128;
					if(ledToggleCount < getParamDataViaPN(LED_BLINK) * 2){
						ledToggleCount++;
						controlLEDbyParam(3);
					}else{
						controlLEDbyParam(GPIO_PIN_SET);
				  }
				}
			}
		}else{
			controlLEDbyParam(GPIO_PIN_RESET);
		}

	    //check for reverse polarity
	    if(isOutputReverse(MEASURED) == STAT_TRUE){
			errorHandler(outputReverse);
		}else{
			errorHandler(outputReverse_false);
		}

	    //Temperature controls
		if(tempTick != tick && tick % 65535 == 0){
			tempTick = tick;

			if(getParamDataViaPN(DEBUG_DERATE) == 1) powerDerator(MEASURED);

			//check for mode
			if(mode != S_CHARGER) continue;

			//check for pt100 status if it is activated and connected
			if(isPT100Connected(MEASURED) != STAT_TRUE){
				//! alt satır neden var?!?!?!
				//PT100 Charging() set edilen voltajı değiştiriyor
				//setParamDataViaPN(OUTPUT_VOLTAGE, outputReferences.settedREFVoltage, CALLBACK_ON);
				if(getParamDataViaPN(TEMP_PT100_ACT) != 1) continue;
				errorHandler(PT100Connection);
				continue;
			}
			//Activate PT100
			setParamDataViaPN(TEMP_PT100_ACT, 1, CALLBACK_OFF);
			errorHandler(PT100Connection_false);

			//Calculate Temp for PT100
			int16_t CPT100 = MEASURED.RPT100 * 0.0258 - 258;
			*getParamDataAdr(MONITORING_PT100)   = CPT100;

			//if dvdc set
			//modify reference voltage accordingly
			//If battery temp drops below 25C then output should be increased
			//but if it gets hotter we should drop the output voltage
			//so CPT100 - 25 produces negative coef if batt is cold and gets multiplied with DVDT and multiplied with "-"
			//setParamDataViaPN(OUTPUT_VOLTAGE, outputReferences.settedREFVoltage + (getParamDataViaPN(TEMP_PT100_DVDT) * (CPT100 - 25)), CALLBACK_OFF);
			PT100Comp = getParamDataViaPN(TEMP_PT100_DVDT) * (CPT100 - 25);
			pt100Flag = 1;


			//Check for power derate
			if(CPT100 < getParamDataViaPN(TEMP_DERATE_PT100)){
				errorHandler(overTempPT100_false);
				continue;
			}

			//Warning
			errorHandler(overTempPT100);

			//if there is error
			if(CPT100 > getParamDataViaPN(TEMP_PT100_MAX)){
				errorHandler(overTempPT100);
			}
		}

		//Check for over voltage situation
		if(isOverVoltage(MEASURED) == STAT_TRUE){
			errorHandler(overVoltageError);
		}else{
			errorHandler(overVoltageError_false);
		}

//		//over current can happen on current limit so no blocking
//		if(isOverCurrent(MEASURED) == STAT_TRUE){
//			setParamDataViaPN(LED_CONSTANT, 0, CALLBACK_OFF);
//			errorHandler(overCurrentWarn);
//		}
//		errorHandler(overCurrentWarn_false);

		//output control mode
		if(getParamDataViaPN(OUTPUT_MODE_PSU) == 1){
			mode = S_PSU;
		}else{
			mode=S_CHARGER;
		}
		switch(mode){
		//These are the device modes that  determines if device is battery charging or power supply
		case S_PSU:{
		//power supply mode 
		//in this mode, Vout and Iout is directly controlled by params
			if(getParamDataViaPN(OUTPUT_VOLTAGE) >= 1800){
				setLedColor(Cyan, LED_CONS);
			}else if(getParamDataViaPN(OUTPUT_VOLTAGE) <= 1600){
				setLedColor(Green, LED_CONS);
			}else{
				setLedColor(Purple, LED_CONS);
			}
			if(getRelayState() != RELAY_CONDUCTING) setRelayOutput(RELAY_CONDUCTING);
			break;
}
		case S_CHARGER:{
		//charger mode with stated output voltages.
		//it does auto boost, auto detect, ...
			*getParamDataAdr(OUTPUT_CURRENT) = outputReferences.REFCurrent;
			if(getRelayState() != RELAY_CONDUCTING) setRelayOutput(RELAY_CONDUCTING);
			
			if(getParamDataViaPN(OUTPUT_MODE_AUTO) == 1){
				//Check for connected battery voltage
				//if it is out of range, we can't auto detect
				if(MEASURED.Vout >= 1800){
					setParamDataViaPN(OUTPUT_VOLTAGE, 2760, CALLBACK_ON);
					setParamDataViaPN(OUTPUT_BOOST_VOLTAGE, 2800, CALLBACK_ON);
					setParamDataViaPN(OUTPUT_MODE_AUTO, 0, CALLBACK_OFF);
					if(MEASURED.Vout >= 1800){
						errorHandler(wrongBattWarn_false);
						continue;
					}
				}else if(MEASURED.Vout <= 1600){
					setParamDataViaPN(OUTPUT_VOLTAGE, 1380, CALLBACK_ON);
					setParamDataViaPN(OUTPUT_BOOST_VOLTAGE, 1450, CALLBACK_ON);
					setParamDataViaPN(OUTPUT_MODE_AUTO, 0, CALLBACK_OFF);
					if(MEASURED.Vout <= 1600){
						errorHandler(wrongBattWarn_false);
						continue;
					}
				}
				//If auto mode can't detect batt type
				setParamDataViaPN(OUTPUT_VOLTAGE, 1200, CALLBACK_ON);
				setParamDataViaPN(OUTPUT_BOOST_VOLTAGE, 1450, CALLBACK_ON);
				setParamDataViaPN(OUTPUT_MODE_AUTO, 0, CALLBACK_OFF);
				errorHandler(wrongBattWarn);
			}else if(getParamDataViaPN(OUTPUT_MODE_AUTO) == 0){

				//Check for parameter voltage, it should be deterministic
				if(*getParamDataAdr(OUTPUT_VOLTAGE) >= 1800){
					setLedColor(Cyan, LED_CONS);
					if(MEASURED.Vout >= 1800){
						errorHandler(wrongBattWarn_false);
						continue;
					}
				}else if(*getParamDataAdr(OUTPUT_VOLTAGE) <= 1600 && *getParamDataAdr(OUTPUT_VOLTAGE) >= 1201){
					setLedColor(Green, LED_CONS);
					if(MEASURED.Vout <= 1600){
						errorHandler(wrongBattWarn_false);
						continue;
					}
				}else{
					setLedColor(Purple, LED_NCONS);
					errorHandler(wrongBattWarn);
				}
			}
			break;
		}
		default:{
		//there must be memory error or debug going on
			setLedColor(Yellow, LED_CONS);
			//setParamDataViaPN(LED_CONSTANT, 0, CALLBACK_OFF);
			setParamDataViaPN(LED_BLINK, 2, CALLBACK_OFF);
			setParamDataViaPN(LED_FLASH_TIME, 1024, CALLBACK_OFF);
			break;
		}
		}
		//This function should be called every minutes or so
		//It does timing and measuring battery
		if(tick % 65535 == 0){
			if(mode == S_CHARGER) Charging(MEASURED, boostLock, PT100Comp);
		}
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  LL_FLASH_SetLatency(LL_FLASH_LATENCY_1);
  while(LL_FLASH_GetLatency() != LL_FLASH_LATENCY_1)
  {
  }
  LL_RCC_HSI14_Enable();

   /* Wait till HSI14 is ready */
  while(LL_RCC_HSI14_IsReady() != 1)
  {

  }
  LL_RCC_HSI14_SetCalibTrimming(16);
  LL_RCC_HSI48_Enable();

   /* Wait till HSI48 is ready */
  while(LL_RCC_HSI48_IsReady() != 1)
  {

  }
  LL_RCC_LSI_Enable();

   /* Wait till LSI is ready */
  while(LL_RCC_LSI_IsReady() != 1)
  {

  }
  LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
  LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_1);
  LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_HSI48);

   /* Wait till System clock is ready */
  while(LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_HSI48)
  {

  }
  LL_SetSystemCoreClock(48000000);

   /* Update the time base */
  if (HAL_InitTick (TICK_INT_PRIORITY) != HAL_OK)
  {
    Error_Handler();
  }
  LL_RCC_HSI14_EnableADCControl();
  LL_RCC_SetUSBClockSource(LL_RCC_USB_CLKSOURCE_HSI48);
}

/* USER CODE BEGIN 4 */

/**
 * @brief function to call periodically inside hal tick it computes and calculates periodic jobs 
 * 
 */
void sysTickCallback(void){

  //Time base for whole main function
	tick++;

	if(tick % 128 == 0){ //If changed OVER_VOLTAGE_COUNT should be updated to

		//Calculate the readed values in human readable form
		MEASURED.ntcC   = getParamDataViaPN(MEASURE_CALIB_NTC)   * (150 - (ADC_Buffer[0] >> 5)) >> 10;
		MEASURED.RPT100 = getParamDataViaPN(MEASURE_CALIB_PT100) * ((ADC_Buffer[1] * 819 >> 8)  + 7487) >> 10;//* 3.2 + 7487);
		MEASURED.Vbat   = getParamDataViaPN(MEASURE_CALIB_VBAT)  * (ADC_Buffer[2] * 500  >> 8 ) >> 10;
		MEASURED.Vout   = getParamDataViaPN(MEASURE_CALIB_VOUT)  * (ADC_Buffer[3] * 1815 >> 11) >> 10;
		MEASURED.Iout   = getParamDataViaPN(MEASURE_CALIB_IOUT)  * (ADC_Buffer[4] * MEASURE_CURRENT_MUL >> MEASURE_CURRENT_SH) >> 10;
		MEASURED.INTC   = ((CalibINTC * (ADC_Buffer[5] - INTC_CAL1)) / 1024) + 30;

		if(getParamDataViaPN(PARALLEL_WORKING) == 1){
			*getParamDataAdr(OUTPUT_VOLTAGE) = outputReferences.nonParallelREFVoltage - ( MEASURED.Iout / 128 * getParamDataViaPN(PARALLEL_CURRENT_COEFF) );//parallelWorking(MEASURED);
		}//else nonparallelvoltage = output_voltage

		//if(pt100Flag == 1 && getParamDataViaPN(TEMP_PT100_ACT) == 0) output_voltage = outputReferences.setted;

		if(getParamDataViaPN(PWMFLAG) == 0) outputController(MEASURED);

	}
}

/**
 * @brief callback for gpio perip. It's been used for Boost input pin
 * 
 * @param GPIO_Pin 
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin){

	if(GPIO_Pin == BOOST_INP_Pin){

	}
}

/**
 * @brief Callback function for DMA peripheral. It sums up all DMA values to be divided in the systickcallback.
 * Data comes from here can't be used without division.
 * 
 * @param hadc handle for adc peripheral
 */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc) {
  static uint64_t dma_prev_tick;
	if(tick % 8 == 0 && tick != dma_prev_tick){
		dma_prev_tick = tick;
		ADC_Buffer[0] = (DMA_Buffer[DMA_IND_0] + ADC_Buffer[0]) / 2;
		ADC_Buffer[1] = (DMA_Buffer[DMA_IND_1] + ADC_Buffer[1]) / 2;
		ADC_Buffer[2] = (DMA_Buffer[DMA_IND_2] + ADC_Buffer[2]) / 2;
		ADC_Buffer[3] = (DMA_Buffer[DMA_IND_3] + ADC_Buffer[3]) / 2;
		ADC_Buffer[4] = (DMA_Buffer[DMA_IND_4] + ADC_Buffer[4]) / 2;
		ADC_Buffer[5] = (DMA_Buffer[DMA_IND_5] + ADC_Buffer[5]) / 2;
  }
}

/**
 * @brief checks for data values on param list against modbus min max values
 */
void Default_isoutputReferencesDefault(){
	for(uint16_t i = 1; i < PARAM_LAST_INDEX; i++){
		if(data[i - 1] < getParamMinViaPN(i) || data[i - 1] > getParamMaxViaPN(i)) data[i - 1] = getParamDefViaPN(i);
	}
}
int16_t wtf[TAG_SIZE];
/**
 * @brief  this func reads control reference values from flash
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef Flash_readReferencesFromFlash(void){

	  //if it is initial loading of the program
	  if(FLASH_POINTER_1 == 0xFFFFFFFF){
	  //Load Default Vector Table
	  Flash_writeWordtoAddr((uint32_t)(&FLASH_POINTER_16 + 4), (uint32_t)&FLASH_POINTER_1);
	  Flash_writeWordtoAddr((uint32_t)(0x0800FFC0), 		   (uint32_t)&FLASH_POINTER_2);
	  Flash_writeWordtoAddr((uint32_t)(&FLASH_POINTER_16 + 4), (uint32_t)&FLASH_POINTER_3);
	  Flash_writeWordtoAddr((uint32_t)(&FLASH_POINTER_16 + 4), (uint32_t)&FLASH_POINTER_4);
	}
	  HAL_StatusTypeDef stat;

	  //read the list
	  stat = Flash_readDataChunkFromAddr(data, FLASH_POINTER_1 , (sizeof data));
	  if(stat != HAL_OK) return stat;

	  stat = Flash_readDataChunkFromAddr(wtf, FLASH_POINTER_2 , (sizeof wtf));
	  if(stat != HAL_OK) return stat;

	  //compare for defaults
	  Default_isoutputReferencesDefault();

	  //load to struct that is used inside code
	  outputReferences.REFCurrent = getParamDataViaPN(OUTPUT_CURRENT);
	  outputReferences.dbgVPWM = getParamDataViaPN(VOUTPWM);
	  outputReferences.dbgIPWM = getParamDataViaPN(IOUTPWM);
	  outputReferences.boostVoltage = getParamDataViaPN(OUTPUT_BOOST_VOLTAGE);
//	  outputReferences.nonParallelREFVoltage = getParamDataViaPN(OUTPUT_VOLTAGE);
	  return HAL_OK;
}

/**
 * @brief erases the control page and writes controller reference values to flash
 *
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef Flash_writeReferencesToFlash(void){

	//Hiç bir zaman readden önce write yapılmaması ön koşuluyla şu yapılabilir
	//read işlemi en baştaki 20 adresin içinde yer alan adreslerden ram'a verileri alacak
	//işlemesi gereken her şeyi işleyecek ve datanın son hali her zaman ramda olacak
	//write yapılması gerektiğinde flash baştan ram'a göre doldurulacak ve en baştaki adresler buna göre güncellenecek

	HAL_StatusTypeDef stat;
	uint32_t* tmpPointer = 0;

	stat = Flash_readWordChunkFromAddr(tmpPointer, 0x0800F800, 16);
	if(stat != HAL_OK) return stat;

	stat = Flash_ErasePageWithNumber(31);
	if(stat != HAL_OK) return stat;

	stat = Flash_writeWordChunktoAddr(tmpPointer, 0x0800F800, 16);
	if(stat != HAL_OK) return stat;


	stat = Flash_writeWordChunktoAddr((uint32_t*)data, FLASH_POINTER_1, (sizeof data) / 2);
	if(stat != HAL_OK) return stat;

	stat = Flash_writeWordChunktoAddr((uint32_t*)wtf, FLASH_POINTER_2, (sizeof wtf) / 2);
	if(stat != HAL_OK) return stat;

	return HAL_OK;
}

void setDeviceParams(void){
  setParamDataViaPN(DEVICE_CUSTOMER_NUMBER1, 0, CALLBACK_OFF);
  setParamDataViaPN(DEVICE_CUSTOMER_NUMBER2, 0, CALLBACK_OFF);
  setParamDataViaPN(DEVICE_CUSTOMER_NUMBER3, 0, CALLBACK_OFF);
  setParamDataViaPN(DEVICE_CUSTOMER_NUMBER4, 0, CALLBACK_OFF);
  setParamDataViaPN(DEVICE_CUSTOMER_NUMBER5, 0, CALLBACK_OFF);
  setParamDataViaPN(DEVICE_CUSTOMER_TEXT1, 0, CALLBACK_OFF);
  setParamDataViaPN(DEVICE_CUSTOMER_TEXT2, 0, CALLBACK_OFF);
  setParamDataViaPN(DEVICE_CUSTOMER_TEXT3, 0, CALLBACK_OFF);
  setParamDataViaPN(DEVICE_CUSTOMER_TEXT4, 0, CALLBACK_OFF);
  setParamDataViaPN(DEVICE_CUSTOMER_TEXT5, 0, CALLBACK_OFF);
  setParamDataViaPN(DEVICE_CUSTOMER_TEXT6, 0, CALLBACK_OFF);
  setParamDataViaPN(DEVICE_CUSTOMER_TEXT7, 0, CALLBACK_OFF);
  setParamDataViaPN(DEVICE_CUSTOMER_TEXT8, 0, CALLBACK_OFF);
  setParamDataViaPN(DEVICE_CUSTOMER_TEXT9, 0, CALLBACK_OFF);
  setParamDataViaPN(DEVICE_CUSTOMER_TEXT10, 0, CALLBACK_OFF);
  setParamDataViaPN(DEVICE_TOTAL_PARAMS_COUNT, PARAM_USERS_LAST_INDEX, CALLBACK_OFF);
  setParamDataViaPN(DEVICE_DEF_VALS_START_ADR, 0, CALLBACK_OFF);
  setParamDataViaPN(DEVICE_MAX_VALS_START_ADR, 0, CALLBACK_OFF);
  setParamDataViaPN(DEVICE_MIN_VALS_START_ADR, 0, CALLBACK_OFF);
  setParamDataViaPN(DEVICE_COEF_VALS_START_ADR, 0, CALLBACK_OFF);
  setParamDataViaPN(DEVICE_DYNPASS_USERCODE, 0, CALLBACK_OFF);
  setParamDataViaPN(DEVICE_DYNPASS_SERVICECODE, 0, CALLBACK_OFF);
  setParamDataViaPN(DEVICE_DYNPASS_FACTORYCODE, 0, CALLBACK_OFF);
  setParamDataViaPN(DEVICE_DYNPASS_ENKOCODE, 0, CALLBACK_OFF);
  setParamDataViaPN(DEVICE_DYNPASS_USERPASS, 0, CALLBACK_OFF);
  setParamDataViaPN(DEVICE_DYNPASS_SERVICEPASS, 0, CALLBACK_OFF);
  setParamDataViaPN(DEVICE_DYNPASS_FACTORYPASS, 0, CALLBACK_OFF);
  setParamDataViaPN(DEVICE_DYNPASS_ENKOPASS, 0, CALLBACK_OFF);
  setParamDataViaPN(DEVICE_USER_PASSWORD, 0, CALLBACK_OFF);
  setParamDataViaPN(DEVICE_SERVICE_PASSWORD, 0, CALLBACK_OFF);
  setParamDataViaPN(DEVICE_FACTORY_PASSWORD, 0, CALLBACK_OFF);
  setParamDataViaPN(DEVICE_ENKO_PASSWORD, 0, CALLBACK_OFF);
  setParamDataViaPN(LOGIN_PASSWORD_ENTRY_CUSTOMER_REG1, 0, CALLBACK_OFF);
  setParamDataViaPN(LOGIN_PASSWORD_ENTRY_CUSTOMER_REG2, 0, CALLBACK_OFF);
  setParamDataViaPN(LOGIN_SECURITY_CUSTOMER_REG1, 0, CALLBACK_OFF);
  setParamDataViaPN(LOGIN_SECURITY_CUSTOMER_REG2, 0, CALLBACK_OFF);
  setParamDataViaPN(LOGIN_PASSWORD_ENTRY_ENKO_REG1, 0, CALLBACK_OFF);
  setParamDataViaPN(LOGIN_PASSWORD_ENTRY_ENKO_REG2, 0, CALLBACK_OFF);
  setParamDataViaPN(LOGIN_SECURITY_ENKO_REG1, 0, CALLBACK_OFF);
  setParamDataViaPN(LOGIN_SECURITY_ENKO_REG2, 0, CALLBACK_OFF);
  setParamDataViaPN(DEVICE_OPTIONAL_MODULE, 0, CALLBACK_OFF);
  // Attention the right-left alignment on flash mem: TAG[1] is actually first two bytes of the TAG
  const uint16_t* TAG = (uint16_t*)USER_PROG_TAG_ADR;
  setParamDataViaPN(DEVICE_HARDWARE_VER_REG1, (uint16_t)TAG[1], CALLBACK_OFF);
  setParamDataViaPN(DEVICE_HARDWARE_VER_REG2, 0, CALLBACK_OFF);
  setParamDataViaPN(DEVICE_BOOTLOADER_VER, (uint16_t)TAG[0], CALLBACK_OFF);
  // setParamDataViaPN(DEVICE_SOFTWARE_VER]                = FW_VERSION;
  const uint16_t* UID = (uint16_t*)0x1FFFF7AC;
  setParamDataViaPN(DEVICE_IDCPU_REG1, UID[0], CALLBACK_OFF);
  setParamDataViaPN(DEVICE_IDCPU_REG2, UID[1], CALLBACK_OFF);
  setParamDataViaPN(DEVICE_IDCPU_REG3, UID[2], CALLBACK_OFF);
  setParamDataViaPN(DEVICE_IDCPU_REG4, UID[3], CALLBACK_OFF);
  setParamDataViaPN(DEVICE_IDCPU_REG5, UID[4], CALLBACK_OFF);
  setParamDataViaPN(DEVICE_IDCPU_REG6, UID[5], CALLBACK_OFF);
  setParamDataViaPN(DEVICE_IDCPU_REG7, 0, CALLBACK_OFF);
  setParamDataViaPN(DEVICE_IDCPU_REG8, 0, CALLBACK_OFF);
  setParamDataViaPN(DEVICE_IDCPU_REG9, 0, CALLBACK_OFF);
  setParamDataViaPN(DEVICE_IDCPU_REG10, 0, CALLBACK_OFF);
  setParamDataViaPN(DEVICE_IDENKO_REG1, 0, CALLBACK_OFF);
  setParamDataViaPN(DEVICE_IDENKO_REG2, 0, CALLBACK_OFF);
  setParamDataViaPN(DEVICE_IDENKO_REG3, 0, CALLBACK_OFF);
  setParamDataViaPN(DEVICE_MODEL_NAME_REG1, ('B' << 8) + 'D', CALLBACK_OFF);
  setParamDataViaPN(DEVICE_MODEL_NAME_REG2, ('2' << 8) + 'C', CALLBACK_OFF);
  // setParamDataViaPN(DEVICE_MODEL_NAME_REG3, ('0' << 8) + '4', CALLBACK_OFF);
  setParamDataViaPN(DEVICE_MODEL_NAME_REG3, 0, CALLBACK_OFF);
  setParamDataViaPN(DEVICE_MODEL_NAME_REG4, 0, CALLBACK_OFF);
  setParamDataViaPN(DEVICE_MODEL_NAME_REG5, 0, CALLBACK_OFF);
  setParamDataViaPN(DEVICE_MODEL_NAME_REG6, 0, CALLBACK_OFF);
  setParamDataViaPN(DEVICE_MODEL_NAME_REG7, 0, CALLBACK_OFF);
  setParamDataViaPN(DEVICE_MODEL_NAME_REG8, 0, CALLBACK_OFF);
  setParamDataViaPN(DEVICE_MODEL_NAME_REG9, 0, CALLBACK_OFF);
  setParamDataViaPN(DEVICE_MODEL_NAME_REG10, 0, CALLBACK_OFF);
}

/**
 *@brief callback function for timers.
 *It is used for modbusrun and Led run functionalities.
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim){
    if (htim->Instance==TIM14){
		//modbus function to respond calls from usb
		//modbusServerSerialRun(&MbServerUsbObj);

/*-----------------------------------------------------------------------------------------------*/
		if(boostPinState != HAL_GPIO_ReadPin(BOOST_INP_GPIO_Port, BOOST_INP_Pin)){
			if(HAL_GPIO_ReadPin(BOOST_INP_GPIO_Port, BOOST_INP_Pin) == GPIO_PIN_RESET){
				boostPinCounter++;
				if(boostPinCounter >= 50){
					if(boostLock == 1) return;
					boostPinCounter = 0;
					boostLock = 1;
					if(mode == S_CHARGER) Charging(MEASURED, boostLock, PT100Comp);
					boostPinState = GPIO_PIN_RESET;
					setParamDataViaPN(OUTPUT_CHARGE_MODE, BOOST, CALLBACK_OFF);
					*getParamDataAdr(MONITORING_IS_BOOST_TRIG) = 1;
				}
			}else{
				boostPinCounter = 0;
				boostLock = 0;
				setParamDataViaPN(OUTPUT_CHARGE_MODE, FLOAT, CALLBACK_OFF);
				*getParamDataAdr(MONITORING_IS_BOOST_TRIG) = 0;
				if(mode == S_CHARGER) Charging(MEASURED, boostLock, PT100Comp);
				boostPinState = GPIO_PIN_SET;
			}
		}
/*-----------------------------------------------------------------------------------------------*/
    }
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
