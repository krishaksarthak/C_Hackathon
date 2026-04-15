// The Include Guard, prevents "redefinition" errors.

#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>

// These are the hardware limits of the sensors.
#define MAX_SPEED 200
#define MIN_SPEED 0
#define MAX_TEMP 150
#define MIN_TEMP (-40)
#define MAX_GEAR 5
#define MIN_GEAR 0

// These are logic thresholds
#define OVERSPEED_THRESHOLD 120
#define TEMP_CRITICAL_THRESHOLD 110
#define TEMP_HIGH_THRESHOLD 95

// The fault must occur in 3 consecutive cycles before it's considered persistent.
#define PERSISTENT_FAULT_LIMIT      3U // Given that it must occur 2 consecutive cycles not 3
#define MAJOR_FAULT_SAFE_THRESHOLD  2U

typedef enum
{
    MODE_OFF = 0,
    MODE_ACC = 1,
    MODE_IGNITION_ON = 2,
    MODE_FAULT = 3,
    MODE_COUNT = 4 // This is always equals the total number of valid modes.
} VehicleMode;

// Three states representing overall vehicle health.
typedef enum
{
    STATE_NORMAL = 0,
    STATE_DEGRADED = 1,
    STATE_SAFE = 2
} SystemState;

// Each value here is a bit position, not a value itself. This enum describes where each fault lives inside a 32-bit integer.
typedef enum
{
    FAULT_BIT_OVERSPEED    = 0,
    FAULT_BIT_OVERTEMP     = 1,   /* Critical Overheat (temp > 110) */
    FAULT_BIT_INVALID_GEAR = 2,
    FAULT_BIT_INVALID_MODE = 3,
    FAULT_BIT_HIGH_TEMP    = 4,   /* High Temperature  (temp >  95) */
    FAULT_BIT_COUNT        = 5
} FaultBit;

typedef enum
{
    FAULT_SEVERITY_MAJOR    = 0,  /* stay in mode, DEGRADED → SAFE on threshold */
    FAULT_SEVERITY_CRITICAL = 1   /* force MODE_FAULT, immediate STATE_SAFE      */
} FaultSeverity;

#define FAULT_OVERSPEED     (1U << (uint32_t)FAULT_BIT_OVERSPEED)     // 0b00001
#define FAULT_OVERTEMP      (1U << (uint32_t)FAULT_BIT_OVERTEMP)      // 0b00010
#define FAULT_INVALID_GEAR  (1U << (uint32_t)FAULT_BIT_INVALID_GEAR)  // 0b00100
#define FAULT_INVALID_MODE  (1U << (uint32_t)FAULT_BIT_INVALID_MODE)  // 0b01000
#define FAULT_HIGH_TEMP     (1U << (uint32_t)FAULT_BIT_HIGH_TEMP)     // 0b10000
// active_flags = 0b0101 = FAULT_OVERSPEED + FAULT_INVALID_GEAR both are active

// VehicleInput holds all raw and validated sensor readings for one cycle.
typedef struct
{
    int16_t speed;       // 0 to 200 (Unsigned 16-bit)
    int16_t temperature; // -40 to 150 (Signed 16-bit)
    int8_t gear;         // 0 to 5 (Unsigned 8-bit)
    VehicleMode requested_mode;

    uint8_t speed_valid;
    uint8_t temp_valid;
    uint8_t gear_valid;
    uint8_t mode_valid;
} VehicleInput;

// This is the runtime memory of the ECU — everything the system needs to remember between cycles.
typedef struct
{
    VehicleMode current_mode;
    VehicleMode previous_mode;
    SystemState system_state;
    SystemState previous_state;
    uint32_t cycle_count; // At 1 cycle per millisecond (typical ECU rate)

    // Fallback values - When a sensor reading is invalid, the system uses these instead of crashing or using garbage.
    int16_t last_valid_speed;
    int16_t last_valid_temp;
    int8_t last_valid_gear;

    // This flag indicates whether the system needs to be reset due to a critical fault. It can be used to trigger a safe shutdown or restart procedure.
    uint8_t  reset_required;
} VehicleStatus;

// This struct manages all fault information
typedef struct
{
    uint32_t active_flags;
    uint32_t persistent_flags;
    uint32_t counters[FAULT_BIT_COUNT];
    uint32_t consecutive[FAULT_BIT_COUNT];
} FaultStatus;

#endif