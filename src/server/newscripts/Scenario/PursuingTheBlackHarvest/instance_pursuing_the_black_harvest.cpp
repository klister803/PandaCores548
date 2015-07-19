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

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "pursuing_the_black_harvest.h"

class instance_pursuing_the_black_harvest : public InstanceMapScript
{
public:
    instance_pursuing_the_black_harvest() : InstanceMapScript("instance_pursuing_the_black_harvest", 1112) { }

    InstanceScript* GetInstanceScript(InstanceMap* map) const
    {
        return new instance_pursuing_the_black_harvest_InstanceMapScript(map);
    }

    struct instance_pursuing_the_black_harvest_InstanceMapScript : public InstanceScript
    {
        instance_pursuing_the_black_harvest_InstanceMapScript(Map* map) : InstanceScript(map)
        { }

        void Initialize()
        {
            essenceData = 0;
            akamaGUID = 0;
            nodelData = 0;
        }

        void OnCreatureCreate(Creature* creature)
        {
            switch (creature->GetEntry())
            {
                case NPC_ESSENCE_OF_ORDER:
                    creature->SetVisible(false);
                    break;
                case NPC_AKAMA:
                    akamaGUID = creature->GetGUID();
                    break;
                default:
                    break;
            }
        }

        void SetData(uint32 type, uint32 data)
        {
            switch (type)
            {
                case DATA_ESSENCE_OF_ORDER_EVENT:
                    essenceData = data;
                    if (data == DONE)
                        if (Creature* akama = instance->GetCreature(DATA_AKAMA))
                            akama->AI()->DoAction(ACTION_3);
                    break;
                case DATA_AKAMA:
                    nodelData = data;
                    break;
                default:
                    break;
            }
        }

        uint64 GetData64(uint32 type)
        {
            switch (type)
            {
                case DATA_AKAMA:
                    return akamaGUID;
                default:
                    return 0;
            }
        }

        uint32 GetData(uint32 type)
        {
            switch (type)
            {
                case DATA_ESSENCE_OF_ORDER_EVENT:
                    return essenceData;
                case DATA_NOBEL_EVENT:
                    return nodelData;
                default:
                    return 0;
            }
        }

    private:
        uint32 essenceData;
        uint32 nodelData;
        uint64 akamaGUID;
    };
};

void AddSC_instance_pursuing_the_black_harvest()
{
    new instance_pursuing_the_black_harvest();
}
