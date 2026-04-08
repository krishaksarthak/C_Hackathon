#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>

#define MAX_SPEED        200
#define MIN_SPEED        0
#define MAX_TEMP         150
#define MIN_TEMP         (-40)
#define MAX_GEAR         5
#define MIN_GEAR         0

#define OVERSPEED_THRESHOLD     120
#define TEMP_CRITICAL_THRESHOLD 110
#define TEMP_HIGH_THRESHOLD     95

#define PERSISTENT_FAULT_LIMIT  3

typedef enum{
    MODE_OFF = 0,
    MODE_ACC,
    MODE_IGNITION_ON,
    MODE_FAULT,
    MODE_COUNT      
} VehicleMode;

typedef enum{
    STATE_NORMAL = 0,
    STATE_DEGRADED,
    STATE_SAFE
} SystemState;

typedef enum{
    FAULT_BIT_OVERSPEED     = 0,   
    FAULT_BIT_OVERTEMP      = 1,   
    FAULT_BIT_INVALID_GEAR  = 2,   
    FAULT_BIT_INVALID_MODE  = 3,   
    FAULT_BIT_COUNT         = 4  
} FaultBit;

#define FAULT_OVERSPEED     (1U << FAULT_BIT_OVERSPEED)
#define FAULT_OVERTEMP      (1U << FAULT_BIT_OVERTEMP)
#define FAULT_INVALID_GEAR  (1U << FAULT_BIT_INVALID_GEAR)
#define FAULT_INVALID_MODE  (1U << FAULT_BIT_INVALID_MODE)


typedef struct{
    int     speed;          
    int     temperature;    
    int     gear;           
    VehicleMode requested_mode;

    uint8_t speed_valid;
    uint8_t temp_valid;
    uint8_t gear_valid;
    uint8_t mode_valid;
} VehicleInput;

typedef struct{
    VehicleMode current_mode;
    VehicleMode previous_mode;
    SystemState system_state;
    SystemState previous_state;
    uint32_t    cycle_count;

    int         last_valid_speed;
    int         last_valid_temp;
    int         last_valid_gear;
} VehicleStatus;

typedef struct{
    uint32_t active_flags;                    
    uint32_t persistent_flags;                  
    uint32_t counters[FAULT_BIT_COUNT];         
    uint32_t consecutive[FAULT_BIT_COUNT];      
} FaultStatus;

#endif