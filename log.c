#include <stdio.h>
#include "log.h"

/*
 * Helper functions to convert enums to readable strings
 */

static const char* mode_to_string(VehicleMode mode)
{
    switch (mode)
    {
        case MODE_OFF:          return "OFF";
        case MODE_ACC:          return "ACC";
        case MODE_IGNITION_ON:  return "IGNITION_ON";
        case MODE_FAULT:        return "FAULT";
        default:                return "UNKNOWN";
    }
}

static const char* state_to_string(SystemState state)
{
    switch (state)
    {
        case STATE_NORMAL:    return "NORMAL";
        case STATE_DEGRADED:  return "DEGRADED";
        case STATE_SAFE:      return "SAFE";
        default:              return "UNKNOWN";
    }
}

/*
 * log_cycle
 * ---------
 * Generates ECU-style console logs for one scheduler cycle.
 */

void log_cycle(int cycle,
               const VehicleInput *input,
               const VehicleStatus *status,
               const FaultStatus *faults)
{
    if (input == 0 || status == 0 || faults == 0)
    {
        return; /* Defensive check */
    }

    printf("\n========================================\n");
    printf("ECU CYCLE: %d\n", cycle);
    printf("----------------------------------------\n");

    /* Input summary */
    printf("Inputs:\n");
    printf("  Speed       : %d km/h\n", input->speed);
    printf("  Temperature : %d C\n", input->temperature);
    printf("  Gear        : %d\n", input->gear);
    printf("  Req Mode    : %s\n",
           mode_to_string(input->requestedMode));

    /* Mode transition */
    printf("\nMode:\n");
    printf("  Previous    : %s\n",
           mode_to_string(status->previousMode));
    printf("  Current     : %s\n",
           mode_to_string(status->currentMode));

    /* Fault information */
    printf("\nFault Status:\n");
    if (faults->flags == 0U)
    {
        printf("  No active faults\n");
    }
    else
    {
        if (faults->flags & FAULT_OVERSPEED)
            printf("  - Overspeed (count=%u)\n",
                   faults->overspeedCount);

        if (faults->flags & FAULT_OVERTEMP)
            printf("  - Overtemperature (count=%u)\n",
                   faults->overtempCount);

        if (faults->flags & FAULT_INVALID_GEAR)
            printf("  - Invalid Gear (count=%u)\n",
                   faults->gearCount);

        if (faults->flags & FAULT_INVALID_MODE)
            printf("  - Invalid Mode Transition (count=%u)\n",
                   faults->modeCount);
    }

    /* System state */
    printf("\nSystem State:\n");
    printf("  %s\n", state_to_string(status->systemState));

    printf("========================================\n");
}