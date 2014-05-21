/*
    Dungeon : Scarlet Halls
    Instance General Script
*/

#include "NewScriptPCH.h"

class instance_scarlet_halls : public InstanceMapScript
{
public:
    instance_scarlet_halls() : InstanceMapScript("instance_scarlet_halls", 1001) { }

    InstanceScript* GetInstanceScript(InstanceMap* map) const
    {
        return new instance_scarlet_halls_InstanceMapScript(map);
    }

    struct instance_scarlet_halls_InstanceMapScript : public InstanceScript
    {

        instance_scarlet_halls_InstanceMapScript(Map* map) : InstanceScript(map)
        {}

        void Initialize()
        {

        }
        
        void OnCreatureCreate(Creature* creature)
        {

        }

        void SetData(uint32 type, uint32 data)
        {}

        uint32 GetData(uint32 type)
        {
            return 0;
        }

        uint64 GetData64(uint32 type)
        {

            return 0;
        }

        void Update(uint32 diff) 
        {
            // Challenge
            InstanceScript::Update(diff);
        }
    };

};

void AddSC_instance_scarlet_halls()
{
    new instance_scarlet_halls();
}
