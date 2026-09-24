/*
 * Controller.h
 *
 *  Created on: 6 Eki 2022
 *      Author: ceyhun.uysal
 */
#ifndef CONTROLLER_H
#define CONTROLLER_H

/*-------------------------------------- Includes --------------------------------------*/

#include "defaults.h"
#include "modbus-table.h"
#include "ImodbusServer.h"
#include "ImodbusCallback.h"
#include "Flash.h"
#include "stdint.h"
#include "PID.h"
#include "J1939Module.h"

/*-------------------------------------- Private Definitions --------------------------------------*/

// Timer States
#define TIMER_OFF               0
#define TIMER_RUNNING           1
#define TIMER_COMPLETE          2

// Battery Connection Detection Configuration (Updated for 10ms DEF_CONTROL_TIMER)
#define BAT_CONN_MIN_CURRENT_TRESHOLD       15     // Duration to monitor voltage after setting output [10mA]
#define BAT_CONN_CHECK_DURATION_MS          30      // Duration to monitor voltage after setting output [ms]
#define BAT_CONN_VOLTAGE_CONDITION_MV       10       // Voltage condition [10mV]
#define BAT_CONN_VOLTAGE_THRESHOLD_MV       30       // Voltage threshold above target [10mV]
#define BAT_CONN_TEST_VOLTAGE_MV            1180     // Test voltage to apply [mV]
#define BAT_CONN_CURRENT_THRESHOLD_PERCENT  95       // Percentage of effective current threshold [%]
#define BAT_CONN_CHECK_INTERVAL_MS          30000    // Interval between detection checks (6 minutes) [ms] - Adjusted for 10ms timer
#define BAT_CONN_MAX_FAILED_CHECKS          10       // Max consecutive failed checks before reporting
#define BAT_CONN_CHECK_WINDOW_MS            360000   // Time window for failed check counting (1 hour) [ms] - Adjusted for 10ms timer
#define BAT_CONN_RATIO_TOLERANCE_PERCENT    20       // Tolerance for voltage/current ratio comparison [%]

/*-------------------------------------- Private Types --------------------------------------*/

/**
 * @brief Calculated measurement values
 */
typedef struct MEASURED_s{
  //output voltage and current values
	int16_t Vout;
	int16_t Iout;
  //reverse battery voltage
	int16_t Vbat;
  //onboard ntc input value
	int16_t ntcC;
  //internal ntc temperature value
	int16_t INTC;
	int16_t PT100Comp;
	int16_t CPT100; // PT100 Temperature (100mC)
  // Operating mode value
  int16_t operationMode;
}MEASURED_t;

/**
 * @brief This struct holds ADC related data like offset and ratio.
 *        TODO should replace MEASURED_t
 */
typedef struct measurement_s{
  volatile uint16_t* adcValue;  // Input of the measurement
  int32_t filterCounter;       // Accumulator of the IIR filter
  int16_t  adcOffset;           // ADC offset to subtract from real adc value
  int16_t  adcRatio;            // Ratio to multiply real ADC values after offsetting
  int16_t adcShift;            // Shift real ADC values after multiplying with ratio to convert 12Bits
  int16_t avgValue;             // Filtered Average Value as output of the measurement
  uint8_t  flagOpen :1;         // ADC Open circuit flag. Set if ADC is out of bounds
}measurement_t;

/**
 * @brief Controller singleton struct to keep reference and effective values
 * 
 */
typedef struct controller_s{
  int16_t psuReferenceVoltage; // PSU Reference voltage value
  int16_t effectiveVoltage; // Effective voltage value
  int16_t psuReferenceCurrent; // PSU Reference current value
  int16_t effectiveCurrent; // Effective current value
  int16_t boostVoltage;     // Reference Boost voltage value
  int16_t equalizationTime; //
  int16_t autoStartBoostTime;
  int16_t cableDrop;        // Voltage Compensation by the cable drop [mV/A]
  int16_t chargeState;       //
  int16_t flagAutoBoost;
  int16_t flagModePSU;
  int16_t flagModeAuto;
  int16_t flagParallel;
  int16_t parallelCoef;
  int16_t flagPt100;
  int16_t Pt100DVDT;
  int16_t Pt100WarningTemp;
  int16_t Pt100AlarmTemp;
  int16_t maxTempMCU;
  int16_t flagBoostTriggered;
  int16_t flagERR;
  int16_t flagWARN;
  int16_t flagPWM;
  int16_t flagCutOff;
  int16_t flagRelay;
  int16_t LEDColor;
  int16_t LEDBlink;
  int16_t outputModel;
  int16_t monitorEffectVolt; // internal cable drop is not included
  int16_t derateStatus;
  
  // New charge parameters  
  int16_t chargeVoltage;           // Charge voltage
  int16_t chargeCurrent;           // Charge current
  int16_t floatVoltage;            // Float voltage
  int16_t floatCurrentThreshold;   // Float current threshold
  int16_t floatDuration;           // Float duration
  int16_t enkoSignature;           // ENKO signature
  
  // New boost parameters
  int16_t boostSource;         // Boost source
  int16_t boostEdgeSelection;  // Boost edge selection
  int16_t boostMaxCurrent;     // Boost max current
  int16_t boostDuration;       // Boost duration
  int16_t boostPeriod;         // Boost period
  
  // New PT100 temperature parameters
  int16_t safetyOutputVoltage;
  int16_t pt100LowWarningTemp; // PT100 low warning temperature
  int16_t pt100LowAlarmTemp;   // PT100 low alarm temperature
  int16_t pt100HighTripTemp;   // PT100 high trip temperature
  
  // Cable compensation and parallel parameters
  int16_t cableDropInternal;// Cable voltage droop compensation
  
  // Voltage limit parameters
  int16_t voltageLimitMax;  // Maximum voltage limit
  int16_t voltageLimitMin;  // Minimum voltage limit
  
  // PID parameters
  int16_t voltagePidKp;     // Voltage PID proportional gain
  int16_t voltagePidKi;     // Voltage PID integral gain
  int16_t voltagePidKd;     // Voltage PID derivative gain
  int16_t currentPidKp;     // Current PID proportional gain
  int16_t currentPidKi;     // Current PID integral gain
  int16_t currentPidKd;     // Current PID derivative gain
}controller_t;

/**
 * @brief Timer Structure
 * 
 */
typedef struct timer_s{
  char      state; // Time state
  long      count; // Timer count
} _timer_t;

/**
 * @brief 64-Point lookup table for NTC
 * Temperature range: -40°C to 125°C
 * Temperature stored as (°C × 10), e.g., 100 means 10.0°C
 * Default series resistor is 1K e.g.: 3V3<--1K--ADC--10K_NTC--|>GND
 */
typedef struct NTC_ADC_s{
  uint16_t adc; // ADC count
  int16_t temp; // Temperature × 10
}NTC_ADC_t;

/**
 * @brief Protection structure for holding realted infos
 *
 */
typedef struct protection_s{

  _timer_t timer;     // timer object for each protection
  uint16_t delay;     // Delay in milliseconds
  int16_t *input;   // Pointer to variable
  int16_t threshold; // Threshold to check against target
  uint8_t triggered;  // Whether the protection is triggered or not
  uint8_t mode;       // Protection mode for triggering

}protection_t;

/**
 * @brief Charging algorithm states. These are defined in Param list excel
 */
typedef enum ALG_STATES_e{
	S_PSU = 1,
	S_CHARGER = 0,
}ALG_STATES;

/**
 * @brief Charger states are different then algorithm states
 */
typedef enum CHARGE_STATES_e{
	//On boot, OUTPUT_CHARGE_STATE params starts with value 0,
	//so if here gets changed, it should be updated too.
	BOOST = 0,
	EQUALIZATION = 1,
	FLOAT = 2,
}CHARGE_STATES;

/**
 * @brief return states for algorithm functions
 */
typedef enum AlgorithmStatus_e{
  STAT_FALSE = 0,
  STAT_TRUE  = 1,
  STAT_OK  = 2,
}AlgorithmStatus;

/**
 * @brief LED current status
 */
typedef enum ledStatus_e{
  LED_ON = 0,
  LED_TOGGLE  = 1,
  LED_OFF  = 2,
}ledStatus_t;

/**
 * @brief LED Color Controls
 */
typedef enum LEDColorType_e{
  Red     = LED_R_Pin,
  Green   = LED_G_Pin,
  Blue    = LED_B_Pin,
  White   = LED_R_Pin | LED_G_Pin | LED_B_Pin,
  Purple  = LED_R_Pin | LED_B_Pin,
  Yellow  = LED_R_Pin | LED_G_Pin,
  Cyan    = LED_G_Pin | LED_B_Pin,
  Black   = 0,
}LEDColorType;

/**
 * @brief LED Blinking status
 */
typedef enum LEDStatus_e{
  LED_CONS = 1,
  LED_NCONS = 0
}LEDStatus;

/**
 * @brief Protection modes
 *
 */
typedef enum PROTECTION_MODES_e{
  MODE_UNDER,
  MODE_OVER
}PROTECTION_MODES;

/**
 * @brief relay controller states
 */
typedef enum RELAY_STATE_e{
  //off means relay is conducting.
  RELAY_CONDUCTING = 0,
  //On means relay is not conducting.
  RELAY_NOT = 1,
  // Relay is control of DEBUG. So, do not change state for now
  RELAY_WAITING = 3,
  // Relay is in control of ERROR
  RELAY_ERROR = 4,
}RELAY_STATE;

/**
 * @brief control params for CUT OFF MOSFET
 */
typedef enum CUT_OFF_STATE_e{
  NOT_CONDUCTING = 0,
  CONDUCTING = 1,
}CUT_OFF_STATE;

/**
 * @brief Battery connection detection states
 */
typedef enum BAT_CONN_STATE_e{
  BAT_CONN_IDLE = 0,           // Waiting for next check interval
  BAT_CONN_STORE_VALUES,       // Storing initial voltage and current
  BAT_CONN_APPLY_VOLTAGE,      // Applying test voltage
  BAT_CONN_MONITORING,         // Monitoring for detection duration
  BAT_CONN_EVALUATE,           // Evaluating results
}BAT_CONN_STATE;

/**
 * @brief Battery connection detection result
 */
typedef enum BAT_CONN_RESULT_e{
  BAT_CONN_RESULT_CONNECTED = 0,
  BAT_CONN_RESULT_NOT_CONNECTED = 1,
  BAT_CONN_RESULT_INDETERMINATE = 2,
}BAT_CONN_RESULT;

/**
 * @brief Battery connection detection context structure
 */
typedef struct batConnDetection_s{
  BAT_CONN_STATE state;           // Current state of detection algorithm
  _timer_t intervalTimer;         // Timer for 6-minute interval between checks
  _timer_t monitorTimer;          // Timer for 100ms monitoring duration
  int16_t storedVoltage;          // Stored voltage before test [mV]
  int16_t storedCurrent;          // Stored current before test [mA]
  int16_t storedEffectiveVoltage; // Stored effective voltage before test [mV]
  int16_t measuredVoltage;        // Measured voltage after test [mV]
  int16_t measuredCurrent;        // Measured current after test [mA]
  uint8_t failedCheckCount;       // Count of consecutive failed checks
  uint32_t firstFailTimestamp;    // Timestamp of first failed check [ms]
  uint8_t batteryNotConnected;    // Flag: 1 = battery not connected, 0 = connected
  uint8_t detectionEnabled;       // Flag to enable/disable detection
}batConnDetection_t;

/*-------------------------------------- Private Data --------------------------------------*/

static const NTC_ADC_t NTC_Lookup[64] = {
    {4085, -400}, // -40.0°C (401860Ω)
    {4081, -350}, // -35.0°C (281577Ω)
    {4075, -300}, // -30.0°C (200204Ω)
    {4067, -250}, // -25.0°C (144317Ω)
    {4057, -200}, // -20.0°C (105385Ω)
    {4043, -150}, // -15.0°C (77898Ω)
    {4026, -100}, // -10.0°C (58246Ω)
    {4004, -50},  // -5.0°C (44026Ω)
    {3977, 0},    // 0.0°C (33621Ω)
    {3943, 50},   // 5.0°C (25925Ω)
    {3902, 100},  // 10.0°C (20175Ω)
    {3852, 150},  // 15.0°C (15837Ω)
    {3792, 200},  // 20.0°C (12535Ω)
    {3779, 210},  // 21.0°C (11974Ω)
    {3766, 220},  // 22.0°C (11441Ω)
    {3752, 230},  // 23.0°C (10936Ω)
    {3738, 240},  // 24.0°C (10456Ω)
    // High resolution between 25°C and 100°C
    {3723, 250},  // 25.0°C (10000Ω)
    {3707, 260},  // 26.0°C (9567Ω)
    {3692, 270},  // 27.0°C (9155Ω)
    {3676, 280},  // 28.0°C (8764Ω)
    {3659, 290},  // 29.0°C (8391Ω)
    {3642, 300},  // 30.0°C (8037Ω)
    {3624, 310},  // 31.0°C (7700Ω)
    {3606, 320},  // 32.0°C (7379Ω)
    {3588, 330},  // 33.0°C (7074Ω)
    {3569, 340},  // 34.0°C (6783Ω)
    {3549, 350},  // 35.0°C (6506Ω)
    {3529, 360},  // 36.0°C (6241Ω)
    {3509, 370},  // 37.0°C (5989Ω)
    {3488, 380},  // 38.0°C (5749Ω)
    {3467, 390},  // 39.0°C (5520Ω)
    {3445, 400},  // 40.0°C (5301Ω)
    {3423, 410},  // 41.0°C (5093Ω)
    {3400, 420},  // 42.0°C (4894Ω)
    {3377, 430},  // 43.0°C (4703Ω)
    {3353, 440},  // 44.0°C (4522Ω)
    {3329, 450},  // 45.0°C (4348Ω)
    {3305, 460},  // 46.0°C (4182Ω)
    {3280, 470},  // 47.0°C (4024Ω)
    {3254, 480},  // 48.0°C (3872Ω)
    {3229, 490},  // 49.0°C (3727Ω)
    {3202, 500},  // 50.0°C (3588Ω)
    {3066, 550},  // 55.0°C (2978Ω)
    {2920, 600},  // 60.0°C (2486Ω)
    {2768, 650},  // 65.0°C (2086Ω)
    {2611, 700},  // 70.0°C (1760Ω)
    {2452, 750},  // 75.0°C (1492Ω)
    {2291, 800},  // 80.0°C (1270Ω)
    {2133, 850},  // 85.0°C (1087Ω)
    {1977, 900},  // 90.0°C (934Ω)
    {1827, 950},  // 95.0°C (805Ω)
    {1683, 1000}, // 100.0°C (698Ω)
    {1417, 1100}, // 110.0°C (529Ω)
    {1185, 1200}, // 120.0°C (407Ω)
    {986, 1300},  // 130.0°C (317Ω)
    {820, 1400},  // 140.0°C (250Ω)
    {682, 1500},  // 150.0°C (200Ω)
};

/*--------------------------------------Function Prototypes --------------------------------------*/

/**
 * @brief
 * 
 * @param _measured 
 */
void _checkTemperature(MEASURED_t *_measured);

/**
 * @brief Charging algorithm function
 * 
 * @param _measured 
 */
void Charging(MEASURED_t *_measured);

/**
 * @brief outputAutoDetect function to detect battery type and set related parameters
 * 
 * @param _measured 
 */
void outputAutoDetect(MEASURED_t *_measured);

/**
 * @brief checks for ADC pin possible values returns STAT_TRUE if it is abnormal"
 * 
 * @param _measured 
 * @return AlgorithmStatus 
 */
AlgorithmStatus isADCOpen(const MEASURED_t _measured);

/**
 * @brief checks for ADC pin possible values returns STAT_TRUE if it is abnormal"
 *
 * @param _measured
 * @return AlgorithmStatus
 */
AlgorithmStatus isPT100Broken(const MEASURED_t _measured);

/**
 * @brief return relayState global function for the updateRelay func.
 */
RELAY_STATE getRelayState(void);

/**
 * @brief set relayState global function for the updateRelay func.
 */
void setRelayState(RELAY_STATE r);

/**
 * @brief Periodic update function for relay state.
 * It checks for current status and updates relay accordingly.
 */
void updateRelay(void);

/**
 * @brief Update LED Status according to set parameters
 */
void controlLEDbyParam(LEDColorType led, ledStatus_t _set);

/**
 * @brief  : This function initalises the timer struct
 * @param  : Pointer to the timer object
 * @retval : void
 */
void initTimer(_timer_t *timer);

/**
 * @brief  : This function sets the timer and starts counting
 * @param  : timerPointer to the timer object
 * @param  : time Timer time in milliseconds
 * @retval : void
 */
void setTimer(_timer_t *timer, long time);

/**
 * @brief  : This function updates the timer, if running
 * @note   : Must be run from a 1ms TIMX_ISR
 * @param  : timer Pointer to the timer object
 * @retval : void
 */
void updateTimer(_timer_t *timer);

/**
 * @brief Initializes the protection
 */
void initProtection(protection_t *prot, int16_t *input, int16_t threshold, uint16_t delay, uint8_t mode);

/**
 * @brief Clears the error flag
 */
void resetProtection(protection_t* prot);

/**
 * @brief Update the error flag of protection according to input and threshold
 */
void updateProtection(protection_t *prot);

/**
 * @brief Function to update measurement struct's avgValue according to related fields
 */
void updateMeasurement(measurement_t* m, int16_t *val);

/**
 * @brief Function to update PT100 measurement struct's avgValue with q8b filter
 */
void updateMeasurementPT100(measurement_t* m, int16_t *val);

/**
 * @brief Function to update PT100 measurement with noise filtering
 * This function uses median filter + aggressive low-pass to handle immunity test noise
 */
void updateMeasurementPT100WithFilter(measurement_t* m, int16_t *val);

/**
 * @brief Initialise DBC2 measurement objects
 */
void initMeasurements(void);

/**
 * @brief load calibrations from modbus if there are any calibrations saved before
 */
void setMeasurements(void);

/**
 * @brief Update measured struct according to DBC2 measurements objects
 */
void updateMeasurements(MEASURED_t *m);

/**
 * @brief Update I Effective by the ntc temperature
 */
uint8_t updateCurrentByDerate(int16_t temperature, uint16_t vOutSet, uint16_t iOutSet);

/**
 * @brief Calculate NTC temperature from the lookup table
 * 
 */
int16_t getNTC(uint16_t adc, int16_t offset, uint16_t ratio);

/**
 * @brief Configures the memory mapping at address 0x00000000.
 * 
 * @param RCC_APB2Periph 
 * @param NewState 
 */
void RCC_APB2PeriphResetCmd(uint32_t RCC_APB2Periph, FunctionalState NewState);

/**
 * @brief Configures the memory mapping at address 0x00000000.
 * 
 * @param SYSCFG_MemoryRemap 
 */
void SYSCFG_MemoryRemapConfig(uint32_t SYSCFG_MemoryRemap);

/**
 * @brief Initialize battery connection detection system
 * 
 * @param ctx Pointer to battery connection detection context
 */
void initBatteryConnectionDetection(batConnDetection_t *ctx);

/**
 * @brief Main battery connection detection function
 * Must be called periodically (e.g., every 1ms from timer interrupt)
 * 
 * @param ctx Pointer to battery connection detection context
 * @param measured Pointer to current measurements
 * @return BAT_CONN_RESULT Current detection result
 */
BAT_CONN_RESULT updateBatteryConnectionDetection(batConnDetection_t *ctx, MEASURED_t *measured);

/**
 * @brief Get battery connection status
 * 
 * @param ctx Pointer to battery connection detection context
 * @return uint8_t 1 if battery not connected, 0 if connected
 */
uint8_t isBatteryDisconnected(const batConnDetection_t *ctx);

/**
 * @brief Reset battery connection detection state
 * 
 * @param ctx Pointer to battery connection detection context
 */
void resetBatteryConnectionDetection(batConnDetection_t *ctx);

/**
 * @brief Enable or disable battery connection detection
 * 
 * @param ctx Pointer to battery connection detection context
 * @param enable 1 to enable, 0 to disable
 */
void enableBatteryConnectionDetection(batConnDetection_t *ctx, uint8_t enable);

#endif //CONTROLLER_H
