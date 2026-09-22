/* Copyright (C) 2016+ AzerothCore, GNU AGPL v3. */

#include "AscensionStormbringerRunemasterTalentProcs.h"
#include "ScriptMgr.h"
#include "SpellAuras.h"
#include "SpellInfo.h"
#include "SpellScript.h"
#include <algorithm>

namespace
{
using AscensionStormbringerRunemasterTalentProcs::Rules;

class spell_ascension_stormbringer_runemaster_talent_proc : public AuraScript
{
    PrepareAuraScript(spell_ascension_stormbringer_runemaster_talent_proc);

    bool CheckProc(ProcEventInfo& eventInfo)
    {
        auto rule = std::find_if(Rules.begin(), Rules.end(),
            [this](AscensionStormbringerRunemasterTalentProcs::Rule const& entry)
            {
                return entry.Talent == GetId();
            });
        if (rule == Rules.end())
            return false;

        Unit* caster = GetTarget();
        if (!caster)
            return false;

        if (rule->RequiredAura && !caster->HasAura(rule->RequiredAura))
            return false;

        if (rule->RequiresStealth && !caster->HasStealthAura())
            return false;

        if (!rule->Spells[0])
            return true;

        SpellInfo const* spellInfo = eventInfo.GetSpellInfo();
        if (!spellInfo)
            return false;

        return std::find(rule->Spells.begin(), rule->Spells.end(), spellInfo->Id) !=
            rule->Spells.end();
    }

    void Register() override
    {
        DoCheckProc += AuraCheckProcFn(spell_ascension_stormbringer_runemaster_talent_proc::CheckProc);
    }
};

constexpr std::uint32_t SPELL_DROWN = 572760;
constexpr std::uint32_t SPELL_DROWN_SECOND_RANK = 572761;

class aura_stormbringer_electrified_waters : public AuraScript
{
    PrepareAuraScript(aura_stormbringer_electrified_waters);

    bool CheckProc(ProcEventInfo& eventInfo)
    {
        Unit* target = eventInfo.GetActionTarget();
        return target && (target->HasAura(SPELL_DROWN) || target->HasAura(SPELL_DROWN_SECOND_RANK));
    }

    void Register() override
    {
        DoCheckProc += AuraCheckProcFn(aura_stormbringer_electrified_waters::CheckProc);
    }
};
}

void AddSC_AscensionStormbringerRunemasterTalentProcs()
{
    RegisterSpellScript(spell_ascension_stormbringer_runemaster_talent_proc);
    RegisterSpellScript(aura_stormbringer_electrified_waters);
}
