CREATE TABLE `spell_talent_linked_spell` (
  `spellid` mediumint(8) NOT NULL DEFAULT '0',
  `spelllink` mediumint(8) NOT NULL DEFAULT '0',
  `type` tinyint(3) NOT NULL DEFAULT '0',
  `comment` text NOT NULL
) ENGINE=MyISAM DEFAULT CHARSET=utf8;
