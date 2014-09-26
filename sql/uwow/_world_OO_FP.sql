UPDATE `creature_template` SET `ScriptName` = 'vehicle_golden_lotus_conteiner' WHERE `entry` in (71711, 71684, 71686);

UPDATE `creature_template` SET `InhabitType` = '3' WHERE `entry` in (71478, 71482, 71474, 71477, 71476, 71481);

DELETE FROM `spell_script_names` WHERE `spell_id` = 143559;

UPDATE `creature_template` SET `ScriptName` = 'npc_measure_of_sun' WHERE `entry` in (71482, 71474);

--
UPDATE `creature_template` SET `flags_extra` = '128' WHERE `creature_template`.`entry` = 71685;

--
DELETE FROM `spell_script_names` WHERE `spell_id` = 143434;
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES ('143434', 'spell_fallen_protectors_shadow_word_bane');

-- Calamity
DELETE FROM `spell_script_names` WHERE `spell_id` = 143491;
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES ('143491', 'spell_fallen_protectors_calamity');

--
UPDATE `creature_template_addon` SET `auras` = '143708' WHERE `entry` in (71478, 71482, 71474, 71477, 71476, 71481);

--
UPDATE `creature_template` SET `ScriptName` = 'npc_measure_of_he' WHERE `entry` in (71478);

--
UPDATE `creature_template` SET `ScriptName` = 'npc_measure_of_rook' WHERE `entry` in (71476, 71477, 71481);

--
DELETE FROM `spell_script_names` WHERE `spell_id` = 143822;
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES ('143822', 'spell_fallen_protectors_mark_of_anguish_select_first_target');