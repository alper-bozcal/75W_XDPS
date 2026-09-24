/*
 * CanCallbackTransmit.h
 *
 *  Created on: Sep 21, 2022
 *      Author: eren.akyol
 */

#ifndef CANCALLBACKTRANSMIT_H_
#define CANCALLBACKTRANSMIT_H_

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
uint8_t getPerkinsAdemTxFlag(void);

/*
 * @brief         :
 * @param[]       :
 * @return        :
 * @precondition  :
 * @postcondition :
 */
void J1939Callback0000(uint8_t *buffer,uint16_t pgn,uint8_t *priority,uint8_t *src_adr);

/*
 * @brief         :
 * @param[]       :
 * @return        :
 * @precondition  :
 * @postcondition :
 */
void J1939CallbackEA00(uint8_t *buffer,uint16_t pgn,uint8_t *priority,uint8_t *src_adr);

/*
 * @brief         :
 * @param[]       :
 * @return        :
 * @precondition  :
 * @postcondition :
 */
void J1939CallbackFF46(uint8_t *buffer,uint16_t pgn,uint8_t *priority,uint8_t *src_adr);

/*
 * @brief         :
 * @param[]       :
 * @return        :
 * @precondition  :
 * @postcondition :
 */
void J1939CallbackFF02(uint8_t *buffer,uint16_t pgn,uint8_t *priority,uint8_t *src_adr);

/*
 * @brief         :
 * @param[]       :
 * @return        :
 * @precondition  :
 * @postcondition :
 */
void J1939CallbackFF03(uint8_t *buffer,uint16_t pgn,uint8_t *priority,uint8_t *src_adr);

/*
 * @brief         :
 * @param[]       :
 * @return        :
 * @precondition  :
 * @postcondition :
 */
void J1939CallbackFF16(uint8_t *buffer,uint16_t pgn,uint8_t *priority,uint8_t *src_adr);

/*
 * @brief         :
 * @param[]       :
 * @return        :
 * @precondition  :
 * @postcondition :
 */
void J1939CallbackRequestFF16(uint8_t *buffer,uint16_t pgn,uint8_t *priority,uint8_t *src_adr);

/*
 * @brief         :
 * @param[]       :
 * @return        :
 * @precondition  :
 * @postcondition :
 */
void J1939CallbackFF69(uint8_t *buffer,uint16_t pgn,uint8_t *priority,uint8_t *src_adr);


/*
 * @brief         :
 * @param[]       :
 * @return        :
 * @precondition  :
 * @postcondition :
 */
void J1939CallbackFF73(uint8_t *buffer,uint16_t pgn,uint8_t *priority,uint8_t *src_adr);

/*
 * @brief         :
 * @param[]       :
 * @return        :
 * @precondition  :
 * @postcondition :
 */
void J1939CallbackCumminsFEF1(uint8_t *buffer,uint16_t pgn,uint8_t *priority,uint8_t *src_adr);

/*
 * @brief         :
 * @param[]       :
 * @return        :
 * @precondition  :
 * @postcondition :
 */
void J1939CallbackFF7E(uint8_t *buffer,uint16_t pgn,uint8_t *priority,uint8_t *src_adr);

/*
 * @brief         :
 * @param[]       :
 * @return        :
 * @precondition  :
 * @postcondition :
 */
void J1939CallbackFF00(uint8_t *buffer,uint16_t pgn,uint8_t *priority,uint8_t *src_adr);

/*
 * @brief         :
 * @param[]       :
 * @return        :
 * @precondition  :
 * @postcondition :
 */
void J1939CallbackEF00(uint8_t *buffer,uint16_t pgn,uint8_t *priority,uint8_t *src_adr);

/*
 * @brief         :
 * @param[]       :
 * @return        :
 * @precondition  :
 * @postcondition :
 */
void J1939CallbackFEC7(uint8_t *buffer,uint16_t pgn,uint8_t *priority,uint8_t *src_adr);

/*
 * @brief         :
 * @param[]       :
 * @return        :
 * @precondition  :
 * @postcondition :
 */
void J1939CallbackDA00(uint8_t *buffer,uint16_t pgn,uint8_t *priority,uint8_t *src_adr);

/*
 * @brief         :
 * @param[]       :
 * @return        :
 * @precondition  :
 * @postcondition :
 */
void J1939CallbackFF80(uint8_t *buffer,uint16_t pgn,uint8_t *priority,uint8_t *src_adr);

/*
 * @brief         :
 * @param[]       :
 * @return        :
 * @precondition  :
 * @postcondition :
 */
void J1939CallbackScaniaFEF1(uint8_t *buffer,uint16_t pgn,uint8_t *priority,uint8_t *src_adr);

/*
 * @brief         :
 * @param[]       :
 * @return        :
 * @precondition  :
 * @postcondition :
 */
void J1939CallbackFFF7(uint8_t *buffer,uint16_t pgn,uint8_t *priority,uint8_t *src_adr);

#ifdef __cplusplus
}
#endif

#endif /* CANCALLBACKTRANSMIT_H_ */
