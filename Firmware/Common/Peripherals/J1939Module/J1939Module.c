/*
#include <J1939-module.h>
 * ecu_setup.c
 *
 *  Created on: Aug 15, 2022
 *      Author: eren.akyol
 *
 *
 *		CAN1 ve CAN2 ayni anda kullanilmak isteniyorsa CAN2'nin filter bank'ini 14 offset ile baslatmak gerekiyor.
 *      https://stackoverflow.com/questions/65290032/using-both-can1-can2-both-in-stm32f446-properly
 *
 *      Sonuc olarak Receiver Mask islemleri icin;
 *
 *      CAN1 - 0...13
 *      CAN2 - 14..28
 *
 *      Filter Banklari kullanilmak gerekmektedir. Bunun icinde CAN2 ye bagli obje tanimlanmasi yapilirken
 *
 *      setAcceptanceFilter fonksiyonun offset parametresine 14 eklemek yeterli olacaktir.
 *
 *
 */

#include <J1939Module.h>

/* ------------------------------ Inline Functions ------------------------------ */

/**
 * @brief Check for queue status
 * @retval REQ_OKEY if it is not empty. REQ_FAIL if it is
 */
RequestState isNewRequestAvaible(Queue *queue){

  return ((queue->head != queue->tail) ? REQ_OKEY: REQ_FAIL);
}

/**
 * @brief Add Node to the Queue of the CANbus object
 */
int8_t enqueue(Queue *queue, J1939accessLevel RWoperation, uint8_t priority, uint16_t PGN, uint8_t DLC, uint8_t SRC, uint8_t *buffer, J1939CallbackFnPtr Callback){

  if(queue->head < MAX_QUEUE_LEN){

    queue->request[queue->head].priority    = priority;
    queue->request[queue->head].PGN         = PGN;
    queue->request[queue->head].DLC         = DLC;
    queue->request[queue->head].SRC         = SRC;
    queue->request[queue->head].CallbackPtr = Callback;

    switch(RWoperation){

    case CAN_ACCESS_ERR:
      break;

    case CAN_ACCESS_WRITE:

      for(uint8_t i = 0; i < 8; ++i){
        queue->request[queue->head].buffer[i] = 0xFF;
      }

      if( *(queue->request[queue->head].CallbackPtr) != NULL){
        (*queue->request[queue->head].CallbackPtr)( &(queue->request[queue->head]) );
      }
      break;

    case CAN_ACCESS_READ:

      for(uint8_t i = 0; i < 8; ++i){
        queue->request[queue->head].buffer[i] = buffer[i];
      }
      break;

    case CAN_ACCESS_PDU:
      for(uint8_t i = 0; i < 8; ++i){
        queue->request[queue->head].buffer[i] = buffer[i];
      }
      break;

    default:
      break;
    }

    ++queue->head;
    if(queue->head >= MAX_QUEUE_LEN) queue->head = 0;

    return REQ_OKEY;
  }else{
    return REQ_FAIL;
  }
}

/**
 * @brief Pop the next element present in the queue
 */
J1939Request_t *dequeue(Queue *queue){
  uint16_t index;

  if(isNewRequestAvaible(queue) == REQ_OKEY){

    index = queue->tail;

    queue->tail++;
    if(queue->tail >= MAX_QUEUE_LEN) queue->tail = 0;

    return &queue->request[index];
  }else{
    return NULL;
  }
}

/**
 * @brief Calculate Filter mask of the given PGN ID for sFilterConfig
 */
uint32_t filterId2MaskVal(uint32_t min_val, uint32_t max_val){

  uint32_t mask_val = 0;

  for(uint8_t i = 0; i < 8; ++i){

    uint8_t temp = 0;
    uint8_t xMin = (min_val >> i * 4) & 0x0F;
    uint8_t xMax = (max_val >> i * 4) & 0x0F;

    if(xMin == xMax){
      temp =  0x0F;
    }else{
      temp =  0x00;
    }

    mask_val |= (uint32_t)temp << i * 4;
  }

  return mask_val;
}

/* ------------------------------ Module Public Functions ------------------------------ */

void J1939addTransmitRequest(J1939Handle_t* j1939Obj, uint8_t priority, uint16_t PGN, uint8_t DLC, J1939CallbackFnPtr CallbackPtr){

  if(enqueue(&j1939Obj->txQ, CAN_ACCESS_WRITE, priority, PGN, DLC, 0/*SRC is set to obj->srcAddr*/, NULL, CallbackPtr) == REQ_OKEY){
  }
}

void J1939addReceiveRequest(J1939Handle_t* j1939Obj, uint8_t priority, uint16_t PGN, uint8_t DLC,  uint8_t SRC, uint8_t *buffer, J1939CallbackFnPtr CallbackPtr){

  if(enqueue(&j1939Obj->rxQ, CAN_ACCESS_READ, priority, PGN, DLC, SRC, buffer, CallbackPtr) == REQ_OKEY){
  }
}

void J1939Init(
    J1939Handle_t* j1939Obj,
    #if defined (__FDCAN_H__)
    FDCAN_HandleTypeDef*  handle // The CANBus handle
    #elif defined (__CAN_H__)
    CAN_HandleTypeDef*  handle // The CANBus handle
    #endif
  ){

  j1939Obj->handle       = handle;
  j1939Obj->filterIndex  = 0;
  j1939Obj->rxReady      = 0;
  j1939Obj->tick         = 0;
  j1939Obj->txReady      = 0;
  j1939Obj->srcAddr      = 0;
  j1939Obj->destAddr     = 0;

#if defined (__FDCAN_H__)
  j1939Obj->TxHeader.IdType = FDCAN_EXTENDED_ID;
  j1939Obj->TxHeader.TxFrameType = FDCAN_DATA_FRAME;
  j1939Obj->TxHeader.DataLength = FDCAN_DLC_BYTES_8;
  j1939Obj->TxHeader.ErrorStateIndicator = FDCAN_ESI_PASSIVE;
  j1939Obj->TxHeader.BitRateSwitch = FDCAN_BRS_OFF;
  j1939Obj->TxHeader.FDFormat = FDCAN_CLASSIC_CAN;
  j1939Obj->TxHeader.TxEventFifoControl = FDCAN_STORE_TX_EVENTS;
  j1939Obj->TxHeader.MessageMarker = 0xCC;
#elif defined (__CAN_H__)
  j1939Obj->TxHeader.RTR = CAN_RTR_DATA;
  j1939Obj->TxHeader.IDE = CAN_ID_EXT;
  j1939Obj->TxHeader.DLC = 8;
  j1939Obj->TxHeader.TransmitGlobalTime = DISABLE;
#endif
}

void J1939DeInit(J1939Handle_t* j1939Obj){

#if defined (__FDCAN_H__)
  HAL_FDCAN_Stop(j1939Obj->handle);
#elif defined (__CAN_H__)
  HAL_CAN_Stop(j1939Obj->handle);
#endif

#if defined (__FDCAN_H__)
  HAL_FDCAN_DeactivateNotification(j1939Obj->handle, FDCAN_FLAG_RX_FIFO0_NEW_MESSAGE | FDCAN_FLAG_TX_FIFO_EMPTY);
#elif defined (__CAN_H__)
  HAL_CAN_DeactivateNotification(j1939Obj->handle,   CAN_IT_RX_FIFO0_MSG_PENDING | CAN_IT_TX_MAILBOX_EMPTY);
#endif

  j1939Obj->handle       = NULL;
  j1939Obj->filterIndex  = 0;
  j1939Obj->rxReady      = 0;
  j1939Obj->tick         = 0;
  j1939Obj->txReady      = 0;
  j1939Obj->srcAddr      = 0;
  j1939Obj->destAddr     = 0;

  for(uint8_t i = 0; i < J1939_TOTAL_PACKET_NUMBER; ++i){
    j1939Obj->table[i].DLC      = 0;
    j1939Obj->table[i].PGN      = 0;
    j1939Obj->table[i].PRI      = 0;
    j1939Obj->table[i].access   = 0;
    j1939Obj->table[i].interval = 0;
  }
}

void J1939Clear(J1939Handle_t* j1939Obj, uint16_t offset){

  for(uint8_t i = offset; i < J1939_TOTAL_PACKET_NUMBER; ++i){
    j1939Obj->table[i].DLC      = 0;
    j1939Obj->table[i].PGN      = 0;
    j1939Obj->table[i].PRI      = 0;
    j1939Obj->table[i].access   = 0;
    j1939Obj->table[i].interval = 0;
  }
}

void J1939Transmit(J1939Handle_t *j1939Obj){

  if(!(j1939Obj->txReady) ){
    return;
  }

  J1939Request_t *request = dequeue(&j1939Obj->txQ);
  if(request == NULL){
    return;
  }

  j1939Obj->txReady        = 0;

  #if defined (__FDCAN_H__)
  if(request->DLC < FDCAN_DLC_BYTES_8) j1939Obj->TxHeader.DataLength = request->DLC;
  j1939Obj->TxHeader.Identifier = ((uint32_t)(request->priority) << 26) | ((uint32_t)(request->PGN) << 8) | (uint32_t)(j1939Obj->srcAddr);
  HAL_FDCAN_AddMessageToTxFifoQ(j1939Obj->handle, &j1939Obj->TxHeader, request->buffer);
  #elif defined (__CAN_H__)
  j1939Obj->TxHeader.DLC   = request->DLC;
  j1939Obj->TxHeader.ExtId = ((uint32_t)(request->priority) << 26) | ((uint32_t)(request->PGN) << 8) | (uint32_t)(j1939Obj->srcAddr);
  HAL_CAN_AddTxMessage(j1939Obj->handle, &j1939Obj->TxHeader, request->buffer, &j1939Obj->TxMailbox);
  #endif
}

void J1939Receive(J1939Handle_t *j1939Obj, J1939Request_t *request){

  if(j1939Obj->rxReady){
    J1939ReceiveNewPacket(j1939Obj);
    j1939Obj->rxReady = 0;
  }

  request = dequeue(&j1939Obj->rxQ);
  if(request != NULL){
    if( request->CallbackPtr != NULL){
      request->CallbackPtr(request);
    }
  }
}

void J1939Run(J1939Handle_t *j1939Obj)
{

  ++j1939Obj->tick;

  // Check for available periodic TX requests and add them to queue if interval is full
  for(uint8_t i = 0; i < J1939_TOTAL_PACKET_NUMBER; ++i){

    if(j1939Obj->table[i].access == CAN_ACCESS_WRITE){

      uint16_t interval = j1939Obj->table[i].interval;

      if(interval != 0){
        if(j1939Obj->tick % interval == 0){

          J1939addTransmitRequest(j1939Obj,
                                  j1939Obj->table[i].PRI,
                                  j1939Obj->table[i].PGN,
                                  j1939Obj->table[i].DLC,
                                  (*j1939Obj->table[i].Callback));
        }
      }
    }
  }

  // If TxReady, send the next one from the TX Queue
  J1939Transmit(j1939Obj);
}

void J1939ReceiveNewPacket(J1939Handle_t *j1939Obj){

  #if defined (__FDCAN_H__)
  HAL_FDCAN_GetRxMessage(j1939Obj->handle, FDCAN_RX_FIFO0, &j1939Obj->RxHeader, j1939Obj->rxBuf);
  uint16_t PGN =  j1939Obj->RxHeader.Identifier >> 8;
  uint8_t  PDU = (j1939Obj->RxHeader.Identifier >> 16) & 0xF0;

  for(uint8_t i = 0; i < J1939_TOTAL_PACKET_NUMBER; ++i){

    if(j1939Obj->table[i].access == CAN_ACCESS_READ){
      if( (j1939Obj->table[i].PGN == PGN)){
        J1939addReceiveRequest(j1939Obj,
                               j1939Obj->RxHeader.Identifier >> 26,
                               PGN,
                               j1939Obj->RxHeader.DataLength,
                               (uint8_t)(j1939Obj->RxHeader.Identifier & 0x000000FF),
                               j1939Obj->rxBuf,
                               (*j1939Obj->table[i].Callback)
                               );
        break;
      }
    }else if(j1939Obj->table[i].access == CAN_ACCESS_PDU){
      uint8_t PDU_i = (j1939Obj->table[i].PGN >> 8) & 0xF0;
      if(PDU_i == PDU){
        J1939addReceiveRequest(j1939Obj,
                               j1939Obj->RxHeader.Identifier >> 26,
                               PGN,
                               j1939Obj->RxHeader.DataLength,
                               (uint8_t)(j1939Obj->RxHeader.Identifier & 0x000000FF),
                               j1939Obj->rxBuf,
                               (*j1939Obj->table[i].Callback)
                               );
        break;
      }
    }
  }
  #elif defined (__CAN_H__)
  HAL_CAN_GetRxMessage(j1939Obj->handle, CAN_RX_FIFO0, &j1939Obj->RxHeader, j1939Obj->rxBuf);
  uint16_t PGN =  j1939Obj->RxHeader.ExtId >> 8;
  uint8_t  PDU = (j1939Obj->RxHeader.ExtId >> 16) & 0xF0;

  for(uint8_t i = 0; i < J1939_TOTAL_PACKET_NUMBER; ++i){

    if(j1939Obj->table[i].access == CAN_ACCESS_READ){
      if( (j1939Obj->table[i].PGN == PGN)){
        J1939addReceiveRequest(j1939Obj,
                               j1939Obj->RxHeader.ExtId >> 26,
                               PGN,
                               j1939Obj->RxHeader.DLC,
                               (uint8_t)(j1939Obj->RxHeader.ExtId & 0x000000FF),
                               j1939Obj->rxBuf,
                               (*j1939Obj->table[i].Callback)
                               );
        break;
      }
    }else if(j1939Obj->table[i].access == CAN_ACCESS_PDU){
      uint8_t PDU_i = (j1939Obj->table[i].PGN >> 8) & 0xF0;
      if(PDU_i == PDU){
        J1939addReceiveRequest(j1939Obj,
                               j1939Obj->RxHeader.ExtId >> 26,
                               PGN,
                               j1939Obj->RxHeader.DLC,
                               (uint8_t)(j1939Obj->RxHeader.ExtId & 0x000000FF),
                               j1939Obj->rxBuf,
                               (*j1939Obj->table[i].Callback)
                               );
        break;
      }
    }
  }
  #endif
}

void J1939FillTable(J1939Handle_t* j1939Obj, const J1939Packet_t *addr, uint16_t size, uint8_t table_offset){

  for(uint8_t i = 0; i < size; ++i){
    j1939Obj->table[i + table_offset].Callback = *(addr + i)->Callback;
    j1939Obj->table[i + table_offset].PGN      = (*(addr + i)).PGN;
    j1939Obj->table[i + table_offset].access   = (*(addr + i)).access;
    j1939Obj->table[i + table_offset].DLC      = (*(addr + i)).DLC;
    j1939Obj->table[i + table_offset].PRI      = (*(addr + i)).PRI;
    j1939Obj->table[i + table_offset].interval = (*(addr + i)).interval;
  }
}

void setAcceptanceFilter(J1939Handle_t* j1939Obj, StartFilterBankNumber offset){

  #if defined (__FDCAN_H__)
  //! There could be errors when initialising the Filter! Check this first in case no RX OP!
  j1939Obj->sFilterConfig.IdType               = FDCAN_EXTENDED_ID;
  j1939Obj->sFilterConfig.FilterType           = FDCAN_FILTER_MASK;
  j1939Obj->sFilterConfig.FilterConfig         = FDCAN_FILTER_TO_RXFIFO0;
  /* Configure global filter:
       Filter all remote frames with STD and EXT ID
       Reject non matching frames with STD ID and EXT ID */
//    HAL_FDCAN_ConfigGlobalFilter(j1939Obj->handle, FDCAN_REJECT, FDCAN_REJECT, FDCAN_FILTER_REMOTE, FDCAN_FILTER_REMOTE);

  j1939Obj->filterIndex = 0;

  for(uint8_t i = 0; i < J1939_TOTAL_PACKET_NUMBER; ++i){

    J1939Packet_t *table = (J1939Packet_t*)j1939Obj->table + i;

    uint32_t filter_id     = 0x00000000;
    uint32_t filter_id_max = 0xFFFFFFFF;

    if(table->access == CAN_ACCESS_READ){

      filter_id     = ((uint32_t)table->PGN << 8);
      filter_id_max = filter_id | 0x00FFFFFF;
      setCanFilterID(j1939Obj, filter_id, filter_id_max, offset);

    }else if(table->access == CAN_ACCESS_PDU){

      filter_id     = ((uint32_t)table->PGN << 8);
      filter_id_max = filter_id | 0x00FFFFFF;
      setCanFilterID(j1939Obj, filter_id, filter_id_max, offset);
    }
  }

  #elif defined (__CAN_H__)
  j1939Obj->sFilterConfig.FilterBank = 0;
  j1939Obj->sFilterConfig.FilterIdHigh = 0x0000;
  j1939Obj->sFilterConfig.FilterIdLow = 0x0000;
  j1939Obj->sFilterConfig.FilterMaskIdHigh = 0x0000;
  j1939Obj->sFilterConfig.FilterMaskIdLow = 0x0000;
  j1939Obj->sFilterConfig.FilterMode           = CAN_FILTERMODE_IDMASK;
  j1939Obj->sFilterConfig.FilterScale          = CAN_FILTERSCALE_32BIT;
  j1939Obj->sFilterConfig.FilterFIFOAssignment = CAN_RX_FIFO0;
  j1939Obj->sFilterConfig.FilterActivation     = ENABLE;
  j1939Obj->sFilterConfig.SlaveStartFilterBank = 14;
  HAL_CAN_ConfigFilter(j1939Obj->handle, &j1939Obj->sFilterConfig);

  #endif

}

void setCanFilterID(J1939Handle_t* j1939Obj, uint32_t min_id, uint32_t max_id, uint8_t offset){

  // Max Filter Bank Number for F3 is 14. So, limit filter count at 14.
  if(j1939Obj->filterIndex > 13){
    assert("Max CANbus ID Filter count exceeded! Consider filtering PDUs or  no filter");
    j1939Obj->filterIndex = 13;
  }

  #if defined (__FDCAN_H__)
  j1939Obj->sFilterConfig.FilterIndex = j1939Obj->filterIndex + offset;
  j1939Obj->sFilterConfig.FilterID1 = min_id;
  j1939Obj->sFilterConfig.FilterID2 = max_id;

  HAL_FDCAN_ConfigFilter(j1939Obj->handle, &j1939Obj->sFilterConfig);

  #elif defined (__CAN_H__)
  uint32_t filter_mask = filterId2MaskVal(min_id, max_id);
  j1939Obj->sFilterConfig.FilterBank       = j1939Obj->filterIndex+ offset;
  j1939Obj->sFilterConfig.FilterIdHigh     = min_id >> 13;
  j1939Obj->sFilterConfig.FilterIdLow      = min_id << 3 | CAN_IDE_32;
  j1939Obj->sFilterConfig.FilterMaskIdHigh = filter_mask >> 13;
  j1939Obj->sFilterConfig.FilterMaskIdLow  = min_id << 3 | CAN_IDE_32;

  HAL_CAN_ConfigFilter(j1939Obj->handle, &j1939Obj->sFilterConfig);
  #endif
  ++j1939Obj->filterIndex;
}
