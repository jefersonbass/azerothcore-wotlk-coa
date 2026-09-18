-- Issue #465: Runemaster "Primordial Salvos" (800752) — Unleashing a Glyph
-- deals an additional 115 + 6% SP damage of the same magic school to all
-- enemies within 8 yds of the target. The salvo damage spells resolve their
-- spell power scaling here; the caster-side handler lives in
-- AscensionRunemasterGlyphs.cpp.
DELETE FROM `spell_bonus_data` WHERE `entry` IN (800729, 800730, 800731);
INSERT INTO `spell_bonus_data` (`entry`, `direct_bonus`, `dot_bonus`, `ap_bonus`, `ap_dot_bonus`, `comments`) VALUES
(800729, 0.06, 0, 0, 0, 'CoA Runemaster - Flame Salvo (Primordial Salvos)'),
(800730, 0.06, 0, 0, 0, 'CoA Runemaster - Frost Salvo (Primordial Salvos)'),
(800731, 0.06, 0, 0, 0, 'CoA Runemaster - Arcane Salvo (Primordial Salvos)');
