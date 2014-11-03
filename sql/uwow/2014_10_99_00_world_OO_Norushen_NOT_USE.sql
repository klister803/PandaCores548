/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET NAMES utf8 */;
/*!40014 SET FOREIGN_KEY_CHECKS=0 */;

DELETE FROM `creature_template` WHERE `entry` = 90008;
UPDATE `creature_template` SET `unit_flags` = '32768' WHERE `entry` = 72276;

DELETE FROM creature where id in (72872, 71967, 72276);
UPDATE `gameobject_template` SET `flags` = `flags` | 0x10 WHERE entry in (223012, 223013, 223014, 223015, 221447, 221446, 221620, 221245);
-- 221620 221245 

DELETE FROM creature_text WHERE entry in (72872, 71967);
INSERT INTO `creature_text` (`entry`, `groupid`, `id`, `text`, `type`, `language`, `probability`, `emote`, `duration`, `sound`, `comment`) VALUES
-- 0xF1311CA800000B02
(72872, 0, 0, 'Невероятно! Еще один зал, похоже, созданный «титанами».', 12, 0, 100, 0, 0, 38138, 0),
(72872, 1,0, 'Да. Должно быть, он был спрятан давным-давно. И на то была причина!', 12, 0, 100, 0, 0, 38139, 0),
(72872, 2,0, 'В этой комнате, в недрах мирного края, спало сердце древнего бога.', 12, 0, 100, 0, 0, 38140, 0),
(72872, 3,0, 'Ох! Что это? Здравствуй! Я Хранитель истории Чо.', 12, 0, 100, 3, 0, 38141, 'Норусхен'),
(72872, 4,0, 'Порча? Мы здесь как раз, чтобы остановить ее и спасти Пандарию!', 12, 0, 100, 274, 0, 38142, 'Норусхен'),
(72872, 5,0, 'Да! Пропусти нас!', 12, 0, 100, 273, 0, 38143, 'Норусхен'),
-- 0xF131191F0000002F
(71967, 0,0, 'Стойте!', 14, 0, 100, 0, 0, 38883, 'Хранитель истории Чо'),
(71967, 1,0, 'Порча больше не проникнет в покои сердца!', 14, 0, 100, 0, 0, 38884, 'Хранитель истории Чо'),
(71967, 2,0, 'Вы хотите победить порчу?', 14, 0, 100, 0, 0, 38885, 'Хранитель истории Чо'),
(71967, 3,0, 'Если вы пройдете через эти двери сейчас, то потерпите неудачу. Вы поддались коварной порче, и имя ей – Гордыня.', 14, 0, 100, 0, 0, 38886, 'Хранитель истории Чо'),
(71967, 4,0, 'Вы гордитесь своими победами, и в этом ваша слабость.', 14, 0, 100, 0, 0, 0, 'Хранитель истории Чо'),
(71967, 5,0, 'Чтобы победить порчу, сначала нужно убить ее в себе.', 14, 0, 100, 0, 0, 0, 'Хранитель истории Чо'),
(71967, 6,0, 'Скажите, когда будете готовы ко встрече со своими демонами.', 14, 0, 100, 0, 0, 38887, 'Хранитель истории Чо'),
--
(71967, 7,0, 'Хорошо, я создам поле для удерживания порчи.', 14, 0, 100, 0, 0, 38888, 0),
(71967, 8,0, 'Докажите, что достойны, и я пропущу вас.', 14, 0, 100, 0, 0, 38889, 0),
(71967, 9,0, 'Свет несет очищение, но принять его непросто. Приготовьтесь! ', 14, 0, 100, 0, 0, 38880, 'Норусхен'),
(71967, 10,0, 'Загляните в себя, изгоните тьму.', 14, 0, 100, 0, 0, 38892, 0);

REPLACE INTO `spell_target_position` (`id`, `target_map`, `target_position_x`, `target_position_y`, `target_position_z`, `target_orientation`) VALUES 
('145188', '1136', '797.9045', '877.7274', '371.099', '0'),
('145143', '1136', '777.3924', '974.2292', '356.3398', '1.786108'),
('145149', '1136', '777.3924', '974.2292', '356.3398', '1.786108');

UPDATE `creature_template` SET `ScriptName` = 'npc_norushen_lowerwalker' WHERE `entry` = 72872;
UPDATE `creature_template` SET `ScriptName` = 'boss_norushen', `gossip_menu_id` = '71967' WHERE `entry` = 71967;

DELETE FROM `gossip_menu_option` WHERE `menu_id`=71967;
INSERT INTO `gossip_menu_option` (`menu_id`, `id`, `option_icon`, `option_text`, `option_id`, `npc_option_npcflag`, `action_menu_id`, `action_poi_id`, `box_coded`, `box_money`, `box_text`) VALUES
(71967, 0, 0, 'Мы готовы...', 1, 1, 0, 0, 0, 0, '');

SET @id = 0;
SET @entry = 72872;
DELETE FROM `script_waypoint` WHERE `entry` = @entry;
INSERT INTO `script_waypoint` (`entry`, `pointid`, `location_x`, `location_y`, `location_z`, `point_comment`) VALUES 
(@entry, @id := @id+ 1, 799.071, 872.569, 371.085, NULL),
(@entry, @id := @id+ 1, 797.319, 880.821, 371.112, NULL),
(@entry, @id := @id+ 1, 797.319, 880.821, 371.112, NULL);

--
REPLACE INTO`spell_script_names` (`spell_id`, `ScriptName`) VALUES ('145571', 'spell_norushen_blind_hatred');
DELETE FROM `conditions` WHERE `SourceEntry` = 145226;
INSERT INTO `conditions` (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `NegativeCondition`, `ErrorTextId`, `ScriptName`, `Comment`) VALUES
 ('13', '3', '145226', '0', '0', '31', '0', '3', '72565', '0', '0', '0', '', NULL);
 
--
UPDATE `creature_template` SET `ScriptName` = 'npc_norushen_manifestation_of_corruption_challenge' WHERE `creature_template`.`entry` = 71977;

--
UPDATE `creature_template` SET `ScriptName` = 'npc_norushen_manifestation_of_corruption_released' WHERE `creature_template`.`entry` = 72264;

--
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES ('145074', 'spell_norushen_residual_corruption');
INSERT INTO `areatrigger_data` (`entry`, `radius`, `radius2`, `activationDelay`, `updateDelay`, `maxCount`, `customVisualId`, `visualId`, `comment`) VALUES 
('5022', '1', '0', '0', '0', '0', '32875', '0', 'OO:NN spell_norushen_residual_corruption');
UPDATE `creature_template` SET `ScriptName` = 'npc_norushen_residual_corruption' WHERE `creature_template`.`entry` = 72550;

--
UPDATE `creature_template` SET `ScriptName` = 'npc_norushen_purifying_light' WHERE `entry` = 72065;
UPDATE `creature_template_addon` SET `auras` = '144717' WHERE `creature_template_addon`.`entry` = 72065;

--
DELETE FROM `spell_script_names` WHERE `ScriptName` LIKE 'spell_norushen_challenge';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES 
('144849', 'spell_norushen_challenge'),
('144850', 'spell_norushen_challenge'),
('144851', 'spell_norushen_challenge');

--
REPLACE INTO `creature_template_addon` (`entry`, `path_id`, `mount`, `bytes1`, `bytes2`, `emote`, `auras`) VALUES 
('71976', '0', '0', '0', '0', '0', '148452');
UPDATE `creature_template` SET `unit_flags` = '34816', `unit_flags2` = '2048', `rangeattacktime` = '2000', `ScriptName` = 'npc_essence_of_corruption_challenge' WHERE `creature_template`.`entry` = 71976;
REPLACE INTO `creature_template_addon` (`entry`, `path_id`, `mount`, `bytes1`, `bytes2`, `emote`, `auras`) VALUES 
('71976', '0', '0', '0', '1', '0', NULL);
UPDATE `creature_model_info` SET `gender` = '2' WHERE `creature_model_info`.`modelid` = 51859;

-- class 1 92 lvl base hp - 421592
-- used for heroic25 but should be same for all.
DELETE FROM `creature_difficulty_stat` WHERE entry = 71976;
INSERT INTO `creature_difficulty_stat` (`entry`, `difficulty`, `dmg_multiplier`, `Health_mod`) VALUES
(71976, 0, 1.15, 1.8),  -- not shure
(71976, 1, 1.15, 2.2),  -- not shure
(71976, 2, 1.15, 2.0),  -- not shure
(71976, 3, 1.15, 2.4),
(71976, 4, 1.15, 2.4);  -- not shure

DELETE FROM `creature_difficulty_stat` WHERE entry = 71977;
INSERT INTO `creature_difficulty_stat` (`entry`, `difficulty`, `dmg_multiplier`, `Health_mod`) VALUES
(71977, 0, 1.15, 2.6),  -- not shure
(71977, 1, 1.15, 3.0),  -- not shure
(71977, 2, 1.15, 2.8),  -- not shure
(71977, 3, 1.15, 3.2),
(71977, 4, 1.15, 3.2);  -- not shure

UPDATE `creature_template` SET `unit_flags` = '34816', `unit_flags2` = '2048', `rangeattacktime` = '2000', `ScriptName` = 'npc_essence_of_corruption_released' WHERE `creature_template`.`entry` = 72263;

-- 145064
REPLACE INTO `areatrigger_data` (`entry`, `radius`, `radius2`, isMoving, `activationDelay`, `updateDelay`, `maxCount`, `customVisualId`, `customEntry`, `comment`) VALUES 
('1106', '0', '0', '1', '0', '0', '0', '32660', '5021', 'OO:NN Expel Corruption released 145064');

REPLACE INTO `areatrigger_actions` (`entry`, `id`, `moment`, `actionType`, `targetFlags`, `spellId`, `maxCharges`, `chargeRecoveryTime`, `comment`) VALUES 
('1106', '0', '1', '0', '1', '145132', '0', '0', 'OO:NN Expel Corruption released 145064 onHit friend'), 
('1106', '1', '1', '0', '2', '145134', '1', '0', 'OO:NN Expel Corruption released 145064 onHit enemy');

-- 144479
REPLACE INTO `areatrigger_data` (`entry`, `radius`, `radius2`, isMoving, `activationDelay`, `updateDelay`, `maxCount`, `customVisualId`, `customEntry`, `comment`) VALUES 
('1080', '100', '0', '1', '0', '0', '0', '32660', '4985', 'OO:NN Expel Corruption 144479');

REPLACE INTO `areatrigger_actions` (`entry`, `id`, `moment`, `actionType`, `targetFlags`, `spellId`, `maxCharges`, `chargeRecoveryTime`, `comment`) VALUES 
('1080', '0', '65', '0', '2', '144480', '0', '0', 'OO:NN Expel Corruption 144479 on enemy between source and dest'),
('1080', '1', 0x2|0x8, '1', '2', '144480', '0', '0', 'OO:NN Expel Corruption 144479 onHit enemy');