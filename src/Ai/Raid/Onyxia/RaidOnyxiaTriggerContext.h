#ifndef _PLAYERBOT_RAIDONYXIATRIGGERCONTEXT_H
#define _PLAYERBOT_RAIDONYXIATRIGGERCONTEXT_H

#include "AiObjectContext.h"
#include "NamedObjectContext.h"
#include "RaidOnyxiaTriggers.h"

class RaidOnyxiaTriggerContext : public NamedObjectContext<Trigger>
{
public:
    RaidOnyxiaTriggerContext()
    {
        creators["ony near tail"] = &RaidOnyxiaTriggerContext::near_tail;
        creators["ony deep breath warning"] = &RaidOnyxiaTriggerContext::deep_breath;
        creators["ony fireball splash incoming"] = &RaidOnyxiaTriggerContext::fireball_splash;
        creators["ony whelps spawn"] = &RaidOnyxiaTriggerContext::whelps_spawn;
        creators["ony avoid eggs"] = &RaidOnyxiaTriggerContext::avoid_eggs;
        creators["onyxia fire resistance trigger"] = &RaidOnyxiaTriggerContext::onyxia_fire_resistance_trigger;
        creators["onyxia mark target trigger"] = &RaidOnyxiaTriggerContext::onyxia_mark_target_trigger;
        creators["onyxia onyxian lair guard casting trigger"] =
            &RaidOnyxiaTriggerContext::onyxia_onyxian_lair_guard_casting_trigger;
        creators["onyxia back to chamber trigger"] = &RaidOnyxiaTriggerContext::onyxia_back_to_chamber_trigger;
        creators["onyxia tank onyxian lair guard trigger"] =
            &RaidOnyxiaTriggerContext::onyxia_tank_onyxian_lair_guard_trigger;
        creators["onyxia healer for main tank trigger"] =
            &RaidOnyxiaTriggerContext::onyxia_healer_for_main_tank_trigger;
    }

private:
    static Trigger* near_tail(PlayerbotAI* ai) { return new OnyxiaNearTailTrigger(ai); }
    static Trigger* deep_breath(PlayerbotAI* ai) { return new OnyxiaDeepBreathTrigger(ai); }
    static Trigger* fireball_splash(PlayerbotAI* ai) { return new RaidOnyxiaFireballSplashTrigger(ai); }
    static Trigger* whelps_spawn(PlayerbotAI* ai) { return new RaidOnyxiaWhelpsSpawnTrigger(ai); }
    static Trigger* avoid_eggs(PlayerbotAI* ai) { return new OnyxiaAvoidEggsTrigger(ai); }
    static Trigger* onyxia_fire_resistance_trigger(PlayerbotAI* ai) { return new BossFireResistanceTrigger(ai, "onyxia"); }
    static Trigger* onyxia_mark_target_trigger(PlayerbotAI* ai) { return new OnyxiaMarkTargetTrigger(ai); }
    static Trigger* onyxia_onyxian_lair_guard_casting_trigger(PlayerbotAI* ai) { return new OnyxiaOnyxianLairGuardCastingTrigger(ai); }
    static Trigger* onyxia_back_to_chamber_trigger(PlayerbotAI* ai) { return new OnyxiaBackToChamberTrigger(ai); }
    static Trigger* onyxia_tank_onyxian_lair_guard_trigger(PlayerbotAI* ai) { return new OnyxiaTankOnyxianLairGuardTrigger(ai); }
    static Trigger* onyxia_healer_for_main_tank_trigger(PlayerbotAI* ai) { return new OnyxiaHealerForMainTankTrigger(ai); }
};

#endif
