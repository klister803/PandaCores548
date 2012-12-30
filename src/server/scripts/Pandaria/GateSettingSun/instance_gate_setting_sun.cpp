/*
    Dungeon : Stormstout Brewery 85-87
    Instance General Script
*/

#include "ScriptMgr.h"
#include "InstanceScript.h"
#include "gate_setting_sun.h"

class instance_gate_setting_sun : public InstanceMapScript
{
public:
    instance_gate_setting_sun() : InstanceMapScript("instance_gate_setting_sun", 962) { }

    InstanceScript* GetInstanceScript(InstanceMapPtr map) const
    {
        return new instance_gate_setting_sun_InstanceMapScript(map);
    }

    struct instance_gate_setting_sun_InstanceMapScript : public InstanceScript
    {
        uint64 kiptilakGuid;
        uint64 gadokGuid;
        uint64 rimokGuid;
        uint64 raigonnGuid;

        instance_gate_setting_sun_InstanceMapScript(MapPtr map) : InstanceScript(map)
        {}

        void Initialize()
        {
            kiptilakGuid    = 0;
            gadokGuid       = 0;
            rimokGuid       = 0;
            raigonnGuid     = 0;
        }

        void OnCreatureCreate(CreaturePtr creature)
        {
            switch (creature->GetEntry())
            {
                case NPC_KIPTILAK:
                    kiptilakGuid = creature->GetGUID();
                    break;
                case NPC_GADOK:
                    gadokGuid = creature->GetGUID();
                    break;
                case NPC_RIMOK:
                    rimokGuid = creature->GetGUID();
                    break;
                case NPC_RAIGONN:
                    raigonnGuid = creature->GetGUID();
                    break;
            }
        }

        void OnGameObjectCreate(GameObjectPtr go)
        {
            switch (go->GetEntry())
            {
                default:
                    break;
            }
        }

        void SetData(uint32 type, uint32 data)
        {}

        uint32 GetData(uint32 type)
        {
            return 0;
        }

        uint64 GetData64(uint32 type)
        {
            switch (type)
            {
                case NPC_KIPTILAK:  return kiptilakGuid;
                case NPC_GADOK:     return gadokGuid;
                case NPC_RIMOK:     return rimokGuid;
                case NPC_RAIGONN:   return raigonnGuid;
            }

            return 0;
        }
    };

};

void AddSC_instance_gate_setting_sun()
{
    new instance_gate_setting_sun();
}
