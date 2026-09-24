/*
#include <J1939PacketSetup.h>
 * CanCallbackReceive.c
 *
 *  Created on: Sep 21, 2022
 *      Author: eren.akyol
 */

#include "CanCallbackReceive.h"
#include "global.h"


//Ecuya gelen paketlerin degerlendirilip mars algoritmasina uyumunun saglandigi fonksiyondur.
void ecuReceivedPacketEval(uint16_t pgn,uint8_t *buffer);
//Lamb_status 1 ve 2.bitleri red lamb durumu,3 ve 4.bitleri warning lam durumu
void ecuEngineSetAlert(uint16_t  SPN_number,uint8_t FMI_number,uint8_t Lamb_status,uint8_t Occur_Count,uint8_t alarm_number);
//
void ecuEnineGetOldAlert(uint16_t SPN_number,uint8_t FMI_number,uint8_t alarm_number);

//Broadcast Announce Message 'a gore diger paketlerin kontorlu yapilmiyor. Bknz : AMF 5.1 RecieveCan Fonksiyonu
uint8_t putInReceiveQueue;

/*LAMB PARAMETERS*/
static uint32_t Next_SPN;
static uint8_t Lamb_stat;
static uint32_t SPN;
static uint8_t FMI;
static uint8_t OC;


extern struct EcuParameters ecu_data;

void J1939CallbackF004(uint8_t *buffer,uint16_t pgn,uint8_t *priority,uint8_t *src_adr)
{
	ecuReceivedPacketEval(pgn, buffer);
}

void J1939CallbackFEEE(uint8_t *buffer,uint16_t pgn,uint8_t *priority,uint8_t *src_adr)
{
	ecuReceivedPacketEval(pgn, buffer);
}

void J1939CallbackFEEF(uint8_t *buffer,uint16_t pgn,uint8_t *priority,uint8_t *src_adr)
{
	ecuReceivedPacketEval(pgn, buffer);
}

void J1939CallbackFECA(uint8_t *buffer,uint16_t pgn,uint8_t *priority,uint8_t *src_adr)
{
	ecuReceivedPacketEval(pgn, buffer);
}

void J1939CallbackBAMEXXX(uint8_t *buffer,uint16_t pgn,uint8_t *priority,uint8_t *src_adr)
{
	uint8_t pdu_format = pgn >> 8;

	putInReceiveQueue = 0;

	switch(pdu_format)
	{
		case 0xEC:

			if(buffer[0] == J1939_BAM_CONTROL_BYTE)
			{
				ecu_data.ecu_transmitter_state = TRANSMITTER_ACTIVE;
				ecu_data.timeout_delay = ECU_TIMEOUT_DELAY;
				ecu_data.BAM_data_start = 1;
				ecu_data.BAM_number_packet = ((uint16_t)buffer[2]<<8)|(uint16_t)buffer[1];
				ecu_data.BAM_processed_packet = 0;

				putInReceiveQueue = 1;
			}

			break;

		case 0xEB:

			if(ecu_data.BAM_data_start==1)
			{
				ecu_data.timeout_delay = ECU_TIMEOUT_DELAY;
				ecu_data.ecu_transmitter_state = TRANSMITTER_ACTIVE;
				if(buffer[0]==1)
				{
					FMI = buffer[5] & 0x1F;
					Lamb_stat = buffer[1];
					OC = buffer[6] & 0x7F;

					if(getParamDataViaPN(J1939_ECU_TYPE) == ECU_TYPE_DEUTZ_EMR2)
					{
						SPN = ((uint32_t )buffer[5] & 0x000000FF)|(((uint32_t)buffer[4] << 8) & 0x0000FF00)| (((uint32_t)buffer[3] << 16) & 0x00FF0000);
						SPN = SPN >> 5; // 32ye bolundu
						Next_SPN = (uint32_t)(((uint32_t)buffer[7]) << 16) & 0x00FF0000;
					}

					else
					{
						SPN = (uint32_t)(buffer[3])|(uint32_t)((uint32_t)buffer[4]<< 8)|(uint32_t)(((uint32_t)buffer[5])<<16);
						Next_SPN = (uint32_t)(buffer[7]);
						ecuEngineSetAlert(SPN,FMI,Lamb_stat,OC,0);
						ecu_data.BAM_processed_packet++;
					}
				}
				else if(buffer[0]==2)
				{
					FMI = buffer[2] & 0x1F;
					OC = buffer[3] & 0x7F;

					if(getParamDataViaPN(J1939_ECU_TYPE) == ECU_TYPE_DEUTZ_EMR2)
					{
						if(ecu_data.BAM_processed_packet < ecu_data.BAM_number_packet)
						{
							SPN = (((uint32_t)buffer[2]) & 0x000000FF)|(((uint32_t)buffer[1] << 8) & 0x0000FF00);
							SPN |= Next_SPN;
							SPN = SPN >> 5; // 32ye bolundu
						}
						else
						{
							ecu_data.BAM_data_start = 0;
						}
					}
					else
					{
						if(ecu_data.BAM_processed_packet < ecu_data.BAM_number_packet)
						{
							SPN = (uint32_t)((uint32_t)Next_SPN&0x000000FF)|(uint32_t)((uint32_t)buffer[1]<< 8)|(uint32_t)(((uint32_t)buffer[2]&0x000000E0)<<16);
						ecuEngineSetAlert(SPN,FMI,Lamb_stat,OC,0);
						ecu_data.BAM_processed_packet++;
						}
						else
						{
							ecu_data.BAM_data_start = 0;
						}

					}
				}

				//12'den fazla aktif alarm gelirse sonrakiler de?erlendirilmeyecek.....
				if(ecu_data.BAM_processed_packet >= 2)ecu_data.BAM_data_start=0;

				putInReceiveQueue = 1;
			}

			break;

		case 0xEA:

			if ((buffer[0] == J1939_PGN0_REQ_ADDRESS_CLAIM) &&
				(buffer[1] == J1939_PGN1_REQ_ADDRESS_CLAIM) &&
				(buffer[2] == J1939_PGN2_REQ_ADDRESS_CLAIM))
			{
				putInReceiveQueue = 1;
			}

			break;
	}
}

void J1939CallbackFF47(uint8_t *buffer,uint16_t pgn,uint8_t *priority,uint8_t *src_adr)
{
	ecuReceivedPacketEval(pgn, buffer);
}


//Lamb_status 1 ve 2.bitleri red lamb durumu,3 ve 4.bitleri warning lam durumu
void ecuEngineSetAlert(uint16_t  SPN_number,uint8_t FMI_number,uint8_t Lamb_status,uint8_t Occur_Count,uint8_t alarm_number)
{
	if(ecu_data.alarm_ack_time<=0)
	{
		if (Occur_Count != 0 && ((Lamb_status & 0xFC) != 0)) ecuEnineGetOldAlert(SPN_number, FMI_number, alarm_number);

		if((getParamDataViaPN(J1939_ECU_TYPE) == ECU_TYPE_DEUTZ_EMR2)
				&& ((ecu_data.deutz_echeck_delay < ECU_ALARM_CHECK_STARTUP)||(FMI_number == 31 && Occur_Count == 127)||(SPN_number == 524287)))
		{
			return;
		}

		if((Lamb_status & 0xFC) != 0 && Occur_Count != 0)
		{
			if(SPN_number == SPN_OIL_PRESSURE) ecuSingleAlarmUpdate(ECU_ALARM_SET,ALARM_EV_OILPRESS_LOW);
			else if(SPN_number == SPN_COOLANT_TEMPERATURE) ecuSingleAlarmUpdate(ECU_ALARM_SET,ALARM_EV_COOL_TEMP_HIGH);
			else if(SPN_number == SPN_ENGINE_SPEED) ecuSingleAlarmUpdate(ECU_ALARM_SET,ALARM_EV_COOL_TEMP_HIGH);
			else if(SPN_number != 0) ecuSingleAlarmUpdate(ECU_ALARM_SET,ALARM_EV_ECU_GENERAL);
		}
	}
	return;
}

void ecuEnineGetOldAlert(uint16_t SPN_number,uint8_t FMI_number,uint8_t alarm_number)
{
	switch(alarm_number)
	{
		case 0:
			ecu_data.old_spn1_alert = SPN_number;
			ecu_data.old_fmi1_alert = FMI_number;

			break;
		case 1:
			ecu_data.old_spn2_alert = SPN_number;
			ecu_data.old_fmi2_alert = FMI_number;

			break;
		default:
			break;
	}

}

void ecuReceivedPacketEval(uint16_t pgn,uint8_t *buffer)
{
	if(putInReceiveQueue) return;

	float data = 0;
	uint8_t data2 = 0;

	uint8_t ecu_type = getParamDataViaPN(J1939_ECU_TYPE);

	switch(pgn)
	{
		/*
		 *  SAE JJ1939
		 */
		//engine rpm : 4 - 5 byte ==> SPN = 190
		case 0xF004:

			if(ecuMeasurementActiveInactive(SPEED_CONTROL_ENABLE) == ECU_CONTROL_ENABLE)
			{
				data = ((uint16_t)buffer[4] << 8 | buffer[3]) * ECU_ENGINE_SPEED_GAIN;

				setEngineRPM(data);
			}

			ecu_data.timeout_delay=ECU_TIMEOUT_DELAY;
			ecu_data.ecu_transmitter_state = TRANSMITTER_ACTIVE;
			break;

		//coolant temp : 1 byte ==> SPN = 110
		case 0xFEEE:

			if(ecuMeasurementActiveInactive(TEMPERATURE_CONTROL_ENABLE) == ECU_CONTROL_ENABLE)
			{

				tempSensor1Obj.val_temp = buffer[0] - 40;
			}

			ecu_data.timeout_delay=ECU_TIMEOUT_DELAY;
			ecu_data.ecu_transmitter_state = TRANSMITTER_ACTIVE;
			break;

		//oil pressure : 4 byte ==> SPN = 100
		case 0xFEEF:

			if(ecuMeasurementActiveInactive(OIL_PRESSURE_CONTROL_ENABLE) == ECU_CONTROL_ENABLE)
			{
				if(getParamDataViaPN(OIL_PRESSURE_UNIT) == PRESS_UNIT_BAR)
				{
					data = buffer[3] * ECU_OIL_PRESSURE_BAR_CONSTANT;
				}
				else
				{
					data = buffer[3] * ECU_OIL_PRESSURE_PSI_CONSTANT;
				}

				pressSensor1Obj.val_press = data;
			}

			ecu_data.timeout_delay=ECU_TIMEOUT_DELAY;
			ecu_data.ecu_transmitter_state = TRANSMITTER_ACTIVE;
			break;

		case 0xFECA:

			FMI = buffer[4] & 0x1F;
			Lamb_stat = buffer[0];
			OC = buffer[5] & 0x7F;

			if(ecu_type == ECU_TYPE_DEUTZ_EMR2)
			{
				SPN = ((uint32_t )buffer[4] & 0x000000FF)|(((uint32_t)buffer[3] << 8) & 0x0000FF00)| (((uint32_t)buffer[2] << 16) & 0x00FF0000);
				SPN = SPN >> 5; // 32ye bolundu
			}
			else
			{
				SPN = ((uint32_t )buffer[2] & 0x000000FF)|(((uint32_t)buffer[3] << 8) & 0x0000FF00)| (((uint32_t)buffer[4] << 16) & 0x00FF0000);
				SPN = SPN >> 5; // 32ye bolundu
			}

			ecuEngineSetAlert(SPN,FMI,Lamb_stat,OC,0);

			ecu_data.timeout_delay=ECU_TIMEOUT_DELAY;
			ecu_data.ecu_transmitter_state = TRANSMITTER_ACTIVE;
			break;

		/*
		 *  VOLVO PENTA EMS2 SPESIFIC
		 */
		case 0xFF47:

			//Alarm Signals

			//Running Indication

			//@todo eklenecek

			//OverSpeed Alarm
			data2 = (buffer[0] >> 4) & 0b00000011;
			ecuDualAlarmUpdate(data2, ALARM_EV_GEN_OF, ALARM_EV_GEN_UF);

			//Oil Pressure Alarm
			data2 = 0;
			data2 = (buffer[0] >> 6) & 0b00000011;
			ecuDualAlarmUpdate(data2, ALARM_EV_OILPRESS_LOW, ALARM_EV_BROKEN_OIL_PRESS);

			//Coolant Temperature Alarm
			data2 = 0;
			data2 = (buffer[1] >> 2) & 0b00000011;
			ecuDualAlarmUpdate(data2, ALARM_EV_COOL_TEMP_HIGH, ALARM_EV_BROKEN_COOL_TEMP);

			//Charge Alarm
			data2 = 0;
			data2 = (buffer[1] >> 6) & 0b00000011;
			ecuDualAlarmUpdate(data2, ALARM_EV_CHARGE_ALT_OV, ALARM_EV_CHARGE_ALT_UV);

			ecu_data.timeout_delay=ECU_TIMEOUT_DELAY;
			ecu_data.ecu_transmitter_state = TRANSMITTER_ACTIVE;
			break;


//		//Measured Data 2
//		case 0xFF12:
//
//			if(getParamDataViaPN(J1939_ECU_TYPE) == ECU_TYPE_DEUTZ_EMR2)
//			{
//
//			}
//
//			break;


	}
}

