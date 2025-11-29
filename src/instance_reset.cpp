// src/instance_reset.cpp
#include "ScriptMgr.h"
#include "World.h"
#include "Config.h"
#include "Chat.h"
#include "Player.h"
#include "Creature.h"
#include "DBCStores.h"
#include "DatabaseEnv.h"
#include "MapMgr.h"
#include "ScriptedGossip.h"
#include "MapInstanced.h"
#include "InstanceSaveMgr.h"
#include "SharedDefines.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Group.h"

#include <vector>
#include <ctime>
#include <string>
#include <sstream>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>

#include "itools_names.h"

namespace itools
{

using namespace Acore::ChatCommands;

// ---------------- Lokalizace ----------------
enum class Lang { CS, EN };

static inline Lang LangOpt()
{
    std::string loc = sConfigMgr->GetOption<std::string>("InstanceTools.Locale", "cs");
    std::transform(loc.begin(), loc.end(), loc.begin(), ::tolower);
    return (loc == "en" || loc == "english") ? Lang::EN : Lang::CS;
}
static inline char const* T(char const* cs, char const* en)
{
    return (LangOpt() == Lang::EN) ? en : cs;
}
static inline char const* Sep() { return "|cff808080---------------------------|r"; }

// --- Guard: jen solo nebo leader může resetovat ---
static inline bool IsSoloOrLeader(Player* p)
{
    if (Group* g = p->GetGroup())
        return g->IsLeader(p->GetGUID());
    return true;
}

static void ShowOnlyLeaderInfo(Player* player, Creature* creature)
{
    ClearGossipMenuFor(player);
    AddGossipItemFor(player, GOSSIP_ICON_CHAT,
        T("Resetovat můžeš pouze mimo partu/raid, nebo pokud jsi leader groupy.",
          "You can reset only when not in a group/raid, or if you're the group leader."),
        GOSSIP_SENDER_MAIN, 0u);
    SendGossipMenuFor(player, 1, creature->GetGUID());
}

static bool CanSeeMenu(Player* p);


// ---------------- Konfigurace ----------------
struct Price
{
    uint32 goldG       = 0;
    uint32 emblemId    = 0;
    uint32 emblemCount = 0;
    bool   configured  = false;
    bool   disabled    = false;
};

struct Cfg
{
    bool   resetEnable        = true;
    bool   checkNotInInstance = true;
    bool   checkNotInCombat   = true;
    bool   resetGroup         = true;
    uint32 heroicPerDay       = 0;
    uint32 raidPerWeek        = 0;
    bool   gmBypassLimits     = true;
    bool   limitsApplyToMember = true;
    uint8  dailyHour   = 5;
    uint8  dailyMin    = 15;
    uint8  weeklyWday  = 0;
    uint8  weeklyHour  = 5;
    uint8  weeklyMin   = 15;
};


static Cfg s;

static bool CanSeeMenu(Player* p)
{
    if (!s.resetGroup)
        return true;

    return IsSoloOrLeader(p);
}

// ---------------- Config helpers ----------------
static uint32 CUInt(char const* k, uint32 d) { return sConfigMgr->GetOption<uint32>(k, d); }
static bool   CBool(char const* k, bool d)   { return sConfigMgr->GetOption<bool>(k, d); }
static std::string CStr(char const* k, char const* d) { return sConfigMgr->GetOption<std::string>(k, d); }

static void LoadCfg()
{
    s.resetEnable        = CBool("ResetNPC.Enable", true);
    s.checkNotInInstance = CBool("ResetNPC.Checks.RequireNotInInstance", true);
    s.checkNotInCombat   = CBool("ResetNPC.Checks.RequireNotInCombat", true);
    s.resetGroup         = CBool("ResetNPC.Reset.Group", true);

    s.heroicPerDay       = CUInt("ResetNPC.Limits.HeroicPerDay", 0);
    s.raidPerWeek        = CUInt("ResetNPC.Limits.RaidPerWeek", 0);
    s.gmBypassLimits     = CBool("ResetNPC.Limits.GMBypass", true);
    s.limitsApplyToMember = CBool("ResetNPC.Limits.ApplyToMember", true);

    // ---- konfigurovatelné hranice období ----
    auto parseHHMM = [](std::string s, uint8 defH, uint8 defM) -> std::pair<uint8,uint8>
    {
        if (s.empty()) return {defH, defM};
        s.erase(0, s.find_first_not_of(" \t\r\n"));
        s.erase(s.find_last_not_of(" \t\r\n") + 1);
        size_t colon = s.find(':');
        if (colon == std::string::npos) return {defH, defM};
        std::string hs = s.substr(0, colon);
        std::string ms = s.substr(colon + 1);
        int h = std::atoi(hs.c_str());
        int m = std::atoi(ms.c_str());
        if (h < 0 || h > 23) h = defH;
        if (m < 0 || m > 59) m = defM;
        return { (uint8)h, (uint8)m };
    };

    auto parseWday = [](std::string s, uint8 def) -> uint8
    {
        auto low = [](std::string x){ std::transform(x.begin(), x.end(), x.begin(), ::tolower); return x; };
        s = low(s);

        if (!s.empty() && std::all_of(s.begin(), s.end(), [](char c){ return (c >= '0' && c <= '9'); }))
        {
            int n = std::atoi(s.c_str());
            if (n == 7) n = 0;
            if (n >= 0 && n <= 6) return (uint8)n;
            return def;
        }

        // EN
        if (s == "sun" || s == "sunday")   return 0;
        if (s == "mon" || s == "monday")   return 1;
        if (s == "tue" || s == "tuesday")  return 2;
        if (s == "wed" || s == "wednesday")return 3;
        if (s == "thu" || s == "thursday") return 4;
        if (s == "fri" || s == "friday")   return 5;
        if (s == "sat" || s == "saturday") return 6;

        // CS
        if (s == "nedele"   || s == "neděle"   || s == "ne") return 0;
        if (s == "pondeli"  || s == "pondělí"  || s == "po") return 1;
        if (s == "utery"    || s == "úterý"    || s == "ut" || s == "út") return 2;
        if (s == "streda"   || s == "středa"   || s == "st") return 3;
        if (s == "ctvrtek"  || s == "čtvrtek"  || s == "ct" || s == "čt") return 4;
        if (s == "patek"    || s == "pátek"    || s == "pa" || s == "pá") return 5;
        if (s == "sobota"   || s == "so") return 6;

        return def;
    };

    {
        auto hhmm = parseHHMM(CStr("ResetNPC.Limits.Daily.ResetTime", "05:15"), 5, 15);
        s.dailyHour = hhmm.first;
        s.dailyMin  = hhmm.second;
    }

    {
        s.weeklyWday = parseWday(CStr("ResetNPC.Limits.Weekly.ResetDay", "Sunday"), 0);
        auto hhmm = parseHHMM(CStr("ResetNPC.Limits.Weekly.ResetTime", "05:15"), 5, 15);
        s.weeklyHour = hhmm.first;
        s.weeklyMin  = hhmm.second;
    }
}

// ---------------- Utils: dungeon/raid, časová okna ----------------
static bool IsDungeon(uint32 mapId)
{
    if (MapEntry const* me = sMapStore.LookupEntry(mapId))
        return me->IsNonRaidDungeon();
    return false;
}

static bool IsRaid(uint32 mapId)
{
    if (MapEntry const* me = sMapStore.LookupEntry(mapId))
        return me->IsRaid();
    return false;
}

static time_t DailyBoundary()
{
    time_t now = std::time(nullptr);
    std::tm tmv;
#ifdef _WIN32
    localtime_s(&tmv, &now);
#else
    localtime_r(&now, &tmv);
#endif

    std::tm border = tmv;
    border.tm_hour = s.dailyHour;
    border.tm_min  = s.dailyMin;
    border.tm_sec  = 0;

    time_t today = std::mktime(&border);
    if (now < today)
    {
        border.tm_mday -= 1;
        return std::mktime(&border);
    }
    return today;
}

static time_t WeeklyBoundary()
{
    time_t now = std::time(nullptr);
    std::tm tmv;
#ifdef _WIN32
    localtime_s(&tmv, &now);
#else
    localtime_r(&now, &tmv);
#endif

    int w = tmv.tm_wday; // 0=Sunday..6=Saturday
    int backDays = (w - int(s.weeklyWday) + 7) % 7;

    std::tm border = tmv;
    border.tm_hour = s.weeklyHour;
    border.tm_min  = s.weeklyMin;
    border.tm_sec  = 0;

    time_t thisDayBoundary = std::mktime(&border);

    if (backDays == 0)
    {
        if (now < thisDayBoundary)
            backDays = 7;
    }

    border = tmv;
    border.tm_mday -= backDays;
    border.tm_hour = s.weeklyHour;
    border.tm_min  = s.weeklyMin;
    border.tm_sec  = 0;

    return std::mktime(&border);
}

static bool InstanceHasAnyBind(uint32 instanceId)
{
    std::ostringstream q;
    q << "SELECT 1 FROM character_instance WHERE instance=" << instanceId << " LIMIT 1";
    return CharacterDatabase.Query(q.str().c_str()) != nullptr;
}

static std::string PriceLine(Price const& p)
{
    std::ostringstream o;
    o << T("Cena: ", "Cost: ");
    bool any = false;

    if (p.goldG)
    {
        o << p.goldG << " " << Names::CountGoldName(p.goldG);
        any = true;
    }

    if (p.emblemId && p.emblemCount)
    {
        if (any)
            o << T(" a ", " and ");
        o << p.emblemCount << " " << Names::CountTokenName(p.emblemCount);
        any = true;
    }

    if (!any)
        o << T("zdarma", "free");

    return o.str();
}

// ===== Sdílené N/H helper pro ICC (631) a RS (724) =====
static inline bool IsIccOrRs(uint32 mapId)
{
    return mapId == 631 /*ICC*/ || mapId == 724 /*RS*/;
}

static int ReadSharedNHConf()
{
    return sConfigMgr->GetOption<int>("Instance.SharedNormalHeroicId", -1);
}

static bool PlayerShowsSharedNH(Player* pl, uint32 mapId)
{
    {
        BoundInstancesMap const& n10 = sInstanceSaveMgr->PlayerGetBoundInstances(pl->GetGUID(), RAID_DIFFICULTY_10MAN_NORMAL);
        BoundInstancesMap const& h10 = sInstanceSaveMgr->PlayerGetBoundInstances(pl->GetGUID(), RAID_DIFFICULTY_10MAN_HEROIC);
        auto itN = n10.find(mapId), itH = h10.find(mapId);
        if (itN != n10.end() && itH != h10.end() && itN->second.save && itH->second.save)
            if (itN->second.save->GetInstanceId() == itH->second.save->GetInstanceId())
                return true;
    }

    {
        BoundInstancesMap const& n25 = sInstanceSaveMgr->PlayerGetBoundInstances(pl->GetGUID(), RAID_DIFFICULTY_25MAN_NORMAL);
        BoundInstancesMap const& h25 = sInstanceSaveMgr->PlayerGetBoundInstances(pl->GetGUID(), RAID_DIFFICULTY_25MAN_HEROIC);
        auto itN = n25.find(mapId), itH = h25.find(mapId);
        if (itN != n25.end() && itH != h25.end() && itN->second.save && itH->second.save)
            if (itN->second.save->GetInstanceId() == itH->second.save->GetInstanceId())
                return true;
    }
    return false;
}

static bool IsNHSharedFor(Player* pl, uint32 mapId)
{
    if (!IsIccOrRs(mapId))
        return false;

    int c = ReadSharedNHConf();
    if (c == 1) return true;
    if (c == 0) return false;

    return PlayerShowsSharedNH(pl, mapId);
}

// ===================== Helpers: catalog/limit keys & labels =====================
static inline Difficulty DifficultyFromRaidKey(uint8 key)
{
    switch (key)
    {
        case 0: return RAID_DIFFICULTY_10MAN_NORMAL;
		case 1: return RAID_DIFFICULTY_25MAN_NORMAL;
        case 2: return RAID_DIFFICULTY_10MAN_HEROIC;
        case 3: return RAID_DIFFICULTY_25MAN_HEROIC;
        default: return RAID_DIFFICULTY_10MAN_NORMAL;
    }
}

static uint8 GetLimitDifficultyKey(uint32 mapId, Difficulty diff, bool isRaid, bool sharedNH)
{
    if (!isRaid)
        return (diff == DUNGEON_DIFFICULTY_HEROIC) ? 1 : 0;

    if (sharedNH && IsIccOrRs(mapId))
    {
        switch (diff)
        {
            case RAID_DIFFICULTY_10MAN_NORMAL:
            case RAID_DIFFICULTY_10MAN_HEROIC: return 0; // 10M shared
            case RAID_DIFFICULTY_25MAN_NORMAL:
            case RAID_DIFFICULTY_25MAN_HEROIC: return 1; // 25M shared
            default: return 0;
        }
    }

    switch (diff)
    {
        case RAID_DIFFICULTY_10MAN_NORMAL:  return 0;
        case RAID_DIFFICULTY_25MAN_NORMAL:  return 1;
        case RAID_DIFFICULTY_10MAN_HEROIC:  return 2;
        case RAID_DIFFICULTY_25MAN_HEROIC:  return 3;
        default: return 0;
    }
}

static inline const char* RaidKeyLabel(uint8 key, bool sharedNH)
{
    if (sharedNH)
    {
        switch (key)
        {
            case 0: return " (10M)";
            case 1: return " (25M)";
            case 4: return ""; // all
            default: return "";
        }
    }
    else
    {
        switch (key)
        {
            case 0: return " (10M-Normal)";
            case 1: return " (25M-Normal)";
            case 2: return " (10M-Heroic)";
            case 3: return " (25M-Heroic)";
            case 4: return "";
            default: return "";
        }
    }
}


// ---------------- Limity přes customs.instance_reset_limit ----------------
enum class LimitKind : uint8
{
    DungeonDay = 0,
    RaidWeek   = 1
};

static bool CheckAndConsumeLimit(Player* player, bool isRaid, uint32 mapId, uint8 diffKey, bool consume, bool sendError)
{
    uint32 maxCount = isRaid ? s.raidPerWeek : s.heroicPerDay;
    if (maxCount == 0)
        return true;
    if (s.gmBypassLimits && player->IsGameMaster())
        return true;

    LimitKind lk = isRaid ? LimitKind::RaidWeek : LimitKind::DungeonDay;
    uint32 guidLow = player->GetGUID().GetCounter();
    uint32 boundary = isRaid ? uint32(WeeklyBoundary()) : uint32(DailyBoundary());

    uint32 windowStart = 0;
    uint32 used = 0;

    {
        std::ostringstream q;
        q << "SELECT window_start, used FROM customs.instance_reset_limit "
          << "WHERE guid=" << guidLow
          << " AND kind=" << uint32(lk)
          << " AND map_id=" << mapId
          << " AND difficulty=" << uint32(diffKey);
        if (QueryResult res = CharacterDatabase.Query(q.str().c_str()))
        {
            Field* f = res->Fetch();
            windowStart = f[0].Get<uint32>();
            used        = f[1].Get<uint32>();
        }
    }

    if (windowStart < boundary)
    {
        windowStart = boundary;
        used = 0;
    }

    if (used >= maxCount)
    {
        if (sendError)
        {
            if (isRaid)
                ChatHandler(player->GetSession()).SendSysMessage(T("[Reset] Týdenní limit resetů pro tento raid/difficultu je vyčerpán.",
                                                                   "[Reset] Weekly reset limit for this raid/difficulty has been reached."));
            else
                ChatHandler(player->GetSession()).SendSysMessage(T("[Reset] Denní limit resetů pro tento dungeon je vyčerpán.",
                                                                   "[Reset] Daily reset limit for this dungeon has been reached."));
        }
        return false;
    }

    if (!consume)
        return true;

    ++used;

    std::ostringstream up;
    up << "REPLACE INTO customs.instance_reset_limit (guid,kind,map_id,difficulty,window_start,used) VALUES ("
       << guidLow << ","
       << uint32(lk) << ","
       << mapId << ","
       << uint32(diffKey) << ","
       << windowStart << ","
       << used << ")";
    CharacterDatabase.DirectExecute(up.str().c_str());

    return true;
}

// ---------------- Ceník per instance: customs.instance_reset_catalog ----------------
static Price PriceForInstance(uint32 mapId, Difficulty diff, bool isRaid)
{
    Price p;

    auto LoadSingleRow = [&](std::ostringstream& q) -> bool
    {
        if (QueryResult res = WorldDatabase.Query(q.str().c_str()))
        {
            Field* f      = res->Fetch();
            uint8 enabled = f[0].Get<uint8>();

            p.configured  = true;
            p.disabled    = (enabled == 0);

            if (!p.disabled)
            {
                p.goldG       = f[1].Get<uint32>();
                p.emblemId    = f[2].Get<uint32>();
                p.emblemCount = f[3].Get<uint32>();
            }
            return true;
        }
        return false;
    };

    if (isRaid)
    {
        uint32 catDiff = 0;
        switch (diff)
        {
            case RAID_DIFFICULTY_10MAN_NORMAL:  catDiff = 0; break;
            case RAID_DIFFICULTY_25MAN_NORMAL:  catDiff = 1; break;
            case RAID_DIFFICULTY_10MAN_HEROIC:  catDiff = 2; break;
            case RAID_DIFFICULTY_25MAN_HEROIC:  catDiff = 3; break;
            default:                            catDiff = 0; break;
        }

        std::ostringstream q;
        q << "SELECT enabled, price_gold, emblem_item, emblem_count "
          << "FROM customs.instance_reset_catalog "
          << "WHERE map_id=" << mapId
          << " AND difficulty=" << catDiff;

        if (!LoadSingleRow(q))
        {
            p.configured = false;
            p.disabled   = true;
        }
        return p;
    }
    else
    {
        std::ostringstream q;
        q << "SELECT enabled, price_gold, emblem_item, emblem_count "
          << "FROM customs.instance_reset_catalog "
          << "WHERE map_id=" << mapId
          << " AND difficulty=" << uint32(diff);

        if (!LoadSingleRow(q))
        {
            p.configured = false;
            p.disabled   = true;
        }
        return p;
    }
}


// ---------------- Stav menu a logika resetu ----------------
enum class LockKind : uint8 { Dungeon = 0, Raid = 1 };

struct LockRow
{
    uint32      instanceId = 0;
    uint32      mapId      = 0;
    Difficulty  diff       = DUNGEON_DIFFICULTY_NORMAL;
    uint8       diffKey    = 0;
    LockKind    kind       = LockKind::Dungeon;
    Price       price;
    std::string name;
};

struct MenuState
{
    std::vector<LockRow> dungeons;
    std::vector<LockRow> raids;
};

static std::unordered_map<ObjectGuid::LowType, MenuState> s_menu;

static std::string GetMapName(uint32 mapId)
{
    if (MapEntry const* me = sMapStore.LookupEntry(mapId))
    {
        LocaleConstant loc = sWorld->GetDefaultDbcLocale();
        if (loc < MAX_LOCALES)
        {
            if (me->name[loc] && *me->name[loc])
                return std::string(me->name[loc]);
        }
        if (me->name[0] && *me->name[0])
            return std::string(me->name[0]);
    }
    return T("Neznámá instance", "Unknown instance");
}

static MenuState BuildMenuStateFor(Player* pl)
{
    MenuState ms;

    std::unordered_set<uint32> raidMapsPresent;

    std::unordered_set<uint32> seenDungeonMaps;

    struct PairKey { uint32 mapId; uint8 key; };
    struct PairHash {
        size_t operator()(PairKey const& p) const noexcept {
            return (size_t(p.mapId) << 8) ^ size_t(p.key);
        }
    };
    struct PairEq {
        bool operator()(PairKey const& a, PairKey const& b) const noexcept {
            return a.mapId == b.mapId && a.key == b.key;
        }
    };
    std::unordered_set<PairKey, PairHash, PairEq> seenRaidPairs;

    auto collect = [&](Difficulty d)
    {
        BoundInstancesMap const& binds = sInstanceSaveMgr->PlayerGetBoundInstances(pl->GetGUID(), d);
        for (auto const& kv : binds)
        {
            uint32 mapId = kv.first;
            InstanceSave const* save = kv.second.save;
            bool perm = kv.second.perm;
            if (!save || !perm)
                continue;

            if (mapId == pl->GetMapId())
                continue;

            bool raid = IsRaid(mapId);
            bool dung = IsDungeon(mapId);
            if (!raid && !dung)
                continue;

            if (dung)
            {
                if (seenDungeonMaps.find(mapId) != seenDungeonMaps.end())
                    continue;
                seenDungeonMaps.insert(mapId);

                LockRow row;
                row.instanceId = save->GetInstanceId();
                row.mapId      = mapId;
                row.diff       = DUNGEON_DIFFICULTY_HEROIC;
                row.diffKey    = GetLimitDifficultyKey(mapId, row.diff, /*isRaid=*/false, /*sharedNH=*/false); // 1 (heroic)
                row.kind       = LockKind::Dungeon;
                row.price      = PriceForInstance(mapId, row.diff, false);
                if (row.price.disabled)
                    continue;
                row.name       = GetMapName(mapId);
                ms.dungeons.push_back(row);
            }
            else
			{
				bool shared = IsNHSharedFor(pl, mapId);
			
				uint8 key = GetLimitDifficultyKey(mapId, d, /*isRaid=*/true, /*sharedNH=*/shared);
			
				PairKey pk{ mapId, key };
				if (seenRaidPairs.find(pk) != seenRaidPairs.end())
					continue;
				seenRaidPairs.insert(pk);
			
				raidMapsPresent.insert(mapId);
			
				LockRow row;
				row.instanceId = save->GetInstanceId();
				row.mapId      = mapId;
			
				if (shared && IsIccOrRs(mapId))
				{
					Difficulty rep = (key == 0) ? RAID_DIFFICULTY_10MAN_HEROIC : RAID_DIFFICULTY_25MAN_HEROIC;
					Price pHero = PriceForInstance(mapId, rep, true);
					if (pHero.configured && !pHero.disabled)
					{
						row.diff  = rep;
						row.price = pHero;
					}
					else
					{
						rep = (key == 0) ? RAID_DIFFICULTY_10MAN_NORMAL : RAID_DIFFICULTY_25MAN_NORMAL;
						row.diff  = rep;
						row.price = PriceForInstance(mapId, rep, true);
					}
			
					if (row.price.disabled)
						continue;
			
					row.diffKey = key;
					row.name    = GetMapName(mapId) + std::string(RaidKeyLabel(key, /*sharedNH=*/true));
				}
				else
				{
					row.diff    = DifficultyFromRaidKey(key);
					row.diffKey = key;
					row.price   = PriceForInstance(mapId, row.diff, true);
					if (row.price.disabled)
						continue;
					row.name    = GetMapName(mapId) + std::string(RaidKeyLabel(key, /*sharedNH=*/false));
				}
			
				row.kind = LockKind::Raid;
				ms.raids.push_back(row);
			}
        }
    };

    collect(DUNGEON_DIFFICULTY_HEROIC);

    collect(RAID_DIFFICULTY_10MAN_NORMAL);
    collect(RAID_DIFFICULTY_25MAN_NORMAL);
    collect(RAID_DIFFICULTY_10MAN_HEROIC);
    collect(RAID_DIFFICULTY_25MAN_HEROIC);

    for (uint32 mapId : raidMapsPresent)
    {
        std::ostringstream q;
        q << "SELECT enabled, price_gold, emblem_item, emblem_count "
          << "FROM customs.instance_reset_catalog "
          << "WHERE map_id=" << mapId << " AND difficulty=4";

        Price pAll;
        bool hasRow = false;
        if (QueryResult res = WorldDatabase.Query(q.str().c_str()))
        {
            Field* f = res->Fetch();
            uint8 enabled = f[0].Get<uint8>();
            pAll.configured  = true;
            pAll.disabled    = (enabled == 0);
            if (!pAll.disabled)
            {
                pAll.goldG       = f[1].Get<uint32>();
                pAll.emblemId    = f[2].Get<uint32>();
                pAll.emblemCount = f[3].Get<uint32>();
            }
            hasRow = true;
        }

        if (hasRow && !pAll.disabled)
        {
            PairKey pk{ mapId, 4 };
            if (seenRaidPairs.find(pk) == seenRaidPairs.end())
            {
                seenRaidPairs.insert(pk);

                LockRow row;
                row.instanceId = 0;
                row.mapId      = mapId;
                row.diff       = RAID_DIFFICULTY_10MAN_NORMAL;
                row.diffKey    = 4;
                row.kind       = LockKind::Raid;
                row.price      = pAll;
                row.name       = GetMapName(mapId);

                ms.raids.push_back(row);
            }
        }
    }

    return ms;
}

static Price SumPrices(std::vector<LockRow> const& v)
{
    Price out;
    if (v.empty())
        return out;

    bool itemSet = false;
    uint32 commonItem = 0;
    bool sameItem = true;

    for (auto const& r : v)
    {
        out.goldG += r.price.goldG;

        if (r.price.emblemId && r.price.emblemCount)
        {
            if (!itemSet)
            {
                itemSet = true;
                commonItem = r.price.emblemId;
                out.emblemCount += r.price.emblemCount;
            }
            else if (r.price.emblemId == commonItem)
            {
                out.emblemCount += r.price.emblemCount;
            }
            else
            {
                sameItem = false;
            }
        }
    }

    if (itemSet && sameItem)
        out.emblemId = commonItem;
    else
    {
        out.emblemId = 0;
        out.emblemCount = 0;
    }

    return out;
}

static bool DoResetForList(Player* player, std::vector<LockRow> const& list, Price const& price, LockKind kind)
{
    if (list.empty())
    {
        ChatHandler(player->GetSession()).SendSysMessage(T("[Reset] Není nic k resetování.", "[Reset] There is nothing to reset."));
        return false;
    }

    if (s.checkNotInCombat && player->IsInCombat())
    {
        ChatHandler(player->GetSession()).SendSysMessage(T("[Reset] Jsi v boji.", "[Reset] You are in combat."));
        return false;
    }

    if (s.checkNotInInstance && player->GetMap()->IsDungeon())
    {
        ChatHandler(player->GetSession()).SendSysMessage(T("[Reset] Nelze provést uvnitř instance.", "[Reset] Cannot be performed inside an instance."));
        return false;
    }
	
	if (s.resetGroup)
	{
		if (Group* g = player->GetGroup())
		{
			if (!g->IsLeader(player->GetGUID()))
			{
				ChatHandler(player->GetSession()).SendSysMessage(
					T("[Reset] Resetovat můžeš pouze mimo partu/raid, nebo pokud jsi leader groupy.",
					"[Reset] You can reset only when not in a group/raid, or if you're the group leader."));
				return false;
			}
		}
	}

    bool isRaid = (kind == LockKind::Raid);

    static Difficulty const dungeonDiffs[] =
    {
        DUNGEON_DIFFICULTY_HEROIC
    };

    static Difficulty const raidDiffs[] =
    {
        RAID_DIFFICULTY_10MAN_NORMAL,
        RAID_DIFFICULTY_25MAN_NORMAL,
        RAID_DIFFICULTY_10MAN_HEROIC,
        RAID_DIFFICULTY_25MAN_HEROIC
    };

    uint32 totalResets = 0;

    for (auto const& r : list)
    {
        if (!CheckAndConsumeLimit(player, isRaid, r.mapId, r.diffKey, false, false))
            continue;

        if (!CheckAndConsumeLimit(player, isRaid, r.mapId, r.diffKey, true, false))
            continue;

        std::vector<Player*> mapTargets;
        mapTargets.push_back(player);

		Group* group = player->GetGroup();
		
		bool canAffectGroup = s.resetGroup && group && group->IsLeader(player->GetGUID());

        if (canAffectGroup)
        {
            for (GroupReference* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
            {
                Player* member = itr->GetSource();
                if (!member || member == player)
                    continue;

                bool shares = false;
                if (kind == LockKind::Dungeon)
                {
                    for (Difficulty d : dungeonDiffs)
                    {
                        BoundInstancesMap const& binds = sInstanceSaveMgr->PlayerGetBoundInstances(member->GetGUID(), d);
                        auto it = binds.find(r.mapId);
                        if (it != binds.end() && it->second.save && it->second.perm)
                        {
                            shares = true;
                            break;
                        }
                    }
                }
				
				else // LockKind::Raid
				{
					auto sharesInMode = [&](Difficulty d) -> bool
					{
						BoundInstancesMap const& leaderBinds = sInstanceSaveMgr->PlayerGetBoundInstances(player->GetGUID(), d);
						auto itLeader = leaderBinds.find(r.mapId);
						if (itLeader == leaderBinds.end() || !itLeader->second.save || !itLeader->second.perm)
							return false;
				
						uint32 instanceId = itLeader->second.save->GetInstanceId();
				
						BoundInstancesMap const& memberBinds = sInstanceSaveMgr->PlayerGetBoundInstances(member->GetGUID(), d);
						auto itMem = memberBinds.find(r.mapId);
						return itMem != memberBinds.end() && itMem->second.save && itMem->second.perm
							&& itMem->second.save->GetInstanceId() == instanceId;
					};
				
					bool sharedNH = IsIccOrRs(r.mapId) && IsNHSharedFor(player, r.mapId);
				
					if (r.diffKey == 4)
					{
						shares = sharesInMode(RAID_DIFFICULTY_10MAN_NORMAL)
							|| sharesInMode(RAID_DIFFICULTY_10MAN_HEROIC)
							|| sharesInMode(RAID_DIFFICULTY_25MAN_NORMAL)
							|| sharesInMode(RAID_DIFFICULTY_25MAN_HEROIC);
					}
					else if (sharedNH && r.diffKey == 0)
					{
						shares = sharesInMode(RAID_DIFFICULTY_10MAN_NORMAL)
							|| sharesInMode(RAID_DIFFICULTY_10MAN_HEROIC);
					}
					else if (sharedNH && r.diffKey == 1)
					{
						shares = sharesInMode(RAID_DIFFICULTY_25MAN_NORMAL)
							|| sharesInMode(RAID_DIFFICULTY_25MAN_HEROIC);
					}
					else
					{
						shares = sharesInMode(DifficultyFromRaidKey(r.diffKey));
					}
				}

                if (!shares)
                    continue;

                if (s.limitsApplyToMember)
                {
                    if (!CheckAndConsumeLimit(member, isRaid, r.mapId, r.diffKey, false, false))
                        continue;

                    if (!CheckAndConsumeLimit(member, isRaid, r.mapId, r.diffKey, true, false))
                        continue;
                }

                mapTargets.push_back(member);
            }
        }

        if (mapTargets.empty())
            continue;

        if (kind == LockKind::Dungeon)
        {
            for (Difficulty d : dungeonDiffs)
            {
                BoundInstancesMap const& leaderBinds = sInstanceSaveMgr->PlayerGetBoundInstances(player->GetGUID(), d);
                auto itLeader = leaderBinds.find(r.mapId);
                if (itLeader == leaderBinds.end() || !itLeader->second.save || !itLeader->second.perm)
                    continue;

                uint32 instanceId = itLeader->second.save->GetInstanceId();

                for (Player* target : mapTargets)
                {
                    BoundInstancesMap const& binds = sInstanceSaveMgr->PlayerGetBoundInstances(target->GetGUID(), d);
                    auto it = binds.find(r.mapId);
                    if (it == binds.end() || !it->second.save)
                        continue;
                    if (it->second.save->GetInstanceId() != instanceId)
                        continue;

                    sInstanceSaveMgr->PlayerUnbindInstance(target->GetGUID(), r.mapId, d, true, target);
                }

                bool someoneStillBound = InstanceHasAnyBind(instanceId);

                if (!someoneStillBound)
                {
                    {
                        std::ostringstream qq;
                        qq << "DELETE FROM instance WHERE id=" << instanceId;
                        CharacterDatabase.Execute(qq.str().c_str());
                    }
                    {
                        std::ostringstream qq;
                        qq << "DELETE FROM creature_respawn WHERE instanceId=" << instanceId;
                        CharacterDatabase.Execute(qq.str().c_str());
                    }
                    {
                        std::ostringstream qq;
                        qq << "DELETE FROM gameobject_respawn WHERE instanceId=" << instanceId;
                        CharacterDatabase.Execute(qq.str().c_str());
                    }
                    {
                        std::ostringstream qq;
                        qq << "DELETE FROM instance_saved_go_state_data WHERE id=" << instanceId;
                        CharacterDatabase.Execute(qq.str().c_str());
                    }

                    if (Map* live = sMapMgr->FindMap(r.mapId, instanceId))
                        if (!live->HavePlayers())
                            live->UnloadAll();
                }

                ++totalResets;
            }
        }
		
		else // LockKind::Raid
		{
			auto resetOneMode = [&](Difficulty d) -> bool
			{
				BoundInstancesMap const& leaderBinds = sInstanceSaveMgr->PlayerGetBoundInstances(player->GetGUID(), d);
				auto itLeader = leaderBinds.find(r.mapId);
				if (itLeader == leaderBinds.end() || !itLeader->second.save || !itLeader->second.perm)
					return false;
		
				uint32 instanceId = itLeader->second.save->GetInstanceId();
		
				for (Player* target : mapTargets)
				{
					BoundInstancesMap const& binds = sInstanceSaveMgr->PlayerGetBoundInstances(target->GetGUID(), d);
					auto it = binds.find(r.mapId);
					if (it == binds.end() || !it->second.save)
						continue;
					if (it->second.save->GetInstanceId() != instanceId)
						continue;
		
					sInstanceSaveMgr->PlayerUnbindInstance(target->GetGUID(), r.mapId, d, true, target);
				}
		
				bool someoneStillBound = InstanceHasAnyBind(instanceId);
		
				if (!someoneStillBound)
				{
					{
						std::ostringstream qq; qq << "DELETE FROM instance WHERE id=" << instanceId;
						CharacterDatabase.Execute(qq.str().c_str());
					}
					{
						std::ostringstream qq; qq << "DELETE FROM creature_respawn WHERE instanceId=" << instanceId;
						CharacterDatabase.Execute(qq.str().c_str());
					}
					{
						std::ostringstream qq; qq << "DELETE FROM gameobject_respawn WHERE instanceId=" << instanceId;
						CharacterDatabase.Execute(qq.str().c_str());
					}
					{
						std::ostringstream qq; qq << "DELETE FROM instance_saved_go_state_data WHERE id=" << instanceId;
						CharacterDatabase.Execute(qq.str().c_str());
					}
		
					if (Map* live = sMapMgr->FindMap(r.mapId, instanceId))
						if (!live->HavePlayers())
							live->UnloadAll();
				}
				return true;
			};
		
			bool sharedNH = IsIccOrRs(r.mapId) && IsNHSharedFor(player, r.mapId);
		
			if (r.diffKey == 4)
			{
				for (Difficulty d : raidDiffs)
					if (resetOneMode(d))
						++totalResets;
			}
			else if (sharedNH && r.diffKey == 0)
			{
				if (resetOneMode(RAID_DIFFICULTY_10MAN_NORMAL))  ++totalResets;
				if (resetOneMode(RAID_DIFFICULTY_10MAN_HEROIC))  ++totalResets;
			}
			else if (sharedNH && r.diffKey == 1)
			{
				if (resetOneMode(RAID_DIFFICULTY_25MAN_NORMAL))  ++totalResets;
				if (resetOneMode(RAID_DIFFICULTY_25MAN_HEROIC))  ++totalResets;
			}
			else
			{
				Difficulty md = DifficultyFromRaidKey(r.diffKey);
				if (resetOneMode(md))
					++totalResets;
			}
		}

    }

    if (totalResets == 0)
    {
        ChatHandler(player->GetSession()).SendSysMessage(T("[Reset] Nic nebylo resetováno (limit nebo lock už mohl zmizet).",
                                                           "[Reset] Nothing was reset (limit reached or lock already gone)."));
        return false;
    }

    if (price.goldG)
        player->ModifyMoney(-int64(uint64(price.goldG) * GOLD));
    if (price.emblemId && price.emblemCount)
        player->DestroyItemCount(price.emblemId, price.emblemCount, true);

    player->SendRaidInfo();

    ChatHandler(player->GetSession()).SendSysMessage(T("[Reset] Hotovo.", "[Reset] Done."));

    return true;
}

// ---------------- Gossip akce ----------------
enum ResetGossipAction : uint32
{
    ACT_ROOT              = 1,

    ACT_CAT_DUNGEON       = 10,
    ACT_CAT_RAID          = 11,

    ACT_DUNGEON_RESET_ALL = 20,
    ACT_RAID_RESET_ALL    = 21,

    ACT_DUNGEON_ITEM_BASE    = 1000,
    ACT_RAID_ITEM_BASE       = 2000,

    ACT_DUNGEON_CONFIRM_BASE = 3000,
    ACT_RAID_CONFIRM_BASE    = 4000,

    ACT_DUNGEON_CONFIRM_ALL  = 30,
    ACT_RAID_CONFIRM_ALL     = 31,

    ACT_BACK_ROOT            = 9000,
    ACT_SEPARATOR            = 9199
};

static void ShowRoot(Player* player, Creature* creature);
static void ShowCategory(Player* player, Creature* creature, LockKind kind, bool refresh = false);
static void ShowConfirmSingle(Player* player, Creature* creature, LockKind kind, uint32 index);
static void ShowConfirmAll(Player* player, Creature* creature, LockKind kind);

// ------- ShowRoot --------
static void ShowRoot(Player* player, Creature* creature)
{
    if (!s.resetEnable)
        return;

	if (!CanSeeMenu(player))
	{
		ShowOnlyLeaderInfo(player, creature);
		return;
	}

    ObjectGuid::LowType key = player->GetGUID().GetCounter();
    s_menu[key] = BuildMenuStateFor(player);
    MenuState& st = s_menu[key];

    ClearGossipMenuFor(player);

    std::vector<std::string> cappedDungeons;
    std::vector<std::string> cappedRaids;

    bool anyDungeonAvailable = false;
    bool anyRaidAvailable    = false;

    for (auto const& row : st.dungeons)
    {
        if (CheckAndConsumeLimit(player, false, row.mapId, row.diffKey, false, false))
            anyDungeonAvailable = true;
        else
            cappedDungeons.push_back(row.name);
    }

    for (auto const& row : st.raids)
    {
        if (CheckAndConsumeLimit(player, true, row.mapId, row.diffKey, false, false))
            anyRaidAvailable = true;
        else
            cappedRaids.push_back(row.name);
    }

    bool hasAnyAvailable = anyDungeonAvailable || anyRaidAvailable;
    bool hasAnyCapInfo   = !cappedDungeons.empty() || !cappedRaids.empty();

    if (!hasAnyAvailable)
    {
        AddGossipItemFor(player, GOSSIP_ICON_CHAT,
            T("Momentálně nemáš žádný Dungeon nebo Raid k odemčení",
              "You currently have no Dungeon or Raid to unlock"),
            GOSSIP_SENDER_MAIN, ACT_SEPARATOR);
    }

    if (!cappedRaids.empty())
    {
        std::ostringstream raidInfo;
        raidInfo << T("Dosáhl jsi limitu pro raidy: ", "You reached the limit for raids: ");
        bool first = true;
        for (std::string const& name : cappedRaids)
        {
            if (!first)
                raidInfo << ", ";
            raidInfo << name;
            first = false;
        }
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, raidInfo.str(), GOSSIP_SENDER_MAIN, ACT_SEPARATOR);
    }

    if (!cappedDungeons.empty())
    {
        std::ostringstream dungInfo;
        dungInfo << T("Dosáhl jsi limitu pro dungeony: ", "You reached the limit for dungeons: ");
        bool first = true;
        for (std::string const& name : cappedDungeons)
        {
            if (!first)
                dungInfo << ", ";
            dungInfo << name;
            first = false;
        }
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, dungInfo.str(), GOSSIP_SENDER_MAIN, ACT_SEPARATOR);
    }

    bool hasInfoLines = (!hasAnyAvailable) || hasAnyCapInfo;

    if (hasInfoLines && hasAnyAvailable)
        AddGossipItemFor(player, 0, Sep(), GOSSIP_SENDER_MAIN, ACT_SEPARATOR);

    if (anyDungeonAvailable)
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, T("Dungeony", "Dungeons"), GOSSIP_SENDER_MAIN, ACT_CAT_DUNGEON);
    if (anyRaidAvailable)
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, T("Raidy", "Raids"),       GOSSIP_SENDER_MAIN, ACT_CAT_RAID);

    SendGossipMenuFor(player, 1, creature->GetGUID());
}

// ------- ShowRoot --------

static void ShowCategory(Player* player, Creature* creature, LockKind kind, bool refresh /*=false*/)
{
    if (!s.resetEnable)
        return;

    ObjectGuid::LowType key = player->GetGUID().GetCounter();
    if (refresh || s_menu.find(key) == s_menu.end())
        s_menu[key] = BuildMenuStateFor(player);

    MenuState& st = s_menu[key];
    std::vector<LockRow>* vec = (kind == LockKind::Dungeon) ? &st.dungeons : &st.raids;

    if (!vec || vec->empty())
    {
        ShowRoot(player, creature);
        return;
    }

    bool isRaid = (kind == LockKind::Raid);

    ClearGossipMenuFor(player);

    uint32 availableCount = 0;
    for (auto const& row : *vec)
    {
        if (CheckAndConsumeLimit(player, isRaid, row.mapId, row.diffKey, false, false))
            ++availableCount;
    }

    if (availableCount >= 2)
    {
        uint32 action = (kind == LockKind::Dungeon) ? ACT_DUNGEON_RESET_ALL : ACT_RAID_RESET_ALL;
        AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, T("Resetovat vše", "Reset all"), GOSSIP_SENDER_MAIN, action);
    }

    for (uint32 i = 0; i < vec->size(); ++i)
    {
        LockRow const& row = (*vec)[i];

        if (!CheckAndConsumeLimit(player, isRaid, row.mapId, row.diffKey, false, false))
            continue;

        uint32 action = (kind == LockKind::Dungeon)
            ? (ACT_DUNGEON_ITEM_BASE + i)
            : (ACT_RAID_ITEM_BASE + i);

        AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, row.name.c_str(), GOSSIP_SENDER_MAIN, action);
    }

    AddGossipItemFor(player, 0, Sep(), GOSSIP_SENDER_MAIN, ACT_SEPARATOR);
    AddGossipItemFor(player, GOSSIP_ICON_TAXI, T("Zpátky", "Back"), GOSSIP_SENDER_MAIN, ACT_BACK_ROOT);

    SendGossipMenuFor(player, 1, creature->GetGUID());
}

static void ShowConfirmSingle(Player* player, Creature* creature, LockKind kind, uint32 index)
{
    ObjectGuid::LowType key = player->GetGUID().GetCounter();
    auto it = s_menu.find(key);
    if (it == s_menu.end())
    {
        ShowRoot(player, creature);
        return;
    }

    MenuState& st = it->second;
    std::vector<LockRow>* vec = (kind == LockKind::Dungeon) ? &st.dungeons : &st.raids;
    if (!vec || index >= vec->size())
    {
        ShowRoot(player, creature);
        return;
    }

    LockRow const& row = (*vec)[index];

    ClearGossipMenuFor(player);

    AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG,
        PriceLine(row.price),
        GOSSIP_SENDER_MAIN, ACT_SEPARATOR);

    AddGossipItemFor(player, 0, Sep(), GOSSIP_SENDER_MAIN, ACT_SEPARATOR);

    uint32 confirmAction = (kind == LockKind::Dungeon)
        ? (ACT_DUNGEON_CONFIRM_BASE + index)
        : (ACT_RAID_CONFIRM_BASE + index);

    uint32 backAction = (kind == LockKind::Dungeon)
        ? ACT_CAT_DUNGEON
        : ACT_CAT_RAID;

    AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, T("Ano, potvrdit", "Yes, confirm"), GOSSIP_SENDER_MAIN, confirmAction);
    AddGossipItemFor(player, GOSSIP_ICON_TAXI,      T("Zpátky", "Back"),               GOSSIP_SENDER_MAIN, backAction);

    SendGossipMenuFor(player, 1, creature->GetGUID());
}

static void ShowConfirmAll(Player* player, Creature* creature, LockKind kind)
{
    ObjectGuid::LowType key = player->GetGUID().GetCounter();
    auto it = s_menu.find(key);
    if (it == s_menu.end())
    {
        ShowRoot(player, creature);
        return;
    }

    MenuState& st = it->second;
    std::vector<LockRow>* vec = (kind == LockKind::Dungeon) ? &st.dungeons : &st.raids;
    if (!vec)
    {
        ShowCategory(player, creature, kind, true);
        return;
    }

    bool isRaid = (kind == LockKind::Raid);

    std::vector<LockRow> allowed;
    allowed.reserve(vec->size());
    for (auto const& row : *vec)
    {
        if (CheckAndConsumeLimit(player, isRaid, row.mapId, row.diffKey, false, false))
            allowed.push_back(row);
    }

    if (allowed.size() < 2)
    {
        ShowCategory(player, creature, kind, true);
        return;
    }

    Price allPrice = SumPrices(allowed);

    ClearGossipMenuFor(player);

    AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG,
        PriceLine(allPrice),
        GOSSIP_SENDER_MAIN, ACT_SEPARATOR);

    AddGossipItemFor(player, 0, Sep(), GOSSIP_SENDER_MAIN, ACT_SEPARATOR);

    uint32 confirmAction = (kind == LockKind::Dungeon)
        ? ACT_DUNGEON_CONFIRM_ALL
        : ACT_RAID_CONFIRM_ALL;

    uint32 backAction = (kind == LockKind::Dungeon)
        ? ACT_CAT_DUNGEON
        : ACT_CAT_RAID;

    AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, T("Ano, potvrdit", "Yes, confirm"), GOSSIP_SENDER_MAIN, confirmAction);
    AddGossipItemFor(player, GOSSIP_ICON_TAXI,      T("Zpátky", "Back"),               GOSSIP_SENDER_MAIN, backAction);

    SendGossipMenuFor(player, 1, creature->GetGUID());
}

// ---------------- NPC: Instance Reset ----------------
class npc_instance_reset : public CreatureScript
{
public:
    npc_instance_reset() : CreatureScript("npc_instance_reset") { }

    bool OnGossipHello(Player* player, Creature* creature) override
    {
        if (!s.resetEnable)
            return false;

        ShowRoot(player, creature);
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 sender, uint32 action) override
    {
        if (sender != GOSSIP_SENDER_MAIN)
            return false;
		
		if (!CanSeeMenu(player))
		{
			ShowOnlyLeaderInfo(player, creature);
			return true;
		}
		
        switch (action)
        {
            case ACT_ROOT:
            case ACT_BACK_ROOT:
                ShowRoot(player, creature);
                return true;

            case ACT_CAT_DUNGEON:
                ShowCategory(player, creature, LockKind::Dungeon);
                return true;

            case ACT_CAT_RAID:
                ShowCategory(player, creature, LockKind::Raid);
                return true;

            case ACT_DUNGEON_RESET_ALL:
                ShowConfirmAll(player, creature, LockKind::Dungeon);
                return true;

            case ACT_RAID_RESET_ALL:
                ShowConfirmAll(player, creature, LockKind::Raid);
                return true;

            case ACT_SEPARATOR:
                SendGossipMenuFor(player, 1, creature->GetGUID());
                return true;

            default:
                break;
        }

        if (action >= ACT_DUNGEON_ITEM_BASE && action < ACT_DUNGEON_ITEM_BASE + 500)
        {
            uint32 idx = action - ACT_DUNGEON_ITEM_BASE;
            ShowConfirmSingle(player, creature, LockKind::Dungeon, idx);
            return true;
        }

        if (action >= ACT_RAID_ITEM_BASE && action < ACT_RAID_ITEM_BASE + 500)
        {
            uint32 idx = action - ACT_RAID_ITEM_BASE;
            ShowConfirmSingle(player, creature, LockKind::Raid, idx);
            return true;
        }

        if (action >= ACT_DUNGEON_CONFIRM_BASE && action < ACT_DUNGEON_CONFIRM_BASE + 500)
        {
            uint32 idx = action - ACT_DUNGEON_CONFIRM_BASE;

            ObjectGuid::LowType key = player->GetGUID().GetCounter();
            auto it = s_menu.find(key);
            if (it == s_menu.end())
            {
                ShowRoot(player, creature);
                return true;
            }

            MenuState& st = it->second;
            if (idx >= st.dungeons.size())
            {
                ShowRoot(player, creature);
                return true;
            }

            {
                std::ostringstream msg;
                msg << T("[Reset] Resetuji dungeon ", "[Reset] Resetting dungeon ") << st.dungeons[idx].name;
                ChatHandler(player->GetSession()).SendSysMessage(msg.str().c_str());
            }

            std::vector<LockRow> list;
            list.push_back(st.dungeons[idx]);

            if (DoResetForList(player, list, st.dungeons[idx].price, LockKind::Dungeon))
                CloseGossipMenuFor(player);
            else
                ShowConfirmSingle(player, creature, LockKind::Dungeon, idx);

            return true;
        }

        if (action >= ACT_RAID_CONFIRM_BASE && action < ACT_RAID_CONFIRM_BASE + 500)
        {
            uint32 idx = action - ACT_RAID_CONFIRM_BASE;

            ObjectGuid::LowType key = player->GetGUID().GetCounter();
            auto it = s_menu.find(key);
            if (it == s_menu.end())
            {
                ShowRoot(player, creature);
                return true;
            }

            MenuState& st = it->second;
            if (idx >= st.raids.size())
            {
                ShowRoot(player, creature);
                return true;
            }

            {
                std::ostringstream msg;
                msg << T("[Reset] Resetuji raid ", "[Reset] Resetting raid ") << st.raids[idx].name;
                ChatHandler(player->GetSession()).SendSysMessage(msg.str().c_str());
            }

            std::vector<LockRow> list;
            list.push_back(st.raids[idx]);

            if (DoResetForList(player, list, st.raids[idx].price, LockKind::Raid))
                CloseGossipMenuFor(player);
            else
                ShowConfirmSingle(player, creature, LockKind::Raid, idx);

            return true;
        }

        if (action == ACT_DUNGEON_CONFIRM_ALL || action == ACT_RAID_CONFIRM_ALL)
        {
            LockKind kind = (action == ACT_DUNGEON_CONFIRM_ALL) ? LockKind::Dungeon : LockKind::Raid;

            ObjectGuid::LowType key = player->GetGUID().GetCounter();
            auto it = s_menu.find(key);
            if (it == s_menu.end())
            {
                ShowRoot(player, creature);
                return true;
            }

            MenuState& st = it->second;
            std::vector<LockRow>* vec = (kind == LockKind::Dungeon) ? &st.dungeons : &st.raids;

            if (!vec)
            {
                ShowCategory(player, creature, kind, true);
                return true;
            }

            bool isRaid = (kind == LockKind::Raid);
            std::vector<LockRow> allowed;
            allowed.reserve(vec->size());
            for (auto const& row : *vec)
            {
                if (CheckAndConsumeLimit(player, isRaid, row.mapId, row.diffKey, false, false))
                    allowed.push_back(row);
            }

            if (allowed.empty())
            {
                ChatHandler(player->GetSession()).SendSysMessage(T("[Reset] Všechny dostupné dungeony/raidy už máš vyresetované.",
                                                                   "[Reset] All available dungeons/raids are already reset."));
                ShowCategory(player, creature, kind, true);
                return true;
            }

            {
                std::ostringstream msg;
                msg << (kind == LockKind::Dungeon
                        ? T("[Reset] Resetuji dungeony ", "[Reset] Resetting dungeons ")
                        : T("[Reset] Resetuji raidy ",    "[Reset] Resetting raids "));
                bool first = true;
                for (auto const& row : allowed)
                {
                    if (!first)
                        msg << ", ";
                    msg << row.name;
                    first = false;
                }
                ChatHandler(player->GetSession()).SendSysMessage(msg.str().c_str());
            }

            Price allPrice = SumPrices(allowed);
            if (DoResetForList(player, allowed, allPrice, kind))
                CloseGossipMenuFor(player);
            else
                ShowConfirmAll(player, creature, kind);

            return true;
        }

        ShowRoot(player, creature);
        return true;
    }
};

// ---------------- Command: .instance limit ----------------
class InstanceTools_CommandScript : public CommandScript
{
public:
    InstanceTools_CommandScript() : CommandScript("InstanceTools_CommandScript") {}

    static bool HandleLimit(ChatHandler* handler)
    {
        Player* pl = handler->GetSession() ? handler->GetSession()->GetPlayer() : nullptr;
        if (!pl) return false;

        uint32 guidLow = pl->GetGUID().GetCounter();

        time_t heroSince = DailyBoundary();
        time_t raidSince  = WeeklyBoundary();

        auto fetchUsed = [&](uint8 kind /*0=dungeon,1=raid*/, time_t since)
            -> std::vector<std::tuple<uint32/*mapId*/, uint8/*diffKey*/, uint32/*used*/>>
        {
            std::vector<std::tuple<uint32,uint8,uint32>> out;
            std::ostringstream q;
            q << "SELECT map_id, difficulty, used "
              << "FROM customs.instance_reset_limit "
              << "WHERE guid=" << guidLow
              << " AND kind=" << uint32(kind)
              << " AND window_start >= " << uint32(since)
              << " AND used > 0";

            if (QueryResult res = CharacterDatabase.Query(q.str().c_str()))
            {
                do
                {
                    Field* f = res->Fetch();
                    out.emplace_back(f[0].Get<uint32>(), f[1].Get<uint8>(), f[2].Get<uint32>());
                } while (res->NextRow());
            }
            return out;
        };

        auto dungUsed = fetchUsed(0, heroSince);
        auto raidUsed = fetchUsed(1, raidSince);

        handler->SendSysMessage(T("Dungeony:", "Dungeons:"));
        if (dungUsed.empty())
        {
            std::string line = std::string("  ") +
                T("- Vše bez resetu (limit ", "- Everything without reset (limit ") +
                (s.heroicPerDay ? std::to_string(s.heroicPerDay) : std::string("∞")) +
                T(" / den)", " / day)");
            handler->SendSysMessage(line.c_str());
        }
        else
        {
            for (auto const& t : dungUsed)
            {
                uint32 mapId; uint8 diffKey; uint32 used;
                std::tie(mapId, diffKey, used) = t;
                std::ostringstream line;
                line << "  " << used << "/" << (s.heroicPerDay ? std::to_string(s.heroicPerDay) : std::string("∞"))
                     << " - " << GetMapName(mapId)
                     << (diffKey == 1 ? " (HC)" : " (NM)");
                handler->SendSysMessage(line.str().c_str());
            }
        }

        handler->SendSysMessage(T("Raidy:", "Raids:"));
        if (raidUsed.empty())
        {
            std::string line = std::string("  ") +
                T("- Vše bez resetu (limit ", "- Everything without reset (limit ") +
                (s.raidPerWeek ? std::to_string(s.raidPerWeek) : std::string("∞")) +
                T(" / týden)", " / week)");
            handler->SendSysMessage(line.c_str());
        }
        else
        {
            for (auto const& t : raidUsed)
            {
                uint32 mapId; uint8 diffKey; uint32 used;
                std::tie(mapId, diffKey, used) = t;
                std::ostringstream line;
                line << "  " << used << "/" << (s.raidPerWeek ? std::to_string(s.raidPerWeek) : std::string("∞"))
                     << " - " << GetMapName(mapId);
                bool shared = IsIccOrRs(mapId) && IsNHSharedFor(pl, mapId);
				if (diffKey <= 4)
					line << RaidKeyLabel(diffKey, shared);
                handler->SendSysMessage(line.str().c_str());
            }
        }

        return true;
    }

    ChatCommandTable GetCommands() const override
    {
        static ChatCommandTable iSub =
        {
            { "limit", HandleLimit, SEC_PLAYER, Console::No },
            { "l",     HandleLimit, SEC_PLAYER, Console::No },
        };

        static ChatCommandTable root =
        {
            { "instance", iSub },
            { "i",        iSub },
        };

        return root;
    }
};

// ---------------- WorldScript: pouze config ----------------
class ITools_WorldScript : public WorldScript
{
public:
    ITools_WorldScript() : WorldScript("ITools_WorldScript") { }

    void OnAfterConfigLoad(bool /*reload*/) override
    {
        LoadCfg();
    }
};

}

// ---------------- Entry point ----------------
void RegisterInstanceToolsCustomsUpdater();
void RegisterVoATeleporter();

void Addmod_instance_toolsScripts()
{
    RegisterInstanceToolsCustomsUpdater();
	
	new itools::ITools_WorldScript();
    new itools::npc_instance_reset();
    new itools::InstanceTools_CommandScript();
    RegisterVoATeleporter();
}
