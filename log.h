#ifndef LOG_H
#define LOG_H

#include "types.h"
#include "fault.h"

/*
 * log_cycle
 * ---------
 * Prints a structured summary of one ECU cycle.
 *
 * Includes:
 *  - Input values
 *  - Mode transition
 *  - Active faults
 *  - System state
 */
void log_cycle(int cycle,
               const VehicleInput *input,
               const VehicleStatus *status,
               const FaultStatus *faults);

#endif /* LOG_H */