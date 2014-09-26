UPDATE `creature_template` SET `ScriptName` = 'vehicle_golden_lotus_conteiner' WHERE `entry` in (71711, 71684, 71686);

UPDATE `creature_template` SET `InhabitType` = '3' WHERE `entry` in (71478, 71482, 71474, 71477, 71476, 71481);

DELETE FROM `spell_script_names` WHERE `spell_id` = 143559;

UPDATE `creature_template` SET `ScriptName` = 'npc_measure_of_sun' WHERE `entry` in (71482, 71474);

-- Calamity
DELETE FROM `spell_scripts` WHERE id = 143491;
REPLACE INTO `spell_scripts` (`id`, `effIndex`, `delay`, `command`, `datalong`, `datalong2`, `dataint`, `x`, `y`, `z`, `o`) VALUES 
('143491', '0', '0', '15', '143493', '2', '0', '0', '0', '0', '0'),
('143491', '0', '0', '14', '143434', '0', '0', '0', '0', '0', '0');

--
UPDATE `creature_template` SET `flags_extra` = '128' WHERE `creature_template`.`entry` = 71685;

--
DELETE FROM `spell_script_names` WHERE `spell_id` = 143434;
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES ('143434', 'spell_fallen_protectors_shadow_word_bane');