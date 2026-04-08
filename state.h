#ifndef STATE_H
#define STATE_H

#include "types.h"
#include "fault.h"

/*
 * evaluate_system_state
 * ---------------------
 * Determines the overall ECU system state based on
 * current and persistent fault conditions.
 *
 * States:
 *  - NORMAL
 *  - DEGRADED
 *  - SAFE
 */
void evaluate_system_state(VehicleStatus *status,
                           const FaultStatus *faults);

#endif /* STATE_H */