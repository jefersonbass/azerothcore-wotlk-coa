/* Copyright (C) 2016+ AzerothCore, GNU AGPL v3. */

#include "AscensionWitchDoctorCompletion.h"
#include "AscensionWitchDoctorCoefficients.h"
#include "CellImpl.h"
#include "DBCStores.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "Spell.h"
#include "SpellAuraEffects.h"
#include "SpellAuras.h"
#include "SpellMgr.h"
#include "SpellScript.h"
#include <algorithm>
#include <memory>
#include <mutex>
#include <unordered_map>

namespace AscensionWitchDoctor
{
namespace
{
std::unordered_map<ObjectGuid, std::unique_ptr<DoctorState>> states;
std::mutex stateMutex;
} // namespace

Player* Owner(Unit const* unit)
{
    Player* player = unit ? unit->GetCharmerOrOwnerPlayerOrPlayerItself() : nullptr;
    return player && player->getClass() == CLASS_WITCH_DOCTOR ? player : nullptr;
}
DoctorState& State(Player* player)
{
    std::lock_guard<std::mutex> lock(stateMutex);
    // The map is locked for the lookup only: the caller then reads and writes the state with no
    // lock held. Kept by pointer, the state itself never moves, so an insert for another player
    // rehashing the map cannot leave that caller writing into freed memory.
    return *states.try_emplace(player->GetGUID(), std::make_unique<DoctorState>()).first->second;
}
void Forget(Player* player)
{
    std::lock_guard<std::mutex> lock(stateMutex);
    states.erase(player->GetGUID());
}

std::list<Unit*> Nearby(Unit* center, float range)
{
    std::list<Unit*> units;
    if (!center || !center->IsInWorld())
        return units;
    Acore::AnyUnitInObjectRangeCheck check(center, range);
    Acore::UnitListSearcher<Acore::AnyUnitInObjectRangeCheck> search(center, units, check);
    Cell::VisitObjects(center, search, range);
    units.remove_if([center](Unit* unit) { return !unit->IsAlive() || !center->InSamePhase(unit); });
    return units;
}
bool Friendly(Player* player, Unit* target)
{
    return player && target && target->IsAlive() && player->IsInMap(target) && player->InSamePhase(target) &&
           player->IsFriendlyTo(target) && (player == target || player->IsInRaidWith(target));
}
std::list<Unit*> Allies(Player* player, Unit* center, float range, uint32 cap)
{
    auto units = Nearby(center, range);
    units.remove_if([player, center](Unit* unit)
                    { return !Friendly(player, unit) || !center->IsWithinLOSInMap(unit); });
    units.sort(
        [](Unit* a, Unit* b)
        {
            if (a->GetHealthPct() != b->GetHealthPct())
                return a->GetHealthPct() < b->GetHealthPct();
            return a->GetGUID() < b->GetGUID();
        });
    if (cap && units.size() > cap)
        units.resize(cap);
    return units;
}
int32 Amount(uint32 id, uint8 effect, Unit* caster)
{
    SpellInfo const* info = sSpellMgr->GetSpellInfo(id);
    return info ? info->Effects[effect].CalcValue(caster) : 0;
}
void Cast(Unit* caster, Unit* target, uint32 id)
{
    if (caster && target && target->IsAlive())
        caster->CastSpell(target, id, true);
}
void Copy(Unit* caster, Unit* target, uint32 id, uint32 amount)
{
    if (caster && target && target->IsAlive() && amount)
        caster->CastCustomSpell(id, SPELLVALUE_BASE_POINT0, int32(std::min(amount, uint32(INT32_MAX))), target, true);
}
uint8 Spirits(Player* player)
{
    Aura const* aura = player ? player->GetAura(Spirit) : nullptr;
    return aura ? aura->GetStackAmount() : 0;
}
void SyncSpirits(Player* player)
{
    uint8 count = Spirits(player);
    for (uint32 id : {SpiritStats, SpiritCast, SpiritChance})
    {
        if (!count)
            player->RemoveAurasDueToSpell(id);
        else
        {
            Aura* aura = player->GetAura(id);
            if (!aura)
                aura = player->AddAura(id, player);
            if (aura)
            {
                aura->SetStackAmount(count);
                aura->RecalculateAmountOfEffects();
            }
        }
    }
    if (AuraEffect* effect = player->GetAuraEffect(JujuSpirits, EFFECT_0))
        effect->ChangeAmount(Amount(JujuSpirits) * count);
    if (count && player->HasAura(SpiritWalk))
    {
        if (Aura* speed = player->AddAura(SpiritSpeed, player))
            speed->SetStackAmount(count);
    }
    else
        player->RemoveAurasDueToSpell(SpiritSpeed);
}
void GainSpirit(Player* player, uint8 count)
{
    if (!player || !player->IsAlive())
        return;
    for (uint8 i = 0; i < count; ++i)
    {
        Cast(player, player, Spirit);
        if (player->HasAura(SpiritDevotee))
            Cast(player, player, SpiritMana);
    }
    SyncSpirits(player);
}
uint32 KnownRank(Player* player, uint32 root)
{
    uint32 result = root;
    for (auto const& [id, state] : player->GetSpellMap())
        if (state->State != PLAYERSPELL_REMOVED && state->Active &&
            sSpellMgr->GetFirstSpellInChain(id) == sSpellMgr->GetFirstSpellInChain(root) &&
            sSpellMgr->GetSpellRank(id) > sSpellMgr->GetSpellRank(result))
            result = id;
    return result;
}
void Reduce(Player* player, uint32 root, int32 milliseconds)
{
    if (!player)
        return;
    for (auto const& [id, state] : player->GetSpellMap())
        if (state->State != PLAYERSPELL_REMOVED &&
            sSpellMgr->GetFirstSpellInChain(id) == sSpellMgr->GetFirstSpellInChain(root))
        {
            if (milliseconds == INT32_MAX)
                player->RemoveSpellCooldown(id, true);
            else
                player->ModifySpellCooldown(id, -milliseconds);
        }
}
Aura* OwnedHex(Player* player, Unit* target)
{
    for (auto const& [key, app] : target->GetAppliedAuras())
        if (Aura* aura = app->GetBase(); aura->GetCasterGUID() == player->GetGUID() && IsHex(aura->GetSpellInfo()))
            return aura;
    return nullptr;
}
void SpreadHex(Player* player, Unit* target)
{
    Aura* source = OwnedHex(player, target);
    if (!source)
        return;
    for (Unit* unit : Nearby(target, 10.0f))
        if (unit != target && player->IsValidAttackTarget(unit) && !OwnedHex(player, unit) &&
            target->IsWithinLOSInMap(unit))
            if (Aura* copy = player->AddAura(source->GetId(), unit))
            {
                copy->SetDuration(source->GetDuration());
                if (AuraEffect* effect = copy->GetEffect(EFFECT_0))
                    if (AuraEffect* original = source->GetEffect(EFFECT_0))
                    {
                        effect->ChangeAmount(original->GetAmount());
                        effect->SetPeriodicTimer(original->GetPeriodicTimer());
                    }
                break;
            }
}

void ApplyContracts(SpellInfo* info)
{
    if (!info || (info->SpellFamilyName != 19 && info->Id != BeamMarker))
        return;
    uint32 id = info->Id;
    auto dummy = [info](uint8 index)
    {
        info->Effects[index].ApplyAuraName = SPELL_AURA_DUMMY;
        info->Effects[index].TriggerSpell = 0;
    };
    auto periodic = [info](uint8 index, uint32 amplitude)
    {
        info->Effects[index].Effect = SPELL_EFFECT_APPLY_AURA;
        info->Effects[index].ApplyAuraName = SPELL_AURA_PERIODIC_DUMMY;
        info->Effects[index].Amplitude = amplitude;
        info->Effects[index].TriggerSpell = 0;
    };
    if (id == Shadowhunter)
        dummy(EFFECT_1);
    if (id == 707505)
    {
        // Jin'do's Wrath keys both halves to Hexfire Wrath's family mask: the flat
        // half reads as crit chance and the percent half as crit damage.
        info->Effects[EFFECT_0].MiscValue = SPELLMOD_CRITICAL_CHANCE;
        info->Effects[EFFECT_1].MiscValue = SPELLMOD_CRIT_DAMAGE_BONUS;
    }
    if (id == 705922)
    {
        // Dark Magic's flat modifiers read as cast-time trims keyed to Malefic Wrath
        // and Bad Juju by the shipped masks.
        info->Effects[EFFECT_0].MiscValue = SPELLMOD_ACTIVATION_TIME;
        info->Effects[EFFECT_1].MiscValue = SPELLMOD_ACTIVATION_TIME;
    }
    if (id == 705903 || id == 705904)
    {
        // Wizened's mana half keys to the max-mana percent aura; its cost half is
        // consumed in SpellInfo::CalcPowerCost and its regen half already reads fine.
        info->Effects[EFFECT_0].ApplyAuraName = SPELL_AURA_MOD_INCREASE_ENERGY_PERCENT;
        info->Effects[EFFECT_0].MiscValue = POWER_MANA;
    }
    if ((id == ChosenOne || id == MojoHigh) &&
        info->Effects[EFFECT_0].ApplyAuraName == SPELL_AURA_ADD_FLAT_MODIFIER &&
        info->Effects[EFFECT_0].MiscValue == SPELLMOD_EFFECT3)
        info->Effects[EFFECT_0].SpellClassMask = flag96(512, 0, 0); // Mimic Ward's summon count
    // "Summon a mimic ward" - one, with Chosen One adding the second. The record's summon count reads
    // as two once the core applies its base-point convention, so the ward always arrived doubled and
    // Chosen One pushed it to three.
    if (id == Mimic && info->Effects[EFFECT_2].Effect == SPELL_EFFECT_SUMMON)
        info->Effects[EFFECT_2].BasePoints = 1;
    if (id == 572871)
    {
        // Issue 803: Presence of the Loa ships without SPELL_ATTR0_PASSIVE,
        // so the learn/login passes never applied its crit aura, and its
        // BasePoints are display-minus-1 with DieSides 0. Mark passive and
        // shift DieSides to 1 so the value resolves as the tooltip's 4%.
        // The native SPELL_AURA_MOD_CRIT_PCT handler covers both melee and
        // spell crit (Player::GetMeleeCritChance, Unit::GetUnitSpellCriticalChance).
        info->Attributes |= SPELL_ATTR0_PASSIVE;
        info->Effects[EFFECT_0].DieSides = 1;
    }
    if (Family(info, 0, 4))
    {
        // Keep all victims in one cast, including rank coefficients and actual hit accounting.
        info->Effects[EFFECT_0].Effect = SPELL_EFFECT_SCHOOL_DAMAGE;
        info->Effects[EFFECT_0].TriggerSpell = 0;
    }
    // Each Call of Sseratus summon also triggers The True Spirit's buff, talent or not. Summon() already grants
    // it per ward when the talent is known.
    if (id == CallSseratus && info->Effects[EFFECT_1].TriggerSpell == TrueSpiritReady)
        info->Effects[EFFECT_1].Effect = 0;
    if (id == ShadowhunterCost)
        info->Effects[EFFECT_0].Effect = info->Effects[EFFECT_1].Effect = 0;
    if (id == Spirit)
        periodic(EFFECT_2, 500);
    if (id == SpiritCast || id == SpiritChance || id == SpiritVisual)
        for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
            dummy(i);
    if (id == SpiritStats)
    {
        dummy(EFFECT_0);
        info->Effects[EFFECT_1].SpellClassMask = flag96(536870912, 2147483648, 0);
    }
    if (id == JujuSpirits)
    {
        info->Effects[EFFECT_0].MiscValue = SPELLMOD_CRITICAL_CHANCE;
        info->Effects[EFFECT_0].SpellClassMask = flag96(0, 1024, 2);
    }
    if (id == VoodooMind || id == SoulFeeder || id == RitualOne || id == RitualTwo || id == Gift)
        for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
            dummy(i);
    if (id == Gift)
        info->Effects[EFFECT_0].ApplyAuraName = SPELL_AURA_MOD_RANGED_HASTE;
    if (id == OutOfBottle)
        for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
            dummy(i);
    if (id == Veil)
        periodic(EFFECT_1, 1500);
    if (id == SpiritPickupBuff)
    {
        periodic(EFFECT_0, 2000);
        info->Effects[EFFECT_0].TargetA = SpellImplicitTargetInfo(TARGET_UNIT_CASTER);
        info->Effects[EFFECT_2].Effect = 0;
    }
    if (id == SpiritPickup)
        info->Effects[EFFECT_1].Effect = info->Effects[EFFECT_2].Effect = 0;
    if (id == LoaSpiritsOne || id == LoaSpiritsTwo)
        dummy(EFFECT_1);
    if (id == SpiritWalkerOne)
        info->Effects[EFFECT_1].BasePoints = 9;
    if (id == Threads)
    {
        dummy(EFFECT_1);
        dummy(EFFECT_2);
    }
    if (IsJuju(info))
        info->Effects[EFFECT_1].Effect = 0;
    if (id == PuppetHit)
        info->Effects[EFFECT_1].Effect = 0;
    if (Family(info, 0, 8))
    {
        periodic(EFFECT_0, info->Effects[EFFECT_0].Amplitude);
        info->Effects[EFFECT_1].Effect = 0;
    }
    if (Family(info, 0, 536870912) && id != EclipseHit && id != EclipseSplash)
    {
        periodic(EFFECT_0, 250);
        info->DurationEntry = sSpellDurationStore.LookupEntry(1);
        info->Effects[EFFECT_1].Effect = 0;
        info->Speed = 0.0f; // snapshot the spend into the immediate aura before consuming Spirits
    }
    if (id == EclipseHit || id == EclipseSplash || id == PuppetHit)
    {
        info->AttributesEx3 |= SPELL_ATTR3_IGNORE_CASTER_MODIFIERS;
        for (SpellEffectInfo& effect : info->Effects)
        {
            effect.RealPointsPerLevel = 0.0f;
            effect.BonusMultiplier = 0.0f;
        }
    }
    if (Family(info, 0, 33554432))
        dummy(EFFECT_1); // Godslayer runs once on expiry, never as an unconditional periodic spell.
    if (id == Frenzy)
    {
        dummy(EFFECT_2);
        info->DurationEntry = sSpellDurationStore.LookupEntry(1);
    }
    if (IsBeam(info))
    {
        periodic(EFFECT_0, 500);
        info->Effects[EFFECT_1].Effect = info->Effects[EFFECT_2].Effect = 0;
        info->AttributesEx5 &= ~SPELL_ATTR5_SPELL_HASTE_AFFECTS_PERIODIC;
    }
    if (id == BeamMarker)
        dummy(EFFECT_0);
    if (id == BeamCost)
    {
        info->Effects[EFFECT_0].SpellClassMask = flag96(0, 2048, 536870913);
        info->Effects[EFFECT_1].MiscValue = SPELLMOD_CASTING_TIME;
        info->Effects[EFFECT_1].BasePoints = -10001;
        info->Effects[EFFECT_1].SpellClassMask = info->Effects[EFFECT_0].SpellClassMask;
        info->Effects[EFFECT_2].Effect = 0;
    }
    if (IsSplash(info))
        info->Effects[EFFECT_1].Effect = info->Effects[EFFECT_2].Effect = 0;
    if (id == Tiki)
    {
        info->Effects[EFFECT_1].Effect = info->Effects[EFFECT_2].Effect = 0;
        info->Effects[EFFECT_0].TriggerSpell = 0;
        info->Effects[EFFECT_0].ApplyAuraName = SPELL_AURA_PERIODIC_HEAL;
    }
    if (id == PotionThistle || id == SplashThistle || id == ConcoctionsBuff || id == Devotion || id == BottleLink)
        dummy(EFFECT_0);
    if (id == ThistleHeal || id == FrenzyHeal || id == ConcoctionsHeal)
    {
        info->Effects[EFFECT_0].Effect = SPELL_EFFECT_HEAL;
        info->Effects[EFFECT_0].TargetA = SpellImplicitTargetInfo(TARGET_UNIT_CASTER);
        info->Effects[EFFECT_0].BasePoints = 0;
        info->Effects[EFFECT_0].DieSides = 1;
    }
    if (id == MojoFish || id == MojoShrooms || id == MojoThistle)
        for (SpellEffectInfo& effect : info->Effects)
            if (!effect.IsAura())
                effect.Effect = 0;
    if (IsIngredient(id))
    {
        info->StackAmount = 1;
        info->DurationEntry = sSpellDurationStore.LookupEntry(21);
    }
    if (id == Crystal)
        dummy(EFFECT_0);
    if (id == Beast)
    {
        info->Effects[EFFECT_0].Effect = SPELL_EFFECT_APPLY_AREA_AURA_RAID;
        info->Effects[EFFECT_0].ApplyAuraName = SPELL_AURA_DUMMY;
        info->Effects[EFFECT_0].TargetA = SpellImplicitTargetInfo(TARGET_UNIT_CASTER);
        info->Effects[EFFECT_0].TriggerSpell = 0;
    }
    if (id == BeastShield)
        periodic(EFFECT_1, 1000);
    if (id == Vigil)
        periodic(EFFECT_1, 1000);
    if (id == Slither)
        info->Effects[EFFECT_2].Effect = 0;
    if (id == StalkerSpeed)
        periodic(EFFECT_1, 500);
    if (id == Mirage)
        info->Effects[EFFECT_1].Effect = 0; // exactly five Spirits from the successful cast
    if (id == SenjinSwiftness)
        info->Effects[EFFECT_1].SpellClassMask = flag96(0, 0, 1073741824); // was empty, so its -60s cooldown mod matched every WD spell instead of just Mirage
    if (id == VillageWisdom)
        for (SpellEffectInfo& effect : info->Effects)
            if (effect.ApplyAuraName == SPELL_AURA_ADD_PCT_MODIFIER)
            {
                // The % half must read as the damage op keyed to Reclamation's family bit
                // (the word Volley copies to inherit Reclamation modifiers); its +5% hit
                // half is the hit-chance aura and natively supported.
                effect.MiscValue = SPELLMOD_DAMAGE;
                effect.SpellClassMask = flag96(0, 4, 0);
            }
    if (id == SenjinWisdom)
        info->Effects[EFFECT_0].SpellClassMask = flag96(0, 0, 1073741824); // pointed at the wrong classmask word, so its +20s duration mod never matched Mirage
    if (id == 706368)
    {
        // Loa Empowerment: the -50% cost half misses Power Wuju and the +20%
        // Power Wuju half ships with an empty mask; key the cost half to every
        // Wuju (Spirit/Resourceful word 0 bit 8, Power word 2 bit 24) and the
        // Power half to Power Wuju alone.
        info->Effects[EFFECT_0].SpellClassMask = flag96(256, 0, 16777216);
        info->Effects[EFFECT_1].SpellClassMask = flag96(0, 0, 16777216);
    }
    if (id == 705856 || id == 707858)
        // Fresh Ingredients: the pct-modifier aura ships with op 0 (damage) and
        // an empty mask, so it never engages. Point it at every effect of the
        // Voodoo Cauldron (word 1, bit 24) so the cauldron's healing pulse
        // (VoodooCauldron EFFECT_2 read via CalcValue) gains the rank bonus.
        info->Effects[EFFECT_0].MiscValue = SPELLMOD_ALL_EFFECTS,
        info->Effects[EFFECT_0].SpellClassMask = flag96(0, 16777216, 0);
    if (id == RageBrewBuff)
        info->Effects[EFFECT_1].BasePoints = 14;
    if (id == Voice)
        info->Effects[EFFECT_2].ApplyAuraName = SPELL_AURA_MOD_PERCENT_STAT;
    if (id == Marionette)
    {
        info->Effects[EFFECT_0].ApplyAuraName = SPELL_AURA_MOD_ROOT;
        info->Effects[EFFECT_1].Effect = info->Effects[EFFECT_2].Effect = 0;
    }
    if (id == MarionetteStacks)
    {
        info->Effects[EFFECT_0].ApplyAuraName = SPELL_AURA_HASTE_SPELLS;
        info->Effects[EFFECT_0].BasePoints = 0;
        info->Effects[EFFECT_0].DieSides = 1;
        info->Effects[EFFECT_0].TargetA = SpellImplicitTargetInfo(TARGET_UNIT_CASTER);
        info->Effects[EFFECT_1].Effect = 0;
    }
    if (id == MarionetteExplosion)
        info->Effects[EFFECT_2].Effect = 0;
    if (id == MalignantJinx)
        dummy(EFFECT_0);
    if (id == PriceReady || id == DambalaReady)
        info->Effects[EFFECT_2].Effect = 0;
    if (id == SenjinBuff)
        info->Effects[EFFECT_1].Effect = 0;
    if (id == SenjinBuff || id == MojoFree || id == OtherSideBuff || id == PriceReady || id == DambalaReady ||
        id == TrueSpiritReady || id == OverflowBuff || id == ConcoctionsBuff)
        info->ProcCharges = 0;
    if (id == Volley)
    {
        info->Effects[EFFECT_1].Effect = info->Effects[EFFECT_2].Effect = 0;
        info->SpellFamilyFlags[1] |= 4; // inherits Reclamation spell modifiers
    }
    if (id == Umbral)
    {
        info->Effects[EFFECT_1].Effect = 0;
        info->SpellFamilyFlags[1] |= 33554432; // inherits Hex modifiers without becoming the stored Hex
    }
    if (id == HexfireWrath || id == Umbral)
    {
        info->MaxAffectedTargets = 3;
        for (SpellEffectInfo& effect : info->Effects)
        {
            effect.TargetB = SpellImplicitTargetInfo();
            effect.ChainTarget = 1; // explicit script adds the same two victims to every relevant effect
        }
    }
    if (id == HexfireWrath)
        info->Effects[EFFECT_2].Effect = 0;
    if (id == BigVoodoo)
        info->Effects[EFFECT_1].Effect = info->Effects[EFFECT_2].Effect = 0;
    if (id == VoodooCauldron)
        info->Effects[EFFECT_2].Effect = 0; // the owned, stationary cauldron AI pulses the rank amount
    if (id == WarGolem)
    {
        SpellEffectInfo& absorb = info->Effects[EFFECT_1];
        absorb.Effect = SPELL_EFFECT_APPLY_AURA;
        absorb.ApplyAuraName = SPELL_AURA_SCHOOL_ABSORB;
        absorb.MiscValue = SPELL_SCHOOL_MASK_ALL;
        absorb.BasePoints = 0;
        absorb.DieSides = 1;
        absorb.TargetA = SpellImplicitTargetInfo(TARGET_UNIT_CASTER);
    }
    for (uint32 child : {WardHeal, SpiritManaTick, Cleanse, SereneField, DarkField, JungleFieldSpell, SwiftField,
                         CauldronBuff, BigVoodooField, VoodooProtectionSpell})
        if (id == child)
            for (SpellEffectInfo& effect : info->Effects)
                if (effect.IsEffect())
                {
                    if (effect.IsAura())
                        effect.Effect = SPELL_EFFECT_APPLY_AURA;
                    effect.TargetA = SpellImplicitTargetInfo(TARGET_UNIT_TARGET_ALLY);
                    effect.TargetB = SpellImplicitTargetInfo();
                }
    for (uint32 child : {ShadowField, ShadowSlow, GravenField, CursedField, SentryRevealSpell, EclipseSplash,
                         GlaiveExplosion, VoodooFire, ViperFire})
        if (id == child)
            for (SpellEffectInfo& effect : info->Effects)
                if (effect.IsEffect())
                {
                    if (effect.IsAura())
                        effect.Effect = SPELL_EFFECT_APPLY_AURA;
                    effect.TargetA = SpellImplicitTargetInfo(TARGET_UNIT_TARGET_ENEMY);
                    effect.TargetB = SpellImplicitTargetInfo();
                }
    if (id == ShadowField)
    {
        info->Effects[EFFECT_0].ApplyAuraName = SPELL_AURA_MOD_CASTING_SPEED_NOT_STACK;
        info->Effects[EFFECT_1].Effect = 0;
    }
    if (id == WardHaste)
    {
        info->Effects[EFFECT_0].Effect = SPELL_EFFECT_APPLY_AURA;
        info->Effects[EFFECT_0].ApplyAuraName = SPELL_AURA_HASTE_SPELLS;
        info->Effects[EFFECT_0].TargetA = SpellImplicitTargetInfo(TARGET_UNIT_TARGET_ALLY);
    }
    if (id == VoodooProtectionSpell)
        info->Effects[EFFECT_0].BasePoints = -21;
    if (id == JungleThistle)
    {
        // Current parent promises area-damage protection; the old threat effect is absent.
        info->Effects[EFFECT_0].Effect = SPELL_EFFECT_TRIGGER_SPELL;
        info->Effects[EFFECT_0].TriggerSpell = JungleProtection;
        info->Effects[EFFECT_2].BasePoints = 19; // changelog 69822: secondary heal +10 percentage points
    }
    if (id == SentryRevealSpell)
    {
        for (SpellEffectInfo& effect : info->Effects)
            effect.Effect = 0;
        info->Effects[EFFECT_0].Effect = SPELL_EFFECT_APPLY_AURA;
        info->Effects[EFFECT_0].ApplyAuraName = SPELL_AURA_MOD_STALKED;
    }
    if (id == Stasis)
        info->Effects[EFFECT_1].Effect = 0; // burst timing belongs to the ward AI
    for (SpellEffectInfo& effect : info->Effects)
        if (effect.Effect == SPELL_EFFECT_SUMMON)
            effect.MiscValueB = 64; // creation, ownership and slots are provided by the class summon script
    if (id == UnstableHeal)
    {
        info->Effects[EFFECT_0].TargetA = SpellImplicitTargetInfo(TARGET_UNIT_TARGET_ALLY);
        info->Effects[EFFECT_0].TargetB = SpellImplicitTargetInfo();
    }
    if (id == MarionetteTransform)
    {
        info->Effects[EFFECT_0].SpellClassMask[2] |= 2;
        info->Effects[EFFECT_1].SpellClassMask[2] |= 2;
    }
    if (id == Hexed)
    {
        info->Effects[EFFECT_1].ApplyAuraName = SPELL_AURA_MOD_CONFUSE;
        info->Effects[EFFECT_2].ApplyAuraName = SPELL_AURA_MOD_ROOT;
        info->AuraInterruptFlags |= AURA_INTERRUPT_FLAG_TAKE_DAMAGE;
    }
    if (IsBeam(info))
        info->ManaPerSecond = 0;
    if (id == Frenzy)
        info->Effects[EFFECT_1].MiscValueB = 0;
    for (DoctorCoefficient const& coefficient : DoctorCoefficients)
        if (coefficient.id == id)
            info->Effects[coefficient.effect].BonusMultiplier = 0.0f;
    for (uint32 child : {ThreadsDamage, BottleDamage, LoaEchoHeal, WaveHeal, ThistleHeal, ConcoctionsHeal, DevotionHeal,
                         FrenzyHeal, EclipseHit, ShadowflareHit, PuppetHit})
        if (id == child)
            for (SpellEffectInfo& effect : info->Effects)
                effect.BonusMultiplier = 0.0f;
    for (uint32 child : {ThreadsDamage, BottleDamage, LoaEchoHeal, WaveHeal, ThistleHeal, ConcoctionsHeal, DevotionHeal,
                         FrenzyHeal, StringsDamage, GuileDamage})
        if (id == child)
        {
            info->AttributesEx2 |= SPELL_ATTR2_CANT_CRIT;
            info->AttributesEx3 |= SPELL_ATTR3_IGNORE_CASTER_MODIFIERS;
            info->AttributesEx4 |= SPELL_ATTR4_IGNORE_DAMAGE_TAKEN_MODIFIERS;
            for (SpellEffectInfo& effect : info->Effects)
                if (effect.IsEffect())
                {
                    bool heal = effect.Effect == SPELL_EFFECT_HEAL;
                    effect.TargetA = SpellImplicitTargetInfo(heal ? TARGET_UNIT_TARGET_ALLY : TARGET_UNIT_TARGET_ENEMY);
                    effect.TargetB = SpellImplicitTargetInfo();
                }
            // Contracts run after the core caches this mask; keep the explicit recipient of copied effects.
            info->_InitializeExplicitTargetMask();
        }
    if (id == JungleSecretsHeal)
    {
        // A share of the effective Brew heal, with one explicitly selected recipient per effigy.
        info->DmgClass = SPELL_DAMAGE_CLASS_NONE;
        info->AttributesEx2 |= SPELL_ATTR2_CANT_CRIT;
        info->AttributesEx3 |= SPELL_ATTR3_IGNORE_CASTER_MODIFIERS;
        info->AscensionInheritsResolvedAmount = true;
        info->Effects[EFFECT_0].BonusMultiplier = 0.0f;
        info->Effects[EFFECT_0].TargetA = SpellImplicitTargetInfo(TARGET_UNIT_TARGET_ALLY);
        info->Effects[EFFECT_0].TargetB = SpellImplicitTargetInfo();
        info->_InitializeExplicitTargetMask();
    }
}
} // namespace AscensionWitchDoctor

namespace
{
using namespace AscensionWitchDoctor;
class witch_doctor_scaling : public UnitScript
{
  public:
    witch_doctor_scaling()
        : UnitScript("witch_doctor_scaling", true,
                     {UNITHOOK_MODIFY_SPELL_EFFECT_BASE_VALUE, UNITHOOK_ON_PERIODIC_DAMAGE_RESULT})
    {
    }
    void OnPeriodicDamageResult(Unit* target, Unit* attacker, uint32 damage, SpellInfo const* info) override
    {
        if (!target || !attacker || !info || !damage)
            return;
        // Loa Communion: Veil of Darkness's damage helper strikes twice as hard.
        if (Player* doctor = Owner(attacker); doctor && doctor == attacker && doctor->HasAura(705907) &&
            sSpellMgr->GetFirstSpellInChain(info->Id) == sSpellMgr->GetFirstSpellInChain(VeilDamage))
        {
            if (AuraEffect const* communion = doctor->GetAuraEffect(705907, EFFECT_0))
                damage = uint32(std::min<uint64>(UINT32_MAX, uint64(damage) * (100 + communion->GetAmount()) / 100));
        }
        Aura* aura = target->GetAura(info->Id, attacker->GetGUID());
        if (!aura || !aura->GetScriptValue(ConcoctionsBuff))
            return;
        Unit* doctor = ObjectAccessor::GetUnit(*attacker, ObjectGuid(aura->GetScriptValue(ConcoctionsHeal)));
        Copy(doctor ? doctor : attacker, attacker, ConcoctionsHeal,
             uint32(std::min<uint64>(UINT32_MAX, uint64(damage) * aura->GetScriptValue(ConcoctionsBuff) / 100)));
    }
    void ModifySpellEffectBaseValue(Unit const* caster, SpellInfo const* info, uint8 index, float& value) override
    {
        Player* player = Owner(caster);
        if (!player)
            return;
        for (DoctorCoefficient const& c : DoctorCoefficients)
            if (c.id == info->Id && c.effect == index)
            {
                float sp = float(c.healing        ? player->SpellBaseHealingBonusDone(SpellSchoolMask(c.school))
                                 : c.school == 12 ? std::max(player->SpellBaseDamageBonusDone(SPELL_SCHOOL_MASK_FIRE),
                                                             player->SpellBaseDamageBonusDone(SPELL_SCHOOL_MASK_NATURE))
                                                  : player->SpellBaseDamageBonusDone(SpellSchoolMask(c.school)));
                value += std::max(0.0f, sp) * c.sp + player->GetStat(STAT_SPIRIT) * c.spirit +
                         player->GetTotalAttackPowerValue(RANGED_ATTACK) * c.rap +
                         player->GetTotalAttackPowerValue(BASE_ATTACK) * c.ap;
            }
    }
};
class witch_doctor_sessions : public PlayerScript
{
  public:
    witch_doctor_sessions() : PlayerScript("witch_doctor_sessions", {PLAYERHOOK_ON_LOGOUT}) {}
    void OnPlayerLogout(Player* player) override { Forget(player); }
};
} // namespace
void AddAscensionWitchDoctorCompletionScripts()
{
    new witch_doctor_scaling();
    new witch_doctor_sessions();
}
