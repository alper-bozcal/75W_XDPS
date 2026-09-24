/**
 * @file modbusServer.c
 * @author ENKO Electronics
 * @brief  Modbus Server Interface source file. This file should be filled with app specific details.
 * @version 0.1
 * @date 2023-05-22
 * 
 * @copyright Copyright (c) 2023 @ ENKO Electronics
 * 
 */
#include "ImodbusServer.h"
#include "ImodbusCallback.h"
#include "Controller.h"
#include "modbus-table.h"
#include "defaults.h"
#include "Flash.h"

/*--------------------------------------External Modbus Data --------------------------------------*/

extern MEASURED_t measuredValues;
extern measurement_t Vout, Iout, Intc, Ontc, Tpt, Vbat;
extern controller_t controller;
extern J1939Handle_t J1939;
// Modbus RAM Values. Added here since they are not used anywhere else with spesific names except Callbacks
const int16_t MODEL_NAME[10] = {(('B' << 8) + 'D'), (('2' << 8) + 'C'), 0, 0,/*(('7' << 8) + '-'), (('W' << 8) + '5')*/ 0, 0, 0, 0, 0, 0};
int16_t FW = FW_VERSION;
int16_t ENKO[15] = {0}; // Password registers

// Backup variables to store actual password values - accessible from modbusServer.c
int16_t ENKO_PASSWORD_BACKUP = 0;
int16_t FACTORY_PASSWORD_BACKUP = 0;
int16_t SERVICE_PASSWORD_BACKUP = 0;
int16_t USER_PASSWORD_BACKUP = 0;

/*--------------------------------------User Implementation--------------------------------------*/

//Total register count for modbus.
//This is not the last MB addr but rather total MB ADDR.
#define MB_REG_COUNT 176

// Logout timer max count value 10Sec of inactivity should be enough
#define MB_TIMEOUT_MS   10000

/**
 * @brief Modbus registers array. This array should be filled with app specific details.
 *
 */
const modBusRegister_t REALmodbusRegisters[MB_REG_COUNT] = {
    // Lacking of address zero was causing shift in the flash so it was not reading right memory addresses, above line fixes it.
    /*  Value Pointer                   Index		           				   DEFAULT               MAX           MIN                Access   SAVE		       Callback FN*/
    {&(controller.psuReferenceVoltage), OUTPUT_PSU_VOLTAGE,                    1200,                 MAX_VOLTAGE,  MIN_VOLTAGE,       LEVEL_1, SAVE_ENABLE,  callBackPsuUpdateVoltageREF},
    {&(controller.effectiveVoltage),    OUTPUT_EFFECTIVE_VOLTAGE,              0,                    32767,        -32767,            LEVEL_4, SAVE_DISABLE, NULL},
    {&(controller.psuReferenceCurrent), OUTPUT_PSU_CURRENT,                    NOM_CURRENT,          MAX_CURRENT,  MIN_CURRENT,       LEVEL_1, SAVE_ENABLE,  callBackPsuUpdateCurrentREF},
    {&(controller.effectiveCurrent),    OUTPUT_EFFECTIVE_CURRENT,              0,                    32767,        0,                 LEVEL_4, SAVE_DISABLE, NULL},
    {&(controller.boostVoltage),        OUTPUT_BOOST_VOLTAGE,                  1450,                 MAX_VOLTAGE,  MIN_VOLTAGE,       LEVEL_3, SAVE_ENABLE,  CallbackUpdateVoltageBoost},
    {&(controller.equalizationTime),    OUTPUT_EQUALIZATION_TIME,              0,                    32000,        0,                 LEVEL_3, SAVE_ENABLE,  NULL},
    {&(controller.autoStartBoostTime),  OUTPUT_BOOST_AUTOSTART_TIME,           10000,                32000,        0,                 LEVEL_3, SAVE_ENABLE,  NULL},
    {NULL,                              OUTPUT_BLANK_1,                        0,                    32767,        0,                 LEVEL_4, SAVE_DISABLE, NULL},
    {NULL,                              OUTPUT_BLANK_2,                        0,                    32767,        0,                 LEVEL_4, SAVE_DISABLE, NULL},
    {&(controller.cableDrop),           OUTPUT_CABLE_DROOP,                    DEF_CABLE_DROP,     MAX_CABLE_DROP, MIN_CABLE_DROP,    LEVEL_2, SAVE_ENABLE,  NULL},
    {&(controller.chargeState),         OUTPUT_CHARGE_STATE,                   0,                    10,           0,                 LEVEL_1, SAVE_DISABLE, NULL},
    {&(controller.flagAutoBoost),       OUTPUT_AUTO_BOOST,                     0,                    1,            0,                 LEVEL_2, SAVE_DISABLE, NULL},
    {&(controller.flagModePSU),         OUTPUT_MODE_PSU,                       1,                    1,            0,                 LEVEL_1, SAVE_ENABLE,  NULL},
    {&(controller.flagModeAuto),        OUTPUT_MODE_AUTO,                      0,                    1,            0,                 LEVEL_4, SAVE_ENABLE,  NULL},
    {&(controller.flagParallel),        PARALLEL_WORKING,                      0,                    1,            0,                 LEVEL_1, SAVE_ENABLE,  NULL},
    {&(controller.parallelCoef),        PARALLEL_CURRENT_COEFF,                3,                    100,          0,                 LEVEL_2, SAVE_ENABLE,  NULL},
    {&(controller.flagPt100),           TEMP_PT100_ACT,                        0,                    1,            0,                 LEVEL_1, SAVE_ENABLE,  NULL},
    {&(controller.Pt100DVDT),           TEMP_PT100_DVDT,                       0,                    10,           0,                 LEVEL_2, SAVE_ENABLE,  NULL},
    {&(controller.Pt100WarningTemp),    TEMP_PT100_WARNING,                    450,                  1000,         0,                 LEVEL_2, SAVE_ENABLE,  NULL},
    {&(controller.Pt100AlarmTemp),      TEMP_PT100_ALARM,                      500,                  2000,         0, 	              LEVEL_2, SAVE_ENABLE,  CallbackPt100OverTemp},
    {&(controller.maxTempMCU),          TEMP_MAX_MCU,                          0,                    255,          0,                 LEVEL_4, SAVE_ENABLE,  NULL},
    {&(measuredValues.Vout),            MONITORING_VOLTAGE,                    0,                    5000,         0,                 LEVEL_4, SAVE_DISABLE, NULL},
    {&(measuredValues.Iout),            MONITORING_CURRENT,                    0,                    5000,         0,                 LEVEL_4, SAVE_DISABLE, NULL},
    {&(measuredValues.operationMode),   MONITORING_OPERATING_MODE,             0,                    4,            0,                 LEVEL_4, SAVE_DISABLE, NULL},
    {&(measuredValues.CPT100),          MONITORING_CPT100,                     0,                    2550,         -2550,             LEVEL_4, SAVE_DISABLE, NULL},
    {&(measuredValues.ntcC),            MONITORING_NTC,                        0,                    2550,         -2550,             LEVEL_4, SAVE_DISABLE, NULL},
    {&(measuredValues.INTC),            MONITORING_INTC,                       0,                    255,          -255,              LEVEL_4, SAVE_DISABLE, NULL},
    {&(measuredValues.Vbat),            MONITORING_VBAT,                       0,                    5000,         -5000,             LEVEL_4, SAVE_DISABLE, NULL},
    {&(controller.flagBoostTriggered),  MONITORING_IS_BOOST_TRIG,              0,                    1,            0,                 LEVEL_4, SAVE_DISABLE, NULL},
    {&(controller.outputModel),         OUTPUT_MODEL,                          0,                    9999,         0,                 LEVEL_4, SAVE_DISABLE, CallbackModel},
    {&(controller.derateStatus),        MONITORING_DERATE_STATUS,              0,                    1,            0,                 LEVEL_4, SAVE_DISABLE, NULL},
    {&(controller.flagERR),             ERR_FLAG,                              0,                    32767,        -32768,            LEVEL_4, SAVE_DISABLE, NULL},
    {&(controller.flagWARN),            WARN_FLAG,                             0,                    32767,        -32768,            LEVEL_4, SAVE_DISABLE, NULL},
    {(int16_t*)&ADC_VAL_VOUT,           MONITORING_ADC_VOUT,                   0,                    4095,         0,                 LEVEL_4, SAVE_DISABLE, NULL},
    {(int16_t*)&ADC_VAL_IOUT,           MONITORING_ADC_IOUT,                   0,                    4095,         0,                 LEVEL_4, SAVE_DISABLE, NULL},
    {(int16_t*)&ADC_VAL_VBAT,           MONITORING_ADC_VBAT,                   0,                    4095,         0,                 LEVEL_4, SAVE_DISABLE, NULL},
    {(int16_t*)&ADC_VAL_PT100,          MONITORING_ADC_PT100,                  0,                    4095,         0,                 LEVEL_4, SAVE_DISABLE, NULL},
    {(int16_t*)&ADC_VAL_ONTC,           MONITORING_ADC_ONTC,                   0,                    4095,         0,                 LEVEL_4, SAVE_DISABLE, NULL},
    {(int16_t*)&ADC_VAL_INTC,           MONITORING_ADC_INTC,                   0,                    4095,         0,                 LEVEL_4, SAVE_DISABLE, NULL},
    {&(controller.monitorEffectVolt),   MONITORING_EFFECTIVE_VOLT,             0,                    32000,        0,                 LEVEL_4, SAVE_DISABLE, NULL},
    {NULL,                              MONITORING_BLANK_2,                    0,                    32000,        0,                 LEVEL_5, SAVE_DISABLE, NULL},
    {&ENKO[13],                         DEBUG_RETURN_FACTORY,                  0,                    2,            0,                 LEVEL_4, SAVE_DISABLE, CallbackFactoryReset},
    {(int16_t*)&V_PWM_DUTY,             VOUTPWM,                               PWM_MIN,              PWM_MAX,      PWM_MIN,           LEVEL_4, SAVE_ENABLE,  CallbackUpdateVPWM},
    {(int16_t*)&I_PWM_DUTY,             IOUTPWM,                               PWM_MAX,              PWM_MAX,      PWM_MIN,           LEVEL_4, SAVE_ENABLE,  CallbackUpdateIPWM},
    {&(controller.flagPWM),             PWMFLAG,                               0,                    1,            0,                 LEVEL_4, SAVE_ENABLE,  NULL},
    {&ENKO[14],                         DEVICE_RESET,                          0,                    1,            0,                 LEVEL_1, SAVE_DISABLE, CallbackDeviceReset},
    {&(controller.flagCutOff),          OUTPUT_CUT_OFF,                        1,                    1,            0,                 LEVEL_4, SAVE_DISABLE, NULL},
    {&(controller.flagRelay),           OUTPUT_RELAY,                          0,                    1,            0,                 LEVEL_4, SAVE_DISABLE, CallbackRelay},
    {&(controller.LEDColor),            LED_COLOR,                             0,                    32767,        -32767,            LEVEL_4, SAVE_DISABLE, CallbackLEDControl},
    {&(controller.LEDBlink),            LED_BLINK,                             2,                    5,            0,                 LEVEL_4, SAVE_DISABLE, CallbackLEDBlink},
    {NULL,                              DEBUG_TAG,                             0,                    32767,        -32768,            LEVEL_4, SAVE_DISABLE, CallbackSaveTag},
    //! These are not working since they are uint8_t. Will be addressed in the future.
    {NULL/*&(J1939.srcAddr)*/,          CANBUS_SOURCE_ADDR,                    1,                    255,          0,                 LEVEL_4, SAVE_ENABLE,  NULL},
    {NULL/*&(J1939.destAddr)*/,         CANBUS_DEST_ADDR,                      0,                    255,          0,                 LEVEL_4, SAVE_ENABLE,  NULL},
    {NULL,                              DEBUG_BLANK_3,                         0,                    1,            0,                 LEVEL_4, SAVE_DISABLE, NULL},
    {NULL,                              DEBUG_BLANK_4,                         0,                    1,            0,                 LEVEL_4, SAVE_DISABLE, NULL},
    {&(Vout.adcOffset),                 MEASURE_OFFSET_VOUT,                   DEF_OFFSET_VOUT,      1024,         -1024,             LEVEL_4, SAVE_ENABLE,  NULL},
    {&(Vout.adcRatio),                  MEASURE_RATIO_VOUT,                    DEF_RATIO_VOUT,       4096,         0,                 LEVEL_4, SAVE_ENABLE,  NULL},
    {&(Vout.adcShift),                  MEASURE_SHIFT_VOUT,                    DEF_SHIFT_VOUT,       16,           0,                 LEVEL_4, SAVE_DISABLE, NULL},
    {&(Iout.adcOffset),                 MEASURE_OFFSET_IOUT,                   DEF_OFFSET_IOUT,      1024,         -1024,             LEVEL_4, SAVE_ENABLE,  NULL},
    {&(Iout.adcRatio),                  MEASURE_RATIO_IOUT,                    DEF_RATIO_IOUT,       4096,         -4096,             LEVEL_4, SAVE_ENABLE,  NULL},
    {&(Iout.adcShift),                  MEASURE_SHIFT_IOUT,                    DEF_SHIFT_IOUT,       16,           0,                 LEVEL_4, SAVE_DISABLE, NULL},
    {&(Intc.adcOffset),                 MEASURE_OFFSET_INTC,                   DEF_OFFSET_INTC,      1024,         -1024,             LEVEL_4, SAVE_ENABLE,  NULL},
    {&(Intc.adcRatio),                  MEASURE_RATIO_INTC,                    DEF_RATIO_INTC,       4096,         -4096,             LEVEL_4, SAVE_ENABLE,  NULL},
    {&(Intc.adcShift),                  MEASURE_SHIFT_INTC,                    DEF_SHIFT_INTC,       16,           0,                 LEVEL_4, SAVE_DISABLE, NULL},
    {&(Ontc.adcOffset),                 MEASURE_OFFSET_ONTC,                   DEF_OFFSET_ONTC,      8192,         -8192,             LEVEL_4, SAVE_ENABLE,  NULL},
    {&(Ontc.adcRatio),                  MEASURE_RATIO_ONTC,                    DEF_RATIO_ONTC,       1024,         -1024,             LEVEL_4, SAVE_ENABLE,  NULL},
    {&(Ontc.adcShift),                  MEASURE_SHIFT_ONTC,                    DEF_SHIFT_ONTC,       16,           0,                 LEVEL_4, SAVE_DISABLE, NULL},
    {&(Vbat.adcOffset),                 MEASURE_OFFSET_VBAT,                   DEF_OFFSET_VBAT,      1024,         -1024,             LEVEL_4, SAVE_ENABLE,  NULL},
    {&(Vbat.adcRatio),                  MEASURE_RATIO_VBAT,                    DEF_RATIO_VBAT,       1024,         -1024,             LEVEL_4, SAVE_ENABLE,  NULL},
    {&(Vbat.adcShift),                  MEASURE_SHIFT_VBAT,                    DEF_SHIFT_VBAT,       16,           0,                 LEVEL_4, SAVE_DISABLE, NULL},
    {&(Tpt.adcOffset),                  MEASURE_OFFSET_PT100,                  DEF_OFFSET_PT100,     4096,         -4096,             LEVEL_4, SAVE_ENABLE,  NULL},
    {&(Tpt.adcRatio),                   MEASURE_RATIO_PT100,                   DEF_RATIO_PT100,      4096,         -4096,             LEVEL_4, SAVE_ENABLE,  NULL},
    {&(Tpt.adcShift),                   MEASURE_SHIFT_PT100,                   DEF_SHIFT_PT100,      16,           0,                 LEVEL_4, SAVE_DISABLE, NULL},
    {NULL,                              STAT_CURR_RANGE_1_1,                   0,                    32767,        -32767,            LEVEL_5, SAVE_DISABLE, NULL},
    {NULL,                              STAT_CURR_RANGE_1_2,                   0,                    32767,        -32767,            LEVEL_5, SAVE_DISABLE, NULL},
    {NULL,                              STAT_CURR_RANGE_2_1,                   0,                    32767,        -32767,            LEVEL_5, SAVE_DISABLE, NULL},
    {NULL,                              STAT_CURR_RANGE_2_2,                   0,                    32767,        -32767,            LEVEL_5, SAVE_DISABLE, NULL},
    {NULL,                              STAT_CURR_RANGE_3_1,                   0,                    32767,        -32767,            LEVEL_5, SAVE_DISABLE, NULL},
    {NULL,                              STAT_CURR_RANGE_3_2,                   0,                    32767,        -32767,            LEVEL_5, SAVE_DISABLE, NULL},
    {NULL,                              STAT_CURR_RANGE_4_1,                   0,                    32767,        -32767,            LEVEL_5, SAVE_DISABLE, NULL},
    {NULL,                              STAT_CURR_RANGE_4_2,                   0,                    32767,        -32767,            LEVEL_5, SAVE_DISABLE, NULL},
    {NULL,                              STAT_CURR_RANGE_5_1,                   0,                    32767,        -32767,            LEVEL_5, SAVE_DISABLE, NULL},
    {NULL,                              STAT_CURR_RANGE_5_2,                   0,                    32767,        -32767,            LEVEL_5, SAVE_DISABLE, NULL},
    {NULL,                              STAT_CURR_RANGE_6_1,                   0,                    32767,        -32767,            LEVEL_5, SAVE_DISABLE, NULL},
    {NULL,                              STAT_CURR_RANGE_6_2,                   0,                    32767,        -32767,            LEVEL_5, SAVE_DISABLE, NULL},
    {NULL,                              STAT_CURR_RANGE_7_1,                   0,                    32767,        -32767,            LEVEL_5, SAVE_DISABLE, NULL},
    {NULL,                              STAT_CURR_RANGE_7_2,                   0,                    32767,        -32767,            LEVEL_5, SAVE_DISABLE, NULL},
    {NULL,                              STAT_CURR_RANGE_8_1,                   0,                    32767,        -32767,            LEVEL_5, SAVE_DISABLE, NULL},
    {NULL,                              STAT_CURR_RANGE_8_2,                   0,                    32767,        -32767,            LEVEL_5, SAVE_DISABLE, NULL},
    {NULL,                              STAT_CURR_RANGE_9_1,                   0,                    32767,        -32767,            LEVEL_5, SAVE_DISABLE, NULL},
    {NULL,                              STAT_CURR_RANGE_9_2,                   0,                    32767,        -32767,            LEVEL_5, SAVE_DISABLE, NULL},
    {NULL,                              STAT_CURR_RANGE_10_1,                  0,                    32767,        -32767,            LEVEL_5, SAVE_DISABLE, NULL},
    {NULL,                              STAT_CURR_RANGE_10_2,                  0,                    32767,        -32767,            LEVEL_5, SAVE_DISABLE, NULL},
    {NULL,                              STAT_TEMP_RANGE_1_1,                   0,                    32767,        -32767,            LEVEL_5, SAVE_DISABLE, NULL},
    {NULL,                              STAT_TEMP_RANGE_1_2,                   0,                    32767,        -32767,            LEVEL_5, SAVE_DISABLE, NULL},
    {NULL,                              STAT_TEMP_RANGE_2_1,                   0,                    32767,        -32767,            LEVEL_5, SAVE_DISABLE, NULL},
    {NULL,                              STAT_TEMP_RANGE_2_2,                   0,                    32767,        -32767,            LEVEL_5, SAVE_DISABLE, NULL},
    {NULL,                              STAT_TEMP_RANGE_3_1,                   0,                    32767,        -32767,            LEVEL_5, SAVE_DISABLE, NULL},
    {NULL,                              STAT_TEMP_RANGE_3_2,                   0,                    32767,        -32767,            LEVEL_5, SAVE_DISABLE, NULL},
    {NULL,                              STAT_TEMP_RANGE_4_1,                   0,                    32767,        -32767,            LEVEL_5, SAVE_DISABLE, NULL},
    {NULL,                              STAT_TEMP_RANGE_4_2,                   0,                    32767,        -32767,            LEVEL_5, SAVE_DISABLE, NULL},
    {NULL,                              STAT_TEMP_RANGE_5_1,                   0,                    32767,        -32767,            LEVEL_5, SAVE_DISABLE, NULL},
    {NULL,                              STAT_TEMP_RANGE_5_2,                   0,                    32767,        -32767,            LEVEL_5, SAVE_DISABLE, NULL},
    {NULL,                              STAT_TEMP_RANGE_6_1,                   0,                    32767,        -32767,            LEVEL_5, SAVE_DISABLE, NULL},
    {NULL,                              STAT_TEMP_RANGE_6_2,                   0,                    32767,        -32767,            LEVEL_5, SAVE_DISABLE, NULL},
    {NULL,                              STAT_TEMP_RANGE_7_1,                   0,                    32767,        -32767,            LEVEL_5, SAVE_DISABLE, NULL},
    {NULL,                              STAT_TEMP_RANGE_7_2,                   0,                    32767,        -32767,            LEVEL_5, SAVE_DISABLE, NULL},
    {NULL,                              STAT_TEMP_RANGE_8_1,                   0,                    32767,        -32767,            LEVEL_5, SAVE_DISABLE, NULL},
    {NULL,                              STAT_TEMP_RANGE_8_2,                   0,                    32767,        -32767,            LEVEL_5, SAVE_DISABLE, NULL},
    {NULL,                              STAT_TEMP_RANGE_9_1,                   0,                    32767,        -32767,            LEVEL_5, SAVE_DISABLE, NULL},
    {NULL,                              STAT_TEMP_RANGE_9_2,                   0,                    32767,        -32767,            LEVEL_5, SAVE_DISABLE, NULL},
    {NULL,                              STAT_TEMP_RANGE_10_1,                  0,                    32767,        -32767,            LEVEL_5, SAVE_DISABLE, NULL},
    {NULL,                              STAT_TEMP_RANGE_10_2,                  0,                    32767,        -32767,            LEVEL_5, SAVE_DISABLE, NULL},
    {&ENKO[11],                         DEVICE_ENKO_PASS_ENTRY,                0,                    9999,         0,                 LEVEL_0, SAVE_DISABLE, CallbackEnkoPasswordEntry},
    {&ENKO[12],                         DEVICE_USER_PASS_ENTRY,                0,                    9999,         0,                 LEVEL_0, SAVE_DISABLE, CallbackUserPasswordEntry},
    {NULL,                              DEVICE_DYNPASS_USERCODE,               0,                    32767,        -32767,            LEVEL_1, SAVE_DISABLE, NULL},
    {NULL,                              DEVICE_DYNPASS_SERVICECODE,            0,                    32767,        -32767,            LEVEL_1, SAVE_DISABLE, NULL},
    {NULL,                              DEVICE_DYNPASS_FACTORYCODE,            0,                    32767,        -32767,            LEVEL_1, SAVE_DISABLE, NULL},
    {NULL,                              DEVICE_DYNPASS_ENKOCODE,               0,                    32767,        -32767,            LEVEL_1, SAVE_DISABLE, NULL},
    {NULL,                              DEVICE_DYNPASS_USERPASS,               0,                    32767,        -32767,            LEVEL_1, SAVE_DISABLE, NULL},
    {NULL,                              DEVICE_DYNPASS_SERVICEPASS,            0,                    32767,        -32767,            LEVEL_1, SAVE_DISABLE, NULL},
    {NULL,                              DEVICE_DYNPASS_FACTORYPASS,            0,                    32767,        -32767,            LEVEL_1, SAVE_DISABLE, NULL},
    {NULL,                              DEVICE_DYNPASS_ENKOPASS,               0,                    32767,        -32767,            LEVEL_1, SAVE_DISABLE, NULL},
    {&ENKO[3],                          DEVICE_USER_PASSWORD,                  PASS_DEF_USER,        9999,          1000,             LEVEL_1, SAVE_ENABLE,  CallbackUpdateUserLevel},
    {&ENKO[2],                          DEVICE_SERVICE_PASSWORD,               PASS_DEF_SERVICE,     9999,         	1000,             LEVEL_2, SAVE_ENABLE,  CallbackUpdateUserLevel},
    {&ENKO[1],                          DEVICE_FACTORY_PASSWORD,               PASS_DEF_FACTORY,     9999,        	1000,             LEVEL_3, SAVE_ENABLE,  CallbackUpdateUserLevel},
    {&ENKO[0],                          DEVICE_ENKO_PASSWORD,                  PASS_DEF_ENKO,        9999,        	1000,             LEVEL_4, SAVE_ENABLE,  CallbackUpdateUserLevel},
    {&ENKO[6],                          LOGIN_PASSWORD_ENTRY_ENKO_REG1,        0x0000,               32767,        -32767,            LEVEL_0, SAVE_DISABLE, CallbackLoginEnko},
    {&ENKO[7],                          LOGIN_PASSWORD_ENTRY_ENKO_REG2,        0x0000,               32767,        -32767,            LEVEL_0, SAVE_DISABLE, CallbackLoginEnko},
    {&ENKO[4],                          LOGIN_SECURITY_ENKO_REG1,              0x66BB,               32767,        -32767,            LEVEL_1, SAVE_DISABLE, NULL},
    {&ENKO[5],                          LOGIN_SECURITY_ENKO_REG2,              0xBB66,               32767,        -32767,            LEVEL_1, SAVE_DISABLE, NULL},
    {(int16_t*)(USER_PROG_START_ADR- 2),DEVICE_BOOTLOADER_VER,                 0,                    32767,        -32767,            LEVEL_1, SAVE_DISABLE, NULL},
    {&FW,                               DEVICE_SOFTWARE_VER,                   FW_VERSION,           9999,         0,                 LEVEL_1, SAVE_DISABLE, NULL},
    {&ENKO[8],                          DEVICE_IDENKO_REG1,                    0,                    32767,        -32767,            LEVEL_1, SAVE_DISABLE, NULL},
    {&ENKO[9],                          DEVICE_IDENKO_REG2,                    0,                    32767,        -32767,            LEVEL_1, SAVE_DISABLE, NULL},
    {&ENKO[10],                         DEVICE_IDENKO_REG3,                    0,                    32767,        -32767,            LEVEL_1, SAVE_DISABLE, NULL},
    {(int16_t*)&MODEL_NAME[0],          DEVICE_MODEL_NAME_REG1,                0,                    32767,        -32767,            LEVEL_1, SAVE_DISABLE, NULL},
    {(int16_t*)&MODEL_NAME[1],          DEVICE_MODEL_NAME_REG2,                0,                    32767,        -32767,            LEVEL_1, SAVE_DISABLE, NULL},
    {(int16_t*)&MODEL_NAME[2],          DEVICE_MODEL_NAME_REG3,                0,                    32767,        -32767,            LEVEL_1, SAVE_DISABLE, NULL},
    {(int16_t*)&MODEL_NAME[3],          DEVICE_MODEL_NAME_REG4,                0,                    32767,        -32767,            LEVEL_1, SAVE_DISABLE, NULL},
    {(int16_t*)&MODEL_NAME[4],          DEVICE_MODEL_NAME_REG5,                0,                    32767,        -32767,            LEVEL_1, SAVE_DISABLE, NULL},
    {(int16_t*)&MODEL_NAME[5],          DEVICE_MODEL_NAME_REG6,                0,                    32767,        -32767,            LEVEL_1, SAVE_DISABLE, NULL},
    {(int16_t*)&MODEL_NAME[6],          DEVICE_MODEL_NAME_REG7,                0,                    32767,        -32767,            LEVEL_1, SAVE_DISABLE, NULL},
    {(int16_t*)&MODEL_NAME[7],          DEVICE_MODEL_NAME_REG8,                0,                    32767,        -32767,            LEVEL_1, SAVE_DISABLE, NULL},
    {(int16_t*)&MODEL_NAME[8],          DEVICE_MODEL_NAME_REG9,                0,                    32767,        -32767,            LEVEL_1, SAVE_DISABLE, NULL},
    {(int16_t*)&MODEL_NAME[9],          DEVICE_MODEL_NAME_REG10,               0,                    32767,        -32767,            LEVEL_1, SAVE_DISABLE, NULL},
    

    // New charge and power supply parameters
	{&(controller.chargeVoltage), 		CHARGE_VOLTAGE,                    	   1380,                 MAX_VOLTAGE,  MIN_VOLTAGE,       LEVEL_1, SAVE_ENABLE,  callBackChargerUpdateVoltageREF},
	{&(controller.chargeCurrent), 		CHARGE_CURRENT,                        NOM_CURRENT,          MAX_CURRENT,  MIN_CURRENT,       LEVEL_1, SAVE_ENABLE,  callBackChargerUpdateCurrentREF},
    {&(controller.floatVoltage),        FLOAT_VOLTAGE,                         1250,                 1400,         1200,              LEVEL_2, SAVE_ENABLE,  NULL},
    {&(controller.floatCurrentThreshold),FLOAT_CURRENT_THRESHOLD,              100,                  300,          0,                 LEVEL_3, SAVE_ENABLE,  NULL},
    {&(controller.floatDuration),       FLOAT_DURATION,                        15,                   300,          0,                 LEVEL_3, SAVE_ENABLE,  NULL},
    {&(controller.enkoSignature),       ENKO_SIGNATURE_REG,                    ENKO_SIGNATURE,       32767,        -32767,            LEVEL_4, SAVE_ENABLE,  NULL},
    {NULL,                              OUTPUT_BLANK_4,                        1300,                 1450,         1100,              LEVEL_4, SAVE_ENABLE,  NULL},
    {NULL,                              OUTPUT_BLANK_5,                        150,                  500,          10,                LEVEL_4, SAVE_ENABLE,  NULL},
    
    // New boost parameters
    {&(controller.boostSource),         BOOST_SOURCE,                          2,                    2,            0,                 LEVEL_2, SAVE_ENABLE,  NULL},
    {&(controller.boostEdgeSelection),  BOOST_EDGE_SELECTION,                  0,                    1,            0,                 LEVEL_2, SAVE_ENABLE,  NULL},
    {&(controller.boostMaxCurrent),     BOOST_MAX_CURRENT,                     300,                  500,          50,                LEVEL_3, SAVE_ENABLE,  NULL},
    {&(controller.boostDuration),       BOOST_DURATION,                        60,                   120,          1,                 LEVEL_3, SAVE_ENABLE,  NULL},
    {&(controller.boostPeriod),         BOOST_PERIOD,                          1440,                 5940,         60,                LEVEL_3, SAVE_ENABLE,  NULL},
    
    // New PT100 temperature parameters
    {&(controller.safetyOutputVoltage), SAFETY_OUTPUT_VOLTAGE,                 1252,                 1400,          1100,             LEVEL_3, SAVE_ENABLE,  NULL},
    {&(controller.pt100LowWarningTemp), PT100_LOW_WARNING_TEMP,                50,                   100,          -400,              LEVEL_2, SAVE_ENABLE,  NULL},
    {&(controller.pt100LowAlarmTemp),   PT100_LOW_ALARM_TEMP,                  0,                    100,          -400,              LEVEL_2, SAVE_ENABLE,  CallbackPt100UnderTemp},
    {&(controller.pt100HighTripTemp),   PT100_HIGH_TRIP_TEMP,                  550,                  2000,          250,              LEVEL_3, SAVE_ENABLE,  CallbackPt100TripTemp},
    
    // Cable compensation and parallel parameters
    {&(controller.cableDropInternal), 	CABLE_VOLTAGE_DROOP_COMP,              0,                    100,          0,                 LEVEL_4, SAVE_ENABLE,  NULL},
    
    // Voltage limit parameters
    {&(controller.voltageLimitMax),     VOLTAGE_LIMIT_MAX,                     1450,                 1500,         1300,              LEVEL_4, SAVE_ENABLE,  NULL},
    {&(controller.voltageLimitMin),     VOLTAGE_LIMIT_MIN,                     1150,                 1300,         1100,              LEVEL_4, SAVE_ENABLE,  NULL},
    
    // PID parameters - Default values from defaults.h with callbacks
    {&(controller.voltagePidKp),        VOLTAGE_PID_KP,                        V_PID_KP,             1000,         0,                 LEVEL_4, SAVE_ENABLE,  CallbackUpdateVoltagePID},
    {&(controller.voltagePidKi),        VOLTAGE_PID_KI,                        V_PID_KI,             1000,         0,                 LEVEL_4, SAVE_ENABLE,  CallbackUpdateVoltagePID},
    {&(controller.voltagePidKd),        VOLTAGE_PID_KD,                        V_PID_KD,             1000,         0,                 LEVEL_4, SAVE_ENABLE,  CallbackUpdateVoltagePID},
    {&(controller.currentPidKp),        CURRENT_PID_KP,                        I_PID_KP,             1000,         0,                 LEVEL_4, SAVE_ENABLE,  CallbackUpdateCurrentPID},
    {&(controller.currentPidKi),        CURRENT_PID_KI,                        I_PID_KI,             1000,         0,                 LEVEL_4, SAVE_ENABLE,  CallbackUpdateCurrentPID},
    {&(controller.currentPidKd),        CURRENT_PID_KD,                        I_PID_KD,             1000,         0,                 LEVEL_4, SAVE_ENABLE,  CallbackUpdateCurrentPID},
  };

void ImodbusCheckEnkoSignature(void)
{
  // Check if controller.enkoSignature matches ENKO_SIGNATURE
  // If not, perform factory reset to restore default values
  if(controller.enkoSignature != ENKO_SIGNATURE) {
    // ENKO signature mismatch - perform factory reset
    extern int16_t ENKO[15];
    
    // Set factory reset flag to restore all defaults (not user calibration)
    ENKO[13] = 1; // Set to restore factory defaults
    
    // Call factory reset function
    CallbackFactoryReset();
    
    // Note: CallbackFactoryReset will reset the device, so execution won't continue here
  }
}
    
void ImodbusSaveParam(
    uint16_t indexAddr, //modbus address index that should be saved
    uint16_t data    //data to save on storage
){
  // Since index is used for saving parameters, adding inside a list is not possible
  // If any new parameter is added even if its address is in the middle
  // It has to be included at the end of the REALmodbusRegisters
  // To not cause compatiblity problem with the devices on the ground
  Flash_writeParameter(USER_PARAM_START_ADDR, indexAddr, data, MB_REG_COUNT);
}

void ImodbusBackupPasswords(void)
{
  // Backup current password values
  ENKO_PASSWORD_BACKUP = ENKO[0];
  FACTORY_PASSWORD_BACKUP = ENKO[1];
  SERVICE_PASSWORD_BACKUP = ENKO[2];
  USER_PASSWORD_BACKUP = ENKO[3];
}

void ImodbusRestorePasswords(void)
{
  // Restore password values from backup
  ENKO[0] = ENKO_PASSWORD_BACKUP;
  ENKO[1] = FACTORY_PASSWORD_BACKUP;
  ENKO[2] = SERVICE_PASSWORD_BACKUP;
  ENKO[3] = USER_PASSWORD_BACKUP;
}

void ImodbusHidePasswordsBasedOnLevel(void)
{
  // Hide passwords that are above current user level
  // This function will be called after user level changes
  // The actual hiding is handled in getParamValByAddress function
  // based on server.userLevel comparison with required access levels
  
  // No direct action needed here since the security is implemented
  // in getParamValByAddress and setParamValByAddress functions
}

void ImodbusServerInit(
    modbusServerObject_t *server //Server object to fill in
){
  // Init sAddr with default value. But this can get changed by core-init
  modbusServerCreate(server, DEF_MB_ADDR, Server_RTU, REALmodbusRegisters, MB_REG_COUNT, &ImodbusSaveParam, MB_TIMEOUT_MS);
  
  // Backup initial password values
  ImodbusBackupPasswords();
}
