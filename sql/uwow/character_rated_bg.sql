ALTER TABLE  `character_rated_bg` CHANGE  `id`  `guid` INT( 11 ) NOT NULL,
ADD  `bracket` SMALLINT( 6 ) NOT NULL AFTER  `guid`,
ADD INDEX (  `bracket` );

ALTER TABLE  `character_rated_bg` DROP PRIMARY KEY ,
ADD PRIMARY KEY (  `guid` ,  `bracket` );