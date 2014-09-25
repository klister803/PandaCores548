UPDATE `creature_template` SET `ScriptName` = 'vehicle_golden_lotus_conteiner' WHERE `entry` in (71711, 71684, 71686);

UPDATE `creature_template` SET `InhabitType` = '3' WHERE `entry` in (71478, 71482, 71474, 71477, 71476, 71481);

DELETE FROM `spell_script_names` WHERE `spell_id` = 143559;