#include <stdio.h>
#include "input.h"
#include "log.h"

// Reads one integer from stdin with a prompt.
// Returns 1 on success, 0 on failure.

static int read_int(const char *prompt, int *out)
{
    printf("%s", prompt);
    fflush(stdout);
    return (scanf("%d", out) == 1);
}

// Reads raw values from the user, one field per line.
void read_inputs(VehicleInput *input)
{
    int mode_raw  = 0;
    int speed_raw = 0;
    int temp_raw  = 0;
    int gear_raw  = 0;

    printf("\n");

    if (!read_int("  Mode  (0=OFF 1=ACC 2=IGN 3=FAULT, -1 to quit): ", &mode_raw)) goto fail;

    if (mode_raw == -1)
    {
        input->requested_mode = (VehicleMode)-1;
        input->speed          = -1;
        input->temperature    = -1;
        input->gear           = -1;
        return;
    }

    if (!read_int("  Speed (0-200 km/h)                            : ", &speed_raw)) goto fail;
    if (!read_int("  Temp  (-40 to 150 deg C)                      : ", &temp_raw))  goto fail;
    if (!read_int("  Gear  (0-5)                                   : ", &gear_raw))  goto fail;

    input->requested_mode = (VehicleMode)mode_raw;
    input->speed          = (int16_t)speed_raw;
    input->temperature    = (int16_t)temp_raw;
    input->gear           = (int8_t)gear_raw;
    return;

fail:
    log_warning("read_inputs: failed to read field, preserving previous values");
}

void validate_inputs(VehicleInput *input, VehicleStatus *status)
{
    if (input->speed >= MIN_SPEED && input->speed <= MAX_SPEED) {
        input->speed_valid       = 1;
        status->last_valid_speed = input->speed;
    } else {
        input->speed_valid = 0;
        input->speed       = status->last_valid_speed;
        log_input_warning("Speed", input->speed);
    }

    if (input->temperature >= MIN_TEMP && input->temperature <= MAX_TEMP) {
        input->temp_valid       = 1;
        status->last_valid_temp = input->temperature;
    } else {
        input->temp_valid   = 0;
        input->temperature  = status->last_valid_temp;
        log_input_warning("Temperature", input->temperature);
    }

    if (input->gear >= MIN_GEAR && input->gear <= MAX_GEAR) {
        input->gear_valid       = 1;
        status->last_valid_gear = input->gear;
    } else {
        input->gear_valid = 0;
        // Do NOT restore last good gear — invalid gear must reach control layer so it can raise FAULT_BIT_INVALID_GEAR.
    }

    if (input->requested_mode >= MODE_OFF && input->requested_mode < MODE_COUNT) {
        input->mode_valid = 1;
    } else {
        input->mode_valid         = 0;
        input->requested_mode     = status->current_mode;
        log_input_warning("Mode (out of enum range)", (int)input->requested_mode);
    }
}