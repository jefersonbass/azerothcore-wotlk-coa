-- Stormbringer and Runemaster talents whose proc auras ship without proc flags (Spell.dbc ProcFlags 0
-- and no `spell_proc` row), so the clauses below never fired. Payloads are already authored and native:
-- the aura-42 handler casts each record's TriggerSpell (Thunder Wave 705693, Wild Steam 802637,
-- Lightning Rod 300928). Chance stays 0 everywhere, deferring to each record's own ProcChance
-- (Thunder Wave 5, Steam Conjurer 5, Lightning Rod 15).
-- Each clause is bound to a named ability, so SpellFamilyMask keys that ability's Spell.dbc family
-- flags: Weapon Engraving: Earth word0 131072 / word1 131072; Forked Lightning word0 256 / word2 32
-- plus its second record's word2 4096, OR-ed to 4128 so either version of the ability qualifies.
-- ProcFlags 69904 = the four direct damage spell classes only ("damage dealt by <ability>");
-- SpellTypeMask 1 = PROC_SPELL_TYPE_DAMAGE, SpellPhaseMask 2 = PROC_SPELL_PHASE_HIT.
DELETE FROM `spell_proc` WHERE `SpellId` IN (705692, 805743, 300609);
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(705692, 0, 22, 256, 0, 4128, 69904, 1, 2, 0, 0, 0, 0, 0, 0, 0),
(805743, 0, 38, 131072, 131072, 0, 69904, 1, 2, 0, 0, 0, 0, 0, 0, 0),
(300609, 0, 22, 256, 0, 4128, 69904, 1, 2, 0, 0, 0, 0, 0, 0, 0);
