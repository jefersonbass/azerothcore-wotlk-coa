-- Felsworn Enslave Elemental (#4835): Noxxion must resist charm. Noxxion (13282)
-- carries the shared set -66 (frost + disorient/fear/stun/knockout, no charm),
-- so Enslave Elemental 954534 (MOD_CHARM, mechanic 1) lands and the boss glitches.
-- Per sql-guidelines: positive ids are single-creature sets; insert a superset under
-- a new id and point the creature at it. 2022 is the next free positive id after the
-- 2026_03_25_02 allocation (1972-2021). New mask = old 20516 | (1 << 1) = 20518.
DELETE FROM `creature_immunities` WHERE `ID` = 2022;
INSERT INTO `creature_immunities` (`ID`, `SchoolMask`, `DispelTypeMask`, `MechanicsMask`, `Effects`, `Auras`, `ImmuneAoE`, `ImmuneChain`, `Comment`) VALUES
(2022, 16, 0, 20518, '', '', 0, 0, 'school=0x10(FROST), mech=0x2812+CHARM (Noxxion #4835)');
UPDATE `creature_template` SET `CreatureImmunitiesId` = 2022 WHERE `entry` = 13282;
