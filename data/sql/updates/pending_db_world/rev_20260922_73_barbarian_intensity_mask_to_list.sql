-- Intensity (705226): replace the family mask with the exact Berserker Axe list.
--
-- rev_20260922_66 gave this row SpellFamilyName 18 with SpellFamilyMask1 0x80000, and that mask is not
-- exclusive: Spite (520541) carries the same word 1 bit 19, so the proc fired on Spite as well. A
-- spell_proc mask cannot separate the two, because SpellInfo::IsAffected is a single 96-bit AND
-- (SpellInfo.cpp:1439) - ANY shared bit selects the spell, so a mask holding the ability's full flags
-- still matches every family member that shares one.
--
-- The narrowing moves to spell_ascension_barbarian_talent_proc, which holds the eight Berserker Axe
-- records exactly (503408-503414, 804138). The row keeps the same gate and keeps a zero mask, because
-- with a mask the script would be the only filter anyway and a non-zero one only risks matching Spite
-- again if the data shifts.
--
-- ProcFlags and phases are unchanged from rev_20260922_66: 69904 (the spell-only DONE set) with
-- SpellTypeMask 1 and SpellPhaseMask 2 (HIT), since the clause is "dealing damage with".
DELETE FROM `spell_proc` WHERE `SpellId` = 705226;
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(705226, 0, 0, 0, 0, 0, 69904, 1, 2, 0, 0, 0, 0, 0, 0, 0);

DELETE FROM `spell_script_names` WHERE `spell_id` = 705226 AND `ScriptName` = 'spell_ascension_barbarian_talent_proc';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(705226, 'spell_ascension_barbarian_talent_proc');
