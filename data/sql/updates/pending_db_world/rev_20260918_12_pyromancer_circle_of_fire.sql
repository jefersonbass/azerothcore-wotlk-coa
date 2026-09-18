-- Issue #471: Pyromancer "Circle of Fire" (800807) — the area aura's periodic
-- trigger (680842: damage + incapacitate) runs natively from the client DBC,
-- but the tooltip's spell power scaling ($spfi*.31) needs a bonus data row.
DELETE FROM `spell_bonus_data` WHERE `entry` = 680842;
INSERT INTO `spell_bonus_data` (`entry`, `direct_bonus`, `dot_bonus`, `ap_bonus`, `ap_dot_bonus`, `comments`) VALUES
(680842, 0.31, 0, 0, 0, 'CoA Pyromancer - Circle of Fire tick (Fire Power x0.31)');
