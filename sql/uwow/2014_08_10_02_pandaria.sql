/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET NAMES utf8 */;
/*!40014 SET FOREIGN_KEY_CHECKS=0 */;

UPDATE `creature_template` SET `AIName`='SmartAI' WHERE `entry` in (53565, 65469, 57748, 61411, 65471, 54587);
DELETE FROM smart_scripts WHERE entryorguid in (53565, 65469, 57748, 61411, 65471, 54587);
INSERT INTO `smart_scripts` (`entryorguid`, `source_type`, `id`, `link`, `event_type`, `event_phase_mask`, `event_chance`, `event_flags`, `event_param1`, `event_param2`, `event_param3`, `event_param4`, `action_type`, `action_param1`, `action_param2`, `action_param3`, `action_param4`, `action_param5`, `action_param6`, `target_type`, `target_param1`, `target_param2`, `target_param3`, `target_x`, `target_y`, `target_z`, `target_o`, `comment`) VALUES 
(53565, 0, 0, 0, 1, 0, 100, 0, 5000, 5000, 5000, 5000, 5, 543, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'cung-fu panda'),
(65469, 0, 0, 0, 1, 0, 100, 0, 5000, 5000, 5000, 5000, 5, 543, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'cung-fu panda'),
(57748, 0, 0, 0, 1, 0, 100, 0, 4000, 4000, 5000, 5000, 5, 543, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'master cung-fu panda'),
(61411, 0, 0, 0, 1, 0, 100, 0, 4000, 4000, 5000, 5000, 5, 543, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'master cung-fu panda');

INSERT INTO `smart_scripts` (`entryorguid`, `source_type`, `id`, `link`, `event_type`, `event_phase_mask`, `event_chance`, `event_flags`, `event_param1`, `event_param2`, `event_param3`, `event_param4`, `action_type`, `action_param1`, `action_param2`, `action_param3`, `action_param4`, `action_param5`, `action_param6`, `target_type`, `target_param1`, `target_param2`, `target_param3`, `target_x`, `target_y`, `target_z`, `target_o`, `comment`) VALUES
(65471, 0, 0, 0, 25, 0, 100, 0, 0, 0, 0, 0, 42, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Reset - Invincibility HP Level 1'),
(65471, 0, 1, 2, 2, 0, 100, 1, 0, 5, 0, 0, 2, 2263, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'HP <5% - Set Faction To 35'),
(65471, 0, 2, 3, 61, 0, 100, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Link - Talk Random Text'),
(65471, 0, 3, 4, 61, 0, 100, 0, 0, 0, 0, 0, 33, 54586, 0, 0, 0, 0, 0, 7, 0, 0, 0, 0, 0, 0, 0, 'Link - Kill Credit'),
(65471, 0, 4, 0, 61, 0, 100, 0, 0, 0, 0, 0, 41, 3000, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Despawn'),
(65471, 0, 5, 0, 1, 0, 100, 0, 5000, 5000, 5000, 5000, 5, 543, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'cung-fu panda');

INSERT INTO `smart_scripts` (`entryorguid`, `source_type`, `id`, `link`, `event_type`, `event_phase_mask`, `event_chance`, `event_flags`, `event_param1`, `event_param2`, `event_param3`, `event_param4`, `action_type`, `action_param1`, `action_param2`, `action_param3`, `action_param4`, `action_param5`, `action_param6`, `target_type`, `target_param1`, `target_param2`, `target_param3`, `target_x`, `target_y`, `target_z`, `target_o`, `comment`) VALUES
(54587, 0, 0, 0, 25, 0, 100, 0, 0, 0, 0, 0, 42, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Reset - Invincibility HP Level 1'),
(54587, 0, 1, 2, 2, 0, 100, 1, 0, 5, 0, 0, 2, 2263, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'HP <5% - Set Faction To 35'),
(54587, 0, 2, 3, 61, 0, 100, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Link - Talk Random Text'),
(54587, 0, 3, 4, 61, 0, 100, 0, 0, 0, 0, 0, 33, 54586, 0, 0, 0, 0, 0, 7, 0, 0, 0, 0, 0, 0, 0, 'Link - Kill Credit'),
(54587, 0, 4, 0, 61, 0, 100, 0, 0, 0, 0, 0, 41, 3000, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'Despawn'),
(54587, 0, 5, 0, 1, 0, 100, 0, 5000, 5000, 5000, 5000, 5, 543, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 'cung-fu panda');

--
DELETE FROM spell_area WHERE spell in (107027, 107032, 100709, 107028, 100711, 102194, 107033, 102429, 102393, 102395, 114735, 102396, 102397, 
102399, 102400, 102521, 108150, 108879, 102873, 102869, 103051, 108834, 102872, 102874, 102870, 102875, 116571, 102871, 128574, 103538,
114455, 109303, 108835, 108823, 108822, 104018, 118028, 104017, 108844, 108842, 105308, 105307, 105005, 105306) AND  area in(5736, 5862, 5827, 5881, 5826, 5860, 5830, 5946, 5831);
INSERT INTO `spell_area` (`spell`, `area`, `quest_start`, `quest_end`, `aura_spell`, `racemask`, `gender`, `autocast`, `quest_start_status`, `quest_end_status`) VALUES 
('100709', '5736', '0', '29524', '0', '0', '2', '1', '0', '66'),
('107028', '5736', '29406', '29409', '0', '0', '2', '1', '74', '66'),
('107027', '5736', '29406', '29409', '0', '0', '2', '1', '74', '66'),
--
('100711', '5736', '29406', '29409', '0', '0', '2', '1', '66', '66'),
('107032', '5736', '29406', '29409', '0', '0', '2', '1', '66', '66'),
--
('107033', '5736', '29409', '29419', '0', '0', '2', '1', '8', '66'),
('102429', '5736', '29409', '29419', '0', '0', '2', '1', '8', '66'),
--
('102194', '5736', '29409', '29419', '0', '0', '2', '1', '66', '66'),
--
('102393', '5736', '29410', '29419', '0', '0', '2', '1', '66', '66'), -- Aysa of the Tushui
--
('102395', '5736', '29419', '29523', '0', '0', '2', '1', '66', '66'),
('114735', '5736', '29419', '29414', '0', '0', '2', '1', '66', '74'),
--
('102396', '5736', '29414', '29523', '0', '0', '2', '1', '66', '66'),
--
('102397', '5736', '29523', '29521', '0', '0', '2', '1', '66', '74'),
-- 102399
('102399', '5736', '29420', '29521', '0', '0', '2', '1', '74', '74'),
('102400', '5736', '29420', '29521', '0', '0', '2', '1', '74', '74'),
('102521', '5736', '29420', '29521', '0', '0', '2', '1', '74', '74'),
--
-- ('108150', '5736', '29421', '0', '0', '0', '2', '1', '8', '0'),
-- 
('102872', '5736', '29521', '29662', '0', '0', '2', '1', '74', '64'),
('108834', '5860', '29521', '0', '0', '0', '2', '1', '74', '0'),
('108834', '5826', '29521', '0', '0', '0', '2', '1', '74', '0'),
('102869', '5860', '29521', '0', '0', '0', '2', '1', '74', '0'),
('102869', '5826', '29521', '0', '0', '0', '2', '1', '74', '0'),
('108879', '5860', '29521', '0', '0', '0', '2', '1', '74', '0'), --
('108879', '5826', '29521', '0', '0', '0', '2', '1', '74', '0'), --
('102873', '5860', '29521', '0', '0', '0', '2', '1', '74', '0'),
('102873', '5826', '29521', '0', '0', '0', '2', '1', '74', '0'),
('103051', '5826', '29521', '0', '0', '0', '2', '1', '74', '0'),
('103051', '5860', '29521', '0', '0', '0', '2', '1', '74', '0'),
--
('102874', '5736', '29666', '29678', '0', '0', '2', '1', '66', '66'), -- qgiver1
('102870', '5736', '29666', '29678', '0', '0', '2', '1', '66', '66'), -- qgiver2
--
('102875', '5826', '29678', '0', '0', '0', '2', '1', '66', '0'), -- qgiver1
('102875', '5860', '29678', '0', '0', '0', '2', '1', '66', '0'), -- qgiver1
('102871', '5826', '29678', '0', '0', '0', '2', '1', '66', '0'), -- qgiver2
('102871', '5860', '29678', '0', '0', '0', '2', '1', '66', '0'), -- qgiver2
('128574', '5736', '29678', '29679', '0', '0', '2', '1', '66', '66'), -- watter spitit
('116571', '5862', '29678', '0', '0', '0', '2', '1', '74', '0'), -- not remove
--
('103538', '5736', '29679', '29680', '0', '0', '2', '1', '66', '66'), -- Summon Spirit of Water
--
('114455', '5881', '29680', '0', '0', '0', '2', '1', '74', '0'),
('109303', '5881', '29680', '29774', '0', '0', '2', '1', '74', '66'), -- slipping spirit of earth
('108835', '5881', '29680', '0', '0', '0', '2', '1', '74', '0'),
('108823', '5881', '29680', '29771', '0', '0', '2', '1', '74', '66'),
('108822', '5881', '29680', '29768', '0', '0', '2', '1', '74', '66'), -- quest giver
--
('104018', '5881', '29768', '29774', '0', '0', '2', '1', '66', '66'), -- quest giver
('118028', '5881', '29768', '0', '0', '0', '2', '1', '66', '0'), -- quest giver
--
('104017', '5736', '29774', '29775', '0', '0', '2', '1', '66', '66'), -- Summon Spirit of Water and Earth
--
('108844', '5830', '29776', '0', '0', '0', '2', '1', '74', '0'),
('108842', '5830', '29776', '29782', '0', '0', '2', '1', '74', '64'),
('105308', '5830', '29776', '0', '0', '0', '2', '1', '74', '0'), -- aisa
('105308', '5946', '29776', '0', '0', '0', '2', '1', '74', '0'), -- aisa
('105307', '5830', '29776', '0', '0', '0', '2', '1', '74', '0'),
('105005', '5830', '29776', '0', '0', '0', '2', '1', '74', '0'),
--
('105306', '5831', '29780', '29784', '0', '0', '2', '1', '74', '74');

DELETE FROM `conditions` WHERE SourceTypeOrReferenceId = 23 AND `SourceEntry` in(1, 2, 3) AND SourceGroup = 5736;
INSERT INTO `conditions` (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `NegativeCondition`, `ErrorTextId`, `ScriptName`, `Comment`) VALUES
(23, 5736, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 'Pandaren Start loc: world area', NULL),
(23, 5736, 3, 0, 0, 28, 0, 29799, 0, 0, 0, 0, 'Pandaren Start loc: tarrain 976', NULL),
(23, 5736, 3, 0, 1, 8, 0, 29799, 0, 0, 0, 0, 'Pandaren Start loc: tarrain 976', NULL),
(23, 5736, 2, 0, 0, 28, 0, 30767, 0, 0, 0, 0, 'Pandaren Start loc: tarrain 975', NULL),
(23, 5736, 2, 0, 1, 8, 0, 30767, 0, 0, 0, 0, 'Pandaren Start loc: tarrain 975', NULL);

-- World Map Area Id: 683
DELETE FROM phase_definitions WHERE `zoneId` = 5736 AND entry in(1, 2, 3);
INSERT INTO `phase_definitions` (`zoneId`, `entry`, `phasemask`, `phaseId`, `terrainswapmap`, `wmAreaId`, `comment`) VALUES
(5736, 1, 0, 0, 0, 683, 'Pandaren Start loc: world area'),
(5736, 2, 0, 0, 975, 0, 'Pandaren Start loc: tarrain 975'),
(5736, 3, 0, 0, 976, 0, 'Pandaren Start loc: tarrain976');

-- ----------------------------------------
-- Q: 29408 The Lesson of the Burning Scroll
-- ----------------------------------------

-- Hack. На самом деле 59591 этот NPC все должен делать
UPDATE `quest_template` SET `SourceItemId` = '0', `RequiredSourceItemId1` = '80212', `RequiredSourceItemId4` = '0' WHERE `Id` = 29408;
UPDATE `creature_template` SET `IconName` = 'openhand', `npcflag` =  npcflag | 16777216, `unit_flags` = '295680' WHERE `entry` = 53566;
UPDATE  `creature_template` SET  `ScriptName` =  'mob_master_shang_xi' WHERE  `entry` =53566;
DELETE FROM `npc_spellclick_spells` WHERE npc_entry = 53566;
INSERT INTO `npc_spellclick_spells` (`npc_entry`, `spell_id`, `cast_flags`, `user_type`) VALUES
(53566, 114746, 1, 1);
DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId` = 18 AND `SourceEntry` = 114746;
INSERT INTO `conditions` (`SourceTypeOrReferenceId`,`SourceGroup`,`SourceEntry`,`ConditionTypeOrReference`, `ConditionTarget`,`ConditionValue1`,`ConditionValue2`,`NegativeCondition`,`Comment`)
VALUES (18, 53566, 114746, 9, 0, 29408, 0, 0, 'Required quest active for spellclick');

DELETE FROM `creature_text` WHERE entry =53566;
INSERT INTO `creature_text` (`entry`, `groupid`, `id`, `text`, `type`, `language`, `probability`, `emote`, `duration`, `sound`, `comment`) VALUES
(53566, 0, 0, 'С первой попытки! Теперь отнеси пламя на самый верх храма и сожги свиток, который ты найдешь там.', 12, 0, 100, 1, 0, 33645, 'Мастер Шан Си');

-- ----------------------------------------
-- Q: 29409 The Disciple's Challenge
-- ----------------------------------------
UPDATE `quest_template` SET `RequiredPOI1` = '252375', `RequiredUnkFlag1` = '1' WHERE `Id` = 29409;
DELETE FROM smart_scripts WHERE entryorguid = 54611;
UPDATE `creature_template` SET `AIName` = '', `ScriptName` = 'boss_jaomin_ro' WHERE  `entry` = 54611;

DELETE FROM `locales_creature_text` WHERE `entry` = 54611;
DELETE FROM `creature_text` WHERE entry =54611;
INSERT INTO `creature_text` (`entry`, `groupid`, `id`, `text`, `type`, `language`, `probability`, `emote`, `duration`, `sound`, `comment`) VALUES
(54611, 0, 0, 'А вот и мой соперник.', 12, 0, 100, 113, 0, 0, 'Цзяоминь Жо'),
(54611, 1, 0, 'Поразительно! Ты куда сильнее, чем кажешься.', 12, 0, 100, 2, 0, 0, 'Цзяоминь Жо'),
(54611, 1, 1, 'Тебя хорошо обучили.', 12, 0, 100, 2, 0, 0, 'Цзяоминь Жо'),
(54611, 1, 2, 'Я преклоняюсь перед твоим мастерством. Я уступил в честном бою.', 12, 0, 100, 2, 0, 0, 'Цзяоминь Жо'),
(54611, 1, 3, 'Похоже, мне придется еще как следует потренироваться. Благодарю за урок.', 12, 0, 100, 2, 0, 0, 'Цзяоминь Жо'),
(54611, 1, 4, 'Быть побежденным тобой – большая честь.', 12, 0, 100, 2, 0, 0, 'Цзяоминь Жо'),
(54611, 1, 5, 'Прекрасный бой. Мастер Шан будет доволен.', 12, 0, 100, 2, 0, 0, 'Цзяоминь Жо');

UPDATE `creature_template` SET `AIName` = '', `ScriptName` = 'npc_panda_announcer' WHERE `entry` = 60183;
DELETE FROM `creature_text` WHERE entry =60183;
INSERT INTO `creature_text` (`entry`, `groupid`, `id`, `text`, `type`, `language`, `probability`, `emote`, `duration`, `sound`, `comment`) VALUES
(60183, 0, 0, 'Надеюсь, ты уже $gготов:готова; к испытанию, $p. Цзяоминь Жо ожидает тебя по ту сторону моста.', 12, 0, 100, 113, 0, 33643, 'Ученик Ним');

-- ----------------------------------------
-- Q: 29409 The Disciple's Challenge
-- ----------------------------------------
UPDATE `creature_template` SET `AIName` = '', `ScriptName` = 'npc_panda_announcer' WHERE `entry` = 60244;
DELETE FROM `creature_text` WHERE entry =60244;
INSERT INTO `creature_text` (`entry`, `groupid`, `id`, `text`, `type`, `language`, `probability`, `emote`, `duration`, `sound`, `comment`) VALUES
(60244, 0, 0, 'Уже покидаешь нас? Завидую твоей доблести. Счастливого пути, |3-6($c).', 12, 0, 100, 2, 0, 33643, 'Ученик Гуан');

UPDATE `creature_template` SET `AIName` = '', `ScriptName` = 'npc_panda_announcer' WHERE `entry` = 54943;
DELETE FROM `creature_text` WHERE entry =54943;
INSERT INTO `creature_text` (`entry`, `groupid`, `id`, `text`, `type`, `language`, `probability`, `emote`, `duration`, `sound`, `comment`) VALUES
(54943, 0, 0, 'Тс-с-с! Тихо. Она тренируется.', 12, 0, 100, 396, 0, 0, 'Торговец Лорво');


-- ----------------------------------------
-- Q: 29419 The Missing Driver
-- ----------------------------------------
DELETE FROM smart_scripts WHERE entryorguid = 54855;
UPDATE `creature_template` SET `AIName` = '', `ScriptName` = 'mob_min_dimwind' WHERE `entry` = 54855;
UPDATE `creature_template` SET `ScriptName` = 'mob_attacker_dimwind' WHERE `creature_template`.`entry` = 54130;
UPDATE `creature_template` SET `unit_flags` = '33544', `unit_flags2` = '2048', `ScriptName` = 'npc_min_dimwind_outro' WHERE `entry` = 56503;

DELETE FROM `creature_text` WHERE entry =56503;
INSERT INTO `creature_text` (`entry`, `groupid`, `id`, `text`, `type`, `language`, `probability`, `emote`, `duration`, `sound`, `comment`) VALUES
(56503, 0, 0, 'Мастер Шан может гордиться $gтаким учеником:такой ученицей;. Спасибо, друг!', 12, 0, 100, 2, 0, 0, 'Минь Попутный Ветер'),
(56503, 1, 0, 'Я не смог бы в одиночку справиться с ними. А теперь, если ты не против, я пойду искать свою телегу.', 12, 0, 100, 1, 0, 0, 'Минь Попутный Ветер'),
(56503, 2, 0, 'Телега!', 14, 0, 100, 22, 0, 0, 'Минь Попутный Ветер'),
(56503, 3, 0, 'Здравствуй, тележка. Так и лежишь перевернутая?..', 12, 0, 100, 1, 0, 0, 'Минь Попутный Ветер');

SET @id = 0;
SET @entry = 56503;
DELETE FROM `script_waypoint` WHERE `entry` = @entry;
INSERT INTO `script_waypoint` (`entry`, `pointid`, `location_x`, `location_y`, `location_z`, `point_comment`) VALUES 
(@entry, @id := @id+ 1, 1409.319, 3536.138, 86.87651, NULL), 
(@entry, @id := @id+ 1, 1411.819, 3534.638, 86.12651, NULL), 
(@entry, @id := @id+ 1, 1333.938, 3564.673, 93.37936, NULL), 
(@entry, @id := @id+ 1, 1335.188, 3568.423, 92.87936, NULL),
(@entry, @id := @id+ 1, 1325.719, 3552.776, 96.56885, NULL),
(@entry, @id := @id+ 1, 1286, 3522, 98, NULL);

DELETE FROM `creature_text` WHERE entry =54130;
INSERT INTO `creature_text` (`entry`, `groupid`, `id`, `text`, `type`, `language`, `probability`, `emote`, `duration`, `sound`, `comment`) VALUES
(54130, 0, 0, 'Берегись! Кажется, прибежали его дружки!', 14, 0, 100, 2, 0, 0, 'Проказник из Янтарного Листа');

-- ----------------------------------------
-- Q: 29414  The Way of the Tushui
-- ----------------------------------------

DELETE FROM `creature_involvedrelation` WHERE `quest` = 29414 AND id = 54567;

UPDATE `quest_template` SET `Flags` = '1048576', `Method` = '2', `StartScript` = '29414' WHERE `Id` = 29414;
DELETE FROM `quest_start_scripts` WHERE id = 29414;
INSERT INTO `quest_start_scripts` (`id`, `delay`, `command`, `datalong`, `datalong2`, `dataint`, `x`, `y`, `z`, `o`) VALUES 
('29414', '0', '15', '114728', '2', '0', '0', '0', '0', '0');

UPDATE `creature_template` SET `ScriptName` = 'mob_aysa_lake_escort' WHERE `creature_template`.`entry` = 59652;
DELETE FROM `script_waypoint` WHERE `entry` = 59652;
INSERT INTO `script_waypoint` (`entry`, `pointid`, `location_x`, `location_y`, `location_z`, `waittime`, `point_comment`) VALUES
(59652, 1, 1225.24, 3449.83, 102.426, 0, ''),
(59652, 2, 1204.46, 3444.84, 102.409, 0, ''),
(59652, 3, 1177.37, 3444.32, 103.093, 0, ''),
(59652, 4, 1165.1, 3441.25, 104.974, 0, '');

DELETE FROM `creature_text` WHERE entry =59652;
INSERT INTO `creature_text` (`entry`, `groupid`, `id`, `text`, `type`, `language`, `probability`, `emote`, `duration`, `sound`, `comment`) VALUES
(59652, 0, 0, 'Встретимся в пещере наверху.', 12, 0, 100, 396, 0, 27397, 'Аиса Воспевающая Облака');

UPDATE `creature_template` SET `ScriptName` = 'mob_aysa' WHERE `entry` = 59642;
DELETE FROM `creature_text` WHERE entry =59642;
INSERT INTO `creature_text` (`entry`, `groupid`, `id`, `text`, `type`, `language`, `probability`, `emote`, `duration`, `sound`, `comment`) VALUES
(59642, 0, 0, 'Не дайте этим созданиям прервать мою медитацию. Скоро мы получим ответы на все наши вопросы.', 12, 0, 100, 0, 0, 27398, 'Аиса Воспевающая Облака'),
(59642, 1, 0, 'Итак, мы теперь знаем, что нам предстоит. Обратитесь к мастеру Шан Си. Он раскажет тебе, что делать дальше.', 12, 0, 100, 0, 0, 0, 'Аиса Воспевающая Облака');

DELETE FROM `creature_text` WHERE entry =54856;
INSERT INTO `creature_text` (`entry`, `groupid`, `id`, `text`, `type`, `language`, `probability`, `emote`, `duration`, `sound`, `comment`) VALUES
(54856, 0, 0, 'Мастер Ли Фэй говорит: \"Хо, дух огня, известен своим чувством голода. Он нуждается в древесине – его пище. Он нуждается в ласке ветра, что пробуждает его\".', 16, 0, 100, 0, 0, 0, 'Мастер Ли Фэй'),
(54856, 1, 0, 'Мастер Ли Фэй говорит: \"Если ты найдешь все это и принесешь к его пещере в дальнем конце деревни У-Сун, там тебя будет ждать испытание\".', 16, 0, 100, 0, 0, 0, 'Мастер Ли Фэй'),
(54856, 2, 0, 'Мастер Ли Фэй говорит: \"Путь Тушуй... это просветление через терпение и медитацию... жизнь, основанная на твердых принципах\".', 16, 0, 100, 6, 0, 0, 'Мастер Ли Фэй'),
(54856, 3, 0, 'Мастер Ли Фэй говорит: \"Пройди это испытание, и тебе явится Хо. Разожги его пламя, и если дух твой чист, Хо последует за тобой\".', 16, 0, 100, 0, 0, 0, 'Мастер Ли Фэй'),
(54856, 4, 0, 'Мастер Ли Фэй говорит: \"Приятно видеть тебя снова, Аиса. Ты отдала дань уважения, и посему я дарую тебе ответы, которые ты ищешь\".', 16, 0, 100, 1, 0, 0, 'Мастер Ли Фэй'),
(54856, 5, 0, 'Мастер Ли Фэй говорит: \"Ступайте же, дети мои. Мы встретимся вновь и очень скоро\".', 16, 0, 100, 0, 0, 0, 'Мастер Ли Фэй'),
(54856, 6, 0, 'Мастер Ли Фэй растворяется в воздухе.', 16, 0, 100, 0, 0, 0, 'Мастер Ли Фэй');

-- ----------------------------------------
-- Q: 29417  The Way of the Huojin
-- ----------------------------------------
UPDATE `creature_template` SET `ScriptName` = 'npc_panda_announcer' WHERE `creature_template`.`entry` = 54568;
DELETE FROM `creature_text` WHERE entry =54568;
INSERT INTO `creature_text` (`entry`, `groupid`, `id`, `text`, `type`, `language`, `probability`, `emote`, `duration`, `sound`, `comment`) VALUES
(54568, 0, 0, 'Получи, обезьянья морда!', 12, 0, 100, 5, 0, 27344, 'Цзи Огненная Лапа');

-- ----------------------------------------
-- Q: 29523  Fanning the Flames
-- ----------------------------------------
UPDATE `creature_template` SET `ScriptName` = 'boss_living_air' WHERE `entry` = 54631;

-- ----------------------------------------
-- Q: 29421 Only the Worthy Shall Pass
-- ----------------------------------------
UPDATE `creature_template` SET `ScriptName` = 'boss_li_fei' WHERE  `entry` = 54135;
UPDATE `creature_template` SET `ScriptName` = 'boss_li_fei_fight' WHERE `entry` = 54734;

DELETE FROM `creature_text` WHERE entry =54135;
INSERT INTO `creature_text` (`entry`, `groupid`, `id`, `text`, `type`, `language`, `probability`, `emote`, `duration`, `sound`, `comment`) VALUES
(54135, 0, 0, 'Ты $gзаслужил:заслужила; право пройти. Хо ждет тебя.', 12, 0, 100, 0, 0, 0, 'Мастер Ли Фэй');

-- ----------------------------------------
-- Q: 29423 The Passion of Shen-zin Su
-- ----------------------------------------


-- ----------------------------------------
-- Q: 29521 The Singing Pools
-- ----------------------------------------
UPDATE `creature_template` SET `ScriptName` = 'npc_childrens_going_to_east' WHERE  `entry` in (60250, 60249);

DELETE FROM `creature_text` WHERE entry =60250;
INSERT INTO `creature_text` (`entry`, `groupid`, `id`, `text`, `type`, `language`, `probability`, `emote`, `duration`, `sound`, `comment`) VALUES
(60250, 0, 0, 'Эй! Это благодаря тебе дух огня снова у нас?', 12, 0, 100, 3, 0, 0, 'Цай'),
(60250, 1, 0, 'Конечно же он был горячим. Не будь дураком, Дэн.', 12, 0, 100, 5, 0, 0, 'Цай'),
(60250, 2, 0, 'Ну-у, Дэн. Готов поспорить, тебя запросто можно запустить пинком во-он за тот холм! Ха!', 12, 0, 100, 274, 0, 0, 'Цай'),
(60250, 3, 0, 'Пока.', 12, 0, 100, 3, 0, 0, 'Цай');

DELETE FROM `creature_text` WHERE entry =60249;
INSERT INTO `creature_text` (`entry`, `groupid`, `id`, `text`, `type`, `language`, `probability`, `emote`, `duration`, `sound`, `comment`) VALUES
(60249, 0, 0, 'А он был горячий? Готов поспорить, что да!', 12, 0, 100, 6, 0, 0, 'Дэн'),
(60249, 1, 0, 'Готов поспорить, силищи тебе не занимать, а? Ты, наверно, можешь одним ударом разломить пополам вон тот мост!', 12, 0, 100, 6, 0, 0, 'Дэн'),
(60249, 2, 0, 'Прощай!', 12, 0, 100, 3, 0, 0, 'Дэн');

DELETE FROM `areatrigger_scripts`  WHERE `ScriptName` LIKE 'at_going_to_east';
INSERT INTO `areatrigger_scripts` (`entry`, `ScriptName`) VALUES ('7858', 'at_going_to_east');

-- ----------------------------------------
-- Q: 29521 The Singing Pools
-- ----------------------------------------
UPDATE `creature_template` SET `ScriptName` = 'vehicle_balance_pole' WHERE  `entry` in (54993, 55083, 57431);
UPDATE `creature_template` SET `ScriptName` = 'mob_tushui_monk' WHERE  `entry` in (55019, 65468);
UPDATE `creature_template` SET `IconName` = 'vehichleCursor' WHERE `entry` = 57626;
DELETE FROM `npc_spellclick_spells` WHERE npc_entry = 57626;
INSERT INTO `npc_spellclick_spells` (`npc_entry`, `spell_id`, `cast_flags`, `user_type`) VALUES
(57626, 46598, 1, 0);
UPDATE `creature_template` SET `AIName` = 'AggressorAI' WHERE `creature_template`.`entry` = 55015;

-- ----------------------------------------
-- Q: 29662 Stronger Than Reeds
-- ----------------------------------------

UPDATE `creature_template` SET `AIName`='SmartAI' WHERE `entry`=55021;
DELETE FROM smart_scripts WHERE entryorguid = 55021;
INSERT INTO `smart_scripts` (`entryorguid`, `source_type`, `id`, `link`, `event_type`, `event_phase_mask`, `event_chance`, `event_flags`, `event_param1`, `event_param2`, `event_param3`, `event_param4`, `action_type`, `action_param1`, `action_param2`, `action_param3`, `action_param4`, `action_param5`, `action_param6`, `target_type`, `target_param1`, `target_param2`, `target_param3`, `target_x`, `target_y`, `target_z`, `target_o`, `comment`) VALUES
(55021, 0, 0, 0, 50, 0, 100, 0, 29662, 0, 0, 0, 11, 108786, 0, 0, 0, 0, 0, 7, 0, 0, 0, 0, 0, 0, 0, 'At finish q: 29662');

-- cast 129272 Jojo Ironbrow
UPDATE `creature_template` SET `ScriptName` = 'mob_jojo_ironbrow_1' WHERE `creature_template`.`entry` = 57638;

DELETE FROM `creature_text` WHERE entry =57638;
INSERT INTO `creature_text` (`entry`, `groupid`, `id`, `text`, `type`, `language`, `probability`, `emote`, `duration`, `sound`, `comment`) VALUES
(57638, 0, 0, 'Тростник Поющих прудов – прочнейший в этих краях, но для моего лба даже он – не прочнее воздуха.', 12, 0, 100, 1, 0, 0, 'Йо-Йо Железная Бровь'),
(57638, 1, 0, 'Многие пытались испытать меня, но я несокрушим.', 12, 0, 100, 1, 0, 0, 'Йо-Йо Железная Бровь');

DELETE FROM `conditions` WHERE SourceEntry = 108798;
INSERT INTO `conditions` (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `NegativeCondition`, `ErrorTextId`, `ScriptName`, `Comment`) VALUES
(13, 1, 108798, 0, 0, 31, 0, 3, 57636, 0, 0, 0, '', 'Cast - Only - 57636');

DELETE FROM `spell_target_position` WHERE id in (108786, 108808);
INSERT INTO `spell_target_position` (`id`, `target_map`, `target_position_x`, `target_position_y`, `target_position_z`, `target_orientation`) VALUES 
('108786', '860', '01038.554', '3286.385', '129.1765', '1.815142'),
('108808', '860', '1039.491', '3283.111', '129.5231', '1.815142');

-- ----------------------------------------
-- Q: 29678 Shu, the Spirit of Water
-- ----------------------------------------
UPDATE `quest_template` SET `Flags` = '327688', `RequiredIdCount1` = '1', `RequiredIdCount2` = '1', `RequiredPOI1` = '251733', `RequiredPOI2` = '251734' WHERE `Id` = 29678;
UPDATE `creature_template` SET `AIName` = '' WHERE `entry` = 65493;
DELETE FROM smart_scripts WHERE entryorguid = 65493;

DELETE FROM `creature_involvedrelation` WHERE `quest` = 29678; -- autosubmit

-- ----------------------------------------
-- Q: 29679 A New Friend
-- ----------------------------------------
DELETE FROM `creature_questrelation` WHERE `quest` = 29679;
DELETE FROM `area_queststart` WHERE id = 5862
INSERT INTO `area_queststart` (`id`, `quest`) VALUES ('5862', '29679');

DELETE FROM `creature_text` WHERE entry = 54975;
INSERT INTO `creature_text` (`entry`, `groupid`, `id`, `text`, `type`, `language`, `probability`, `emote`, `duration`, `sound`, `comment`) VALUES 
(54975, 0, 0, 'Должна признать, за вами было интересно наблюдать!', 12, 0, 100, 0, 0, 27394, 'Аиса Воспевающая Облака'),
(54975, 1, 0, 'И мне кажется, у тебя появился новый друг.', 12, 0, 100, 0, 0, 27395, 'Аиса Воспевающая Облака');

DELETE FROM `spell_script_names` WHERE `spell_id` = 103538;
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES ('103538', 'spell_summon_spirit_of_watter');

UPDATE `creature_template` SET `ScriptName` = 'mob_aysa_cloudsinger_watter_outro' WHERE `entry` = 54975;

-- ----------------------------------------
-- Q: 29680 The Source of Our Livelihood
-- ----------------------------------------
UPDATE `creature_template` SET `IconName` = 'vehichleCursor' WHERE `entry` in (57710, 57741, 59497);
DELETE FROM `npc_spellclick_spells` WHERE `npc_entry` in (57710, 57741, 59497);
INSERT INTO `npc_spellclick_spells` (`npc_entry`, `spell_id`, `cast_flags`, `user_type`) VALUES
(57710, 115904, 1, 0),
(57741, 115904, 1, 0),
(59497, 115904, 1, 0);

UPDATE `creature_template` SET `ScriptName` = 'vehicle_carriage' WHERE `creature_template`.`entry` = 57208;
DELETE FROM `spell_script_names`  WHERE `spell_id` = 115904;
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES ('115904', 'spell_grab_carriage');

UPDATE `creature_template` SET `VehicleId` = '1944' WHERE `entry` = 57208;

UPDATE `creature_template_addon` SET `auras` = '' WHERE `entry` = 59499;

UPDATE `creature_template` SET `ScriptName` = 'npc_nourished_yak' WHERE `entry` in (57207, 59499, 57743);
SET @id = 0;
SET @entry = 57207;
DELETE FROM `script_waypoint` WHERE `entry` = @entry;
INSERT INTO `script_waypoint` (`entry`, `pointid`, `location_x`, `location_y`, `location_z`, `point_comment`) VALUES 
(@entry, @id := @id+ 1, 979.338, 2852.28, 87.276, NULL), 
(@entry, @id := @id+ 1, 892.593, 2810.91, 86.4438, NULL), 
(@entry, @id := @id+ 1, 875.519, 2806.39, 81.961, NULL), 
(@entry, @id := @id+ 1, 854.392, 2800.24, 83.5591, NULL), 
(@entry, @id := @id+ 1, 801.608, 2784.76, 76.2802, NULL), 
(@entry, @id := @id+ 1, 749.569, 2827.86, 75.4519, NULL), 
(@entry, @id := @id+ 1, 746.148, 2851.48, 75.5912, NULL), 
(@entry, @id := @id+ 1, 755.357, 2883.45, 74.7007, NULL), 
(@entry, @id := @id+ 1, 742.717, 2904.04, 74.6945, NULL), 
(@entry, @id := @id+ 1, 680.361, 2958.9, 74.9462, NULL), 
(@entry, @id := @id+ 1, 666.782, 2981.86, 74.6082, NULL), 
(@entry, @id := @id+ 1, 659.627, 2990.9, 79.4118, NULL), 
(@entry, @id := @id+ 1, 652.523, 2998.18, 74.6114, NULL), 
(@entry, @id := @id+ 1, 626.525, 3018.83, 74.9584, NULL), 
(@entry, @id := @id+ 1, 615.27, 3036.96, 76.3462, NULL), 
(@entry, @id := @id+ 1, 614.827, 3097.02, 86.6553, NULL), 
(@entry, @id := @id+ 1, 617.181, 3139.57, 87.7514, NULL);

SET @id = 0;
SET @entry = 59499;
DELETE FROM `script_waypoint` WHERE `entry` = @entry;
INSERT INTO `script_waypoint` (`entry`, `pointid`, `location_x`, `location_y`, `location_z`, `point_comment`) VALUES 
(@entry, @id := @id+ 1, 577.366, 3148.85, 87.346, NULL), 
(@entry, @id := @id+ 1, 554.24, 3161.87, 77.325, NULL), 
(@entry, @id := @id+ 1, 538.584, 3211.28, 75.9284, NULL), 
(@entry, @id := @id+ 1, 512.599, 3230.89, 74.053, NULL), 
(@entry, @id := @id+ 1, 508.76, 3260.13, 77.5314, NULL), 
(@entry, @id := @id+ 1, 519.125, 3313.21, 73.1593, NULL), 
(@entry, @id := @id+ 1, 524.149, 3328.19, 78.111, NULL), 
(@entry, @id := @id+ 1, 551.182, 3363.74, 77.8148, NULL), 
(@entry, @id := @id+ 1, 647.98, 3400.74, 97.0092, NULL), 
(@entry, @id := @id+ 1, 683.443, 3440.83, 110.622, NULL), 
(@entry, @id := @id+ 1, 696.219, 3472.66, 118.161, NULL), 
(@entry, @id := @id+ 1, 745.56, 3496.28, 135.538, NULL), 
(@entry, @id := @id+ 1, 756.682, 3523.21, 139.03, NULL), 
(@entry, @id := @id+ 1, 742.424, 3597.72, 140.545, NULL);


UPDATE `creature_template` SET `AIName` = '', `ScriptName` = 'npc_panda_announcer' WHERE `entry` = 57712;
DELETE FROM `creature_text` WHERE entry = 54975;
INSERT INTO `creature_text` (`entry`, `groupid`, `id`, `text`, `type`, `language`, `probability`, `emote`, `duration`, `sound`, `comment`) VALUES 
(57712, 0, 0, 'Здравствуй, друг! Если хочешь, можешь взять мою повозку. Она отвезет тебя на крестьянский двор Дай-Ло.', 12, 0, 100, 3, 0, 0, 'Хозяин повозки'),
(57712, 1, 0, 'Здравствуй, друг! Если хочешь, можешь взять мою повозку. Она отвезет тебя в Храм Пяти Рассветов.', 12, 0, 100, 3, 0, 0, 'Хозяин повозки');

-- ----------------------------------------
-- Q: 29680 Missing Mallet
-- ----------------------------------------
DELETE FROM `creature_text` WHERE entry = 54975;
INSERT INTO `creature_text` (`entry`, `groupid`, `id`, `text`, `type`, `language`, `probability`, `emote`, `duration`, `sound`, `comment`) VALUES 
(55477, 0, 0, 'Проснись уже!', 12, 0, 100, 15, 0, 27345, 'Цзи Огненная Лапа'),
(55477, 1, 0, 'Да', 12, 0, 100, 509, 0, 27350, 'Цзи Огненная Лапа'),
(55477, 2, 0, 'когда же', 12, 0, 100, 507, 0, 27352, 'Цзи Огненная Лапа'),
(55477, 3, 0, '%s вздыхает.', 16, 0, 100, 0, 0, 0, 'Цзи Огненная Лапа');

-- ----------------------------------------
-- Q: 29771 Stronger Than Wood
-- ----------------------------------------
UPDATE `creature_template` SET `AIName`='SmartAI' WHERE `entry`=55478;
DELETE FROM smart_scripts WHERE entryorguid = 55478;
INSERT INTO `smart_scripts` (`entryorguid`, `source_type`, `id`, `link`, `event_type`, `event_phase_mask`, `event_chance`, `event_flags`, `event_param1`, `event_param2`, `event_param3`, `event_param4`, `action_type`, `action_param1`, `action_param2`, `action_param3`, `action_param4`, `action_param5`, `action_param6`, `target_type`, `target_param1`, `target_param2`, `target_param3`, `target_x`, `target_y`, `target_z`, `target_o`, `comment`) VALUES
(55478, 0, 0, 0, 50, 0, 100, 0, 29771, 0, 0, 0, 11, 108827, 0, 0, 0, 0, 0, 7, 0, 0, 0, 0, 0, 0, 0, 'At finish q: 29662');

-- cast 129272 Jojo Ironbrow
UPDATE `creature_template` SET `ScriptName` = 'mob_jojo_ironbrow_2' WHERE `entry` = 57669;

DELETE FROM `creature_text` WHERE entry = 54975;
INSERT INTO `creature_text` (`entry`, `groupid`, `id`, `text`, `type`, `language`, `probability`, `emote`, `duration`, `sound`, `comment`) VALUES 
(57669, 0, 0, 'Наши ремесленники выстругивают отличные доски. Эти доски могут выдержать даже самую сильную бурю. Для моего сокрушительного черепа они не прочнее воды.', 12, 0, 100, 1, 0, 0, 'Йо-Йо Железная Бровь'),
(57669, 1, 0, 'Пандаренам не по силам соорудить барьер, который остановил бы меня.', 12, 0, 100, 2, 0, 0, 'Йо-Йо Железная Бровь');

DELETE FROM `conditions` WHERE SourceEntry = 108831;
INSERT INTO `conditions` (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `NegativeCondition`, `ErrorTextId`, `ScriptName`, `Comment`) VALUES
(13, 1, 108831, 0, 0, 31, 0, 3, 57667, 0, 0, 0, '', 'Cast - Only - 57667');

DELETE FROM `spell_target_position` WHERE id in (108830, 108827);
INSERT INTO `spell_target_position` (`id`, `target_map`, `target_position_x`, `target_position_y`, `target_position_z`, `target_orientation`) VALUES 
('108830', '860', '598.3958', '3132.111', '89.17931', '0.122173'),
('108827', '860', '601.6198', '3132.89', '89.0976', '3.316126');

-- ----------------------------------------
-- Q: 29774 Not In the Face!
-- ----------------------------------------
UPDATE `creature_template` SET `AIName` = '', `ScriptName` = 'npc_water_spirit_dailo' WHERE  `entry` = 55556;
DELETE FROM smart_scripts WHERE entryorguid = 55556;

DELETE FROM `areatrigger_scripts`  WHERE `ScriptName` LIKE 'AreaTrigger_at_middle_temple_from_east';
INSERT INTO `areatrigger_scripts` (`entry`, `ScriptName`) VALUES ('8588', 'AreaTrigger_at_middle_temple_from_east');

-- ----------------------------------------
-- Q: 29776 Morning Breeze Village
-- ----------------------------------------
UPDATE `creature_template` SET `gossip_menu_id` = '13158' WHERE `entry` = 54786;
DELETE FROM `gossip_menu` WHERE `entry`= 13158;
INSERT INTO `gossip_menu` (`entry`, `text_id`) VALUES
(13158, 18536); -- 54786

DELETE FROM `gossip_menu_option` WHERE `menu_id`=13158;
INSERT INTO `gossip_menu_option` (`menu_id`, `id`, `option_icon`, `option_text`, `option_id`, `npc_option_npcflag`, `action_menu_id`, `action_poi_id`, `box_coded`, `box_money`, `box_text`) VALUES
(13158, 0, 0, 'Я хочу вернуться на вершину храма!', 1, 2, 0, 0, 0, 0, '');
DELETE FROM `conditions` WHERE SourceTypeOrReferenceId = 15 AND SourceGroup = 13158;
INSERT INTO `conditions` (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `NegativeCondition`, `ErrorTextId`, `ScriptName`, `Comment`) VALUES 
('15', '13158', '0', '0', '0', '9', '0', '29776', '0', '0', '0', '0', '', NULL),
('15', '13158', '0', '0', '1', '28', '0', '29776', '0', '0', '0', '0', '', NULL),
('15', '13158', '0', '0', '2', '8', '0', '29776', '0', '0', '0', '0', '', NULL);

DELETE FROM `conditions` WHERE SourceEntry = 104396;
INSERT INTO `conditions` (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `NegativeCondition`, `ErrorTextId`, `ScriptName`, `Comment`) VALUES
(13, 1, 104396, 0, 0, 31, 0, 4, 0, 0, 0, 0, '', 'Cast - Only on Player'),
(13, 2, 104396, 0, 0, 31, 0, 4, 0, 0, 0, 0, '', 'Cast - Only on Player');

DELETE FROM `spell_target_position` WHERE id = 104450;
INSERT INTO `spell_target_position` (`id`, `target_map`, `target_position_x`, `target_position_y`, `target_position_z`, `target_orientation`) VALUES 
('104450', '860', '909.137', '3610.38', '252.092', '3.997228');

UPDATE `creature_template` SET `InhabitType` = '4', `VehicleId` = '1800', `ScriptName` = 'npc_wind_vehicle' WHERE `creature_template`.`entry` = 55685;
DELETE FROM `npc_spellclick_spells` WHERE `npc_entry` = 55685;
INSERT INTO `npc_spellclick_spells` (`npc_entry`, `spell_id`, `cast_flags`, `user_type`) VALUES ('55685', '46598', '1', '0');

SET @id = 0;
SET @entry = 55685;
DELETE FROM `script_waypoint` WHERE `entry` = @entry;
INSERT INTO `script_waypoint` (`entry`, `pointid`, `location_x`, `location_y`, `location_z`, `point_comment`) VALUES 
(@entry, @id := @id+ 1, 922.8226, 3604.356, 196.415, NULL), 
(@entry, @id := @id+ 1, 923.8143, 3604.228, 196.415, NULL), 
(@entry, @id := @id+ 1, 930.6059, 3609.285, 201.1691, NULL), 
(@entry, @id := @id+ 1, 938.9011, 3615.786, 203.6904, NULL), 
(@entry, @id := @id+ 1, 950.2917, 3614.42, 204.5032, NULL), 
(@entry, @id := @id+ 1, 960.1268, 3607.947, 209.4728, NULL), 
(@entry, @id := @id+ 1, 951.7205, 3595.486, 218.8211, NULL), 
(@entry, @id := @id+ 1, 941.7274, 3596.609, 228.3404, NULL), 
(@entry, @id := @id+ 1, 932.559, 3604.833, 235.3123, NULL), 
(@entry, @id := @id+ 1, 939.1528, 3613.667, 242.6738, NULL), 
(@entry, @id := @id+ 1, 946.7136, 3611.303, 250.3387, NULL), 
(@entry, @id := @id+ 1, 948.6129, 3602.809, 255.2892, NULL), 
(@entry, @id := @id+ 1, 942.1215, 3596.853, 257.2684, NULL), 
(@entry, @id := @id+ 1, 933.0104, 3600.142, 256.6022, NULL), 
(@entry, @id := @id+ 1, 920.4496, 3604.771, 253.1732, NULL), 
(@entry, @id := @id+ 1, 920.4496, 3604.771, 253.1732, NULL);

UPDATE `creature_template` SET `AIName`='', `ScriptName` = 'mob_master_shang_xi_temple' WHERE `entry` = 54786;
DELETE FROM smart_scripts WHERE entryorguid = 54786;

DELETE FROM `creature_text` WHERE entry =54786;
INSERT INTO `creature_text` (`entry`, `groupid`, `id`, `text`, `type`, `language`, `probability`, `emote`, `duration`, `sound`, `comment`) VALUES
(54786, 0, 0, 'Добро пожаловать, Хо. Наш народ соскучился по твоему теплу.', 12, 0, 100, 2, 0, 27788, 'Мастер Шан Си'),
(54786, 1, 0, 'Тебе удалось справиться со всеми моими испытаниями. У тебя получилось найти Хо и благополучно привести его в храм.', 12, 0, 100, 1, 0, 27789, 'Мастер Шан Си'),
(54786, 2, 0, 'Нам предстоит решить гораздо более сложную задачу, мои ученики. Шэнь-Цзынь Су страдает от боли. Если мы ничего не предпримем, сама земля, на которой мы стоим, может погибнуть. И мы вместе с ней.', 12, 0, 100, 1, 0, 27790, 'Мастер Шан Си'),
(54786, 3, 0, 'Мы должны поговорить с Шэнь-Цзынь Су и узнать, как исцелить его. А для этого нужно вернуть на свои места четырех духов стихий. Хо был первым.', 12, 0, 100, 1, 0, 27791, 'Мастер Шан Си'),
(54786, 4, 0, 'Цзи, я хочу, чтобы ты отправился на крестьянский двор Дай-Ло и разыскал Угоу, духа земли.', 12, 0, 100, 1, 0, 27792, 'Мастер Шан Си'),
(54786, 5, 0, 'Аиса, я хочу, чтобы ты отправилась к Поющим прудам и разыскала Шу, духа воды.', 12, 0, 100, 1, 0, 27793, 'Мастер Шан Си'),
(54786, 6, 0, 'Ты же будешь направлять наши усилия. Поговори со мной перед тем, как отправиться на восток, к Поющим прудам, и встретиться с Аисой.', 12, 0, 100, 1, 0, 27794, 'Мастер Шан Си'),
(54786, 7, 0, 'Ты снова здесь, вместе с духами воды и земли. Старый учитель гордится тобой.', 12, 0, 100, 1, 0, 27776, 'Мастер Шан Си');

UPDATE `creature_template` SET `ScriptName` = 'npc_panda_announcer' WHERE `entry` = 55694;
DELETE FROM `creature_text` WHERE entry =55694;
INSERT INTO `creature_text` (`entry`, `groupid`, `id`, `text`, `type`, `language`, `probability`, `emote`, `duration`, `sound`, `comment`) VALUES
(55694, 0, 0, 'Эй! Деревня Утреннего Бриза – это сюда.', 12, 0, 100, 0, 0, 27294, 'Цзи Огненная Лапа');

UPDATE `creature_template` SET `ScriptName` = 'npc_panda_announcer' WHERE `entry` = 64885;
DELETE FROM `creature_text` WHERE entry =64885;
INSERT INTO `creature_text` (`entry`, `groupid`, `id`, `text`, `type`, `language`, `probability`, `emote`, `duration`, `sound`, `comment`) VALUES
(64885, 0, 0, 'Здравствуй, $n! Хранитель истории внизу как раз начинает урок, может, ты хочешь послушать?', 12, 0, 100, 3, 0, 0, 'Хранитель истории Цзань');


UPDATE `creature_template` SET `ScriptName` = 'npc_panda_history_leason' WHERE `entry` = 64875;
DELETE FROM `creature_text` WHERE entry =64875;
INSERT INTO `creature_text` (`entry`, `groupid`, `id`, `text`, `type`, `language`, `probability`, `emote`, `duration`, `sound`, `comment`) VALUES
(64875, 0, 0, 'Он открыл зонтик, воткнул в землю и сел в его прохладной тени.', 12, 0, 100, 6, 0, 0, 'Хранительница истории Амай'),
(64875, 1, 0, 'Он закрыл глаза, и стал единым целым с землей. А потом – знаете, что случилось потом?', 12, 0, 100, 2, 0, 0, 'Хранительница истории Амай'),
(64875, 2, 0, 'Его зонтик... пророс! У него выросли корни, он стал деревом, и на нем расцвели цветы!', 12, 0, 100, 5, 0, 0, 'Хранительница истории Амай'),
(64875, 3, 0, 'Да, это правда. Можете пойти в Лес Посохов и увидеть его собственными глазами.', 12, 0, 100, 1, 0, 0, 'Хранительница истории Амай'),
(64875, 4, 0, 'Окруженный посохами всех предков, что были до нас, посохами, выросшими в гигантские деревья.', 12, 0, 100, 274, 0, 0, 'Хранительница истории Амай'),
(64875, 5, 0, 'Это не грустная история! Сам Лю Лан говорил: \"Не стоит сожалеть о жизни, прожитой не зря\".', 12, 0, 100, 1, 0, 0, 'Хранительница истории Амай'),
(64875, 6, 0, 'А он, я думаю, прожил очень хорошую жизнь. Всем, что у нас есть, мы обязаны Лю Лану, первому пандарену-путешественнику.', 12, 0, 100, 273, 0, 0, 'Хранительница истории Амай'),
(64875, 7, 0, 'Мы рассказываем его историю, чтобы память о нем не истерлась никогда.', 12, 0, 100, 2, 0, 0, 'Хранительница истории Амай'),
(64875, 8, 0, 'Спасибо, Жуолинь! Это было прекрасно.', 12, 0, 100, 21, 0, 0, 'Хранительница истории Амай'),
(64875, 9, 0, 'Можешь спеть ее еще раз, для тех, кто только что пришел?', 12, 0, 100, 6, 0, 0, 'Хранительница истории Амай'),
(64875, 10, 0, 'Это песня о Лю Лане, первом пандарене-путешественнике.', 12, 0, 100, 1, 0, 0, 'Хранительница истории Амай'),
(64875, 11, 0, 'Песня написана на древнем наречии, языке императоров и ученых. Вряд ли сейчас кто-то говорит на нем.', 12, 0, 100, 6, 0, 0, 'Хранительница истории Амай'),
(64875, 12, 0, 'Она рассказывает о его приключениях.', 12, 0, 100, 273, 0, 0, 'Хранительница истории Амай'),
(64875, 13, 0, 'Храбрый Лю Лан оседлал морскую черепаху и отправился исследовать мир.', 12, 0, 100, 1, 0, 0, 'Хранительница истории Амай'),
(64875, 14, 0, 'Кто-нибудь помнит, как звали черепаху? Инь?', 12, 0, 100, 25, 0, 0, 'Хранительница истории Амай');

-- ----------------------------------------
-- Q: 29778 Rewritten Wisdoms
-- ----------------------------------------

-- spell: 104126
DELETE FROM `db_script_string` WHERE `entry` in ( 2000009994, 2000009995, 2000009996, 2000009997, 2000009998, 2000009999);
INSERT INTO `db_script_string` (`entry`, `content_default`, `content_loc8`) VALUES 
('2000009994', 'Хлопушки – бросать. Банану – есть.', 'Хлопушки – бросать. Банану – есть.'),
('2000009995', 'С мокрый мех плохо спать.', 'С мокрый мех плохо спать.'),
('2000009996', 'В каках не кататься, а то пахнуть плохо.', 'В каках не кататься, а то пахнуть плохо.'),
('2000009997', 'Банану чистить, потом есть.', 'Банану чистить, потом есть.'),
('2000009998', 'Свой хвост не тянуть, когда другой есть.', 'Свой хвост не тянуть, когда другой есть.'),
('2000009999', 'Кака не для есть, а для бросать.', 'Кака не для есть, а для бросать.');

-- ----------------------------------------
-- Q: 29783 Stronger Than Stone
-- ----------------------------------------

UPDATE `creature_template` SET `AIName`='SmartAI' WHERE `entry`=55585;
DELETE FROM smart_scripts WHERE entryorguid = 55585;
INSERT INTO `smart_scripts` (`entryorguid`, `source_type`, `id`, `link`, `event_type`, `event_phase_mask`, `event_chance`, `event_flags`, `event_param1`, `event_param2`, `event_param3`, `event_param4`, `action_type`, `action_param1`, `action_param2`, `action_param3`, `action_param4`, `action_param5`, `action_param6`, `target_type`, `target_param1`, `target_param2`, `target_param3`, `target_x`, `target_y`, `target_z`, `target_o`, `comment`) VALUES
(55585, 0, 0, 0, 50, 0, 100, 0, 29783, 0, 0, 0, 11, 108847, 0, 0, 0, 0, 0, 7, 0, 0, 0, 0, 0, 0, 0, 'At finish q: 29783'),
(55585, 0, 1, 0, 50, 0, 100, 0, 29782, 0, 0, 0, 11, 108858, 0, 0, 0, 0, 0, 7, 0, 0, 0, 0, 0, 0, 0, 'At finish q: 29782');

-- cast 129272 Jojo Ironbrow
UPDATE `creature_template` SET `ScriptName` = 'mob_jojo_ironbrow_3' WHERE `entry` = 57670;

DELETE FROM `creature_text` WHERE entry =57670;
INSERT INTO `creature_text` (`entry`, `groupid`, `id`, `text`, `type`, `language`, `probability`, `emote`, `duration`, `sound`, `comment`) VALUES
(57670, 0, 0, 'Эти камни – самые твердые, что есть на Шэнь-Цзынь Су, но и они не устоят перед мощью моих ударов.', 12, 0, 100, 1, 0, 0, 'Йо-Йо Железная Бровь'),
(57670, 1, 0, 'Ни одно разумное создание не усомнится в моей мощи.', 12, 0, 100, 2, 0, 0, 'Йо-Йо Железная Бровь');

-- 129294
DELETE FROM `conditions` WHERE SourceEntry = 108846;
INSERT INTO `conditions` (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `NegativeCondition`, `ErrorTextId`, `ScriptName`, `Comment`) VALUES
(13, 1, 108846, 0, 0, 31, 0, 3, 57668, 0, 0, 0, '', 'Cast - Only - 57668');

DELETE FROM `spell_target_position` WHERE id in (108845, 108847);
INSERT INTO `spell_target_position` (`id`, `target_map`, `target_position_x`, `target_position_y`, `target_position_z`, `target_orientation`) VALUES 
('108845', '860', '1078.087', '4180.681', '205.8848', '3.892084'),
('108847', '860', '1075.602', '4177.969', '205.6298', '0.7853982');

-- ----------------------------------------
-- Q: 29782 Stronger Than Bone
-- ----------------------------------------

UPDATE `creature_template` SET `ScriptName` = 'mob_jojo_ironbrow_4' WHERE `entry` = 57692;
DELETE FROM `creature_text` WHERE entry =57692;
INSERT INTO `creature_text` (`entry`, `groupid`, `id`, `text`, `type`, `language`, `probability`, `emote`, `duration`, `sound`, `comment`) VALUES
(57692, 0, 0, 'Невероятно! Никогда не видел столь искусной работы.', 12, 0, 100, 1, 0, 0, 'Йо-Йо Железная Бровь'),
(57692, 1, 0, 'Остается только одно. Разбить на куски! И тогда все раз и навсегда уверятся в силе моих могучих ударов.', 12, 0, 100, 1, 0, 0, 'Йо-Йо Железная Бровь'),
(57692, 2, 0, 'Ох... моя голова... Как больно! Я... В этом пьедестале, должно быть, содержится невероятная мощь... он мощнее, чем мой лоб.', 12, 0, 100, 18, 0, 0, 'Йо-Йо Железная Бровь'),
(57692, 3, 0, 'Я... я, пожалуй, прилягу... А то что-то голова разболелась...', 12, 0, 100, 0, 0, 0, 'Йо-Йо Железная Бровь');

DELETE FROM `spell_target_position` WHERE id in (108857, 108858);
INSERT INTO `spell_target_position` (`id`, `target_map`, `target_position_x`, `target_position_y`, `target_position_z`, `target_orientation`) VALUES 
('108857', '860', '1078.087', '4180.681', '205.8848', '3.892084'),
('108858', '860', '1075.535', '4177.896', '205.5825', '5.550147');

UPDATE `creature_template_addon` SET `auras` = '' WHERE `creature_template_addon`.`entry` = 57692;
UPDATE `creature_template` SET `VehicleId` = '1950' WHERE `creature_template`.`entry` = 57690;
DELETE FROM `npc_spellclick_spells` `npc_entry` = 57690;
INSERT INTO `npc_spellclick_spells` (`npc_entry`, `spell_id`, `cast_flags`, `user_type`) VALUES ('57690', '46598', '1', '0');
DELETE FROM `vehicle_template_accessory` WHERE `EntryOrAura` = 57690;
INSERT INTO `vehicle_template_accessory` (`EntryOrAura`, `accessory_entry`, `seat_id`, `minion`, `description`, `summontype`, `summontimer`) VALUES 
('57690', '57691', '0', '1', '', '8', '0');
-- ----------------------------------------
-- Q: 29780 Do No Evil
-- ----------------------------------------
UPDATE `quest_template` SET `RequiredPOI1` = '263756', `RequiredUnkFlag1` = '1' WHERE `quest_template`.`Id` = 29780;
UPDATE `creature_template` SET `mindmg` = '20', `maxdmg` = '30', `attackpower` = '15', `ScriptName` = 'mob_huojin_monk' WHERE `entry` = 65558;
DELETE FROM `creature_text` WHERE entry =65558;
INSERT INTO `creature_text` (`entry`, `groupid`, `id`, `text`, `type`, `language`, `probability`, `emote`, `duration`, `sound`, `comment`) VALUES
(65558, 0, 0, 'Давай возьмемся за это вместе!', 12, 0, 100, 5, 0, 27320, 'Цзи Огненная Лапа'),
(65558, 1, 0, 'Мне нравится твой стиль боя!', 12, 0, 100, 5, 0, 27322, 'Цзи Огненная Лапа'),
(65558, 1, 1, 'Это все, на что ты способен?!', 12, 0, 100, 5, 0, 27316, 'Цзи Огненная Лапа'),
(65558, 1, 2, 'Эта обезьяна нарывается!', 12, 0, 100, 5, 0, 27317, 'Цзи Огненная Лапа'),
(65558, 1, 3, 'Еще одной мартышкой меньше. Неплохо!', 12, 0, 100, 5, 0, 27315, 'Цзи Огненная Лапа'),
(65558, 2, 0, 'Увидимся у причала!', 12, 0, 100, 0, 0, 27319, 'Цзи Огненная Лапа');

DELETE FROM `spell_script_names` WHERE spell_id = 105306;
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES ('105306', 'spell_summon_ji_yung');

UPDATE `creature_template` SET `AIName`='SmartAI' WHERE `entry`=55633;
DELETE FROM smart_scripts WHERE entryorguid = 55633;
INSERT INTO `smart_scripts` (`entryorguid`, `source_type`, `id`, `link`, `event_type`, `event_phase_mask`, `event_chance`, `event_flags`, `event_param1`, `event_param2`, `event_param3`, `event_param4`, `action_type`, `action_param1`, `action_param2`, `action_param3`, `action_param4`, `action_param5`, `action_param6`, `target_type`, `target_param1`, `target_param2`, `target_param3`, `target_x`, `target_y`, `target_z`, `target_o`, `comment`) VALUES
(55633, 0, 0, 0, 60, 0, 100, 0, 1000, 1000, 3000, 3000, 11, 127940, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 'Cast Spell 127940'),
(55633, 0, 1, 0, 60, 0, 100, 0, 12000, 12000, 10000, 10000, 75, 109104, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 'add aura 127940');


-- Pandashan
DELETE FROM `areatrigger_scripts`  WHERE `ScriptName` LIKE 'AreaTrigger_at_bassin_curse';
INSERT INTO `areatrigger_scripts` (`entry`, `ScriptName`) VALUES
(6986, 'AreaTrigger_at_bassin_curse'),
(6987, 'AreaTrigger_at_bassin_curse'),
(6988, 'AreaTrigger_at_bassin_curse'),
(6989, 'AreaTrigger_at_bassin_curse'),
(6990, 'AreaTrigger_at_bassin_curse'),
(6991, 'AreaTrigger_at_bassin_curse'),
(6992, 'AreaTrigger_at_bassin_curse'),
(7011, 'AreaTrigger_at_bassin_curse'),
(7012, 'AreaTrigger_at_bassin_curse');

DELETE FROM `areatrigger_scripts`  WHERE `ScriptName` LIKE 'AreaTrigger_at_temple_entrance';
INSERT INTO `areatrigger_scripts` (`entry`, `ScriptName`) VALUES
(7835, 'AreaTrigger_at_temple_entrance');

DELETE FROM `areatrigger_scripts`  WHERE `ScriptName` LIKE 'AreaTrigger_at_mandori';
INSERT INTO `areatrigger_scripts` (`entry`, `ScriptName`) VALUES
(7710, 'AreaTrigger_at_mandori');

DELETE FROM `areatrigger_scripts`  WHERE `ScriptName` LIKE 'AreaTrigger_at_rescue_soldiers';
INSERT INTO `areatrigger_scripts` (`entry`, `ScriptName`) VALUES
(7087, 'AreaTrigger_at_rescue_soldiers');

DELETE FROM `areatrigger_scripts`  WHERE `ScriptName` LIKE 'AreaTrigger_at_wind_temple_entrance';
INSERT INTO `areatrigger_scripts` (`entry`, `ScriptName`) VALUES
(7041, 'AreaTrigger_at_wind_temple_entrance');
