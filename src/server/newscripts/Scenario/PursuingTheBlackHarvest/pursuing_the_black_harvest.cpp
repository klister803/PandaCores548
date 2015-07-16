/*
 * Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2006-2009 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "pursuing_the_black_harvest.h"
#include "ScriptedCreature.h"

enum Texts
{ };

enum Spells
{
    //< Misc
    SPELL_PLACE_EMPOWED_SOULCORE        = 138680, //< SS
    SPELL_DRAIN_FEL_ENERGY              = 139200,

    //< S0
    SPELL_SEARCHING_FOR_INTRUDERS       = 134110, //< AT

    //< S1
    SPELL_NETTED                        = 134111,
    SPELL_BLACKOUT                      = 134112,

    //< S2
    SPELL_UPDATE_PLAYER_PHASE_AURAS     = 134209,

    //< S3 - NO SPELLS
    //< S4 - NO SPELLS

    //< S5
    SPELL_SPELLFLAME                    = 134234, //< SS
    SPELL_SPELLFLAME_TRIGGER            = 134235,
    SPELL_HELLFIRE                      = 134225,
    SPELL_HELLFIRE_TRIGGER              = 134224,

    //< S6
    SPELL_SHOOT                         = 41188,
    SPELL_MULTI_SHOOT                   = 41187,
    SPELL_CLEAVE                        = 15284,
    SPELL_SWEEPING_WING_CLIP            = 39584,
    SPELL_SONIC_STRIKE                  = 41168,
    SPELL_SHADOW_BOLT                   = 34344,
    SPELL_DAZED                         = 1604,
    SPELL_FIREBOLT                      = 134245,
    SPELL_LIGHTING_BOLT                 = 42024,

    //< S7
    SPELL_SUMMON_SHADOWFIEND            = 41159,
    SPELL_SHADOW_INFERNO                = 39646,
    SPELL_SMELT_FLASH                   = 37629,

    //< S8
    SPELL_DEMONIC_GATEWAY               = 138649, //< SS
    SPELL_BURNING_EMBERS                = 138557, //< SS
    SPELL_SOULSHARDS                    = 138556, //< SS
    SPELL_METAMORPHOSIS                 = 138555,
    SPELL_FACE_PLAYER                   = 139053, //< SS
    SPELL_RITUAL_ENSLAVEMENT            = 22987,  //< SS
    SPELL_DOOMGUARD_SUMMON_DND          = 42010,  //< SS
    SPELL_DOOM_BOLT                     = 85692,  //< SS
    SPELL_SUMMONING_PIT_LORD            = 138789, //< SS
    SPELL_FEL_FLAME_BREATH              = 138814,
    SPELL_FEL_FLAME_BREATH_DUMMY        = 138813,
    SPELL_RAID_OF_FIRE                  = 138561,
    SPELL_SEED_OF_TERRIBLE_DESTRUCTION  = 138587,
    SPELL_CLEAVE_2                      = 138794,
    SPELL_AURA_OF_OMNIPOTENCE           = 138563,
    SPELL_CURSE_OF_ULTIMATE_DOOM        = 138558,
    SPELL_EXCRUCIATING_AGONY            = 138560,
    SPELL_DEMONIC_SIPHON                = 138829,
    SPELL_CHAOS_BOLT                    = 138559,
    SPELL_BACKFIRE                      = 138619,
    SPELL_SOULFIRE                      = 138554,
    SPELL_CATACLYSM                     = 138564,
    SPELL_CHARGE                        = 138796,
    SPELL_CHARGE_TRIGGER                = 138827,
    SPELL_FEL_FIREBOLT                  = 138747,

    //< Last Step
    SPELL_ANNIHILATE_DEMONS             = 139141,
    SPELL_DEMONIC_GRASP                 = 139142,
    SPELL_ETERNAL_BANISHMENT            = 139186,

};

enum Events
{
    EVENT_NONE,
};

enum Actions
{
    ACTION_NONE,

};

enum Sounds
{ };

class at_stage_8696: public AreaTriggerScript
{
public:
    at_stage_8696() : AreaTriggerScript("at_stage_8696") { }

    bool OnTrigger(Player* player, AreaTriggerEntry const* /*at*/, bool enter)
    {
        if (enter)
        {
            player->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_SCRIPT_EVENT_2, 34539, 1);
            return true;
        }
        return false;
    }
};

class at_stage_8698: public AreaTriggerScript
{
public:
    at_stage_8698() : AreaTriggerScript("at_stage_8698") { }

    bool OnTrigger(Player* player, AreaTriggerEntry const* /*at*/, bool enter)
    {
        if (enter)
        {
            player->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_SCRIPT_EVENT, 34547, 1);
            return true;
        }
        return false;
    }
};

void AddSC_pursing_the_black_harvest()
{
    new at_stage_8696();
    new at_stage_8698();
}
