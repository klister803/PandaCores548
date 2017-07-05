-- Теперь http://ru.wowhead.com/spell=53651 больше не будет срабатывать от эффектов периодического исцеления
UPDATE `spell_proc_event` SET `procFlags`='16384' WHERE `entry`='53651'; 
-- Переписана способность http://ru.wowhead.com/spell=106839
DELETE FROM `spell_script_names` WHERE `spell_id`='80964' AND `ScriptName`='spell_dru_skull_bash'; 
DELETE FROM `spell_script_names` WHERE `spell_id`='80965' AND `ScriptName`='spell_dru_skull_bash';
DELETE FROM `spell_linked_spell` WHERE `spell_trigger`='80964';
DELETE FROM `spell_linked_spell` WHERE `spell_trigger`='80965';
DELETE FROM `spell_dummy_trigger` WHERE `spell_id`='80964';
DELETE FROM `spell_dummy_trigger` WHERE `spell_id`='80965';
INSERT INTO `spell_dummy_trigger`(`spell_id`,`spell_trigger`,`option`,`target`,`caster`,`targetaura`,`bp0`,`bp1`,`bp2`,`effectmask`,`aura`,`chance`,`comment`) VALUES 
('80965','82365','6','0','0','0','0','0','0','1','0','0','Лобовая атака'),
('80965','93983','6','0','0','0','0','0','0','1','0','0','Лобовая атака'),
('80965','93985','6','0','0','0','0','0','0','1','0','0','Лобовая атака'),
('80964','82365','6','0','0','0','0','0','0','1','0','0','Лобовая атака'),
('80964','93983','6','0','0','0','0','0','0','1','0','0','Лобовая атака'),
('80964','93985','6','0','0','0','0','0','0','1','0','0','Лобовая атака'); 
-- Переписано наложение вредоносного эффекта "Слабые удары" способностями http://ru.wowhead.com/spell=6343 http://ru.wowhead.com/spell=35395 http://ru.wowhead.com/spell=48721 http://ru.wowhead.com/spell=77758
DELETE FROM `spell_script_names` WHERE `spell_id`='6343' AND `ScriptName`='spell_warr_thunder_clap'; 
DELETE FROM `spell_linked_spell` WHERE `spell_effect`='115798';
INSERT INTO `spell_linked_spell`(`spell_trigger`,`spell_effect`,`type`,`caster`,`target`,`hastype`,`hastalent`,`hastype2`,`hastalent2`,`chance`,`cooldown`,`duration`,`hitmask`,`removeMask`,`targetCountType`,`targetCount`,`actiontype`,`group`,`comment`) VALUES 
('77758','115798','1','0','0','0','0','0','0','0','0','0','0','0','0','-1','0','0','Слабые удары'),
('35395','115798','1','0','0','0','0','0','0','0','0','0','0','0','0','-1','0','0','Слабые удары'),
('6343','115798','1','0','0','0','0','0','0','0','0','0','0','0','0','-1','0','0','Слабые удары'),
('48721','115798','1','0','0','0','81132','0','0','0','0','0','0','0','0','-1','0','0','Слабые удары'); 
-- Переписана пассивная способность http://ru.wowhead.com/spell=46953 . Исправлен баг, когда при срабатывании визуального оповещения у вас не сбрасывался кулдаун. Теперь же кулдаун способности http://ru.wowhead.com/spell=23922 будет сбрасываться сразу же с проком этой визуалки
DELETE FROM `spell_script_names` WHERE `spell_id`='20243' AND `ScriptName`='spell_warr_sword_and_board'; 
DELETE FROM `spell_linked_spell` WHERE `spell_trigger`='50227' AND `spell_effect`='23922';
DELETE FROM `spell_linked_spell` WHERE `spell_trigger`='20243' AND `spell_effect`='50227';
INSERT INTO `spell_linked_spell`(`spell_trigger`,`spell_effect`,`type`,`caster`,`target`,`hastype`,`hastalent`,`hastype2`,`hastalent2`,`chance`,`cooldown`,`duration`,`hitmask`,`removeMask`,`targetCountType`,`targetCount`,`actiontype`,`group`,`comment`) VALUES 
('50227','23922','6','0','0','0','0','0','0','0','0','0','0','0','0','-1','8','0','Щит и меч'),
('20243','50227','0','0','0','0','46953','0','0','30','0','0','0','0','0','-1','0','0','Щит и меч'); 
-- Переписан "Символ смертельного удара"
DELETE FROM `spell_script_names` WHERE `spell_id`='12294' AND `ScriptName`='spell_warr_mortal_strike'; 
DELETE FROM `spell_aura_dummy` WHERE `spellId`='12294' AND `spellDummyId`='58368';
INSERT INTO `spell_aura_dummy`(`spellId`,`spellDummyId`,`option`,`target`,`caster`,`targetaura`,`aura`,`removeAura`,`effectDummy`,`effectmask`,`chance`,`attr`,`attrValue`,`custombp`,`specId`,`comment`) VALUES 
('12294','58368','4','0','0','0','0','0','0','16','0','0','0','0','0','Символ смертельного удара'); 
-- Переписан урон способности http://ru.wowhead.com/spell=52174
DELETE FROM `spell_bonus_data` WHERE `entry`='52174'; 
DELETE FROM `spell_script_names` WHERE `spell_id`='52174' AND `ScriptName`='spell_warr_heroic_leap_damage'; 
INSERT INTO `spell_bonus_data`(`entry`,`direct_bonus`,`dot_bonus`,`ap_bonus`,`ap_dot_bonus`,`damage_bonus`,`heal_bonus`,`comments`) VALUES 
('52174','0','0','0.5','0','0','0','Героический прыжок'); 
-- Переписано срабатывание лечащего эффекта способности http://ru.wowhead.com/spell=23881
DELETE FROM `spell_script_names` WHERE `ScriptName`='spell_warr_deep_wounds'; 
DELETE FROM `spell_script_names` WHERE `spell_id`='23881' AND `ScriptName`='spell_warr_bloodthirst'; 
DELETE FROM `spell_linked_spell` WHERE `spell_trigger`='23881' AND `spell_effect`='117313';
INSERT INTO `spell_linked_spell`(`spell_trigger`,`spell_effect`,`type`,`caster`,`target`,`hastype`,`hastalent`,`hastype2`,`hastalent2`,`chance`,`cooldown`,`duration`,`hitmask`,`removeMask`,`targetCountType`,`targetCount`,`actiontype`,`group`,`comment`) VALUES 
('23881','117313','0','0','0','0','0','0','0','0','0','0','1','0','0','-1','0','0','Кровожадность'); 
-- Переписан механизм снятия затрудняющих перемещение эффектов талантом http://ru.wowhead.com/spell=107574
DELETE FROM `spell_script_names` WHERE `spell_id`='107574' AND `ScriptName`='spell_war_avatar';  
DELETE FROM `spell_linked_spell` WHERE `spell_trigger`='107574' AND `spell_effect`='107574';
INSERT INTO `spell_linked_spell`(`spell_trigger`,`spell_effect`,`type`,`caster`,`target`,`hastype`,`hastalent`,`hastype2`,`hastalent2`,`chance`,`cooldown`,`duration`,`hitmask`,`removeMask`,`targetCountType`,`targetCount`,`actiontype`,`group`,`comment`) VALUES 
('107574','107574','6','0','0','0','0','0','0','0','0','0','0','0','0','-1','9','0','Аватара'); 
-- Переписан фильтр целей урона по области от способности http://ru.wowhead.com/spell=1464, когда воин находится под эффектом "Размашистые удары"
DELETE FROM `spell_script_names` WHERE `spell_id`='146361' AND `ScriptName`='spell_war_slam_aoe'; 
DELETE FROM `spell_target_filter` WHERE `spellId`='146361'; 
INSERT INTO `spell_target_filter`(`spellId`,`targetId`,`option`,`param1`,`param2`,`param3`,`aura`,`chance`,`effectMask`,`resizeType`,`count`,`maxcount`,`addcount`,`addcaster`,`comments`) VALUES 
('146361','16','9','0','0','0','0','0','1','0','0','0','0','0','Мощный удар'); 
-- Переписана пассивная способность http://ru.wowhead.com/spell=32215 . Переписано снятие ауры http://ru.wowhead.com/spell=32216
DELETE FROM `spell_script_names` WHERE `spell_id`='34428' AND `ScriptName`='spell_warr_victory_rush'; 
UPDATE `spell_proc_event` SET `SpellFamilyMask1`='256',`procFlags`='16',`effectmask`='1' WHERE `entry`='32216'; 
DELETE FROM `spell_linked_spell` WHERE `spell_trigger`='34428' AND `spell_effect`='-32216';
DELETE FROM `spell_linked_spell` WHERE `spell_trigger`='34428' AND `spell_effect`='118779';
INSERT INTO `spell_linked_spell`(`spell_trigger`,`spell_effect`,`type`,`caster`,`target`,`hastype`,`hastalent`,`hastype2`,`hastalent2`,`chance`,`cooldown`,`duration`,`hitmask`,`removeMask`,`targetCountType`,`targetCount`,`actiontype`,`group`,`comment`) VALUES 
('34428','-32216','0','0','0','0','0','0','0','0','0','0','0','0','0','-1','0','0','Победный раж'), 
('34428','118779','6','0','0','0','0','0','0','0','0','0','0','0','0','-1','0','0','Победный раж');
-- Исправлен баг, когда талант http://ru.wowhead.com/spell=116847 не создавал энергию ци, когда тактами попадает по 3м целям подряд
-- Исправлен баг, когда способность http://ru.wowhead.com/spell=107270 не создавал энергию ци, когда тактами попадает по 3м целям подряд
-- Исправлено взаимодействие способности "Мышечная память" и способностей http://ru.wowhead.com/spell=116847 и http://ru.wowhead.com/spell=107270. Исправлен баг, когда монах получал полезный эффект только раз в 6 секунд
DELETE FROM `spell_script_names` WHERE `spell_id`='107270' AND `ScriptName`='spell_monk_spinning_crane_kick'; 
DELETE FROM `spell_script_names` WHERE `spell_id`='148187' AND `ScriptName`='spell_monk_spinning_crane_kick';
DELETE FROM `spell_linked_spell` WHERE `spell_trigger`='148187' AND `spell_effect`='129881';
DELETE FROM `spell_linked_spell` WHERE `spell_trigger`='107270' AND `spell_effect`='129881';
DELETE FROM `spell_linked_spell` WHERE `spell_trigger`='148187' AND `spell_effect`='139597';
DELETE FROM `spell_linked_spell` WHERE `spell_trigger`='107270' AND `spell_effect`='139597';
INSERT INTO `spell_linked_spell`(`spell_trigger`,`spell_effect`,`type`,`caster`,`target`,`hastype`,`hastalent`,`hastype2`,`hastalent2`,`chance`,`cooldown`,`duration`,`hitmask`,`removeMask`,`targetCountType`,`targetCount`,`actiontype`,`group`,`comment`) VALUES 
('148187','129881','0','0','0','0','0','0','0','0','0','0','1','0','0','3','0','0','Порыв нефритового ветра'),
('107270','129881','0','0','0','0','0','0','0','0','0','0','1','0','0','3','0','0','Танцующий журавль'), 
('148187','139597','0','0','0','2','139598','0','0','0','1','0','1','0','0','3','0','0','Мышечная память'),
('107270','139597','0','0','0','2','139598','0','0','0','1','0','1','0','0','3','0','0','Мышечная память');
-- Переработано группирование "смертельных и несмертельных" ядов
DELETE FROM `spell_script_names` WHERE `spell_id`='2823' AND `ScriptName`='spell_rog_poisons'; 
DELETE FROM `spell_script_names` WHERE `spell_id`='3408' AND `ScriptName`='spell_rog_poisons'; 
DELETE FROM `spell_script_names` WHERE `spell_id`='5761' AND `ScriptName`='spell_rog_poisons'; 
DELETE FROM `spell_script_names` WHERE `spell_id`='8679' AND `ScriptName`='spell_rog_poisons'; 
DELETE FROM `spell_script_names` WHERE `spell_id`='108211' AND `ScriptName`='spell_rog_poisons'; 
DELETE FROM `spell_script_names` WHERE `spell_id`='108215' AND `ScriptName`='spell_rog_poisons'; 
REPLACE INTO `spell_group`(`id`,`spell_id`) VALUES ('2823','2823'), ('2823','8679');
REPLACE INTO `spell_group`(`id`,`spell_id`) VALUES ('3408','3408'), ('3408','5761'), ('3408','108211'), ('3408','108215');
REPLACE INTO `spell_group_stack_rules`(`group_id`,`stack_rule`) VALUES ('2823','1'), ('3408','1'); 
DELETE FROM `spell_script_names` WHERE `ScriptName`='spell_rog_deadly_poison';