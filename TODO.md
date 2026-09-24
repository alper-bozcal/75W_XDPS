# TODO

- Create 300W .ioc
- Add CANBus Lib
- Add SPNs
- STATs are Disabled because of second signature. Re-enable them and open them up on modbus
- SCADA should be able to display stats
- TAG should be tested with new modbus, it is also disabled
- Main coreLoop() should be a state machine with well defined behaviour, not a mess like this!

Before updating the software version:
-Fix version 9999.
-Resolve CubeMX compatibility issues to ensure operation with V7 PCB (PWM frequency was increased by a factor of 10).
-Set the password to always default to Level1 for proper communication with SCADA.
-Remove USBPinDown function.
+Add tempVal to allow software-based derating.

