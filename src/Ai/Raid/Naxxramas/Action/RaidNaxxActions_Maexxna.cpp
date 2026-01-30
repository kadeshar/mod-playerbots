#include <limits>

#include "Playerbots.h"
#include "RaidNaxxActions.h"
#include "RaidNaxxSpellIds.h"

bool MaexxnaAttackWebWrapAction::isUseful()
{
    // Prefer ranged DPS/casters for cocoon breaking to minimize movement and boss downtime.
    if (botAI->IsHeal(bot) || botAI->IsTank(bot))
        return false;

    return botAI->IsRanged(bot);
}

bool MaexxnaAttackWebWrapAction::Execute(Event /*event*/)
{
    Unit* best = nullptr;
    float bestDist = std::numeric_limits<float>::max();

    GuidVector targets = AI_VALUE(GuidVector, "possible targets no los");
    for (ObjectGuid const& guid : targets)
    {
        Unit* unit = botAI->GetUnit(guid);
        if (!unit)
            continue;

        if (unit->GetEntry() != NaxxSpellIds::MaexxnaWebWrapEntry)
            continue;

        float d = bot->GetDistance(unit);
        if (!best || d < bestDist)
        {
            best = unit;
            bestDist = d;
        }
    }

    if (!best)
        return false;

    if (AI_VALUE(Unit*, "current target") == best)
        return false;

    return Attack(best);
}

bool MaexxnaTankSpiderlingsAction::isUseful()
{
    // Keep main tank on Maexxna; use this only for an off-tank (25-man typical).
    return botAI->IsTank(bot) && !botAI->IsMainTank(bot);
}

bool MaexxnaTankSpiderlingsAction::Execute(Event /*event*/)
{
    Unit* best = nullptr;
    float bestDist = std::numeric_limits<float>::max();

    GuidVector attackers = AI_VALUE(GuidVector, "attackers");
    for (ObjectGuid const& guid : attackers)
    {
        Unit* unit = botAI->GetUnit(guid);
        if (!unit)
            continue;

        if (unit->GetEntry() != NaxxSpellIds::MaexxnaSpiderlingEntry)
            continue;

        float d = bot->GetDistance(unit);
        if (!best || d < bestDist)
        {
            best = unit;
            bestDist = d;
        }
    }

    if (!best)
        return false;

    if (AI_VALUE(Unit*, "current target") == best)
        return false;

    return Attack(best);
}