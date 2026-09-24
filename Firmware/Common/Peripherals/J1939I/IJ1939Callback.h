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

#include <IJ1939.h>

void receivePGN65283(J1939Request_t *self);

void sendPGN65271(J1939Request_t *self);

void sendPGN65104(J1939Request_t *self);

void sendPGN65106(J1939Request_t *self);

#ifdef __cplusplus
}
#endif

#endif /* CANCALLBACKRECEIVE_H_ */
