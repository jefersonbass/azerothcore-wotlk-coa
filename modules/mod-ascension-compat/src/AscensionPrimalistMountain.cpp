/* Copyright (C) 2016+ AzerothCore, GNU AGPL v3. */
#include "Player.h"
#include "ScriptMgr.h"
#include "SpellAuraEffects.h"
#include "SpellMgr.h"
#include "SpellScript.h"

namespace
{
enum MountainSpells : uint32
{
    EarthsRage = 806068,
    CallOfTheMountain = 680406,
    MountainBuff = 680472,
    Terrasmash = 706220,
    Geode = 804002,
    ResourcesOfTheEarth = 560548,
    Replenishment = 1257670,
    MountainMover = 805643,
    MountainMoverStacks = 805644,
    Bash = 680964,
    Bashed = 680949,
    ImprovedUrsocsBellow = 560505,
    EarthenforgedBarrier = 680408,
    RockBarrier = 503630
};

class aura_ascension_blessed_by_earth : public AuraScript
{
    PrepareAuraScript(aura_ascension_blessed_by_earth);

    bool CheckProc(ProcEventInfo& event)
    {
        Unit* owner = GetTarget();
        DamageInfo const* damage = event.GetDamageInfo();
        SpellInfo const* info = damage ? damage->GetSpellInfo() : nullptr;
        return owner->IsPlayer() && owner->getClass() == CLASS_WILDWALKER && owner->IsAlive() &&
            event.GetActor() == owner && damage && damage->GetDamage() && info && info->SpellFamilyName == 37 &&
            ((info->SpellFamilyFlags[0] & 16384) || (info->SpellFamilyFlags[2] & 256)) &&
            event.GetActionTarget() && !owner->IsFriendlyTo(event.GetActionTarget());
    }

    void Gain(AuraEffect const*, ProcEventInfo& event)
    {
        PreventDefaultAction();
        if (CheckProc(event))
            GetTarget()->CastSpell(GetTarget(), EarthsRage, true);
    }

    void Register() override
    {
        DoCheckProc += AuraCheckProcFn(aura_ascension_blessed_by_earth::CheckProc);
        OnEffectProc += AuraEffectProcFn(aura_ascension_blessed_by_earth::Gain,
            EFFECT_0, SPELL_AURA_PROC_TRIGGER_SPELL);
    }
};

class aura_ascension_mountain_threshold : public AuraScript
{
    PrepareAuraScript(aura_ascension_mountain_threshold);
    uint8 _previous = 0;

    void Changed(AuraEffect const*, AuraEffectHandleModes mode)
    {
        uint8 current = GetStackAmount();
        bool crossed = _previous < 5 && current >= 5;
        _previous = current;
        // REAL also runs when saved auras load. Loading five stacks is not a gain.
        Unit* owner = GetTarget();
        if (mode == AURA_EFFECT_HANDLE_REAL || !crossed || !owner->IsPlayer() ||
            owner->getClass() != CLASS_WILDWALKER || !owner->IsAlive() ||
            GetCasterGUID() != owner->GetGUID() || !owner->HasAura(CallOfTheMountain))
            return;
        owner->CastSpell(owner, MountainBuff, true);
    }

    void Register() override
    {
        AfterEffectApply += AuraEffectApplyFn(aura_ascension_mountain_threshold::Changed,
            EFFECT_0, SPELL_AURA_MOD_INCREASE_SPEED, AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK);
    }
};

// Terrasmash (706220): damage dealt by the off hand weapon has a thirty
// percent chance to hurl a Geode (804002), whose damage and Rage energize
// are native. The proc aura sits on effect 1 per the DBC audit and on
// effect 0 per the archive dump, so both are bound; only the effect
// carrying the proc aura type fires.
class aura_ascension_terrasmash : public AuraScript
{
    PrepareAuraScript(aura_ascension_terrasmash);

    bool CheckProc(ProcEventInfo& event)
    {
        Unit* owner = GetTarget();
        DamageInfo const* damage = event.GetDamageInfo();
        return owner->IsPlayer() && owner->getClass() == CLASS_WILDWALKER && owner->IsAlive() &&
            event.GetActor() == owner && damage && damage->GetDamage() &&
            (event.GetTypeMask() & PROC_FLAG_DONE_OFFHAND_ATTACK) &&
            event.GetActionTarget() && !owner->IsFriendlyTo(event.GetActionTarget());
    }

    void Hurl(AuraEffect const*, ProcEventInfo& event)
    {
        PreventDefaultAction();
        GetTarget()->CastSpell(event.GetActionTarget(), Geode, true);
    }

    void Register() override
    {
        DoCheckProc += AuraCheckProcFn(aura_ascension_terrasmash::CheckProc);
        OnEffectProc += AuraEffectProcFn(aura_ascension_terrasmash::Hurl,
            EFFECT_0, SPELL_AURA_PROC_TRIGGER_SPELL);
        OnEffectProc += AuraEffectProcFn(aura_ascension_terrasmash::Hurl,
            EFFECT_1, SPELL_AURA_PROC_TRIGGER_SPELL);
    }
};

// Resources of the Earth (560548): critical strikes grant party and raid
// allies within one hundred yards Replenishment (1257670), whose mana
// energize is native. The proc aura sits on effect 0 per the archive dump
// and on effect 1 per the DBC audit, so both are bound; only the effect
// carrying the proc aura type fires.
class aura_ascension_resources_of_the_earth : public AuraScript
{
    PrepareAuraScript(aura_ascension_resources_of_the_earth);

    bool CheckProc(ProcEventInfo& event)
    {
        Unit* owner = GetTarget();
        DamageInfo const* damage = event.GetDamageInfo();
        return owner->IsPlayer() && owner->getClass() == CLASS_WILDWALKER && owner->IsAlive() &&
            event.GetActor() == owner && damage && damage->GetDamage() &&
            (event.GetHitMask() & PROC_HIT_CRITICAL) &&
            event.GetActionTarget() && !owner->IsFriendlyTo(event.GetActionTarget());
    }

    void Replenish(AuraEffect const*, ProcEventInfo&)
    {
        PreventDefaultAction();
        Player* player = GetTarget()->ToPlayer();
        for (auto const& reference : player->GetMap()->GetPlayers())
            if (Player* member = reference.GetSource())
                if (member->IsInWorld() && !member->IsGameMaster() &&
                    member->IsWithinDistInMap(player, 100.0f) &&
                    (member == player || member->IsInPartyWith(player) || member->IsInRaidWith(player)))
                    player->CastSpell(member, Replenishment, true);
    }

    void Register() override
    {
        DoCheckProc += AuraCheckProcFn(aura_ascension_resources_of_the_earth::CheckProc);
        OnEffectProc += AuraEffectProcFn(aura_ascension_resources_of_the_earth::Replenish,
            EFFECT_0, SPELL_AURA_PROC_TRIGGER_SPELL);
        OnEffectProc += AuraEffectProcFn(aura_ascension_resources_of_the_earth::Replenish,
            EFFECT_1, SPELL_AURA_PROC_TRIGGER_SPELL);
    }
};

// Mountain Mover (805643): avoiding an attack grants a stack of Mountain
// Mover (805644) for ten seconds, stacking five times; the stacking aura
// config is native. Wildclaw consumes the stacks in the shared cast hooks.
class aura_ascension_mountain_mover : public AuraScript
{
    PrepareAuraScript(aura_ascension_mountain_mover);

    bool CheckProc(ProcEventInfo& event)
    {
        Unit* owner = GetTarget();
        return owner->IsPlayer() && owner->getClass() == CLASS_WILDWALKER && owner->IsAlive() &&
            event.GetActionTarget() == owner && event.GetActor() &&
            !event.GetActor()->IsFriendlyTo(owner) &&
            (event.GetHitMask() & (PROC_HIT_MISS | PROC_HIT_DODGE | PROC_HIT_PARRY));
    }

    void Stack(AuraEffect const*, ProcEventInfo&)
    {
        PreventDefaultAction();
        GetTarget()->CastSpell(GetTarget(), MountainMoverStacks, true);
    }

    void Register() override
    {
        DoCheckProc += AuraCheckProcFn(aura_ascension_mountain_mover::CheckProc);
        OnEffectProc += AuraEffectProcFn(aura_ascension_mountain_mover::Stack,
            EFFECT_0, SPELL_AURA_PROC_TRIGGER_SPELL);
        OnEffectProc += AuraEffectProcFn(aura_ascension_mountain_mover::Stack,
            EFFECT_1, SPELL_AURA_PROC_TRIGGER_SPELL);
        OnEffectProc += AuraEffectProcFn(aura_ascension_mountain_mover::Stack,
            EFFECT_2, SPELL_AURA_PROC_TRIGGER_SPELL);
    }
};

// Bash (680964): auto attacks roll a thirty percent chance to Bash the
// victim for half a weapon swing plus a one-second stun (680949), whose
// weapon damage and stun are native. Auto attacks never reach the spell
// hit-result hooks, so the roll rides the proc system instead.
class aura_ascension_bash : public AuraScript
{
    PrepareAuraScript(aura_ascension_bash);

    bool CheckProc(ProcEventInfo& event)
    {
        Unit* owner = GetTarget();
        DamageInfo const* damage = event.GetDamageInfo();
        return owner->IsPlayer() && owner->getClass() == CLASS_WILDWALKER && owner->IsAlive() &&
            event.GetActor() == owner && damage && damage->GetDamage() &&
            (event.GetTypeMask() & PROC_FLAG_DONE_MELEE_AUTO_ATTACK) &&
            event.GetActionTarget() && !owner->IsFriendlyTo(event.GetActionTarget());
    }

    void Strike(AuraEffect const*, ProcEventInfo& event)
    {
        PreventDefaultAction();
        GetTarget()->CastSpell(event.GetActionTarget(), Bashed, true);
    }

    void Register() override
    {
        DoCheckProc += AuraCheckProcFn(aura_ascension_bash::CheckProc);
        OnEffectProc += AuraEffectProcFn(aura_ascension_bash::Strike,
            EFFECT_0, SPELL_AURA_PROC_TRIGGER_SPELL);
    }
};

// Improved Ursoc's Bellow (560505): the passive's Add % Modifier aura has
// no scoping data, so Ursoc's Bellow's attack power reduction is scaled
// here by forty percent when the caster holds the passive. The hook runs
// on every amount calculation, so refreshes never compound.
class aura_ascension_ursocs_bellow : public AuraScript
{
    PrepareAuraScript(aura_ascension_ursocs_bellow);

    void Weaken(AuraEffect const*, int32& amount, bool& /*canBeRecalculated*/)
    {
        Unit const* caster = GetCaster();
        if (caster && caster->IsPlayer() && caster->HasAura(ImprovedUrsocsBellow))
            amount += CalculatePct(amount, 40);
    }

    void Register() override
    {
        DoEffectCalcAmount += AuraEffectCalcAmountFn(aura_ascension_ursocs_bellow::Weaken,
            EFFECT_0, SPELL_AURA_MOD_ATTACK_POWER);
    }
};

// Earthenforged Barrier (680408): Rock Barrier's armor is fifty percent
// stronger while the passive is held; the cost reduction rides on the
// shared cast hook. The amount hook runs on every calculation, so
// refreshes never compound.
class aura_ascension_earthenforged_barrier : public AuraScript
{
    PrepareAuraScript(aura_ascension_earthenforged_barrier);

    void Fortify(AuraEffect const*, int32& amount, bool& /*canBeRecalculated*/)
    {
        Unit const* caster = GetCaster();
        if (caster && caster->IsPlayer() && caster->HasAura(EarthenforgedBarrier))
            amount += CalculatePct(amount, 50);
    }

    void Register() override
    {
        DoEffectCalcAmount += AuraEffectCalcAmountFn(aura_ascension_earthenforged_barrier::Fortify,
            EFFECT_0, SPELL_AURA_MOD_RESISTANCE);
    }
};

class mountain_talent_metadata : public GlobalScript
{
public:
    mountain_talent_metadata() : GlobalScript("mountain_talent_metadata", {GLOBALHOOK_ON_LOAD_SPELL_CUSTOM_ATTR}) { }

    void OnLoadSpellCustomAttr(SpellInfo* info) override
    {
        // Old trigger granted Mountain on the first stack without its talent.
        if (info->Id == EarthsRage && info->SpellFamilyName == 37)
            info->Effects[EFFECT_2].Effect = 0;

        // Mountain Mover stacks (805644): the authored minus ten percent
        // modifier has no scoping data and would fight the scripted
        // consumption, so it is neutralized; the cost and damage bonuses
        // ride on the shared Wildclaw cast hooks instead.
        if (info->Id == MountainMoverStacks && info->SpellFamilyName == 37)
            info->Effects[EFFECT_0].ApplyAuraName = SPELL_AURA_DUMMY;
    }
};
}

void AddSC_AscensionPrimalistMountain()
{
    RegisterSpellScript(aura_ascension_blessed_by_earth);
    RegisterSpellScript(aura_ascension_mountain_threshold);
    RegisterSpellScript(aura_ascension_terrasmash);
    RegisterSpellScript(aura_ascension_resources_of_the_earth);
    RegisterSpellScript(aura_ascension_mountain_mover);
    RegisterSpellScript(aura_ascension_bash);
    RegisterSpellScript(aura_ascension_ursocs_bellow);
    RegisterSpellScript(aura_ascension_earthenforged_barrier);
    new mountain_talent_metadata();
}
