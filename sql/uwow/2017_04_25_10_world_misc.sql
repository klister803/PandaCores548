UPDATE `pet_stats` SET `type`='2' WHERE (`entry`='54983');

INSERT INTO `spell_area` (`spell`, `area`, `quest_start`, `autocast`, `quest_start_status`) VALUES ('59073', '4714', '14091', '1', '74');
INSERT INTO `phase_definitions` (`zoneId`, `entry`, `phasemask`, `terrainswapmap`, `comment`) VALUES ('4714', '3', '1', '638', 'Worgen Start loc');

INSERT INTO `spell_area` (`spell`, `area`, `quest_end`, `autocast`, `quest_start_status`) VALUES ('68481', '4714', '14375', '1', '74');
INSERT INTO `spell_area` (`spell`, `area`, `quest_end`, `autocast`, `quest_start_status`) VALUES ('68481', '4786', '14375', '1', '74');

UPDATE `spell_linked_spell` SET `chance`='32409' WHERE (`spell_trigger`='118') AND (`spell_effect`='-3') AND (`type`='0') AND (`hastalent`='56375') AND (`actiontype`='2');
UPDATE `spell_linked_spell` SET `chance`='32409' WHERE (`spell_trigger`='19503') AND (`spell_effect`='-3') AND (`type`='0') AND (`hastalent`='119407') AND (`actiontype`='2');
UPDATE `spell_linked_spell` SET `chance`='32409' WHERE (`spell_trigger`='3355') AND (`spell_effect`='-3') AND (`type`='2') AND (`hastalent`='119407') AND (`actiontype`='2');
UPDATE `spell_linked_spell` SET `chance`='32409' WHERE (`spell_trigger`='2094') AND (`spell_effect`='-3') AND (`type`='0') AND (`hastalent`='91299') AND (`actiontype`='2');

DELETE FROM `spell_script_names` WHERE (`spell_id`='115078') AND (`ScriptName`='spell_monk_glyph_of_paralysis');
INSERT INTO `spell_linked_spell` (`spell_trigger`, `spell_effect`, `hastalent`, `chance`, `hitmask`, `actiontype`, `comment`) VALUES ('115078', '-3', '125755', '32409', '1', '2', 'Glyph of Paralysis');