#ifndef MODE_H
#define MODE_H

#include "types.h"

int is_mode_transition_legal(VehicleMode current, VehicleMode requested);
void update_mode(VehicleStatus *status, const VehicleInput *input, FaultStatus *faults);
const char *mode_to_string(VehicleMode mode);

#endif