the high-side current of the motor driver was measured

using a 50mOhm shunt, INA240A1 [on the debugadapter v1]

blue curve: raw measurement
yellow curve: filtered with 100nF and 33kOhm;

visible phases

- initial start of movement, with high current spike up to 450mA (> 300mA for ~7ms)
- normal movement phase, currents between 100mA and 300mA
- transition to mechanical stop (where the lock bolt is moved, fluctuating 100mA-300mA for ~20ms)
- mechanical stop (fluctuating 300mA - 600mA)
- holding position (constant ~420mA)
- opening (slowly rising 100mA - 300mA, plus sine wave period=7.5ms, amplitude 200mA)
- idle (0mA)
