/* Copyright (C) 2016+ AzerothCore, GNU AGPL v3. */

#include "Player.h"
#include "ScriptMgr.h"
#include "Spell.h"
#include "SpellAuraEffects.h"
#include "SpellMgr.h"
#include "SpellScript.h"
#include <algorithm>
#include <array>
#include <limits>

namespace
{
constexpr uint32 SPELL_TONIC_SUPPLY = 705497;
constexpr uint32 SPELL_JUST_A_SIP = 705533;
constexpr uint32 SPELL_JUST_A_SIP_DRINK = 680616;
constexpr std::array<uint32, 11> WITCH_HUNTER_TONIC_CASTS =
{{
    680491, 572295, 572296, 572297, 572298,
    802276, 802277, 802278, 802279, 802308, 802826
}};

uint32 GetWitchHunterTonicRoot(SpellInfo const* spellInfo)
{
    if (!spellInfo || spellInfo->SpellFamilyName != uint32(CLASS_WITCH_HUNTER) + 6 ||
        !(spellInfo->SpellFamilyFlags[2] & 512))
        return 0;

    switch (spellInfo->Id)
    {
        case 572295:
        case 572296:
        case 572297:
        case 572298:
        case 680491:
            return 680491;
        case 802308:
        case 802279:
            return 802279;
        case 802276:
        case 802277:
        case 802278:
        case 802826:
            return spellInfo->Id;
        default:
            return 0;
    }
}

class spell_ascension_witch_hunter_tonic_supply : public SpellScript
{
    PrepareSpellScript(spell_ascension_witch_hunter_tonic_supply);

    bool Validate(SpellInfo const* spellInfo) override
    {
        SpellInfo const* talent = sSpellMgr->GetSpellInfo(SPELL_TONIC_SUPPLY);
        return GetWitchHunterTonicRoot(spellInfo) && talent &&
            talent->SpellFamilyName == uint32(CLASS_WITCH_HUNTER) + 6 &&
            talent->Effects[EFFECT_0].IsAura(SPELL_AURA_DUMMY);
    }

    bool Load() override
    {
        Unit* caster = GetCaster();
        return caster && caster->IsPlayer() && caster->ToPlayer()->getClass() == CLASS_WITCH_HUNTER && !GetSpell()->IsTriggered();
    }

    void ReduceOtherTonics()
    {
        Player* player = GetCaster()->ToPlayer();
        AuraEffect const* effect = player->GetAuraEffect(SPELL_TONIC_SUPPLY, EFFECT_0);
        if (!effect || effect->GetAmount() >= 0)
            return;

        uint32 reduction = uint32(std::min(-int64(effect->GetAmount()), int64(std::numeric_limits<int32>::max())));
        uint32 consumedRoot = GetWitchHunterTonicRoot(GetSpellInfo());
        if (!consumedRoot)
            return;

        for (uint32 id : WITCH_HUNTER_TONIC_CASTS)
        {
            uint32 otherRoot = GetWitchHunterTonicRoot(sSpellMgr->GetSpellInfo(id));
            if (!otherRoot || otherRoot == consumedRoot)
                continue;

            uint32 remaining = player->GetSpellCooldownDelay(id);
            if (!remaining)
                continue;

            if (remaining <= reduction)
                player->RemoveSpellCooldown(id, true);
            else
                player->ModifySpellCooldown(id, -int32(reduction));
        }
    }

    void Register() override
    {
        AfterCast += SpellCastFn(spell_ascension_witch_hunter_tonic_supply::ReduceOtherTonics);
    }
};

// Just A Sip: drinking Tonics also restores 5% health and mana. The DBC wires
// the passive as a proc aura (705533, PROC_TRIGGER_SPELL, trigger 680616), but
// no proc event exists for drinking, so fire the restore directly on a tonic cast.
// 680616 is self-cast HEAL_PCT + ENERGIZE_PCT at 5% each, fully native.
class spell_ascension_witch_hunter_just_a_sip : public SpellScript
{
    PrepareSpellScript(spell_ascension_witch_hunter_just_a_sip);

    bool Validate(SpellInfo const* spellInfo) override
    {
        SpellInfo const* drink = sSpellMgr->GetSpellInfo(SPELL_JUST_A_SIP_DRINK);
        return GetWitchHunterTonicRoot(spellInfo) && drink &&
            drink->SpellFamilyName == 31 &&
            drink->Effects[EFFECT_0].Effect == SPELL_EFFECT_HEAL_PCT &&
            drink->Effects[EFFECT_1].Effect == SPELL_EFFECT_ENERGIZE_PCT;
    }

    bool Load() override
    {
        Unit* caster = GetCaster();
        return caster && caster->IsPlayer() && caster->ToPlayer()->getClass() == CLASS_WITCH_HUNTER && !GetSpell()->IsTriggered();
    }

    void SipOnCast()
    {
        Player* player = GetCaster()->ToPlayer();
        if (player->HasAura(SPELL_JUST_A_SIP))
            player->CastSpell(player, SPELL_JUST_A_SIP_DRINK, TRIGGERED_FULL_MASK);
    }

    void Register() override
    {
        AfterCast += SpellCastFn(spell_ascension_witch_hunter_just_a_sip::SipOnCast);
    }
};
}

void AddAscensionWitchHunterTonicTalentScripts()
{
    RegisterSpellScript(spell_ascension_witch_hunter_tonic_supply);
    RegisterSpellScript(spell_ascension_witch_hunter_just_a_sip);
}
