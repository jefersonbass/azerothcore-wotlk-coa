-- Earthmaker (560150) trims one second off Earthen Avatar whenever Seismic
-- abilities or Quake deal damage, and reduces Seismic cooldowns by three
-- seconds. The minus three second part is native through the authored Add
-- Flat Modifier (effect 1, SPELLMOD_COOLDOWN with the Seismic class mask).
-- The Earthen Avatar trim rides a scripted proc instead: the authored proc
-- trigger (effect 0) and its cooldown helper (560151) carry no proc flags
-- or class mask in the DBC, so the native proc chain never fires. The
-- scripted proc fires on harmful damage only: Seismic Smash and Quake are
-- melee damage class (PROC_FLAG_DONE_SPELL_MELEE_DMG_CLASS) while the other
-- Seismic ranks are magic (PROC_FLAG_DONE_SPELL_MAGIC_DMG_CLASS_NEG),
-- excluding periodic ticks.
START TRANSACTION;
DELETE FROM `spell_script_names` WHERE `ScriptName` = 'aura_ascension_earthmaker';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(560150, 'aura_ascension_earthmaker');

DELETE FROM `spell_proc` WHERE `SpellId` = 560150;
INSERT INTO `spell_proc`
(`SpellId`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `Chance`) VALUES
(560150, 65552, 7, 2, 2, 0, 100);
COMMIT;
