/* Copyright (C) 2016+ AzerothCore, GNU AGPL v3. */
#include "Player.h"
#include "ScriptMgr.h"
#include "SpellInfo.h"

namespace
{
constexpr uint32 SPELL_PLAGUE_DOCTOR = 803087;
constexpr uint32 SPELL_EXPUNGING_LIFE = 803088;

class necromancer_talent_events : public UnitScript
{
public:
    necromancer_talent_events() : UnitScript("necromancer_talent_events", true,
        {UNITHOOK_ON_PERIODIC_DAMAGE_RESULT}) { }

    void OnPeriodicDamageResult(Unit*, Unit* attacker, uint32 damage,
        SpellInfo const*) override
    {
        Player* player = attacker && attacker->IsPlayer() ? attacker->ToPlayer() : nullptr;
        if (!player || player->getClass() != CLASS_NECROMANCER || !damage ||
            !player->HasAura(SPELL_PLAGUE_DOCTOR))
            return;

        player->CastCustomSpell(SPELL_EXPUNGING_LIFE, SPELLVALUE_BASE_POINT0,
            int32(damage * 25 / 100), player, true);
    }
};
}

void AddSC_AscensionNecromancerTalents()
{
    new necromancer_talent_events();
}
