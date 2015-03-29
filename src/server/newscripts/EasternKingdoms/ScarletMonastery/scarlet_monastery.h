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

#ifndef DEF_SCARLET_M
#define DEF_SCARLET_M

enum eEnums
{
    DATA_THALNOS                = 1,
    DATA_KORLOFF                = 2,
    DATA_WHITEMANE              = 3,
    MAX_ENCOUNTER,

    DATA_HORSEMAN_EVENT         = 4,
    GAMEOBJECT_PUMPKIN_SHRINE   = 5,
};

enum eCreatures
{
    NPC_HORSEMAN            = 23682,
    NPC_HEAD                = 23775,
    NPC_PUMPKIN             = 23694,
    NPC_THALNOS             = 59789,
    
    //Summons Thalnos
    NPC_EVICTED_SOUL        = 59974,
    NPC_EMPOWERING_SPIRIT   = 59893,
    NPC_FALLEN_CRUSADER     = 59884,
};

enum eGameObects
{
    GO_THALNOS_DOOR      = 211844,
    GO_PUMPKIN_SHRINE    = 186267,
};
#endif
