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

class instance_scarlet_monastery : public InstanceMapScript
{
public:
    instance_scarlet_monastery() : InstanceMapScript("instance_scarlet_monastery", 1004) { }

    InstanceScript* GetInstanceScript(InstanceMap* map) const
    {
        return new instance_scarlet_monastery_InstanceMapScript(map);
    }

    struct instance_scarlet_monastery_InstanceMapScript : public InstanceScript
    {
        instance_scarlet_monastery_InstanceMapScript(Map* map) : InstanceScript(map) {}

        uint64 PumpkinShrineGUID;
        uint64 HorsemanGUID;
        uint64 HeadGUID;
        std::set<uint64> HorsemanAdds;

        uint32 encounter[MAX_ENCOUNTER];

        void Initialize()
        {
            memset(&encounter, 0, sizeof(encounter));

            PumpkinShrineGUID  = 0;
            HorsemanGUID = 0;
            HeadGUID = 0;
            HorsemanAdds.clear();
        }

        void OnGameObjectCreate(GameObject* go)
        {
            switch (go->GetEntry())
            {
                case ENTRY_PUMPKIN_SHRINE: PumpkinShrineGUID = go->GetGUID();break;
                default:
                    break;
            }
        }

        void OnCreatureCreate(Creature* creature)
        {
            switch (creature->GetEntry())
            {
                case ENTRY_HORSEMAN:    HorsemanGUID = creature->GetGUID(); break;
                case ENTRY_HEAD:        HeadGUID = creature->GetGUID(); break;
                case ENTRY_PUMPKIN:     HorsemanAdds.insert(creature->GetGUID());break;
            }
        }

        void SetData(uint32 type, uint32 data)
        {
            switch (type)
            {
            case GAMEOBJECT_PUMPKIN_SHRINE:
                HandleGameObject(PumpkinShrineGUID, false);
                break;
            case DATA_HORSEMAN_EVENT:
                encounter[0] = data;
                if (data == DONE)
                {
                    for (std::set<uint64>::const_iterator itr = HorsemanAdds.begin(); itr != HorsemanAdds.end(); ++itr)
                    {
                        Creature* add = instance->GetCreature(*itr);
                        if (add && add->isAlive())
                            add->Kill(add);
                    }
                    HorsemanAdds.clear();
                    HandleGameObject(PumpkinShrineGUID, false);
                }
                break;
            }
        }

        uint64 GetData64(uint32 type)
        {
            switch (type)
            {
                case GAMEOBJECT_PUMPKIN_SHRINE:   return PumpkinShrineGUID;
                case ENTRY_HORSEMAN:              return HorsemanGUID;
                case ENTRY_HEAD:                  return HeadGUID;
            }
            return 0;
        }

        uint32 GetData(uint32 type)
        {
            if (type == DATA_HORSEMAN_EVENT)
                return encounter[0];
            return 0;
        }
    };
};

void AddSC_instance_scarlet_monastery()
{
    new instance_scarlet_monastery();
}