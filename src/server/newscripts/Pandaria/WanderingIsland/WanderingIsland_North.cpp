#include "NewScriptPCH.h"
#include "ScriptedEscortAI.h"
#include "CreatureTextMgr.h"

enum panda_text
{
    TEXT_GENERIC_0                                 = 0,
    TEXT_GENERIC_1                                 = 1,
    TEXT_GENERIC_2                                 = 2,
    TEXT_GENERIC_3                                 = 3,
    TEXT_GENERIC_4                                 = 4,
    TEXT_GENERIC_5                                 = 5,
    TEXT_GENERIC_6                                 = 6,
    TEXT_GENERIC_7                                 = 7,
};

enum panda_npc
{
    NPC_ANNOUNCER_1                                = 60183,
    NPC_ANNOUNCER_2                                = 60244,
    NPC_ANNOUNCER_3                                = 54943,
    NPC_ANNOUNCER_4                                = 54568,
    NPC_AMBERLEAF_SCAMP                            = 54130, //Amberleaf Scamp
    NPC_MIN_DIMWIND_OUTRO                          = 56503, //Min Dimwind
    NPC_MASTER_LI_FAI                              = 54856, 
};

enum panda_quests
{
    QUEST_THE_DISCIPLE_CHALLENGE                   = 29409, //29409 The Disciple's Challenge
    QUEST_AYSA_OF_TUSHUI                           = 29410, // Aysa of the Tushui
    QUEST_PARCHEMIN_VOLANT                         = 29421,
};

class npc_panda_announcer : public CreatureScript
{
    public:
        npc_panda_announcer() : CreatureScript("npc_panda_announcer") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_panda_announcerAI(creature);
    }
    
    struct npc_panda_announcerAI : public ScriptedAI
    {
        npc_panda_announcerAI(Creature* creature) : ScriptedAI(creature)
        {

        }

        void Reset()
        {
        }

        std::set<uint64> m_player_for_event;

        void MoveInLineOfSight(Unit* who)
        {
            if (who->GetTypeId() != TYPEID_PLAYER)
                return;

            std::set<uint64>::iterator itr = m_player_for_event.find(who->GetGUID());
            if (itr != m_player_for_event.end())
                return;

            switch(me->GetEntry())
            {
                case NPC_ANNOUNCER_1:
                    if (who->ToPlayer()->GetQuestStatus(QUEST_THE_DISCIPLE_CHALLENGE) != QUEST_STATUS_INCOMPLETE)
                        return;
                    break;
                case NPC_ANNOUNCER_2:
                case NPC_ANNOUNCER_3:
                    if (who->ToPlayer()->GetQuestStatus(QUEST_AYSA_OF_TUSHUI) == QUEST_STATUS_REWARDED)
                        return;
                    break;
                default:
                    break;
            }

            m_player_for_event.insert(who->GetGUID());
            sCreatureTextMgr->SendChat(me, TEXT_GENERIC_0, who->GetGUID());
        }
    };
};

// Should be done by summon npc 59591
class mob_master_shang_xi : public CreatureScript
{
    public:
        mob_master_shang_xi() : CreatureScript("mob_master_shang_xi") { }

        bool OnQuestAccept(Player* player, Creature* creature, Quest const* quest)
        {
            if (quest->GetQuestId() == 29408) // La lecon du parchemin brulant
            {
                creature->AddAura(114610, creature);
                creature->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP | UNIT_NPC_FLAG_QUESTGIVER);
                creature->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK);
            }

            return true;
        }

        CreatureAI* GetAI(Creature* creature) const
        {
            return new mob_master_shang_xi_AI(creature);
        }

        struct mob_master_shang_xi_AI : public ScriptedAI
        {
            mob_master_shang_xi_AI(Creature* creature) : ScriptedAI(creature)
            {
                checkPlayersTime = 2000;
            }

            uint32 checkPlayersTime;

            void SpellHit(Unit* caster, const SpellInfo* pSpell)
            {
                if (pSpell->Id == 114746) // Attraper la flamme
                {
                    if (caster->GetTypeId() == TYPEID_PLAYER)
                    {
                        if (caster->ToPlayer()->GetQuestStatus(29408) == QUEST_STATUS_INCOMPLETE)
                        {
                            sCreatureTextMgr->SendChat(me, TEXT_GENERIC_0);
                            me->CastSpell(caster, 114611, true);
                            me->RemoveAurasDueToSpell(114610);
                            me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP | UNIT_NPC_FLAG_QUESTGIVER);
                            me->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK);
                        }
                    }
                }
            }

            void UpdateAI(const uint32 diff)
            {
                if (checkPlayersTime <= diff)
                {
                    std::list<Player*> playerList;
                    GetPlayerListInGrid(playerList, me, 5.0f);

                    bool playerWithQuestNear = false;

                    for (std::list<Player*>::const_iterator itr = playerList.begin(); itr != playerList.end(); ++itr)
                    {
                        if ((*itr)->GetQuestStatus(29408) == QUEST_STATUS_INCOMPLETE && !(*itr)->HasItemCount(80212))
                                playerWithQuestNear = true;
                    }

                    if (playerWithQuestNear && !me->HasAura(114610))
                    {
                        me->AddAura(114610, me);
                        me->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP | UNIT_NPC_FLAG_QUESTGIVER);
                        me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK);
                    }
                    else if (!playerWithQuestNear && me->HasAura(114610))
                    {
                        me->RemoveAurasDueToSpell(114610);
                        me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP | UNIT_NPC_FLAG_QUESTGIVER);
                        me->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK);
                    }

                    checkPlayersTime = 2000;
                }
                else
                    checkPlayersTime -= diff;
            }
        };
};

// cast 88811 for check something
class boss_jaomin_ro : public CreatureScript
{
public:
    boss_jaomin_ro() : CreatureScript("boss_jaomin_ro") { }
    
    CreatureAI* GetAI(Creature* creature) const
    {
        return new boss_jaomin_roAI(creature);
    }
    
    struct boss_jaomin_roAI : public ScriptedAI
    {
        boss_jaomin_roAI(Creature* creature) : ScriptedAI(creature)
        {
        }

        enum eEvents
        {
            EVENT_JAOMIN_JUMP   = 1,
            EVENT_HIT_CIRCLE    = 2,
            EVENT_FALCON        = 3,
            EVENT_RESET         = 4,
            EVENT_CHECK_AREA    = 5,
        };
        
        EventMap events;
        bool isInFalcon;

        void EnterCombat(Unit* unit)
        {
            sCreatureTextMgr->SendChat(me, TEXT_GENERIC_0);
            events.ScheduleEvent(EVENT_JAOMIN_JUMP, 1000);
            events.ScheduleEvent(EVENT_HIT_CIRCLE, 2000);
            events.ScheduleEvent(EVENT_CHECK_AREA, 2500);
        }

        void Reset()
        {
            isInFalcon = false;
            me->SetDisplayId(39755);
            //me->CombatStop(true);

            //me->GetMotionMaster()->MovePoint(1, 1380.35f, 3170.68f, 136.93f);
        }

        void DamageTaken(Unit* attacker, uint32& damage)
        {
            if (me->HealthBelowPctDamaged(30, damage) && !isInFalcon)
            {
                isInFalcon = true;
                me->SetDisplayId(39796); //faucon
                events.ScheduleEvent(EVENT_FALCON, 1000);
                events.CancelEvent(EVENT_JAOMIN_JUMP);
                events.CancelEvent(EVENT_HIT_CIRCLE);
            }

            if (me->HealthBelowPctDamaged(5, damage))
            {
                sCreatureTextMgr->SendChat(me, TEXT_GENERIC_1);

                std::list<Player*> playerList;
                GetPlayerListInGrid(playerList, me, 10.0f);
                for (std::list<Player*>::const_iterator itr = playerList.begin(); itr != playerList.end(); ++itr)
                    (*itr)->KilledMonsterCredit(me->GetEntry(), 0);

                me->CombatStop();
                me->setFaction(35);
                me->SetFullHealth();
                me->HandleEmoteCommand(EMOTE_ONESHOT_SALUTE);
                events.Reset();
                events.ScheduleEvent(EVENT_RESET, 5000);
                damage = 0;
            }

            if (damage > me->GetHealth())
                damage = 0;
        }
        
        void UpdateAI(const uint32 diff)
        {
            if (!UpdateVictim())
                return;
            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch(eventId)
                {
                    case EVENT_JAOMIN_JUMP: //on monte
                        if (me->getVictim())
                            me->CastSpell(me->getVictim(), 108938, true);
                        events.ScheduleEvent(EVENT_JAOMIN_JUMP, 30000);
                        break;
                    case EVENT_HIT_CIRCLE: //baffe
                        if (me->getVictim())
                            me->CastSpell(me->getVictim(), 119301, true);

                        events.ScheduleEvent(EVENT_HIT_CIRCLE, 3000);
                        break;
                    case EVENT_FALCON: //attaque du faucon
                        if (me->getVictim())
                            me->CastSpell(me->getVictim(), 108935, true);

                        events.ScheduleEvent(EVENT_FALCON, 4000);
                        break;
                    case EVENT_RESET: //remechant
                        Reset();
                    	break;
                    case EVENT_CHECK_AREA:
                        if (me->GetAreaId() != 5843) // Grotte Paisible
                            Reset();
                        break;
                }
            }
            
            DoMeleeAttackIfReady();
        }
    };
};

class mob_attacker_dimwind : public CreatureScript
{
public:
    mob_attacker_dimwind() : CreatureScript("mob_attacker_dimwind") { }
    CreatureAI* GetAI(Creature* creature) const
    {
        return new mob_attacker_dimwindAI(creature);
    }
    
    struct mob_attacker_dimwindAI : public ScriptedAI
    {
    	mob_attacker_dimwindAI(Creature* creature) : ScriptedAI(creature) {}
    	
        void DamageTaken(Unit* pDoneBy, uint32 &uiDamage)
        {
            if(me->GetHealthPct() < 90 && pDoneBy && pDoneBy->ToCreature() && pDoneBy->ToCreature()->GetEntry() == 54785)
                uiDamage = 0;
        }

        void JustDied(Unit* killer)
        {
            if (killer->GetTypeId() != TYPEID_PLAYER || !me->ToTempSummon())
                return;

            if (Creature* owner = Unit::GetCreature(*me, me->ToTempSummon()->GetSummonerGUID()))
                owner->AI()->SetGUID(killer->GetGUID(), 0);
        }
    };
};

class mob_min_dimwind : public CreatureScript
{
public:
    mob_min_dimwind() : CreatureScript("mob_min_dimwind") { }
    
    CreatureAI* GetAI(Creature* creature) const
    {
        return new mob_min_dimwindAI(creature);
    }
    
    struct mob_min_dimwindAI : public ScriptedAI
    {
        EventMap events;
        std::set<uint64> guidMob;
        uint64 plrGUID;
        std::set<uint64> m_player_for_event;

        mob_min_dimwindAI(Creature* creature) : ScriptedAI(creature)
        {
        }

        void Reset()
        {
            ResetMobs();
            me->HandleEmoteCommand(EMOTE_STATE_READY2H);
        }

        void MoveInLineOfSight(Unit* who)
        {
            if (who->GetTypeId() != TYPEID_PLAYER)
                return;

            std::set<uint64>::iterator itr = m_player_for_event.find(who->GetGUID());
            if (itr != m_player_for_event.end())
                return;
            m_player_for_event.insert(who->GetGUID());
            for(std::set<uint64>::iterator itr = guidMob.begin(); itr != guidMob.end(); ++itr)
                if (Creature* c = Unit::GetCreature(*me, *itr))
                {
                    sCreatureTextMgr->SendChat(c, TEXT_GENERIC_0, who->GetGUID());
                    break;
                }
        }

        void DamageTaken(Unit* pDoneBy, uint32 &uiDamage)
        {
            if(me->GetHealthPct() < 25 && pDoneBy && pDoneBy->ToCreature() && pDoneBy->ToCreature()->GetEntry() == NPC_AMBERLEAF_SCAMP)
                uiDamage = 0;
        }
        
        void SetGUID(uint64 guid, int32 /*id*/ = 0)
        {
            plrGUID = guid;
        }

        void SummonedCreatureDespawn(Creature* summon)
        {
            guidMob.erase(summon->GetGUID());
            if (guidMob.empty())
            {
                me->HandleEmoteCommand(EMOTE_STATE_STAND);
                if (Player* target = sObjectAccessor->FindPlayer(plrGUID))
                {
                    target->KilledMonsterCredit(54855, 0);
                    // by spell 106205
                    if(TempSummon* mind = target->SummonCreature(NPC_MIN_DIMWIND_OUTRO, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), me->GetOrientation(), TEMPSUMMON_CORPSE_TIMED_DESPAWN, 1000))
                    {
                        mind->AI()->SetGUID(plrGUID, 0);
                        mind->setFaction(35);
                    }
                }
                me->DespawnOrUnsummon(1000);
            }
        }
        
        void ResetMobs()
        {
            me->HandleEmoteCommand(EMOTE_STATE_READY2H);
            for(std::set<uint64>::iterator itr = guidMob.begin(); itr != guidMob.end(); ++itr)
                if (Creature* c = Unit::GetCreature(*me, *itr))
                    c->DespawnOrUnsummon();
            guidMob.clear();

            for(int i = 0; i < 4; i++)
            {
                if(TempSummon* temp = me->SummonCreature(NPC_AMBERLEAF_SCAMP, me->GetPositionX()-3+rand()%6, me->GetPositionY() + 4 + rand()%4, me->GetPositionZ()+2, 3.3f, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 1000))
                {
                    guidMob.insert(temp->GetGUID());
                    
                    temp->SetFacingToObject(me);
                    temp->HandleEmoteCommand(EMOTE_STATE_READY2H);
                    
                    temp->GetMotionMaster()->Clear(false);
                    temp->GetMotionMaster()->MoveChase(me);
                    temp->Attack(me, true);
                    temp->getThreatManager().addThreat(me, 250.0f);
                }
            }
        }
    };
};

class npc_min_dimwind_outro : public CreatureScript
{
public:
    npc_min_dimwind_outro() : CreatureScript("npc_min_dimwind_outro") {}

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_min_dimwind_outroAI (creature);
    }

    struct npc_min_dimwind_outroAI : public npc_escortAI
    {
        npc_min_dimwind_outroAI(Creature* creature) : npc_escortAI(creature) {}

        EventMap events;
        uint64 playerGUID;

        enum eEvents
        {
            EVENT_1    = 1,
            EVENT_2    = 2,
        };

        void Reset()
        {
            playerGUID = 0;
        }

        void SetGUID(uint64 guid, int32 id)
        {
            playerGUID = guid;
            events.ScheduleEvent(EVENT_1, 1000);
        }

        void WaypointReached(uint32 pointId)
        {            
            switch(pointId)
            {
                case 3:
                case 4:
                    sCreatureTextMgr->SendChat(me, TEXT_GENERIC_2, playerGUID);
                    break;
                case 12:
                    sCreatureTextMgr->SendChat(me, TEXT_GENERIC_3, playerGUID);
                    me->DespawnOrUnsummon(30000);
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
                    case EVENT_1:
                        sCreatureTextMgr->SendChat(me, TEXT_GENERIC_0, playerGUID);
                        events.ScheduleEvent(EVENT_2, 1000);
                        break;
                    case EVENT_2:
                        sCreatureTextMgr->SendChat(me, TEXT_GENERIC_1, playerGUID);
                        Start(true, true);
                        break;
                    default:
                        break;
                }
            }
        }
    };
};

class mob_aysa_lake_escort : public CreatureScript
{
public:
    mob_aysa_lake_escort() : CreatureScript("mob_aysa_lake_escort") { }

    struct mob_aysa_lake_escortAI : public npc_escortAI
    {        
        mob_aysa_lake_escortAI(Creature* creature) : npc_escortAI(creature)
        {}

        uint32 IntroTimer;

        void Reset()
        {
            IntroTimer = 2500;
        }

        void MovementInform(uint32 uiType, uint32 uiId)
        {
            npc_escortAI::MovementInform(uiType, uiId);

            if (uiType != POINT_MOTION_TYPE && uiType != EFFECT_MOTION_TYPE)
                return;

            switch (uiId)
            {
                case 10:
                    me->GetMotionMaster()->MoveJump(1227.11f, 3489.73f, 100.37f, 10, 20, 11);
                    break;
                case 11:
                    me->GetMotionMaster()->MoveJump(1236.68f, 3456.68f, 102.58f, 10, 20, 12);
                    break;
                case 12:
                    Start(false, true);
                    break;
                default:
                    break;
            }
        }

        void IsSummonedBy(Unit* /*summoner*/)
        {
            sCreatureTextMgr->SendChat(me, TEXT_GENERIC_0);
        }

        void WaypointReached(uint32 waypointId)
        {
            if (waypointId == 4)
                me->DespawnOrUnsummon(500);
        }

        void UpdateAI(const uint32 diff)
        {
            if (IntroTimer)
            {
                if (IntroTimer <= diff)
                {
                    IntroTimer = 0;
                    me->GetMotionMaster()->MoveJump(1216.78f, 3499.44f, 91.15f, 10, 20, 10);
                }
                else
                    IntroTimer -= diff;
            }

            npc_escortAI::UpdateAI(diff);
        }
    };
    
    CreatureAI* GetAI(Creature* creature) const
    {
        return new mob_aysa_lake_escortAI(creature);
    }
    
};

class mob_aysa : public CreatureScript
{
public:
    mob_aysa() : CreatureScript("mob_aysa") { }
   
    CreatureAI* GetAI(Creature* creature) const
    {
        return new mob_aysaAI(creature);
    }
    
    struct mob_aysaAI : public ScriptedAI
    {
    	EventMap events;
        std::list<Player*> playersInvolved;
        uint64 lifeiGUID;
        bool inCombat;
        uint32 timer;
        
        mob_aysaAI(Creature* creature) : ScriptedAI(creature)
        {
            events.ScheduleEvent(1, 600); //Begin script
            inCombat = false;
            timer = 0;
            lifeiGUID = 0;
            me->SetReactState(REACT_DEFENSIVE);
            me->setFaction(2263);
        }

        enum eEvents
        {
            EVENT_START         = 1,
            EVENT_SPAWN_MOBS    = 2,
            EVENT_PROGRESS      = 3,
            EVENT_END           = 4,
        };

        enum eText
        {
            TEXT_TUSHI_0          = 0,
            TEXT_TUSHI_1          = 1,
            TEXT_TUSHI_2          = 2,
            TEXT_TUSHI_3          = 3,
            TEXT_TUSHI_4          = 4,
            TEXT_TUSHI_5          = 5,
            TEXT_TUSHI_6          = 6,
        };
        
        void DamageTaken(Unit* pDoneBy, uint32 &uiDamage)
        {
            if(me->HealthBelowPctDamaged(5, uiDamage))
            {
                if (Creature* lifei = Unit::GetCreature(*me, lifeiGUID))
                {
                    lifei->DespawnOrUnsummon(0);
                    lifeiGUID = NULL;
                    timer = 0;
                }
                
                uiDamage = 0;
                me->SetFullHealth();
                me->SetReactState(REACT_DEFENSIVE);
                
                std::list<Creature*> unitlist;
                GetCreatureListWithEntryInGrid(unitlist, me, 59637, 50.0f);
                for (std::list<Creature*>::const_iterator itr = unitlist.begin(); itr != unitlist.end(); ++itr)
                    me->Kill(*itr);
                	
                events.ScheduleEvent(EVENT_START, 20000);
                events.CancelEvent(EVENT_SPAWN_MOBS);
                events.CancelEvent(EVENT_PROGRESS);
                events.CancelEvent(EVENT_END);
            }
        }
        
        void updatePlayerList()
        {
            playersInvolved.clear();
            
            std::list<Player*> PlayerList;
            GetPlayerListInGrid(PlayerList, me, 20.0f);

            for (std::list<Player*>::const_iterator itr = PlayerList.begin(); itr != PlayerList.end(); ++itr)
            {
                Player* player = *itr;
                if(player->GetQuestStatus(29414) == QUEST_STATUS_INCOMPLETE)
                    playersInvolved.push_back(player);
            }
        }
        
        uint32 getLang(uint32 timer) const
        {
            switch(timer)
            {
                case 30: return TEXT_TUSHI_2;
                case 42: return TEXT_TUSHI_0;
                case 54: return TEXT_TUSHI_1;
                case 66: return TEXT_TUSHI_3;
                case 78: return TEXT_TUSHI_5;
                case 85: return TEXT_TUSHI_6;
                default: return 0;
            }
            return 0;
        }
        void UpdateAI(const uint32 diff)
        {
            events.Update(diff);
            while (uint32 eventId = events.ExecuteEvent())
            {
                switch(eventId)
                {
                    case EVENT_START: //Begin script if playersInvolved is not empty
                    {
                    	updatePlayerList();
                        if(playersInvolved.empty())
                            events.ScheduleEvent(1, 600);
                        else
                        {
                            sCreatureTextMgr->SendChat(me, TEXT_GENERIC_0);
                            me->SetReactState(REACT_PASSIVE);
                            timer = 0;
                            events.ScheduleEvent(EVENT_SPAWN_MOBS, 5000); //spawn mobs
                            events.ScheduleEvent(EVENT_PROGRESS, 1000); //update time
                            events.ScheduleEvent(EVENT_END, 90000); //end quest
                        }
                        break;
                    }
                    case EVENT_SPAWN_MOBS: //Spawn 3 mobs
                    {
                        updatePlayerList();
                        for(int i = 0; i < std::max((int)playersInvolved.size()*3,3); i++)
                        {
                            if(TempSummon* temp = me->SummonCreature(59637, 1144.55f, 3435.65f, 104.97f, 3.3f,TEMPSUMMON_CORPSE_TIMED_DESPAWN, 10000))
                            {
                                if (temp->AI())
                                    temp->AI()->AttackStart(me);

			                    temp->AddThreat(me, 250.0f);
                                temp->GetMotionMaster()->Clear();
                                temp->GetMotionMaster()->MoveChase(me);
                            }
                        }
                        events.ScheduleEvent(EVENT_SPAWN_MOBS, 20000); //spawn mobs
                        break;
                    }
                    case EVENT_PROGRESS: //update energy
                    {
                        timer++;
                        if(timer == 25 && !lifeiGUID)
                        {
                            if (Creature *lifei = me->SummonCreature(NPC_MASTER_LI_FAI, 1130.162231f, 3435.905518f, 105.496597f, 0.0f, TEMPSUMMON_MANUAL_DESPAWN))
                            {
                                sCreatureTextMgr->SendChat(lifei, TEXT_TUSHI_4);
                                lifeiGUID = lifei->GetGUID();
                            }
                        }
                        
                        if (uint32 lang = getLang(timer))
                        {
                            if (Creature* lifei = Unit::GetCreature(*me, lifeiGUID))
                            {
                                sCreatureTextMgr->SendChat(lifei, lang);
                                if(timer == 85)
                                {
                                    lifei->DespawnOrUnsummon(0);
                                    lifeiGUID = 0;
                                }
                            }
                        }
                        
                        updatePlayerList();
                        for (std::list<Player*>::const_iterator itr = playersInvolved.begin(); itr != playersInvolved.end(); ++itr)
                        {
                            Player* player = *itr;
                            if(!player->HasAura(116421))
                                player->CastSpell(player, 116421);

                            player->ModifyPower(POWER_ALTERNATE_POWER, timer/25);
                            player->SetMaxPower(POWER_ALTERNATE_POWER, 90);
                        }

                        events.ScheduleEvent(EVENT_PROGRESS, 1000);
                        break;
                    }
                    case EVENT_END: //script end
                    {
                        if (Creature* lifei = Unit::GetCreature(*me, lifeiGUID))
                        {
                            lifei->DespawnOrUnsummon(0);
                            lifeiGUID = 0;
                            timer = 0;
                        }
                        events.ScheduleEvent(EVENT_START, 10000);
                        events.CancelEvent(EVENT_SPAWN_MOBS);
                        events.CancelEvent(EVENT_PROGRESS);
                        sCreatureTextMgr->SendChat(me, TEXT_GENERIC_1);
                        updatePlayerList();
                        me->SetReactState(REACT_DEFENSIVE);
                        for (std::list<Player*>::const_iterator itr = playersInvolved.begin(); itr != playersInvolved.end(); ++itr)
                        {
                            Player* player = *itr;
                            player->KilledMonsterCredit(NPC_MASTER_LI_FAI, 0);
                            player->RemoveAura(116421);
                        }
                        break;
                    }
                }
            }
        }
    };
};

class boss_living_air : public CreatureScript
{
public:
    boss_living_air() : CreatureScript("boss_living_air") { }
    
    CreatureAI* GetAI(Creature* creature) const
    {
        return new boss_living_airAI(creature);
    }
    
    struct boss_living_airAI : public ScriptedAI
    {
        boss_living_airAI(Creature* creature) : ScriptedAI(creature)
        {
            me->SetReactState(REACT_AGGRESSIVE);
        }
        
        EventMap events;
        
        void EnterCombat(Unit* unit)
        {
            events.ScheduleEvent(1, 3000);
            events.ScheduleEvent(2, 5000);
        }
        
        void UpdateAI(const uint32 diff)
        {
            if (!UpdateVictim())
                return;
            
            events.Update(diff);
            
            while (uint32 eventId = events.ExecuteEvent())
            {
                switch(eventId)
                {
                    case 1:
                    	me->CastSpell(me->getVictim(), 108693);
                    	break;
                    case 2:
                    	me->CastSpell(me->getVictim(), 73212);
                    	events.ScheduleEvent(2, 5000);
                    	break;
                }
            }
            
            DoMeleeAttackIfReady();
        }
    };
};

class boss_li_fei : public CreatureScript
{
public:
    boss_li_fei() : CreatureScript("boss_li_fei") {}

    bool OnQuestAccept(Player* player, Creature* creature, Quest const* quest)
    {
        if (quest->GetQuestId() == QUEST_PARCHEMIN_VOLANT) // La lecon du parchemin brulant
        {
            // used by spell 102445
            if (Creature* tempSummon = creature->SummonCreature(54734, creature->GetPositionX(), creature->GetPositionY(), creature->GetPositionZ(), creature->GetOrientation(), TEMPSUMMON_MANUAL_DESPAWN, 0, player->GetGUID()))
            {
                player->CastSpell(player, 108149);  //visib
                //player->CastSpell(player, 108150);  //invis
                
                tempSummon->AddPlayerInPersonnalVisibilityList(player->GetGUID());
                tempSummon->AI()->AttackStart(player);
                tempSummon->AI()->SetGUID(player->GetGUID());
            }
        }

        return true;
    }
};

class boss_li_fei_fight : public CreatureScript
{
public:
    boss_li_fei_fight() : CreatureScript("boss_li_fei_fight") { }
    
    CreatureAI* GetAI(Creature* creature) const
    {
        return new boss_li_fei_fightAI(creature);
    }
    
    struct boss_li_fei_fightAI : public ScriptedAI
    {
        EventMap events;
        std::list<Player*> playersInvolved;
        uint64 playerGuid;

        boss_li_fei_fightAI(Creature* creature) : ScriptedAI(creature)
        {}

        enum eEvents
        {
            EVENT_CHECK_PLAYER      = 1,
            EVENT_FEET_OF_FURY      = 2,
            EVENT_SHADOW_KICK       = 3,
            EVENT_SHADOW_KICK_STUN  = 4,
        };

        void Reset()
        {
            // This particular entry is also spawned on an other event
            if (me->GetAreaId() != 5849) // Cavern areaid
                return;

            playerGuid = 0;
            me->SetReactState(REACT_AGGRESSIVE);
            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_IMMUNE_TO_PC);
            me->setFaction(16);
            events.ScheduleEvent(EVENT_CHECK_PLAYER, 2500);
            events.ScheduleEvent(EVENT_FEET_OF_FURY, 5000);
            events.ScheduleEvent(EVENT_SHADOW_KICK,  1000);
        }

        void SetGUID(uint64 guid, int32 /*type*/)
        {
            playerGuid = guid;
        }
        
        void DamageTaken(Unit* attacker, uint32& damage)
        {
            if (me->HealthBelowPctDamaged(10, damage))
            {
                damage = 0;
                me->setFaction(35);
                me->CombatStop();
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC);
                me->HandleEmoteCommand(EMOTE_ONESHOT_SALUTE);

                if (Player* player = ObjectAccessor::FindPlayer(playerGuid))
                {
                    player->KilledMonsterCredit(54734, 0);
                    player->RemoveAurasDueToSpell(108150);
                }
                if (Creature* owner = Unit::GetCreature(*me, me->ToTempSummon()->GetSummonerGUID()))
                    sCreatureTextMgr->SendChat(owner, TEXT_GENERIC_0, attacker->GetGUID());
            }
        }

        void KilledUnit(Unit* victim)
        {
            if (victim->GetTypeId() == TYPEID_PLAYER)
            {
                victim->ToPlayer()->SetQuestStatus(QUEST_PARCHEMIN_VOLANT, QUEST_STATUS_FAILED);

                if (victim->GetGUID() == playerGuid)
                    me->DespawnOrUnsummon(3000);
            }
        }
        
        void UpdateAI(const uint32 diff)
        {
            events.Update(diff);
            
            while (uint32 eventId = events.ExecuteEvent())
            {
                switch(eventId)
                {
                    case EVENT_CHECK_PLAYER:
                    {
                        bool checkPassed = true;
                        Player* player = ObjectAccessor::FindPlayer(playerGuid);

                        if (!player || !player->isAlive())
                        {
                            me->DespawnOrUnsummon(1000);
                            playerGuid = 0;
                            break;
                        }
                        
                        if (player->GetQuestStatus(QUEST_PARCHEMIN_VOLANT) != QUEST_STATUS_INCOMPLETE)
                        {
                            me->DespawnOrUnsummon(1000);
                            playerGuid = 0;
                            break;
                        }

                        events.ScheduleEvent(EVENT_CHECK_PLAYER, 2500);
                        break;
                    }
                    case EVENT_FEET_OF_FURY:
                        if(me->getVictim())
                    	    me->CastSpell(me->getVictim(), 108958);

                        events.ScheduleEvent(EVENT_FEET_OF_FURY, 5000);
                    	break;
                    case EVENT_SHADOW_KICK:
                        if(me->getVictim())
                    	    me->CastSpell(me->getVictim(), 108936);

                    	events.ScheduleEvent(EVENT_SHADOW_KICK_STUN, 2500);
                    	events.ScheduleEvent(EVENT_SHADOW_KICK, 30000);
                    	break;
                    case 4:
                        if(me->getVictim())
                    	    me->CastSpell(me->getVictim(), 108944);
                    	break;
                }
            }
            
            DoMeleeAttackIfReady();
        }
    };
};

class AreaTrigger_at_temple_entrance : public AreaTriggerScript
{
    public:
        AreaTrigger_at_temple_entrance() : AreaTriggerScript("AreaTrigger_at_temple_entrance")
        {}

        bool OnTrigger(Player* player, AreaTriggerEntry const* trigger)
        {
            if (player->GetQuestStatus(29423) == QUEST_STATUS_INCOMPLETE)
            {
                player->KilledMonsterCredit(61128, 0);

                std::list<Creature*> huoList;
                GetCreatureListWithEntryInGrid(huoList, player, 54958, 20.0f);

                for (std::list<Creature*>::const_iterator itr = huoList.begin(); itr != huoList.end(); ++itr)
                {
                    Creature* huo = *itr;
                    if (huo->ToTempSummon())
                    {
                        if (huo->ToTempSummon()->GetOwnerGUID() == player->GetGUID())
                        {
                            huo->GetMotionMaster()->Clear();
                            huo->GetMotionMaster()->MovePoint(1, 950.0f, 3601.0f, 203.0f);
                            huo->DespawnOrUnsummon(5000);
                        }
                    }
                }
            }

            return true;
        }
};

void AddSC_WanderingIsland_North()
{
    new mob_master_shang_xi();
    new boss_jaomin_ro();
    new npc_panda_announcer();
    new mob_attacker_dimwind();
    new mob_min_dimwind();
    new npc_min_dimwind_outro();
    new mob_aysa_lake_escort();
    new mob_aysa();
    new boss_living_air();
    new boss_li_fei();
    new boss_li_fei_fight();
    new AreaTrigger_at_temple_entrance();
}
