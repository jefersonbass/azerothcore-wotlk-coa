/* Copyright (C) 2016+ AzerothCore, GNU AGPL v3. */

#include "AscensionReaperDeathwind.h"
#include "AscensionReaperTalents.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SpellAuras.h"
#include "SpellInfo.h"
#include <array>

namespace
{
constexpr uint32 REAPER_SPELL_FAMILY = 36;
constexpr uint32 EATER_OF_SOULS = 805181;
constexpr uint32 EATER_OF_SOULS_BUFF = 805182;
constexpr uint32 REAPED_SOUL = 500363;
constexpr uint8 EATER_SOULS_REQUIRED = 3;

struct DeathwindRank
{
    uint32 Id;
    uint32 SpellLevel;
    uint32 MaxLevel;
};

constexpr std::array<DeathwindRank, 7> DEATHWIND_RANKS =
{{
    {800174, 16, 22},
    {502989, 24, 29},
    {502990, 31, 36},
    {502991, 38, 43},
    {502992, 45, 50},
    {502993, 52, 57},
    {502994, 59, 64}
}};
}

void ApplyAscensionReaperDeathwindContracts(SpellInfo* spellInfo)
{
    if (!spellInfo || spellInfo->SpellFamilyName != REAPER_SPELL_FAMILY ||
        spellInfo->SpellFamilyFlags != flag96(0, 536870912, 0))
        return;

    SpellEffectInfo const& leech = spellInfo->Effects[EFFECT_0];
    if (leech.Effect != SPELL_EFFECT_PERSISTENT_AREA_AURA || leech.ApplyAuraName != SPELL_AURA_PERIODIC_LEECH ||
        leech.Amplitude != 2000 || leech.TargetA.GetTarget() != TARGET_UNIT_DEST_AREA_ENEMY ||
        leech.TargetB.GetTarget() || leech.RealPointsPerLevel || leech.TriggerSpell)
        return;

    for (DeathwindRank const& rank : DEATHWIND_RANKS)
    {
        if (spellInfo->Id != rank.Id)
            continue;

        if (spellInfo->SpellLevel == rank.SpellLevel && spellInfo->BaseLevel == rank.SpellLevel &&
            spellInfo->MaxLevel == rank.MaxLevel)
            // Keep native rank levels and dice. The current visible formula
            // explicitly promises 4% SP; legacy level penalties must not reduce it.
            spellInfo->IgnoreSpellLevelPenalty = true;
        return;
    }
}

// Eater of Souls: "Reaching 3 Reaped Souls now reduces your Physical damage
// taken by 10% and you regain 1% of your maximum health every 2 sec for 10
// seconds. This will not trigger while already active. In addition, the
// healing done by Deathwind is doubled." The 805182 buff (periodic heal +
// physical damage reduction) and the heal doubling live server-side.
class reaper_eater_of_souls : public UnitScript
{
public:
    reaper_eater_of_souls() : UnitScript("reaper_eater_of_souls", true,
        {UNITHOOK_MODIFY_HEAL_RECEIVED}) { }

    void ModifyHealReceived(Unit* target, Unit* healer, uint32& heal, SpellInfo const* spellInfo) override
    {
        Player* player = healer ? healer->ToPlayer() : nullptr;
        if (!player || !heal || player->getClass() != CLASS_REAPER ||
            !spellInfo || spellInfo->SpellFamilyName != REAPER_SPELL_FAMILY ||
            spellInfo->SpellFamilyFlags != flag96(0, 536870912, 0) ||
            !player->HasAura(EATER_OF_SOULS))
            return;
        heal *= 2;
    }

};

void HandleAscensionReaperEaterOfSouls(Player* player)
{
    if (!player || player->getClass() != CLASS_REAPER || !player->HasAura(EATER_OF_SOULS) ||
        player->HasAura(EATER_OF_SOULS_BUFF))
        return;

    if (Aura* souls = player->GetAura(REAPED_SOUL, player->GetGUID());
        souls && souls->GetStackAmount() >= EATER_SOULS_REQUIRED)
        player->CastSpell(player, EATER_OF_SOULS_BUFF, true);
}

void AddSC_AscensionReaperDeathwind()
{
    new reaper_eater_of_souls();
}
