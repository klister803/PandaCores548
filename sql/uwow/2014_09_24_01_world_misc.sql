ALTER TABLE `spell_proc_check`
ADD `specId` int(8) DEFAULT '0' NOT NULL AFTER dmgclass,
ADD `spellAttr0` int(11) DEFAULT '0' NOT NULL AFTER specId;
