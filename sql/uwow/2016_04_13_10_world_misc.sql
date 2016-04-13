DELETE FROM `spell_proc_event` WHERE `entry` IN (44448);
INSERT INTO `spell_proc_event` VALUES (44448, 4, 3, 4194304+1+16, 67108864+4096, 2097152, 0, 0, 0x00000C03, 0, 0, 0, 7);

INSERT INTO `spell_linked_spell` (`spell_trigger`, `spell_effect`, `hitmask`, `comment`) VALUES ('33876', '34071', '1', 'Mangle combo point');