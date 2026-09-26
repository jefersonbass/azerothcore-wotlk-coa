-- Starcaller Celestial Mind / Arcane Protection (#5167): normal and greater versions of
-- each buff can stay on the same recipient. Celestial Mind ranks 1-5 share the chain
-- 300255 -> 301222 -> 301223 -> 301224 -> 301225 (spell_ranks, rev_20260903_01) and
-- Greater Celestial Mind 680301 is unranked, so only the first rank and the greater spell
-- are listed (SpellMgr::LoadSpellGroups drops other ranks; chains resolve through
-- GetFirstSpellInChain, src/server/game/Spells/SpellMgr.cpp). Arcane Protection ranks
-- 1-3 share the chain 573343 -> 573344 -> 573345, with Greater Arcane Protection 573348
-- unranked, listed the same way. Rule 2 (SPELL_GROUP_STACK_RULE_EXCLUSIVE_FROM_SAME_CASTER)
-- makes the newest replace the caster's previous one while a second Starcaller's still
-- coexists, the same choice recorded for the Templar Gift (group 1140), Felsworn
-- Illidari Intuition (group 1141) and Witch Hunter Edict (group 1139) groups.
START TRANSACTION;
DELETE FROM `spell_group` WHERE `id` = 1142;
INSERT INTO `spell_group` (`id`, `spell_id`) VALUES
(1142, 300255),
(1142, 680301);
DELETE FROM `spell_group_stack_rules` WHERE `group_id` = 1142;
INSERT INTO `spell_group_stack_rules` (`group_id`, `stack_rule`, `description`) VALUES
(1142, 2, 'Local Starcaller: one Celestial Mind per caster on each recipient');
DELETE FROM `spell_group` WHERE `id` = 1143;
INSERT INTO `spell_group` (`id`, `spell_id`) VALUES
(1143, 573343),
(1143, 573348);
DELETE FROM `spell_group_stack_rules` WHERE `group_id` = 1143;
INSERT INTO `spell_group_stack_rules` (`group_id`, `stack_rule`, `description`) VALUES
(1143, 2, 'Local Starcaller: one Arcane Protection per caster on each recipient');
COMMIT;
