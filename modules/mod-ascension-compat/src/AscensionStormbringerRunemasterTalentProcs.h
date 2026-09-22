/* Copyright (C) 2016+ AzerothCore, GNU AGPL v3. */

#ifndef ASCENSION_STORMBRINGER_RUNEMASTER_TALENT_PROCS_H
#define ASCENSION_STORMBRINGER_RUNEMASTER_TALENT_PROCS_H

#include <array>
#include <cstdint>

namespace AscensionStormbringerRunemasterTalentProcs
{
struct Rule
{
    std::uint32_t Talent;
    std::array<std::uint32_t, 26> Spells;
    std::uint32_t RequiredAura;
    bool RequiresStealth;
};

inline constexpr std::array<Rule, 3> Rules =
{{
    {500476, {{502828, 502829, 502830, 502831, 502832, 502833, 502834, 502835, 502836, 502837, 502838, 520064, 520065, 520066, 802202}}, 653022, false},
    {520237, {{500287, 500562}}, 0, true},
    {520917, {{653272}}, 806982, false},
}};
}

#endif
