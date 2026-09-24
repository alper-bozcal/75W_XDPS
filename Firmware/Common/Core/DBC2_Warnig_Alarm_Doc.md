# DBC2 Battery Charger – Error, Alarm & Warning Flags Documentation

This document describes the runtime diagnostic bitmasks `controller.flagERR` (errors/alarms) and `controller.flagWARN` (warnings) used in the DBC2 Battery Charger firmware.

> NOTE: File name typo kept for backward compatibility (`DBC2_Warnig_Alarm_Doc.md`). Prefer referencing it as “Warning & Alarm Documentation”.

## Overview

- **flagERR (ERR_FLAG)**: Critical, blocking faults OR alarms requiring protective action. Type: `int16_t` (bitmask)
- **flagWARN (WARN_FLAG)**: Non‑blocking informational conditions. Type: `int16_t` (bitmask)
- Multiple bits may be active simultaneously; firmware clears bits automatically when conditions disappear.
- Recent refactor (Oct 2025) moved PT100 connection failure from warning to error (bit 6 / 0x40) and deprecated ADC open / over‑current warnings from public documentation.

## Error / Alarm Bits (`controller.flagERR`)

| Bit | Hex | Symbol | Condition Source | Trigger Summary | Action |
|-----|-----|--------|------------------|-----------------|--------|
| 0 | 0x01 | OutputReverse | `protectionOutputReverse` | Battery polarity reversed | Relay -> `RELAY_ERROR`, output disabled, LED pattern 1 |
| 1 | 0x02 | OverTempError | `protectionOverTemp` (NTC/internal) | Internal temperature over limit | Relay error, LED pattern 5 (see LED section) |
| 2 | 0x04 | OverVolt | `protectionOverVolt` | Output voltage > (boostVoltage + 100mV) | Relay error, LED pattern 2 |
| 3 | 0x08 | PT100UnderTemp | `protectionUnderTempPT100` | PT100 < `pt100LowAlarmTemp` | Relay error, LED pattern 8 |
| 4 | 0x10 | PT100TripTemp | `protectionTripTemp` | PT100 ≥ `pt100HighTripTemp` (Trip) | Output cut off (`flagCutOff = NOT_CONDUCTING`), LED pattern 6 |
| 5 | 0x20 | PT100OverTemp | `protectionOverTempPT100` | PT100 ≥ `Pt100AlarmTemp` (Alarm) | Relay error, LED pattern 7 |
| 6 | 0x40 | PT100Broken | `protectionPT100Broken` | PT100 physically disconnected / broken | Relay error, LED pattern 4 |
| 7 | 0x80 | BatteryDisconnect | `protectionBatteryDisconnect` | 10 failed connection detections within 1h | Relay error, LED pattern 3 |

Notes:
1. Bit numbering follows least‑significant bit = 0.
2. “LED pattern” refers to `controller.LEDBlink` values (NOT fixed Hz frequencies; pattern sequencing logic described later).
3. PT100 “Trip” (bit 4) is a hard shutdown; PT100 “OverTemp” (bit 5) is pre‑trip alarm; PT100 “Broken” (bit 6) indicates sensor integrity failure.

## Warning Bits (`controller.flagWARN`)

| Bit | Hex | Symbol | Implementation | Trigger Condition | Source File | Comments |
|-----|-----|--------|---------------|-------------------|-------------|----------|
| 0 | 0x01 | ADCOpen | **Disabled (commented)** | `isADCOpen(measuredValues) == STAT_TRUE` | Interfaces.c:330-332 | Logic completely commented out |
| 1 | 0x02 | OverTempWarn | **Active** | Temperature > `POWER_DERATE_START_TEMP` during power derating | Controller.c:494,498,516 | Thermal derating warning system |
| 2 | 0x04 | PT100Connection | **Active** | `controller.flagPt100 == ENABLED` && `Tpt.flagOpen == ENABLED` | Controller.c:36,39,46 | PT100 enabled but sensor not connected |
| 3 | 0x08 | Reserved | **Unused** | – | – | Not implemented |
| 4 | 0x10 | PT100WarningTemp | **Active** | `controller.flagPt100` && `measuredValues.CPT100 > controller.Pt100WarningTemp` | Interfaces.c:297,300 | PT100 over temperature warning |
| 5 | 0x20 | WrongBattWarn | **Active** | Battery voltage mismatch in auto-detect or manual mode | Controller.c:141; Interfaces.c:483 | Wrong battery type detection |
| 6 | 0x40 | PT100UnderTempWarning | **Active** | `controller.flagPt100` && `measuredValues.CPT100 < controller.pt100LowWarningTemp` | Interfaces.c:305,308 | PT100 under temperature warning |
| 7-15 | – | Reserved | **Unused** | – | – | Not implemented |

### Disabled/Commented Warning Bits

| Bit | Hex | Symbol | Status | Location | Reason |
|-----|-----|--------|--------|----------|--------|
| 1 | 0x02 | OverCurrent | **Disabled (commented)** | Interfaces.c:266,268 | Over current warning commented out to avoid noise |

Rationale: 
- **ADC Open Detection**: Completely removed to eliminate false positives
- **Over Current Warning**: Disabled in favor of protection-only approach to reduce noise
- **Bit Reuse**: Bit 1 (0x02) repurposed from OverCurrent to OverTempWarn for thermal derating
- **PT100 Separation**: 
  - **Connection Warning** (bit 2): Informational when feature enabled but sensor missing
  - **Broken Sensor Error** (error bit 6): Critical failure requiring protective action
- **Dual Implementation**: WrongBattWarn implemented in both auto-detect and manual mode validation

## Protection Parameters

### Error / Alarm Protection Definitions
- **protectionOverVolt**: Over-voltage protection with threshold at controller.boostVoltage + 100 mV
- **protectionOutputReverse**: Reverse polarity protection (when battery is connected backwards)
- **protectionOverCurrent**: Over-current protection (when maximum current limit is exceeded) - *Note: Currently monitored but does not set warning flags*
- **protectionOverTemp**: Internal temperature protection (for MCU internal temperature)
- **protectionTripTemp**: PT100 high temperature trip protection with threshold at controller.pt100HighTripTemp (default: 450 = 45.0°C)
- **protectionOverTempPT100**: PT100 alarm temperature protection with threshold at controller.Pt100AlarmTemp (default: 1000 = 100.0°C)
- **protectionUnderTempPT100**: PT100 under-temperature protection with threshold at controller.pt100LowAlarmTemp (default: 0 = 0.0°C)
- **protectionBatteryDisconnect**: Battery connection detection protection (1s delay) triggers when `batteryConnectionStatus >= 1` after algorithm concludes disconnection.
- **protectionPT100Broken**: PT100 integrity protection, triggers when `pt100ConnectionStatus` indicates sensor loss (1s delay). Sets error bit 6.

### Warning Temperature Parameters & Derate System
- **Pt100WarningTemp**: PT100 warning temperature (default: 750 = 75.0°C)
- **Pt100AlarmTemp**: PT100 alarm temperature (default: 1000 = 100.0°C)
- **pt100LowWarningTemp**: PT100 low warning temperature (default: 50 = 5.0°C)
- **pt100LowAlarmTemp**: PT100 low alarm temperature (default: 0 = 0.0°C)
- **POWER_DERATE_START_TEMP**: Temperature at which power limiting begins
- **POWER_DERATE_END_TEMP**: Temperature at which power limiting reaches maximum
- Derate system applies power limiting with temperature increase and cuts output at very high temperatures

## Relay State Control

Any active error bit forces the relay into a safe error state (`RELAY_ERROR`). PT100TripTemp additionally sets `flagCutOff = NOT_CONDUCTING` for immediate output shutdown.

## LED Indication Logic

LED behavior is priority‑based (first matching condition in `Interfaces.c`). `controller.LEDBlink` is an integer pattern code, not a direct Hz value.

Priority & Pattern Codes (higher up = higher priority):

| Priority Order | Condition (Protection) | Error Bit | LEDBlink Code |
|----------------|------------------------|-----------|---------------|
| 1 | Reverse Polarity (`protectionOutputReverse`) | 0x01 | 1 |
| 2 | Over Voltage (`protectionOverVolt`) | 0x04 | 2 |
| 3 | Battery Disconnect (`protectionBatteryDisconnect`) | 0x80 | 3 |
| 4 | PT100 Broken (`protectionPT100Broken`) | 0x40 | 4 |
| 5 | Over Temp MCU (`protectionOverTemp`) | 0x02 | 5 |
| 6 | PT100 Trip (`protectionTripTemp`) | 0x10 | 6 |
| 7 | PT100 Over Temp (`protectionOverTempPT100`) | 0x20 | 7 |
| 8 | PT100 Under Temp (`protectionUnderTempPT100`) | 0x08 | 8 |
| 9 | Over Current (`protectionOverCurrent`) | (no active bit / warning disabled) | 9 |

Blink Timing: The firmware toggles LED state with a tick window derived from `LED_DEFAULT_BLINK_TIME` and `(LEDBlink * 2)` offsets. Exact visual frequency depends on timer configuration and may not match legacy Hz documentation.

## Modbus Addresses & Access

- **ERR_FLAG**: Modbus address for error flags
- **WARN_FLAG**: Modbus address for warning flags
- **Access Level**: LEVEL_0 (read-only)
- **Save**: SAVE_DISABLE (not saved to memory)
- Values reflect real-time system status

## Flag Details

### ADC Open Circuit Detection (Deprecated)
- **Implementation**: Completely commented out in `Interfaces.c` lines 330-332
- **Previous Logic**: Would set warning bit 0 (0x01) when `isADCOpen(measuredValues) == STAT_TRUE`
- **Current Status**: Not active; code commented with "//Uyari kaldirildi." (Warning removed)

### Over Temperature Warning System (Power Derating)
- **Active Bit**: 1 (0x02) - `OverTempWarn`
- **Implementation**: `Controller.c` lines 494, 498, 516 in `updateCurrentByDerate()` function
- **Trigger Conditions**:
  - Sets when: `temperature > POWER_DERATE_START_TEMP`
  - Clears when: `temperature <= POWER_DERATE_START_TEMP`
- **Purpose**: Indicates active thermal power limiting/derating
- **Operation**: Works with linear power derating system to reduce current as temperature increases

### Over Current Warning (Disabled)
- **Implementation**: Commented out in `Interfaces.c` lines 266-268
- **Previous Logic**: Would set/clear warning bit 1 (0x02) based on `protectionOverCurrent.triggered`
- **Current Status**: Disabled to avoid noise; same bit (0x02) now used for OverTempWarn

### PT100 Connection Warning System
- **Active Bit**: 2 (0x04) - `PT100Connection`  
- **Implementation**: `Controller.c` lines 36, 39, 46 in `_checkTemperature()` function
- **Trigger Logic**:
  - Sets when: `controller.flagPt100 == ENABLED` AND `Tpt.flagOpen == ENABLED`
  - Clears when: PT100 is connected OR PT100 feature is disabled
- **Purpose**: Warns when PT100 temperature monitoring is enabled but sensor is not physically connected
- **Distinction**: This is different from PT100Broken error - this warns about missing sensor when feature is enabled

### PT100 Temperature Warning System
- **Warning Bits**: 
  - Bit 4 (0x10): `PT100WarningTemp` - Over temperature warning
  - Bit 6 (0x40): `PT100UnderTempWarning` - Under temperature warning
- **Implementation**: `Interfaces.c` lines 297-300 (over temp), 305-308 (under temp)
- **Trigger Conditions**:
  - **Over Temp**: `controller.flagPt100` AND `measuredValues.CPT100 > controller.Pt100WarningTemp`
  - **Under Temp**: `controller.flagPt100` AND `measuredValues.CPT100 < controller.pt100LowWarningTemp`
- **Requirements**: Both require `controller.flagPt100` to be enabled
- **Purpose**: Early warning system before reaching error/alarm thresholds

### Battery Type Mismatch Warning System
- **Active Bit**: 5 (0x20) - `WrongBattWarn`
- **Implementation**: 
  - `Controller.c` lines 129, 135, 141 in `outputAutoDetect()` function
  - `Interfaces.c` lines 472, 478, 483 in mode switching logic
- **Trigger Conditions**:
  - **Auto-detect mode**: When battery voltage doesn't match expected ranges (12V/24V)
  - **Manual mode**: When measured voltage doesn't match configured charge voltage
- **Clearing**: Automatically clears when voltage matches expected ranges
- **Purpose**: Alerts user to check battery type selection or wiring connections

### Battery Connection Detection System
- **Purpose**: Detects whether a battery is actually connected to the charger output
- **Detection Method**: Applies 12V test voltage and analyzes voltage/current response ratios
- **Detection Algorithm**:
  - Runs every 6 minutes when current is below 90% of effective current
  - Stores baseline voltage and current measurements
  - Applies 12V test voltage for 100ms
  - Compares voltage and current change ratios
  - Battery connected: Current ratio significantly higher than voltage ratio
  - No battery: Voltage and current ratios approximately equal (resistive load)
- **Protection Trigger**: After 10 consecutive failed detections within 1 hour
- **Protection Delay**: 1 second after batteryConnectionStatus becomes 1
- **Status Variable**: `batteryConnectionStatus` (0 = connected, 1 = disconnected)
- **Configuration Parameters** (in Controller.h):
  - `BAT_CONN_CHECK_DURATION_MS`: 100ms monitoring duration
  - `BAT_CONN_TEST_VOLTAGE_MV`: 12000mV test voltage
  - `BAT_CONN_CHECK_INTERVAL_MS`: 360000ms (6 minutes) between checks
  - `BAT_CONN_MAX_FAILED_CHECKS`: 10 consecutive failures required
  - `BAT_CONN_CHECK_WINDOW_MS`: 3600000ms (1 hour) time window
  - `BAT_CONN_CURRENT_THRESHOLD_PERCENT`: 90% effective current threshold
  - `BAT_CONN_VOLTAGE_THRESHOLD_MV`: 100mV voltage tolerance
  - `BAT_CONN_RATIO_TOLERANCE_PERCENT`: 20% ratio comparison tolerance
- **Result**: When triggered, sets error flag bit 7 (0x80), relay enters RELAY_ERROR state
- **Detailed Documentation**: See `battery_connection_detection.md` for complete algorithm description

### Power Limiting (Derate) System
- Dynamic power limiting vs. temperature (linear derate) with final cutoff at trip temperature.

## Notes & Revision History

1. Bits are auto‑clearing when protection conditions reset.
2. Errors enforce relay safe state; warnings are informational only.
3. LED priority ensures a single visible pattern (most critical first).
4. Deprecated warning bits (ADCOpen, OverCurrent) retained for traceability.
5. Battery disconnect & PT100 integrity moved to ERR domain for safety.

### Recent Updates (Oct 2025)
- **Complete Code Analysis**: Thoroughly analyzed `Controller.c` and `Interfaces.c` for all `controller.flagWARN` usages
- **Accurate Implementation Table**: Created detailed table showing exact source code locations and trigger conditions
- **Bit Usage Clarification**:
  - Bit 0 (0x01): ADCOpen - Completely commented out, not functional
  - Bit 1 (0x02): Repurposed from OverCurrent to OverTempWarn for thermal derating system  
  - Bit 2 (0x04): PT100Connection - Active warning for missing sensor when feature enabled
  - Bit 4 (0x10): PT100WarningTemp - Active temperature warning system
  - Bit 5 (0x20): WrongBattWarn - Dual implementation in auto and manual modes
  - Bit 6 (0x40): PT100UnderTempWarning - Active cold temperature warning
- **Disabled Features**: Documented OverCurrent warning disable in Interfaces.c (lines 266-268)
- **Source Code Synchronization**: Ensured 100% accuracy between documentation and actual implementation
- **Function-Level Detail**: Added specific function names and line numbers for each warning implementation

---
*Last update: October 22, 2025*

---
*This document was prepared for DBC2 Battery Charger Firmware.*
*Last update: October 17, 2025*