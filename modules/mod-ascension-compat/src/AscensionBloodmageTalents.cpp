/* Copyright (C) 2016+ AzerothCore, GNU AGPL v3. */
#include "AscensionPooledVitality.h"
#include "DBCStores.h"
#include "Log.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SpellAuraEffects.h"
#include "SpellAuras.h"
#include "SpellAuraEffects.h"
#include "SpellScript.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "TemporarySummon.h"
#include <algorithm>
#include <utility>
#include <limits>
#include <vector>

namespace
{
enum BloodmageTalentSpells : uint32
{
    SPELL_LIQUIFY = 806310,
    SPELL_VAMPIRIC_POOLS = 504088,
    SPELL_VAMPIRIC_POOLS_LEECH = 806311,
    SPELL_DARKCASTING = 712383,
    SPELL_BLOOD_TEAR_SPAWN = 712417,
    SPELL_ACCURSED_FORM = 562572,
    SPELL_SANGUINE_SCRIPTURE = 804851,
    SPELL_SANGUINE_SCRIPTURE_BUFF = 504264,
    SPELL_CURSED_FORM_REQUIREMENT = 525031,
    SPELL_CURSED_FORM_REQUIREMENT_2 = 524861,
    SPELL_CURSED_FORM_OR_SANGUINE_ESSENCE = 803427,
    SPELL_BLOODMOON_POWER = 801961,
    SPELL_ATHERANNS_ANGUISH = 680680,
    SPELL_ATHERANNS_ANGUISH_BURST = 680681,
    SPELL_HUNTER_AND_HUNTED = 807487,
    SPELL_HUNTER_AND_HUNTED_NET = 100614,
    SPELL_VAMPYR_LORD = 560259,
    SPELL_COAGULATION = 706258,
    SPELL_SHADOWS_IN_THE_NIGHT = 704662,
    SPELL_ENDURE_THE_CURSE = 681190,
    SPELL_ETERNAL_CURSE = 800157,
    SPELL_ETERNAL_CURSE_ARMOR = 804320,
    SPELL_CURSED_BLOOD_TALENT = 681792,
    SPELL_CURSED_BLOOD_DEBUFF = 803722,
    SPELL_BLOOD_SHIELD = 504296,
    SPELL_COAGULATION_DISPEL = 504102,
    SPELL_DARK_MARK = 705731,
    SPELL_DARK_MARK_AURA = 707375,
    SPELL_TERRORIZER = 806210,
    SPELL_ENDURING = 300585,
    SPELL_ADRENALINE_BOOST = 680675,
    SPELL_BLOOD_PLAGUE = 575335,
    SPELL_APPETITE_FOR_BLOOD = 560479,
    SPELL_DARK_SIGIL = 560535,
    SPELL_BLOODLORDS_CURSE = 707449,
    SPELL_BLOOD_MOON = 707623,
    SPELL_BLOOD_MOON_HEAL = 572786,
    SPELL_CURSED_BLOOD = 707435,
    SPELL_CURSED_BLOOD_RUPTURE = 707708,
    SPELL_BLOODSURGE = 553267,
    SPELL_BLOODCHASER = 523721,
    SPELL_BLOOD_BOND_REWARD = 505325,
    SPELL_GORE_TOME = 807788,
    SPELL_GORE_TOME_WINDOW = 808014,
    SPELL_ONE_MANS_CURSE = 680661,
    SPELL_ONE_MANS_CURSE_HEAL = 680662,
    SPELL_BLOOD_CONSTRUCTOR = 561196,
    SPELL_THIRST = 706613,
    SPELL_THIRST_ANIMATED_BLOOD = 300796,
    SPELL_CRIMSON_EXPEDITION = 523727,
    SPELL_SANGUINE_SCION = 807292,
    SPELL_BLOOD_RUNS_COLD = 560257
};

constexpr uint32 BloodboltClassMask1 = 0x00020000;

enum AscensionRawCombatSelector : int32
{
    RAW_MASKED_CRIT = 20000,
    RAW_MASKED_CRIT_DAMAGE = 20001,
    RAW_CREATURE_DAMAGE = 20014
};

constexpr uint32 AnimatedBloodSummons[] = {325301, 335301, 315301};

constexpr uint32 CursedForms[] = {562572, 562720, 680692, 800157, 801076, 524865};

bool IsCursedForm(uint32 id)
{
    return std::find(std::begin(CursedForms), std::end(CursedForms), id) != std::end(CursedForms);
}

constexpr uint8 CursedFormWeaponSlots[] = {EQUIPMENT_SLOT_MAINHAND, EQUIPMENT_SLOT_OFFHAND, EQUIPMENT_SLOT_RANGED};

constexpr uint32 CursedFormsKeepingMortalAbilities[] = {680692, 801076};

bool HasCursedForm(Player const* player, Aura const* ignored = nullptr)
{
    for (uint32 form : CursedForms)
        if (Aura const* aura = player->GetAura(form, player->GetGUID()); aura && aura != ignored)
            return true;
    return false;
}

bool HasCursedFormBlockingMortalAbilities(Player const* player)
{
    for (uint32 form : CursedForms)
        if (std::find(std::begin(CursedFormsKeepingMortalAbilities), std::end(CursedFormsKeepingMortalAbilities),
            form) == std::end(CursedFormsKeepingMortalAbilities))
            if (Aura const* aura = player->GetAura(form, player->GetGUID()); aura)
                return true;
    return false;
}

void ClearVisibleWeapon(Player* player, uint8 slot)
{
    player->SetUInt32Value(PLAYER_VISIBLE_ITEM_1_ENTRYID + slot * 2, 0);
    player->SetUInt32Value(PLAYER_VISIBLE_ITEM_1_ENCHANTMENT + slot * 2, 0);
}

void UpdateCursedFormWeapons(Player* player, bool hidden)
{
    for (uint8 slot : CursedFormWeaponSlots)
        if (hidden)
            ClearVisibleWeapon(player, slot);
        else
            player->SetVisibleItemSlot(slot, player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot));
}

void SyncCursedFormRequirement(Player* player)
{
    bool const active = HasCursedForm(player);
    bool const blocksMortalAbilities = HasCursedFormBlockingMortalAbilities(player);

    for (uint32 marker : {uint32(SPELL_CURSED_FORM_REQUIREMENT), uint32(SPELL_CURSED_FORM_REQUIREMENT_2),
        uint32(AscensionBloodmage::CursedForm), uint32(SPELL_CURSED_FORM_OR_SANGUINE_ESSENCE)})
    {
        bool const wanted = active && (marker == SPELL_CURSED_FORM_REQUIREMENT ||
            marker == SPELL_CURSED_FORM_OR_SANGUINE_ESSENCE || blocksMortalAbilities);

        if (!wanted)
            player->RemoveAurasDueToSpell(marker, player->GetGUID());
        else if (player->IsInWorld() && player->IsAlive() && !player->HasAura(marker, player->GetGUID()))
            player->CastSpell(player, marker, true);
    }
}

void ApplyBloodmageConditionalContracts(SpellInfo* info)
{
    auto scoped = [info](uint8 index, int32 raw, AuraType aura)
    {
        SpellEffectInfo& effect = info->Effects[index];
        if (effect.Effect != SPELL_EFFECT_APPLY_AURA ||
            effect.ApplyAuraName != SPELL_AURA_OVERRIDE_CLASS_SCRIPTS || effect.MiscValue != raw ||
            effect.MiscValueB <= 0 || !effect.SpellClassMask)
        {
            LOG_ERROR("module.ascension_compat", "Skipped unexpected Bloodmage scoped damage record {}",
                info->Id);
            return;
        }
        effect.ApplyAuraName = aura;
        std::swap(effect.MiscValue, effect.MiscValueB);
    };

    auto conditional = [info](uint8 index, int32 raw, int32 selector)
    {
        SpellEffectInfo& effect = info->Effects[index];
        if (effect.Effect != SPELL_EFFECT_APPLY_AURA ||
            effect.ApplyAuraName != SPELL_AURA_OVERRIDE_CLASS_SCRIPTS || effect.MiscValue != raw ||
            effect.MiscValueB <= 0 || !effect.SpellClassMask)
        {
            LOG_ERROR("module.ascension_compat", "Skipped unexpected Bloodmage conditional record {}",
                info->Id);
            return;
        }
        effect.MiscValue = selector;
    };

    switch (info->Id)
    {
        case SPELL_TERRORIZER:
        case SPELL_ENDURING:
            scoped(EFFECT_0, ASCENSION_CLASSMASK_AURASTATE_DAMAGE, SPELL_AURA_MOD_DAMAGE_DONE_VERSUS_AURASTATE);
            break;
        case SPELL_APPETITE_FOR_BLOOD:
            scoped(EFFECT_0, RAW_CREATURE_DAMAGE, SPELL_AURA_MOD_DAMAGE_DONE_VERSUS);
            break;
        case SPELL_ADRENALINE_BOOST:
            conditional(EFFECT_0, RAW_MASKED_CRIT, ASCENSION_STATE_MASKED_CRIT);
            conditional(EFFECT_2, RAW_MASKED_CRIT, ASCENSION_STATE_MASKED_CRIT);
            break;
        case SPELL_BLOOD_PLAGUE:
            conditional(EFFECT_0, RAW_MASKED_CRIT, ASCENSION_STATE_MASKED_CRIT);
            conditional(EFFECT_1, RAW_MASKED_CRIT_DAMAGE, ASCENSION_STATE_MASKED_CRIT_DAMAGE);
            break;
        default:
            break;
    }
}

class spell_ascension_animated_blood : public SpellScript
{
    PrepareSpellScript(spell_ascension_animated_blood);

    bool Validate(SpellInfo const*) override
    {
        return ValidateSpellInfo({SPELL_DARKCASTING, SPELL_BLOOD_TEAR_SPAWN});
    }

    void HandleExtraWorms(SpellEffIndex index)
    {
        if (GetSpellInfo()->Effects[index].TriggerSpell != SPELL_BLOOD_TEAR_SPAWN)
            return;
        PreventHitDefaultEffect(index);
        Unit* caster = GetCaster();
        Aura* darkcasting = caster->GetAura(SPELL_DARKCASTING, caster->GetGUID());
        if (!darkcasting)
            return;
        uint8 const count = darkcasting->GetStackAmount();
        darkcasting->Remove();
        if (count)
            caster->CastCustomSpell(SPELL_BLOOD_TEAR_SPAWN, SPELLVALUE_BASE_POINT0, count, caster, true);
    }

    void ReplacePreviousBrood()
    {
        if (Unit* caster = GetCaster())
            for (uint32 entry : AnimatedBloodSummons)
                caster->RemoveAllMinionsByEntry(entry);
    }

    void ApplyVampyrLord()
    {
        // Vampyr Lord (560259): the brood inherits the passive's Mod Damage %
        // aura so its thirty-five percent bonus rides on their attacks.
        Player* caster = GetCaster()->ToPlayer();
        if (!caster || !caster->HasAura(SPELL_VAMPYR_LORD))
            return;
        std::list<Creature*> brood;
        for (uint32 entry : AnimatedBloodSummons)
        {
            caster->GetCreatureListWithEntryInGrid(brood, entry, 100.0f);
            for (Creature* worm : brood)
                if (worm->GetOwnerGUID() == caster->GetGUID() && !worm->HasAura(SPELL_VAMPYR_LORD))
                    caster->CastSpell(worm, SPELL_VAMPYR_LORD, true);
            brood.clear();
        }
    }

    void Register() override
    {
        BeforeCast += SpellCastFn(spell_ascension_animated_blood::ReplacePreviousBrood);
        AfterCast += SpellCastFn(spell_ascension_animated_blood::ApplyVampyrLord);
        OnEffectLaunch += SpellEffectFn(spell_ascension_animated_blood::HandleExtraWorms,
            EFFECT_1, SPELL_EFFECT_TRIGGER_SPELL);
    }
};

class bloodmage_talent_events : public UnitScript
{
public:
    bloodmage_talent_events() : UnitScript("bloodmage_talent_events", true,
        {UNITHOOK_MODIFY_SPELL_DAMAGE_TAKEN, UNITHOOK_ON_DAMAGE, UNITHOOK_ON_AURA_APPLY,
        UNITHOOK_ON_AURA_REMOVE}) { }

    void OnAuraApply(Unit* unit, Aura* aura) override
    {
        Player* player = unit ? unit->ToPlayer() : nullptr;
        if (!player || player->getClass() != CLASS_SON_OF_ARUGAL || !aura)
            return;
        if (IsCursedForm(aura->GetId()))
            SyncCursedFormRequirement(player);
        if (IsCursedForm(aura->GetId()))
            UpdateCursedFormWeapons(player, true);
            if (player->HasAura(SPELL_HUNTER_AND_HUNTED) &&
                aura->GetCasterGUID() == player->GetGUID() && player->IsAlive() && player->IsInWorld())
                if (Unit* target = player->GetSelectedUnit())
                    if (target != player && !player->IsFriendlyTo(target) && target->IsAlive() &&
                        player->IsWithinDistInMap(target, 20.0f) && player->IsWithinLOSInMap(target))
                    {
                        player->GetMotionMaster()->MoveCharge(target->GetPositionX(),
                            target->GetPositionY(), target->GetPositionZ(), 42.0f);
                        player->CastSpell(target, SPELL_HUNTER_AND_HUNTED_NET, true);
                    }
        if (aura->GetId() == SPELL_ETERNAL_CURSE)
            player->CastSpell(player, SPELL_ETERNAL_CURSE_ARMOR, true);
        if (aura->GetId() == SPELL_DARK_MARK)
            player->CastSpell(player, SPELL_DARK_MARK_AURA, true);
        if (IsCursedForm(aura->GetId()) && aura->GetCasterGUID() == player->GetGUID() &&
            player->HasAura(SPELL_GORE_TOME, player->GetGUID()))
            player->CastSpell(player, SPELL_GORE_TOME_WINDOW, true);
        if (aura->GetId() == SPELL_ACCURSED_FORM && aura->GetCasterGUID() == player->GetGUID() &&
            player->HasAura(SPELL_ONE_MANS_CURSE, player->GetGUID()))
            player->CastSpell(player, SPELL_ONE_MANS_CURSE_HEAL, true);
        if (aura->GetId() == SPELL_ACCURSED_FORM && aura->GetCasterGUID() == player->GetGUID() &&
            player->HasAura(SPELL_BLOODSURGE, player->GetGUID()))
        {
            player->RemoveAurasDueToSpell(SPELL_BLOODSURGE, player->GetGUID());
            player->CastSpell(player, SPELL_BLOODCHASER, true);
        }
    }

    void OnAuraRemove(Unit* unit, AuraApplication* application, AuraRemoveMode mode) override
    {
        Player* player = unit ? unit->ToPlayer() : nullptr;
        if (!player || player->getClass() != CLASS_SON_OF_ARUGAL || !application)
            return;
        Aura* aura = application->GetBase();
        if (IsCursedForm(aura->GetId()))
            SyncCursedFormRequirement(player);
        if (IsCursedForm(aura->GetId()) && !HasCursedForm(player, aura))
            UpdateCursedFormWeapons(player, false);
        if (aura->GetId() == SPELL_ETERNAL_CURSE)
            player->RemoveAurasDueToSpell(SPELL_ETERNAL_CURSE_ARMOR);
        if (aura->GetId() == SPELL_DARK_MARK)
            player->RemoveAurasDueToSpell(SPELL_DARK_MARK_AURA, player->GetGUID());
        if (!player->IsAlive() || !player->IsInWorld() || mode == AURA_REMOVE_BY_DEATH)
            return;
        if (aura->GetId() == SPELL_LIQUIFY && aura->GetCasterGUID() == player->GetGUID() &&
            player->HasAura(SPELL_VAMPIRIC_POOLS))
            player->CastSpell(player, SPELL_VAMPIRIC_POOLS_LEECH, true);
        // Atherann's Anguish (680680): the hemoplague mark explodes for its banked
        // damage when it runs its full ten seconds. Early dispels or deaths fizzle.
        if (aura->GetId() == SPELL_ATHERANNS_ANGUISH && mode == AURA_REMOVE_BY_EXPIRE &&
            aura->GetCasterGUID() == player->GetGUID())
            if (AuraEffect* bank = aura->GetEffect(EFFECT_2); bank && bank->GetAmount() > 0)
                player->CastCustomSpell(SPELL_ATHERANNS_ANGUISH_BURST,
                    SPELLVALUE_BASE_POINT0, bank->GetAmount(), unit, true);
        // Thirst for Blood: without a Thirst stack the Sated and Ravenous bonuses
        // lose their basis and are stripped.
        if (aura->GetId() == 706613)
        {
            player->RemoveAurasDueToSpell(570024);
            player->RemoveAurasDueToSpell(570025);
        }
        // Coagulation (706258): the Blood Shield gains the bleed-dispel pulse, whose
        // five-second periodic trigger into the Dispel Mechanic helper is native.
        if (aura->GetId() == 504296 && player->HasAura(SPELL_COAGULATION))
            player->CastSpell(player, SPELL_COAGULATION, true);
        // Shadows In The Night (704662): the five percent damage reduction only
        // counts while the Bloodmage is above seventy-five percent health.
        if (aura->GetId() == SPELL_SHADOWS_IN_THE_NIGHT)
            if (AuraEffect* reduction = aura->GetEffect(EFFECT_1))
                reduction->ChangeAmount(player->HealthAbovePct(75) ? -5 : 0);
        // Bloodmoon Power: Liquify cleanses all negative dispellable effects when it ends.
        if (aura->GetId() == SPELL_LIQUIFY && aura->GetCasterGUID() == player->GetGUID() &&
            player->HasAura(SPELL_BLOODMOON_POWER))
        {
            std::vector<uint32> remove;
            for (auto const& pair : player->GetAppliedAuras())
                if (!pair.second->IsPositive() && pair.second->GetBase()->GetSpellInfo()->Dispel != DISPEL_NONE)
                    remove.push_back(pair.second->GetBase()->GetId());
            for (uint32 id : remove)
                player->RemoveAurasDueToSpell(id);
        }
        if (aura->GetId() == SPELL_ACCURSED_FORM && aura->GetCasterGUID() == player->GetGUID() &&
            player->HasAura(SPELL_SANGUINE_SCRIPTURE))
            player->CastSpell(player, SPELL_SANGUINE_SCRIPTURE_BUFF, true);
        if (aura->GetId() == SPELL_BLOODSURGE && aura->GetCasterGUID() == player->GetGUID() &&
            mode == AURA_REMOVE_BY_EXPIRE)
            player->CastSpell(player, SPELL_BLOODCHASER, true);
    }

    void ModifySpellDamageTaken(Unit* target, Unit*, int32& damage, SpellInfo const*) override
    {
        // Shadows In The Night (704662): the aura's native -5% only counts while
        // the Bloodmage is above seventy-five percent health; below the threshold
        // the reduction is refunded here.
        Player* player = target ? target->ToPlayer() : nullptr;
        if (player && player->getClass() == CLASS_SON_OF_ARUGAL &&
            player->HasAura(SPELL_SHADOWS_IN_THE_NIGHT) && !player->HealthAbovePct(75))
            damage = int32(damage / 0.95f);
    }
};

class bloodmage_talent_guard : public UnitScript
{
public:
    bloodmage_talent_guard() : UnitScript("bloodmage_talent_guard", true,
        {UNITHOOK_ON_DAMAGE}) { }

    void OnDamage(Unit* attacker, Unit* victim, uint32& damage) override
    {
        // Endure the Curse (681190): while the ten-second guard is up, a hit that
        // would drop the Bloodmage below ten percent health instead heals for
        // thirty percent of maximum health. Once per activation.
        Player* player = victim ? victim->ToPlayer() : nullptr;
        if (!player || player->getClass() != CLASS_SON_OF_ARUGAL || !damage)
            return;
        Aura* guard = player->GetAura(SPELL_ENDURE_THE_CURSE);
        if (!guard || !guard->GetEffect(EFFECT_0) || guard->GetEffect(EFFECT_0)->GetAmount() != 0)
            return;
        if (int64(player->GetHealth()) - int64(damage) >= int64(player->GetMaxHealth() / 10))
            return;
        guard->GetEffect(EFFECT_0)->ChangeAmount(1);
        damage = 0;
        player->ModifyHealth(int32(player->CountPctFromMaxHealth(30)));
    }
};

class bloodmage_cursed_form_death : public PlayerScript
{
public:
    bloodmage_cursed_form_death() : PlayerScript("bloodmage_cursed_form_death", {PLAYERHOOK_ON_PLAYER_JUST_DIED}) { }

    void OnPlayerJustDied(Player* player) override
    {
        if (!player || player->getClass() != CLASS_SON_OF_ARUGAL)
            return;
        for (uint32 form : CursedForms)
            player->RemoveAurasDueToSpell(form);
        SyncCursedFormRequirement(player);
    }
};

class bloodmage_cursed_form_weapons : public PlayerScript
{
public:
    bloodmage_cursed_form_weapons() : PlayerScript("bloodmage_cursed_form_weapons",
        {PLAYERHOOK_ON_AFTER_SET_VISIBLE_ITEM_SLOT}) { }

    void OnPlayerAfterSetVisibleItemSlot(Player* player, uint8 slot, Item*) override
    {
        if (!player || player->getClass() != CLASS_SON_OF_ARUGAL)
            return;
        if (std::find(std::begin(CursedFormWeaponSlots), std::end(CursedFormWeaponSlots), slot) ==
            std::end(CursedFormWeaponSlots) || !HasCursedForm(player))
            return;
        ClearVisibleWeapon(player, slot);
    }
};

class bloodmage_blood_constructor : public PlayerScript
{
public:
    bloodmage_blood_constructor() : PlayerScript("bloodmage_blood_constructor",
        {PLAYERHOOK_ON_BEFORE_TEMP_SUMMON_INIT_STATS}) { }

    void OnPlayerBeforeTempSummonInitStats(Player* player, TempSummon* summon, uint32& duration) override
    {
        if (!player || player->getClass() != CLASS_SON_OF_ARUGAL || !summon || !duration)
            return;
        if (std::find(std::begin(AnimatedBloodSummons), std::end(AnimatedBloodSummons), summon->GetEntry())
            == std::end(AnimatedBloodSummons))
            return;
        if (!player->HasAura(SPELL_BLOOD_CONSTRUCTOR, player->GetGUID()))
            return;
        Aura const* thirst = player->GetAura(SPELL_THIRST, player->GetGUID());
        SpellInfo const* helper = sSpellMgr->GetSpellInfo(SPELL_THIRST_ANIMATED_BLOOD);
        if (!thirst || !helper)
            return;
        int32 const perStack = helper->Effects[EFFECT_0].CalcValue(player);
        if (perStack > 0)
            duration += uint32(perStack) * thirst->GetStackAmount();
    }
};

bool IsBloodmageDamageProc(Unit* player, Unit* caster, ProcEventInfo& event)
{
    Unit* victim = event.GetActionTarget();
    DamageInfo const* damage = event.GetDamageInfo();
    return player->IsPlayer() && player->getClass() == CLASS_SON_OF_ARUGAL && player->IsAlive() &&
        caster == player && event.GetActor() == player && victim && victim != player &&
        !player->IsFriendlyTo(victim) && damage && damage->GetDamage();
}

int32 BloodmageProcShare(AuraEffect const* effect, ProcEventInfo& event)
{
    uint64 share = uint64(event.GetDamageInfo()->GetDamage()) * std::clamp(effect->GetAmount(), 0, 100) / 100;
    return int32(std::min<uint64>(share, std::numeric_limits<int32>::max()));
}

class aura_ascension_bloodmage_dark_sigil : public AuraScript
{
    PrepareAuraScript(aura_ascension_bloodmage_dark_sigil);

    bool Check(ProcEventInfo& event)
    {
        return IsBloodmageDamageProc(GetTarget(), GetCaster(), event) && (event.GetHitMask() & PROC_HIT_CRITICAL);
    }

    void Proc(AuraEffect const* effect, ProcEventInfo& event)
    {
        PreventDefaultAction();
        if (int32 heal = BloodmageProcShare(effect, event))
            GetTarget()->CastCustomSpell(SPELL_BLOODLORDS_CURSE, SPELLVALUE_BASE_POINT0, heal, GetTarget(),
                TRIGGERED_FULL_MASK);
    }

    void Register() override
    {
        DoCheckProc += AuraCheckProcFn(aura_ascension_bloodmage_dark_sigil::Check);
        OnEffectProc += AuraEffectProcFn(aura_ascension_bloodmage_dark_sigil::Proc, EFFECT_1, AuraType(354));
    }
};

constexpr float BloodMoonHealthThreshold = 75.0f;

class aura_ascension_bloodmage_blood_moon : public AuraScript
{
    PrepareAuraScript(aura_ascension_bloodmage_blood_moon);

    bool Check(ProcEventInfo& event)
    {
        return IsBloodmageDamageProc(GetTarget(), GetCaster(), event) &&
            GetTarget()->GetHealthPct() < BloodMoonHealthThreshold;
    }

    void Proc(AuraEffect const* effect, ProcEventInfo& event)
    {
        PreventDefaultAction();
        if (int32 heal = BloodmageProcShare(effect, event))
            GetTarget()->CastCustomSpell(SPELL_BLOOD_MOON_HEAL, SPELLVALUE_BASE_POINT0, heal, GetTarget(),
                TRIGGERED_FULL_MASK);
    }

    void Register() override
    {
        DoCheckProc += AuraCheckProcFn(aura_ascension_bloodmage_blood_moon::Check);
        OnEffectProc += AuraEffectProcFn(aura_ascension_bloodmage_blood_moon::Proc, EFFECT_0, AuraType(354));
    }
};

class aura_ascension_bloodmage_cursed_blood : public AuraScript
{
    PrepareAuraScript(aura_ascension_bloodmage_cursed_blood);

    bool Check(ProcEventInfo& event)
    {
        return IsBloodmageDamageProc(GetTarget(), GetCaster(), event);
    }

    void Proc(AuraEffect const* effect, ProcEventInfo& event)
    {
        PreventDefaultAction();
        if (int32 damage = BloodmageProcShare(effect, event))
            GetTarget()->CastCustomSpell(SPELL_CURSED_BLOOD_RUPTURE, SPELLVALUE_BASE_POINT0, damage,
                event.GetActionTarget(), TRIGGERED_FULL_MASK);
    }

    void Register() override
    {
        DoCheckProc += AuraCheckProcFn(aura_ascension_bloodmage_cursed_blood::Check);
        OnEffectProc += AuraEffectProcFn(aura_ascension_bloodmage_cursed_blood::Proc, EFFECT_0, AuraType(354));
    }
};

class aura_ascension_bloodmage_crimson_feast : public AuraScript
{
    PrepareAuraScript(aura_ascension_bloodmage_crimson_feast);

    void Tick(AuraEffect const*)
    {
        Player* player = GetTarget() ? GetTarget()->ToPlayer() : nullptr;
        if (!player || player->getClass() != CLASS_SON_OF_ARUGAL || !HasCursedForm(player))
            PreventDefaultAction();
    }

    void Register() override
    {
        OnEffectPeriodic += AuraEffectPeriodicFn(aura_ascension_bloodmage_crimson_feast::Tick,
            EFFECT_0, SPELL_AURA_PERIODIC_TRIGGER_SPELL);
    }
};

class aura_ascension_bloodmage_coagulation : public AuraScript
{
    PrepareAuraScript(aura_ascension_bloodmage_coagulation);

    void Tick(AuraEffect const*)
    {
        Unit* target = GetTarget();
        if (!target || !target->HasAura(SPELL_BLOOD_SHIELD, target->GetGUID()))
            PreventDefaultAction();
    }

    void Register() override
    {
        OnEffectPeriodic += AuraEffectPeriodicFn(aura_ascension_bloodmage_coagulation::Tick,
            EFFECT_0, SPELL_AURA_PERIODIC_TRIGGER_SPELL);
    }
};

class aura_ascension_bloodmage_forbidden_power : public AuraScript
{
    PrepareAuraScript(aura_ascension_bloodmage_forbidden_power);

    void Calculate(AuraEffect const*, int32& amount, bool&)
    {
        amount = 0;
        Player* player = GetUnitOwner() ? GetUnitOwner()->ToPlayer() : nullptr;
        if (!player || player->getClass() != CLASS_SON_OF_ARUGAL)
            return;
        uint32 rating = player->GetUInt32Value(
            static_cast<uint16>(PLAYER_FIELD_COMBAT_RATING_1) + static_cast<uint16>(CR_ARMOR_PENETRATION));
        amount = -int32(std::min<uint32>(rating, uint32(std::numeric_limits<int32>::max())));
    }

    void Register() override
    {
        DoEffectCalcAmount += AuraEffectCalcAmountFn(aura_ascension_bloodmage_forbidden_power::Calculate,
            EFFECT_0, SPELL_AURA_MOD_TARGET_RESISTANCE);
    }
};

constexpr float EssenceHarvesterHealthThreshold = 35.0f;

class aura_ascension_bloodmage_essence_harvester : public AuraScript
{
    PrepareAuraScript(aura_ascension_bloodmage_essence_harvester);

    bool Check(ProcEventInfo& event)
    {
        Unit* target = GetTarget();
        Unit* attacker = event.GetActor();
        return target && target->IsPlayer() && target->IsAlive() && attacker && attacker != target &&
            !target->IsFriendlyTo(attacker) && target->GetHealthPct() < EssenceHarvesterHealthThreshold;
    }

    void Register() override
    {
        DoCheckProc += AuraCheckProcFn(aura_ascension_bloodmage_essence_harvester::Check);
    }
};

class aura_ascension_bloodmage_blood_bond : public AuraScript
{
    PrepareAuraScript(aura_ascension_bloodmage_blood_bond);

    bool Validate(SpellInfo const*) override { return ValidateSpellInfo({SPELL_BLOOD_BOND_REWARD}); }

    bool Check(ProcEventInfo& event)
    {
        Unit* owner = GetCaster();
        DamageInfo const* damage = event.GetDamageInfo();
        return owner && owner->IsPlayer() && owner->IsAlive() && owner->IsInWorld() && GetTarget() &&
            GetTarget() != owner && damage && damage->GetDamage();
    }

    void Proc(AuraEffect const*, ProcEventInfo&)
    {
        PreventDefaultAction();
        if (Unit* owner = GetCaster())
            owner->CastSpell(owner, SPELL_BLOOD_BOND_REWARD, true);
    }

    void Register() override
    {
        DoCheckProc += AuraCheckProcFn(aura_ascension_bloodmage_blood_bond::Check);
        OnEffectProc += AuraEffectProcFn(aura_ascension_bloodmage_blood_bond::Proc, EFFECT_0,
            SPELL_AURA_PROC_TRIGGER_SPELL);
    }
};

class bloodmage_talent_contracts : public GlobalScript
{
public:
    bloodmage_talent_contracts() : GlobalScript("bloodmage_talent_contracts",
        {GLOBALHOOK_ON_LOAD_SPELL_CUSTOM_ATTR}) { }

    void OnLoadSpellCustomAttr(SpellInfo* info) override
    {
        if (!info || info->SpellFamilyName != 26)
            return;
        if (info->Id == 806423)
        {
            // Issue 851: Bloodleaper ships without SPELL_ATTR0_PASSIVE, so
            // the learn/login passes never applied its cooldown mod. Mark
            // passive. Effect 0 (op 11 = SPELLMOD_COOLDOWN, bp -10001, maskA
            // 0x1000000) matches Lunge (500126, family-26 flag 0x1000000)
            // and resolves as the tooltip's -10s cooldown via the native
            // cooldown-mod path.
            info->Attributes |= SPELL_ATTR0_PASSIVE;
            return;
        }
        if (info->Id == 520493)
        {
            // Issue 828: Red Thirst is a Bloodlust-style raid haste (30%
            // for 20s on allies in 40 yds, then the 804457 Worn Out lockout
            // for 5 min). Its DBC already carries the right shape: effect 0
            // aura 65 (cast haste), effect 1 aura 192 (melee/ranged haste),
            // effect 2 trigger 804457, all on TARGET_UNIT_PARTY_CASTER (56).
            // The only defect is display-minus-1 BasePoints with DieSides 0;
            // shift DieSides to 1 so both haste halves resolve as the
            // tooltip's 30%. The native aura handlers apply them.
            info->Effects[EFFECT_0].DieSides = 1;
            info->Effects[EFFECT_1].DieSides = 1;
            return;
        }
        if (info->Id == SPELL_CURSED_BLOOD_TALENT)
        {
            // Issue 806: Cursed Blood ships without SPELL_ATTR0_PASSIVE, so the
            // learn/login passes never applied its spellmod auras, and its
            // BasePoints are display-minus-1 with DieSides 0. Mark passive and
            // shift DieSides to 1 so effect 1 resolves as the tooltip's 20%
            // damage (op 0, maskB 0x22000 covers Bloodbolt/Bloodmoon Blast)
            // and effect 2 as 20% SP scaling (op 22). Effect 0's trigger
            // (803722) is applied in script on Bloodbolt hits.
            info->Attributes |= SPELL_ATTR0_PASSIVE;
            info->Effects[EFFECT_1].DieSides = 1;
            info->Effects[EFFECT_2].DieSides = 1;
            return;
        }
        if (info->Id == SPELL_SANGUINE_SCION)
        {
            // "Increases the critical strike chance of Sanguine Mend and
            // Bloodbolt by 10%." The shipped mask matches neither spell, so
            // rekey to their union: Sanguine Mend (word0 0x80000), Bloodbolt
            // (word1 0x20000).
            info->Effects[EFFECT_0].SpellClassMask = flag96(0x80000, 0x20000, 0);
            return;
        }
        if (info->Id == 704621)
        {
            // Issue 958: Easy Prey's +1 sec Hemostasis duration ships without
            // the passive flag, and its mask is keyed to the wrong word, so
            // the learn/login passes never applied it. Mark passive and rekey
            // to Hemostasis's own family bit; the native duration-mod path
            // then applies the authored +1 sec.
            info->Attributes |= SPELL_ATTR0_PASSIVE;
            info->Effects[EFFECT_0].SpellClassMask = flag96(0, 0, 0x8);
            return;
        }

        ApplyBloodmageConditionalContracts(info);

        if ((info->Id == SPELL_CRIMSON_EXPEDITION || info->Id == SPELL_SANGUINE_SCION) &&
            info->Effects[EFFECT_0].ApplyAuraName == SPELL_AURA_ADD_FLAT_MODIFIER &&
            info->Effects[EFFECT_0].MiscValue == SPELLMOD_CRITICAL_CHANCE)
            info->Effects[EFFECT_0].SpellClassMask[1] |= BloodboltClassMask1;

        if (info->Id == SPELL_BLOOD_RUNS_COLD &&
            info->Effects[EFFECT_0].ApplyAuraName == SPELL_AURA_ADD_FLAT_MODIFIER &&
            info->Effects[EFFECT_0].MiscValue == SPELLMOD_EFFECT1)
            info->Effects[EFFECT_0].MiscValue = SPELLMOD_EFFECT2;

        if (info->Id == SPELL_COAGULATION_DISPEL &&
            info->Effects[EFFECT_0].Effect == SPELL_EFFECT_DISPEL_MECHANIC &&
            info->Effects[EFFECT_0].MiscValue == int32(MECHANIC_BLEED))
            info->Effects[EFFECT_0].BasePoints = 1;

        if (info->Id != SPELL_VAMPIRIC_POOLS_LEECH ||
            info->Effects[EFFECT_0].Effect != SPELL_EFFECT_HEALTH_LEECH)
            return;

        info->DurationEntry = sSpellDurationStore.LookupEntry(32);
        info->AttributesCu |= SPELL_ATTR0_CU_NEGATIVE_EFF1;
        auto& fear = info->Effects[EFFECT_1];
        fear.Effect = SPELL_EFFECT_APPLY_AURA;
        fear.ApplyAuraName = SPELL_AURA_MOD_FEAR;
        fear.Mechanic = MECHANIC_FEAR;
        fear.TargetA = info->Effects[EFFECT_0].TargetA;
        fear.TargetB = info->Effects[EFFECT_0].TargetB;
        fear.RadiusEntry = info->Effects[EFFECT_0].RadiusEntry;
    }
};
}

void AddSC_AscensionBloodmageTalents()
{
    new bloodmage_talent_events();
    new bloodmage_talent_guard();
    new bloodmage_cursed_form_death();
    new bloodmage_cursed_form_weapons();
    new bloodmage_blood_constructor();
    new bloodmage_talent_contracts();
    RegisterSpellScript(spell_ascension_animated_blood);
    RegisterSpellScript(aura_ascension_bloodmage_crimson_feast);
    RegisterSpellScript(aura_ascension_bloodmage_coagulation);
    RegisterSpellScript(aura_ascension_bloodmage_forbidden_power);
    RegisterSpellScript(aura_ascension_bloodmage_dark_sigil);
    RegisterSpellScript(aura_ascension_bloodmage_blood_moon);
    RegisterSpellScript(aura_ascension_bloodmage_cursed_blood);
    RegisterSpellScript(aura_ascension_bloodmage_essence_harvester);
    RegisterSpellScript(aura_ascension_bloodmage_blood_bond);
}
