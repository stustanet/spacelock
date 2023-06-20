#pragma once

// super secure!
#define WITH_BACKDOOR 0

#define ENDSTOP_DEAD_WINDOW_USTEPS 10000  // ignore motor start-up current
#define ENDSTOP_ROTATE_AFTER_EDGE_USTEPS 200000 // how many microsteps to rotate after endstop detection (revolution of key)

// parameters for endstop: edge detection, it integral... read the code (hardware.h)!
#define EDGE_DETECTION_THRESHOLD_UA 210000
#define EDGE_STEEPNESS_THRESHOLD_A_PER_S 5
#define FALLING_EDGE_HEIGHT_RESET_THRESHOLD_RATIO 4
#define IT_INTEGRAL_DECAY_UA 300000
#define IT_INTEGRAL_THRESHOLD_UA_US 10000000000
