/* Copyright (C) 2016+ AzerothCore, GNU AGPL v3. */
#include "AscensionRangerTalents.h"
#include "Creature.h"
#include "Item.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "Spell.h"
#include "SpellAuraEffects.h"
#include "SpellAuras.h"
#include "SpellMgr.h"
#include "SpellScript.h"
#include "SpellScriptLoader.h"
#include "TemporarySummon.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace
{
enum RangerTalentSpells : uint32
{
    SPELL_LIGHT_ARROWS = 681292,
    SPELL_HIGHWAYMAN_TRIGGER = 705063,
    SPELL_KNOCKOUT_INCAPACITATE = 706762,
    SPELL_STONEMASONS_SECRET = 524654,
    SPELL_DIRTY_BLADES = 680276,
    SPELL_ADVANTAGE = 804329,
    SPELL_EXTEND_DIRTY_BLADES = 524653,
    SPELL_SNATCH = 803115,
    SPELL_SNATCH_DISARM = 803123,
    SPELL_PHOENIX_PLUMES = 705074,
    SPELL_PHOENIX_PLUMES_WAR_FALCON = 520558,
    SPELL_SWIFTSHOT = 705028,
    SPELL_SWIFTSHOT_VULNERABILITY = 800578,
    SPELL_WAR_FALCON_PRESENCE = 680278,
    SPELL_DRAGONHAWK_PRESENCE = 681394,
    SPELL_TACTICAL_ADVANTAGE = 706748,
    SPELL_TACTICAL_ADVANTAGE_DEBUFF = 706749,
    SPELL_BUSHWHACK = 557333,
    SPELL_PIERCED = 705033,
    SPELL_FRENZY = 520492,
    SPELL_WORN_OUT = 804457,
    SPELL_PILFERING = 705087,
    SPELL_PILFERING_HEAL = 520880,
    SPELL_RAPID_STRIKES = 560341,
    SPELL_RAPID_STRIKE = 560342,
    SPELL_GUIDANCE = 532261,
    SPELL_MARKED_FOR_DEATH = 806973,
    SPELL_MARKED_FOR_DEATH_BUFF = 806974,
    SPELL_DEEPWOOD_POISON = 800079,
    SPELL_DEEPWOOD_POISON_DOT = 801472,
    SPELL_RANGER_EXPLOIT = 520570,
    SPELL_DEADLY_ACCURATE = 560345
};

constexpr uint32 STONEMASON_SOURCE_FIRST = 501715;
constexpr uint32 STONEMASON_SOURCE_LAST = 501723;
constexpr uint32 STONEMASON_SOURCE_SKULLPIERCER_LATEST = 802036;
constexpr uint32 STONEMASON_ASSAULT_FIRST = 503099;
constexpr uint32 STONEMASON_ASSAULT_LAST = 503105;
constexpr uint32 STONEMASON_ASSAULT_LATEST = 803108;

inline bool IsSkullpiercerOrAssault(uint32 spellId)
{
    return (spellId >= STONEMASON_SOURCE_FIRST && spellId <= STONEMASON_SOURCE_LAST) ||
        spellId == STONEMASON_SOURCE_SKULLPIERCER_LATEST ||
        (spellId >= STONEMASON_ASSAULT_FIRST && spellId <= STONEMASON_ASSAULT_LAST) ||
        spellId == STONEMASON_ASSAULT_LATEST;
}

enum RangerTalentRankChains : uint32
{
    CHAIN_SKULLPIERCER = 802036,
    CHAIN_WOODLAND_ARROW = 806368,
    CHAIN_PRECISION_SHOT = 500075
};

enum RangerCompanionEntries : uint32
{
    NPC_WAR_FALCON_FALCONS_CALL = 50264,
    NPC_WAR_FALCON = 50393,
    NPC_DRAGONHAWK = 52393
};

struct WingmanCompanion
{
    uint32 Entry;
    uint32 Presence;
};

constexpr std::array<WingmanCompanion, 3> WingmanCompanions =
{{
    {NPC_WAR_FALCON_FALCONS_CALL, SPELL_WAR_FALCON_PRESENCE},
    {NPC_WAR_FALCON, SPELL_WAR_FALCON_PRESENCE},
    {NPC_DRAGONHAWK, SPELL_DRAGONHAWK_PRESENCE}
}};

constexpr uint8 RANGER_ADVANTAGE_MAX_STACKS = 5;
constexpr int32 WINGMAN_REFRESH_MS = 500;

template <typename Visitor>
void ForEachPresentCompanion(Unit* owner, Visitor&& visit)
{
    for (Unit* controlled : owner->m_Controlled)
    {
        if (!controlled || !controlled->IsAlive() || controlled->GetOwnerGUID() != owner->GetGUID())
            continue;
        for (WingmanCompanion const& companion : WingmanCompanions)
        {
            SpellInfo const* presence = sSpellMgr->GetSpellInfo(companion.Presence);
            if (controlled->GetEntry() == companion.Entry && presence &&
                owner->IsWithinDistInMap(controlled, presence->Effects[EFFECT_0].CalcRadius()))
                visit(presence);
        }
    }
}

class ranger_wingman_companions : public PlayerScript
{
public:
    ranger_wingman_companions() : PlayerScript("ranger_wingman_companions",
        {PLAYERHOOK_ON_AFTER_GUARDIAN_INIT_STATS_FOR_LEVEL}) { }

    void OnPlayerAfterGuardianInitStatsForLevel(Player* player, Guardian* guardian) override
    {
        if (!player || player->getClass() != CLASS_RANGER || !IsWingmanCompanion(guardian))
            return;

        CreatureTemplate const* info = guardian->GetCreatureTemplate();
        CreatureBaseStats const* stats = sObjectMgr->GetCreatureBaseStats(guardian->GetLevel(), info->unit_class);
        float const damage = stats->GenerateBaseDamage(info);
        for (WeaponAttackType attack : {BASE_ATTACK, OFF_ATTACK, RANGED_ATTACK})
        {
            guardian->SetBaseWeaponDamage(attack, MINDAMAGE, damage);
            guardian->SetBaseWeaponDamage(attack, MAXDAMAGE, damage * 1.5f);
        }
        guardian->SetStatFlatModifier(UNIT_MOD_ATTACK_POWER, BASE_VALUE, float(stats->AttackPower));
        guardian->SetStatFlatModifier(UNIT_MOD_ATTACK_POWER_RANGED, BASE_VALUE, float(stats->RangedAttackPower));
        guardian->UpdateAllStats();
    }

    static bool IsWingmanCompanion(Creature const* creature)
    {
        if (!creature)
            return false;

        for (WingmanCompanion const& companion : WingmanCompanions)
            if (creature->GetEntry() == companion.Entry)
                return true;
        return false;
    }
};

bool HasFullAdvantage(Player const* player)
{
    Aura const* advantage = player->GetAura(SPELL_ADVANTAGE);
    return advantage && advantage->GetStackAmount() == RANGER_ADVANTAGE_MAX_STACKS;
}

class spell_ascension_ranger_light_arrows : public SpellScript
{
    PrepareSpellScript(spell_ascension_ranger_light_arrows);
    int32 _bonus = 0;

    bool Load() override { return GetCaster()->IsPlayer() && GetCaster()->getClass() == CLASS_RANGER; }

    void Snapshot()
    {
        Unit* target = GetExplTargetUnit();
        if (target && GetCaster()->GetExactDist(target) >= 40.0f)
            if (AuraEffect const* talent = GetCaster()->GetAuraEffect(SPELL_LIGHT_ARROWS, EFFECT_0))
                _bonus = std::max(0, talent->GetAmount());
    }

    void Damage()
    {
        if (_bonus && GetHitDamage() > 0)
            SetHitDamage(int32(std::min<int64>(int64(GetHitDamage()) * (int64(100) + _bonus) / 100,
                std::numeric_limits<int32>::max())));
    }

    void Register() override
    {
        BeforeCast += SpellCastFn(spell_ascension_ranger_light_arrows::Snapshot);
        OnHit += SpellHitFn(spell_ascension_ranger_light_arrows::Damage);
    }
};

class spell_ascension_ranger_knockout : public SpellScript
{
    PrepareSpellScript(spell_ascension_ranger_knockout);

    bool Validate(SpellInfo const*) override { return ValidateSpellInfo({SPELL_KNOCKOUT_INCAPACITATE}); }
    bool Load() override { return GetCaster()->IsPlayer() && GetCaster()->getClass() == CLASS_RANGER; }

    void Incapacitate(SpellEffIndex)
    {
        if (Unit* target = GetHitUnit())
            GetCaster()->CastSpell(target, SPELL_KNOCKOUT_INCAPACITATE, true);
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_ascension_ranger_knockout::Incapacitate, EFFECT_1, SPELL_EFFECT_DUMMY);
    }
};

class aura_ascension_ranger_wingman : public AuraScript
{
    PrepareAuraScript(aura_ascension_ranger_wingman);

    bool Validate(SpellInfo const*) override
    {
        return ValidateSpellInfo({SPELL_WAR_FALCON_PRESENCE, SPELL_DRAGONHAWK_PRESENCE});
    }

    void Calculate(AuraEffect const*, int32& amount, bool& recalculate)
    {
        recalculate = true;
        amount = 0;
        ForEachPresentCompanion(GetUnitOwner(), [&amount](SpellInfo const* presence)
        {
            amount += presence->Effects[EFFECT_2].CalcValue();
        });
    }

    void Period(AuraEffect const*, bool& periodic, int32& interval)
    {
        periodic = true;
        interval = WINGMAN_REFRESH_MS;
    }

    void Refresh(AuraEffect const* effect)
    {
        PreventDefaultAction();
        GetAura()->GetEffect(effect->GetEffIndex())->RecalculateAmount();
    }

    void Register() override
    {
        DoEffectCalcAmount += AuraEffectCalcAmountFn(aura_ascension_ranger_wingman::Calculate,
            EFFECT_2, SPELL_AURA_MOD_DAMAGE_PERCENT_TAKEN);
        DoEffectCalcPeriodic += AuraEffectCalcPeriodicFn(aura_ascension_ranger_wingman::Period,
            EFFECT_2, SPELL_AURA_MOD_DAMAGE_PERCENT_TAKEN);
        OnEffectPeriodic += AuraEffectPeriodicFn(aura_ascension_ranger_wingman::Refresh,
            EFFECT_2, SPELL_AURA_MOD_DAMAGE_PERCENT_TAKEN);
    }
};

class aura_ascension_ranger_highwayman : public AuraScript
{
    PrepareAuraScript(aura_ascension_ranger_highwayman);

    bool Validate(SpellInfo const*) override
    {
        return ValidateSpellInfo({SPELL_HIGHWAYMAN_TRIGGER});
    }

    bool CheckProc(ProcEventInfo& eventInfo)
    {
        Unit* ranger = GetTarget();
        Unit* victim = eventInfo.GetActionTarget();
        return eventInfo.GetActor() == ranger && victim && victim != ranger && eventInfo.GetDamageInfo() &&
            !victim->HasInArc(float(M_PI), ranger);
    }

    void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
    {
        PreventDefaultAction();
        GetTarget()->CastSpell(eventInfo.GetActionTarget(), SPELL_HIGHWAYMAN_TRIGGER, true, nullptr, aurEff);
    }

    void Register() override
    {
        DoCheckProc += AuraCheckProcFn(aura_ascension_ranger_highwayman::CheckProc);
        OnEffectProc += AuraEffectProcFn(aura_ascension_ranger_highwayman::HandleProc, EFFECT_0, SPELL_AURA_ANY);
    }
};

class spell_ascension_ranger_frenzy : public SpellScript
{
    PrepareSpellScript(spell_ascension_ranger_frenzy);

    bool Validate(SpellInfo const* spellInfo) override
    {
        SpellEffectInfo const& wornOut = spellInfo->Effects[EFFECT_2];
        return spellInfo->Id == SPELL_FRENZY && wornOut.Effect == SPELL_EFFECT_TRIGGER_SPELL &&
            wornOut.TriggerSpell == SPELL_WORN_OUT && ValidateSpellInfo({SPELL_WORN_OUT});
    }

    void SkipWornOut(std::list<WorldObject*>& targets)
    {
        targets.remove_if([](WorldObject* target)
        {
            Unit* unit = target->ToUnit();
            return !unit || unit->HasAura(SPELL_WORN_OUT);
        });
    }

    void Register() override
    {
        OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_ascension_ranger_frenzy::SkipWornOut,
            EFFECT_0, TARGET_UNIT_CASTER_AREA_RAID);
        OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_ascension_ranger_frenzy::SkipWornOut,
            EFFECT_1, TARGET_UNIT_CASTER_AREA_RAID);
        OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_ascension_ranger_frenzy::SkipWornOut,
            EFFECT_2, TARGET_UNIT_CASTER_AREA_RAID);
    }
};

class aura_ascension_ranger_pilfering : public AuraScript
{
    PrepareAuraScript(aura_ascension_ranger_pilfering);

    bool Validate(SpellInfo const* spellInfo) override
    {
        return spellInfo->Id == SPELL_PILFERING &&
            spellInfo->Effects[EFFECT_1].IsAura(AuraType(354)) &&
            spellInfo->Effects[EFFECT_1].TriggerSpell == SPELL_PILFERING_HEAL &&
            ValidateSpellInfo({SPELL_PILFERING_HEAL, SPELL_DIRTY_BLADES});
    }

    bool Load() override
    {
        Unit* ranger = GetUnitOwner();
        return ranger && ranger->IsPlayer() && ranger->ToPlayer()->getClass() == CLASS_RANGER;
    }

    bool CheckProc(ProcEventInfo& event)
    {
        DamageInfo const* damage = event.GetDamageInfo();
        Unit* victim = event.GetActionTarget();
        return event.GetActor() == GetTarget() && victim && victim != GetTarget() &&
            !GetTarget()->IsFriendlyTo(victim) && damage && damage->GetDamage() &&
            (damage->GetDamageType() == DIRECT_DAMAGE || damage->GetDamageType() == SPELL_DIRECT_DAMAGE) &&
            GetTarget()->HasAura(SPELL_DIRTY_BLADES);
    }

    void Heal(AuraEffect const* effect, ProcEventInfo& event)
    {
        PreventDefaultAction();
        AuraEffect const* blades = GetTarget()->GetAuraEffect(SPELL_DIRTY_BLADES, EFFECT_0);
        if (!blades)
            return;

        uint64 amount = uint64(event.GetDamageInfo()->GetDamage()) * uint64(std::max(blades->GetAmount(), 0)) *
            uint64(std::clamp(effect->GetAmount(), 0, 100)) / 10000;
        if (amount && amount <= uint64(std::numeric_limits<int32>::max()))
            GetTarget()->CastCustomSpell(SPELL_PILFERING_HEAL, SPELLVALUE_BASE_POINT0,
                int32(amount), GetTarget(), true);
    }

    void Register() override
    {
        DoCheckProc += AuraCheckProcFn(aura_ascension_ranger_pilfering::CheckProc);
        OnEffectProc += AuraEffectProcFn(aura_ascension_ranger_pilfering::Heal, EFFECT_1, AuraType(354));
    }
};

class aura_ascension_ranger_rapid_strikes : public AuraScript
{
    PrepareAuraScript(aura_ascension_ranger_rapid_strikes);

    bool Validate(SpellInfo const* spellInfo) override
    {
        SpellEffectInfo const& strike = spellInfo->Effects[EFFECT_0];
        return spellInfo->Id == SPELL_RAPID_STRIKES && spellInfo->SpellFamilyName == 27 &&
            strike.IsAura(AuraType(354)) && strike.TriggerSpell == SPELL_RAPID_STRIKE &&
            ValidateSpellInfo({ SPELL_RAPID_STRIKE });
    }

    bool Load() override
    {
        Unit* ranger = GetUnitOwner();
        return ranger && ranger->IsPlayer() && ranger->ToPlayer()->getClass() == CLASS_RANGER;
    }

    bool CheckProc(ProcEventInfo& event)
    {
        Unit* recipient = GetTarget();
        DamageInfo const* damage = event.GetDamageInfo();
        Unit* victim = event.GetActionTarget();
        return recipient && recipient->IsAlive() && event.GetActor() == recipient && victim &&
            victim != recipient && !recipient->IsFriendlyTo(victim) && damage && damage->GetDamage() &&
            (damage->GetDamageType() == DIRECT_DAMAGE || damage->GetDamageType() == SPELL_DIRECT_DAMAGE);
    }

    void Strike(AuraEffect const* effect, ProcEventInfo& event)
    {
        PreventDefaultAction();
        Unit* victim = event.GetActionTarget();
        uint64 amount = uint64(event.GetDamageInfo()->GetDamage()) *
            uint64(std::clamp(effect->GetAmount(), 0, 100)) / 100;
        if (victim && amount && amount <= uint64(std::numeric_limits<int32>::max()))
            GetTarget()->CastCustomSpell(SPELL_RAPID_STRIKE, SPELLVALUE_BASE_POINT0,
                int32(amount), victim, true);
    }

    void Register() override
    {
        DoCheckProc += AuraCheckProcFn(aura_ascension_ranger_rapid_strikes::CheckProc);
        OnEffectProc += AuraEffectProcFn(aura_ascension_ranger_rapid_strikes::Strike, EFFECT_0, AuraType(354));
    }
};

class aura_ascension_ranger_guidance : public AuraScript
{
    PrepareAuraScript(aura_ascension_ranger_guidance);

    bool Validate(SpellInfo const* spellInfo) override
    {
        SpellEffectInfo const& damage = spellInfo->Effects[EFFECT_0];
        return spellInfo->Id == SPELL_GUIDANCE && spellInfo->IsPassive() &&
            damage.IsAura(SPELL_AURA_MOD_DAMAGE_PERCENT_DONE) && damage.DieSides == 1 &&
            ValidateSpellInfo({SPELL_WAR_FALCON_PRESENCE, SPELL_DRAGONHAWK_PRESENCE});
    }

    void Calculate(AuraEffect const*, int32& amount, bool& canBeRecalculated)
    {
        canBeRecalculated = true;
        amount = 0;
        Unit* owner = GetUnitOwner();
        if (!owner)
            return;

        ForEachPresentCompanion(owner, [&amount](SpellInfo const*) { ++amount; });
    }

    void Period(AuraEffect const*, bool& isPeriodic, int32& timer)
    {
        isPeriodic = true;
        timer = WINGMAN_REFRESH_MS;
    }

    void Refresh(AuraEffect const* effect)
    {
        PreventDefaultAction();
        GetAura()->GetEffect(effect->GetEffIndex())->RecalculateAmount();
    }

    void Register() override
    {
        DoEffectCalcAmount += AuraEffectCalcAmountFn(aura_ascension_ranger_guidance::Calculate,
            EFFECT_0, SPELL_AURA_MOD_DAMAGE_PERCENT_DONE);
        DoEffectCalcPeriodic += AuraEffectCalcPeriodicFn(aura_ascension_ranger_guidance::Period,
            EFFECT_0, SPELL_AURA_MOD_DAMAGE_PERCENT_DONE);
        OnEffectPeriodic += AuraEffectPeriodicFn(aura_ascension_ranger_guidance::Refresh,
            EFFECT_0, SPELL_AURA_MOD_DAMAGE_PERCENT_DONE);
    }
};

class ranger_swiftshot_hits : public AllSpellScript
{
public:
    ranger_swiftshot_hits() : AllSpellScript("ranger_swiftshot_hits", {ALLSPELLHOOK_ON_HIT_RESULT}) { }

    void OnSpellHitResult(Spell* spell, Unit* target, uint8 miss, uint32 damage, uint32, bool) override
    {
        Player* player = spell->GetCaster()->ToPlayer();
        if (!player || miss != SPELL_MISS_NONE || !damage || !target || target == player || !target->IsAlive() ||
            sSpellMgr->GetFirstSpellInChain(spell->GetSpellInfo()->Id) != CHAIN_PRECISION_SHOT ||
            !player->HasAura(SPELL_SWIFTSHOT))
            return;
        player->CastSpell(target, SPELL_SWIFTSHOT_VULNERABILITY, true);
    }
};

class aura_ascension_ranger_marked_for_death : public AuraScript
{
    PrepareAuraScript(aura_ascension_ranger_marked_for_death);

    bool Validate(SpellInfo const* spellInfo) override
    {
        SpellEffectInfo const& trigger = spellInfo->Effects[EFFECT_0];
        return spellInfo->Id == SPELL_MARKED_FOR_DEATH && spellInfo->SpellFamilyName == 27 &&
            trigger.IsAura(SPELL_AURA_PROC_TRIGGER_SPELL) &&
            trigger.TriggerSpell == SPELL_MARKED_FOR_DEATH_BUFF &&
            ValidateSpellInfo({ SPELL_MARKED_FOR_DEATH_BUFF });
    }

    bool Load() override
    {
        Unit* ranger = GetUnitOwner();
        return ranger && ranger->IsPlayer() && ranger->ToPlayer()->getClass() == CLASS_RANGER;
    }

    static bool HasDaggers(Player const* ranger)
    {
        for (WeaponAttackType attack : { BASE_ATTACK, OFF_ATTACK })
            if (Item const* weapon = ranger->GetWeaponForAttack(attack, true))
                if (weapon->GetTemplate()->SubClass == ITEM_SUBCLASS_WEAPON_DAGGER)
                    return true;
        return false;
    }

    bool CheckProc(ProcEventInfo& eventInfo)
    {
        Player const* ranger = GetTarget()->ToPlayer();
        return ranger && eventInfo.GetActor() == GetTarget() && HasDaggers(ranger);
    }

    void Register() override
    {
        DoCheckProc += AuraCheckProcFn(aura_ascension_ranger_marked_for_death::CheckProc);
    }
};

class ranger_deepwood_exploit : public AllSpellScript
{
public:
    ranger_deepwood_exploit() : AllSpellScript("ranger_deepwood_exploit", {ALLSPELLHOOK_ON_HIT_RESULT}) { }

    void OnSpellHitResult(Spell* spell, Unit* target, uint8 miss, uint32 damage, uint32, bool) override
    {
        Player* player = spell->GetCaster() ? spell->GetCaster()->ToPlayer() : nullptr;
        SpellInfo const* info = spell->GetSpellInfo();
        // Deepwood Poison (800079): "Flank and Exploit apply Deepwood Poison." The Flank
        // half already works through the spell_proc row in
        // rev_20260921_40_ranger_dead_procs.sql; Exploit 520570 carries empty
        // SpellFamilyFlags (0,0,0), so no family-masked row can ever match it
        // (SpellInfo.cpp:1440) and the strike is paid here, gated on the player
        // holding the talent. Flank is deliberately excluded: its own row applies
        // the same poison and would double-apply.
        if (!player || player->getClass() != CLASS_RANGER || !target || target == player ||
            !target->IsAlive() || miss != SPELL_MISS_NONE || !damage ||
            !player->IsValidAttackTarget(target) || info->SpellFamilyName != 27 ||
            sSpellMgr->GetFirstSpellInChain(info->Id) != SPELL_RANGER_EXPLOIT ||
            !player->HasAura(SPELL_DEEPWOOD_POISON))
            return;
        player->CastSpell(target, SPELL_DEEPWOOD_POISON_DOT, true);
    }
};
}

void HandleAscensionRangerStonemason(Spell* spell, Player* player)
{
    SpellInfo const* info = spell->GetSpellInfo();
    if (player->getClass() != CLASS_RANGER || info->SpellFamilyName != 27 ||
        !IsSkullpiercerOrAssault(info->Id) || !player->HasAura(SPELL_STONEMASONS_SECRET) ||
        !player->HasAura(SPELL_DIRTY_BLADES, player->GetGUID()))
        return;
    if (Aura const* advantage = player->GetAura(SPELL_ADVANTAGE); advantage && advantage->GetStackAmount() == 5)
        player->CastSpell(player, SPELL_EXTEND_DIRTY_BLADES, true);

    // 'Tactical' Advantage marks the victim of a Knockout or Bushwhack.
    if (player->HasAura(SPELL_TACTICAL_ADVANTAGE) &&
        (info->Id == SPELL_KNOCKOUT_INCAPACITATE || info->Id == SPELL_BUSHWHACK))
        player->CastSpell(spell->m_targets.GetUnitTarget(), SPELL_TACTICAL_ADVANTAGE_DEBUFF, true);
}

void HandleAscensionRangerPhoenixPlumes(Spell* spell, Player* player)
{
    uint32 chain = sSpellMgr->GetFirstSpellInChain(spell->GetSpellInfo()->Id);
    if ((chain != CHAIN_SKULLPIERCER && chain != CHAIN_WOODLAND_ARROW) || !player->HasAura(SPELL_PHOENIX_PLUMES) ||
        !HasFullAdvantage(player))
        return;
    if (Unit* target = spell->m_targets.GetUnitTarget())
        player->CastSpell(target, SPELL_PHOENIX_PLUMES_WAR_FALCON, true);
}

void ApplyAscensionRangerTalentContracts(SpellInfo* info)
{
    if (info->Id == SPELL_KNOCKOUT_INCAPACITATE && info->SpellFamilyName == 27)
        info->AuraInterruptFlags |= AURA_INTERRUPT_FLAG_TAKE_DAMAGE;
    if (info->Id == SPELL_SNATCH_DISARM && info->SpellFamilyName == 27)
        if (SpellInfo const* parent = sSpellMgr->GetSpellInfo(SPELL_SNATCH))
            info->DurationEntry = parent->DurationEntry;
}

class ranger_pierced_crits : public AllSpellScript
{
public:
    ranger_pierced_crits() : AllSpellScript("ranger_pierced_crits", {ALLSPELLHOOK_ON_HIT_RESULT}) { }

    void OnSpellHitResult(Spell* spell, Unit* target, uint8 miss, uint32 damage, uint32, bool critical) override
    {
        Player* player = spell->GetCaster() ? spell->GetCaster()->ToPlayer() : nullptr;
        SpellInfo const* info = spell->GetSpellInfo();
        if (!player || player->getClass() != CLASS_RANGER || !target || miss != SPELL_MISS_NONE ||
            !critical || !damage || info->SpellFamilyName != 27 || !player->HasAura(SPELL_PIERCED))
            return;
        // Pierced (705033): critical strikes with Skullpiercer and Precision
        // Shot bleed the enemy for 15% of the damage dealt plus 35% over 4
        // sec. The DBC's aura has no engine handler, so both halves are paid
        // here where the crit lands; the over-time half reuses the 4-second
        // Venom Blade vehicle with a scaled amount.
        uint32 root = sSpellMgr->GetFirstSpellInChain(info->Id);
        if (root != 501715 && root != 500075)
            return;
        int32 direct = int32(CalculatePct(damage, 15));
        int32 tick = int32(CalculatePct(damage, 35) / 4);
        player->CastCustomSpell(target, 803116, &direct, &tick, nullptr, true);
    }
};

class ranger_deadly_accurate_consume : public AllSpellScript
{
public:
    ranger_deadly_accurate_consume() : AllSpellScript("ranger_deadly_accurate_consume", {ALLSPELLHOOK_ON_HIT_RESULT}) { }

    void OnSpellHitResult(Spell* spell, Unit* target, uint8 miss, uint32 damage, uint32, bool) override
    {
        Player* player = spell->GetCaster() ? spell->GetCaster()->ToPlayer() : nullptr;
        SpellInfo const* info = spell->GetSpellInfo();
        if (!player || player->getClass() != CLASS_RANGER || !target || miss != SPELL_MISS_NONE || !damage ||
            info->SpellFamilyName != 27 || !player->HasAura(SPELL_DEADLY_ACCURATE))
            return;
        if (sSpellMgr->GetFirstSpellInChain(info->Id) != 501715)
            return;
        player->RemoveAurasDueToSpell(SPELL_DEADLY_ACCURATE);
    }
};

void AddSC_AscensionRangerTalents()
{
    new ranger_pierced_crits();
    new ranger_deadly_accurate_consume();
    new ranger_deepwood_exploit();
    RegisterSpellScript(spell_ascension_ranger_light_arrows);
    RegisterSpellScript(spell_ascension_ranger_knockout);
    RegisterSpellScript(aura_ascension_ranger_wingman);
    RegisterSpellScript(aura_ascension_ranger_highwayman);
    RegisterSpellScript(spell_ascension_ranger_frenzy);
    RegisterSpellScript(aura_ascension_ranger_pilfering);
    RegisterSpellScript(aura_ascension_ranger_rapid_strikes);
    RegisterSpellScript(aura_ascension_ranger_guidance);
    RegisterSpellScript(aura_ascension_ranger_marked_for_death);
    new ranger_wingman_companions();
    new ranger_swiftshot_hits();
}
