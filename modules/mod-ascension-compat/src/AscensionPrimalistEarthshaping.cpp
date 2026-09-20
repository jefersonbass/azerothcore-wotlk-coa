/* Copyright (C) 2016+ AzerothCore, GNU AGPL v3. */

#include "AscensionPrimalistEarthshaping.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SpellAuraEffects.h"
#include "SpellAuras.h"
#include "SpellMgr.h"
#include "SpellScript.h"
#include <array>

namespace
{
constexpr uint32 PRIMALIST_FAMILY = uint32(CLASS_WILDWALKER) + 6;
constexpr uint32 SPELL_EARTHSHAPING = 680441;
constexpr uint32 SPELL_STONESHARD_MODIFIER = 680846;
constexpr uint32 SPELL_EARTHQUAKE_MODIFIER = 532562;
constexpr uint32 SPELL_ERUPTION_MODIFIER = 680450;
constexpr uint32 SPELL_BLESSING_OF_THERAZANE = 680439;
constexpr std::array<uint32, 3> EARTHSHAPING_HELPERS =
    {SPELL_STONESHARD_MODIFIER, SPELL_EARTHQUAKE_MODIFIER, SPELL_ERUPTION_MODIFIER};

bool IsEarthshapingOwner(Unit* owner)
{
    return owner && owner->IsPlayer() && owner->getClass() == CLASS_WILDWALKER;
}

void SynchronizeEarthshapingHelpers(Unit* owner, bool remove)
{
    if (!IsEarthshapingOwner(owner))
        return;

    Aura const* main = remove ? nullptr : owner->GetAura(SPELL_EARTHSHAPING, owner->GetGUID());
    for (uint32 spellId : EARTHSHAPING_HELPERS)
    {
        if (!main)
        {
            owner->RemoveAurasDueToSpell(spellId, owner->GetGUID());
            continue;
        }

        Aura* helper = owner->GetAura(spellId, owner->GetGUID());
        if (!helper)
            helper = owner->AddAura(spellId, owner);
        if (helper)
        {
            // SetStackAmount also recalculates Blessing's effect-2 modifier.
            // The visible aura alone owns expiry, including no-refresh gains.
            helper->SetStackAmount(main->GetStackAmount());
            helper->SetDuration(-1);
        }
    }
}

bool ValidateEarthshapingHelpers()
{
    SpellInfo const* stoneshard = sSpellMgr->GetSpellInfo(SPELL_STONESHARD_MODIFIER);
    SpellInfo const* earthquake = sSpellMgr->GetSpellInfo(SPELL_EARTHQUAKE_MODIFIER);
    SpellInfo const* eruption = sSpellMgr->GetSpellInfo(SPELL_ERUPTION_MODIFIER);
    if (!stoneshard || !earthquake || !eruption)
        return false;

    for (SpellInfo const* helper : {stoneshard, earthquake, eruption})
        if (!helper->HasAttribute(SPELL_ATTR0_CU_AURA_CANNOT_BE_SAVED) ||
            helper->HasAttribute(SPELL_ATTR0_CU_FORCE_AURA_SAVING))
            return false;

    // Refuse the old wildcard damage modifier if the metadata correction was
    // not installed. Every other effect keeps its native calculation path.
    return stoneshard->Effects[EFFECT_0].IsAura(SPELL_AURA_DUMMY) &&
        stoneshard->Effects[EFFECT_1].IsAura(SPELL_AURA_ADD_PCT_MODIFIER) &&
        stoneshard->Effects[EFFECT_1].SpellClassMask == flag96(0, 512, 0) &&
        earthquake->Effects[EFFECT_1].IsAura(SPELL_AURA_MOD_CRIT_DAMAGE_BONUS) &&
        earthquake->Effects[EFFECT_2].IsAura(SPELL_AURA_ADD_PCT_MODIFIER) &&
        earthquake->Effects[EFFECT_2].MiscValue == SPELLMOD_DOT &&
        earthquake->Effects[EFFECT_2].SpellClassMask == flag96(0, 0, 268435456) &&
        eruption->Effects[EFFECT_2].IsAura(SPELL_AURA_ADD_FLAT_MODIFIER) &&
        eruption->Effects[EFFECT_2].MiscValue == SPELLMOD_DURATION &&
        eruption->Effects[EFFECT_2].SpellClassMask == flag96(0, 0, 2097152);
}

class spell_ascension_primalist_earthshaping : public AuraScript
{
    PrepareAuraScript(spell_ascension_primalist_earthshaping);

    bool Validate(SpellInfo const* spellInfo) override
    {
        return spellInfo && spellInfo->Id == SPELL_EARTHSHAPING &&
            spellInfo->SpellFamilyName == PRIMALIST_FAMILY &&
            spellInfo->Effects[EFFECT_0].IsAura(SPELL_AURA_HASTE_SPELLS) && ValidateEarthshapingHelpers();
    }

    bool Load() override
    {
        return IsEarthshapingOwner(GetTarget()) && GetCasterGUID() == GetTarget()->GetGUID();
    }

    void Apply(AuraEffect const*, AuraEffectHandleModes)
    {
        SynchronizeEarthshapingHelpers(GetTarget(), false);
    }

    void Remove(AuraEffect const*, AuraEffectHandleModes)
    {
        SynchronizeEarthshapingHelpers(GetTarget(), true);
    }

    void Register() override
    {
        AfterEffectApply += AuraEffectApplyFn(spell_ascension_primalist_earthshaping::Apply,
            EFFECT_0, SPELL_AURA_HASTE_SPELLS, AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK);
        AfterEffectRemove += AuraEffectRemoveFn(spell_ascension_primalist_earthshaping::Remove,
            EFFECT_0, SPELL_AURA_HASTE_SPELLS, AURA_EFFECT_HANDLE_REAL);
    }
};

class spell_ascension_primalist_therazane_update : public AuraScript
{
    PrepareAuraScript(spell_ascension_primalist_therazane_update);

    bool Validate(SpellInfo const* spellInfo) override
    {
        return spellInfo && spellInfo->Id == SPELL_BLESSING_OF_THERAZANE &&
            spellInfo->SpellFamilyName == PRIMALIST_FAMILY &&
            spellInfo->Effects[EFFECT_0].IsAura(SPELL_AURA_ADD_FLAT_MODIFIER) &&
            spellInfo->Effects[EFFECT_0].MiscValue == SPELLMOD_EFFECT2 &&
            spellInfo->Effects[EFFECT_0].SpellClassMask == flag96(0, 65536, 0) &&
            ValidateEarthshapingHelpers();
    }

    bool Load() override
    {
        return IsEarthshapingOwner(GetTarget()) && GetCasterGUID() == GetTarget()->GetGUID();
    }

    void Update(AuraEffect const*, AuraEffectHandleModes)
    {
        SynchronizeEarthshapingHelpers(GetTarget(), false);
    }

    void Register() override
    {
        AfterEffectApply += AuraEffectApplyFn(spell_ascension_primalist_therazane_update::Update,
            EFFECT_0, SPELL_AURA_ADD_FLAT_MODIFIER, AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK);
        AfterEffectRemove += AuraEffectRemoveFn(spell_ascension_primalist_therazane_update::Update,
            EFFECT_0, SPELL_AURA_ADD_FLAT_MODIFIER, AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK);
    }
};
}

void ApplyAscensionPrimalistEarthshapingContracts(SpellInfo* spellInfo)
{
    if (!spellInfo || spellInfo->SpellFamilyName != PRIMALIST_FAMILY)
        return;

    for (uint32 spellId : EARTHSHAPING_HELPERS)
        if (spellInfo->Id == spellId && spellInfo->StackAmount == 15)
            spellInfo->AttributesCu |= SPELL_ATTR0_CU_AURA_CANNOT_BE_SAVED;

    if (spellInfo->Id == 706137)
    {
        // Keeper of the Grove's native cooldown modifier would trim 30 sec
        // from every spell the owner casts; the Bramblepatch-only trim is
        // scripted in the cast hook instead.
        spellInfo->Effects[EFFECT_0].Effect = SPELL_EFFECT_APPLY_AURA;
        spellInfo->Effects[EFFECT_0].ApplyAuraName = SPELL_AURA_DUMMY;
        return;
    }
    if (spellInfo->Id == 706165)
    {
        // Issue 1039: Fury of the Elements ships without the passive flag, so
        // the learn/login passes never applied its aura, and its mask is
        // empty, which would double every Primalist energize. Mark passive
        // and key to Primal Rush's own family bit; the native flat-mod path
        // then grants the authored +10 Rage.
        spellInfo->Attributes |= SPELL_ATTR0_PASSIVE;
        spellInfo->Effects[EFFECT_0].SpellClassMask = flag96(0x4000, 0, 0);
        return;
    }
    if (spellInfo->Id == 504220)
    {
        // Issue 969: Spiritual Warrior ships without the passive flag, so the
        // learn/login passes never applied its crit aura, and its mask is
        // keyed to the wrong word, missing Spirit Charge's own family bit.
        // Mark passive and rekey; the native crit-chance mod path then grants
        // the authored +25%.
        spellInfo->Attributes |= SPELL_ATTR0_PASSIVE;
        spellInfo->Effects[EFFECT_0].SpellClassMask = flag96(0, 0x20000, 0);
        return;
    }

    if (spellInfo->Id == 504215)
    {
        // Issue 987: Vitality Surge's haste buff ships as a self-only instant
        // with no duration; the authored mark is +5% haste on party and raid
        // members for 20 sec, so retarget to the caster's raid and set the
        // 20-second duration. Both haste halves are native.
        for (uint8 slot : {EFFECT_0, EFFECT_1})
            spellInfo->Effects[slot].TargetA = SpellImplicitTargetInfo(TARGET_UNIT_CASTER_AREA_RAID);
        spellInfo->DurationEntry = sSpellDurationStore.LookupEntry(18); // Twenty seconds.
        return;
    }

    if (spellInfo->Id != SPELL_STONESHARD_MODIFIER)
        return;

    SpellEffectInfo& effect = spellInfo->Effects[EFFECT_0];
    if (effect.Effect == SPELL_EFFECT_APPLY_AURA &&
        (effect.ApplyAuraName == SPELL_AURA_ADD_PCT_MODIFIER || effect.ApplyAuraName == SPELL_AURA_DUMMY) &&
        effect.BasePoints == 1 && effect.DieSides == 1 && effect.MiscValue == SPELLMOD_DAMAGE &&
        effect.SpellClassMask == flag96(0, 0, 0))
        effect.ApplyAuraName = SPELL_AURA_DUMMY;
}

class vitality_surge_heals : public UnitScript
{
public:
    vitality_surge_heals() : UnitScript("vitality_surge_heals", true, {UNITHOOK_ON_HEAL}) { }

    void OnHeal(Unit* healer, Unit*, uint32& gain) override
    {
        // Vitality Surge (504214): effective healing rolls 25% to raise party
        // and raid haste 5% for 20 sec through the authored buff (504215).
        Player* player = healer ? healer->ToPlayer() : nullptr;
        if (!player || player->getClass() != CLASS_WILDWALKER || !gain ||
            !player->HasAura(504214) || !roll_chance_i(25))
            return;
        player->CastSpell(player, 504215, true);
    }
};

void AddSC_AscensionPrimalistEarthshaping()
{
    new vitality_surge_heals();
    RegisterSpellScript(spell_ascension_primalist_earthshaping);
    RegisterSpellScript(spell_ascension_primalist_therazane_update);
}
