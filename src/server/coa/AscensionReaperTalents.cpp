/* Copyright (C) 2016+ AzerothCore, GNU AGPL v3. */
#include "AscensionReaperTalents.h"
#include "CellImpl.h"
#include "GridNotifiersImpl.h"
#include "ObjectAccessor.h"
#include "Random.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SpellAuraEffects.h"
#include "SpellAuras.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "SpellScript.h"
#include <algorithm>
#include <limits>
#include <list>

namespace
{
enum ReaperTalentSpells : uint32
{
    SPELL_HARVESTER_AMOUNT = 500283,
    SPELL_BLOOD_HARVEST = 504565,
    SPELL_UNDERWALK = 800797,
    SPELL_BEYOND_THE_VEIL = 804053,
    SPELL_BEYOND_THE_VEIL_BUFF = 560591,
    SPELL_FROM_THE_SHADOWS = 561099,
    SPELL_FROM_THE_SHADOWS_CRIT = 561128,
    SPELL_REAPED_SOUL = 500363,
    SPELL_SOUL_CAPTURED = 572887,
    SPELL_SPECTRAL_WARDEN = 805716,
    SPELL_SOUL_SPLINTERS = 805719,
    SPELL_SOUL_SPLINTER = 805720,
    SPELL_LIMBO = 800845,
    SPELL_LIMBO_SHELL = 805872,
    SPELL_JAILERS_WILL = 524939,
    SPELL_SOUL_STRIKE_FIRST = 500517,
    SPELL_SOUL_STRIKE_FIFTH = 500521,
    SPELL_SOUL_STRIKE_SIXTH = 500646,
    SPELL_PAINBRINGER = 680995,
    SPELL_PAINBRINGER_APPLY = 520533,
    SPELL_PAINBRINGER_EXTEND = 520877,
    SPELL_MASOCHISTIC_RAGE = 570097,
    SPELL_BLOOD_FRENZY_TALENT = 707899,
    SPELL_BLOOD_FRENZY = 803039,
    SPELL_HARVEST_TIME_LOW = 704188,
    SPELL_HARVEST_TIME = 803995,
    SPELL_SOUL_HARVEST_TALENT = 504012,
    SPELL_SOUL_HARVEST = 573050,
    SPELL_SPIRIT_CULLING = 301986,
    SPELL_SPECTRAL_SCYTHE = 500576,
    SPELL_FATESEALER = 705442,
    SPELL_FATESEALER_PROTECTION = 705443,
    SPELL_EATER_OF_SOULS = 805181,
    SPELL_EATER_OF_SOULS_PROTECTION = 805182,
    SPELL_DAMNED = 706786,
    SPELL_DAMNED_HASTE = 560420,
    SPELL_PURGATORY = 504046,
    SPELL_PURGATORY_DAMAGE = 504047,
    SPELL_ESSENCE_INVIGORATION = 805186,
    SPELL_ESSENCE_INVIGORATION_HEAL = 805187,
    SPELL_WEAKENED_SOULS = 92146,
    SPELL_WEAKENED_SOUL = 803433
};

class spell_ascension_reaper_limbo : public SpellScript
{
    PrepareSpellScript(spell_ascension_reaper_limbo);

    void GrantShell()
    {
        if (Unit* caster = GetCaster())
            caster->CastSpell(caster, SPELL_LIMBO_SHELL, true);
    }

    void Register() override
    {
        AfterCast += SpellCastFn(spell_ascension_reaper_limbo::GrantShell);
    }
};

constexpr float BloodFrenzyRange = 20.0f;

Unit* HostileTargetInRange(Player* player, uint32 spellId)
{
    SpellInfo const* info = sSpellMgr->GetSpellInfo(spellId);
    Unit* target = player->GetSelectedUnit();
    if (!target)
        target = player->GetVictim();
    if (!info || !target || target == player || !target->IsAlive() ||
        !player->IsValidAttackTarget(target) ||
        !player->IsWithinDistInMap(target, info->GetMaxRange(false)))
        return nullptr;
    return target;
}

bool RollTalent(Player* player, uint32 talentId)
{
    SpellInfo const* talent = sSpellMgr->GetSpellInfo(talentId);
    return talent && player->HasAura(talentId) && roll_chance_i(int32(talent->ProcChance));
}

int32 PainbringerMilliseconds(uint32 spellId)
{
    SpellInfo const* info = sSpellMgr->GetSpellInfo(spellId);
    return info ? info->Effects[EFFECT_0].CalcValue() : 0;
}

void ApplyPainbringer(Player* player)
{
    if (!RollTalent(player, SPELL_PAINBRINGER))
        return;

    if (Aura* rage = player->GetAura(SPELL_MASOCHISTIC_RAGE, player->GetGUID()))
    {
        int32 const extension = PainbringerMilliseconds(SPELL_PAINBRINGER_EXTEND);
        if (extension <= 0)
            return;

        rage->SetMaxDuration(rage->GetMaxDuration() + extension);
        rage->SetDuration(rage->GetDuration() + extension);
        return;
    }

    player->CastSpell(player, SPELL_MASOCHISTIC_RAGE, true);
    int32 const duration = PainbringerMilliseconds(SPELL_PAINBRINGER_APPLY);
    if (duration <= 0)
        return;

    if (Aura* rage = player->GetAura(SPELL_MASOCHISTIC_RAGE, player->GetGUID()))
    {
        rage->SetMaxDuration(duration);
        rage->SetDuration(duration);
    }
}

void ApplySoulHarvest(Player* player)
{
    if (!RollTalent(player, SPELL_SOUL_HARVEST_TALENT))
        return;

    if (Unit* target = HostileTargetInRange(player, SPELL_SOUL_HARVEST))
        player->CastSpell(target, SPELL_SOUL_HARVEST, true);
}

void ApplySpiritCulling(Player* player)
{
    if (!RollTalent(player, SPELL_SPIRIT_CULLING))
        return;

    player->CastSpell(player, SPELL_SPECTRAL_SCYTHE, true);
}

void ApplyFatesealer(Player* player, uint8 gainedSouls)
{
    if (!player->HasAura(SPELL_FATESEALER))
        return;

    for (uint8 soul = 0; soul < gainedSouls; ++soul)
        player->CastSpell(player, SPELL_FATESEALER_PROTECTION, true);
}

void ApplyEaterOfSouls(Player* player, uint8 soulStacks)
{
    if (soulStacks == 3 && player->HasAura(SPELL_EATER_OF_SOULS) &&
        !player->HasAura(SPELL_EATER_OF_SOULS_PROTECTION))
        player->CastSpell(player, SPELL_EATER_OF_SOULS_PROTECTION, true);
}

void ApplyHarvestedSoulTalents(Player* player, uint8 soulStacks, uint8 gainedSouls)
{
    ApplyPainbringer(player);
    ApplySoulHarvest(player);
    ApplySpiritCulling(player);
    ApplyFatesealer(player, gainedSouls);
    ApplyEaterOfSouls(player, soulStacks);
}

void CastTalentTrigger(Player* player, uint32 talentId, uint32 triggerId)
{
    if (RollTalent(player, talentId))
        player->CastSpell(player, triggerId, true);
}

class spell_ascension_reaper_essence_invigoration_heal : public SpellScript
{
    PrepareSpellScript(spell_ascension_reaper_essence_invigoration_heal);

    void Heal(SpellEffIndex)
    {
        Unit* target = GetHitUnit();
        if (!target)
            return;

        uint64 missing = target->GetMaxHealth() - std::min(target->GetHealth(), target->GetMaxHealth());
        uint64 amount = missing * std::clamp(GetEffectValue(), 0, 100) / 100;
        SetEffectValue(int32(std::min<uint64>(amount, std::numeric_limits<int32>::max())));
    }

    void Register() override
    {
        OnEffectLaunchTarget += SpellEffectFn(spell_ascension_reaper_essence_invigoration_heal::Heal,
            EFFECT_0, SPELL_EFFECT_HEAL);
    }
};

class reaper_essence_invigoration_metadata : public GlobalScript
{
public:
    reaper_essence_invigoration_metadata() : GlobalScript("reaper_essence_invigoration_metadata",
        {GLOBALHOOK_ON_LOAD_SPELL_CUSTOM_ATTR}) { }

    void OnLoadSpellCustomAttr(SpellInfo* info) override
    {
        if (info->Id == SPELL_ESSENCE_INVIGORATION_HEAL && info->SpellFamilyName == 36 &&
            info->Effects[EFFECT_0].Effect == SPELL_EFFECT_HEAL_PCT && info->Effects[EFFECT_0].MiscValueB == 1)
            info->Effects[EFFECT_0].Effect = SPELL_EFFECT_HEAL;
    }
};

class spell_ascension_soul_capture : public SpellScript
{
    PrepareSpellScript(spell_ascension_soul_capture);
    ObjectGuid _corpse;

    bool Validate(SpellInfo const*) override
    {
        return ValidateSpellInfo({SPELL_SOUL_CAPTURED, SPELL_REAPED_SOUL});
    }

    bool Eligible(Unit* unit)
    {
        Unit* caster = GetCaster();
        return unit && unit != caster && !unit->IsAlive() && !caster->IsFriendlyTo(unit) &&
            !unit->HasAura(SPELL_SOUL_CAPTURED) && caster->IsInMap(unit) && caster->InSamePhase(unit) &&
            caster->IsWithinDistInMap(unit, GetSpellInfo()->Effects[EFFECT_0].CalcRadius(caster)) &&
            caster->IsWithinLOSInMap(unit);
    }

    SpellCastResult CheckCorpse()
    {
        _corpse.Clear();
        Unit* caster = GetCaster();
        std::list<Unit*> corpses;
        Acore::AnyDeadUnitCheck check;
        Acore::UnitListSearcher<Acore::AnyDeadUnitCheck> searcher(caster, corpses, check);
        Cell::VisitObjects(caster, searcher, GetSpellInfo()->Effects[EFFECT_0].CalcRadius(caster));
        for (Unit* corpse : corpses)
            if (Eligible(corpse))
            {
                _corpse = corpse->GetGUID();
                return SPELL_CAST_OK;
            }
        return SPELL_FAILED_NO_EDIBLE_CORPSES;
    }

    void SuppressTrigger(SpellEffIndex index)
    {
        PreventHitDefaultEffect(index);
    }

    void Capture(SpellEffIndex index)
    {
        Unit* caster = GetCaster();
        Unit* corpse = ObjectAccessor::GetUnit(*caster, _corpse);
        if (!Eligible(corpse) || !caster->AddAura(SPELL_SOUL_CAPTURED, corpse))
        {
            PreventHitDefaultEffect(index);
            return;
        }
        caster->CastSpell(caster, SPELL_REAPED_SOUL, true);
    }

    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_ascension_soul_capture::CheckCorpse);
        OnEffectLaunch += SpellEffectFn(spell_ascension_soul_capture::SuppressTrigger,
            EFFECT_ALL, SPELL_EFFECT_TRIGGER_SPELL);
        OnEffectLaunchTarget += SpellEffectFn(spell_ascension_soul_capture::SuppressTrigger,
            EFFECT_ALL, SPELL_EFFECT_TRIGGER_SPELL);
        OnEffectHitTarget += SpellEffectFn(spell_ascension_soul_capture::Capture, EFFECT_2, SPELL_EFFECT_HEAL_PCT);
    }
};

class aura_ascension_harvester : public AuraScript
{
    PrepareAuraScript(aura_ascension_harvester);

    bool Validate(SpellInfo const*) override
    {
        return ValidateSpellInfo({SPELL_HARVESTER_AMOUNT, SPELL_BLOOD_HARVEST});
    }

    bool CheckProc(ProcEventInfo& event)
    {
        Unit* owner = GetTarget();
        Unit* victim = event.GetActionTarget();
        return owner->IsPlayer() && owner->getClass() == CLASS_REAPER && owner->IsAlive() &&
            event.GetActor() == owner && victim && victim != owner && !owner->IsFriendlyTo(victim) &&
            event.GetDamageInfo() && event.GetDamageInfo()->GetDamage();
    }

    void Heal(AuraEffect const*, ProcEventInfo& event)
    {
        PreventDefaultAction();
        Unit* owner = GetTarget();
        int32 percent = sSpellMgr->GetSpellInfo(SPELL_HARVESTER_AMOUNT)->Effects[EFFECT_0].CalcValue(owner);
        uint64 amount = uint64(event.GetDamageInfo()->GetDamage()) * std::clamp(percent, 0, 100) / 100;
        if (amount)
            owner->CastCustomSpell(SPELL_BLOOD_HARVEST, SPELLVALUE_BASE_POINT0,
                int32(std::min<uint64>(amount, std::numeric_limits<int32>::max())), owner, TRIGGERED_FULL_MASK);
    }

    void Register() override
    {
        DoCheckProc += AuraCheckProcFn(aura_ascension_harvester::CheckProc);
        OnEffectProc += AuraEffectProcFn(aura_ascension_harvester::Heal, EFFECT_0, SPELL_AURA_DUMMY);
    }
};

class aura_ascension_reaper_ghastly_form : public AuraScript
{
    PrepareAuraScript(aura_ascension_reaper_ghastly_form);

    static constexpr float AttackPowerCoefficient = 0.2f;

    bool Load() override { return GetUnitOwner() && GetUnitOwner()->IsPlayer(); }

    void CalculateAmount(AuraEffect const*, int32& amount, bool& canBeRecalculated)
    {
        if (Unit* owner = GetUnitOwner())
            amount += int32(owner->GetTotalAttackPowerValue(BASE_ATTACK) * AttackPowerCoefficient);

        canBeRecalculated = false;
    }

    void Register() override
    {
        DoEffectCalcAmount += AuraEffectCalcAmountFn(
            aura_ascension_reaper_ghastly_form::CalculateAmount, EFFECT_0,
            SPELL_AURA_SCHOOL_ABSORB);
    }
};

class aura_ascension_jailers_call : public AuraScript
{
    PrepareAuraScript(aura_ascension_jailers_call);

    bool CheckProc(ProcEventInfo& event)
    {
        Unit* victim = event.GetActionTarget();
        return victim && victim != GetTarget() && victim->IsAlive() && victim->HealthBelowPct(20);
    }

    void Register() override
    {
        DoCheckProc += AuraCheckProcFn(aura_ascension_jailers_call::CheckProc);
    }
};

class aura_ascension_reaper_blood_frenzy : public AuraScript
{
    PrepareAuraScript(aura_ascension_reaper_blood_frenzy);

    Unit* FrenzyTarget() const
    {
        Player* player = GetTarget()->ToPlayer();
        if (!player || !player->IsAlive())
            return nullptr;
        return HostileTargetInRange(player, SPELL_BLOOD_FRENZY);
    }

    bool CheckProc(ProcEventInfo& event)
    {
        SpellInfo const* spell = event.GetSpellInfo();
        return spell && (spell->Id == SPELL_HARVEST_TIME || spell->Id == SPELL_HARVEST_TIME_LOW) &&
            event.GetActor() == GetTarget() && FrenzyTarget() != nullptr;
    }

    void Proc(AuraEffect const* effect, ProcEventInfo&)
    {
        PreventDefaultAction();
        if (Unit* target = FrenzyTarget())
            GetTarget()->CastSpell(target, SPELL_BLOOD_FRENZY, true, nullptr, effect);
    }

    void Register() override
    {
        DoCheckProc += AuraCheckProcFn(aura_ascension_reaper_blood_frenzy::CheckProc);
        OnEffectProc += AuraEffectProcFn(aura_ascension_reaper_blood_frenzy::Proc, EFFECT_0,
            SPELL_AURA_PROC_TRIGGER_SPELL);
    }
};

constexpr uint32 SPELL_ANIMA_AMBUSHER = 705424;
constexpr uint32 SPELL_ANIMA_AMBUSH = 705425;
constexpr uint32 SPELL_SPECTRE_STRIDE_HIT = 803742;
constexpr uint32 SPELL_LAMENTING = 705397;
constexpr uint32 SPELL_GHOSTLY_WEAPON_HIT = 804474;
constexpr uint32 SPELL_GHOSTLY_WEAPON_HEAL = 807420;

class reaper_talent_casts : public AllSpellScript
{
public:
    reaper_talent_casts() : AllSpellScript("reaper_talent_casts",
        {ALLSPELLHOOK_ON_HIT_RESULT}) { }

    void OnSpellHitResult(Spell* spell, Unit* target, uint8 miss, uint32 damage, uint32, bool) override
    {
        Player* player = spell->GetCaster() ? spell->GetCaster()->ToPlayer() : nullptr;
        if (!player || player->getClass() != CLASS_REAPER || !target || miss != SPELL_MISS_NONE || !damage)
            return;

        uint32 const spellId = spell->GetSpellInfo()->Id;

        if (spellId == SPELL_SPECTRE_STRIDE_HIT && player->HasAura(SPELL_ANIMA_AMBUSHER) &&
            !spell->GetScriptValue(SPELL_ANIMA_AMBUSHER))
        {
            spell->SetScriptValue(SPELL_ANIMA_AMBUSHER, 1);
            player->CastCustomSpell(SPELL_ANIMA_AMBUSH, SPELLVALUE_BASE_POINT0,
                int32(damage * 125 / 100), target, true);
            return;
        }

        if (spellId == SPELL_GHOSTLY_WEAPON_HIT && player->HasAura(SPELL_LAMENTING) &&
            !spell->GetScriptValue(SPELL_LAMENTING))
        {
            spell->SetScriptValue(SPELL_LAMENTING, 1);
            player->CastCustomSpell(SPELL_GHOSTLY_WEAPON_HEAL, SPELLVALUE_BASE_POINT0,
                int32(damage * 50 / 100), player, true);
        }
    }
};

class spell_ascension_reaper_weakened_souls : public SpellScript
{
    PrepareSpellScript(spell_ascension_reaper_weakened_souls);

    bool Validate(SpellInfo const* info) override
    {
        return info && info->Effects[EFFECT_0].Effect == SPELL_EFFECT_SCHOOL_DAMAGE &&
            ValidateSpellInfo({SPELL_WEAKENED_SOUL});
    }

    void ApplyWeakenedSoul(SpellEffIndex)
    {
        Unit* caster = GetCaster();
        if (Unit* target = GetHitUnit(); target && caster->HasAura(SPELL_WEAKENED_SOULS))
            caster->CastSpell(target, SPELL_WEAKENED_SOUL, true);
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_ascension_reaper_weakened_souls::ApplyWeakenedSoul,
            EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
    }
};

class reaper_talent_events : public UnitScript
{
public:
    reaper_talent_events() : UnitScript("reaper_talent_events", true,
        {UNITHOOK_ON_AURA_APPLY, UNITHOOK_ON_AURA_REMOVE, UNITHOOK_MODIFY_SPELL_EFFECT_BASE_VALUE}) { }

    void OnAuraApply(Unit* unit, Aura* aura) override
    {
        Player* player = unit ? unit->ToPlayer() : nullptr;
        if (!player || player->getClass() != CLASS_REAPER || !aura ||
            aura->GetCasterGUID() != player->GetGUID() ||
            (aura->GetId() != SPELL_UNDERWALK && aura->GetId() != SPELL_BEYOND_THE_VEIL))
            return;

        if (player->HasAura(SPELL_UNDERWALK, player->GetGUID()) &&
            player->HasAura(SPELL_BEYOND_THE_VEIL, player->GetGUID()) &&
            !player->HasAura(SPELL_BEYOND_THE_VEIL_BUFF, player->GetGUID()))
            player->CastSpell(player, SPELL_BEYOND_THE_VEIL_BUFF, true);
    }

    void OnAuraRemove(Unit* unit, AuraApplication* application, AuraRemoveMode mode) override
    {
        Player* player = unit ? unit->ToPlayer() : nullptr;
        if (!player || player->getClass() != CLASS_REAPER || !application)
            return;
        Aura* aura = application->GetBase();
        if (aura->GetCasterGUID() != player->GetGUID())
            return;

        if (aura->GetId() == SPELL_UNDERWALK || aura->GetId() == SPELL_BEYOND_THE_VEIL)
            player->RemoveAurasDueToSpell(SPELL_BEYOND_THE_VEIL_BUFF, player->GetGUID());

        if (!player->IsAlive() || !player->IsInWorld() || mode == AURA_REMOVE_BY_DEATH)
            return;
        if (aura->GetId() == SPELL_UNDERWALK &&
            player->HasAura(SPELL_FROM_THE_SHADOWS))
            player->CastSpell(player, SPELL_FROM_THE_SHADOWS_CRIT, true);
    }

    void ModifySpellEffectBaseValue(Unit const* caster, SpellInfo const* info, uint8 index, float& value) override
    {
        Player const* player = caster && caster->IsPlayer() ? caster->ToPlayer() : nullptr;
        if (!player || player->getClass() != CLASS_REAPER || !info || info->SpellFamilyName != 36)
            return;
        if (info->Id == SPELL_SPECTRAL_WARDEN && index == EFFECT_1 &&
            info->Effects[index].IsAura(SPELL_AURA_SCHOOL_ABSORB))
            value += player->GetStat(STAT_STAMINA) * 1.5f;
        else if (info->Id == SPELL_SOUL_SPLINTER && index == EFFECT_0 &&
            info->Effects[index].IsAura(SPELL_AURA_PERIODIC_DAMAGE))
            value += std::max(0.0f, player->GetStat(STAT_STAMINA)) * 0.035f;
        // Jailer's Will (524939): "Soul Strike now deals 30% Strength increased
        // damage, scaling with Strength." The client splits this into a 2.5 s
        // flat-modifier scaling aura (578264) with no usable engine mod; feed
        // the Strength term straight into Soul Strike's normalized weapon
        // damage base, mirroring the Soul Splinter Stamina contract above.
        else if (player->HasAura(SPELL_JAILERS_WILL) && index == EFFECT_1 &&
            info->Effects[index].Effect == SPELL_EFFECT_NORMALIZED_WEAPON_DMG &&
            info->SpellFamilyFlags == flag96(0, 2048, 0) &&
            ((info->Id >= SPELL_SOUL_STRIKE_FIRST && info->Id <= SPELL_SOUL_STRIKE_FIFTH) ||
                info->Id == SPELL_SOUL_STRIKE_SIXTH))
            value += std::max(0.0f, player->GetStat(STAT_STRENGTH)) * 0.3f;
    }
};
}

bool HandleAscensionReaperResource(Player* player, uint32 spellId, int32 amount)
{
    if (player->getClass() != CLASS_REAPER || spellId != SPELL_REAPED_SOUL)
        return false;
    Aura* aura = player->GetAura(spellId, player->GetGUID());
    uint8 previous = aura ? aura->GetStackAmount() : 0;
    if (aura)
        aura->ModStackAmount(amount);
    else if (amount > 0)
        if (Aura* created = player->AddAura(spellId, player); created && amount > 1)
            created->ModStackAmount(amount - 1);
    aura = player->GetAura(spellId, player->GetGUID());
    if (aura && aura->GetStackAmount() > previous && player->IsAlive())
    {
        uint8 const soulStacks = aura->GetStackAmount();
        if (player->HasAura(SPELL_SOUL_SPLINTERS))
            player->CastSpell(player, SPELL_SOUL_SPLINTER, true);
        ApplyHarvestedSoulTalents(player, soulStacks, soulStacks - previous);
    }
    return true;
}

void ApplyAscensionReaperSoulInfusionGained(Player* player)
{
    if (!player || player->getClass() != CLASS_REAPER || !player->IsAlive())
        return;

    CastTalentTrigger(player, SPELL_DAMNED, SPELL_DAMNED_HASTE);
    CastTalentTrigger(player, SPELL_PURGATORY, SPELL_PURGATORY_DAMAGE);
}

void ApplyAscensionReaperSoulInfusionSpent(Player* player)
{
    if (!player || player->getClass() != CLASS_REAPER || !player->IsAlive())
        return;

    CastTalentTrigger(player, SPELL_ESSENCE_INVIGORATION, SPELL_ESSENCE_INVIGORATION_HEAL);
}

constexpr uint32 SPELL_HARD_BARGAIN_TALENT = 300569;
constexpr uint32 SPELL_HARD_BARGAIN = 572300;
constexpr uint32 SPELL_TORMENTED_SOULS = 500481;

class aura_ascension_reaper_hard_bargain : public AuraScript
{
    PrepareAuraScript(aura_ascension_reaper_hard_bargain);

    bool Validate(SpellInfo const*) override
    {
        return ValidateSpellInfo({SPELL_HARD_BARGAIN, SPELL_TORMENTED_SOULS});
    }

    void ApplyHardBargain(AuraEffect const*, ProcEventInfo&)
    {
        Unit* owner = GetTarget();
        if (owner->IsAlive() && owner->HasAura(SPELL_TORMENTED_SOULS))
            owner->CastSpell(owner, SPELL_HARD_BARGAIN, true);
    }

    void Register() override
    {
        OnEffectProc += AuraEffectProcFn(aura_ascension_reaper_hard_bargain::ApplyHardBargain, EFFECT_0, SPELL_AURA_PROC_TRIGGER_SPELL);
    }
};

constexpr uint32 SPELL_WAKE_UP_ITS_DREAD_TIME = 705429;
constexpr uint32 SPELL_DREAD_TIME = 572180;

class aura_ascension_reaper_wake_up : public AuraScript
{
    PrepareAuraScript(aura_ascension_reaper_wake_up);

    bool Validate(SpellInfo const*) override
    {
        return ValidateSpellInfo({SPELL_DREAD_TIME});
    }

    void GrantDreadTime(AuraEffect const*, ProcEventInfo&)
    {
        Unit* owner = GetTarget();
        if (owner->IsAlive())
            owner->CastSpell(owner, SPELL_DREAD_TIME, true);
    }

    void Register() override
    {
        OnEffectProc += AuraEffectProcFn(aura_ascension_reaper_wake_up::GrantDreadTime, EFFECT_0, SPELL_AURA_PROC_TRIGGER_SPELL);
    }
};

class aura_ascension_counterscythe : public AuraScript
{
    PrepareAuraScript(aura_ascension_counterscythe);

    void CountParry(AuraEffect const*, ProcEventInfo&)
    {
        if (++_parries >= 10)
            Remove(AURA_REMOVE_BY_DEFAULT);
    }

    void Register() override
    {
        OnEffectProc += AuraEffectProcFn(aura_ascension_counterscythe::CountParry, EFFECT_1, SPELL_AURA_PROC_TRIGGER_SPELL);
    }

private:
    uint32 _parries = 0;
};

void AddSC_AscensionReaperTalents()
{
    RegisterSpellScript(aura_ascension_counterscythe);
    RegisterSpellScript(aura_ascension_reaper_hard_bargain);
    RegisterSpellScript(aura_ascension_reaper_wake_up);
    new reaper_essence_invigoration_metadata();
    RegisterSpellScript(spell_ascension_reaper_essence_invigoration_heal);
    RegisterSpellScript(spell_ascension_soul_capture);
    RegisterSpellScript(aura_ascension_harvester);
    RegisterSpellScript(aura_ascension_jailers_call);
    RegisterSpellScript(spell_ascension_reaper_limbo);
    RegisterSpellScript(aura_ascension_reaper_blood_frenzy);
    RegisterSpellScript(aura_ascension_reaper_ghastly_form);
    new reaper_talent_casts();
    RegisterSpellScript(spell_ascension_reaper_weakened_souls);
    new reaper_talent_events();
}
