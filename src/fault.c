/*
 * File   : fault.c
 * Purpose: Fault flag management, counter tracking, persistence promotion,
 *          and severity classification.
 *
 * ── consecutive[] counter — Option A vs Option B ────────────────────────
 *
 * The consecutive[] array tracks how many times each fault has fired without
 * a manual ECU reset.  Two behaviours are available; only one is active:
 *
 * OPTION A — Immediate Reset (commented out by default)
 *   clear_fault() resets consecutive[bit] to 0.
 *   The fault must re-arm from zero after clearing.
 *   Use this if you want "strictly consecutive cycles" semantics:
 *   OVERSPEED for 2 cycles in a row → SAFE; one clear breaks the streak.
 *
 * OPTION B — Latched / Cumulative (default — currently active)
 *   clear_fault() does NOT touch consecutive[bit].
 *   The counter accumulates across the ECU lifetime and only resets on a
 *   manual operator reset (init_system() via memset).
 *   A fault that fires twice with a normal cycle in between still escalates
 *   to STATE_SAFE.  This is the safer choice for a safety-critical ECU.
 *
 * To switch from Option B to Option A:
 *   1. Uncomment the line marked [OPTION A] inside clear_fault().
 *   2. Comment out or delete the [OPTION B] note block.
 *
 * MISRA C:2012 compliance notes
 *   • All loop variables explicitly typed as uint32_t (Rule 10.1).
 *   • Bitwise operations only on unsigned types (Rule 12.2).
 *   • Single exit point per function (Rule 15.5).
 *   • Explicit cast on enum-to-index conversions (Rule 10.5).
 */

#include "fault.h"

/* ── set_fault ──────────────────────────────────────────────────────────── */
void set_fault(FaultStatus *faults, FaultBit bit)
{
    if (bit >= FAULT_BIT_COUNT) { return; }

    faults->active_flags     |= (1U << (uint32_t)bit);
    faults->counters[bit]++;
    faults->consecutive[bit]++;
}

/* ── clear_fault ────────────────────────────────────────────────────────── */
void clear_fault(FaultStatus *faults, FaultBit bit)
{
    if (bit >= FAULT_BIT_COUNT) { return; }

    faults->active_flags     &= ~(1U << (uint32_t)bit);
    faults->persistent_flags &= ~(1U << (uint32_t)bit);

    /* ------------------------------------------------------------------ *
     * [OPTION A] Immediate reset — uncomment to enable:                   *
     *                                                                      *
     * Resets consecutive[bit] to 0 when the fault condition clears.       *
     * The fault must fire for MAJOR_FAULT_SAFE_THRESHOLD unbroken cycles  *
     * to escalate to STATE_SAFE; one clear resets the streak.             *
     * ------------------------------------------------------------------ */
    /* faults->consecutive[bit] = 0U; */

    /* ------------------------------------------------------------------ *
     * [OPTION B] Latched accumulation — DEFAULT (active):                 *
     *                                                                      *
     * consecutive[bit] is intentionally NOT reset here.                   *
     * It accumulates every time the fault fires and only resets in        *
     * init_system() via memset.  This means two non-adjacent fault        *
     * occurrences are enough to reach MAJOR_FAULT_SAFE_THRESHOLD (= 2)   *
     * and lock the ECU in STATE_SAFE — the conservative, safe choice.     *
     * ------------------------------------------------------------------ */
}

/* ── increment_fault_counter ────────────────────────────────────────────── */
void increment_fault_counter(FaultStatus *faults, FaultBit bit)
{
    if (bit >= FAULT_BIT_COUNT) { return; }
    faults->counters[bit]++;
}

/* ── update_fault_status ────────────────────────────────────────────────── */
void update_fault_status(FaultStatus *faults)
{
    uint32_t i;

    for (i = 0U; i < (uint32_t)FAULT_BIT_COUNT; i++)
    {
        /*
         * Promote to persistent once the consecutive (or cumulative, with
         * Option B) counter reaches PERSISTENT_FAULT_LIMIT.
         * This flag is displayed in log summaries and cycle reports.
         */
        if (faults->consecutive[i] >= PERSISTENT_FAULT_LIMIT)
        {
            faults->persistent_flags |= (1U << i);
        }
    }
}

/* ── get_fault_severity ─────────────────────────────────────────────────── */
/*
 * Returns the severity of a given fault bit.
 *
 * CRITICAL faults:
 *   OVERTEMP, INVALID_GEAR, INVALID_MODE
 *   Response: force MODE_FAULT this cycle → STATE_SAFE + ECU lock.
 *
 * MAJOR faults:
 *   OVERSPEED, HIGH_TEMP
 *   Response: stay in current mode → DEGRADED (1st occurrence),
 *             STATE_SAFE + ECU lock after MAJOR_FAULT_SAFE_THRESHOLD cycles.
 */
FaultSeverity get_fault_severity(FaultBit bit)
{
    FaultSeverity severity;

    switch (bit)
    {
        case FAULT_BIT_OVERTEMP:
        case FAULT_BIT_INVALID_GEAR:
        case FAULT_BIT_INVALID_MODE:
            severity = FAULT_SEVERITY_CRITICAL;
            break;

        case FAULT_BIT_OVERSPEED:
        case FAULT_BIT_HIGH_TEMP:
        default:
            severity = FAULT_SEVERITY_MAJOR;
            break;
    }

    return severity;
}

/* ── fault_to_string ────────────────────────────────────────────────────── */
const char *fault_to_string(FaultBit bit)
{
    const char *name;

    switch (bit)
    {
        case FAULT_BIT_OVERSPEED:    name = "OVERSPEED";         break;
        case FAULT_BIT_OVERTEMP:     name = "CRITICAL OVERHEAT"; break;
        case FAULT_BIT_INVALID_GEAR: name = "INVALID_GEAR";      break;
        case FAULT_BIT_INVALID_MODE: name = "INVALID_MODE";      break;
        case FAULT_BIT_HIGH_TEMP:    name = "HIGH TEMPERATURE";  break;
        default:                     name = "UNKNOWN_FAULT";     break;
    }

    return name;
}

/* ── is_fault_active ────────────────────────────────────────────────────── */
int is_fault_active(const FaultStatus *faults, FaultBit bit)
{
    int result = 0;

    if (bit < FAULT_BIT_COUNT)
    {
        result = ((faults->active_flags & (1U << (uint32_t)bit)) != 0U) ? 1 : 0;
    }

    return result;
}

/* ── is_fault_persistent ────────────────────────────────────────────────── */
int is_fault_persistent(const FaultStatus *faults, FaultBit bit)
{
    int result = 0;

    if (bit < FAULT_BIT_COUNT)
    {
        result = ((faults->persistent_flags & (1U << (uint32_t)bit)) != 0U) ? 1 : 0;
    }

    return result;
}

/* ── count_active_faults ────────────────────────────────────────────────── */
int count_active_faults(const FaultStatus *faults)
{
    int      count = 0;
    uint32_t i;

    for (i = 0U; i < (uint32_t)FAULT_BIT_COUNT; i++)
    {
        if ((faults->active_flags & (1U << i)) != 0U)
        {
            count++;
        }
    }

    return count;
}

/* ── count_persistent_faults ────────────────────────────────────────────── */
int count_persistent_faults(const FaultStatus *faults)
{
    int      count = 0;
    uint32_t i;

    for (i = 0U; i < (uint32_t)FAULT_BIT_COUNT; i++)
    {
        if ((faults->persistent_flags & (1U << i)) != 0U)
        {
            count++;
        }
    }

    return count;
}