#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ScriptedEscortAI.h"

class AreaTrigger_at_mandori : public AreaTriggerScript
{
    public:
        AreaTrigger_at_mandori() : AreaTriggerScript("AreaTrigger_at_mandori")
        {}

        bool OnTrigger(Player* player, AreaTriggerEntry const* trigger)
        {
           if (player->GetPositionX() < 710.0f)
               return true;

           if (player->GetQuestStatus(29792) != QUEST_STATUS_INCOMPLETE)
               return true;

           uint64 playerGuid = player->GetGUID();

            Creature* Aysa = player->SummonCreature(59986, 698.04f, 3601.79f, 142.82f, 3.254830f, TEMPSUMMON_MANUAL_DESPAWN, 0, playerGuid); // Aysa
            Creature* Ji   = player->SummonCreature(59988, 698.06f, 3599.34f, 142.62f, 2.668790f, TEMPSUMMON_MANUAL_DESPAWN, 0, playerGuid); // Ji
            Creature* Jojo = player->SummonCreature(59989, 702.78f, 3603.58f, 142.01f, 3.433610f, TEMPSUMMON_MANUAL_DESPAWN, 0, playerGuid); // Jojo

            if (!Aysa || !Ji || !Jojo)
                return true;
            
            Aysa->AI()->SetGUID(playerGuid);
              Ji->AI()->SetGUID(playerGuid);
            Jojo->AI()->SetGUID(playerGuid);

            return true;
        }
};

class mob_mandori_escort : public CreatureScript
{
    public:
        mob_mandori_escort() : CreatureScript("mob_mandori_escort") { }

    struct mob_mandori_escortAI : public npc_escortAI
    {        
        mob_mandori_escortAI(Creature* creature) : npc_escortAI(creature)
        {}

        enum escortEntry
        {
            NPC_AYSA    = 59986,
            NPC_JI      = 59988,
            NPC_JOJO    = 59989
        };
        
        uint32 IntroTimer;
        uint32 doorEventTimer;

        uint8  IntroState;
        uint8  doorEventState;

        uint64 playerGuid;
        
        uint64 mandoriDoorGuid;
        uint64 peiwuDoorGuid;

        void Reset()
        {
            IntroTimer      = 250;
            doorEventTimer  = 0;

            IntroState      = 0;
            doorEventState  = 0;

            playerGuid      = 0;
            mandoriDoorGuid = 0;
            peiwuDoorGuid   = 0;

            me->SetReactState(REACT_PASSIVE);
        }

        void SetGUID(uint64 guid, int32 type)
        {
            playerGuid = guid;

            if (!Is(NPC_AYSA))
                return;

            if (GameObject* mandoriDoor = me->SummonGameObject(211294, 695.26f, 3600.99f, 142.38f, 3.04f, 0.0f, 0.0f, 0.0f, 0.0f, RESPAWN_IMMEDIATELY, playerGuid))
                mandoriDoorGuid = mandoriDoor->GetGUID();

            if (GameObject* peiwuDoor = me->SummonGameObject(211298, 566.52f, 3583.46f, 92.16f, 3.14f, 0.0f, 0.0f, 0.0f, 0.0f, RESPAWN_IMMEDIATELY, playerGuid))
                peiwuDoorGuid = peiwuDoor->GetGUID();
        }

        bool Is(uint32 npc_entry)
        {
            return me->GetEntry() == npc_entry;
        }

        void WaypointReached(uint32 waypointId)
        {
            switch (waypointId)
            {
                case 5:
                    SetEscortPaused(true);

                    // Jojo reach the waypoint 1 sec after the others
                    if (!Is(NPC_JOJO))
                        doorEventTimer = 2000;
                    else
                        doorEventTimer = 1000;
                    break;
                default:
                    break;
            }
        }

        void LastWaypointReached()
        {
            if (Is(NPC_JI))
                ; // Set new phase to player
            
            if (Is(NPC_AYSA))
            {
                if (GameObject* mandoriDoor = me->GetMap()->GetGameObject(mandoriDoorGuid))
                    mandoriDoor->Delete();
                if (GameObject* peiwuDoor = me->GetMap()->GetGameObject(peiwuDoorGuid))
                    peiwuDoor->Delete();
            }
        }

        void UpdateAI(const uint32 diff)
        {
            if (IntroTimer)
            {
                if (IntroTimer <= diff)
                {
                    switch (++IntroState)
                    {
                        case 1:
                            if (Is(NPC_AYSA))
                                me->MonsterYell("Let's go !", LANG_UNIVERSAL, playerGuid);
                            IntroTimer = 1000;
                            break;
                        case 2:
                            if (Is(NPC_AYSA))
                            {
                                if (GameObject* mandoriDoor = me->GetMap()->GetGameObject(mandoriDoorGuid))
                                    mandoriDoor->SetGoState(GO_STATE_ACTIVE);

                                if (Player* player = ObjectAccessor::FindPlayer(playerGuid))
                                    player->KilledMonsterCredit(59946);
                            }
                            IntroTimer = 1000;
                            break;
                        case 3:
                            Start(false, true);
                            IntroTimer = 0;
                            break;
                    }
                }
                else
                    IntroTimer -= diff;
            }

            if (doorEventTimer)
            {
                if (doorEventTimer <= diff)
                {
                    switch (++doorEventState)
                    {
                        case 1:
                            if (Is(NPC_AYSA))
                                me->MonsterSay("The door is blocked!", LANG_UNIVERSAL, playerGuid);
                            doorEventTimer = 2500;
                            break;
                        case 2:
                            if (Is(NPC_JI))
                                me->MonsterSay("They blocked it with a rock on the other side, I can't open it!", LANG_UNIVERSAL, playerGuid);
                            doorEventTimer = 4000;
                            break;
                        case 3:
                            if (Is(NPC_JOJO))
                                me->GetMotionMaster()->MoveCharge(567.99f, 3583.41f, 94.74f);
                            doorEventTimer = 250;
                            break;
                        case 4:
                            if (Is(NPC_AYSA))
                                if (GameObject* peiwuDoor = me->GetMap()->GetGameObject(peiwuDoorGuid))
                                    peiwuDoor->SetGoState(GO_STATE_ACTIVE);
                            doorEventTimer = 2000;
                            break;
                       case 5:
                            if (Is(NPC_AYSA))
                            {
                                me->MonsterSay("Well done, Jojo!", LANG_UNIVERSAL, playerGuid);

                                if (Player* player = ObjectAccessor::FindPlayer(playerGuid))
                                    player->KilledMonsterCredit(59947);
                            }
                           if (!Is(NPC_JOJO))
                               SetEscortPaused(false);
                            doorEventTimer = 2000;
                            break;
                       case 6:
                           if (Is(NPC_JOJO))
                               SetEscortPaused(false);
                            doorEventTimer = 0;
                            break;
                    }
                }
                else
                    doorEventTimer -= diff;
            }

            npc_escortAI::UpdateAI(diff);
        }
    };
    
    CreatureAI* GetAI(Creature* creature) const
    {
        return new mob_mandori_escortAI(creature);
    }
};

class AreaTrigger_at_rescue_soldiers : public AreaTriggerScript
{
    public:
        AreaTrigger_at_rescue_soldiers() : AreaTriggerScript("AreaTrigger_at_rescue_soldiers")
        {}

        bool OnTrigger(Player* player, AreaTriggerEntry const* trigger)
        {
           if (player->GetQuestStatus(29794) != QUEST_STATUS_INCOMPLETE)
               return true;

           if (!player->HasAura(129340))
               return true;

           player->RemoveAurasDueToSpell(129340);
           player->KilledMonsterCredit(55999);

            return true;
        }
};

class npc_hurted_soldier : public CreatureScript
{
public:
    npc_hurted_soldier() : CreatureScript("npc_hurted_soldier") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_hurted_soldierAI (creature);
    }

    struct npc_hurted_soldierAI : public ScriptedAI
    {
        npc_hurted_soldierAI(Creature* creature) : ScriptedAI(creature) {}

        uint32 savedTimer;
        bool HasBeenSaved;

        void Reset()
        {
            savedTimer = 0;
            HasBeenSaved = false;
        }

        void OnSpellClick(Unit* Clicker)
        {
            me->EnterVehicle(Clicker);
            me->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK);
            HasBeenSaved = true;
        }

        void UpdateAI(const uint32 diff)
        {
            if (savedTimer)
            {
                if (savedTimer <= diff)
                {
                    if (HasBeenSaved && !me->GetVehicle())
                    {
                        me->MonsterSay("Thanks you, i'll never forget that.", LANG_UNIVERSAL, 0);
                        me->DespawnOrUnsummon(5000);
                    }
                    savedTimer = 0;
                }
                else
                    savedTimer -= diff;
            }
        }
    };
};

class boss_zhao_ren : public CreatureScript
{
public:
    boss_zhao_ren() : CreatureScript("boss_zhao_ren") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new boss_zhao_renAI(creature);
    }

    struct boss_zhao_renAI : public ScriptedAI
    {
        boss_zhao_renAI(Creature* creature) : ScriptedAI(creature)
        {}

        EventMap _events;

        enum eEnums
        {
            QUEST_ANCIEN_MAL        = 29798,

            EVENT_DEEP_ATTACK       = 1,
            EVENT_DEEP_SEA_RUPTURE  = 2,

            SPELL_DEEP_ATTACK       = 117287,
            SPELL_DEEP_SEA_RUPTURE  = 117456,
        };

        void Reset()
        {
            _events.ScheduleEvent(EVENT_DEEP_ATTACK, 10000);
            _events.ScheduleEvent(SPELL_DEEP_SEA_RUPTURE, 12500);
        }

        void JustDied(Unit* attacker)
        {
            std::list<Player*> playerList;
            GetPlayerListInGrid(playerList, me, 50.0f);

            for (auto player : playerList)
                if (player->GetQuestStatus(QUEST_ANCIEN_MAL) == QUEST_STATUS_INCOMPLETE)
                    if (player->isAlive())
                        player->KilledMonsterCredit(me->GetEntry());
        }

        void UpdateAI(const uint32 diff)
        {
            _events.Update(diff);

            switch (_events.ExecuteEvent())
            {
                case EVENT_DEEP_ATTACK:
                {
                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 25.0f, true))
                        me->CastSpell(target, SPELL_DEEP_ATTACK, false);

                    _events.ScheduleEvent(EVENT_DEEP_ATTACK, 10000);
                    break;
                }
                case EVENT_DEEP_SEA_RUPTURE:
                {
                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 25.0f, true))
                        me->CastSpell(target, SPELL_DEEP_SEA_RUPTURE, false);

                    _events.ScheduleEvent(EVENT_DEEP_ATTACK, 10000);
                    break;
                }
            }
        }
    };
};

class npc_ji_end_event : public CreatureScript
{
public:
    npc_ji_end_event() : CreatureScript("npc_ji_end_event") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_ji_end_eventAI(creature);
    }

    struct npc_ji_end_eventAI : public ScriptedAI
    {
        npc_ji_end_eventAI(Creature* creature) : ScriptedAI(creature)
        {}

        EventMap _events;

        enum eEnums
        {
            QUEST_ANCIEN_MAL        = 29798,

            EVENT_DEEP_ATTACK       = 1,
            EVENT_DEEP_SEA_RUPTURE  = 2,

            SPELL_DEEP_ATTACK       = 117287,
            SPELL_DEEP_SEA_RUPTURE  = 117456,
        };

        void Reset()
        {
            _events.ScheduleEvent(EVENT_DEEP_ATTACK, 10000);
            _events.ScheduleEvent(SPELL_DEEP_SEA_RUPTURE, 12500);
        }

        void JustDied(Unit* attacker)
        {
            std::list<Player*> playerList;
            GetPlayerListInGrid(playerList, me, 50.0f);

            for (auto player : playerList)
                if (player->GetQuestStatus(QUEST_ANCIEN_MAL) == QUEST_STATUS_INCOMPLETE)
                    if (player->isAlive())
                        player->KilledMonsterCredit(me->GetEntry());
        }

        void UpdateAI(const uint32 diff)
        {
            _events.Update(diff);

            switch (_events.ExecuteEvent())
            {
                case EVENT_DEEP_ATTACK:
                {
                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 25.0f, true))
                        me->CastSpell(target, SPELL_DEEP_ATTACK, false);

                    _events.ScheduleEvent(EVENT_DEEP_ATTACK, 10000);
                    break;
                }
                case EVENT_DEEP_SEA_RUPTURE:
                {
                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 25.0f, true))
                        me->CastSpell(target, SPELL_DEEP_SEA_RUPTURE, false);

                    _events.ScheduleEvent(EVENT_DEEP_ATTACK, 10000);
                    break;
                }
            }
        }
    };
};

void AddSC_WanderingIsland_South()
{
    new AreaTrigger_at_mandori();
    new mob_mandori_escort();
    new AreaTrigger_at_rescue_soldiers();
    new npc_hurted_soldier();
    new npc_ji_end_event();
}
