/* Copyright (C) 2016+ AzerothCore, GNU AGPL v3. */
#include "AscensionPooledVitality.h"
#include "EventProcessor.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "Random.h"
#include "ScriptMgr.h"
#include "Spell.h"
#include "SpellAuraEffects.h"
#include "SpellAuras.h"
#include "SpellMgr.h"
#include "SpellScript.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <list>
#include <vector>

namespace
{
enum BloodmageSecondarySpells : uint32
{
    SPELL_VEINBURST = 504260,
    SPELL_REAVE = 800490,
    SPELL_REAVE_BLEED = 802883,
    SPELL_REAVE_EXECUTE = 802884,
    SPELL_REAVE_BACK = 805612,
    SPELL_HEMOBURST = 572855,
    SPELL_HEMOTURGY = 572856,
    SPELL_CURSED_FORM = 562720,
    SPELL_ACCURSED_FORM = 562572,
    SPELL_VAMPYRS_KISS = 504275,
    SPELL_VAMPYRS_KISS_COPY = 504785,
    SPELL_BLACK_HEART = 680731,
    SPELL_NIGHT_HUNTER = 704659,
    SPELL_BLOOD_FEAST_RESTORE = 706608,
    SPELL_ATHERANNS_ANGUISH = 680680,
    SPELL_ATHERANNS_ANGUISH_BURST = 680681,
    SPELL_DARK_ESSENCE = 680732,
    SPELL_BLOOD_RITUALS_MARK = 706623,
    SPELL_BLOOD_RITUALS_HEAL = 704119,
    SPELL_INFUSE = 681403,
    SPELL_INFUSE_BURST = 681404,
    SPELL_THIRST_FOR_BLOOD = 570023,
    SPELL_THIRST = 706613,
    SPELL_SATED = 570024,
    SPELL_RAVENOUS = 570025,
    SPELL_BLOOD_PRINCES_COMMAND = 704641,
    SPELL_FANG_OVER_FANG = 504116,
    SPELL_ENTHRALLER = 706619,
    SPELL_AORTIC_AEGIS = 806274,
    SPELL_BLOOD_VEIL = 504263,
    SPELL_SOVEREIGNTY = 806049,
    SPELL_SOVEREIGNTY_BUFF = 504272,
    SPELL_FLESH_FOUNDRY = 704633,
    SPELL_FLESHCRAFT = 801952,
    SPELL_CRIMSON_SCION = 806424,
    SPELL_CRIMSON_SCION_PROC = 806425,
    SPELL_SANGUINE_MEND = 504079,
    SPELL_HEMAL_EXCISION = 803681,
    SPELL_HEMAL_EXCISION_HOLD = 803734,
    SPELL_DISSIPATION = 680730,
    SPELL_ROTCLAW = 804197,
    SPELL_ROTCLAW_ENERGIZE = 805352, // Ravenous Strike (Energize): 30..70 internal, i.e. 3 to 7 Rage
    SPELL_CURSED_BLOOD = 681792,
    SPELL_CURSED_BLOOD_DEBUFF = 803722
};

// Every rank of the two abilities Enthraller empowers.
constexpr uint32 EnthralledRanks[] = {560249, 561175, 561176, 561177,
    560315, 561027, 561028, 561029};

// Hemal Excision: collects the target's curse auras for the reactivation window.
std::vector<Aura*> CollectCurses(Unit* target)
{
    std::vector<Aura*> curses;
    if (!target)
        return curses;
    for (auto const& [_, applications] : target->GetAppliedAuras())
        if (Aura* aura = applications->GetBase(); aura && aura->GetSpellInfo()->Dispel == DISPEL_CURSE &&
            aura->GetCasterGUID() != target->GetGUID())
            curses.push_back(aura);
    return curses;
}

// Every rank of Bloodfang Bite the kit currently teaches.
constexpr uint32 BloodfangBiteRanks[] = {501695, 501696, 501697, 503613, 503614,
    503615, 572549, 572550, 572551, 800156};

// Dark Essence (680732): heals a Blood-Rituals-marked ally every 1.5 seconds for
// three seconds — two scheduled ticks.
class DarkEssenceTick : public BasicEvent
{
public:
    DarkEssenceTick(ObjectGuid owner, ObjectGuid ally, uint32 amount) : _owner(owner),
        _ally(ally), _amount(amount) { }

    bool Execute(uint64, uint32) override
    {
        Player* player = ObjectAccessor::FindConnectedPlayer(_owner);
        if (!player || !player->IsInWorld())
            return true;
        Unit* target = ObjectAccessor::GetUnit(*player, _ally);
        if (target && target->IsInWorld() && target->HasAura(SPELL_BLOOD_RITUALS_MARK, _owner))
            player->CastCustomSpell(SPELL_BLOOD_RITUALS_HEAL, SPELLVALUE_BASE_POINT0,
                int32(_amount), target, true);
        return true;
    }

private:
    ObjectGuid _owner;
    ObjectGuid _ally;
    uint32 _amount;
};

bool RankOf(uint32 id, uint32 root)
{
    return id == root || sSpellMgr->GetFirstSpellInChain(id) == root;
}

Player* Bloodmage(Spell* spell)
{
    Player* player = spell->GetCaster()->ToPlayer();
    return player && player->getClass() == CLASS_SON_OF_ARUGAL &&
        spell->GetSpellInfo()->SpellFamilyName == 26 ? player : nullptr;
}

void CopyDamage(Player* player, Unit* target, uint32 id, uint32 damage)
{
    if (!damage)
        return;
    // Custom basepoints pass through native float arithmetic before conversion back to int32.
    uint32 maximum = uint32(std::nextafter(float(std::numeric_limits<int32>::max()), 0.0f));
    player->CastCustomSpell(id, SPELLVALUE_BASE_POINT0, int32(std::min(damage, maximum)), target, true);
}

class bloodmage_secondary_casts : public AllSpellScript
{
public:
    bloodmage_secondary_casts() : AllSpellScript("bloodmage_secondary_casts",
        {ALLSPELLHOOK_ON_CAST, ALLSPELLHOOK_ON_CALCULATED_TARGET, ALLSPELLHOOK_ON_CRIT_CHANCE,
            ALLSPELLHOOK_ON_HIT_RESULT}) { }

    void OnSpellCast(Spell* spell, Unit*, SpellInfo const* info, bool) override
    {
        Player* player = Bloodmage(spell);
        if (!player || spell->IsTriggered())
            return;
        if (player->HasAura(SPELL_NIGHT_HUNTER))
        {
            if (RankOf(info->Id, SPELL_VEINBURST))
            {
                if (roll_chance_i(40))
                    player->RemoveSpellCooldown(info->Id, true);
            }
            else if (AscensionBloodmage::GetEmpowerment(info->Id) == AscensionBloodmage::Bloodbolt &&
                info->PowerType == POWER_RAGE && spell->GetPowerCost() > 0 && roll_chance_i(40))
                player->ModifyPower(POWER_RAGE, spell->GetPowerCost() / 2);
        }
        // Dark Essence (680732): Cursed Form abilities and Bloodbolt pulse a small
        // heal every 1.5 seconds for three seconds into every ally carrying this
        // Bloodmage's Blood Rituals mark (114 + 10% healing bonus per tick).
        bool cursedAbility = player->HasAura(AscensionBloodmage::CursedForm) &&
            info->SpellFamilyName == 26 && info->PowerType == POWER_RAGE &&
            (info->ManaCost || info->ManaCostPercentage);
        if (!player->HasAura(SPELL_DARK_ESSENCE) ||
            (!cursedAbility && AscensionBloodmage::GetEmpowerment(info->Id) != AscensionBloodmage::Bloodbolt))
            return;
        uint32 const amount = 114 + uint32(player->SpellBaseHealingBonusDone(SPELL_SCHOOL_MASK_SHADOW) / 10);
        std::vector<Unit*> marked;
        for (auto const& reference : player->GetMap()->GetPlayers())
            if (Player* member = reference.GetSource())
                if (member->IsInWorld() && member->HasAura(SPELL_BLOOD_RITUALS_MARK, player->GetGUID()))
                    marked.push_back(member);
        for (Unit* ally : marked)
        {
            player->m_Events.AddEvent(new DarkEssenceTick(player->GetGUID(), ally->GetGUID(), amount),
                player->m_Events.CalculateTime(1500));
            player->m_Events.AddEvent(new DarkEssenceTick(player->GetGUID(), ally->GetGUID(), amount),
                player->m_Events.CalculateTime(3000));
        }
        // Fang Over Fang (504116): Bloodfang Bite has a twenty percent chance to
        // reset Reave's cooldown.
        if (player->HasAura(SPELL_FANG_OVER_FANG) && roll_chance_i(20) &&
            std::find(std::begin(BloodfangBiteRanks), std::end(BloodfangBiteRanks), info->Id) !=
                std::end(BloodfangBiteRanks))
            player->RemoveSpellCooldown(SPELL_REAVE, true);
        // Aortic Aegis (806274): Blood Veil spreads to the target's party members.
        if (player->HasAura(SPELL_AORTIC_AEGIS) && RankOf(info->Id, SPELL_BLOOD_VEIL) && !spell->IsTriggered())
            if (Unit* hitTarget = spell->m_targets.GetUnitTarget())
                if (Player* target = hitTarget->ToPlayer())
                for (auto const& reference : target->GetMap()->GetPlayers())
                    if (Player* member = reference.GetSource())
                        if (member->IsInWorld() && member != target &&
                            member->IsWithinDistInMap(target, 30.0f) &&
                            (member->IsInPartyWith(target) || member->IsInRaidWith(target)))
                            player->CastSpell(member, info->Id, true);
        // Hemal Excision (803681): the Dispel effect siphons curses natively; the
        // reactivation window is banked here so the second cast can reapply them.
        if (info->Id == SPELL_HEMAL_EXCISION && !spell->IsTriggered())
        {
            if (Unit* target = spell->m_targets.GetUnitTarget())
                if (std::vector<Aura*> curses = CollectCurses(target); !curses.empty())
                {
                    spell->SetScriptValue(SPELL_HEMAL_EXCISION_HOLD, 1);
                    for (Aura* curse : curses)
                        spell->SetScriptValue(uint32(curse->GetId()), 1);
                }
            player->CastSpell(player, SPELL_HEMAL_EXCISION_HOLD, true);
        }
        // Dissipation (680730): Fleshcraft's cooldown starts a minute shorter.
        if (player->HasAura(SPELL_DISSIPATION) && RankOf(info->Id, SPELL_FLESHCRAFT) && !spell->IsTriggered())
            player->ModifySpellCooldown(SPELL_FLESHCRAFT, -60000);
        // Thirst for Blood (570023): mirror the Thirst stack range onto the
        // Sated (1-5) and Ravenous (6-10) bonuses.
        if (player->HasAura(SPELL_THIRST_FOR_BLOOD))
        {
            uint32 const thirst = player->GetAura(SPELL_THIRST) ? player->GetAura(SPELL_THIRST)->GetStackAmount() : 0;
            bool const wantSated = thirst >= 1 && thirst <= 5;
            bool const wantRavenous = thirst >= 6;
            if (wantSated != bool(player->GetAura(SPELL_SATED)))
                if (wantSated)
                    player->CastSpell(player, SPELL_SATED, true);
                else
                    player->RemoveAurasDueToSpell(SPELL_SATED);
            if (wantRavenous != bool(player->GetAura(SPELL_RAVENOUS)))
                if (wantRavenous)
                    player->CastSpell(player, SPELL_RAVENOUS, true);
                else
                    player->RemoveAurasDueToSpell(SPELL_RAVENOUS);
        }
    }

    void OnSpellCritChance(Spell* spell, Unit* target, float& chance) override
    {
        if (Bloodmage(spell) && target && RankOf(spell->GetSpellInfo()->Id, SPELL_HEMOBURST) &&
            target->HasAuraState(AURA_STATE_BLEEDING))
            chance = 100;
        // Atherann's Anguish: hemoplague damage cannot critically strike.
        if (spell->GetSpellInfo()->Id == SPELL_ATHERANNS_ANGUISH_BURST)
            chance = 0;
        // Blood Prince's Command (704641): Bloodbolt always crits targets carrying
        // Taldaram's Torment (any rank of the torment DoT).
        Player* player = Bloodmage(spell);
        if (player && target && player->HasAura(SPELL_BLOOD_PRINCES_COMMAND) &&
            AscensionBloodmage::GetEmpowerment(spell->GetSpellInfo()->Id) == AscensionBloodmage::Bloodbolt)
            for (uint32 torment : {800772, 802568, 802569, 802570})
                if (target->HasAura(torment))
                {
                    chance = 100;
                    break;
                }
        // Enthraller (706619): Valanar's Vengeance and Keleseth's Calamity gain
        // ten percent critical strike chance.
        if (player && player->HasAura(SPELL_ENTHRALLER) &&
            std::find(std::begin(EnthralledRanks), std::end(EnthralledRanks), spell->GetSpellInfo()->Id) !=
                std::end(EnthralledRanks))
            chance += 10.0f;
    }

    void OnSpellCalculatedTarget(Spell* spell, Unit* target, TargetInfo& hit) override
    {
        Player* player = Bloodmage(spell);
        if (!player || !target || hit.damage <= 0 || hit.missCondition != SPELL_MISS_NONE)
            return;
        uint32 id = spell->GetSpellInfo()->Id;
        if (RankOf(id, SPELL_VEINBURST) && target->HasAuraState(AURA_STATE_BLEEDING))
        {
            hit.damage = uint32(hit.damage * 1.25f);
            hit.damageBeforeTakenMods = uint32(hit.damageBeforeTakenMods * 1.25f);
        }
        if (!spell->IsTriggered() && RankOf(id, SPELL_REAVE))
        {
            uint32 conditions = target->HasAuraState(AURA_STATE_BLEEDING) ? 1 : 0;
            if (target->HealthBelowPct(35))
                conditions |= 2;
            if (!target->HasInArc(float(M_PI), player))
                conditions |= 4;
            // Reave has one victim. Snapshot its conditions before the initial hit changes health.
            spell->SetScriptValue(SPELL_REAVE, conditions);
        }
    }

    void OnSpellHitResult(Spell* spell, Unit* target, uint8 miss, uint32 damage, uint32, bool critical) override
    {
        Player* player = Bloodmage(spell);
        if (!player || spell->IsTriggered() || !target || miss != SPELL_MISS_NONE)
            return;
        uint32 id = spell->GetSpellInfo()->Id;
        if (target == player && (id == SPELL_CURSED_FORM || id == SPELL_ACCURSED_FORM))
            // Native duration modifiers have already extended this application.
            player->RemoveAurasDueToSpell(SPELL_HEMOTURGY);
        if (target == player || player->IsFriendlyTo(target))
            return;
        if (RankOf(id, SPELL_HEMOBURST) && !spell->GetScriptValue(SPELL_HEMOTURGY))
        {
            spell->SetScriptValue(SPELL_HEMOTURGY, 1);
            player->CastSpell(player, SPELL_HEMOTURGY, true);
        }
        // Rotclaw's description promises the income ("dealing ... Shadow damage, generating Rage and
        // infecting their wounds"), but none of its three effects is an energize and no companion
        // "Rotclaw (Energize)" record exists, unlike Ravenous Strike, Bloodmoon Blast, Sanguine Rupture
        // and Lunge, which all carry effect 142 pointing at their own Energize spell. Unit::DealDamage
        // only pays Rage for weapon damage, so the ability granted none. The amount is in no source -
        // not Spell.dbc, not spell_proc/spell_linked_spell, not the 2026-09-13 exiles-db export - so the
        // Rage is paid with Ravenous Strike's own Energize record rather than a new number, and once per
        // cast: Rotclaw hits up to 25 enemies.
        if (RankOf(id, SPELL_ROTCLAW) && !spell->GetScriptValue(SPELL_ROTCLAW_ENERGIZE))
        {
            spell->SetScriptValue(SPELL_ROTCLAW_ENERGIZE, 1);
            player->CastSpell(player, SPELL_ROTCLAW_ENERGIZE, true);
        }
        if (!damage)
            return;
        // Sovereignty (806049): Crimson Tide damage banks a stack of the buff,
        // whose cost reduction and extra Bloodbolt bounce are native.
        if (player->HasAura(SPELL_SOVEREIGNTY) &&
            AscensionBloodmage::GetEmpowerment(id) == AscensionBloodmage::CrimsonTide)
            player->CastSpell(player, SPELL_SOVEREIGNTY_BUFF, true);
        // Flesh Foundry (704633): critical strikes trim four seconds off
        // Fleshcraft's cooldown. The crit-rating-from-Spirit effect is native.
        if (critical && player->HasAura(SPELL_FLESH_FOUNDRY))
            player->ModifySpellCooldown(SPELL_FLESHCRAFT, -4000);
        // Crimson Scion (806424): direct damage rolls a ten percent chance to make
        // the next Sanguine Mend instant; the proc aura carries the -100% cast
        // modifier natively.
        if (player->HasAura(SPELL_CRIMSON_SCION) && !spell->IsTriggered() &&
            roll_chance_i(10) && !player->HasAura(SPELL_CRIMSON_SCION_PROC))
            player->CastSpell(player, SPELL_CRIMSON_SCION_PROC, true);
        // Issue 806: Cursed Blood makes Bloodbolt damage apply the 803722
        // Jinx (resistance shred + magic damage taken) for 10s.
        if (AscensionBloodmage::GetEmpowerment(id) == AscensionBloodmage::Bloodbolt &&
            player->HasAura(SPELL_CURSED_BLOOD))
            player->CastSpell(target, SPELL_CURSED_BLOOD_DEBUFF, true);
        if (RankOf(id, SPELL_REAVE) && !spell->GetScriptValue(SPELL_REAVE_BLEED))
        {
            spell->SetScriptValue(SPELL_REAVE_BLEED, 1);
            uint32 conditions = uint32(spell->GetScriptValue(SPELL_REAVE));
            if (conditions & 1)
                CopyDamage(player, target, SPELL_REAVE_BLEED, damage);
            if (conditions & 2)
                CopyDamage(player, target, SPELL_REAVE_EXECUTE, damage);
            if (conditions & 4)
                CopyDamage(player, target, SPELL_REAVE_BACK, damage);
        }
        if (target->HasAura(SPELL_ATHERANNS_ANGUISH, player->GetGUID()))
        {
            Aura* mark = target->GetAura(SPELL_ATHERANNS_ANGUISH, player->GetGUID());
            if (AuraEffect* bank = mark->GetEffect(EFFECT_2))
                bank->ChangeAmount(bank->GetAmount() + int32(damage * 0.30f));
        }
    }
};

// Infuse (681403): every point of damage the Bloodmage's party, raid and their
// minions deal to the marked target banks on the mark and detonates as Shadow
// damage when the ten seconds run out. Early dispels fizzle.
class bloodmage_infuse_marks : public UnitScript
{
public:
    bloodmage_infuse_marks() : UnitScript("bloodmage_infuse_marks", true,
        {UNITHOOK_ON_DAMAGE, UNITHOOK_ON_AURA_REMOVE}) { }

    static bool Contributes(Player const* source, Player const* infuser)
    {
        return source && infuser && (source == infuser || source->IsInPartyWith(infuser) ||
            source->IsInRaidWith(infuser));
    }

    void OnDamage(Unit* attacker, Unit* victim, uint32& damage) override
    {
        if (!victim || !damage)
            return;
        Aura* infuse = victim->GetAura(SPELL_INFUSE);
        if (!infuse)
            return;
        Player* infuser = ObjectAccessor::FindConnectedPlayer(infuse->GetCasterGUID());
        if (!Contributes(attacker ? attacker->GetCharmerOrOwnerPlayerOrPlayerItself() : nullptr, infuser))
            return;
        if (AuraEffect* bank = infuse->GetEffect(EFFECT_0))
            bank->ChangeAmount(bank->GetAmount() + int32(damage));
    }

    void OnAuraRemove(Unit* unit, AuraApplication* aurApp, AuraRemoveMode mode) override
    {
        Aura const* aura = aurApp ? aurApp->GetBase() : nullptr;
        if (!aura || aura->GetId() != SPELL_INFUSE || mode != AURA_REMOVE_BY_EXPIRE || !unit)
            return;
        Player* infuser = ObjectAccessor::FindConnectedPlayer(aura->GetCasterGUID());
        if (!infuser || !infuser->IsAlive() || !infuser->IsInWorld())
            return;
        if (AuraEffect const* bank = aura->GetEffect(EFFECT_0); bank && bank->GetAmount() > 0)
            infuser->CastCustomSpell(SPELL_INFUSE_BURST, SPELLVALUE_BASE_POINT0,
                bank->GetAmount(), unit, true);
    }
};

class bloodmage_kiss_periodic : public UnitScript
{
public:
    bloodmage_kiss_periodic() : UnitScript("bloodmage_kiss_periodic", true,
        {UNITHOOK_ON_PERIODIC_DAMAGE_RESULT}) { }

    void OnPeriodicDamageResult(Unit* target, Unit*, uint32 damage, SpellInfo const*) override
    {
        if (!target || !target->IsAlive() || !damage)
            return;
        // The active curse copies all periodic damage to its victim, including allied casters' ticks.
        // Collect owner GUIDs before casting, since a copy can kill the target and remove its auras.
        std::vector<ObjectGuid> owners;
        for (auto const& pair : target->GetAppliedAuras())
        {
            Aura* aura = pair.second->GetBase();
            if (RankOf(aura->GetId(), SPELL_VAMPYRS_KISS) &&
                std::find(owners.begin(), owners.end(), aura->GetCasterGUID()) == owners.end())
                owners.push_back(aura->GetCasterGUID());
        }
        for (ObjectGuid const& guid : owners)
            if (Unit* unit = ObjectAccessor::GetUnit(*target, guid))
                if (Player* player = unit->ToPlayer(); player && player->getClass() == CLASS_SON_OF_ARUGAL &&
                    player->IsAlive() && player->IsInWorld() && player->InSamePhase(target) && target->IsAlive() &&
                    player->IsValidAttackTarget(target))
                {
                    CopyDamage(player, target, SPELL_VAMPYRS_KISS_COPY, damage / 4);
                    // Black Heart: Vampyr's Kiss also regenerates 20% of maximum Rage when it copies damage.
                    if (player->HasAura(SPELL_BLACK_HEART))
                        player->ModifyPower(POWER_RAGE, int32(player->GetMaxPower(POWER_RAGE)) / 5);
                }
    }
};

class bloodmage_secondary_contracts : public GlobalScript
{
public:
    bloodmage_secondary_contracts() : GlobalScript("bloodmage_secondary_contracts",
        {GLOBALHOOK_ON_LOAD_SPELL_CUSTOM_ATTR}) { }

    void OnLoadSpellCustomAttr(SpellInfo* info) override
    {
        if (!info || info->SpellFamilyName != 26)
            return;
        if (info->Id == SPELL_REAVE_BLEED || info->Id == SPELL_REAVE_EXECUTE || info->Id == SPELL_REAVE_BACK ||
            info->Id == SPELL_VAMPYRS_KISS_COPY)
        {
            info->AttributesEx2 |= SPELL_ATTR2_CANT_CRIT;
            info->AttributesEx3 |= SPELL_ATTR3_IGNORE_CASTER_MODIFIERS;
            info->AttributesEx4 |= SPELL_ATTR4_IGNORE_DAMAGE_TAKEN_MODIFIERS;
            info->AscensionInheritsResolvedAmount = true;
            info->Effects[EFFECT_0].BonusMultiplier = 0.0f;
        }
    }
};

class spell_ascension_blood_feast_corpses : public SpellScript
{
    PrepareSpellScript(spell_ascension_blood_feast_corpses);

    void Select(std::list<WorldObject*>& targets)
    {
        Unit* caster = GetCaster();
        // Filter before the native three-target cap so living units cannot take corpse slots.
        targets.remove_if([caster](WorldObject* object)
        {
            Unit* target = object ? object->ToUnit() : nullptr;
            return !target || target->IsAlive() || target == caster || caster->IsFriendlyTo(target);
        });
    }

    void Register() override
    {
        OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_ascension_blood_feast_corpses::Select,
            EFFECT_0, TARGET_UNIT_SRC_AREA_ENTRY);
    }
};

class spell_ascension_blood_feast_drain : public SpellScript
{
    PrepareSpellScript(spell_ascension_blood_feast_drain);

    bool Validate(SpellInfo const*) override { return ValidateSpellInfo({SPELL_BLOOD_FEAST_RESTORE}); }

    void Drain(SpellEffIndex)
    {
        Player* player = GetCaster()->ToPlayer();
        Unit* corpse = GetHitUnit();
        if (player && player->getClass() == CLASS_SON_OF_ARUGAL && player->IsAlive() && !player->IsInCombat() &&
            corpse && !corpse->IsAlive() && corpse != player && !player->IsFriendlyTo(corpse))
            player->CastSpell(player, SPELL_BLOOD_FEAST_RESTORE, true);
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_ascension_blood_feast_drain::Drain, EFFECT_0, SPELL_EFFECT_DUMMY);
    }
};
}

void AddSC_AscensionBloodmageSecondary()
{
    new bloodmage_secondary_casts();
    new bloodmage_kiss_periodic();
    new bloodmage_infuse_marks();
    new bloodmage_secondary_contracts();
    RegisterSpellScript(spell_ascension_blood_feast_corpses);
    RegisterSpellScript(spell_ascension_blood_feast_drain);
}
