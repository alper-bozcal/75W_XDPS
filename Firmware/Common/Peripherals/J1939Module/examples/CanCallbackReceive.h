/*
 * CanCallbackReceive.h
 *
 *  Created on: Sep 21, 2022
 *      Author: eren.akyol
 */

#ifndef CANCALLBACKRECEIVE_H_
#define CANCALLBACKRECEIVE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"

#include "j1939-adapter.h"

/*
 * @brief         :
 * @param[]       :
 * @return        :
 * @precondition  :
 * @postcondition :
 */
void J1939CallbackF004(uint8_t *buffer,uint16_t pgn,uint8_t *priority,uint8_t *src_adr);

/*
 * @brief         :
 * @param[]       :
 * @return        :
 * @precondition  :
 * @postcondition :
 */
void J1939CallbackFEEE(uint8_t *buffer,uint16_t pgn,uint8_t *priority,uint8_t *src_adr);

/*
 * @brief         :
 * @param[]       :
 * @return        :
 * @precondition  :
 * @postcondition :
 */
void J1939CallbackFEEF(uint8_t *buffer,uint16_t pgn,uint8_t *priority,uint8_t *src_adr);

/*
 * @brief         :
 * @param[]       :
 * @return        :
 * @precondition  :
 * @postcondition :
 */
void J1939CallbackFECA(uint8_t *buffer,uint16_t pgn,uint8_t *priority,uint8_t *src_adr);

/*
 * @brief         :
 * @param[]       :
 * @return        :
 * @precondition  :
 * @postcondition :
 */
void J1939CallbackBAMEXXX(uint8_t *buffer,uint16_t pgn,uint8_t *priority,uint8_t *src_adr);


/*
 * @brief         :
 * @param[]       :
 * @return        :
 * @precondition  :
 * @postcondition :
 */
void J1939CallbackFF47(uint8_t *buffer,uint16_t pgn,uint8_t *priority,uint8_t *src_adr);

#ifdef __cplusplus
}
#endif

#endif /* CANCALLBACKRECEIVE_H_ */
