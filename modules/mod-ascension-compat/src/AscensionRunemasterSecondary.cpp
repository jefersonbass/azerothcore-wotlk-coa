/* Copyright (C) 2016+ AzerothCore, GNU AGPL v3. */
#include "Player.h"
#include "Random.h"
#include "ScriptMgr.h"
#include "Spell.h"
#include "SpellAuraEffects.h"
#include "SpellAuras.h"
#include "SpellMgr.h"
#include "CellImpl.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "SpellScript.h"
#include <algorithm>
#include <limits>

namespace
{
enum RunemasterSecondarySpells : uint32
{
    SPELL_RUNEBLADE = 707141,
    SPELL_FISTS_OF_POWER = 92153,
    SPELL_FISTS_AMOUNT = 500462,
    SPELL_FISTS_HIT = 712298,
    SPELL_RIFTBLADE = 92154,
    SPELL_RIFTBLADE_COUNTER = 500468,
    SPELL_RIFTBLADE_MANA = 500466,
    SPELL_WATER_AMOUNT = 802645,
    SPELL_PRIMORDIAL_BLAST = 800732,
    SPELL_ELEMENTAL_MASTERY = 806711,
    SPELL_RUNIC_BRAND = 712299,
    SPELL_POWER_OVERWHELMING = 707876,
    SPELL_SMOLDER = 801087,
    SPELL_WARPDAGGER = 500287,
    SPELL_SPELLFIRE_RUNES = 801511,
    SPELL_SPELLFIRE_CHANCE = 802661,
    SPELL_SPELLFIRE_READY = 801512,
    SPELL_UNLEASHED_TATTOOS = 804561,
    SPELL_UNLEASHED_FIRE = 807377,
    SPELL_UNLEASHED_WATER = 807378,
    SPELL_FIRE_TATTOO = 801106,
    SPELL_WATER_TATTOO = 801107,
    SPELL_ARCANE_SIGIL = 805380,
    SPELL_ARCANE_SIGIL_DOT = 807819,
    SPELL_ARCANE_SIGIL_SILENCE = 808020,
    SPELL_FIRE_ENGRAVING = 653211,
    SPELL_FIREBRAND = 653210,
    SPELL_FIREBRAND_EXPLOSION = 653212,
    SPELL_PRIMORDIAL_FURY = 806543,
    PRIMORDIAL_FURY_CLEAVE_RADIUS = 10,
    PRIMORDIAL_FURY_CLEAVE_TARGETS = 5,
    SPELL_WINDSAGE = 705568,
    SPELL_WINDSAGE_STRIKE = 706457,
    WINDSAGE_CHARGES = 3,
    WINDSAGE_PCT = 75
};

bool HasTattoo(Player* player, uint32 root)
{
    for (auto const& [key, application] : player->GetAppliedAuras())
    {
        Aura* aura = application->GetBase();
        if (aura->GetCasterGUID() == player->GetGUID() && sSpellMgr->GetFirstSpellInChain(aura->GetId()) == root)
            return true;
    }
    return false;
}

uint32 KnownRank(Player* player, uint32 root)
{
    for (auto const& [id, entry] : player->GetSpellMap())
        if (player->HasActiveSpell(id) && sSpellMgr->GetFirstSpellInChain(id) == root)
            return id;
    return 0;
}

void SyncUnleashed(Player* player)
{
    bool active = player->IsAlive() && player->HasAura(SPELL_UNLEASHED_TATTOOS, player->GetGUID());
    for (auto const& [tattoo, helper] : {std::pair{SPELL_FIRE_TATTOO, SPELL_UNLEASHED_FIRE},
        std::pair{SPELL_WATER_TATTOO, SPELL_UNLEASHED_WATER}})
        if (!active || !HasTattoo(player, tattoo))
            player->RemoveAurasDueToSpell(helper, player->GetGUID());
        else if (!player->HasAura(helper, player->GetGUID()))
            player->AddAura(helper, player);
}

class runemaster_secondary_auras : public UnitScript
{
public:
    runemaster_secondary_auras() : UnitScript("runemaster_secondary_auras", true,
        {UNITHOOK_ON_AURA_APPLY, UNITHOOK_ON_AURA_REMOVE, UNITHOOK_MODIFY_SPELL_EFFECT_BASE_VALUE}) { }

    void OnAuraApply(Unit* unit, Aura* aura) override
    {
        Player* player = unit ? unit->ToPlayer() : nullptr;
        if (!player || player->getClass() != CLASS_SPIRIT_MAGE || !aura)
            return;
        uint32 root = sSpellMgr->GetFirstSpellInChain(aura->GetId());
        if (root == SPELL_UNLEASHED_TATTOOS || root == SPELL_FIRE_TATTOO || root == SPELL_WATER_TATTOO)
            SyncUnleashed(player);
    }

    void OnAuraRemove(Unit* unit, AuraApplication* application, AuraRemoveMode) override
    {
        Player* player = unit ? unit->ToPlayer() : nullptr;
        if (!player || player->getClass() != CLASS_SPIRIT_MAGE || !application)
            return;
        Aura* aura = application->GetBase();
        uint32 root = sSpellMgr->GetFirstSpellInChain(aura->GetId());
        if (root == SPELL_UNLEASHED_TATTOOS || root == SPELL_FIRE_TATTOO || root == SPELL_WATER_TATTOO)
            SyncUnleashed(player);
        if (aura->GetCasterGUID() != player->GetGUID())
            return;
        if (root == SPELL_RIFTBLADE)
            player->RemoveAurasDueToSpell(SPELL_RIFTBLADE_COUNTER, player->GetGUID());
        if (root == SPELL_SPELLFIRE_RUNES)
            player->RemoveAurasDueToSpell(SPELL_SPELLFIRE_READY, player->GetGUID());
    }

    void ModifySpellEffectBaseValue(Unit const* caster, SpellInfo const* info, uint8 index, float& value) override
    {
        Player* player = caster ? const_cast<Unit*>(caster)->ToPlayer() : nullptr;
        if (!player || player->getClass() != CLASS_SPIRIT_MAGE || info->Id != SPELL_RIFTBLADE_MANA || index != EFFECT_0)
            return;
        value += std::max(0.0f, player->GetTotalAttackPowerValue(BASE_ATTACK)) * 0.3f;
        if (HasTattoo(player, SPELL_WATER_TATTOO))
            if (SpellInfo const* reference = sSpellMgr->GetSpellInfo(SPELL_WATER_AMOUNT))
                AddPct(value, reference->Effects[EFFECT_2].CalcValue(player));
    }
};

class runemaster_secondary_casts : public AllSpellScript
{
public:
    runemaster_secondary_casts() : AllSpellScript("runemaster_secondary_casts",
        {ALLSPELLHOOK_ON_CAST, ALLSPELLHOOK_ON_HIT_RESULT}) { }

    void OnSpellCast(Spell* spell, Unit* caster, SpellInfo const* info, bool) override
    {
        Player* player = caster ? caster->ToPlayer() : nullptr;
        if (!player || player->getClass() != CLASS_SPIRIT_MAGE || !player->IsAlive() || spell->IsTriggered())
            return;
        uint32 root = sSpellMgr->GetFirstSpellInChain(info->Id);
        // Power Overwhelming (707876): Runeblade and Primordial Blast roll
        // 35% to reset Runic Brand's cooldown. The DBC's proc trigger is a
        // dead custom effect, so the roll happens here on their damage.
        if ((root == SPELL_RUNEBLADE || root == SPELL_PRIMORDIAL_BLAST) &&
            player->HasAura(SPELL_POWER_OVERWHELMING) && roll_chance_i(35))
            player->RemoveSpellCooldown(SPELL_RUNIC_BRAND, true);
        // Elemental Mastery (806711): Runic Brand damage rolls 33% to
        // transform the next Primordial Blast into a random unique elemental
        // version of itself. The DBC's proc names no trigger spell, so the
        // roll happens here on Brand damage and the transform on Blast cast.
        if (root == SPELL_RUNIC_BRAND && player->HasAura(SPELL_ELEMENTAL_MASTERY) && roll_chance_i(33))
            player->CastSpell(player, SPELL_ELEMENTAL_MASTERY, true);
        if (root == SPELL_PRIMORDIAL_BLAST && player->HasAura(SPELL_ELEMENTAL_MASTERY))
        {
            static uint32 const bursts[] = {502828, 502829, 502830, 502831, 502832, 502833,
                502834, 502835, 502836, 502837, 502838, 802202};
            player->RemoveAurasDueToSpell(SPELL_ELEMENTAL_MASTERY, player->GetGUID());
            if (Unit* victim = spell->m_targets.GetUnitTarget())
                player->CastSpell(victim, bursts[urand(0, 11)], true);
            spell->cancel();
            return;
        }
        if (root == SPELL_SMOLDER)
            player->RemoveAurasDueToSpell(SPELL_SPELLFIRE_READY, player->GetGUID());
        if (player->HasAura(SPELL_RIFTBLADE, player->GetGUID()))
        {
            if (root == SPELL_PRIMORDIAL_BLAST || root == SPELL_SMOLDER)
            {
                if (uint32 rank = KnownRank(player, SPELL_RUNEBLADE))
                    player->RestoreSpellCharge(rank);
            }
            else if (root == SPELL_RUNEBLADE)
            {
                player->CastSpell(player, SPELL_RIFTBLADE_COUNTER, true);
                if (Aura* counter = player->GetAura(SPELL_RIFTBLADE_COUNTER, player->GetGUID());
                    counter && counter->GetStackAmount() >= 3)
                {
                    player->RemoveAurasDueToSpell(SPELL_RIFTBLADE_COUNTER, player->GetGUID());
                    player->CastSpell(player, SPELL_RIFTBLADE_MANA, true);
                }
            }
        }
        if ((root == SPELL_RUNEBLADE || root == SPELL_WARPDAGGER) &&
            player->HasAura(SPELL_SPELLFIRE_RUNES, player->GetGUID()))
        {
            SpellInfo const* reference = sSpellMgr->GetSpellInfo(root == SPELL_RUNEBLADE
                ? SPELL_SPELLFIRE_CHANCE : SPELL_SPELLFIRE_RUNES);
            if (reference && roll_chance_i(reference->ProcChance))
            {
                if (uint32 rank = KnownRank(player, SPELL_SMOLDER))
                    player->RemoveSpellCooldown(rank, true);
                player->AddAura(SPELL_SPELLFIRE_READY, player);
            }
        }
    }

    void OnSpellHitResult(Spell* spell, Unit* target, uint8 miss, uint32 damage, uint32, bool) override
    {
        Player* player = spell->GetCaster()->ToPlayer();
        if (!player || player->getClass() != CLASS_SPIRIT_MAGE || !player->IsAlive() || !player->IsInWorld() ||
            spell->IsTriggered() || !damage ||
            miss != SPELL_MISS_NONE || !target || !target->IsAlive() || target == player ||
            player->IsFriendlyTo(target) || !player->GetWeaponForAttack(OFF_ATTACK, true) ||
            sSpellMgr->GetFirstSpellInChain(spell->GetSpellInfo()->Id) != SPELL_RUNEBLADE ||
            !player->HasAura(SPELL_FISTS_OF_POWER, player->GetGUID()) || spell->GetScriptValue(SPELL_FISTS_HIT))
            return;
        SpellInfo const* reference = sSpellMgr->GetSpellInfo(SPELL_FISTS_AMOUNT);
        SpellInfo const* helper = sSpellMgr->GetSpellInfo(SPELL_FISTS_HIT);
        if (!reference || !helper)
            return;
        spell->SetScriptValue(SPELL_FISTS_HIT, 1);
        uint64 amount = uint64(damage) * std::clamp(reference->Effects[EFFECT_0].CalcValue(player), 0, 100) / 100;
        SpellCastTargets targets;
        targets.SetUnitTarget(target);
        CustomSpellValues values;
        values.AddSpellMod(SPELLVALUE_BASE_POINT0, int32(std::min<uint64>(amount, std::numeric_limits<int32>::max())));
        values.AddSpellMod(SPELLVALUE_MELEE_ATTACK_TYPE, OFF_ATTACK);
        player->CastSpell(targets, helper, &values, TRIGGERED_FULL_MASK);
    }
};

// Primordial Fury (806543): "Your runic tattoos flare up for 15 sec, causing
// Runeblade to strike up to 5 additional enemies." The 15 s DBC aura cannot
// turn a single-target melee hit into cleaves, so the first Runeblade damage
// of each cast strikes nearby enemies through the same melee helper the
// Fists of Power strike uses, at full damage with the main hand. The 100%
// Runic Tattoo effectiveness half rides the DBC's native Add % Modifier slot.
class runemaster_primordial_fury : public AllSpellScript
{
public:
    runemaster_primordial_fury() : AllSpellScript("runemaster_primordial_fury",
        {ALLSPELLHOOK_ON_HIT_RESULT}) { }

    void OnSpellHitResult(Spell* spell, Unit* target, uint8 miss, uint32 damage, uint32, bool) override
    {
        Player* player = spell->GetCaster()->ToPlayer();
        if (!player || player->getClass() != CLASS_SPIRIT_MAGE || !player->IsAlive() || !player->IsInWorld() ||
            spell->IsTriggered() || !damage ||
            miss != SPELL_MISS_NONE || !target || !target->IsAlive() || target == player ||
            player->IsFriendlyTo(target) ||
            sSpellMgr->GetFirstSpellInChain(spell->GetSpellInfo()->Id) != SPELL_RUNEBLADE ||
            !player->HasAura(SPELL_PRIMORDIAL_FURY) || spell->GetScriptValue(SPELL_PRIMORDIAL_FURY))
            return;
        SpellInfo const* helper = sSpellMgr->GetSpellInfo(SPELL_FISTS_HIT);
        if (!helper)
            return;
        spell->SetScriptValue(SPELL_PRIMORDIAL_FURY, 1);

        std::list<Unit*> enemies;
        Acore::AnyUnitInObjectRangeCheck check(target, PRIMORDIAL_FURY_CLEAVE_RADIUS);
        Acore::UnitListSearcher<Acore::AnyUnitInObjectRangeCheck> search(target, enemies, check);
        Cell::VisitObjects(target, search, PRIMORDIAL_FURY_CLEAVE_RADIUS);
        uint32 struck = 0;
        for (Unit* enemy : enemies)
        {
            if (struck >= PRIMORDIAL_FURY_CLEAVE_TARGETS)
                break;
            if (enemy == target || !enemy->IsAlive() || !player->IsValidAttackTarget(enemy) ||
                !target->IsWithinLOSInMap(enemy))
                continue;
            SpellCastTargets targets;
            targets.SetUnitTarget(enemy);
            CustomSpellValues values;
            values.AddSpellMod(SPELLVALUE_BASE_POINT0, int32(std::min<uint64>(damage, std::numeric_limits<int32>::max())));
            values.AddSpellMod(SPELLVALUE_MELEE_ATTACK_TYPE, BASE_ATTACK);
            player->CastSpell(targets, helper, &values, TRIGGERED_FULL_MASK);
            ++struck;
        }
    }
};

// Windsage (705568): "Level 30 Passive Smolder now envelops you in wind,
// causing your next 3 Runeblades to strike an additional time equal to 75%
// of the damage dealt as Nature damage." The talent's Proc Trigger slot is
// inert (ProcFlags 0, trigger 705569 whose aura-354 effect has no handler),
// so the envelope and the echo live here: Smolder casts grant 3 charges of
// the 706457 aura, and each non-triggered Runeblade hit consumes one charge
// to echo 75% of the dealt damage as Nature. The echo goes through
// Unit::DealDamage (not the melee helper) so the school is Nature rather
// than the weapon's physical school.
class runemaster_windsage : public AllSpellScript
{
public:
    runemaster_windsage() : AllSpellScript("runemaster_windsage",
        {ALLSPELLHOOK_ON_CAST, ALLSPELLHOOK_ON_HIT_RESULT}) { }

    void OnSpellCast(Spell* spell, Unit* caster, SpellInfo const* info, bool) override
    {
        Player* player = caster ? caster->ToPlayer() : nullptr;
        if (!player || player->getClass() != CLASS_SPIRIT_MAGE || !player->IsAlive() || spell->IsTriggered())
            return;
        if (sSpellMgr->GetFirstSpellInChain(info->Id) != SPELL_SMOLDER ||
            !player->HasAura(SPELL_WINDSAGE, player->GetGUID()))
            return;
        if (Aura* envelope = player->AddAura(SPELL_WINDSAGE_STRIKE, player))
            envelope->SetStackAmount(WINDSAGE_CHARGES);
    }

    void OnSpellHitResult(Spell* spell, Unit* target, uint8 miss, uint32 damage, uint32, bool) override
    {
        Player* player = spell->GetCaster() ? spell->GetCaster()->ToPlayer() : nullptr;
        if (!player || player->getClass() != CLASS_SPIRIT_MAGE || !player->IsAlive() || !player->IsInWorld() ||
            spell->IsTriggered() || !damage || miss != SPELL_MISS_NONE || !target || !target->IsAlive() ||
            target == player || player->IsFriendlyTo(target) ||
            sSpellMgr->GetFirstSpellInChain(spell->GetSpellInfo()->Id) != SPELL_RUNEBLADE ||
            spell->GetScriptValue(SPELL_WINDSAGE_STRIKE))
            return;
        Aura* envelope = player->GetAura(SPELL_WINDSAGE_STRIKE, player->GetGUID());
        if (!envelope)
            return;
        spell->SetScriptValue(SPELL_WINDSAGE_STRIKE, 1);
        if (envelope->GetStackAmount() <= 1)
            player->RemoveAurasDueToSpell(SPELL_WINDSAGE_STRIKE, player->GetGUID());
        else
            envelope->SetStackAmount(envelope->GetStackAmount() - 1);
        uint32 amount = uint32(std::min<uint64>(uint64(damage) * WINDSAGE_PCT / 100, std::numeric_limits<int32>::max()));
        Unit::DealDamage(player, target, amount, nullptr, SPELL_DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NATURE,
            spell->GetSpellInfo(), false);
    }
};

class aura_ascension_arcane_palm_sigil : public AuraScript
{
    PrepareAuraScript(aura_ascension_arcane_palm_sigil);

    bool Check(ProcEventInfo& event)
    {
        Unit* player = GetTarget();
        DamageInfo const* damage = event.GetDamageInfo();
        Unit* target = event.GetActionTarget();
        return player->IsPlayer() && player->getClass() == CLASS_SPIRIT_MAGE && player->IsAlive() &&
            GetCaster() == player && event.GetActor() == player && target && target->IsAlive() &&
            target != player && !player->IsFriendlyTo(target) && damage && damage->GetDamage() &&
            damage->GetDamageType() == SPELL_DIRECT_DAMAGE && (damage->GetSchoolMask() & SPELL_SCHOOL_MASK_MAGIC);
    }

    void Proc(AuraEffect const* effect, ProcEventInfo& event)
    {
        PreventDefaultAction();
        Unit* player = GetTarget();
        uint64 amount = uint64(event.GetDamageInfo()->GetDamage()) * std::clamp(effect->GetAmount(), 0, 100) / 100;
        GetAura()->Remove();
        player->CastCustomSpell(SPELL_ARCANE_SIGIL_DOT, SPELLVALUE_BASE_POINT0,
            int32(std::min<uint64>(amount, std::numeric_limits<int32>::max())),
            event.GetActionTarget(), TRIGGERED_FULL_MASK);
        player->CastSpell(event.GetActionTarget(), SPELL_ARCANE_SIGIL_SILENCE, true);
    }

    void Register() override
    {
        DoCheckProc += AuraCheckProcFn(aura_ascension_arcane_palm_sigil::Check);
        OnEffectProc += AuraEffectProcFn(aura_ascension_arcane_palm_sigil::Proc, EFFECT_0, AuraType(354));
    }
};

class aura_ascension_runemaster_fire_engraving : public AuraScript
{
    PrepareAuraScript(aura_ascension_runemaster_fire_engraving);

    bool Check(ProcEventInfo& event)
    {
        Unit* player = GetTarget();
        DamageInfo const* damage = event.GetDamageInfo();
        Unit* target = event.GetActionTarget();
        return player->IsPlayer() && player->getClass() == CLASS_SPIRIT_MAGE && event.GetActor() == player &&
            target && target != player && target->IsAlive() && damage && damage->GetDamage() &&
            damage->GetDamageType() != DOT;
    }

    void Proc(AuraEffect const*, ProcEventInfo& event)
    {
        PreventDefaultAction();
        Unit* player = GetTarget();
        Unit* target = event.GetActionTarget();
        if (Aura* brand = target->GetAura(SPELL_FIREBRAND, player->GetGUID()))
        {
            int32 const duration = brand->GetDuration();
            brand->ModStackAmount(1);
            brand->SetDuration(duration);
            return;
        }
        player->CastSpell(target, SPELL_FIREBRAND, true);
    }

    void Register() override
    {
        DoCheckProc += AuraCheckProcFn(aura_ascension_runemaster_fire_engraving::Check);
        OnEffectProc += AuraEffectProcFn(aura_ascension_runemaster_fire_engraving::Proc, EFFECT_0,
                                         SPELL_AURA_PROC_TRIGGER_SPELL);
    }
};

class aura_ascension_runemaster_firebrand : public AuraScript
{
    PrepareAuraScript(aura_ascension_runemaster_firebrand);

    void Explode(AuraEffect const*, AuraEffectHandleModes)
    {
        Unit* caster = GetCaster();
        Unit* target = GetTarget();
        if (GetTargetApplication()->GetRemoveMode() != AURA_REMOVE_BY_EXPIRE || !caster || !target->IsAlive())
            return;
        for (uint8 stack = GetStackAmount(); stack > 0; --stack)
            caster->CastSpell(target, SPELL_FIREBRAND_EXPLOSION, true);
    }

    void Register() override
    {
        AfterEffectRemove += AuraEffectRemoveFn(aura_ascension_runemaster_firebrand::Explode, EFFECT_0,
                                                SPELL_AURA_PERIODIC_DAMAGE, AURA_EFFECT_HANDLE_REAL);
    }
};

class runemaster_secondary_metadata : public GlobalScript
{
public:
    runemaster_secondary_metadata() : GlobalScript("runemaster_secondary_metadata",
        {GLOBALHOOK_ON_LOAD_SPELL_CUSTOM_ATTR}) { }

    void OnLoadSpellCustomAttr(SpellInfo* info) override
    {
        if (info->SpellFamilyName != 38)
            return;
        if (info->Id == 806982)
        {
            // Issue 663 (Fists of Power): Earthen Fists is granted by the
            // corrected 805796 proc below. Its amounts are authored correctly
            // (effect 0 resolves as +3% melee haste, aura 138; effect 1 as
            // +10% chance of success, aura 107 op 18 SPELLMOD_CHANCE_OF_
            // SUCCESS), but effect 1's mask keys maskC 0x2000000, which only
            // matches Speed Rune (572134/801103). Rekey it to the Weapon
            // Engraving: Earth record's family flags (653219, maskA
            // 0x40000000, maskB 0x4) so the engraving chance actually scales.
            info->Effects[EFFECT_1].SpellClassMask = flag96(0x40000000, 0x4, 0);
        }
        if (info->Id == SPELL_FISTS_HIT || info->Id == SPELL_ARCANE_SIGIL_DOT)
        {
            info->AttributesEx2 |= SPELL_ATTR2_CANT_CRIT;
            info->AttributesEx3 |= SPELL_ATTR3_IGNORE_CASTER_MODIFIERS;
            info->AttributesEx4 |= SPELL_ATTR4_IGNORE_DAMAGE_TAKEN_MODIFIERS;
            info->AscensionInheritsResolvedAmount = true;
            info->Effects[EFFECT_0].BonusMultiplier = 0.0f;
        }
        // Ley Magician (804580): "Increases your spell damage by 30% of Spirit
        // and 10% of your Intellect, and spell hit rating by 6% of your Spirit."
        // The shipped aura slots carry trigger-only misc values, so rebind the
        // rating source and both damage sources to their intended stats.
        if (info->Id == 804580)
        {
            SpellEffectInfo& rating = info->Effects[EFFECT_0];
            rating.ApplyAuraName = SPELL_AURA_MOD_RATING_FROM_STAT;
            rating.BasePoints = 5;      // DieSides 2 -> 6
            rating.DieSides = 2;
            rating.MiscValue = 1 << CR_HIT_SPELL;
            rating.MiscValueB = STAT_SPIRIT;
            for (uint8 effectIndex : {EFFECT_1, EFFECT_2})
            {
                SpellEffectInfo& damage = info->Effects[effectIndex];
                damage.ApplyAuraName = SPELL_AURA_MOD_SPELL_DAMAGE_OF_STAT_PERCENT;
                damage.MiscValue = SPELL_SCHOOL_MASK_ALL;
            }
            info->Effects[EFFECT_1].MiscValueB = STAT_INTELLECT;
            info->Effects[EFFECT_2].MiscValueB = STAT_SPIRIT;
        }
        if (info->Id == SPELL_UNLEASHED_FIRE || info->Id == SPELL_UNLEASHED_WATER ||
            info->Id == SPELL_SPELLFIRE_READY || info->Id == SPELL_RIFTBLADE_COUNTER)
        {
            info->AttributesCu &= ~SPELL_ATTR0_CU_FORCE_AURA_SAVING;
            info->AttributesCu |= SPELL_ATTR0_CU_AURA_CANNOT_BE_SAVED;
        }
        if (info->Id == 705540)
        {
            // Issue 817: Spellblade ships without SPELL_ATTR0_PASSIVE, so the
            // learn/login passes never applied its stat auras, and its
            // BasePoints are display-minus-1 with DieSides 0. Mark passive
            // and shift DieSides to 1 so both effects resolve as the
            // tooltip's 5% (effect 0 misc 1 = Agility, effect 1 misc 3 =
            // Intellect). The native MOD_TOTAL_STAT_PERCENTAGE handler
            // applies them.
            info->Attributes |= SPELL_ATTR0_PASSIVE;
            info->Effects[EFFECT_0].DieSides = 1;
            info->Effects[EFFECT_1].DieSides = 1;
        }
        if (info->Id == 806993)
        {
            // Issue 862: Devastating Flames ships without SPELL_ATTR0_PASSIVE,
            // so the learn/login passes never applied its damage mod, and its
            // BasePoints are display-minus-1 with DieSides 0. Mark passive
            // and shift DieSides to 1 so the value resolves as the tooltip's
            // 20% (op 22 = SPELLMOD_DOT... actually op 22 with misc 22: the
            // authored op is 22/SPELLMOD_DOT but the mask is empty, so
            // retarget it as op 0/SPELLMOD_DAMAGE on Weapon Engraving: Fire's
            // explosion 653210, family-38 flags 0x20000/0x10). The native
            // damage-mod path applies it.
            info->Attributes |= SPELL_ATTR0_PASSIVE;
            info->Effects[EFFECT_0].DieSides = 1;
            info->Effects[EFFECT_0].MiscValue = SPELLMOD_DAMAGE;
            info->Effects[EFFECT_0].SpellClassMask = flag96(0x20000, 0x10, 0);
        }
        if (info->Id == 706671)
        {
            // Issue 684: Elemental Acuity's effect 0 is the authored half (+5%
            // Fire/Frost/Nature damage done, aura 79, misc 28 = that school
            // mask). Effect 1 is the real defect: an AP-coefficient scaling
            // whose authored op 32 is beyond MAX_SPELLMOD, so the engine drops
            // it. Rebind it as the percentage AP-coefficient script on
            // Runeblade (family-38 flags[2] 0x40000), resolving as the
            // tooltip's +25% attack power scaling. The DBC already carries
            // SPELL_ATTR0_PASSIVE and DieSides 1, so the re-marks below are
            // defensive no-ops kept in case the record is ever regenerated
            // without them.
            info->Attributes |= SPELL_ATTR0_PASSIVE;
            info->Effects[EFFECT_0].DieSides = 1;
            SpellEffectInfo& apCoefficient = info->Effects[EFFECT_1];
            apCoefficient.ApplyAuraName = SPELL_AURA_OVERRIDE_CLASS_SCRIPTS;
            apCoefficient.MiscValue = ASCENSION_DIRECT_AP_COEFFICIENT_PCT;
            apCoefficient.DieSides = 1;
            apCoefficient.SpellClassMask = flag96(0, 0, 0x40000);
        }
    }
};
}

void AddSC_AscensionRunemasterSecondary()
{
    new runemaster_secondary_auras();
    new runemaster_secondary_casts();
    new runemaster_primordial_fury();
    new runemaster_windsage();
    new runemaster_secondary_metadata();
    RegisterSpellScript(aura_ascension_arcane_palm_sigil);
    RegisterSpellScript(aura_ascension_runemaster_fire_engraving);
    RegisterSpellScript(aura_ascension_runemaster_firebrand);
}
