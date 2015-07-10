DELETE FROM `creature_text` WHERE `entry` IN 
(70100, 70500, 70070, 69835, 69828, 70061, 69837, 70228, 69833, 70283, 70074, 70099);

INSERT INTO `creature_text` (`entry`, `groupid`, `id`, `text`, `type`, `language`, `probability`, `emote`, `duration`, `sound`, `comment`) VALUES
(70100, 0, 0, 'Принесенный тобой триллий надо сначала очистить.', 12, 0, 100, 0, 0, 0, 'Гневион to Player'),
(70100, 1, 0, 'Эта плавильня вполне подойдет. Помоги мне с формами.', 12, 0, 100, 0, 0, 0, 'Гневион to Player'),
(70100, 2, 0, 'Эта плавильня вполне подойдет. Мы усилим металл электрическим зарядом.', 12, 0, 100, 0, 0, 0, 'Гневион'),
(70100, 3, 0, 'Придется драться, если нас заметят.', 12, 0, 100, 0, 0, 0, 'Гневион'),
(70100, 4, 0, 'Прекрасно. Материалы готовы. Следуй за мной.', 12, 0, 100, 0, 0, 0, 'Гневион to Player'),
(70100, 5, 0, 'Как интересно! Эта кузня выстроена поверх древнего механизма титанов. Ах, Лэй Шэнь, ты умен!', 12, 0, 100, 0, 0, 0, 'Гневион to Player'),
(70100, 6, 0, 'Будь я дворфом, я бы стал нажимать все кнопки подряд. Но мы поступим иначе.', 12, 0, 100, 0, 0, 0, 'Гневион to Player'),
(70100, 7, 0, 'Ты только посмотри… Лэй Шэню легко было выстроить империю, ибо он обладал силой создавать миры.', 12, 0, 100, 0, 0, 0, 'Гневион to Player'),
(70100, 8, 0, 'Могу пользовались лишь частью открытых для них возможностей. Из них мало кто обладал мудростью Властелина Грома.', 12, 0, 100, 0, 0, 0, 'Гневион to Player'),
(70100, 9, 0, '""Выявлена аномалия."" Какая еще аномалия?', 12, 0, 100, 0, 0, 0, 'Гневион to Player'),
(70100, 10, 0, 'Смотри-ка… технологии титанов притягивают ша как магнит!', 12, 0, 100, 0, 0, 0, 'Гневион to Player'),
(70100, 11, 0, 'Ты уже наверняка знаешь, как с ними справиться. Убери их отсюда!', 12, 0, 100, 0, 0, 0, 'Гневион to Player'),
(70100, 12, 0, 'Нет, нет, так не пойдет. Уходим отсюда! -> Исчадье ша', 14, 0, 100, 0, 0, 0, 'Гневион to Исчадье ша'),
(70100, 13, 0, 'Может, если увеличить подачу энергии, проблема с ша решится? Давай попробуем. -> Небесный кузнец', 12, 0, 100, 0, 0, 0, 'Гневион to Небесный кузнец'),
(70100, 14, 0, 'Отлично! Получилось. -> Небесный кузнец', 12, 0, 100, 0, 0, 0, 'Гневион to Небесный кузнец'),
(70100, 15, 0, 'Ой, кажется, я поторопился. Сила ша растет! Уничтожь его! -> Небесный кузнец', 12, 0, 100, 0, 0, 0, 'Гневион to Небесный кузнец'),
(70100, 16, 0, 'Ты же не слишком сердишься, правда? -> Небесный кузнец', 12, 0, 100, 0, 0, 0, 'Гневион to Небесный кузнец'),
(70100, 17, 0, 'Наковальни заряжены. Теперь их можно использовать против ша, но каждую лишь один раз! -> Небесный кузнец', 12, 0, 100, 0, 0, 0, 'Гневион to Небесный кузнец'),
(70100, 18, 0, 'Копье выковано. Скорее брось его в ша!', 12, 0, 100, 0, 0, 0, 'Гневион'),
(70100, 19, 0, 'Все! Мы сделали это! Выковали настоящий шедевр!', 12, 0, 100, 0, 0, 0, 'Гневион'),
(70100, 20, 0, 'Тут мы закончили, но оружие еще надо закалить.', 12, 0, 100, 0, 0, 0, 'Гневион'),
(70100, 21, 0, 'Тебе надо пронзить этим копьем бьющееся сердце Повелителя Гроз.', 12, 0, 100, 0, 0, 0, 'Гневион to Player'),
(70100, 22, 0, 'Но учти – как только копье вонзится в его шкуру, тебе придется несладко.', 12, 0, 100, 0, 0, 0, 'Гневион to Player'),
(70100, 23, 0, 'Бросай его прямо перед тем, как зверь умрет – ни секундой раньше, ни секундой позже!', 12, 0, 100, 0, 0, 0, 'Гневион to Player'),
(70100, 24, 0, 'Мне надо кое-что сделать на материке. Когда закончишь дело, приходи на Сокрытую лестницу.', 12, 0, 100, 0, 0, 0, 'Гневион to Player'),
(70100, 25, 0, 'И постарайся не умереть.', 12, 0, 100, 0, 0, 0, 'Гневион to Player'),

(70500, 0, 0, '|cFFFF2222[Копье молний]|r заряжено. Прикончите слияние ша!', 41, 0, 100, 0, 0, 0, 'Lightning Spear Float Stalker to 0'),

(70070, 0, 0, 'Обнаружены лазутчики! Необходимо защитить Кузню Грома!', 14, 0, 100, 0, 0, 0, 'Направляющий молнии Шань''цзэ to 0'),

(69835, 0, 0, 'Обнаружены лазутчики! Необходимо защитить Кузню Грома!', 14, 0, 100, 0, 0, 0, 'Полководец Шань''цзэ  to 0'),
(69835, 1, 0, 'Кузня Грома наша! Мы не пожалеем жизни ради ее защиты!', 14, 0, 100, 0, 0, 0, 'Полководец Шань''цзэ  to 0'),

(69828, 0, 0, '|cFFFF2222[Наковальня грома]|r перегружена! Активируйте ее, чтобы |cFFFF2222|Hspell:138834|h[Разрядка]|h|r высвободила ее энергию!', 41, 0, 100, 0, 0, 0, 'Небесный кузнец to Небесный кузнец'),
(69828, 1, 0, '|cFFFF2222[Наковальня грома]|r перегружена! Щелкните по ней, чтобы использовать |cFFFF2222|Hspell:138834|h[Раскат грома]|h|r!', 41, 0, 100, 0, 0, 0, 'Небесный кузнец to Небесный кузнец'),
(69828, 2, 0, 'Небесный кузнец может умереть!', 41, 0, 100, 0, 0, 0, 'Небесный кузнец to 0'),

(70061, 0, 0, 'Вас преследует молния!', 41, 0, 100, 0, 0, 0, 'Кузня Грома to Кузня Грома'),

(69837, 0, 0, 'Небесный защитник создает область |cFFFF2222|Hspell:140068|h[исцеляющей энергии]|h|r!', 41, 0, 100, 0, 0, 0, 'Небесный защитник to 0'),
(69837, 1, 0, 'Небесный защитник может умереть!', 41, 0, 100, 0, 0, 0, 'Небесный защитник to 0'),
(69837, 2, 0, 'Небесный защитник тяжело ранен и больше не способен сражаться!', 41, 0, 100, 0, 0, 0, 'Небесный защитник to 0'),

(70228, 0, 0, 'Слияние ша начинает произносить заклинание |cFFFF2222|Hspell:139382|h[Безумие]|h|r! Разрядите одну из наковален, чтобы предотвратить это!', 41, 0, 100, 0, 0, 0, 'Слияние ша to 0'),
(70228, 1, 0, 'Слияние ша изгоняет небесного защитника. Защищайтесь!', 41, 0, 100, 0, 0, 0, 'Слияние ша to 0'),

(69833, 0, 0, 'Обнаружены лазутчики! Необходимо защитить Кузню Грома!', 14, 0, 100, 0, 0, 0, 'Воин Шань''цзэ to 0'),

(70283, 0, 0, 'Кузня Грома наполнила окружающее пространство электрическими разрядами. Подбегите к ней, чтобы |cFFFF2222|Hspell:139397|h[Перегрузка]|h|r пополнила ваши силы!', 41, 0, 100, 0, 0, 0, 'Кузня Грома to 0'),

(70074, 0, 0, 'Главный кузнец Вул''кон берет в руки свой посох и пропускает сквозь него разряды молнии!', 41, 0, 100, 0, 0, 0, 'Главный кузнец Вул''кон to 0'),
(70074, 1, 0, 'Главный кузнец Вул''кон берет в руки свой молот и начинает размахивать им с огромной силой!', 41, 0, 100, 0, 0, 0, 'Главный кузнец Вул''кон to 0'),

(70099, 0, 0, 'Защитник Шадо-Пан создал |cFFFF2222|Hspell:132744|h[Исцеляющую сферу]|h|r!', 41, 0, 100, 0, 0, 0, 'Защитник Шадо-Пан to 0'),
(70099, 1, 0, 'Спасибо за помощь, герой. Мы поможем тебе защититься от могу, чем только сможем.', 14, 0, 100, 0, 0, 0, 'Защитник Шадо-Пан to Гневион'),
(70099, 2, 0, 'Кузня в безопасности, нам нужно возвращаться. Удачи тебе в твоих дальнейших приключениях.', 12, 0, 100, 2, 0, 0, 'Защитник Шадо-Пан to Гневион');


UPDATE `creature_template` SET  `minlevel`='1', `maxlevel`='1', `faction_A`='35', `speed_walk`='1', `speed_run`='1.14286', `speed_fly`='1.14286', `baseattacktime`='2000', `rangeattacktime`='2000', `unit_class`='1', `unit_flags`='512', `HoverHeight`='1' WHERE (`entry`='70556');
UPDATE `creature_template` SET `minlevel`='90', `maxlevel`='90', `faction_A`='35', `speed_walk`='3.2', `speed_run`='2', `speed_fly`='1.14286', `baseattacktime`='2000', `rangeattacktime`='2000', `unit_class`='2', `unit_flags`='33570816', `unit_flags2`='2048', `HoverHeight`='1' WHERE (`entry`='70283');
UPDATE `creature_template` SET `minlevel`='1', `maxlevel`='1', `faction_A`='35', `speed_walk`='1', `speed_run`='1.14286', `speed_fly`='1.14286', `baseattacktime`='2000', `rangeattacktime`='2000', `unit_class`='1', `unit_flags`='512', `HoverHeight`='1' WHERE (`entry`='70148');
UPDATE `creature_template` SET `minlevel`='1', `maxlevel`='1', `faction_A`='14', `speed_walk`='1', `speed_run`='1.14286', `speed_fly`='1.14286', `baseattacktime`='2000', `rangeattacktime`='2000', `unit_class`='1', `unit_flags`='768', `HoverHeight`='1' WHERE (`entry`='70299');
UPDATE `creature_template` SET `minlevel`='1', `maxlevel`='1', `faction_A`='35', `speed_walk`='1', `speed_run`='1.14286', `speed_fly`='1.14286', `baseattacktime`='2000', `rangeattacktime`='2000', `unit_class`='1', `unit_flags`='512', `HoverHeight`='1' WHERE (`entry`='70162');
UPDATE `creature_template` SET `minlevel`='90', `maxlevel`='90', `faction_A`='35', `speed_walk`='2.8', `speed_run`='1', `speed_fly`='1.14286', `baseattacktime`='2000', `rangeattacktime`='2000', `unit_class`='2', `unit_flags`='33570816', `unit_flags2`='2048', `HoverHeight`='1' WHERE (`entry`='70577');
UPDATE `creature_template` SET `minlevel`='1', `maxlevel`='1', `faction_A`='14', `speed_walk`='1', `speed_run`='1.14286', `speed_fly`='1.14286', `baseattacktime`='2000', `rangeattacktime`='2000', `unit_class`='1', `unit_flags`='768', `HoverHeight`='1' WHERE (`entry`='70449');
UPDATE `creature_template` SET `minlevel`='91', `maxlevel`='91', `faction_A`='14', `speed_walk`='1', `speed_run`='1.42857', `speed_fly`='1.14286', `baseattacktime`='1500', `rangeattacktime`='2000', `unit_class`='8', `unit_flags`='32768', `unit_flags2`='2048', `HoverHeight`='1' WHERE (`entry`='70039');
UPDATE `creature_template` SET `minlevel`='90', `maxlevel`='90', `faction_A`='35', `speed_walk`='1', `speed_run`='1.14286', `speed_fly`='1.14286', `baseattacktime`='2000', `rangeattacktime`='2000', `unit_class`='1', `unit_flags`='33555200', `unit_flags2`='2048', `HoverHeight`='1' WHERE (`entry`='69217');
UPDATE `creature_template` SET `minlevel`='91', `maxlevel`='91', `faction_A`='14', `speed_walk`='1', `speed_run`='1.42857', `speed_fly`='1.14286', `baseattacktime`='1500', `rangeattacktime`='2000', `unit_class`='8', `unit_flags`='32768', `unit_flags2`='2048', `HoverHeight`='1' WHERE (`entry`='70048');
UPDATE `creature_template` SET `minlevel`='1', `maxlevel`='1', `faction_A`='35', `npcflag`='16777216', `speed_walk`='1', `speed_run`='1', `speed_fly`='1.14286', `baseattacktime`='2000', `rangeattacktime`='2000', `unit_class`='1', `unit_flags`='33555200', `HoverHeight`='1' WHERE (`entry`='70460');
UPDATE `creature_template` SET `minlevel`='1', `maxlevel`='1', `faction_A`='35', `speed_walk`='1', `speed_run`='1.14286', `speed_fly`='1.14286', `baseattacktime`='2000', `rangeattacktime`='2000', `unit_class`='1', `unit_flags`='768', `HoverHeight`='1' WHERE (`entry`='70481');
UPDATE `creature_template` SET `minlevel`='92', `maxlevel`='92', `faction_A`='16', `speed_walk`='0.888888', `speed_run`='0.952381', `speed_fly`='1.14286', `baseattacktime`='2000', `rangeattacktime`='2000', `unit_class`='8', `unit_flags`='32768', `unit_flags2`='2048', `HoverHeight`='1' WHERE (`entry`='70070');
UPDATE `creature_template` SET `minlevel`='90', `maxlevel`='90', `faction_A`='35', `speed_walk`='0.4', `speed_run`='0.142857', `speed_fly`='1.14286', `baseattacktime`='2000', `rangeattacktime`='2000', `unit_class`='2', `unit_flags`='33570816', `unit_flags2`='2048', `HoverHeight`='1' WHERE (`entry`='69798');
UPDATE `creature_template` SET `minlevel`='93', `maxlevel`='93', `faction_A`='16', `speed_walk`='0.888888', `speed_run`='0.952381', `speed_fly`='1.14286', `baseattacktime`='2000', `rangeattacktime`='2000', `unit_class`='8', `unit_flags`='32768', `unit_flags2`='2048', `HoverHeight`='1' WHERE (`entry`='70074');
UPDATE `creature_template` SET `minlevel`='1', `maxlevel`='1', `faction_A`='1665', `speed_walk`='1', `speed_run`='1.14286', `speed_fly`='1.14286', `baseattacktime`='2000', `rangeattacktime`='2000', `unit_class`='1', `unit_flags`='768', `HoverHeight`='1' WHERE (`entry`='70079');
UPDATE `creature_template` SET `minlevel`='90', `maxlevel`='90', `faction_A`='35', `speed_walk`='3.2', `speed_run`='2', `speed_fly`='1.14286', `baseattacktime`='2000', `rangeattacktime`='2000', `unit_class`='2', `unit_flags`='33570816', `unit_flags2`='2048', `HoverHeight`='1' WHERE (`entry`='69813');
UPDATE `creature_template` SET `minlevel`='1', `maxlevel`='1', `faction_A`='35', `speed_walk`='1', `speed_run`='1.14286', `speed_fly`='1.14286', `baseattacktime`='2000', `rangeattacktime`='2000', `unit_class`='1', `unit_flags`='768', `unit_flags2`='67108864', `HoverHeight`='1' WHERE (`entry`='62142');
UPDATE `creature_template` SET `minlevel`='90', `maxlevel`='90', `faction_A`='35', `speed_walk`='2.8', `speed_run`='1', `speed_fly`='1.14286', `baseattacktime`='2000', `rangeattacktime`='2000', `unit_class`='2', `unit_flags`='33570816', `unit_flags2`='2048', `VehicleId`='2772', `HoverHeight`='1' WHERE (`entry`='70500');
UPDATE `creature_template` SET `minlevel`='93', `maxlevel`='93', `faction_A`='14', `speed_walk`='1', `speed_run`='1.42857', `speed_fly`='1.14286', `baseattacktime`='1500', `rangeattacktime`='2000', `unit_class`='1', `unit_flags`='32768', `unit_flags2`='4196352', `HoverHeight`='1' WHERE (`entry`='70228');
UPDATE `creature_template` SET `minlevel`='91', `maxlevel`='91', `faction_A`='16', `speed_walk`='0.888888', `speed_run`='0.952381', `speed_fly`='1.14286', `baseattacktime`='800', `rangeattacktime`='2000', `unit_class`='8', `unit_flags`='32768', `unit_flags2`='2048', `HoverHeight`='1' WHERE (`entry`='69824');
UPDATE `creature_template` SET `maxlevel`='91', `faction_A`='1665', `speed_walk`='1', `speed_run`='1.42857', `speed_fly`='1.14286', `baseattacktime`='2000', `rangeattacktime`='2000', `unit_class`='1', `unit_flags`='32768', `HoverHeight`='1' WHERE (`entry`='70099');
UPDATE `creature_template` SET `gossip_menu_id`='15615', `minlevel`='90', `maxlevel`='90', `faction_A`='35', `npcflag`='1', `speed_walk`='1.6', `speed_run`='0.571429', `speed_fly`='1.14286', `baseattacktime`='2000', `rangeattacktime`='2000', `unit_class`='1', `unit_flags`='33536', `unit_flags2`='2048', `HoverHeight`='1', `ScriptName` ='npc_wrathion' WHERE (`entry`='70100');
UPDATE `creature_template` SET `minlevel`='91', `maxlevel`='91', `faction_A`='16', `speed_walk`='0.888888', `speed_run`='0.952381', `speed_fly`='1.14286', `baseattacktime`='800', `rangeattacktime`='2000', `unit_class`='8', `unit_flags`='32768', `unit_flags2`='2048', `HoverHeight`='1' WHERE (`entry`='69827');
UPDATE `creature_template` SET `minlevel`='91', `maxlevel`='91', `faction_A`='2580', `speed_walk`='1', `speed_run`='1.42857', `speed_fly`='1.14286', `baseattacktime`='800', `rangeattacktime`='2000', `unit_class`='8', `unit_flags`='32768', `unit_flags2`='4196352', `HoverHeight`='1' WHERE (`entry`='69828');
UPDATE `creature_template` SET `minlevel`='91', `maxlevel`='91', `faction_A`='1665', `speed_walk`='1', `speed_run`='1.42857', `speed_fly`='1.14286', `baseattacktime`='2000', `rangeattacktime`='2000', `unit_class`='1', `unit_flags`='32768', `HoverHeight`='1' WHERE (`entry`='70106');
UPDATE `creature_template` SET `minlevel`='91', `maxlevel`='91', `faction_A`='16', `speed_walk`='0.888888', `speed_run`='0.952381', `speed_fly`='1.14286', `baseattacktime`='2000', `rangeattacktime`='2000', `unit_class`='8', `unit_flags`='32768', `unit_flags2`='2048', `HoverHeight`='1' WHERE (`entry`='69833');
UPDATE `creature_template` SET `minlevel`='92', `maxlevel`='92', `faction_A`='16', `speed_walk`='0.888888', `speed_run`='0.952381', `speed_fly`='1.14286', `baseattacktime`='2000', `rangeattacktime`='2000', `unit_class`='1', `unit_flags`='32768', `unit_flags2`='2048', `HoverHeight`='1' WHERE (`entry`='69835');
UPDATE `creature_template` SET `minlevel`='93', `maxlevel`='93', `faction_A`='1665', `speed_walk`='1', `speed_run`='1.42857', `speed_fly`='1.14286', `baseattacktime`='1500', `rangeattacktime`='2000', `unit_class`='8', `unit_flags`='32768', `unit_flags2`='4194304', `HoverHeight`='1' WHERE (`entry`='69837');
UPDATE `creature_template` SET `minlevel`='90', `maxlevel`='90', `faction_A`='35', `npcflag`='32768', `speed_walk`='1', `speed_run`='1', `speed_fly`='1.14286', `baseattacktime`='2000', `rangeattacktime`='2000', `unit_class`='1', `unit_flags`='768', `unit_flags2`='2048', `HoverHeight`='1' WHERE (`entry`='65183');


DELETE FROM `gossip_menu` WHERE (`entry`=15535 AND `text_id`=22315) OR (`entry`=15615 AND `text_id`=22423) OR (`entry`=15618 AND `text_id`=22441) OR (`entry`=15607 AND `text_id`=22414);
INSERT INTO `gossip_menu` (`entry`, `text_id`) VALUES
(15535, 22315), -- 70100
(15615, 22423), -- 70100
(15618, 22441), -- 70100
(15607, 22414); -- 70438

DELETE FROM `gossip_menu_option` WHERE (`menu_id`=15535 AND `id`=0) OR (`menu_id`=15615 AND `id`=0) OR (`menu_id`=15618 AND `id`=0) OR (`menu_id`=15607 AND `id`=0);
INSERT INTO `gossip_menu_option` (`menu_id`, `id`, `option_icon`, `option_text`, `box_coded`, `box_money`, `box_text`) VALUES
(15535, 0, 0, 'Пойдем!', 0, 0, ''), -- 70100
(15615, 1, 0, 'Давай сюда этих ша!', 0, 0, ''), -- 70100
(15618, 2, 0, 'Запускай!', 0, 0, ''), -- 70100
(15607, 0, 0, 'Я готова.', 0, 0, ''); -- 70438

INSERT INTO `locales_gossip_menu_option` (`menu_id`, `id`, `option_text_loc1`, `option_text_loc2`, `option_text_loc3`, `option_text_loc4`, `option_text_loc5`, `option_text_loc6`, `option_text_loc7`, `option_text_loc8`, `option_text_loc9`, `option_text_loc10`, `box_text_loc1`, `box_text_loc2`, `box_text_loc3`, `box_text_loc4`, `box_text_loc5`, `box_text_loc6`, `box_text_loc7`, `box_text_loc8`, `box_text_loc9`, `box_text_loc10`) VALUES
('15535', '0', NULL, NULL, NULL, NULL, NULL, NULL, NULL, 'Пойдем!', NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL),
('15615', '0', NULL, NULL, NULL, NULL, NULL, NULL, NULL, 'Давай сюда этих ша!', NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL),
('15618', '0', NULL, NULL, NULL, NULL, NULL, NULL, NULL, 'Запускай!', NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL),
('15607', '0', NULL, NULL, NULL, NULL, NULL, NULL, NULL, 'Я готова.', NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);


DELETE FROM `npc_text` WHERE ID IN (22315, 22423, 22441);
INSERT INTO `npc_text` (`ID`, `text0_0`, `text0_1`, `lang0`, `prob0`, `em0_0`, `em0_1`, `em0_2`, `em0_3`, `em0_4`, `em0_5`, `text1_0`, `text1_1`, `lang1`, `prob1`, `em1_0`, `em1_1`, `em1_2`, `em1_3`, `em1_4`, `em1_5`, `text2_0`, `text2_1`, `lang2`, `prob2`, `em2_0`, `em2_1`, `em2_2`, `em2_3`, `em2_4`, `em2_5`, `text3_0`, `text3_1`, `lang3`, `prob3`, `em3_0`, `em3_1`, `em3_2`, `em3_3`, `em3_4`, `em3_5`, `text4_0`, `text4_1`, `lang4`, `prob4`, `em4_0`, `em4_1`, `em4_2`, `em4_3`, `em4_4`, `em4_5`, `text5_0`, `text5_1`, `lang5`, `prob5`, `em5_0`, `em5_1`, `em5_2`, `em5_3`, `em5_4`, `em5_5`, `text6_0`, `text6_1`, `lang6`, `prob6`, `em6_0`, `em6_1`, `em6_2`, `em6_3`, `em6_4`, `em6_5`, `text7_0`, `text7_1`, `lang7`, `prob7`, `em7_0`, `em7_1`, `em7_2`, `em7_3`, `em7_4`, `em7_5`, `WDBVerified`) VALUES 
('22315', 'Ну что, я запускаю Кузню Грома? Учти, это может быть опасно.', '', '0', '1', '0', '0', '0', '0', '0', '0', '', '', '0', '0', '0', '0', '0', '0', '0', '0', '', '', '0', '0', '0', '0', '0', '0', '0', '0', '', '', '0', '0', '0', '0', '0', '0', '0', '0', '', '', '0', '0', '0', '0', '0', '0', '0', '0', '', '', '0', '0', '0', '0', '0', '0', '0', '0', '', '', '0', '0', '0', '0', '0', '0', '0', '0', '', '', '0', '0', '0', '0', '0', '0', '0', '0', '18414'),
('22423', 'Обед кончился, дамочки.', '', '0', '1', '0', '0', '0', '0', '0', '0', '', '', '0', '0', '0', '0', '0', '0', '0', '0', '', '', '0', '0', '0', '0', '0', '0', '0', '0', '', '', '0', '0', '0', '0', '0', '0', '0', '0', '', '', '0', '0', '0', '0', '0', '0', '0', '0', '', '', '0', '0', '0', '0', '0', '0', '0', '0', '', '', '0', '0', '0', '0', '0', '0', '0', '0', '', '', '0', '0', '0', '0', '0', '0', '0', '0', '18414'),
('22441', 'Мы – дворфы Черного Железа,
С большой горы недавно слезли.
Гора та Черною зовется,
В ее пещерах пиво льется.', '', '0', '1', '0', '0', '0', '0', '0', '0', '', '', '0', '0', '0', '0', '0', '0', '0', '0', '', '', '0', '0', '0', '0', '0', '0', '0', '0', '', '', '0', '0', '0', '0', '0', '0', '0', '0', '', '', '0', '0', '0', '0', '0', '0', '0', '0', '', '', '0', '0', '0', '0', '0', '0', '0', '0', '', '', '0', '0', '0', '0', '0', '0', '0', '0', '', '', '0', '0', '0', '0', '0', '0', '0', '0', '18414');

DELETE FROM `npc_spellclick_spells` WHERE (`npc_entry`=70460 AND `spell_id`=139697);
INSERT INTO `npc_spellclick_spells` (`npc_entry`, `spell_id`, `cast_flags`, `user_type`) VALUES
(70460, 139697, 1, 0);

DELETE FROM `instance_template` WHERE (`map`=1126);
INSERT INTO `instance_template` (`map`, `parent`, `script`, `allowMount`, `bonusChance`) VALUES
('1126', '0', 'instance_fall_of_shan_bu', '1', '20');

UPDATE `creature_template` SET `ScriptName` ='npc_shado_pan_defender' WHERE (`entry`='70099');
UPDATE `creature_template` SET `ScriptName` ='npc_shado_pan_warrior' WHERE (`entry`='70106');
UPDATE `creature_template` SET `ScriptName` ='npc_invisible_hunter' WHERE (`entry`='62142');
UPDATE `creature_template` SET `ScriptName` ='npc_lighting_pilar_beam_stalker' WHERE (`entry`='69798');
UPDATE `creature_template` SET `ScriptName` ='npc_lighting_pilar_spark_stalker' WHERE (`entry`='69813');
UPDATE `creature_template` SET `ScriptName` ='npc_forgemaster_vulkon' WHERE (`entry`='70074');
UPDATE `gameobject_template` SET `ScriptName` ='go_mogu_crucible' WHERE (`entry`='218910');
UPDATE `creature_template` SET `ScriptName` ='npc_shanze_shadowcaster' WHERE (`entry`='69827');
UPDATE `creature_template` SET `ScriptName` ='npc_shanze_battlemaster' WHERE (`entry`='69835');
UPDATE `creature_template` SET `ScriptName` ='npc_shanze_warrior' WHERE (`entry`='69833');
UPDATE `creature_template` SET `ScriptName` ='npc_shanze_electro_coutioner' WHERE (`entry`='70070');
UPDATE `creature_template` SET `ScriptName` ='npc_shanze_pyromancer' WHERE (`entry`='69824');
UPDATE `creature_template` SET `ScriptName` ='npc_celestial_blacksmith' WHERE (`entry`='69828');
UPDATE `creature_template` SET `ScriptName` ='npc_celestial_defender' WHERE (`entry`='69837');
UPDATE `creature_template` SET `ScriptName` ='npc_thunder_forge_second' WHERE (`entry`='70283');
UPDATE `creature_template` SET `ScriptName` ='npc_sha_beast' WHERE (`entry`='70048');
UPDATE `creature_template` SET `ScriptName` ='npc_sha_fiend' WHERE (`entry`='70039');
UPDATE `creature_template` SET `ScriptName` ='npc_sha_amalgamation' WHERE (`entry`='70228');

DELETE FROM areatrigger_scripts where entry in (840, 503, 868);
INSERT INTO `areatrigger_scripts` (`entry`, `ScriptName`) VALUES 
(840, 'at_thunder_forge_buff'),
(503, 'at_healing_orb'),
(868, 'at_power_surge');

DELETE FROM `spell_script_names` WHERE (`spell_id`=138805 AND `ScriptName`='spell_avnil_click_dummy') OR(`spell_id`=134715 AND `ScriptName`='spell_phase_shift_update') OR (`spell_id`=138869 AND `ScriptName`='spell_forging') OR (`spell_id`=140382  AND `ScriptName`='spell_thundder_forge_charging');
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
('134715', 'spell_phase_shift_update'),
('138869', 'spell_forging'),
('138805', 'spell_avnil_click_dummy'),
('140382', 'spell_thundder_forge_charging');
