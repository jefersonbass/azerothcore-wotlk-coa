/* Copyright (C) 2016+ AzerothCore, GNU AGPL v3. */

#ifndef ASCENSION_KNIGHT_OF_XOROTH_TALENT_PROCS_H
#define ASCENSION_KNIGHT_OF_XOROTH_TALENT_PROCS_H

#include <array>
#include <cstdint>

namespace AscensionKnightOfXorothTalentProcs
{
struct Rule
{
    std::uint32_t Talent;
    std::array<std::uint32_t, 26> Spells;
    std::uint32_t RequiredAura;
    bool RequiresStealth;
};

inline constexpr std::array<Rule, 1> Rules =
{{
    {680216, {{801002, 802606, 803251}}, 0, false},
}};
}

#endif
