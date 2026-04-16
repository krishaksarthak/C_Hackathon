// File   : control.c
// Purpose: Per-cycle sensor checks - temperature, gear, overspeed.

// Checks fault severity and ecu response

// Fault                  Severity   ECU Response
// --------------------  ---------  ------------------------------------------
// CRITICAL OVERHEAT      CRITICAL   Force MODE_FAULT this cycle -> STATE_SAFE
// INVALID_GEAR           CRITICAL   Force MODE_FAULT this cycle -> STATE_SAFE
// INVALID_MODE           CRITICAL   Handled in mode.c
// OVERSPEED              MAJOR      Stays in current mode goes into DEGRADED 1st cycle,
//                                   STATE_SAFE + lock after two or more cycles
// HIGH TEMPERATURE       MAJOR      Same as OVERSPEED
// Invalid Input          MAJOR      Same as OVERSPEED

// Finalised priority order for log reporting (highest to lowest)
//   P1  Critical Overheat  (CRITICAL)
//   P2  Invalid Mode / Gear (CRITICAL)
//   P3  Overspeed           (MAJOR)
//   P4  High Temperature    (MAJOR)

#include "control.h"
#include "fault.h"
#include "log.h"
#include <stdio.h>

// When entered into fault mode system should go into safe
static void force_fault_mode(VehicleStatus *status)
{
    if (status->current_mode != MODE_FAULT)
    {
        log_mode_transition(status->current_mode, MODE_FAULT, 1);
        status->previous_mode = status->current_mode;
        status->current_mode  = MODE_FAULT;
    }
}

// For temp checks critical and high temp.
static void check_temperature(const VehicleInput *input, VehicleStatus *status, FaultStatus *faults)
{
    if (input->temperature > (int16_t)TEMP_CRITICAL_THRESHOLD)
    {
        set_fault(faults, FAULT_BIT_OVERTEMP);
        clear_fault(faults, FAULT_BIT_HIGH_TEMP);
        log_fault_event(FAULT_BIT_OVERTEMP, "CRITICAL OVERHEAT detected — forcing FAULT mode");
        force_fault_mode(status);
    }
    else if (input->temperature > (int16_t)TEMP_HIGH_THRESHOLD)
    {
        set_fault(faults, FAULT_BIT_HIGH_TEMP);
        clear_fault(faults, FAULT_BIT_OVERTEMP);
        log_fault_event(FAULT_BIT_HIGH_TEMP, "HIGH TEMPERATURE — major fault, mode preserved");
    }
    else
    {
        clear_fault(faults, FAULT_BIT_OVERTEMP);
        clear_fault(faults, FAULT_BIT_HIGH_TEMP);
    }
}

// Gear check - range is 0-5 and if invalid it is a critical fault
static void check_gear(const VehicleInput *input, VehicleStatus *status, FaultStatus *faults)
{
    if (input->gear_valid == 0U)
    {
        set_fault(faults, FAULT_BIT_INVALID_GEAR);
        log_fault_event(FAULT_BIT_INVALID_GEAR, "Gear out of range [0-5] — critical fault, forcing FAULT mode");
        force_fault_mode(status);
    }
    else
    {
        clear_fault(faults, FAULT_BIT_INVALID_GEAR);
    }
}

// Overspeedcheck if speed is over 120mph it goes into degraded mode
static void check_overspeed(const VehicleInput *input, FaultStatus *faults)
{
    char detail[72]; // Used this according to MISRA Rule 18.8

    if (input->speed > (int16_t)OVERSPEED_THRESHOLD)
    {
        set_fault(faults, FAULT_BIT_OVERSPEED);
        (void)snprintf(detail, sizeof(detail), "Speed %d mph exceeds %d mph limit [MAJOR — mode preserved]", (int)input->speed, OVERSPEED_THRESHOLD);
        log_fault_event(FAULT_BIT_OVERSPEED, detail);
    }
    else
    {
        clear_fault(faults, FAULT_BIT_OVERSPEED);
    }
}

// This is the entry point for all control checks
void run_control_checks(const VehicleInput *input, VehicleStatus *status, FaultStatus *faults)
{
    check_temperature(input, status, faults);
    check_gear(input, status, faults);
    check_overspeed(input, faults);
}