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
102399, 102400, 102521, 108150) AND  area = 5736;
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
('102397', '5736', '29523', '0', '0', '0', '2', '1', '66', '0'),
-- 102399
('102399', '5736', '29420', '0', '0', '0', '2', '1', '74', '0'),
('102400', '5736', '29420', '0', '0', '0', '2', '1', '74', '0'),
('102521', '5736', '29420', '0', '0', '0', '2', '1', '74', '0');
--
-- ('108150', '5736', '29421', '0', '0', '0', '2', '1', '8', '0'),
--


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

DELETE FROM smart_scripts WHERE entryorguid = 54786;
INSERT INTO `smart_scripts` (`entryorguid`, `source_type`, `id`, `link`, `event_type`, `event_phase_mask`, `event_chance`, `event_flags`, `event_param1`, `event_param2`, `event_param3`, `event_param4`, `action_type`, `action_param1`, `action_param2`, `action_param3`, `action_param4`, `action_param5`, `action_param6`, `target_type`, `target_param1`, `target_param2`, `target_param3`, `target_x`, `target_y`, `target_z`, `target_o`, `comment`) VALUES
(54786, 0, 0, 0, 9, 0, 100, 0, 30, 30, 60000, 60000, 1, 0, 0, 0, 0, 0, 0, 7, 0, 0, 0, 0, 0, 0, 0, 'At 54958 in range'),
(54786, 0, 1, 0, 47, 0, 100, 0, 29521, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 7, 0, 0, 0, 0, 0, 0, 0, 'At start q: 29521');

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
