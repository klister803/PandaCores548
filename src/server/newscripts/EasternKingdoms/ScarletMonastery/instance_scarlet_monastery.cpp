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

/* ScriptData
SDName: Instance_Scarlet_Monastery
SD%Complete: 50
SDComment:
SDCategory: Scarlet Monastery
EndScriptData */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "scarlet_monastery.h"

//186269
//186314

#define MAX_ENCOUNTER 1

class instance_scarlet_monastery_old : public InstanceMapScript
{
public:
    instance_scarlet_monastery_old() : InstanceMapScript("instance_scarlet_monastery_old", 189) { }

    InstanceScript* GetInstanceScript(InstanceMap* map) const
    {
        return new instance_scarlet_monastery_InstanceMapScript(map);
    }

    struct instance_scarlet_monastery_InstanceMapScript : public InstanceScript
    {
        instance_scarlet_monastery_InstanceMapScript(Map* map) : InstanceScript(map) {}

        uint64 MograineGUID;
        uint64 WhitemaneGUID;
        uint64 VorrelGUID;
        uint64 DoorHighInquisitorGUID;

        uint32 encounter[MAX_ENCOUNTER];

        void Initialize()
        {
            memset(&encounter, 0, sizeof(encounter));

            MograineGUID = 0;
            WhitemaneGUID = 0;
            VorrelGUID = 0;
            DoorHighInquisitorGUID = 0;
        }

        void OnGameObjectCreate(GameObject* go)
        {
            switch (go->GetEntry())
            {
            case 104600: DoorHighInquisitorGUID = go->GetGUID(); break;
            }
        }

        void OnCreatureCreate(Creature* creature)
        {
            switch (creature->GetEntry())
            {
                case 3976: MograineGUID = creature->GetGUID(); break;
                case 3977: WhitemaneGUID = creature->GetGUID(); break;
                case 3981: VorrelGUID = creature->GetGUID(); break;
            }
        }

        void SetData(uint32 type, uint32 data)
        {
            switch (type)
            {
            case TYPE_MOGRAINE_AND_WHITE_EVENT:
                if (data == IN_PROGRESS)
                    DoUseDoorOrButton(DoorHighInquisitorGUID);
                if (data == FAIL)
                    DoUseDoorOrButton(DoorHighInquisitorGUID);

                encounter[0] = data;
                break;
            }
        }

        uint64 GetData64(uint32 type)
        {
            switch (type)
            {
                case DATA_MOGRAINE:             return MograineGUID;
                case DATA_WHITEMANE:            return WhitemaneGUID;
                case DATA_VORREL:               return VorrelGUID;
                case DATA_DOOR_WHITEMANE:       return DoorHighInquisitorGUID;
            }
            return 0;
        }

        uint32 GetData(uint32 type)
        {
            if (type == TYPE_MOGRAINE_AND_WHITE_EVENT)
                return encounter[0];
            return 0;
        }
    };
};

void AddSC_instance_scarlet_monasteryOld()
{
    new instance_scarlet_monastery_old();
}
