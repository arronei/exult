# Changing `EXULT_PARTY_MAX` (8 → 13)

Project doc for raising the max party size. `EXULT_PARTY_MAX` is the max number of
**non-Avatar** followers. Buffers are sized `EXULT_PARTY_MAX + 1` to include the Avatar.
So 8 → 13 means party buffers go 9 → 14.

The one-line change ([party.h:34](party.h#L34)) is trivial. The work is everywhere that
**hardcodes 9** instead of `EXULT_PARTY_MAX + 1`, plus two things a bigger number alone
can't fix: **save-file format** and **formation-walk layout data**.

---

## The two hard problems (do these first — they gate the rest)

### 1. Save-game format changes → breaks compatibility
[usecode/ucinternal.cc:3176-3180](usecode/ucinternal.cc#L3176) writes, and
[usecode/ucinternal.cc:3274-3278](usecode/ucinternal.cc#L3274) reads, exactly
`EXULT_PARTY_MAX` party-member slots to/from `USEDAT` (gamedat/usecode.dat).

- Bumping the constant changes the on-disk record length. **Old saves (and original
  U7/SI saves) have 8 slots; reading 13 over-reads into the timer/pos data that follows →
  corrupt load.** New saves become unreadable by stock Exult.
- **Decision needed:** either (a) keep the on-disk loop fixed at 8 for the classic format
  and only allow >8 in a new save version, or (b) add a format/version guard around this
  block and migrate. Do NOT just widen the loop.
- `party_count` itself is `size_t` and read straight from the file with no clamp — this is
  already the buffer-overflow flagged in [AUDIT_TASKS.md](AUDIT_TASKS.md#L48). Clamp
  `set_count()` to `EXULT_PARTY_MAX` regardless of which option you pick.

### 2. Formation-walking table is hand-authored for 8
[party.cc:243](party.cc#L243) `followers[EXULT_PARTY_MAX + 1][2]` is a binary-tree layout
(who walks behind whom) driving the diamond formation. It currently defines real follow
slots only up to member 8; the trailing rows are `{-1,-1}` placeholders.

- Growing the array to 14 rows is mechanical, but members **9-13 will not formation-walk**
  until you design actual follow relationships (which existing member each new one trails,
  left/right slot). Uses [party.cc:259-272](party.cc#L259) `left_offsets`/`right_offsets`
  and recursion in `move_followers` ([party.cc:305](party.cc#L305)).
- This is **game-design data, not a number.** Sketch the formation on paper first. Without
  it the extra members just clump on the Avatar's tile.

---

## Mechanical changes (safe once the above are decided)

### Definition
- [ ] [party.h:34](party.h#L34) — `#define EXULT_PARTY_MAX 8` → `13`.
- The two members already using it are fine:
  [party.h:41](party.h#L41) `party` and [party.h:49](party.h#L49) `valid`.

### Hardcoded `party[9]` / `[10]` buffers → replace magic number with `EXULT_PARTY_MAX + 1`
`get_party(list, 1)` writes Avatar + up to `EXULT_PARTY_MAX` actors. Every caller-supplied
buffer sized `9` (or `10`) overflows at 13. Fix each to `EXULT_PARTY_MAX + 1`:

- [ ] [gamewin.cc:1991-1994](gamewin.cc#L1991) — `get_party()` impl; update the
      `// Room for 9.` comment. Optionally add a `max_count` param (see AUDIT_TASKS.md fix).
- [ ] [gamewin.cc:771](gamewin.cc#L771) `all[9]`, [gamewin.cc:2022](gamewin.cc#L2022),
      [gamewin.cc:2899](gamewin.cc#L2899)
- [ ] [actors.cc:2698](actors.cc#L2698), [actors.cc:4848](actors.cc#L4848)
- [ ] [combat.cc:646](combat.cc#L646)
- [ ] [cheat.cc:347](cheat.cc#L347), [cheat.cc:792](cheat.cc#L792), [cheat.cc:1407](cheat.cc#L1407)
- [ ] [gameclk.cc:216](gameclk.cc#L216), [gameclk.cc:227](gameclk.cc#L227)
- [ ] [gamerend.cc:392](gamerend.cc#L392)
- [ ] [schedule.cc:2623](schedule.cc#L2623)
- [ ] [objs/egg.cc:122](objs/egg.cc#L122)
- [ ] [keyactions.cc:397](keyactions.cc#L397) — note this one is `party[10]`, still fix to `EXULT_PARTY_MAX + 1`
- [ ] [usecode/intrinsics.cc:2391](usecode/intrinsics.cc#L2391)
- [ ] [gumps/misc_buttons.cc:147](gumps/misc_buttons.cc#L147)
- [ ] [gumps/ShortcutBar_gump.cc:56](gumps/ShortcutBar_gump.cc#L56)

Grep to confirm none missed: `Actor\s*\*\s*party\s*\[` and `all\[9\]`.

### Usecode save loop
- [ ] [usecode/ucinternal.cc:3178](usecode/ucinternal.cc#L3178) and
      [usecode/ucinternal.cc:3276](usecode/ucinternal.cc#L3276) — see hard problem #1.
      These loops bound the save format; only widen them under the new format decision.

---

## UI / art — layouts assume ≤ 8 (visual work, not just buffers)

### CombatStats gump
- [ ] [gumps/CombatStats_gump.h:46](gumps/CombatStats_gump.h#L46) `party[9]` → `EXULT_PARTY_MAX + 1`.
- [ ] [gumps/CombatStats_gump.cc:49](gumps/CombatStats_gump.cc#L49) picks background shape
      `gumps/cstats/1 + party_size - 1` — **there is one art shape per party size.** Sizes
      >8 need new gump art in the data files, or a fallback.
- [ ] Columns are laid out `colx + i * coldx` ([:54](gumps/CombatStats_gump.cc#L54),
      [:81](gumps/CombatStats_gump.cc#L81)). 13 columns likely overflow the gump width —
      needs layout rework.

### Face_stats
- [ ] [gumps/Face_stats.h:35](gumps/Face_stats.h#L35) `Portrait_button* party[8]` is
      **already undersized** (Avatar + up to `party_size` written to `party[i+1]`). Size it
      to `EXULT_PARTY_MAX + 1`.
- [ ] [gumps/Face_stats.cc:512-523](gumps/Face_stats.cc#L512) vertical mode splits the list
      "first 4 left / rest right" at `i == 3` and centers on `PORTRAIT_HEIGHT * 4`. Hardcoded
      for 8; rebalance the split and check portraits fit the screen height at 13.

---

## Probably fine — verify, don't assume
- `dead_party` is a separate `std::array<int,16>` ([party.h:45](party.h#L45)); not tied to
  `EXULT_PARTY_MAX`. Leave unless dead members can now exceed 16.
- Usecode intrinsic `get_party()` ([usecode/ucinternal.cc:866](usecode/ucinternal.cc#L866))
  builds a dynamically sized array — code is fine. But the **compiled game usecode scripts**
  (party menus, combat, `for` loops over the party) may assume max 8; that's data outside
  this repo and can't be fixed here.

## Reality check
The original U7/SI story provides a fixed cast of joinable NPCs (≤ 8). Beyond 8 there are no
stock NPCs, art, or usecode to fill the slots — so this only pays off for **mods / Keyring /
custom games** that supply their own. Confirm the target use case before doing the UI/art work.

---

## Suggested order
1. Decide save-format strategy (#1) — everything else is throwaway if saves break.
2. Design the formation table (#2).
3. Do the mechanical buffer sweep + clamp `set_count()`.
4. UI/art last, once the engine handles 13 without crashing.
5. Test: recruit >8, walk (formation), save/reload, open Combat + Face stats gumps.
