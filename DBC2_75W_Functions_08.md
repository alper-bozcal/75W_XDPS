# DBC2 75W Functions

|  |  |  | min-max | Unit | Default |
| --- | --- | --- | --- | --- | --- |
| Power  Supply | V_ps_set | Power Supply output voltage | 10.0-15.0 | VDC | 12 |
| Power  Supply | I_ps_limit | Power Supply current limit | 5.0 | ADC | 5 |
| Charge | I_bulk_max | Maximum current during Bulk (I) stage | 0.5-5.0 | ADC | 12 |
| Charge | V_abs | Absorption (U) voltage | 12.0-15.0 | VDC | 13.8 |
| Charge | T_bulk_max | Maximum duration for Bulk stage | 0-24 | Hour | 6 |
| Charge | T_abs | Absorption duration | 0-240 | min. | 120 |
| Charge | I_tail_threshold | Current threshold for transition from Absorption to Float | 0.1-0.2 | ADC | 1 |
| Charge | V_float | Float voltage | 11.0-14.5 | VDC | 13 |
| Charge | Ifloat_to_abs | Current threshold to switch from Float to Absorption (must be > I_tail_threshold) | 0.1-5.0 | ADC | 1.5 |
| Boost | Boost_En | Boost mode enable | 0-1 | - | 1 |
| Boost | Boost_Source | Boost source selection | Auto-Man-Both | - | Both |
| Boost | Boost Edge Selection | External Boost input rising/falling edge selection | Rising / Falling | - | Falling |
| Boost | V_boost | Boost voltage | 12.0-15.0 | VDC | 14.8 |
| Boost | I_boost_max | Maximum Boost current | 0.5-5.0 | ADC | 3 |
| Boost | T_boost_dur | Boost duration | 1-120 | min. | 60 |
| Boost | T_boost_per | Boost period | 1-99 | hour | 1440 |
| Pt100 | Pt100_Act | Pt100 temperature sensor active/passive | 0-1 | - | 0 |
| Pt100 | Pt100 dV/dT | Temperature compensation coefficient | -40 to +40 | mv/ºC | 0 |
| Pt100 | Pt100_temp_comp_minC | Minimum temperature for compensation | -10 to 10 | ºC | 0 |
| Pt100 | Pt100_temp_comp_maxC | Maximum temperature for compensation | 35 to 50 | ºC | 50 |
| Pt100 | Pt100_low_warning_temp | Battery low temperature warning level | -10 to 10 | ºC | 5 |
| Pt100 | Pt100_low_alarm_temp | Battery low temperature alarm level | -10 to 10 | ºC | 0 |
| Pt100 | Pt100_high_warning_temp | Battery high temperature warning level | 25 to 60 | ºC | 35 |
| Pt100 | Pt100_high_alarm_temp | Battery high temperature alarm level | 25 to 60 | ºC | 40 |
| Pt100 | Pt100_high_trip_temp | Battery high temperature trip level (output cut-off) | 25 to 60 | ºC | 45 |
| Cable Compensation | CableVoltage droop Compensation | Compensation coefficient for voltage drop across cable | 0 to 100 | mv/A | 0 |
| Paralel Working | Paralel_Working | Parallel working enable/disable | 0-1 | - | 0 |
| Paralel Working | Voltage Droop | Voltage droop for load sharing in parallel operation | 0 to -100 | mV/A | 0 |

| Calibration (ENKO Leevel) | Iout_offset |  |  |  |  |
| --- | --- | --- | --- | --- | --- |
| Calibration (ENKO Leevel) | Iout_gain |  |  |  |  |
| Calibration (ENKO Leevel) | Vout_offset |  |  |  |  |
| Calibration (ENKO Leevel) | Vout_gain |  |  |  |  |
| Calibration (ENKO Leevel) | Vbat_offset |  |  |  |  |
| Calibration (ENKO Leevel) | Vbat_gain |  |  |  |  |
| Calibration (ENKO Leevel) | Pt100_offset |  |  |  |  |
| Calibration (ENKO Leevel) | Pt100_gain |  |  |  |  |
| Calibration (ENKO Leevel) | NTC_offset |  |  |  |  |
| Calibration (ENKO Leevel) | NTC_gain |  |  |  |  |
| Calibration (ENKO Leevel) | Cupper Lines Voltage Droop Compensation |  |  |  |  |

## Detailed Parameter Descriptions

Parameters

Detailed Parameter Descriptions Based on DBC2 75W Functions

V_ps_set

Sets the output voltage of the device when operating in Power Supply mode. Adjustable between 10.0 V and 15.0 V to power external electronic circuits.

I_ps_limit

Defines the maximum current limit in Power Supply mode. If the load exceeds this value, the output current is clamped to this limit to prevent overload.

I_bulk_max

Maximum current supplied during the Bulk (I) charging stage. The device maintains constant current until the battery voltage reaches the absorption setpoint or the bulk time limit expires.

V_abs

Voltage setpoint for the Absorption (U) stage. The device maintains this voltage while the current gradually decreases as the battery approaches full charge.

T_bulk_max

Maximum duration for the Bulk stage. If the battery does not reach the absorption voltage within this time, the device transitions to the next stage.

T_abs

Duration of the Absorption stage. The device maintains the absorption voltage for this period unless the current drops below the tail threshold earlier.

I_tail_threshold

Current threshold used to determine the transition from Absorption to Float stage. When the charging current falls below this value, the device switches to Float mode.

V_float

Voltage level maintained during the Float stage to keep the battery fully charged without overcharging. Provides a trickle charge to compensate for self-discharge.

Ifloat_to_abs

Current threshold to trigger a transition from Float back to Absorption mode. Must be higher than the tail threshold to ensure proper recharge behavior.

Boost_En

Enables or disables the Boost charging mode. Boost mode applies a higher voltage to equalize battery cells and accelerate charging.

Boost_Source

Selects the source that initiates Boost mode. Can be time-based automatic activation, external input trigger, or both.

Boost Edge Selection

Defines the edge type (rising or falling) for external Boost input signal triggering. Determines how the Boost mode is activated externally.

V_boost

Voltage applied during Boost mode. This higher voltage helps equalize battery cells and ensure uniform charge levels.

I_boost_max

Maximum current allowed during Boost mode. Protects the system from excessive current during high-voltage charging.

T_boost_dur

Duration of the Boost mode operation. After this time, the device returns to Float voltage level.

T_boost_per

Periodicity of Boost mode activation. Defines how often Boost mode is triggered automatically.

Pt100_Act

Activates or deactivates the external Pt100 temperature sensor. Used for temperature-based voltage compensation.

Pt100 dV/dT

Temperature compensation coefficient in mV/°C. Determines how output voltage adjusts based on battery temperature.

Pt100_temp_comp_minC

Minimum temperature at which compensation is applied. Below this value, voltage compensation is not performed.

Pt100_temp_comp_maxC

Maximum temperature at which compensation is applied. Above this value, voltage compensation is not performed.

Pt100_low_warning_temp

Battery low temperature warning threshold. Triggers a warning when battery temperature drops below this level.

Pt100_low_alarm_temp

Battery low temperature alarm threshold. Triggers an alarm and may affect output behavior.

Pt100_high_warning_temp

Battery high temperature warning threshold. Triggers a warning when battery temperature exceeds this level.

Pt100_high_alarm_temp

Battery high temperature alarm threshold. Triggers an alarm and may affect output behavior.

Pt100_high_trip_temp

Battery high temperature trip threshold. If exceeded, output is cut off to protect the system.

CableVoltage droop Compensation

Compensation coefficient for voltage drop across output cables. Helps maintain accurate voltage at the battery terminals.

Paralel_Working

Enables or disables parallel operation mode. Allows multiple devices to share load current.

Voltage Droop

Voltage droop setting used in parallel operation for load sharing. Adjusts output voltage based on load to balance current among devices.

Boost (Equalization) 
Boost mode utilizes a higher charging voltage to rapidly recharge batteries and ensure that all cells within a battery string are equalized to the same charge level.

Boost mode can be initiated either automatically based on predefined timing parameters or manually by connecting the Boost terminal to the negative terminal of the battery. When the Boost function is activated externally, the automatic Boost timer is reset.

The external Boost input operates with edge triggering and can be configured to respond to either a rising or falling edge. Upon receiving an external Boost signal, the device output is elevated to the Boost voltage for the specified duration, after which it returns to the Float voltage level. Following this phase, the device automatically decides whether to switch to Float or Absorption mode based on the load conditions.

Remote Temperature Compensation Functionality (dV/dT) / Alarm

The charger supports remote temperature compensation based on the battery temperature when an optional external sensor is placed near the batteries and connected to the remote temperature sensor terminal block on the main circuit board. This feature is especially critical in applications where the charger and batteries are located in different ambient environments.

A Pt-100 sensor can be used as the external temperature sensor. If the temperature reading from the external sensor falls below -45°C or rises above +180°C, and remains outside these limits for a defined duration, a sensor fault is triggered. In this case, the alarm output is activated, and the output voltage is set to 12V.

The temperature compensation function is only active in Charger Mode. It is disabled in Power Supply Mode.

The temperature compensation value is expressed in mV/°C and can be either positive or negative:

A positive value increases the output voltage as temperature rises.

A negative value decreases the output voltage as temperature rises.

On the Power Cloud main screen, the Set Output value (for both current and voltage) reflects the user-defined setpoint. However, if temperature compensation is active, the effective output value shown will be adjusted accordingly.
For example, if the user sets the output voltage to 13.8V, and the external sensor reads 45°C with a compensation value of -10mV/°C, the adjustment will be:

Thus, the effective output voltage displayed will be 13.6V.

To protect against excessively high or low output voltages, the compensation effect is clamped at 0°C and +50°C. Beyond these temperatures, the output voltage is held constant at the values corresponding to these limits.

In a future update, the output voltage variation based on ambient temperature and set parameters will also be graphically displayed on the screen.

The temperature compensation value can be configured within the range of -40mV/°C to +40mV/°C.

If the external temperature sensor reading exceeds the Temperature Sensor Alarm Threshold, the output voltage is disabled, the LED turns red, and the alarm output is activated.

Internal Temperature Sensor / Alarm

The charger includes two internal temperature protection mechanisms and a power derating function, both utilizing onboard temperature sensors:

Sensor-1: Located on the secondary side, this sensor is also monitored by the MCU. It is used to implement power derating functions and to prevent device failure due to overheating.

Sensor-2: An NTC-type sensor also located on the secondary side, but not monitored by the MCU. It is directly connected to the primary SMPS IC and serves as a fail-safe. If Sensor-1 becomes non-functional or disconnected, Sensor-2 will shut down the output to protect the system. Once the temperature returns within safe limits, the output is automatically re-enabled.

Since only Sensor-1 is evaluated by the software, the following functions apply specifically to it:

The Sensor-1 temperature value can be monitored on the Power Cloud display and is also available in the log screen for recording.

If the Sensor-1 temperature is below 60°C, no action is taken.

Starting from 60°C, the output power is gradually limited.

At 70°C, the output power is reduced to 25% of the rated value.

If the temperature exceeds 70°C, the output is shut down, the red LED is turned on, and the alarm relay is activated.

Leds

Charger Mode:

Bulk : Flsahing Green ( 100msec ON, 100msec OFF)

Absorption: Flashing Green ( 400msec ON, 400msec OFF)

Float mode: Steady GREEN

Boost mode: Flashing BLUE (50msec ON, 500msec OFF)

Power Supply

Çıkış gerilimi set edilen değerde: Steady GREEN

Failure: Flashing RED

Over temperature (1 times 100msec ON, 500msec OFF)

Lower temperature (2 times 100msec ON, 500msec OFF)

Output Over Voltage (3 times 100msec ON, 500msec OFF)

Output Low Voltage (4 times 100msec ON, 500msec OFF)

Reverse Battery Connection (5 times 100msec ON, 500msec OFF)

Power Derate (6 times 100msec ON, 500msec OFF)

Battery Disconnected (only charger mode) (7 times 100msec ON, 500msec OFF)

High Ouput Current (only power supply mode) (8 times 100msec ON, 500msec OFF)

USB Communication:

Boot Mode (Firmware Update):  Steady Blue

Alarm Relay and Fault Monitoring Functions

The charger includes a SPST N/O (Single Pole Single Throw Normally Open) contact relay, which is energized during normal operation. This means the contact remains closed under normal conditions.

AC/DC Failure Condition

The alarm relay is de-energized (contact opens) under the following conditions:

Loss of AC input power

Loss of DC output from the charger

⚠️ Note: This alarm condition cannot be indicated via LED.

User-Configurable Alarm Functions

The following alarm functions can be enabled or disabled via the user interface. Each alarm can be configured as either:

Warning: Only the alarm relay is activated.

Trip: The alarm relay is activated and the output is shut down.

All alarm functions support a configurable delay time, with a default of 3 seconds. The delay can be set between 0 and 60 seconds.

Alarm Conditions

Low DC Output Voltage
Relay de-energized if:
VDC<88%×VSTATEV_{DC} < 88\% \times V_{STATE}VDC​<88%×VSTATE​
Alarm resets if:
VDC>92%×VSTATEV_{DC} > 92\% \times V_{STATE}VDC​>92%×VSTATE​

High DC Output Voltage
Relay de-energized if: VDC>107%×VSTATEVDC

Alarm resets if: VDC<102%×VSTATEVDC ​

Overvoltage Protection
Relay de-energized if:
VDC>15.1 VDCV  (This is a fixed internal threshold.)

Battery Disconnect
Relay de-energized if the battery is disconnected.

Alarm delay: 10 to 20 minutes

This function is only active in Charger Mode

High Battery Temperature
Software-configurable threshold.
Relay de-energized if the high temperature limit is exceeded.

Low Battery Temperature
Software-configurable threshold.
Relay de-energized if the low temperature limit is exceeded.

Reverse Polarity Protection
(Relay de-energized if reverse battery connection is detected.)

Connection of Deeply Discharged Battery
(Relay de-energized if a battery with excessively low voltage is connected.)

Ters Bağlantı Koruması

Düşük gerilime Sahip Akü Bağlantısı

