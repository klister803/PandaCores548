/*
    Dungeon : Template of Mogu'shan Palace 87-89
    Instance General Script
    Jade servers
*/

#include "ScriptMgr.h"
#include "InstanceScript.h"
#include "VMapFactory.h"

enum eSpells
{
    //Kuai the brute
    SPELL_COMBAT_SOUND_LOOP         = 126252,
    SPELL_SHOCKWAVE                 = 119922,
    SPELL_PICK_SHOCKWAVE_TARGET     = 120499,
    SPELL_SHOCKWAVE_2               = 119929,
    SPELL_SHOCKWAVE_3               = 119930,
    SPELL_SHOCKWAVE_4               = 119931,
    SPELL_SHOCKWAVE_5               = 119932,
    SPELL_SHOCKWAVE_6               = 119933,
    SPELL_GUARDIAN_TAUNT            = 85667,
    //Ming the cunning
    SPELL_LIGHTNING_BOLT            = 123654,
    SPELL_WHIRLING_DERVISH          = 119981,
    SPELL_MAGNETIC_FIELD            = 120100,
    SPELL_MAGNETIC_FIELD_2          = 120101,
    SPELL_MAGNETIC_FIELD_3          = 120099,
    //Haiyan the unstoppable
    SPELL_TRAUMATIC_BLOW            = 123655,
    SPELL_CONFLAGRATE               = 120160,
    SPELL_CONFLAGRATE_2             = 120167,
    SPELL_CONFLAGRATE_3             = 120161,
    SPELL_CONFLAGRATE_4             = 120201,
    SPELL_METEOR                    = 120195,
    SPELL_METEOR_2                  = 120194,
    SPELL_METEOR_3                  = 120196,
    //Xin trigger
    SPELL_PING                      = 120510,
    SPELL_MOGU_JUMP                 = 120444,
    //Gurthan scrapper, harthak adept and kargesh grunt
    SPELL_GRUNT_AURA                = 121746,
    //Whirling dervish trigger
    SPELL_WIRHLING_DERVISH_2        = 119982,
    SPELL_WHIRLING_DERVISH_3        = 119994,
    SPELL_THROW                     = 120087,
    SPELL_THROW_2                   = 120035,
};

enum eCreatures
{
    //Boss
    CREATURE_KUAI_THE_BRUTE                 = 61442,
    CREATURE_MING_THE_CUNNING               = 61444,
    CREATURE_HAIYAN_THE_UNSTOPPABLE         = 61445,
    CREATURE_XIN_THE_WEAPONMASTER_TRIGGER   = 61884,
    CREATURE_XIN_THE_WEAPONMASTER           = 61398,
    //Trash
    CREATURE_GURTHAN_SCRAPPER               = 61447,
    CREATURE_HARTHAK_ADEPT                  = 61449,
    CREATURE_KARGESH_GRUNT                  = 61450,
    //Trigger
    CREATURE_WHIRLING_DERVISH               = 61626,
};

enum eTypes
{
    TYPE_MING_ATTACK,
    TYPE_KUAI_ATTACK,
    TYPE_HAIYAN_ATTACK,
    TYPE_ALL_ATTACK,

    TYPE_MING_RETIRED,
    TYPE_KUAI_RETIRED,
};

class instance_mogu_shan_palace : public InstanceMapScript
{
public:
    instance_mogu_shan_palace() : InstanceMapScript("instance_mogu_shan_palace", 994) { }

    InstanceScript* GetInstanceScript(InstanceMap* map) const
    {
        return new instance_mogu_shan_palace_InstanceMapScript(map);
    }

    struct instance_mogu_shan_palace_InstanceMapScript : public InstanceScript
    {
        /*
        ** Trial of the king.
        */
        uint64 xin_guid;
        uint64 kuai_guid;
        uint64 ming_guid;
        uint64 haiyan_guid;
        std::list<uint64> scrappers;
        std::list<uint64> adepts;
        std::list<uint64> grunts;
        /*
        ** End of the trial of the king.
        */
        instance_mogu_shan_palace_InstanceMapScript(Map* map) : InstanceScript(map)
        {
        }

        void Initialize()
        {
        }

        void OnGameObjectCreate(GameObject* go)
        {
        }

        void OnCreatureCreate(Creature* creature)
        {
            OnCreatureCreate_trial_of_the_king(creature);
        }

        void OnUnitDeath(Unit* unit)
        {
        }
        
        virtual void Update(uint32 diff) 
        {
        }

        void SetData(uint32 type, uint32 data)
        {
            SetData_trial_of_the_king(type, data);
        }

        uint32 GetData(uint32 type)
        {
            return 0;
        }

        uint64 GetData64(uint32 type)
        {
            return 0;
        }

        bool isWipe()
        {
            Map::PlayerList const& PlayerList = instance->GetPlayers();

            if (!PlayerList.isEmpty())
            {
                for (Map::PlayerList::const_iterator i = PlayerList.begin(); i != PlayerList.end(); ++i)
                {
                    Player* plr = i->getSource();
                    if( !plr)
                        continue;
                    if (plr->isAlive() && !plr->isGameMaster())
                        return false;
                }
            }
            return true;
        }

        void SetData_trial_of_the_king(uint32 type, uint32 data)
        {
            switch (type)
            {
            case TYPE_MING_ATTACK:
                {
                    //Move the adepts
                    for (auto guid : adepts)
                    {
                        Creature* creature = instance->GetCreature(guid);

                        if (creature && creature->GetAI())
                            creature->GetAI()->DoAction(0); //EVENT_ENCOURAGE
                    }
                    Creature* ming = instance->GetCreature(ming_guid);
                    if (!ming)
                        return;
                    ming->GetMotionMaster()->MovePoint(0, -4237.658f, -2613.860f, 16.48f);
                    ming->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC);
                    ming->SetReactState(REACT_AGGRESSIVE);
                }
                break;
            case TYPE_KUAI_ATTACK:
                {
                    //Move the scrappers
                    for (auto guid : scrappers)
                    {
                        Creature* creature = instance->GetCreature(guid);

                        if (creature && creature->GetAI())
                            creature->GetAI()->DoAction(0); //EVENT_ENCOURAGE
                    }
                    Creature* kuai = instance->GetCreature(kuai_guid);
                    if (!kuai)
                        return;
                    kuai->GetMotionMaster()->MovePoint(0, -4215.359f, -2601.283f, 16.48f);
                    kuai->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC);
                    kuai->SetReactState(REACT_AGGRESSIVE);
                }
                break;
            case TYPE_HAIYAN_ATTACK:
                {
                    //Move the scrappers
                    for (auto guid : grunts)
                    {
                        Creature* creature = instance->GetCreature(guid);

                        if (creature && creature->GetAI())
                            creature->GetAI()->DoAction(0); //EVENT_ENCOURAGE
                    }
                    Creature* haiyan = instance->GetCreature(haiyan_guid);
                    if (!haiyan)
                        return;
                    haiyan->GetMotionMaster()->MovePoint(0, -4215.772f, -2627.216f, 16.48f);
                    haiyan->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC);
                    haiyan->SetReactState(REACT_AGGRESSIVE);
                }
                break;
            case TYPE_ALL_ATTACK:
                break;
            case TYPE_MING_RETIRED:
                //Retire the adepts
                for (auto guid : adepts)
                {
                    Creature* creature = instance->GetCreature(guid);

                    if (creature && creature->GetAI())
                        creature->GetAI()->DoAction(1); //EVENT_RETIRE
                }
                break;
            case TYPE_KUAI_RETIRED:
                //Retire the adepts
                for (auto guid : scrappers)
                {
                    Creature* creature = instance->GetCreature(guid);

                    if (creature && creature->GetAI())
                        creature->GetAI()->DoAction(1); //EVENT_RETIRE
                }
                break;
            }
        }
        void OnCreatureCreate_trial_of_the_king(Creature* creature)
        {
            switch (creature->GetEntry())
            {
            case CREATURE_GURTHAN_SCRAPPER:
                scrappers.push_back(creature->GetGUID());
                creature->SetReactState(REACT_PASSIVE);
                break;
            case CREATURE_HARTHAK_ADEPT:
                adepts.push_back(creature->GetGUID());
                creature->SetReactState(REACT_PASSIVE);
                break;
            case CREATURE_KARGESH_GRUNT:
                grunts.push_back(creature->GetGUID());
                creature->SetReactState(REACT_PASSIVE);
                break;
            case CREATURE_KUAI_THE_BRUTE:
                kuai_guid = creature->GetGUID();
                creature->SetReactState(REACT_PASSIVE);
                break;
            case CREATURE_MING_THE_CUNNING:
                ming_guid = creature->GetGUID();
                creature->SetReactState(REACT_PASSIVE);
                break;
            case CREATURE_HAIYAN_THE_UNSTOPPABLE:
                haiyan_guid = creature->GetGUID();
                creature->SetReactState(REACT_PASSIVE);
                break;
            case CREATURE_XIN_THE_WEAPONMASTER_TRIGGER:
                xin_guid = creature->GetGUID();
                creature->SetReactState(REACT_PASSIVE);
                break;
            case CREATURE_WHIRLING_DERVISH:
                break;
            }
        }
    };

};

void AddSC_instance_mogu_shan_palace()
{
    new instance_mogu_shan_palace();
}
