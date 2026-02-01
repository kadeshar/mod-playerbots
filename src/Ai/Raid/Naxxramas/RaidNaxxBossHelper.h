#ifndef _PLAYERBOT_RAIDNAXXBOSSHELPER_H
#define _PLAYERBOT_RAIDNAXXBOSSHELPER_H

#include <algorithm>
#include <cmath>
#include <limits>
#include <string>
#include <vector>

#include "AiObject.h"
#include "AiObjectContext.h"
#include "EventMap.h"
#include "Log.h"
#include "MotionMaster.h"
#include "NamedObjectContext.h"
#include "ObjectGuid.h"
#include "ObjectAccessor.h"
#include "Pet.h"
#include "Player.h"
#include "PlayerbotAI.h"
#include "Playerbots.h"
#include "ScriptedCreature.h"
#include "SharedDefines.h"
#include "Spell.h"
#include "Timer.h"
#include "RaidNaxxSpellIds.h"

const uint32 NAXX_MAP_ID = 533;

template <class BossAiType>
class GenericBossHelper : public AiObject
{
public:
    GenericBossHelper(PlayerbotAI* botAI, std::string name) : AiObject(botAI), _name(name) {}
    virtual bool UpdateBossAI()
    {
        if (!bot->IsInCombat())
        {
            _unit = nullptr;
        }
        if (_unit && (!_unit->IsInWorld() || !_unit->IsAlive()))
        {
            _unit = nullptr;
        }
        if (!_unit)
        {
            _unit = AI_VALUE2(Unit*, "find target", _name);
            if (!_unit)
            {
                return false;
            }
            _target = _unit->ToCreature();
            if (!_target)
            {
                return false;
            }
            _ai = dynamic_cast<BossAiType*>(_target->GetAI());
            if (!_ai)
            {
                return false;
            }
            _event_map = &_ai->events;
            if (!_event_map)
            {
                return false;
            }
        }
        if (!_event_map)
        {
            return false;
        }
        _timer = getMSTime();
        return true;
    }
    virtual void Reset()
    {
        _unit = nullptr;
        _target = nullptr;
        _ai = nullptr;
        _event_map = nullptr;
        _timer = 0;
    }

protected:
    std::string _name;
    Unit* _unit = nullptr;
    Creature* _target = nullptr;
    BossAiType* _ai = nullptr;
    EventMap* _event_map = nullptr;
    uint32 _timer = 0;
};

class KelthuzadBossHelper : public AiObject
{
public:
    KelthuzadBossHelper(PlayerbotAI* botAI) : AiObject(botAI) {}

    static constexpr uint32 NPC_GUARDIAN_OF_ICECROWN = 16441;

    bool IsGuardian(Unit* unit) const
    {
        if (!unit)
            return false;
        if (Creature* c = unit->ToCreature())
            if (c->GetEntry() == NPC_GUARDIAN_OF_ICECROWN)
                return true;
        return botAI->EqualLowercaseName(unit->GetName(), "guardian of icecrown");
    }

    const std::pair<float, float> center = {3716.19f, -5106.58f};
    const std::pair<float, float> tank_pos = {3709.19f, -5104.86f};
    const std::pair<float, float> assist_tank_pos = {3746.05f, -5112.74f};

    static constexpr float ROOM_MIN_RADIUS = 6.0f;
    static constexpr float ROOM_MAX_RADIUS = 24.0f;
    static constexpr float DETONATE_MIN_RADIUS = 20.0f;
    static constexpr float DETONATE_MAX_RADIUS = 24.0f;
    static constexpr float TANK_HOLD_MAX_RADIUS = 20.0f;
    static constexpr float PHASE1_TANK_MAX_RADIUS = 16.0f;
    static constexpr float PHASE1_TANK_HOLD_RADIUS = 12.0f;

    bool UpdateBossAI()
    {
        if (!bot->IsInCombat())
        {
            Reset();
        }
        if (_unit && (!_unit->IsInWorld() || !_unit->IsAlive()))
        {
            Reset();
        }
        if (!_unit)
        {
            _unit = AI_VALUE2(Unit*, "find target", "kel'thuzad");
        }
        return _unit != nullptr;
    }
    bool IsPhaseOne() { return _unit && _unit->HasUnitFlag(UNIT_FLAG_NON_ATTACKABLE); }
    bool IsPhaseTwo() { return _unit && !_unit->HasUnitFlag(UNIT_FLAG_NON_ATTACKABLE); }

    Unit* GetBoss() const { return _unit; }

    bool IsBossCasting(uint32 spellId) const
    {
        if (!_unit)
        {
            return false;
        }

        if (Spell* spell = _unit->GetCurrentSpell(CURRENT_GENERIC_SPELL))
        {
            if (SpellInfo const* info = spell->GetSpellInfo())
            {
                return info->Id == spellId;
            }
        }
        return false;
    }

    uint32 GetRangedCount() const
    {
        Group* group = bot->GetGroup();
        if (!group)
        {
            return 0;
        }

        uint32 count = 0;
        for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
        {
            Player* member = ref->GetSource();
            if (!member)
            {
                continue;
            }

            if (botAI->IsRanged(member))
            {
                ++count;
            }
        }
        return count;
    }

    void ClampToRoom(float& x, float& y, float minRadius = ROOM_MIN_RADIUS, float maxRadius = ROOM_MAX_RADIUS) const
    {
        float dx = x - center.first;
        float dy = y - center.second;
        float r2 = dx * dx + dy * dy;
        if (r2 < 0.0001f)
        {
            x = center.first + minRadius;
            y = center.second;
            return;
        }

        float r = std::sqrt(r2);
        float clamped = std::clamp(r, minRadius, maxRadius);
        x = center.first + dx / r * clamped;
        y = center.second + dy / r * clamped;
    }

    bool IsWithinRoom(WorldObject const* obj, float maxRadius = ROOM_MAX_RADIUS) const
    {
        return obj && obj->GetDistance2d(center.first, center.second) <= maxRadius;
    }

    bool RecallControlledPetsToBot(float leashRadius = (ROOM_MAX_RADIUS + 2.0f), float followDist = 1.5f)
    {
        bool recalled = false;

        auto RecallUnit = [&](Unit* u)
        {
            if (!u)
                return;

            Creature* creature = u->ToCreature();
            if (!creature)
                return;

            if (creature->IsTotem())
                return;

            if (creature->GetDistance2d(center.first, center.second) <= leashRadius)
                return;

            creature->AttackStop();

            if (CharmInfo* charm = creature->GetCharmInfo())
            {
                charm->SetIsCommandAttack(false);
                charm->SetIsAtStay(false);
                charm->SetIsFollowing(true);
                charm->SetIsCommandFollow(true);
                charm->SetIsReturning(false);
            }

            creature->GetMotionMaster()->MoveFollow(bot, followDist, M_PI);
            recalled = true;
        };

        RecallUnit(bot->GetPet());

        for (Unit::ControlSet::const_iterator itr = bot->m_Controlled.begin(); itr != bot->m_Controlled.end(); ++itr)
        {
            RecallUnit(*itr);
        }

        return recalled;
    }

    std::pair<float, float> GetAssistTankHoldPosition() const
    {
        float x = assist_tank_pos.first;
        float y = assist_tank_pos.second;
        ClampToRoom(x, y, ROOM_MIN_RADIUS, TANK_HOLD_MAX_RADIUS);
        return {x, y};
    }

    std::pair<float, float> GetMainTankHoldPosition() const
    {
        float x = tank_pos.first;
        float y = tank_pos.second;
        ClampToRoom(x, y, ROOM_MIN_RADIUS, TANK_HOLD_MAX_RADIUS);
        return {x, y};
    }

    void ComputeRangedSpreadPosition(uint32 index, uint32 total, float& outX, float& outY) const
    {
        if (total == 0)
        {
            outX = center.first;
            outY = center.second;
            return;
        }

        float radii[3] = {18.0f, 21.0f, 24.0f};
        uint32 ringSizes[3] = {0, 0, 0};
        uint32 rings = 1;

        if (total <= 10)
        {
            rings = 1;
            ringSizes[0] = total;
        }
        else if (total <= 18)
        {
            rings = 2;
            ringSizes[0] = (total + 1) / 2;
            ringSizes[1] = total - ringSizes[0];
        }
        else
        {
            rings = 3;
            ringSizes[0] = (total + 2) / 3;
            ringSizes[1] = (total + 1) / 3;
            ringSizes[2] = total - ringSizes[0] - ringSizes[1];
        }

        uint32 ring = 0;
        uint32 localIndex = index;
        for (uint32 r = 0; r < rings; ++r)
        {
            if (localIndex < ringSizes[r])
            {
                ring = r;
                break;
            }
            localIndex -= ringSizes[r];
        }

        uint32 slots = std::max<uint32>(1, ringSizes[ring]);
        float angle = 2.0f * float(M_PI) * (float(localIndex) / float(slots));

        angle += float(ring) * (float(M_PI) / 8.0f);

        outX = center.first + std::cos(angle) * radii[ring];
        outY = center.second + std::sin(angle) * radii[ring];
        ClampToRoom(outX, outY);
    }

    Player* GetPlayerWithAura(uint32 spellId)
    {
        Group* group = bot->GetGroup();
        if (!group)
        {
            return nullptr;
        }
        for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
        {
            Player* member = ref->GetSource();
            if (!member || !member->IsAlive())
            {
                continue;
            }
            if (botAI->HasAura(spellId, member))
            {
                return member;
            }
        }
        return nullptr;
    }
    bool HasAuraInGroup(uint32 spellId) { return GetPlayerWithAura(spellId) != nullptr; }
    bool HasDetonateMana(Player* player)
    {
        if (!player)
        {
            return false;
        }
        return botAI->HasAura(NaxxSpellIds::DetonateMana, player);
    }
    bool HasChains(Player* player)
    {
        if (!player)
        {
            return false;
        }
        return botAI->HasAura(NaxxSpellIds::ChainsOfKelthuzad, player);
    }

    std::vector<Unit*> GetGuardians() const
    {
        std::vector<Unit*> guardians;
        GuidVector targets = context->GetValue<GuidVector>("possible targets")->Get();
        for (ObjectGuid const& guid : targets)
        {
            Unit* unit = botAI->GetUnit(guid);
            if (!unit)
            {
                continue;
            }

            if (!IsGuardian(unit))
            {
                continue;
            }

            if (unit->GetDistance2d(center.first, center.second) > (ROOM_MAX_RADIUS + 4.0f))
            {
                continue;
            }

            guardians.push_back(unit);
        }

        return guardians;
    }

    bool AllGuardiansOnAssistTank(Player* assistTank) const
    {
        if (!assistTank)
        {
            return true;
        }

        std::vector<Unit*> guardians = GetGuardians();
        if (guardians.empty())
        {
            return true;
        }

        for (Unit* g : guardians)
        {
            if (!g)
            {
                continue;
            }

            if (g->GetVictim() != assistTank)
            {
                return false;
            }
        }
        return true;
    }

    Unit* GetGuardianToPickup(Player* assistTank) const
    {
        if (!assistTank)
        {
            return nullptr;
        }

        std::vector<Unit*> guardians = GetGuardians();
        if (guardians.empty())
        {
            return nullptr;
        }

        Unit* best = nullptr;
        float bestDist = std::numeric_limits<float>::max();
        for (Unit* g : guardians)
        {
            if (!g)
            {
                continue;
            }

            if (g->GetVictim() == assistTank)
            {
                continue;
            }

            float d = assistTank->GetDistance2d(g);
            if (!best || d < bestDist)
            {
                best = g;
                bestDist = d;
            }
        }
        if (!best)
        {
            for (Unit* g : guardians)
            {
                float d = assistTank->GetDistance2d(g);
                if (!best || d < bestDist)
                {
                    best = g;
                    bestDist = d;
                }
            }
        }

        return best;
    }

    Unit* GetGuardian()
    {
        GuidVector targets = context->GetValue<GuidVector>("possible targets")->Get();
        for (auto i = targets.begin(); i != targets.end(); ++i)
        {
            Unit* unit = botAI->GetUnit(*i);
            if (!unit)
            {
                continue;
            }
            if (IsGuardian(unit))
            {
                return unit;
            }
        }
        return nullptr;
    }

    Unit* GetGuardianForAssistTank(Player* assistTank)
    {
        if (!assistTank)
        {
            return nullptr;
        }

        GuidVector targets = context->GetValue<GuidVector>("possible targets")->Get();
        Unit* best = nullptr;
        float bestScore = std::numeric_limits<float>::max();

        for (ObjectGuid const& guid : targets)
        {
            Unit* unit = botAI->GetUnit(guid);
            if (!unit)
            {
                continue;
            }

            if (!IsGuardian(unit))
            {
                continue;
            }
            if (unit->GetDistance2d(center.first, center.second) > (ROOM_MAX_RADIUS + 4.0f))
            {
                continue;
            }

            Player* victimPlayer = unit->GetVictim() ? unit->GetVictim()->ToPlayer() : nullptr;
            bool victimIsAssistTank = victimPlayer && botAI->IsAssistTank(victimPlayer);

            float score = unit->GetDistance2d(assistTank);
            if (victimIsAssistTank)
            {
                score += 1000.0f;
            }

            if (!best || score < bestScore)
            {
                best = unit;
                bestScore = score;
            }
        }

        return best;
    }

    Unit* GetAnyShadowFissure()
    {
        Unit* shadow_fissure = nullptr;
        GuidVector units = *context->GetValue<GuidVector>("nearest triggers");
        for (auto i = units.begin(); i != units.end(); i++)
        {
            Unit* unit = botAI->GetUnit(*i);
            if (!unit)
                continue;
            if (botAI->EqualLowercaseName(unit->GetName(), "shadow fissure"))
            {
                shadow_fissure = unit;
            }
        }
        return shadow_fissure;
    }

private:
    void Reset() { _unit = nullptr; }

    Unit* _unit = nullptr;
};

class RazuviousBossHelper : public AiObject
{
public:
    RazuviousBossHelper(PlayerbotAI* botAI) : AiObject(botAI) {}
    bool UpdateBossAI()
    {
        if (!bot->IsInCombat())
        {
            Reset();
        }
        if (_unit && (!_unit->IsInWorld() || !_unit->IsAlive()))
        {
            Reset();
        }
        if (!_unit)
        {
            _unit = AI_VALUE2(Unit*, "find target", "instructor razuvious");
        }
        return _unit != nullptr;
    }

private:
    void Reset() { _unit = nullptr; }

    Unit* _unit = nullptr;
};

class SapphironBossHelper : public AiObject
{
public:
    const std::pair<float, float> mainTankPos = {3512.07f, -5274.06f};
    const std::pair<float, float> center = {3517.31f, -5253.74f};
    const float GENERIC_HEIGHT = 137.29f;
    SapphironBossHelper(PlayerbotAI* botAI) : AiObject(botAI) {}
    bool UpdateBossAI()
    {
        if (!bot->IsInCombat())
        {
            Reset();
        }
        if (_unit && (!_unit->IsInWorld() || !_unit->IsAlive()))
        {
            Reset();
        }
        if (!_unit)
        {
            _unit = AI_VALUE2(Unit*, "find target", "sapphiron");
            if (!_unit)
            {
                return false;
            }
        }
        bool now_flying = _unit->IsFlying();
        if (_was_flying && !now_flying)
        {
            _last_land_ms = getMSTime();
        }
        _was_flying = now_flying;
        UpdateIceboltState();
        return true;
    }
    bool IsPhaseGround() { return _unit && !_unit->IsFlying(); }
    bool IsPhaseFlight() { return _unit && _unit->IsFlying(); }
    bool JustLanded()
    {
        if (!_last_land_ms)
        {
            return false;
        }
        return getMSTime() - _last_land_ms <= POSITION_TIME_AFTER_LANDED;
    }
    bool WaitForExplosion()
    {
        if (!IsPhaseFlight())
        {
            return false;
        }
        return HasIceboltInGroup();
    }
    bool IsBreathWindow()
    {
        if (!IsPhaseFlight())
        {
            return false;
        }
        if (IsBreathCasting())
        {
            return true;
        }
        if (!_last_icebolt_ms)
        {
            return false;
        }
        uint32 elapsed = getMSTime() - _last_icebolt_ms;
        return elapsed >= BREATH_MIN_MS && elapsed <= BREATH_MAX_MS;
    }
    bool HasLifeDrainInGroup()
    {
        Group* group = bot->GetGroup();
        if (!group)
        {
            return false;
        }
        for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
        {
            Player* member = ref->GetSource();
            if (!member)
            {
                continue;
            }
            if (NaxxSpellIds::HasAnyAura(botAI, member, {NaxxSpellIds::LifeDrain}) || botAI->HasAura("life drain", member))
            {
                return true;
            }
        }
        return false;
    }
    bool FindPosToAvoidChill(std::vector<float>& dest)
    {
        Aura* aura = NaxxSpellIds::GetAnyAura(bot, {NaxxSpellIds::Chill25});
        if (!aura)
        {
            aura = botAI->GetAura("chill", bot);
        }
        if (!aura)
        {
            return false;
        }
        /*DynamicObject* dyn_obj = aura->GetDynobjOwner();
        if (!dyn_obj)*/
        // Prefer the dynobject (classic ground effect), but keep a fallback for cases where
        // the aura is applied by a moving caster (e.g. Blizzard NPC) without a dynobject.
        WorldObject* source = aura->GetDynobjOwner();
        if (!source)
        {
            //return false;
            if (Unit* caster = ObjectAccessor::GetUnit(*bot, aura->GetCasterGUID()))
            {
                source = caster;
            }
        }
        if (!source)
        {
            return false;
        }
        Unit* currentTarget = AI_VALUE(Unit*, "current target");
        float angle = 0;
        uint32 index = botAI->GetGroupSlotIndex(bot);
        if (currentTarget)
        {
            if (botAI->IsRanged(bot))
            {
                if (bot->GetExactDist2d(currentTarget) <= 45.0f)
                {
                    angle = bot->GetAngle(source) - M_PI + (rand_norm() - 0.5) * M_PI / 2;
                }
                else
                {
                    if (index % 2 == 0)
                    {
                        angle = bot->GetAngle(currentTarget) + M_PI / 2;
                    }
                    else
                    {
                        angle = bot->GetAngle(currentTarget) - M_PI / 2;
                    }
                }
            }
            else
            {
                if (index % 3 == 0)
                {
                    angle = bot->GetAngle(currentTarget);
                }
                else if (index % 3 == 1)
                {
                    angle = bot->GetAngle(currentTarget) + M_PI / 2;
                }
                else
                {
                    angle = bot->GetAngle(currentTarget) - M_PI / 2;
                }
            }
        }
        else
        {
            angle = bot->GetAngle(source) - M_PI + (rand_norm() - 0.5) * M_PI / 2;
        }
        dest = {bot->GetPositionX() + cos(angle) * 5.0f, bot->GetPositionY() + sin(angle) * 5.0f, bot->GetPositionZ()};
        return true;
    }

private:
    void Reset()
    {
        _unit = nullptr;
        _was_flying = false;
        _last_land_ms = 0;
        _last_icebolt_ms = 0;
    }

    const uint32 POSITION_TIME_AFTER_LANDED = 5000;
    const uint32 BREATH_MIN_MS = 1000;
    const uint32 BREATH_MAX_MS = 12000;
    bool HasIceboltInGroup()
    {
        Group* group = bot->GetGroup();
        if (!group)
        {
            return false;
        }
        bool hasIcebolt = false;
        for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
        {
            Player* member = ref->GetSource();
            if (!member)
            {
                continue;
            }
            if (NaxxSpellIds::HasAnyAura(botAI, member, {NaxxSpellIds::Icebolt10, NaxxSpellIds::Icebolt25}) ||
                botAI->HasAura("icebolt", member, false, false, -1, true))
            {
                hasIcebolt = true;
                break;
            }
        }
        if (hasIcebolt)
        {
            _last_icebolt_ms = getMSTime();
        }
        return hasIcebolt;
    }
    void UpdateIceboltState()
    {
        if (!IsPhaseFlight())
        {
            _last_icebolt_ms = 0;
            return;
        }
        HasIceboltInGroup();
    }
    bool IsBreathCasting()
    {
        if (!_unit || !_unit->HasUnitState(UNIT_STATE_CASTING))
        {
            return false;
        }
        Spell* spell = _unit->GetCurrentSpell(CURRENT_GENERIC_SPELL);
        if (!spell)
        {
            spell = _unit->GetCurrentSpell(CURRENT_CHANNELED_SPELL);
        }
        if (!spell)
        {
            return false;
        }
        SpellInfo const* info = spell->GetSpellInfo();
        return NaxxSpellIds::MatchesAnySpellId(info, {NaxxSpellIds::FrostMissile, NaxxSpellIds::FrostExplosion});
    }

    Unit* _unit = nullptr;
    bool _was_flying = false;
    uint32 _last_land_ms = 0;
    uint32 _last_icebolt_ms = 0;
};

class GluthBossHelper : public AiObject
{
public:
    const std::pair<float, float> mainTankPos25 = {3331.48f, -3109.06f};
    const std::pair<float, float> mainTankPos10 = {3278.29f, -3162.06f};
    const std::pair<float, float> beforeDecimatePos = {3267.34f, -3175.68f};
    const std::pair<float, float> leftSlowDownPos = {3290.68f, -3141.65f};
    const std::pair<float, float> rightSlowDownPos = {3300.78f, -3151.98f};
    const std::pair<float, float> rangedPos = {3301.45f, -3139.29f};
    const std::pair<float, float> healPos = {3303.09f, -3135.24f};

    const float decimatedZombiePct = 10.0f;
    GluthBossHelper(PlayerbotAI* botAI) : AiObject(botAI) {}
    bool UpdateBossAI()
    {
        if (!bot->IsInCombat())
        {
            Reset();
        }
        if (_unit && (!_unit->IsInWorld() || !_unit->IsAlive()))
        {
            Reset();
        }
        if (!_unit)
        {
            _unit = AI_VALUE2(Unit*, "find target", "gluth");
            if (!_unit)
            {
                return false;
            }
        }
        if (_unit->IsInCombat())
        {
            if (_combat_start_ms == 0)
            {
                _combat_start_ms = getMSTime();
            }
        }
        else
        {
            _combat_start_ms = 0;
        }
        return true;
    }
    bool BeforeDecimate()
    {
        if (!_unit || !_unit->HasUnitState(UNIT_STATE_CASTING))
        {
            return false;
        }
        Spell* spell = _unit->GetCurrentSpell(CURRENT_GENERIC_SPELL);
        if (!spell)
        {
            spell = _unit->GetCurrentSpell(CURRENT_CHANNELED_SPELL);
        }
        if (!spell)
        {
            return false;
        }
        SpellInfo const* info = spell->GetSpellInfo();
        if (!info)
        {
            return false;
        }
        if (NaxxSpellIds::MatchesAnySpellId(
                info, {NaxxSpellIds::Decimate10, NaxxSpellIds::Decimate25, NaxxSpellIds::Decimate25Alt}))
        {
            return true;
        }

        return info->SpellName[LOCALE_enUS] && botAI->EqualLowercaseName(info->SpellName[LOCALE_enUS], "decimate");
    }
    bool JustStartCombat() const { return _combat_start_ms != 0 && getMSTime() - _combat_start_ms < 10000; }
    bool IsZombieChow(Unit* unit) const { return unit && botAI->EqualLowercaseName(unit->GetName(), "zombie chow"); }

private:
    void Reset()
    {
        _unit = nullptr;
        _combat_start_ms = 0;
    }

    Unit* _unit = nullptr;
    uint32 _combat_start_ms = 0;
};

class LoathebBossHelper : public AiObject
{
public:
    const std::pair<float, float> mainTankPos = {2910.1597f, -4010.0f};
    const std::pair<float, float> rangePos = {2896.96f, -3980.61f};
    LoathebBossHelper(PlayerbotAI* botAI) : AiObject(botAI) {}
    Unit* GetBoss() const { return _unit; }
    bool UpdateBossAI()
    {
        if (!bot->IsInCombat())
        {
            Reset();
        }
        if (_unit && (!_unit->IsInWorld() || !_unit->IsAlive()))
        {
            Reset();
        }
        if (!_unit)
        {
            _unit = AI_VALUE2(Unit*, "find target", "loatheb");
        }
        return _unit != nullptr;
    }

private:
    void Reset() { _unit = nullptr; }

    Unit* _unit = nullptr;
};

class NothBossHelper : public AiObject
{
public:
    const std::pair<float, float> center = {2684.94f, -3502.53f};
    NothBossHelper(PlayerbotAI* botAI) : AiObject(botAI) {}
    bool UpdateBossAI()
    {
        if (!bot->IsInCombat())
        {
            Reset();
        }
        if (_unit && (!_unit->IsInWorld() || !_unit->IsAlive()))
        {
            Reset();
        }
        if (!_unit)
        {
            _unit = AI_VALUE2(Unit*, "find target", "noth the plaguebringer");
        }
        if (!_unit)
        {
            return false;
        }
        if (_unit->HasUnitState(UNIT_STATE_CASTING))
        {
            Spell* spell = _unit->GetCurrentSpell(CURRENT_GENERIC_SPELL);
            if (!spell)
            {
                spell = _unit->GetCurrentSpell(CURRENT_CHANNELED_SPELL);
            }
            if (spell)
            {
                SpellInfo const* info = spell->GetSpellInfo();
                bool isBlink = NaxxSpellIds::MatchesAnySpellId(info, {NaxxSpellIds::Blink});
                if (!isBlink && info && info->SpellName[LOCALE_enUS])
                {
                    isBlink = botAI->EqualLowercaseName(info->SpellName[LOCALE_enUS], "blink");
                }
                if (isBlink)
                {
                    _last_blink_ms = getMSTime();
                }
            }
        }
        return true;
    }
    bool IsBalconyPhase() const { return _unit && _unit->HasUnitFlag(UNIT_FLAG_NOT_SELECTABLE); }
    bool IsBlinkWindow() const { return _last_blink_ms != 0 && getMSTime() - _last_blink_ms < 3000; }
    bool HasCurseInGroup() const
    {
        GuidVector members = AI_VALUE(GuidVector, "group members");
        for (ObjectGuid const& guid : members)
        {
            Unit* member = botAI->GetUnit(guid);
            if (!member)
            {
                continue;
            }
            if (NaxxSpellIds::HasAnyAura(botAI, member, {NaxxSpellIds::CurseOfThePlaguebringer}) ||
                botAI->HasAura("curse of the plaguebringer", member))
            {
                return true;
            }
        }
        return false;
    }

private:
    void Reset()
    {
        _unit = nullptr;
        _last_blink_ms = 0;
    }

    Unit* _unit = nullptr;
    uint32 _last_blink_ms = 0;
};

class FourhorsemanBossHelper : public AiObject
{
public:
    const float posZ = 241.27f;
    const std::pair<float, float> attractPos[2] = {{2502.03f, -2910.90f},
                                                   {2484.61f, -2947.07f}};  // left (sir zeliek), right (lady blaumeux)
    FourhorsemanBossHelper(PlayerbotAI* botAI) : AiObject(botAI) {}
    bool UpdateBossAI()
    {
        if (!bot->IsInCombat())
        {
            Reset();
        }
        else if (_combat_start_ms == 0)
        {
            _combat_start_ms = getMSTime();
        }
        if (_sir && (!_sir->IsInWorld() || !_sir->IsAlive()))
        {
            Reset();
        }
        if (!_sir)
        {
            _sir = AI_VALUE2(Unit*, "find target", "sir zeliek");
            if (!_sir)
            {
                return false;
            }
        }
        _lady = AI_VALUE2(Unit*, "find target", "lady blaumeux");
        return true;
    }
    void Reset()
    {
        _sir = nullptr;
        _lady = nullptr;
        _combat_start_ms = 0;
        posToGo = 0;
    }
    bool IsAttracter(Player* bot)
    {
        Difficulty diff = bot->GetRaidDifficulty();
        if (diff == RAID_DIFFICULTY_25MAN_NORMAL)
        {
            return botAI->IsAssistRangedDpsOfIndex(bot, 0) || botAI->IsAssistHealOfIndex(bot, 0) ||
                   botAI->IsAssistHealOfIndex(bot, 1) || botAI->IsAssistHealOfIndex(bot, 2);
        }
        return botAI->IsAssistRangedDpsOfIndex(bot, 0) || botAI->IsAssistHealOfIndex(bot, 0);
    }
    void CalculatePosToGo(Player* bot)
    {
        bool raid25 = bot->GetRaidDifficulty() == RAID_DIFFICULTY_25MAN_NORMAL;
        Unit* lady = _lady;
        if (!lady)
        {
            posToGo = 0;
        }
        else
        {
            uint32 elapsed_ms = _combat_start_ms ? getMSTime() - _combat_start_ms : 0;
            // Interval: 24s - 15s - 15s - ...
            posToGo = !(elapsed_ms <= 9000 || ((elapsed_ms - 9000) / 67500) % 2 == 0);
            if (botAI->IsAssistRangedDpsOfIndex(bot, 0) || (raid25 && botAI->IsAssistHealOfIndex(bot, 1)))
            {
                posToGo = 1 - posToGo;
            }
        }
    }
    std::pair<float, float> CurrentAttractPos()
    {
        bool raid25 = bot->GetRaidDifficulty() == RAID_DIFFICULTY_25MAN_NORMAL;
        float posX = attractPos[posToGo].first, posY = attractPos[posToGo].second;
        if (posToGo == 1)
        {
            float offset_x = 0.0f;
            float offset_y = 0.0f;
            float bias = 4.5f;
            if (raid25)
            {
                offset_x = -bias;
                offset_y = bias;
            }
            posX += offset_x;
            posY += offset_y;
        }
        return {posX, posY};
    }
    Unit* CurrentAttackTarget()
    {
        if (posToGo == 0)
        {
            return _sir;
        }
        return _lady;
    }

protected:
    Unit* _sir = nullptr;
    Unit* _lady = nullptr;
    uint32 _combat_start_ms = 0;
    int posToGo = 0;
};
class ThaddiusBossHelper : public AiObject
{
public:
    const std::pair<float, float> tankPosFeugen = {3522.94f, -3002.60f};
    const std::pair<float, float> tankPosStalagg = {3436.14f, -2919.98f};
    const std::pair<float, float> rangedPosFeugen = {3500.45f, -2997.92f};
    const std::pair<float, float> rangedPosStalagg = {3441.01f, -2942.04f};
    const float tankPosZ = 312.61f;
    ThaddiusBossHelper(PlayerbotAI* botAI) : AiObject(botAI) {}
    bool UpdateBossAI()
    {
        if (!bot->IsInCombat())
        {
            Reset();
        }
        if (_unit && (!_unit->IsInWorld() || !_unit->IsAlive()))
        {
            Reset();
        }
        if (!_unit)
        {
            _unit = AI_VALUE2(Unit*, "find target", "thaddius");
            if (!_unit)
            {
                return false;
            }
        }
        feugen = AI_VALUE2(Unit*, "find target", "feugen");
        stalagg = AI_VALUE2(Unit*, "find target", "stalagg");
        return true;
    }
    bool IsPhasePet() { return (feugen && feugen->IsAlive()) || (stalagg && stalagg->IsAlive()); }
    bool IsPhaseTransition()
    {
        if (IsPhasePet())
        {
            return false;
        }
        return _unit && _unit->HasUnitFlag(UNIT_FLAG_NON_ATTACKABLE);
    }
    bool IsPhaseThaddius() { return !IsPhasePet() && !IsPhaseTransition(); }
    Unit* GetNearestPet()
    {
        Unit* unit = nullptr;
        if (feugen && feugen->IsAlive())
        {
            unit = feugen;
        }
        if (stalagg && stalagg->IsAlive() && (!feugen || bot->GetDistance(stalagg) < bot->GetDistance(feugen)))
        {
            unit = stalagg;
        }
        return unit;
    }
    std::pair<float, float> PetPhaseGetPosForTank()
    {
        if (GetNearestPet() == feugen)
        {
            return tankPosFeugen;
        }
        return tankPosStalagg;
    }
    std::pair<float, float> PetPhaseGetPosForRanged()
    {
        if (GetNearestPet() == feugen)
        {
            return rangedPosFeugen;
        }
        return rangedPosStalagg;
    }

protected:
    void Reset()
    {
        _unit = nullptr;
        feugen = nullptr;
        stalagg = nullptr;
    }

    Unit* _unit = nullptr;
    Unit* feugen = nullptr;
    Unit* stalagg = nullptr;
};

#endif
