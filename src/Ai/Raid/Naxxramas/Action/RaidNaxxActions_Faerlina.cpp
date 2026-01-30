#include "RaidNaxxActions.h"

#include "RaidNaxxSpellIds.h"

#include <limits>

namespace
{
    constexpr uint32 NpcNaxxramasWorshipper = 16506;
    constexpr uint32 NpcNaxxramasFollower = 16505;
    constexpr float MaxAddDistanceToBoss = 60.0f;
}

bool FaerlinaSacrificeWorshipperAction::Execute(Event event)
{
    return AttackAction::Execute(event);
}

bool FaerlinaSacrificeWorshipperAction::isUseful()
{
    if (!bot->IsInCombat())
    {
        return false;
    }

    if (!botAI->IsAssistTankOfIndex(bot, 0))
    {
        return false;
    }

    Unit* boss = AI_VALUE2(Unit*, "find target", "grand widow faerlina");
    if (!boss)
    {
        return false;
    }

    if (boss->HasAura(NaxxSpellIds::FaerlinaWidowsEmbrace))
    {
        return false;
    }

    if (!boss->HasAura(NaxxSpellIds::FaerlinaFrenzy))
    {
        return false;
    }

    return GetTarget() != nullptr;
}

Unit* FaerlinaSacrificeWorshipperAction::GetTarget()
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "grand widow faerlina");
    if (!boss)
    {
        return nullptr;
    }

    Creature* bestWorshipper = nullptr;
    float bestWorshipperDist = std::numeric_limits<float>::max();

    Creature* bestFollower = nullptr;
    float bestFollowerDist = std::numeric_limits<float>::max();

    GuidVector const npcs = AI_VALUE(GuidVector, "nearest hostile npcs");
    for (ObjectGuid const guid : npcs)
    {
        Unit* unit = botAI->GetUnit(guid);
        if (!unit || unit->isDead())
        {
            continue;
        }

        Creature* creature = unit->ToCreature();
        if (!creature)
        {
            continue;
        }

        if (!creature->IsWithinDistInMap(boss, MaxAddDistanceToBoss))
        {
            continue;
        }

        uint32 const entry = creature->GetEntry();
        float const distToBoss = creature->GetDistance(boss);

        if (entry == NpcNaxxramasWorshipper)
        {
            if (distToBoss < bestWorshipperDist)
            {
                bestWorshipper = creature;
                bestWorshipperDist = distToBoss;
            }
        }
        else if (entry == NpcNaxxramasFollower)
        {
            if (distToBoss < bestFollowerDist)
            {
                bestFollower = creature;
                bestFollowerDist = distToBoss;
            }
        }
    }

    // Prefer worshippers for the intended mechanic. Followers are a fallback for clean-up.
    return bestWorshipper ? static_cast<Unit*>(bestWorshipper) : static_cast<Unit*>(bestFollower);
}