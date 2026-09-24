/*
 * j1939-adapter.c
 *
 *  Created on: Sep 30, 2022
 *      Author: eren.akyol
 */


#include "CanCallbackReceive.h"
#include "CanCallbackTransmit.h"

#define SIZE_OF_STANDRTJ1939 		3// 8 byte = 2^n => n=3
#define SOURCE_ADDRESS				0x00


/*Common Engine Parameters*/
static const StandartJ1939_t COMMON_ENGINE_TABLE[] = {

		  //time(1ms)  	//length 		//priority 			//pgn	          //src adr     		//rw				    //Callback
/*EEC1*/  {50,    			8,      		 3,        		 0xF004,          SOURCE_ADDRESS,       READ_PGN_COMMAND,      (J1939Callback*)J1939CallbackF004}, //Engine Speed "4-5" bytes - WRITE FROM ECU TO AMF
/*ET*/    {1000,    		8,      		 6,        		 0xFEEE,          SOURCE_ADDRESS,  	    READ_PGN_COMMAND,      (J1939Callback*)J1939CallbackFEEE}, //Coolant Temp "0"   byte  - WRITE FROM ECU TO AMF
/*EFLP*/  {500,	    		8,      		 6,        		 0xFEEF,          SOURCE_ADDRESS,  	    READ_PGN_COMMAND,      (J1939Callback*)J1939CallbackFEEF}, //Oil Pressure "3"   byte  - WRITE FROM ECU TO AMF
		  {500,	    		8,      		 6,        		 0xFECA,          SOURCE_ADDRESS,  	    READ_PGN_COMMAND,      (J1939Callback*)J1939CallbackFECA},
		  {50,	    		8,      		 3,        		 0x0000,          SOURCE_ADDRESS,  	    WRITE_PGN_COMMAND,     (J1939Callback*)J1939Callback0000},
		  {500,	  			3,      		 6,        		 0xEA00,          SOURCE_ADDRESS,  	    WRITE_PGN_COMMAND,     (J1939Callback*)J1939CallbackEA00},
		  {0,	  			0,      		 0,        		 0xE000,          SOURCE_ADDRESS,  	    READ_PDU_FORMAT_COMMAND,(J1939Callback*)J1939CallbackBAMEXXX},//Broadcast Announce Messages
};


//Volvo-Penta Spescific Engine Parameters
static const StandartJ1939_t SPECIFIC_VOLVO_EMS2[] = {

		  //time(1ms)   	//length 		//priority 		//pgn	    	  //src adr     		//rw					//Callback
		  {50,    			8,      		 3,        		 0xFF47,    	  0x11,			      	READ_PGN_COMMAND,      (J1939Callback*)J1939CallbackFF47}, //Alarm Signals - WRITE FROM ECU TO AMF
		  {20,    			8,      		 3,        		 0xFF46,    	  0x11,  				WRITE_PGN_COMMAND,     (J1939Callback*)J1939CallbackFF46}, //Engine State  - READ  FROM AMF TO ECU
};


//Deutz EMR2-EMR3 Spescific Engine Parameters
static const StandartJ1939_t SPECIFIC_DEUTZ_EMR2[] = {

		  //time(1ms)   	//length 		//priority 		//pgn             //src adr      	    //rw					//Callback
		  {500,    			8,      		 6,        		0xFF02,           0x03,			  	    WRITE_PGN_COMMAND,     (J1939Callback*)J1939CallbackFF02},
		  {100,    			8,      		 3,        		0xFF03,           0x03,  			    WRITE_PGN_COMMAND,     (J1939Callback*)J1939CallbackFF03},
		  {100,    			1,      		 3,        		0xFF16,  		  0X11,			 		WRITE_PGN_COMMAND,     (J1939Callback*)J1939CallbackFF16},
};

//Cummins CM850 Spescific Engine Parameters
static const StandartJ1939_t SPECIFIC_CUMMINS_CM850[] = {

		  //time(1ms)   	//length 		//priority 		//pgn             //src adr      	    //rw					//Callback
		  {10,    			8,      		 0,        		0xFF69,           0xDC,  			    WRITE_PGN_COMMAND,     (J1939Callback*)J1939CallbackFF69},
		  {20,    			8,      		 1,        		0xFF73,           0xDC,  	  			WRITE_PGN_COMMAND,     (J1939Callback*)J1939CallbackFF73},
		  {100,	  			8,      		 6,        	    0xFEF1,           0xDC,			  	    WRITE_PGN_COMMAND,		(J1939Callback*)J1939CallbackCumminsFEF1},
		  {500,    			8,      		 2,        		0xFF7E,  		  0xDC, 				WRITE_PGN_COMMAND,     (J1939Callback*)J1939CallbackFF7E},
};

//Iveco T3 Spescific Engine Parameters
static const StandartJ1939_t SPECIFIC_IVECO_T3[] = {

		  //time(1ms)   	//length 		//priority 		//pgn	    	  //src adr     		//rw					//Callback
		  {50,    			8,      		 3,        		0xFF00,  	  	  0x11,			      	WRITE_PGN_COMMAND,     (J1939Callback*)J1939CallbackFF00},
};

//Perkins-1300 Spescific Engine Parameters
static const StandartJ1939_t SPECIFIC_PERKINS_1300[] = {

		  //time(1ms)   	//length 		//priority 		//pgn	    	 //src adr     			//rw					//Callback
		  {1000,    		8,      		 6,        		0xEF00, 	   	 0x11,			      	WRITE_PGN_COMMAND,     (J1939Callback*)J1939CallbackEF00},
};

//Perkins Adem3 Spescific Engine Parameters
static const StandartJ1939_t SPECIFIC_PERKINS_ADEM3[] = {

		  //time(1ms)   	//length 		//priority 		//pgn	    	 //src adr     			//rw					//Callback
		  {50,  	  		8,      		3,        		0xFEC7, 	   	 0x11,			      	WRITE_PGN_COMMAND,     (J1939Callback*)J1939CallbackFEC7},
};

//Scania S6 Spescific Engine Parameters
static const StandartJ1939_t SPECIFIC_SCANIA_S6[] = {

		  //time(1ms)   	//length 		//priority 		//pgn	    	 //src adr     			//rw					//Callback
		  {1000,  	  		8,      		6,        		0xDA00, 	   	 0xFB,			      	WRITE_PGN_COMMAND,     (J1939Callback*)J1939CallbackDA00},
		  {20,  	  		8,      		3,        		0xFF80, 	   	 0x27,			      	WRITE_PGN_COMMAND,     (J1939Callback*)J1939CallbackFF80},
		  {100,  	  		8,      		6,        		0xFEF1, 	   	 0x27,			      	WRITE_PGN_COMMAND,     (J1939Callback*)J1939CallbackScaniaFEF1},
		  {50,  	  		8,      		3,        		0xFFF7, 	   	 0x27,			      	WRITE_PGN_COMMAND,     (J1939Callback*)J1939CallbackFFF7},
};

CanPackets_t setTypeTable(uint8_t table_type){

	CanPackets_t ecu_table_data;

	switch (table_type)
	{
		case ECU_TYPE_COMMON:  				ecu_table_data.table_len = sizeof(COMMON_ENGINE_TABLE);  ecu_table_data.table_addr = (uint32_t)COMMON_ENGINE_TABLE;      break;
		case ECU_TYPE_CUMMINS_CM850:    	ecu_table_data.table_len = sizeof(SPECIFIC_CUMMINS_CM850);  ecu_table_data.table_addr = (uint32_t)SPECIFIC_CUMMINS_CM850;      break;
		case ECU_TYPE_CUMMINS_ISB:  		ecu_table_data.table_len = sizeof(COMMON_ENGINE_TABLE);  ecu_table_data.table_addr = (uint32_t)COMMON_ENGINE_TABLE;      break;
		case ECU_TYPE_DEUTZ_EMR2:       	ecu_table_data.table_len = sizeof(SPECIFIC_DEUTZ_EMR2);  ecu_table_data.table_addr = (uint32_t)SPECIFIC_DEUTZ_EMR2;      break;
		case ECU_TYPE_DEUTZ_EMR3:       	ecu_table_data.table_len = sizeof(SPECIFIC_DEUTZ_EMR2);  ecu_table_data.table_addr = (uint32_t)SPECIFIC_DEUTZ_EMR2;      break;
		case ECU_TYPE_GENERIC_J1939:    	ecu_table_data.table_len = sizeof(COMMON_ENGINE_TABLE);  ecu_table_data.table_addr = (uint32_t)COMMON_ENGINE_TABLE;      break;
		case ECU_TYPE_IVECO_T3:             ecu_table_data.table_len = sizeof(SPECIFIC_IVECO_T3);  ecu_table_data.table_addr = (uint32_t)SPECIFIC_IVECO_T3;      break;
		case ECU_TYPE_JOHN_DEERE:           ecu_table_data.table_len = sizeof(COMMON_ENGINE_TABLE);  ecu_table_data.table_addr = (uint32_t)COMMON_ENGINE_TABLE;      break;
		case ECU_TYPE_MTU_ADEC:             ecu_table_data.table_len = sizeof(COMMON_ENGINE_TABLE);  ecu_table_data.table_addr = (uint32_t)COMMON_ENGINE_TABLE;      break;
		case ECU_TYPE_PERKINS_1300:         ecu_table_data.table_len = sizeof(SPECIFIC_PERKINS_1300);  ecu_table_data.table_addr = (uint32_t)SPECIFIC_PERKINS_1300;      break;
		case ECU_TYPE_PERKINS_ADEM3:        ecu_table_data.table_len = sizeof(SPECIFIC_PERKINS_ADEM3);  ecu_table_data.table_addr = (uint32_t)SPECIFIC_PERKINS_ADEM3;      break;
		case ECU_TYPE_PERKINS_ADEM4:        ecu_table_data.table_len = sizeof(SPECIFIC_PERKINS_ADEM3);  ecu_table_data.table_addr = (uint32_t)SPECIFIC_PERKINS_ADEM3;      break;
		case ECU_TYPE_SCANIA_S6:            ecu_table_data.table_len = sizeof(SPECIFIC_SCANIA_S6);  ecu_table_data.table_addr = (uint32_t)SPECIFIC_SCANIA_S6;      break;
		case ECU_TYPE_VOLVO_EDC3:           ecu_table_data.table_len = sizeof(SPECIFIC_VOLVO_EMS2);  ecu_table_data.table_addr = (uint32_t)SPECIFIC_VOLVO_EMS2;      break;
		case ECU_TYPE_VOLVO_EDC4:           ecu_table_data.table_len = sizeof(SPECIFIC_DEUTZ_EMR2);  ecu_table_data.table_addr = (uint32_t)SPECIFIC_DEUTZ_EMR2;      break;
		case ECU_TYPE_VOLVO_EMS2:           ecu_table_data.table_len = sizeof(SPECIFIC_VOLVO_EMS2);  ecu_table_data.table_addr = (uint32_t)SPECIFIC_VOLVO_EMS2;      break;
		case ECU_TYPE_VOLVO_EMS2B:          ecu_table_data.table_len = sizeof(COMMON_ENGINE_TABLE);  ecu_table_data.table_addr = (uint32_t)COMMON_ENGINE_TABLE;      break;
		case ECU_TYPE_YANMAR_ECO:           ecu_table_data.table_len = sizeof(COMMON_ENGINE_TABLE);  ecu_table_data.table_addr = (uint32_t)COMMON_ENGINE_TABLE;      break;
		case ECU_TYPE_LAST:															   	  														  					 break;
	}

	ecu_table_data.table_len = ecu_table_data.table_len>>SIZE_OF_STANDRTJ1939;

	return ecu_table_data;
}





