#pragma once

// super secure!
#define WITH_BACKDOOR 0
#define WITH_DEBUG_OUTPUT 1

//parameters for lock
#define UNLOCKING_UREVS 2'000'000 //2 key rotations
#define UNLOCK_SPEED_UREVS_PER_SEC 1'000'000
#define UNLOCK_FINAL_SPEED_UREVS_PER_SEC 500'000
#define UNLOCK_HOLD_DELAY_US 1'000'000
#define LOCKING_UREVS 250'000 //quarter rotation of the key
#define LOCKING_SPEED_UREVS_PER_SEC 1'000'000

#define ENDSTOP_DEAD_WINDOW_UREVS 400'000
#define ENDSTOP_ROTATE_AFTER_EDGE_USTEPS 200'000 // how many microsteps to rotate after endstop detection (revolution of key)

// parameters for endstop: edge detection, it integral... read the code (hardware.h)!
#define EDGE_DETECTION_THRESHOLD_UA 210000
#define EDGE_STEEPNESS_THRESHOLD_A_PER_S 5
#define FALLING_EDGE_HEIGHT_RESET_THRESHOLD_RATIO 4
#define IT_INTEGRAL_DECAY_UA 300000
#define IT_INTEGRAL_THRESHOLD_UA_US 10000000000

// motor parameters
#define MOTOR_REVS_TO_DECIKEY_REVS 4 // actually 4,0 but we divide later in the code
#define UREV_PER_STEP (uint32_t)(1000000 / 200) // 200 steps per revolution