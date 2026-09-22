-- Stormbringer and Runemaster talents whose proc clauses are gated on a STATE, which a `spell_proc`
-- row alone cannot express. The row restores the proc event; the AuraScript bound below
-- (spell_ascension_stormbringer_runemaster_talent_proc) then keeps it silent unless the state holds.
-- Mirrors the reviewed Reaper mechanism in rev_20260920_17; issues covered here: #1948, #796, #1877
-- and (mask-only, no script) #3341. If the upstream class sweep lands its own rows for these talents,
-- theirs win and these are dropped.
-- Payloads are already authored and native: the aura-42 handler casts each record's TriggerSpell
-- (Burned Etching 500475, Blade Rift 520238, Stone Savant 520934, Violent Monsoon 704225). Chance stays
-- 0 everywhere, deferring to each record's own ProcChance (all 100).
--   #1877 Burned Etching (500476) - "While Weapon Engraving: Fire is active, your Elemental Burst now
--        deals 10% increased damage": script requires aura 653211 (Fire Engraving, the aura the merged
--        sibling code already gates on at AscensionRunemasterSecondary.cpp) and restricts the source to
--        the Elemental Burst records. 653022 is the spellbook entry and is never carried as an aura.
--   #1948 Blade Rift (520237) - "While stealthed, using Warpdagger now increases your Magic Damage
--        dealt by 10% for 8 sec": script requires the caster to be stealthed (HasStealthAura, so any
--        stealth form qualifies) and restricts the source to the Warpdagger records. Phase 1 because
--        the clause says "using", not "damage dealt by".
--   #796 Stone Savant (520917) - "While Earthen Fists is active, critical strikes with Weapon
--        Engraving: Earth now causes you to strike twice": script requires aura 806982 (Earthen Fists)
--        and restricts the source to Weapon Engraving: Earth; HitMask 2 because the clause is critical.
--   #3341 Violent Monsoon (804016) - "Damage dealt by Conjure Storm and Arm of Thorim now applies
--        Violent Monsoon": no state, so the mask alone carries it - word1 2 / word2 16777296 is the OR
--        of Conjure Storm (word2 48) with Arm of Thorim (word1 2, word2 32 and 16777216).
DELETE FROM `spell_proc` WHERE `SpellId` IN (500476, 520237, 520917, 804016);
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(500476, 0, 0, 0, 0, 0, 69904, 1, 2, 0, 0, 0, 0, 0, 0, 0),
(520237, 0, 0, 0, 0, 0, 69904, 1, 1, 0, 0, 0, 0, 0, 0, 0),
(520917, 0, 0, 0, 0, 0, 69972, 1, 2, 2, 0, 0, 0, 0, 0, 0),
(804016, 0, 22, 0, 0, 0, 69904, 1, 2, 0, 0, 0, 0, 0, 0, 0);

DELETE FROM `spell_script_names` WHERE `spell_id` IN (500476, 520237, 520917, 804016)
  AND `ScriptName` = 'spell_ascension_stormbringer_runemaster_talent_proc';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(500476, 'spell_ascension_stormbringer_runemaster_talent_proc'),
(520237, 'spell_ascension_stormbringer_runemaster_talent_proc'),
(520917, 'spell_ascension_stormbringer_runemaster_talent_proc'),
(804016, 'spell_ascension_stormbringer_runemaster_talent_proc');
