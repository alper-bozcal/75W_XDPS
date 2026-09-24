/*
 * modbus_adapter_iot_dma.h
 *
 *  Created on: 20 April 2021
 *      Author: Nazim Yildiz
 */

 #ifndef _MODBUS_ADAPTER_IOT_DMA_H_
 #define _MODBUS_ADAPTER_IOT_DMA_H_

#include <stdint.h>

/*
 * @brief Usart3 donanimi icin gerekli ayarlarin yapildigi fonksiyondur
 */
void ModbusAdapterIotInit();

/*
 * @brief DMA ile paket gonderimi saglayan arayuz fonksiyonudur
 * @param buffer Gonderilecek paketin adresini belirtir
 * @param size Gonderilecek paketin boyutunu belirtir [byte]
 * @return 
 * @precondition DMA1_CH2'nin USART3'e baglanmis olmasi gerekir 
 * @postcondition 
 */
int8_t ModbusAdapterIotTransmitterDMA(int8_t *buffer, uint16_t size);

/*
 * @brief Kac adet paket alindigini bildiren fonksiyondur(DMA sayesinde kesme-fonksiyonuna gidilmesine gerek yoktur)
 * @param Gelen paketlerin yazilmaya baslanilacagi RAM adresi
 * @param size Gelmesi beklenen maks paketin boyutunu belirtir [byte]
 * @return Suana kadar gelen byte miktar
 * @precondition DMA1_CH3'nin USART3'e baglanmis olmasi gerekir 
 * @postcondition 
 */
uint16_t ModbusAdapterIotReceiverDMA(int8_t *buffer, uint16_t size);

/*
 * @brief Alim islemini durdurmak icin kullanilir, DMA1_CH3 receive kapatilir...
 * @return 
 */
int8_t ModbusAdapterIotReceiverStopDMA();

/*
 * @brief RS485 icin DIR pini yonetimini saglayan fonksiyondur
 * @return 
 */
int8_t isModbusAdapterIotTransmitDone();
 #endif // _MODBUS_ADAPTER_IOT_DMA_H_