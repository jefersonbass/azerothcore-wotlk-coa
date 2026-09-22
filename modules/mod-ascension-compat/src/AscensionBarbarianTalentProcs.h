/* Copyright (C) 2016+ AzerothCore, GNU AGPL v3. */

#ifndef ASCENSION_BARBARIAN_TALENT_PROCS_H
#define ASCENSION_BARBARIAN_TALENT_PROCS_H

#include <array>
#include <cstdint>

namespace AscensionBarbarianTalentProcs
{
struct Rule
{
    std::uint32_t Talent;
    std::array<std::uint32_t, 26> Spells;
    std::uint32_t RequiredAura;
    bool RequiresStealth;
};

inline constexpr std::array<Rule, 4> Rules =
{{
    {705223, {{514050, 514051, 514052, 514053, 514054, 514055}}, 0, false},
    {705159, {{801761, 804338}}, 0, false},
    {705245, {{560518, 561010, 561011, 561012, 561013}}, 0, false},
    {704591, {{500919, 504912, 504913, 504914, 504915, 504916}}, 0, false},
}};
}

#endif
