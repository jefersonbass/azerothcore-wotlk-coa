-- CoA Ice Engraving (653266): direct damage has a 40% chance (Spell.dbc ProcChance) to cast
-- Weapon Engraving: Ice (653217). Reported as "Ice Engraving Not Triggering".
--
-- Measured before writing:
--   * Ice Engraving (653266) is SPELL_AURA_PROC_TRIGGER_SPELL (aura 42) with EffectTriggerSpell 653217,
--     ProcChance 40, and the tooltip "Your direct damage has a $h% chance to deal ... Frost damage".
--   * Its Spell.dbc ProcFlags are 0, and the world database had no spell_proc row for it, so the proc
--     carried no ProcFlags at all and could never fire. Same for the other engraving enablers
--     (653216 Water, 653221 Earth, 653267 Arcane, 653271 Fire, 520123 Primordial) - 653211 Fire is the
--     only one that works, and only because it already has the row below plus its aura script.
--   * The CoA world package has no spell_proc row for any of them either, so this is inherited, not lost.
--   * 653217 is a plain enemy-targeted damage spell (SPELL_EFFECT_SCHOOL_DAMAGE, TARGET_UNIT_TARGET_ENEMY),
--     unlike the Fire trigger which needs the Firebrand script, so the default proc action is enough here.
--     The aura script only refines the trigger condition to direct damage.
--
-- This mirrors the existing Fire Engraving fix (rev_1789574894945530251.sql) field for field: the same
-- "your direct damage" ProcFlags, the damage spell type and the on-hit phase.
DELETE FROM `spell_proc` WHERE `SpellId` = 653266;
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(653266, 0, 0, 0, 0, 0, 0x00010154, 0x1, 0x2, 0, 0, 0, 0, 0, 0, 0);

DELETE FROM `spell_script_names` WHERE `spell_id` = 653266 AND `ScriptName` = 'aura_ascension_runemaster_ice_engraving';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(653266, 'aura_ascension_runemaster_ice_engraving');
