/*
 * File   : mode.c
 * Purpose: Mode transition validation and state updates.
 *
 * ── Transition table rationale ───────────────────────────────────────────
 *
 *   Key decisions:
 *     OFF → IGN         illegal — must pass through ACC first (sequential start)
 *     IGN → ACC         legal   — controlled downgrade (engine off, accessories on)
 *     IGN → OFF         illegal — must step down through ACC (no jump-powerdown)
 *     FAULT → non-OFF   illegal — only OFF clears FAULT mode (manual reset path)
 *     Any → FAULT       legal   — ECU or control layer may force it directly
 *
 *   All illegal transitions set FAULT_BIT_INVALID_MODE (CRITICAL severity)
 *   and force current_mode to MODE_FAULT.  evaluate_system_state() will
 *   then escalate to STATE_SAFE and set reset_required on the same cycle.
 *
 *   Note: With the new reset architecture, the FAULT → OFF transition is
 *   effectively a dead path during normal operation — the ECU reaches SAFE
 *   state and only init_system() (via -1 input) clears it.  The transition
 *   table entry is kept for completeness and for any future explicit test.
 *
 * MISRA C:2012 compliance notes
 *   • Transition table uses int literals, not enum, to keep 2D array
 *     initialisation simple and verifiable (Rule 9.3).
 *   • All enum comparisons use explicit bounds check (Rule 10.5).
 *   • Single exit point per function (Rule 15.5).
 *   • No implicit fallthrough in switch (Rule 16.3).
 */

#include "mode.h"
#include "fault.h"
#include "log.h"

/* ── Legal transition table ─────────────────────────────────────────────── */
/*
 * transition_table[current][requested]
 *   1 = legal transition
 *   0 = illegal transition (triggers FAULT mode + FAULT_BIT_INVALID_MODE)
 *
 *             OFF  ACC  IGN  FAULT
 */
static const int8_t transition_table[MODE_COUNT][MODE_COUNT] =
{
/*  OFF   */ { 1,   1,   0,   1 },
/*  ACC   */ { 1,   1,   1,   1 },
/*  IGN   */ { 0,   1,   1,   1 },   /* IGN→ACC legal: controlled downgrade */
/*  FAULT */ { 1,   0,   0,   1 },   /* only OFF can exit FAULT mode         */
};

/* ── is_mode_transition_legal ───────────────────────────────────────────── */
int is_mode_transition_legal(VehicleMode current, VehicleMode requested)
{
    int legal = 0;

    if ((current < MODE_COUNT) && (requested < MODE_COUNT))
    {
        legal = (int)transition_table[(int)current][(int)requested];
    }

    return legal;
}

/* ── update_mode ────────────────────────────────────────────────────────── */
void update_mode(VehicleStatus *status, const VehicleInput *input,
                 FaultStatus *faults)
{
    VehicleMode requested = input->requested_mode;
    VehicleMode current   = status->current_mode;

    /* Case 1: mode_valid == 0 means the raw mode value was out of enum range */
    if (input->mode_valid == 0U)
    {
        set_fault(faults, FAULT_BIT_INVALID_MODE);
        log_mode_transition(current, MODE_FAULT, 1);
        status->previous_mode = current;
        status->current_mode  = MODE_FAULT;
        return;
    }

    /* Case 2: no change requested */
    if (requested == current)
    {
        return;
    }

    /* Case 3: currently in FAULT — only OFF is legal */
    if (current == MODE_FAULT)
    {
        if (requested != MODE_OFF)
        {
            /* Attempted to leave FAULT mode to something other than OFF */
            set_fault(faults, FAULT_BIT_INVALID_MODE);
            log_mode_transition(current, requested, 1);
            /* Mode stays FAULT — no update to status */
        }
        else
        {
            /* FAULT → OFF: legal manual-reset path (typically reached via
             * init_system in this architecture, but kept for table completeness) */
            clear_fault(faults, FAULT_BIT_INVALID_MODE);
            log_mode_transition(current, requested, 0);
            status->previous_mode = current;
            status->current_mode  = requested;
        }
        return;
    }

    /* Case 4: normal transition — check table */
    if (is_mode_transition_legal(current, requested) == 0)
    {
        /* Illegal transition: OFF→IGN, IGN→OFF, etc. — CRITICAL fault */
        set_fault(faults, FAULT_BIT_INVALID_MODE);
        log_mode_transition(current, requested, 1);
        status->previous_mode = current;
        status->current_mode  = MODE_FAULT;
        return;
    }

    /* Case 5: legal transition */
    log_mode_transition(current, requested, 0);
    status->previous_mode = current;
    status->current_mode  = requested;
}

/* ── mode_to_string ─────────────────────────────────────────────────────── */
const char *mode_to_string(VehicleMode mode)
{
    const char *name;

    switch (mode)
    {
        case MODE_OFF:         name = "OFF";         break;
        case MODE_ACC:         name = "ACC";         break;
        case MODE_IGNITION_ON: name = "IGNITION_ON"; break;
        case MODE_FAULT:       name = "FAULT";       break;
        default:               name = "UNKNOWN";     break;
    }

    return name;
}