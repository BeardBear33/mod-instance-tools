-- ================================
-- Custom tables and DB for mod-instance-tools
-- instance_reset_catalog
-- instance_reset_limit
-- ================================

-- Create customs DB if not exist
CREATE DATABASE IF NOT EXISTS `customs`;
USE `customs`;

-- Create instance catalog for price / enable or disable specific dungeon and raid reset
CREATE TABLE IF NOT EXISTS `instance_reset_catalog` (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `map_id` smallint unsigned NOT NULL,
  `difficulty` tinyint unsigned NOT NULL,
  `is_raid` tinyint(1) NOT NULL DEFAULT '0',
  `price_gold` int unsigned NOT NULL DEFAULT '0',
  `emblem_item` int unsigned NOT NULL DEFAULT '0',
  `emblem_count` int unsigned NOT NULL DEFAULT '0',
  `enabled` tinyint(1) NOT NULL DEFAULT '1',
  `comment` varchar(255) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  PRIMARY KEY (`id`),
  UNIQUE KEY `uq_map_diff` (`map_id`,`difficulty`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- Insert basic dungeons and raids to catalog
INSERT INTO `instance_reset_catalog` (`id`, `map_id`, `difficulty`, `is_raid`, `price_gold`, `emblem_item`, `emblem_count`, `enabled`, `comment`) VALUES
	(1, 309, 0, 1, 5, 0, 0, 1, 'Zul\'Gurub'),
	(2, 409, 0, 1, 10, 0, 0, 1, 'Molten Core'),
	(3, 469, 0, 1, 15, 0, 0, 1, 'Blackwing Lair'),
	(4, 509, 0, 1, 20, 0, 0, 1, 'Ahn\'Qiraj Ruins (AQ20)'),
	(5, 531, 0, 1, 25, 0, 0, 1, 'Ahn\'Qiraj Temple (AQ40)'),
	(6, 269, 1, 0, 30, 37711, 1, 1, 'Caverns Of Time: Black Morass'),
	(7, 540, 1, 0, 30, 37711, 1, 1, 'The Shattered Halls'),
	(8, 542, 1, 0, 30, 37711, 1, 1, 'The Blood Furnace'),
	(9, 543, 1, 0, 30, 37711, 1, 1, 'Hellfire Ramparts'),
	(10, 545, 1, 0, 30, 37711, 1, 1, 'The Steamvault'),
	(11, 546, 1, 0, 30, 37711, 1, 1, 'The Underbog'),
	(12, 547, 1, 0, 30, 37711, 1, 1, 'The Slave Pens'),
	(13, 552, 1, 0, 30, 37711, 1, 1, 'The Arcatraz'),
	(14, 553, 1, 0, 30, 37711, 1, 1, 'The Botanica'),
	(15, 554, 1, 0, 30, 37711, 1, 1, 'The Mechanar'),
	(16, 555, 1, 0, 30, 37711, 1, 1, 'Shadow Labyrinth'),
	(17, 556, 1, 0, 30, 37711, 1, 1, 'Sethekk Halls'),
	(18, 557, 1, 0, 30, 37711, 1, 1, 'Mana Tombs'),
	(19, 558, 1, 0, 30, 37711, 1, 1, 'Auchenai Crypts'),
	(20, 560, 1, 0, 30, 37711, 1, 1, 'Old Hillsbrad Foothills'),
	(21, 585, 1, 0, 30, 37711, 1, 1, 'Magisters\' Terrace'),
	(22, 532, 0, 1, 50, 37711, 2, 1, 'Karazhan'),
	(23, 534, 0, 1, 50, 37711, 2, 1, 'Battle Of Mount Hyjal'),
	(24, 544, 0, 1, 50, 37711, 2, 1, 'Hellfire Citadel: Magtheridon\'s Lair'),
	(25, 548, 0, 1, 50, 37711, 2, 1, 'Coilfang Reservoir: Serpentshrine Cavern'),
	(26, 550, 0, 1, 40, 37711, 2, 1, 'The Eye'),
	(27, 564, 0, 1, 60, 37711, 2, 1, 'Black Temple'),
	(28, 565, 0, 1, 60, 37711, 2, 1, 'Gruul\'s Lair'),
	(29, 568, 0, 1, 70, 37711, 2, 1, 'Zul\'Aman'),
	(30, 580, 0, 1, 80, 37711, 2, 1, 'Sunwell Plateau'),
	(31, 574, 1, 0, 100, 37711, 3, 1, 'Utgarde Keep'),
	(32, 576, 1, 0, 100, 37711, 3, 1, 'The Nexus'),
	(33, 601, 1, 0, 100, 37711, 3, 1, 'Azjol-Nerub'),
	(34, 619, 1, 0, 100, 37711, 3, 1, 'Ahn\'Kahet: The Old Kingdom'),
	(35, 600, 1, 0, 100, 37711, 3, 1, 'Drak\'Tharon Keep'),
	(36, 604, 1, 0, 100, 37711, 3, 1, 'Gundrak'),
	(37, 608, 1, 0, 100, 37711, 3, 1, 'Violet Hold'),
	(38, 599, 1, 0, 100, 37711, 3, 1, 'Halls of Stone'),
	(39, 602, 1, 0, 100, 37711, 3, 1, 'Halls of Lightning'),
	(40, 578, 1, 0, 100, 37711, 3, 1, 'The Oculus'),
	(41, 595, 1, 0, 100, 37711, 3, 1, 'Culling of Stratholme'),
	(42, 575, 1, 0, 100, 37711, 3, 1, 'Utgarde Pinnacle'),
	(43, 650, 1, 0, 200, 37711, 4, 1, 'Trial of the Champion'),
	(44, 624, 0, 1, 250, 37711, 5, 1, 'Vault of Archavon'),
	(45, 632, 1, 0, 300, 37711, 5, 1, 'Forge of Souls'),
	(46, 658, 1, 0, 300, 37711, 5, 1, 'Pit of Saron'),
	(47, 668, 1, 0, 300, 37711, 5, 1, 'Halls of Reflection'),
	(48, 533, 0, 1, 400, 37711, 6, 1, 'Naxxramas'),
	(49, 615, 0, 1, 400, 37711, 6, 1, 'The Obsidian Sanctum'),
	(50, 616, 0, 1, 400, 37711, 6, 1, 'The Eye of Eternity'),
	(51, 603, 0, 1, 600, 37711, 7, 1, 'Ulduar'),
	(52, 249, 0, 1, 650, 37711, 7, 1, 'Onyxia\'s Lair'),
	(53, 649, 0, 1, 750, 37711, 8, 1, 'Trial of the Crusader'),
	(54, 631, 0, 1, 900, 37711, 9, 1, 'Icecrown Citadel'),
	(55, 724, 0, 1, 1000, 37711, 10, 1, 'The Ruby Sanctum');

-- Create limit check for player instance reset
CREATE TABLE IF NOT EXISTS `instance_reset_limit` (
  `guid` int unsigned NOT NULL,
  `kind` tinyint unsigned NOT NULL,
  `map_id` smallint unsigned NOT NULL,
  `window_start` int unsigned NOT NULL,
  `used` smallint unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`guid`,`kind`,`map_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
