/* Copyright (C) 2016+ AzerothCore, GNU AGPL v3. */
#include "AscensionSunCleric.h"
#include "AscensionSunClericData.h"
#include "DBCStores.h"
#include "DynamicObject.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SpellAuraEffects.h"
#include "SpellAuras.h"
#include <algorithm>
namespace AscensionSunCleric
{
void ApplyContracts(SpellInfo* info)
{
    if (!info || info->SpellFamilyName != 33)
        return;
    uint32 id = info->Id;
    if (id == 560857)
    {
        // Issue 708: Burn The Heretics' two effects are the authored halves
        // (+10% damage and healing against low-health targets, aura 303 =
        // MOD_DAMAGE_DONE_VERSUS_AURASTATE and aura 360 =
        // ASCENSION_MOD_HEALING_DONE_VERSUS_AURASTATE, misc 13 = the low-health
        // aurastate, bp 9 with DieSides 1 resolving as 10; both consumed by
        // the native Unit damage/healing bonus paths). The DBC already carries
        // SPELL_ATTR0_PASSIVE, so the re-mark below is a defensive no-op kept
        // in case the record is ever regenerated without it.
        info->Attributes |= SPELL_ATTR0_PASSIVE;
    }
    if (id == Rejuvenating)
        for (auto& effect : info->Effects)
            if (effect.IsAura())
            {
                effect.TargetA = SpellImplicitTargetInfo(TARGET_UNIT_TARGET_ALLY);
                effect.TargetB = SpellImplicitTargetInfo();
            }
    auto dummy = [info](uint8 slot)
    {
        info->Effects[slot].ApplyAuraName = SPELL_AURA_DUMMY;
        info->Effects[slot].TriggerSpell = 0;
    };
    auto aura = [info](uint8 slot, AuraType type, int32 amount, int32 misc, uint32 target)
    {
        auto& effect = info->Effects[slot];
        effect.Effect = SPELL_EFFECT_APPLY_AURA;
        effect.ApplyAuraName = type;
        effect.BasePoints = amount;
        effect.DieSides = 0;
        effect.MiscValue = misc;
        effect.TriggerSpell = 0;
        effect.TargetA = SpellImplicitTargetInfo(target);
        effect.TargetB = SpellImplicitTargetInfo();
    };
    for (auto list : {std::pair(SunClericEvents, std::size(SunClericEvents)),
                     std::pair(SunClericDrivers, std::size(SunClericDrivers)),
                     std::pair(SunClericFinite, std::size(SunClericFinite))})
        for (size_t n = 0; n < list.second; ++n)
            if (list.first[n] == id)
            {
                for (uint8 slot = 0; slot < MAX_SPELL_EFFECTS; ++slot)
                    if (info->Effects[slot].ApplyAuraName == SPELL_AURA_PROC_TRIGGER_SPELL ||
                        info->Effects[slot].ApplyAuraName == 354)
                        dummy(slot);
                info->ProcFlags = info->ProcCharges = 0;
            }
    for (uint32 copy : SunClericCopies)
        if (id == copy)
        {
            info->AttributesEx2 |= SPELL_ATTR2_CANT_CRIT;
            info->AttributesEx3 |= SPELL_ATTR3_IGNORE_CASTER_MODIFIERS;
            info->AttributesEx4 |= SPELL_ATTR4_IGNORE_DAMAGE_TAKEN_MODIFIERS;
            info->AttributesCu |= SPELL_ATTR0_CU_IGNORE_ARMOR;
            info->AscensionInheritsResolvedAmount = true;
            bool damage = id == 803490 || id == 803493 || id == 807853 || id == 707774 ||
                          id == 570147 || id == 807078 || id == 807215;
            info->Effects[0].TargetA = SpellImplicitTargetInfo(damage ? TARGET_UNIT_TARGET_ENEMY : TARGET_UNIT_TARGET_ALLY);
            info->Effects[0].TargetB = SpellImplicitTargetInfo();
            if (id == 707774)
                info->Effects[0].Effect = SPELL_EFFECT_SCHOOL_DAMAGE;
        }
    if (id == 807994)
        info->AttributesEx2 &= ~SPELL_ATTR2_CANT_CRIT; // Holy Form may crit and trigger Redeemer once.
    if (id == 807058)
        info->Effects[0].TargetA = SpellImplicitTargetInfo(TARGET_UNIT_TARGET_ENEMY);
    if (id == 807507)
    {
        // Issue 820: Valkyr's Grip ships without SPELL_ATTR0_PASSIVE, so the
        // learn/login passes never applied its auras, and its BasePoints are
        // display-minus-1 with DieSides 0. Mark passive and shift DieSides
        // to 1 so effect 0 resolves as the tooltip's 31% off-hand damage
        // (aura 122, native MOD_OFFHAND_DAMAGE_PCT path) and effect 1 as
        // -15% disarm duration (aura 232 misc 3 = MECHANIC_DISARM, native
        // MECHANIC_DURATION_MOD path).
        info->Attributes |= SPELL_ATTR0_PASSIVE;
        info->Effects[EFFECT_0].DieSides = 1;
        info->Effects[EFFECT_1].DieSides = 1;
    }
    if (id == 807654)
    {
        // Issue 687: Solar Discernment's two effects are the authored halves
        // (+12% of Intellect into ratings, aura 220, miscB 3 = STAT_INTELLECT;
        // misc bitmasks 1792 = crit ratings and 8388608 = expertise, native
        // MOD_RATING_FROM_STAT path). The DBC already carries
        // SPELL_ATTR0_PASSIVE and DieSides 1, so the re-mark below is a
        // defensive no-op kept in case the record is ever regenerated without
        // them.
        info->Attributes |= SPELL_ATTR0_PASSIVE;
        info->Effects[EFFECT_0].DieSides = 1;
        info->Effects[EFFECT_1].DieSides = 1;
    }
    if (id == 681337)
    {
        // Issue 666: Holy Giant's real defect is its mask: the authored 0x2
        // only keys Vow of the Valkyr (807749, flags 0x3), while Vow of Light
        // (807547) sits on flag 0x1, so widen the mask to 0x3 to double both
        // Vows as the tooltip states (op 8 = SPELLMOD_ALL_EFFECTS, native
        // path). The DBC already carries SPELL_ATTR0_PASSIVE and DieSides 1,
        // so the re-mark below is a defensive no-op kept in case the record
        // is ever regenerated without them.
        info->Attributes |= SPELL_ATTR0_PASSIVE;
        info->Effects[EFFECT_0].DieSides = 1;
        info->Effects[EFFECT_0].SpellClassMask |= flag96(0x1, 0, 0);
    }
    if (id == 704911)
    {
        info->Effects[0].TargetA = SpellImplicitTargetInfo(TARGET_UNIT_TARGET_ALLY);
        info->Effects[0].TargetB = SpellImplicitTargetInfo();
    }
    if (id == 504248)
    {
        // Issue 865: Holy Conquest ships without SPELL_ATTR0_PASSIVE, so the
        // learn/login passes never applied its cost-mod aura. Mark passive.
        // Effect 0 (op 14 = SPELLMOD_COST, bp -51, maskA 0x1008) matches
        // Chosen of the Light (800622, family-33 flag 0x1000) and Judgement
        // Day (806121, family-33 flag 0x8) and resolves as the tooltip's
        // -50% mana cost via the native cost-mod path. Paragon (680639,
        // family-33 flag 0x800000 in maskC) is outside the authored mask, so
        // union it in.
        info->Attributes |= SPELL_ATTR0_PASSIVE;
        info->Effects[EFFECT_0].SpellClassMask |= flag96(0, 0, 0x800000);
    }
    if (id == Dawn)
    {
        dummy(0);
        dummy(1);
        info->ProcCharges = 10;
    }
    if (id == SolarPower)
        dummy(1);
    if (id == 800039)
        // Issue 993: Sun Warrior's Guidance ships without the passive flag,
        // so the learn/login passes never applied its crit aura, and its mask
        // only keys Gavel of Light (word0 0x40000000), missing Dawnbreak's
        // own family bit (word1 0x40000000). Mark passive and union both in;
        // the native crit-chance mod path then grants the authored +5%.
        info->Attributes |= SPELL_ATTR0_PASSIVE,
        info->Effects[0].SpellClassMask = flag96(0x40000210, 0x40000020, 0);
    if (id == 560547)
        // Issue 1028: Sun Disc ships without the passive flag, so the
        // learn/login passes never applied its auras, and its Illumination
        // half ships with an empty mask that would boost every Sun Cleric
        // spell. Mark passive and key the Illumination half to its own
        // family bits; the raid healing half is already native, and the
        // native damage-mod path then grants the authored +20%.
        info->Attributes |= SPELL_ATTR0_PASSIVE,
        info->Effects[1].SpellClassMask = flag96(0, 0x200, 0x200);
    if (id == 681468)
        // Issue 1008: Paying The Tithe ships without the passive flag, so the
        // learn/login passes never applied its auras, and its charges half
        // ships with an empty mask that would grant +4 blocks to every
        // charge-based spell. Mark passive and key the charges half to
        // Seraphic Bulwark's own family bits; the Dawnbreak crit half is
        // already native, and the native charge/crit paths apply both.
        info->Attributes |= SPELL_ATTR0_PASSIVE,
        info->Effects[0].SpellClassMask = flag96(0x500, 0, 0);

    if (id == 704945)
    {
        // Issue 1138: Unbreakable Focus ships with a mask (word 2 bit 20) that
        // keys only cooldown-less passive modifiers (Holy Giant, Champion's
        // Arrival, Vindicator, Empowered Holy Form), so its -20% cooldown mod
        // (aura 108, op 11 = SPELLMOD_COOLDOWN, native path) never reaches the
        // tooltip's targets. Re-key to the union of Gavel of Light
        // ([0x40000000, 0x20, 0] and [0x40000000, 0, 0]) and Horusath Blast
        // ([0, 0x40, 0]) family bits, which carry the 5s category and 25s
        // cooldowns the tooltip promises to reduce.
        info->Effects[EFFECT_0].SpellClassMask = flag96(0x40000000, 0x60, 0);
    }
    if (id == 800764)
    {
        info->Effects[1].Effect = SPELL_EFFECT_DUMMY;
        info->Effects[2].MiscValue = 10;
    }
    if (id == 803719)
        dummy(2);
    if (id == 807435)
        dummy(2);
    if (id == 807749)
    {
        dummy(0);
        dummy(1);
    }
    if (id == 803492 || id == 807750 || id == 807751 || id == 807752 || id == 803500 || id == 807446 ||
        id == 805481 || id == 805491 || id == 681471)
        for (uint8 slot = 0; slot < MAX_SPELL_EFFECTS; ++slot)
            if (info->Effects[slot].Effect)
                dummy(slot);
    if (id == 803238)
    {
        // Spell.dbc gives the Sun Ray marker no effect, so casting it applied no aura for Refresh to find.
        aura(0, SPELL_AURA_DUMMY, 0, 0, TARGET_UNIT_CASTER);
        info->DurationEntry = sSpellDurationStore.LookupEntry(1);
    }
    if (id == 806118)
    {
        // Dawnfall's healing-received area names its allies in TargetA; the persistent area aura only
        // searches allies from TargetB, so the effect fell back to enemies.
        info->Effects[2].TargetA = SpellImplicitTargetInfo(TARGET_DEST_DEST);
        info->Effects[2].TargetB = SpellImplicitTargetInfo(TARGET_UNIT_DEST_AREA_ALLY);
    }
    if (id == 301242)
        info->Effects[0].SpellClassMask = flag96(0x8000, 0x40200, 0x4000);
    if (id == 680642)
        info->Effects[0].SpellClassMask = flag96(0, 512, 0);
    if (id == 300354)
        info->Effects[0].SpellClassMask = flag96(32768, 0, 0);
    if (id == 800722)
    {
        info->Effects[1].SpellClassMask = flag96(0, 512, 0);
        dummy(2);
    }
    if (id == 807299)
        for (auto& effect : info->Effects)
            effect.SpellClassMask = flag96(0, 2, 0);
    if (id == 301242)
        info->ProcCharges = 2;
    if (id == 301265)
        info->ProcCharges = 3;
    if (id == 560095)
        info->ProcCharges = 5;
    if (id == 680888)
        info->ProcCharges = 1;
    if (id == 503691)
    {
        info->Effects[0].Effect = SPELL_EFFECT_DUMMY;
        info->Effects[2].Effect = 0;
    }
    if (id == 681506 || id == 707517 || id == 300325 || id == 680618 || id == 806458)
        dummy(0);
    if (id == 300343)
    {
        dummy(0);
        dummy(1);
    }
    if (id == 707767)
        dummy(0);
    if (id == 681448)
        // Last Call's duration bonus ships keyed to the wrong mask word, so
        // it never reaches Valkyr's Calling; rekey to its family bit.
        info->Effects[0].SpellClassMask = flag96(0, 0, 0x200000);
    if (id == 680620)
        // By The Light's +50% crit ships keyed to the wrong mask word, so it
        // never reaches Valkyr's Calling; rekey to its family bit.
        info->Effects[0].SpellClassMask = flag96(0, 0x10, 0);
    if (id == 704932)
        // Heat Death extends the eclipse burst's hit debuff to 15 sec; the
        // shipped duration bonus is keyed to the wrong mask word, so rekey
        // to the burst's own family bit.
        info->Effects[0].SpellClassMask = flag96(0, 0, 0x2);
    if (id == 804032 || id == 680650)
        dummy(1);
    if (id == 300353)
    {
        dummy(1);
        dummy(2);
    }
    if (id == 805816)
        dummy(0);
    if (id == 300723)
        aura(2, SPELL_AURA_MOD_DAMAGE_PERCENT_DONE, 30, SPELL_SCHOOL_MASK_HOLY | SPELL_SCHOOL_MASK_FIRE, TARGET_UNIT_CASTER);
    if (id == 707776)
    {
        aura(0, SPELL_AURA_MOD_SHIELD_BLOCKVALUE, 0, 0, TARGET_UNIT_CASTER);
        info->DurationEntry = sSpellDurationStore.LookupEntry(21);
    }
    if (id == 301006)
        info->DurationEntry = sSpellDurationStore.LookupEntry(21);
    if (id == 301368)
        info->Effects[1].Effect = 0;
    if (id == 800602)
        dummy(1);
    if (id == 300344)
    {
        dummy(0);
        dummy(1);
    }
    if (id == 704909)
    {
        dummy(1); // Its native aura modifies critical damage, while the description specifies chance.
        aura(2, SPELL_AURA_MOD_HEALING_DONE_PERCENT, info->Effects[0].CalcValue(), 0, TARGET_UNIT_CASTER);
    }
    if (id == 807080)
        dummy(0);
    if (id == 680624)
        dummy(1);
    if (id == 520647)
        info->Effects[1].Effect = 0;
    if (id == 560347)
        aura(0, SPELL_AURA_SPLIT_DAMAGE_PCT, 30, SPELL_SCHOOL_MASK_ALL, TARGET_UNIT_TARGET_ALLY);
    if (id == 505340)
    {
        info->AttributesEx2 |= SPELL_ATTR2_CANT_CRIT;
        info->AttributesEx3 |= SPELL_ATTR3_IGNORE_CASTER_MODIFIERS;
        info->AttributesEx4 |= SPELL_ATTR4_IGNORE_DAMAGE_TAKEN_MODIFIERS;
        info->AscensionInheritsResolvedAmount = true;
    }
    if (Named(info, 500141))
        info->Effects[0].ChainTarget = 5; // Primary plus four additional allies from the active description.
    if (Named(info, 800231))
        for (auto& effect : info->Effects)
            if (effect.Effect == 168)
                effect.Effect = SPELL_EFFECT_DUMMY;
    if (id == 707768)
    {
        info->Effects[0].TargetA = SpellImplicitTargetInfo(TARGET_UNIT_CASTER);
        info->Effects[0].BasePoints = 9;
    }
    if (id == 520024 || id == 520026 || id == 807441 || id == 680630)
        for (auto& effect : info->Effects)
            if (effect.Effect)
                effect.Effect = SPELL_EFFECT_DUMMY;
    if (id == 680639)
        info->Effects[2].SpellClassMask = flag96(67108864, 0, 8388608);
    if (id == Bless)
    {
        info->Effects[0].ApplyAuraName = SPELL_AURA_PERIODIC_DUMMY;
        info->Effects[0].TriggerSpell = 0;
        dummy(2);
    }
    if (id == 704926)
        info->Effects[1].Effect = SPELL_EFFECT_DUMMY;
    if (id == 804252)
        for (auto& effect : info->Effects)
            if (effect.Effect)
            {
                effect.TargetA = SpellImplicitTargetInfo(TARGET_UNIT_TARGET_ENEMY);
                effect.TargetB = SpellImplicitTargetInfo();
            }
    for (uint32 talent : {561328,704585,805267,300314,707078,300363,300621,300626,300334,704935,680656})
        if (id == talent)
            for (uint8 slot = 0; slot < MAX_SPELL_EFFECTS; ++slot)
                if (info->Effects[slot].ApplyAuraName == SPELL_AURA_PERIODIC_TRIGGER_SPELL ||
                    info->Effects[slot].ApplyAuraName == SPELL_AURA_PERIODIC_TRIGGER_SPELL_WITH_VALUE)
                    dummy(slot);
    for (uint32 periodic : {300361,572752,704930,560123,570125})
        if (id == periodic)
        {
            uint8 slot = id == 570125 ? 2 : 0;
            if (id == 560123 || id == 570125)
                aura(slot, SPELL_AURA_PERIODIC_DUMMY, 0, 0, id == 570125 ? TARGET_UNIT_TARGET_ENEMY : TARGET_UNIT_CASTER);
            else
                info->Effects[slot].ApplyAuraName = SPELL_AURA_PERIODIC_DUMMY;
            info->Effects[slot].TriggerSpell = 0;
            if (id == 560123)
                info->Effects[slot].Amplitude = 3000;
            if (id == 570125)
                info->Effects[slot].Amplitude = 500;
        }
    info->_InitializeExplicitTargetMask();
}
} // namespace AscensionSunCleric
namespace
{
using namespace AscensionSunCleric;
class sun_cleric_scaling : public UnitScript
{
public:
    sun_cleric_scaling() : UnitScript("sun_cleric_scaling", true,
        {UNITHOOK_MODIFY_SPELL_EFFECT_BASE_VALUE, UNITHOOK_MODIFY_SPELL_DAMAGE_TAKEN,
         UNITHOOK_MODIFY_PERIODIC_DAMAGE_AURAS_TICK, UNITHOOK_MODIFY_MELEE_DAMAGE,
         UNITHOOK_ON_DAMAGE}) { }
    void ModifySpellEffectBaseValue(Unit const* caster, SpellInfo const* info, uint8 index, float& value) override
    {
        Player* player = Owner(caster);
        if (!player || !info || info->SpellFamilyName != 33 || Derived(info))
            return;
        SpellSchoolMask powerSchool = info->Id == 804752 ? SPELL_SCHOOL_MASK_HOLY : info->GetSchoolMask();
        for (auto const& row : SunClericCoefficients)
            if (row.spell == info->Id && row.effect == index)
                value += row.sp * std::max(0, player->SpellBaseDamageBonusDone(powerSchool)) +
                         row.ap * player->GetTotalAttackPowerValue(BASE_ATTACK) +
                         row.healing * std::max(0, player->SpellBaseHealingBonusDone(info->GetSchoolMask())) +
                         row.stamina * player->GetStat(STAT_STAMINA) + row.intellect * player->GetStat(STAT_INTELLECT) +
                         row.strength * player->GetStat(STAT_STRENGTH);
        if (Named(info, 500154) && !index && player->HasAura(680656))
            value += .25f * player->GetTotalAttackPowerValue(BASE_ATTACK); // Explicit local AP policy.
        if (Named(info, 800654) && !index)
            value += player->GetShieldBlockValue();
        if (info->Id == 807064 && !index)
            value *= 1 + State(player).sunchargeStacks * .25f;
        if (idIsJustice(info) && index == 1)
        {
            // Both authored hand hits scale from Fire spell power even when the main hand is Physical.
            value += .2f * (player->SpellBaseDamageBonusDone(SPELL_SCHOOL_MASK_FIRE) -
                            player->SpellBaseDamageBonusDone(info->GetSchoolMask()));
        }
        value = std::clamp(value, float(INT32_MIN / 2), float(INT32_MAX / 2));
    }
    bool idIsJustice(SpellInfo const* info)
    {
        return Any(info, {524857,525005,525006,525007,572849,572850,572040,572041,572042,572043,572044,572045});
    }
    float Factor(Unit* target, Unit* caster, SpellInfo const* info)
    {
        Player* player = Owner(caster);
        if (!player || !target || !info || info->SpellFamilyName != 33 || Derived(info))
            return 1;
        float factor = 1;
        if ((info->SchoolMask & SPELL_SCHOOL_MASK_FIRE) && player->HasAura(806458))
        {
            bool burning = false;
            for (auto const& pair : target->GetAppliedAuras())
                if (auto* aura = pair.second->GetBase(); aura->GetSpellInfo()->SchoolMask & SPELL_SCHOOL_MASK_FIRE)
                    if (aura->HasEffectType(SPELL_AURA_PERIODIC_DAMAGE))
                        burning = true;
            if (burning)
                factor *= 1 + Amount(806458) / 100.0f;
        }
        if (info->Id == 807058 && target->GetHealthPct() < 35 && player->HasAura(300325))
            factor *= 1 + Amount(300325) / 100.0f;
        return factor;
    }
    void ModifySpellDamageTaken(Unit* target, Unit* caster, int32& damage, SpellInfo const* info) override
    {
        damage = int32(damage * Factor(target, caster, info));
    }
    void ModifyPeriodicDamageAurasTick(Unit* target, Unit* caster, uint32& damage, SpellInfo const* info) override
    {
        if (!info || !info->HasAura(SPELL_AURA_PERIODIC_HEAL))
            damage = uint32(damage * Factor(target, caster, info));
    }
    void ModifyMeleeDamage(Unit*, Unit* caster, uint32& damage) override
    {
        Player* player = Owner(caster);
        if (player == caster && player && player->HasAura(807749))
            AddPct(damage, Amount(807749, 1));
    }
    void OnDamage(Unit*, Unit* victim, uint32& damage) override
    {
        Player* player = Owner(victim);
        if (player && player == victim && player->HasAura(805816) &&
            int64(player->GetHealth()) - damage < int64(player->CountPctFromMaxHealth(35)))
            damage -= CalculatePct(damage, Amount(805816));
        if (player && player == victim && int64(player->GetHealth()) - damage < int64(player->CountPctFromMaxHealth(10)))
            if (DynamicObject* zone = player->GetDynObject(520647))
                zone->Remove();
    }
};
}
void AddSC_AscensionSunClericContracts()
{
    new sun_cleric_scaling();
}
