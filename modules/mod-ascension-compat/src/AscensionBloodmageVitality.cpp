/* Copyright (C) 2016+ AzerothCore, GNU AGPL v3. */
#include "AscensionPooledVitality.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "Spell.h"
#include "SpellAuraEffects.h"
#include "SpellAuras.h"
#include "SpellAuraEffects.h"
#include "SpellScript.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace
{
using namespace AscensionBloodmage;
constexpr uint32 VitalityCost = 10;
constexpr uint32 RagingHunger = 681336;
constexpr uint32 OrganOrbs = 807490;
constexpr uint32 SPELL_BLOOD_CRAVING_PAYOUT = 805985;
constexpr uint32 SPELL_FORBIDDEN_POWER_MIRROR = 500447;

bool IsBloodmage(Player const* player)
{
    return player && player->getClass() == CLASS_SON_OF_ARUGAL && player->IsAlive() && player->IsInWorld();
}

bool CanEmpower(Player* player)
{
    if (!IsBloodmage(player) || !player->HasAura(PooledVitalityTalent) || player->HasAura(CursedFormCheck) ||
        player->HasAura(CursedForm))
        return false;
    Aura const* pool = player->GetAura(PooledVitality, player->GetGUID());
    return pool && uint32(pool->GetStackAmount()) >= VitalityCost;
}

class bloodmage_vitality_casts : public AllSpellScript
{
public:
    bloodmage_vitality_casts() : AllSpellScript("bloodmage_vitality_casts",
        {ALLSPELLHOOK_CAN_PREPARE, ALLSPELLHOOK_ON_SPELL_CHECK_CAST, ALLSPELLHOOK_ON_BEFORE_EFFECTS,
            ALLSPELLHOOK_ON_CAST, ALLSPELLHOOK_ON_HIT_RESULT}) { }

    bool CanPrepare(Spell* spell, SpellCastTargets const*, AuraEffect const*) override
    {
        Player* player = spell->GetCaster()->ToPlayer();
        SpellInfo const* info = spell->GetSpellInfo();
        if (!spell->IsTriggered() && info->SpellFamilyName == 26 && info->PowerType == POWER_RAGE &&
            (info->ManaCost || info->ManaCostPercentage) && CanEmpower(player))
            spell->SetScriptValue(PooledVitalityTalent, GetEmpowerment(info->Id));
        return true;
    }

    void OnSpellCheckCast(Spell* spell, bool, SpellCastResult& result) override
    {
        // A cast prepared with an instant/free benefit cannot finish with stacks
        // that expired or were spent by a different cast during its cast time.
        if (spell->GetScriptValue(PooledVitalityTalent) && !CanEmpower(spell->GetCaster()->ToPlayer()))
            result = SPELL_FAILED_CASTER_AURASTATE;
    }

    void OnSpellBeforeEffects(Spell* spell, Unit* caster, SpellInfo const*) override
    {
        Player* player = caster->ToPlayer();
        if (!spell->GetScriptValue(PooledVitalityTalent) || spell->GetScriptValue(PooledVitality) ||
            !CanEmpower(player))
            return;
        spell->SetScriptValue(PooledVitality, 1);
        player->GetAura(PooledVitality, player->GetGUID())->ModStackAmount(-int32(VitalityCost));
    }

    void OnSpellCast(Spell* spell, Unit* caster, SpellInfo const* info, bool) override
    {
        Player* player = caster->ToPlayer();
        if (!IsBloodmage(player) || spell->IsTriggered() || info->SpellFamilyName != 26 ||
            !spell->GetScriptValue(PooledVitality) || spell->GetScriptValue(VitalityForLater))
            return;
        spell->SetScriptValue(VitalityForLater, 1);
        if (spell->GetScriptValue(PooledVitalityTalent))
        {
            if (player->HasAura(VitalityForLater))
                player->CastSpell(player, VitalityHeal, true);
        }
        else if (info->PowerType == POWER_HEALTH && player->HasAura(PooledVitalityTalent))
        {
            player->CastSpell(player, PooledVitality, true);
            // Raging Hunger (681336): triggering Pooled Vitality also pays out
            // three Rage.
            if (player->HasAura(RagingHunger))
                player->ModifyPower(POWER_RAGE, 30);
            // Organ Orbs (807490): every Pooled Vitality stack raises Spirit by
            // three percent, refreshed through the passive's own stat aura.
            if (Aura* orbs = player->GetAura(OrganOrbs))
                if (Aura* pool = player->GetAura(PooledVitality))
                    if (AuraEffect* spirit = orbs->GetEffect(EFFECT_0))
                    {
                        spirit->ChangeAmount(3 * int32(pool->GetStackAmount()));
                        player->UpdateAllStats();
                    }
        }
    }

    void OnSpellHitResult(Spell* spell, Unit*, uint8 miss, uint32, uint32 healing, bool) override
    {
        Player* player = spell->GetCaster()->ToPlayer();
        if (!IsBloodmage(player) || spell->IsTriggered() || miss != SPELL_MISS_NONE || !healing ||
            spell->GetScriptValue(PooledVitalityTalent) != Mend || spell->GetScriptValue(MendSelfHeal))
            return;
        spell->SetScriptValue(MendSelfHeal, 1);
        // Custom basepoints pass through native float arithmetic before returning to int32.
        uint32 maximum = uint32(std::nextafter(float(std::numeric_limits<int32>::max()), 0.0f));
        int32 amount = int32(std::min(healing / 2, maximum));
        if (amount)
            player->CastCustomSpell(player, MendSelfHeal, &amount, nullptr, nullptr, true);
    }
};

class bloodmage_vitality_scaling : public UnitScript
{
public:
    bloodmage_vitality_scaling() : UnitScript("bloodmage_vitality_scaling", true,
        {UNITHOOK_MODIFY_SPELL_EFFECT_BASE_VALUE}) { }

    void ModifySpellEffectBaseValue(Unit const* caster, SpellInfo const* info, uint8 index, float& value) override
    {
        if (!caster || !caster->IsPlayer() || caster->getClass() != CLASS_SON_OF_ARUGAL)
            return;
        if (info->Id == VitalityHeal && info->SpellFamilyName == 26 && index == EFFECT_0 &&
            info->Effects[EFFECT_0].Effect == SPELL_EFFECT_HEAL)
        {
            double amount = double(value) + caster->GetStat(STAT_SPIRIT) * 0.5;
            if (std::isfinite(amount) && amount >= 0 &&
                double(float(amount)) <= std::numeric_limits<int32>::max())
                value = float(amount);
            return;
        }
        // Issue 872: Visceral Magic adds 40% to the Armor contribution from
        // items to Blood Shield. The talent's flat-mod (aura 107, op 3 =
        // SPELLMOD_EFFECT1, mask 0x10000000) never applies natively: Blood
        // Shield (504263) is family 26 with an empty spellmod mask, so the
        // mod matches nothing. Scale the absorb's effect-0 base value here
        // when the caster owns the talent.
        if (info->Id == 504263 && index == EFFECT_0 && caster->HasAura(707371))
        {
            double amount = double(value) * 1.4;
            if (std::isfinite(amount) && amount >= 0 &&
                double(float(amount)) <= std::numeric_limits<int32>::max())
                value = float(amount);
        }
    }
};

void SetHealthPct(Unit* unit, float pct)
{
    uint32 maximum = unit->GetMaxHealth();
    double health = double(maximum) * std::clamp(double(pct), 0.0, 100.0) / 100.0;
    unit->SetHealth(std::min<uint32>(maximum, std::max<uint32>(1, uint32(health + 0.5))));
}

// Transfusion's only effect is a SPELL_EFFECT_SCRIPT_EFFECT, and Spell::EffectScriptEffect has no
// family-26 entry, so the cast spent its cost and cooldown and moved no health. The empowered
// cooldown reduction is separate and already works through Empowerment::Transfusion.
class spell_ascension_bloodmage_transfusion : public SpellScript
{
    PrepareSpellScript(spell_ascension_bloodmage_transfusion);

    void Swap(SpellEffIndex index)
    {
        PreventHitDefaultEffect(index);
        Unit* caster = GetCaster();
        Unit* target = GetHitUnit();
        if (!caster || !target || caster == target || !caster->IsAlive() || !target->IsAlive() ||
            !caster->GetMaxHealth() || !target->GetMaxHealth())
            return;
        float casterPct = caster->GetHealthPct();
        float targetPct = target->GetHealthPct();
        // The swap only exchanges the pair, so its minimum is the same before and after it. The effect's
        // own value (40) raises that one member, not both, per "the lower health percentage of the two".
        float lower = std::min(casterPct, targetPct);
        float raised = std::max(lower, float(GetEffectValue()));
        SetHealthPct(caster, targetPct > lower ? targetPct : raised);
        SetHealthPct(target, casterPct > lower ? casterPct : raised);
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_ascension_bloodmage_transfusion::Swap,
            EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
    }
};

class spell_ascension_bloodmage_empowered : public SpellScript
{
    PrepareSpellScript(spell_ascension_bloodmage_empowered);
    bool _heartbreak = false;

    bool Load() override
    {
        return GetCaster()->IsPlayer() && GetCaster()->getClass() == CLASS_SON_OF_ARUGAL &&
            GetSpellInfo()->SpellFamilyName == 26;
    }

    bool Empowered(Empowerment kind)
    {
        return !GetSpell()->IsTriggered() && GetSpell()->GetScriptValue(PooledVitalityTalent) == kind;
    }

    void HeartbreakPower(SpellEffIndex index)
    {
        if (GetSpellInfo()->Effects[index].TriggerSpell != HeartbreakBuff)
            return;
        // Native effect 142 otherwise grants the empowered buff unconditionally,
        // and its old enemy selector gives the party buff the wrong anchor.
        PreventHitDefaultEffect(index);
        if (!Empowered(Heartbreak) || _heartbreak)
            return;
        _heartbreak = true;
        int32 amount = GetEffectValue();
        Unit* player = GetCaster();
        player->CastCustomSpell(player, HeartbreakBuff, &amount, &amount, nullptr, true);
        player->CastSpell(player, VisceralPower, true);
    }

    void ModifyHit()
    {
        if (Empowered(Bloodbolt) && GetHitUnit() == GetExplTargetUnit() && GetHitDamage() > 0)
            SetHitDamage(int32(std::min<int64>(int64(GetHitDamage()) * 2, std::numeric_limits<int32>::max())));
        if (Empowered(Fleshcraft) && GetHitUnit() && GetHitUnit()->IsAlive() && GetHitHeal() > 0)
        {
            Unit* caster = GetCaster();
            Unit* target = GetHitUnit();
            // Blood Redistribution (706256): the Fleshcraft pool grows by a
            // quarter while the passive is held.
            uint32 const pool = caster->HasAura(706256)
                ? caster->CountPctFromMaxHealth(50) : caster->CountPctFromMaxHealth(25);
            uint32 bonus = caster->SpellHealingBonusDone(target, GetSpellInfo(),
                pool, HEAL, EFFECT_0);
            bonus = target->SpellHealingBonusTaken(caster, GetSpellInfo(), bonus, HEAL);
            SetHitHeal(int32(std::min<int64>(int64(GetHitHeal()) + bonus, std::numeric_limits<int32>::max())));
        }
    }

    void Register() override
    {
        if (GetEmpowerment(m_scriptSpellId) == Heartbreak)
            OnEffectLaunchTarget += SpellEffectFn(spell_ascension_bloodmage_empowered::HeartbreakPower,
                EFFECT_1, SPELL_EFFECT_TRIGGER_SPELL_WITH_VALUE);
        OnHit += SpellHitFn(spell_ascension_bloodmage_empowered::ModifyHit);
    }
};

// Blood Craving (800780) pays out through its periodic trigger (805985,
// aura 23, every 2 s). The heal half (effect 0, SPELL_EFFECT_HEAL_PCT +4
// = 5% of maximum health) matches the tooltip. The Rage half (effect 1,
// SPELL_EFFECT_ENERGIZE_PCT, misc 1, +14 = 15%) would pay 15% of MAXIMUM
// Rage per tick through the native handler, but the tooltip reads 15% of
// MISSING Rage — so the effect is skipped and the payout is computed here.
class spell_ascension_blood_craving_payout : public SpellScript
{
    PrepareSpellScript(spell_ascension_blood_craving_payout);

    bool Load() override
    {
        return GetCaster() && GetCaster()->IsPlayer() &&
            GetSpellInfo()->Id == SPELL_BLOOD_CRAVING_PAYOUT;
    }

    void MissingRage(SpellEffIndex index)
    {
        if (GetSpellInfo()->Effects[index].MiscValue != POWER_RAGE)
            return;
        PreventHitDefaultEffect(index);
        Player* player = GetCaster()->ToPlayer();
        uint32 const maxRage = player->GetMaxPower(POWER_RAGE);
        uint32 const missing = maxRage - player->GetPower(POWER_RAGE);
        uint32 const gain = CalculatePct(missing, GetSpellInfo()->Effects[index].CalcValue(player));
        if (gain)
            player->EnergizeBySpell(player, GetSpellInfo()->Id, gain, POWER_RAGE);
// Eternal Presence (560001): "Increases the attack power of party and raid members by $s1%. Does not stack
// with similar effects. / In addition, you now gain attack power equal to $s3% of the damage taken for
// $560010d. Can only occur once every 10 sec." Effects 0 and 1 (SPELL_EFFECT_APPLY_AREA_AURA_RAID, auras 166
// and 167, 5% each) are native and keep the spell_group 2000180 stack rule; nothing here touches them.
// Effect 2 is the private aura 354, which has no handler in AuraEffectHandler (SpellAuraEffects.cpp:419) and
// no core consumer, and the record carries ProcFlags 0, so its TriggerSpell 560010 was never cast. That
// helper's own effect is a flat +5 attack power, so the 15% share of the damage taken is forwarded as a
// custom base point; the taken-damage proc flag and the 10 second internal cooldown the tooltip states come
// from the companion spell_proc row. The amount is read from the effect, so a future rank still applies.
constexpr uint32 EternalPresenceBuff = 560010;

class aura_ascension_eternal_presence : public AuraScript
{
    PrepareAuraScript(aura_ascension_eternal_presence);

    bool Validate(SpellInfo const*) override { return ValidateSpellInfo({EternalPresenceBuff}); }

    bool Check(ProcEventInfo& event)
    {
        Unit* target = GetTarget();
        DamageInfo const* damage = event.GetDamageInfo();
        return target->IsPlayer() && target->IsAlive() && target->IsInWorld() &&
            event.GetActionTarget() == target && damage && damage->GetDamage();
    }

    void Proc(AuraEffect const* effect, ProcEventInfo& event)
    {
        PreventDefaultAction();
        uint64 amount = uint64(event.GetDamageInfo()->GetDamage()) * std::clamp(effect->GetAmount(), 0, 100) / 100;
        if (!amount)
            return;
        GetTarget()->CastCustomSpell(EternalPresenceBuff, SPELLVALUE_BASE_POINT0,
            int32(std::min<uint64>(amount, std::numeric_limits<int32>::max())), GetTarget(), TRIGGERED_FULL_MASK);
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_ascension_blood_craving_payout::MissingRage,
            EFFECT_1, SPELL_EFFECT_ENERGIZE_PCT);
    }
};

// Forbidden Power (500445) mirror half: the passive's periodic (aura 23,
// every 3 s) re-triggers 500447, whose effect 0 (aura 123,
// MOD_TARGET_RESISTANCE, school mask 124 = every magic school) is the
// spell-penetration half. Its amount is a -1 placeholder in the DBC, so on
// each apply it is set to the caster's armor-penetration rating — the
// periodic re-trigger keeps the value current as gear changes.
class aura_ascension_forbidden_pen : public AuraScript
{
    PrepareAuraScript(aura_ascension_forbidden_pen);

    bool Load() override
    {
        return GetCaster() && GetCaster()->IsPlayer() &&
            GetSpellInfo()->Id == SPELL_FORBIDDEN_POWER_MIRROR;
    }

    void SetPenAmount(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        if (Player* player = GetCaster()->ToPlayer())
            const_cast<AuraEffect*>(GetAura()->GetEffect(EFFECT_0))->SetAmount(
                int32(player->GetRatingBonusValue(CR_ARMOR_PENETRATION)));
    }

    void Register() override
    {
        AfterEffectApply += AuraEffectApplyFn(aura_ascension_forbidden_pen::SetPenAmount,
            EFFECT_0, SPELL_AURA_MOD_TARGET_RESISTANCE, AURA_EFFECT_HANDLE_REAL);
    }
};

// Endure the Curse (681190): "Reduce all damage taken by $s2% for $d. While active, taking damage that would
// reduce your health below 10% will instantly heal you for $681189s1% of your maximum health. Can only occur
// once." Effect 1 (aura 87, -30%, MiscValue 127) is native. Effect 0 is a SPELL_AURA_SCHOOL_ABSORB whose
// Spell.dbc amount is 0 (BasePoints -1, DieSides 1) over the same all-school mask, and with no script
// Unit::CalcAbsorbResist subtracts the nothing it absorbed and removes the whole aura on the first point of
// damage of any school (Unit.cpp:2515-2523), taking the reduction the spell exists for with it. Declaring the
// absorb infinite keeps the effect out of that branch - the shape aura_ascension_templar_stagger already uses
// - while the handler still absorbs nothing, so the spell mitigates for its full duration. 681189
// (SPELL_EFFECT_HEAL_PCT, 30% of maximum health) is a real record that nothing referenced; it is cast from
// the same handler, at most once per application, with its own value rather than a repeated number.
constexpr uint32 EndureTheCurseHeal = 681189;

class aura_ascension_endure_the_curse : public AuraScript
{
    PrepareAuraScript(aura_ascension_endure_the_curse);
    bool _healed = false;

    bool Validate(SpellInfo const*) override { return ValidateSpellInfo({EndureTheCurseHeal}); }

    void Amount(AuraEffect const*, int32& amount, bool& recalculate)
    {
        amount = -1;
        recalculate = false;
    }

    void Absorb(AuraEffect*, DamageInfo& damage, uint32& absorb)
    {
        // The tooltip promises no shield: this effect only carries the lethal-damage clause.
        absorb = 0;
        Unit* target = GetTarget();
        if (_healed || !target->IsAlive() || !damage.GetDamage())
            return;
        // The handler runs before the damage is applied, so the health here is the health before the hit.
        if (uint64(target->GetHealth()) >= uint64(damage.GetDamage()) + target->CountPctFromMaxHealth(10))
            return;
        _healed = true;
        target->CastSpell(target, EndureTheCurseHeal, true);
    }

    void Register() override
    {
        DoEffectCalcAmount += AuraEffectCalcAmountFn(aura_ascension_endure_the_curse::Amount,
            EFFECT_0, SPELL_AURA_SCHOOL_ABSORB);
        OnEffectAbsorb += AuraEffectAbsorbFn(aura_ascension_endure_the_curse::Absorb, EFFECT_0);
    }
};
}

void AddSC_AscensionBloodmageVitality()
{
    new bloodmage_vitality_casts();
    new bloodmage_vitality_scaling();
    RegisterSpellScript(spell_ascension_bloodmage_transfusion);
    RegisterSpellScript(spell_ascension_bloodmage_empowered);
    RegisterSpellScript(spell_ascension_blood_craving_payout);
    new aura_ascension_forbidden_pen();
    RegisterSpellScript(aura_ascension_eternal_presence);
    RegisterSpellScript(aura_ascension_endure_the_curse);
}
