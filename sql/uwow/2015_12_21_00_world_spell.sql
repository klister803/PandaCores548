insert into `spell_proc_check` (`entry`, `entry2`, `entry3`, `checkspell`, `hastalent`, `chance`, `target`, `effectmask`, `powertype`, `dmgclass`, `specId`, `spellAttr0`, `targetTypeMask`, `mechanicMask`, `fromlevel`, `perchp`, `spelltypeMask`, `combopoints`, `deathstateMask`, `hasDuration`, `comment`) values
('145449','0','0','119611','0','0','0','7','-1','-1','0','0','0','0','0','0','0','0','0','0','Item - Monk T16 Mistweaver 4P Bonus');
insert into `spell_trigger` (`spell_id`, `spell_trigger`, `spell_cooldown`, `option`, `target`, `caster`, `targetaura`, `bp0`, `bp1`, `bp2`, `effectmask`, `aura`, `chance`, `group`, `procFlags`, `procEx`, `check_spell_id`, `addptype`, `schoolMask`, `dummyId`, `dummyEffect`, `comment`) values
('145449','145450','0','0','0','0','0','0','0','0','7','0','0','0','0','0','0','-1','0','0','0','Туманная волна');

DELETE FROM pet_stats WHERE entry IN (73967);
insert into `pet_stats` (`entry`, `hp`, `ap`, `ap_type`, `spd`, `school_mask`, `maxspdorap`, `state`, `energy`, `energy_type`, `armor`, `type`, `damage`, `haste`, `comment`) values
('73967','1','-1','0','1','127','1','2','1','0','4','0','0','1','Призыв Сюэня, Белого Тигра - спек хмелевара');

insert into `spell_linked_spell` (`spell_trigger`, `spell_effect`, `type`, `caster`, `target`, `hastype`, `hastalent`, `hastype2`, `hastalent2`, `chance`, `cooldown`, `duration`, `hitmask`, `removeMask`, `targetCountType`, `targetCount`, `actiontype`, `group`, `comment`) values
('107428','130320','1','0','0','0','0','0','0','0','0','0','0','0','0','-1','0','0','Удар восходящего солнца');

INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES ('117368', 'spell_monk_grapple_weapon');
