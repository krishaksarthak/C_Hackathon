#ifndef CONTROL_H
#define CONTROL_H

#include "types.h"
#include "fault.h"

/*
 * run_control_logic
 * -----------------
 * Executes ECU control checks in a deterministic order.
 *
 * Checks performed:
 * 1. Critical overtemperature
 * 2. High temperature
 * 3. Invalid gear
 * 4. Overspeed
 *
 * Detected issues are reported via fault flags and counters.
 */
void run_control_logic(const VehicleInput *input,
                       FaultStatus *faults);

#endif /* CONTROL_H */