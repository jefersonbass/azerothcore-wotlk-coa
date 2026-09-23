-- Templar Gifts (#4449): one Gift per caster on each recipient. The single-target and raid versions of
-- Gift of Fervor (Oathkeeper) and Gift of Zeal (Zealot) are separate spells with different auras - Fervor
-- applies aura 137, Zeal aura 29 - and no spell_group listed them, so Aura::CanStackWith let all four
-- coexist. Rule 2 (SPELL_GROUP_STACK_RULE_EXCLUSIVE_FROM_SAME_CASTER) makes the newest Gift replace the
-- caster's previous one while a second Templar's Gift still coexists, the same choice recorded for the
-- Witch Hunter Edicts, Venomancer Pheromones and Chronomancer Wisdom groups. Rank chains resolve through
-- spell_ranks (SpellMgr::GetSpellSpellGroupMapBounds calls GetFirstSpellInChain,
-- src/server/game/Spells/SpellMgr.cpp), so only the first rank of Gift of Zeal is listed.
DELETE FROM `spell_group` WHERE `id` = 1140;
INSERT INTO `spell_group` (`id`, `spell_id`) VALUES
(1140, 572629),
(1140, 572630),
(1140, 706634),
(1140, 680306);
DELETE FROM `spell_group_stack_rules` WHERE `group_id` = 1140;
INSERT INTO `spell_group_stack_rules` (`group_id`, `stack_rule`, `description`) VALUES
(1140, 2, 'Local Templar: one active Gift per caster');
