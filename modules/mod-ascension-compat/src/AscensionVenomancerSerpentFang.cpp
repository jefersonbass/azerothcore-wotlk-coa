/* Copyright (C) 2016+ AzerothCore, GNU AGPL v3. */
#include "Player.h"
#include "ScriptMgr.h"
#include "Spell.h"
#include "SpellMgr.h"
#include "AscensionVenomancer.h"

namespace
{
enum SerpentFangSpells : uint32
{
    SPELL_SERPENTS_FANG = 800946,
    SPELL_SERPENTS_FANG_DAMAGE = 503931,
    SPELL_SERPENTS_FANG_HEAL = 560202,
    SPELL_VENOM_FANATIC = 681059
};

class venomancer_serpent_fang : public AllSpellScript
{
public:
    venomancer_serpent_fang() : AllSpellScript("venomancer_serpent_fang", {ALLSPELLHOOK_ON_HIT_RESULT}) { }

    void OnSpellHitResult(Spell* spell, Unit* target, uint8 miss, uint32 damage, uint32, bool) override
    {
        Player* player = spell->GetCaster()->ToPlayer();
        if (!player || player->getClass() != CLASS_PROPHET || !player->IsAlive() || !player->IsInWorld() ||
            spell->GetSpellInfo()->SpellFamilyName != 35 ||
            sSpellMgr->GetFirstSpellInChain(spell->GetSpellInfo()->Id) != SPELL_SERPENTS_FANG ||
            !target || !target->IsInWorld() || target == player || player->IsFriendlyTo(target) ||
            player->GetMap() != target->GetMap() || !player->InSamePhase(target) || miss != SPELL_MISS_NONE ||
            spell->GetScriptValue(SPELL_SERPENTS_FANG_HEAL))
            return;
        SpellInfo const* heal = sSpellMgr->GetSpellInfo(SPELL_SERPENTS_FANG_HEAL);
        if (!heal)
            return;
        spell->SetScriptValue(SPELL_SERPENTS_FANG_HEAL, 1);
        // Use the struck enemy's position, including killing blows. Native area selection applies
        // the five-ally cap and Lifemender's additional targets, with the existing healing coefficient.
        SpellCastTargets targets;
        targets.SetDst(target->GetPosition());
        player->CastSpell(targets, heal, nullptr, TRIGGERED_FULL_MASK);
        // Venom Fanatic: Serpent's Fang also strikes 2 additional nearby enemies.
        // Only fan from untriggered casts so the copied strikes (triggered) do not recurse.
        if (player->HasAura(SPELL_VENOM_FANATIC) && !spell->IsTriggered() && damage)
        {
            uint32 remaining = 2;
            for (Unit* enemy : Nearby(target, 10.0f))
            {
                if (!remaining)
                    break;
                if (enemy == target || !player->IsValidAttackTarget(enemy) || !target->IsWithinLOSInMap(enemy))
                    continue;
                Copy(player, enemy, Highest(player, SPELL_SERPENTS_FANG_DAMAGE), damage);
                --remaining;
            }
        }
    }
};

class venomancer_serpent_fang_metadata : public GlobalScript
{
public:
    venomancer_serpent_fang_metadata() : GlobalScript("venomancer_serpent_fang_metadata",
        {GLOBALHOOK_ON_LOAD_SPELL_CUSTOM_ATTR}) { }

    void OnLoadSpellCustomAttr(SpellInfo* info) override
    {
        if (info->Id == SPELL_SERPENTS_FANG_HEAL && info->SpellFamilyName == 35)
        {
            // A destination keeps the healing valid after the hostile unit dies from the damage hit.
            info->Effects[EFFECT_0].TargetA = SpellImplicitTargetInfo(TARGET_DEST_DEST);
            info->_InitializeExplicitTargetMask();
        }
    }
};
}

void AddSC_AscensionVenomancerSerpentFang()
{
    new venomancer_serpent_fang();
    new venomancer_serpent_fang_metadata();
}
