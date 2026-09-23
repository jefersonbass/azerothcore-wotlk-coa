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
    RockBarrier = 503630,
    MountainThane = 680404,
    ThanesGuidance = 680410,
    Earthbreaker = 560147,
    GeodeBarrageDamage = 803138,
    MountainFury = 806185,
    Earthmaker = 560150,
    EarthDestroyer = 560508,
    EarthenAvatar = 680421,
    Stonebound = 680415,
    BoonOfTheTurtle = 500935,
    Spiritbound = 681364
};

// Seismic abilities on the eight-second cooldown track. Cooldowns are
// stored per spell id, so every rank is trimmed; entries that are not on
// cooldown are untouched no-ops.
constexpr uint32 SeismicAbilities[] = {
    300693, 301276, 301277, 301278, 301279,                      // Seismic Smash
    503258, 503259, 503260, 503261, 503262,                      // Seismic Crash
    503263, 503264, 503265, 503266, 803981,
    560171, 560172, 560173, 560174, 560175,                      // Seismic Spike
    582532, 804433, 807093,
    807432, 807843                                               // Seismic Grasp
};

// Quake ranks on the same damage track as the Seismic abilities. Damage from
// any of these ranks feeds Earthmaker's Earthen Avatar cooldown trim.
constexpr uint32 QuakeAbilities[] = {
    503267, 503268, 504571, 505155, 505156, 505157                  // Quake
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

// Mountain Thane (680404): Earth's Rage stacks one additional time; the
// five percent stamina boost is native through Mod Total Stat Percentage.
// The engine caps applications at the authored five stacks, so the sixth
// is applied here once the aura reaches its cap.
class aura_ascension_mountain_thane : public AuraScript
{
    PrepareAuraScript(aura_ascension_mountain_thane);

    void Extend(AuraEffect const*, AuraEffectHandleModes)
    {
        Unit* owner = GetTarget();
        if (!owner->IsPlayer() || !owner->HasAura(MountainThane) || GetStackAmount() != 5)
            return;
        GetAura()->SetStackAmount(6);
    }

    void Register() override
    {
        AfterEffectApply += AuraEffectApplyFn(aura_ascension_mountain_thane::Extend,
            EFFECT_0, SPELL_AURA_MOD_INCREASE_SPEED, AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK);
    }
};

// Thane's Guidance (680410): auto attacks trim one second off Mountain
// Hammer (681130, 681420-681424) and Primal Rush (500696, 500768-500771,
// 502723-502724). Cooldowns are stored per spell id, so every rank is
// trimmed; entries that are not on cooldown are untouched no-ops.
class aura_ascension_thanes_guidance : public AuraScript
{
    PrepareAuraScript(aura_ascension_thanes_guidance);

    bool CheckProc(ProcEventInfo& event)
    {
        Unit* owner = GetTarget();
        DamageInfo const* damage = event.GetDamageInfo();
        return owner->IsPlayer() && owner->getClass() == CLASS_WILDWALKER && owner->IsAlive() &&
            event.GetActor() == owner && damage && damage->GetDamage() &&
            (event.GetTypeMask() & PROC_FLAG_DONE_MELEE_AUTO_ATTACK) &&
            event.GetActionTarget() && !owner->IsFriendlyTo(event.GetActionTarget());
    }

    void Trim(AuraEffect const*, ProcEventInfo&)
    {
        PreventDefaultAction();
        Player* player = GetTarget()->ToPlayer();
        if (!player)
            return;
        for (uint32 ability : {681130, 681420, 681421, 681422, 681423, 681424,
            500696, 500768, 500769, 500770, 500771, 502723, 502724})
            player->ModifySpellCooldown(ability, -1000);
    }

    void Register() override
    {
        DoCheckProc += AuraCheckProcFn(aura_ascension_thanes_guidance::CheckProc);
        OnEffectProc += AuraEffectProcFn(aura_ascension_thanes_guidance::Trim,
            EFFECT_0, SPELL_AURA_PROC_TRIGGER_SPELL);
    }
};

// Earthbreaker (560147): the ten percent melee haste is native through Mod
// Melee Haste; the authored Add % Modifier threat boost has no scoping
// data, so the Geode threat bonus rides on the shared cast hooks and this
// script only registers the talent binding.
class aura_ascension_earthbreaker : public AuraScript
{
    PrepareAuraScript(aura_ascension_earthbreaker);

    void Register() override { }
};

// Mountain Fury (806185): the channeled cone is fully authored in the DBC —
// the parent's two periodic-trigger effects fire 807724 (cone damage plus
// pull through 806186) and 807434 (the stacking two percent damage-taken
// reduction) every two seconds, and the pull's leap destination resolves
// natively from its three-yard radius. The damage helper's flat base and
// per-level term are native; only the Attack Power coefficient from its
// description rides in `spell_bonus_data`. The local Spell.dbc carries no
// cooldown for 806185, so the official forty-five second cooldown is set
// here on cast; Earth Destroyer (560508) shortens it to twenty-five seconds
// through its own script.
class aura_ascension_mountain_fury : public AuraScript
{
    PrepareAuraScript(aura_ascension_mountain_fury);

    void SetCooldown(AuraEffect const*, AuraEffectHandleModes)
    {
        if (Player* player = GetTarget()->ToPlayer())
        {
            uint32 cooldown = player->HasAura(EarthDestroyer) ? 25000 : 45000;
            player->AddSpellCooldown(MountainFury, 0, cooldown);
        }
    }

    void Register() override
    {
        AfterEffectApply += AuraEffectApplyFn(aura_ascension_mountain_fury::SetCooldown,
            EFFECT_0, SPELL_AURA_PERIODIC_TRIGGER_SPELL, AURA_EFFECT_HANDLE_REAL);
    }
};

// Earth Destroyer (560508): Mountain Fury's cooldown is forty-five seconds
// and its duration is four seconds longer. The plus four second duration is
// native through the authored Add Flat Modifier (effect 0,
// SPELLMOD_DURATION with the Mountain Fury class mask). The minus twenty
// second cooldown is handled by the Mountain Fury script above, which reads
// this aura; this script only registers the talent binding.
class aura_ascension_earth_destroyer : public AuraScript
{
    PrepareAuraScript(aura_ascension_earth_destroyer);

    void Register() override { }
};

// Stonebound (680415): Boon of the Turtle and Earth's Rage are fifty
// percent more effective while the passive is held. Both auras' amounts
// are recalculated from the base on every application, so refreshes
// never compound. Boon of the Turtle's rank chains (502793-502800) and
// the shared aura (500935) are bound below.
class aura_ascension_stonebound : public AuraScript
{
    PrepareAuraScript(aura_ascension_stonebound);

    void Amplify(AuraEffect const*, int32& amount, bool& /*canBeRecalculated*/)
    {
        Unit const* caster = GetCaster();
        if (caster && caster->IsPlayer() && caster->HasAura(Stonebound))
            amount += CalculatePct(amount, 50);
    }

    void Register() override
    {
        DoEffectCalcAmount += AuraEffectCalcAmountFn(aura_ascension_stonebound::Amplify,
            EFFECT_0, SPELL_AURA_MOD_RESISTANCE);
        DoEffectCalcAmount += AuraEffectCalcAmountFn(aura_ascension_stonebound::Amplify,
            EFFECT_1, SPELL_AURA_DAMAGE_SHIELD);
        DoEffectCalcAmount += AuraEffectCalcAmountFn(aura_ascension_stonebound::Amplify,
            EFFECT_2, SPELL_AURA_MOD_DAMAGE_PERCENT_TAKEN);
        DoEffectCalcAmount += AuraEffectCalcAmountFn(aura_ascension_stonebound::Amplify,
            EFFECT_0, SPELL_AURA_MOD_INCREASE_SPEED);
        DoEffectCalcAmount += AuraEffectCalcAmountFn(aura_ascension_stonebound::Amplify,
            EFFECT_1, SPELL_AURA_MOD_MELEE_HASTE);
    }
};

// Spiritbound (681364): avoiding an attack grants five Rage and trims one
// second off every Seismic ability. The avoidance roll rides the proc
// system like the other avoidance talents; the Rage gain and cooldown
// trim happen in the proc hook.
class aura_ascension_spiritbound : public AuraScript
{
    PrepareAuraScript(aura_ascension_spiritbound);

    bool CheckProc(ProcEventInfo& event)
    {
        Unit* owner = GetTarget();
        return owner->IsPlayer() && owner->getClass() == CLASS_WILDWALKER && owner->IsAlive() &&
            event.GetActionTarget() == owner && event.GetActor() &&
            !event.GetActor()->IsFriendlyTo(owner) &&
            (event.GetHitMask() & (PROC_HIT_MISS | PROC_HIT_DODGE | PROC_HIT_PARRY));
    }

    void Reward(AuraEffect const*, ProcEventInfo&)
    {
        PreventDefaultAction();
        Player* player = GetTarget()->ToPlayer();
        if (!player)
            return;
        player->ModifyPower(POWER_RAGE, 50);
        for (uint32 ability : SeismicAbilities)
            player->ModifySpellCooldown(ability, -1000);
    }

    void Register() override
    {
        DoCheckProc += AuraCheckProcFn(aura_ascension_spiritbound::CheckProc);
        OnEffectProc += AuraEffectProcFn(aura_ascension_spiritbound::Reward,
            EFFECT_0, SPELL_AURA_PROC_TRIGGER_SPELL);
        OnEffectProc += AuraEffectProcFn(aura_ascension_spiritbound::Reward,
            EFFECT_1, SPELL_AURA_PROC_TRIGGER_SPELL);
    }
};

// Earthmaker (560150): damage dealt by Seismic abilities and Quake trims one
// second off Earthen Avatar. The authored proc trigger (effect 0) and the
// triggered cooldown helper (560151) carry no proc flags or class mask in
// the DBC, so the native proc chain never fires; the trim happens here on
// the shared Seismic/Quake damage check instead. The minus three second
// Seismic cooldown part is native through the authored Add Flat Modifier
// (effect 1, SPELLMOD_COOLDOWN with the Seismic class mask).
class aura_ascension_earthmaker : public AuraScript
{
    PrepareAuraScript(aura_ascension_earthmaker);

    bool CheckProc(ProcEventInfo& event)
    {
        Unit* owner = GetTarget();
        DamageInfo const* damage = event.GetDamageInfo();
        SpellInfo const* info = damage ? damage->GetSpellInfo() : nullptr;
        return owner->IsPlayer() && owner->getClass() == CLASS_WILDWALKER && owner->IsAlive() &&
            event.GetActor() == owner && damage && damage->GetDamage() && info && info->SpellFamilyName == 37 &&
            !owner->IsFriendlyTo(event.GetActionTarget());
    }

    void Trim(AuraEffect const*, ProcEventInfo& event)
    {
        PreventDefaultAction();
        if (!CheckProc(event))
            return;
        SpellInfo const* info = event.GetDamageInfo()->GetSpellInfo();
        bool seismic = false;
        for (uint32 ability : SeismicAbilities)
            if (sSpellMgr->GetFirstSpellInChain(info->Id) == sSpellMgr->GetFirstSpellInChain(ability))
            {
                seismic = true;
                break;
            }
        if (!seismic)
            for (uint32 ability : QuakeAbilities)
                if (sSpellMgr->GetFirstSpellInChain(info->Id) == sSpellMgr->GetFirstSpellInChain(ability))
                {
                    seismic = true;
                    break;
                }
        if (seismic)
            if (Player* player = GetTarget()->ToPlayer())
                player->ModifySpellCooldown(EarthenAvatar, -1000);
    }

    void Register() override
    {
        DoCheckProc += AuraCheckProcFn(aura_ascension_earthmaker::CheckProc);
        OnEffectProc += AuraEffectProcFn(aura_ascension_earthmaker::Trim,
            EFFECT_0, SPELL_AURA_PROC_TRIGGER_SPELL);
    }
};

class mountain_talent_metadata : public GlobalScript
{
public:
    mountain_talent_metadata() : GlobalScript("mountain_talent_metadata", {GLOBALHOOK_ON_LOAD_SPELL_CUSTOM_ATTR}) { }

    void OnLoadSpellCustomAttr(SpellInfo* info) override
    {
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
    RegisterSpellScript(aura_ascension_mountain_thane);
    RegisterSpellScript(aura_ascension_thanes_guidance);
    RegisterSpellScript(aura_ascension_earthbreaker);
    RegisterSpellScript(aura_ascension_mountain_fury);
    RegisterSpellScript(aura_ascension_stonebound);
    RegisterSpellScript(aura_ascension_spiritbound);
    RegisterSpellScript(aura_ascension_earthmaker);
    RegisterSpellScript(aura_ascension_earth_destroyer);
    new mountain_talent_metadata();
}
