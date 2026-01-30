#include "RaidNaxxActions.h"

#include "Playerbots.h"
#include "PlayerbotAI.h"

#include <cmath>

namespace
{
constexpr float kTankZ = 273.5857f;
constexpr float kMeleeBehindDistance = 3.0f;
constexpr float kCasterBehindDistance = 12.0f;
constexpr float kCasterSideOffset = 6.0f;
constexpr float kHealerBehindDistance = 18.0f;

uint8 GetCasterSideIndex(Player* bot)
{
    if (Group* group = bot->GetGroup())
    {
        return bot->GetSubGroup() % 2;
    }
    return 0;
}

float GetSideSign(uint8 sideIndex)
{
    return sideIndex == 0 ? -1.0f : 1.0f;
}

void GetBossBehindPosition(Unit* boss, float distance, float& outX, float& outY, float& outZ)
{
    float orientation = boss->GetOrientation();
    float dirX = -std::cos(orientation);
    float dirY = -std::sin(orientation);
    outX = boss->GetPositionX() + dirX * distance;
    outY = boss->GetPositionY() + dirY * distance;
    outZ = boss->GetPositionZ();
}

void GetCasterGroupPosition(Unit* boss, uint8 sideIndex, float& outX, float& outY, float& outZ)
{
    float orientation = boss->GetOrientation();
    float backX = -std::cos(orientation);
    float backY = -std::sin(orientation);
    float rightX = -backY;
    float rightY = backX;
    float sideSign = GetSideSign(sideIndex);
    outX = boss->GetPositionX() + backX * kCasterBehindDistance + rightX * kCasterSideOffset * sideSign;
    outY = boss->GetPositionY() + backY * kCasterBehindDistance + rightY * kCasterSideOffset * sideSign;
    outZ = boss->GetPositionZ();
}

void GetHealerPosition(Unit* boss, float& outX, float& outY, float& outZ)
{
    GetBossBehindPosition(boss, kHealerBehindDistance, outX, outY, outZ);
}

uint8 GetSporeSideIndex(Unit* boss, Unit* spore)
{
    float orientation = boss->GetOrientation();
    float backX = -std::cos(orientation);
    float backY = -std::sin(orientation);
    float rightX = -backY;
    float rightY = backX;
    float dx = spore->GetPositionX() - boss->GetPositionX();
    float dy = spore->GetPositionY() - boss->GetPositionY();
    float side = dx * rightX + dy * rightY;
    return side < 0.0f ? 0 : 1;
}

uint8 GetCasterCountForSide(Player* bot, PlayerbotAI* botAI, uint8 sideIndex)
{
    if (!botAI)
    {
        return 0;
    }
    Group* group = bot->GetGroup();
    if (!group)
    {
        return 0;
    }
    uint8 count = 0;
    for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
    {
        Player* member = ref->GetSource();
        if (!member || !member->IsAlive())
        {
            continue;
        }
        if (member->GetSubGroup() % 2 != sideIndex)
        {
            continue;
        }
        if (bot->GetGUID() != member->GetGUID() && bot->GetMapId() != member->GetMapId())
        {
            continue;
        }
        if (!botAI->IsRanged(member) || botAI->IsHeal(member))
        {
            continue;
        }
        ++count;
    }
    return count;
}

bool IsSporeRunner(Player* bot, PlayerbotAI* botAI, uint8 sideIndex)
{
    if (!botAI)
    {
        return false;
    }
    Group* group = bot->GetGroup();
    if (!group)
    {
        return false;
    }
    ObjectGuid selected = ObjectGuid::Empty;
    for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
    {
        Player* member = ref->GetSource();
        if (!member || !member->IsAlive())
        {
            continue;
        }
        if (member->GetSubGroup() % 2 != sideIndex)
        {
            continue;
        }
        if (bot->GetGUID() != member->GetGUID() && bot->GetMapId() != member->GetMapId())
        {
            continue;
        }
        if (!botAI->IsRanged(member) || botAI->IsHeal(member))
        {
            continue;
        }
        if (selected.IsEmpty() || member->GetGUID() < selected)
        {
            selected = member->GetGUID();
        }
    }
    return !selected.IsEmpty() && bot->GetGUID() == selected;
}
} // namespace

bool LoathebPositionAction::Execute(Event event)
{
    if (!helper.UpdateBossAI())
    {
        return false;
    }
    Unit* boss = helper.GetBoss();
    if (!boss)
    {
        return false;
    }
    if (botAI->IsTank(bot))
    {
        if (AI_VALUE2(bool, "has aggro", "boss target"))
        {
            return MoveTo(533, helper.mainTankPos.first, helper.mainTankPos.second, kTankZ, false, false, false, false,
                          MovementPriority::MOVEMENT_COMBAT);
        }
    }
    else
    {
        float targetX = 0.0f;
        float targetY = 0.0f;
        float targetZ = boss->GetPositionZ();

        if (botAI->IsHeal(bot))
        {
            GetHealerPosition(boss, targetX, targetY, targetZ);
        }
        else if (botAI->IsMelee(bot))
        {
            GetBossBehindPosition(boss, kMeleeBehindDistance, targetX, targetY, targetZ);
        }
        else
        {
            uint8 sideIndex = GetCasterSideIndex(bot);
            uint8 otherSideIndex = sideIndex == 0 ? 1 : 0;
            bool hasOtherSideCasters = GetCasterCountForSide(bot, botAI, otherSideIndex) > 0;
            bool isSporeRunner = IsSporeRunner(bot, botAI, sideIndex);
            Unit* sporeTarget = nullptr;
            if (isSporeRunner)
            {
                GuidVector attackers = context->GetValue<GuidVector>("attackers")->Get();
                for (ObjectGuid const& guid : attackers)
                {
                    Unit* unit = botAI->GetUnit(guid);
                    if (!unit || !unit->IsAlive())
                    {
                        continue;
                    }
                    if (botAI->EqualLowercaseName(unit->GetName(), "spore") &&
                        (!hasOtherSideCasters || GetSporeSideIndex(boss, unit) == sideIndex))
                    {
                        if (!sporeTarget || bot->GetDistance(unit) < bot->GetDistance(sporeTarget))
                        {
                            sporeTarget = unit;
                        }
                    }
                }
            }

            if (sporeTarget)
            {
                targetX = sporeTarget->GetPositionX();
                targetY = sporeTarget->GetPositionY();
                targetZ = sporeTarget->GetPositionZ();
            }
            else
            {
                GetCasterGroupPosition(boss, sideIndex, targetX, targetY, targetZ);
            }
        }

        return MoveInside(bot->GetMapId(), targetX, targetY, targetZ, 1.5f, MovementPriority::MOVEMENT_COMBAT);
    }
    return false;
}

bool LoathebChooseTargetAction::Execute(Event event)
{
    if (!helper.UpdateBossAI())
    {
        return false;
    }
    Unit* boss = helper.GetBoss();
    if (!boss)
    {
        return false;
    }
    GuidVector attackers = context->GetValue<GuidVector>("attackers")->Get();
    Unit* target = nullptr;
    Unit* target_boss = nullptr;
    Unit* target_spore = nullptr;
    bool isSporeRunner = false;
    uint8 sideIndex = GetCasterSideIndex(bot);
    uint8 otherSideIndex = sideIndex == 0 ? 1 : 0;
    bool hasOtherSideCasters = false;
    if (botAI->IsRanged(bot) && !botAI->IsHeal(bot))
    {
        hasOtherSideCasters = GetCasterCountForSide(bot, botAI, otherSideIndex) > 0;
        isSporeRunner = IsSporeRunner(bot, botAI, sideIndex);
    }
    for (auto i = attackers.begin(); i != attackers.end(); ++i)
    {
        Unit* unit = botAI->GetUnit(*i);
        if (!unit)
            continue;
        if (!unit->IsAlive())
        {
            continue;
        }
        if (botAI->EqualLowercaseName(unit->GetName(), "spore"))
        {
            if (isSporeRunner)
            {
                uint8 sporeSide = GetSporeSideIndex(boss, unit);
                if (!hasOtherSideCasters || sporeSide == sideIndex)
                {
                    if (!target_spore || bot->GetDistance(unit) < bot->GetDistance(target_spore))
                    {
                        target_spore = unit;
                    }
                }
            }
        }
        if (botAI->EqualLowercaseName(unit->GetName(), "loatheb"))
        {
            target_boss = unit;
        }
    }
    if (target_spore)
    {
        target = target_spore;
    }
    else
    {
        target = target_boss;
    }
    if (!target || context->GetValue<Unit*>("current target")->Get() == target)
    {
        return false;
    }
    return Attack(target);
}