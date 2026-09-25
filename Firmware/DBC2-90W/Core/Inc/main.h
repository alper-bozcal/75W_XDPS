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
#include "stm32f0xx_ll_iwdg.h"
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
#include "defaults.h"
#include "stm32f0xx_ll_adc.h"
/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

// PCB Version Control
// Define PCB_VERSION_V7 for V7 hardware, leave undefined for V9 (default)
//#define PCB_VERSION_V7
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

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define OUTPUT_HEALTY_Pin GPIO_PIN_13
#define OUTPUT_HEALTY_GPIO_Port GPIOC
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
#define I_OUT_ADC_Pin GPIO_PIN_7
#define I_OUT_ADC_GPIO_Port GPIOA
#define I_PWM_Pin GPIO_PIN_0
#define I_PWM_GPIO_Port GPIOB
#define V_PWM_Pin GPIO_PIN_10
#define V_PWM_GPIO_Port GPIOB
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
// V7 kartinda OUTPUT_HEALTY PA6'da. CubeMX PC13 uretir, burada ezilir.
#ifdef PCB_VERSION_V7
#undef  OUTPUT_HEALTY_Pin
#undef  OUTPUT_HEALTY_GPIO_Port
#define OUTPUT_HEALTY_Pin       GPIO_PIN_6
#define OUTPUT_HEALTY_GPIO_Port GPIOA
#endif
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
