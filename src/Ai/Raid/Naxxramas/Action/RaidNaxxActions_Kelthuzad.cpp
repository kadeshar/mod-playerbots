#include "RaidNaxxActions.h"

#include <algorithm>
#include <cmath>

#include "PlayerbotAIConfig.h"
#include "Playerbots.h"

bool KelthuzadChooseTargetAction::Execute(Event event)
{
    if (!helper.UpdateBossAI())
    {
        return false;
    }
    GuidVector attackers = context->GetValue<GuidVector>("possible targets")->Get();
    Unit* target = nullptr;
    Unit *target_soldier = nullptr, *target_weaver = nullptr, *target_abomination = nullptr, *target_kelthuzad = nullptr,
         *target_guardian = nullptr;

    bool isOffTankForKT = botAI->IsTank(bot) && !botAI->IsMainTank(bot) &&
                          (botAI->IsAssistTank(bot) || botAI->HasStrategy("tank assist", BOT_STATE_COMBAT));

    for (auto i = attackers.begin(); i != attackers.end(); ++i)
    {
        Unit* unit = botAI->GetUnit(*i);
        if (!unit)
            continue;

        if (unit->GetDistance2d(helper.center.first, helper.center.second) > 30.0f)
        {
            continue;
        }
        if (bot->GetDistance2d(unit) > sPlayerbotAIConfig.spellDistance)
        {
            continue;
        }
        if (botAI->EqualLowercaseName(unit->GetName(), "unstoppable abomination"))
        {
            if (target_abomination == nullptr ||
                target_abomination->GetDistance2d(helper.center.first, helper.center.second) >
                    unit->GetDistance2d(helper.center.first, helper.center.second))
            {
                target_abomination = unit;
            }
        }
        if (botAI->EqualLowercaseName(unit->GetName(), "soldier of the frozen wastes"))
        {
            if (target_soldier == nullptr ||
                target_soldier->GetDistance2d(helper.center.first, helper.center.second) >
                    unit->GetDistance2d(helper.center.first, helper.center.second))
            {
                target_soldier = unit;
            }
        }
        if (botAI->EqualLowercaseName(unit->GetName(), "soul weaver"))
        {
            if (target_weaver == nullptr || target_weaver->GetDistance2d(helper.center.first, helper.center.second) >
                                                unit->GetDistance2d(helper.center.first, helper.center.second))
            {
                target_weaver = unit;
            }
        }
        if (botAI->EqualLowercaseName(unit->GetName(), "kel'thuzad"))
        {
            target_kelthuzad = unit;
        }
    }

    std::vector<Unit*> guardians = helper.GetGuardians();
    bool guardiansPresent = !guardians.empty();
    if (isOffTankForKT && guardiansPresent)
    {
        target_guardian = helper.GetGuardianToPickup(bot);
    }
    std::vector<Unit*> targets;
    if (botAI->IsRanged(bot))
    {
        bool hasRemainingP1Adds = (target_weaver || target_soldier || target_abomination);

        if (helper.IsPhaseTwo() && hasRemainingP1Adds && !botAI->IsHeal(bot))
            targets = {target_weaver, target_soldier, target_abomination, target_kelthuzad};
        else if (helper.IsPhaseTwo())
            targets = {target_kelthuzad, target_weaver, target_soldier, target_abomination};
        else
            targets = {target_weaver, target_soldier, target_abomination, target_kelthuzad};
    }
    else if (isOffTankForKT)
    {
       if (guardiansPresent)
           targets = {target_guardian};
       else
           targets = {target_abomination, target_kelthuzad};
    }
    else
    {
        targets = {target_abomination, target_kelthuzad};
    }
    for (Unit* t : targets)
    {
        if (!botAI->IsRanged(bot))
        {
            float maxCenterDist = 20.0f;

            if (isOffTankForKT && guardiansPresent)
                maxCenterDist = KelthuzadBossHelper::ROOM_MAX_RADIUS + 2.0f;

            if (t && t->GetDistance2d(helper.center.first, helper.center.second) > maxCenterDist)
            {
                continue;
            }
        }
        if (t)
        {
            target = t;
            break;
        }
    }
    if (context->GetValue<Unit*>("current target")->Get() == target)
    {
        return false;
    }
    if (target_kelthuzad && target == target_kelthuzad)
    {
        return Attack(target, true);
    }
    return Attack(target, false);
}

bool KelthuzadPositionAction::Execute(Event event)
{
    if (!helper.UpdateBossAI())
    {
        return false;
    }

    helper.RecallControlledPetsToBot();

    if (helper.IsPhaseOne())
    {
        if (botAI->IsTank(bot))
        {
            float dx = helper.center.first;
            float dy = helper.center.second;

            helper.ClampToRoom(dx, dy,
                KelthuzadBossHelper::PHASE1_TANK_HOLD_RADIUS,
                KelthuzadBossHelper::PHASE1_TANK_HOLD_RADIUS);

            if (bot->GetDistance2d(helper.center.first, helper.center.second) > KelthuzadBossHelper::PHASE1_TANK_MAX_RADIUS)
            {
                return MoveTo(NAXX_MAP_ID, dx, dy, bot->GetPositionZ(), false, false, false, false,
                              MovementPriority::MOVEMENT_COMBAT);
            }

            Unit* currentTarget = AI_VALUE(Unit*, "current target");
            if (currentTarget &&
                currentTarget->GetDistance2d(helper.center.first, helper.center.second) <= KelthuzadBossHelper::PHASE1_TANK_MAX_RADIUS)
            {
                if (bot->GetDistance2d(currentTarget) > 3.0f)
                    return MoveNear(currentTarget, 3.0f, MovementPriority::MOVEMENT_COMBAT);
            }

            return false;
        }
        if (bot->GetDistance2d(helper.center.first, helper.center.second) > 20.0f)
        {
            return MoveInside(NAXX_MAP_ID, helper.center.first, helper.center.second, bot->GetPositionZ(), 3.0f,
                              MovementPriority::MOVEMENT_COMBAT);
        }
        if (!botAI->IsRanged(bot))
        {
            Unit* currentTarget = AI_VALUE(Unit*, "current target");
            if (currentTarget &&
                currentTarget->GetDistance2d(helper.center.first, helper.center.second) <= 20.0f &&
                bot->GetDistance2d(currentTarget) > 3.0f)
            {
                return MoveNear(currentTarget, 3.0f, MovementPriority::MOVEMENT_COMBAT);
            }
        }
        if (AI_VALUE(Unit*, "current target") == nullptr)
        {
            return MoveInside(NAXX_MAP_ID, helper.center.first, helper.center.second, bot->GetPositionZ(), 3.0f,
                              MovementPriority::MOVEMENT_COMBAT);
        }
    }
    else if (helper.IsPhaseTwo())
    {
        if (helper.HasDetonateMana(bot))
        {
            float angle = helper.center.first == bot->GetPositionX() && helper.center.second == bot->GetPositionY()
                              ? 0.0f
                              : bot->GetAngle(helper.center.first, helper.center.second) + M_PI;
            float currentDist = bot->GetDistance2d(helper.center.first, helper.center.second);
            float spreadDistance = std::clamp(currentDist + 12.0f,
                                              KelthuzadBossHelper::DETONATE_MIN_RADIUS,
                                              KelthuzadBossHelper::DETONATE_MAX_RADIUS);
            float dx = helper.center.first + std::cos(angle) * spreadDistance;
            float dy = helper.center.second + std::sin(angle) * spreadDistance;
            helper.ClampToRoom(dx, dy, KelthuzadBossHelper::DETONATE_MIN_RADIUS, KelthuzadBossHelper::DETONATE_MAX_RADIUS);
            return MoveTo(NAXX_MAP_ID, dx, dy, bot->GetPositionZ(), false, false, false, false,
                          MovementPriority::MOVEMENT_COMBAT);
        }
        if (helper.HasChains(bot))
        {
            bot->AttackStop();
            return false;
        }
        Player* frostBlastTarget = helper.GetPlayerWithAura(NaxxSpellIds::FrostBlast);
        if (frostBlastTarget && frostBlastTarget != bot && bot->GetDistance2d(frostBlastTarget) <= 8.0f)
        {
            float angle = frostBlastTarget->GetAngle(bot);
            float dx = frostBlastTarget->GetPositionX() + cos(angle) * 8.0f;
            float dy = frostBlastTarget->GetPositionY() + sin(angle) * 8.0f;
            helper.ClampToRoom(dx, dy);
            return MoveTo(NAXX_MAP_ID, dx, dy, bot->GetPositionZ(), false, false, false, false, MovementPriority::MOVEMENT_COMBAT);
        }
        Unit* shadow_fissure = helper.GetAnyShadowFissure();
        if (!shadow_fissure || !bot->IsWithinDistInMap(shadow_fissure, 10.0f))
        {
            bool isOffTankForKT = botAI->IsTank(bot) && !botAI->IsMainTank(bot) &&
                                  (botAI->IsAssistTank(bot) || botAI->HasStrategy("tank assist", BOT_STATE_COMBAT));

            if (botAI->IsMainTank(bot))
            {
                if (AI_VALUE2(bool, "has aggro", "current target"))
                {
                    auto hold = helper.GetMainTankHoldPosition();
                    return MoveTo(NAXX_MAP_ID, hold.first, hold.second, bot->GetPositionZ(), false, false, false, false,
                                  MovementPriority::MOVEMENT_COMBAT);
                }
                else
                {
                    return false;
                }
            }
            else if (botAI->IsRanged(bot))
            {
                float dx, dy;
                uint32 index = botAI->GetRangedIndex(bot);
                uint32 total = std::max<uint32>(1, helper.GetRangedCount());
                helper.ComputeRangedSpreadPosition(index, total, dx, dy);
                if (bot->GetDistance2d(dx, dy) <= 2.0f)
                {
                    return false;
                }
                return MoveTo(NAXX_MAP_ID, dx, dy, bot->GetPositionZ(), false, false, false, false,
                              MovementPriority::MOVEMENT_COMBAT);
            }
            else if (botAI->IsTank(bot))
            {
                if (isOffTankForKT)
                {
                    std::vector<Unit*> guardians = helper.GetGuardians();
                    if (!guardians.empty())
                    {
                        Unit* pickup = helper.GetGuardianToPickup(bot);
                        if (pickup && pickup->GetVictim() != bot)
                        {
                            if (bot->GetDistance2d(pickup) > 6.0f)
                            {
                                return MoveNear(pickup, 4.0f, MovementPriority::MOVEMENT_COMBAT);
                            }
                            return false;
                        }
                        if (helper.AllGuardiansOnAssistTank(bot))
                        {
                            auto hold = helper.GetAssistTankHoldPosition();
                            if (bot->GetDistance2d(hold.first, hold.second) > 3.0f)
                            {
                                return MoveTo(NAXX_MAP_ID, hold.first, hold.second, bot->GetPositionZ(), false, false, false, false,
                                              MovementPriority::MOVEMENT_COMBAT);
                            }
                        }
                        return false;
                    }
                }

                return false;
            }
        }
        else
        {
            float dx, dy;
            float angle;
            if (!botAI->IsRanged(bot))
            {
                angle = shadow_fissure->GetAngle(helper.center.first, helper.center.second);
            }
            else
            {
                angle = bot->GetAngle(shadow_fissure) + M_PI;
            }
            dx = shadow_fissure->GetPositionX() + cos(angle) * 10.0f;
            dy = shadow_fissure->GetPositionY() + sin(angle) * 10.0f;
            helper.ClampToRoom(dx, dy);
            return MoveTo(NAXX_MAP_ID, dx, dy, bot->GetPositionZ(), false, false, false, false, MovementPriority::MOVEMENT_COMBAT);
        }
    }
    return false;
}