#include "RaidNaxxActions.h"

#include <algorithm>
#include <limits>

#include "PlayerbotAIConfig.h"
#include "Playerbots.h"
#include "RaidNaxxBossHelper.h"
#include "RaidNaxxSpellIds.h"

bool SapphironGroundPositionAction::Execute(Event event)
{
    if (!helper.UpdateBossAI())
    {
        return false;
    }
    if (botAI->IsHeal(bot) && helper.HasLifeDrainInGroup() && !botAI->HasStrategy("cure", BOT_STATE_COMBAT))
    {
        botAI->ChangeStrategy("cure", BOT_STATE_COMBAT);
    }
    if (botAI->IsMainTank(bot))
    {
        if (AI_VALUE2(bool, "has aggro", "current target"))
        {
            return MoveTo(NAXX_MAP_ID, helper.mainTankPos.first, helper.mainTankPos.second, helper.GENERIC_HEIGHT, false, false, false,
                          false, MovementPriority::MOVEMENT_COMBAT);
        }
        return false;
    }
    Unit* boss = AI_VALUE2(Unit*, "find target", "sapphiron");
    if (boss && helper.IsPhaseGround())
    {
        bool needsSideStack = boss->isInFront(bot) || boss->isInBack(bot);
        if (!needsSideStack)
        {
            needsSideStack = NaxxSpellIds::HasAnyAura(botAI, bot, {NaxxSpellIds::LifeDrain}) || botAI->HasAura("life drain", bot);
        }
        if (needsSideStack)
        {
            float distance;
            if (botAI->IsRanged(bot) || botAI->IsHeal(bot) ||
                NaxxSpellIds::HasAnyAura(botAI, bot, {NaxxSpellIds::LifeDrain}) || botAI->HasAura("life drain", bot))
            {
                distance = 30.0f;
            }
            else
            {
                distance = 5.0f;
            }
            float angle = boss->GetOrientation() + M_PI / 2;
            float posX = boss->GetPositionX() + cos(angle) * distance;
            float posY = boss->GetPositionY() + sin(angle) * distance;
            if (MoveTo(NAXX_MAP_ID, posX, posY, helper.GENERIC_HEIGHT, false, false, false, false, MovementPriority::MOVEMENT_COMBAT))
            {
                return true;
            }
            return MoveInside(NAXX_MAP_ID, posX, posY, helper.GENERIC_HEIGHT, 2.0f, MovementPriority::MOVEMENT_COMBAT);
        }
    }
    if (helper.JustLanded())
    {
        uint32 index = botAI->GetGroupSlotIndex(bot);
        float start_angle = 0.85 * M_PI;
        float offset_angle = M_PI * 0.02 * index;
        float angle = start_angle + offset_angle;
        float distance;
        if (botAI->IsRanged(bot))
        {
            distance = 35.0f;
        }
        else if (botAI->IsHeal(bot))
        {
            distance = 30.0f;
        }
        else
        {
            distance = 5.0f;
        }
        float posX = helper.center.first + cos(angle) * distance;
        float posY = helper.center.second + sin(angle) * distance;
        if (MoveTo(NAXX_MAP_ID, posX, posY, helper.GENERIC_HEIGHT, false, false, false, false, MovementPriority::MOVEMENT_COMBAT))
        {
            return true;
        }
        return MoveInside(NAXX_MAP_ID, posX, posY, helper.GENERIC_HEIGHT, 2.0f, MovementPriority::MOVEMENT_COMBAT);
    }
    else
    {
        std::vector<float> dest;
        if (helper.FindPosToAvoidChill(dest))
        {
            return MoveTo(NAXX_MAP_ID, dest[0], dest[1], dest[2], false, false, false, false, MovementPriority::MOVEMENT_COMBAT);
        }
    }
    return false;
}

bool SapphironFlightPositionAction::Execute(Event event)
{
    if (!helper.UpdateBossAI())
    {
        return false;
    }
    if (botAI->IsHeal(bot) && helper.HasLifeDrainInGroup() && !botAI->HasStrategy("cure", BOT_STATE_COMBAT))
    {
        botAI->ChangeStrategy("cure", BOT_STATE_COMBAT);
    }
    if (NaxxSpellIds::HasAnyAura(botAI, bot, {NaxxSpellIds::Icebolt10, NaxxSpellIds::Icebolt25}) ||
        botAI->HasAura("icebolt", bot, false, false, -1, true))
    {
        return false;
    }
    if (helper.WaitForExplosion())
    {
        bool inShelter = MoveToNearestIcebolt();
        if (inShelter && botAI->IsHeal(bot))
        {
            return false;
        }
        return inShelter;
    }
    else
    {
        std::vector<float> dest;
        if (helper.FindPosToAvoidChill(dest))
        {
            return MoveTo(NAXX_MAP_ID, dest[0], dest[1], dest[2], false, false, false, false, MovementPriority::MOVEMENT_COMBAT);
        }
    }
    return false;
}

bool SapphironFlightPositionAction::MoveToNearestIcebolt()
{
    Group* group = bot->GetGroup();
    if (!group)
    {
        return false;
    }
    Unit* boss = AI_VALUE2(Unit*, "find target", "sapphiron");
    if (!boss)
    {
        return false;
    }
    Player* playerWithIcebolt = nullptr;
    float minDistance = std::numeric_limits<float>::max();
    for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
    {
        Player* member = ref->GetSource();
        if (!member || !member->IsAlive())
        {
            continue;
        }
        if (NaxxSpellIds::HasAnyAura(botAI, member, {NaxxSpellIds::Icebolt10, NaxxSpellIds::Icebolt25}) ||
            botAI->HasAura("icebolt", member, false, false, -1, true))
        {
            if (!playerWithIcebolt || minDistance > bot->GetDistance(member))
            {
                playerWithIcebolt = member;
                minDistance = bot->GetDistance(member);
            }
        }
    }
    if (playerWithIcebolt)
    {
        constexpr float shelterDistance = 9.0f;
        constexpr float shelterEpsilon = 0.35f;
        constexpr float lateralOffset = 1.5f;
        float angle = boss->GetAngle(playerWithIcebolt);
        float posX = playerWithIcebolt->GetPositionX() + cos(angle) * shelterDistance;
        float posY = playerWithIcebolt->GetPositionY() + sin(angle) * shelterDistance;
        float posZ = playerWithIcebolt->GetPositionZ();
        int32 slotIndex = botAI->GetGroupSlotIndex(bot);
        float offsetSign = (slotIndex % 2 == 0) ? 1.0f : -1.0f;
        float offsetAngle = angle + (M_PI / 2.0f);
        float offsetX = cos(offsetAngle) * lateralOffset * offsetSign;
        float offsetY = sin(offsetAngle) * lateralOffset * offsetSign;
        float candidateX = posX + offsetX;
        float candidateY = posY + offsetY;
        float bossX = boss->GetPositionX();
        float bossY = boss->GetPositionY();
        float lineDx = candidateX - bossX;
        float lineDy = candidateY - bossY;
        float lineLen = sqrt(lineDx * lineDx + lineDy * lineDy);
        if (lineLen > 0.1f)
        {
            float relX = playerWithIcebolt->GetPositionX() - bossX;
            float relY = playerWithIcebolt->GetPositionY() - bossY;
            float proj = (relX * lineDx + relY * lineDy) / lineLen;
            float clamped = std::max(0.0f, std::min(lineLen, proj));
            float closestX = bossX + (lineDx / lineLen) * clamped;
            float closestY = bossY + (lineDy / lineLen) * clamped;
            float distToLine = playerWithIcebolt->GetDistance2d(closestX, closestY);
            if (distToLine <= 2.0f && playerWithIcebolt->IsWithinDist2d(candidateX, candidateY, 10.0f))
            {
                posX = candidateX;
                posY = candidateY;
            }
        }
        float bossDist = boss->GetDistance2d(bot);
        float iceboltDist = boss->GetDistance2d(playerWithIcebolt);
        if (bossDist <= iceboltDist + 0.5f)
        {
            posX = playerWithIcebolt->GetPositionX() + cos(angle) * (shelterDistance + 1.5f);
            posY = playerWithIcebolt->GetPositionY() + sin(angle) * (shelterDistance + 1.5f);
        }
        float distToLosPos = bot->GetDistance2d(posX, posY);
        if (distToLosPos > shelterEpsilon)
        {
            return MoveTo(NAXX_MAP_ID, posX, posY, posZ, false, false, false, true,
                          MovementPriority::MOVEMENT_FORCED);
        }
        if (!playerWithIcebolt->IsInBetween(boss, bot, 1.5f))
        {
            return MoveTo(NAXX_MAP_ID, posX, posY, posZ, false, false, false, true,
                          MovementPriority::MOVEMENT_FORCED);
        }
        if (botAI->IsHeal(bot))
        {
            bot->StopMoving();
        }
        return true;
    }
    return false;
}