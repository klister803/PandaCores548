#include "NewScriptPCH.h"
#include "ScriptedEscortAI.h"
#include "CreatureTextMgr.h"

class npc_water_spirit_dailo : public CreatureScript
{
public:
    npc_water_spirit_dailo() : CreatureScript("npc_water_spirit_dailo") { }

    bool OnGossipHello(Player* player, Creature* creature)
    {
        if (creature->isQuestGiver())
            player->PrepareQuestMenu(creature->GetGUID());

        if (player->GetQuestStatus(29774) == QUEST_STATUS_INCOMPLETE)
             player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Can you please help us to wake up Wugou ?", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1);

        player->SEND_GOSSIP_MENU(player->GetGossipTextId(creature), creature->GetGUID());

        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action)
    {
        player->PlayerTalkClass->ClearMenus();
        if (action == GOSSIP_ACTION_INFO_DEF+1)
        {
            player->CLOSE_GOSSIP_MENU();
            player->KilledMonsterCredit(55548);
            player->RemoveAurasDueToSpell(59073); // Remove Phase 2, first water spirit disapear

            if (Creature* shu = player->SummonCreature(55558, creature->GetPositionX(), creature->GetPositionY(), creature->GetPositionZ(), creature->GetOrientation(), TEMPSUMMON_MANUAL_DESPAWN, 0, player->GetGUID()))
            {
                if (shu->AI())
                {
                    shu->AI()->DoAction(0);
                    shu->AI()->SetGUID(player->GetGUID());
                }
            }
        }

        return true;
    }

    struct npc_water_spirit_dailoAI : public ScriptedAI
    {
        npc_water_spirit_dailoAI(Creature* creature) : ScriptedAI(creature)
        {}

        uint64 playerGuid;
        uint16 eventTimer;
        uint8  eventProgress;

        void Reset()
        {
            eventTimer = 0;
            eventProgress = 0;
            playerGuid = 0;
        }

        void DoAction(const int32 actionId)
        {
            eventTimer = 2500;
        }

        void SetGUID(uint64 guid, int32 /*type*/)
        {
            playerGuid = guid;
        }

        void MovementInform(uint32 typeId, uint32 pointId)
        {
            if (typeId != POINT_MOTION_TYPE)
                return;

            switch (pointId)
            {
                case 1:
                    eventTimer = 250;
                    ++eventProgress;
                    break;
                case 2:
                    eventTimer = 250;
                    ++eventProgress;
                    break;
                case 3:
                    if (Creature* wugou = GetClosestCreatureWithEntry(me, 60916, 20.0f))
                        me->SetFacingToObject(wugou);
                    me->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_STATE_READYUNARMED);
                    eventTimer = 2000;
                    ++eventProgress;
                    break;
                default:
                    break;
            }
        }

        void UpdateAI(const uint32 diff)
        {
            if (eventTimer)
            {
                if (eventTimer <= diff)
                {
                    switch (eventProgress)
                    {
                        case 0:
                            me->GetMotionMaster()->MovePoint(1, 650.30f, 3127.16f, 89.62f);
                            eventTimer = 0;
                            break;
                        case 1:
                            me->GetMotionMaster()->MovePoint(2, 625.25f, 3127.88f, 87.95f);
                            eventTimer = 0;
                            break;
                        case 2:
                            me->GetMotionMaster()->MovePoint(3, 624.44f, 3142.94f, 87.75f);
                            eventTimer = 0;
                            break;
                        case 3:
                            if (Creature* wugou = GetClosestCreatureWithEntry(me, 60916, 20.0f))
                                wugou->CastSpell(wugou, 118027, false);
                            me->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_ONESHOT_NONE);
                            eventTimer = 3000;
                            ++eventProgress;
                            break;
                        case 4:
                            eventTimer = 0;
                            if (Player* owner = ObjectAccessor::FindPlayer(playerGuid))
                            {
                                owner->KilledMonsterCredit(55547);
                                owner->RemoveAurasDueToSpell(59074); // Remove phase 4, asleep wugou disappear
                                
                                if (Creature* wugou = GetClosestCreatureWithEntry(me, 60916, 20.0f))
                                    if (Creature* newWugou = owner->SummonCreature(60916, wugou->GetPositionX(), wugou->GetPositionY(), wugou->GetPositionZ(), wugou->GetOrientation(), TEMPSUMMON_MANUAL_DESPAWN, 0, owner->GetGUID()))
                                        newWugou->GetMotionMaster()->MoveFollow(owner, PET_FOLLOW_DIST, PET_FOLLOW_ANGLE);
                            
                                me->GetMotionMaster()->MoveFollow(owner, PET_FOLLOW_DIST, -PET_FOLLOW_ANGLE);
                            }
                            break;
                        default:
                            break;
                    }
                }
                else
                    eventTimer -= diff;
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_water_spirit_dailoAI(creature);
    }
};

class AreaTrigger_at_middle_temple_from_east : public AreaTriggerScript
{
    public:
        AreaTrigger_at_middle_temple_from_east() : AreaTriggerScript("AreaTrigger_at_middle_temple_from_east")
        {}

        bool OnTrigger(Player* player, AreaTriggerEntry const* trigger)
        {
            if (Creature* shu = GetClosestCreatureWithEntry(player, 55558, 25.0f))
                shu->DespawnOrUnsummon();

            if (Creature* wugou = GetClosestCreatureWithEntry(player, 60916, 25.0f))
                wugou->DespawnOrUnsummon();

            return true;
        }
};

void AddSC_WanderingIsland_East()
{
    new npc_water_spirit_dailo();
}
