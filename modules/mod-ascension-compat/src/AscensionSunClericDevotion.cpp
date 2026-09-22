/* Copyright (C) 2016+ AzerothCore, GNU AGPL v3. */
#include "AscensionSunCleric.h"
#include "AscensionSunClericData.h"
#include "AscensionSunClericDevotion.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SpellInfo.h"
#include <algorithm>
#include <array>
namespace
{
using namespace AscensionSunCleric;
constexpr uint32 SUN_CLERIC_GRACE = 504070;

constexpr std::array<uint32, 8> DAWNSEAR_SPELLS = {
    500146, 502366, 502367, 502368, 502369, 502370, 502371, 554406};
class sun_cleric_devotion_scaling : public UnitScript
{
public:
    sun_cleric_devotion_scaling() : UnitScript("sun_cleric_devotion_scaling", true,
        {UNITHOOK_MODIFY_SPELL_EFFECT_BASE_VALUE}) { }
    void ModifySpellEffectBaseValue(Unit const* caster, SpellInfo const* info, uint8 index, float& value) override
    {
        Player* player = Owner(caster);
        if (!player || !info || info->SpellFamilyName != 33 || Derived(info) || !player->HasAura(SUN_CLERIC_GRACE))
            return;
        bool const dawnsear = std::find(DAWNSEAR_SPELLS.begin(), DAWNSEAR_SPELLS.end(), info->Id) !=
            DAWNSEAR_SPELLS.end();
        bool const sunflare = !dawnsear && info->SpellFamilyFlags.HasFlag(0, 0x100, 0);
        if (!dawnsear && !sunflare)
            return;
        for (auto const& row : SunClericCoefficients)
            if (row.spell == info->Id && row.effect == index && row.sp != 0.0f)
            {
                float const bonusPercent = Amount(SUN_CLERIC_GRACE, dawnsear ? 0 : 1) / 100.0f;
                value += row.sp * std::max(0, player->SpellBaseDamageBonusDone(info->GetSchoolMask())) * bonusPercent;
            }
    }
};
}
void AddSC_AscensionSunClericDevotion()
{
    new sun_cleric_devotion_scaling();
}
