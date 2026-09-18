-- Issue #731: Starcaller "Warden Training" (704790) — "Reduces the cooldown of
-- Warden's Blade by 25%." Warden's Blade carries no spell family, so the
-- passive's native Spell Pct Mod can never match it; the refund handler in
-- AscensionStarcaller.cpp needs a binding to the blade's chain root.
DELETE FROM `spell_script_names` WHERE `spell_id` = -805508 AND `ScriptName` = 'spell_ascension_starcaller_wardens_blade';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(-805508, 'spell_ascension_starcaller_wardens_blade');
