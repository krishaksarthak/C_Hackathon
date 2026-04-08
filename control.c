#include "control.h"

/*
 * Control Logic Priority (highest to lowest):
 *
 * 1. Critical Overheat   (temperature > 110)
 * 2. Invalid Gear        (gear < 0 or gear > 5)
 * 3. Overspeed           (speed > 120)
 * 4. High Temperature    (temperature > 95)
 *
 * This priority ensures safety‑critical faults dominate logs and state behavior.
 */

void run_control_logic(const VehicleInput *input,
                       FaultStatus *faults)
{
    /* Defensive checks */
    if (input == 0 || faults == 0)
    {
        return;
    }

    /* ---- Critical Overtemperature ---- */
    if (input->temperature > 110)
    {
        set_fault(faults, FAULT_OVERTEMP);
        return; /* Highest priority – no further checks this cycle */
    }

    /* ---- Invalid Gear ---- */
    if (input->gear < 0 || input->gear > 5)
    {
        set_fault(faults, FAULT_INVALID_GEAR);
    }

    /* ---- Overspeed ---- */
    if (input->speed > 120)
    {
        set_fault(faults, FAULT_OVERSPEED);
    }

    /* ---- High Temperature (Warning‑Level) ---- */
    if (input->temperature > 95)
    {
        set_fault(faults, FAULT_OVERTEMP);
    }
}