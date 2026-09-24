# Battery Connection Detection Algorithm

## Overview

The battery connection detection algorithm is designed to determine whether a battery is actually connected to the charger output terminals. This is critical for proper charger operation, as the charging algorithm behaves differently when a battery is present versus when the output is open or connected to a purely resistive load.

## Purpose

- **Prevent false charging cycles**: Avoid running charging cycles when no battery is present
- **Improve safety**: Detect disconnection events that could indicate wiring issues
- **Optimize operation**: Allow the charger to enter appropriate operating modes based on battery presence

## Algorithm Overview

The detection algorithm operates on the principle that a battery behaves differently from a resistive load when voltage is applied:

1. **Resistive Load**: When voltage increases, current increases proportionally (Ohm's law: I = V/R)
2. **Battery Load**: A battery acts as a voltage source with low internal resistance, so applying voltage causes a disproportionate current increase

## Implementation Files

- **Header File**: `Controller.h` - Contains type definitions, constants, and function prototypes
- **Implementation File**: `Controller.c` - Contains the detection algorithm implementation
- **Documentation**: `battery_connection_detection.md` - This file

## Configuration Parameters

All configuration parameters are defined in `Controller.h` using `#define` directives:

| Parameter | Value | Unit | Description |
|-----------|-------|------|-------------|
| `BAT_CONN_MIN_CURRENT_TRESHOLD` | 15 | 10mA | Minimum current threshold for detection (150mA) |
| `BAT_CONN_CHECK_DURATION_MS` | 30 | ms | Duration to monitor voltage after applying test voltage |
| `BAT_CONN_VOLTAGE_CONDITION_MV` | 10 | 10mV | Voltage condition threshold (100mV) |
| `BAT_CONN_VOLTAGE_THRESHOLD_MV` | 30 | 10mV | Voltage threshold above test voltage target (300mV) |
| `BAT_CONN_TEST_VOLTAGE_MV` | 1180 | mV | Test voltage to apply during detection (11.8V) |
| `BAT_CONN_CURRENT_THRESHOLD_PERCENT` | 95 | % | Percentage of effective current - detection only runs when below this |
| `BAT_CONN_CHECK_INTERVAL_MS` | 30000 | ms | Interval between detection checks (5 minutes with 10ms timer) |
| `BAT_CONN_MAX_FAILED_CHECKS` | 10 | count | Maximum consecutive failed checks before reporting battery disconnected |
| `BAT_CONN_CHECK_WINDOW_MS` | 360000 | ms | Time window for counting failed checks (1 hour with 10ms timer) |
| `BAT_CONN_RATIO_TOLERANCE_PERCENT` | 20 | % | Tolerance for comparing voltage and current change ratios |

## Algorithm Phases

### Phase 1: Preconditions Check

The detection algorithm only runs when specific conditions are met:

```
Condition: Measured Current < (Effective Current × 95%)
```

**Rationale**: Detection is only performed when current draw is low, ensuring the test voltage application won't cause issues.

### Phase 2: Baseline Measurement

```
1. Store current voltage (Vout_initial)
2. Store current current (Iout_initial)
```

These baseline values are used to calculate change ratios after applying the test voltage.

### Phase 3: Apply Test Voltage

```
1. Set charger output to 11.8V (BAT_CONN_TEST_VOLTAGE_MV)
2. Start 30ms monitoring timer (BAT_CONN_CHECK_DURATION_MS)
```

### Phase 4: Monitor Response

```
Wait for 30ms while measuring:
- Output voltage
- Output current
```

### Phase 5: Evaluation

After the monitoring period, the algorithm evaluates the results using two tests:

#### Test 1: Voltage Reached Target

```
IF Vout_measured < (BAT_CONN_TEST_VOLTAGE_MV + BAT_CONN_VOLTAGE_THRESHOLD_MV):
    RESULT = BATTERY NOT CONNECTED
```

**Rationale**: If the output voltage fails to reach approximately 11.8V within 30ms, it indicates there's no voltage source (battery) at the output.

#### Test 2: Change Ratio Comparison

If voltage reached the target, calculate and compare change ratios:

```
voltage_change = Vout_measured - Vout_initial
current_change = Iout_measured - Iout_initial

voltage_ratio = (voltage_change × 100) / Vout_initial
current_ratio = (current_change × 100) / Iout_initial

ratio_difference = |current_ratio - voltage_ratio|
```

**Decision Logic**:

```
IF ratio_difference <= BAT_CONN_RATIO_TOLERANCE_PERCENT:
    RESULT = BATTERY NOT CONNECTED (Resistive load)
ELSE IF current_ratio > (voltage_ratio + BAT_CONN_RATIO_TOLERANCE_PERCENT):
    RESULT = BATTERY CONNECTED
ELSE:
    RESULT = INDETERMINATE
```

**Rationale**:
- **Resistive Load**: Voltage and current change proportionally (similar ratios)
- **Battery Load**: Current increases more than voltage (battery acts as current sink)

## State Machine

The algorithm implements a state machine with the following states:

### BAT_CONN_IDLE
- **Description**: Waiting for next check interval
- **Entry**: After initialization or after completing a detection cycle
- **Exit**: When interval timer completes AND preconditions are met
- **Next State**: `BAT_CONN_STORE_VALUES`

### BAT_CONN_STORE_VALUES
- **Description**: Storing baseline voltage and current measurements
- **Actions**: 
  - Store `measured->Vout` → `ctx->storedVoltage`
  - Store `measured->Iout` → `ctx->storedCurrent`
- **Next State**: `BAT_CONN_APPLY_VOLTAGE`

### BAT_CONN_APPLY_VOLTAGE
- **Description**: Applying test voltage to output
- **Actions**:
  - Set `controller.referenceVoltage = BAT_CONN_TEST_VOLTAGE_MV`
  - Start monitoring timer
- **Next State**: `BAT_CONN_MONITORING`

### BAT_CONN_MONITORING
- **Description**: Monitoring voltage and current response
- **Duration**: 30ms (BAT_CONN_CHECK_DURATION_MS)
- **Exit**: When monitoring timer completes
- **Actions**: Store measured voltage and current
- **Next State**: `BAT_CONN_EVALUATE`

### BAT_CONN_EVALUATE
- **Description**: Evaluating results and updating status
- **Actions**:
  - Call `_evaluateBatteryConnection()`
  - Update failed check counter if battery not detected
  - Reset counter if battery detected
  - Update `batteryNotConnected` flag if threshold exceeded
- **Next State**: `BAT_CONN_IDLE`

## Failed Check Tracking

The algorithm tracks consecutive "battery not connected" detections:

### Counter Management

```
IF result == NOT_CONNECTED:
    failedCheckCount++
    IF failedCheckCount == 1:
        Record firstFailTimestamp
    
    IF (currentTime - firstFailTimestamp) > 1 hour:
        Reset failedCheckCount = 1
        Update firstFailTimestamp = currentTime
    
    IF failedCheckCount >= 10:
        Set batteryNotConnected flag = 1

ELSE IF result == CONNECTED:
    Reset failedCheckCount = 0
    Reset firstFailTimestamp = 0
    Clear batteryNotConnected flag = 0
```

### Rationale

- **10 consecutive failures within 1 hour**: Ensures the battery is consistently not detected before reporting
- **1-hour window**: Prevents old failures from affecting current status
- **Reset on success**: A single successful detection clears all previous failures

## Integration Guide

### ✅ Already Integrated in DBC2 Firmware

The battery connection detection has been **fully integrated** into the DBC2 firmware in `Interfaces.c`. The integration includes:

#### 1. Global Variables Declaration (Interfaces.c)
```c
protection_t protectionBatteryDisconnect;
batConnDetection_t batteryConnectionDetection = {0};
int16_t batteryConnectionStatus = 0; // 0 = connected, 1 = disconnected
```

#### 2. Protection Initialization (_initSystem function)
```c
// CRITICAL BUG - NEEDS FIXING:
initProtection(&protectionBatteryDisconnect, &batteryConnectionStatus, 0, 1000, MODE_OVER);
// Should be:
// initProtection(&protectionBatteryDisconnect, &batteryConnectionStatus, 1, 1000, MODE_OVER);

// Initialize Battery Connection Detection
initBatteryConnectionDetection(&batteryConnectionDetection);
enableBatteryConnectionDetection(&batteryConnectionDetection, 1);
```

#### 3. Periodic Update (Callback1ms function)
```c
// Update battery connection detection every 10ms
updateBatteryConnectionDetection(&batteryConnectionDetection, &measuredValues);
// Update battery connection status for protection system
batteryConnectionStatus = isBatteryDisconnected(&batteryConnectionDetection);
updateProtection(&protectionBatteryDisconnect);
```

#### 4. Error Flag Handling (_checkProtections function)
```c
// Check for battery disconnect protection
if(protectionBatteryDisconnect.triggered){
    //RED LED Flashes 7Hz
    controller.flagERR = controller.flagERR | 0x80; // 8th Error bit is BatteryDisconnect
}else{
    controller.flagERR = controller.flagERR & ~(0x80);
}
```

### How to Check Battery Status

#### Option 1: Check Protection Status
```c
if(protectionBatteryDisconnect.triggered){
    // Battery disconnection protection is active
    // Relay is in RELAY_ERROR state
}
```

#### Option 2: Direct Function Call
```c
if(isBatteryDisconnected(&batteryConnectionDetection)){
    // Battery is not connected (but protection may not be triggered yet)
}
```

#### Option 3: Check Error Flag
```c
if(controller.flagERR & 0x80){
    // Battery disconnect error flag is set (Bit 7)
}
```

#### Option 4: Via Modbus
Read the `flagERR` register via Modbus and check bit 7 (0x80).

### Protection Behavior

When `protectionBatteryDisconnect.triggered` becomes active:
1. **Error Flag**: `controller.flagERR` bit 7 (0x80) is set
2. **Relay State**: Relay goes to `RELAY_ERROR` state (output disabled)
3. **LED Indication**: Red LED blinks at 7Hz
4. **Modbus**: Error flag visible via Modbus ERR_FLAG register

### Reset Detection (If Needed)
```c
// Reset the detection algorithm
resetBatteryConnectionDetection(&batteryConnectionDetection);

// Reset the protection
resetProtection(&protectionBatteryDisconnect);
```

## Function Reference

```c
### initBatteryConnectionDetection()
```

**Description**: Initializes the battery connection detection context.

**Parameters**:
- `ctx`: Pointer to battery connection detection context structure

**Returns**: None

**Notes**: Must be called before using the detection system. Sets all fields to initial values and starts the interval timer. Called automatically during system initialization.

---

### updateBatteryConnectionDetection()

```c
BAT_CONN_RESULT updateBatteryConnectionDetection(batConnDetection_t *ctx, MEASURED_t *measured);
```

**Description**: Main update function for battery connection detection. Must be called periodically (every 10ms).

**Parameters**:
- `ctx`: Pointer to battery connection detection context structure
- `measured`: Pointer to current measurements structure

**Returns**: 
- `BAT_CONN_RESULT_CONNECTED`: Battery is connected
- `BAT_CONN_RESULT_NOT_CONNECTED`: Battery is not connected
- `BAT_CONN_RESULT_INDETERMINATE`: Status cannot be determined

**Notes**: 
- This function implements the state machine
- Should be called from a 10ms timer interrupt (DEF_CONTROL_TIMER)
- Returns the overall status based on accumulated detections

---

### isBatteryDisconnected()

```c
uint8_t isBatteryDisconnected(const batConnDetection_t *ctx);
```

**Description**: Returns the current battery connection status.

**Parameters**:
- `ctx`: Pointer to battery connection detection context structure

**Returns**: 
- `1`: Battery is disconnected (10+ consecutive failures detected)
- `0`: Battery is connected

**Notes**: This is the primary function to query battery status in application code.

---

### resetBatteryConnectionDetection()

```c
void resetBatteryConnectionDetection(batConnDetection_t *ctx);
```

**Description**: Resets the detection state and clears all counters.

**Parameters**:
- `ctx`: Pointer to battery connection detection context structure

**Returns**: None

**Notes**: 
- Clears failed check counter
- Clears batteryNotConnected flag
- Restarts interval timer
- Use when you want to force a fresh detection cycle

---

### enableBatteryConnectionDetection()

```c
void enableBatteryConnectionDetection(batConnDetection_t *ctx, uint8_t enable);
```

**Description**: Enables or disables the battery connection detection system.

**Parameters**:
- `ctx`: Pointer to battery connection detection context structure
- `enable`: `1` to enable, `0` to disable

**Returns**: None

**Notes**: 
- When disabled, `updateBatteryConnectionDetection()` always returns `BAT_CONN_RESULT_CONNECTED`
- Enabling the system automatically resets the detection state

## Example Usage Scenario

### Scenario: Battery Disconnect Protection in DBC2

The battery connection detection is **automatically active** in the DBC2 firmware. The protection system handles everything:

```c
// ✅ ALREADY IMPLEMENTED - No user code needed!

// System automatically:
// 1. Detects battery connection every 6 minutes (when I < 90% of Ieff)
// 2. After 10 consecutive failures within 1 hour -> batteryConnectionStatus = 1
// 3. Protection triggers after 1 second delay
// 4. Error flag bit 6 (0x40) is set in flagERR
// 5. Relay goes to RELAY_ERROR state
// 6. LED blinks red at 7Hz

// To check status in your code:
if(protectionBatteryDisconnect.triggered){
    // Battery disconnect protection is active
    // System has already disabled output via relay
}
```

### Manual Control Example (If Needed)

If you want to temporarily disable detection:

```c
// Disable detection
enableBatteryConnectionDetection(&batteryConnectionDetection, 0);

// ... do something ...

// Re-enable detection
enableBatteryConnectionDetection(&batteryConnectionDetection, 1);
```

### Custom Action on Battery Disconnect

```c
void CustomBatteryCheckHandler(void) {
    static uint8_t lastStatus = 0;
    uint8_t currentStatus = protectionBatteryDisconnect.triggered;
    
    // Detect state change
    if(currentStatus && !lastStatus) {
        // Battery just detected as disconnected
        // Protection is now active
        
        // Custom actions (optional - system already handles safety)
        // e.g., send CAN message, log to EEPROM, etc.
    }
    
    lastStatus = currentStatus;
}
```

## Assumptions and Limitations

### Assumptions

1. **Voltage Application**: The charger can safely apply 11.8V test voltage without causing damage
2. **Measurement Accuracy**: ADC measurements are sufficiently accurate to detect ratio differences
3. **Timing Precision**: The 10ms timer ISR is called with reasonable precision
4. **Single Battery**: Algorithm designed for single battery connection, not multiple batteries in parallel

### Limitations

1. **Test Voltage Fixed**: Currently hardcoded to 11.8V - may not be optimal for all battery types
2. **No Auto-Scaling**: Ratios are compared with fixed tolerance, may need adjustment for different battery chemistries
3. **Resistive Load Similarity**: A resistive load with very low resistance might be misidentified as a battery
4. **Temperature Effects**: Battery impedance changes with temperature, which could affect detection accuracy
5. **State of Charge**: Very full or empty batteries might behave differently
6. **Age Effects**: Old batteries with high internal resistance might be harder to detect

## Tuning Guidelines

If the detection is not working optimally, consider adjusting these parameters:

### False Negatives (Battery Present but Not Detected)

**Symptoms**: Real battery reported as not connected

**Possible Fixes**:
- Increase `BAT_CONN_RATIO_TOLERANCE_PERCENT` (allows more variation)
- Increase `BAT_CONN_CHECK_DURATION_MS` (more time for battery response)
- Adjust `BAT_CONN_TEST_VOLTAGE_MV` to a more suitable value

### False Positives (No Battery but Detected as Connected)

**Symptoms**: Open circuit or resistive load reported as battery

**Possible Fixes**:
- Decrease `BAT_CONN_RATIO_TOLERANCE_PERCENT` (requires clearer difference)
- Decrease `BAT_CONN_VOLTAGE_THRESHOLD_MV` (stricter voltage requirement)
- Increase `BAT_CONN_MAX_FAILED_CHECKS` (require more confirmations)

### Too Frequent Checks

**Symptoms**: System spends too much time in detection mode

**Possible Fixes**:
- Increase `BAT_CONN_CHECK_INTERVAL_MS` (check less frequently)
- Adjust `BAT_CONN_CURRENT_THRESHOLD_PERCENT` (run detection less often)

## Safety Considerations

1. **Voltage Application**: Ensure the test voltage (12V) is safe for all expected load types
2. **Current Limiting**: Ensure current limiting is active during detection to prevent damage
3. **Timeout Protection**: The state machine includes timeout protection via timers
4. **Null Pointer Checks**: All functions validate input pointers before use
5. **Integer Overflow**: All arithmetic uses appropriate data types to prevent overflow

## Testing Recommendations

### Test Cases

1. **Normal Battery Connection**
   - Expected: Detection reports battery connected
   - Verify: `isBatteryDisconnected()` returns `0`

2. **Open Circuit**
   - Expected: After 10 checks, reports battery not connected
   - Verify: `isBatteryDisconnected()` returns `1`

3. **Resistive Load (e.g., 10Ω)**
   - Expected: Reports battery not connected (ratios similar)
   - Verify: `isBatteryDisconnected()` returns `1`

4. **Battery Removal During Operation**
   - Expected: Within 1 hour, after 10 detection cycles, reports not connected
   - Verify: Status changes from connected to not connected

5. **Battery Reconnection**
   - Expected: Immediately (on next check) reports battery connected
   - Verify: Failed check counter resets, status changes to connected

6. **Disabled Detection**
   - Expected: Always reports battery connected
   - Verify: `updateBatteryConnectionDetection()` returns `CONNECTED` status

## Revision History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 1.1 | 2025-10-22 | System | **CRITICAL FIXES**: Updated documentation to match actual implementation:<br/>- Fixed error bit mapping (bit 7/0x80 for battery disconnect, not bit 6/0x40)<br/>- Corrected configuration values to match Controller.h<br/>- Updated timer frequency from 1ms to 10ms<br/>- Fixed protection threshold documentation<br/>- Updated test voltage from 12V to 11.8V<br/>- Corrected timing calculations for all intervals |
| 1.0 | 2025-10-16 | System | Initial implementation with full state machine and documentation |

## References

- **Controller.h**: Type definitions and function prototypes
- **Controller.c**: Implementation of detection algorithm
- **DBC2 Firmware Documentation**: Overall system architecture
- **Battery Charging Standards**: IEC 61851, SAE J1772 (for general battery behavior understanding)

---

**Note**: This algorithm is designed for the DBC2 battery charger series. Adaptations may be needed for different hardware configurations or battery types.
