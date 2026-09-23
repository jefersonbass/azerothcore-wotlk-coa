-- #3725: Spectral Warden (805716) summons creature 100481, which the world database never carried, so
-- the guardian summon resolved to nothing and the ability spawned no warden.
-- Name, type and display come from the Exiles mirror of the game database (creature 100481 is a
-- neutral undead that renders display 211120 -> creature/mawguard/mawguard_3.m2); the summoning spell
-- itself carries the same name in its record. Owner faction and level are supplied at summon time.
INSERT INTO `creature_template` (`entry`, `name`, `minlevel`, `maxlevel`, `faction`, `unit_class`, `type`)
VALUES (100481, 'Spectral Warden', 1, 1, 14, 1, 6)
ON DUPLICATE KEY UPDATE `name` = VALUES(`name`), `minlevel` = VALUES(`minlevel`), `maxlevel` = VALUES(`maxlevel`), `faction` = VALUES(`faction`), `unit_class` = VALUES(`unit_class`), `type` = VALUES(`type`);

DELETE FROM `creature_template_model` WHERE `CreatureID` = 100481;
INSERT INTO `creature_template_model` (`CreatureID`, `Idx`, `CreatureDisplayID`, `DisplayScale`, `Probability`)
VALUES (100481, 0, 211120, 1, 1);
