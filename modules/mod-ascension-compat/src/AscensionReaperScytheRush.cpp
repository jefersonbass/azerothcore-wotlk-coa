/* Copyright (C) 2016+ AzerothCore, GNU AGPL v3. */

#include "AscensionReaperScytheRush.h"
#include "SpellInfo.h"

namespace
{
constexpr uint32 REAPER_SPELL_FAMILY = 36;
constexpr uint32 SPELL_REAPER_SCYTHE_RUSH = 500359;
constexpr uint32 SPELL_REAPER_DARK_SOLDIER = 560490;
constexpr uint32 SPELL_REAPER_DARK_SOLDIER_RANK_2 = 561338;
constexpr uint32 SPELL_SOULFORGED_WEAPONRY = 561127;
constexpr uint32 SPELL_SOULFORGED_WEAPONRY_RANK_2 = 561340;

// Scythe Rush's public record is a bare charge with Speed 0, so Spell::EffectCharge falls back
// to SPEED_CHARGE, the same 42 yards/second warrior Charge uses. The generic correction in
// SpellInfoCorrections only fills that field for family 0, so this Reaper record is skipped.
// The movement belonged to server-side helper 805689 "Scythe Rush (Charge/Root/Trigger)", whose
// own Spell.dbc Speed is 200; take that value rather than a tuned estimate.
constexpr float REAPER_SCYTHE_RUSH_SPEED = 200.0f;
}

void ApplyAscensionReaperDarkSoldierContracts(SpellInfo* spellInfo)
{
    if (!spellInfo || spellInfo->SpellFamilyName != REAPER_SPELL_FAMILY ||
        (spellInfo->Id != SPELL_REAPER_DARK_SOLDIER && spellInfo->Id != SPELL_REAPER_DARK_SOLDIER_RANK_2))
        return;

    // Issue 990: Dark Soldier ships without the passive flag, so the
    // learn/login passes never applied its auras (87 = -3%/-6% Magic damage
    // taken, 333 = +3%/+6% hit chance, both resolving through the native
    // taken-mod and hit-chance paths).
    spellInfo->Attributes |= SPELL_ATTR0_PASSIVE;
}

void ApplyAscensionReaperSoulforgedContracts(SpellInfo* spellInfo)
{
    if (!spellInfo || spellInfo->SpellFamilyName != REAPER_SPELL_FAMILY ||
        (spellInfo->Id != SPELL_SOULFORGED_WEAPONRY && spellInfo->Id != SPELL_SOULFORGED_WEAPONRY_RANK_2))
        return;

    // Issue 1024: Soulforged Weaponry ships without the passive flag, so the
    // learn/login passes never applied its off-hand aura (122, resolving the
    // tooltip's +25%/+50% off-hand damage through the native offhand path).
    // The free-Murder proc is scripted in the melee hook; the DBC's proc
    // names no trigger spell.
    spellInfo->Attributes |= SPELL_ATTR0_PASSIVE;
}

void ApplyAscensionReaperScytheRushContracts(SpellInfo* spellInfo)
{
    if (!spellInfo || spellInfo->Id != SPELL_REAPER_SCYTHE_RUSH ||
        spellInfo->SpellFamilyName != REAPER_SPELL_FAMILY ||
        spellInfo->SpellFamilyFlags != flag96(0, 512, 0))
        return;

    SpellEffectInfo const& charge = spellInfo->Effects[EFFECT_0];
    if (charge.Effect != SPELL_EFFECT_CHARGE || charge.TargetA.GetTarget() != TARGET_UNIT_TARGET_ENEMY ||
        spellInfo->Effects[EFFECT_1].Effect || spellInfo->Effects[EFFECT_2].Effect)
        return;

    // Keep the correction idempotent: a Spell.dbc that already carries a speed is left alone.
    if (!spellInfo->Speed)
        spellInfo->Speed = REAPER_SCYTHE_RUSH_SPEED;
}
