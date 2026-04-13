# Vehicle ECU Simulator — Design Note

**Project:** Modular Embedded C Vehicle ECU Simulator  
**Language:** C99  
**Build:** GCC via Makefile  

---

## 1. Scheduler Flow

The system runs as a **cyclic executable** — an infinite loop where every cycle
executes the same eight steps in the same fixed order, without exception.

```
EVERY CYCLE:
┌─────────────────────────────────────────────┐
│  1. read_inputs()           ← get raw data   │
│  2. Quit sentinel check                     │
│  3. validate_inputs()       ← range-check    │
│  4. update_mode()           ← mode FSM       │
│  5. run_control_checks()    ← fault detect   │
│  6. update_fault_status()   ← persist logic  │
│  7. evaluate_system_state() ← NORMAL/DEG/SAFE│
│  8. log_cycle_summary()     ← struct log     │
└─────────────────────────────────────────────┘
```

Note: `log_faults_prioritised()` is called before `log_cycle_summary()` to emit the fault report.

### Why This Order?

| Step | Must Come After | Reason |
|------|----------------|--------|
| Quit sentinel check | read_inputs | Detect early exit before validation |
| validate_inputs | read_inputs | No module should ever see unvalidated data |
| update_mode | validate_inputs | Mode FSM needs validated mode request |
| run_control_checks | update_mode | Control logic runs only when mode is determined |
| update_fault_status | run_control_checks | All faults must be set before persistence is evaluated |
| evaluate_system_state | update_fault_status | Needs final persistent flags to decide SAFE state |
| log_faults_prioritised | evaluate_system_state | Report faults in priority order |
| log_cycle_summary | log_faults_prioritised | Must log the final, fully-updated state |

### What Goes Wrong If the Order Changes?

- **validate after control** → control logic acts on garbage sensor data; ECU may raise false faults or miss real ones
- **state before fault manager** → persistent flags are not yet updated; system can never reach SAFE state correctly
- **log before state eval** → cycle summary prints a stale, incorrect system state
- **mode after control** → control logic cannot react to a newly requested mode this cycle
- **fault manager before control** → fault flags from this cycle are not yet set; persistence tracking is one cycle behind
- **sentinel check after validation** → invalid inputs may pass validation before exit, wasting a cycle

---

## 2. Module Responsibilities

### `types.h` — Shared Type Definitions
The single source of truth for all enums, structs, and macros shared across modules.  
No logic. No function prototypes. Only types.

- `VehicleMode` enum — OFF, ACC, IGNITION_ON, FAULT
- `SystemState` enum — NORMAL, DEGRADED, SAFE
- `FaultBit` enum — bit positions for each fault type
- `VehicleInput` struct — raw sensor values + per-field validity flags
- `VehicleStatus` struct — current mode, state, cycle count, last-known-good values
- `FaultStatus` struct — active flags, persistent flags, counters, consecutive counts
- Threshold macros — MAX_SPEED, TEMP_CRITICAL_THRESHOLD, PERSISTENT_FAULT_LIMIT, etc.

---

### `input.h / input.c` — Input Layer
Responsible for acquiring and validating all sensor data.

**`read_inputs()`**
- Reads speed, temperature, gear, and mode from stdin each cycle
- Does not validate — only reads raw values

**`validate_inputs()`**
- Checks each field against its allowed range (speed: 0–200, temp: -40–150, gear: 0–5, mode: valid enum)
- Sets per-field `_valid` flags (1 = OK, 0 = rejected)
- On invalid input: logs a warning, preserves the last known good value in `VehicleStatus`
- Never crashes — always produces a safe output regardless of input

---

### `mode.h / mode.c` — Mode Controller
Implements a Finite State Machine (FSM) for vehicle operating modes.

**Allowed Transitions:**

```
        ┌──────────────────────────────┐
        ▼                              │
[OFF] ──► [ACC] ──► [IGNITION_ON]      │  (all can go to FAULT)
  ▲         ▲           │              │
  │         └───────────┘              │
  └──────────────── [FAULT] ◄──────────┘
```

- Uses a 2D transition table `transition_table[from][to]` for clean, scalable logic
- Uses `switch-case` on current mode for mode-specific behaviour
- Any illegal transition forces the system into `MODE_FAULT` and sets `FAULT_BIT_INVALID_MODE`
- Recovery from FAULT: only `FAULT → OFF` is allowed (manual reset)

---

### `control.h / control.c` — Control Logic
Evaluates sensor readings and raises or clears fault flags each cycle.

**Priority Order (highest to lowest):**
1. Critical Overheat — temperature > 110°C (`FAULT_BIT_OVERTEMP`)
2. Invalid Mode — illegal mode transition (`FAULT_BIT_INVALID_MODE`)
2. Invalid Gear — gear outside 0–5 (`FAULT_BIT_INVALID_GEAR`)
3. Overspeed — speed > 120 km/h (`FAULT_BIT_OVERSPEED`)
4. High Temperature — temperature > 95°C and ≤ 110°C (`FAULT_BIT_HIGH_TEMP`)

All applicable faults in a cycle are set. Priority governs logging order and
future escalation decisions, not suppression of lower-priority faults.

**Fault Clearing:** If a condition returns to normal (e.g., speed drops below 120),
the corresponding fault flag is **actively cleared** that same cycle. This ensures
`active_flags` always reflects the real-time vehicle condition.

**Temperature Handling:** `FAULT_BIT_OVERTEMP` and `FAULT_BIT_HIGH_TEMP` are mutually exclusive.
When temperature exceeds 110°C, only OVERTEMP is active. When temperature is > 95°C but ≤ 110°C,
only HIGH_TEMP is active. Both are cleared when temperature ≤ 95°C.

---

### `fault.h / fault.c` — Fault Manager
Manages all fault tracking using bitwise flags, counters, and persistence logic.

**Bitmask Design:**
```
Bit 4         Bit 3         Bit 2         Bit 1       Bit 0
HIGH_TEMP  INVALID_MODE  INVALID_GEAR  OVERTEMP  OVERSPEED
```

Fault bits enumerated in `types.h`:
- `FAULT_BIT_OVERSPEED` (Bit 0)
- `FAULT_BIT_OVERTEMP` (Bit 1) — Critical Overheat (temp > 110°C)
- `FAULT_BIT_INVALID_GEAR` (Bit 2)
- `FAULT_BIT_INVALID_MODE` (Bit 3)
- `FAULT_BIT_HIGH_TEMP` (Bit 4) — High Temperature warning (temp > 95°C)

**`set_fault(faults, bit)`** — Sets the active bit, increments total counter and consecutive counter  
**`clear_fault(faults, bit)`** — Clears active bit, resets consecutive counter  
**`update_fault_status(faults)`** — Promotes faults to persistent if consecutive ≥ `PERSISTENT_FAULT_LIMIT` (3)

**Persistence Rule:**
A fault that occurs in 3 or more consecutive cycles is marked persistent in
`persistent_flags`. Persistent faults are cleared when the fault itself clears.
The total lifetime counter (`counters[]`) never resets during a run.

**Extensibility:** Adding a new fault type requires only:
1. A new entry in the `FaultBit` enum
2. A new `#define` macro in `types.h`
3. A `set_fault()` call in the relevant control module

---

### `state.h / state.c` — Safe-State Manager
Determines overall system health based on the fault picture each cycle.

**Transition Rules:**

```
Active faults = 0  AND  persistent = 0  →  NORMAL
Active faults ≥ 1  OR   persistent ≥ 1  →  DEGRADED
Critical persistent faults ≥ 2          →  SAFE
```

*Critical faults: OVERTEMP and OVERSPEED*

**Escalation is deterministic:**
- NORMAL → DEGRADED → SAFE follows the rules above strictly
- SAFE is a **latching state** — the system does not self-recover from SAFE
- Recovery from SAFE requires a manual reset (mode set to OFF, all faults cleared)
- Every state change is logged immediately with its reason

---

### `log.h / log.c` — Structured Logger
Provides all console and file output for the system.

- `log_cycle_summary()` — full boxed table of inputs, mode, state, all fault details
- `log_mode_transition()` — logs legal and illegal mode changes
- `log_state_transition()` — logs state changes with reason string
- `log_fault_event()` — logs individual fault activations with detail
- `log_input_warning()` — logs invalid input fields
- `log_info()` / `log_warning()` — general purpose messages

All output is prefixed with a severity tag: `[INFO ]`, `[WARN ]`, `[FAULT]`, `[STATE]`  
Output goes to both `stdout` (console) and `logs/logs.txt` (file) simultaneously.

---

## 3. State Transition Logic

```
                    ┌─────────────────────────────────────┐
                    │             STATE_NORMAL             │
                    │         (no active faults)           │
                    └──────────────┬──────────────────────┘
                                   │ any fault becomes active
                                   ▼
                    ┌─────────────────────────────────────┐
                    │            STATE_DEGRADED            │
                    │   (≥1 active fault OR ≥1 persistent) │
                    └──────────────┬──────────────────────┘
                                   │ ≥2 critical persistent faults
                                   ▼
                    ┌─────────────────────────────────────┐
                    │             STATE_SAFE               │
                    │  (latching — manual reset required)  │
                    └─────────────────────────────────────┘

  SAFE → NORMAL only possible after: mode reset to OFF + all faults cleared
```

---

## 4. Fault Handling Strategy

### Detection
Faults are detected in `run_control_checks()` every cycle by comparing validated
sensor inputs against defined thresholds. The control layer has no memory —
it re-evaluates from scratch each cycle based on current inputs.

### Recording
`set_fault()` in `fault.c` does three things atomically per fault event:
1. Sets the bit in `active_flags`
2. Increments `counters[bit]` (lifetime total)
3. Increments `consecutive[bit]` (streak counter)

### Persistence
`update_fault_status()` runs after all faults are set/cleared. It scans all fault
bits and promotes any fault whose `consecutive` count has reached
`PERSISTENT_FAULT_LIMIT` (3) into `persistent_flags`.

### Clearing
When a fault condition resolves (sensor value returns to normal range),
`clear_fault()` is called. This clears both the active bit and the consecutive
counter. The lifetime counter is never cleared. The persistent flag is also
cleared when the fault clears — persistence resumes if the fault returns.

### Escalation
The state manager reads `persistent_flags` each cycle. Two or more persistent
critical faults (OVERTEMP, OVERSPEED) escalate the system to `STATE_SAFE`.
The state manager does not clear faults — it only reads them.

### Priority (multi-fault cycles)
When multiple faults trigger in the same cycle, they are all recorded.
Logging order follows the defined priority:
1. Critical Overheat
2. Invalid Gear / Invalid Mode
3. Overspeed
4. High Temperature

---

## 5. Defensive Programming Practices Used

| Practice | Where Applied |
|----------|--------------|
| Input range validation with fallback | `validate_inputs()` in `input.c` |
| Null pointer check on log file | `main.c` after `fopen()` |
| `memset` zero-init of all structs | `init_system()` in `main.c` |
| Enum bounds checking | `mode_transition_table`, `fault.c` bit operations |
| No logic in header files | All `.h` files — prototypes only |
| `static` for internal-only functions | `check_temperature()`, `check_gear()`, etc. |
| Safe string formatting with `snprintf` | `control.c` fault detail strings |
| `fclose()` with NULL guard | `main.c` cleanup block |
| `1U` for all bitmask shifts | `types.h` macro definitions |

---

*End of Design Note*