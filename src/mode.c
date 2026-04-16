
//  File   : mode.c
//  Purpose: Mode transition validation and state updates.

//   Key decisions:
//     OFF -> IGN         illegal - must pass through ACC first (sequential start)
//     IGN -> ACC         legal   - controlled downgrade (engine off, accessories on)
//     IGN -> OFF         illegal - must step down through ACC (no jump-powerdown)
//     FAULT -> non-OFF   illegal - only OFF clears FAULT mode (manual reset path)
//     Any -> FAULT       legal   - ECU or control layer may force it directly

#include "mode.h"
#include "fault.h"
#include "log.h"


// For transition_table[current][requested]
//   1 = legal transition
//   0 = illegal transition (triggers FAULT mode + FAULT_BIT_INVALID_MODE)

static const int8_t transition_table[MODE_COUNT][MODE_COUNT] =
{
/*  OFF   */ { 1,   1,   0,   1 },
/*  ACC   */ { 1,   1,   1,   1 },
/*  IGN   */ { 0,   1,   1,   1 },
/*  FAULT */ { 1,   0,   0,   1 },
};

int is_mode_transition_legal(VehicleMode current, VehicleMode requested)
{
    int legal = 0;

    if ((current < MODE_COUNT) && (requested < MODE_COUNT))
    {
        legal = (int)transition_table[(int)current][(int)requested];
    }

    return legal;
}

void update_mode(VehicleStatus *status, const VehicleInput *input,
                 FaultStatus *faults)
{
    VehicleMode requested = input->requested_mode;
    VehicleMode current   = status->current_mode;

    if (input->mode_valid == 0U)
    {
        set_fault(faults, FAULT_BIT_INVALID_MODE);
        log_mode_transition(current, MODE_FAULT, 1);
        status->previous_mode = current;
        status->current_mode  = MODE_FAULT;
        return;
    }

    if (requested == current)
    {
        return;
    }

    if (current == MODE_FAULT)
    {
        if (requested != MODE_OFF)
        {
            set_fault(faults, FAULT_BIT_INVALID_MODE);
            log_mode_transition(current, requested, 1);
        }
        else
        {
            clear_fault(faults, FAULT_BIT_INVALID_MODE);
            log_mode_transition(current, requested, 0);
            status->previous_mode = current;
            status->current_mode  = requested;
        }
        return;
    }

    if (is_mode_transition_legal(current, requested) == 0)
    {
        set_fault(faults, FAULT_BIT_INVALID_MODE);
        log_mode_transition(current, requested, 1);
        status->previous_mode = current;
        status->current_mode  = MODE_FAULT;
        return;
    }

    log_mode_transition(current, requested, 0);
    status->previous_mode = current;
    status->current_mode  = requested;
}

const char *mode_to_string(VehicleMode mode)
{
    const char *name;

    switch (mode)
    {
        case MODE_OFF: name = "OFF"; break;
        case MODE_ACC: name = "ACC"; break;
        case MODE_IGNITION_ON: name = "IGNITION_ON"; break;
        case MODE_FAULT: name = "FAULT"; break;
        default: name = "UNKNOWN"; break;
    }

    return name;
}