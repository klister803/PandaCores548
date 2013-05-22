/*
    Dungeon : Template of Mogu'shan Palace 87-89
    Instance General Script
    Jade servers
*/

#include "ScriptMgr.h"
#include "InstanceScript.h"
#include "VMapFactory.h"
#include "mogu_shan_vault.h"

class instance_mogu_shan_vault : public InstanceMapScript
{
public:
    instance_mogu_shan_vault() : InstanceMapScript("instance_mogu_shan_vault", 1008) { }

    InstanceScript* GetInstanceScript(InstanceMap* map) const
    {
        return new instance_mogu_shan_vault_InstanceMapScript(map);
    }

    struct instance_mogu_shan_vault_InstanceMapScript : public InstanceScript
    {
        instance_mogu_shan_vault_InstanceMapScript(Map* map) : InstanceScript(map) {}

        uint32 actualPetrifierEntry;
        uint8  randomDespawnStoneGuardian;

        uint32 StoneGuardPetrificationTimer;

        uint64 stoneGuardControlerGuid;
        uint64 fengGuid;
        std::vector<uint64> stoneGuardGUIDs;
        std::vector<uint64> fengStatuesGUIDs;

        void Initialize()
        {
            SetBossNumber(DATA_MAX_BOSS_DATA);

            actualPetrifierEntry = 0;
            StoneGuardPetrificationTimer = 10000;
            stoneGuardControlerGuid = 0;

            stoneGuardGUIDs.clear();
            fengStatuesGUIDs.clear();

            randomDespawnStoneGuardian = urand(1,4);
        }

        void OnCreatureCreate(Creature* creature)
        {
            switch (creature->GetEntry())
            {
                case NPC_STONE_GUARD_CONTROLER:
                    stoneGuardControlerGuid = creature->GetGUID();
                    break;
                case NPC_JASPER:
                case NPC_JADE:
                case NPC_AMETHYST:
                case NPC_COBALT:
                    stoneGuardGUIDs.push_back(creature->GetGUID());

                    if (--randomDespawnStoneGuardian == 0)
                    {
                        creature->DespawnOrUnsummon();
                        randomDespawnStoneGuardian = -1;
                    }
                    break;
                case NPC_FENG:
                    fengGuid = creature->GetGUID();
                    break;
                default:
                    break;
            }
        }

        void OnGameObjectCreate(GameObject* go)
        {
            switch (go->GetEntry())
            {
                case GOB_SPEAR_STATUE:
                case GOB_FIST_STATUE:
                case GOB_SHIELD_STATUE:
                case GOB_STAFF_STATUE:
                    fengStatuesGUIDs.push_back(go->GetGUID());
                    break;
            }
        }

        bool SetBossState(uint32 id, EncounterState state)
        {
            if (!InstanceScript::SetBossState(id, state))
                return false;

            switch (id)
            {
                case DATA_STONE_GUARD:
                {
                    switch (state)
                    {
                        case IN_PROGRESS:
                        {
                            if (Creature* stoneGuardControler = instance->GetCreature(stoneGuardControlerGuid))
                                stoneGuardControler->AI()->DoAction(ACTION_ENTER_COMBAT);

                            for (auto stoneGuardGuid: stoneGuardGUIDs)
                                if (Creature* stoneGuard = instance->GetCreature(stoneGuardGuid))
                                    stoneGuard->AI()->DoAction(ACTION_ENTER_COMBAT);
                            break;
                        }
                        case FAIL:
                        {
                            for (auto stoneGuardGuid: stoneGuardGUIDs)
                                if (Creature* stoneGuard = instance->GetCreature(stoneGuardGuid))
                                    stoneGuard->AI()->DoAction(ACTION_FAIL);
                        }
                        default:
                            break;
                    }
                    break;
                }
                default:
                    break;
            }

            return true;
        }

        void SetData(uint32 type, uint32 data)
        {
        }

        uint32 GetData(uint32 type)
        {
            return 0;
        }

        uint64 GetData64(uint32 type)
        {
            switch (type)
            {
                // Creature
                case NPC_STONE_GUARD_CONTROLER:
                    return stoneGuardControlerGuid;
                case NPC_JASPER:
                case NPC_JADE:
                case NPC_AMETHYST:
                case NPC_COBALT:
                {
                    for (auto guid: stoneGuardGUIDs)
                        if (Creature* stoneGuard = instance->GetCreature(guid))
                            if (stoneGuard->GetEntry() == type)
                                return guid;
                    break;
                }
                case NPC_FENG:
                    return fengGuid;
                // Gameobject
                case GOB_SPEAR_STATUE:
                case GOB_FIST_STATUE:
                case GOB_SHIELD_STATUE:
                case GOB_STAFF_STATUE:
                {
                    for (auto guid: fengStatuesGUIDs)
                        if (GameObject* fengStatue = instance->GetGameObject(guid))
                            if (fengStatue->GetEntry() == type)
                                return guid;
                    break;
                }
            }
            return 0;
        }

        bool IsWipe()
        {
            Map::PlayerList const& PlayerList = instance->GetPlayers();

            if (PlayerList.isEmpty())
                return true;

            for (Map::PlayerList::const_iterator Itr = PlayerList.begin(); Itr != PlayerList.end(); ++Itr)
            {
                Player* player = Itr->getSource();

                if (!player)
                    continue;

                if (player->isAlive() && !player->isGameMaster() && !player->HasAura(115877)) // Aura 115877 = Totaly Petrified
                    return false;
            }

            return true;
        }
    };

};

void AddSC_instance_mogu_shan_vault()
{
    new instance_mogu_shan_vault();
}
