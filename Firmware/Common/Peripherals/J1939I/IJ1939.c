/*
 * j1939-adapter.c
 *
 *  Created on: Sep 30, 2022
 *      Author: eren.akyol
 */

/* ------------------------------ Includes ------------------------------ */
#include <J1939Module.h>
#include <IJ1939Callback.h>

/* ------------------------------ Private Variables ------------------------------ */
J1939Handle_t J1939;

const J1939Packet_t COMMON_ENGINE_TABLE[J1939_STANDART_PACKET_NUMBER] = {
 /* Interval [ms]  Length(DLC)[Byte]   Priority      PGN              Access Level RW       Callback Function */
  { 1000,          8,                  6,            65104,           CAN_ACCESS_WRITE,     sendPGN65104},
  { 1000,          8,                  6,            65106,           CAN_ACCESS_WRITE,     sendPGN65106},
  { 1000,          8,                  6,            65271,           CAN_ACCESS_WRITE,     sendPGN65271},
};


/* ------------------------------ Interface Public Functions ------------------------------ */

void initCAN(
    #if defined (__FDCAN_H__)
    FDCAN_HandleTypeDef*  handle // The CANBus handle
    #elif defined (__CAN_H__)
    CAN_HandleTypeDef*  handle // The CANBus handle
    #endif
)
{
  // Initialise the CANBus core object
  J1939Init(&J1939,handle);
  J1939FillTable(&J1939, COMMON_ENGINE_TABLE, sizeof(COMMON_ENGINE_TABLE) / sizeof(J1939Packet_t), 0);
  setAcceptanceFilter(&J1939, START_FILTER_BANK_NUMBER_CAN1);
  J1939.txReady = 1;

  #if defined (__FDCAN_H__)
    HAL_FDCAN_Start(handle);
  #elif defined (__CAN_H__)
    HAL_CAN_Start(handle);
  #endif

#if defined (__FDCAN_H__)
  HAL_FDCAN_ActivateNotification(handle, FDCAN_FLAG_RX_FIFO0_NEW_MESSAGE | FDCAN_FLAG_TX_FIFO_EMPTY, FDCAN_TX_BUFFER0);
#elif defined (__CAN_H__)
  HAL_CAN_ActivateNotification(handle,   CAN_IT_RX_FIFO0_MSG_PENDING | CAN_IT_TX_MAILBOX_EMPTY);
#endif
}

void runCAN(
){
  J1939Run(&J1939);
}

/**
 * These are the CANbus Interrupts.
 * Use them only for setting flags. OR else MCU can get overwhelmed.
 */
#if defined (__FDCAN_H__)
  void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs){
    J1939.rxReady = 1;
  }
  void HAL_FDCAN_TxBufferCompleteCallback(FDCAN_HandleTypeDef *hfdcan, uint32_t BufferIndexes){
    J1939.txReady = 1;
  }
#elif defined (__CAN_H__)
  void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
  {
    J1939.rxReady = 1;
    // If there is anything new on the RX Queue, Start receive process
    J1939Request_t *request = NULL;
    J1939Receive(&J1939, request);
  }
  void HAL_CAN_TxMailbox0CompleteCallback(CAN_HandleTypeDef *hcan){
    J1939.txReady = 1;
    // If TxReady, send the next one from the TX Queue
    J1939Transmit(&J1939);
  }
#endif
