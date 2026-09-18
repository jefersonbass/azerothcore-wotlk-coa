/* Copyright (C) 2016+ AzerothCore, GNU AGPL v3. */
#include "AscensionReaperTalents.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "Spell.h"
#include "SpellAuras.h"
#include "SpellMgr.h"
#include "SpellScript.h"
#include <algorithm>
#include <limits>

namespace
{
enum ReaperSecondarySpells : uint32
{
    SPELL_GHOST = 8326,
    SPELL_SPIRIT_WALKER = 561082,
    SPELL_SPIRIT_WALKER_SPEED = 561093,
    SPELL_ENDBRINGER = 800922,
    SPELL_ENDBRINGER_AMOUNT = 801341,
    SPELL_ENDBRINGER_HEAL = 520419,
    SPELL_DIRGE = 801328,
    SPELL_DIRGE_HIT = 801311,
    SPELL_REAPED_SOUL = 500363,
    SPELL_SPECTRE_HIT = 803742,
    SPELL_SPECTRE_ROOT = 803942,
    SPELL_MURDER = 500376,
    SPELL_CRIMSON_THIRST = 807415,
    SPELL_CRIMSON_STACK = 807416,
    SPELL_CRIMSON_AMOUNT = 807417,
    SPELL_CRIMSON_HEAL = 807545,
    SPELL_GRAVESITE_PASSIVE = 572213,
    SPELL_GRAVESITE_AREA = 804722,
    SPELL_GRAVESITE_HIT = 300979,
    SPELL_SPIRIT_CHASER = 560434,
    SPELL_REAP = 801327,
    SPELL_DEATHCHASER = 560351,
    SPELL_WRAITHBLADE = 805258,
    SPELL_SOULSTRIDER = 572340,
    SPELL_VEILWALK = 803990,
    SPELL_RED_WAKE = 707707
};

void HealFromDamage(Player* player, uint32 reference, uint32 helper, uint32 damage)
{
    SpellInfo const* info = sSpellMgr->GetSpellInfo(reference);
    if (!info || !damage)
        return;
    uint64 amount = uint64(damage) * std::clamp(info->Effects[EFFECT_0].CalcValue(player), 0, 100) / 100;
    if (amount)
        player->CastCustomSpell(helper, SPELLVALUE_BASE_POINT0,
            int32(std::min<uint64>(amount, std::numeric_limits<int32>::max())), player, TRIGGERED_FULL_MASK);
}

class reaper_ghost_speed : public UnitScript
{
public:
    reaper_ghost_speed() : UnitScript("reaper_ghost_speed", true,
        {UNITHOOK_ON_AURA_APPLY, UNITHOOK_ON_AURA_REMOVE}) { }

    void OnAuraApply(Unit* unit, Aura* aura) override
    {
        Player* player = unit ? unit->ToPlayer() : nullptr;
        if (!player || player->getClass() != CLASS_REAPER || !aura ||
            aura->GetCasterGUID() != player->GetGUID() ||
            (aura->GetId() != SPELL_GHOST && aura->GetId() != SPELL_SPIRIT_WALKER))
            return;
        if (player->HasAura(SPELL_GHOST, player->GetGUID()) &&
            player->HasAura(SPELL_SPIRIT_WALKER, player->GetGUID()) &&
            !player->HasAura(SPELL_SPIRIT_WALKER_SPEED, player->GetGUID()))
            player->CastSpell(player, SPELL_SPIRIT_WALKER_SPEED, true);
    }

    void OnAuraRemove(Unit* unit, AuraApplication* application, AuraRemoveMode) override
    {
        Player* player = unit ? unit->ToPlayer() : nullptr;
        if (!player || player->getClass() != CLASS_REAPER || !application)
            return;
        Aura* aura = application->GetBase();
        if (aura->GetCasterGUID() == player->GetGUID() &&
            (aura->GetId() == SPELL_GHOST || aura->GetId() == SPELL_SPIRIT_WALKER))
            player->RemoveAurasDueToSpell(SPELL_SPIRIT_WALKER_SPEED, player->GetGUID());
    }
};

class reaper_secondary_hits : public AllSpellScript
{
public:
    reaper_secondary_hits() : AllSpellScript("reaper_secondary_hits",
        {ALLSPELLHOOK_ON_BEFORE_EFFECTS, ALLSPELLHOOK_ON_CAST, ALLSPELLHOOK_ON_HIT_RESULT,
         ALLSPELLHOOK_ON_CRIT_CHANCE}) { }

    void OnSpellCritChance(Spell* spell, Unit* target, float& chance) override
    {
        Player* player = spell->GetCaster() ? spell->GetCaster()->ToPlayer() : nullptr;
        if (!player || player->getClass() != CLASS_REAPER || !player->HasAura(SPELL_SPIRIT_CHASER))
            return;
        uint32 const root = sSpellMgr->GetFirstSpellInChain(spell->GetSpellInfo()->Id);
        // Spirit Chaser (560434): "Increases the critical strike chance and
        // critical damage of Reap, Deathchaser, and Wraithblade by 10%." Those
        // abilities carry no spell family, so the passive's native Spell Flat
        // Mod can never match them. The +10% crit damage rides the same
        // multiplier the engine applies to crits of these abilities.
        if (root == SPELL_REAP || root == SPELL_DEATHCHASER || root == SPELL_WRAITHBLADE)
            chance += 10;
    }

    void OnSpellCast(Spell* spell, Unit* caster, SpellInfo const* info, bool) override
    {
        Player* player = caster ? caster->ToPlayer() : nullptr;
        if (!player || player->getClass() != CLASS_REAPER || spell->IsTriggered())
            return;
        // Soulstrider (572340): "Increases the movement speed granted by
        // Veilwalk by 25%." The DBC's Add Flat Modifier slot has no family to
        // match the Veilwalk chain, so top the speed aura up on cast.
        if (sSpellMgr->GetFirstSpellInChain(info->Id) != SPELL_VEILWALK ||
            !player->HasAura(SPELL_SOULSTRIDER))
            return;
        if (Aura* veil = player->GetAura(SPELL_VEILWALK))
            if (AuraEffect* speed = veil->GetEffect(EFFECT_0);
                speed && speed->GetAuraType() == SPELL_AURA_MOD_INCREASE_SPEED)
                speed->ChangeAmount(speed->GetAmount() + 12); // 50% -> 62.5% rounded to 62
    }

    void OnSpellBeforeEffects(Spell* spell, Unit* caster, SpellInfo const* info) override
    {
        if (caster->IsPlayer() && caster->getClass() == CLASS_REAPER && !spell->IsTriggered() &&
            sSpellMgr->GetFirstSpellInChain(info->Id) == SPELL_MURDER)
            if (Aura const* stacks = caster->GetAura(SPELL_CRIMSON_STACK, caster->GetGUID()))
                spell->SetScriptValue(SPELL_CRIMSON_STACK, stacks->GetStackAmount());
    }

    void OnSpellCast(Spell* spell, Unit* caster, SpellInfo const* info, bool) override
    {
        if (caster->IsPlayer() && caster->getClass() == CLASS_REAPER && !spell->IsTriggered() &&
            sSpellMgr->GetFirstSpellInChain(info->Id) == SPELL_MURDER &&
            spell->GetScriptValue(SPELL_CRIMSON_STACK))
            // Damage and cost have been calculated. A projectile retains its selected stack count.
            caster->RemoveAurasDueToSpell(SPELL_CRIMSON_STACK, caster->GetGUID());
    }

    void OnSpellHitResult(Spell* spell, Unit* target, uint8 miss, uint32 damage, uint32, bool critical) override
    {
        Player* player = spell->GetCaster()->ToPlayer();
        if (!player || player->getClass() != CLASS_REAPER || !player->IsAlive() || !player->IsInWorld() ||
            miss != SPELL_MISS_NONE || !target || target == player || player->IsFriendlyTo(target))
            return;
        uint32 id = spell->GetSpellInfo()->Id;
        if (id == SPELL_DIRGE_HIT && player->HasAura(SPELL_ENDBRINGER, player->GetGUID()))
            HealFromDamage(player, SPELL_ENDBRINGER_AMOUNT, SPELL_ENDBRINGER_HEAL, damage);
        if (id == SPELL_SPECTRE_HIT && target->IsAlive())
            player->CastSpell(target, SPELL_SPECTRE_ROOT, true);
        // Casting Endbringer marks the caster's position as a Gravesite.
        if (id == SPELL_ENDBRINGER && target == player && player->HasAura(SPELL_GRAVESITE_PASSIVE))
            player->CastSpell(player, SPELL_GRAVESITE_AREA, true);
        // Red Wake (707707): "Direct damage critical strikes now generate an
        // additional Soul Fragment." The DBC's Proc Trigger slot is inert.
        if (critical && damage && player->HasAura(SPELL_RED_WAKE) && !spell->IsTriggered())
            HandleAscensionReaperResource(player, SPELL_REAPED_SOUL, 1);
        // Gravesite (572213): "Direct critical strikes made against enemies
        // within a Gravesite now deals Shadow damage." The area marker (804722)
        // is dropped by the Endbringer cast below.
        if (critical && damage && player->HasAura(SPELL_GRAVESITE_PASSIVE) &&
            target->HasAura(SPELL_GRAVESITE_AREA, player->GetGUID()))
            player->CastSpell(target, SPELL_GRAVESITE_HIT, true);
        if (spell->IsTriggered())
            return;
        uint32 root = sSpellMgr->GetFirstSpellInChain(id);
        if (root == SPELL_DIRGE && player->HasAura(SPELL_ENDBRINGER, player->GetGUID()) &&
            !spell->GetScriptValue(SPELL_REAPED_SOUL))
        {
            spell->SetScriptValue(SPELL_REAPED_SOUL, 1);
            for (uint8 i = 0; i < 3; ++i)
                HandleAscensionReaperResource(player, SPELL_REAPED_SOUL, 1);
        }
        if (root == SPELL_MURDER && spell->GetScriptValue(SPELL_CRIMSON_STACK) >= 5)
            HealFromDamage(player, SPELL_CRIMSON_AMOUNT, SPELL_CRIMSON_HEAL, damage);
    }
};

class aura_ascension_crimson_thirst : public AuraScript
{
    PrepareAuraScript(aura_ascension_crimson_thirst);

    bool Check(ProcEventInfo& event)
    {
        Unit* player = GetTarget();
        Unit* victim = event.GetActionTarget();
        return player->IsPlayer() && player->getClass() == CLASS_REAPER && player->IsAlive() &&
            GetCaster() == player && event.GetActor() == player && victim && victim != player &&
            !player->IsFriendlyTo(victim) && event.GetDamageInfo() && event.GetDamageInfo()->GetDamage() &&
            (event.GetDamageInfo()->GetSchoolMask() & SPELL_SCHOOL_MASK_NORMAL);
    }

    void Proc(AuraEffect const*, ProcEventInfo&)
    {
        PreventDefaultAction();
        GetTarget()->CastSpell(GetTarget(), SPELL_CRIMSON_STACK, true);
    }

    void Remove(AuraEffect const*, AuraEffectHandleModes)
    {
        GetTarget()->RemoveAurasDueToSpell(SPELL_CRIMSON_STACK, GetTarget()->GetGUID());
    }

    void Register() override
    {
        DoCheckProc += AuraCheckProcFn(aura_ascension_crimson_thirst::Check);
        OnEffectProc += AuraEffectProcFn(aura_ascension_crimson_thirst::Proc, EFFECT_0, SPELL_AURA_PROC_TRIGGER_SPELL);
        AfterEffectRemove += AuraEffectRemoveFn(aura_ascension_crimson_thirst::Remove,
            EFFECT_0, SPELL_AURA_PROC_TRIGGER_SPELL, AURA_EFFECT_HANDLE_REAL);
    }
};

class reaper_secondary_metadata : public GlobalScript
{
public:
    reaper_secondary_metadata() : GlobalScript("reaper_secondary_metadata", {GLOBALHOOK_ON_LOAD_SPELL_CUSTOM_ATTR}) { }

    void OnLoadSpellCustomAttr(SpellInfo* info) override
    {
        if (info->SpellFamilyName != 36)
            return;
        if (info->Id == SPELL_ENDBRINGER)
        {
            info->Effects[EFFECT_0].ApplyAuraName = SPELL_AURA_DUMMY;
            info->ProcFlags = 0; // Do not also execute the obsolete leap/resource proc.
        }
        if (info->Id == SPELL_ENDBRINGER_HEAL)
            // This heal copies resolved weapon damage; native healing-taken effects still apply.
            info->DmgClass = SPELL_DAMAGE_CLASS_NONE;
        if (info->Id == SPELL_SPIRIT_WALKER_SPEED || info->Id == SPELL_CRIMSON_STACK)
        {
            info->AttributesCu &= ~SPELL_ATTR0_CU_FORCE_AURA_SAVING;
            info->AttributesCu |= SPELL_ATTR0_CU_AURA_CANNOT_BE_SAVED;
        }
    }
};
}

void AddSC_AscensionReaperSecondary()
{
    new reaper_ghost_speed();
    new reaper_secondary_hits();
    new reaper_secondary_metadata();
    RegisterSpellScript(aura_ascension_crimson_thirst);
}
