/*
 * request_structure.c
 *
 *  Created on: 22 Jan 2018
 *      Author: Nazim Yildiz
 */

#include "stddef.h"
#include "string.h"
#include "request_structure.h"

/*
 * Callback fonksiyonu icin bos fonksiyon. Gelistirici callback kullanmak istemiyorsa bos bir fonksiyon atanmis olmali ki
 * calisma aninda Hard-Fault problemi ile karsilasilmasin.
 */
static void CallBackNull(int16_t *register_arr, uint16_t reg_count){}

ERROR_REQUEST_e modbusRequestAdd(ModbusRequest_t *MbReqListObj, Queue_t *queue, void *header, uint16_t header_size,
		uint8_t func_no, uint16_t start_addr, uint16_t reg_count, int16_t *data){

	static ModbusRequest_t request;
	ERROR_REQUEST_e retval;
	retval = ERROR_OK;

	if(header == NULL){
		retval = ERROR_HEADER_NULL;
		return retval;
	}
	/* ModbusTCP icin header boyutu 7 byte
	 * ModbusSerial icin header boyutu 1 byte*/
	// header boyutu sizeof komutuyla belirlendiginden dolayi
	// mimariden mimariye degisebilir bu nedenle asagidaki denetim
	// kaldirilmistir.
//	if(!(header_size == 1 || header_size == 7)){
//		retval = ERROR_HEADER_SIZE;
//		return retval;
//	}

	/* Istek olusturuluyor*/
	request.header = header;
	request.header_size = header_size;
	request.func_no = func_no;
	request.start_addr = start_addr;
	request.reg_count = reg_count;
	request.buffer = data;
	request.status = QUEUE_FULL;
	request.CallBack = CallBackNull;		/* Callback fonksiyonuna bos fonksiyon ataniyor. Eger ilgili request alininca yapilmasi istenen ekstra islemler
	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 * varsa uygulama katmaninda CallBack() fonksiyonuna atama isleminin yapilmasi gerekir.*/

	/* Istek kuyruga ekleniyor*/
	if(enqueueMC(MbReqListObj, queue, &request) < 0)
		retval = ERROR_QUEUE_FULL;

	return retval;
}

ModbusRequest_t *modbusRequestFetch(ModbusRequest_t *MbReqListObj, Queue_t *queue){
	static ModbusRequest_t request;
	dequeueMC(MbReqListObj, queue, &request);
	return &request;
}

void queueInitMC(Queue_t *queue, uint16_t size){
	queue->head = 0;
	queue->tail = 0;
	queue->field_used = 0;
	queue->len = size;
}

char enqueueMC(ModbusRequest_t *MbReqListObj, Queue_t *queue, ModbusRequest_t *req){

	char retval = -1;

	do{
		/* Kuyrukta yer yoksa bos alan varmi diye aramaya gerek yok*/
		if(queue->field_used >= queue->len){
			break;
		}

		/* Kuyrukta yeni talep ekleyelim*/
		if(MbReqListObj[queue->head].status == QUEUE_EMPTY){
			/* Header bilgisi ekleniyor*/
			for (int16_t i = 0; i < req->header_size; ++i) {
				*((char *)MbReqListObj[queue->head].header + i) = *((char *)req->header + i);
			}
//			memcpy(requestList[queue->head].header, req->header, req->header_size);
			MbReqListObj[queue->head].header_size = req->header_size;

			MbReqListObj[queue->head].func_no = req->func_no;
			MbReqListObj[queue->head].reg_count = req->reg_count;
			MbReqListObj[queue->head].start_addr = req->start_addr;
			MbReqListObj[queue->head].buffer = req->buffer;
			MbReqListObj[queue->head].status = QUEUE_FULL;

			MbReqListObj[queue->head].CallBack = req->CallBack;

			queue->field_used++;
			queue->head++;
			if(queue->head >= queue->len){
				queue->head = 0;
			}
			/* Daha sonra gelecek olan talep istegi icin bir sonraki liste alanini gosterelim*/
//			queue->head = (queue->head >= queue->len) ? 0 : (queue->head + 1);
			retval = 1;
			break;
		}
		else{

//			queue->head = (queue->head >= queue->len) ? 0 : (queue->head + 1);
			queue->head++;
			if(queue->head >= queue->len){
				queue->head = 0;
			}
		}

	}while(queue->head != queue->tail);

	return retval;
}

char dequeueMC(ModbusRequest_t *MbReqListObj, Queue_t *queue, ModbusRequest_t *req){

	char retval = 0;

	if(MbReqListObj[queue->tail].status == QUEUE_FULL){
		req->header = MbReqListObj[queue->tail].header;
		req->header_size = MbReqListObj[queue->tail].header_size;

		req->func_no = MbReqListObj[queue->tail].func_no;
		req->start_addr = MbReqListObj[queue->tail].start_addr;
		req->reg_count = MbReqListObj[queue->tail].reg_count;
		req->buffer = MbReqListObj[queue->tail].buffer;
		req->status = MbReqListObj[queue->tail].status;

		req->CallBack = MbReqListObj[queue->tail].CallBack;

		MbReqListObj[queue->tail].status = QUEUE_EMPTY;
		queue->tail = (queue->tail + 1) >= queue->len ? 0 : (queue->tail + 1);

		if(queue->field_used > 0)
			queue->field_used--;

		retval = 1;
	}
	else{
		req->header = NULL;
		req->header_size = 0;
		req->func_no = 0;
		req->start_addr = 0;
		req->reg_count = 0;
		req->buffer = NULL;
		req->status = QUEUE_EMPTY;
		req->CallBack = CallBackNull;
	}

	return retval;
}
