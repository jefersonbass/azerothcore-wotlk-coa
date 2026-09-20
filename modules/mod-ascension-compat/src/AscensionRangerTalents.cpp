/* Copyright (C) 2016+ AzerothCore, GNU AGPL v3. */
#include "AscensionRangerTalents.h"
#include "Player.h"
#include "Spell.h"
#include "SpellAuraEffects.h"
#include "SpellAuras.h"
#include "SpellMgr.h"
#include "SpellScript.h"
#include "SpellScriptLoader.h"
#include <algorithm>
#include <limits>

namespace
{
enum RangerTalentSpells : uint32
{
    SPELL_LIGHT_ARROWS = 681292,
    SPELL_KNOCKOUT_INCAPACITATE = 706762,
    SPELL_STONEMASONS_SECRET = 524654,
    SPELL_DIRTY_BLADES = 680276,
    SPELL_ADVANTAGE = 804329,
    SPELL_EXTEND_DIRTY_BLADES = 524653,
    SPELL_SNATCH = 803115,
    SPELL_SNATCH_DISARM = 803123,
    SPELL_TACTICAL_ADVANTAGE = 706748,
    SPELL_TACTICAL_ADVANTAGE_DEBUFF = 706749,
    SPELL_BUSHWHACK = 557333,
    SPELL_PIERCED = 705033
};

class spell_ascension_ranger_light_arrows : public SpellScript
{
    PrepareSpellScript(spell_ascension_ranger_light_arrows);
    int32 _bonus = 0;

    bool Load() override { return GetCaster()->IsPlayer() && GetCaster()->getClass() == CLASS_RANGER; }

    void Snapshot()
    {
        Unit* target = GetExplTargetUnit();
        if (target && GetCaster()->GetExactDist(target) >= 40.0f)
            if (AuraEffect const* talent = GetCaster()->GetAuraEffect(SPELL_LIGHT_ARROWS, EFFECT_0))
                _bonus = std::max(0, talent->GetAmount());
    }

    void Damage()
    {
        // All native base/AP terms have been calculated; retain subsequent crit,
        // armor, absorption and resistance. Distance was captured at cast launch.
        if (_bonus && GetHitDamage() > 0)
            SetHitDamage(int32(std::min<int64>(int64(GetHitDamage()) * (int64(100) + _bonus) / 100,
                std::numeric_limits<int32>::max())));
    }

    void Register() override
    {
        BeforeCast += SpellCastFn(spell_ascension_ranger_light_arrows::Snapshot);
        OnHit += SpellHitFn(spell_ascension_ranger_light_arrows::Damage);
    }
};

class spell_ascension_ranger_knockout : public SpellScript
{
    PrepareSpellScript(spell_ascension_ranger_knockout);

    bool Validate(SpellInfo const*) override { return ValidateSpellInfo({SPELL_KNOCKOUT_INCAPACITATE}); }
    bool Load() override { return GetCaster()->IsPlayer() && GetCaster()->getClass() == CLASS_RANGER; }

    void Incapacitate(SpellEffIndex)
    {
        if (Unit* target = GetHitUnit())
            GetCaster()->CastSpell(target, SPELL_KNOCKOUT_INCAPACITATE, true);
    }

    void Register() override
    {
        // Effect 0 has already made the victim execute the native bleed/poison cleanser.
        OnEffectHitTarget += SpellEffectFn(spell_ascension_ranger_knockout::Incapacitate, EFFECT_1, SPELL_EFFECT_DUMMY);
    }
};
}

void HandleAscensionRangerStonemason(Spell* spell, Player* player)
{
    SpellInfo const* info = spell->GetSpellInfo();
    if (player->getClass() != CLASS_RANGER || info->SpellFamilyName != 27 ||
        !(info->SpellFamilyFlags[1] & (32768 | 134217728)) || !player->HasAura(SPELL_STONEMASONS_SECRET) ||
        !player->HasAura(SPELL_DIRTY_BLADES, player->GetGUID()))
        return;
    if (Aura const* advantage = player->GetAura(SPELL_ADVANTAGE); advantage && advantage->GetStackAmount() == 5)
        player->CastSpell(player, SPELL_EXTEND_DIRTY_BLADES, true);

    // 'Tactical' Advantage marks the victim of a Knockout or Bushwhack.
    if (player->HasAura(SPELL_TACTICAL_ADVANTAGE) &&
        (info->Id == SPELL_KNOCKOUT_INCAPACITATE || info->Id == SPELL_BUSHWHACK))
        player->CastSpell(spell->m_targets.GetUnitTarget(), SPELL_TACTICAL_ADVANTAGE_DEBUFF, true);
}

void ApplyAscensionRangerTalentContracts(SpellInfo* info)
{
    if (info->Id == SPELL_KNOCKOUT_INCAPACITATE && info->SpellFamilyName == 27)
        info->AuraInterruptFlags |= AURA_INTERRUPT_FLAG_TAKE_DAMAGE;
    if (info->Id == SPELL_SNATCH_DISARM && info->SpellFamilyName == 27)
        if (SpellInfo const* parent = sSpellMgr->GetSpellInfo(SPELL_SNATCH))
            info->DurationEntry = parent->DurationEntry;
    if (info->Id == SPELL_TACTICAL_ADVANTAGE_DEBUFF && info->SpellFamilyName == 27)
    {
        // The debuff ships with a bogus 300000 s duration and a cast target;
        // the authored mark is an 8-second enemy debuff.
        info->DurationEntry = sSpellDurationStore.LookupEntry(31); // Eight seconds.
        info->Effects[EFFECT_0].TargetA = SpellImplicitTargetInfo(TARGET_UNIT_TARGET_ENEMY);
        info->Effects[EFFECT_0].TargetB = SpellImplicitTargetInfo();
    }
}

class ranger_pierced_crits : public AllSpellScript
{
public:
    ranger_pierced_crits() : AllSpellScript("ranger_pierced_crits", {ALLSPELLHOOK_ON_HIT_RESULT}) { }

    void OnSpellHitResult(Spell* spell, Unit* target, uint8 miss, uint32 damage, uint32, bool critical) override
    {
        Player* player = spell->GetCaster() ? spell->GetCaster()->ToPlayer() : nullptr;
        SpellInfo const* info = spell->GetSpellInfo();
        if (!player || player->getClass() != CLASS_RANGER || !target || miss != SPELL_MISS_NONE ||
            !critical || !damage || info->SpellFamilyName != 27 || !player->HasAura(SPELL_PIERCED))
            return;
        // Pierced (705033): critical strikes with Skullpiercer and Precision
        // Shot bleed the enemy for 15% of the damage dealt plus 35% over 4
        // sec. The DBC's aura has no engine handler, so both halves are paid
        // here where the crit lands; the over-time half reuses the 4-second
        // Venom Blade vehicle with a scaled amount.
        uint32 root = sSpellMgr->GetFirstSpellInChain(info->Id);
        if (root != 501715 && root != 500075)
            return;
        player->CastCustomSpell(target, 803116, SpellValueMod(SPELLVALUE_BASE_POINT0),
            int32(CalculatePct(damage, 15) + CalculatePct(damage, 35) / 4), nullptr, true);
    }
};

void AddSC_AscensionRangerTalents()
{
    new ranger_pierced_crits();
    RegisterSpellScript(spell_ascension_ranger_light_arrows);
    RegisterSpellScript(spell_ascension_ranger_knockout);
}
