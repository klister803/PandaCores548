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

#include "NewScriptPCH.h"
#include "SpellScript.h"
#include "Map.h"
#include "Creature.h"
#include "CreatureAIImpl.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "MoveSplineInit.h"

#ifndef DEF_FALL_OF_SHANBU
#define DEF_FALL_OF_SHANBU

enum Data
{
    DATA_NONE,

    DATA_TRUNDER_FORGE_DOOR,
    DATA_START_EVENT,
    DATA_EVENT_PART_1,
    DATA_EVENT_PART_2,
    DATA_JUMP_POS,
    DATA_COMPLETE_EVENT_STAGE_1,
    DATA_SUMMONS_COUNTER,

    DATA_LR_START,
    DATA_LR_STAGE_2,
};

enum eCreatures
{
    NPC_WRATHION                        = 70100,
    NPC_SHADO_PAN_WARRIOR               = 70106, //< x2
    NPC_SHADO_PAN_DEFENDER              = 70099,

    NPC_THUNDER_FORGE                   = 70577,
    NPC_FORGEMASTER_VULKON              = 70074,

    NPC_SHANZE_SHADOWCASTER             = 69827,
    NPC_SHANZE_WARRIOR                  = 69833,
    NPC_SHANZE_BATTLEMASTER             = 69835,
    NPC_SHANZE_ELECTRO_COUTIONER        = 70070,
    NPC_SHANZE_PYROMANCER               = 69824,

    NPC_INVISIBLE_STALKER               = 62142,
    NPC_SPIRIT_HEALER                   = 65183,
    NPC_JUVENILE_SKYSCREAMER            = 69162,
    NPC_ZANALARI_COMMONER               = 69170,
    NPC_DRAKKARI_GOD_HULK               = 69200,
    NPC_MANFRED                         = 69217,
    NPC_ZANALARI_STONE_SHIELD           = 69223,
    NPC_FIERCE_ANKLEBITER               = 69244,
    NPC_ZANDALARI_PROSPECT              = 69269,
    NPC_ZANDALARI_BEASTCALLER           = 69379,
    NPC_ZANDALARI_BEASTCALLER2          = 69397,
    NPC_JUVENILE_SKYSCREAMER2           = 69404,
    NPC_ZANDALARI_BEASTCALLER3          = 69412,
    NPC_LORTHEMAR_THERON                = 69481,
    NPC_LIGHTING_PILAR_BEAM_STALKER     = 69798,
    NPC_LIGHTING_PILAR_SPARK_STALKER    = 69813,
    NPC_SLAVEMASTER_SHIAXU              = 69923,
    NPC_SCOUT_CAPTAIN_ELSIA             = 70042,
    NPC_THUNDER_FORGE2                  = 70283,
    NPC_THUNDER_FORGE_CRUCIBLE          = 70556,
    
    //< VEH
    NPC_ZANDALARI_SKYSCREAMER           = 69156,
    NPC_ZANDALARI_SKYSCREAMER2          = 69411,
    NPC_THUNDERWING                     = 69509,

    //< second room
    NPC_CELESTIAL_BLACKSMITH            = 69828,
    NPC_CELESTIAL_DEFENDER              = 69837,

};

enum eGameObects
{
    GO_MOGU_CRICUBLE            = 218910,
    GO_THUNDER_FORGE_DOOR       = 218832,
};

template<class AI>
CreatureAI* GetInstanceAI(Creature* creature)
{
    if (InstanceMap* instance = creature->GetMap()->ToInstanceMap())
        if (instance->GetInstanceScript())
            if (instance->GetScriptId() == sObjectMgr->GetScriptId("instance_fall_of_shan_bu"))
                return new AI(creature);
    return NULL;
}

void AttakersCounter(Creature* me, InstanceScript* instance);

#endif
