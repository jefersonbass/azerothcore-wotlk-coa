/* Copyright (C) 2016+ AzerothCore, GNU AGPL v3. */

#include "CellImpl.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "Item.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "Spell.h"
#include "SpellScript.h"
#include <algorithm>
#include <limits>

namespace
{
enum RangerAssaultSpells : uint32
{
    SPELL_WILD_MAN = 560344
};

constexpr uint32 WILD_STRIKE_FAMILY_FLAG = 256;
constexpr uint32 WILD_MAN_EXTRA_TARGETS = 2;
constexpr float WILD_MAN_CLEAVE_RANGE = 10.0f;
constexpr float WILD_MAN_FALLOFF_STEP = 0.10f;
constexpr float WILD_MAN_FALLOFF_FLOOR = 0.20f;

std::list<Unit*> Nearby(Unit* center, float range)
{
    std::list<Unit*> units;
    if (!center || !center->IsInWorld())
        return units;
    Acore::AnyUnitInObjectRangeCheck check(center, range);
    Acore::UnitListSearcher<Acore::AnyUnitInObjectRangeCheck> searcher(center, units, check);
    Cell::VisitObjects(center, searcher, range);
    units.remove_if([center](Unit* unit) { return !unit->IsAlive() || !center->InSamePhase(unit); });
    units.sort([center](Unit* left, Unit* right)
    {
        return center->GetExactDistSq(left) < center->GetExactDistSq(right);
    });
    return units;
}

Player* WildManRanger(Spell* spell)
{
    Player* player = spell->GetCaster() ? spell->GetCaster()->ToPlayer() : nullptr;
    SpellInfo const* info = spell->GetSpellInfo();
    if (!player || player->getClass() != CLASS_RANGER || !player->IsAlive() || !player->IsInWorld() ||
        info->SpellFamilyName != 27 || !(info->SpellFamilyFlags[1] & WILD_STRIKE_FAMILY_FLAG) ||
        spell->IsTriggered() || !player->HasAura(SPELL_WILD_MAN))
        return nullptr;
    return player;
}

class spell_ascension_ranger_assault : public SpellScript
{
    PrepareSpellScript(spell_ascension_ranger_assault);

    bool _scaled = false;

    bool Validate(SpellInfo const* spellInfo) override
    {
        return spellInfo && (spellInfo->Id == 803108 || (spellInfo->Id >= 503099 && spellInfo->Id <= 503105)) &&
            spellInfo->SpellFamilyName == uint32(CLASS_RANGER) + 6 &&
            spellInfo->SpellFamilyFlags == flag96(0, 32768, 0) && spellInfo->DmgClass == SPELL_DAMAGE_CLASS_MELEE &&
            spellInfo->Effects[EFFECT_0].Effect == SPELL_EFFECT_SCHOOL_DAMAGE &&
            spellInfo->Effects[EFFECT_2].Effect == SPELL_EFFECT_WEAPON_PERCENT_DAMAGE &&
            spellInfo->Effects[EFFECT_2].BasePoints == 99 && spellInfo->Effects[EFFECT_2].DieSides == 1;
    }

    bool Load() override
    {
        Unit* caster = GetCaster();
        return caster && caster->IsPlayer() && caster->ToPlayer()->getClass() == CLASS_RANGER;
    }

    void ScaleWeaponDamage(SpellEffIndex)
    {
        if (_scaled)
            return;
        _scaled = true;

        Player* caster = GetCaster() ? GetCaster()->ToPlayer() : nullptr;
        Item* weapon = caster ? caster->GetWeaponForAttack(BASE_ATTACK, true) : nullptr;
        if (!weapon || weapon->GetTemplate()->SubClass != ITEM_SUBCLASS_WEAPON_DAGGER)
            return;

        int64 const percent = (int64(GetSpellValue()->EffectBasePoints[EFFECT_2]) + 1) * 7 / 4;
        GetSpell()->SetSpellValue(SPELLVALUE_BASE_POINT2, int32(std::clamp<int64>(percent, 0, std::numeric_limits<int32>::max())));
    }

    void Register() override
    {
        OnEffectLaunch += SpellEffectFn(spell_ascension_ranger_assault::ScaleWeaponDamage, EFFECT_2, SPELL_EFFECT_WEAPON_PERCENT_DAMAGE);
    }
};

class ranger_wild_man_cleave : public AllSpellScript
{
public:
    ranger_wild_man_cleave() : AllSpellScript("ranger_wild_man_cleave",
        {ALLSPELLHOOK_ON_BEFORE_EFFECTS, ALLSPELLHOOK_ON_CALCULATED_TARGET}) { }

    void OnSpellBeforeEffects(Spell* spell, Unit*, SpellInfo const*) override
    {
        Player* player = WildManRanger(spell);
        Unit* target = spell->m_targets.GetUnitTarget();
        if (!player || !target || !player->IsValidAttackTarget(target))
            return;
        uint32 added = 0;
        for (Unit* enemy : Nearby(target, WILD_MAN_CLEAVE_RANGE))
        {
            if (added >= WILD_MAN_EXTRA_TARGETS)
                break;
            if (enemy == target || !player->IsValidAttackTarget(enemy))
                continue;
            spell->AddUnitTargetForScript(enemy, 1);
            ++added;
        }
    }

    void OnSpellCalculatedTarget(Spell* spell, Unit* target, TargetInfo& hit) override
    {
        if (!WildManRanger(spell) || !target || hit.damage <= 0)
            return;
        uint32 index = 0;
        for (auto const& row : *spell->GetUniqueTargetInfo())
        {
            if (row.targetGUID == target->GetGUID())
                break;
            if (row.effectMask & 1)
                ++index;
        }
        if (!index)
            return;
        float factor = std::max(WILD_MAN_FALLOFF_FLOOR, 1.0f - WILD_MAN_FALLOFF_STEP * index);
        hit.damage = int32(hit.damage * factor);
        hit.damageBeforeTakenMods = int32(hit.damageBeforeTakenMods * factor);
    }
};
}

void AddAscensionRangerAssaultScripts()
{
    new ranger_wild_man_cleave();
    RegisterSpellScript(spell_ascension_ranger_assault);
}
