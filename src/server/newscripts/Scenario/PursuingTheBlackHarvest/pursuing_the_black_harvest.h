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
#include "AchievementMgr.h"

#ifndef DEF_PURSING_THE_BLACK_HARVEST
#define DEF_PURSING_THE_BLACK_HARVEST

enum Data
{
    DATA_NONE,

    DATA_ESSENCE_OF_ORDER_EVENT,
    DATA_AKAMA,
    DATA_NOBEL_EVENT,
    DATA_KANRETHAD,
};

enum eCreatures
{
    NPC_AKAMA                       = 68137,
    NPC_ASTHONGUE_PRIMALIST         = 68096,
    NPC_ASHTONGUE_WORKER            = 68098,
    NPC_SUFFERING_SOUL_FRAGMENT     = 68139,
    NPC_HUNGERING_SOUL_FRAGMENT     = 68140,
    NPC_ESSENCE_OF_ORDER            = 68151,
    NPC_LOST_SOULS                  = 68156,
    NPC_UNBOUND_NIGHTLORD           = 68174,
    NPC_UNBOUND_CENTURION           = 68176,
    NPC_UNBOUND_BONEMENDER          = 68175,
    NPC_FREED_IMP                   = 68173,
    NPC_DEMONIC_SOULWELL            = 70052,
    NPC_KANRETHAD_EBONLOCKE         = 69964,
    NPC_DEMONIC_GATEWAY             = 70028,
    NPC_JUBEKA_SHADOWBREAKER        = 70166,
};

enum eGameObects
{
    GO_CONSPICUOUS_ILLIDARI_SCROLL  = 216364,
};

enum Actions
{
    ACTION_NONE,

    ACTION_1,
    ACTION_2,
    ACTION_3,
};

#endif
