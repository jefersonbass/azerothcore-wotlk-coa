-- Ranger talents that act while Elude is up: Ranger's Gambit (803103, issue #3307) and Lie In Wait
-- (804709, issue #3431). Binds the aura script that applies and removes their payloads with the state.
--
-- Measured before writing:
--   * Both markers are aura 4 (DUMMY) with no trigger spell, so neither record wires a payload and
--     neither has a handler: the two talents do nothing today.
--   * Their tooltips are state-gated rather than event-gated - "While in Elude, gain $801492s1%
--     increased movement speed ..." and "While in Elude, you generate an Archery Point every $806349t1
--     sec" - and both payloads exist and are native: 801492 Forest Dweller (aura 31 plus an effect 3)
--     and 806349 (a periodic aura 23, duration 21).
--   * The module already names the state: SPELL_RANGER_ELUDE = 801345 in AscensionClassMechanics.cpp.
--     Its effects are 108, 16 and 36 on the caster, so the script hooks EFFECT_0 (SPELL_AURA_ADD_FLAT_MODIFIER).
--   * The existing Elude handling in SynchronizeAscensionClassMechanics runs only from OnPlayerLogin
--     (AscensionCompat.cpp:5520), which is the wrong trigger for a window that opens and closes in
--     combat, so the state is driven by an aura script on 801345 instead - the same shape as
--     aura_ascension_ranger_advantage beside it, which binds the same way.
--   * One script covers both talents, as they share the state and the moment.
--
-- The class is added in AscensionRangerSecondary.cpp.
DELETE FROM `spell_script_names` WHERE `spell_id` = 801345 AND `ScriptName` = 'aura_ascension_ranger_elude';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(801345, 'aura_ascension_ranger_elude');
