#ifndef MODE_H
#define MODE_H

#include "types.h"
#include "fault.h"

/*
 * update_mode
 * -----------
 * Updates the vehicle mode based on the requested mode.
 * Detects illegal transitions and forces FAULT mode if needed.
 *
 * Inputs:
 *  - VehicleStatus: holds current and previous mode
 *  - VehicleInput:  contains requestedMode
 *  - FaultStatus:   updated if illegal transition occurs
 */
void update_mode(VehicleStatus *status,
                 VehicleInput *input,
                 FaultStatus *faults);

#endif /* MODE_H */