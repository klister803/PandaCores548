DELETE FROM `gossip_menu` WHERE (`entry`=15607 AND `text_id`=22414);
INSERT INTO `gossip_menu` (`entry`, `text_id`) VALUES
(15607, 22414); -- 70438

DELETE FROM `gossip_menu_option` WHERE (`menu_id`=15607 AND `id`=0);
INSERT INTO `gossip_menu_option` (`menu_id`, `id`, `option_icon`, `option_text`, `box_coded`, `box_money`, `box_text`) VALUES
(15607, 0, 0, 'Я готова.', 0, 0, ''); -- 70438

UPDATE `gameobject_template` SET `ScriptName` ='go_cospicuous_illidari_scroll' WHERE (`entry`='216364');
-- UPDATE `gameobject_template` SET `ScriptName` ='go_treasure_chest' WHERE (`entry`='');

UPDATE `creature_template` SET `ScriptName` ='npc_akama' WHERE (`entry`='68137');
UPDATE `creature_template` SET `ScriptName` ='npc_asthongue_primalist' WHERE (`entry`='68096');
UPDATE `creature_template` SET `ScriptName` ='npc_ashtongue_worker' WHERE (`entry`='68098');
UPDATE `creature_template` SET `ScriptName` ='npc_suffering_soul_fragment' WHERE (`entry`='68139');
UPDATE `creature_template` SET `ScriptName` ='npc_hungering_soul_fragment' WHERE (`entry`='68140');
UPDATE `creature_template` SET `ScriptName` ='npc_essence_of_order' WHERE (`entry`='68151');
UPDATE `creature_template` SET `ScriptName` ='npc_demonic_gateway_scen' WHERE (`entry`='70028');
UPDATE `creature_template` SET `ScriptName` ='npc_kanrethad_ebonlocke' WHERE (`entry`='69964');
UPDATE `creature_template` SET `ScriptName` ='npc_jubeka_shadowbreaker' WHERE (`entry`='70166');

DELETE FROM `areatrigger_scripts` WHERE `entry` IN ('8696', '8699', '8698', '8701', '8706', '8702', '8708', '8908');
INSERT INTO `areatrigger_scripts` (`entry`, `ScriptName`) VALUES
('8696', 'at_pursuing_the_black_harvest_main'),
('8699', 'at_pursuing_the_black_harvest_main'),
('8698', 'at_pursuing_the_black_harvest_main'),
('8701', 'at_pursuing_the_black_harvest_main'),
('8706', 'at_pursuing_the_black_harvest_main'),
('8702', 'at_pursuing_the_black_harvest_main'),
('8708', 'at_pursuing_the_black_harvest_main'),
('8908', 'at_pursuing_the_black_harvest_main');

UPDATE `creature_template` SET `minlevel`='90', `maxlevel`='90', `faction`='16', `speed_walk`='1.11111', `speed_run`='1.42857', `speed_fly`='1.14286', `baseattacktime`='2000', `rangeattacktime`='2000', `unit_class`='2', `dynamicflags`='4360768', `unit_flags`='32832', `unit_flags2`='2048', `HoverHeight`='1' WHERE (`entry`='68139');
UPDATE `creature_template` SET `minlevel`='90', `maxlevel`='90', `faction`='1813', `speed_walk`='1.2', `speed_run`='0.992063', `speed_fly`='1.14286', `baseattacktime`='1300', `rangeattacktime`='2000', `unit_class`='2', `dynamicflags`='4360768', `unit_flags`='0', `unit_flags2`='2048', `HoverHeight`='1' WHERE (`entry`='68137');
UPDATE `creature_template` SET `minlevel`='90', `maxlevel`='90', `faction`='1813', `speed_walk`='1', `speed_run`='0.992063', `speed_fly`='1.14286', `baseattacktime`='2000', `rangeattacktime`='2000', `unit_class`='2', `dynamicflags`='4360256', `unit_flags`='32832', `unit_flags2`='2048', `HoverHeight`='1' WHERE (`entry`='68129');
UPDATE `creature_template` SET `minlevel`='90', `maxlevel`='90', `faction`='1813', `speed_walk`='1', `speed_run`='0.992063', `speed_fly`='1.14286', `baseattacktime`='2000', `rangeattacktime`='2000', `unit_class`='2', `dynamicflags`='4358144', `unit_flags`='32768', `unit_flags2`='4196352', `HoverHeight`='1' WHERE (`entry`='68098');
UPDATE `creature_template` SET `minlevel`='90', `maxlevel`='90', `faction`='1813', `speed_walk`='1', `speed_run`='0.992063', `speed_fly`='1.14286', `baseattacktime`='2000', `rangeattacktime`='2000', `unit_class`='2', `dynamicflags`='4358144', `unit_flags`='32832', `unit_flags2`='4196352', `HoverHeight`='1' WHERE (`entry`='68096');
UPDATE `creature_template` SET `faction`='35', `speed_walk`='1.2', `speed_run`='1', `speed_fly`='1.14286', `baseattacktime`='2000', `rangeattacktime`='2000', `unit_class`='1', `dynamicflags`='1595200', `unit_flags`='33554432', `unit_flags2`='4196352', `HoverHeight`='1' WHERE (`entry`='24925');
UPDATE `creature_template` SET `minlevel`='90', `maxlevel`='90', `faction`='16', `speed_walk`='1.6', `speed_run`='1.71429', `speed_fly`='1.14286', `baseattacktime`='2000', `rangeattacktime`='2000', `unit_class`='1', `dynamicflags`='4361536', `unit_flags`='768', `unit_flags2`='2048', `HoverHeight`='1' WHERE (`entry`='68151');
UPDATE `creature_template` SET `minlevel`='90', `maxlevel`='90', `faction`='16', `speed_walk`='1', `speed_run`='1.14286', `speed_fly`='1.14286', `baseattacktime`='2000', `rangeattacktime`='2000', `unit_class`='1', `dynamicflags`='4361984', `unit_flags`='32768', `unit_flags2`='2048', `HoverHeight`='1' WHERE (`entry`='68156');
UPDATE `creature_template` SET `minlevel`='90', `maxlevel`='90', `faction`='90', `speed_walk`='1', `speed_run`='0.857143', `speed_fly`='1.14286', `baseattacktime`='2000', `rangeattacktime`='2000', `unit_class`='8', `dynamicflags`='4363072', `unit_flags`='0', `unit_flags2`='2048', `HoverHeight`='1' WHERE (`entry`='68173');
UPDATE `creature_template` SET `minlevel`='90', `maxlevel`='90', `faction`='1786', `speed_walk`='2', `speed_run`='1.71429', `speed_fly`='1.14286', `baseattacktime`='2000', `rangeattacktime`='2000', `unit_class`='1', `dynamicflags`='4363008', `unit_flags`='32832', `unit_flags2`='2099200', `HoverHeight`='1' WHERE (`entry`='68174');
UPDATE `creature_template` SET `minlevel`='90', `maxlevel`='90', `faction`='1786', `speed_walk`='2', `speed_run`='1.42857', `speed_fly`='1.14286', `baseattacktime`='2000', `rangeattacktime`='2000', `unit_class`='8', `dynamicflags`='4363072', `unit_flags`='32768', `unit_flags2`='2099200', `HoverHeight`='1' WHERE (`entry`='68175');
UPDATE `creature_template` SET `minlevel`='90', `maxlevel`='90', `faction`='1786', `speed_walk`='2', `speed_run`='1.71429', `speed_fly`='1.14286', `baseattacktime`='2000', `rangeattacktime`='2000', `unit_class`='1', `dynamicflags`='4363264', `unit_flags`='32832', `unit_flags2`='2099200', `HoverHeight`='1' WHERE (`entry`='68176');
UPDATE `creature_template` SET `minlevel`='90', `maxlevel`='90', `faction`='1786', `speed_walk`='2', `speed_run`='1.42857', `speed_fly`='1.14286', `baseattacktime`='2000', `rangeattacktime`='2000', `unit_class`='8', `dynamicflags`='4365056', `unit_flags`='32768', `unit_flags2`='2048', `HoverHeight`='1' WHERE (`entry`='68204');
UPDATE `creature_template` SET `minlevel`='90', `maxlevel`='90', `faction`='1786', `speed_walk`='2', `speed_run`='1.42857', `speed_fly`='1.14286', `baseattacktime`='2000', `rangeattacktime`='2000', `unit_class`='8', `dynamicflags`='4365120', `unit_flags`='32768', `unit_flags2`='2048', `HoverHeight`='1' WHERE (`entry`='68205');
UPDATE `creature_template` SET `minlevel`='90', `maxlevel`='90', `faction`='1786', `speed_walk`='2', `speed_run`='1.42857', `speed_fly`='1.14286', `baseattacktime`='2000', `rangeattacktime`='2000', `unit_class`='8', `dynamicflags`='4365056', `unit_flags`='32768', `unit_flags2`='2048', `HoverHeight`='1' WHERE (`entry`='68206');
UPDATE `creature_template` SET `minlevel`='93', `maxlevel`='93', `faction`='1771', `npcflag`='16777216', `speed_walk`='0.944444', `speed_run`='1.11111', `speed_fly`='1.14286', `baseattacktime`='2000', `rangeattacktime`='2000', `unit_class`='8', `dynamicflags`='4477696', `unit_flags`='256', `unit_flags2`='4194304', `VehicleId`='2745', `HoverHeight`='1' WHERE (`entry`='69964');
UPDATE `creature_template` SET `minlevel`='91', `maxlevel`='91', `faction`='1771', `speed_walk`='1', `speed_run`='1.14286', `speed_fly`='1.14286', `baseattacktime`='2000', `rangeattacktime`='2000', `unit_class`='8', `dynamicflags`='4481344', `unit_flags`='33554432', `HoverHeight`='1' WHERE (`entry`='70023');
UPDATE `creature_template` SET `minlevel`='90', `maxlevel`='90', `faction`='35', `speed_walk`='1', `speed_run`='0.992063', `speed_fly`='1.14286', `baseattacktime`='2000', `rangeattacktime`='2000', `unit_class`='1', `dynamicflags`='4481792', `unit_flags`='33554688', `unit_flags2`='4229152', `HoverHeight`='1' WHERE (`entry`='70028');
UPDATE `creature_template` SET `minlevel`='90', `maxlevel`='90', `Health_mod`='35' WHERE (`entry`='70075');

DELETE FROM `npc_spellclick_spells` WHERE `npc_entry` IN ('70052', '69964');
INSERT INTO `npc_spellclick_spells` (`npc_entry`, `spell_id`, `cast_flags`, `user_type`) VALUES
('70052', '138680', '1', '0'),
('69964', '139200', '1', '0');

DELETE FROM `spell_script_names` WHERE `spell_id` IN ('138680');
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
('138680', 'spell_place_empowered_soulcore');

DELETE FROM `creature_text` WHERE `entry` IN ('70166', '69964');

INSERT INTO `creature_text` (`entry`, `groupid`, `id`, `text`, `type`, `language`, `probability`, `emote`, `duration`, `sound`, `comment`) VALUES
(70166, 0, 0, 'НЕТ!', 12, 0, 100, 0, 0, 0, 'to Player'),
(70166, 1, 0, 'Я сделала лишь то, о чем ты сам просил меня раньше, Канретад. Ты сам во всем виноват.', 12, 0, 100, 0, 0, 0, 'to Player'),
(70166, 2, 0, 'Канретад настолько погрузился в энергию Скверны, что она полностью подчинила его. Но лишь глоток его силы не должен повредить. Испей ее.', 12, 0, 100, 0, 0, 0, 'to Player'),
(70166, 3, 0, 'А теперь иди и используй полученные знания. Если когда-нибудь тебе захочется избавиться от энергии Скверны или вновь ощутить силу Канретада, ищи меня в Алтаре Проклятия в Долине Призрачной Луны. Там я буду присматривать за своим пленником.', 12, 0, 100, 0, 0, 0, 'to Player'),

(69964, 0, 0, 'БОЙСЯ! Теперь я овладел всеми энергиями Скверны этого мира! Демоническая сила, которой я управляю… невероятна! Безгранична! Я ВСЕМОГУЩ!', 12, 0, 100, 0, 0, 0, 'to Player'),
(69964, 1, 0, 'Думаешь, я слишком далеко зашел? Нет такого понятия. Мрачная Жатва поработит самых могущественных демонов и уничтожит всех, кто станет противостоять ей – в этом мире и на Азероте! И ТЫ хочешь остановить меня, кроха?', 12, 0, 100, 0, 0, 0, 'to Player'),
(69964, 2, 0, 'Ну-ну, попытайся.', 12, 0, 100, 0, 0, 0, 'to Player'),
(69964, 3, 0, 'Ха! Твои слабые попытки ранить меня никуда не годятся. Хочешь посмотреть на одного из моих новых питомцев?', 12, 0, 100, 0, 0, 0, 'to Player'),
(69964, 4, 0, 'Разрушение у искоренителей в крови. Огромная сила и мощь делают их непобедимыми.', 12, 0, 100, 0, 0, 0, 'to Player'),
(69964, 5, 0, 'ЧТО?! Вот теперь я зол!', 12, 0, 100, 0, 0, 0, 'to Player'),
(69964, 6, 0, 'Часики тикают.', 12, 0, 100, 0, 0, 0, 'to Player'),
(69964, 7, 0, 'Боль усиливается.', 12, 0, 100, 0, 0, 0, 'to Player'),
(69964, 8, 0, 'Всем будет править хаос.', 12, 0, 100, 0, 0, 0, 'to Player'),
(69964, 9, 0, '|TInterface\Icons\ability_warlock_chaosbolt.blp:20|t%s начинает читать заклинание |cFFFF0000|Hspell:138559|h[Стрела Хаоса]|h|r!', 12, 0, 100, 0, 0, 0, 'to Player'),
(69964, 10, 0, 'МОЩЬ КАТАКЛИЗМА!', 12, 0, 100, 0, 0, 0, 'to Player'),
(69964, 11, 0, '|TInterface\Icons\achievement_zone_cataclysm.blp:20|t%s начинает призывать чудовищный |cFFFF0000|Hspell:138564|h[Катаклизм]|h|r!', 12, 0, 100, 0, 0, 0, 'to Player'),
(69964, 12, 0, 'Один бес – хорошо, два – лучше, а ШЕСТЬДЕСЯТ – просто замечательно!', 12, 0, 100, 0, 0, 0, 'to Player'),
(69964, 13, 0, 'Ха-ха-ха… у тебя была надежда на победу? Теперь меня НИЧТО не сможет убить, жалкое смертное существо!', 12, 0, 100, 0, 0, 0, 'to Player'),
(69964, 14, 0, 'А теперь твоя сказка с грустным концом закончится!', 12, 0, 100, 0, 0, 0, 'to Player'),
(69964, 15, 0, 'Джубека?! Что ты…', 12, 0, 100, 0, 0, 0, 'to Player'),
(69964, 16, 0, 'АААААААААА!', 12, 0, 100, 0, 0, 0, 'to Player');
