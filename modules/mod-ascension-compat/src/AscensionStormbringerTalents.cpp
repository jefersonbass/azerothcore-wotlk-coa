/* Copyright (C) 2016+ AzerothCore, GNU AGPL v3. */
#include "Pet.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "Spell.h"
#include "SpellAuraEffects.h"
#include "SpellAuras.h"
#include "SpellMgr.h"
#include "SpellScript.h"

namespace
{
enum StormbringerTalentSpells : uint32
{
    SPELL_CLOUDBURST = 801838,
    SPELL_CLOUDBURST_KNOCKBACK = 802385,
    SPELL_SHOCK = 804020,
    SPELL_SHOCK_DOT = 560336,
    SPELL_PERPETUAL_SHOCK = 570054,
    SPELL_CALL_LIGHTNING = 500040,
    SPELL_THUNDER_WARD = 800098,
    SPELL_STATIC = 803102,
    SPELL_GENERATE_STATIC_20 = 804086,
    SPELL_BAROMETRIC_SLOW = 803566,
    SPELL_ELECTRICAL_CHARGE = 800299,
    SPELL_CHARGED_CONDUIT = 803790,
    SPELL_GALE = 804036,
    SPELL_ENVELOPING_WINDS = 707546,
    SPELL_AEROMANCY = 705708,
    SPELL_ELEMENTAL_UPDRAFT = 807717,
    SPELL_UPDRAFT = 570161,
    SPELL_STORM_ASCENDANCE = 681110,
    SPELL_TEMPEST_SOVEREIGN = 560020,
    SPELL_SHOCK_STATIC_GRANT = 500039,
    SPELL_TORRENTIAL_WRATH = 503352,
    SPELL_CONDUCTION = 567560,
    SPELL_UNDERTOW = 705666,
    SPELL_UNDERTOW_RANK_2 = 707796,
    SPELL_DROWN_HIT = 806408,
    SPELL_ELECTROCUTIONER_PASSIVE = 500068,
    SPELL_ELECTROCUTIONER_TALENT = 92096,
    SPELL_ELECTROCUTIONER = 804592,
    SPELL_VOLTAIC_MASTERY = 706661,
    SPELL_CONDUCTIVE = 567559,
    SPELL_DELUGE = 806400,
    SPELL_DELUGE_BONUS = 806399,
    SPELL_TITANSTORM = 801869,
    SPELL_TITANSTORM_COOLDOWN = 801854
};

// The passive's tooltip gives a base chance (92096 carries 5%) and says it grows with
// Static. No public record has the rate, so each Static adds a fifth of a percent.
uint32 ElectrocutionerChance(Player const* player)
{
    SpellInfo const* talent = sSpellMgr->GetSpellInfo(SPELL_ELECTROCUTIONER_TALENT);
    Aura const* staticAura = player->GetAura(SPELL_STATIC);
    return (talent ? talent->ProcChance : 0) + (staticAura ? staticAura->GetStackAmount() / 5 : 0);
}

class stormbringer_talent_casts : public AllSpellScript
{
public:
    stormbringer_talent_casts() : AllSpellScript("stormbringer_talent_casts",
        {ALLSPELLHOOK_ON_CAST, ALLSPELLHOOK_ON_HIT_RESULT, ALLSPELLHOOK_ON_CRIT_CHANCE}) { }

    void OnSpellCast(Spell* spell, Unit* caster, SpellInfo const* info, bool) override
    {
        Player* player = caster ? caster->ToPlayer() : nullptr;
        if (player && player->getClass() == CLASS_STORMBRINGER && info->SpellFamilyName == 22 &&
            info->Id == SPELL_CLOUDBURST && !spell->IsTriggered())
            // The active spell has a zero-radius dummy. Its separate native
            // helper supplies the ten-yard area and authored knockback speeds.
            player->CastSpell(player, SPELL_CLOUDBURST_KNOCKBACK, true);
        // Enveloping Winds (707546): casting Gale makes the Air Elemental cast Gale
        // as well. The passive's Dummy aura is inert; the armor emanation is native
        // through its periodic trigger into the raid area aura.
        if (player && player->getClass() == CLASS_STORMBRINGER && info->SpellFamilyName == 22 &&
            !spell->IsTriggered() && player->HasAura(SPELL_ENVELOPING_WINDS) &&
            sSpellMgr->GetFirstSpellInChain(info->Id) == SPELL_GALE)
            if (Pet* pet = player->GetPet(); pet && pet->IsAlive() && pet->IsInWorld())
                if (Unit* victim = pet->GetVictim())
                    pet->CastSpell(victim, info->Id, true);
        // Aeromancy (705708): casting Updraft makes the Air Elemental cast its
        // own Updraft beneath the caster; the +25% damage half is native
        // through the aura's family mask.
        if (player && player->getClass() == CLASS_STORMBRINGER && info->SpellFamilyName == 22 &&
            !spell->IsTriggered() && player->HasAura(SPELL_AEROMANCY) &&
            sSpellMgr->GetFirstSpellInChain(info->Id) == SPELL_UPDRAFT)
            if (Pet* pet = player->GetPet(); pet && pet->IsAlive() && pet->IsInWorld())
                pet->CastSpell(player, SPELL_ELEMENTAL_UPDRAFT, true);
        // Storm Ascendance (681110): damaging spells generate 10 extra Static,
        // at most once per second, while the transform lasts.
        if (player && player->getClass() == CLASS_STORMBRINGER && info->SpellFamilyName == 22 &&
            !spell->IsTriggered() && player->HasAura(SPELL_STORM_ASCENDANCE) &&
            info->DmgClass != SPELL_DAMAGE_CLASS_NONE && info->DmgClass != SPELL_DAMAGE_CLASS_MELEE)
        {
            uint32 now = getMSTime();
            if (now - spell->GetScriptValue(SPELL_STORM_ASCENDANCE) >= 1000)
            {
                spell->SetScriptValue(SPELL_STORM_ASCENDANCE, now);
                player->CastSpell(player, SPELL_GENERATE_STATIC_20, true);
                player->CastSpell(player, SPELL_GENERATE_STATIC_20, true);
            }
        }
        // Tempest Sovereign (560020): Shock and Call Lightning gain 25 Static, and
        // Torrential Wrath consumes all Static, triggering Conduction per stack.
        if (!player || player->getClass() != CLASS_STORMBRINGER || info->SpellFamilyName != 22 ||
            spell->IsTriggered() || !player->HasAura(SPELL_TEMPEST_SOVEREIGN))
            return;
        uint32 const root = sSpellMgr->GetFirstSpellInChain(info->Id);
        if (root == SPELL_SHOCK || root == SPELL_SHOCK_STATIC_GRANT || root == SPELL_CALL_LIGHTNING)
        {
            if (Aura* staticAura = player->GetAura(SPELL_STATIC))
                staticAura->ModStackAmount(25);
            return;
        }
        if (root == SPELL_TORRENTIAL_WRATH)
        {
            Aura* staticAura = player->GetAura(SPELL_STATIC);
            if (!staticAura)
                return;
            uint8 const stacks = staticAura->GetStackAmount();
            staticAura->Remove();
            for (uint8 i = 0; i < stacks; ++i)
                player->CastSpell(spell->m_targets.GetUnitTarget(), SPELL_CONDUCTION, true);
        }
    }

    void OnSpellCritChance(Spell* spell, Unit* target, float& chance) override
    {
        // Undertow: Drown's burst component crits more often, 25% at rank 1
        // (705666) and 50% at rank 2 (707796). The authored bonus lives on
        // the talent's crit effect, so it is read from the aura, not hardcoded.
        Player* player = spell->GetCaster() ? spell->GetCaster()->ToPlayer() : nullptr;
        SpellInfo const* info = spell->GetSpellInfo();
        if (!player || info->Id != SPELL_DROWN_HIT)
            return;
        for (uint32 undertow : {SPELL_UNDERTOW, SPELL_UNDERTOW_RANK_2})
            if (AuraEffect const* crit = player->GetAuraEffect(undertow, EFFECT_1))
            {
                chance += float(crit->GetAmount());
                return;
            }
    }

    void OnSpellHitResult(Spell* spell, Unit* target, uint8 miss, uint32 damage, uint32, bool) override
    {
        Player* player = spell->GetCaster()->ToPlayer();
        SpellInfo const* info = spell->GetSpellInfo();
        if (!player || player->getClass() != CLASS_STORMBRINGER || info->SpellFamilyName != 22 ||
            !target || target == player || player->IsFriendlyTo(target) || miss != SPELL_MISS_NONE)
            return;

        // Neither passive carries proc flags, so the spell-damage event is supplied here.
        if (damage && !spell->IsTriggered() &&
            (player->HasSpell(SPELL_ELECTROCUTIONER_PASSIVE) || player->HasSpell(SPELL_ELECTROCUTIONER_TALENT)) &&
            roll_chance_i(ElectrocutionerChance(player)))
            player->CastSpell(player, SPELL_ELECTROCUTIONER, true);

        bool repeat =info->Id == SPELL_PERPETUAL_SHOCK;
        if (!repeat && (spell->IsTriggered() || sSpellMgr->GetFirstSpellInChain(info->Id) != SPELL_SHOCK))
            return;

        // Two native half-second ticks each copy ten percent of the resolved hit.
        // The damage-over-time component does not require Call Lightning.
        if (damage && !spell->GetScriptValue(SPELL_SHOCK_DOT))
        {
            spell->SetScriptValue(SPELL_SHOCK_DOT, 1);
            player->CastCustomSpell(SPELL_SHOCK_DOT, SPELLVALUE_BASE_POINT0, int32(damage / 10), target, true);
        }
        if (player->HasSpell(SPELL_CALL_LIGHTNING) && !player->HasAura(SPELL_THUNDER_WARD) &&
            !spell->GetScriptValue(SPELL_STATIC))
        {
            spell->SetScriptValue(SPELL_STATIC, 1);
            player->CastSpell(player, SPELL_GENERATE_STATIC_20, true);
        }
        // Issue 819: Voltaic Mastery increases Deluge damage by 5% per
        // Conductive stack. The talent's native flat-mod auras carry empty
        // masks and no stack scaling, so deal the bonus here: on any
        // successful Deluge hit (chain head 806400, family-22 flag 0x8000),
        // read the caster's Conductive (567559) stacks and strike the target
        // for 5% of the resolved damage per stack as bonus Nature damage.
        if (sSpellMgr->GetFirstSpellInChain(info->Id) == SPELL_DELUGE &&
            player->HasAura(SPELL_VOLTAIC_MASTERY) && !spell->GetScriptValue(SPELL_VOLTAIC_MASTERY))
        {
            spell->SetScriptValue(SPELL_VOLTAIC_MASTERY, 1);
            if (Aura* conductive = player->GetAura(SPELL_CONDUCTIVE))
                if (uint8 stacks = conductive->GetStackAmount())
                    player->CastCustomSpell(SPELL_DELUGE_BONUS, SPELLVALUE_BASE_POINT0,
                        int32(damage * stacks * 5 / 100), target, true);
        }
    }
};

class stormbringer_resource_contracts : public GlobalScript
{
public:
    stormbringer_resource_contracts() : GlobalScript("stormbringer_resource_contracts",
        {GLOBALHOOK_ON_LOAD_SPELL_CUSTOM_ATTR}) { }

    void OnLoadSpellCustomAttr(SpellInfo* info) override
    {
        if (!info || info->SpellFamilyName != 22)
            return;
        if (info->Id == SPELL_SHOCK_DOT)
        {
            info->AttributesEx2 |= SPELL_ATTR2_CANT_CRIT;
            info->AttributesEx3 |= SPELL_ATTR3_IGNORE_CASTER_MODIFIERS;
            info->AttributesEx4 |= SPELL_ATTR4_IGNORE_DAMAGE_TAKEN_MODIFIERS;
            info->AscensionInheritsResolvedAmount = true;
            info->Effects[EFFECT_0].BonusMultiplier = 0.0f;
        }
        if (info->Id == SPELL_PERPETUAL_SHOCK)
            // The hit callback supplies the learned-spell gate and one 20-Static grant.
            info->Effects[EFFECT_1].Effect = 0;
        if (info->Id == 560568)
            // Issue 992: Electrifying Aura ships without the passive flag, so
            // the learn/login passes never applied its raid aura (65 =
            // APPLY_AREA_AURA_RAID, resolving the tooltip's +3% party/raid
            // crit through the native crit-pct path).
            info->Attributes |= SPELL_ATTR0_PASSIVE;
        if (info->Id == 705655)
        {
            // Issue 1027: Stormy Days ships without the passive flag, so the
            // learn/login passes never applied its tick-rate aura, and its
            // mask is keyed to the wrong word, missing Conjure Storm's own
            // family bits. Mark passive and rekey; the native
            // activation-time mod path then quickens the authored 25%.
            info->Attributes |= SPELL_ATTR0_PASSIVE;
            info->Effects[EFFECT_0].SpellClassMask = flag96(0, 0, 0x30);
        }
        if (info->Id == SPELL_TITANSTORM)
        {
            // Issue 686: Titanstorm ships without SPELL_ATTR0_PASSIVE, so the
            // learn/login passes never applied its proc aura. Effect 0 is the
            // guaranteed (ProcChance 100, proc flags 0x50000) proc into the
            // cooldown reducer 801854 on Call Lightning and Electrocute.
            info->Attributes |= SPELL_ATTR0_PASSIVE;
        }
        if (info->Id == SPELL_TITANSTORM_COOLDOWN)
        {
            // The reducer's two ASCENSION_MODIFY_COOLDOWN effects carry
            // display-minus-1 BasePoints with DieSides 0, which would reduce
            // Arm of Thorim (801847) and Lightning Cage (560030) by 1501 ms
            // instead of the tooltip's 1.5 sec. Shift DieSides to 1.
            info->Effects[EFFECT_0].DieSides = 1;
            info->Effects[EFFECT_1].DieSides = 1;
        }
        if (info->Id == SPELL_STORM_ASCENDANCE)
        {
            // "Transform into a storm elemental for 15 sec, increasing your
            // Magic damage dealt by 20% and reducing your Magic damage taken
            // by 50%." The damage-dealt half is native; the dead flat-mod
            // slot becomes the taken half, and the instant record becomes the
            // authored 15-second transform.
            info->DurationEntry = sSpellDurationStore.LookupEntry(8); // Fifteen seconds.
            info->Effects[EFFECT_2].ApplyAuraName = SPELL_AURA_MOD_DAMAGE_PERCENT_TAKEN;
            info->Effects[EFFECT_2].MiscValue = SPELL_SCHOOL_MASK_MAGIC;
            info->Effects[EFFECT_2].BasePoints = -51;
            info->Effects[EFFECT_2].DieSides = 1;
        }
        if (info->Id == SPELL_CHARGED_CONDUIT)
            // Keep the charges until this ten-second buff ends.
            info->Effects[EFFECT_2].Effect = 0;
        if (info->Id == 806397)
        {
            // Issue 842: Bursting ships without SPELL_ATTR0_PASSIVE, so the
            // learn/login passes never applied its Drowning mod, and its
            // BasePoints are display-minus-1 with DieSides 0. Mark passive
            // and shift DieSides to 1 so the value resolves as the tooltip's
            // 20% (op 3 = SPELLMOD_FLAT, maskC 0x40000 matches both Drowning
            // debuffs 806406/806491's family-22 flag 0x40000). The native
            // flat-mod path applies it to the Drowning stack value.
            info->Attributes |= SPELL_ATTR0_PASSIVE;
            info->Effects[EFFECT_0].DieSides = 1;
        }
        if (info->Id == 705657)
        {
            // Issue 864: Studious ships without SPELL_ATTR0_PASSIVE, so the
            // learn/login passes never applied its cast-time mod, and its
            // BasePoints are display-minus-1 with DieSides 0. Mark passive
            // and shift DieSides to 1 so the value resolves as the tooltip's
            // 15% (op 10 = SPELLMOD_CASTING_TIME, maskA 0x800 matches Shock
            // 503326's family-22 flag 0x800). The native casting-time mod
            // path applies it.
            info->Attributes |= SPELL_ATTR0_PASSIVE;
            info->Effects[EFFECT_0].DieSides = 1;
        }
}
};

class aura_ascension_barometric_pressure : public AuraScript
{
    PrepareAuraScript(aura_ascension_barometric_pressure);

    bool Validate(SpellInfo const*) override { return ValidateSpellInfo({SPELL_BAROMETRIC_SLOW}); }

    void Apply(AuraEffect const*, AuraEffectHandleModes)
    {
        // Only the owner's dummy effect may start the companion area aura.
        // Starting it on each hostile recipient would create extra area sources.
        if (GetCaster() && GetCaster() == GetTarget())
            GetCaster()->AddAura(SPELL_BAROMETRIC_SLOW, GetTarget());
    }

    void OnRemove(AuraEffect const*, AuraEffectHandleModes)
    {
        if (GetCasterGUID() == GetTarget()->GetGUID())
            GetTarget()->RemoveAurasDueToSpell(SPELL_BAROMETRIC_SLOW, GetCasterGUID());
    }

    void Register() override
    {
        AfterEffectApply += AuraEffectApplyFn(aura_ascension_barometric_pressure::Apply,
            EFFECT_1, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
        AfterEffectRemove += AuraEffectRemoveFn(aura_ascension_barometric_pressure::OnRemove,
            EFFECT_1, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
    }
};

class aura_ascension_electrical_charge : public AuraScript
{
    PrepareAuraScript(aura_ascension_electrical_charge);

    bool Validate(SpellInfo const*) override
    {
        return ValidateSpellInfo({SPELL_ELECTRICAL_CHARGE, SPELL_CHARGED_CONDUIT});
    }

    void Tick(AuraEffect const*)
    {
        PreventDefaultAction();
        Unit* owner = GetTarget();
        if (owner->isMoving() || owner->HasAura(SPELL_CHARGED_CONDUIT))
            return;
        owner->CastSpell(owner, SPELL_ELECTRICAL_CHARGE, true);
        if (Aura* charges = owner->GetAura(SPELL_ELECTRICAL_CHARGE))
            if (charges->GetStackAmount() >= sSpellMgr->GetSpellInfo(SPELL_ELECTRICAL_CHARGE)->StackAmount)
                owner->CastSpell(owner, SPELL_CHARGED_CONDUIT, true);
    }

    void OnRemove(AuraEffect const*, AuraEffectHandleModes)
    {
        GetTarget()->RemoveAurasDueToSpell(SPELL_CHARGED_CONDUIT);
        GetTarget()->RemoveAurasDueToSpell(SPELL_ELECTRICAL_CHARGE);
    }

    void Register() override
    {
        OnEffectPeriodic += AuraEffectPeriodicFn(aura_ascension_electrical_charge::Tick,
            EFFECT_0, SPELL_AURA_PERIODIC_TRIGGER_SPELL);
        AfterEffectRemove += AuraEffectRemoveFn(aura_ascension_electrical_charge::OnRemove,
            EFFECT_0, SPELL_AURA_PERIODIC_TRIGGER_SPELL, AURA_EFFECT_HANDLE_REAL);
    }
};

class aura_ascension_charged_conduit : public AuraScript
{
    PrepareAuraScript(aura_ascension_charged_conduit);

    void OnRemove(AuraEffect const*, AuraEffectHandleModes)
    {
        GetTarget()->RemoveAurasDueToSpell(SPELL_ELECTRICAL_CHARGE);
    }

    void Register() override
    {
        AfterEffectRemove += AuraEffectRemoveFn(aura_ascension_charged_conduit::OnRemove,
            EFFECT_1, SPELL_AURA_HASTE_SPELLS, AURA_EFFECT_HANDLE_REAL);
    }
};
}

void AddSC_AscensionStormbringerTalents()
{
    new stormbringer_talent_casts();
    new stormbringer_resource_contracts();
    RegisterSpellScript(aura_ascension_barometric_pressure);
    RegisterSpellScript(aura_ascension_electrical_charge);
    RegisterSpellScript(aura_ascension_charged_conduit);
}
