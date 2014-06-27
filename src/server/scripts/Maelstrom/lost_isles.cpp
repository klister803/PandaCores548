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
    SPELL_SUMMON_FRIGTNED_MINER                  = 68059,
    SPELL_SUMMON_ORE_CART                        = 68064,
    SPELL_VISUAL_ORE_CART_CHAIN                  = 68122,
    SPELL_VISUAL_CART_TRANSFORM                  = 68065,
};

enum isle_npc
{
    NPC_DOC_ZAPNNOZZLE                           = 36608,
    NPC_GIZMO                                    = 36600, // Geargrinder Gizmo
    NPC_FRIGHTENED_MINER                         = 35813, // Frightened Miner
    NPC_FOREMAN_DAMPWICK                         = 35769, // Foreman Dampwick
    NPC_ORE_CART                                 = 35814, // Miner Troubles Ore Cart 35814
    NPC_QUEST_MINE_TROUBLES_CREDIT               = 35816,
};

enum isle_go
{
    GO_KAJAMITE_ORE                             = 195622, //Kaja'mite Ore
};

enum isle_events
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
    EVENT_THE_VERY_BEGINING_13                     = 13,

    EVENT_GENERIC_1                                = 1,
    EVENT_GENERIC_2                                = 2,

    EVENT_POINT_MINE                               = 1000,
};

enum isle_emote
{
    EMOTE_FIND_MINE                                = 396,
    EMOTE_FIND_MINING                              = 233,
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

// NPC_FOREMAN_DAMPWICK                         = 35769, //Foreman Dampwick

class npc_foreman_dampwick : public CreatureScript
{
    public:
        npc_foreman_dampwick() : CreatureScript("npc_foreman_dampwick") { }

    struct npc_foreman_dampwickAI : public ScriptedAI
    {
        npc_foreman_dampwickAI(Creature* creature) : ScriptedAI(creature)
        {

        }

        void Reset()
        {
        }

        void OnStartQuest(Player* player, Quest const* quest)   
        {
            if (!quest || quest->GetQuestId() != QUEST_MINER_TROUBLES)
                return;

            Position pos;
            pos.Relocate(492.4184f, 2976.321f, 8.040207f);
            if (TempSummon* summon = player->GetMap()->SummonCreature(NPC_FRIGHTENED_MINER, pos, NULL, 0, player))
            {
                summon->AddPlayerInPersonnalVisibilityList(player->GetGUID());
                summon->AI()->SetGUID(player->GetGUID(), 0);
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_foreman_dampwickAI(creature);
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

        uint64 plrGUID;
        uint64 cartGUID;
        uint64 mineGUID;
        EventMap events;
        uint32 wpMine;

        void Reset()
        {
            plrGUID = 0;
            cartGUID = 0;
            mineGUID = 0;
            wpMine = 0;
            events.Reset();
            
        }

        void SetGUID(uint64 guid, int32 id)
        {
            plrGUID = guid;
            Start(true, false, guid);
            DoCast(me, SPELL_SUMMON_ORE_CART);
        }

        void EnterEvadeMode()
        {
            npc_escortAI::EnterEvadeMode();
            if (Creature* cart = Unit::GetCreature(*me, cartGUID))
                me->CastSpell(cart, SPELL_VISUAL_ORE_CART_CHAIN, true);
        }

        void JustSummoned(Creature* summon)
        {
            summon->AddPlayerInPersonnalVisibilityList(plrGUID);
            summon->SetWalk(false);
            summon->SetSpeed(MOVE_RUN, 1.25f);
            summon->GetMotionMaster()->MoveFollow(me, 1.0f, 0);
            cartGUID = summon->GetGUID();
        }

        void JustDied(Unit* /*killer*/) 
        {
            if (Player* player = GetPlayerForEscort())
                player->FailQuest(QUEST_MINER_TROUBLES);
        }

        void seachMine()
        {
            if (GameObject* mine = me->FindNearestGameObject(GO_KAJAMITE_ORE, 20.0f))
            {
                mineGUID = mine->GetGUID();
                SetEscortPaused(true);

                events.Reset();

                //Position pos;
                //mine->GetNearPosition(pos, 1.0f, 0.0f);
                me->GetMotionMaster()->MovePoint(EVENT_POINT_MINE, mine->m_positionX, mine->m_positionY, mine->m_positionZ);
                me->HandleEmoteCommand(EMOTE_FIND_MINE);
            }else if (HasEscortState(STATE_ESCORT_PAUSED))
                SetEscortPaused(false);
        }
        
        void MovementInform(uint32 moveType, uint32 pointId)
        {
            if (pointId == EVENT_POINT_MINE)
            {
                me->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_FIND_MINING);
                events.ScheduleEvent(EVENT_GENERIC_1, 10000);
                return;
            }
            if (HasEscortState(STATE_ESCORT_PAUSED))
                seachMine();

            npc_escortAI::MovementInform(moveType, pointId);
        }

        void WaypointReached(uint32 pointId)
        {            
            switch(pointId)
            {
                case 1:
                    sCreatureTextMgr->SendChat(me, TEXT_MINER_0, plrGUID);
                    if (Creature* cart = Unit::GetCreature(*me, cartGUID))
                        me->CastSpell(cart, SPELL_VISUAL_ORE_CART_CHAIN, true);
                    break;
                case 7:
                    sCreatureTextMgr->SendChat(me, TEXT_MINER_1, plrGUID);
                    break;
                case 10:
                case 14:
                case 17:
                case 22:
                    wpMine = pointId;
                    seachMine();
                    break;
                case 23:
                    sCreatureTextMgr->SendChat(me, TEXT_MINER_5, plrGUID);
                    if (Player* player = GetPlayerForEscort())
                        player->KilledMonsterCredit(NPC_QUEST_MINE_TROUBLES_CREDIT);
                    break;
                case 24:
                    me->SetWalk(false);
                    break;
                default:
                    break;
            }
        }

        void UpdateAI(uint32 const diff)
        {
            npc_escortAI::UpdateAI(diff);
            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    // emotes only when in vehicle.
                    case EVENT_GENERIC_1:
                    {
                        uint32 text = 0;
                        switch(wpMine)
                        {
                            case 10: text = TEXT_MINER_2; break;
                            case 14: text = TEXT_MINER_3; break;
                            case 17: text = TEXT_MINER_4; break;
                        }
                        me->SetUInt32Value(UNIT_NPC_EMOTESTATE, 0);
                        SetEscortPaused(false);
                        if (text)
                            sCreatureTextMgr->SendChat(me, text, plrGUID);
                        if (GameObject* go = ObjectAccessor::GetGameObject(*me, mineGUID))
                        {
                            go->SendCustomAnim(0);
                            go->SetLootState(GO_JUST_DEACTIVATED);
                        }
                        break;
                    }
                }
            }
        }
    };
};

void AddSC_lost_isle()
{
    new npc_gizmo();
    new npc_doc_zapnnozzle();
    new npc_foreman_dampwick();
    new npc_frightened_miner();
}