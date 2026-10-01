-- Marked for Death (806973), issue #3660: "While you have daggers equipped, direct
-- critical strikes on enemies below 35% health now have a chance to allow the use of
-- abilities as if you were in Elude for 5 sec."
--
-- Measured before writing (server-dbc/Spell.dbc):
--   * 806973 carries a single aura effect, SPELL_AURA_PROC_TRIGGER_SPELL (42) on 806974,
--     with Spell.dbc ProcFlags 0. The `spell_proc` row from
--     rev_20260921_40_ranger_dead_procs.sql (69972, type 1, phase 2, crit-only) makes the
--     aura fire; 806974 lands on the Ranger by itself (both effects TargetA 1) and its
--     aura 275 (SPELL_AURA_MOD_IGNORE_SHAPESHIFT, mask 32832) natively unlocks the Elude
--     abilities Toxic Dart, Guise, Sticky Fingers and Rusty Shiv for 5 sec.
--   * Two tooltip clauses cannot be delivered by that row and ship here. The "below 35%
--     health" half is the `conditions` row below: SourceType 24 =
--     CONDITION_SOURCE_TYPE_SPELL_PROC (ConditionMgr.h:150), type 38 = CONDITION_HP_PCT
--     on ConditionTarget 1 (the struck enemy, from ConditionSourceInfo(actor, actionTarget)
--     at SpellAuras.cpp:2210), value 35 with COMP_TYPE_LOW (2, Util.h:581). Same pattern
--     as Mineralization in rev_1789913115262463100.sql.
--   * The "while you have daggers equipped" half is aura_ascension_ranger_marked_for_death
--     (AscensionRangerTalents.cpp): 806974's own EquippedItem gate is bypassed by the
--     non-passive branch of Player::HasItemFitToSpellRequirements when no fitting weapon
--     is equipped at all, so the CheckProc requires a main- or off-hand dagger
--     (ITEM_SUBCLASS_WEAPON_DAGGER) on the Ranger before the proc may fire.
--   * Chance stays on the record's own ProcChance 100: the tooltip says "have a chance"
--     but states no number, and inventing one would be worse than using the record's.
DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId` = 24 AND `SourceEntry` = 806973;
INSERT INTO `conditions` (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`,
`ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`,
`NegativeCondition`, `ErrorType`, `ErrorTextId`, `ScriptName`, `Comment`) VALUES
(24, 0, 806973, 0, 0, 38, 1, 35, 2, 0, 0, 0, 0, '', 'Marked for Death only procs on a target below 35 percent health');

DELETE FROM `spell_script_names` WHERE `spell_id` = 806973 AND `ScriptName` = 'aura_ascension_ranger_marked_for_death';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(806973, 'aura_ascension_ranger_marked_for_death');
