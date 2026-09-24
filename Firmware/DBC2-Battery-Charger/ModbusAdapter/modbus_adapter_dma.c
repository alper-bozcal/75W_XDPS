/*
 * modbus_adapter_dma.c
 *
 *  Created on: 23 Jan 2017
 *      Author: Nazim Yildiz
 */

#include "modbus_adapter_dma.h"
#include "stm32f0xx_hal.h"
#include "hardware.h"

void ModbusAdapterUartInit() {
	GPIO_InitTypeDef gpioLeds;
//	HAL_UART_DeInit(&huart2);
	MX_USART2_UART_Init();
	huart2.hdmarx->Instance->CPAR = (uint32_t)&huart2.Instance->RDR;
	huart2.hdmatx->Instance->CPAR = (uint32_t)&huart2.Instance->TDR;
#if (CCS32x == CCS3210)
	HAL_GPIO_WritePin(RS485_DIR_Port, RS485_DIR_Pin, GPIO_PIN_RESET);
	huart2.Instance->ICR |= (1 << 6);	// Transmission Complete bayragini temizle.
#endif

	// Led inits
	__HAL_RCC_GPIOB_CLK_ENABLE();
	gpioLeds.Pin = GPIO_PIN_6 | GPIO_PIN_7;
	gpioLeds.Mode = GPIO_MODE_OUTPUT_PP;
	gpioLeds.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOB, &gpioLeds);
}

int8_t ModbusAdapterTransmitterDMA(int8_t *buffer, uint16_t size) {

	huart2.hdmatx->Instance->CCR &= (uint16_t)~DMA_CCR_EN;
	huart2.hdmatx->Instance->CMAR = (uint32_t)buffer;
	huart2.hdmatx->Instance->CPAR = (uint32_t)&huart2.Instance->TDR;
	huart2.hdmatx->Instance->CNDTR = size;
	__HAL_UART_CLEAR_FLAG(&huart2, UART_FLAG_TC);
	huart2.Instance->CR3 |= USART_CR3_DMAT;
	huart2.hdmatx->Instance->CCR |= DMA_CCR_EN;

	return 0;
}

uint16_t ModbusAdapterReceiverDMA(int8_t *buffer, uint16_t size) {

	static uint16_t led_counter;

	if((huart2.hdmarx->Instance->CCR & 0x0001) == 0x0000){
		//Frame error, Over run temizleme
		__HAL_UART_CLEAR_OREFLAG(&huart2);
		__HAL_UART_CLEAR_FEFLAG(&huart2);

	#if (CCS32x == CCS3210)
		HAL_GPIO_WritePin(RS485_DIR_Port, RS485_DIR_Pin, GPIO_PIN_RESET);
	#endif
		huart2.hdmarx->Instance->CPAR = (uint32_t)&huart2.Instance->RDR;
		huart2.hdmarx->Instance->CMAR = (uint32_t)buffer;
		huart2.hdmarx->Instance->CNDTR = size;
		huart2.hdmarx->Instance->CCR |= DMA_CCR_EN;
		huart2.Instance->CR3 |= USART_CR3_DMAR;
	}

	// Led durumunu degistir
	if(size - huart2.hdmarx->Instance->CNDTR > 0){
		led_counter++;
		if(led_counter % 50 == 0){
			HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_6);
		}
	}

	// Bu ana kadar kac bytelik verinin alindi bilgisi
	return (size - huart2.hdmarx->Instance->CNDTR);
}

int8_t ModbusAdapterReceiverStopDMA() {
	huart2.Instance->CR3 &= (uint32_t)~USART_CR3_DMAR;
	huart2.hdmarx->Instance->CCR &= (uint32_t)~DMA_CCR_EN;
	huart2.hdmarx->Instance->CNDTR = 0;
#if (CCS32x == CCS3210)
	// Gonderim islemi icin DIR pinini set edelim
	HAL_GPIO_WritePin(RS485_DIR_Port, RS485_DIR_Pin, GPIO_PIN_SET);
#endif
	return 0;
}

int8_t isModbusAdapterTransmitDone(){
	if(((huart2.Instance->ISR >> 6) & 0x00000001) == 1){
		huart2.Instance->ICR |= ((1 << 6) & 0x00000040);	// Transmission Complete bayragini temizle.
		return 1;
	}
	else{
		return 0;
	}
}
