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

#define ENABLED                1
#define DISABLED               0
/*----------------------------------------------------------------------------------------------------------------*/

#define MB_REQ_SIZE            8

/*----------------------------------------------------------------------------------------------------------------*/

#define FW_VERSION             2001

/*----------------------------------------------------------------------------------------------------------------*/

//TAG address for SCADA integration this is for where to look for tag readings, should be the last part of app code
//! This ADDR should be same with the define inside BL.h
#define USER_PROG_TAG_ADR      0x800FFC0
//TAG SIZE for SCADA integration this is for how much bytes should be read at the last section of app region
#define TAG_SIZE               32

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
#define MEASURE_CURRENT_MUL    2531
#define MEASURE_CURRENT_SH     12

// Since all //  VOLTAGE_X values were 0.293 it can be rounded up to 0.25 so divide by 4
//  #define VOLTAGE_X            0.293
#define VOLTAGE_X              2
#define VOLTAGE_100Y           100

#define CURRENT_X              98
#define CURRENT_100Y           1110

#define DEF_OFFSET_VOUT        0
#define DEF_RATIO_VOUT         1815
#define DEF_SHIFT_VOUT         11

#define DEF_OFFSET_IOUT        0
#define DEF_RATIO_IOUT         MEASURE_CURRENT_MUL
#define DEF_SHIFT_IOUT         MEASURE_CURRENT_SH

#define DEF_OFFSET_INTC        0
#define DEF_RATIO_INTC         0
#define DEF_SHIFT_INTC         10

#define DEF_OFFSET_ONTC        4800
#define DEF_RATIO_ONTC         -1
#define DEF_SHIFT_ONTC         5

#define DEF_OFFSET_VBAT        0
#define DEF_RATIO_VBAT         500
#define DEF_SHIFT_VBAT         8

#define DEF_OFFSET_PT100       -2340
#define DEF_RATIO_PT100        819
#define DEF_SHIFT_PT100        8

  #define MIN_VOLTAGE          1200
  #define MAX_VOLTAGE          3000
  #define MAX_CURRENT          1000
  #define NOM_CURRENT          500
  #define POWER                1500000

  //macros for pwm generator timer registers
  #define Voltage_PWM          (TIM3->CCR4)
  #define Current_PWM          (TIM3->CCR3)
  #define V_PWM_CH             TIM_CHANNEL_4
  #define I_PWM_CH             TIM_CHANNEL_3
/*----------------------------------------------------------------------------------------------------------------*/

#define PWM_DEFAULT_HIGH_LIMIT 1000
#define PWM_DEFAULT_LOW_LIMIT  0
#define PWM_DEFAULT_V_LIM      100
#define PWM_DEFAULT_I_LIM      100

/*----------------------------------------------------------------------------------------------------------------*/

#define OV_DEFAULT_DELAY       3000
#define OV_DEFAULT_MODE        MODE_OVER
#define OV_DEFAULT_TARGET      MAX_VOLTAGE + 100 // Note: changed to init with parameter output_voltage

#define OR_DEFAULT_DELAY       1
#define OR_DEFAULT_MODE        MODE_OVER
#define OR_DEFAULT_TARGET      MAX_VOLTAGE - 200

#define OC_DEFAULT_DELAY       1000
#define OC_DEFAULT_MODE        MODE_OVER
#define OC_DEFAULT_TARGET      MAX_CURRENT + 100

#define OT_DEFAULT_DELAY       3000
#define OT_DEFAULT_MODE        MODE_OVER
#define OT_DEFAULT_TARGET      POWER_DERATE_START_TEMP

/*----------------------------------------------------------------------------------------------------------------*/

//Power derator check values
#define POWER_DERATE_START_TEMP 93
#define POWER_DERATE_HIST_TEMP  90
#define POWER_DERATE_MAX_TEMP   105

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
#define ADC_ORD_IOUT           4 // CH 7
#define ADC_ORD_INTC           5 // CH X
//ADC Values
#define ADC_VAL_ONTC           ADCValues[ADC_ORD_ONTC]
#define ADC_VAL_PT100          ADCValues[ADC_ORD_PT100]
#define ADC_VAL_VBAT           ADCValues[ADC_ORD_VBAT]
#define ADC_VAL_VOUT           ADCValues[ADC_ORD_VOUT]
#define ADC_VAL_IOUT           ADCValues[ADC_ORD_IOUT]
#define ADC_VAL_INTC           ADCValues[ADC_ORD_INTC]


#define DEF_ADCMIN_VOUT        5
#define DEF_ADCMAX_VOUT        4090
#define DEF_ADCMIN_IOUT        5
#define DEF_ADCMAX_IOUT        4090
#define DEF_ADCMIN_INTC        5
#define DEF_ADCMAX_INTC        4090
#define DEF_ADCMIN_ONTC        5
#define DEF_ADCMAX_ONTC        4090
#define DEF_ADCMIN_PT100       40
#define DEF_ADCMAX_PT100       4090
#define DEF_ADCMIN_VBAT        5
#define DEF_ADCMAX_VBAT        4090

/*----------------------------------------------------------------------------------------------------------------*/

#define RELAY_ON               HAL_GPIO_WritePin(RELAY_OP_GPIO_Port, RELAY_OP_Pin, GPIO_PIN_SET)
#define RELAY_OFF              HAL_GPIO_WritePin(RELAY_OP_GPIO_Port, RELAY_OP_Pin, GPIO_PIN_RESET)

/*----------------------------------------------------------------------------------------------------------------*/

extern CAN_HandleTypeDef hcan;
#define CAN_HANDLE             hcan

/*----------------------------------------------------------------------------------------------------------------*/
extern TIM_HandleTypeDef       htim14;
extern TIM_HandleTypeDef       htim3;
#define TIMER_1MS              htim14
#define TIMER_PWM              htim3

// Multiply of these two should make 2 seconds
#define LED_DEFAULT_BLINK_TIME 200  // Time takes for 1 blink
#define LED_DEFAULT_TIME       10   // Multiplier for total time

#define DEF_STAT_TIMER         43200000 // 12 Hours [ms]
#define DEF_BATT_TIMER         60000
#define DEF_CONTROL_TIMER      10
/*----------------------------------------------------------------------------------------------------------------*/

#define READ_BOOST_PIN          HAL_GPIO_ReadPin(BOOST_INP_GPIO_Port, BOOST_INP_Pin)
#define BOOST_INP               BOOST_INP_Pin
#define DEBOUNCE_TIME_BOOST_PIN 100 // [ms] debounce time for checking against boost pin in coreLoop()

/*----------------------------------------------------------------------------------------------------------------*/

#define PASS_DEF_ENKO           1234
#define PASS_DEF_FACTORY        1234
#define PASS_DEF_SERVICE        1234
#define PASS_DEF_USER           1234

/*----------------------------------------------------------------------------------------------------------------*/


#endif  //DEFAULTS_H
