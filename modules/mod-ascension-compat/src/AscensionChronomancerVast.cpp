/* Copyright (C) 2016+ AzerothCore, GNU AGPL v3. */
#include "Player.h"
#include "ScriptMgr.h"
#include "SpellAuraEffects.h"
#include "SpellAuras.h"
#include "SpellMgr.h"
#include "SpellScript.h"
#include "Group.h"
#include <algorithm>
#include <limits>

namespace
{
enum VastSpells : uint32
{
    VastInfinite = 706083
};

// Tooltip: "All party and raid members become one, equally sharing 25% of all
// damage dealt to them for 10 sec, up to a maximum of 100% of their total
// health. At the end of the duration, all affected allies are healed equal to
// the total damage shared split evenly amongst all allies."
//
// The DBC casts the aura raid-wide (target 56, 100 yd), but its only mechanical
// effect is a plain school absorb, which does not model sharing. The dummy
// effect banks each member's contributed share; the share is also deducted
// from incoming damage, so the group survives as one. At aura end the bank is
// paid out evenly.

uint32 MemberShare(Unit* victim)
{
    if (Aura* aura = victim->GetAura(VastInfinite))
        if (AuraEffect* bank = aura->GetEffect(EFFECT_1))
            return uint32(std::max(0, bank->GetAmount()));
    return 0;
}

void AddShare(Unit* victim, uint32 share)
{
    if (Aura* aura = victim->GetAura(VastInfinite))
        if (AuraEffect* bank = aura->GetEffect(EFFECT_1))
            bank->SetAmount(int32(std::min<uint64>(uint64(std::max(0, bank->GetAmount())) + share,
                std::numeric_limits<int32>::max())));
}

class vast_infinite_share : public UnitScript
{
public:
    vast_infinite_share() : UnitScript("vast_infinite_share", true,
        {UNITHOOK_MODIFY_PERIODIC_DAMAGE_AURAS_TICK, UNITHOOK_MODIFY_SPELL_DAMAGE_TAKEN, UNITHOOK_MODIFY_MELEE_DAMAGE}) { }

    void Share(Unit* victim, uint32& damage)
    {
        Player* player = victim ? victim->ToPlayer() : nullptr;
        if (!player || !player->IsAlive() || !player->HasAura(VastInfinite) || !damage)
            return;
        // Each hit contributes 25%, capped by the member's remaining lifetime
        // budget of 100% of their max health.
        uint32 lifetime = player->GetMaxHealth();
        uint32 room = lifetime > MemberShare(player) ? lifetime - MemberShare(player) : 0;
        uint32 share = uint32(std::min<uint64>(uint64(damage) * 25 / 100, room));
        if (!share)
            return;
        AddShare(player, share);
        damage -= share;
    }

    void ModifyPeriodicDamageAurasTick(Unit* target, Unit*, uint32& damage, SpellInfo const*) override
    {
        Share(target, damage);
    }

    void ModifySpellDamageTaken(Unit* target, Unit*, int32& damage, SpellInfo const*) override
    {
        if (damage > 0)
        {
            uint32 share = uint32(damage);
            Share(target, share);
            damage = int32(share);
        }
    }

    void ModifyMeleeDamage(Unit*, Unit* victim, uint32& damage) override
    {
        Share(victim, damage);
    }
};

class aura_vast_infinite : public AuraScript
{
    PrepareAuraScript(aura_vast_infinite);

    void SavedAmount(AuraEffect const*, int32&, bool& recalculate) { recalculate = false; }

    void End(AuraEffect const*, AuraEffectHandleModes)
    {
        AuraRemoveMode mode = GetTargetApplication()->GetRemoveMode();
        if (mode != AURA_REMOVE_BY_EXPIRE && mode != AURA_REMOVE_BY_CANCEL)
            return;
        Unit* member = GetTarget();
        Player* starter = member->ToPlayer();
        if (!starter)
            return;
        // Each member banks its own contributed share; one payout per member,
        // split evenly among every ally still carrying the aura.
        std::vector<Player*> affected;
        if (Group* group = starter->GetGroup())
            for (GroupReference* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
                if (Player* ally = itr->GetSource())
                    if (ally->IsInMap(starter) && ally->HasAura(VastInfinite))
                        affected.push_back(ally);
        uint32 total = MemberShare(starter);
        if (!total)
            return;
        uint32 slice = uint32(uint64(total) / std::max<size_t>(affected.size(), 1));
        for (Player* ally : affected)
            Pay(ally, slice);
    }

    void Pay(Unit* ally, uint32 amount)
    {
        if (!ally->IsAlive() || !amount)
            return;
        SpellInfo const* info = sSpellMgr->GetSpellInfo(VastInfinite);
        if (!info)
            return;
        // This payout is not a new heal spell: no procs, no mitigation.
        HealInfo healInfo(ally, ally, amount, info, SPELL_SCHOOL_MASK_ALL);
        ally->HealBySpell(healInfo, false);
    }

    void Register() override
    {
        DoEffectCalcAmount += AuraEffectCalcAmountFn(aura_vast_infinite::SavedAmount,
            EFFECT_ALL, SPELL_AURA_ANY);
        AfterEffectRemove += AuraEffectRemoveFn(aura_vast_infinite::End,
            EFFECT_1, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
    }
};

class vast_infinite_metadata : public GlobalScript
{
public:
    vast_infinite_metadata() : GlobalScript("vast_infinite_metadata",
        {GLOBALHOOK_ON_LOAD_SPELL_CUSTOM_ATTR}) { }

    void OnLoadSpellCustomAttr(SpellInfo* info) override
    {
        if (info->Id != VastInfinite)
            return;
        // The native school absorb would apply on top of the scripted share,
        // doubling the 25% transfer; the bank lives on the dummy effect.
        SpellEffectInfo& shield = info->Effects[EFFECT_0];
        shield.Effect = SPELL_EFFECT_APPLY_AURA;
        shield.ApplyAuraName = SPELL_AURA_DUMMY;
        shield.BasePoints = 0;
        shield.DieSides = 0;
        shield.MiscValue = 0;
        shield.Amplitude = 0;
        shield.TriggerSpell = 0;
    }
};
}

void AddSC_AscensionChronomancerVast()
{
    new vast_infinite_share();
    new vast_infinite_metadata();
    RegisterSpellScript(aura_vast_infinite);
}
