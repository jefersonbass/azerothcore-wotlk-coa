/* Copyright (C) 2016+ AzerothCore, GNU AGPL v3. */

#include "AscensionKnightOfXorothTalentProcs.h"
#include "ScriptMgr.h"
#include "SpellAuras.h"
#include "SpellInfo.h"
#include "SpellScript.h"
#include <algorithm>

namespace
{
using AscensionKnightOfXorothTalentProcs::Rules;

class spell_ascension_knight_of_xoroth_talent_proc : public AuraScript
{
    PrepareAuraScript(spell_ascension_knight_of_xoroth_talent_proc);

    bool CheckProc(ProcEventInfo& eventInfo)
    {
        auto rule = std::find_if(Rules.begin(), Rules.end(),
            [this](AscensionKnightOfXorothTalentProcs::Rule const& entry)
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
        DoCheckProc += AuraCheckProcFn(spell_ascension_knight_of_xoroth_talent_proc::CheckProc);
    }
};
}

void AddSC_AscensionKnightOfXorothTalentProcs()
{
    RegisterSpellScript(spell_ascension_knight_of_xoroth_talent_proc);
}
