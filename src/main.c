#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "types.h"
#include "input.h"
#include "mode.h"
#include "control.h"
#include "fault.h"
#include "state.h"
#include "log.h"

FILE *log_file = NULL;

static void init_system(VehicleStatus *status, FaultStatus *faults) {
    memset(status, 0, sizeof(VehicleStatus));
    memset(faults, 0, sizeof(FaultStatus));

    status->current_mode     = MODE_OFF;
    status->previous_mode    = MODE_OFF;
    status->system_state     = STATE_NORMAL;
    status->previous_state   = STATE_NORMAL;
    status->cycle_count      = 0;

    status->last_valid_speed = 0;
    status->last_valid_temp  = 20;
    status->last_valid_gear  = 0;

    log_info("System initialised - Mode: OFF, State: NORMAL");
}

int main(void) {
    VehicleInput  input  = {0};
    VehicleStatus status = {0};
    FaultStatus   faults = {0};

    log_file = fopen("logs/logs.txt", "w");
    if (log_file == NULL) {
        printf("ERROR: Could not open logs/logs.txt. Please ensure the 'logs' folder exists in the project root.\n");
        return 1;
    }

    init_system(&status, &faults);

    input.speed           = 0;
    input.temperature     = 20;
    input.gear            = 0;
    input.requested_mode  = MODE_OFF;
    input.speed_valid     = 1;
    input.temp_valid      = 1;
    input.gear_valid      = 1;
    input.mode_valid      = 1;

    printf("\nVehicle ECU Simulator\n");
    printf("Logging active: writing to logs/logs.txt\n");
    printf("Enter inputs each cycle: <speed> <temp> <gear> <mode>\n");
    printf("Modes: 0=OFF  1=ACC  2=IGNITION_ON  3=FAULT\n");
    printf("Valid speed: 0-200 | temp: -40 to 150 | gear: 0-5\n");
    printf("Enter -1 -1 -1 -1 to quit\n\n");

    while (1) {
        status.cycle_count++;

        printf("Enter inputs for Cycle #%u: ", status.cycle_count);
        read_inputs(&input);

        if (input.speed == -1 && input.temperature == -1 && input.gear  == -1 && (int)input.requested_mode == -1) {
            log_info("Quit sentinel received - shutting down.");
            break;
        }

        validate_inputs(&input, &status);
        update_mode(&status, &input, &faults);
        run_control_checks(&input, &status, &faults);
        update_fault_status(&faults);
        evaluate_system_state(&status, &faults);
        log_cycle_summary(&input, &status, &faults);

        printf("   -> Cycle #%u complete. Log entry added to logs/logs.txt\n", status.cycle_count);
    }

    if (log_file != NULL) {
        fclose(log_file);
    }

    return 0;
}