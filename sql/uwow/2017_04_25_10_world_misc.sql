UPDATE `pet_stats` SET `type`='2' WHERE (`entry`='54983');

INSERT INTO `spell_area` (`spell`, `area`, `quest_start`, `autocast`, `quest_start_status`) VALUES ('59073', '4714', '14091', '1', '74');
INSERT INTO `phase_definitions` (`zoneId`, `entry`, `phasemask`, `terrainswapmap`, `comment`) VALUES ('4714', '3', '1', '638', 'Worgen Start loc');

INSERT INTO `spell_area` (`spell`, `area`, `quest_end`, `autocast`, `quest_start_status`) VALUES ('68481', '4714', '14375', '1', '74');
INSERT INTO `spell_area` (`spell`, `area`, `quest_end`, `autocast`, `quest_start_status`) VALUES ('68481', '4786', '14375', '1', '74');