-- #4474: Jar of Spiders (450806) casts spell 901519, whose summon effect names creature 901519 three
-- times; the upgraded item's spell 313682 names the same entry. The creature is absent from the world
-- database, so the guardian summon resolves to nothing and the item spawns no spiders.
-- Name, type and display come from the Exiles mirror of the game database (creature 901519 renders
-- display 959 -> MineSpider); owner faction and level are supplied by the guardian summon at runtime.
INSERT INTO `creature_template` (`entry`, `name`, `minlevel`, `maxlevel`, `faction`, `unit_class`, `type`)
VALUES (901519, 'Angry Spider', 1, 1, 14, 1, 1)
ON DUPLICATE KEY UPDATE `name` = VALUES(`name`), `minlevel` = VALUES(`minlevel`), `maxlevel` = VALUES(`maxlevel`), `faction` = VALUES(`faction`), `unit_class` = VALUES(`unit_class`), `type` = VALUES(`type`);

DELETE FROM `creature_template_model` WHERE `CreatureID` = 901519;
INSERT INTO `creature_template_model` (`CreatureID`, `Idx`, `CreatureDisplayID`, `DisplayScale`, `Probability`)
VALUES (901519, 0, 959, 1, 1);

-- 901535 is the summon's own damage control: a self-targeted MOD_DAMAGE_PERCENT_DONE aura named for
-- this item, with the same +16 offset the three sibling Worldforged summons use for theirs. Applied at
-- spawn so the guardian does not deal a full creature's damage on top of the owner's level.
DELETE FROM `creature_template_addon` WHERE `entry` = 901519;
INSERT INTO `creature_template_addon` (`entry`, `auras`) VALUES (901519, '901535');
