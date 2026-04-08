#ifndef CONTROL_H
#define CONTROL_H

#include "types.h"

// Control checks
void run_control_checks(const VehicleInput *input, VehicleStatus *status, FaultStatus *faults);

#endif