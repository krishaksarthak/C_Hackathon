#include "control.h"
#include "fault.h"
#include "log.h"
#include <stdio.h>

/* =========================================================
   Priority order (highest to lowest):
     1. Critical Overheat  (FAULT_BIT_OVERTEMP   when temp > 110)
     2. Invalid Gear       (FAULT_BIT_INVALID_GEAR)
     3. Overspeed          (FAULT_BIT_OVERSPEED)
     4. High Temperature   (FAULT_BIT_OVERTEMP   when 95 < temp <= 110)

   All faults that apply in a cycle are SET; the priority order
   only governs which one is printed first / treated as primary
   in multi-fault scenarios.
   ========================================================= */

static void check_temperature(const VehicleInput *input, FaultStatus *faults)
{
    if (input->temperature > TEMP_CRITICAL_THRESHOLD)
    {
        set_fault(faults, FAULT_BIT_OVERTEMP);
        log_fault_event(FAULT_BIT_OVERTEMP, "CRITICAL OVERHEAT");
    }
    else if (input->temperature > TEMP_HIGH_THRESHOLD)
    {
        set_fault(faults, FAULT_BIT_OVERTEMP);
        log_fault_event(FAULT_BIT_OVERTEMP, "HIGH TEMPERATURE WARNING");
    }
    else
    {
        clear_fault(faults, FAULT_BIT_OVERTEMP);
    }
}

static void check_gear(const VehicleInput *input, FaultStatus *faults)
{
    if (!input->gear_valid)
    {
        set_fault(faults, FAULT_BIT_INVALID_GEAR);
        char detail[64];
        snprintf(detail, sizeof(detail), "Gear value out of range");
        log_fault_event(FAULT_BIT_INVALID_GEAR, detail);
    }
    else
    {
        clear_fault(faults, FAULT_BIT_INVALID_GEAR);
    }
}

static void check_overspeed(const VehicleInput *input, FaultStatus *faults)
{
    if (input->speed > OVERSPEED_THRESHOLD)
    {
        set_fault(faults, FAULT_BIT_OVERSPEED);
        char detail[64];
        snprintf(detail, sizeof(detail), "Speed %d km/h exceeds limit %d km/h",
                 input->speed, OVERSPEED_THRESHOLD);
        log_fault_event(FAULT_BIT_OVERSPEED, detail);
    }
    else
    {
        clear_fault(faults, FAULT_BIT_OVERSPEED);
    }
}

void run_control_checks(const VehicleInput *input,
                        VehicleStatus      *status,
                        FaultStatus        *faults)
{
    (void)status;

    check_temperature(input, faults);
    check_gear(input, faults);
    check_overspeed(input, faults);
    /* Priority 4 (High Temperature) is handled inside check_temperature */
}