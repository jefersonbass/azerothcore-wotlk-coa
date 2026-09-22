-- Kirin Tor Adept (705635): binds aura_runemaster_kirin_tor_adept so the talent's
-- SPELLMOD_ALL_EFFECTS modifier is retargeted to Eye of the Beholder.
--
-- 705635 ships effect 0 as aura 108 (SPELL_AURA_ADD_PCT_MODIFIER) with MiscValue 8
-- (SPELLMOD_ALL_EFFECTS), BasePoints 49 / DieSides 1 -> +50%, and an EMPTY spell class mask.
-- AuraEffect::CalculateSpellMod copies that mask onto the SpellModifier, and
-- SpellInfo::IsAffected skips the family-flag test when the mask is zero (flag96::operator bool
-- is false for an all-zero mask), so the +50% reached every Runemaster spell instead of the one
-- ability the tooltip names.
-- Eye of the Beholder (500121) shares its family flags (0x0, 0x40, 0x0) with Warp
-- (500586/500587), so a mask rekey cannot separate them; the script narrows the modifier to the
-- named spell through SpellModifier::targetSpellId instead, which SpellInfo::IsAffectedBySpellMod
-- honours ahead of the family/mask test. The modifier is consumed natively: AuraEffect::CalculateAmount
-- calls SpellEffectInfo::CalcValue, which routes through Unit::ApplyEffectModifiers.
--
-- Registered with RegisterSpellScript, so it needs this spell_script_names row to be reachable;
-- SpellMgr::LoadSpellScripts binds only the names the table lists.
DELETE FROM `spell_script_names` WHERE `spell_id` = 705635 AND `ScriptName` = 'aura_runemaster_kirin_tor_adept';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(705635, 'aura_runemaster_kirin_tor_adept');
