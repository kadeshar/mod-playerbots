// RaidOnyxiaActions.cpp
#include "RaidOnyxiaActions.h"

#include <RaidOnyxiaTriggers.h>
#include <RtiTargetValue.h>

#include "GenericSpellActions.h"
#include "LastMovementValue.h"
#include "MovementActions.h"
#include "Playerbots.h"
#include "PositionAction.h"

const std::string REMOVE_STRATEGY_CHAR = "-";

bool RaidOnyxiaMoveToSideAction::Execute(Event event)
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "onyxia");
    if (!boss)
        return false;

    float angleToBot = boss->GetAngle(bot);
    float bossFacing = boss->GetOrientation();
    float diff = fabs(angleToBot - bossFacing);
    if (diff > M_PI)
        diff = 2 * M_PI - diff;

    float distance = bot->GetDistance(boss);

    // Too close (30 yards) and either in front or behind
    if (distance <= 30.0f && (diff < M_PI / 4 || diff > 3 * M_PI / 4))
    {
        float offsetAngle = bossFacing + M_PI_2;  // 90° to the right
        float offsetDist = 15.0f;

        float sideX = boss->GetPositionX() + offsetDist * cos(offsetAngle);
        float sideY = boss->GetPositionY() + offsetDist * sin(offsetAngle);

        // bot->Yell("Too close to front or tail — moving to side of Onyxia!", LANG_UNIVERSAL);
        return MoveTo(boss->GetMapId(), sideX, sideY, boss->GetPositionZ(), false, false, false, false,
                      MovementPriority::MOVEMENT_COMBAT);
    }

    return false;
}

bool RaidOnyxiaSpreadOutAction::Execute(Event event)
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "onyxia");

    if (!boss)
        return false;

    Player* target = boss->GetCurrentSpell(CURRENT_GENERIC_SPELL)->m_targets.GetUnitTarget()->ToPlayer();
    if (target != bot)
        return false;

    // bot->Yell("Spreading out — I'm the Fireball target!", LANG_UNIVERSAL);
    return MoveFromGroup(9.0f);  // move 9 yards
}

bool RaidOnyxiaMoveToSafeZoneAction::Execute(Event event)
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "onyxia");
    if (!boss)
        return false;

    Spell* currentSpell = boss->GetCurrentSpell(CURRENT_GENERIC_SPELL);
    if (!currentSpell)
        return false;

    uint32 spellId = currentSpell->m_spellInfo->Id;

    std::vector<SafeZone> safeZones = GetSafeZonesForBreath(spellId);
    if (safeZones.empty())
        return false;

    // Find closest safe zone
    SafeZone* bestZone = nullptr;
    float bestDist = std::numeric_limits<float>::max();

    for (auto& zone : safeZones)
    {
        float dist = bot->GetExactDist2d(zone.pos.GetPositionX(), zone.pos.GetPositionY());
        if (dist < bestDist)
        {
            bestDist = dist;
            bestZone = &zone;
        }
    }

    if (!bestZone)
        return false;

    if (bot->IsWithinDist2d(bestZone->pos.GetPositionX(), bestZone->pos.GetPositionY(), bestZone->radius))
        return false;  // Already safe

    // bot->Yell("Moving to Safe Zone!", LANG_UNIVERSAL);
    return MoveTo(bot->GetMapId(), bestZone->pos.GetPositionX(), bestZone->pos.GetPositionY(), bot->GetPositionZ(),
                  false, false, false, false, MovementPriority::MOVEMENT_COMBAT);
}

bool RaidOnyxiaKillWhelpsAction::Execute(Event event)
{
    Unit* currentTarget = AI_VALUE(Unit*, "current target");
    // If already attacking a whelp, don't swap targets
    if (currentTarget && currentTarget->GetEntry() == NPC_ONYXIAN_WHELP)
    {
        return false;
    }
    GuidVector targets = AI_VALUE(GuidVector, "possible targets");
    for (ObjectGuid guid : targets)
    {
        Creature* unit = botAI->GetCreature(guid);
        if (!unit || !unit->IsAlive() || !unit->IsInWorld())
            continue;

        if (unit->GetEntry() == NPC_ONYXIAN_WHELP)  // Onyxia Whelp
        {
            // bot->Yell("Attacking Whelps!", LANG_UNIVERSAL);
            return Attack(unit);
        }
    }
    return false;
}

bool OnyxiaAvoidEggsAction::Execute(Event event)
{
    Position botPos = Position(bot->GetPositionX(), bot->GetPositionY(), bot->GetPositionZ());

    float x, y;

    // get safe zone slightly away from eggs (Can this be dynamic?)
    if (botPos.GetExactDist2d(-36.0f, -164.0f) <= 5.0f)
    {
        x = -10.0f;
        y = -180.0f;
    }
    else if (botPos.GetExactDist2d(-34.0f, -262.0f) <= 5.0f)
    {
        x = -16.0f;
        y = -250.0f;
    }
    else
    {
        return false;  // Not in danger zone
    }

    // bot->Yell("Too close to eggs — backing off!", LANG_UNIVERSAL);

    return MoveTo(bot->GetMapId(), x, y, bot->GetPositionZ(), false, false, false, false,
                  MovementPriority::MOVEMENT_COMBAT);
}


bool OnyxiaMarkTargetAction::Execute(Event event)
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "onyxia");
    if (!boss)
    {
        return false;
    }

    std::string currentRtiMark = AI_VALUE(std::string, "rti");
    if (currentRtiMark != "cross" && (botAI->IsRanged(bot) || boss->HealthBelowPct(40.0f)))
    {
        botAI->GetAiObjectContext()->GetValue<std::string>("rti")->Set("cross");
    }

    if (botAI->IsMelee(bot) && currentRtiMark != "skull" && !boss->HealthBelowPct(40.0f))
    {
        botAI->GetAiObjectContext()->GetValue<std::string>("rti")->Set("skull");
        botAI->GetAiObjectContext()->GetValue<std::string>("rti cc")->Set("cross");
    }

    // Main bot tank need to mark targets
    if (!botAI->IsBotMainTank(bot))
    {
        return true;
    }

    Group* group = bot->GetGroup();
    if (!group)
    {
        return false;
    }

    // Onyxia need to be mark
    ObjectGuid currentCrossTarget = group->GetTargetIcon(RtiTargetValue::crossIndex);
    if (!currentCrossTarget || currentCrossTarget != boss->GetGUID())
    {
        group->SetTargetIcon(RtiTargetValue::crossIndex, bot->GetGUID(), boss->GetGUID());
        botAI->ChangeStrategy(REMOVE_STRATEGY_CHAR + "aoe", BotState::BOT_STATE_COMBAT);
    }

    // Add need to be mark
    ObjectGuid currentSkullTarget = group->GetTargetIcon(RtiTargetValue::skullIndex);

    if (currentSkullTarget)
    {
        Unit* currentSkullUnit = botAI->GetUnit(currentSkullTarget);
        if (currentSkullUnit && currentSkullUnit->IsAlive() && currentSkullUnit->IsInWorld() &&
            (currentSkullUnit->GetEntry() == NPC_ONYXIAN_WHELP ||
             currentSkullUnit->GetEntry() == NPC_ONYXIAN_LAIR_GUARD))  // Onyxia Whelp
        {
            return true;
        }
    }

    GuidVector targets = AI_VALUE(GuidVector, "possible targets");
    for (ObjectGuid guid : targets)
    {
        Creature* unit = botAI->GetCreature(guid);
        if (!unit || !unit->IsAlive() || !unit->IsInWorld())
            continue;

        if (unit->GetEntry() == NPC_ONYXIAN_WHELP)  // Onyxia Whelp
        {
            group->SetTargetIcon(RtiTargetValue::skullIndex, bot->GetGUID(), unit->GetGUID());
            return true;
        }
    }

    return true;
}

bool OnyxiaOnyxianLairGuardCastingAction::Execute(Event event)
{
    bool shouldRunAway = false;
    float distanceToRun = 0.0f;

    GuidVector targets = AI_VALUE(GuidVector, "possible targets");
    for (ObjectGuid guid : targets)
    {
        Creature* unit = botAI->GetCreature(guid);
        if (!unit || !unit->IsAlive() || !unit->IsInWorld() || unit->GetEntry() != 36561)
        {
            continue;
        }

        // Check if Onyxian Lair Guard is casting
        if (unit->HasUnitState(UNIT_STATE_CASTING))
        {
            Spell* currentSpell = unit->GetCurrentSpell(CURRENT_GENERIC_SPELL);

            if (!currentSpell)
            {
                continue;
            }

            int spellId = currentSpell->m_spellInfo->Id;

            // Blast nova
            float distance = bot->GetDistance2d(unit);
            if (spellId == SPELL_OLG_BLASTNOVA && distance < 15.0f && distanceToRun < 15.0f)
            {
                shouldRunAway = true;
                distanceToRun = 15.0f;
                continue;
            }

            // Ignite weapon
            if (spellId == SPELL_OLG_IGNITEWEAPON && distance < 5.0f && distanceToRun < 5.0f)
            {
                shouldRunAway = true;
                distanceToRun = 5.0f;
                continue;
            }
        }
    }

    if (!shouldRunAway)
    {
        return false;
    }

    MoveAwayFromCreatureAction moveAwayFromCreatureAction(botAI, "move away from creature", 36561, distanceToRun);
    return moveAwayFromCreatureAction.Execute(event);
}

bool OnyxiaBackToChamberAction::Execute(Event event)
{
    return MoveNear(bot->GetMapId(), ONYXIA_CHAMBER_MIDDLE.GetPositionX(), ONYXIA_CHAMBER_MIDDLE.GetPositionY(),
                    ONYXIA_CHAMBER_MIDDLE.GetPositionZ(), ONYXIA_CHAMBER_RADIUS - 5.0f,
                    MovementPriority::MOVEMENT_FORCED);
}

bool OnyxiaTankOnyxianLairGuardAction::Execute(Event event)
{
    Group* group = bot->GetGroup();
    if (!group)
    {
        return false;
    }

    OnyxiaTankOnyxianLairGuardTrigger onyxiaTankOnyxianLairGuardTrigger(botAI);

    if (onyxiaTankOnyxianLairGuardTrigger.HaveOnyxianLairGuardToAggro() &&
        !onyxiaTankOnyxianLairGuardTrigger.CurrentTargetIsMarkedOnyxianLairGuard())
    {
        ObjectGuid currentSkullTarget = group->GetTargetIcon(RtiTargetValue::skullIndex);
        if (currentSkullTarget)
        {
            Unit* currentSkullUnit = botAI->GetUnit(currentSkullTarget);
            botAI->DoSpecificAction("attack rti target");
            return bot->Attack(currentSkullUnit, true);
        }
    }

    Unit* guard = onyxiaTankOnyxianLairGuardTrigger.GetOnyxianLairGuardToTank();

    if (guard)
    {
        if (AI_VALUE(std::string, "rti") != "skull")
        {
            context->GetValue<std::string>("rti")->Set("skull");
        }

        Group* group = bot->GetGroup();
        if (!group)
        {
            return false;
        }

        group->SetTargetIcon(RtiTargetValue::skullIndex, bot->GetGUID(), guard->GetGUID());
        bot->SetTarget(guard->GetGUID());
        return true;
    }

    return MoveTo(bot->GetMapId(), ONYXIA_OLG_ENTRANCE.GetPositionX(), ONYXIA_OLG_ENTRANCE.GetPositionY(),
                  ONYXIA_OLG_ENTRANCE.GetPositionZ(), false, false, false, false, MovementPriority::MOVEMENT_FORCED);
}

bool OnyxiaHealerForMainTankAction::Execute(Event event)
{
    return MoveTo(bot->GetMapId(), ONYXIA_OLG_HEALER_SPOT.GetPositionX(), ONYXIA_OLG_HEALER_SPOT.GetPositionY(),
                  ONYXIA_OLG_HEALER_SPOT.GetPositionZ(), false, false, false, false, MovementPriority::MOVEMENT_FORCED);
}
