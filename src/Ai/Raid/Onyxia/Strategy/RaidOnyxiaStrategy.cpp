#include "RaidOnyxiaStrategy.h"

void RaidOnyxiaStrategy::InitTriggers(std::vector<TriggerNode*>& triggers)
{
    

    // ----------- Phase 1 (100% - 65%) -----------

    triggers.push_back(new TriggerNode("ony near tail",
        { NextAction("ony move to side", ACTION_RAID + 2) }));

    triggers.push_back(new TriggerNode("ony avoid eggs",
        {NextAction("ony avoid eggs move", ACTION_RAID + 5)}));

    // ----------- Phase 2 (65% - 40%) -----------

    triggers.push_back(new TriggerNode("ony deep breath warning",
        { NextAction("ony move to safe zone", ACTION_EMERGENCY + 5) }));

    triggers.push_back(new TriggerNode("ony fireball splash incoming",
        {NextAction("ony spread out", ACTION_RAID + 2)}));

    //triggers.push_back(
    //    new TriggerNode("ony whelps spawn",
    //        { NextAction("ony kill whelps", ACTION_RAID + 1) }));

    triggers.push_back(new TriggerNode("onyxia fire resistance trigger",
        {NextAction("onyxia fire resistance action", ACTION_RAID) }));

    triggers.push_back(new TriggerNode("onyxia mark target trigger",
        {NextAction("onyxia mark target action", ACTION_RAID)}));

    triggers.push_back(new TriggerNode("onyxia onyxian lair guard casting trigger",
        {NextAction("onyxia onyxian lair guard casting action", ACTION_RAID + 3)}));

    triggers.push_back(new TriggerNode("onyxia back to chamber trigger",
        {NextAction("onyxia back to chamber action", ACTION_RAID + 4)}));

    triggers.push_back(new TriggerNode("onyxia tank onyxian lair guard trigger",
        {NextAction("onyxia tank onyxian lair guard action", ACTION_RAID + 4)}));

    triggers.push_back(new TriggerNode("onyxia healer for main tank trigger",
        {NextAction("onyxia healer for main tank action", ACTION_RAID + 4)}));
}

void RaidOnyxiaStrategy::InitMultipliers(std::vector<Multiplier*>& multipliers)
{
    // Empty for now
}
