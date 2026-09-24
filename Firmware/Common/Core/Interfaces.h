/**
 * @file Interfaces.h
 * @author C.U.
 * @brief This C/H files contains interface functions to peripherals
 *        Like __initEEPROM(), __initMODBUS() etc.
 *        This way all DBC2 core spesific code can live in it
 * 
 */

#ifndef INTERFACES_H
#define INTERFACES_H

#include "Controller.h"
#include "stm32f0xx_hal.h"

#include "ImodbusServer.h"
#include "ImodbusCallback.h"

#include "main.h"
#include "defaults.h"
#include "modbus-table.h"

#include <IJ1939.h>


/**
 * @brief Vector table moved to first 200 bytes of RAM
 *        For bootloader to work.
 *        This function loads vector table to RAM before app starts
 * 
 */
void _initVectorTable(void);

/**
 * @brief If DEBUG_MODE_OPTIONS not defined this function checks and locks option byte
 * 
 */
void _lockOB(void);

/**
 * @brief Init modbus server object with default values
 * 
 */
void _initModbus(void);

/**
 * @brief Load parameters from Flash, check if they fit in their respective range.
 *        If there is error set param to default.
 *        Checks calibration saves status and loads if there is any
 * 
 */
void _loadParameters(void);

/**
 * @brief 1ms timer callback function to handle timed core functions
 * 
 */
void Callback1ms(void);

/**
 * @brief Start the actual core app. This function should be called after device inits and before loop starts
 * 
 */
void _corePostInit(void);

/**
 * @brief Core loop function. This function should be called on loop.
 * 
 */
void _coreLoop(void);

/**
 * @brief Determines the current operating mode of the device
 * @param measured Pointer to measured values structure
 * @Operating mode value (0-4)
 */
void getOperatingMode(MEASURED_t *measured);


#endif //INTERFACES_H
