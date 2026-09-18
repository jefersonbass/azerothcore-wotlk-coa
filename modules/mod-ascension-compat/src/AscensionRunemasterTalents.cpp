/* Copyright (C) 2016+ AzerothCore, GNU AGPL v3. */
#include "AscensionRunemasterTalents.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SpellAuraEffects.h"
#include "SpellAuras.h"
#include "SpellInfo.h"
#include "SpellScript.h"

namespace
{
bool IsEarthTattoo(uint32 id)
{
    return id == 801094 || (id >= 803754 && id <= 803758);
}

bool EarthTattooActive(Unit* unit)
{
    if (!unit || !unit->IsAlive())
        return false;
    if (unit->HasAura(801094, unit->GetGUID()))
        return true;
    for (uint32 id = 803754; id <= 803758; ++id)
        if (unit->HasAura(id, unit->GetGUID()))
            return true;
    return false;
}

bool StonePetroglyphActive(Player* player)
{
    if (!player->IsAlive() || !player->HasAura(707157))
        return false;
    if (player->HasAura(801094, player->GetGUID()))
        return true;
    for (uint32 id = 803754; id <= 803758; ++id)
        if (player->HasAura(id, player->GetGUID()))
            return true;
    return false;
}

void SyncStonePetroglyph(Player* player)
{
    if (!StonePetroglyphActive(player))
        player->RemoveAurasDueToSpell(712310, player->GetGUID());
    else if (!player->HasAura(712310, player->GetGUID()))
        player->CastSpell(player, 712310, true);
}

// Palm Sigil (805380/805381/805382) gates its cast on CasterAuraSpell 808089, a marker spell
// literally named "Runeshroud or Waveforged" that nothing else ever grants, making it permanently
// uncastable. Mirror the real Runeshroud/Waveforged state onto it instead.
// Runic Tempest (560036) also opens the gate for its 8 sec duration.
void SyncRuneshroudOrWaveforged(Player* player)
{
    bool active = player->HasAura(500288, player->GetGUID()) || player->HasAura(705565, player->GetGUID()) ||
        player->HasAura(560036, player->GetGUID());
    if (!active)
        player->RemoveAurasDueToSpell(808089, player->GetGUID());
    else if (!player->HasAura(808089, player->GetGUID()))
        player->CastSpell(player, 808089, true);
}

class runemaster_talent_events : public UnitScript
{
public:
    runemaster_talent_events() : UnitScript("runemaster_talent_events", true,
        {UNITHOOK_ON_AURA_APPLY, UNITHOOK_ON_AURA_REMOVE}) { }

    void OnAuraApply(Unit* unit, Aura* aura) override
    {
        Player* player = unit ? unit->ToPlayer() : nullptr;
        if (!player || player->getClass() != CLASS_SPIRIT_MAGE || !aura)
            return;
        uint32 id = aura->GetId();
        if (id == 707157 || id == 712310 || IsEarthTattoo(id))
            SyncStonePetroglyph(player);
        if (id == 500288 || id == 705565)
            SyncRuneshroudOrWaveforged(player);
        if (id == 560036)
        {
            // Runic Tempest: "Harness the power of your runic tattoos,
            // resetting the cooldown of Fist of the Ancients" (712326 chain).
            player->RemoveSpellCooldown(712326);
            SyncRuneshroudOrWaveforged(player);
        }
    }

    void OnAuraRemove(Unit* unit, AuraApplication* application, AuraRemoveMode mode) override
    {
        Player* player = unit ? unit->ToPlayer() : nullptr;
        if (!player || player->getClass() != CLASS_SPIRIT_MAGE || !application)
            return;
        Aura* aura = application->GetBase();
        uint32 id = aura->GetId();
        if (id == 707157 || IsEarthTattoo(id))
            SyncStonePetroglyph(player);
        if (id == 500288 && aura->GetCasterGUID() == player->GetGUID() && mode != AURA_REMOVE_BY_DEATH &&
            player->IsAlive() && player->IsInWorld() && player->HasAura(520054))
            player->CastSpell(player, 520768, true);
        if (id == 500288 || id == 705565 || id == 560036)
            SyncRuneshroudOrWaveforged(player);
    }
};

// Granite Shield (806996): "While Runic Tattoos: Earth is active, you now periodically gain
// Granite Shield every 20 sec." The load-time contract repoints the dead DBC trigger at the
// real absorb (520822); this gate keeps the tick silent while no Earth tattoo is active.
class aura_runemaster_granite_shield : public AuraScript
{
    PrepareAuraScript(aura_runemaster_granite_shield);

    void Tick(AuraEffect const*)
    {
        if (!EarthTattooActive(GetTarget()))
            PreventDefaultAction();
    }

    void Register() override
    {
        OnEffectPeriodic += AuraEffectPeriodicFn(aura_runemaster_granite_shield::Tick,
            EFFECT_1, SPELL_AURA_PERIODIC_TRIGGER_SPELL);
    }
};
}

void ApplyAscensionRunemasterTalentContracts(SpellInfo* info)
{
    if (info->Id == 712310 && info->SpellFamilyName == 38)
    {
        // The native periodic heal and effect-98 immunity already exist. Complete
        // knockback immunity for the separate destination-based effect as well.
        auto& effect = info->Effects[EFFECT_1];
        effect.Effect = SPELL_EFFECT_APPLY_AURA;
        effect.ApplyAuraName = SPELL_AURA_EFFECT_IMMUNITY;
        effect.MiscValue = SPELL_EFFECT_KNOCK_BACK_DEST;
        effect.BasePoints = 0;
        effect.DieSides = 0;
    }
    else if (info->Id == 806996)
    {
        // Issue 883: the shipped periodic trigger targets a dead spell, so the passive
        // never granted anything. Point the 20 s tick at the real Granite Shield absorb.
        auto& effect = info->Effects[EFFECT_1];
        effect.Effect = SPELL_EFFECT_APPLY_AURA;
        effect.ApplyAuraName = SPELL_AURA_PERIODIC_TRIGGER_SPELL;
        effect.Amplitude = 20000;
        effect.TriggerSpell = 520822;
    }
}

void AddSC_AscensionRunemasterTalents()
{
    new runemaster_talent_events();
    RegisterSpellScript(aura_runemaster_granite_shield);
}
