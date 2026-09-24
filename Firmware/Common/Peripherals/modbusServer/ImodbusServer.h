/**
 * @file ImodbusServer.h
 * @author ENKO Electronics
 * @brief  Modbus Server Interface header file.
 * @version 0.1
 * @date 2023-05-22
 *
 * @copyright Copyright (c) 2023 @ ENKO Electronics
 *
 */

#ifndef IMODBUSSERVER_H
#define IMODBUSSERVER_H

#include "modbusServer.h"

/**
 * @brief Function to call when save enabled modbus addresses set. This should be implemented with app specific details.
 * @note this function could be used for FRAM etc. calls to store parameter statically.
 * Also modbusRegisters array could be stored as bulk in case of Flash memory types.
 */
void ImodbusSaveParam(
    const uint16_t indexAddr, //modbus address that should be saved.
    uint16_t data 	 //data to save on storage
);

/**
 * @brief Backup current password values to secure storage
 */
void ImodbusBackupPasswords(void);

/**
 * @brief Restore password values from secure storage
 */
void ImodbusRestorePasswords(void);

/**
 * @brief Hide passwords based on current user level
 */
void ImodbusHidePasswordsBasedOnLevel(void);

#endif // IMODBUSSERVER_H
