-- Pyroblast Clearcasting Driver
DELETE FROM `spell_proc_event` WHERE `entry` = 44448;
INSERT INTO `spell_proc_event` (`entry`, `SpellFamilyName`, `SchoolMask`, `procEx`) VALUE
(44448, 3, 4, 0); -- < 0 here