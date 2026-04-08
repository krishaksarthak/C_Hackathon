#include "mode.h"
#include "fault.h"
#include "log.h"

static const int transition_table[MODE_COUNT][MODE_COUNT] = {
    /*             OFF  ACC  IGN  FAULT */
    /* OFF   */  {  1,   1,   0,   1  },
    /* ACC   */  {  1,   1,   1,   1  },
    /* IGN   */  {  0,   1,   1,   1  },
    /* FAULT */  {  1,   0,   0,   1  },
};

int is_mode_transition_legal(VehicleMode current, VehicleMode requested){
    if (current  >= MODE_COUNT) return 0;
    if (requested >= MODE_COUNT) return 0;
    return transition_table[current][requested];
}

void update_mode(VehicleStatus *status, const VehicleInput *input, FaultStatus *faults){
    VehicleMode requested = input->requested_mode;
    VehicleMode current   = status->current_mode;

    if (!input->mode_valid){
        set_fault(faults, FAULT_BIT_INVALID_MODE);
        log_mode_transition(current, MODE_FAULT, 1);
        status->previous_mode = current;
        status->current_mode  = MODE_FAULT;
        return;
    }

    if (requested == current){
        return;
    }

    switch (current){
        case MODE_OFF:
            if (!is_mode_transition_legal(MODE_OFF, requested)){
                set_fault(faults, FAULT_BIT_INVALID_MODE);
                log_mode_transition(current, requested, 1);
                status->previous_mode = current;
                status->current_mode  = MODE_FAULT;
                return;
            }
            break;

        case MODE_ACC:
            if (!is_mode_transition_legal(MODE_ACC, requested)){
                set_fault(faults, FAULT_BIT_INVALID_MODE);
                log_mode_transition(current, requested, 1);
                status->previous_mode = current;
                status->current_mode  = MODE_FAULT;
                return;
            }
            break;

        case MODE_IGNITION_ON:
            if (!is_mode_transition_legal(MODE_IGNITION_ON, requested)){
                set_fault(faults, FAULT_BIT_INVALID_MODE);
                log_mode_transition(current, requested, 1);
                status->previous_mode = current;
                status->current_mode  = MODE_FAULT;
                return;
            }
            break;

        case MODE_FAULT:
            if (requested != MODE_OFF){
                set_fault(faults, FAULT_BIT_INVALID_MODE);
                log_mode_transition(current, requested, 1);
                return;
            }
            clear_fault(faults, FAULT_BIT_INVALID_MODE);
            break;

        default:
            set_fault(faults, FAULT_BIT_INVALID_MODE);
            status->previous_mode = current;
            status->current_mode  = MODE_FAULT;
            return;
    }

    log_mode_transition(current, requested, 0);
    status->previous_mode = current;
    status->current_mode  = requested;
}

const char *mode_to_string(VehicleMode mode){
    switch (mode){
        case MODE_OFF:          return "OFF";
        case MODE_ACC:          return "ACC";
        case MODE_IGNITION_ON:  return "IGNITION_ON";
        case MODE_FAULT:        return "FAULT";
        default:                return "UNKNOWN";
    }
}