/*
#include <J1939PacketSetup.h>
 * CanCallbackTransmit.c
 *
 *  Created on: Sep 21, 2022
 *      Author: eren.akyol
 *
 *      @note
 *
 *      Her paket gonderiminde buffer[] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF}
 */

#include "CanCallbackTransmit.h"
#include "global.h"

uint8_t perkins_adem_tx_flag=0;

/////////////////////////////////////Private Function Prototype/////////////////////////////////////////////
/**
 * @brief Set the Deutz Stop Message object
 * 
 * @param buffer 
 * @param priority 
 * @param src_adr 
 */
void setDeutzStopMessage(uint8_t *priority,uint8_t *src_adr);

/**
 *
 */
void PerkinsAdemTxControl(void);

/**
 *
 */
void setSrcAdrByEcuType(uint8_t ecu_param,uint8_t *pData);
////////////////////////////////////Public Functions////////////////////////////////////////////////////////
/*
 * @brief         :
 * @param[]       :
 * @return        :
 * @precondition  :
 * @postcondition :
 */
void J1939Callback0000(uint8_t *buffer,uint16_t pgn,uint8_t *priority,uint8_t *src_adr)
{
	PerkinsAdemTxControl() ;

	uint8_t ecu_param = getParamDataViaPN(J1939_ECU_TYPE);

	setSrcAdrByEcuType(ecu_param, &(*src_adr));

	if(ecu_param==ECU_TYPE_DEUTZ_EMR2 || ecu_param==ECU_TYPE_VOLVO_EDC3){
		src_adr = (uint8_t *) 0x11;
	}
	else if(ecu_param==ECU_TYPE_PERKINS_ADEM3){
		priority = (uint8_t *) 0x07;
	}

	buffer[0] = 0x01; // Control mode speed seciliyor
	buffer[1] = ecu_data.deutz_speed_data;
	buffer[2] = (ecu_data.deutz_speed_data >> 8);
	buffer[3] = 0; //Torque Ref
}

/*
 * @brief         :
 * @param[]       :
 * @return        :
 * @precondition  :
 * @postcondition :
 */
void J1939CallbackEA00(uint8_t *buffer,uint16_t pgn,uint8_t *priority,uint8_t *src_adr)
{
	static uint8_t request_alarm_reset=0; //Alarm silmek icin motor tipine gore kosullar koymak yerine
										  //iki farkli silme pgn ile art arda iki request eklenerek cozum saglanmistir

	PerkinsAdemTxControl() ;

	setSrcAdrByEcuType(getParamDataViaPN(J1939_ECU_TYPE), &(*src_adr));

	switch (ecu_data.ecu_request_state)
	{
		case ECU_REQUEST_ALARM_RESET:

			if(request_alarm_reset==0)
			{
				buffer[0] = 0xD3;
				buffer[1] = 0xFE;
				buffer[2] = 0x00;

				request_alarm_reset = 1;
			}

			else if(request_alarm_reset == 1)
			{
				buffer[0] = 0xCC;
				buffer[1] = 0xFE;
				buffer[2] = 0x00;

				//Alarm Silme istegi bir kere gerceklestirilip fuel level state ine gecilmistir.
				ecu_data.ecu_request_state = ECU_REQUEST_FUEL_LIQUID;
				request_alarm_reset = 0;
			}

			break;

		case ECU_REQUEST_FUEL_LIQUID:

				buffer[0] = 0xE9;
				buffer[1] = 0xFE;
				buffer[2] = 0x00;

				ecu_data.ecu_request_state = ECU_REQUEST_BAM_CONTROL;

			break;

		case ECU_REQUEST_BAM_CONTROL:

				buffer[0] = 0xCA;
				buffer[1] = 0xFE;
				buffer[2] = 0x00;

				ecu_data.ecu_request_state = ECU_REQUEST_FUEL_LIQUID;

			break;
	}
}
/*
 * @brief         :
 * @param[]       :
 * @return        :
 * @precondition  :
 * @postcondition :
 */
void J1939CallbackFF46(uint8_t *buffer,uint16_t pgn,uint8_t *priority,uint8_t *src_adr)
{
	uint16_t set_point=0;
	uint8_t percent_val=0;
	uint16_t gen_freq=0;
	float temp=0;

	*src_adr = 0x11;

	buffer[0] = 0b00000000;  //all bites not okey
	buffer[1] = 0b00000000;  //all bites not okey

	//start - stop request
	if(GenSet_t.state == GEN_PRESTART_STATE)
	{
		buffer[0] = 0b00000000;  //start not okey
		buffer[1] = 0b00010000;  //preheat okey
	}

	else if(GenSet_t.state == GEN_CRANKING_STATE)
	{
		buffer[0] = 0b00000001;  //start okey
		buffer[1] = 0b00000000;  //all bites not okey
	}

	else if(GenSet_t.state == GEN_STOP_STATE || GenSet_t.state == GEN_CRANKING_FAIL_STATE)
	{
		buffer[0] = 0b00000100;  //stop okey
		buffer[1] = 0b00000000;  //all bites not okey
		ecu_data.speed_correction = 0;
	}

	else if(GenSet_t.state == GEN_GCB_WAITING_STATE || GenSet_t.state == GEN_GCB_STATE || GenSet_t.state ==  GEN_COOLING_STATE)
	{
		buffer[0] = 0b00000000;  //all bites not okey
		buffer[1] = 0b00000000;  //all bites not okey

		// Frekans modundayken referans hatasi duzeltme
		ecu_data.per_corr++; // Cok hizli correction yapinca salinim olusuyor. Yaklasik 2sn yeterli.
		if(ecu_data.per_corr % 10 == 0){
			ecu_data.per_corr = 0;

				if(getParamDataViaPN(ENGINE_SPEED_SET_POINT) == 0)
				{
					set_point = VALUE_50_HZ;
				}
				else
				{
					set_point = VALUE_60_HZ;
					//temp = VALUE_50_DIFF_47_HZ; ??????
				}

				percent_val = getParamDataViaPN(ENGINE_SPEED_CORRECTION);

				if(percent_val > PERCENT_FIFTY)
				{
					percent_val = percent_val - PERCENT_FIFTY;
				}

				else if(percent_val < PERCENT_FIFTY)
				{
					percent_val = PERCENT_FIFTY - percent_val;
					percent_val *= -1;
				}

				else
				{
					percent_val = 0;
				}

				set_point += percent_val;

				gen_freq = PhasesValues_t.Ph1_f*10;

				// Voltaj kontrol modundaysak set edilen rpme donulsun
				if(set_point > gen_freq){
					ecu_data.speed_correction++;
				}
				else if(set_point < gen_freq){
					ecu_data.speed_correction--;
				}
		}
	}

	//frequency select(secondary selected)
	buffer[1] |= 0x01;

	// Volvo Penta EMS2'nin paneli uzerinden
	// Pedal kontrolu icin grafik karakteristigi
	// Min 47.0Hz max 54.0Hz olacak sekilde min max degerleri
	// Dijital pedal karakteristigi olan 0 - 1023

	if(getParamDataViaPN(ENGINE_SPEED_SET_POINT) == 0)
	{
		temp = VALUE_50_DIFF_47_HZ;
	}
	else
	{
		//temp = VALUE_50_DIFF_47_HZ; ??????
	}

	temp = temp / VALUE_54_DIFF_47_HZ;

	gen_freq = (temp * PEDAL_POSS_SOLUTION) + ecu_data.speed_correction;
	//Sentbuffer[2] = 0xFF; //%50 h?z ayar? (tam ayarlanan hizda donmesi icin)
	//Sentbuffer[3] = 0x01;
	buffer[2] = gen_freq;
	buffer[3] = gen_freq >> 8;
}

/*
 * @brief         :
 * @param[]       :
 * @return        :
 * @precondition  :
 * @postcondition :
 */
void J1939CallbackFF02(uint8_t *buffer,uint16_t pgn,uint8_t *priority,uint8_t *src_adr)
{
	buffer[0] = 0x00;  // No modification for torque map
	buffer[1] = 0x00;  // No modification for droop
	buffer[2] = 0x03;  // Switches to variable engine speed
	buffer[3] = 0x01;  // Engine speed governer
}

/*
 * @brief         :
 * @param[]       :
 * @return        :
 * @precondition  :
 * @postcondition :
 */
void J1939CallbackFF03(uint8_t *buffer,uint16_t pgn,uint8_t *priority,uint8_t *src_adr)
{
	buffer[0] = 0x64;  // %100 Power yani Power Reduction yok
	buffer[1] = 0x00;  // No engine start prohibition
}

/*
 * @brief         :
 * @param[]       :
 * @return        :
 * @precondition  :
 * @postcondition :
 */
void J1939CallbackFF16(uint8_t *buffer,uint16_t pgn,uint8_t *priority,uint8_t *src_adr)
{
	setDeutzStopMessage(&(*priority),&(*src_adr));
	/*Request olmasina ragmen guvenlik onlemi almak adina stop state inde 100ms'de bir gonderilecektir*/
	if(GenSet_t.state == GEN_STOP_STATE)
	{
		buffer[0] = 0x01;
	}
	else
	{
		buffer[0] = 0x00;
	}
}


void J1939CallbackRequestFF16(uint8_t *buffer,uint16_t pgn,uint8_t *priority,uint8_t *src_adr)
{
	setDeutzStopMessage(&(*priority),&(*src_adr));
	buffer[0] = 0x01;
}

/////Cummins CM850
/*
 * @brief         :
 * @param[]       :
 * @return        :
 * @precondition  :
 * @postcondition :
 */
void J1939CallbackFF69(uint8_t *buffer,uint16_t pgn,uint8_t *priority,uint8_t *src_adr)
{
	buffer[0] = 0x00;
	buffer[1] = 0x80;

	for(uint8_t i=2;i<8;i++)
	{
		buffer[i] = 0x00;
	}
}

/*
 * @brief         :
 * @param[]       :
 * @return        :
 * @precondition  :
 * @postcondition :
 */
void J1939CallbackFF73(uint8_t *buffer,uint16_t pgn,uint8_t *priority,uint8_t *src_adr)
{
	GenStateMachine_e state = getGenSetState();

	if(state==GEN_CRANKING_STATE || state==GEN_GCB_WAITING_STATE || state==GEN_GCB_STATE || state==GEN_COOLING_STATE) buffer[0] = 0x1F;
	else buffer[0] = 0x0F;

	if(getParamDataViaPN(NOMINAL_FREQUENCY) > 500) buffer[1] = 0x3F;
	else buffer[1] = 0x1F;
}

/*
 * @brief         :
 * @param[]       :
 * @return        :
 * @precondition  :
 * @postcondition :
 */
void J1939CallbackCumminsFEF1(uint8_t *buffer,uint16_t pgn,uint8_t *priority,uint8_t *src_adr)
{
	buffer[7] = 0x00;
}

/*
 * @brief         :
 * @param[]       :
 * @return        :
 * @precondition  :
 * @postcondition :
 */
void J1939CallbackFF7E(uint8_t *buffer,uint16_t pgn,uint8_t *priority,uint8_t *src_adr)
{
	static uint16_t temp=0;
	GenStateMachine_e state = getGenSetState();

	if(getParamDataViaPN(CAN_ECU_DROOP_ENABLE) && ((state==GEN_GCB_WAITING_STATE) || (state==GEN_GCB_STATE) || (state==GEN_COOLING_STATE)))
	{
		temp = (getParamDataViaPN(CAN_ECU_DROOP_PERCENT)<<10) / 10;
		buffer[0] = (temp)&0xff;
		buffer[1] = (temp >> 8)&0xff;
	}
	else
	{
		buffer[0] = 0x00;
		buffer[1] = 0x00;
	}

	buffer[4] = 0x00;
	buffer[5] = 0x80;
	buffer[6] = 0x00;
	buffer[7] = 0x30;
}

///Iveco T3
/*
 * @brief         :
 * @param[]       :
 * @return        :
 * @precondition  :
 * @postcondition :
 */
void J1939CallbackFF00(uint8_t *buffer,uint16_t pgn,uint8_t *priority,uint8_t *src_adr)
{
	GenStateMachine_e prev_state = getGenSetPrevState();
	GenStateMachine_e state      = getGenSetState();

	buffer[0] = 0x08;

	if( prev_state != state && (prev_state==GEN_CRANKING_STATE || state==GEN_CRANKING_STATE)) buffer[0]=0x00;
	else if(state==GEN_CRANKING_STATE || state==GEN_GCB_WAITING_STATE || state==GEN_GCB_STATE || state==GEN_COOLING_STATE) buffer[0] = 0x80;

	for(uint8_t i = 1; i < 8; i++)buffer[i] = 0x00;
}

///Perkins 1300
/*
 * @brief         :
 * @param[]       :
 * @return        :
 * @precondition  :
 * @postcondition :
 */
void J1939CallbackEF00(uint8_t *buffer,uint16_t pgn,uint8_t *priority,uint8_t *src_adr)
{
	for(uint8_t i = 0; i < 8; i++)buffer[i] = 0x00;

	if(getGenSetState() == GEN_STOP_STATE) buffer[0] = 0x01;

}

uint8_t getPerkinsAdemTxFlag(void)
{
	return perkins_adem_tx_flag;
}

///Perkins Adem3-4
/*
 * @brief         :
 * @param[]       :
 * @return        :
 * @precondition  :
 * @postcondition :
 */
void J1939CallbackFEC7(uint8_t *buffer,uint16_t pgn,uint8_t *priority,uint8_t *src_adr)
{
	PerkinsAdemTxControl();

	for (uint8_t  i = 0; i < 8; i++)buffer[i] = 0x00;

	if(getGenSetState() == GEN_STOP_STATE) buffer[5]=0x10;
}

///Scania S6
/*
 * @brief         :
 * @param[]       :
 * @return        :
 * @precondition  :
 * @postcondition :
 */
void J1939CallbackDA00(uint8_t *buffer,uint16_t pgn,uint8_t *priority,uint8_t *src_adr)
{
	buffer[0] = 0x03;

	//Alarm silme istegi yok
	if(ecu_data.scania_alarm_state == ECU_ALARM_SET) {
		buffer[1] = 0x17;
	}
	//Alarm silme istegi var
	else {
		buffer[1] = 0x14;
		ecu_data.scania_alarm_state = ECU_ALARM_SET;
	}

	for(uint8_t i=4;i<8;i++){
		buffer[i] = 0x00;
	}
}

/*
 * @brief         :
 * @param[]       :
 * @return        :
 * @precondition  :
 * @postcondition :
 */
void J1939CallbackFF80(uint8_t *buffer,uint16_t pgn,uint8_t *priority,uint8_t *src_adr)
{
	static uint8_t scania_state=0;
	GenStateMachine_e state = getGenSetState();

	buffer[0] = 0xA0;
	buffer[1] = 0x00;
	buffer[2] = 0x00;
	buffer[3] = 0xF0;
	buffer[4] = 0xFF;
	buffer[5] = 0xF5;
	buffer[6] = 0xCF;
	buffer[7] = 0xFF;

	switch(scania_state)
	{
		case 0:

			scania_state=1;

			break;

		case 1:

			buffer[3] = 0xF1;

			break;

		case 2:

			if(state > GEN_SOUND_WARNING_STATE)scania_state=3;

			break;
		case 3:

			if(state == GEN_STOP_STATE)buffer[3]=0xF1;
			else if(state == GEN_CRANKING_STATE) buffer[2] = 0x10;

			if(state > GEN_CRANKING_FAIL_STATE)buffer[1]=0x7D;
			if(state > GEN_CRANKING_FAIL_STATE && state < GEN_TRANSFER_STATE && getParamDataViaPN(CAN_ECU_DROOP_ENABLE))buffer[2]=0x01;

			break;
	}

}

/*
 * @brief         :
 * @param[]       :
 * @return        :
 * @precondition  :
 * @postcondition :
 */
void J1939CallbackScaniaFEF1(uint8_t *buffer,uint16_t pgn,uint8_t *priority,uint8_t *src_adr)
{
	buffer[3] = 0x03;
	buffer[4] = 0x03;
	buffer[7] = 0x0F;
}

/*
 * @brief         :
 * @param[]       :
 * @return        :
 * @precondition  :
 * @postcondition :
 */
void J1939CallbackFFF7(uint8_t *buffer,uint16_t pgn,uint8_t *priority,uint8_t *src_adr)
{
	GenStateMachine_e state = getGenSetState();

	buffer[0] = 0x3C;
	buffer[4] = 0x3F;

	if(getParamDataViaPN(CAN_ECU_DROOP_ENABLE) && ((state==GEN_GCB_WAITING_STATE) || (state==GEN_GCB_STATE) || (state==GEN_COOLING_STATE)))
	{
		buffer[5] = getParamDataViaPN(CAN_ECU_DROOP_PERCENT);
	}
	else
	{
		buffer[5] = 0x00;
	}
}

/////////////////////////////Private Functions///////////////////////////////////////////////
void setDeutzStopMessage(uint8_t *priority,uint8_t *src_adr)
{
	uint8_t ecu_type = getParamDataViaPN(J1939_ECU_TYPE);
	if(ecu_type == ECU_TYPE_DEUTZ_EMR2)
	{
		*priority = 0x03;
		*src_adr = 0x11;
	}
	else if(ecu_type == ECU_TYPE_DEUTZ_EMR3)
	{
		*priority = 0x02;
		*src_adr = 0x03;
	}
	else if(ecu_type == ECU_TYPE_VOLVO_EDC4)
	{
		*priority = 0x03;
		*src_adr = 0x03;
	}
}

void PerkinsAdemTxControl(void)
{
	uint8_t ecu_type = getParamDataViaPN(J1939_ECU_TYPE);

	perkins_adem_tx_flag = 0;

	if(getGenSetState()==GEN_IDLE_STATE && (ecu_type==ECU_TYPE_PERKINS_ADEM3 || ecu_type==ECU_TYPE_PERKINS_ADEM4))
	{
		perkins_adem_tx_flag = 1;
		return;
	}
}

void setSrcAdrByEcuType(uint8_t ecu_param,uint8_t *pData)
{
	switch(ecu_param)
	{
		case ECU_TYPE_VOLVO_EDC3:
		case ECU_TYPE_VOLVO_EMS2:
		case ECU_TYPE_DEUTZ_EMR2:
		case ECU_TYPE_GENERIC_J1939:
		case ECU_TYPE_CUMMINS_ISB:
			*pData = 0x2B;
			break;
		case ECU_TYPE_PERKINS_1300:
		case ECU_TYPE_CUMMINS_CM850:
			*pData = 0xDC;
			break;
		case ECU_TYPE_JOHN_DEERE:
		case ECU_TYPE_IVECO_T3:
		case ECU_TYPE_DEUTZ_EMR3:
		case ECU_TYPE_VOLVO_EDC4:
			*pData = 0x03;
			break;
		case ECU_TYPE_PERKINS_ADEM3:
		case ECU_TYPE_PERKINS_ADEM4:
			*pData = 0x11;
			break;
	}
}
