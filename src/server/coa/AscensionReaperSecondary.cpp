/* Copyright (C) 2016+ AzerothCore, GNU AGPL v3. */
#include "AscensionReaperTalents.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "Spell.h"
#include "SpellAuraEffects.h"
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
    SPELL_GRAVESITE = 572213,
    SPELL_GRAVESITE_AREA = 804722,
    SPELL_ENDBRINGER_AMOUNT = 801341,
    SPELL_ENDBRINGER_HEAL = 520419,
    SPELL_DIRGE = 801328,
    SPELL_DIRGE_HIT = 801311,
    SPELL_REAPED_SOUL = 500363,
    SPELL_SPECTRE_HIT = 803742,
    SPELL_SPECTRE_ROOT = 803942,
    SPELL_MURDER = 500376,
    SPELL_SOULFORGED_WEAPONRY = 561127,
    SPELL_SOULFORGED_WEAPONRY_RANK_2 = 561340,
    SPELL_HARD_BARGAIN = 300569,
    SPELL_TORMENTED_SOULS = 500481,
    SPELL_HARD_BARGAIN_HELPER = 572300,
    SPELL_CRIMSON_THIRST = 807415,
    SPELL_CRIMSON_STACK = 807416,
    SPELL_CRIMSON_AMOUNT = 807417,
    SPELL_CRIMSON_HEAL = 807545,
    SPELL_GRAVESITE_PASSIVE = 572213,
    SPELL_GRAVESITE_HIT = 300979,
    SPELL_SPIRIT_CHASER = 560434,
    SPELL_SPIRIT_SCYTHES = 300548,
    SPELL_SWIFT_DEATH = 300549,
    SPELL_REAP = 801327,
    SPELL_DEATHCHASER = 560351,
    SPELL_WRAITHBLADE = 805258,
    SPELL_SOULSTRIDER = 572340,
    SPELL_VEILWALK = 803990,
    SPELL_RED_WAKE = 707707,
    SPELL_HAUNTER = 705410,
    SPELL_SOULREND = 572341,
    SPELL_CHASING_DEATH = 707455,
    SPELL_CHASING_DEATH_TRACKER = 807418,
    SPELL_DEATHCHASER_DAMAGE = 805190,
    HAUNTER_BONUS = 10000,
    SPELL_ESSENCE_HARVEST = 707908,
    SPELL_GHOSTLY_WEAPON_HIT = 804474,
    ESSENCE_HARVEST_BONUS = 10,
    SPELL_PURGATORY_PASSIVE = 504046,
    SPELL_PURGATORY_BUFF = 504047,
    SPELL_RELIQUARY = 500631,
    SPELL_SOUL_INFUSION = 803031,
    SPELL_WARDEN_OF_THE_LOST = 707116,
    PURGATORY_BONUS = 20,
    SPELL_GHOSTLY_WEAPON = 803997,
    SPELL_GHOSTLY_WEAPON_FROST = 804474,
    SPELL_LAMENTING = 705397,
    SPELL_LAMENTING_HEAL = 807420
};

// Essence Harvest (707908): "Increases the additional Frost damage dealt by
// Ghostly Weapon by 10%." The weapon's proc damage spell (804474) has no
// family for the passive's Spell Flat Mod to match, so boost its damage here.
class reaper_essence_harvest : public UnitScript
{
public:
    reaper_essence_harvest() : UnitScript("reaper_essence_harvest", true,
        {UNITHOOK_MODIFY_SPELL_DAMAGE_TAKEN}) { }

    void ModifySpellDamageTaken(Unit* target, Unit* source, int32& damage, SpellInfo const* spellInfo) override
    {
        Player* player = source ? source->ToPlayer() : nullptr;
        if (!player || !damage || player->getClass() != CLASS_REAPER ||
            !spellInfo || sSpellMgr->GetFirstSpellInChain(spellInfo->Id) != SPELL_GHOSTLY_WEAPON_HIT ||
            !player->HasAura(SPELL_ESSENCE_HARVEST))
            return;
        damage += CalculatePct(damage, ESSENCE_HARVEST_BONUS);
    }
};

// Purgatory (504046): "When you gain Soul Infusion, the damage of your
// Soulrend and Reliquary of the Lost is increased by 20% for 6 sec, stacking
// 2 times." Gaining an aura has no native proc flag, so watch for the Soul
// Infusion application here and cast the buff (504047, 6 s / 2 stacks) whose
// DBC-only Add % Modifier slot cannot match the familyless consumers.
class reaper_purgatory : public UnitScript
{
public:
    reaper_purgatory() : UnitScript("reaper_purgatory", true,
        {UNITHOOK_ON_AURA_APPLY, UNITHOOK_MODIFY_SPELL_DAMAGE_TAKEN}) { }

    void OnAuraApply(Unit* unit, Aura* aura) override
    {
        Player* player = unit ? unit->ToPlayer() : nullptr;
        if (!player || player->getClass() != CLASS_REAPER || !aura ||
            aura->GetId() != SPELL_SOUL_INFUSION || aura->GetCasterGUID() != player->GetGUID() ||
            !player->HasAura(SPELL_PURGATORY_PASSIVE))
            return;
        player->CastSpell(player, SPELL_PURGATORY_BUFF, true);
    }

    void ModifySpellDamageTaken(Unit* target, Unit* source, int32& damage, SpellInfo const* spellInfo) override
    {
        Player* player = source ? source->ToPlayer() : nullptr;
        if (!player || !damage || player->getClass() != CLASS_REAPER || !spellInfo)
            return;
        Aura const* buff = player->GetAura(SPELL_PURGATORY_BUFF);
        if (!buff)
            return;
        uint32 const root = sSpellMgr->GetFirstSpellInChain(spellInfo->Id);
        if (root != SPELL_SOULREND && root != SPELL_RELIQUARY)
            return;
        damage += CalculatePct(damage, PURGATORY_BONUS * buff->GetStackAmount());
    }
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

class reaper_hard_bargain : public UnitScript
{
public:
    reaper_hard_bargain() : UnitScript("reaper_hard_bargain", true,
        {UNITHOOK_ON_AURA_APPLY, UNITHOOK_ON_AURA_REMOVE}) { }

    void OnAuraApply(Unit* unit, Aura* aura) override
    {
        Player* player = unit ? unit->ToPlayer() : nullptr;
        if (!player || player->getClass() != CLASS_REAPER || !aura ||
            aura->GetCasterGUID() != player->GetGUID() ||
            (aura->GetId() != SPELL_HARD_BARGAIN && aura->GetId() != SPELL_TORMENTED_SOULS))
            return;

        if (player->HasAura(SPELL_HARD_BARGAIN, player->GetGUID()) &&
            player->HasAura(SPELL_TORMENTED_SOULS, player->GetGUID()) &&
            !player->HasAura(SPELL_HARD_BARGAIN_HELPER, player->GetGUID()))
            player->CastSpell(player, SPELL_HARD_BARGAIN_HELPER, true);
    }

    void OnAuraRemove(Unit* unit, AuraApplication* application, AuraRemoveMode) override
    {
        Player* player = unit ? unit->ToPlayer() : nullptr;
        if (!player || player->getClass() != CLASS_REAPER || !application)
            return;

        Aura* aura = application->GetBase();
        if (aura->GetCasterGUID() == player->GetGUID() &&
            (aura->GetId() == SPELL_HARD_BARGAIN || aura->GetId() == SPELL_TORMENTED_SOULS))
            player->RemoveAurasDueToSpell(SPELL_HARD_BARGAIN_HELPER, player->GetGUID());
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
        if (!player || player->getClass() != CLASS_REAPER)
            return;
        uint32 const root = sSpellMgr->GetFirstSpellInChain(spell->GetSpellInfo()->Id);
        if (player->HasAura(SPELL_SPIRIT_CHASER))
        {
            // Spirit Chaser (560434): "Increases the critical strike chance and
            // critical damage of Reap, Deathchaser, and Wraithblade by 10%." Those
            // abilities carry no spell family, so the passive's native Spell Flat
            // Mod can never match them. The +10% crit damage rides the same
            // multiplier the engine applies to crits of these abilities.
            if (root == SPELL_REAP || root == SPELL_DEATHCHASER || root == SPELL_WRAITHBLADE)
                chance += 10;
        }
        // Issue 1638: Spirit Scythes (300548) "Increases the critical strike
        // chance of Soulrend and Reap by 3%." The shipped 107 masks (0x2001,
        // 0x2001+0x4) match neither Soulrend (fam 36 flag 0x2000) nor Reap
        // (familyless), so apply the authored +3% here.
        if (player->HasAura(SPELL_SPIRIT_SCYTHES) &&
            (root == SPELL_SOULREND || root == SPELL_REAP))
            chance += 3;
    }

    void OnSpellCast(Spell* spell, Unit* caster, SpellInfo const* info, bool) override
    {
        Player* player = caster ? caster->ToPlayer() : nullptr;
        if (!player || player->getClass() != CLASS_REAPER || spell->IsTriggered())
            return;
        // Soulforged Weaponry: consume the free-Murder grant after its cost
        // was calculated.
        if (sSpellMgr->GetFirstSpellInChain(info->Id) == SPELL_MURDER)
            for (SpellModifier* mod : player->GetSpellModList(SPELLMOD_COST))
                if (mod->targetSpellId == SPELL_MURDER)
                {
                    player->AddSpellMod(mod, false);
                    delete mod;
                    break;
                }
        // Soulstrider (572340): "Increases the movement speed granted by
        // Veilwalk by 25%." The DBC's Add Flat Modifier slot has no family to
        // match the Veilwalk chain, so top the speed aura up on cast.
        if (sSpellMgr->GetFirstSpellInChain(info->Id) == SPELL_VEILWALK &&
            player->HasAura(SPELL_SOULSTRIDER))
            if (Aura* veil = player->GetAura(SPELL_VEILWALK))
                if (AuraEffect* speed = veil->GetEffect(EFFECT_0);
                    speed && speed->GetAuraType() == SPELL_AURA_MOD_INCREASE_SPEED)
                    speed->ChangeAmount(speed->GetAmount() + 12); // 50% -> 62.5% rounded to 62
        if (info->Id == SPELL_ENDBRINGER && caster->IsPlayer() && caster->getClass() == CLASS_REAPER &&
            !spell->IsTriggered() && caster->HasAura(SPELL_GRAVESITE, caster->GetGUID()))
            caster->CastSpell(caster, SPELL_GRAVESITE_AREA, true);
        // Damage and cost have been calculated. A projectile retains its selected stack count.
        if (sSpellMgr->GetFirstSpellInChain(info->Id) == SPELL_MURDER &&
            spell->GetScriptValue(SPELL_CRIMSON_STACK))
            caster->RemoveAurasDueToSpell(SPELL_CRIMSON_STACK, caster->GetGUID());
    }

    void OnSpellBeforeEffects(Spell* spell, Unit* caster, SpellInfo const* info) override
    {
        if (caster->IsPlayer() && caster->getClass() == CLASS_REAPER && !spell->IsTriggered() &&
            sSpellMgr->GetFirstSpellInChain(info->Id) == SPELL_MURDER)
            if (Aura const* stacks = caster->GetAura(SPELL_CRIMSON_STACK, caster->GetGUID()))
                spell->SetScriptValue(SPELL_CRIMSON_STACK, stacks->GetStackAmount());
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
        if (id == SPELL_GHOSTLY_WEAPON_FROST && player->HasAura(SPELL_LAMENTING, player->GetGUID()))
            HealFromDamage(player, SPELL_LAMENTING, SPELL_LAMENTING_HEAL, damage);
        if (id == SPELL_SPECTRE_HIT && target->IsAlive())
            player->CastSpell(target, SPELL_SPECTRE_ROOT, true);
        // Casting Endbringer marks the caster's position as a Gravesite.
        if (id == SPELL_ENDBRINGER && target == player && player->HasAura(SPELL_GRAVESITE_PASSIVE))
            player->CastSpell(player, SPELL_GRAVESITE_AREA, true);
        // Red Wake (707707): "Direct damage critical strikes now generate an
        // additional Soul Fragment." The DBC's Proc Trigger slot is inert.
        if (critical && damage && player->HasAura(SPELL_RED_WAKE) && !spell->IsTriggered())
            HandleAscensionReaperResource(player, SPELL_REAPED_SOUL, 1);
        // Haunter (705410): "Increases the duration of Soulrend by 10 sec."
        // The passive's Spell Flat Mod slot has no family to match the
        // Soulrend chain, so stretch the aura on application instead.
        if (sSpellMgr->GetFirstSpellInChain(id) == SPELL_SOULREND && player->HasAura(SPELL_HAUNTER))
            if (Aura* rend = target->GetAura(spell->GetSpellInfo()->Id, player->GetGUID()))
                rend->SetDuration(rend->GetDuration() + HAUNTER_BONUS);
        // Essence Harvest (707908): handled in reaper_essence_harvest below —
        // OnSpellHitResult cannot adjust the final damage.
        // Gravesite (572213): "Direct critical strikes made against enemies
        // within a Gravesite now deals Shadow damage." The area marker (804722)
        // is dropped by the Endbringer cast below.
        if (critical && damage && player->HasAura(SPELL_GRAVESITE_PASSIVE) &&
            target->HasAura(SPELL_GRAVESITE_AREA, player->GetGUID()))
            player->CastSpell(target, SPELL_GRAVESITE_HIT, true);
        // Chasing Death (707455): "Deathchaser used against a target below 20%
        // health will now reapply itself at the end of its duration until they
        // die or until 5 sec passes." The talent's Proc Trigger slot is inert
        // (ProcFlags 0), so start the 5 sec tracker on hit instead. While the
        // tracker (807418) ticks, its chained 807546 stretches the Deathchaser
        // damage aura natively via MODIFY_AURA_DURATION.
        uint32 const hitRoot = sSpellMgr->GetFirstSpellInChain(id);
        if (player->HasAura(SPELL_CHASING_DEATH) && !spell->IsTriggered() && target->IsAlive() &&
            target->HealthBelowPct(20) && (hitRoot == SPELL_DEATHCHASER || hitRoot == SPELL_DEATHCHASER_DAMAGE) &&
            !target->HasAura(SPELL_CHASING_DEATH_TRACKER, player->GetGUID()))
            player->CastSpell(target, SPELL_CHASING_DEATH_TRACKER, true);
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

class aura_ascension_gravesite : public AuraScript
{
    PrepareAuraScript(aura_ascension_gravesite);

    bool Check(ProcEventInfo& event)
    {
        Unit* player = GetTarget();
        Unit* victim = event.GetActionTarget();
        if (!player->IsPlayer() || player->getClass() != CLASS_REAPER || GetCaster() != player ||
            event.GetActor() != player || !victim || victim == player || player->IsFriendlyTo(victim) ||
            !event.GetDamageInfo() || !event.GetDamageInfo()->GetDamage() ||
            !(event.GetHitMask() & PROC_HIT_CRITICAL) ||
            (event.GetTypeMask() & (PROC_FLAG_DONE_PERIODIC | PROC_FLAG_TAKEN_PERIODIC)))
            return false;

        return victim->HasAura(SPELL_GRAVESITE_AREA);
    }

    void Register() override
    {
        DoCheckProc += AuraCheckProcFn(aura_ascension_gravesite::Check);
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

class aura_ascension_ghostly_weapon : public AuraScript
{
    PrepareAuraScript(aura_ascension_ghostly_weapon);

    bool Validate(SpellInfo const* info) override
    {
        SpellInfo const* frost = sSpellMgr->GetSpellInfo(SPELL_GHOSTLY_WEAPON_FROST);
        return info->Id == SPELL_GHOSTLY_WEAPON && info->Effects[EFFECT_0].ApplyAuraName == 354 &&
            frost && frost->SchoolMask == SPELL_SCHOOL_MASK_FROST &&
            frost->Effects[EFFECT_0].Effect == SPELL_EFFECT_SCHOOL_DAMAGE;
    }

    bool Check(ProcEventInfo& event)
    {
        Unit* owner = GetTarget();
        Unit* victim = event.GetActionTarget();
        DamageInfo const* damage = event.GetDamageInfo();
        SpellInfo const* spell = event.GetSpellInfo();
        return owner->IsPlayer() && owner->getClass() == CLASS_REAPER && owner->IsAlive() &&
            GetCasterGUID() == owner->GetGUID() && event.GetActor() == owner && victim &&
            victim->IsAlive() && !owner->IsFriendlyTo(victim) && damage && damage->GetDamage() &&
            damage->GetDamageType() != DOT && (!spell || spell->Id != SPELL_GHOSTLY_WEAPON_FROST) &&
            (event.GetTypeMask() & (PROC_FLAG_DONE_MELEE_AUTO_ATTACK | PROC_FLAG_DONE_SPELL_MELEE_DMG_CLASS));
    }

    void Proc(AuraEffect const* effect, ProcEventInfo& event)
    {
        PreventDefaultAction();
        uint64 amount = uint64(event.GetDamageInfo()->GetDamage()) * std::max(0, effect->GetAmount()) / 100;
        if (amount)
            GetTarget()->CastCustomSpell(SPELL_GHOSTLY_WEAPON_FROST, SPELLVALUE_BASE_POINT0,
                int32(std::min<uint64>(amount, std::numeric_limits<int32>::max())),
                event.GetActionTarget(), TRIGGERED_FULL_MASK);
    }

    void Register() override
    {
        DoCheckProc += AuraCheckProcFn(aura_ascension_ghostly_weapon::Check);
        OnEffectProc += AuraEffectProcFn(aura_ascension_ghostly_weapon::Proc, EFFECT_0, AuraType(354));
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
            info->ProcFlags = 0;
        }
        if (info->Id == SPELL_ENDBRINGER_HEAL)
            info->DmgClass = SPELL_DAMAGE_CLASS_NONE;
        if (info->Id == SPELL_SPIRIT_WALKER_SPEED || info->Id == SPELL_CRIMSON_STACK ||
            info->Id == SPELL_HARD_BARGAIN_HELPER)
        {
            info->AttributesCu &= ~SPELL_ATTR0_CU_FORCE_AURA_SAVING;
            info->AttributesCu |= SPELL_ATTR0_CU_AURA_CANNOT_BE_SAVED;
        }
        if (info->Id == 707909)
        {
            // Issue 662: To The Shadowlands' duration mod is the authored half
            // (+1 sec, aura 107, op 1 = SPELLMOD_DURATION, maskA 0x4000000
            // keys Soulslam 504014's family-36 flag). The DBC already carries
            // SPELL_ATTR0_PASSIVE and DieSides 1, so the re-mark below is a
            // defensive no-op kept in case the record is ever regenerated
            // without them.
            info->Attributes |= SPELL_ATTR0_PASSIVE;
            info->Effects[EFFECT_0].DieSides = 1;
        }
        if (info->Id == SPELL_WARDEN_OF_THE_LOST)
        {
            // Issue 683: Warden of the Lost's aura (effect 0, aura 42 on the
            // summons, trigger 707128 -> force cast of 707127's -0.5 sec on
            // Spectral Warden 805716) ships with ProcFlags 0. LoadSpellProcs
            // skips the default proc entry for a trigger aura without flags,
            // so the authored chain never fired. The aura sits on the Reaper's
            // summons, so the flags are the damage-dealt set.
            info->ProcFlags = DONE_HIT_PROC_FLAG_MASK;
        }
        if (info->Id == SPELL_GHOSTLY_WEAPON_FROST)
            info->AscensionInheritsResolvedAmount = true;
    }
};
}

class soulforged_weaponry_melee : public UnitScript
{
public:
    soulforged_weaponry_melee() : UnitScript("soulforged_weaponry_melee", true, {UNITHOOK_MODIFY_MELEE_DAMAGE}) { }

    void ModifyMeleeDamage(Unit* target, Unit* attacker, uint32& damage) override
    {
        Player* player = attacker ? attacker->ToPlayer() : nullptr;
        if (!player || player->getClass() != CLASS_REAPER || !damage || !target || target == player)
            return;
        // Soulforged Weaponry: while dual-wielding, auto attacks roll 8%/15%
        // to make the next Murder free of cost. The DBC's proc names no
        // trigger spell, so the roll happens here on melee damage.
        if (!player->GetWeaponForAttack(OFF_ATTACK, true))
            return;
        uint32 chance = 0;
        if (player->HasAura(SPELL_SOULFORGED_WEAPONRY))
            chance = 8;
        if (player->HasAura(SPELL_SOULFORGED_WEAPONRY_RANK_2))
            chance = 15;
        if (!chance || !roll_chance_i(chance))
            return;
        // The next Murder costs nothing: register a one-shot cost modifier
        // keyed to Murder's own spell id. The cast hook below consumes it.
        SpellModifier* free = new SpellModifier();
        free->op = SPELLMOD_COST;
        free->type = SPELLMOD_PCT;
        free->value = -100;
        free->targetSpellId = SPELL_MURDER;
        player->AddSpellMod(free, true);
    }

};

void AddSC_AscensionReaperSecondary()
{
    new reaper_ghost_speed();
    new reaper_hard_bargain();
    new reaper_secondary_hits();
    new reaper_secondary_metadata();
    new reaper_essence_harvest();
    new reaper_purgatory();
    new soulforged_weaponry_melee();
    RegisterSpellScript(aura_ascension_gravesite);
    RegisterSpellScript(aura_ascension_crimson_thirst);
    RegisterSpellScript(aura_ascension_ghostly_weapon);
}
