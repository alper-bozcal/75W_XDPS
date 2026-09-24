/*
 * J1939-module.h
 *
 *  Created on: Sep 28, 2022
 *      Author: eren.akyol
 *
 *      @warning
 *
 *		Alinan paket sayisi stm32f3 yapisindan dolayı maks. 14 list mask yapabilmektedir.
 *		(stm32f303 referance manuel - 31.7.4 Identifier filtering - sayfa=1020)
 *
 *		j1939 projesine ozeldir.
 */

#ifndef J1939_MODULE_H_
#define J1939_MODULE_H_

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------ Includes ------------------------------ */
// Interface file for the static defines.
// It should contain at least J1939_SPECIFIC_PACKET_NUMBER and J1939_STANDART_PACKET_NUMBER defines for the size of the table
// It should contain can.h or fdcan.h depending on the device
#include <IJ1939Config.h>

/* ------------------------------ Private Defines ------------------------------ */
#if defined (J1939_STANDART_PACKET_NUMBER)
  #define J1939_TOTAL_PACKET_NUMBER	    J1939_SPECIFIC_PACKET_NUMBER + J1939_STANDART_PACKET_NUMBER	// Tanınan toplam paket sayısı R/W paketlerin tamamıdır.
  #define MAX_QUEUE_LEN				          J1939_TOTAL_PACKET_NUMBER + 1                               // Write Operasyonu için gerekli queue uzunluğu
  #define CAN_IDE_32                    4// 0b0000100
#else
  #error "Packet counts for the J1939 should be defined statically on build time."
#endif

/* ------------------------------ Private Types ------------------------------ */

/**
 * @brief R/W operation selection defines for the J1939 PGNs table
 * @note  Access Level Field is 2 bit. This enum can't go higher than 3!
 */
typedef enum J1939accessLevel_e{
  CAN_ACCESS_ERR   = 0,     // No R/W Operation selected Error
  CAN_ACCESS_READ  = 1,     // Read Operation selected for this PGN
  CAN_ACCESS_WRITE = 2,     // Write Operation selected for this PGN
  CAN_ACCESS_PDU   = 3,     // Read OP: This is not a PGN, rather a PDU filter
}J1939accessLevel;

/**
 * @brief Request Status Indication Types
 */
typedef enum RequestState_e{
  REQ_OKEY = 0,
  REQ_FAIL = 1,
}RequestState;

/**
 * @brief Index of the filters for CANbus. If used CAN2, filters should start from the 14th
 */
typedef enum StartFilterBankNumber_e{
  START_FILTER_BANK_NUMBER_CAN1 = 0,
  START_FILTER_BANK_NUMBER_CAN2 = 14,
}StartFilterBankNumber;

/**
 * @brief queue Node type for the CANbus
 */
typedef struct J1939Request_s{
	uint8_t  priority :3;     // Priority of this instance
	uint8_t  DLC      :5;     // DLC of the PGN of this instance
	uint8_t  SRC;             // SRC address incoming from the other device
	uint16_t PGN;             // PGN of this instance
	uint8_t  buffer[8];       // Data Buffer for this instance
	// Callback function for calling just before W or after R OP
	void (*CallbackPtr)(struct J1939Request_s *self);
}J1939Request_t;

/**
 * @brief Callback function type for the every PGN in the table
 * They get called before if it is W or after if it is R OP.
 */
typedef void (*J1939CallbackFnPtr) (J1939Request_t* self);

/**
 * @brief PGN objects type for filling in the table
 */
typedef struct J1939Packet_s{
  uint16_t          interval;   // Re-transmit time interval as ms
  uint8_t           DLC :5;     // Packet size as bytes for J1939 Protocol
  uint8_t           PRI :3;     // Packet Priority for J1939 Protocol
	uint16_t          PGN;        // Packet ID specified in the J1939 Protocol
	J1939accessLevel  access;	    // Read or Write Operation type selection for the table entry
	J1939CallbackFnPtr Callback;  // Callback function prototype for the specified PGN table entry
}J1939Packet_t;

/**
 * @brief Main queue object type for TX/RX queues. It holds requests and indexes
 */
typedef struct Queue_s{
	J1939Request_t request[MAX_QUEUE_LEN];   //
	uint16_t       head;	                   // ilk firsatta yerine getiriliecek islemin indeks numarasini belirtir.s
	uint16_t       tail;	                   // Son sirada yer alan islemin indeks numarasini belirtir.
}Queue;

/**
 * @brief Main type for the J1939 supporting CANbus object
 * @note This type has few HAL related fields so be careful when changing MCUs!
 */
typedef struct J1939Handle_s{

  #if defined (__FDCAN_H__)
  FDCAN_HandleTypeDef*  handle;           // The CANBus handle
  FDCAN_FilterTypeDef   sFilterConfig;
  FDCAN_RxHeaderTypeDef RxHeader;
  FDCAN_TxHeaderTypeDef TxHeader;
  #elif defined (__CAN_H__)
  CAN_HandleTypeDef*    handle; // The CANBus handle
  CAN_FilterTypeDef     sFilterConfig;
  CAN_RxHeaderTypeDef   RxHeader;
  CAN_TxHeaderTypeDef   TxHeader;
  uint32_t              TxMailbox;
  #else
    #error "One of the CAN HAL Interfaces must be defined"
  #endif
  uint8_t               txReady     :1;  // CANbus Peripheral is ready for TX OP. Can be 0 or 1
  uint8_t               rxReady     :1;  // CANbus Peripheral is ready for RX OP. Can be 0 or 1
  uint8_t               filterIndex :6;  // CANbus Peripheral HW filter index. Can be 64 max
  uint8_t               srcAddr;         // Parametric Source Address of the ECU to use in J1939 packet
  uint8_t               destAddr;        // Parametric Destination Address of the target ECU this one shuld listen
  uint16_t              tick;            // TX Interval timer counter
  uint8_t               rxBuf[8];        // RX data buffer
  J1939Packet_t         table[J1939_TOTAL_PACKET_NUMBER];
	Queue                 rxQ;             // RX OP requests' buffer for queuing until MCU can process data
	Queue                 txQ;             // TX OP requests' buffer for queuing until MCU can process data
}J1939Handle_t;

/* ------------------------------ Module Functions ------------------------------ */

/*
 * @brief Initialise the J1939 Object with associated CANbus obj and table
 *
 */
void J1939Init(
    J1939Handle_t* j1939Obj,
    #if defined (__FDCAN_H__)
    FDCAN_HandleTypeDef*  handle // The CANBus handle
    #elif defined (__CAN_H__)
    CAN_HandleTypeDef*  handle // The CANBus handle
    #else
      #error "One of the CAN HAL Interfaces must be defined"
    #endif
    );

/*
 * @brief Ecu tablo adresini temizler.
 * @param
 * @param
 */
void J1939DeInit(J1939Handle_t* j1939Obj);

/**
 * @brief Clear the PGN table starting from offset
 * @note  This function suppose to be used in multi-table cases.
 */
void J1939Clear(J1939Handle_t* j1939Obj, uint16_t offset);

/*
 * @brief Ecu gonderim paketlerinin rutinidir.
 * @note This function should be called in a strict 1 ms interval
 */
void J1939Run(J1939Handle_t* j1939Obj);

/**
 * @brief Process new incoming packet and add to the related queue if it is OK
 */
void J1939ReceiveNewPacket(J1939Handle_t* j1939Obj);

/**
 * @brief Add specific PGN to the Transmit queue for next available transmission
 * @note  This function can be used to transmit one time only packets too
 */
void J1939addTransmitRequest(J1939Handle_t* j1939Obj, uint8_t priority, uint16_t PGN, uint8_t DLC,  J1939CallbackFnPtr CallbackPtr);

/**
 * @brief Add specific PGN to receive queue for next processing cycle
 */
void J1939addReceiveRequest(J1939Handle_t* j1939Obj, uint8_t priority, uint16_t PGN, uint8_t DLC, uint8_t SRC, uint8_t *buffer, J1939CallbackFnPtr CallbackPtr);

/*
 * @brief Ecu gonderim paketlerinin en uygun kosulda gonderiliminin yapildigi fonksiyondur.
 * Bu fonnkiyon 1 ms lik bir task'in icinde ve Can Tx Interrupt'inin icinde cagrilmalidir.
 * @param uint8_t kosul 0 ise gonderim yapilacak kosul 1 ise gonderim yapilmayacak
 * @param
 */
void J1939Transmit(J1939Handle_t* j1939Obj);

/**
 * @brief Check for receive IT Flag and process if there is new on the receive queue
 */
void J1939Receive(J1939Handle_t* j1939Obj, J1939Request_t *request);

/*
 * @brief Transmit Request Function
 * @param ext_id: extended identifier
 * @param dlc: the length of the frame that will be transmitted
 */
uint8_t addJ1939TransmitRequest(J1939Handle_t* j1939Obj,uint8_t *priority,uint16_t PGN,uint8_t *src_adr,uint8_t dlc,J1939CallbackFnPtr Callback);

/**
 * @brief Add Receive request packet to the receive queue
 */
uint8_t addJ1939ReceiveRequest(J1939Handle_t* j1939Obj, uint8_t priority, uint16_t PGN, uint8_t src_adr, uint8_t dlc, uint8_t *buffer, J1939CallbackFnPtr Callback);

/*
 * @brief J1939 nesnesine adapterde belirtlen tablolarin atamasinin yapildigi fonksiyondur.
 * @param
 * @param
 * @param
 */
void J1939FillTable(J1939Handle_t* j1939Obj, const J1939Packet_t *addr, uint16_t size, uint8_t table_offset);

/**
 * @brief // list mask ile mesajlar filtrelenir
 */
void setAcceptanceFilter(J1939Handle_t* j1939Obj, StartFilterBankNumber offset);

/**
 * @brief // min ile max filter_id degerleri girilerek bu aralıkta filtreleme yapılır.
 */
void setCanFilterID(J1939Handle_t* j1939Obj,uint32_t min_id,uint32_t max_id,uint8_t offset);

#ifdef __cplusplus
}
#endif

#endif /* J1939_MODULE_H_ */
