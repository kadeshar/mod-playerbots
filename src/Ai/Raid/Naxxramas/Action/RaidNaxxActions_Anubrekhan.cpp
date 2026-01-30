#include "RaidNaxxActions.h"

#include "ObjectGuid.h"
#include "Playerbots.h"
#include "RaidNaxxSpellIds.h"

bool AnubrekhanChooseTargetAction::Execute(Event event)
{
    GuidVector attackers = context->GetValue<GuidVector>("attackers")->Get();
    Unit* target = nullptr;
    Unit* targetBoss = nullptr;
    std::vector<Unit*> cryptGuards;
    std::vector<Unit*> corpseScarabs;
    for (ObjectGuid const guid : attackers)
    {
        Unit* unit = botAI->GetUnit(guid);
        if (!unit)
            continue;

        if (botAI->EqualLowercaseName(unit->GetName(), "anub'rekhan"))
            targetBoss = unit;
        else if (botAI->EqualLowercaseName(unit->GetName(), "crypt guard"))
            cryptGuards.push_back(unit);
        else if (botAI->EqualLowercaseName(unit->GetName(), "corpse scarab"))
            corpseScarabs.push_back(unit);
    }

    if (!targetBoss && cryptGuards.empty() && corpseScarabs.empty())
        return false;

    if (botAI->IsMainTank(bot))
    {
        target = targetBoss;
    }
    else if (botAI->IsAssistTank(bot))
    {
        for (Unit* add : cryptGuards)
        {
            Player* victim = add->GetVictim() ? add->GetVictim()->ToPlayer() : nullptr;
            if (!victim || !botAI->IsTank(victim))
            {
                target = add;
                break;
            }
        }
        if (!target)
        {
            if (!cryptGuards.empty())
                target = cryptGuards.front();
            else
                target = targetBoss;
        }
    }
    else
    {
        if (!cryptGuards.empty())
        {
            for (Unit* add : cryptGuards)
            {
                if (!target || target->GetHealthPct() > add->GetHealthPct())
                    target = add;
            }
        }
        else if (!corpseScarabs.empty())
        {
            for (Unit* scarab : corpseScarabs)
            {
                Player* victim = scarab->GetVictim() ? scarab->GetVictim()->ToPlayer() : nullptr;
                if (victim && !botAI->IsTank(victim))
                {
                    target = scarab;
                    break;
                }
            }

            if (!target)
                target = corpseScarabs.front();
        }
        else
        {
            target = targetBoss;
        }
    }

    if (!target)
        return false;

    if (context->GetValue<Unit*>("current target")->Get() == target)
    {
        return false;
    }
    return Attack(target);
}

bool AnubrekhanPositionAction::Execute(Event event)
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "anub'rekhan");
    if (!boss)
    {
        return false;
    }

    bool inPhase = NaxxSpellIds::HasAnyAura(botAI, boss, {NaxxSpellIds::LocustSwarm10, NaxxSpellIds::LocustSwarm10Alt,
                                                         NaxxSpellIds::LocustSwarm25}) ||
                   botAI->HasAura("locust swarm", boss);
    if (inPhase)
    {
        if (botAI->IsMainTank(bot))
        {
            uint32 nearest = FindNearestWaypoint();
            uint32 nextPoint = (nearest + 1) % intervals;
            return MoveTo(bot->GetMapId(), waypoints[nextPoint].first, waypoints[nextPoint].second, bot->GetPositionZ(), false, false,
                          false, false, MovementPriority::MOVEMENT_COMBAT);
        }
        else
        {
            return MoveInside(533, 3272.49f, -3476.27f, bot->GetPositionZ(), 3.0f, MovementPriority::MOVEMENT_COMBAT);
        }
    }
    return false;
}