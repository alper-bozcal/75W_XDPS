/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2022 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f0xx_hal.h"

#include "stm32f0xx_ll_dma.h"
#include "stm32f0xx_ll_crs.h"
#include "stm32f0xx_ll_rcc.h"
#include "stm32f0xx_ll_bus.h"
#include "stm32f0xx_ll_system.h"
#include "stm32f0xx_ll_exti.h"
#include "stm32f0xx_ll_cortex.h"
#include "stm32f0xx_ll_utils.h"
#include "stm32f0xx_ll_pwr.h"
#include "stm32f0xx_ll_gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
//Macro statement for Option Byte programming. If not defined; option byte will be set on level 1 protection.
#define DEBUG_MODE_OPTIONS

//TAG address for SCADA integration this is for where to look for tag readings, should be the last part of app code
//! This ADDR should be same with the define inside BL.h
#define USER_PROG_TAG_ADR  0x800FFC0

#define DBC_2_90W
#ifdef  DBC_2_90W

	#define VOLTAGE_X   	0.293
	#define VOLTAGE_100Y	179

	#define CURRENT_X		98
	#define CURRENT_100Y	1110

	#define MIN_VOLTAGE		1200
	#define MAX_VOLTAGE		1500
	#define MAX_CURRENT		600
	#define POWER 			900000

	//I * 100 = ADC * 100 * VACC / ( 4095 * (R71/1000 + 1) * (R77/1000 + 1) * RSHUNT(R65) )
	//FOR 90W I * 100 = ADC * 100 * 3,3 / ( 4095 * 3,2 * 7,8 * 0,01)
	#define MEASURE_CURRENT_MUL 2531
	#define MEASURE_CURRENT_SH	12

	#define DMA_IND_0		0
	#define DMA_IND_1		1
	#define DMA_IND_2		2
	#define DMA_IND_3		3
	#define DMA_IND_4		4
	#define DMA_IND_5		5

	//macros for pwm generator timer registers
	#define Voltage_PWM             (TIM3->CCR4)
	#define Current_PWM             (TIM3->CCR3)
	#define V_PWM_CH				TIM_CHANNEL_4
	#define I_PWM_CH				TIM_CHANNEL_3

#elif defined DBC_2_150W
 	 //degerler ölcülerek dogru grafiği cıkartılır. grafik formülü voltaj 100 katı alındığı için ona göre yazılır.
	// PWM = V . X - y ;
	// PWM = ( V . x - 100y )/100 ;   V 100 katı seklinde geldiginden

	#define VOLTAGE_X   	0.293
	#define VOLTAGE_100Y	179

	//#define VOLTAGE_X   	0.293
	//#define VOLTAGE_100Y	179.40  // deklenmden cıkan 179,4 'tü

	//PWM = CURRENT_100Y - (CURRENT_X * reference / 100)
	#define CURRENT_X		98
	#define CURRENT_100Y	1110

	#define MIN_VOLTAGE		1200
	#define MAX_VOLTAGE		3000
	#define MAX_CURRENT 	1200
	#define POWER 			1500000

	//measured output current multiplier changes from one layout to another
	//I * 100 = ADC * 100 * VACC / ( 4095 * (R71/1000 + 1) * (R77/1000 + 1) * RSHUNT(R65) )
	//FOR 150W I * 100 = ADC * 100 * 3,3 / ( 4095 * 5,7 * 7,8 * 0,003)
	#define MEASURE_CURRENT_MUL 2531
	#define MEASURE_CURRENT_SH	12

	#define DMA_IND_0		0
	#define DMA_IND_1		1
	#define DMA_IND_2		2
	#define DMA_IND_3		3
	#define DMA_IND_4		4
	#define DMA_IND_5		5

	//macros for pwm generator timer registers
	#define Voltage_PWM             (TIM3->CCR4)
	#define Current_PWM             (TIM3->CCR3)
	#define V_PWM_CH				TIM_CHANNEL_4
	#define I_PWM_CH				TIM_CHANNEL_3

#elif defined DBC_2_150W_2
   //degerler ölcülerek dogru grafiği cıkartılır. grafik formülü voltaj 100 katı alındığı için ona göre yazılır.
  // PWM = V . X - y ;
  // PWM = ( V . x - 100y )/100 ;   V 100 katı seklinde geldiginden

  #define VOLTAGE_X     0.293
  #define VOLTAGE_100Y  179

  //#define VOLTAGE_X     0.293
  //#define VOLTAGE_100Y  179.40  // deklenmden cıkan 179,4 'tü

  //PWM = CURRENT_100Y - (CURRENT_X * reference / 100)
  #define CURRENT_X   98
  #define CURRENT_100Y  1110

  #define MIN_VOLTAGE   1200
  #define MAX_VOLTAGE   3000
  #define MAX_CURRENT   1200
  #define POWER       1500000

  //measured output current multiplier changes from one layout to another
  //I * 100 = ADC * 100 * VACC / ( 4095 * (R71/1000 + 1) * (R77/1000 + 1) * RSHUNT(R65) )
  //FOR 150W I * 100 = ADC * 100 * 3,3 / ( 4095 * 5,7 * 7,8 * 0,003)
  #define MEASURE_CURRENT_MUL 2531
  #define MEASURE_CURRENT_SH  12

  #define DMA_IND_0   0
  #define DMA_IND_1   1
  #define DMA_IND_2   2
  #define DMA_IND_3   3
  #define DMA_IND_4   4
  #define DMA_IND_5   5

  //macros for pwm generator timer registers
  #define Voltage_PWM             (TIM3->CCR4)
  #define Current_PWM             (TIM3->CCR3)
  #define V_PWM_CH        TIM_CHANNEL_4
  #define I_PWM_CH        TIM_CHANNEL_3


#elif defined DBC_2_300W

	//150-300W Seconders are the same so same values used, not tested
	#define VOLTAGE_X   	0.293
	#define VOLTAGE_100Y	179

	#define CURRENT_X		98
	#define CURRENT_100Y	1110

	#define MIN_VOLTAGE		2400
	#define MAX_VOLTAGE		3000
	#define MAX_CURRENT		1200
	#define POWER 			3600000

	//I * 100 = ADC * 100 * VACC / ( 4095 * (R104/1000 + 1) * (R111/1000 + 1) * RSHUNT(R98 // R97) )
	//FOR 300W I * 100 = ADC * 100 * 3,3 / ( 4095 * 5,7 * 7,8 * 0,003)
	#define MEASURE_CURRENT_MUL 2531
	#define MEASURE_CURRENT_SH	12

	#define DMA_IND_0		4
	#define DMA_IND_1		0
	#define DMA_IND_2		2
	#define DMA_IND_3		1
	#define DMA_IND_4		3
	#define DMA_IND_5		5

	//macros for pwm generator timer registers
	#define Voltage_PWM             (TIM2->CCR4)
	#define Current_PWM             (TIM2->CCR3)
	#define V_PWM_CH				TIM_CHANNEL_4
	#define I_PWM_CH				TIM_CHANNEL_3

#endif

//TAG SIZE for SCADA integration this is for how much bytes should be read at the last section of app region
#define TAG_SIZE 64

//struct to keep input values
//this monitoring values updated on main syscallback
typedef struct MEASURED_s{
  //output voltage and current values
	int16_t Vout;
	int16_t Iout;
  //reverse battery voltage
	int16_t Vbat;
  //PT100 input resistor value as Ohms
	int16_t RPT100;
  //onboard ntc input value
	int16_t ntcC;
  //internal ntc temperature value
	int16_t INTC;

}MEASURED_t;

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */
#ifdef DBC_2_90W

#define V_NTC_ADC_Pin GPIO_PIN_0
#define V_NTC_ADC_GPIO_Port GPIOA
#define PT100_ADC_Pin GPIO_PIN_1
#define PT100_ADC_GPIO_Port GPIOA
#define RELAY_OP_Pin GPIO_PIN_2
#define RELAY_OP_GPIO_Port GPIOA
#define VBAT_ADC_Pin GPIO_PIN_3
#define VBAT_ADC_GPIO_Port GPIOA
#define VOUT__ADC_Pin GPIO_PIN_4
#define VOUT__ADC_GPIO_Port GPIOA
#define BOOST_INP_Pin GPIO_PIN_5
#define BOOST_INP_GPIO_Port GPIOA
#define BOOST_INP_EXTI_IRQn EXTI4_15_IRQn
#define CUT_OFF_OUTPUT_Pin GPIO_PIN_6
#define CUT_OFF_OUTPUT_GPIO_Port GPIOA
#define I_OUT_ADC_Pin GPIO_PIN_7
#define I_OUT_ADC_GPIO_Port GPIOA
#define I_PWM_Pin GPIO_PIN_0
#define I_PWM_GPIO_Port GPIOB
#define V_PWM_Pin GPIO_PIN_1
#define V_PWM_GPIO_Port GPIOB
#define TP4_Pin GPIO_PIN_15
#define TP4_GPIO_Port GPIOB
#define TP4_EXTI_IRQn EXTI4_15_IRQn
#define OVER_VOLT_Pin GPIO_PIN_15
#define OVER_VOLT_GPIO_Port GPIOA
#define JTDO_Pin GPIO_PIN_3
#define JTDO_GPIO_Port GPIOB
#define LED_G_Pin GPIO_PIN_4
#define LED_G_GPIO_Port GPIOB
#define LED_B_Pin GPIO_PIN_5
#define LED_B_GPIO_Port GPIOB
#define LED_R_Pin GPIO_PIN_6
#define LED_R_GPIO_Port GPIOB

#endif //DBC_2_90W

#ifdef DBC_2_150W
/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define V_NTC_ADC_Pin GPIO_PIN_0
#define V_NTC_ADC_GPIO_Port GPIOA
#define PT100_ADC_Pin GPIO_PIN_1
#define PT100_ADC_GPIO_Port GPIOA
#define RELAY_OP_Pin GPIO_PIN_2
#define RELAY_OP_GPIO_Port GPIOA
#define VBAT_ADC_Pin GPIO_PIN_3
#define VBAT_ADC_GPIO_Port GPIOA
#define VOUT__ADC_Pin GPIO_PIN_4
#define VOUT__ADC_GPIO_Port GPIOA
#define BOOST_INP_Pin GPIO_PIN_5
#define BOOST_INP_GPIO_Port GPIOA
#define BOOST_INP_EXTI_IRQn EXTI4_15_IRQn
#define CUT_OFF_OUTPUT_Pin GPIO_PIN_6
#define CUT_OFF_OUTPUT_GPIO_Port GPIOA
#define I_OUT_ADC_Pin GPIO_PIN_7
#define I_OUT_ADC_GPIO_Port GPIOA
#define I_PWM_Pin GPIO_PIN_0
#define I_PWM_GPIO_Port GPIOB
#define V_PWM_Pin GPIO_PIN_1
#define V_PWM_GPIO_Port GPIOB
#define TP4_Pin GPIO_PIN_15
#define TP4_GPIO_Port GPIOB
#define TP4_EXTI_IRQn EXTI4_15_IRQn
#define OVER_VOLT_Pin GPIO_PIN_15
#define OVER_VOLT_GPIO_Port GPIOA
#define JTDO_Pin GPIO_PIN_3
#define JTDO_GPIO_Port GPIOB
#define LED_G_Pin GPIO_PIN_4
#define LED_G_GPIO_Port GPIOB
#define LED_B_Pin GPIO_PIN_5
#define LED_B_GPIO_Port GPIOB
#define LED_R_Pin GPIO_PIN_6
#define LED_R_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */
#endif //DBC_2_150W || DBC_2_90W

#ifdef DBC_2_150W_2
/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define V_NTC_ADC_Pin GPIO_PIN_0
#define V_NTC_ADC_GPIO_Port GPIOA
#define PT100_ADC_Pin GPIO_PIN_1
#define PT100_ADC_GPIO_Port GPIOA
#define RELAY_OP_Pin GPIO_PIN_2
#define RELAY_OP_GPIO_Port GPIOA
#define VBAT_ADC_Pin GPIO_PIN_3
#define VBAT_ADC_GPIO_Port GPIOA
#define VOUT__ADC_Pin GPIO_PIN_4
#define VOUT__ADC_GPIO_Port GPIOA
#define BOOST_INP_Pin GPIO_PIN_5
#define BOOST_INP_GPIO_Port GPIOA
#define BOOST_INP_EXTI_IRQn EXTI4_15_IRQn
#define CUT_OFF_OUTPUT_Pin GPIO_PIN_6
#define CUT_OFF_OUTPUT_GPIO_Port GPIOA
#define I_OUT_ADC_Pin GPIO_PIN_7
#define I_OUT_ADC_GPIO_Port GPIOA
#define V_PWM_Pin GPIO_PIN_0
#define V_PWM_GPIO_Port GPIOB
#define I_PWM_Pin GPIO_PIN_1
#define I_PWM_GPIO_Port GPIOB
#define TP4_Pin GPIO_PIN_15
#define TP4_GPIO_Port GPIOB
#define OVER_VOLT_Pin GPIO_PIN_15
#define OVER_VOLT_GPIO_Port GPIOA
#define JTDO_Pin GPIO_PIN_3
#define JTDO_GPIO_Port GPIOB
#define LED_G_Pin GPIO_PIN_4
#define LED_G_GPIO_Port GPIOB
#define LED_B_Pin GPIO_PIN_5
#define LED_B_GPIO_Port GPIOB
#define LED_R_Pin GPIO_PIN_6
#define LED_R_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */
#endif //DBC_2_150W_2

#ifdef DBC_2_300W
#define PT100_ADC_Pin GPIO_PIN_1
#define PT100_ADC_GPIO_Port GPIOA
#define CUT_OFF_OUTPUT_Pin GPIO_PIN_2
#define CUT_OFF_OUTPUT_GPIO_Port GPIOA
#define V_PWM_Pin GPIO_PIN_3
#define V_PWM_GPIO_Port GPIOA
#define VOUT__ADC_Pin GPIO_PIN_5
#define VOUT__ADC_GPIO_Port GPIOA
#define VBAT_ADC_Pin GPIO_PIN_6
#define VBAT_ADC_GPIO_Port GPIOA
#define RELAY_OP_Pin GPIO_PIN_7
#define RELAY_OP_GPIO_Port GPIOA
#define I_OUT_ADC_Pin GPIO_PIN_0
#define I_OUT_ADC_GPIO_Port GPIOB
#define V_NTC_ADC_Pin GPIO_PIN_1
#define V_NTC_ADC_GPIO_Port GPIOB
#define I_PWM_Pin GPIO_PIN_10
#define I_PWM_GPIO_Port GPIOB
#define BOOST_INP_Pin GPIO_PIN_12
#define BOOST_INP_GPIO_Port GPIOB
#define BOOST_INP_EXTI_IRQn EXTI4_15_IRQn
#define PWM_CAN_Pin GPIO_PIN_15
#define PWM_CAN_GPIO_Port GPIOB
#define LED_B_Pin GPIO_PIN_15
#define LED_B_GPIO_Port GPIOA
#define JTDO_Pin GPIO_PIN_3
#define JTDO_GPIO_Port GPIOB
#define LED_R_Pin GPIO_PIN_4
#define LED_R_GPIO_Port GPIOB
#define LED_G_Pin GPIO_PIN_7
#define LED_G_GPIO_Port GPIOB
#endif //DBC_2_300W
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
