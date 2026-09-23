/* Copyright (C) 2016+ AzerothCore, GNU AGPL v3. */

#include "AscensionPrimalistEarthquake.h"
#include "Player.h"
#include "SpellAuraDefines.h"
#include "SpellMgr.h"
#include <array>

namespace
{
constexpr uint32 PRIMALIST_FAMILY = uint32(CLASS_WILDWALKER) + 6;
constexpr std::array<uint32, 6> EARTHQUAKE_RANKS =
    {520412, 520741, 520742, 520743, 520744, 520745};
constexpr uint32 SPELL_TECTONIC_RESONANCE = 707173;
}

void ApplyAscensionPrimalistEarthquakeContract(SpellInfo* spellInfo)
{
    if (!spellInfo || spellInfo->SpellFamilyName != PRIMALIST_FAMILY)
        return;

    if (spellInfo->Id == SPELL_TECTONIC_RESONANCE)
    {
        // Tectonic Resonance: without SPELL_ATTR0_PASSIVE the learned talent is
        // never applied as a standing aura, so its modifier never comes online.
        // Effect 2 is the real mechanic (aura 107, SPELLMOD_EFFECT1, flat -31,
        // mask 0x10000000) — the native effect-index modifier route deepens the
        // Earthquake slow (effect 0 of the root 520696, -31 percent) by another
        // 31 points when the slow's amount is calculated. Effects 0 and 1 are
        // empty-mask cooldown/activation-time strays and are zeroed.
        spellInfo->Attributes |= SPELL_ATTR0_PASSIVE;
        for (uint8 i = 0; i < 2; ++i)
        {
            spellInfo->Effects[i].Effect = SpellEffects(0);
            spellInfo->Effects[i].ApplyAuraName = SPELL_AURA_NONE;
        }
        return;
    }

    bool isEarthquakeRank = false;
    for (uint32 rankId : EARTHQUAKE_RANKS)
        if (spellInfo->Id == rankId)
        {
            isEarthquakeRank = true;
            break;
        }
    if (!isEarthquakeRank)
        return;

    if (spellInfo->Effects[EFFECT_0].Effect == SPELL_EFFECT_PERSISTENT_AREA_AURA &&
        spellInfo->Effects[EFFECT_0].ApplyAuraName == SPELL_AURA_PERIODIC_DAMAGE &&
        spellInfo->Effects[EFFECT_1].Effect == SPELL_EFFECT_PERSISTENT_AREA_AURA &&
        spellInfo->Effects[EFFECT_1].ApplyAuraName == SPELL_AURA_PERIODIC_TRIGGER_SPELL)
        spellInfo->AttributesCu &= ~SPELL_ATTR0_CU_BINARY_SPELL;
}
