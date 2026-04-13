#include "control.h"
#include "fault.h"
#include "log.h"
#include <stdio.h>

/* =========================================================
 * Multi-fault Priority Order (highest → lowest):
 *   1. Critical Overheat  (temp > TEMP_CRITICAL_THRESHOLD)
 *   2. Invalid Mode / Gear
 *   3. Overspeed          (major fault, not critical)
 *   4. High Temperature   (temp > TEMP_HIGH_THRESHOLD)
 *
 * ALL faults that apply in a cycle are SET and logged.
 * The priority order determines which is marked [PRIMARY FAULT]
 * when more than one fault fires in the same cycle.
 * ========================================================= */

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
        snprintf(detail, sizeof(detail),
                 "Speed %d km/h exceeds limit %d km/h [MAJOR]",
                 input->speed, OVERSPEED_THRESHOLD);
        log_fault_event(FAULT_BIT_OVERSPEED, detail);
    }
    else
    {
        clear_fault(faults, FAULT_BIT_OVERSPEED);
    }
}


void run_control_checks(const VehicleInput *input, VehicleStatus *status,
                        FaultStatus *faults)
{
    (void)status;

    /*
     * Run every check so that all faults are SET this cycle.
     * Individual helpers call log_fault_event() for their own detail lines.
     * After all checks, log_faults_prioritised() (called from log_cycle_summary
     * via main) will emit the ordered fault list + [PRIMARY FAULT] marker.
     *
     * Priority 4 (High Temperature) is handled inside check_temperature.
     */
    check_temperature(input, faults);   /* covers P1 and P4 */
    check_gear(input, faults);          /* P2 (gear half)   */
    check_overspeed(input, faults);     /* P3               */
    /* P2 (mode half) is handled in mode.c / update_mode()  */
}