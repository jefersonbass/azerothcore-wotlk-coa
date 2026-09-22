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
-- Current Conversion (705670) belongs in this file after all: it is granted through SkillLineAbility
-- (SkillLine 61, "Lightning"), so it is reachable. Ghast (705684) stays out - it is absent from both
-- CharacterAdvancement.dbc and SkillLineAbility.dbc and no spell teaches it, so a row would be dead code.
-- SpellFamilyName is set on every row that carries a mask: SpellInfo::IsAffected returns true as soon
-- as the family name is 0, so a mask without its family is never consulted.
DELETE FROM `spell_proc` WHERE `SpellId` IN (707542, 705717, 705670, 500580);
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(707542, 0, 22, 32768, 0, 0, 69904, 1, 2, 0, 0, 0, 0, 0, 0, 0),
(705717, 0, 22, 0, 0, 0, 69904, 1, 2, 0, 0, 0, 0, 0, 0, 0),
(705670, 0, 22, 0, 0, 0, 69904, 1, 2, 0, 0, 0, 0, 0, 0, 0),
(500580, 0, 0, 0, 0, 0, 69972, 1, 2, 0, 0, 0, 0, 0, 0, 0);

DELETE FROM `spell_script_names` WHERE `spell_id` IN (705717, 705670)
  AND `ScriptName` = 'spell_ascension_stormbringer_runemaster_talent_proc';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(705717, 'spell_ascension_stormbringer_runemaster_talent_proc'),
(705670, 'spell_ascension_stormbringer_runemaster_talent_proc');
