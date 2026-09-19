/* Copyright (C) 2016+ AzerothCore, GNU AGPL v3. */
#include "AscensionChronomancerTalents.h"
#include "Player.h"
#include "Random.h"
#include "ScriptMgr.h"
#include "Spell.h"
#include "SpellMgr.h"
#include "SpellScript.h"

namespace
{
enum ChronomancerTalentSpells : uint32
{
    SPELL_SHIMMERING_SHARD = 806302,
    SPELL_SHIMMER = 806303,
    SPELL_AEON_RENEWAL = 806290,
    SPELL_AEON_RESILIENCE = 806291,
    SPELL_AEON_PROTECTION = 806292,
    SPELL_AEON_OBLIVION = 806293,
    SPELL_DIMENSIONAL_DIVERGENCE = 802790,
    SPELL_DIVERGENCE_SLOW = 803301,
    SPELL_DIVERGENCE_SPEED = 803703,
    SPELL_RIPPLING_POWER = 806300,
    SPELL_RIPPLING_POWER_RANK_2 = 807893,
    SPELL_DISTORTED_TIME = 707553,
    SPELL_ARCHAEOLOGY = 560130,
    SPELL_DISCOVERY = 500116
};

bool IsAeonActivation(uint32 id)
{
    return id == SPELL_AEON_RENEWAL || id == SPELL_AEON_RESILIENCE ||
        id == SPELL_AEON_PROTECTION || id == SPELL_AEON_OBLIVION;
}

bool CanSwapPlayers(Player* player, Player* target)
{
    if (!player || !target || player == target || player->getClass() != CLASS_CHRONOMANCER ||
        !player->IsAlive() || !target->IsAlive() || !player->IsInWorld() || !target->IsInWorld() ||
        player->GetMap() != target->GetMap() || !player->InSamePhase(target))
        return false;
    if (player->IsBeingTeleported() || target->IsBeingTeleported() || player->IsInFlight() || target->IsInFlight() ||
        player->GetTransport() || target->GetTransport() || player->GetVehicle() || target->GetVehicle())
        return false;
    return player->IsWithinLOSInMap(target) &&
        (player->IsValidAttackTarget(target) || player->IsValidAssistTarget(target));
}

class spell_ascension_dimensional_divergence : public SpellScript
{
    PrepareSpellScript(spell_ascension_dimensional_divergence);

    bool Validate(SpellInfo const*) override
    {
        return ValidateSpellInfo({SPELL_DIVERGENCE_SLOW, SPELL_DIVERGENCE_SPEED});
    }

    SpellCastResult CheckSwap()
    {
        Unit* target = GetExplTargetUnit();
        return CanSwapPlayers(GetCaster()->ToPlayer(), target ? target->ToPlayer() : nullptr)
            ? SPELL_CAST_OK : SPELL_FAILED_BAD_TARGETS;
    }

    void Swap(SpellEffIndex)
    {
        Player* player = GetCaster()->ToPlayer();
        Unit* hit = GetHitUnit();
        Player* target = hit ? hit->ToPlayer() : nullptr;
        if (!CanSwapPlayers(player, target))
            return;
        // Snapshot both live positions before either native teleport changes one.
        Position origin = player->GetPosition();
        Position destination = target->GetPosition();
        bool hostile = player->IsValidAttackTarget(target);
        target->NearTeleportTo(origin);
        player->NearTeleportTo(destination, true);
        if (hostile)
        {
            player->CastSpell(target, SPELL_DIVERGENCE_SLOW, true);
            if (target->HasAura(SPELL_DIVERGENCE_SLOW, player->GetGUID()))
                player->CastSpell(player, SPELL_DIVERGENCE_SPEED, true);
        }
    }

    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_ascension_dimensional_divergence::CheckSwap);
        OnEffectHitTarget += SpellEffectFn(spell_ascension_dimensional_divergence::Swap, EFFECT_0, SPELL_EFFECT_DUMMY);
    }
};

class chronomancer_talent_casts : public AllSpellScript
{
public:
    chronomancer_talent_casts() : AllSpellScript("chronomancer_talent_casts",
        {ALLSPELLHOOK_ON_CAST, ALLSPELLHOOK_ON_HIT_RESULT}) { }

    void OnSpellCast(Spell* spell, Unit* caster, SpellInfo const* info, bool) override
    {
        Player* player = caster ? caster->ToPlayer() : nullptr;
        if (!player || player->getClass() != CLASS_CHRONOMANCER || !info)
            return;
        if (info->SpellFamilyName == 28 && !spell->IsTriggered() &&
            IsAeonActivation(info->Id) && player->HasAura(SPELL_SHIMMERING_SHARD))
            player->CastSpell(player, SPELL_SHIMMER, true);

        // Resonance (706079): "Casting Artificer's Wand or Crystal Cannon now
        // has a 35% chance to reduce the cooldown of Hasten by 1 sec." Those
        // abilities carry no spell family, so the passive's native Proc
        // Trigger Spell can never match them.
        if (player->HasAura(SPELL_RESONANCE) && !spell->IsTriggered() &&
            IsArtificerCast(info->Id) && roll_chance_i(RESONANCE_CHANCE))
            if (uint32 cooldown = player->GetSpellCooldownDelay(SPELL_HASTEN))
                player->ModifySpellCooldown(SPELL_HASTEN, -std::min<uint32>(cooldown, RESONANCE_REDUCTION));

        // Incarnation of Chaos (570067): "instantly resetting the cooldown of
        // Chromatic Shard, allowing you to cast it while moving, and causing
        // your next cast to not incur a cooldown." The transform, the move
        // casting (via the buff's own DBC spellmods) and the 15% damage aura
        // run natively; the reset and free-cast handling live here. The buff
        // lasts 15 sec, matching its Add Flat/Add % Modifier cooldown slots.
        if (info->Id == SPELL_INCARNATION_OF_CHAOS)
            player->RemoveSpellCooldown(SPELL_CHROMATIC_SHARD);
        else if (info->Id == SPELL_CHROMATIC_SHARD && player->HasAura(SPELL_INCARNATION_OF_CHAOS))
            player->RemoveSpellCooldown(SPELL_CHROMATIC_SHARD);

        // Archaeology (560130): "Your Discovery now also generates 3% of your
        // base mana and health." The talent's flat-mod slots point at dead
        // operations, so pay out here on each Discovery cast.
        if (info->Id == SPELL_DISCOVERY && !spell->IsTriggered() &&
            player->HasAura(SPELL_ARCHAEOLOGY, player->GetGUID()))
        {
            player->EnergizeBySpell(player, SPELL_ARCHAEOLOGY,
                int32(CalculatePct(player->GetCreateMana(), ARCHAEOLOGY_PCT)), POWER_MANA);
            HealInfo heal(player, player, CalculatePct(player->GetCreateHealth(), ARCHAEOLOGY_PCT),
                info, info->GetSchoolMask());
            player->HealBySpell(heal);
        }
    }

    void OnSpellHitResult(Spell* spell, Unit* target, uint8 missInfo,
        uint32 damage, uint32, bool) override
    {
        Player* player = spell->GetCaster() ? spell->GetCaster()->ToPlayer() : nullptr;
        if (!player || !damage || missInfo != SPELL_MISS_NONE ||
            player->getClass() != CLASS_CHRONOMANCER ||
            !spell->GetSpellInfo()->HasAura(SPELL_AURA_PERIODIC_DAMAGE) || !player->HasAura(SPELL_CHAOTIC_TIME))
            return;
        // Chaotic Time (583245): "periodic damage dealt now reduces the
        // cooldown of Incarnation of Chaos by 1 sec." The DBC's Proc Trigger
        // slot is inert; every periodic tick pays out here.
        if (uint32 cooldown = player->GetSpellCooldownDelay(SPELL_INCARNATION_OF_CHAOS))
            player->ModifySpellCooldown(SPELL_INCARNATION_OF_CHAOS,
                -std::min<uint32>(cooldown, CHAOTIC_TIME_REDUCTION));
        // Anomaly Spikes (503825): "Periodic damage dealt now has a 8% chance
        // to launch an Anomaly Spike at your target." The spike (503826)
        // carries the damage natively; the proc roll lives here.
        if (player->HasAura(SPELL_ANOMALY_SPIKES) && roll_chance_i(ANOMALY_CHANCE))
            spell->GetCaster()->CastSpell(target, SPELL_ANOMALY_SPIKE_HIT, true);
    }

private:
    static bool IsArtificerCast(uint32 id)
    {
        uint32 const root = sSpellMgr->GetFirstSpellInChain(id);
        return root == 804478 || root == 806204;
    }

    static constexpr uint32 SPELL_RESONANCE = 706079;
    static constexpr uint32 SPELL_HASTEN = 801304;
    static constexpr uint32 RESONANCE_CHANCE = 35;
    static constexpr uint32 RESONANCE_REDUCTION = 1000;
    static constexpr uint32 SPELL_INCARNATION_OF_CHAOS = 570067;
    static constexpr uint32 SPELL_CHROMATIC_SHARD = 801292;
    static constexpr uint32 SPELL_CHAOTIC_TIME = 583245;
    static constexpr uint32 CHAOTIC_TIME_REDUCTION = 1000;
    static constexpr uint32 SPELL_ANOMALY_SPIKES = 503825;
    static constexpr uint32 SPELL_ANOMALY_SPIKE_HIT = 503826;
    static constexpr uint32 ANOMALY_CHANCE = 8;
    static constexpr uint32 ARCHAEOLOGY_PCT = 3;
};
}

void ApplyAscensionChronomancerTalentContracts(SpellInfo* info)
{
    if (info->SpellFamilyName != 28)
        return;
    if (info->Id == SPELL_DIMENSIONAL_DIVERGENCE)
    {
        // The copied client-destination and caster-front teleports do not swap
        // players. The effect-0 script uses authoritative live server positions.
        info->Effects[EFFECT_1].Effect = 0;
        info->Effects[EFFECT_2].Effect = 0;
        info->_InitializeExplicitTargetMask();
    }
    if (info->Id == SPELL_DISTORTED_TIME)
    {
        // "Reduces the cooldown of Timerend by 2 sec and increases the
        // duration of Decomposition by 3 sec." The talent ships without
        // SPELL_ATTR0_PASSIVE, so the learn/login passes never applied its
        // spellmod auras. Mark passive and rebind the two slots to the real
        // targets: the cooldown slot (effect 0, empty mask) to the Timerend
        // chain root (707430; the castable ranks 801291/501831-35/572578
        // carry flags[1] 0x1000000), and the duration slot (effect 1) to the
        // Decomposition chain root (800856; ranks carry flags[1] 0x80000).
        // BasePoints are display-minus-1 with DieSides 0, so shift DieSides
        // to 1: cooldown 1999+1=2000 ms, duration 2999+1=3000 ms.
        info->Attributes |= SPELL_ATTR0_PASSIVE;
        info->Effects[EFFECT_0].MiscValue = SPELLMOD_COOLDOWN;
        info->Effects[EFFECT_0].BasePoints = 1999;
        info->Effects[EFFECT_0].DieSides = 1;
        info->Effects[EFFECT_0].SpellClassMask = flag96(0, 0x1000000, 0);
        info->Effects[EFFECT_1].MiscValue = SPELLMOD_DURATION;
        info->Effects[EFFECT_1].BasePoints = 2999;
        info->Effects[EFFECT_1].DieSides = 1;
        info->Effects[EFFECT_1].SpellClassMask = flag96(0, 0x80000, 0);
    }
    if (info->Id == SPELL_RIPPLING_POWER || info->Id == SPELL_RIPPLING_POWER_RANK_2)
    {
        // "Increases your Spirit by 8%/15%." Both ranks ship without
        // SPELL_ATTR0_PASSIVE, so the talent learn and the login-load pass
        // (Player::addSpell casts only IsPassive spellbooks) never applied the
        // auras. Mark passive and shift DieSides to 1 so BasePoints resolve as
        // the displayed values: Intellect -1+1=0 (a no-op, not a -1% penalty)
        // and Spirit 7+1=8 / 14+1=15.
        info->Attributes |= SPELL_ATTR0_PASSIVE;
        info->Effects[EFFECT_0].DieSides = 1;
        info->Effects[EFFECT_1].DieSides = 1;
    }
    if (info->Id != SPELL_SHIMMER)
        return;
    // The talent promises the same percentage for damage and healing. Retain
    // the native stack cap and duration, and match healing to the displayed amount.
    info->Effects[EFFECT_1].BasePoints = info->Effects[EFFECT_0].BasePoints;
    info->Effects[EFFECT_1].DieSides = info->Effects[EFFECT_0].DieSides;
}

void AddSC_AscensionChronomancerTalents()
{
    new chronomancer_talent_casts();
    RegisterSpellScript(spell_ascension_dimensional_divergence);
}
