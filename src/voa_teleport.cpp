// src/voa_teleport.cpp
#include "ScriptMgr.h"
#include "World.h"
#include "Config.h"
#include "Chat.h"
#include "Player.h"
#include "Creature.h"
#include "ScriptedGossip.h"
#include "Group.h"
#include "SharedDefines.h"
#include "InstanceSaveMgr.h"

#include "BattlefieldMgr.h"
#include "BattlefieldWG.h"

#include <string>
#include <algorithm>
#include <cstdlib>
#include <sstream>
#include <ctime>

namespace itools
{

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

// ---------------- Konfigurace jen pro VoA ----------------
struct Price
{
    uint32 goldG       = 0;
    uint32 emblemId    = 0;
    uint32 emblemCount = 0;
};

struct UnitNames { std::string sg, du, pl; };

static std::string CountName(uint64 n, UnitNames const& u)
{
    if (n == 1) return u.sg;
    if (n >= 2 && n <= 4) return u.du;
    return u.pl;
}

struct CfgVoA
{
    bool   voaEnable             = true;
    uint32 voaMinLv              = 80;
    bool   voaAllowCombat        = false;
    Price  voaPrice;
    uint32 voaMap = 624;
    float  voaX   = -406.07938f;
    float  voaY   = -103.20971f;
    float  voaZ   = 104.65892f;
    float  voaO   = 0.f;
    bool   voaOverrideIgnoreWGOwnership = false;

    UnitNames goldNames;
    UnitNames emblemNames;
};

static CfgVoA s;
static bool s_cfgLoaded = false;

static uint32 CUInt(char const* k, uint32 d) { return sConfigMgr->GetOption<uint32>(k, d); }
static bool   CBool(char const* k, bool d)   { return sConfigMgr->GetOption<bool>(k, d); }
static std::string CStr(char const* k, char const* d) { return sConfigMgr->GetOption<std::string>(k, d); }

static void LoadCfg()
{
    s.voaEnable      = CBool("VoATeleporter.Enable", true);
    s.voaMinLv       = CUInt("VoATeleporter.MinLevel", 80);
    s.voaAllowCombat = CBool("VoATeleporter.AllowWhileInCombat", false);

    s.voaPrice.goldG       = CUInt("VoATeleporter.Price.Gold", 0);
    s.voaPrice.emblemId    = CUInt("VoATeleporter.Price.EmblemItemId", 0);
    s.voaPrice.emblemCount = CUInt("VoATeleporter.Price.EmblemCount", 0);

    s.voaMap = CUInt("VoATeleporter.Target.Inside.MapId", 624);
    s.voaX   = float(atof(CStr("VoATeleporter.Target.Inside.X", "-406.07938").c_str()));
    s.voaY   = float(atof(CStr("VoATeleporter.Target.Inside.Y", "-103.20971").c_str()));
    s.voaZ   = float(atof(CStr("VoATeleporter.Target.Inside.Z", "104.65892").c_str()));
    s.voaO   = float(atof(CStr("VoATeleporter.Target.Inside.O", "0").c_str()));

    s.voaOverrideIgnoreWGOwnership = CBool("VoATeleporter.Override.IgnoreWintergraspOwnership", false);

    if (LangOpt() == Lang::EN)
    {
        s.goldNames   = { "gold", "gold", "gold" };
        s.emblemNames = { "Emblem", "Emblems", "Emblems" };
    }
    else
    {
        s.goldNames   = { "Zlatý", "Zlaté", "Zlatých" };
        s.emblemNames = { "Emblém", "Emblémy", "Emblémů" };
    }
    s.emblemNames.sg = CStr("VoATeleporter.EmblemItem.Singular", s.emblemNames.sg.c_str());
    s.emblemNames.du = CStr("VoATeleporter.EmblemItem.Dual",     s.emblemNames.du.c_str());
    s.emblemNames.pl = CStr("VoATeleporter.EmblemItem.Plural",   s.emblemNames.pl.c_str());

    s.goldNames.sg   = CStr("VoATeleporter.Gold.Singular", s.goldNames.sg.c_str());
    s.goldNames.du   = CStr("VoATeleporter.Gold.Dual",     s.goldNames.du.c_str());
    s.goldNames.pl   = CStr("VoATeleporter.Gold.Plural",   s.goldNames.pl.c_str());

    s_cfgLoaded = true;
}

// ---------------- Wintergrasp helpers ----------------
static BattlefieldWG* GetWG()
{
    if (Battlefield* bf = sBattlefieldMgr->GetBattlefieldToZoneId(4197))
        if (auto* wg = dynamic_cast<BattlefieldWG*>(bf))
            return wg;
    return nullptr;
}

static bool IsWGBattleActive()
{
    if (auto* wg = GetWG())
        return wg->IsWarTime();
    return false;
}

static uint32 WGCountdownSeconds(BattlefieldWG* wg)
{
    if (!wg || wg->IsWarTime())
        return 0;

    static uint32 s_lastNonZeroSecs = 0;
    static time_t s_lastUpdateTs    = 0;

    int32 raw = wg->GetTimer();
    uint32 secs = 0;

    if (raw > 0)
    {
        uint32 v = static_cast<uint32>(raw);

        const uint32 SECS_24H = 24u * 60u * 60u;
        const uint32 SECS_7D  = 7u  * 24u * 60u * 60u;
        const uint32 MS_7D    = SECS_7D * 1000u;

        bool looksMs = false;
        if ((v % 1000u == 0u && v / 1000u <= SECS_7D) || (v > SECS_24H && v <= MS_7D))
            looksMs = true;

        secs = looksMs ? v / 1000u : v;

        s_lastNonZeroSecs = secs;
        s_lastUpdateTs    = std::time(nullptr);
    }
    else
    {
        time_t now = std::time(nullptr);
        if (s_lastNonZeroSecs > 0 && s_lastUpdateTs != 0 && (now - s_lastUpdateTs) <= 180)
            secs = s_lastNonZeroSecs;
        else
            secs = 0;
    }

    return secs;
}

static bool IsWGQueueActiveSmart()
{
    BattlefieldWG* wg = GetWG();
    if (!wg)
        return false;

    if (wg->IsWarTime())
        return true;

    uint32 secs = WGCountdownSeconds(wg);
    if (secs == 0)
        return false;

    return (secs <= 30u * 60u);
}

static bool PlayerFactionControlsWG(Player* pl)
{
    if (!pl)
        return true;
    if (auto* wg = GetWG())
        return wg->GetDefenderTeam() == pl->GetTeamId();
    return true;
}

// ---------------- Platba ----------------
static bool TakePayment(Player* pl, Price const& pr, std::string& err)
{
    if (pr.goldG)
    {
        uint64 needCopper = uint64(pr.goldG) * GOLD;
        if (pl->GetMoney() < needCopper)
        {
            err = "NOT_ENOUGH_GOLD";
            return false;
        }
    }

    if (pr.emblemId && pr.emblemCount)
    {
        if (!pl->HasItemCount(pr.emblemId, pr.emblemCount, false))
        {
            err = "NOT_ENOUGH_EMBLEMS";
            return false;
        }
    }
    return true;
}

static void ConsumePayment(Player* pl, Price const& pr)
{
    if (pr.goldG)
        pl->ModifyMoney(-int64(uint64(pr.goldG) * GOLD));
    if (pr.emblemId && pr.emblemCount)
        pl->DestroyItemCount(pr.emblemId, pr.emblemCount, true);
}

// ---------------- UI helper: řádka ceny
static bool HasAnyPrice()
{
    return (s.voaPrice.goldG > 0) || (s.voaPrice.emblemId != 0 && s.voaPrice.emblemCount > 0);
}

static std::string PriceLine()
{
    if (!HasAnyPrice())
        return "";

    std::ostringstream o;
    o << T("Cena: ", "Cost: ");
    bool any = false;

    if (s.voaPrice.goldG > 0)
    {
        o << s.voaPrice.goldG << " " << CountName(s.voaPrice.goldG, s.goldNames);
        any = true;
    }
    if (s.voaPrice.emblemId != 0 && s.voaPrice.emblemCount > 0)
    {
        if (any) o << T(" a ", " and ");
        o << s.voaPrice.emblemCount << " " << CountName(s.voaPrice.emblemCount, s.emblemNames);
    }
    return o.str();
}

// ---------------- Gossip helpers (sekce) ----------------
enum : uint32
{
    ACT_ROOT_OPEN     = 1,
    ACT_CONFIRM_YES   = 2,
    ACT_BACK          = 3,
    ACT_NOP_ALERT     = 100,
    ACT_NOP_CONFIRM   = 101
};

static void ShowAlertOnly(Player* player, Creature* creature, char const* msg)
{
    ClearGossipMenuFor(player);
    AddGossipItemFor(player, GOSSIP_ICON_CHAT, msg, GOSSIP_SENDER_MAIN, ACT_NOP_ALERT);
    SendGossipMenuFor(player, 1, creature->GetGUID());
}

static void ShowRoot(Player* player, Creature* creature)
{
    ClearGossipMenuFor(player);
    AddGossipItemFor(player, GOSSIP_ICON_CHAT,
        T("Teleport do Vault of Archavon", "Teleport to Vault of Archavon"),
        GOSSIP_SENDER_MAIN, ACT_ROOT_OPEN);
    SendGossipMenuFor(player, 1, creature->GetGUID());
}

static void ShowConfirmSubmenu(Player* player, Creature* creature)
{
    ClearGossipMenuFor(player);

    bool hasPrice = HasAnyPrice();
    if (hasPrice)
    {
        AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, PriceLine(), GOSSIP_SENDER_MAIN, ACT_NOP_CONFIRM);

        AddGossipItemFor(player, GOSSIP_ICON_CHAT,
            T("Info: Vstup platí každý hráč z raidu zvlášť",
              "Info: Each raid member pays individually"),
            GOSSIP_SENDER_MAIN, ACT_NOP_CONFIRM);

        AddGossipItemFor(player, 0, Sep(), GOSSIP_SENDER_MAIN, ACT_NOP_CONFIRM);
    }

    AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG,
        T("Ano, potvrdit", "Yes, confirm"),
        GOSSIP_SENDER_MAIN, ACT_CONFIRM_YES);

    AddGossipItemFor(player, GOSSIP_ICON_TAXI,
        T("Zpátky", "Back"),
        GOSSIP_SENDER_MAIN, ACT_BACK);

    SendGossipMenuFor(player, 1, creature->GetGUID());
}

// ---------------- Teleport provedení ----------------
static bool DoTeleport(Player* player)
{
    if (player->GetLevel() < s.voaMinLv)
    {
        std::string msg = std::string(T("[VoA] Potřebuješ level ", "[VoA] You need level "))
                        + std::to_string(s.voaMinLv) + ".";
        ChatHandler(player->GetSession()).SendSysMessage(msg.c_str());
        return false;
    }
    if (!s.voaAllowCombat && player->IsInCombat())
    {
        ChatHandler(player->GetSession()).SendSysMessage(T("[VoA] Jsi v boji.", "[VoA] You are in combat."));
        return false;
    }

    if (!player->GetGroup() || !player->GetGroup()->isRaidGroup())
    {
        ChatHandler(player->GetSession()).SendSysMessage(
            T("[VoA] Pro teleport je potřeba mít Raid partu.", "[VoA] A raid group is required to teleport."));
        return false;
    }

    if (IsWGBattleActive() || IsWGQueueActiveSmart())
    {
        ChatHandler(player->GetSession()).SendSysMessage(
            T("[VoA] Wintergrasp bitva/queue právě probíhá.", "[VoA] Wintergrasp battle/queue is currently active."));
        return false;
    }

    if (!s.voaOverrideIgnoreWGOwnership && !PlayerFactionControlsWG(player))
    {
        ChatHandler(player->GetSession()).SendSysMessage(
            T("[VoA] Tvá frakce nyní Wintergrasp nevlastní.", "[VoA] Your faction does not control Wintergrasp right now."));
        return false;
    }

    std::string err;
    if (!TakePayment(player, s.voaPrice, err))
    {
        ChatHandler(player->GetSession()).SendSysMessage(T("[VoA] Nemáš dostatek měny.", "[VoA] You don't have enough currency."));
        return false;
    }

    bool ok = player->TeleportTo(s.voaMap, s.voaX, s.voaY, s.voaZ, s.voaO);
    if (!ok)
    {
        ChatHandler(player->GetSession()).SendSysMessage(T("[VoA] Teleport selhal.", "[VoA] Teleport failed."));
        return false;
    }

    ConsumePayment(player, s.voaPrice);
    return true;
}

// ---------------- VoA Teleporter ----------------
class npc_voa_teleporter : public CreatureScript
{
public:
    npc_voa_teleporter() : CreatureScript("npc_voa_teleporter") { }

    bool OnGossipHello(Player* player, Creature* creature) override
    {
        if (!s_cfgLoaded)
            LoadCfg();

        if (!s.voaEnable)
            return false;

        if (IsWGBattleActive() || IsWGQueueActiveSmart())
        {
            ShowAlertOnly(player, creature,
                T("Omlouvám se ale nyní probíhá bitva a nemohu tě teleportovat",
                  "Sorry, a battle is in progress and I cannot teleport you."));
            return true;
        }

        if (!s.voaOverrideIgnoreWGOwnership && !PlayerFactionControlsWG(player))
        {
            ShowAlertOnly(player, creature,
                T("Omlouvám se ale tvá frakce nevlastní Wintergrasp",
                  "Sorry, your faction does not control Wintergrasp."));
            return true;
        }

        if (!player->GetGroup() || !player->GetGroup()->isRaidGroup())
        {
            ShowAlertOnly(player, creature,
                T("Promiň ale pro teleport je potřeba mít Raid partu",
                  "Sorry, a raid group is required to teleport"));
            return true;
        }

        ShowRoot(player, creature);
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action) override
    {
        if (action == ACT_NOP_ALERT)
        {
            return OnGossipHello(player, creature);
        }
        if (action == ACT_NOP_CONFIRM)
        {
            ShowConfirmSubmenu(player, creature);
            return true;
        }

        if (action == ACT_BACK)
        {
            return OnGossipHello(player, creature);
        }

        if (action == ACT_ROOT_OPEN)
        {
            if (IsWGBattleActive() || IsWGQueueActiveSmart())
            {
                ShowAlertOnly(player, creature,
                    T("Omlouvám se ale nyní probíhá bitva a nemohu tě teleportovat",
                      "Sorry, a battle is in progress and I cannot teleport you."));
                return true;
            }

            if (s.voaOverrideIgnoreWGOwnership)
            {
                ShowConfirmSubmenu(player, creature);
                return true;
            }

            if (DoTeleport(player))
                CloseGossipMenuFor(player);
            else
                ShowRoot(player, creature);
            return true;
        }

        if (action == ACT_CONFIRM_YES)
        {
            if (DoTeleport(player))
                CloseGossipMenuFor(player);
            else
                ShowConfirmSubmenu(player, creature);
            return true;
        }

        return OnGossipHello(player, creature);
    }
};

}

void RegisterVoATeleporter()
{
    new itools::npc_voa_teleporter();
}
