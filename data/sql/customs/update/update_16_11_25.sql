-- ================================
-- MIGRACE pro mod-instance-tools
-- ================================

USE `customs`;

-- 1) DROP & CREATE NOVÉ STRUKTURY
DROP TABLE IF EXISTS `customs`.`instance_reset_catalog`;
CREATE TABLE IF NOT EXISTS `customs`.`instance_reset_catalog` (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `map_id` smallint unsigned NOT NULL,
  `difficulty` tinyint unsigned NOT NULL,
  `price_gold` int unsigned NOT NULL DEFAULT '0',
  `emblem_item` int unsigned NOT NULL DEFAULT '0',
  `emblem_count` int unsigned NOT NULL DEFAULT '0',
  `enabled` tinyint(1) NOT NULL DEFAULT '1',
  `comment` varchar(255) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  PRIMARY KEY (`id`),
  UNIQUE KEY `uq_map_diff` (`map_id`,`difficulty`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
  COMMENT='Ceník per instance pro mod-instance-tools';

-- 2) NOVÁ VÝCHOZÍ DATA
INSERT INTO `customs`.`instance_reset_catalog`
(`id`,`map_id`,`difficulty`,`price_gold`,`emblem_item`,`emblem_count`,`enabled`,`comment`)
VALUES
  (1,309,4,5,0,0,1,'Zul\'Gurub'),
  (2,409,4,10,0,0,1,'Molten Core'),
  (3,469,4,15,0,0,1,'Blackwing Lair'),
  (4,509,4,20,0,0,1,'Ahn\'Qiraj Ruins (AQ20)'),
  (5,531,4,25,0,0,1,'Ahn\'Qiraj Temple (AQ40)'),
  (6,269,1,30,37711,1,1,'Caverns Of Time: Black Morass'),
  (7,540,1,30,37711,1,1,'The Shattered Halls'),
  (8,542,1,30,37711,1,1,'The Blood Furnace'),
  (9,543,1,30,37711,1,1,'Hellfire Ramparts'),
  (10,545,1,30,37711,1,1,'The Steamvault'),
  (11,546,1,30,37711,1,1,'The Underbog'),
  (12,547,1,30,37711,1,1,'The Slave Pens'),
  (13,552,1,30,37711,1,1,'The Arcatraz'),
  (14,553,1,30,37711,1,1,'The Botanica'),
  (15,554,1,30,37711,1,1,'The Mechanar'),
  (16,555,1,30,37711,1,1,'Shadow Labyrinth'),
  (17,556,1,30,37711,1,1,'Sethekk Halls'),
  (18,557,1,30,37711,1,1,'Mana Tombs'),
  (19,558,1,30,37711,1,1,'Auchenai Crypts'),
  (20,560,1,30,37711,1,1,'Old Hillsbrad Foothills'),
  (21,585,1,30,37711,1,1,'Magisters\' Terrace'),
  (22,532,4,50,37711,2,1,'Karazhan'),
  (23,534,4,50,37711,2,1,'Battle Of Mount Hyjal'),
  (24,544,4,50,37711,2,1,'Hellfire Citadel: Magtheridon\'s Lair'),
  (25,548,4,50,37711,2,1,'Coilfang Reservoir: Serpentshrine Cavern'),
  (26,550,4,40,37711,2,1,'The Eye'),
  (27,564,4,60,37711,2,1,'Black Temple'),
  (28,565,4,60,37711,2,1,'Gruul\'s Lair'),
  (29,568,4,70,37711,2,1,'Zul\'Aman'),
  (30,580,4,80,37711,2,1,'Sunwell Plateau'),
  (31,574,1,100,37711,3,1,'Utgarde Keep'),
  (32,576,1,100,37711,3,1,'The Nexus '),
  (33,601,1,100,37711,3,1,'Azjol-Nerub'),
  (34,619,1,100,37711,3,1,'Ahn\'Kahet: The Old Kingdom'),
  (35,600,1,100,37711,3,1,'Drak\'Tharon Keep'),
  (36,604,1,100,37711,3,1,'Gundrak'),
  (37,608,1,100,37711,3,1,'Violet Hold'),
  (38,599,1,100,37711,3,1,'Halls of Stone'),
  (39,602,1,100,37711,3,1,'Halls of Lightning'),
  (40,578,1,100,37711,3,1,'The Oculus'),
  (41,595,1,100,37711,3,1,'Culling of Stratholme'),
  (42,575,1,100,37711,3,1,'Utgarde Pinnacle'),
  (43,650,1,200,37711,4,1,'Trial of the Champion'),
  (44,624,0,250,37711,5,1,'Vault of Archavon 10-man'),
  (45,624,1,275,37711,5,1,'Vault of Archavon 25-man'),
  (46,624,4,250,37711,5,0,'Vault of Archavon 10/25-man'),
  (47,632,1,300,37711,5,1,'Forge of Souls'),
  (48,658,1,300,37711,5,1,'Pit of Saron'),
  (49,668,1,300,37711,5,1,'Halls of Reflection'),
  (50,533,0,400,37711,6,1,'Naxxramas 10-man'),
  (51,533,1,450,37711,6,1,'Naxxramas 25-man'),
  (52,533,4,400,37711,6,0,'Naxxramas 10/25-man'),
  (53,615,0,400,37711,6,1,'The Obsidian Sanctum 10-man'),
  (54,615,1,450,37711,6,1,'The Obsidian Sanctum 25-man'),
  (55,615,4,400,37711,6,0,'The Obsidian Sanctum 10/25-man'),
  (56,616,0,400,37711,6,1,'The Eye of Eternity 10-man'),
  (57,616,1,450,37711,6,1,'The Eye of Eternity 25-man'),
  (58,616,4,400,37711,6,0,'The Eye of Eternity 10/25-man'),
  (59,603,0,600,37711,7,1,'Ulduar 10-man'),
  (60,603,1,625,37711,7,1,'Ulduar 25-man'),
  (61,603,4,600,37711,7,0,'Ulduar 10/25-man'),
  (62,249,0,650,37711,7,1,'Onyxia\'s Lair 10-man'),
  (63,249,1,675,37711,7,1,'Onyxia\'s Lair 25-man'),
  (64,249,4,650,37711,7,0,'Onyxia\'s Lair 10/25-man'),
  (65,649,0,700,37711,8,1,'Trial of the Crusader 10-man Normal'),
  (66,649,1,725,37711,8,1,'Trial of the Crusader 25-man Normal'),
  (67,649,2,750,37711,8,1,'Trial of the Crusader 10-man Heroic'),
  (68,649,3,800,37711,8,1,'Trial of the Crusader 25-man Heroic'),
  (69,649,4,700,37711,8,0,'Trial of the Crusader 10/25/Normal/Heroic'),
  (70,631,0,825,37711,8,1,'Icecrown Citadel 10-man Normal (default shared with heroic)'),
  (71,631,1,850,37711,9,1,'Icecrown Citadel 25-man Normal (default shared with heroic)'),
  (72,631,2,875,37711,9,0,'Icecrown Citadel 10-man Heroic'),
  (73,631,3,900,37711,9,0,'Icecrown Citadel 25-man Heroic'),
  (74,631,4,825,37711,9,0,'Icecrown Citadel 10/25/Normal/Heroic'),
  (75,724,0,850,37711,9,1,'The Ruby Sanctum 10-man Normal (default shared with heroic)'),
  (76,724,1,900,37711,10,1,'The Ruby Sanctum 25-man Normal (default shared with heroic)'),
  (77,724,2,950,37711,10,0,'The Ruby Sanctum 10-man Heroic'),
  (78,724,3,1000,37711,10,0,'The Ruby Sanctum 25-man Heroic'),
  (79,724,4,900,37711,10,0,'The Ruby Sanctum 10/25/Normal/Heroic');

-- ------------------------------------------------------------
-- Instance reset limits (clean recreate)
-- ------------------------------------------------------------

DROP TABLE IF EXISTS `customs`.`instance_reset_limit`;

CREATE TABLE `customs`.`instance_reset_limit` (
  `guid`          INT UNSIGNED       NOT NULL,
  `kind`          TINYINT UNSIGNED   NOT NULL,           -- 0=dungeon, 1=raid
  `map_id`        SMALLINT UNSIGNED  NOT NULL,
  `difficulty`    TINYINT UNSIGNED   NOT NULL,           -- viz legenda
  `window_start`  INT UNSIGNED       NOT NULL,           -- epoch boundary
  `used`          SMALLINT UNSIGNED  NOT NULL DEFAULT 0,
  PRIMARY KEY (`guid`,`kind`,`map_id`,`difficulty`),
  KEY `idx_guid_kind_window` (`guid`,`kind`,`window_start`) -- ⚡ pro rychlý SELECT s window_start >=
) ENGINE=InnoDB
  DEFAULT CHARSET=utf8mb4
  COLLATE=utf8mb4_unicode_ci;
