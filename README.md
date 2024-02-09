# Spacelock

StuStaNet Hackerspace Lock

# Firmware build instructions

The spacelock firmware can be configured for several use cases.

For each use case there is a config file in firmware/build_config.

In order to activate a config, `ln -s` the chosen config file to firmware/src/config.h

# How to adapt the config to your usecase

The firmware can be easily adapted to your hardware setup via the config file.
See [# Firmware build instructions] for more details on how to build.

Be aware that many parameters use "u" to indicate that the parameters decimal point is shifted by 1.000.000 this is done to prevent floating point operations. 

At most places where you see UREVS in the code, UREVS of the key are meanted.
`#define ENDSTOP_DEAD_WINDOW_UREVS 100'000` is one 10th of a key revolution.

## Feature Configuration
 You can decide how you want to use the spacelock board. Below are to configuraiton examples

### 1. Secure Door Unlocking via QR Code Sensor (StuSta)
In this case the board is autonomous and installed in combination with a UART QR-Code reader (`#define WITH_QRCODE`). The authentication happens via a web service and the unlock cmd is a HMAC signed QR-Code. In order to prevent e.g. replay attacks. The QR-Code holds a timestamp, and is only for valid for a limited time. Therefore an external time source is needed. In this case DCF77 this is. To prevent unwanted access set (`#define WITH_BACKDOOR 0`)


 ### 2. Unsecure UART Unlocking with Door Sensor (Biederstein)
In this case the board is controlled from a raspberry pi via UART. 
The raspberry pi does all the authorization and simply requests to open the door via UART with "open". To enable this functionality `#define WITH_BACKDOOR 1`. Furthermore a door sensor is used to check if the door is open and lock it, when it's closed (`#define WITH_DOOR_SENSOR 1`).  

## Motor Configuration
The motor should be any stepper that can be driven by the DRV8266.
The relevant motor parameters are:
- UREVS_PER_STEP eg. (1.000.000 / 200) for a motor that has 200 steps per revolution 
- MOTOR_REVS_TO_UKEY_REVS e.g. 4.000.000 for a 4:1 ratio (4 rotations of the motor is one rotation of the key)
    if you have a gearing between motor and key you can but the ratio hear.
- ENDSTOP_DEAD_WINDOW_UREVS e.g. 100'000 ignore the current for the first 10th of a key rotation (starting current)

## Lock Configuration
- UNLOCKING_UREVS maximum distance the key has to travel to turn from totally locked to unlocked
- UNLOCK_SPEED_UREVS_PER_SEC unlocking search speed in µrev/s (until a resistance is found)
- UNLOCK_FINAL_SPEED_UREVS_PER_SEC final unlocking speed in µrev/s 
- UNLOCK_HOLD_DELAY_US time to hold the lock open in µs. In our case the doors are spring loaded and will automatically swing open, when unlocked.
- UNLOCK_BACKING_UREVS µRevolutions to back the key up so the door is no longer forced open
- ENDSTOP_ROTATE_AFTER_EDGE_UREVS how far the key is to be turned after a resistance is found in µRevolutions.
- FORCE_UNLOCK_UREVS how far the key is to be forcefully turned in case gracefull unlocking fails in µRevolutions. The key is assumed to be allready close to the resistance. Thus, this should be not much more than ENDSTOP_ROTATE_AFTER_EDGE_UREVS.

## Locking Parameters
- LOCK_DELAY how long to wait until a closed door is locked (!!!TODO!!!) 
be ware something is broken here!!! in my experience a value of 5.000.000.000 relates to about 8 sec of delay.
- LOCKING_UREVS how far to lock e.g. 2 revolutions given in µRevolutions
- LOCKING_SPEED_UREVS_PER_SEC locking speed in µrev/s

## Door Sensor
- DOOR_OPEN_STATE either true or false depending on your sensor (default open, default closed)
- DOOR_PORT where you connected the sensor
- DOOR_PIN which pin you connected the sensor (be aware that the default location GPIOA PIN8 clashes with the DCF77)

## Edge Detection
These parameters depend on your motor and are best found by testing. You can use the debug output (`#define WITH_DEBUG_OUTPUT 1` and see hardware.cpp line 283) and the process_data.py to stream live current measurements into a csv and monitor it with e.g. PlotJuggler.

- EDGE_DETECTION_THRESHOLD_UA how many µA the current has to rise in order to be detected as an edge
- EDGE_STEEPNESS_THRESHOLD_A_PER_S how fast the current has to rise in A/s
- FALLING_EDGE_HEIGHT_RESET_THRESHOLD_RATIO This needs some explanation. The idea is that in theory we would only like to detect one continous edge and reset as soon as the current is dropping again. However sometimes there is a small drop in the current, but then it increases further. Actually this should be covered by EDGE_STEEPNESS_THRESHOLD_A_PER_S and should be removed in the future. For now a number of e.g. 4 means, that the edge is reset as soon as there is a 1/4 decrease from the previous max edge height. 

Apart from the edge detection there is also an current integral. power = current * voltage. Thus we use the current to estimate the power drawn from the motor and its integral to compute the dumped energy. This feature basically prevents the motor from drawing too much power for too much time. 
- IT_INTEGRAL_DECAY_UA basically gives the power draw in normal operation, any excess current is then cumulated
- IT_INTEGRAL_THRESHOLD_UA_US a "energy" threshold in µA µs for the endstop detector