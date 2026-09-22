-- "Wanted: Hogger" (quest 176) has no quest giver in the live world database.
--
-- Reported as "2nd wanted poster at the intersection between Westbrook Garrison and Forest's Edge should
-- act as a quest giver for quest ID 176": the poster is placed but unclickable, shows no quest tooltip
-- and offers nothing.
--
-- Measured before writing: `gameobject_questrelation` has NO row for quest 176, and
-- `creature_questrelation` has none either, so nothing in the live database offers it. The poster
-- itself exists - gameobject entry 68 "Wanted Poster" spawns at (-9668, 683) on map 0, which is the
-- Westbrook Garrison area the report names.
--
-- The correct target comes from the primary source, not from inference: the CoA world package
-- (data/coa-world/coa-world-20260912.zip, table gameobject_queststarter.sql) ships the pair
-- (68, 176). That same table pairs 164867 with quest 4081 instead, so entry 68 is the only poster that
-- ever offered 176 and no other entry is a candidate. The live database is missing a relation the
-- original server had, which is why the reporter sees a poster with no quest.
--
-- Only the missing pair is restored; nothing else about the poster is changed.
DELETE FROM `gameobject_questrelation` WHERE `id` = 68 AND `quest` = 176;
INSERT INTO `gameobject_questrelation` (`id`, `quest`) VALUES
(68, 176);
