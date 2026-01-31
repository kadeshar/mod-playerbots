// OnyxiaTriggers.h
#ifndef _PLAYERBOT_ONYXIATRIGGERS_H_
#define _PLAYERBOT_ONYXIATRIGGERS_H_

#include "PlayerbotAI.h"
#include "Trigger.h"

const float ONYXIA_CHAMBER_RADIUS = 48.0f;
const Position ONYXIA_CHAMBER_MIDDLE = Position(-29.181866f, -218.85127f, -88.981255f);
const Position ONYXIA_OLG_ENTRANCE = Position(-78.74222f, -214.71198f, -83.13323f);
const Position ONYXIA_OLG_HEALER_SPOT = Position(-54.955112f, -210.87793f, -85.414444f);

enum OnyxiaStrategyIDs
{
    NPC_ONYXIAN_WHELP = 11262,
    SPELL_BREATH_N_TO_S = 17086,
    SPELL_BREATH_S_TO_N = 18351,
    SPELL_FIREBALL = 18392,
    SPELL_BREATH_SE_TO_NW = 18564,
    SPELL_BREATH_E_TO_W = 18576,
    SPELL_BREATH_NW_TO_SE = 18584,
    SPELL_BREATH_SW_TO_NE = 18596,
    SPELL_BREATH_W_TO_E = 18609,
    SPELL_BREATH_NE_TO_SW = 18617,
    NPC_ONYXIAN_LAIR_GUARD = 36561,
    SPELL_OLG_BLASTNOVA = 68958,
    SPELL_OLG_IGNITEWEAPON = 68959
};

// Mechanics
class OnyxiaTrigger : public Trigger
{
public:
    OnyxiaTrigger(PlayerbotAI* botAI, std::string const name) : Trigger(botAI, name) {}

protected:
    bool IsOnyxiaFight();
    bool IsOnyxiaFlying(Unit* boss);
};

class OnyxiaDeepBreathTrigger : public Trigger
{
public:
    OnyxiaDeepBreathTrigger(PlayerbotAI* botAI) : Trigger(botAI, "ony deep breath warning") {}
    bool IsActive() override;
};

class OnyxiaNearTailTrigger : public Trigger
{
public:
    OnyxiaNearTailTrigger(PlayerbotAI* botAI) : Trigger(botAI, "ony near tail") {}
    bool IsActive() override;
};

class RaidOnyxiaFireballSplashTrigger : public Trigger
{
public:
    RaidOnyxiaFireballSplashTrigger(PlayerbotAI* botAI) : Trigger(botAI, "ony fireball splash incoming") {}
    bool IsActive() override;
};

class RaidOnyxiaWhelpsSpawnTrigger : public Trigger
{
public:
    RaidOnyxiaWhelpsSpawnTrigger(PlayerbotAI* botAI) : Trigger(botAI, "ony whelps spawn") {}
    bool IsActive() override;
};

class OnyxiaAvoidEggsTrigger : public Trigger
{
public:
    OnyxiaAvoidEggsTrigger(PlayerbotAI* botAI) : Trigger(botAI, "ony avoid eggs") {}
    bool IsActive() override;
};

class OnyxiaMarkTargetTrigger : public OnyxiaTrigger
{
public:
    OnyxiaMarkTargetTrigger(PlayerbotAI* botAI) : OnyxiaTrigger(botAI, "onyxia mark target trigger") {}
    bool IsActive() override;
};

class OnyxiaOnyxianLairGuardCastingTrigger : public OnyxiaTrigger
{
public:
    OnyxiaOnyxianLairGuardCastingTrigger(PlayerbotAI* botAI)
        : OnyxiaTrigger(botAI, "onyxia onyxian lair guard casting trigger")
    {
    }
    bool IsActive() override;
};

class OnyxiaBackToChamberTrigger : public OnyxiaTrigger
{
public:
    OnyxiaBackToChamberTrigger(PlayerbotAI* botAI) : OnyxiaTrigger(botAI, "onyxia back to chamber trigger") {}
    bool IsActive() override;
};

class OnyxiaTankOnyxianLairGuardTrigger : public OnyxiaTrigger
{
public:
    OnyxiaTankOnyxianLairGuardTrigger(PlayerbotAI* botAI)
        : OnyxiaTrigger(botAI, "onyxia tank onyxian lair guard trigger")
    {
    }
    bool IsActive() override;
    bool CurrentTargetIsMarkedOnyxianLairGuard();
    bool HaveOnyxianLairGuardToAggro();
    Unit* GetOnyxianLairGuardToTank();
};

class OnyxiaHealerForMainTankTrigger : public OnyxiaTrigger
{
public:
    OnyxiaHealerForMainTankTrigger(PlayerbotAI* botAI) : OnyxiaTrigger(botAI, "onyxia healer for main tank trigger") {}
    bool IsActive() override;
};


#endif
