#ifndef TYPES_H
#define TYPES_H

/*
 * types.h
 * --------
 * Centralized type definitions for the Vehicle ECU project.
 * Contains all enums, structs, and shared data types.
 *
 * This file is included by all modules.
 */

/* ============================= */
/*        MODE ENUMERATION       */
/* ============================= */

/*
 * Vehicle operating modes
 */
typedef enum
{
    MODE_OFF = 0,
    MODE_ACC,
    MODE_IGNITION_ON,
    MODE_FAULT
} VehicleMode;


/* ============================= */
/*      SYSTEM STATE ENUM        */
/* ============================= */

/*
 * ECU system safety states
 */
typedef enum
{
    STATE_NORMAL = 0,
    STATE_DEGRADED,
    STATE_SAFE
} SystemState;


/* ============================= */
/*        INPUT STRUCTURE        */
/* ============================= */

/*
 * VehicleInput
 * ------------
 * Holds validated input data for one ECU cycle.
 */
typedef struct
{
    int speed;             /* km/h : 0 – 200 */
    int temperature;       /* °C   : -40 – 150 */
    int gear;              /* 0 – 5 */
    VehicleMode requestedMode;
} VehicleInput;


/* ============================= */
/*       VEHICLE STATUS          */
/* ============================= */

/*
 * VehicleStatus
 * -------------
 * Holds ECU internal state and mode information.
 */
typedef struct
{
    VehicleMode currentMode;
    VehicleMode previousMode;
    SystemState systemState;
} VehicleStatus;


/* ============================= */
/*        FAULT STRUCTURE        */
/* ============================= */

/*
 * FaultStatus
 * -----------
 * Tracks active faults and persistent fault counters.
 * Fault flags are bitwise.
 */
typedef struct
{
    unsigned int flags;            /* Active fault bitmask */

    unsigned int overspeedCount;
    unsigned int overtempCount;
    unsigned int gearCount;
    unsigned int modeCount;

} FaultStatus;

#endif /* TYPES_H */