#include <stdio.h>
#include "input.h"
#include "log.h"

static int read_int(const char *prompt, int *out)
{
    printf("%s", prompt);
    fflush(stdout);
    return (scanf("%d", out) == 1);
}

void read_inputs(VehicleInput *input)
{
    int mode_raw  = 0;
    int speed_raw = 0;
    int temp_raw  = 0;
    int gear_raw  = 0;

    printf("\n");

    if (!read_int(" Mode (0=OFF 1=ACC 2=IGN 3=FAULT, -1 to reset, -2 to quit): ", &mode_raw)) goto fail;

    if (mode_raw == -1 || mode_raw == -2)
    {
        input->requested_mode = (VehicleMode)mode_raw;
        input->speed          = 0;
        input->temperature    = 0;
        input->gear           = 0;
        return;
    }

    if (!read_int(" Speed (0-200 mph)                            : ", &speed_raw)) goto fail;
    if (!read_int(" Temp  (-40 to 150 F)                         : ", &temp_raw))  goto fail;
    if (!read_int(" Gear  (0-5)                                  : ", &gear_raw))  goto fail;

    if (speed_raw < MIN_SPEED || speed_raw > MAX_SPEED)
    {
        log_warning("read_inputs: speed out of range — rejecting, keeping last valid");
        input->speed_valid = 0;
        /* Do not assign — validate_inputs will substitute last_valid_speed */
    }
    else
    {
        input->speed       = (int16_t)speed_raw;   /* safe: 0-200 fits int16_t */
        input->speed_valid = 1;
    }

    if (temp_raw < MIN_TEMP || temp_raw > MAX_TEMP)
    {
        log_warning("read_inputs: temperature out of range — rejecting, keeping last valid");
        input->temp_valid = 0;
    }
    else
    {
        input->temperature = (int16_t)temp_raw;    /* safe: -40..150 fits int16_t */
        input->temp_valid  = 1;
    }

    if (gear_raw < MIN_GEAR || gear_raw > MAX_GEAR)
    {
        log_warning("read_inputs: gear out of range — rejecting, keeping last valid");
        input->gear_valid = 0;
    }
    else
    {
        input->gear       = (int8_t)gear_raw;      /* safe: 0-5 fits int8_t */
        input->gear_valid = 1;
    }

    input->requested_mode = (VehicleMode)mode_raw;
    return;

fail:
    log_warning("read_inputs: failed to read field, preserving previous values");
}

void validate_inputs(VehicleInput *input, VehicleStatus *status)
{
    if (input->speed >= MIN_SPEED && input->speed <= MAX_SPEED) {
        input->speed_valid = 1;
        status->last_valid_speed = input->speed;
    } else {
        input->speed_valid = 0;
        input->speed = status->last_valid_speed;
        log_input_warning("Speed", input->speed);
    }

    if (input->temperature >= MIN_TEMP && input->temperature <= MAX_TEMP) {
        input->temp_valid = 1;
        status->last_valid_temp = input->temperature;
    } else {
        input->temp_valid = 0;
        input->temperature = status->last_valid_temp;
        log_input_warning("Temperature", input->temperature);
    }

    if (input->gear >= MIN_GEAR && input->gear <= MAX_GEAR) {
        input->gear_valid = 1;
        status->last_valid_gear = input->gear;
    } else {
        input->gear_valid = 0;
        // This is given specifically in the documents
    }

    if (input->requested_mode >= MODE_OFF && input->requested_mode < MODE_COUNT) {
        input->mode_valid = 1;
    } else {
        input->mode_valid = 0;
        input->requested_mode = status->current_mode;
        log_input_warning("Mode (out of enum range)", (int)input->requested_mode);
    }
}