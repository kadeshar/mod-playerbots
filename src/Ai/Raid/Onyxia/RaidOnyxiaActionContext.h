#ifndef _PLAYERBOT_RAIDONYXIAACTIONS_CONTEXT_H
#define _PLAYERBOT_RAIDONYXIAACTIONS_CONTEXT_H

#include "Action.h"
#include "NamedObjectContext.h"
#include "RaidOnyxiaActions.h"

class RaidOnyxiaActionContext : public NamedObjectContext<Action>
{
public:
    RaidOnyxiaActionContext()
    {
        creators["ony move to side"] = &RaidOnyxiaActionContext::move_to_side;
        creators["ony spread out"] = &RaidOnyxiaActionContext::spread_out;
        creators["ony move to safe zone"] = &RaidOnyxiaActionContext::move_to_safe_zone;
        creators["ony kill whelps"] = &RaidOnyxiaActionContext::kill_whelps;
        creators["ony avoid eggs move"] = &RaidOnyxiaActionContext::avoid_eggs;
        creators["onyxia fire resistance action"] = &RaidOnyxiaActionContext::onyxia_fire_resistance_action;
        creators["onyxia mark target action"] = &RaidOnyxiaActionContext::onyxia_mark_target_action;
        creators["onyxia onyxian lair guard casting action"] =
            &RaidOnyxiaActionContext::onyxia_onyxian_lair_guard_casting_action;
        creators["onyxia back to chamber action"] = &RaidOnyxiaActionContext::onyxia_back_to_chamber_action;
        creators["onyxia tank onyxian lair guard action"] =
            &RaidOnyxiaActionContext::onyxia_tank_onyxian_lair_guard_action;
        creators["onyxia healer for main tank action"] = &RaidOnyxiaActionContext::onyxia_healer_for_main_tank_action;
    }

private:
    static Action* move_to_side(PlayerbotAI* ai) { return new RaidOnyxiaMoveToSideAction(ai); }
    static Action* spread_out(PlayerbotAI* ai) { return new RaidOnyxiaSpreadOutAction(ai); }
    static Action* move_to_safe_zone(PlayerbotAI* ai) { return new RaidOnyxiaMoveToSafeZoneAction(ai); }
    static Action* kill_whelps(PlayerbotAI* ai) { return new RaidOnyxiaKillWhelpsAction(ai); }
    static Action* avoid_eggs(PlayerbotAI* ai) { return new OnyxiaAvoidEggsAction(ai); }
    static Action* onyxia_fire_resistance_action(PlayerbotAI* ai) { return new BossFireResistanceAction(ai, "onyxia"); }
    static Action* onyxia_mark_target_action(PlayerbotAI* ai) { return new OnyxiaMarkTargetAction(ai); }
    static Action* onyxia_onyxian_lair_guard_casting_action(PlayerbotAI* ai) { return new OnyxiaOnyxianLairGuardCastingAction(ai); }
    static Action* onyxia_back_to_chamber_action(PlayerbotAI* ai) { return new OnyxiaBackToChamberAction(ai); }
    static Action* onyxia_tank_onyxian_lair_guard_action(PlayerbotAI* ai) { return new OnyxiaTankOnyxianLairGuardAction(ai); }
    static Action* onyxia_healer_for_main_tank_action(PlayerbotAI* ai) { return new OnyxiaHealerForMainTankAction(ai); }
};

#endif
