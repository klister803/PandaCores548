ALTER TABLE `spell_linked_spell`   
  CHANGE `type2` `hastype` TINYINT(3) DEFAULT 0  NOT NULL  AFTER `hastalent`,
  ADD COLUMN `hastype2` TINYINT(3) DEFAULT 0  NOT NULL AFTER `hastalent2`,
  ADD COLUMN `actiontype` TINYINT(3) NOT NULL AFTER `removeMask`;

update spell_linked_spell set actiontype = 1 where learnspell = 1;
update spell_linked_spell set actiontype = 2 where hastype = 5;
update spell_linked_spell set hastype = 0 where hastype = 5;
update spell_linked_spell set actiontype = 3 where hastype = 6;
update spell_linked_spell set hastype = 0 where hastype = 6;
update spell_linked_spell set actiontype = 4 where hastype = 7;
update spell_linked_spell set hastype = 0 where hastype = 7;
update spell_linked_spell set actiontype = 5 where hastype = 8;
update spell_linked_spell set hastype = 0 where hastype = 8;

ALTER TABLE `spell_linked_spell`   
  DROP COLUMN `learnspell`;

update spell_linked_spell set caster = 5 where caster = 3;

ALTER TABLE `spell_linked_spell`   
  CHANGE `hastype` `hastype` TINYINT(3) DEFAULT 0  NOT NULL  AFTER `target`,
  CHANGE `hastype2` `hastype2` TINYINT(3) DEFAULT 0  NOT NULL  AFTER `hastalent`;
