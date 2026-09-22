/* Copyright (C) 2016+ AzerothCore, GNU AGPL v3. */

#ifndef ASCENSION_NECROMANCER_TALENT_PROCS_H
#define ASCENSION_NECROMANCER_TALENT_PROCS_H

#include <array>
#include <cstdint>

namespace AscensionNecromancerTalentProcs
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
    {704683, {{500338, 501855, 501856, 501857, 501858, 501859, 501860, 501861}}, 0, false},
    {704727, {{500965, 501055, 501880, 501881, 501882, 501883, 501884, 501885, 501886, 501887, 501888,
               504610, 800343, 800344, 803530}}, 0, false},
    {804689, {{561125, 804559, 9666680}}, 0, false},
    {704705, {{0}}, 680986, false},
}};
}

#endif
