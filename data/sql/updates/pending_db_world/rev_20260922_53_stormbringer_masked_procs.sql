-- Stormbringer talents whose proc auras ship without proc flags (Spell.dbc ProcFlags 0 and no
-- `spell_proc` row), so the clauses below never fired. Payloads are already authored and native: the
-- aura-42 handler casts each record's TriggerSpell. Chance stays 0 everywhere, deferring to each
-- record's own ProcChance (Binder of Storms 60, Evergale 100, Storm Bond 100).
-- ProcFlags 69904 = the four direct damage spell classes only; 69972 adds melee and ranged auto
-- attacks. SpellTypeMask 1 = PROC_SPELL_TYPE_DAMAGE, SpellPhaseMask 2 = PROC_SPELL_PHASE_HIT.
-- Where the clause names one ability, SpellFamilyMask keys that ability's own Spell.dbc family flags so
-- the proc fires only for it: Conduction word0 32768 (Binder of Storms), Call Lightning word1 16
-- (Evergale). Storm Bond's aura is applied to the Air Elemental by effect 190, so its proc rides the
-- pet's damage.
-- Current Conversion (705670) and Ghast (705684) were dropped from this file: neither is present in
-- CharacterAdvancement.dbc nor in SkillLineAbility.dbc and no spell teaches them, so a proc row for
-- them would be dead code.
DELETE FROM `spell_proc` WHERE `SpellId` IN (707542, 705717, 500580);
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(707542, 0, 0, 32768, 0, 0, 69904, 1, 2, 0, 0, 0, 0, 0, 0, 0),
(705717, 0, 0, 0, 16, 0, 69904, 1, 2, 0, 0, 0, 0, 0, 0, 0),
(500580, 0, 0, 0, 0, 0, 69972, 1, 2, 0, 0, 0, 0, 0, 0, 0);
