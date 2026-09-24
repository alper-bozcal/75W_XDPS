#ifndef MODBUS_TABLE_H
#define MODBUS_TABLE_H


#define OUTPUT_PSU_VOLTAGE			0
#define OUTPUT_EFFECTIVE_VOLTAGE    1
#define OUTPUT_PSU_CURRENT			2
#define OUTPUT_EFFECTIVE_CURRENT    3
#define OUTPUT_BOOST_VOLTAGE        4
#define OUTPUT_EQUALIZATION_TIME    5
#define OUTPUT_BOOST_AUTOSTART_TIME 6
#define OUTPUT_BLANK_1              7
#define OUTPUT_BLANK_2              8
#define OUTPUT_CABLE_DROOP          9

#define OUTPUT_CHARGE_STATE         10
#define OUTPUT_AUTO_BOOST           11
#define OUTPUT_MODE_PSU             12
#define OUTPUT_MODE_AUTO            13
#define PARALLEL_WORKING            14
#define PARALLEL_CURRENT_COEFF      15

#define TEMP_PT100_ACT              16  // flagPt100
#define TEMP_PT100_DVDT             17  // Pt100DVDT  
#define TEMP_PT100_WARNING          18  // Pt100WarningTemp
#define TEMP_PT100_ALARM            19  // Pt100AlarmTemp
#define TEMP_MAX_MCU                20  // maxTempMCU

#define MONITORING_VOLTAGE          21
#define MONITORING_CURRENT          22
#define MONITORING_OPERATING_MODE   23

#define MONITORING_CPT100           24
#define MONITORING_NTC              25
#define MONITORING_INTC             26
#define MONITORING_VBAT             27
#define MONITORING_IS_BOOST_TRIG    28
#define OUTPUT_MODEL                29
#define MONITORING_DERATE_STATUS    30
#define ERR_FLAG                    31
#define WARN_FLAG                   32

#define MONITORING_ADC_VOUT         33
#define MONITORING_ADC_IOUT         34
#define MONITORING_ADC_VBAT         35
#define MONITORING_ADC_PT100        36
#define MONITORING_ADC_ONTC         37
#define MONITORING_ADC_INTC         38
#define MONITORING_EFFECTIVE_VOLT   39
#define MONITORING_BLANK_2          40

#define DEBUG_RETURN_FACTORY        41
#define VOUTPWM                     42
#define IOUTPWM                     43
#define PWMFLAG                     44
#define DEVICE_RESET                45
#define OUTPUT_CUT_OFF              46
#define OUTPUT_RELAY                47
#define LED_COLOR                   48
#define LED_BLINK                   49
#define DEBUG_TAG                   50
#define CANBUS_SOURCE_ADDR          52
#define CANBUS_DEST_ADDR            53
#define DEBUG_BLANK_3               54
#define DEBUG_BLANK_4               55

#define MEASURE_OFFSET_VOUT			56
#define MEASURE_RATIO_VOUT			57
#define MEASURE_SHIFT_VOUT			58
#define MEASURE_OFFSET_IOUT			59
#define MEASURE_RATIO_IOUT			60
#define MEASURE_SHIFT_IOUT		    61
#define MEASURE_OFFSET_INTC			62
#define MEASURE_RATIO_INTC			63
#define MEASURE_SHIFT_INTC			64
#define MEASURE_OFFSET_ONTC			65
#define MEASURE_RATIO_ONTC          66
#define MEASURE_SHIFT_ONTC          67
#define MEASURE_OFFSET_VBAT         68
#define MEASURE_RATIO_VBAT          69
#define MEASURE_SHIFT_VBAT          70
#define MEASURE_OFFSET_PT100        71
#define MEASURE_RATIO_PT100         72
#define MEASURE_SHIFT_PT100         73

// Charge parameters
#define CHARGE_VOLTAGE              74  // chargeVoltage
#define CHARGE_CURRENT              75  // chargeCurrent  
#define FLOAT_VOLTAGE               76  // floatVoltage
#define FLOAT_CURRENT_THRESHOLD     77  // floatCurrentThreshold
#define FLOAT_DURATION              78  // floatDuration
#define ENKO_SIGNATURE_REG          79  // enkoSignature
#define OUTPUT_BLANK_4              80
#define OUTPUT_BLANK_5              81

// Boost parameters
#define BOOST_SOURCE                82  // boostSource
#define BOOST_EDGE_SELECTION        83  // boostEdgeSelection
#define BOOST_MAX_CURRENT           84  // boostMaxCurrent
#define BOOST_DURATION              85  // boostDuration
#define BOOST_PERIOD                86  // boostPeriod

// PT100 temperature parameters
#define SAFETY_OUTPUT_VOLTAGE 		87  // safetyOutputVoltage
#define PT100_LOW_WARNING_TEMP      88  // pt100LowWarningTemp
#define PT100_LOW_ALARM_TEMP        89  // pt100LowAlarmTemp

#define STAT_CURR_RANGE_1_1         90
#define STAT_CURR_RANGE_1_2         91
#define STAT_CURR_RANGE_2_1         92
#define STAT_CURR_RANGE_2_2         93
#define STAT_CURR_RANGE_3_1         94
#define STAT_CURR_RANGE_3_2         95
#define STAT_CURR_RANGE_4_1         96
#define STAT_CURR_RANGE_4_2         97
#define STAT_CURR_RANGE_5_1         98
#define STAT_CURR_RANGE_5_2         99
#define STAT_CURR_RANGE_6_1         100
#define STAT_CURR_RANGE_6_2         101
#define STAT_CURR_RANGE_7_1         102
#define STAT_CURR_RANGE_7_2         103
#define STAT_CURR_RANGE_8_1         104
#define STAT_CURR_RANGE_8_2         105
#define STAT_CURR_RANGE_9_1         106
#define STAT_CURR_RANGE_9_2         107
#define STAT_CURR_RANGE_10_1        108
#define STAT_CURR_RANGE_10_2        109

#define STAT_TEMP_RANGE_1_1         110
#define STAT_TEMP_RANGE_1_2         111
#define STAT_TEMP_RANGE_2_1         112
#define STAT_TEMP_RANGE_2_2         113
#define STAT_TEMP_RANGE_3_1         114
#define STAT_TEMP_RANGE_3_2         115
#define STAT_TEMP_RANGE_4_1         116
#define STAT_TEMP_RANGE_4_2         117
#define STAT_TEMP_RANGE_5_1         118
#define STAT_TEMP_RANGE_5_2         119
#define STAT_TEMP_RANGE_6_1         120
#define STAT_TEMP_RANGE_6_2         121
#define STAT_TEMP_RANGE_7_1         122
#define STAT_TEMP_RANGE_7_2         123
#define STAT_TEMP_RANGE_8_1         124
#define STAT_TEMP_RANGE_8_2         125
#define STAT_TEMP_RANGE_9_1         126
#define STAT_TEMP_RANGE_9_2         127
#define STAT_TEMP_RANGE_10_1        128
#define STAT_TEMP_RANGE_10_2        129

#define PT100_HIGH_TRIP_TEMP        132  // pt100HighTripTemp from controller_s

// Cable compensation
#define CABLE_VOLTAGE_DROOP_COMP    133  // cableDropInternal

// Voltage limit parameters
#define VOLTAGE_LIMIT_MAX           134  // voltageLimitMax
#define VOLTAGE_LIMIT_MIN           135  // voltageLimitMin

// PID parameters
#define VOLTAGE_PID_KP              136  // voltagePidKp
#define VOLTAGE_PID_KI              137  // voltagePidKi
#define VOLTAGE_PID_KD              138  // voltagePidKd
#define CURRENT_PID_KP              139  // currentPidKp
#define CURRENT_PID_KI              140  // currentPidKi
#define CURRENT_PID_KD              141  // currentPidKd



#define DEVICE_ENKO_PASS_ENTRY             9941
#define DEVICE_USER_PASS_ENTRY             9942
#define DEVICE_ENKO_PASSWORD               9943
#define DEVICE_FACTORY_PASSWORD            9944
#define DEVICE_SERVICE_PASSWORD            9945
#define DEVICE_USER_PASSWORD               9946
#define DEVICE_DYNPASS_USERCODE            9951
#define DEVICE_DYNPASS_SERVICECODE         9952
#define DEVICE_DYNPASS_FACTORYCODE         9953
#define DEVICE_DYNPASS_ENKOCODE            9954
#define DEVICE_DYNPASS_USERPASS            9955
#define DEVICE_DYNPASS_SERVICEPASS         9956
#define DEVICE_DYNPASS_FACTORYPASS         9957
#define DEVICE_DYNPASS_ENKOPASS            9958
#define LOGIN_PASSWORD_ENTRY_CUSTOMER_REG1 9963
#define LOGIN_PASSWORD_ENTRY_CUSTOMER_REG2 9964
#define LOGIN_SECURITY_CUSTOMER_REG1       9965
#define LOGIN_SECURITY_CUSTOMER_REG2       9966
#define LOGIN_PASSWORD_ENTRY_ENKO_REG1     9967
#define LOGIN_PASSWORD_ENTRY_ENKO_REG2     9968
#define LOGIN_SECURITY_ENKO_REG1           9969
#define LOGIN_SECURITY_ENKO_REG2           9970
#define DEVICE_BOOTLOADER_VER              9974
#define DEVICE_SOFTWARE_VER                9975
#define DEVICE_IDENKO_REG1                 9986
#define DEVICE_IDENKO_REG2                 9987
#define DEVICE_IDENKO_REG3                 9988
#define DEVICE_MODEL_NAME_REG1             9989
#define DEVICE_MODEL_NAME_REG2             9990
#define DEVICE_MODEL_NAME_REG3             9991
#define DEVICE_MODEL_NAME_REG4             9992
#define DEVICE_MODEL_NAME_REG5             9993
#define DEVICE_MODEL_NAME_REG6             9994
#define DEVICE_MODEL_NAME_REG7             9995
#define DEVICE_MODEL_NAME_REG8             9996
#define DEVICE_MODEL_NAME_REG9             9997
#define DEVICE_MODEL_NAME_REG10            9998


//!In any case, if above becomes bigger than 960, TAG will be overwritten by params on last page of flash

#endif //MODBUS_TABLE_H
