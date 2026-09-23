-- #473: Harvesting Grounds (705413) is a SPELL_EFFECT_SUMMON that names creature 300662, which the
-- world database never carried, so the ground never appeared. The spell carries the summon on its
-- second effect (27, 28, 64) with 300662 in that effect's MiscValue - 707591 is a different record
-- of the same name and has no summon effect.
-- Name, side and display come from the Exiles mirror (creature 300662 is a neutral, type-less trigger
-- that renders display 11686 -> Creature/InvisibleStalker/InvisibleStalker.mdx, the invisible model a
-- ground marker is meant to use).
INSERT INTO `creature_template` (`entry`, `name`, `minlevel`, `maxlevel`, `faction`, `unit_class`, `type`)
VALUES (300662, 'Harvesting Grounds', 1, 1, 14, 1, 0)
ON DUPLICATE KEY UPDATE `name` = VALUES(`name`), `minlevel` = VALUES(`minlevel`), `maxlevel` = VALUES(`maxlevel`), `faction` = VALUES(`faction`), `unit_class` = VALUES(`unit_class`), `type` = VALUES(`type`);

DELETE FROM `creature_template_model` WHERE `CreatureID` = 300662;
INSERT INTO `creature_template_model` (`CreatureID`, `Idx`, `CreatureDisplayID`, `DisplayScale`, `Probability`)
VALUES (300662, 0, 11686, 1, 1);
