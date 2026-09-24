/*
 * j1939-adapter.h
 *
 *  Created on: Sep 30, 2022
 *      Author: eren.akyol
 */

#ifndef J1939_ADAPTER_H_
#define J1939_ADAPTER_H_


#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------ Includes ------------------------------ */
#include <J1939Module.h>

/* ------------------------------ Exported Defines ------------------------------ */
// Extension macro to initialise the CANBus interface
#define __initCAN(CANBUS_PERIPHERAL_HANDLE)     initCAN(CANBUS_PERIPHERAL_HANDLE)
// Extension macro to run the CANBus interface periodically
#define __runCAN()      runCAN()

/* ------------------------------ Interface Functions ------------------------------ */

/**
 * @brief CANbus Initialisation function.
 */
void initCAN(
    #if defined (__FDCAN_H__)
    FDCAN_HandleTypeDef*  handle // The CANBus handle
    #elif defined (__CAN_H__)
    CAN_HandleTypeDef*  handle // The CANBus handle
    #endif
);

/**
 * @brief Periodic runner function for the CANbus Interface.
 * @note Only running J1939 Interface since it is the only protocol supported.
 */
void runCAN();

#ifdef __cplusplus
}
#endif


#endif /* J1939_ADAPTER_H_ */
