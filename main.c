#include <stdio.h>
#include "types.h"
#include "input.h"
#include "mode.h"
#include "control.h"
#include "fault.h"
#include "state.h"
#include "log.h"

/*
 * main.c
 * --------
 * Acts as the ECU scheduler.
 * Executes all tasks in a fixed order every cycle.
 * Runs all mandatory hackathon test cases.
 */

int main(void)
{
    VehicleInput input = {0};
    VehicleStatus status = {
        .currentMode = MODE_OFF,
        .previousMode = MODE_OFF,
        .systemState = STATE_NORMAL
    };
    FaultStatus faults = {0};

    printf("\n====== VEHICLE ECU SIMULATION START ======\n");

    /* Run mandatory test cases as ECU cycles */
    for (int cycle = 1; cycle <= 9; cycle++)
    {
        printf("\n------ ECU CYCLE %d ------\n", cycle);

        /* 1. Read Inputs */
        read_inputs(&input, cycle);

        /* 2. Validate Inputs */
        validate_inputs(&input);

        /* 3. Update Vehicle Mode */
        update_mode(&status, &input, &faults);

        /* 4. Run Control Logic */
        run_control_logic(&input, &faults);

        /* 5. Update System State */
        evaluate_system_state(&status, &faults);

        /* 6. Generate Logs */
        log_cycle(cycle, &input, &status, &faults);
    }

    printf("\n====== ECU SIMULATION END ======\n");
    return 0;
}