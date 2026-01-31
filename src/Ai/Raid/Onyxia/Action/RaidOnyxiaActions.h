// RaidOnyxiaActions.h
#ifndef _PLAYERBOT_RAIDONYXIAACTIONS_H_
#define _PLAYERBOT_RAIDONYXIAACTIONS_H_

#include "Action.h"
#include "AttackAction.h"
#include "GenericSpellActions.h"
#include "MovementActions.h"
#include "RaidOnyxiaTriggers.h"

class PlayerbotAI;

class RaidOnyxiaMoveToSideAction : public MovementAction
{
public:
    RaidOnyxiaMoveToSideAction(PlayerbotAI* botAI, std::string const name = "ony move to side")
        : MovementAction(botAI, name)
    {
    }
    bool Execute(Event event) override;
};

class RaidOnyxiaSpreadOutAction : public MovementAction
{
public:
    RaidOnyxiaSpreadOutAction(PlayerbotAI* botAI, std::string const name = "ony spread out")
        : MovementAction(botAI, name)
    {
    }
    bool Execute(Event event) override;
};

struct SafeZone
{
    Position pos;
    float radius;
};

class RaidOnyxiaMoveToSafeZoneAction : public MovementAction
{
public:
    RaidOnyxiaMoveToSafeZoneAction(PlayerbotAI* botAI, std::string const name = "ony move to safe zone")
        : MovementAction(botAI, name)
    {
    }
    bool Execute(Event event) override;

private:
    std::vector<SafeZone> GetSafeZonesForBreath(uint32 spellId)
    {
        // Define your safe zone coordinates based on the map
        // Example assumes Onyxia's lair map coordinates
        float z = bot->GetPositionZ();  // Stay at current height

        switch (spellId)
        {
            case SPELL_BREATH_N_TO_S:  // N to S
            case SPELL_BREATH_S_TO_N:  // S to N
                return {SafeZone{Position(-10.0f, -180.0f, z), 5.0f},
                        SafeZone{Position(-20.0f, -250.0f, z), 5.0f}};  // Bottom Safe Zone

            case SPELL_BREATH_E_TO_W:  // E to W
            case SPELL_BREATH_W_TO_E:  // W to E
                return {
                    SafeZone{Position(20.0f, -210.0f, z), 5.0f},
                    SafeZone{Position(-75.0f, -210.0f, z), 5.0f},
                };  // Left Safe Zone

            case SPELL_BREATH_SE_TO_NW:  // SE to NW
            case SPELL_BREATH_NW_TO_SE:  // NW to SE
                return {
                    SafeZone{Position(-60.0f, -195.0f, z), 5.0f},
                    SafeZone{Position(10.0f, -240.0f, z), 5.0f},
                };  // NW Safe Zone

            case SPELL_BREATH_SW_TO_NE:  // SW to NE
            case SPELL_BREATH_NE_TO_SW:  // NE to SW
                return {
                    SafeZone{Position(7.0f, -185.0f, z), 5.0f},
                    SafeZone{Position(-60.0f, -240.0f, z), 5.0f},
                };  // NE Safe Zone

            default:
                return {SafeZone{Position(0.0f, 0.0f, z), 5.0f}};  // Fallback center - shouldn't ever happen
        }
    }
};

class RaidOnyxiaKillWhelpsAction : public AttackAction
{
public:
    RaidOnyxiaKillWhelpsAction(PlayerbotAI* botAI, std::string const name = "ony kill whelps")
        : AttackAction(botAI, name)
    {
    }

    bool Execute(Event event) override;
};

class OnyxiaAvoidEggsAction : public MovementAction
{
public:
    OnyxiaAvoidEggsAction(PlayerbotAI* botAI) : MovementAction(botAI, "ony avoid eggs move") {}

    bool Execute(Event event) override;
};

class OnyxiaMarkTargetAction : public Action
{
public:
    OnyxiaMarkTargetAction(PlayerbotAI* botAI, std::string const name = "onyxia mark target action")
        : Action(botAI, name)
    {
    }

    bool Execute(Event event) override;
};

class OnyxiaOnyxianLairGuardCastingAction : public MovementAction
{
public:
    OnyxiaOnyxianLairGuardCastingAction(PlayerbotAI* botAI,
                                        std::string const name = "onyxia onyxian lair guard casting action")
        : MovementAction(botAI, name)
    {
    }

    bool Execute(Event event) override;
};

class OnyxiaBackToChamberAction : public MovementAction
{
public:
    OnyxiaBackToChamberAction(PlayerbotAI* botAI, std::string const name = "onyxia back to chamber action")
        : MovementAction(botAI, name)
    {
    }

    bool Execute(Event event) override;
};

class OnyxiaTankOnyxianLairGuardAction : public MovementAction
{
public:
    OnyxiaTankOnyxianLairGuardAction(PlayerbotAI* botAI,
                                     std::string const name = "onyxia tank onyxian lair guard action")
        : MovementAction(botAI, name)
    {
    }

    bool Execute(Event event) override;
};

class OnyxiaHealerForMainTankAction : public MovementAction
{
public:
    OnyxiaHealerForMainTankAction(PlayerbotAI* botAI, std::string const name = "onyxia healer for main tank action")
        : MovementAction(botAI, name)
    {
    }

    bool Execute(Event event) override;
};

#endif
