/* Copyright (C) 2016+ AzerothCore, GNU AGPL v3. */

#ifndef ASCENSION_SPELL_LIST_TALENT_PROCS_H
#define ASCENSION_SPELL_LIST_TALENT_PROCS_H

#include <array>
#include <cstdint>

namespace AscensionSpellListTalentProcs
{
struct Rule
{
    std::uint32_t Talent;
    std::array<std::uint32_t, 26> Spells;
};

inline constexpr std::array<Rule, 5> Rules =
{{
    {705451, {{680244, 681181, 681182, 804193, 806846, 806847, 806848, 806849, 806850, 806851}}},
    {705483, {{574335, 574342, 574343, 802020, 802024, 802248, 802249, 802250, 802251, 802252,
               802253, 802254, 802255, 802256, 802257, 802258, 803069, 803502, 804294}}},
    {300582, {{502823, 502824, 502825, 502826, 502827, 800732, 804568}}},
    {705853, {{804049}}},
    {705868, {{804684}}},
}};
}

#endif
