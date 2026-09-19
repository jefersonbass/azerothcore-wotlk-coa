/* Copyright (C) 2016+ AzerothCore, GNU AGPL v3. */
#include "AscensionRunemasterTalents.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SpellAuraEffects.h"
#include "SpellAuras.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "SpellScript.h"

namespace
{
bool IsEarthTattoo(uint32 id)
{
    return id == 801094 || (id >= 803754 && id <= 803758);
}

// Guarding Rune (500464): the 2 min Engravement defensive whose cooldown
// Protective Warding shaves on critical hits taken.
constexpr uint32 SPELL_RUNE_OF_GUARDING = 500464;

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

// Protective Warding (800756): "Critical damage taken reduces the cooldown of
// Rune of Guarding by 10%." Gaining damage has no standalone proc event, so the
// load-time contract arms the passive with PROC_FLAG_TAKEN_DAMAGE and this
// script filters to critical hits before shaving the Guarding Rune cooldown.
class aura_runemaster_protective_warding : public AuraScript
{
    PrepareAuraScript(aura_runemaster_protective_warding);

    bool Check(ProcEventInfo& event)
    {
        Unit* victim = GetTarget();
        return victim && event.GetActionTarget() == victim &&
            event.GetTypeMask() & (PROC_FLAG_TAKEN_DAMAGE | PROC_FLAG_TAKEN_PERIODIC) &&
            event.GetHitMask() & PROC_HIT_CRITICAL;
    }

    void Proc(ProcEventInfo& /*event*/)
    {
        if (Player* player = GetTarget()->ToPlayer())
            if (uint32 remaining = player->GetSpellCooldownDelay(SPELL_RUNE_OF_GUARDING))
                player->ModifySpellCooldown(SPELL_RUNE_OF_GUARDING, -int32(remaining * 0.1f));
    }

    void Register() override
    {
        DoCheckProc += AuraCheckProcFn(aura_runemaster_protective_warding::Check);
        OnProc += AuraProcFn(aura_runemaster_protective_warding::Proc);
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

// Elemental Carvings (705618): "Fist of the Ancients now unleashes an elemental
// carving at random. Fire: +15% Armor penetration 6 s. Water: 300 mana over 6 s.
// Earth: 3% missing health every 1 s 6 s. Air: +10% melee haste 6 s." The
// talent's DBC proc points at a dead random-trigger helper, so pick one of the
// four real carving buffs (all valid auras in the shipped DBC) on each cast.
constexpr uint32 SPELL_ELEMENTAL_CARVINGS = 705618;
constexpr uint32 SPELL_FIST_OF_THE_ANCIENTS = 712326;
constexpr uint32 CARVING_FIRE = 706520;
constexpr uint32 CARVING_WATER = 707142;
constexpr uint32 CARVING_EARTH = 712300;
constexpr uint32 CARVING_AIR = 653253;

class runemaster_elemental_carvings : public AllSpellScript
{
public:
    runemaster_elemental_carvings() : AllSpellScript("runemaster_elemental_carvings", {ALLSPELLHOOK_ON_CAST}) { }

    void OnSpellCast(Spell* spell, Unit* caster, SpellInfo const* info, bool) override
    {
        Player* player = caster ? caster->ToPlayer() : nullptr;
        if (!player || player->getClass() != CLASS_SPIRIT_MAGE || spell->IsTriggered())
            return;
        if (!player->HasAura(SPELL_ELEMENTAL_CARVINGS, player->GetGUID()))
            return;
        if (sSpellMgr->GetFirstSpellInChain(info->Id) != SPELL_FIST_OF_THE_ANCIENTS)
            return;
        switch (urand(0, 3))
        {
            case 0: player->CastSpell(player, CARVING_FIRE, true); break;
            case 1: player->CastSpell(player, CARVING_WATER, true); break;
            case 2: player->CastSpell(player, CARVING_EARTH, true); break;
            case 3: player->CastSpell(player, CARVING_AIR, true); break;
        }
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
    else if (info->Id == 800756)
    {
        // Issue 905: the DBC ships a dead Proc Trigger Spell with no proc flags, so
        // "critical damage taken" never reached any handler. Arm the taken-damage
        // proc; the bound script filters to crits and shaves the Guarding Rune CD.
        info->ProcFlags = PROC_FLAG_TAKEN_DAMAGE | PROC_FLAG_TAKEN_PERIODIC;
    }
    else if (info->Id == 705618)
    {
        // Issue 973: Elemental Carvings ships without SPELL_ATTR0_PASSIVE, so the
        // talent learn and login-load passes never applied anything, and its DBC
        // proc points at "Primeval Carving" (712327), whose random-trigger effect
        // (184) has a null handler and a dead spell id. The bound script picks
        // the carving directly on Fist of the Ancients casts instead.
        info->Attributes |= SPELL_ATTR0_PASSIVE;
    }
}

void AddSC_AscensionRunemasterTalents()
{
    new runemaster_talent_events();
    new runemaster_elemental_carvings();
    RegisterSpellScript(aura_runemaster_granite_shield);
    RegisterSpellScript(aura_runemaster_protective_warding);
}
