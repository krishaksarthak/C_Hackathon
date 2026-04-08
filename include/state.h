#ifndef STATE_H
#define STATE_H

#include "types.h"

void evaluate_system_state(VehicleStatus *status, const FaultStatus *faults);
const char *state_to_string(SystemState state);

#endif