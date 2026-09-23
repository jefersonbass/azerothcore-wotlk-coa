-- Witch Hunter Edicts (#3928, #4415): one Edict per caster on each recipient. The group written by
-- rev_20260921_30_witch_hunter_edicts_exclusive.sql used spell_group 1138, which
-- rev_20260922_01_venomancer_pheromones.sql reassigns to the Venomancer Pheromones afterwards, so the
-- Edicts were left in no group at all. They move to the next free id; the rank chains of the three
-- Edicts resolve through spell_ranks (SpellMgr::GetSpellSpellGroupMapBounds calls GetFirstSpellInChain,
-- src/server/game/Spells/SpellMgr.cpp), so only the first rank of each is listed - SpellMgr::LoadSpellGroups
-- drops any other rank ("is not first rank of spell").
DELETE FROM `spell_group` WHERE `id` = 1139;
INSERT INTO `spell_group` (`id`, `spell_id`) VALUES
(1139, 523485),
(1139, 706741),
(1139, 707684),
(1139, 523510),
(1139, 680303),
(1139, 681442);
DELETE FROM `spell_group_stack_rules` WHERE `group_id` = 1139;
INSERT INTO `spell_group_stack_rules` (`group_id`, `stack_rule`, `description`) VALUES
(1139, 2, 'Local CoA: one Witch Hunter Edict per caster on each recipient');
