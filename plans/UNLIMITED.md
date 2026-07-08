# Unlimited Ammo cheat option

Add an "Unlimited Ammo" toggle (Enabled/Disabled) to the Game Engine Options gump,
directly below **Feeding**, visible only when **Cheats** is Yes. When on, ranged/thrown
weapons still fire and still require ammo to be present, but the ammo's quantity/charges
are not decremented. Only applies to the party (mirrors how Feeding's `use_food()` is only
ever called on `party[]`, [gameclk.cc:219](gameclk.cc#L219)) — monsters/NPCs must keep
consuming ammo normally, or the cheat trivializes combat against the player.

Modeled directly on the existing **Feeding** cheat (`Cheat::FoodUse`), which is the
closest analog already wired end-to-end: `cheat.h`/`cheat.cc` state + config persistence,
gump toggle button shown conditionally, save/load through `Configuration`.

---

## 1. `cheat.h` / `cheat.cc` — state + persistence

- [ ] [cheat.h:262-277](cheat.h#L262) — next to the `FoodUse` enum/`food_use` field, add:
  ```cpp
  private:
      bool unlimited_ammo = false;
  public:
      bool GetUnlimitedAmmo(bool saved = false) {
          return enabled || saved ? unlimited_ammo : false;
      }
      void SetUnlimitedAmmo(bool newval, bool writeout);
  ```
  `saved` param mirrors `GetFoodUse(bool saved)` — the options gump needs to read the
  stored value even while `cheats` is currently off, to prefill the toggle.

- [ ] [cheat.cc:302-316](cheat.cc#L302) `Cheat::init()` — load alongside `feeding`:
  ```cpp
  std::string unlimited_ammo_str;
  config->value("config/gameplay/unlimited_ammo", unlimited_ammo_str, "no");
  unlimited_ammo = unlimited_ammo_str == "yes";
  ```

- [ ] [cheat.cc:1513](cheat.cc#L1513) — add setter next to `SetFoodUse`:
  ```cpp
  void Cheat::SetUnlimitedAmmo(bool newval, bool writeout) {
      unlimited_ammo = newval;
      config->set("config/gameplay/unlimited_ammo", newval ? "yes" : "no", writeout);
  }
  ```

## 2. `gumps/GameEngineOptions_gump.h` — button plumbing

- [ ] [GameEngineOptions_gump.h:42](GameEngineOptions_gump.h#L42) — add `int unlimited_ammo;`
      member next to `int feeding;`.
- [ ] [GameEngineOptions_gump.h:60](GameEngineOptions_gump.h#L60) — add
      `id_unlimited_ammo,` to `button_ids` enum, right after `id_feeding`.
- [ ] [GameEngineOptions_gump.h:121-123](GameEngineOptions_gump.h#L121) — add toggle next to
      `toggle_feeding`:
  ```cpp
  void toggle_unlimited_ammo(int state) {
      unlimited_ammo = state;
  }
  ```

## 3. `gumps/GameEngineOptions_gump.cc` — wire it into the gump

- [ ] [GameEngineOptions_gump.cc:138-140](gumps/GameEngineOptions_gump.cc#L138) — add a
      `Strings` accessor next to `Feeding_()`:
  ```cpp
  static auto UnlimitedAmmo_() {
      return get_text_msg(0x635 - msg_file_start);
  }
  ```
- [ ] [GameEngineOptions_gump.cc:239-258](gumps/GameEngineOptions_gump.cc#L239)
      `update_cheat_buttons()` — same conditional pattern as `id_feeding`: reset when
      `!cheats`, otherwise create a Yes/No toggle button positioned on the next row after
      Feeding:
  ```cpp
  if (!cheats) {
      buttons[id_feeding].reset();
      buttons[id_unlimited_ammo].reset();
  } else {
      std::vector<std::string> feedingOpts = {Strings::Manual(), Strings::Automatic(), Strings::Disabled()};
      buttons[id_feeding]                  = std::make_unique<GameEngineTextToggle>(
              this, &GameEngineOptions_gump::toggle_feeding, std::move(feedingOpts), feeding,
              get_button_pos_for_label(Strings::Feeding_()), yForRow(++y_index), large_size);

      const std::vector<std::string> yesNo = {Strings::No(), Strings::Yes()};
      buttons[id_unlimited_ammo]           = std::make_unique<GameEngineTextToggle>(
              this, &GameEngineOptions_gump::toggle_unlimited_ammo, yesNo, unlimited_ammo,
              get_button_pos_for_label(Strings::UnlimitedAmmo_()), yForRow(++y_index), small_size);
  }
  ```
  Note: `ResizeWidthToFitWidgets`/`RightAlignWidgets` calls below already operate on the
  whole `buttons` span, so no change needed there — just confirm the gump's initial height
  (`SetProceduralBackground(TileRect(0, 0, 100, yForRow(13))` in the constructor,
  [GameEngineOptions_gump.cc:302](gumps/GameEngineOptions_gump.cc#L302)) still fits 14 rows
  when Cheats+Feeding+Unlimited Ammo are all showing; bump `yForRow(13)` → `yForRow(14)` if
  the extra row gets clipped.
- [ ] [GameEngineOptions_gump.cc:298](gumps/GameEngineOptions_gump.cc#L298)
      `load_settings()` — next to `feeding = int(cheat.GetFoodUse(true));`:
  ```cpp
  unlimited_ammo = cheat.GetUnlimitedAmmo(true) ? 1 : 0;
  ```
- [ ] [GameEngineOptions_gump.cc:339](gumps/GameEngineOptions_gump.cc#L339)
      `save_settings()` — next to `cheat.SetFoodUse(...)`:
  ```cpp
  cheat.SetUnlimitedAmmo(unlimited_ammo != 0, false);
  ```
  (leave `writeout=false` — the shared `config->write_back()` at the end of
  `save_settings()` already flushes it, same as Feeding.)
- [ ] [GameEngineOptions_gump.cc:364-366](gumps/GameEngineOptions_gump.cc#L364) `paint()` —
      next to the `id_feeding` label paint, add the matching conditional label:
  ```cpp
  if (buttons[id_unlimited_ammo]) {
      font->paint_text(iwin->get_ib8(), Strings::UnlimitedAmmo_(), x + label_margin, y + yForRow(++y_index) + 1);
  }
  ```

## 4. `data/exultmsg.txt` — new string

- [ ] [data/exultmsg.txt:302](data/exultmsg.txt#L302) — `0x635` is free (next used id is
      `0x640`, InputOptions_gump.cc block). Insert after the `Enhancements:` line:
  ```
  0x635:Unlimited Ammo:
  ```
  Not translating `exultmsg_de.txt`/`_es.txt`/`_fr.txt` — those lag the English source
  file for new strings as a matter of course elsewhere in this project; a missing id in a
  translation file does not crash (`get_text_msg`/`get_text_internal` — verify fallback,
  [shapes/items.cc:160](shapes/items.cc#L160)).

## 5. `combat.cc` — actually skip the decrement

This is the behavioral core. Ammo consumption happens in
`Combat_schedule::attack_target()`.

- [ ] [combat.cc:1278-1304](combat.cc#L1278) — guard the two decrement branches
      (`winf->uses_charges()` quality path, and the `modify_quantity` quantity path) so
      they're skipped when the cheat is on **and** the attacker is a party member:
  ```cpp
  if (need_ammo) {
      // We should only ever get here for containers and NPCs.
      // Also, ammo should never be zero in this branch.
      bool       need_new_weapon = false;
      const bool ready           = att ? att->find_readied(ammo) >= 0 : false;
      const bool skip_consume    = cheat.GetUnlimitedAmmo() && att && att->is_in_party();

      // Time to use up ammo.
      if (!skip_consume) {
          if (winf->uses_charges()) {
              if (ammo->get_info().has_quality()) {
                  ammo->set_quality(ammo->get_quality() - need_ammo);
              }
              if (winf->delete_depleted() && (!ammo->get_quality() || !ammo->get_info().has_quality())) {
                  if (att) {
                      att->remove(ammo);
                  }
                  ammo->remove_this();
                  need_new_weapon = true;
              }
          } else {
              const int quant = ammo->get_quantity();
              if (att && quant == need_ammo) {
                  att->remove(ammo);
              }
              ammo->modify_quantity(-need_ammo, &need_new_weapon);
          }
      }

      if (att && need_new_weapon && ready) {
          ...
  ```
  `need_new_weapon` stays `false` when skipped, so the "ammo depleted, ready a new
  weapon/wait for return" branch below is naturally a no-op — no other change needed there.
  This reuses the same `att->is_in_party()` check the function already computes later as
  `att_party` ([combat.cc:1330](combat.cc#L1330)) for combat-bias math — not worth
  refactoring to share a variable across that gap, just recompute locally where needed.

## 6. Build & verify

Per project build instructions (MSBuild via vswhere, Release|x64, root `Exult.exe`):

```powershell
$msbuild = & "C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe" -latest -requires Microsoft.Component.MSBuild -find "MSBuild\**\Bin\MSBuild.exe"
& $msbuild "D:\GitHub\exult\msvcstuff\vs2019\Exult.vcxproj" /p:Configuration=Release /p:Platform=x64
```

- [ ] Confirm root `Exult.exe` timestamp/size actually changed (default target relinks;
      don't rely on `/t:ClCompile`-only output as proof).
- [ ] In-game: Game Engine Options → Cheats = Yes → confirm "Unlimited Ammo" row appears
      below Feeding, toggles Enabled/Disabled, and persists after closing/reopening the
      gump and after restarting Exult (config file round-trip).
- [ ] With Cheats = No, confirm the row is hidden (matches Feeding's behavior).
- [ ] Gameplay: equip a ranged/thrown weapon with countable ammo (e.g. arrows for a bow),
      note stack count, fire with Unlimited Ammo Enabled → stack count unchanged, projectile
      still fires/hits normally. Toggle Disabled → stack count decrements again as normal.
- [ ] Test a charge-based weapon too (e.g. a wand) if available, since it takes the
      `uses_charges()` branch instead of `modify_quantity()`.
- [ ] Confirm enemy NPCs firing ranged weapons at the party still consume their own ammo
      normally (the `att->is_in_party()` guard) — don't let the cheat make monsters
      invincible/inexhaustible too.
