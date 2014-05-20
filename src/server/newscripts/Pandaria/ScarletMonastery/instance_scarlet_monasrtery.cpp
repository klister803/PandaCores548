/*
    Dungeon : Scarlet Monastery
    Instance General Script
*/

#include "NewScriptPCH.h"

class instance_scarlet_monasrtery : public InstanceMapScript
{
public:
    instance_scarlet_monasrtery() : InstanceMapScript("instance_scarlet_monasrtery", 1004) { }

    InstanceScript* GetInstanceScript(InstanceMap* map) const
    {
        return new instance_scarlet_monasrtery_InstanceMapScript(map);
    }

    struct instance_scarlet_monasrtery_InstanceMapScript : public InstanceScript
    {

        instance_scarlet_monasrtery_InstanceMapScript(Map* map) : InstanceScript(map)
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

void AddSC_instance_scarlet_monasrtery()
{
    new instance_scarlet_monasrtery();
}
