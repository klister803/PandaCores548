/*
 * Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
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
#include "eye_of_eternity.h"

class instance_eye_of_eternity : public InstanceMapScript
{
public:
    instance_eye_of_eternity() : InstanceMapScript("instance_eye_of_eternity", 616) {}

    InstanceScript* GetInstanceScript(InstanceMap* map) const
    {
        return new instance_eye_of_eternity_InstanceMapScript(map);
    }

    struct instance_eye_of_eternity_InstanceMapScript : public InstanceScript
    {
        instance_eye_of_eternity_InstanceMapScript(Map* map) : InstanceScript(map)
        {
            SetBossNumber(MAX_ENCOUNTER);

            malygosGUID = 0;
            platformGUID = 0;
            exitPortalGUID = 0;
            chestGUID = 0;
            magicchestGUID = 0;
        };

        bool SetBossState(uint32 id, EncounterState state)
        {
            if (!InstanceScript::SetBossState(id, state))
                return false;

            if (id == DATA_MALYGOS_EVENT)
            {
                if (state == DONE)
                {
                    if (Creature* malygos = instance->GetCreature(malygosGUID))
                        malygos->SummonCreature(NPC_ALEXSTRASZA, 829.0679f, 1244.77f, 279.7453f, 2.32f);

                    if (GameObject* chest = instance->GetGameObject(chestGUID)) //Alexstrasza's Gift
                        chest->SetRespawnTime(7*DAY);

                    if (GameObject* magicchest = instance->GetGameObject(magicchestGUID)) //Heart of Magic
                        magicchest->SetRespawnTime(0.5*DAY);

                }
            }
            return true;
        }

        void OnGameObjectCreate(GameObject* go)
        {
            switch (go->GetEntry())
            {
                case GO_NEXUS_RAID_PLATFORM:
                    platformGUID = go->GetGUID();
                    break;
                case GO_EXIT_PORTAL:
                    exitPortalGUID = go->GetGUID();
                    break;
                case GO_ALEXSTRASZA_S_GIFT:
                case GO_ALEXSTRASZA_S_GIFT_2:
                    chestGUID = go->GetGUID();
                    break;
                case GO_HEART_OF_MAGIC:
                case GO_HEART_OF_MAGIC_2:
                    magicchestGUID = go->GetGUID();
                    break;
            }
        }

        void OnCreatureCreate(Creature* creature)
        {
            switch (creature->GetEntry())
            {
                case NPC_MALYGOS:
                    malygosGUID = creature->GetGUID();
                    break;
            }
        }

        void ProcessEvent(WorldObject* obj, uint32 eventId)
        {
            if (eventId == EVENT_FOCUSING_IRIS)
            {
                if (GameObject* go = obj->ToGameObject())
                    go->Delete(); // this is not the best way.

                if (Creature* malygos = instance->GetCreature(malygosGUID))
                    malygos->AI()->DoAction(ACTION_START_MALYGOS);
            }
        }

        std::string GetSaveData()
        {
            OUT_SAVE_INST_DATA;

            std::ostringstream saveStream;
            saveStream << "E E " << GetBossSaveData();

            OUT_SAVE_INST_DATA_COMPLETE;
            return saveStream.str();
        }

        void Load(const char* str)
        {
            if (!str)
            {
                OUT_LOAD_INST_DATA_FAIL;
                return;
            }

            OUT_LOAD_INST_DATA(str);

            char dataHead1, dataHead2;

            std::istringstream loadStream(str);
            loadStream >> dataHead1 >> dataHead2;

            if (dataHead1 == 'E' && dataHead2 == 'E')
            {
                for (uint8 i = 0; i < MAX_ENCOUNTER; ++i)
                {
                    uint32 tmpState;
                    loadStream >> tmpState;
                    if (tmpState == IN_PROGRESS || tmpState > SPECIAL)
                        tmpState = NOT_STARTED;
                    SetBossState(i, EncounterState(tmpState));
                }

            } else OUT_LOAD_INST_DATA_FAIL;

            OUT_LOAD_INST_DATA_COMPLETE;
        }

        private:
            uint64 malygosGUID;
            uint64 platformGUID;
            uint64 exitPortalGUID;
            uint64 chestGUID;
            uint64 magicchestGUID;
    };
};

void AddSC_instance_eye_of_eternity()
{
   new instance_eye_of_eternity();
}
