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
enum PrimalistAbilitySpells : uint32
{
    SPELL_GEODE_BARRAGE_DAMAGE = 803138,
    SPELL_GEODE_BARRAGE_RAGE = 802885,
    SPELL_LEGACY_OF_REXXAR = 800184,
    SPELL_BONES_MARK = 806552,
    SPELL_BONES_STACK = 806554,
    SPELL_BONES_DAMAGE = 806553,
    SPELL_PRIMAL_SHRED = 500940,
    SPELL_RYLAKS_BITE = 706342,
    SPELL_WILDCLAW = 800140,
    SPELL_MISHAS_RAGE = 504227,
    SPELL_LEOKKS_FURY = 560974,
    SPELL_HUFFERS_SPEED = 560973
};

Player* Primalist(Unit* unit)
{
    Player* player = unit ? unit->ToPlayer() : nullptr;
    return player && player->getClass() == CLASS_WILDWALKER ? player : nullptr;
}

class primalist_talent_events : public UnitScript
{
public:
    primalist_talent_events() : UnitScript("primalist_talent_events", true,
        {UNITHOOK_MODIFY_SPELL_DAMAGE_TAKEN, UNITHOOK_ON_DAMAGE, UNITHOOK_ON_AURA_REMOVE}) { }

    void ModifySpellDamageTaken(Unit* target, Unit* attacker, int32& damage, SpellInfo const* spellInfo) override
    {
        // Bring Me Their Bones (806552): pet ability damage on the marked target
        // stacks the debuff; at 5 stacks the pet consumes it for armor-ignoring damage.
        if (!target || !attacker || attacker == target || !spellInfo || !damage)
            return;
        Unit* ownerUnit = attacker->GetOwner();
        if (!ownerUnit || ownerUnit->ToPlayer())
            return;
        Player* player = Primalist(ownerUnit);
        if (!player || !player->IsAlive() || attacker->GetOwnerGUID() != player->GetGUID())
            return;
        Aura* mark = target->GetAura(SPELL_BONES_STACK, player->GetGUID());
        if (!mark)
            return;
        if (mark->GetStackAmount() >= 5)
        {
            mark->Remove();
            attacker->CastSpell(target, SPELL_BONES_DAMAGE, true);
        }
        else
            mark->ModStackAmount(1);
    }

    void OnDamage(Unit*, Unit* victim, uint32& damage) override
    {
        Player* player = Primalist(victim);
        if (!player || !player->IsAlive() || !damage || damage < player->GetHealth() ||
            !player->HasAura(560157) || player->HasSpellCooldown(560157))
            return;
        // DealDamage reaches this hook after mitigation and absorption. Mark the
        // native saved cooldown before casting the heal, including any nested events.
        player->AddSpellCooldown(560157, 0, 120000);
        damage = 0;
        player->CastSpell(player, 560179, true);
    }

    void OnAuraRemove(Unit* unit, AuraApplication* application, AuraRemoveMode mode) override
    {
        Player* player = Primalist(unit);
        if (!player || !player->IsAlive() || !player->IsInWorld() || !application ||
            mode == AURA_REMOVE_BY_DEATH || !player->HasAura(300728))
            return;
        Aura* aura = application->GetBase();
        if (aura->GetCasterGUID() != player->GetGUID())
            return;
        // Only the visible defenses, not their separately removed SLS helpers.
        if (aura->GetId() == 680421 || aura->GetId() == 800094 || aura->GetId() == 503630)
            player->CastSpell(player, 503716, true);
    }
};

class primalist_talent_casts : public AllSpellScript
{
public:
    primalist_talent_casts() : AllSpellScript("primalist_talent_casts",
        {ALLSPELLHOOK_ON_CRIT_CHANCE, ALLSPELLHOOK_ON_HIT_RESULT}) { }

    void OnSpellHitResult(Spell* spell, Unit* target, uint8 miss, uint32, uint32, bool critical) override
    {
        Player* player = Primalist(spell->GetCaster());
        SpellInfo const* info = spell->GetSpellInfo();
        if (!player || !target || target == player || player->IsFriendlyTo(target) || miss != SPELL_MISS_NONE ||
            info->SpellFamilyName != 37)
            return;
        if (info->Id == SPELL_GEODE_BARRAGE_DAMAGE && !spell->GetScriptValue(SPELL_GEODE_BARRAGE_RAGE))
        {
            // Each channel tick casts this damage helper. Its authored energize
            // companion rolls 30-80 internal Rage (3-8 visible Rage) per successful stone.
            spell->SetScriptValue(SPELL_GEODE_BARRAGE_RAGE, 1);
            player->CastSpell(player, SPELL_GEODE_BARRAGE_RAGE, true);
        }
        // Legacy of Rexxar (800184): crits with the three Wildwalker attacks grant
        // the matching companion buff to the player and the pet. The proc chain in
        // the DBC is inert, so the buffs are granted directly.
        if (critical && player->IsAlive() && player->HasAura(SPELL_LEGACY_OF_REXXAR))
        {
            uint32 buff = 0;
            if (sSpellMgr->GetFirstSpellInChain(info->Id) == sSpellMgr->GetFirstSpellInChain(SPELL_PRIMAL_SHRED))
                buff = SPELL_MISHAS_RAGE;
            else if (sSpellMgr->GetFirstSpellInChain(info->Id) == sSpellMgr->GetFirstSpellInChain(SPELL_RYLAKS_BITE))
                buff = SPELL_LEOKKS_FURY;
            else if (sSpellMgr->GetFirstSpellInChain(info->Id) == sSpellMgr->GetFirstSpellInChain(SPELL_WILDCLAW))
                buff = SPELL_HUFFERS_SPEED;
            if (buff)
            {
                player->CastSpell(player, buff, true);
                if (Pet* pet = player->GetPet(); pet && pet->IsAlive())
                    player->CastSpell(pet, buff, true);
            }
        }
    }

    void OnSpellCritChance(Spell* spell, Unit* target, float& chance) override
    {
        Player* player = Primalist(spell->GetCaster());
        SpellInfo const* info = spell->GetSpellInfo();
        if (!player || !target || !player->HasAura(805336) || info->SpellFamilyName != 37 ||
            !(info->SpellFamilyFlags[0] & 8))
            return;
        for (AuraEffect const* effect : target->GetAuraEffectsByType(SPELL_AURA_PERIODIC_DAMAGE))
            if (effect->GetCasterGUID() == player->GetGUID() && effect->GetSpellInfo()->SpellFamilyName == 37 &&
                (effect->GetSpellInfo()->SpellFamilyFlags[0] & 64))
            {
                chance = 100.0f;
                break;
            }
    }
};

SpellCastResult CheckThroatClamp(Player* player, Unit* target)
{
    Pet* pet = player ? player->GetPet() : nullptr;
    if (!pet || !pet->IsAlive() || pet->GetOwnerGUID() != player->GetGUID())
        return SPELL_FAILED_NO_PET;
    if (!target || !pet->IsInMap(target) || !pet->InSamePhase(target) || !pet->IsValidAttackTarget(target))
        return SPELL_FAILED_BAD_TARGETS;
    if (pet->HasUnitState(UNIT_STATE_NOT_MOVE | UNIT_STATE_LOST_CONTROL))
        return SPELL_FAILED_CANT_DO_THAT_RIGHT_NOW;
    SpellInfo const* helper = sSpellMgr->GetSpellInfo(500811);
    if (!helper || !pet->IsWithinDistInMap(target, helper->GetMaxRange(false, pet)))
        return SPELL_FAILED_OUT_OF_RANGE;
    if (!pet->IsWithinLOSInMap(target))
        return SPELL_FAILED_LINE_OF_SIGHT;
    return SPELL_CAST_OK;
}

class spell_ascension_throat_clamp : public SpellScript
{
    PrepareSpellScript(spell_ascension_throat_clamp);

    bool Validate(SpellInfo const*) override { return ValidateSpellInfo({500811}); }
    bool Load() override { return Primalist(GetCaster()) != nullptr; }

    SpellCastResult CheckCast() { return CheckThroatClamp(Primalist(GetCaster()), GetExplTargetUnit()); }

    void Handle(SpellEffIndex)
    {
        Player* player = Primalist(GetCaster());
        Unit* target = GetHitUnit();
        if (CheckThroatClamp(player, target) == SPELL_CAST_OK)
            player->GetPet()->CastSpell(target, 500811, false); // Native dash, interrupt and school lockout.
    }

    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_ascension_throat_clamp::CheckCast);
        OnEffectHitTarget += SpellEffectFn(spell_ascension_throat_clamp::Handle, EFFECT_0, SPELL_EFFECT_DUMMY);
    }
};
}

void AddSC_AscensionPrimalistTalents()
{
    new primalist_talent_events();
    new primalist_talent_casts();
    RegisterSpellScript(spell_ascension_throat_clamp);
}
