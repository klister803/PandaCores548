#include "ScriptMgr.h"
#include "ScriptedCreature.h"

class mob_tushui_trainee : public CreatureScript
{
    public:
        mob_tushui_trainee() : CreatureScript("mob_tushui_trainee") { }

        CreatureAI* GetAI(Creature* creature) const
        {
            return new mob_tushui_trainee_AI(creature);
        }

        struct mob_tushui_trainee_AI : public ScriptedAI
        {
            mob_tushui_trainee_AI(Creature* creature) : ScriptedAI(creature)
            {
                me->SetReactState(REACT_DEFENSIVE);
                me->setFaction(2357);
            }
            
            EventMap events;
            
            void EnterCombat(Unit* unit) { }

            void DamageTaken(Unit* attacker, uint32& damage)
            {
                if (me->HealthBelowPctDamaged(5, damage))
                {
                    if(me->getVictim() && me->getVictim()->GetTypeId() == TYPEID_PLAYER)
                        ((Player*)me->getVictim())->KilledMonsterCredit(54586, 0);
                    me->CombatStop();
                    me->SetHealth(me->GetMaxHealth());
                    me->HandleEmoteCommand(EMOTE_ONESHOT_SALUTE);
                    me->setFaction(2101);
                    me->DespawnOrUnsummon(3000);
                }
            }

            void UpdateAI(const uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                DoMeleeAttackIfReady();
            }
        };
};

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
            me->SetReactState(REACT_DEFENSIVE);
            me->SetDisplayId(39755);
            me->setFaction(14); //mechant!
        }
        
        EventMap events;
        bool first;

        void EnterCombat(Unit* unit)
        {
            Talk(0);
            events.ScheduleEvent(1, 1000);
            events.ScheduleEvent(3, 2000);
        }
        
        void Reset()
        {
            first = true;
            me->SetReactState(REACT_DEFENSIVE);
            me->SetDisplayId(39755);
            me->setFaction(14); //mechant!
        }
        
        void DamageTaken(Unit* attacker, uint32& damage)
        {
            if (me->HealthBelowPctDamaged(5, damage))
            {
                me->SetDisplayId(39755);
                if(me->getVictim() && me->getVictim()->GetTypeId() == TYPEID_PLAYER)
                    ((Player*)me->getVictim())->KilledMonsterCredit(me->GetEntry(), 0);
                me->CombatStop();
                me->setFaction(2104);
                me->SetHealth(me->GetMaxHealth());
                me->DespawnOrUnsummon(3000);
            }
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
                    case 1: //on monte
                        me->CastSpell(me->getVictim(), 108938, false);
                        events.ScheduleEvent(1, 30000);
                        break;
                    case 3: //baffe
                        me->CastSpell(me->getVictim(), 119301, false);
                        events.ScheduleEvent(3, 3000);
                        break;
                    case 4: //attaque du faucon
                        me->CastSpell(me->getVictim(), 108935, false);
                        events.ScheduleEvent(4, 4000);
                        break;
                    case 5: //remechant
                        me->setFaction(14);
                    	break;
                }
            }
            
            DoMeleeAttackIfReady();
            
            if((me->GetHealthPct() <= 30)&&(first))
            {
                first = false;
                me->SetDisplayId(39796); //faucon
                events.ScheduleEvent(4, 1000);
                events.CancelEvent(3);
                events.CancelEvent(1);
            }
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
            {
                uiDamage = 0;
            }
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
        uint64 guidMob[4];
        
        mob_min_dimwindAI(Creature* creature) : ScriptedAI(creature)
        {
            for(int i = 0; i < 4; i++)
                guidMob[i] = 0;
            ResetMobs();
        }
        
        void DamageTaken(Unit* pDoneBy, uint32 &uiDamage)
        {
            if(me->GetHealthPct() < 25 && pDoneBy && pDoneBy->ToCreature() && pDoneBy->ToCreature()->GetEntry() == 54130)
            {
                uiDamage = 0;
            }
        }
        
        
        bool VerifyMobs()
        {
            bool remaining = false;
            for(int i = 0; i < 4; i++)
            {
                if(guidMob[i] != 0)
                {
                    if (Unit* unit = sObjectAccessor->FindUnit(guidMob[i]))
                    {
                        if(unit->ToCreature())
                        {
                            if(!unit->ToCreature()->isDead())
                                remaining = true;
                        }
                        else
                            guidMob[i] = 0;
                    }
                    else
                    {
                        guidMob[i] = 0;
                    }
                }
            }
            return !remaining;
        }
        
        void ResetMobs()
        {
            events.ScheduleEvent(1, 600); //verify mobs
            for(int i = 0; i < 4; i++)
            {
                if(guidMob[i] != 0)
                {
                    if (Unit* unit = sObjectAccessor->FindUnit(guidMob[i]))
                        if(unit->ToCreature())
                            unit->ToCreature()->DespawnOrUnsummon();
                    guidMob[i] = 0;
                }
                
                me->HandleEmoteCommand(EMOTE_STATE_READY2H);
                TempSummon* temp = me->SummonCreature(54130, me->GetPositionX()+rand()%5, me->GetPositionY()+2+rand()%5, me->GetPositionZ()+1, 3.3f, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 10000);
                if(temp)
                {
                    guidMob[i] = temp->GetGUID();
                    
                    temp->SetFacingToObject(me);
                    temp->HandleEmoteCommand(EMOTE_STATE_READY2H);
                }
                else
                    guidMob[i] = 0;
            }
        }
        
        void UpdateAI(const uint32 diff)
        {
            events.Update(diff);
            while (uint32 eventId = events.ExecuteEvent())
            {
                if(eventId == 1) //verifions les mobs
                {
                    if(VerifyMobs()) //plus de mobs, win!
                    {
                    	me->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_STATE_STAND);
                    	me->MonsterYell("Thank you!", LANG_UNIVERSAL, 0);
                        
                        std::list<Player*> PlayerList;
                        Trinity::AnyPlayerInObjectRangeCheck checker(me, 50.0f);
                        Trinity::PlayerListSearcher<Trinity::AnyPlayerInObjectRangeCheck> searcher(me, PlayerList, checker);
                        me->VisitNearbyWorldObject(50.0f, searcher);
                        for (std::list<Player*>::const_iterator itr = PlayerList.begin(); itr != PlayerList.end(); ++itr)
                            (*itr)->KilledMonsterCredit(54855, 0);
                        
                        events.ScheduleEvent(2, 30000);
                    }
                    else
                    {
                        events.ScheduleEvent(1, 600);
                    }
                }
                else if(eventId == 2) //Reset
                {
                    ResetMobs();
                }
            }
        }
    };
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
        TempSummon* lifei;
        bool inCombat;
        uint32 timer;
        
        mob_aysaAI(Creature* creature) : ScriptedAI(creature)
        {
            events.ScheduleEvent(1, 600); //Begin script
            inCombat = false;
            timer = 0;
            lifei = NULL;
            me->SetReactState(REACT_DEFENSIVE);
            me->setFaction(2263);
        }
        
        void DamageTaken(Unit* pDoneBy, uint32 &uiDamage)
        {
            if(me->GetHealth() - uiDamage <=0)
            {
                if(lifei)
                {
                    lifei->UnSummon();
                    lifei = NULL;
                }
                
                uiDamage = 0;
                me->MonsterSay("I can't meditate!", LANG_UNIVERSAL, 0);
                me->SetHealth(me->GetMaxHealth());
                me->SetReactState(REACT_DEFENSIVE);
                
                std::list<Creature*> unitlist;
                Trinity::AllCreaturesOfEntryInRange check(me, 59637, 50.0f);
                Trinity::CreatureListSearcher<Trinity::AllCreaturesOfEntryInRange> searcher(me, unitlist, check);
                me->VisitNearbyWorldObject(50.0f, searcher);
                for (std::list<Creature*>::const_iterator itr = unitlist.begin(); itr != unitlist.end(); ++itr)
                	me->Kill(*itr);
                	
                events.ScheduleEvent(1, 20000);
                events.CancelEvent(2);
                events.CancelEvent(3);
                events.CancelEvent(4);
            }
        }
        
        void updatePlayerList()
        {
            playersInvolved.clear();
            
            std::list<Player*> PlayerList;
            Trinity::AnyPlayerInObjectRangeCheck checker(me, 50.0f);
            Trinity::PlayerListSearcher<Trinity::AnyPlayerInObjectRangeCheck> searcher(me, PlayerList, checker);
            me->VisitNearbyWorldObject(50.0f, searcher);
            for (std::list<Player*>::const_iterator itr = PlayerList.begin(); itr != PlayerList.end(); ++itr)
                if(*itr && (*itr)->GetQuestStatus(29414) == QUEST_STATUS_INCOMPLETE)
                    playersInvolved.push_back(*itr);
        }
        
        void UpdateAI(const uint32 diff)
        {
            events.Update(diff);
            while (uint32 eventId = events.ExecuteEvent())
            {
                switch(eventId)
                {
                    case 1: //Begin script if playersInvolved is not empty
                    {
                    	updatePlayerList();
                        if(playersInvolved.size() == 0)
                            events.ScheduleEvent(1, 600);
                        else
                        {
                            me->MonsterSay("Keep those creatures at bay while I meditate. We'll soon have the answers we seek...", LANG_UNIVERSAL, 0);
                            me->SetReactState(REACT_PASSIVE);
                            timer = 0;
                            events.ScheduleEvent(2, 5000); //spawn mobs
                            events.ScheduleEvent(3, 1000); //update time
                            events.ScheduleEvent(4, 90000); //end quest
                        }
                        break;
                    }
                    case 2: //Spawn 3 mobs
                    {
                        updatePlayerList();
                        for(int i = 0; i < std::max((int)playersInvolved.size()*3,3); i++)
                        {
                            TempSummon* temp = me->SummonCreature(59637, me->GetPositionX()+rand()%5, me->GetPositionY()+2+rand()%5, me->GetPositionZ()+1, 3.3f,TEMPSUMMON_CORPSE_TIMED_DESPAWN, 10000);
                            if(temp)
                            {
                                temp->AI()->AttackStart(me);
			                    temp->AddThreat(me, 250.0f);
                            }
                        }
                        events.ScheduleEvent(2, 20000); //spawn mobs
                        break;
                    }
                    case 3: //update energy
                    {
                        timer++;
                        
                        if(timer == 25 && !lifei)
                        {
                            lifei = me->SummonCreature(54856, 1130.162231f, 3435.905518f, 105.496597f, 0.0f,TEMPSUMMON_MANUAL_DESPAWN);
                            lifei->MonsterSay("The way of the Tushui... enlightenment through patience and mediation... the principled life", LANG_UNIVERSAL, 0);
                        }
                        
                        if(timer == 30)
                            lifei->MonsterSay("It is good to see you again, Aysa. You've come with respect, and so I shall give you the answers you seek.", LANG_UNIVERSAL, 0);
                        
                        if(timer == 42)
                            lifei->MonsterSay("Huo, the spirit of fire, is known for his hunger. He wants for tinder to eat. He needs the caress of the wind to rouse him.", LANG_UNIVERSAL, 0);
                        
                        if(timer == 54)
                            lifei->MonsterSay("If you find these things and bring them to his cave, on the far side of Wu-Song Village, you will face a challenge within.", LANG_UNIVERSAL, 0);
                        
                        if(timer == 66)
                            lifei->MonsterSay("Overcome that challenge, and you shall be graced by Huo's presence. Rekindle his flame, and if your spirit is pure, he shall follow you.", LANG_UNIVERSAL, 0);
                        
                        if(timer == 78)
                            lifei->MonsterSay("Go, children. We shall meet again very soon.", LANG_UNIVERSAL, 0);
                        
                        if(timer == 85)
                        {
                            lifei->UnSummon();
                            lifei = NULL;
                        }
                        
                        updatePlayerList();
                        for(std::list<Player*>::iterator itr = playersInvolved.begin(); itr != playersInvolved.end(); itr++)
                        {
                            Player* plr = *itr;
                            if(plr)
                            {
                                if(!plr->HasAura(116421))
                                    plr->CastSpell(plr, 116421);
                                plr->ModifyPower(POWER_ALTERNATE_POWER, timer/25);
                                plr->SetMaxPower(POWER_ALTERNATE_POWER, 90);
                            }
                        }
                        events.ScheduleEvent(3, 1000);
                        break;
                    }
                    case 4: //script end
                    {
                        if(lifei)
                        {
                            lifei->UnSummon();
                            lifei = NULL;
                        }
                        events.ScheduleEvent(1, 10000);
                        events.CancelEvent(2);
                        events.CancelEvent(3);
                        me->MonsterSay("And so our path lays before us. Speak to Master Shang Xi, he will tell you what comes next.", LANG_UNIVERSAL, 0);
                        updatePlayerList();
                        me->SetReactState(REACT_DEFENSIVE);
                        for(std::list<Player*>::iterator itr = playersInvolved.begin(); itr != playersInvolved.end(); itr++)
                        {
                            (*itr)->KilledMonsterCredit(54856, 0);
                            (*itr)->RemoveAura(116421);
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
    boss_li_fei() : CreatureScript("boss_li_fei") { }
    
    CreatureAI* GetAI(Creature* creature) const
    {
        return new boss_li_feiAI(creature);
    }
    
    struct boss_li_feiAI : public ScriptedAI
    {
        EventMap events;
        std::list<Player*> playersInvolved;

        boss_li_feiAI(Creature* creature) : ScriptedAI(creature)
        {
             /* TODO: React state aggressive, attackable, reset*/
            events.ScheduleEvent(1, 2000);
        }
        void EnterCombat(Unit* unit)
        {
        }

        void Reset()
        {
            me->CombatStop();
            me->setFaction(35);
            me->SetHealth(me->GetMaxHealth());
            events.ScheduleEvent(1, 10000);
            events.CancelEvent(2);
            events.CancelEvent(3);
            events.CancelEvent(4);
        }

        void updatePlayerList()
        {
            playersInvolved.clear();
            
            std::list<Player*> PlayerList;
            Trinity::AnyPlayerInObjectRangeCheck checker(me, 50.0f);
            Trinity::PlayerListSearcher<Trinity::AnyPlayerInObjectRangeCheck> searcher(me, PlayerList, checker);
            me->VisitNearbyWorldObject(50.0f, searcher);
            for (std::list<Player*>::const_iterator itr = PlayerList.begin(); itr != PlayerList.end(); ++itr)
                if(*itr && (*itr)->GetQuestStatus(29421) == QUEST_STATUS_INCOMPLETE)
                    playersInvolved.push_back(*itr);
        }
        
        void DamageTaken(Unit* attacker, uint32& damage)
        {
            if (me->HealthBelowPctDamaged(10, damage))
            {
                        me->CombatStop();
                        me->setFaction(35);
                        me->SetReactState(REACT_PASSIVE);
                         me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC);
                        me->SetHealth(me->GetMaxHealth());
                        events.ScheduleEvent(1, 10000);
                        events.CancelEvent(2);
                        events.CancelEvent(3);
                        events.CancelEvent(4);
                        updatePlayerList();
                        for(std::list<Player*>::iterator itr = playersInvolved.begin(); itr != playersInvolved.end(); itr++)
                        {
                            (*itr)->KilledMonsterCredit(54734, 0);
                        }
            }
        }
        
        void UpdateAI(const uint32 diff)
        {
            events.Update(diff);
            
            while (uint32 eventId = events.ExecuteEvent())
            {
                switch(eventId)
                {
                    case 1: //Begin script if playersInvolved is not empty
                    {
                    	updatePlayerList();
                        if(playersInvolved.size() == 0)
                        {
                            me->setFaction(35);
                            me->CombatStop();
                            me->SetHealth(me->GetMaxHealth());
                            events.CancelEvent(2);
                            events.CancelEvent(3);
                            events.CancelEvent(4);
                            events.ScheduleEvent(1, 2000);
                        }
                        else
                        {
                            me->SetReactState(REACT_AGGRESSIVE);
                            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_IMMUNE_TO_PC);
                            me->setFaction(16);
                            events.ScheduleEvent(2, 5000); 
                            events.ScheduleEvent(3, 1000); 
                        }
                    }
                    break;
                    case 2:
                        updatePlayerList();
                        if(playersInvolved.size() > 0)
                        {
                    	    me->CastSpell(me->getVictim(), 108958);
                    	    events.ScheduleEvent(2, 5000);
                        }
                        else
                            events.ScheduleEvent(1, 2000);
                    	break;
                    case 3:
                        updatePlayerList();
                        if(playersInvolved.size() > 0)
                        {
                    	    me->CastSpell(me->getVictim(), 108936);
                    	    events.ScheduleEvent(4, 2500);
                    	    events.ScheduleEvent(3, 150000);
                        }
                        else
                            events.ScheduleEvent(1, 2000);
                    	break;
                    case 4:
                        updatePlayerList();
                        if(playersInvolved.size() > 0)
                        {
                    	    me->CastSpell(me->getVictim(), 108944);
                        }
                        else
                            events.ScheduleEvent(1, 2000);
                    	break;
                }
            }
            
            DoMeleeAttackIfReady();
        }
    };
};

void AddSC_WanderingIsland()
{
    new mob_tushui_trainee();
    new boss_jaomin_ro();
    new mob_attacker_dimwind();
    new mob_min_dimwind();
    new mob_aysa();
    new boss_living_air();
    new boss_li_fei();
}