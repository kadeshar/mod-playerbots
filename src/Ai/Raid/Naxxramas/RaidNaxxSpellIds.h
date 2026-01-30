#ifndef _PLAYERBOT_RAIDNAXXSPELLIDS_H
#define _PLAYERBOT_RAIDNAXXSPELLIDS_H

#include <initializer_list>

#include "PlayerbotAI.h"

// use src/server/scripts/Northrend/Naxxramas/naxxramas.h for CreatureId, NaxxramasSay, NaxxramasEvent, NaxxramasMisc
namespace NaxxSpellIds
{
    // Grand Widow Faerlina
    static constexpr uint32 FaerlinaFrenzy = 28798;
    static constexpr uint32 FaerlinaWidowsEmbrace = 28732;

    // Maexxna
    static constexpr uint32 MaexxnaWebWrapStun = 28622;
    static constexpr uint32 MaexxnaWebWrapEntry = 16486;
    static constexpr uint32 MaexxnaSpiderlingEntry = 17055;

    // Gothik the Harvester
    static constexpr float GothikGateY = -3360.78f;
    static constexpr uint32 GothikLivingTraineeEntry = 16124;
    static constexpr uint32 GothikLivingKnightEntry  = 16125;
    static constexpr uint32 GothikLivingRiderEntry   = 16126;
    static constexpr uint32 GothikDeadTraineeEntry   = 16127;
    static constexpr uint32 GothikDeadKnightEntry    = 16148;
    static constexpr uint32 GothikDeadHorseEntry     = 16149;
    static constexpr uint32 GothikDeadRiderEntry     = 16150;

    // Heigan
    static constexpr uint32 Eruption10 = 29371;
/*
    SPELL_SPELL_DISRUPTION          = 29310,
    SPELL_DECREPIT_FEVER            = 29998,
    SPELL_PLAGUE_CLOUD              = 29350,
    SPELL_TELEPORT_SELF             = 30211
*/

    // Grobbulus
    static constexpr uint32 PoisonCloud = 28240;

    // Noth the Plaguebringer
    static constexpr uint32 CurseOfThePlaguebringer = 29213;
    static constexpr uint32 Cripple = 29212;
    static constexpr uint32 Blink = 29208;

    // Thaddius polarity
    static constexpr uint32 PositiveCharge10 = 28059;
    static constexpr uint32 PositiveCharge25 = 28062;
    static constexpr uint32 PositiveChargeStack = 29659;
    static constexpr uint32 NegativeCharge10 = 28084;
    static constexpr uint32 NegativeCharge25 = 28085;
    static constexpr uint32 NegativeChargeStack = 29660;
/*
    SPELL_MAGNETIC_PULL                 = 28337,
    SPELL_TESLA_SHOCK                   = 28099,
    SPELL_SHOCK_VISUAL                  = 28159,

    // Stalagg
    SPELL_POWER_SURGE                   = 54529,
    SPELL_STALAGG_CHAIN                 = 28096,

    // Feugen
    SPELL_STATIC_FIELD                  = 28135,
    SPELL_FEUGEN_CHAIN                  = 28111,

    // Thaddius
    SPELL_POLARITY_SHIFT                = 28089,
    SPELL_BALL_LIGHTNING                = 28299,
    SPELL_CHAIN_LIGHTNING               = 28167,
    SPELL_BERSERK                       = 27680,
    SPELL_THADDIUS_VISUAL_LIGHTNING     = 28136,
    SPELL_THADDIUS_SPAWN_STUN           = 28160,

    SPELL_POSITIVE_CHARGE               = 28062,
    SPELL_POSITIVE_CHARGE_STACK         = 29659,
    SPELL_NEGATIVE_CHARGE               = 28085,
    SPELL_NEGATIVE_CHARGE_STACK         = 29660,
    SPELL_POSITIVE_POLARITY             = 28059,
    SPELL_NEGATIVE_POLARITY             = 28084
*/
    // Sapphiron
    static constexpr uint32 Icebolt10 = 28522;
    static constexpr uint32 Icebolt25 = 28526;
    static constexpr uint32 Chill25 = 55699;
    static constexpr uint32 LifeDrain = 28542;
    static constexpr uint32 FrostMissile = 30101;
    static constexpr uint32 FrostExplosion = 28524;

    // Kel'Thuzad
    static constexpr uint32 FrostBlast = 27808;
    static constexpr uint32 DetonateMana = 27819;
    static constexpr uint32 ChainsOfKelthuzad = 28410;

    // Gluth
    static constexpr uint32 Decimate10 = 28374;
    static constexpr uint32 Decimate25 = 54426;
    static constexpr uint32 Decimate25Alt = 28375;
    static constexpr uint32 MortalWound10 = 25646;
    static constexpr uint32 MortalWound25 = 54378;
/*
    SPELL_MORTAL_WOUND                  = 25646,
    SPELL_ENRAGE                        = 28371,
    SPELL_DECIMATE                      = 28374,
    SPELL_DECIMATE_DAMAGE               = 28375,
    SPELL_BERSERK                       = 26662,
    SPELL_INFECTED_WOUND                = 29306,
    SPELL_CHOW_SEARCHER                 = 28404
*/
    // Anub'Rekhan
    static constexpr uint32 LocustSwarm10 = 28785;
    static constexpr uint32 LocustSwarm10Alt = 28786;
    static constexpr uint32 LocustSwarm25 = 54021;  // 25-man Locust Swarm

    // Loatheb
    static constexpr uint32 NecroticAura10 = 55593;

    inline bool HasAnyAura(PlayerbotAI* botAI, Unit* unit, std::initializer_list<uint32> spellIds)
    {
        if (!botAI || !unit)
        {
            return false;
        }

        for (uint32 spellId : spellIds)
        {
            if (botAI->HasAura(spellId, unit))
            {
                return true;
            }
        }
        return false;
    }

    inline Aura* GetAnyAura(Unit* unit, std::initializer_list<uint32> spellIds)
    {
        if (!unit)
        {
            return nullptr;
        }

        for (uint32 spellId : spellIds)
        {
            if (Aura* aura = unit->GetAura(spellId))
            {
                return aura;
            }
        }
        return nullptr;
    }

    inline bool MatchesAnySpellId(SpellInfo const* info, std::initializer_list<uint32> spellIds)
    {
        if (!info)
        {
            return false;
        }

        for (uint32 spellId : spellIds)
        {
            if (info->Id == spellId)
            {
                return true;
            }
        }
        return false;
    }
}  // namespace NaxxSpellIds

#endif