DELETE FROM spell_pet_auras WHERE petEntry IN (54569) and spellId IN (124416);
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES ('119053', 'spell_monk_transcendence_clone_visual');

insert into `spell_target_filter` (`spellId`, `targetId`, `option`, `param1`, `param2`, `param3`, `aura`, `chance`, `effectMask`, `resizeType`, `count`, `maxcount`, `addcount`, `addcaster`, `comments`) values
('80265','7','3','0','16','0','0','0','7','0','1','0','0','-1','Зелье иллюзии'),
('80265','7','4','0','0','0','0','0','7','0','0','0','0','-1','Зелье иллюзии');

insert into `spell_proc_check` (`entry`, `entry2`, `entry3`, `checkspell`, `hastalent`, `chance`, `target`, `effectmask`, `powertype`, `dmgclass`, `specId`, `spellAttr0`, `targetTypeMask`, `mechanicMask`, `fromlevel`, `perchp`, `spelltypeMask`, `combopoints`, `deathstateMask`, `hasDuration`, `comment`) values
('114015','0','0','-121471','0','0','0','7','-1','-1','0','0','0','0','0','0','0','0','0','0','Предчувствие');

