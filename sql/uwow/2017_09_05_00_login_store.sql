
СПЕЦОМ ПООРЧУ СКУЛЬ. НА ЛАЙВ НЕ ЛИТЬ! ОНЛИ ДЛЯ ЛОКАЛОК!

SET FOREIGN_KEY_CHECKS=0;

ALTER TABLE `account`
ADD COLUMN `battlenet_account`  int(10) NULL AFTER `activate`;

----------------------
-- Table structure for store_categories
----------------------
DROP TABLE IF EXISTS `store_categories`;
CREATE TABLE `store_categories` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `pid` int(11) unsigned NOT NULL,
  `type` enum('0','1','2','3','4','5') NOT NULL DEFAULT '0',
  `sort` int(11) unsigned NOT NULL DEFAULT '0',
  `enable` tinyint(1) unsigned NOT NULL DEFAULT '1',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB AUTO_INCREMENT=230287 DEFAULT CHARSET=utf8;

----------------------
-- Table structure for store_categories_locales
----------------------
DROP TABLE IF EXISTS `store_categories_locales`;
CREATE TABLE `store_categories_locales` (
  `category` int(11) unsigned NOT NULL,
  `en` varchar(255) NOT NULL DEFAULT '',
  `ko` varchar(255) NOT NULL DEFAULT '',
  `fr` varchar(255) NOT NULL DEFAULT '',
  `de` varchar(255) NOT NULL DEFAULT '',
  `cn` varchar(255) NOT NULL DEFAULT '',
  `tw` varchar(255) NOT NULL DEFAULT '',
  `es` varchar(255) NOT NULL DEFAULT '',
  `em` varchar(255) NOT NULL DEFAULT '',
  `ru` varchar(255) NOT NULL DEFAULT '',
  `pt` varchar(255) NOT NULL DEFAULT '',
  `it` varchar(255) NOT NULL DEFAULT '',
  `ua` varchar(255) NOT NULL DEFAULT ''
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

----------------------
-- Table structure for store_category_locales
----------------------
DROP TABLE IF EXISTS `store_category_locales`;
CREATE TABLE `store_category_locales` (
  `category` int(11) NOT NULL DEFAULT '0',
  `en` varchar(255) NOT NULL DEFAULT '',
  `ko` varchar(255) NOT NULL DEFAULT '',
  `fr` varchar(255) NOT NULL DEFAULT '',
  `de` varchar(255) NOT NULL DEFAULT '',
  `cn` varchar(255) NOT NULL DEFAULT '',
  `tw` varchar(255) NOT NULL DEFAULT '',
  `es` varchar(255) NOT NULL DEFAULT '',
  `em` varchar(255) NOT NULL DEFAULT '',
  `ru` varchar(255) NOT NULL DEFAULT '',
  `pt` varchar(255) NOT NULL DEFAULT '',
  `it` varchar(255) NOT NULL DEFAULT '',
  `ua` varchar(255) NOT NULL DEFAULT '',
  PRIMARY KEY (`category`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

----------------------
-- Table structure for store_category_realms
----------------------
DROP TABLE IF EXISTS `store_category_realms`;
CREATE TABLE `store_category_realms` (
  `category` int(11) NOT NULL DEFAULT '0',
  `realm` int(11) unsigned NOT NULL DEFAULT '0',
  `return` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `enable` tinyint(1) unsigned NOT NULL DEFAULT '1',
  UNIQUE KEY `unique` (`category`,`realm`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

----------------------
-- Table structure for store_discounts
----------------------
DROP TABLE IF EXISTS `store_discounts`;
CREATE TABLE `store_discounts` (
  `id` int(11) unsigned NOT NULL AUTO_INCREMENT,
  `realm` int(11) unsigned NOT NULL DEFAULT '0',
  `category` int(11) NOT NULL DEFAULT '0',
  `product` int(11) NOT NULL DEFAULT '0',
  `start` timestamp NULL DEFAULT NULL,
  `end` timestamp NULL DEFAULT NULL,
  `rate` float(5,2) unsigned NOT NULL DEFAULT '0.00',
  `enable` tinyint(1) unsigned NOT NULL DEFAULT '1',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB AUTO_INCREMENT=5 DEFAULT CHARSET=utf8;

----------------------
-- Table structure for store_donate_service
----------------------
DROP TABLE IF EXISTS `store_donate_service`;
CREATE TABLE `store_donate_service` (
  `id` int(11) unsigned NOT NULL AUTO_INCREMENT,
  `realm` int(11) unsigned NOT NULL,
  `account` int(11) NOT NULL DEFAULT '0',
  `bnet_account` int(11) NOT NULL DEFAULT '0',
  `guid` bigint(20) NOT NULL DEFAULT '0',
  `service` varchar(50) NOT NULL DEFAULT '',
  `cost` int(11) NOT NULL DEFAULT '0',
  `targetguid` int(11) NOT NULL DEFAULT '0',
  `dt_buy` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

----------------------
-- Table structure for store_favorites
----------------------
DROP TABLE IF EXISTS `store_favorites`;
CREATE TABLE `store_favorites` (
  `id` int(11) unsigned NOT NULL AUTO_INCREMENT,
  `realm` int(11) unsigned NOT NULL DEFAULT '0',
  `product` int(11) NOT NULL DEFAULT '0',
  `acid` int(11) unsigned NOT NULL,
  `bacid` int(11) unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`id`),
  UNIQUE KEY `unique` (`realm`,`product`,`acid`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

----------------------
-- Table structure for store_history
----------------------
DROP TABLE IF EXISTS `store_history`;
CREATE TABLE `store_history` (
  `id` int(11) unsigned NOT NULL AUTO_INCREMENT,
  `realm` int(11) unsigned NOT NULL,
  `account` int(11) unsigned NOT NULL,
  `bnet_account` int(11) unsigned NOT NULL DEFAULT '0',
  `char_guid` int(11) unsigned NOT NULL DEFAULT '0',
  `char_level` int(11) unsigned NOT NULL DEFAULT '0',
  `art_level` varchar(255) NOT NULL DEFAULT '',
  `item_guid` int(11) unsigned DEFAULT NULL,
  `item` int(11) NOT NULL DEFAULT '0',
  `bonus` varchar(11) DEFAULT NULL,
  `product` int(11) NOT NULL DEFAULT '0',
  `count` int(11) unsigned NOT NULL DEFAULT '1',
  `token` int(11) unsigned NOT NULL,
  `karma` int(1) unsigned NOT NULL,
  `return` tinyint(1) unsigned NOT NULL DEFAULT '0',
  `status` tinyint(1) unsigned NOT NULL DEFAULT '0',
  `type` enum('cp','game') NOT NULL DEFAULT 'game',
  `dt_buy` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `dt_return` timestamp NULL DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB AUTO_INCREMENT=109 DEFAULT CHARSET=utf8;

----------------------
-- Table structure for store_level_prices
----------------------
DROP TABLE IF EXISTS `store_level_prices`;
CREATE TABLE `store_level_prices` (
  `type` tinyint(1) unsigned NOT NULL DEFAULT '0',
  `realm` int(11) unsigned NOT NULL DEFAULT '0',
  `level` smallint(4) unsigned NOT NULL DEFAULT '0',
  `token` int(11) unsigned NOT NULL DEFAULT '0',
  `karma` int(11) unsigned NOT NULL DEFAULT '0',
  UNIQUE KEY `unique` (`realm`,`level`,`token`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

----------------------
-- Table structure for store_locales
----------------------
DROP TABLE IF EXISTS `store_locales`;
CREATE TABLE `store_locales` (
  `category` int(11) unsigned NOT NULL,
  `en` varchar(255) NOT NULL DEFAULT '',
  `ko` varchar(255) NOT NULL DEFAULT '',
  `fr` varchar(255) NOT NULL DEFAULT '',
  `de` varchar(255) NOT NULL DEFAULT '',
  `cn` varchar(255) NOT NULL DEFAULT '',
  `tw` varchar(255) NOT NULL DEFAULT '',
  `es` varchar(255) NOT NULL DEFAULT '',
  `em` varchar(255) NOT NULL DEFAULT '',
  `ru` varchar(255) NOT NULL DEFAULT '',
  `pt` varchar(255) NOT NULL DEFAULT '',
  `it` varchar(255) NOT NULL DEFAULT '',
  `ua` varchar(255) NOT NULL DEFAULT ''
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

----------------------
-- Table structure for store_product_locales
----------------------
DROP TABLE IF EXISTS `store_product_locales`;
CREATE TABLE `store_product_locales` (
  `product` int(11) NOT NULL DEFAULT '0',
  `en` varchar(255) NOT NULL DEFAULT '',
  `ko` varchar(255) NOT NULL DEFAULT '',
  `fr` varchar(255) NOT NULL DEFAULT '',
  `de` varchar(255) NOT NULL DEFAULT '',
  `cn` varchar(255) NOT NULL DEFAULT '',
  `tw` varchar(255) NOT NULL DEFAULT '',
  `es` varchar(255) NOT NULL DEFAULT '',
  `em` varchar(255) NOT NULL DEFAULT '',
  `ru` varchar(255) NOT NULL DEFAULT '',
  `pt` varchar(255) NOT NULL DEFAULT '',
  `it` varchar(255) NOT NULL DEFAULT '',
  `ua` varchar(255) NOT NULL DEFAULT '',
  PRIMARY KEY (`product`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

----------------------
-- Table structure for store_product_realms
----------------------
DROP TABLE IF EXISTS `store_product_realms`;
CREATE TABLE `store_product_realms` (
  `product` int(11) NOT NULL DEFAULT '0',
  `realm` int(11) unsigned NOT NULL DEFAULT '0',
  `token` int(11) unsigned NOT NULL DEFAULT '0',
  `karma` int(11) unsigned NOT NULL DEFAULT '0',
  `return` tinyint(1) unsigned NOT NULL DEFAULT '0',
  `enable` tinyint(1) unsigned NOT NULL DEFAULT '1',
  UNIQUE KEY `unique` (`realm`,`product`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

----------------------
-- Table structure for store_products
----------------------
DROP TABLE IF EXISTS `store_products`;
CREATE TABLE `store_products` (
  `id` int(11) unsigned NOT NULL AUTO_INCREMENT,
  `category` int(11) NOT NULL DEFAULT '0',
  `item` int(11) NOT NULL DEFAULT '0',
  `bonus` varchar(255) DEFAULT NULL,
  `icon` text NOT NULL,
  `quality` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `token` int(11) unsigned NOT NULL DEFAULT '0',
  `karma` int(11) unsigned NOT NULL DEFAULT '0',
  `enable` tinyint(1) unsigned NOT NULL DEFAULT '1',
  `dt` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  UNIQUE KEY `unique` (`item`,`bonus`) USING BTREE
) ENGINE=InnoDB AUTO_INCREMENT=4134 DEFAULT CHARSET=utf8;

----------------------
-- Table structure for store_products_locales
----------------------
DROP TABLE IF EXISTS `store_products_locales`;
CREATE TABLE `store_products_locales` (
  `product` int(11) unsigned NOT NULL DEFAULT '0',
  `en` varchar(255) NOT NULL DEFAULT '',
  `ko` varchar(255) NOT NULL DEFAULT '',
  `fr` varchar(255) NOT NULL DEFAULT '',
  `de` varchar(255) NOT NULL DEFAULT '',
  `cn` varchar(255) NOT NULL DEFAULT '',
  `tw` varchar(255) NOT NULL DEFAULT '',
  `es` varchar(255) NOT NULL DEFAULT '',
  `em` varchar(255) NOT NULL DEFAULT '',
  `ru` varchar(255) NOT NULL DEFAULT '',
  `pt` varchar(255) NOT NULL DEFAULT '',
  `it` varchar(255) NOT NULL DEFAULT '',
  `ua` varchar(255) NOT NULL DEFAULT '',
  PRIMARY KEY (`product`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

----------------------
-- Table structure for store_rating
----------------------
DROP TABLE IF EXISTS `store_rating`;
CREATE TABLE `store_rating` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `realm` int(11) unsigned NOT NULL DEFAULT '0',
  `product` int(11) NOT NULL DEFAULT '0',
  `rating` tinyint(1) unsigned NOT NULL DEFAULT '0',
  `acid` int(11) unsigned NOT NULL,
  `bacid` int(11) unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`id`),
  UNIQUE KEY `unique` (`realm`,`product`,`acid`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

----------------------
-- Table structure for store_statistics
----------------------
DROP TABLE IF EXISTS `store_statistics`;
CREATE TABLE `store_statistics` (
  `id` int(11) unsigned NOT NULL AUTO_INCREMENT,
  `product` int(11) NOT NULL DEFAULT '0',
  `realm` int(11) unsigned NOT NULL DEFAULT '0',
  `rating_count` int(11) unsigned NOT NULL DEFAULT '0',
  `rating_value` int(11) unsigned NOT NULL DEFAULT '0',
  `buy` int(11) unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`id`),
  UNIQUE KEY `unique` (`realm`,`product`) USING BTREE
) ENGINE=InnoDB AUTO_INCREMENT=920 DEFAULT CHARSET=utf8;
