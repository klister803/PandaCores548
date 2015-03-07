update quest_template set rewardspell = 0 where id in (31311,31470,31472,31475,31478,31479);

delete from spell_script_names where ScriptName = 'spell_gen_cooking_way';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES 
(124693, 'spell_gen_cooking_way'),
(124694, 'spell_gen_cooking_way'),
(125584, 'spell_gen_cooking_way'),
(125585, 'spell_gen_cooking_way'),
(125586, 'spell_gen_cooking_way'),
(125588, 'spell_gen_cooking_way');