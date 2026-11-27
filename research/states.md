# Character states

The character's states are a 192-bit set (3 × `uint64_t`: states 0–63, 64–127, 128–191) at offset `+0x53E880` of the
character component record. State changes go through `sub_6A6F220` (RVA `0xA3F220`), which the logging hook used to
produce the observations below.

"Observed" means seen in-game with the state-change log. "Code" means read from the game's code in IDA.

## Known states

| State  | Meaning                             | Source          | Notes                                                                                                                                              |
|--------|-------------------------------------|-----------------|----------------------------------------------------------------------------------------------------------------------------------------------------|
| 1      | can act? (alive / controllable)     | code            | required by the "can act?" gate (`sub_6AABD10`); removing it cancels a reload                                                                      |
| 20     | stationary reloading                | observed + code | set by "request reload" for stationary reloads only, cleared when the reload ends                                                                  |
| 22     | jump pack boost                     | observed        | on at jump pack start, off ~0.5 s later                                                                                                            |
| 48     | first-person view                   | observed        | on while in first person, off in third person                                                                                                      |
| 53, 54 | ?                                   | observed        | on together for ~0.75 s, seen before a reload (weapon switch?)                                                                                     |
| 57     | stance-change pulse                 | observed + code | switches on and off in the same frame on every stance change (crouch, prone, stand, dive, reload start/end). The stationary cancel rule watches it |
| 59     | standing up from prone              | observed        | on for ~0.6 s after leaving prone                                                                                                                  |
| 60     | stationary reload (second flag)     | observed + code | set with 20 by "request reload" for stationary reloads                                                                                             |
| 62     | readied up / loading                | observed        | on from ready-up until loaded in                                                                                                                   |
| 64     | sprinting                           | observed        | on while holding Shift                                                                                                                             |
| 66     | airborne (not grounded)             | observed        | on during both dive and jump pack, off on landing                                                                                                  |
| 69     | ?                                   | code            | the reload button refuses stationary reloads while it's set (the "state 69 check" patch). Never seen in the logs so far                            |
| 75, 79 | moving                              | observed        | on together while moving, off when stopped                                                                                                         |
| 77     | crouched                            | observed + code | a stationary reload crouches you: "request reload" sets 77 if neither 77 nor 78 is set                                                             |
| 78     | prone                               | observed + code | on when diving or going prone                                                                                                                      |
| 82     | aiming down sights                  | observed        |                                                                                                                                                    |
| 97     | jump pack: after thrust, in the air | observed        | on when 98 turns off, off on landing. Probably "airborne after jump pack"                                                                          |
| 98     | jump pack thrusting                 | observed        | on at jump pack start, off when 97 turns on                                                                                                        |
| 100    | ragdolled                           | observed        |                                                                                                                                                    |
| 101    | diving                              | observed        | on for the whole dive (~1.1 s)                                                                                                                     |
| 102    | ?                                   | observed        | switched on and off around a weapon action                                                                                                         |
| 112    | landing recovery                    | observed        | on while stuck in the landing animation                                                                                                            |
| 114    | ?                                   | code            | if set, "request reload" skips the stationary setup (crouch, 20/60)                                                                                |
| 115    | Galactic War map open               | observed        |                                                                                                                                                    |
| 121    | reloading                           | observed + code | set for every reload, stationary or not                                                                                                            |

## Lists in the code

### Stationary reload blocking list (in "can reload?", `sub_67A5580`)

Applies to **stationary reloads only**. If any of these states is set, the reload is refused.

| State | Meaning                            |
|-------|------------------------------------|
| 66    | airborne                           |
| 98    | jump pack thrusting                |
| 101   | diving                             |
| 97    | jump pack: in the air after thrust |

Built at RVA `0x775800` (66), `0x775812` (98), `0x775820` (101), `0x775829` (97). Tested at RVA `0x7758B2`.

Each entry is a `mov` of a 4-byte state number onto the stack, so the list is rebuilt from these instructions on
every call:

| RVA        | Bytes                                 | State | Value at |
|------------|---------------------------------------|-------|----------|
| `0x775800` | `48 C7 44 24 68 42 00 00 00`          | 66    | +5       |
| `0x775812` | `48 C7 44 24 70 62 00 00 00`          | 98    | +5       |
| `0x775820` | `48 C7 44 24 78 65 00 00 00`          | 101   | +5       |
| `0x775829` | `48 C7 84 24 80 00 00 00 61 00 00 00` | 97    | +8       |

The same function also refuses stationary reloads when byte `+0x0F` of the movement record is set: the `jnz` at RVA
`0x7758BC`.

### "Can act?" gate (`sub_6AABD10`)

Applies to **all reloads**. State 1 must be set and none of these may be set. Otherwise the reload press is
buffered for ~1.2 s and retried.

29, 38, 70, 83, 90, 100 (ragdolled), 102, 105, 107, 108, 109, 111, 112 (landing recovery), 113, 122, 124, 128, 132, 134

### Generic reload interrupts (`sub_6AABEA0`)

Applies to **all reloads**. If any of these is *added* during a reload, the reload is cancelled.

29, 32, 38, 70, 83, 84, 90, 93, 100 (ragdolled), 102, 105, 107, 108, 109, 111, 112 (landing recovery), 128, 132, 134

Removing state 1 or 121 also cancels.

### Stationary cancel rule (rule table at RVA `0x22510E0`)

Applies to **stationary reloads only**. While 20 is set, adding 57 cancels the reload. Patched out by setting the
table's key (20) to 0: `sub_6AABEA0` skips a rule whose key is 0.

## Functions

Game build 25480438. IDA's names are for a database based at `0x6030000`, so RVA = address − `0x6030000`.

| Function      | RVA        | Role                                                                                     |
|---------------|------------|------------------------------------------------------------------------------------------|
| `sub_67A5580` | `0x775580` | can reload? (holds the stationary blocking list and the movement flag check)             |
| `sub_67A4B60` | `0x774B60` | start reload                                                                             |
| `sub_6AABFE0` | `0xA7BFE0` | reload button handler (holds the state 69 check, at `0xA7C364`)                          |
| `sub_6AAC870` | `0xA7C870` | request reload: sets 121, and for stationary reloads 20, 60 and the crouch               |
| `sub_6AACCC0` | `0xA7CCC0` | end / cancel reload                                                                      |
| `sub_6AABD10` | `0xA7BD10` | "can act?" gate                                                                          |
| `sub_6AABBD0` | `0xA7BBD0` | is reloading? (state 121)                                                                |
| `sub_6AABEA0` | `0xA7BEA0` | reload interrupt decision (generic interrupts and the rule table)                        |
| `sub_6A6F220` | `0xA3F220` | states changed: called with the changed bits and whether they were added or removed      |

## What the mod changes

| Change                | RVA         | Original → replacement                                  | When                              |
|-----------------------|-------------|---------------------------------------------------------|-----------------------------------|
| blocking list         | see above   | each slot's state number                                | something is blocked (see below)  |
| blocking states check | `0x7758B2`  | `0F 85 65 FD FF FF` → `66 0F 1F 44 00 00` (`jnz` → NOP) | nothing is blocked                |
| movement flag check   | `0x7758BC`  | `0F 85 5B FD FF FF` → `66 0F 1F 44 00 00` (`jnz` → NOP) | `allow_air`                       |
| state 69 check        | `0xA7C364`  | `44 38 68 01 75 60` → `44 38 68 01 EB 60` (`jnz` → `jmp`) | `allow_air` or `allow_dive`     |
| cancel rule           | `0x22510E0` | `14 00 00 00` → `00 00 00 00` (data)                    | `allow_air` or `allow_dive`       |

The blocked states come from the settings: 101 without `allow_dive`; 98 and 97 without `allow_air`; 66 as well when
both are off, which leaves the game's own list. The four slots get the list in order, and a list shorter than four
repeats its first state in the rest.

Not yet confirmed: whether the movement flag check and the state 69 check need patching at all. State 69 has never
been seen in the logs.
