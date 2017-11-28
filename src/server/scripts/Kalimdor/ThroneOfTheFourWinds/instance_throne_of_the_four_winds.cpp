//
//UWoWCore Throne of the Four Winds
//

#include "throne_of_the_four_winds.h"


class instance_throne_of_the_four_winds : public InstanceMapScript
{
public:
    instance_throne_of_the_four_winds() : InstanceMapScript("instance_throne_of_the_four_winds", 754) { }

    struct instance_throne_of_the_four_winds_InstanceMapScript: public InstanceScript
    {
        instance_throne_of_the_four_winds_InstanceMapScript(InstanceMap* map) : InstanceScript(map) {}

        uint64 uiAnshal;
        uint64 uiNezir;
        uint64 uiRohash;
        uint64 uiAlakir;
        uint64 alakirplatformGuid;
        uint64 drafteffcenterGuid;
        std::vector<uint64>conclavelist;
        std::vector<uint64>splipstreamlist;

        void Initialize()
        {
            SetBossNumber(DATA_MAX_BOSSES);
            uiAnshal           = 0;
            uiNezir            = 0;
            uiRohash           = 0;
            uiAlakir           = 0;
            alakirplatformGuid = 0;
            conclavelist.clear();
            splipstreamlist.clear();
        }

        void OnPlayerEnter(Player* player)
        {
            if (GetBossState(DATA_ALAKIR) == DONE)
                player->CastSpell(player, SPELL_TFW_SERENITY, true);
        }

        void OnCreatureCreate(Creature* creature)
        {
            switch (creature->GetEntry())
            {
                case BOSS_NEZIR:
                case BOSS_ROHASH:
                case BOSS_ANSHAL:
                    conclavelist.push_back(creature->GetGUID());
                    break;
                case BOSS_ALAKIR:
                    uiAlakir = creature->GetGUID();
                    if (GetBossState(DATA_CONCLAVE_OF_WIND) == DONE)
                        creature->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_IMMUNE_TO_PC);
                    break;
                case NPC_SPLIP_STREAM:
                    if (creature->GetPhaseMask() == 2)
                    {
                        splipstreamlist.push_back(creature->GetGUID());
                        //Activate SlipStream to Alakir
                        if (GetBossState(DATA_CONCLAVE_OF_WIND) == DONE)
                        {
                            creature->CastSpell(creature, SPELL_ALAKIR_SLIPSTREAM, true);
                            creature->SetPhaseMask(1, true);
                        }
                    }
                    break;
            }
        }

        void OnGameObjectCreate(GameObject* go)
        {
            switch (go->GetEntry())
            {
                case GO_ALAKIR_PLATFORM:
                    alakirplatformGuid = go->GetGUID();
                    if (GetBossState(DATA_ALAKIR) == DONE)
                        go->SetFlag(GAMEOBJECT_FLAGS, GO_FLAG_DESTROYED);
                    break;
                case GOB_WIND_DRAFTEFFECT_CENTER:
                    drafteffcenterGuid = go->GetGUID();
                    if (GetBossState(DATA_ALAKIR) == DONE)
                        go->SetPhaseMask(2, true);
                    break;
            }
        }

        bool SetBossState(uint32 type, EncounterState state)
        {
            if (!InstanceScript::SetBossState(type, state))
                return false;

            switch (type)
            {
                case DATA_CONCLAVE_OF_WIND:
                    switch (state)
                    {
                        case NOT_STARTED:
                            if (!conclavelist.empty())
                                for (std::vector<uint64>::const_iterator Itr = conclavelist.begin(); Itr != conclavelist.end(); ++Itr)
                                    if (Creature* boss = instance->GetCreature(*Itr))
                                        if (boss->isAlive() && boss->isInCombat())
                                            boss->AI()->EnterEvadeMode();
                            break;
                        case IN_PROGRESS:
                            if (!conclavelist.empty())
                                for (std::vector<uint64>::const_iterator Itr = conclavelist.begin(); Itr != conclavelist.end(); ++Itr)
                                    if (Creature* boss = instance->GetCreature(*Itr))
                                        if (boss->isAlive() && !boss->isInCombat())
                                            boss->AI()->DoZoneInCombat(boss, 500.0f);
                            break;
                        case DONE:
                            if (!conclavelist.empty())
                                for (std::vector<uint64>::const_iterator Itr = conclavelist.begin(); Itr != conclavelist.end(); ++Itr)
                                    if (Creature* boss = instance->GetCreature(*Itr))
                                        if (boss->isAlive())
                                            boss->Kill(boss, true);

                            if (Creature* Alakir = instance->GetCreature(uiAlakir))
                                Alakir->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_IMMUNE_TO_PC);

                            //Activate SlipStream to Alakir
                            if (!splipstreamlist.empty())
                            {
                                for (std::vector<uint64>::const_iterator Itr = splipstreamlist.begin(); Itr != splipstreamlist.end(); ++Itr)
                                {
                                    if (Creature* slipstream = instance->GetCreature(*Itr))
                                    {
                                        slipstream->CastSpell(slipstream, SPELL_ALAKIR_SLIPSTREAM, true);
                                        slipstream->SetPhaseMask(1, true);
                                    }
                                }
                            }
                            break;
                    }
                    break;
                case DATA_ALAKIR:
                    switch (state)
                    {
                        case NOT_STARTED:
                            if (GameObject* alakirplatform = instance->GetGameObject(alakirplatformGuid))
                                alakirplatform->SetDestructibleState(GO_DESTRUCTIBLE_REBUILDING);
                            break;
                        case DONE:
                            if (GameObject* alakirwind = instance->GetGameObject(drafteffcenterGuid))
                                alakirwind->SetPhaseMask(2, true);
                            break;
                    }
                    break;
            }
            return true;
        }

        void SetData(uint32 type, uint32 data)
        {
            if (type == DATA_CONCLAVE_DONE)
                if (GetData(DATA_IS_CONCLAVE_DONE))
                    SetBossState(DATA_CONCLAVE_OF_WIND, DONE);
        }

        uint64 GetData64(uint32 type)
        {
            switch (type)
            {
                case DATA_ALAKIR:
                    return uiAlakir;
                case GO_ALAKIR_PLATFORM:
                    return alakirplatformGuid;
            }
            return 0;
        }

        uint32 GetData(uint32 type)
        {
            if (type == DATA_IS_CONCLAVE_DONE)
            {
                if (!conclavelist.empty())
                    for (std::vector<uint64>::const_iterator Itr = conclavelist.begin(); Itr != conclavelist.end(); ++Itr)
                        if (Creature* boss = instance->GetCreature(*Itr))
                            if (!boss->AI()->GetData(DATA_IS_CONCLAVE_DONE))
                                return false;

                return true;
            }
            return 0;
        }

        std::string GetSaveData()
        {
            std::ostringstream saveStream;
            saveStream << " " << GetBossSaveData();
            return saveStream.str();
        }

        void Load(const char* data)
        {
            std::istringstream loadStream(LoadBossState(data));
            uint32 buff;
            for (uint32 i = 0; i < DATA_MAX_BOSSES; ++i)
            {
                loadStream >> buff;
                if (buff == IN_PROGRESS || buff > SPECIAL)
                    buff = NOT_STARTED;
            }
        }
    };

    InstanceScript* GetInstanceScript(InstanceMap* map) const
    {
        return new instance_throne_of_the_four_winds_InstanceMapScript(map);
    }
};

void AddSC_instance_throne_of_the_four_winds()
{
    new instance_throne_of_the_four_winds();
}
