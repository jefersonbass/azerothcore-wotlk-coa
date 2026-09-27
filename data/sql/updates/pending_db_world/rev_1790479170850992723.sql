-- Chronomancer Continuum (#5021): only one Continuum spell active at a time.
-- Paradox Cannon 806203, Flux Emitter 804435, Aether Compression 804436 and
-- Singularity Core 804438 all carry the tooltip line but share no family flags,
-- stance or group, so Aura::CanStackWith lets them coexist. They share spell
-- group 1145 with stack rule 1 (SPELL_GROUP_STACK_RULE_EXCLUSIVE): a new
-- Continuum replaces any active one regardless of caster, matching the tooltip.
DELETE FROM `spell_group` WHERE `id` = 1145;
INSERT INTO `spell_group` (`id`, `spell_id`) VALUES
(1145, 806203),
(1145, 804435),
(1145, 804436),
(1145, 804438);
DELETE FROM `spell_group_stack_rules` WHERE `group_id` = 1145;
INSERT INTO `spell_group_stack_rules` (`group_id`, `stack_rule`, `description`) VALUES
(1145, 1, 'Local CoA: one Chronomancer Continuum spell active at a time');
