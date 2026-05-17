# mod-instance-tools

### 🇨🇿 [Czech version](README_CS.md)

## Description (EN)
This module allows you to:
- Reset individual or all instances
- Set a price for instance resets
- Set a limit for resetting individual instances
- Set a date and time for resetting the limit
- Teleport to VoA even when you don’t own Wintergrasp
- Set a price for teleporting to VoA

### Installation / Requirements
The module includes an autoupdater, so there’s no need to manually import any .sql files.  
For the autoupdater to function correctly, it is necessary to ensure that the database user from `(WorldDatabaseInfo) – "127.0.0.1;3306;acore;acore;acore_world"`  
has permissions for the new `customs` database as well:

```
GRANT CREATE ON *.* TO 'acore'@'127.0.0.1';
GRANT ALL PRIVILEGES ON customs.* TO 'acore'@'127.0.0.1';
FLUSH PRIVILEGES;
```

**Optional:**
- Add this line to worldserver.conf:  
  Logger.gv.customs=3,Console Server

##

### ⚠️ Warning
The module uses its own **entry** in the tables `creature_template`, `creature_template_model`.
It is necessary to ensure that these IDs are not already occupied by other content in `acore_world`:

- **Custom creatures (`creature_template.entry`):**
  - `987456`, `987457`

If you already have other records in the database using these IDs, you must shift the numbers in the module and in the SQL to another free range.

**Notes:**
- The module uses its own `customs` DB where the tables set the price for instance reset and perform checks for how many times a player has reset and whether they have the selected limit until reset.


### Commands
.instance limit / l
.i limit / l
➝ Displays whether the player has already used a limit for something.


## Database tables and their descriptions
- instance_reset_limit = This table only writes and reads values when someone resets an instance, for correctly determining the reset limit. Do not touch this unless you want to remove someone's reached limit before the configured reset time.

- instance_reset_catalog
  - id = catalog number only
  - map_id = ID of the instance
  - difficulty = usable only for raids. 0 = 10-man Normal, 1 = 25-man Normal, 2 = 10-man Heroic, 3 = 25-man Heroic, 4 = all variants. Note: if you set the value to 4, you must not use 0, 1, 2, 3, and vice versa.
  - price_gold = gold price for resetting the instance
  - emblem_item = any item required to reset the instance
  - emblem_count = number of items required to reset the instance
  - enabled = enables or disables resetting of the given instance
  - display_name = Optional custom label for gossip and the .instance limit / .i limit commands. If empty (or whitespace only), the default name is used (map name from the client, plus the raid difficulty suffix). If set, that text is shown instead.
  - comment = informational only
  - `Warning: do not change enabled for ICC and RS unless you have disabled lockout sharing in worldserver.conf.`
- worldboss_reset_catalog
  - id = catalog number only
  - creature_id = id1 from acore_world.creature (creature template entry on the spawn)
  - price_gold = gold price for resetting the boss
  - emblem_item = any item required to reset the boss
  - emblem_count = number of items required to reset the boss
  - enabled = enables or disables reset for the given world boss
  - display_name = optional custom label for gossip and
  - comment = informational only

## License
This module is licensed under the [GNU General Public License v3.0 (GPL-3.0)](LICENSE).

