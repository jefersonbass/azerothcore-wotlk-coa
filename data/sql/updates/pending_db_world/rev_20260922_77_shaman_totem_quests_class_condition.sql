-- Call of Earth (1516-1521): restrict the totem questline to Shaman.
--
-- Reported as a Witch Doctor being able to pick up the Shaman totem quests. Confirmed as a defect rather
-- than intent: the Witch Doctor's kit is spirits, Reclamation/Volley, Master of Puppets, effigies, Loa's
-- Brew, Mojo Madness, Jungle Secrets, cauldrons and Jungle Thistle - no totem - and the class is a
-- reconstruction of its own, not a Shaman variant.
--
-- Measured before writing:
--   * quest_template has no class column at all (only AllowableRaces), so the quest data cannot express
--     "Shaman only". AllowableRaces is 130 (Horde) on 1516-1518 and 32 (Tauren) on 1519-1521.
--   * the givers are creature 5887 Canaga Earthcaller and 5888 Seer Ravenfeather, and both carry
--     npcflag 2 (QUESTGIVER, not TRAINER), so no shared trainer is involved. Removing the relation was
--     therefore rejected: those NPCs serve the legitimate Shaman of the same races, and deleting the
--     relation would break the questline for them.
--
-- The gate goes through the condition system instead, which is what the core reads for quest
-- availability: PlayerQuest.cpp:1229, 1745 and 1778 all call GetConditionsForNotGroupedEntry with
-- CONDITION_SOURCE_TYPE_QUEST_AVAILABLE.
--   * CONDITION_SOURCE_TYPE_QUEST_AVAILABLE = 19 (ConditionMgr.h:145)
--   * CONDITION_CLASS = 15 (ConditionMgr.h:49), which compares a 32-bit class mask
--   * ConditionValue1 = 128 = 1 << 7 = CLASS_SHAMAN. The custom CoA classes occupy the same 32-bit mask
--     (bits 12-27), so the Witch Doctor (class 13) does not match and no longer sees the quests.
--
-- Only the Shaman bit is set: no other custom class in the roster carries a totem fantasy, so none is
-- granted access.
DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId` = 19 AND `SourceEntry` IN (1516, 1517, 1518, 1519, 1520, 1521) AND `ConditionTypeOrReference` = 15;
INSERT INTO `conditions` (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `NegativeCondition`, `ErrorType`, `ErrorTextId`, `ScriptName`, `Comment`) VALUES
(19, 0, 1516, 0, 0, 15, 0, 128, 0, 0, 0, 0, 0, '', 'Call of Earth: Shaman only'),
(19, 0, 1517, 0, 0, 15, 0, 128, 0, 0, 0, 0, 0, '', 'Call of Earth: Shaman only'),
(19, 0, 1518, 0, 0, 15, 0, 128, 0, 0, 0, 0, 0, '', 'Call of Earth: Shaman only'),
(19, 0, 1519, 0, 0, 15, 0, 128, 0, 0, 0, 0, 0, '', 'Call of Earth: Shaman only'),
(19, 0, 1520, 0, 0, 15, 0, 128, 0, 0, 0, 0, 0, '', 'Call of Earth: Shaman only'),
(19, 0, 1521, 0, 0, 15, 0, 128, 0, 0, 0, 0, 0, '', 'Call of Earth: Shaman only');
