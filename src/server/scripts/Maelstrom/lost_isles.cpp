/*
phase 67851 ind 2 - for quest 14474 
*/

#include "ScriptPCH.h"
#include "CreatureTextMgr.h"
#include "ScriptedEscortAI.h"

enum isle_quests
{
    QUEST_DONT_GO_INTO_LIGHT                     = 14239,
    QUEST_GOBLIN_ESCAPE_PODS                     = 14474,  // Goblin Escape Pods or 14001
    QUEST_MINER_TROUBLES                         = 14021,  // Miner Troubles
};

enum isle_spells
{
    //Intro
    SPELL_SUMMON_DOC_ZAPNOZZLE                   = 69018,  //Don't Go Into The Light!: Summon Doc Zapnozzle
    SPELL_NEAR_DEATH                             = 69010,  //Near Death!
    SPELL_DOC_TO_CHAR                            = 69085,  //Don't Go Into The Light!: Force Cast from Doc to Character + proc 69086
    SPELL_INTRO_VISUAL                           = 69085,
    SPELL_INTRO_RES                              = 69022,
    SPELL_INVISIBLE_INRO_DUMMY                   = 76354,
};

enum isle_npc
{
    NPC_DOC_ZAPNNOZZLE                           = 36608,
    NPC_GIZMO                                    = 36600, // Geargrinder Gizmo
    NPC_FRIGHTENED_MINER                         = 35813, // Frightened Miner
};

enum gizmo_text
{
    TEXT_GIZMO_QUEST                             = 1,
};

class npc_gizmo : public CreatureScript
{
    public:
        npc_gizmo() : CreatureScript("npc_gizmo") { }

    struct npc_gizmoAI : public ScriptedAI
    {
        npc_gizmoAI(Creature* creature) : ScriptedAI(creature)
        {
            me->m_invisibilityDetect.AddFlag(INVISIBILITY_UNK7);
            me->m_invisibilityDetect.AddValue(INVISIBILITY_UNK7, 100000);
        }

        std::set<uint64> m_player_for_event;
        void Reset()
        {
            m_player_for_event.clear();
        }

        void OnStartQuest(Player* player, Quest const* quest)
        {
            if (!quest || quest->GetQuestId() != QUEST_GOBLIN_ESCAPE_PODS)
                return;

            sCreatureTextMgr->SendChat(me, TEXT_GIZMO_QUEST, player ? player->GetGUID(): 0);
        }

        // Remove from conteiner for posibility repeat it.
        // If plr disconect or not finish quest.
        void SetGUID(uint64 guid, int32 id)
        {
            m_player_for_event.erase(guid);
        }

        void MoveInLineOfSight(Unit* who)
        {
            if (who->HasAura(SPELL_NEAR_DEATH))
            {
                // always player. don't warry
                // waiting then plr finish movie.
                if (who->ToPlayer()->isWatchingMovie())
                    return;

                std::set<uint64>::iterator itr = m_player_for_event.find(who->GetGUID());
                if (itr == m_player_for_event.end())
                {
                    m_player_for_event.insert(who->GetGUID());
                    //me->CastSpell(537.135f, 3272.25f, 0.18f, SPELL_SUMMON_DOC_ZAPNOZZLE, true);
                    Position pos;
                    pos.Relocate(537.135f, 3272.25f, 0.18f, 2.46f);
                    if (TempSummon* summon = me->GetMap()->SummonCreature(NPC_DOC_ZAPNNOZZLE, pos))
                    {
                        summon->AddPlayerInPersonnalVisibilityList(who->GetGUID());
                        summon->AI()->SetGUID(who->GetGUID(), 0);
                        summon->AI()->SetGUID(me->GetGUID(), 1);
                    }
                }
            }
        }

        void UpdateAI(const uint32 diff)
        {

        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_gizmoAI(creature);
    }
};

enum isle_event
{
    EVENT_THE_VERY_BEGINING_1                      = 1,
    EVENT_THE_VERY_BEGINING_2                      = 2,
    EVENT_THE_VERY_BEGINING_3                      = 3,
    EVENT_THE_VERY_BEGINING_4                      = 4,
    EVENT_THE_VERY_BEGINING_5                      = 5,
    EVENT_THE_VERY_BEGINING_6                      = 6,
    EVENT_THE_VERY_BEGINING_7                      = 7,
    EVENT_THE_VERY_BEGINING_8                      = 8,
    EVENT_THE_VERY_BEGINING_9                      = 9,
    EVENT_THE_VERY_BEGINING_10                     = 10,
    EVENT_THE_VERY_BEGINING_11                     = 11,
    EVENT_THE_VERY_BEGINING_12                     = 12,
    EVENT_THE_VERY_BEGINING_13                     = 13
};

enum intro_text
{
    TEXT_INTRO_1                                   = 0,
    TEXT_INTRO_2                                   = 1,
    TEXT_INTRO_3                                   = 2,
    TEXT_INTRO_4                                   = 3,
    TEXT_INTRO_5                                   = 4,
};

class npc_doc_zapnnozzle : public CreatureScript
{
    public:
        npc_doc_zapnnozzle() : CreatureScript("npc_doc_zapnnozzle") { }

    struct npc_doc_zapnnozzleAI : public ScriptedAI
    {
        npc_doc_zapnnozzleAI(Creature* creature) : ScriptedAI(creature)
        {
            creature->m_invisibilityDetect.AddFlag(INVISIBILITY_UNK7);
            creature->m_invisibilityDetect.AddValue(INVISIBILITY_UNK7, 100000);
        }

        uint64 plrGUID;
        uint64 gizmoGUID;
        EventMap events;

        void Reset()
        {
            plrGUID = 0;
            gizmoGUID = 0;
            events.Reset();
        }

        void SetGUID(uint64 guid, int32 id)
        {
            switch(id)
            {
                case 0: plrGUID = guid; break;
                case 1:
                    gizmoGUID = guid;
                    events.ScheduleEvent(EVENT_THE_VERY_BEGINING_1, 1000);
                    break;
            }
        }

        void UpdateAI(const uint32 diff)
        {
            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_THE_VERY_BEGINING_1:
                        sCreatureTextMgr->SendChat(me, TEXT_INTRO_1, plrGUID);
                        events.ScheduleEvent(++eventId, 2000);
                        break;
                    case EVENT_THE_VERY_BEGINING_2:
                        events.ScheduleEvent(++eventId, 3000);
                        if (Creature* gizmo = Unit::GetCreature(*me, gizmoGUID))
                            sCreatureTextMgr->SendChat(gizmo, TEXT_INTRO_1, plrGUID);
                        break;
                    case EVENT_THE_VERY_BEGINING_3:
                        sCreatureTextMgr->SendChat(me, TEXT_INTRO_2, plrGUID);
                        events.ScheduleEvent(++eventId, 3000);
                        break;
                    case EVENT_THE_VERY_BEGINING_4:
                        if (Player* target = sObjectAccessor->FindPlayer(plrGUID))
                            DoCast(target, SPELL_INTRO_VISUAL);
                        sCreatureTextMgr->SendChat(me, TEXT_INTRO_3, plrGUID);
                        events.ScheduleEvent(++eventId, 1000);
                        break;
                    case EVENT_THE_VERY_BEGINING_5:
                    case EVENT_THE_VERY_BEGINING_6:
                    case EVENT_THE_VERY_BEGINING_8:
                    case EVENT_THE_VERY_BEGINING_9:
                        if (Player* target = sObjectAccessor->FindPlayer(plrGUID))
                            DoCast(target, SPELL_INTRO_RES);
                        events.ScheduleEvent(++eventId, 2500);
                        break;
                    case EVENT_THE_VERY_BEGINING_7:
                        if (Player* target = sObjectAccessor->FindPlayer(plrGUID))
                            DoCast(target, SPELL_INTRO_RES);
                        events.ScheduleEvent(++eventId, 2000);
                        sCreatureTextMgr->SendChat(me, TEXT_INTRO_4, plrGUID);
                        break;
                    case EVENT_THE_VERY_BEGINING_10:
                        sCreatureTextMgr->SendChat(me, TEXT_INTRO_5, plrGUID);
                        events.ScheduleEvent(++eventId, 2000);
                        break;
                    case EVENT_THE_VERY_BEGINING_11:
                        me->SetUInt32Value(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER);
                        if (Player* target = sObjectAccessor->FindPlayer(plrGUID))
                            target->RemoveAura(SPELL_NEAR_DEATH);
                        me->RemoveAura(SPELL_INVISIBLE_INRO_DUMMY);   
                        me->DespawnOrUnsummon(30000);
                        if (Creature* gizmo = Unit::GetCreature(*me, gizmoGUID))
                            gizmo->AI()->SetGUID(plrGUID, 0);
                        break;
                    default:
                        break;
                }
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_doc_zapnnozzleAI(creature);
    }
};

enum miner_text
{
    TEXT_MINER_0      = 0,
    TEXT_MINER_1      = 1,
    TEXT_MINER_2      = 2,
    TEXT_MINER_3      = 3,
    TEXT_MINER_4      = 4,
    TEXT_MINER_5      = 5,
};

class npc_frightened_miner : public CreatureScript
{
public:
    npc_frightened_miner() : CreatureScript("npc_frightened_miner") {}

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_frightened_minerAI (creature);
    }

    struct npc_frightened_minerAI : public npc_escortAI
    {
        npc_frightened_minerAI(Creature* creature) : npc_escortAI(creature) {}

        void Reset()
        {
            Start(false, true/*, who->GetGUID()*/);
        }

        void WaypointReached(uint32 i)
        {
            Player* player = GetPlayerForEscort();
            switch(i)
            {
                case 36:
                    break;
                default:
                    break;
            }
        }

        void JustDied(Unit* /*killer*/)
        {

        }

        void UpdateAI(uint32 const diff)
        {
            npc_escortAI::UpdateAI(diff);
        }
    };
};

void AddSC_lost_isle()
{
    new npc_gizmo();
    new npc_doc_zapnnozzle();
    new npc_frightened_miner();
}