/* Copyright (C) 2016+ AzerothCore, GNU AGPL v3. */
#include "AscensionPyromancer.h"
#include "AscensionPyromancerData.h"
#include "DBCStores.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SpellAuraEffects.h"
#include "SpellAuras.h"
#include "SpellMgr.h"
#include <algorithm>
namespace AscensionPyromancer
{
void ApplyContracts(SpellInfo* info)
{
    if (!info || info->SpellFamilyName != 30)
        return;
    uint32 id = info->Id;
    auto dummy = [info](uint8 slot)
    {
        info->Effects[slot].ApplyAuraName = SPELL_AURA_DUMMY;
        info->Effects[slot].TriggerSpell = 0;
    };
    auto mod = [info](uint8 slot, AuraType type, int32 value, uint32 operation, flag96 mask)
    {
        auto& e = info->Effects[slot];
        e.Effect = SPELL_EFFECT_APPLY_AURA;
        e.ApplyAuraName = type;
        e.BasePoints = value - 1;
        e.DieSides = 1;
        e.MiscValue = operation;
        e.SpellClassMask = mask;
        e.TargetA = SpellImplicitTargetInfo(TARGET_UNIT_CASTER);
        e.TargetB = SpellImplicitTargetInfo();
        e.TriggerSpell = 0;
    };
    if (id == 500167 || id == 500644)
    {
        // Spirit of Fire: "Increases the effectiveness of your absorption
        // effects by 20%/40%." Both ranks ship without SPELL_ATTR0_PASSIVE,
        // so learn/login never applied the aura-317 absorb multiplier, and
        // their BasePoints are display-minus-1 with DieSides 0. Mark passive
        // and shift DieSides to 1 so the values resolve as 20/40; the
        // engine's absorb path (SpellAuraEffects.cpp) picks the aura up
        // natively for family-30 casters.
        info->Attributes |= SPELL_ATTR0_PASSIVE;
        info->Effects[EFFECT_0].DieSides = 1;
        info->Effects[EFFECT_1].DieSides = 1;
    }
    // Incinerator (807510): the client DBC ships a wrong aura (Mod Spell Damage
    // Taken School -70). The tooltip grants 30 spell penetration, scaling with
    // level via the passive's native level scaling on BasePoints.
    // Critical Pressure (704804/707789): "Increases your Intellect and critical
    // strike chance by 3%." The client DBC ships the stat aura without a usable
    // school/stat pairing and leaves the crit slot unbound; rewrite both.
    if (id == 704804 || id == 707789)
    {
        int32 const amount = id == 704804 ? 3 : 6;
        mod(0, SPELL_AURA_MOD_TOTAL_STAT_PERCENTAGE, amount, STAT_INTELLECT, flag96());
        mod(1, SPELL_AURA_MOD_CRIT_PCT, amount, 0, flag96());
        info->Effects[2].Effect = 0;
    }
    // Hasty Incantation (802169): "Increases the range of Flame Step by 10 yds."
    // The talent ships without SPELL_ATTR0_PASSIVE, so learn/login never cast the
    // modifier aura; the DBC also binds the flat mod to SPELLMOD_RADIUS instead
    // of SPELLMOD_RANGE (range lives in the spell's RangeIndex, not a radius
    // entry) and leaves the class mask empty so it can never match Flame Step
    // (family 30, flags 0x40 in slot 2). Mark passive, rebind the operation,
    // and copy Flame Step's family mask so IsAffected matches all three ranks.
    if (id == 802169)
    {
        auto& range = info->Effects[EFFECT_0];
        range.ApplyAuraName = SPELL_AURA_ADD_FLAT_MODIFIER;
        range.BasePoints = 9; // DieSides 1 -> displayed 10 yds
        range.DieSides = 1;
        range.MiscValue = SPELLMOD_RANGE;
        range.SpellClassMask = flag96(0, 0x40, 0);
        info->Attributes |= SPELL_ATTR0_PASSIVE;
    }
    if (id == 807510)
    {
        auto& e = info->Effects[EFFECT_0];
        e.Effect = SPELL_EFFECT_APPLY_AURA;
        e.ApplyAuraName = SPELL_AURA_MOD_TARGET_RESISTANCE;
        e.BasePoints = 29;
        e.DieSides = 1;
        e.MiscValue = SPELL_SCHOOL_MASK_MAGIC;
        e.TargetA = SpellImplicitTargetInfo(TARGET_UNIT_CASTER);
        e.TriggerSpell = 0;
    }
    for (auto const& list : {std::pair(PyromancerEvents, std::size(PyromancerEvents)),
                             std::pair(PyromancerDrivers, std::size(PyromancerDrivers))})
        for (size_t n = 0; n < list.second; ++n)
            if (list.first[n] == id)
            {
                for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
                    if (info->Effects[i].ApplyAuraName == SPELL_AURA_PROC_TRIGGER_SPELL ||
                        info->Effects[i].ApplyAuraName == SPELL_AURA_PROC_TRIGGER_DAMAGE ||
                        info->Effects[i].ApplyAuraName == 347 || info->Effects[i].ApplyAuraName == 354)
                        dummy(i);
                info->ProcFlags = info->ProcCharges = 0;
            }
    for (uint32 copy : PyromancerCopies)
        if (id == copy)
        {
            info->AttributesEx2 |= SPELL_ATTR2_CANT_CRIT;
            info->AttributesEx3 |= SPELL_ATTR3_IGNORE_CASTER_MODIFIERS;
            info->AttributesEx4 |= SPELL_ATTR4_IGNORE_DAMAGE_TAKEN_MODIFIERS;
            info->AttributesCu |= SPELL_ATTR0_CU_IGNORE_ARMOR;
            info->AscensionInheritsResolvedAmount = id != 520826;
            bool helpful = id == 704274 || id == 707892 || id == 707595 || id == 807403;
            info->Effects[0].TargetA =
                SpellImplicitTargetInfo(helpful ? TARGET_UNIT_TARGET_ALLY : TARGET_UNIT_TARGET_ENEMY);
            info->Effects[0].TargetB = SpellImplicitTargetInfo();
        }
    if (id == 503864)
        info->Effects[0].Effect = SPELL_EFFECT_SCHOOL_DAMAGE;
    if (id == 707325)
    {
        // Flames of Fate doubles down on Aspect's Blessing: the extra charges
        // belong to the Blessing (word0 0x20) while the doubled cooldown and
        // the +50% mana belong to Echo of Nozdormu (word0 0x4). All three
        // modifiers ship keyed to a word2 bit that only matches Cataclysm.
        info->Effects[0].SpellClassMask = flag96(0x20, 0, 0);
        info->Effects[1].SpellClassMask = flag96(0x4, 0, 0);
        info->Effects[2].SpellClassMask = flag96(0x4, 0, 0);
    }
    if (id == 520868)
        info->Effects[0].Effect = SPELL_EFFECT_DUMMY;
    if (id == 572806)
        info->Effects[1].Effect = info->Effects[2].Effect = 0;
    if (Named(info, 802174))
        info->Effects[1].Effect = info->Effects[2].Effect = 0;
    if (Named(info, 805500))
        dummy(1), dummy(2); // Persist next-Blaze critical snapshots in the existing zero-valued slots.
    if (Any(info, {800791, 706874}))
        dummy(1); // Persist Stoke's consumed extension budget through normal aura saves.
    if (id == 520927)
        dummy(1);
    if (id == 524707)
        info->ProcCharges = 3; // Saved native charges; only Finish consumes them, with no proc row.
    if (id == 680962 || id == 807403 || id == 520826)
        for (uint8 slot = 1; slot < MAX_SPELL_EFFECTS; ++slot)
        {
            auto& effect = info->Effects[slot];
            effect.Effect = SPELL_EFFECT_APPLY_AURA;
            dummy(slot);
            effect.BasePoints = -1;
            effect.DieSides = 1;
            effect.Amplitude = 0;
            effect.SpellClassMask = flag96();
            effect.TargetA = info->Effects[0].TargetA;
            effect.TargetB = SpellImplicitTargetInfo();
        }
    if (id == 680367 || id == 681366)
    {
        info->Effects[id == 680367 ? 0 : 1].ApplyAuraName = SPELL_AURA_MOD_HEALING_DONE_PERCENT;
        if (id == 680367)
            info->Effects[1].Effect = 0;
    }
    if (id == 807402)
        info->Effects[1].Effect = 0;
    if (id == 807768)
        info->Effects[0].Effect = SPELL_EFFECT_DUMMY;
    if (id == 802120 || id == 680369)
    {
        info->Effects[0].ApplyAuraName = SPELL_AURA_PERIODIC_DUMMY;
        info->Effects[0].TriggerSpell = 0;
    }
    if (id == 573220)
    {
        // Transform uses a creature template, not a display ID.
        info->Effects[1].MiscValue = 21362;
    }
    if (id == 706650)
        dummy(0), info->Effects[0].BasePoints = 29;
    if (id == 706238)
        dummy(0), info->Effects[1].Effect = 0;
    if (id == 706877 || id == 706889 || id == 300751)
        dummy(0);
    if (id == 706875)
        // Cinderstorm's crit half ships with Effect NONE, so the authored +10%
        // Firefall crit never loads; the threat half is already native.
        info->Effects[2].Effect = SPELL_EFFECT_APPLY_AURA;
    if (id == 805474)
        dummy(0); // Target's caster-owned modifier is evaluated at heal time.
    if (id == 807542)
    {
        mod(0, SPELL_AURA_ADD_PCT_MODIFIER, 0, SPELLMOD_CASTING_TIME, flag96(2097152, 0, 0));
        info->StackAmount = 1;
    }
    if (id == 900755)
        info->DurationEntry = sSpellDurationStore.LookupEntry(21);
    if (id == 806783)
        info->Effects[0].SpellClassMask = flag96(2, 0, 1048576);
    if (id == 525059)
        info->Effects[1].SpellClassMask = flag96(0, 0, 2048);
    if (id == 520823)
    {
        info->Effects[1].Effect = 0;
        mod(2, SPELL_AURA_ADD_PCT_MODIFIER, -100, SPELLMOD_COST, flag96(134217728, 0, 8192));
    }
    if (id == 680387)
        dummy(1); // Retaliation scales from the owner's Spirit/SP once.
    if (id == 807944)
        for (uint8 i = 0; i < 2; ++i)
        {
            info->Effects[i].ApplyAuraName = SPELL_AURA_PERIODIC_DUMMY;
            info->Effects[i].TriggerSpell = 0;
            info->Effects[i].Amplitude = i ? 1000 : 5000;
        }
    if (id == 807540)
        info->Effects[1].Effect = 0;
    if (id == 680372)
        info->Effects[2].Effect = 0;
    if (id == 803380)
        info->Effects[0].Effect = 0; // Obsolete Overheat remover points to absent legacy ID 802565.
    if (id == 706893)
        mod(1, SPELL_AURA_ADD_FLAT_MODIFIER, -500, SPELLMOD_DURATION, flag96(0, 0, 8388608));
    if (id == 807224)
        info->StackAmount = 5;
    if (id == 803712)
        info->StackAmount = 3;
    if (id == 707480 || id == 704823)
    {
        flag96 mask;
        for (uint32 root : {800792, 802174, 801915, 805500, 800818, 520019, 680369, 704278, 706854, 802791})
            if (SpellInfo const* spender = sSpellMgr->GetSpellInfo(root))
                mask |= spender->SpellFamilyFlags;
        info->Effects[0].SpellClassMask = mask;
    }
    if (id == 704277)
        info->DurationEntry = sSpellDurationStore.LookupEntry(1);
    if (id == 807159)
    {
        // Issue 859: Purifying Flames ships without SPELL_ATTR0_PASSIVE, so
        // the learn/login passes never applied its duration mod. Mark
        // passive. Effect 0 (op 1 = SPELLMOD_DURATION, bp 2999, maskC 0x1)
        // matches Essence of Malygos (802120, family-30 flag 0x1) and
        // resolves as the tooltip's +3s duration via the native
        // duration-mod path.
        info->Attributes |= SPELL_ATTR0_PASSIVE;
    }
    if (id == 525018)
    {
        // Issue 837: Fiery Intent ships without SPELL_ATTR0_PASSIVE, so the
        // learn/login passes never applied its cooldown mod. Mark passive.
        // Effect 0 (op 11 = SPELLMOD_COOLDOWN, bp -30001, maskC 0x20)
        // matches Cataclysm (520218, family-30 flag 0x20) and resolves as
        // the tooltip's -30s cooldown via the native cooldown-mod path.
        info->Attributes |= SPELL_ATTR0_PASSIVE;
    }
    if (id == 707126)
    {
        // Issue 814: Constant Burning ships without SPELL_ATTR0_PASSIVE, so
        // the learn/login passes never applied its Spellburn mods. Mark
        // passive. Effect 0 (op 11 = SPELLMOD_COOLDOWN, bp -4001, maskA/B
        // 0x8) matches Spellburn (800808, family-30 flag 0x8) and resolves
        // as the tooltip's -4s cooldown via the native cooldown-mod path.
        // Effect 1 (op 1 = SPELLMOD_DURATION, bp 999, empty mask) targets
        // nothing; retarget it to Spellburn so the +1s duration applies.
        info->Attributes |= SPELL_ATTR0_PASSIVE;
        info->Effects[EFFECT_1].SpellClassMask = flag96(0x8, 0, 0);
    }
    if (id == 802173 || id == 520826 || id == 680370 || id == 680371)
    {
        info->AscensionIgnoreAbsorbAndResistance = true;
    }
    if (id == 680370 || id == 680371)
        info->Attributes |= SPELL_ATTR0_NO_IMMUNITIES;
    for (uint32 heal : {707110, 706856, 806742, 806743, 806749, 712482})
        if (id == heal)
        {
            info->Effects[0].TargetA = SpellImplicitTargetInfo(TARGET_UNIT_TARGET_ALLY);
            info->Effects[0].TargetB = SpellImplicitTargetInfo();
        }
    if (id == 803704 || id == 680366)
    {
        info->Effects[0].TargetA = SpellImplicitTargetInfo(TARGET_UNIT_TARGET_ENEMY);
        info->Effects[0].TargetB = SpellImplicitTargetInfo();
    }
    if (id == 803704)
    {
        info->Effects[1].TargetA = SpellImplicitTargetInfo(TARGET_UNIT_TARGET_ENEMY);
        info->Effects[1].TargetB = SpellImplicitTargetInfo();
    }
    info->_InitializeExplicitTargetMask();
}
} // namespace AscensionPyromancer
namespace
{
using namespace AscensionPyromancer;
class pyromancer_scaling : public UnitScript
{
  public:
    pyromancer_scaling()
        : UnitScript("pyromancer_scaling", true,
                     {UNITHOOK_MODIFY_SPELL_EFFECT_BASE_VALUE, UNITHOOK_MODIFY_SPELL_DAMAGE_TAKEN,
                      UNITHOOK_MODIFY_PERIODIC_DAMAGE_AURAS_TICK, UNITHOOK_MODIFY_HEAL_RECEIVED, UNITHOOK_ON_AURA_APPLY,
                      UNITHOOK_ON_PERIODIC_DAMAGE_RESULT})
    {
    }
    void ModifySpellEffectBaseValue(Unit const* caster, SpellInfo const* info, uint8 index, float& value) override
    {
        Player* player = Owner(caster);
        if (!player || !info)
            return;
        for (auto const& row : PyromancerCoefficients)
            if (row.spell == info->Id && row.effect == index)
                value += row.sp * std::max(0, player->SpellBaseDamageBonusDone(SPELL_SCHOOL_MASK_FIRE)) +
                         row.spirit * player->GetStat(STAT_SPIRIT) +
                         row.healing * std::max(0, player->SpellBaseHealingBonusDone(SPELL_SCHOOL_MASK_FIRE));
        if ((info->Id == 680370 || info->Id == 680371) && !index)
            value *= 1 + State(player).ignis * Amount(680382) / 100.0f;
        value = std::clamp(value, float(INT32_MIN / 2), float(INT32_MAX / 2));
    }
    float Factor(Unit* target, Unit* caster, SpellInfo const* info)
    {
        Player* player = Owner(caster);
        if (!player || !info || !target || Derived(info))
            return 1;
        float factor = 1;
        if (Any(info, {800792, 801915}) && player->HasAura(706889) && Burning(player, target))
            factor *= 1 + Amount(706889) / 100.0f;
        if (Named(info, 805500) && player->HasAura(706238) && target->GetHealthPct() < 35)
            factor *= 1 + Amount(706238) / 100.0f;
        if (Named(info, 800792) && player->HasAura(520884))
            factor *= 1 + Burning(player, target) * .15f;
        return factor;
    }
    void ModifySpellDamageTaken(Unit* target, Unit* caster, int32& damage, SpellInfo const* info) override
    {
        damage = int32(damage * Factor(target, caster, info));
    }
    void ModifyPeriodicDamageAurasTick(Unit* target, Unit* caster, uint32& damage, SpellInfo const* info) override
    {
        damage = uint32(damage * Factor(target, caster, info));
    }
    void ModifyHealReceived(Unit* target, Unit* caster, uint32& healing, SpellInfo const* info) override
    {
        Player* player = Owner(caster);
        if (!player || !target || !info || info->SpellFamilyName != 30 || Derived(info))
            return;
        if (Aura* aura = target->GetAura(805474, player->GetGUID()))
            AddPct(healing, aura->GetStackAmount() * Amount(805474));
    }
    void OnAuraApply(Unit* target, Aura* aura) override
    {
        Player* player = Owner(target);
        if (player == target && player && aura->GetSpellInfo()->HasAura(SPELL_AURA_MOD_STUN) && player->HasAura(524818))
            Cast(player, player, 524819);
    }
    void OnPeriodicDamageResult(Unit*, Unit* caster, uint32 damage, SpellInfo const* info) override
    {
        Player* player = Owner(caster);
        if (player && info && info->Id == 807540 && damage)
        {
            Resource(player, HeatAura, 1);
            Mana(player, (player->GetMaxPower(POWER_MANA) - player->GetPower(POWER_MANA)) * 2 / 100, 807402);
        }
    }
};
} // namespace
void AddSC_AscensionPyromancerContracts()
{
    new pyromancer_scaling();
}
