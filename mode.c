#include "mode.h"

void update_mode(VehicleStatus *status, VehicleInput *input, FaultStatus *faults) {
    status->previousMode = status->currentMode;

    if (status->currentMode == MODE_OFF && input->requestedMode == MODE_IGNITION_ON) {
        status->currentMode = MODE_FAULT;
        set_fault(faults, FAULT_INVALID_MODE);
        return;
    }

    status->currentMode = input->requestedMode;
}