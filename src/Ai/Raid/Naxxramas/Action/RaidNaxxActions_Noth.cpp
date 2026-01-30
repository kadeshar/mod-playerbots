#include "RaidNaxxActions.h"

#include "PlayerbotAIConfig.h"
#include "Playerbots.h"
#include "RaidNaxxBossHelper.h"
#include "RaidNaxxSpellIds.h"

bool NothChooseTargetAction::Execute(Event event)
{
    if (!helper.UpdateBossAI())
    {
        return false;
    }

    GuidVector attackers = context->GetValue<GuidVector>("attackers")->Get();
    Unit* target = nullptr;
    Unit* target_boss = nullptr;
    Unit* target_champion = nullptr;
    Unit* target_guardian = nullptr;
    Unit* target_warrior = nullptr;

    for (auto i = attackers.begin(); i != attackers.end(); ++i)
    {
        Unit* unit = botAI->GetUnit(*i);
        if (!unit || !unit->IsAlive())
        {
            continue;
        }

        if (botAI->EqualLowercaseName(unit->GetName(), "noth the plaguebringer"))
        {
            target_boss = unit;
        }
        else if (botAI->EqualLowercaseName(unit->GetName(), "plagued champion"))
        {
            if (!target_champion || bot->GetDistance2d(unit) < bot->GetDistance2d(target_champion))
            {
                target_champion = unit;
            }
        }
        else if (botAI->EqualLowercaseName(unit->GetName(), "plagued guardian"))
        {
            if (!target_guardian || bot->GetDistance2d(unit) < bot->GetDistance2d(target_guardian))
            {
                target_guardian = unit;
            }
        }
        else if (botAI->EqualLowercaseName(unit->GetName(), "plagued warrior"))
        {
            if (!target_warrior || bot->GetDistance2d(unit) < bot->GetDistance2d(target_warrior))
            {
                target_warrior = unit;
            }
        }
    }

    std::vector<Unit*> targets;
    if (botAI->IsAssistTank(bot))
    {
        Unit* warrior_needs_pickup = nullptr;
        for (auto i = attackers.begin(); i != attackers.end(); ++i)
        {
            Unit* unit = botAI->GetUnit(*i);
            if (!unit || !unit->IsAlive())
            {
                continue;
            }
            if (!botAI->EqualLowercaseName(unit->GetName(), "plagued warrior"))
            {
                continue;
            }
            if (unit->GetVictim() && unit->GetVictim()->ToPlayer() &&
                !botAI->IsAssistTank(unit->GetVictim()->ToPlayer()))
            {
                warrior_needs_pickup = unit;
                break;
            }
        }
        if (helper.IsBalconyPhase())
        {
            targets = {warrior_needs_pickup, target_warrior, target_champion, target_guardian};
        }
        else
        {
            targets = {warrior_needs_pickup, target_warrior, target_boss};
        }
    }
    else if (helper.IsBalconyPhase())
    {
        targets = {target_champion, target_guardian, target_warrior};
    }
    else
    {
        targets = {target_boss};
    }

    for (Unit* t : targets)
    {
        if (t)
        {
            target = t;
            break;
        }
    }

    if (!target || context->GetValue<Unit*>("current target")->Get() == target)
    {
        return false;
    }

    return Attack(target);
}

bool NothPositionAction::Execute(Event event)
{
    if (!helper.UpdateBossAI())
    {
        return false;
    }

    if (botAI->IsAssistTank(bot))
    {
        GuidVector attackers = context->GetValue<GuidVector>("attackers")->Get();
        Unit* loose_warrior = nullptr;
        for (auto i = attackers.begin(); i != attackers.end(); ++i)
        {
            Unit* unit = botAI->GetUnit(*i);
            if (!unit || !unit->IsAlive())
            {
                continue;
            }
            if (!botAI->EqualLowercaseName(unit->GetName(), "plagued warrior"))
            {
                continue;
            }
            if (unit->GetVictim() && unit->GetVictim()->ToPlayer() &&
                !botAI->IsAssistTank(unit->GetVictim()->ToPlayer()))
            {
                loose_warrior = unit;
                break;
            }
        }
        if (loose_warrior && bot->GetDistance2d(loose_warrior) > 5.0f)
        {
            return MoveTo(NAXX_MAP_ID, loose_warrior->GetPositionX(), loose_warrior->GetPositionY(),
                          loose_warrior->GetPositionZ(), false, false, false, false, MovementPriority::MOVEMENT_COMBAT);
        }
        Unit* currentTarget = AI_VALUE(Unit*, "current target");
        if (currentTarget && botAI->EqualLowercaseName(currentTarget->GetName(), "plagued warrior"))
        {
            GuidVector friendlyPlayers = AI_VALUE(GuidVector, "nearest friendly players");
            Unit* closestPlayer = nullptr;
            float closestDistance = 0.0f;
            for (ObjectGuid const& guid : friendlyPlayers)
            {
                Unit* member = botAI->GetUnit(guid);
                if (!member || member == bot)
                {
                    continue;
                }
                float distance = bot->GetDistance2d(member);
                if (distance <= 5.0f && (!closestPlayer || distance < closestDistance))
                {
                    closestPlayer = member;
                    closestDistance = distance;
                }
            }
            if (closestPlayer)
            {
                float angle = closestPlayer->GetAngle(bot);
                float dx = closestPlayer->GetPositionX() + cos(angle) * 5.0f;
                float dy = closestPlayer->GetPositionY() + sin(angle) * 5.0f;
                return MoveTo(NAXX_MAP_ID, dx, dy, bot->GetPositionZ(), false, false, false, false,
                              MovementPriority::MOVEMENT_COMBAT);
            }
        }
        return false;
    }

    if (!helper.IsBalconyPhase() || !botAI->IsRanged(bot))
    {
        return false;
    }

    GuidVector attackers = context->GetValue<GuidVector>("attackers")->Get();
    Unit* nearest_champion = nullptr;
    float nearest_distance = 0.0f;

    for (auto i = attackers.begin(); i != attackers.end(); ++i)
    {
        Unit* unit = botAI->GetUnit(*i);
        if (!unit || !unit->IsAlive())
        {
            continue;
        }
        if (!botAI->EqualLowercaseName(unit->GetName(), "plagued champion"))
        {
            continue;
        }
        float distance = bot->GetDistance2d(unit);
        if (!nearest_champion || distance < nearest_distance)
        {
            nearest_champion = unit;
            nearest_distance = distance;
        }
    }

    if (nearest_champion && nearest_distance < 25.0f)
    {
        float angle = nearest_champion->GetAngle(bot);
        float dx = nearest_champion->GetPositionX() + cos(angle) * 25.0f;
        float dy = nearest_champion->GetPositionY() + sin(angle) * 25.0f;
        return MoveTo(NAXX_MAP_ID, dx, dy, bot->GetPositionZ(), false, false, false, false, MovementPriority::MOVEMENT_COMBAT);
    }

    return false;
}