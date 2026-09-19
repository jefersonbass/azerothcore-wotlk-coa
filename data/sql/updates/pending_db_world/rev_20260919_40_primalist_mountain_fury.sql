-- Mountain Fury (806185) channels a cone that damages and pulls enemies toward the
-- caster while stacking a two percent damage-taken reduction. The whole chain is
-- authored in the client DBC: the parent's two periodic-trigger effects fire
-- 807724 (cone damage plus pull through 806186) and 807434 (the stacking
-- reduction aura) every two seconds, and the pull's leap destination resolves
-- natively from its three-yard radius. The damage helper 806186 is
-- SPELL_DAMAGE_CLASS_MELEE with a flat base (BasePoints 891, DieSides 15) and a
-- native per-level term (RealPointsPerLevel 5.2), but it has no
-- `spell_bonus_data` row, so SpellDamageBonusDone adds no attack power term.
-- Its description states "Attack Power, Level (+5.2/lvl)", so the attack power
-- term resolves to melee attack power (no ranged mask, no weapon requirement).
START TRANSACTION;
DELETE FROM `spell_bonus_data` WHERE `entry` = 806186;
INSERT INTO `spell_bonus_data` (`entry`, `direct_bonus`, `dot_bonus`, `ap_bonus`, `ap_dot_bonus`, `comments`) VALUES
(806186, 0, 0, 1, 0, 'Local Primalist: Mountain Fury - cone damage, attack power term from its description');

DELETE FROM `spell_script_names` WHERE `ScriptName` = 'aura_ascension_mountain_fury';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(806185, 'aura_ascension_mountain_fury');
COMMIT;
