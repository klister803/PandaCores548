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

UPDATE `creature_template` SET `minlevel`='90', `maxlevel`='90', `faction`='16', `speed_walk`='1.11111', `speed_run`='1.42857', `speed_fly`='1.14286', `baseattacktime`='2000', `rangeattacktime`='2000', `unit_class`='2', `dynamicflags`='4360768', `unit_flags`='32832', `unit_flags2`='2048' `HoverHeight`='1' WHERE (`entry`='68139');
UPDATE `creature_template` SET `minlevel`='90', `maxlevel`='90', `faction`='1813', `speed_walk`='1.2', `speed_run`='0.992063', `speed_fly`='1.14286', `baseattacktime`='1300', `rangeattacktime`='2000', `unit_class`='2', `dynamicflags`='4360768', `unit_flags`='0', `unit_flags2`='2048' `HoverHeight`='1' WHERE (`entry`='68137');
UPDATE `creature_template` SET `minlevel`='90', `maxlevel`='90', `faction`='1813', `speed_walk`='1', `speed_run`='0.992063', `speed_fly`='1.14286', `baseattacktime`='2000', `rangeattacktime`='2000', `unit_class`='2', `dynamicflags`='4360256', `unit_flags`='32832', `unit_flags2`='2048' `HoverHeight`='1' WHERE (`entry`='68129');
UPDATE `creature_template` SET `minlevel`='90', `maxlevel`='90', `faction`='1813', `speed_walk`='1', `speed_run`='0.992063', `speed_fly`='1.14286', `baseattacktime`='2000', `rangeattacktime`='2000', `unit_class`='2', `dynamicflags`='4358144', `unit_flags`='32768', `unit_flags2`='4196352' `HoverHeight`='1' WHERE (`entry`='68098');
UPDATE `creature_template` SET `minlevel`='90', `maxlevel`='90', `faction`='1813', `speed_walk`='1', `speed_run`='0.992063', `speed_fly`='1.14286', `baseattacktime`='2000', `rangeattacktime`='2000', `unit_class`='2', `dynamicflags`='4358144', `unit_flags`='32832', `unit_flags2`='4196352' `HoverHeight`='1' WHERE (`entry`='68096');
UPDATE `creature_template` SET `faction`='35', `speed_walk`='1.2', `speed_run`='1', `speed_fly`='1.14286', `baseattacktime`='2000', `rangeattacktime`='2000', `unit_class`='1', `dynamicflags`='1595200', `unit_flags`='33554432', `unit_flags2`='4196352' `HoverHeight`='1' WHERE (`entry`='24925');
UPDATE `creature_template` SET `minlevel`='90', `maxlevel`='90', `faction`='16', `speed_walk`='1.6', `speed_run`='1.71429', `speed_fly`='1.14286', `baseattacktime`='2000', `rangeattacktime`='2000', `unit_class`='1', `dynamicflags`='4361536', `unit_flags`='768', `unit_flags2`='2048' `HoverHeight`='1' WHERE (`entry`='68151');
UPDATE `creature_template` SET `minlevel`='90', `maxlevel`='90', `faction`='16', `speed_walk`='1', `speed_run`='1.14286', `speed_fly`='1.14286', `baseattacktime`='2000', `rangeattacktime`='2000', `unit_class`='1', `dynamicflags`='4361984', `unit_flags`='32768', `unit_flags2`='2048' `HoverHeight`='1' WHERE (`entry`='68156');
UPDATE `creature_template` SET `minlevel`='90', `maxlevel`='90', `faction`='90', `speed_walk`='1', `speed_run`='0.857143', `speed_fly`='1.14286', `baseattacktime`='2000', `rangeattacktime`='2000', `unit_class`='8', `dynamicflags`='4363072', `unit_flags`='0', `unit_flags2`='2048' `HoverHeight`='1' WHERE (`entry`='68173');
UPDATE `creature_template` SET `minlevel`='90', `maxlevel`='90', `faction`='1786', `speed_walk`='2', `speed_run`='1.71429', `speed_fly`='1.14286', `baseattacktime`='2000', `rangeattacktime`='2000', `unit_class`='1', `dynamicflags`='4363008', `unit_flags`='32832', `unit_flags2`='2099200' `HoverHeight`='1' WHERE (`entry`='68174');
UPDATE `creature_template` SET `minlevel`='90', `maxlevel`='90', `faction`='1786', `speed_walk`='2', `speed_run`='1.42857', `speed_fly`='1.14286', `baseattacktime`='2000', `rangeattacktime`='2000', `unit_class`='8', `dynamicflags`='4363072', `unit_flags`='32768', `unit_flags2`='2099200' `HoverHeight`='1' WHERE (`entry`='68175');
UPDATE `creature_template` SET `minlevel`='90', `maxlevel`='90', `faction`='1786', `speed_walk`='2', `speed_run`='1.71429', `speed_fly`='1.14286', `baseattacktime`='2000', `rangeattacktime`='2000', `unit_class`='1', `dynamicflags`='4363264', `unit_flags`='32832', `unit_flags2`='2099200' `HoverHeight`='1' WHERE (`entry`='68176');
UPDATE `creature_template` SET `minlevel`='90', `maxlevel`='90', `faction`='1786', `speed_walk`='2', `speed_run`='1.42857', `speed_fly`='1.14286', `baseattacktime`='2000', `rangeattacktime`='2000', `unit_class`='8', `dynamicflags`='4365056', `unit_flags`='32768', `unit_flags2`='2048' `HoverHeight`='1' WHERE (`entry`='68204');
UPDATE `creature_template` SET `minlevel`='90', `maxlevel`='90', `faction`='1786', `speed_walk`='2', `speed_run`='1.42857', `speed_fly`='1.14286', `baseattacktime`='2000', `rangeattacktime`='2000', `unit_class`='8', `dynamicflags`='4365120', `unit_flags`='32768', `unit_flags2`='2048' `HoverHeight`='1' WHERE (`entry`='68205');
UPDATE `creature_template` SET `minlevel`='90', `maxlevel`='90', `faction`='1786', `speed_walk`='2', `speed_run`='1.42857', `speed_fly`='1.14286', `baseattacktime`='2000', `rangeattacktime`='2000', `unit_class`='8', `dynamicflags`='4365056', `unit_flags`='32768', `unit_flags2`='2048' `HoverHeight`='1' WHERE (`entry`='68206');
UPDATE `creature_template` SET `minlevel`='93', `maxlevel`='93', `faction`='1771', `npcflag`='16777216', `speed_walk`='0.944444', `speed_run`='1.11111', `speed_fly`='1.14286', `baseattacktime`='2000', `rangeattacktime`='2000', `unit_class`='8', `dynamicflags`='4477696', `unit_flags`='256', `unit_flags2`='4194304', `VehicleId`='2745', `HoverHeight`='1' WHERE (`entry`='69964');
UPDATE `creature_template` SET `minlevel`='91', `maxlevel`='91', `faction`='1771', `speed_walk`='1', `speed_run`='1.14286', `speed_fly`='1.14286', `baseattacktime`='2000', `rangeattacktime`='2000', `unit_class`='8', `dynamicflags`='4481344', `unit_flags`='33554432', `HoverHeight`='1' WHERE (`entry`='70023');
UPDATE `creature_template` SET `minlevel`='90', `maxlevel`='90', `faction`='35', `speed_walk`='1', `speed_run`='0.992063', `speed_fly`='1.14286', `baseattacktime`='2000', `rangeattacktime`='2000', `unit_class`='1', `dynamicflags`='4481792', `unit_flags`='33554688', `unit_flags2`='4229152', `HoverHeight`='1' WHERE (`entry`='70028');
