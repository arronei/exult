# Enchant Spell: Magic Armor (Forge of Virtue / bgkeyring)

Goal: casting Enchant on great helm, gorget, antique armor, curved heater, gauntlets,
plate leggings, or leather boots turns the item into its magic counterpart, the same
way it already turns arrows/bolts into magic arrows/bolts.

**Scope: BG / Forge of Virtue (`content/bgkeyring/`) only.** All edits below land in that
tree. `si_shapes.uc` shows up only as a *read-only reference* for shape numbers that
`bg_shapes2.uc` never declared (curved heater, antique armor aren't vanilla BG items, so
bgkeyring never had a const for them) — nothing under `content/sifixes/` or SI's usecode
is touched or built by this plan.

## Confirmed mechanism: no delete+recreate needed

`set_item_shape()` swaps an object's shape in place — same object, same position,
same equip slot if worn. This is already the established pattern in bgkeyring, used on
both loose and worn/equipped items:

- [spell_functions.uc:122-129](content/bgkeyring/src/spells/spell_functions.uc#L122) —
  `spellEnchantEffect()`, the exact precedent: arrows → magic arrows via
  `set_item_shape(magic_missiles[index])`.
- [items/cloth.uc:83](content/bgkeyring/src/items/cloth.uc#L83),
  [items/torch.uc:43-116](content/bgkeyring/src/items/torch.uc#L43),
  [misc/rest_functions.uc:72-74](content/bgkeyring/src/misc/rest_functions.uc#L72) — same
  intrinsic used elsewhere for item-type changes.

So: **no new shape/frame art, no destroy-and-spawn. Just extend the existing
arrays with armor shape pairs.**

Exception: leather boots — see the boots entry below, it swaps **frame**, not shape,
so it needs a small separate code path (`set_item_frame()`), not another array pair.

`inMagicStorm()` ([bg_externals.uc:106](content/bgkeyring/src/headers/bg/bg_externals.uc#L106))
despite the name is the standard success-gate checked by *every* spell in this reimplementation
(see any of first_circle.uc/third_circle.uc/etc.) — it's not literal weather. Nothing special
needed to test; existing missile-enchant behavior is the baseline.

## Antique armor: decided — reuse SHAPE_MAGIC_ARMOR (666)

Checked every named shape in the game's shape list (`docs/bgitems.txt`, cross-referenced
with `si_shapes.uc`/`bg_shapes2.uc`). No "magic antique armor" shape exists anywhere in
the shape list, so antique armor (836) can't get its own distinct magic look.

**Decision: antique armor enchants into `SHAPE_MAGIC_ARMOR` (666)** — the same target
plate armor already upgrades to. It loses the "antique" look on enchant, but reuses a
shape that's confirmed to exist and already have valid armor data in BG (it's plate
armor's own upgrade target), so no risk of the "shape exists but has no armor value in
BG" problem that antique armor's base shape itself might have.

Convenient bonus: `SHAPE_MAGIC_ARMOR` is already declared at
[bg_shapes2.uc:74](content/bgkeyring/src/headers/bg/bg_shapes2.uc#L74) — nothing new to
add for the *target* side of this pair, only the antique-armor *source* const (see table).

## Leather boots is a frame swap, not a shape swap

Same shape (587, `SHAPE_LEATHER_BOOTS`/`SHAPE_BOOTS`) — the magic version is a different
**frame** of that same shape, not a different shape number. Current lead: **frame 2**,
but unconfirmed whether frame indices are 0-based (so "2nd frame" = index 1) or the index
is genuinely 2 — **verify visually before wiring any code** (Exult Studio's shape browser,
or spawn shape 587 in-game and cycle frames with a cheat, and compare against the base
"leather boots" frame, presumably frame 0).

This means boots can't go through the `normal_missiles`/`magic_missiles` shape-pair arrays
like the other six items — those swap `get_item_shape()`/`set_item_shape()`. Boots needs
its own small branch in both `spellEnchant()` and `spellEnchantEffect()`: check
`target_shape == SHAPE_LEATHER_BOOTS && target_frame == <base frame, likely 0>`, then
`set_item_frame(<magic frame, likely 2, TBD>)` instead of changing the shape. Guard on the
current frame so re-casting on boots already in the magic frame (or some other color
variant) doesn't do something unintended.

BG's `data/bg/shape_info.txt` has no `framenames` entry for shape 587 (SI's does, frames
0-6), so Exult won't show a distinct name for the magic frame in BG — it'll still read as
"boots" in tooltips/identify even once transformed. Cosmetic, but worth knowing going in.

## Shape mapping table

| Item | Base shape | Magic shape | Const names | Confidence |
|---|---|---|---|---|
| Great helm | 541 (`SHAPE_GREAT_HELM`) | 383 | need `SHAPE_MAGIC_HELM` | High — exact name match (`si_shapes.uc:239`, comment "Includes Helm of Courage") |
| Gorget | 586 (`SHAPE_GORGET`) | 843 | need `SHAPE_MAGIC_GORGET` | High — exact name match in shape list |
| Gauntlets | 580 (`SHAPE_GAUNTLETS`) | 835 | need `SHAPE_MAGIC_GAUNTLETS` | High — exact name match, also in `si_shapes.uc:269` |
| Plate leggings | 576 (`SHAPE_PLATE_LEGGINGS`) | 686 | need `SHAPE_MAGIC_LEGGINGS` | High — `si_shapes.uc:267` |
| Curved heater | 545 | 663 | need `SHAPE_CURVED_HEATER`, `SHAPE_MAGIC_SHIELD` | Medium — reuses the *generic* magic shield graphic, not a heater-specific one (none exists) |
| Antique armor | 836 | 666 (`SHAPE_MAGIC_ARMOR`, already declared) | need `SHAPE_ANTIQUE_ARMOUR` (source only) | Decided — reuses plate armor's magic target |
| Leather boots | 587 frame 0 (`SHAPE_LEATHER_BOOTS`) | same shape 587, frame 2 *(tentative — confirm 0- vs 1-based)* | — (frame swap, no new consts) | Medium — visual lead, not yet confirmed |

Other than `SHAPE_MAGIC_ARMOR` (already declared), none of the "need" constants above
exist yet in [bg_shapes2.uc](content/bgkeyring/src/headers/bg/bg_shapes2.uc) — add each
alongside its base shape as you reach that item (base consts for
helm/gorget/gauntlets/leggings/boots already exist at lines 57-72; curved heater and
antique armor have no BG const at all yet, only in `si_shapes.uc`, since they're not part
of vanilla BG's item set — the graphic exists in the shared shapes file, but **verify
in-game that the antique armor *source* shape is actually wearable with a sane armor
value**, since BG's data never had reason to define one. The magic-armor *target* is
already proven — it's plate armor's existing upgrade).

## Per-item workflow (repeat for each shape-swap item: helm, gorget, gauntlets, leggings, heater)

1. Add the base + magic shape consts to
   [bg_shapes2.uc](content/bgkeyring/src/headers/bg/bg_shapes2.uc) (skip if the base
   const already exists).
2. Add the pair to **both** array literals — easy to update only one and get a spell
   that "succeeds" but does nothing, or silently fails on a valid target:
   - [second_circle.uc:97-98](content/bgkeyring/src/spells/second_circle.uc#L97) —
     `normal_missiles`/`magic_missiles` in `spellEnchant()` (gates eligibility).
   - [spell_functions.uc:123-124](content/bgkeyring/src/spells/spell_functions.uc#L123) —
     the same two arrays, re-declared locally in `spellEnchantEffect()` (does the swap).

   (Rename the arrays or their variable names if `normal_missiles`/`magic_missiles` reads
   wrong once armor's in there — cosmetic, doesn't affect behavior.)
3. Compile via the bgkeyring Makefile (`ucc.exe`, `U7PATH` pointed at the installed
   game/mod folder — see `content/bgkeyring/Makefile.mingw`), pack with `expack.exe`.
4. In-game: spawn the base item, cast Enchant on it (target it with the spell), confirm:
   - [ ] Shape changes to the magic version (check both in inventory *and* worn/equipped —
     paperdoll should update live, no re-equip needed per the `set_item_shape` precedent).
   - [ ] Armor/defense value actually increased (open stats or `identify`) — this is the
     real risk for curved heater/antique armor specifically, since their shapes aren't in
     BG's normal item set.
   - [ ] Item still occupies the correct equip slot and nothing else broke (weight,
     stacking if relevant).
5. Check the item off, move to the next.

Leather boots follows the same *compile → spawn → cast → verify* loop, but step 2 is the
frame-swap branch described above instead of an array pair, and step 4 needs an extra
check: confirm the frame index before relying on it (see boots section above).

## Suggested order
1. Great helm, gorget, gauntlets, plate leggings — highest confidence, do these to prove
   the pattern end-to-end (including the worn/equipped check) before the trickier ones.
2. Curved heater — same pattern, but confirm the generic magic-shield look reads OK for it.
3. Leather boots — confirm the frame index visually first, then add the frame-swap branch.
4. Antique armor — same shape-swap pattern as step 1, targeting the already-declared
   `SHAPE_MAGIC_ARMOR`; just double-check the source shape (836) actually carries a valid
   armor value in BG before shipping it.
