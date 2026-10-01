-- #2004 Rapid Strikes (560341) reads "Melee and ranged auto attacks now have a 25%
-- chance to strike an additional time for 50% of the damage dealt. Can only occur once
-- per sec." Effect 0 is an unnamed Ascension aura 354 (SpellAuraEffects.cpp: no handler)
-- triggering 560342 'Rapid Strike', with Spell.dbc ProcFlags 0 and no `spell_proc` row,
-- so the aura never fires: LoadSpellProcs skips it (SpellMgr.cpp) and GetProcEffectMask
-- returns 0 on the missing entry. ProcFlags 68 = DONE_MELEE_AUTO_ATTACK (0x4) |
-- DONE_RANGED_AUTO_ATTACK (0x40), and only those bits: the tooltip names auto attacks,
-- not abilities. Family stays 0 on purpose: melee swings carry no SpellInfo and the
-- ranged auto shot is family 9 (spell 75), so any family-27 value would kill one half
-- (same reason as the 560810 and 572372 rows). SpellTypeMask 1, SpellPhaseMask 2
-- (both flags sit in the phase mask), HitMask 0 for the landed default. Chance 0 so the
-- record's own ProcChance 25 applies (rev_20260919_20_coa_proc_chance_parity.sql);
-- Cooldown 1000 is "once per sec" via AddProcCooldown. The C++ half
-- (aura_ascension_ranger_rapid_strikes, Talents) pays 50% of the dealt swing damage
-- through 560342, whose effect 0 is a zero-based SCHOOL_DAMAGE record driven by BP0.
DELETE FROM `spell_proc` WHERE `SpellId` = 560341;
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(560341, 0, 0, 0, 0, 0, 68, 1, 2, 0, 0, 0, 0, 0, 1000, 0);
DELETE FROM `spell_script_names` WHERE `spell_id` = 560341 AND `ScriptName` = 'aura_ascension_ranger_rapid_strikes';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(560341, 'aura_ascension_ranger_rapid_strikes');
