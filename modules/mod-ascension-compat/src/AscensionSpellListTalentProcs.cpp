/* Copyright (C) 2016+ AzerothCore, GNU AGPL v3. */

#include "AscensionSpellListTalentProcs.h"
#include "ScriptMgr.h"
#include "SpellAuras.h"
#include "SpellInfo.h"
#include "SpellScript.h"
#include <algorithm>

namespace
{
using AscensionSpellListTalentProcs::Rules;

class spell_ascension_spell_list_talent_proc : public AuraScript
{
    PrepareAuraScript(spell_ascension_spell_list_talent_proc);

    bool CheckProc(ProcEventInfo& eventInfo)
    {
        auto rule = std::find_if(Rules.begin(), Rules.end(),
            [this](AscensionSpellListTalentProcs::Rule const& entry)
            {
                return entry.Talent == GetId();
            });
        if (rule == Rules.end())
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
        DoCheckProc += AuraCheckProcFn(spell_ascension_spell_list_talent_proc::CheckProc);
    }
};
}

void AddSC_AscensionSpellListTalentProcs()
{
    RegisterSpellScript(spell_ascension_spell_list_talent_proc);
}
