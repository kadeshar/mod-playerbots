#include "RaidOnyxiaTriggers.h"

#include <RtiTargetValue.h>

#include "GenericTriggers.h"
#include "ObjectAccessor.h"
#include "PlayerbotAI.h"
#include "Playerbots.h"
#include "NearestNpcsValue.h"

bool OnyxiaDeepBreathTrigger::IsActive()
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "onyxia");
    if (!boss || !boss->HasUnitState(UNIT_STATE_CASTING))
        return false;

    // Check if Onyxia is casting
    Spell* currentSpell = boss->GetCurrentSpell(CURRENT_GENERIC_SPELL);

    if (!currentSpell)
        return false;

    uint32 spellId = currentSpell->m_spellInfo->Id;

    if (spellId == SPELL_BREATH_N_TO_S ||    // North to South
        spellId == SPELL_BREATH_S_TO_N ||    // South to North
        spellId == SPELL_BREATH_E_TO_W ||    // East to West
        spellId == SPELL_BREATH_W_TO_E ||    // West to East
        spellId == SPELL_BREATH_SE_TO_NW ||  // Southeast to Northwest
        spellId == SPELL_BREATH_NW_TO_SE ||  // Northwest to Southeast
        spellId == SPELL_BREATH_SW_TO_NE ||  // Southwest to Northeast
        spellId == SPELL_BREATH_NE_TO_SW     // Northeast to Southwest
    )
    {
        return true;
    }

    return false;
}

bool OnyxiaNearTailTrigger::IsActive()
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "onyxia");
    if (!boss || botAI->IsTank(bot))
        return false;

    // Skip if Onyxia is in air or transitioning
    if (!boss->IsInCombat() || boss->IsFlying() || !boss->GetVictim())
        return false;

    return true;
}

bool RaidOnyxiaFireballSplashTrigger::IsActive()
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "onyxia");
    if (!boss || !boss->HasUnitState(UNIT_STATE_CASTING))
        return false;

    // Check if Onyxia is casting Fireball
    Spell* currentSpell = boss->GetCurrentSpell(CURRENT_GENERIC_SPELL);
    if (!currentSpell || currentSpell->m_spellInfo->Id != SPELL_FIREBALL)  // 18392 is the classic Fireball ID
        return false;

    GuidVector nearbyUnits = AI_VALUE(GuidVector, "nearest friendly players");

    for (ObjectGuid guid : nearbyUnits)
    {
        Unit* unit = botAI->GetUnit(guid);
        if (!unit || unit == bot || !unit->IsAlive())
            continue;

        if (bot->GetDistance(unit) < 8.0f)
            return true;
    }

    return false;
}

bool RaidOnyxiaWhelpsSpawnTrigger::IsActive()
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "onyxia");
    if (!boss)
        return false;

    return botAI->IsMelee(bot) && boss->IsFlying();  // DPS + Tanks only
}

bool OnyxiaAvoidEggsTrigger::IsActive()
{
    Position botPos = Position(bot->GetPositionX(), bot->GetPositionY(), bot->GetPositionZ());

    if (botPos.GetExactDist2d(-35.0f, -165.0f) <= 5.0f)
        return true;

    if (botPos.GetExactDist2d(-35.0f, -260.0f) <= 5.0f)
        return true;

    return false;
}


bool OnyxiaMarkTargetTrigger::IsActive()
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "onyxia");
    if (!IsOnyxiaFlying(boss))
    {
        return false;
    }

    if (botAI->IsHeal(bot))
    {
        return false;
    }

    // Bot have wrong rti mark
    std::string currentRtiMark = AI_VALUE(std::string, "rti");

    if (currentRtiMark != "cross" && (botAI->IsRanged(bot) || boss->HealthBelowPct(40.0f)))
    {
        return true;
    }

    if (botAI->IsMelee(bot) && currentRtiMark != "skull")
    {
        return true;
    }

    if (!botAI->IsTank(bot))
    {
        return false;
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
        return true;
    }

    // Add need to be mark
    ObjectGuid currentSkullTarget = group->GetTargetIcon(RtiTargetValue::skullIndex);

    if (currentSkullTarget)
    {
        Unit* currentSkullUnit = botAI->GetUnit(currentSkullTarget);
        if (currentSkullUnit && currentSkullUnit->IsAlive() && currentSkullUnit->IsInWorld() &&
            (currentSkullUnit->GetEntry() == NPC_ONYXIAN_WHELP ||
             currentSkullUnit->GetEntry() == NPC_ONYXIAN_LAIR_GUARD))
        {
            return false;
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
            return true;
        }
    }

    return false;
}

bool OnyxiaOnyxianLairGuardCastingTrigger::IsActive()
{
    if (!IsOnyxiaFight())
    {
        return false;
    }

    if (botAI->IsTank(bot))
    {
        return false;
    }

    GuidVector targets = AI_VALUE(GuidVector, "possible targets");
    for (ObjectGuid guid : targets)
    {
        Creature* unit = botAI->GetCreature(guid);
        if (!unit || !unit->IsAlive() || !unit->IsInWorld() || unit->GetEntry() != NPC_ONYXIAN_LAIR_GUARD)
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
            if (spellId == SPELL_OLG_BLASTNOVA && bot->GetDistance2d(unit) < 15.0f)
            {
                return true;
            }

            // Ignite weapon
            if (spellId == SPELL_OLG_IGNITEWEAPON && bot->GetDistance2d(unit) < 5.0f)
            {
                return true;
            }
        }
    }

    return false;
}

bool OnyxiaBackToChamberTrigger::IsActive()
{
    if (!IsOnyxiaFight())
    {
        return false;
    }

    return bot->GetDistance2d(ONYXIA_CHAMBER_MIDDLE.GetPositionX(), ONYXIA_CHAMBER_MIDDLE.GetPositionY()) >
           ONYXIA_CHAMBER_RADIUS;
}

bool OnyxiaTrigger::IsOnyxiaFight()
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "onyxia");
    if (!boss || !boss->IsAlive())
    {
        return false;
    }

    return true;
}

bool OnyxiaTrigger::IsOnyxiaFlying(Unit* boss)
{
    if (!boss)
    {
        boss = AI_VALUE2(Unit*, "find target", "onyxia");
    }

    if (!boss || !boss->IsAlive())
    {
        return false;
    }

    return boss->IsFlying();
}

bool OnyxiaTankOnyxianLairGuardTrigger::IsActive()
{
    if (!IsOnyxiaFlying(nullptr))
    {
        return false;
    }

    if (!botAI->IsBotMainTank(bot))
    {
        return false;
    }

    OnyxiaDeepBreathTrigger onyxiaDeepBreathTrigger(botAI);
    if (onyxiaDeepBreathTrigger.IsActive())
    {
        return false;
    }

    if (HaveOnyxianLairGuardToAggro())
    {
        if (!CurrentTargetIsMarkedOnyxianLairGuard())
        {
            return true;
        }

        return false;
    }

    if (GetOnyxianLairGuardToTank())
    {
        return true;
    }

    // Check that bot is too far away from Onyxian Lair Guard entrance
    return bot->GetDistance2d(ONYXIA_OLG_ENTRANCE.GetPositionX(), ONYXIA_OLG_ENTRANCE.GetPositionY()) > 5.0f;
}

bool OnyxiaTankOnyxianLairGuardTrigger::HaveOnyxianLairGuardToAggro()
{
    // Check that current diamond rti target is not Onyxian Lair Guard
    Group* group = bot->GetGroup();
    if (!group)
    {
        return false;
    }

    ObjectGuid currentSkullTarget = group->GetTargetIcon(RtiTargetValue::skullIndex);
    if (currentSkullTarget)
    {
        Unit* currentSkullUnit = botAI->GetUnit(currentSkullTarget);
        if (currentSkullUnit && currentSkullUnit->IsAlive() && currentSkullUnit->IsInWorld() &&
            currentSkullUnit->GetEntry() == NPC_ONYXIAN_LAIR_GUARD)
        {
            ObjectGuid unitTargetGuid = currentSkullUnit->GetTarget();
            if (!unitTargetGuid)
            {
                return true;
            }

            Player* targetedPlayer = botAI->GetPlayer(unitTargetGuid);
            if (!targetedPlayer || targetedPlayer != bot)
            {
                return true;
            }
        }
    }

    return false;
}

Unit* OnyxiaTankOnyxianLairGuardTrigger::GetOnyxianLairGuardToTank()
{
    // Find alive Onyxian Lair Guard that is not targeting the bot
    GuidVector targets = AI_VALUE(GuidVector, "possible targets");
    for (ObjectGuid guid : targets)
    {
        Creature* unit = botAI->GetCreature(guid);
        if (!unit || !unit->IsAlive() || !unit->IsInWorld() || unit->GetEntry() != NPC_ONYXIAN_LAIR_GUARD)
        {
            continue;
        }

        ObjectGuid unitTargetGuid = unit->GetTarget();
        if (!unitTargetGuid)
        {
            return unit;
        }

        Player* targetedPlayer = botAI->GetPlayer(unitTargetGuid);
        if (!targetedPlayer || targetedPlayer != bot)
        {
            return unit;
        }
    }

    return nullptr;
}

bool OnyxiaTankOnyxianLairGuardTrigger::CurrentTargetIsMarkedOnyxianLairGuard()
{
    Group* group = bot->GetGroup();
    if (!group)
    {
        return false;
    }

    ObjectGuid currentSkullTarget = group->GetTargetIcon(RtiTargetValue::skullIndex);
    if (!currentSkullTarget)
    {
        return false;
    }

    return bot->GetTarget() == currentSkullTarget;
}

bool OnyxiaHealerForMainTankTrigger::IsActive()
{
    if (!IsOnyxiaFlying(nullptr))
    {
        return false;
    }

    OnyxiaDeepBreathTrigger onyxiaDeepBreathTrigger(botAI);
    if (onyxiaDeepBreathTrigger.IsActive())
    {
        return false;
    }

    if (!botAI->IsHeal(bot))
    {
        return false;
    }

    Group* group = bot->GetGroup();
    if (!group)
    {
        return false;
    }

    // First alive healer in group
    int assignedHealers = 0;
    for (GroupReference* gref = group->GetFirstMember(); gref; gref = gref->next())
    {
        Player* member = gref->GetSource();
        if (!member || !member->IsAlive() || !botAI->IsHeal(member))
        {
            continue;
        }

        assignedHealers++;
        if (member != bot && assignedHealers == 2)
        {
            return false;
        }

        break;
    }

    return bot->GetDistance2d(ONYXIA_OLG_HEALER_SPOT.GetPositionX(), ONYXIA_OLG_HEALER_SPOT.GetPositionY()) > 5.0f;
}
