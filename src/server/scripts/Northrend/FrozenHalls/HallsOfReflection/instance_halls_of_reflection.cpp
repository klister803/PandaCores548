/*
 * Copyright (C) 2008-2011 TrinityCore <http://www.trinitycore.org/>
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

#include "ScriptPCH.h"
#include "halls_of_reflection.h"
#include "MapManager.h"
#include "Transport.h"

/* Halls of Reflection encounters:
0- Falric
1- Marwyn
2- Frostworn General
3- The Lich King
*/

enum Events
{
    EVENT_NONE,
    EVENT_START_LICH_KING,
};


class instance_halls_of_reflection : public InstanceMapScript
{
public:
    instance_halls_of_reflection() : InstanceMapScript("instance_halls_of_reflection", 668) { }

    InstanceScript* GetInstanceScript(InstanceMap* pMap) const
    {
        return new instance_halls_of_reflection_InstanceMapScript(pMap);
    }

    struct instance_halls_of_reflection_InstanceMapScript : public InstanceScript
    {
        instance_halls_of_reflection_InstanceMapScript(Map* pMap) : InstanceScript(pMap) {};

        uint64 uiFalric;
        uint64 uiMarwyn;
        uint64 uiLichKing;
        uint64 uiJainaPart1;
        uint64 uiSylvanasPart1;
        uint64 uiLider;
        uint64 uiCaptain;

        uint64 uiChest;
        uint64 uiPortal;

        uint64 uiFrostmourne;
        uint64 uiFrontDoor;
        uint64 uiFrostwornDoor;
        uint64 uiArthasDoor;
        uint64 uiRunDoor;
        uint64 uiWall[4];
        uint64 uiWallID[4];
        uint64 uiCaveDoor;

        uint32 uiEncounter[MAX_ENCOUNTER];
        uint32 uiTeamInInstance;
        uint32 uiIntroDone;
        uint32 uiSummons;
        uint32 uiDataPhase;

        EventMap events;
        bool isLoaded;

        void Initialize()
        {
            events.Reset();

            uiFalric = 0;
            uiMarwyn = 0;
            uiLichKing = 0;
            uiJainaPart1 = 0;
            uiSylvanasPart1 = 0;
            uiLider = 0;
            uiCaptain = 0;
            uiChest = 0;
            uiPortal = 0;

            uiFrostmourne = 0;
            uiArthasDoor = 0;
            uiFrostwornDoor = 0;
            uiFrontDoor = 0;
            uiCaveDoor = 0;
            uiTeamInInstance = 0;
            uiIntroDone = 0;
            isLoaded = false;
            
            for (uint8 i = 0; i < 4; ++i)
            {
                uiWall[i] = 0;
                uiWallID[i] = 0;
            }

            for (uint8 i = 0; i < MAX_ENCOUNTER; ++i)
                uiEncounter[i] = NOT_STARTED;
        }

        void OpenDoor(uint64 guid)
        {
            if(!guid) return;
            GameObject* go = instance->GetGameObject(guid);
            if(go) go->SetGoState(GO_STATE_ACTIVE);
        }

        void CloseDoor(uint64 guid)
        {
            if(!guid) return;
            GameObject* go = instance->GetGameObject(guid);
            if(go) go->SetGoState(GO_STATE_READY);
        }

        void OnCreatureCreate(Creature* creature)
        {
            Map::PlayerList const &players = instance->GetPlayers();
            if (!players.isEmpty())
                if (Player* player = players.begin()->getSource())
                    uiTeamInInstance = player->GetTeam();

            switch(creature->GetEntry())
            {
                case NPC_FALRIC:
                    uiFalric = creature->GetGUID();
                    break;
                case NPC_MARWYN:
                    uiMarwyn = creature->GetGUID();
                    break;
                case NPC_LICH_KING_EVENT:
                    break;
                case NPC_JAINA_PART1:
                    if (uiTeamInInstance == HORDE)
                        creature->UpdateEntry(NPC_SYLVANAS_PART1, HORDE);                    
                    uiJainaPart1 = creature->GetGUID();
                    uiSylvanasPart1 = creature->GetGUID();                    
                    break;
                case NPC_SYLVANAS_PART1:
                    uiSylvanasPart1 = creature->GetGUID();
                    break;
                case NPC_FROSTWORN_GENERAL:
                    creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                    break;
                case NPC_JAINA_OUTRO:
                    if (uiTeamInInstance == HORDE)
                        creature->UpdateEntry(NPC_SYLVANA_OUTRO, HORDE);
                    creature->SetHealth(252000);
                    uiLider = creature->GetGUID();
                    break;
                case BOSS_LICH_KING:
                    creature->SetHealth(20917000);
                    uiLichKing = creature->GetGUID();
                    break;        
                case NPC_BARTLETT:
                case NPC_KORM:
                    uiCaptain = creature->GetGUID();
                    break;
            }
        }

        void OnGameObjectCreate(GameObject* go)
        {
            // TODO: init state depending on encounters
            switch(go->GetEntry())
            {
                case GO_FROSTMOURNE:
                    uiFrostmourne = go->GetGUID();
                    go->SetFlag(GAMEOBJECT_FLAGS, GO_FLAG_INTERACT_COND);
                    HandleGameObject(0, false, go);
                    break;
                case GO_FROSTMOURNE_ALTAR:
                    go->SetFlag(GAMEOBJECT_FLAGS, GO_FLAG_INTERACT_COND);
                    HandleGameObject(0, true, go);
                    break;
                case GO_FRONT_DOOR:
                    uiFrontDoor = go->GetGUID();
                    go->SetFlag(GAMEOBJECT_FLAGS, GO_FLAG_INTERACT_COND);
                    OpenDoor(uiFrontDoor);
                    break;
                case GO_FROSTWORN_DOOR:
                    uiFrostwornDoor = go->GetGUID();
                    go->SetFlag(GAMEOBJECT_FLAGS, GO_FLAG_INTERACT_COND);

                    if (uiEncounter[1] == DONE)
                        OpenDoor(uiFrostwornDoor);
                    else
                        CloseDoor(uiFrostwornDoor);
                    break;
                case GO_RUN_DOOR:
                    uiRunDoor = go->GetGUID();
                    break;
                case GO_ARTHAS_DOOR:
                    uiArthasDoor = go->GetGUID();
                    break;
                case GO_ICE_WALL_1:
                    uiWallID[0] = go->GetGUID();
                    break;
                case GO_ICE_WALL_2:
                    uiWallID[1] = go->GetGUID();
                    break;
                case GO_ICE_WALL_3:
                    uiWallID[2] = go->GetGUID();
                    break;
                case GO_ICE_WALL_4:
                    uiWallID[3] = go->GetGUID();
                    break;
                case GO_CAVE:
                    uiCaveDoor = go->GetGUID();
                    break;
                case GO_CAPTAIN_CHEST_1:
                   // go->SetPhaseMask(2, true);
                    if (!instance->IsHeroic() && uiTeamInInstance == HORDE)
                        uiChest = go->GetGUID();
                    break;
                case GO_CAPTAIN_CHEST_3:
                   //go->SetPhaseMask(2, true);
                    if (instance->IsHeroic() && uiTeamInInstance == HORDE)
                        uiChest = go->GetGUID();
                    break;
                case GO_CAPTAIN_CHEST_2:
                   //go->SetPhaseMask(2, true);
                    if (!instance->IsHeroic() && uiTeamInInstance == ALLIANCE)
                        uiChest = go->GetGUID();
                    break;
                case GO_CAPTAIN_CHEST_4:
                   // go->SetPhaseMask(2, true);
                    if (instance->IsHeroic() && uiTeamInInstance == ALLIANCE)
                        uiChest = go->GetGUID();
                    break;
                case GO_PORTAL:
                    //go->SetPhaseMask(2, true);
                    uiPortal = go->GetGUID();
                    break;
            }
        }

        void SetData(uint32 type, uint32 data)
        {
            switch(type)
            {
                case DATA_INTRO_EVENT:
                    uiIntroDone = data;
                    break;
                case DATA_FALRIC_EVENT:
                    uiEncounter[0] = data;
                    break;
                case DATA_MARWYN_EVENT:
                    uiEncounter[1] = data;
                    if (data == DONE)
                    {
                        OpenDoor(uiFrostwornDoor);
                        OpenDoor(uiFrontDoor);
                    }
                    break;
                case DATA_FROSWORN_EVENT:
                    uiEncounter[2] = data;
                    if (data == DONE)
                    {
                        OpenDoor(uiArthasDoor);
                        SetData(DATA_PHASE, 3);
                        instance->SummonCreature(BOSS_LICH_KING, OutroSpawns[0]);
                        instance->SummonCreature(NPC_JAINA_OUTRO, OutroSpawns[1]);
                    }
                    break;
                case DATA_LICHKING_EVENT:
                    uiEncounter[3] = data;
                    if(data == IN_PROGRESS)
                    {
                        OpenDoor(uiRunDoor);

                        if(instance->IsHeroic())
                            DoStartTimedAchievement(ACHIEVEMENT_TIMED_TYPE_EVENT, ACHIEV_NOT_RETREATING_EVENT);
                    }
                    if(data == FAIL)
                    {
                        for(uint8 i = 0; i<4; i++)
                            OpenDoor(uiWallID[i]);

                        CloseDoor(uiRunDoor);
                        OpenDoor(uiFrontDoor);

                        if(Creature* pLichKing = instance->GetCreature(uiLichKing))
                            pLichKing->DespawnOrUnsummon(10000);
                        if(Creature* pLider = instance->GetCreature(uiLider))
                            pLider->DespawnOrUnsummon(10000);

                        DoStopTimedAchievement(ACHIEVEMENT_TIMED_TYPE_EVENT, ACHIEV_NOT_RETREATING_EVENT);
                        DoCastSpellOnPlayers(67375); // Kill all players

                        SetData(DATA_PHASE, 3);
                        instance->SummonCreature(BOSS_LICH_KING, OutroSpawns[0]);
                        instance->SummonCreature(NPC_JAINA_OUTRO, OutroSpawns[1]);
                    }
                    if(data == DONE)
                    {
                        if(GameObject *pChest = instance->GetGameObject(uiChest))
                            pChest->SetPhaseMask(1, true);
                        if(GameObject *pPortal = instance->GetGameObject(uiPortal))
                            pPortal->SetPhaseMask(1, true);

                        OpenDoor(uiFrontDoor);

                        if(instance->IsHeroic())
                        {
                            DoCastSpellOnPlayers(SPELL_ACHIEV_CHECK);
                            DoStopTimedAchievement(ACHIEVEMENT_TIMED_TYPE_EVENT, ACHIEV_NOT_RETREATING_EVENT);
                        }
                    }
                    break;
                case DATA_SUMMONS:
                    if (data == 3) uiSummons = 0;
                    else if (data == 1) ++uiSummons;
                    else if (data == 0) --uiSummons;
                    data = NOT_STARTED;
                    break;
                case DATA_ICE_WALL_1:
                    uiWall[0] = data;
                    break;
                case DATA_ICE_WALL_2:
                    uiWall[1] = data;
                    break;
                case DATA_ICE_WALL_3:
                    uiWall[2] = data;
                    break;
                case DATA_ICE_WALL_4:
                    uiWall[3] = data;
                    break;
                case DATA_PHASE:
                    uiDataPhase = data;
                    break;
            }

            if (data == DONE)
                SaveToDB();
        }

        uint32 GetData(uint32 type)
        {
            switch(type)
            {
                case DATA_INTRO_EVENT:          return uiIntroDone;
                case DATA_TEAM_IN_INSTANCE:     return uiTeamInInstance;

                case DATA_FALRIC_EVENT:         return uiEncounter[0];
                case DATA_MARWYN_EVENT:         return uiEncounter[1];

                case DATA_FROSWORN_EVENT:       return uiEncounter[2];

                case DATA_LICHKING_EVENT:       return uiEncounter[3];
                case DATA_ICE_WALL_1:           return uiWall[0];
                case DATA_ICE_WALL_2:           return uiWall[1];
                case DATA_ICE_WALL_3:           return uiWall[2];
                case DATA_ICE_WALL_4:           return uiWall[3];
                case DATA_SUMMONS:              return uiSummons;

                case DATA_PHASE:                return uiDataPhase;
            }

            return 0;
        }

        uint64 GetData64(uint32 identifier)
        {
            switch(identifier)
            {
                case DATA_FALRIC:               return uiFalric;
                case DATA_MARWYN:               return uiMarwyn;
                case DATA_LICHKING:             return uiLichKing;
                case DATA_ESCAPE_LIDER:         return uiLider;
                case DATA_FROSTMOURNE:          return uiFrostmourne;
                case DATA_FRONT_DOOR:           return uiFrontDoor;
                case DATA_FROSTWORN_DOOR:       return uiFrostwornDoor;
                case DATA_ARTHAS_DOOR:          return uiArthasDoor;
                case GO_ICE_WALL_1:             return uiWallID[0];
                case GO_ICE_WALL_2:             return uiWallID[1];
                case GO_ICE_WALL_3:             return uiWallID[2];
                case GO_ICE_WALL_4:             return uiWallID[3];
                case GO_CAVE:                   return uiCaveDoor;
                case DATA_CAPTAIN:              return uiCaptain;
                case GO_CAPTAIN_CHEST_1:
                case GO_CAPTAIN_CHEST_2:
                case GO_CAPTAIN_CHEST_3:
                case GO_CAPTAIN_CHEST_4:        return uiChest;
            }

            return 0;
        }

        std::string GetSaveData()
        {
            OUT_SAVE_INST_DATA;

            std::ostringstream saveStream;
            saveStream << "H R 1 " << uiEncounter[0] << " " << uiEncounter[1] << " " << uiEncounter[2] << " " << uiEncounter[3] << " " << uiIntroDone;

            OUT_SAVE_INST_DATA_COMPLETE;
            return saveStream.str();
        }

        void Load(const char* in)
        {
            if (!in)
            {
                OUT_LOAD_INST_DATA_FAIL;
                return;
            }

            OUT_LOAD_INST_DATA(in);

            char dataHead1, dataHead2;
            uint16 version;
            uint16 data0, data1, data2, data3, data4;

            std::istringstream loadStream(in);
            loadStream >> dataHead1 >> dataHead2 >> version >> data0 >> data1 >> data2 >> data3 >> data4;

            if (dataHead1 == 'H' && dataHead2 == 'R')
            {
                uiEncounter[0] = data0;
                uiEncounter[1] = data1;
                uiEncounter[2] = data2;
                uiEncounter[3] = data3;
                uiIntroDone = data4;

                for (uint8 i = 0; i < MAX_ENCOUNTER; ++i)
                    if (uiEncounter[i] == IN_PROGRESS)
                        uiEncounter[i] = NOT_STARTED;

                OpenDoor(uiFrontDoor);
                if (uiEncounter[1] == DONE)
                    OpenDoor(uiFrostwornDoor);
                if (uiEncounter[2] == DONE)
                    OpenDoor(uiArthasDoor);

            } else OUT_LOAD_INST_DATA_FAIL;

            OUT_LOAD_INST_DATA_COMPLETE;
        }

        void Update(uint32 diff)
        {
            
            if (!instance->HavePlayers())
                return;

            events.Update(diff);

        }
        
        void OnPlayerEnter(Player* player)
        {

            LoadGunship(); // Spawn Gunship
        }
        
        void LoadGunship()
        {
            if (isLoaded)
                return;
        
            /*if(uiTeamInInstance == ALLIANCE)
            {
                if(Transport* th = sMapMgr->LoadTransportInMap(instance, THE_SKYBREAKER, 23970))
                {
                    th->AddNPCPassengerInInstance(NPC_BARTLETT, -3.45774f, 10.2614f, 20.4492f, 1.53856f);
                }
            }
        
            if(uiTeamInInstance == HORDE)
            {
                if(Transport* th = sMapMgr->LoadTransportInMap(instance, ORGRIMS_HAMMER, 23970))
                {
                    th->AddNPCPassengerInInstance(NPC_KORM, 11.7784f, 19.0924f, 34.8982f, 1.53856f);
                }
            }*/
            isLoaded = true;
        }
    };

};


void AddSC_instance_halls_of_reflection()
{
    new instance_halls_of_reflection();
}