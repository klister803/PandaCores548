CREATE TABLE `spell_talent_linked_spell` (
  `spellid` mediumint(8) NOT NULL DEFAULT '0',
  `spelllink` mediumint(8) NOT NULL DEFAULT '0',
  `type` tinyint(3) NOT NULL DEFAULT '0',
  `comment` text NOT NULL
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- исправление кое каких талантов
insert into `spell_talent_linked_spell` (`spellid`, `spelllink`, `type`, `comment`) values('-108415','-108503','0','при удалении таланта 108415 снимаем ауру Гримуар жертвоприношения');
insert into `spell_talent_linked_spell` (`spellid`, `spelllink`, `type`, `comment`) values('-50371','-48263','0','Дк - При снятии таланта снимаем его ауру - Великая власть крови');
insert into `spell_talent_linked_spell` (`spellid`, `spelllink`, `type`, `comment`) values('-2259','-53042','0','Алха - Миксология');
insert into `spell_talent_linked_spell` (`spellid`, `spelllink`, `type`, `comment`) values('118424','1494','1','Выслеживание животных');
insert into `spell_talent_linked_spell` (`spellid`, `spelllink`, `type`, `comment`) values('-118424','-1494','1','Выслеживание животных');
insert into `spell_talent_linked_spell` (`spellid`, `spelllink`, `type`, `comment`) values('118424','19878','1','Выслеживание демонов');
insert into `spell_talent_linked_spell` (`spellid`, `spelllink`, `type`, `comment`) values('-118424','-19878','1','Выслеживание демонов');
insert into `spell_talent_linked_spell` (`spellid`, `spelllink`, `type`, `comment`) values('118424','19879','1','Выслеживание драконов');
insert into `spell_talent_linked_spell` (`spellid`, `spelllink`, `type`, `comment`) values('-118424','-19879','1','Выслеживание драконов');
insert into `spell_talent_linked_spell` (`spellid`, `spelllink`, `type`, `comment`) values('118424','19880','1','Выслеживание элементалей');
insert into `spell_talent_linked_spell` (`spellid`, `spelllink`, `type`, `comment`) values('-118424','-19880','1','Выслеживание элементалей');
insert into `spell_talent_linked_spell` (`spellid`, `spelllink`, `type`, `comment`) values('118424','19882','1','Выслеживание великанов');
insert into `spell_talent_linked_spell` (`spellid`, `spelllink`, `type`, `comment`) values('-118424','-19882','1','Выслеживание великанов');
insert into `spell_talent_linked_spell` (`spellid`, `spelllink`, `type`, `comment`) values('118424','19883','1','Выслеживание гуманоидов');
insert into `spell_talent_linked_spell` (`spellid`, `spelllink`, `type`, `comment`) values('-118424','-19883','1','Выслеживание гуманоидов');
insert into `spell_talent_linked_spell` (`spellid`, `spelllink`, `type`, `comment`) values('118424','19884','1','Выслеживание нежити');
insert into `spell_talent_linked_spell` (`spellid`, `spelllink`, `type`, `comment`) values('-118424','-19884','1','Выслеживание нежити');
insert into `spell_talent_linked_spell` (`spellid`, `spelllink`, `type`, `comment`) values('118424','19885','1','Выслеживание скрытых');
insert into `spell_talent_linked_spell` (`spellid`, `spelllink`, `type`, `comment`) values('-118424','-19885','1','Выслеживание скрытых');
