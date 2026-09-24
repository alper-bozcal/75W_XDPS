/*
 * modbus_adapter_usb.c
 *
 *  Created on: 30 Jan 2017
 *      Author: EN
 *
 *  @note: USB Full-Speed haberlesme hizinda musade edilen maksimum transfer boyutu
 *  		64Byte oldugu icin PC tarafindan cihazimiza(STM32f303) MULTI_WRITE fonksiyonu ile
 *  		27 registerdan daha fazlasini yazamiyoruz. Modbus-Master yaziliminin 27 registerdan
 *  		daha yuksek boyuttaki talepleri bir kac parcaya ayirmasi gerekiyor.
 *  		Ancak 100 registeri okuyabiliyoruz burada problem yok. Yani STM32'den yaklasik 100*2 bytelik
 *  		veri gonderilebiliyor CDC_Transmit_FS fonksiyonu ile... Muhtemelen bir kac parcaya bolerek
 *  		gonderim islemi STM'nin driveri tarafindan gerceklestiriliyor. Bknz. HAL_PCD_EP_Transmit() fonksiyonu.
 */


#include "modbus_adapter_usb.h"
#include "ModbusServerSerial/msserial.h"
#include "usbd_cdc_if.h"
#include "usb_device.h"

//extern USBD_HandleTypeDef hUsbDeviceFS;
//extern PCD_HandleTypeDef hpcd_USB_FS;
//extern ModbusSerial_t mbUsb;
extern ModbusServerSerial_t MbServerUsbObj;
extern int16_t CDC_getReceived_FS();
extern void CDC_setReceivedCounter(uint16_t val);
uint16_t debug = 0;
void ModbusAdapterUsbInit(){
	//MX_USB_DEVICE_Init();
	MbServerUsbObj.size_rcv = 0;
	MbServerUsbObj.size_rcv_prev = 0;
}

int8_t ModbusAdapterTransmitterUsb(int8_t *buffer, uint16_t size){

	MbServerUsbObj.size_rcv = 0;
	MbServerUsbObj.size_rcv_prev = 0;

	CDC_Transmit_FS((uint8_t *)buffer, size);
	return 0;
}

uint16_t ModbusAdapterReceiverUsb(int8_t *buffer, uint16_t size){

	/*
	 *	usbd_cdc_if.c dosyasindaki CDC_Receive fonksiyonu icinde cagrilan
	 *	  USBD_CDC_SetRxBuffer(hUsbDevice_0, &Buf[0]); USBD_CDC_ReceivePacket(hUsbDevice_0);
	 *	  fonksiyonlari sonrasinda xfer_count yani ilgili endpoint uzerinden alinan byte uzunlugu
	 *	  siliniyor. Bu nedenle mbUsb.Size_rcv degeri cdc_if.c dosyasinda gerceklestirilmistir.
	 *
	 */

	return CDC_getReceived_FS();
}

int8_t ModbusAdapterReceiverStopUsb(){

	/*
	 * Receive asamasinda alinan veride bir hata varsa ornegin CRC,
	 * Transmit asamasina gecilemeyecegi icin her stop islemi ardindan
	 * USB uzerinden alinan toplam byte sayaci sifirlanmalidir.
	 */
	CDC_setReceivedCounter(0);
	return 0;
}

int8_t isModbusAdapterTransmitUSBDone(){
	return 1;
}
