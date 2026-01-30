#include "RaidNaxxActions.h"

#include "Item.h"
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
constexpr float kSporeOnHealerRadius = 10.0f;
constexpr float kSporeCleanupMaxRange = 35.0f;     // keep it strictly "no chase"
constexpr float kSporeRunnerNearRange = 15.0f;     // if runner is already near, don't steal the job

uint8 GetCasterSideIndex(Player* bot)
{
    if (Group* group = bot->GetGroup())
    {
        return bot->GetSubGroup() % 2;
    }
    return 0;
}

ObjectGuid SelectRangedDpsLeaderForSide(Player* bot, PlayerbotAI* botAI, uint8 sideIndex)
{
    if (!botAI)
        return ObjectGuid::Empty;

    Group* group = bot->GetGroup();
    if (!group)
        return ObjectGuid::Empty;

    ObjectGuid selected = ObjectGuid::Empty;
    for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
    {
        Player* member = ref->GetSource();
        if (!member || !member->IsAlive())
            continue;

        if (member->GetSubGroup() % 2 != sideIndex)
            continue;

        if (bot->GetGUID() != member->GetGUID() && bot->GetMapId() != member->GetMapId())
            continue;

        if (!botAI->IsRanged(member) || botAI->IsHeal(member))
            continue;

        if (selected.IsEmpty() || member->GetGUID() < selected)
            selected = member->GetGUID();
    }

    return selected;
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

float GetDistance2d(float x1, float y1, float x2, float y2)
{
    float dx = x1 - x2;
    float dy = y1 - y2;
    return std::sqrt(dx * dx + dy * dy);
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
    ObjectGuid selected = SelectRangedDpsLeaderForSide(bot, botAI, sideIndex);
    return !selected.IsEmpty() && bot->GetGUID() == selected;
}

bool IsSporeInHealerPack(Player* bot, PlayerbotAI* botAI, Unit* spore)
{
    if (!botAI || !spore)
        return false;

    Group* group = bot->GetGroup();
    if (!group)
        return false;

    for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
    {
        Player* member = ref->GetSource();
        if (!member || !member->IsAlive())
            continue;

        if (bot->GetGUID() != member->GetGUID() && bot->GetMapId() != member->GetMapId())
            continue;

        if (!botAI->IsHeal(member))
            continue;

        if (member->GetDistance(spore) <= kSporeOnHealerRadius)
            return true;
    }

    return false;
}

bool HasWandEquipped(Player* bot)
{
    if (!bot)
        return false;

    Item* const ranged = bot->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_RANGED);
    if (!ranged || !ranged->GetTemplate())
        return false;

    return ranged->GetTemplate()->SubClass == ITEM_SUBCLASS_WEAPON_WAND;
}

ObjectGuid SelectSporeCleanupCaster(Player* bot, PlayerbotAI* botAI, Unit* spore, ObjectGuid sporeRunnerGuid)
{
    if (!botAI || !spore)
        return ObjectGuid::Empty;

    Group* group = bot->GetGroup();
    if (!group)
        return ObjectGuid::Empty;

    ObjectGuid selected = ObjectGuid::Empty;
    float bestDist = 0.0f;

    for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
    {
        Player* member = ref->GetSource();
        if (!member || !member->IsAlive())
            continue;

        if (member->GetGUID() == sporeRunnerGuid)
            continue;

        if (bot->GetGUID() != member->GetGUID() && bot->GetMapId() != member->GetMapId())
            continue;

        // We want a true caster cleanup (no healers, no hunters), and no movement commitment.
        if (!botAI->IsCaster(member) || botAI->IsHeal(member) || member->getClass() == CLASS_HUNTER)
            continue;

        float const dist = member->GetDistance(spore);
        if (dist > kSporeCleanupMaxRange)
            continue;

        // Pick the nearest eligible caster (stable tie-breaker by GUID).
        if (selected.IsEmpty() || dist < bestDist || (std::abs(dist - bestDist) < 0.001f && member->GetGUID() < selected))
        {
            selected = member->GetGUID();
            bestDist = dist;
        }
    }

    return selected;
}

void DoSafeInstantRangedHit(PlayerbotAI* botAI, Player* bot, Unit* target)
{
    if (!botAI || !bot || !target)
        return;

    // Prefer wand "shoot" if available: lowest commitment.
    if (HasWandEquipped(bot))
    {
        botAI->DoSpecificAction("shoot", Event(), true);
        return;
    }

    // Otherwise, use a single instant ranged spell that is generally safe and low-commitment.
    switch (bot->getClass())
    {
        case CLASS_WARLOCK:
            botAI->DoSpecificAction("corruption", Event(), true);
            break;
        case CLASS_PRIEST:
            botAI->DoSpecificAction("shadow word: pain", Event(), true);
            break;
        case CLASS_DRUID:
            botAI->DoSpecificAction("moonfire", Event(), true);
            break;
        case CLASS_SHAMAN:
            botAI->DoSpecificAction("flame shock", Event(), true);
            break;
        case CLASS_MAGE:
            botAI->DoSpecificAction("fire blast", Event(), true);
            break;
        default:
            // Fallback: if we are considered a caster but we don't have a known instant here, do nothing.
            break;
    }
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
                GetCasterGroupPosition(boss, sideIndex, targetX, targetY, targetZ);
                float groupDistance = GetDistance2d(targetX, targetY, sporeTarget->GetPositionX(), sporeTarget->GetPositionY());
                if (groupDistance > 6.0f && bot->GetDistance(sporeTarget) > 5.0f)
                {
                    targetX = sporeTarget->GetPositionX();
                    targetY = sporeTarget->GetPositionY();
                    targetZ = sporeTarget->GetPositionZ();
                }
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
    Unit* target_spore_any = nullptr;
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
            // Track a consistent spore reference for everyone (closest to boss).
            if (!target_spore_any || boss->GetDistance(unit) < boss->GetDistance(target_spore_any))
                target_spore_any = unit;

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

    // --- Spore cleanup: if a spore sticks on the healer pack and the spore-runner isn't close, assign ONE caster to "tap" it safely. ---
    if (target_spore_any && target_boss && IsSporeInHealerPack(bot, botAI, target_spore_any))
    {
        // Identify the spore runner for the spore's side (if any).
        uint8 const sporeSide = GetSporeSideIndex(boss, target_spore_any);
        ObjectGuid const sporeRunnerGuid = SelectRangedDpsLeaderForSide(bot, botAI, sporeSide);
        Player* sporeRunner = nullptr;
        if (!sporeRunnerGuid.IsEmpty())
        {
            if (Group* group = bot->GetGroup())
            {
                sporeRunner = ObjectAccessor::FindPlayer(sporeRunnerGuid);
                if (sporeRunner && (bot->GetGUID() != sporeRunner->GetGUID() && bot->GetMapId() != sporeRunner->GetMapId()))
                    sporeRunner = nullptr;
            }
        }

        bool const runnerIsNear = (sporeRunner && sporeRunner->IsAlive() && sporeRunner->GetDistance(target_spore_any) <= kSporeRunnerNearRange);
        if (!runnerIsNear)
        {
            ObjectGuid const cleanupGuid = SelectSporeCleanupCaster(bot, botAI, target_spore_any, sporeRunnerGuid);
            if (!cleanupGuid.IsEmpty() && bot->GetGUID() == cleanupGuid)
            {
                // No chase: only do it if we're already in a safe ranged window and LoS.
                if (bot->IsWithinDistInMap(target_spore_any, kSporeCleanupMaxRange) && bot->IsWithinLOSInMap(target_spore_any))
                {
                    // Temporarily switch target -> apply ONE instant ranged hit -> immediately go back to boss.
                    Attack(target_spore_any);
                    DoSafeInstantRangedHit(botAI, bot, target_spore_any);
                    Attack(target_boss);
                    return true;
                }
            }
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

    if (!target)
        return false;

    // Spore runner: keep a "light" ranged attack running to reliably secure spore aggro.
    // This is intentionally re-tried even if target didn't change (e.g. while moving into range/LoS).
    if (target_spore && isSporeRunner && target == target_spore)
    {
        if (bot->getClass() == CLASS_HUNTER)
            botAI->DoSpecificAction("auto shot", Event(), true);
        else if (botAI->IsCaster(bot) && HasWandEquipped(bot))
            botAI->DoSpecificAction("shoot", Event(), true);
    }

    if (context->GetValue<Unit*>("current target")->Get() == target)
        return false;

    bool attacked = Attack(target);

    // Re-try after target switch so the spore runner starts the ranged attack immediately.
    if (target_spore && isSporeRunner && target == target_spore)
    {
        if (bot->getClass() == CLASS_HUNTER)
            botAI->DoSpecificAction("auto shot", Event(), true);
        else if (botAI->IsCaster(bot) && HasWandEquipped(bot))
            botAI->DoSpecificAction("shoot", Event(), true);
    }

    return attacked;
}