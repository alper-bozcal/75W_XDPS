/*
#include <J1939PacketSetup.h>
 * CanCallbackReceive.c
 *
 *  Created on: Sep 21, 2022
 *      Author: eren.akyol
 */

#include <IJ1939Callback.h>
#include "Interfaces.h"
/*------------------------------ Parameters ----------------------------------------*/

extern modbusServerObject_t server;
extern MEASURED_t measuredValues;
extern J1939Handle_t J1939;

// Just for to be an example of receive function
void receivePGN65283(J1939Request_t *self){

  if(self->SRC != J1939.destAddr){
    if(J1939.destAddr != 0){
      return;
    }
  }

  int16_t buffer = 0;

  buffer =   self->buffer[0];
  buffer |= (self->buffer[1] << 8);

  buffer =   self->buffer[2];
  buffer |= (self->buffer[3] << 8);

  buffer =   self->buffer[4];
  buffer |= (self->buffer[5] << 8);

  buffer =   self->buffer[6];
  buffer |= (self->buffer[7] << 8);

}

void sendPGN65104(J1939Request_t *self){

  int16_t buffer = 0;

  // SPN 1800 - 1 Byte - Battery 1 Temperature
  buffer = measuredValues.CPT100;
  if(buffer < -400) buffer = -400; // Limit the Temperature on -40C degrees
  buffer = ((buffer * 3) >> 5) + 40;
  self->buffer[0] = buffer;

  // SPN 1801 - 1 Byte - Battery 2 Temperature
  buffer = 0; // Not Supported
  self->buffer[1] = buffer >> 8;

  buffer = 0; // Reserved on J1939
  self->buffer[2] = buffer;
  self->buffer[3] = buffer >> 8;

  buffer = 0; // Reserved on J1939
  self->buffer[4] = buffer;
  self->buffer[5] = buffer >> 8;

  buffer = 0; // Reserved on J1939
  self->buffer[6] = buffer;
  self->buffer[7] = buffer >> 8;
}

void sendPGN65106(J1939Request_t *self){

  int16_t buffer = 0;

  // SPN 1795 - 2 Bytes - Alternator Current (High Range / Resolution)
  buffer = ((measuredValues.Iout * 51) >> 8) + 1600;
  self->buffer[0] = buffer;
  self->buffer[1] = buffer >> 8;

  // SPN 2579 - 2 Bytes - Net Battery Current (High Range / Resolution)
  buffer = 0; // Not Supported
  self->buffer[2] = buffer;
  self->buffer[3] = buffer >> 8;

  buffer = 0; // Reserved on J1939
  self->buffer[4] = buffer;
  self->buffer[5] = buffer >> 8;

  buffer = 0; // Reserved on J1939
  self->buffer[6] = buffer;
  self->buffer[7] = buffer >> 8;
}

void sendPGN65271(J1939Request_t *self){

  int16_t buffer = 0;

  // SPN 114 - 1 Byte - Net Battery Current
  buffer = 0; // Not Supported
  self->buffer[0] = buffer;

  // SPN 115 - 1 Byte - Alternator Current
  buffer = ((measuredValues.Iout + 24) * 5) >> 9; // + 24 added to not cause over estimation from the typecasting
  self->buffer[1] = buffer >> 8;

  // SPN 167 - 2 Bytes - Charging System Potential (Voltage)
  buffer = (measuredValues.Vout * 13) >> 6;
  self->buffer[2] = buffer;
  self->buffer[3] = buffer >> 8;

  // SPN 168 - 2 Bytes - Electrical Potential (Voltage)
  buffer = 0; // Not Supported
  self->buffer[4] = buffer;
  self->buffer[5] = buffer >> 8;

  // SPN 158 - 2 Bytes - Battery Potential (Voltage), Switched
  buffer = 0; // Not Supported
  self->buffer[6] = buffer;
  self->buffer[7] = buffer >> 8;
}
