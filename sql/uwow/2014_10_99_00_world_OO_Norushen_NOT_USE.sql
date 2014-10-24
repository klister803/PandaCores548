/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET NAMES utf8 */;
/*!40014 SET FOREIGN_KEY_CHECKS=0 */;

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


remove at start.
[0] [1] Object GUID: Full: 0xF113672400000193 Type: GameObject Entry: 223012 Low: 403
[0] [2] Object GUID: Full: 0xF113672500000194 Type: GameObject Entry: 223013 Low: 404
[0] [3] Object GUID: Full: 0xF113672600000195 Type: GameObject Entry: 223014 Low: 405
[0] [4] Object GUID: Full: 0xF113672700000196 Type: GameObject Entry: 223015 Low: 406

ServerToClient: SMSG_PLAY_SOUND (0x102A) Length: 10 ConnectionIndex: 0 Time: 10/08/2014 18:28:10.000 Type: Normal Opcode Number: 1579
Sound Id: 38883
Guid: Full: 0xF131191F00000038 Type: Unit Entry: 71967 Low: 56

ServerToClient: SMSG_MESSAGECHAT (0x1A9A) Length: 99 ConnectionIndex: 0 Time: 10/08/2014 18:28:10.000 Type: Unknown Opcode Type Number: 1580
Show only in bubble: 0
Chat type: MonsterYell (14)
Realm Id 2: 50659408
Text: Стойте!
Receiver Name: Хранитель истории Чо
Sender Name: Норусхен
Realm Id: 50659408
SenderGUID: Full: 0xF131191F00000038 Type: Unit Entry: 71967 Low: 56
ReceiverGUID: Full: 0xF1311CA8000006C0 Type: Unit Entry: 72872 Low: 1728
GuildGUID: 0x0
GroupGUID: 0x0