/*
    Dungeon : Template of Mogu'shan Palace 87-89
    Instance General Script
    Jade servers
*/

#include "ScriptMgr.h"
#include "InstanceScript.h"
#include "VMapFactory.h"
#include "mogu_shan_palace.h"

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
        /*
        ** Gekkan.
        */
        uint64 gekkan;
        uint64 glintrok_ironhide;
        uint64 glintrok_skulker;
        uint64 glintrok_oracle;
        uint64 glintrok_hexxer;
        /*
        ** End of Gekkan.
        */
        /*
        ** Xin the weaponmaster.
        */
        std::list<uint64> animated_staffs;
        std::list<uint64> swordLauncherGuids;
        /*
        ** End of Xin the weaponmaster.
        */
        instance_mogu_shan_palace_InstanceMapScript(Map* map) : InstanceScript(map)
        {
            xin_guid = 0;
            kuai_guid = 0;
            ming_guid = 0;
            haiyan_guid = 0;

            gekkan = 0;
            glintrok_ironhide = 0;
            glintrok_skulker = 0;
            glintrok_oracle = 0;
            glintrok_hexxer = 0;
        }

        void Initialize()
        {
        }

        void OnGameObjectCreate(GameObject* go)
        {
        }

        void OnCreatureCreate(Creature* creature)
        {
            OnCreatureCreate_gekkan(creature);
            OnCreatureCreate_trial_of_the_king(creature);
            OnCreatureCreate_xin_the_weaponmaster(creature);
        }

        void OnUnitDeath(Unit* unit)
        {
            OnUnitDeath_gekkan(unit);
        }
        
        virtual void Update(uint32 diff) 
        {
        }

        void SetData(uint32 type, uint32 data)
        {
            SetData_trial_of_the_king(type, data);
            SetData_xin_the_weaponmaster(type, data);
        }

        uint32 GetData(uint32 type)
        {
            return 0;
        }

        uint64 GetData64(uint32 type)
        {
            switch (type)
            {
            case TYPE_GET_ENTOURAGE_0:
                return glintrok_hexxer;
            case TYPE_GET_ENTOURAGE_1:
                return glintrok_ironhide;
            case TYPE_GET_ENTOURAGE_2:
                return glintrok_oracle;
            case TYPE_GET_ENTOURAGE_3:
                return glintrok_skulker;
            }
            return 0;
        }

        bool isWipe()
        {
            Map::PlayerList const& PlayerList = instance->GetPlayers();

            if (!PlayerList.isEmpty())
            {
                for (Map::PlayerList::const_iterator i = PlayerList.begin(); i != PlayerList.end(); ++i)
                {
                    if(Player* plr = i->getSource())
                        if (plr->isAlive() && !plr->isGameMaster())
                            return false;
                }
            }
            return true;
        }

        void SetData_xin_the_weaponmaster(uint32 type, uint32 data)
        {
            switch (type)
            {
                case TYPE_ACTIVATE_ANIMATED_STAFF:
                {
                    if (Creature* creature = instance->GetCreature(Trinity::Containers::SelectRandomContainerElement(animated_staffs)))
                        if (creature->GetAI())
                            creature->GetAI()->DoAction(0); //ACTION_ACTIVATE
                }
                case TYPE_ACTIVATE_SWORD:
                {
                    for (auto itr: swordLauncherGuids)
                    {
                        if (Creature* launcher = instance->GetCreature(itr))
                        {
                            if (data)
                                launcher->AddAura(SPELL_THROW_AURA, launcher);
                            else
                                launcher->RemoveAurasDueToSpell(SPELL_THROW_AURA);
                        }
                    }
                }
                break;
            }
        }
        void OnCreatureCreate_xin_the_weaponmaster(Creature* creature)
        {
            switch (creature->GetEntry())
            {
                case 59481:
                case 61451:
                    creature->SetReactState(REACT_PASSIVE);
                    break;
                case CREATURE_ANIMATED_STAFF:
                    animated_staffs.push_back(creature->GetGUID());
                    break;
                case CREATURE_LAUNCH_SWORD:
                    swordLauncherGuids.push_back(creature->GetGUID());
                    break;
            }
        }

        void OnUnitDeath_gekkan(Unit* unit)
        {
            if (unit->ToCreature())
            {
                switch (unit->ToCreature()->GetEntry())
                {
                    case CREATURE_GLINTROK_IRONHIDE:
                    case CREATURE_GLINTROK_SKULKER:
                    case CREATURE_GLINTROK_ORACLE:
                    case CREATURE_GLINTROK_HEXXER:
                        {
                            Creature* c = instance->GetCreature(gekkan);
                            if (!c)
                                return;
                            if (c->GetAI())
                                c->GetAI()->DoAction(0); //ACTION_ENTOURAGE_DIED
                        }
                        break;
                }
            }
        }
        void OnCreatureCreate_gekkan(Creature* creature)
        {
            switch (creature->GetEntry())
            {
                case CREATURE_GEKKAN:
                    gekkan = creature->GetGUID();
                    break;
                case CREATURE_GLINTROK_IRONHIDE:
                    glintrok_ironhide = creature->GetGUID();
                    break;
                case CREATURE_GLINTROK_SKULKER:
                    glintrok_skulker = creature->GetGUID();
                    break;
                case CREATURE_GLINTROK_ORACLE:
                    glintrok_oracle = creature->GetGUID();
                    break;
                case CREATURE_GLINTROK_HEXXER:
                    glintrok_hexxer = creature->GetGUID();
                    break;
            }
        }

        void SetData_trial_of_the_king(uint32 type, uint32 data)
        {
            switch (type)
            {
            case TYPE_OUTRO_05:
                {
                    if (Creature* haiyan = instance->GetCreature(haiyan_guid))
                        if (haiyan->GetAI())
                            haiyan->GetAI()->DoAction(1); //ACTION_OUTRO_02
                }
                break;
            case TYPE_OUTRO_04:
                {
                    if (Creature* kuai = instance->GetCreature(kuai_guid))
                        if (kuai->GetAI())
                            kuai->GetAI()->DoAction(3); //ACTION_OUTRO_02
                }
                break;
            case TYPE_OUTRO_03:
                {
                    if (Creature* ming = instance->GetCreature(ming_guid))
                        if (ming->GetAI())
                            ming->GetAI()->DoAction(2); //ACTION_OUTRO_02
                }
                break;
            case TYPE_OUTRO_02:
                {
                    if (Creature* haiyan = instance->GetCreature(haiyan_guid))
                        if (haiyan->GetAI())
                            haiyan->GetAI()->DoAction(0); //ACTION_OUTRO_01
                }
                break;
            case TYPE_OUTRO_01:
                {
                    if (Creature* ming = instance->GetCreature(ming_guid))
                        if (ming->GetAI())
                            ming->GetAI()->DoAction(1); //ACTION_OUTRO_01
                }
                break;
            case TYPE_MING_INTRO:
                {
                    if (Creature* ming = instance->GetCreature(ming_guid))
                        if (ming->GetAI())
                            ming->GetAI()->DoAction(0); //ACTION_INTRO
                }
                break;
            case TYPE_WIPE_FIRST_BOSS:
                {
                    Creature* xin = instance->GetCreature(xin_guid);
                    if (!xin)
                        return;
                    xin->SetVisible(true);
                    if (xin->GetAI())
                        xin->GetAI()->Reset();
                    switch (data)
                    {
                    case 0:
                        for (auto guid : adepts)
                        {
                            Creature* creature = instance->GetCreature(guid);
                            if (!creature)
                                continue;

                            if (creature && creature->GetAI())
                                creature->GetAI()->DoAction(1); //EVENT_RETIRE
                            creature->RemoveAura(121569);
                        }
                        break;
                    case 1:
                        for (auto guid : scrappers)
                        {
                            Creature* creature = instance->GetCreature(guid);
                            if (!creature)
                                continue;

                            if (creature && creature->GetAI())
                                creature->GetAI()->DoAction(1); //EVENT_RETIRE
                            creature->RemoveAura(121569);
                        }
                        break;
                    case 2:
                        for (auto guid : grunts)
                        {
                            Creature* creature = instance->GetCreature(guid);
                            if (!creature)
                                continue;

                            if (creature && creature->GetAI())
                                creature->GetAI()->DoAction(1); //EVENT_RETIRE
                            creature->RemoveAura(121569);
                        }
                        break;
                    }
                }
                break;
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
                {
                    for (auto guid : adepts)
                    {
                        Creature* creature = instance->GetCreature(guid);

                        if (creature && creature->GetAI())
                            creature->GetAI()->DoAction(2); //ACTION_ATTACK
                        
                        std::list<uint64>::iterator itr = grunts.begin();
                        std::advance(itr, urand(0, grunts.size() - 1));

                        Creature* grunt = instance->GetCreature(*itr);
                        if (grunt)
                            creature->Attack(grunt, true);
                    }
                    for (auto guid : grunts)
                    {
                        Creature* creature = instance->GetCreature(guid);

                        if (creature && creature->GetAI())
                            creature->GetAI()->DoAction(2); //ACTION_ATTACK

                        std::list<uint64>::iterator itr = scrappers.begin();
                        std::advance(itr, urand(0, scrappers.size() - 1));

                        Creature* scrapper = instance->GetCreature(*itr);
                        if (scrapper)
                            creature->Attack(scrapper, true);
                    }
                    for (auto guid : scrappers)
                    {
                        Creature* creature = instance->GetCreature(guid);

                        if (creature && creature->GetAI())
                            creature->GetAI()->DoAction(2); //ACTION_ATTACK

                        std::list<uint64>::iterator itr = adepts.begin();
                        std::advance(itr, urand(0, adepts.size() - 1));

                        Creature* adept = instance->GetCreature(*itr);
                        if (adept)
                            creature->Attack(adept, true);
                    }
                }
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
            case TYPE_HAIYAN_RETIRED:
                //Retire the adepts
                for (auto guid : grunts)
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
                break;
            case CREATURE_HARTHAK_ADEPT:
                adepts.push_back(creature->GetGUID());
                break;
            case CREATURE_KARGESH_GRUNT:
                grunts.push_back(creature->GetGUID());
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
