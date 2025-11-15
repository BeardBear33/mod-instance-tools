#pragma once

#include <string>
#include <algorithm>
#include "Config.h"

namespace itools { namespace Names {

// Tři tvary jednotky – singular / dual / plural
struct Unit
{
    std::string sg; // 1
    std::string du; // 2-4 (pro EN může být stejné jako plural)
    std::string pl; // 5+
};

struct All
{
    Unit token;
    Unit gold;
};

// Interní: defaulty podle jazyka (pokud user nic nenastaví v configu)
inline All DefaultsByLocale()
{
    std::string loc = sConfigMgr->GetOption<std::string>("InstanceTools.Locale", "cs");
    std::transform(loc.begin(), loc.end(), loc.begin(), ::tolower);

    All a;

    if (loc == "en" || loc == "english")
    {
        // EN: dual == plural
        a.token.sg = "Mystery Token";
        a.token.du = "Mystery Tokens";
        a.token.pl = "Mystery Tokens";

        a.gold.sg = "gold";
        a.gold.du = "gold";
        a.gold.pl = "gold";
    }
    else
    {
        // CS defaulty
        a.token.sg = "Mystery Token";
        a.token.du = "Mystery Tokeny";
        a.token.pl = "Mystery Tokenů";

        a.gold.sg = "Zlatý";
        a.gold.du = "Zlaté";
        a.gold.pl = "Zlatých";
    }

    return a;
}

// Načte a zcacheuje názvy z configu
inline All const& Get()
{
    static All cache;
    static bool inited = false;
    if (!inited)
    {
        All d = DefaultsByLocale();

        cache.token.sg = sConfigMgr->GetOption<std::string>("InstanceTools.TokenItem.Singular", d.token.sg);
        cache.token.du = sConfigMgr->GetOption<std::string>("InstanceTools.TokenItem.Dual",     d.token.du);
        cache.token.pl = sConfigMgr->GetOption<std::string>("InstanceTools.TokenItem.Plural",   d.token.pl);

        cache.gold.sg = sConfigMgr->GetOption<std::string>("InstanceTools.Gold.Singular", d.gold.sg);
        cache.gold.du = sConfigMgr->GetOption<std::string>("InstanceTools.Gold.Dual",     d.gold.du);
        cache.gold.pl = sConfigMgr->GetOption<std::string>("InstanceTools.Gold.Plural",   d.gold.pl);

        inited = true;
    }
    return cache;
}

// 1 -> singular, 2-4 -> dual, jinak plural
inline std::string CountTokenName(uint64 n)
{
    All const& a = Get();
    if (n == 1) return a.token.sg;
    if (n >= 2 && n <= 4) return a.token.du;
    return a.token.pl;
}

inline std::string CountGoldName(uint64 n)
{
    All const& a = Get();
    if (n == 1) return a.gold.sg;
    if (n >= 2 && n <= 4) return a.gold.du;
    return a.gold.pl;
}

}} // namespace itools::Names
