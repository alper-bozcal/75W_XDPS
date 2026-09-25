#ifndef DEFAULTS_H
#define DEFAULTS_H

/**
 * @brief Header for holding all the 90W related defines.
 *        Should not hold any code block or structure.
 */

#include "main.h"

/*----------------------------------------------------------------------------------------------------------------*/

//Macro statement for Option Byte programming. If not defined; option byte will be set on level 1 protection.
#define                        DEBUG_MODE_OPTIONS
/*----------------------------------------------------------------------------------------------------------------*/

#define FALLING_EDGE           0
#define RISING_EDGE  		   1

/*----------------------------------------------------------------------------------------------------------------*/

#define ENABLED                1
#define DISABLED               0

/*----------------------------------------------------------------------------------------------------------------*/

#define MB_REQ_SIZE            256
#define DEF_MB_ADDR            1

/*----------------------------------------------------------------------------------------------------------------*/

#define FW_VERSION             2021

/*----------------------------------------------------------------------------------------------------------------*/

//TAG address for SCADA integration this is for where to look for tag readings, should be the last part of app code
//! This ADDR should be same with the define inside BL.h
#define USER_PROG_TAG_ADR      0x800FFC0
//TAG SIZE for SCADA integration this is for how much bytes should be read at the last section of app region
#define TAG_SIZE               32
// User Prog Start Address: App start address for Vector Table and BL TAG Loading
#define USER_PROG_START_ADR      0x8004000

/*----------------------------------------------------------------------------------------------------------------*/

// Address of the user params on the Flash
#define USER_PARAM_START_ADDR  0x0800F800
#define USER_PARAM_PAGE_NO     31

#define USER_TAG_START_ADDR    0x0800FF00

#define STAT_PAGE_START_ADDR   0x0800F000
#define STAT_PAGE_NO           30

/*----------------------------------------------------------------------------------------------------------------*/

//This should be changed for other MCU's. This one works for STM32072
#define PAGESIZEASBYTES        2048
//This should be changed for other MCU's. This one works for STM32072
#define BASEADDR (uint32_t)    0x08000000
//This is app specific. Determines the reserved size for flash application
#define USERBASEADDR (uint32_t)0x0800F800

/*----------------------------------------------------------------------------------------------------------------*/

//I * 100 = ADC * 100 * VACC / ( 4095 * (R71/1000 + 1) * (R77/1000 + 1) * RSHUNT(R65) )
//FOR 90W I * 100 = ADC * 100 * 3,3 / ( 4095 * 3,2 * 7,8 * 0,01)
#define MEASURE_CURRENT_MUL    2363//2531
#define MEASURE_CURRENT_SH     12

// Since all //  VOLTAGE_X values were 0.293 it can be rounded up to 0.25 so divide by 4
//  #define VOLTAGE_X            0.293
#define VOLTAGE_X              2
#define VOLTAGE_100Y           100

#define CURRENT_X              98
#define CURRENT_100Y           1110

#define DEF_OFFSET_VOUT        0
#define DEF_RATIO_VOUT         3630
#define DEF_SHIFT_VOUT         12

#define DEF_OFFSET_IOUT        23
#define DEF_RATIO_IOUT         MEASURE_CURRENT_MUL
#define DEF_SHIFT_IOUT         MEASURE_CURRENT_SH

#define DEF_OFFSET_INTC        0
#define DEF_RATIO_INTC         0
#define DEF_SHIFT_INTC         10

#define DEF_OFFSET_ONTC        0
#define DEF_RATIO_ONTC         1024
#define DEF_SHIFT_ONTC         0

#define DEF_OFFSET_VBAT        0
#define DEF_RATIO_VBAT         8000
#define DEF_SHIFT_VBAT         12

#define DEF_OFFSET_PT100       937
#define DEF_RATIO_PT100        1953
#define DEF_SHIFT_PT100        12

#define MIN_VOLTAGE             1200
#define MAX_VOLTAGE             1500
#define MAX_CURRENT             600
#define NOM_CURRENT             500
#define MIN_CURRENT             100
#define POWER                   900000

#define MIN_CABLE_DROP            0
#define MAX_CABLE_DROP            200
#define DEF_CABLE_DROP        	  0
#define MAX_ALLOWED_INT_DROP      20 // Maximum internal drop voltage allowed to be raised [10 * 20 = 200mV]
#define MAX_ALLOWED_EXT_DROP      50 // Maximum external drop voltage allowed to be raised [10 * 50 = 500mV]
/*----------------------------------------------------------------------------------------------------------------*/

// Voltage PID Defaults - Very aggressive for steady-state elimination
#define V_PID_KP  8    // Proportional gain: 0.08 (increased)
#define V_PID_KI  25   // Integral gain: 0.25 (much higher for tracking)
#define V_PID_KD  2    // Derivative gain: 0.02 (increased for stability)
#define V_PID_KA  0
// Current PID Defaults - Reduced gains for stability  
#define I_PID_KP  130  // Reduced from 1 to 0.08 (will be divided by 100 in implementation)
#define I_PID_KI  100 // Reduced from 1 to 0.03 (will be divided by 100 in implementation)
#define I_PID_KD  0 // Small derivative term for damping
#define I_PID_KA  0

/*----------------------------------------------------------------------------------------------------------------*/

#define OV_DEFAULT_DELAY       3000
#define OV_DEFAULT_MODE        MODE_OVER
#define OV_DEFAULT_TARGET      MAX_VOLTAGE + 100 // Note: changed to init with parameter output_voltage

#define OR_DEFAULT_DELAY       1
#define OR_DEFAULT_MODE        MODE_OVER
#define OR_DEFAULT_TARGET      600

#define OC_DEFAULT_DELAY       1000
#define OC_DEFAULT_MODE        MODE_OVER
#define OC_DEFAULT_TARGET      MAX_CURRENT + 100

#define OT_DEFAULT_DELAY       3000
#define OT_DEFAULT_MODE        MODE_OVER
#define OT_DEFAULT_TARGET      POWER_DERATE_START_TEMP

/*----------------------------------------------------------------------------------------------------------------*/

//Power derator check values
#define POWER_DERATE_START_TEMP 75
#define POWER_DERATE_END_TEMP   85
#define POWER_DERATE_HIST_TEMP  74
#define POWER_DERATE_MAX_TEMP   86

/*----------------------------------------------------------------------------------------------------------------*/

// Number of channels to convert from ADC to Buffer
#define DMA_SIZE               6

// Buffer for ADC Values to be filled with DMA
extern volatile uint16_t ADCValues[DMA_SIZE];

// ADC Channels' Order -1
#define ADC_ORD_ONTC           0 // CH 0
#define ADC_ORD_PT100          1 // CH 1
#define ADC_ORD_VBAT           2 // CH 3
#define ADC_ORD_VOUT           3 // CH 4
#define ADC_ORD_IOUT           4 // CH 9 (PB1)
#define ADC_ORD_INTC           5 // CH X
//ADC Values
#define ADC_VAL_ONTC           ADCValues[ADC_ORD_ONTC]
#define ADC_VAL_PT100          ADCValues[ADC_ORD_PT100]
#define ADC_VAL_VBAT           ADCValues[ADC_ORD_VBAT]
#define ADC_VAL_VOUT           ADCValues[ADC_ORD_VOUT]
#define ADC_VAL_IOUT           ADCValues[ADC_ORD_IOUT]
#define ADC_VAL_INTC           ADCValues[ADC_ORD_INTC]

#define DEF_ADCMIN_PT100       40
#define DEF_ADCMAX_PT100       4090

/*----------------------------------------------------------------------------------------------------------------*/

#define RELAY_ON               HAL_GPIO_WritePin(RELAY_OP_GPIO_Port, RELAY_OP_Pin, GPIO_PIN_SET)
#define RELAY_OFF              HAL_GPIO_WritePin(RELAY_OP_GPIO_Port, RELAY_OP_Pin, GPIO_PIN_RESET)
#define RELAY_VALUE			   HAL_GPIO_ReadPin(RELAY_OP_GPIO_Port, RELAY_OP_Pin)

#define OUTPUT_READ_VALUE      HAL_GPIO_ReadPin(OUTPUT_HEALTY_GPIO_Port, OUTPUT_HEALTY_Pin)
/*----------------------------------------------------------------------------------------------------------------*/

extern CAN_HandleTypeDef hcan;
#define CAN_HANDLE             hcan

/*----------------------------------------------------------------------------------------------------------------*/
extern TIM_HandleTypeDef       htim14;
extern TIM_HandleTypeDef       htim3;
extern TIM_HandleTypeDef       htim2;
#define TIMER_1MS              htim14
#define TIMER_PWM              htim3   // I_PWM (PB0, TIM3_CH3)
#define TIMER_V_PWM            htim2   // V_PWM (PB10, TIM2_CH3)

// PWM Output Timer Configs
// V_PWM TIM2'de (32 bit CCR). Modbus 16 bit yazar; APB bunu 32 bite cogaltir
// ((x<<16)|x -> %100 duty). Bu yuzden V_PWM_DUTY RAM'de tutulur, V_PWM_APPLY()
// ile 32 bit olarak TIM2->CCR3'e yazilir.
extern volatile uint16_t vPwmDuty;
#define V_PWM_DUTY    vPwmDuty
#define V_PWM_APPLY() (TIM2->CCR3 = (uint32_t)vPwmDuty)
#define I_PWM_DUTY    (TIM3->CCR3)
#define V_PWM_CH      TIM_CHANNEL_3
#define I_PWM_CH      TIM_CHANNEL_3
// PWM Duty Limits
#define PWM_MAX   3200
#define PWM_MIN   0
#define PWM_VMAX  950 // 15.5V for 90W

// Multiply of these two should make 2 seconds
#define LED_DEFAULT_BLINK_TIME 400  // Time takes for 1 blink
#define LED_DEFAULT_TIME       5   // Multiplier for total time

#define DEF_STAT_TIMER         43200000 // 12 Hours [ms]
#define DEF_BATT_TIMER         60000
#define DEF_CONTROL_TIMER      10
/*----------------------------------------------------------------------------------------------------------------*/

#define READ_BOOST_PIN          HAL_GPIO_ReadPin(BOOST_INP_GPIO_Port, BOOST_INP_Pin)
#define BOOST_INP               BOOST_INP_Pin
#define DEBOUNCE_TIME_BOOST_PIN 10 // [ms] debounce time for checking against boost pin in coreLoop()
#define BOOST_DEBOUNCE_MS 	    (5*DEBOUNCE_TIME_BOOST_PIN)

/*----------------------------------------------------------------------------------------------------------------*/

#define PASS_DEF_ENKO          1234
#define PASS_DEF_FACTORY       1923
#define PASS_DEF_SERVICE       2000
#define PASS_DEF_USER          3000

/*----------------------------------------------------------------------------------------------------------------*/

#define ENKO_SIGNATURE         1982

/*----------------------------------------------------------------------------------------------------------------*/


#define RETURN_ENKO_DEFAULT    1
#define RETURN_USER_DEFAULT    2

/*----------------------------------------------------------------------------------------------------------------*/


#endif  //DEFAULTS_H
