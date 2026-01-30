#include "RaidNaxxActions.h"

#include <algorithm>
#include <unordered_map>

#include "Playerbots.h"
#include "RaidNaxxSpellIds.h"

namespace
{
    constexpr float LiveX = 2691.2f;
    constexpr float LiveY = -3387.0f;
    constexpr float LiveZ = 267.68f;

    constexpr float DeadX = 2693.5f;
    constexpr float DeadY = -3334.6f;
    constexpr float DeadZ = 267.68f;

    constexpr char const* StratCoTank     = "co tank";
    constexpr char const* StratTankFace   = "tank face";
    constexpr char const* StratTankAssist = "tank assist";
    constexpr char const* StratHeal       = "heal";

    enum class GothikSide : uint8
    {
        Live = 0,
        Dead = 1
    };

    inline bool IsLiveSide(Unit const* who)
    {
        return who && who->GetPositionY() < NaxxSpellIds::GothikGateY;
    }

    inline bool IsGothikAdd(uint32 entry)
    {
        switch (entry)
        {
            case NaxxSpellIds::GothikLivingTraineeEntry:
            case NaxxSpellIds::GothikLivingKnightEntry:
            case NaxxSpellIds::GothikLivingRiderEntry:
            case NaxxSpellIds::GothikDeadTraineeEntry:
            case NaxxSpellIds::GothikDeadKnightEntry:
            case NaxxSpellIds::GothikDeadHorseEntry:
            case NaxxSpellIds::GothikDeadRiderEntry:
                return true;
        }
        return false;
    }

    inline uint32 GetAddPriority(uint32 entry)
    {
        switch (entry)
        {
            case NaxxSpellIds::GothikLivingRiderEntry:   return 70;
            case NaxxSpellIds::GothikLivingKnightEntry:  return 60;
            case NaxxSpellIds::GothikLivingTraineeEntry: return 50;
            case NaxxSpellIds::GothikDeadRiderEntry:     return 40;
            case NaxxSpellIds::GothikDeadKnightEntry:    return 30;
            case NaxxSpellIds::GothikDeadHorseEntry:     return 20;
            case NaxxSpellIds::GothikDeadTraineeEntry:   return 10;
        }
        return 0;
    }

    static inline bool HasStrategyAnyState(Player* p, char const* name)
    {
        if (!p || !name)
            return false;
        if (PlayerbotAI* ai = GET_PLAYERBOT_AI(p))
        {
            return ai->HasStrategy(name, BOT_STATE_NON_COMBAT) ||
                   ai->HasStrategy(name, BOT_STATE_COMBAT);
        }
        return false;
    }

    static inline bool IsTankRoleForGothik(PlayerbotAI* localAI, Player* p)
    {
        if (!p)
            return false;

        if (p->HasTankSpec())
            return true;

        if (localAI && localAI->IsTank(p))
            return true;

        return HasStrategyAnyState(p, StratTankFace) || HasStrategyAnyState(p, StratTankAssist) || HasStrategyAnyState(p, "tank") ||
               HasStrategyAnyState(p, "bear");
    }

    static inline bool IsHealerRoleForGothik(PlayerbotAI* localAI, Player* p)
    {
        if (!p)
            return false;

        if (localAI && localAI->IsHeal(p))
            return true;

        return HasStrategyAnyState(p, StratHeal) || HasStrategyAnyState(p, "holy heal") || HasStrategyAnyState(p, "offheal");
    }

    static inline bool IsMainTankByStrategies(Player* p)
    {
        return HasStrategyAnyState(p, StratTankFace);
    }

    static inline bool IsOffTankByStrategies(Player* p)
    {
        return HasStrategyAnyState(p, StratTankAssist) && !HasStrategyAnyState(p, StratTankFace);
    }

    GothikSide GetAssignedSide(PlayerbotAI* localAI, Player* bot)
    {
        if (!localAI || !bot)
            return GothikSide::Live;

        Group* group = bot->GetGroup();
        if (!group)
            return GothikSide::Live;

        std::vector<Player*> members;
        for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
        {
            Player* m = ref->GetSource();
            if (m && m->IsAlive())
                members.push_back(m);
        }

        if (members.empty())
            return GothikSide::Live;

        std::sort(members.begin(), members.end(), [](Player* a, Player* b)
        {
            return a->GetGUID() < b->GetGUID();
        });

        std::vector<Player*> tanks, heals, dps;
        tanks.reserve(2);
        heals.reserve(4);
        dps.reserve(25);

        for (Player* m : members)
        {
            if (IsTankRoleForGothik(localAI, m))
                tanks.push_back(m);
            else if (IsHealerRoleForGothik(localAI, m))
                heals.push_back(m);
            else
                dps.push_back(m);
        }

        Player* mainTank = nullptr;
        Player* offTank = nullptr;

        for (Player* t : tanks)
        {
            if (HasStrategyAnyState(t, StratCoTank) && HasStrategyAnyState(t, StratTankFace))
            {
                mainTank = t;
                break;
            }
        }

        if (!mainTank)
        {
            for (Player* t : tanks)
            {
                if (IsMainTankByStrategies(t))
                {
                    mainTank = t;
                    break;
                }
            }
        }

        for (Player* t : tanks)
        {
            if (t != mainTank && IsOffTankByStrategies(t))
            {
                offTank = t;
                break;
            }
        }

        if (!mainTank)
        {
            for (Player* t : tanks)
            {
                if (HasStrategyAnyState(t, StratTankFace) && !HasStrategyAnyState(t, StratTankAssist))
                {
                    mainTank = t;
                    break;
                }
            }
        }

        if (!mainTank && !tanks.empty())
            mainTank = tanks.front();

        if (!offTank)
        {
            for (Player* t : tanks)
            {
                if (t != mainTank)
                {
                    offTank = t;
                    break;
                }
            }
        }

        std::unordered_map<ObjectGuid, GothikSide> side;
        side.reserve(members.size());

        if (mainTank)
            side[mainTank->GetGUID()] = GothikSide::Live;
        if (offTank)
            side[offTank->GetGUID()] = GothikSide::Dead;

        for (Player* t : tanks)
        {
            if (t == mainTank || t == offTank)
                continue;
            side[t->GetGUID()] = GothikSide::Dead;
        }

        if (heals.size() == 1)
        {
            side[heals[0]->GetGUID()] = GothikSide::Live;
        }
        else if (heals.size() >= 2)
        {
            side[heals[0]->GetGUID()] = GothikSide::Live;
            side[heals[1]->GetGUID()] = GothikSide::Dead;

            uint32 liveCount = 0, deadCount = 0;
            for (Player* m : members)
            {
                auto it = side.find(m->GetGUID());
                GothikSide s = (it != side.end()) ? it->second : GothikSide::Live;
                (s == GothikSide::Live ? liveCount : deadCount)++;
            }

            for (size_t i = 2; i < heals.size(); ++i)
            {
                GothikSide s = (liveCount <= deadCount) ? GothikSide::Live : GothikSide::Dead;
                side[heals[i]->GetGUID()] = s;
                (s == GothikSide::Live ? liveCount : deadCount)++;
            }
        }
        uint32 const minLiveDps = std::min<uint32>(2u, uint32(dps.size()));
        uint32 liveDps = 0;
        for (Player* m : dps)
        {
            auto it = side.find(m->GetGUID());
            if (it != side.end() && it->second == GothikSide::Live)
                ++liveDps;
        }
        for (Player* m : dps)
        {
            if (liveDps >= minLiveDps)
                break;

            if (side.find(m->GetGUID()) != side.end())
                continue;

            side[m->GetGUID()] = GothikSide::Live;
            ++liveDps;
        }
        uint32 liveCount = 0, deadCount = 0;
        for (Player* m : members)
        {
            auto it = side.find(m->GetGUID());
            GothikSide s = (it != side.end()) ? it->second : GothikSide::Live;
            (s == GothikSide::Live ? liveCount : deadCount)++;
        }

        for (Player* m : dps)
        {
            if (side.find(m->GetGUID()) != side.end())
                continue;

            GothikSide s = (liveCount <= deadCount) ? GothikSide::Live : GothikSide::Dead;
            side[m->GetGUID()] = s;
            (s == GothikSide::Live ? liveCount : deadCount)++;
        }

        auto it = side.find(bot->GetGUID());
        return (it != side.end()) ? it->second : GothikSide::Live;
    }
} // namespace

bool GothikMoveToAssignedSideAction::isUseful()
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "gothik the harvester");
    if (!boss)
        return false;

    if (bot->GetDistance(boss) > 160.0f)
        return false;

    GothikSide assigned = GetAssignedSide(botAI, bot);
    bool wantLive = (assigned == GothikSide::Live);
    bool isLive = IsLiveSide(bot);

    if (wantLive != isLive)
        return true;

    float ax = wantLive ? LiveX : DeadX;
    float ay = wantLive ? LiveY : DeadY;
    float az = wantLive ? LiveZ : DeadZ;
    return bot->GetDistance(ax, ay, az) > 12.0f;
}

bool GothikMoveToAssignedSideAction::Execute(Event /*event*/)
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "gothik the harvester");
    if (!boss)
        return false;

    GothikSide assigned = GetAssignedSide(botAI, bot);
    bool wantLive = (assigned == GothikSide::Live);

    float ax = wantLive ? LiveX : DeadX;
    float ay = wantLive ? LiveY : DeadY;
    float az = wantLive ? LiveZ : DeadZ;

    if (MoveTo(NAXX_MAP_ID, ax, ay, az, false, false, false, false, MovementPriority::MOVEMENT_COMBAT))
        return true;

    return MoveInside(NAXX_MAP_ID, ax, ay, az, 3.0f, MovementPriority::MOVEMENT_COMBAT);
}

bool GothikChooseTargetAction::isUseful()
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "gothik the harvester");
    if (!boss)
        return false;

    return boss->IsInCombat() || bot->IsInCombat();
}

bool GothikChooseTargetAction::Execute(Event /*event*/)
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "gothik the harvester");
    if (!boss)
        return false;

    bool myLiveSide = IsLiveSide(bot);

    Creature* gothik = boss->ToCreature();
    bool bossAttackable = false;
    if (gothik && gothik->IsAlive())
    {
        bossAttackable = (gothik->GetReactState() == REACT_AGGRESSIVE) && !gothik->HasUnitFlag(UNIT_FLAG_DISABLE_MOVE);
    }

    GuidVector candidates = context->GetValue<GuidVector>("possible targets")->Get();

    Unit* bestAdd = nullptr;
    uint32 bestPrio = 0;
    float bestDist = 0.0f;

    for (ObjectGuid const& guid : candidates)
    {
        Unit* unit = botAI->GetUnit(guid);
        if (!unit || !unit->IsAlive())
            continue;

        if (!IsGothikAdd(unit->GetEntry()))
            continue;

        if (IsLiveSide(unit) != myLiveSide)
            continue;

        uint32 prio = GetAddPriority(unit->GetEntry());
        float dist = bot->GetDistance(unit);

        if (!bestAdd || prio > bestPrio || (prio == bestPrio && dist < bestDist))
        {
            bestAdd = unit;
            bestPrio = prio;
            bestDist = dist;
        }
    }

    if (bestAdd)
    {
        if (AI_VALUE(Unit*, "current target") == bestAdd)
            return false;
        return Attack(bestAdd);
    }
    if (bossAttackable && IsLiveSide(boss) == myLiveSide)
    {
        if (AI_VALUE(Unit*, "current target") == boss)
            return false;
        return Attack(boss);
    }

    return false;
}