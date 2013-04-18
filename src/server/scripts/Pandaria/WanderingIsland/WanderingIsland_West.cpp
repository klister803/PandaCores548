#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ScriptedEscortAI.h"
#include "Vehicle.h"

class mob_master_shang_xi_temple : public CreatureScript
{
    public:
        mob_master_shang_xi_temple() : CreatureScript("mob_master_shang_xi_temple") { }

        bool OnQuestAccept(Player* player, Creature* creature, Quest const* quest)
        {
            if (quest->GetQuestId() == 29776) // Brise du matin
            {
                if (Creature* vehicle = player->SummonCreature(55685, player->GetPositionX(), player->GetPositionY(), player->GetPositionZ(), player->GetOrientation()))
                {
                    player->AddAura(99385, vehicle);
                    player->EnterVehicle(vehicle);
                }
            }

            return true;
        }
};

class npc_wind_vehicle : public CreatureScript
{
public:
    npc_wind_vehicle() : CreatureScript("npc_wind_vehicle") { }

    struct npc_wind_vehicleAI : public npc_escortAI
    {        
        npc_wind_vehicleAI(Creature* creature) : npc_escortAI(creature)
        {}

        uint32 IntroTimer;

        void Reset()
        {
            IntroTimer = 100;
        }

        void WaypointReached(uint32 waypointId)
        {
            if (waypointId == 6)
            {
                if (me->GetVehicleKit())
                    me->GetVehicleKit()->RemoveAllPassengers();

                me->DespawnOrUnsummon();
            }
        }

        void UpdateAI(const uint32 diff)
        {
            if (IntroTimer)
            {
                if (IntroTimer <= diff)
                {
                    Start(false, true);
                    IntroTimer = 0;
                }
                else
                    IntroTimer -= diff;
            }

            npc_escortAI::UpdateAI(diff);
        }
    };
    
    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_wind_vehicleAI(creature);
    }
    
};

class mob_frightened_wind : public CreatureScript
{
public:
    mob_frightened_wind() : CreatureScript("mob_frightened_wind") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new mob_frightened_windAI(creature);
    }

    struct mob_frightened_windAI : public ScriptedAI
    {
        mob_frightened_windAI(Creature* creature) : ScriptedAI(creature)
        {}

        uint32 tornadeTimer;

        enum Spells
        {
            SPELL_TORNADE    = 107278,
        };

        void Reset()
        {
            me->SetReactState(REACT_PASSIVE);
            tornadeTimer = 8 * IN_MILLISECONDS;
        }

        void UpdateAI(const uint32 diff)
        {
            if (tornadeTimer <= diff)
            {
                me->ToggleAura(SPELL_TORNADE, me);
                tornadeTimer = 8 * IN_MILLISECONDS;
            }
            else
                tornadeTimer -= diff;
        }
    };
};

enum Enums
{
    NPC_ROCKET_LAUNCHER = 64507,
    SPELL_ROCKET_LAUNCH = 104855,
            
    EVENT_NEXT_MOVEMENT = 1,
    EVENT_STUNNED       = 2,
    EVENT_LIGHTNING     = 3,

    SPELL_SERPENT_SWEEP = 125990,
    SPELL_STUNNED       = 125992,
    SPELL_LIGHTNING     = 126006,
};

Position ZhaoPos[] = 
{
    {719.36f, 4164.60f, 216.06f}, // Center
    {745.91f, 4154.35f, 223.48f},
    {717.04f, 4141.16f, 219.83f},
    {689.62f, 4153.16f, 217.63f},
    {684.53f, 4173.24f, 216.98f},
    {704.77f, 4190.16f, 218.24f},
    {736.90f, 4183.85f, 221.41f}
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
        bool eventStarted;
        uint8 hitCount;
        uint8 currentPos;

        void Reset()
        {
            _events.Reset();
            me->SetReactState(REACT_PASSIVE);

            eventStarted = false;
            hitCount = 0;
            currentPos = 0;

            me->SetFullHealth();
            me->RemoveAurasDueToSpell(SPELL_STUNNED);

            me->GetMotionMaster()->Clear();
            me->GetMotionMaster()->MovePoint(0, ZhaoPos[0].GetPositionX(), ZhaoPos[0].GetPositionY(), ZhaoPos[0].GetPositionZ());
        }

        void SpellHit(Unit* caster, const SpellInfo* spell)
        {
            if (spell->Id == SPELL_ROCKET_LAUNCH)
            {
                if (++hitCount >= 5)
                {
                    me->CastSpell(me, SPELL_STUNNED, true);
                    me->GetMotionMaster()->Clear();
                    me->GetMotionMaster()->MoveFall();
                    _events.ScheduleEvent(EVENT_STUNNED, 12000);
                    hitCount = 0;
                }
            }
        }
        
        bool checkPlayers()
        {
            std::list<Player*> playerList;
            GetPlayerListInGrid(playerList, me, 80.0f);

            for (auto player : playerList)
                if (player->GetQuestStatus(29786) == QUEST_STATUS_INCOMPLETE)
                    if (player->isAlive())
                        return true;

            return false;
        }

        void GoToNextPos()
        {
            if (++currentPos > 6)
                currentPos = 1;

            me->GetMotionMaster()->MovePoint(currentPos, ZhaoPos[currentPos].GetPositionX(), ZhaoPos[currentPos].GetPositionY(), ZhaoPos[currentPos].GetPositionZ());
        }

        Player* GetRandomPlayer()
        {
            std::list<Player*> playerList;
            GetPlayerListInGrid(playerList, me, 50.0f);

            if (playerList.empty())
                return NULL;

            JadeCore::Containers::RandomResizeList(playerList, 1);

            return *playerList.begin();
        }

        void MovementInform(uint32 type, uint32 id)
        {
            if (type != POINT_MOTION_TYPE)
                return;

            if (!id)
                return;

            _events.ScheduleEvent(EVENT_NEXT_MOVEMENT, 200);
        }

        void JustDied(Unit* attacker)
        {
            std::list<Player*> playerList;
            GetPlayerListInGrid(playerList, me, 50.0f);

            for (auto player : playerList)
                if (player->GetQuestStatus(29786) == QUEST_STATUS_INCOMPLETE)
                    if (player->isAlive())
                        player->KilledMonsterCredit(me->GetEntry());
        }

        void UpdateAI(const uint32 diff)
        {
            if (checkPlayers())
            {
                if (!eventStarted)  // Event not started, player found
                {
                    _events.ScheduleEvent(EVENT_NEXT_MOVEMENT, 1000);
                    _events.ScheduleEvent(EVENT_LIGHTNING, 5000);
                    eventStarted = true;
                }
            }
            else
            {
                if (eventStarted)  // Event started, no player found
                    Reset();

                return;
            }

            _events.Update(diff);

            switch (_events.ExecuteEvent())
            {
                case EVENT_NEXT_MOVEMENT:
                {
                    if (me->HasAura(SPELL_STUNNED))
                        _events.ScheduleEvent(EVENT_NEXT_MOVEMENT, 2000);

                    GoToNextPos();
                    break;
                }
                case EVENT_STUNNED:
                {
                    me->RemoveAurasDueToSpell(SPELL_STUNNED);
                    me->CastSpell(me, SPELL_SERPENT_SWEEP, false);
                    _events.ScheduleEvent(EVENT_NEXT_MOVEMENT, 3000);
                    break;
                }
                case EVENT_LIGHTNING:
                {
                    if (Player* player = GetRandomPlayer())
                        me->CastSpell(player, SPELL_LIGHTNING, false);

                    _events.ScheduleEvent(EVENT_LIGHTNING, 5000);
                    break;
                }
            }
        }
    };
};

class npc_rocket_launcher : public CreatureScript
{
public:
    npc_rocket_launcher() : CreatureScript("npc_rocket_launcher") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_rocket_launcherAI (creature);
    }

    struct npc_rocket_launcherAI : public ScriptedAI
    {
        npc_rocket_launcherAI(Creature* creature) : ScriptedAI(creature) {}

        uint32 cooldown;

        void Reset()
        {
            cooldown = 0;
            me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK);
        }

        void OnSpellClick(Unit* Clicker)
        {
            if (cooldown)
                return;

            if (Creature* zhao = GetClosestCreatureWithEntry(me, 55786, 50.0f))
                me->CastSpell(zhao, SPELL_ROCKET_LAUNCH, false);

            cooldown = 5000;
            me->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK);
        }

        void EnterCombat(Unit* /*who*/)
        {
            return;
        }

        void UpdateAI(const uint32 diff)
        {
            if (cooldown)
            {
                if (cooldown <= diff)
                {
                    me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK);
                    cooldown = 0;
                }
                else
                    cooldown -= diff;
            }
        }
    };
};

class mob_master_shang_xi_after_zhao : public CreatureScript
{
    public:
        mob_master_shang_xi_after_zhao() : CreatureScript("mob_master_shang_xi_after_zhao") { }

        bool OnQuestAccept(Player* player, Creature* creature, Quest const* quest)
        {
            if (quest->GetQuestId() == 29787) // Digne de passer
                if (Creature* master = player->SummonCreature(56159, creature->GetPositionX(), creature->GetPositionY(), creature->GetPositionZ(), creature->GetOrientation(), TEMPSUMMON_MANUAL_DESPAWN, 0, player->GetGUID()))
                    master->AI()->SetGUID(player->GetGUID());

            return true;
        }
};

class mob_master_shang_xi_after_zhao_escort : public CreatureScript
{
    public:
        mob_master_shang_xi_after_zhao_escort() : CreatureScript("mob_master_shang_xi_after_zhao_escort") { }

    struct mob_master_shang_xi_after_zhao_escortAI : public npc_escortAI
    {        
        mob_master_shang_xi_after_zhao_escortAI(Creature* creature) : npc_escortAI(creature)
        {}
        
        uint32 IntroTimer;

        uint64 playerGuid;

        void Reset()
        {
            IntroTimer = 250;
            me->SetReactState(REACT_PASSIVE);
        }

        void SetGUID(uint64 guid, int32)
        {
            playerGuid = guid;
        }

        void WaypointReached(uint32 waypointId)
        {
            switch (waypointId)
            {
                case 6:
                    me->SummonCreature(56274, 845.89f, 4372.62f, 223.98f, 4.78f, TEMPSUMMON_CORPSE_DESPAWN, 0, playerGuid);
                    break;
                case 12:
                    me->SetFacingTo(0.0f);
                    SetEscortPaused(true);
                    break;
                case 17:
                    me->SetFacingTo(4.537860f);
                    me->DespawnOrUnsummon(1000);
                    break;
                default:
                    break;
            }
        }

        void SummonedCreatureDespawn(Creature* summon)
        {
            if (summon->GetEntry() == 56274)
                SetEscortPaused(false);
        }

        void UpdateAI(const uint32 diff)
        {
            if (IntroTimer)
            {
                if (IntroTimer <= diff)
                {
                    Start(false, true);
                    IntroTimer = 0;
                }
                else
                    IntroTimer -= diff;
            }

            npc_escortAI::UpdateAI(diff);
        }
    };
    
    CreatureAI* GetAI(Creature* creature) const
    {
        return new mob_master_shang_xi_after_zhao_escortAI(creature);
    }
};

class spell_grab_air_balloon: public SpellScriptLoader
{
    public:
        spell_grab_air_balloon() : SpellScriptLoader("spell_grab_air_balloon") { }

        class spell_grab_air_balloon_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_grab_air_balloon_SpellScript);

            void HandleScriptEffect(SpellEffIndex effIndex)
            {
                PreventHitAura();

                if (Unit* caster = GetCaster())
                    if (Creature* balloon = caster->SummonCreature(55649, 915.55f, 4563.66f, 230.68f, 2.298090f, TEMPSUMMON_MANUAL_DESPAWN, 0, caster->GetGUID()))
                        caster->EnterVehicle(balloon, 0);
            }

            void Register()
            {
                OnEffectLaunch += SpellEffectFn(spell_grab_air_balloon_SpellScript::HandleScriptEffect, EFFECT_0, SPELL_EFFECT_APPLY_AURA);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_grab_air_balloon_SpellScript();
        }
};

class mob_shang_xi_air_balloon : public VehicleScript
{
    public:
        mob_shang_xi_air_balloon() : VehicleScript("mob_shang_xi_air_balloon") { }

    void OnAddPassenger(Vehicle* /*veh*/, Unit* passenger, int8 seatId)
    {
        if (seatId == 0)
            if (Player* player = passenger->ToPlayer())
                player->KilledMonsterCredit(56378);
    }

    struct mob_shang_xi_air_balloonAI : public npc_escortAI
    {        
        mob_shang_xi_air_balloonAI(Creature* creature) : npc_escortAI(creature)
        {}
        
        uint32 IntroTimer;

        void Reset()
        {
            IntroTimer = 250;
            me->SetReactState(REACT_PASSIVE);
        }

        void WaypointReached(uint32 waypointId)
        {
            switch (waypointId)
            {
                case 19:
                    if (me->GetVehicleKit())
                    {
                        if (Unit* passenger = me->GetVehicleKit()->GetPassenger(0))
                            if (Player* player = passenger->ToPlayer())
                            {
                                player->KilledMonsterCredit(55939);
                                player->AddAura(50550, player);
                            }

                        me->GetVehicleKit()->RemoveAllPassengers();
                    }

                    me->DespawnOrUnsummon();
                    break;
                default:
                    break;
            }
        }

        void UpdateAI(const uint32 diff)
        {
            if (IntroTimer)
            {
                if (IntroTimer <= diff)
                {
                    Start(false, true);
                    IntroTimer = 0;
                }
                else
                    IntroTimer -= diff;
            }

            npc_escortAI::UpdateAI(diff);
        }
    };
    
    CreatureAI* GetAI(Creature* creature) const
    {
        return new mob_shang_xi_air_balloonAI(creature);
    }
};

void AddSC_WanderingIsland_West()
{
    new mob_master_shang_xi_temple();
    new npc_wind_vehicle();
    new mob_frightened_wind();
    new boss_zhao_ren();
    new npc_rocket_launcher();
    new mob_master_shang_xi_after_zhao();
    new mob_master_shang_xi_after_zhao_escort();
    new spell_grab_air_balloon();
    new mob_shang_xi_air_balloon();
}
