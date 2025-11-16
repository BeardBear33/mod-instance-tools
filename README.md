# Instance Tools

**Instance Tools**

- Instance Tools modul přidává NPC umožňující resetovat libovolný raid/dungeon a lze nastavit ceny v DB. Taktéž umožňuje resetovat všechny raidy/dungeony, které mají aktivní lock ID, a v případě nastavené ceny se částka sčítá dohromady. Lze nastavit limit, kolikrát lze jednotlivé dungeony/raidy resetovat, a datum + čas, kdy má dojít k resetování.
- Taktéž je možnost spawnout NPC, které umožňuje teleportovat se do VoA i v případě, že frakce nevlastní Wintergrasp. Cenu lze nastavit v configu.
- NPC pro Instance Reset spawnete příkazem `.npc add 987457`.
- NPC pro VoA teleport spawnete příkazem `.npc add 987456`.

---

- The Instance Tools module adds an NPC that allows resetting any raid/dungeon, with prices configurable in the DB. It also allows resetting all raids/dungeons with an active lock ID, and if a price is set, the total is summed. You can set limits on how many times individual dungeons/raids can be reset, and also the date + time when an automatic reset should occur.
- There is also an option to spawn an NPC that allows teleporting to VoA even if the faction does not own Wintergrasp. The price can be configured.
- NPC for Instance Reset is spawned with the command `.npc add 987457`.
- NPC for VoA teleport is spawned with the command `.npc add 987456`.

---

🌍 **Výběr jazyka / Choose language:**
- [Čeština](https://github.com/BeardBear33/mod-instance-tools/blob/main/README_CS.md)
- [English](https://github.com/BeardBear33/mod-instance-tools/blob/main/README_EN.md)

> ⚠️ **Upozornění (CZ):**
> Před použitím tohoto modulu si **důkladně** přečti soubor [README_CS.md](https://github.com/BeardBear33/mod-instance-tools/blob/main/README_CS.md). Obsahuje zásadní informace o fungování a omezeních modulu.

> ⚠️ **Notice (EN):**
> Before applying this module, make sure to **carefully** read [README_EN.md](https://github.com/BeardBear33/mod-instance-tools/blob/main/README_EN.md). It contains important information about the module’s behavior and limitations.

---

🔧 Tento modul je primárně vyvíjen a testován na Ubuntu v kombinaci s:  
[AzerothCore WotLK (Playerbot branch)](https://github.com/liyunfan1223/azerothcore-wotlk/tree/Playerbot)

[mod-playerbots (master branch)](https://github.com/liyunfan1223/mod-playerbots)

🔧 This module is primarily developed and tested on Ubuntu using:  
[AzerothCore WotLK (Playerbot branch)](https://github.com/liyunfan1223/azerothcore-wotlk/tree/Playerbot)

[mod-playerbots (master branch)](https://github.com/liyunfan1223/mod-playerbots)

---

### **To‑Do list:**

🌐 **Česky**
- Rozlišování a následné samostatné resetování pro 10/25/normal/heroic raidy.

🌐 **English**
- Distinguishing and then separately resetting 10/25/normal/heroic raids.

---

## License
This module is licensed under the [GNU AFFERO GENERAL PUBLIC LICENSE](LICENSE).