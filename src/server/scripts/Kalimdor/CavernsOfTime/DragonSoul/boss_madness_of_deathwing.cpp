#include "ScriptPCH.h"
#include "dragon_soul.h"


// send SendPlaySpellVisualKit from limbs
// 22445 + 0 before casting visual spell
// 22446 + 0 after visual spell
enum ScriptedTexts
{
    //trall
    SAY_TRALL_START             = 0,
    SAY_TRALL_START_1           = 1,
    SAY_TRALL_20PROCENT         = 2,
    SAY_TRALL_DEATH_DEATHWING   = 3,

    //Deathwing
    SAY_AGGRO                   = 0,
    SAY_KILL_ALL                = 1,
    SAY_SPELL_1                 = 2,
    SAY_SPELL_2                 = 3,
    SAY_SPELL_3                 = 4,
};

#define CreatCacheOfTheAspects RAID_MODE(GO_GREATER_CACHE_OF_THE_ASPECTS_10N, GO_GREATER_CACHE_OF_THE_ASPECTS_25N, GO_GREATER_CACHE_OF_THE_ASPECTS_10H, GO_GREATER_CACHE_OF_THE_ASPECTS_25H)

enum Actions
{
    ACTION_CORRUPTED_BLOOD = 1,
};

enum Events
{
    // Deathwing
    EVENT_ASSAULT_ASPECTS       = 1,
    EVENT_CATACLYSM,
    EVENT_CORRUPTING_PARASITE,
    EVENT_ELEMENTIUM_BOLT,

    //tentacle
    EVENT_AGONUZUNG_PAIN,
    EVENT_CRUSH,
    EVENT_IMPALE,

    //other
    EVENT_SUMMON,
    EVENT_SUMMON_MUTATED_CORRUPTION,
    ATTACK_START,

    //Phase
    PHASE_1,
    PHASE_2,
    HAS_20PROCENT_HEALTH_NEW_PHASE,

    //Phase 2
    EVENT_ELEMENTIUM_FRAGMENT,
    EVENT_ELEMENTIUM_TERROR,
    EVENT_CORRUPTED_BLOOD,
    EVENT_CONGEALING_BLOOD,


    //Phase 2 mob events
    EVENT_SHRAPNEL,
    EVENT_TETANUS,
    EVENT_CONGEALING_BLOOD_CAST,

    //trall
    EVENT_SAY_TRALL_START,
    EVENT_SAY_TRALL_1,
};

enum Spells
{
    //DeathWing
    SPELL_CATACLYSM                 = 106523,
    SPELL_CORRUPTING_PARASITE       = 108649,
    SPELL_ELEMENTIUM_BOLT           = 105651,
    SPELL_IMPALE_ASPECT             = 107029,
    //106843,

    // Mutated corruption
    SPELL_IMPALE                    = 106400,
    PELL_CRUSH                      = 106385,

    // tentacle
    SPELL_BURNING_BLODD             = 105401,

    // Phase 2
    SPELL_CONGEALING_BLOOD          = 109102,
    SPELL_SHRAPNEL                  = 106791,
    SPELL_TETANUS                   = 106728,
    SPELL_CORRUPTED_BLOOD           = 106835,
    SPELL_CORRUPTED_BLOOD_BAR       = 106843,

    // Thrall
    SPELL_ASTRAL_RECALL             = 108537, 
    SPELL_LIMB_EMERGE_VISUAL        = 107991,
    SPELL_IDLE                      = 106187, // tail tentacle has it
    SPELL_TRIGGER_ASPECT_BUFFS      = 106943, // casted by deathwing 56173
    SPELL_SHARE_HEALTH              = 109547, // casted by deathwing 56173 on self ?
    SPELL_ASSAULT_ASPECTS           = 107018, // casted by deathwing 56173
    SPELL_CRASH                     = 109628, // casted by mutated tentacle and tail tentacle ?
    SPELL_ACHIEVEMENT               = 111533,

    // Jump Pad
    SPELL_CARRYING_WINDS_1          = 106663, // casted by player, from 1 to 2
    SPELL_CARRYING_WINDS_SCRIPT_1   = 106666, // casted by pad on player
    SPELL_CARRYING_WINDS_2          = 106668, // from 2 to 1
    SPELL_CARRYING_WINDS_SCRIPT_2   = 106669,
    SPELL_CARRYING_WINDS_3          = 106670, // from 2 to 3
    SPELL_CARRYING_WINDS_SCRIPT_3   = 106671,
    SPELL_CARRYING_WINDS_4          = 106672, // from 3 to 2
    SPELL_CARRYING_WINDS_SCRIPT_4   = 106673,
    SPELL_CARRYING_WINDS_5          = 106674, // from 3 to 4
    SPELL_CARRYING_WINDS_SCRIPT_5   = 106675,
    SPELL_CARRYING_WINDS_6          = 106676, // from 4 to 3
    SPELL_CARRYING_WINDS_SCRIPT_6   = 106677,

    SPELL_CARRYING_WINDS_DUMMY      = 106678, // visual ?

    SPELL_CARRYING_WINDS_SPEED_10   = 106664,
    SPELL_CARRYING_WINDS_SPEED_25   = 109963,
    SPELL_CARRYING_WINDS_SPEED_10H  = 109962,
    SPELL_CARRYING_WINDS_SPEED_25H  = 109961,
};

enum Adds
{
    NPC_BLISTERING_TENTACLE         = 56188,

    NPC_COSMETIC_TENTACLE           = 57693,
    NPC_TAIL_TENTACLE               = 56844,
    NPC_DEATHWIND_WING_R            = 57695,
    NPC_DEATHWIND_WING_L            = 57696,
    NPC_DEATHWIND_ARM_R             = 57686,
    NPC_DEATHWIND_ARM_L             = 57694,

    NPC_JUMP_PAD                    = 56699,
};

const Position MutatedCorruptionPos[4] = 
{
    {-12107.4f, 12201.9f, -5.32397f, 5.16617f},
    {-12028.8f, 12265.6f, -6.27147f, 4.13643f},
    {-11993.3f, 12286.3f, -2.58115f, 5.91667f},
    {-12160.9f, 12057.0f, 2.47362f,  0.733038f}
};

const Position thrallPos = {-12128.3f, 12253.8f, 0.0451f, 0.0f}; // Thrall teleports here

const Position jumpPos[] = 
{
    {-12026.30f, 12223.31f, -6.05f, 0.0f}, // from 1 to 2
    {-11977.64f, 12268.25f, 1.39f,  0.0f}, // from 2 to 1
    {-12086.50f, 12167.23f, -2.64f, 0.0f}, // from 2 to 3
    {-12051.42f, 12218.58f, -5.93f, 0.0f}, // from 3 to 2
    {-12118.76f, 12079.09f, 2.40f,  0.0f}, // from 3 to 4
    {-12099.18f, 12147.16f, -2.64f, 0.0f}  // from 4 to 3
};

const Position CreatureSpawnPos[14] =
{
    /* NPC_DEATHWING, */            {-11903.9f,  11989.1f,   -113.204f,  2.16421f},
    /* NPC_DEATHWIND_ARM_L, */      {-11967.1f,  11958.8f,   -49.9822f,  2.16421f},
    /* NPC_DEATHWIND_ARM_R, */      {-11852.1f,  12036.4f,   -49.9821f,  2.16421f},
    /* NPC_DEATHWIND_WING_L, */     {-11913.8f,  11926.5f,   -60.3749f,  2.16421f},
    /* NPC_DEATHWIND_WING_R, */     {-11842.2f,  11974.8f,   -60.3748f,  2.16421f},
    /* NPC_TAIL_TENTACLE, */        {-11857.0f,  11795.6f,   -73.9549f,  2.23402f},

    /* NPC_ALEXSTRASZA_DRAGON, */   {-11957.3f,  12338.3f,   38.9364f,   5.06145f},
    /* NPC_NOZDORMU_DRAGON, */      {-12093.8f,  12312.0f,   43.228f,    5.42797f},
    /* NPC_YSERA_DRAGON, */         {-12157.4f,  12212.5f,   36.0152f,   5.75959f},
    /* NPC_KALECGOS_DRAGON, */      {-12224.0f,  12129.0f,   41.9999f,   5.82915f},

    /* NPC_ARM_TENTACLE_1, */       {-12005.8f,  12190.3f,   -6.59399f,  2.1293f},
    /* NPC_ARM_TENTACLE_2, */       {-12065.0f,  12127.2f,   -3.2946f,   2.33874f},
    /* NPC_WING_TENTACLE_1, */      {-11941.2f,  12248.9f,   12.1499f,   1.98968f},
    /* NPC_WING_TENTACLE_2, */      {-12097.8f,  12067.4f,   13.4888f,   2.21657f}
};

class boss_deathwing : public CreatureScript
{
public:
    boss_deathwing() : CreatureScript("boss_deathwing") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new boss_deathwingAI(creature);
    }

    struct boss_deathwingAI: public BossAI
    {
        boss_deathwingAI(Creature* creature) : BossAI(creature, DATA_DEATHWING)
        {
            instance = creature->GetInstanceScript();
            me->SetCanFly(true);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
        }

        InstanceScript* instance;
        EventMap events;

        void Reset()
        {
            events.Reset();
            me->RemoveAllAuras();
            me->SetCanFly(true);
            me->SetReactState(REACT_AGGRESSIVE);

            Talk(SAY_AGGRO);
            _Reset();
        }

        void EnterEvadeMode()
        {
            Reset();

            instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me); // Remove

            if (Unit* creature = me->FindNearestCreature(NPC_DEATHWING, 600.0f))
                me->DespawnOrUnsummon(100);
            if (Unit* creature = me->FindNearestCreature(NPC_MUTATED_CORRUPTION, 600.0f))
                me->DespawnOrUnsummon(100);

            if (Creature* MaelstormThrall = me->FindNearestCreature(NPC_THRALL_2, 400.0f))
            {
                MaelstormThrall->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
            }

            Talk(SAY_KILL_ALL);
            _EnterEvadeMode();
        }

        void EnterCombat(Unit* /*who*/)
        {
            events.ScheduleEvent(PHASE_1, 1);

            instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me);

            events.ScheduleEvent(EVENT_SAY_TRALL_START, 5000);
            _EnterCombat();

            me->SummonCreature(NPC_DEATHWIND_ARM_L,       CreatureSpawnPos[1]);
            me->SummonCreature(NPC_DEATHWIND_ARM_R,       CreatureSpawnPos[2]);
            me->SummonCreature(NPC_DEATHWIND_WING_L,      CreatureSpawnPos[3]);
            me->SummonCreature(NPC_DEATHWIND_WING_R,      CreatureSpawnPos[4]);
            me->SummonCreature(NPC_TAIL_TENTACLE,         CreatureSpawnPos[5]);
            me->SummonCreature(NPC_ALEXSTRASZA_DRAGON,    CreatureSpawnPos[6]);
            me->SummonCreature(NPC_NOZDORMU_DRAGON,       CreatureSpawnPos[7]);
            me->SummonCreature(NPC_YSERA_DRAGON,          CreatureSpawnPos[8]);
            me->SummonCreature(NPC_KALECGOS_DRAGON,       CreatureSpawnPos[9]);
            me->SummonCreature(NPC_ARM_TENTACLE_1,        CreatureSpawnPos[10]);
            me->SummonCreature(NPC_ARM_TENTACLE_2,        CreatureSpawnPos[11]);
            me->SummonCreature(NPC_WING_TENTACLE_1,       CreatureSpawnPos[12]);
            me->SummonCreature(NPC_WING_TENTACLE_2,       CreatureSpawnPos[13]);
        }

        void UpdateAI(uint32 diff)
        {
            if (!UpdateVictim() || me->HasUnitState(UNIT_STATE_CASTING))
                return;

            events.Update(diff);

            if (me->HealthBelowPct(20))
            {
                events.ScheduleEvent(HAS_20PROCENT_HEALTH_NEW_PHASE, 150);

                return;
            }

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    // SAY
                case EVENT_SAY_TRALL_START:
                    if (Creature* trall = me->FindNearestCreature(NPC_THRALL_2, 300.0f, true))
                        trall->AI()->Talk(SAY_TRALL_START);
                    events.ScheduleEvent(EVENT_SAY_TRALL_1, 3000);
                    break;

                case EVENT_SAY_TRALL_1:
                    if (Creature* trall = me->FindNearestCreature(NPC_THRALL_2, 300.0f, true))
                        trall->AI()->Talk(SAY_TRALL_START_1);
                    break;

                    //Phase 1
                case PHASE_1:
                    events.ScheduleEvent(EVENT_ASSAULT_ASPECTS, 3000);

                    events.ScheduleEvent(EVENT_CATACLYSM, 2000);
                    events.ScheduleEvent(EVENT_CORRUPTING_PARASITE, urand(60000, 120000));
                    events.ScheduleEvent(EVENT_ELEMENTIUM_BOLT, 150);
                    //events.ScheduleEvent(HAS_20PROCENT_HEALTH_NEW_PHASE, 150);
                    break;

                case EVENT_ELEMENTIUM_BOLT:
                    break;

                case EVENT_CATACLYSM:
                    {
                        if (Creature* ARM_TENTACLE_1 = me->FindNearestCreature(NPC_ARM_TENTACLE_1, 600.0f))
                            if (ARM_TENTACLE_1->GetHealthPct() < 50.0f)
                                DoCast(SPELL_CATACLYSM);

                        if (Creature* ARM_TENTACLE_2 = me->FindNearestCreature(NPC_ARM_TENTACLE_2, 600.0f))
                            if (ARM_TENTACLE_2->GetHealthPct() < 50.0f)
                                DoCast(SPELL_CATACLYSM);

                        if (Creature* WING_TENTACLE_1 = me->FindNearestCreature(NPC_WING_TENTACLE_1, 600.0f))
                            if (WING_TENTACLE_1->GetHealthPct() < 50.0f)
                                DoCast(SPELL_CATACLYSM);

                        if (Creature* WING_TENTACLE_2 = me->FindNearestCreature(NPC_WING_TENTACLE_2, 600.0f))
                            if (WING_TENTACLE_2->GetHealthPct() < 50.0f)
                                DoCast(SPELL_CATACLYSM);
                    }

                    events.ScheduleEvent(EVENT_CATACLYSM, 3000);
                    break;

                case HAS_20PROCENT_HEALTH_NEW_PHASE:
                    if (me->GetHealthPct() < 20)
                    {
                        events.CancelEvent(HAS_20PROCENT_HEALTH_NEW_PHASE);
                        events.ScheduleEvent(PHASE_2, 150);
                        if (Creature* trall = me->FindNearestCreature(NPC_THRALL_2, 300.0f, true))
                        {
                            trall->AI()->Talk(SAY_TRALL_20PROCENT);
                            if (Player* players = trall->FindNearestPlayer(500.0f))
                                trall->SendPlaySound(26600, players);
                        }
                    }
                    events.ScheduleEvent(HAS_20PROCENT_HEALTH_NEW_PHASE, 5000);
                    break;

                case EVENT_ASSAULT_ASPECTS:
                    if (Unit* target = ObjectAccessor::GetCreature(*me, instance->GetData64(NPC_THRALL_2)))
                        DoCast(target, SPELL_ASSAULT_ASPECTS);
                    events.ScheduleEvent(EVENT_ASSAULT_ASPECTS, urand(40000, 80000));
                    break;

                case EVENT_CORRUPTING_PARASITE:
                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM))
                    {
                        DoCast(target, SPELL_CORRUPTING_PARASITE);
                        me->SummonCreature(NPC_CORRUPTION_PARASITE, target->GetPositionX(), target->GetPositionY(), target->GetPositionZ(), 0.0f, TEMPSUMMON_CORPSE_DESPAWN, 120000);
                    }
                    events.ScheduleEvent(EVENT_CORRUPTING_PARASITE, urand(40000, 80000));
                    break;

                case PHASE_2:
                    events.CancelEvent(EVENT_ASSAULT_ASPECTS);
                    //events.CancelEvent(EVENT_CATACLYSM);
                    events.CancelEvent(EVENT_CORRUPTING_PARASITE);
                    events.CancelEvent(EVENT_ELEMENTIUM_BOLT);
                    events.CancelEvent(PHASE_2);
                    //me->Kill(me);

                    if (IsHeroic())
                        events.ScheduleEvent(EVENT_CONGEALING_BLOOD,    urand(30000, 60000));
                    events.ScheduleEvent(EVENT_ELEMENTIUM_FRAGMENT,     urand(60000, 120000));
                    events.ScheduleEvent(EVENT_ELEMENTIUM_TERROR,       urand(40000, 80000));
                    events.ScheduleEvent(EVENT_CORRUPTED_BLOOD,         150);
                    break;

                case EVENT_CORRUPTED_BLOOD:
                    if (me->GetHealthPct() < 15)
                    {
                        events.CancelEvent(EVENT_CORRUPTED_BLOOD);
                        DoCast(SPELL_CORRUPTED_BLOOD);
                    }
                    events.ScheduleEvent(EVENT_CORRUPTED_BLOOD, 5000);
                    break;

                case EVENT_CONGEALING_BLOOD:
                    if(Unit* target = SelectTarget(SELECT_TARGET_RANDOM))
                        me->SummonCreature(NPC_CONGEALING_BLOOD, target->GetPositionX(), target->GetPositionY(), target->GetPositionZ(), 0.0f, TEMPSUMMON_CORPSE_DESPAWN, 120000);
                    events.ScheduleEvent(EVENT_CORRUPTED_BLOOD, urand(30000, 80000));
                    break;

                case EVENT_ELEMENTIUM_TERROR:
                    events.ScheduleEvent(EVENT_ELEMENTIUM_FRAGMENT, urand(60000, 90000));
                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM))
                        me->SummonCreature(NPC_ELEMENTIUM_FRAGMENT, target->GetPositionX(), target->GetPositionY(), target->GetPositionZ(), 0.0f, TEMPSUMMON_CORPSE_DESPAWN, 120000);
                    break;

                case EVENT_ELEMENTIUM_FRAGMENT:
                    events.ScheduleEvent(EVENT_ELEMENTIUM_FRAGMENT, urand(120000, 200000));
                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM))
                        me->SummonCreature(NPC_ELEMENTIUM_TERROR, target->GetPositionX(), target->GetPositionY(), target->GetPositionZ(), 0.0f, TEMPSUMMON_CORPSE_DESPAWN, 120000);
                    break;
                default:
                    break;
                }
            }
        }

        void JustDied(Unit* /*killer*/)
        {
            instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);

            if (Unit* creature = me->FindNearestCreature(NPC_DEATHWIND_ARM_L, 100.0f))
                me->Kill(creature);
            if (Unit* creature = me->FindNearestCreature(NPC_DEATHWIND_ARM_R, 100.0f))
                me->Kill(creature);
            if (Unit* creature = me->FindNearestCreature(NPC_DEATHWIND_WING_L, 100.0f))
                me->Kill(creature);
            if (Unit* creature = me->FindNearestCreature(NPC_DEATHWIND_WING_R, 100.0f))
                me->Kill(creature);
            if (Unit* creature = me->FindNearestCreature(NPC_TAIL_TENTACLE, 100.0f))
                me->DespawnOrUnsummon(100);
            if (Unit* creature = me->FindNearestCreature(NPC_MUTATED_CORRUPTION, 600.0f))
                me->DespawnOrUnsummon(100);


            if (Unit* killer = me->FindNearestPlayer(1000.0f))
                killer->SummonGameObject(CreatCacheOfTheAspects, -12075.2f,  12168.2f, -2.56926f, 3.57793f, 0.0f, 0.0f, -0.976295f, 0.216442f, 320000);

            if (Creature* trall = me->FindNearestCreature(NPC_THRALL_2, 300.0f, true))
                trall->AI()->Talk(SAY_TRALL_DEATH_DEATHWING);

            _JustDied();
            //me->DespawnOrUnsummon(5000);
        }
    };
};

class boss_deathwing_head : public CreatureScript
{
public:
    boss_deathwing_head() : CreatureScript("boss_deathwing_head") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new boss_deathwing_headAI(creature);
    }

    struct boss_deathwing_headAI: public BossAI
    {
        boss_deathwing_headAI(Creature* creature) : BossAI(creature, DATA_DAMAGE_DEATHWING)
        {
            instance = creature->GetInstanceScript();
            me->SetCanFly(true);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
        }

        InstanceScript* instance;
        EventMap events;

        void Reset()
        {
            events.Reset();
            me->RemoveAllAuras();
            me->SetCanFly(true);
            me->SetReactState(REACT_PASSIVE);

            _Reset();
        }

        void EnterEvadeMode()
        {
            Reset();

            instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
            instance->SetBossState(DATA_DAMAGE_DEATHWING, NOT_STARTED);

            if (Creature* MaelstormThrall = me->FindNearestCreature(NPC_THRALL_2, 400.0f))
            {
                MaelstormThrall->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
            }

            _EnterEvadeMode();
        }

        void EnterCombat(Unit* /*who*/)
        {
            instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me);
            instance->SetBossState(DATA_DAMAGE_DEATHWING, IN_PROGRESS);

            events.ScheduleEvent(EVENT_SAY_TRALL_START, 5000);
            _EnterCombat();
        }

        void UpdateAI(uint32 diff)
        {
            if (!UpdateVictim() || me->HasUnitState(UNIT_STATE_CASTING))
                return;

            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                case EVENT_SAY_TRALL_START:
                    break;
                default:
                    break;
                }
            }
        }

        void JustDied(Unit* /*killer*/)
        {
            instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
            instance->SetBossState(DATA_DAMAGE_DEATHWING, DONE);

            me->DespawnOrUnsummon(5000);
            _JustDied();
        }
    };
};

class npc_maelstrom_trall : public CreatureScript
{
public:
    npc_maelstrom_trall() : CreatureScript("npc_maelstrom_trall") { }

    bool OnGossipHello(Player* player, Creature* creature)
    {
        if (InstanceScript* instance = creature->GetInstanceScript())
        {
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "WE ARE READY", GOSSIP_SENDER_MAIN, 10);
        }

        player->SEND_GOSSIP_MENU(DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 sender, uint32 uiAction)
    {
        if (sender == GOSSIP_SENDER_MAIN)
        {
            player->PlayerTalkClass->ClearMenus();
            switch (uiAction)
            {
            case 10:
                if (InstanceScript* instance = creature->GetInstanceScript())
                {
                    player->CLOSE_GOSSIP_MENU();
                    creature->SummonCreature(NPC_DEATHWING, CreatureSpawnPos[0]);

                    if (Creature* deathwing = creature->FindNearestCreature(NPC_DEATHWING, 400.0f))
                    {
                        deathwing->AI()->AttackStart(player);
                        deathwing->SetInCombatWithZone();
                    }

                    creature->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                }
                break;
            default:
                break;
            }
        }
        return true;
    }
};

class npc_arm_tentacle_one : public CreatureScript
{
public:
    npc_arm_tentacle_one() : CreatureScript("npc_arm_tentacle_one") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_arm_tentacle_oneAI(creature);
    }

    struct npc_arm_tentacle_oneAI : public ScriptedAI
    {
        npc_arm_tentacle_oneAI(Creature* creature) : ScriptedAI(creature)
        {
            me->SetReactState(REACT_PASSIVE);
            me->AttackStop();
        }

        InstanceScript* instance;
        EventMap events;

        void Reset()
        {
            DoZoneInCombat();
            me->RemoveAllAuras();

            events.Reset();
        }

        void EnterCombat(Unit* /*who*/)
        {
            DoCast(SPELL_BURNING_BLODD);
            events.ScheduleEvent(EVENT_SUMMON_MUTATED_CORRUPTION, 3000);
        }

        void UpdateAI(uint32 diff)
        {
            if (!UpdateVictim() || me->HasUnitState(UNIT_STATE_CASTING))
                return;

            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                case EVENT_SUMMON_MUTATED_CORRUPTION:
                    events.ScheduleEvent(EVENT_SUMMON_MUTATED_CORRUPTION, 3000);

                    if (me->GetHealthPct() < 50)
                    {
                        switch (me->GetMap()->GetDifficulty())
                        {
                        case MAN10_DIFFICULTY:
                        case MAN10_HEROIC_DIFFICULTY:
                            {
                                if (me->FindNearestPlayer(15.0f)/* >= 8*/)
                                {
                                    events.CancelEvent(EVENT_SUMMON_MUTATED_CORRUPTION);
                                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM))
                                    {
                                        me->SummonCreature(NPC_MUTATED_CORRUPTION, MutatedCorruptionPos[1], TEMPSUMMON_CORPSE_DESPAWN, 120000);
                                    }
                                }
                                break;
                            }

                        case MAN25_DIFFICULTY:
                        case MAN25_HEROIC_DIFFICULTY:
                            {
                                if (me->FindNearestPlayer(15.0f/*, true*/)/* >= 20*/)
                                {
                                    events.CancelEvent(EVENT_SUMMON_MUTATED_CORRUPTION);
                                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM))
                                    {
                                        me->SummonCreature(NPC_MUTATED_CORRUPTION, MutatedCorruptionPos[1], TEMPSUMMON_CORPSE_DESPAWN, 120000);
                                    }
                                }
                                break;
                            }
                        }
                    }
                    break;
                }
            }
        }

        void JustDied(Unit* /*killer*/)
        {
            if (Unit* DeathwingTR = me->FindNearestCreature(NPC_DEATHWING, 600.0f))
            {
                me->AddAura(SPELL_AGONUZUNG_PAIN, DeathwingTR);
                if (DeathwingTR->GetHealthPct() >= 99)
                    DeathwingTR->SetHealth(DeathwingTR->CountPctFromMaxHealth(80));
                else if (DeathwingTR->GetHealthPct() >= 79)
                    DeathwingTR->SetHealth(DeathwingTR->CountPctFromMaxHealth(60));
                else if (DeathwingTR->GetHealthPct() >= 59)
                    DeathwingTR->SetHealth(DeathwingTR->CountPctFromMaxHealth(40));
                else if (DeathwingTR->GetHealthPct() >= 39)
                    DeathwingTR->SetHealth(DeathwingTR->CountPctFromMaxHealth(20));
            }
        }
    };
};

class npc_arm_tentacle_two : public CreatureScript
{
public:
    npc_arm_tentacle_two() : CreatureScript("npc_arm_tentacle_two") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_arm_tentacle_twoAI(creature);
    }

    struct npc_arm_tentacle_twoAI : public ScriptedAI
    {
        npc_arm_tentacle_twoAI(Creature* creature) : ScriptedAI(creature)
        {
            me->SetReactState(REACT_PASSIVE);
            me->AttackStop();
        }

        InstanceScript* instance;
        EventMap events;

        void Reset()
        {
            DoZoneInCombat();
            me->RemoveAllAuras();
            events.Reset();
        }

        void EnterCombat(Unit* /*who*/)
        {
            DoCast(SPELL_BURNING_BLODD);
            events.ScheduleEvent(EVENT_SUMMON_MUTATED_CORRUPTION, 3000);
        }

        void UpdateAI(uint32 diff)
        {
            if (!UpdateVictim() || me->HasUnitState(UNIT_STATE_CASTING))
                return;

            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                case EVENT_SUMMON_MUTATED_CORRUPTION:
                    events.ScheduleEvent(EVENT_SUMMON_MUTATED_CORRUPTION, 3000);

                    if (me->GetHealthPct() < 50)
                    {
                        switch (me->GetMap()->GetDifficulty())
                        {
                        case MAN10_DIFFICULTY:
                        case MAN10_HEROIC_DIFFICULTY:
                            {
                                if (me->FindNearestPlayer(15.0f)/* >= 8*/)
                                {
                                    events.CancelEvent(EVENT_SUMMON_MUTATED_CORRUPTION);
                                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM))
                                    {
                                        me->SummonCreature(NPC_MUTATED_CORRUPTION, MutatedCorruptionPos[2], TEMPSUMMON_CORPSE_DESPAWN, 120000);
                                    }
                                }
                                break;
                            }

                        case MAN25_DIFFICULTY:
                        case MAN25_HEROIC_DIFFICULTY:
                            {
                                if (me->FindNearestPlayer(15.0f)/* >= 20*/)
                                {
                                    events.CancelEvent(EVENT_SUMMON_MUTATED_CORRUPTION);
                                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM))
                                    {
                                        me->SummonCreature(NPC_MUTATED_CORRUPTION, MutatedCorruptionPos[2], TEMPSUMMON_CORPSE_DESPAWN, 120000);
                                    }
                                }
                                break;
                            }
                        }
                    }
                    break;
                }
            }
        }

        void JustDied(Unit* /*killer*/)
        {
            if (Unit* DeathwingTR = me->FindNearestCreature(NPC_DEATHWING, 600.0f))
            {
                me->AddAura(SPELL_AGONUZUNG_PAIN, DeathwingTR);
                if (DeathwingTR->GetHealthPct() >= 99)
                    DeathwingTR->SetHealth(DeathwingTR->CountPctFromMaxHealth(80));
                else if (DeathwingTR->GetHealthPct() >= 79)
                    DeathwingTR->SetHealth(DeathwingTR->CountPctFromMaxHealth(60));
                else if (DeathwingTR->GetHealthPct() >= 59)
                    DeathwingTR->SetHealth(DeathwingTR->CountPctFromMaxHealth(40));
                else if (DeathwingTR->GetHealthPct() >= 39)
                    DeathwingTR->SetHealth(DeathwingTR->CountPctFromMaxHealth(20));
            }
        }
    };
};

class npc_wing_tentacle_one : public CreatureScript
{
public:
    npc_wing_tentacle_one() : CreatureScript("npc_wing_tentacle_one") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_wing_tentacle_oneAI(pCreature);
    }

    struct npc_wing_tentacle_oneAI : public ScriptedAI
    {
        npc_wing_tentacle_oneAI(Creature* pCreature) : ScriptedAI(pCreature)
        {             
            me->setActive(true);
            me->SetReactState(REACT_PASSIVE);
        }

        void IsSummonedBy(Unit* /*owner*/)
        {
            events.ScheduleEvent(EVENT_SUMMON_MUTATED_CORRUPTION, 3000);
        }

        void EnterCombat(Unit* /*who*/)
        {
            DoCast(SPELL_BURNING_BLODD);
            me->AttackStop();
        }

        void JustDied(Unit* /*killer*/)
        {
            if (Unit* DeathwingTR = me->FindNearestCreature(NPC_DEATHWING, 600.0f))
            {
                me->AddAura(SPELL_AGONUZUNG_PAIN, DeathwingTR);
                if (DeathwingTR->GetHealthPct() >= 99)
                    DeathwingTR->SetHealth(DeathwingTR->CountPctFromMaxHealth(80));
                else if (DeathwingTR->GetHealthPct() >= 79)
                    DeathwingTR->SetHealth(DeathwingTR->CountPctFromMaxHealth(60));
                else if (DeathwingTR->GetHealthPct() >= 59)
                    DeathwingTR->SetHealth(DeathwingTR->CountPctFromMaxHealth(40));
                else if (DeathwingTR->GetHealthPct() >= 39)
                    DeathwingTR->SetHealth(DeathwingTR->CountPctFromMaxHealth(20));
            }
        }

        void Reset()
        {
            DoZoneInCombat();
            me->RemoveAllAuras();
            events.Reset();
        }

        void UpdateAI(uint32 diff)
        {
            events.Update(diff);

            if (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                case EVENT_SUMMON_MUTATED_CORRUPTION:
                    events.ScheduleEvent(EVENT_SUMMON_MUTATED_CORRUPTION, 3000);

                    if (me->GetHealthPct() < 50)
                    {
                        switch (me->GetMap()->GetDifficulty())
                        {
                        case MAN10_DIFFICULTY:
                        case MAN10_HEROIC_DIFFICULTY:
                            {
                                if (me->FindNearestPlayer(15.0f)/* >= 8*/)
                                {
                                    events.CancelEvent(EVENT_SUMMON_MUTATED_CORRUPTION);
                                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM))
                                    {
                                        me->SummonCreature(NPC_MUTATED_CORRUPTION, MutatedCorruptionPos[4], TEMPSUMMON_CORPSE_DESPAWN, 120000);
                                    }
                                }
                                break;
                            }

                        case MAN25_DIFFICULTY:
                        case MAN25_HEROIC_DIFFICULTY:
                            {
                                if (me->FindNearestPlayer(15.0f)/* >= 20*/)
                                {
                                    events.CancelEvent(EVENT_SUMMON_MUTATED_CORRUPTION);
                                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM))
                                    {
                                        me->SummonCreature(NPC_MUTATED_CORRUPTION, MutatedCorruptionPos[4], TEMPSUMMON_CORPSE_DESPAWN, 120000);
                                    }
                                }
                                break;
                            }
                        }
                    }
                    break;
                default:
                    break;
                }
            }
        }
    private:
        EventMap events;
        InstanceScript* instance;
    };
};

class npc_wing_tentacle_two: public CreatureScript
{
public:
    npc_wing_tentacle_two() : CreatureScript("npc_wing_tentacle_two") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_wing_tentacle_twoAI(pCreature);
    }

    struct npc_wing_tentacle_twoAI : public ScriptedAI
    {
        npc_wing_tentacle_twoAI(Creature* pCreature) : ScriptedAI(pCreature)
        {             
            me->setActive(true);
            me->SetReactState(REACT_PASSIVE);
        }

        void IsSummonedBy(Unit* /*owner*/)
        {
            events.ScheduleEvent(EVENT_SUMMON_MUTATED_CORRUPTION, 3000);
        }

        void EnterCombat(Unit* /*who*/)
        {
            DoCast(SPELL_BURNING_BLODD);
            me->AttackStop();
        }

        void JustDied(Unit* /*killer*/)
        {
            if (Unit* DeathwingTR = me->FindNearestCreature(NPC_DEATHWING, 600.0f))
            {
                me->AddAura(SPELL_AGONUZUNG_PAIN, DeathwingTR);
                if (DeathwingTR->GetHealthPct() >= 99)
                    DeathwingTR->SetHealth(DeathwingTR->CountPctFromMaxHealth(80));
                else if (DeathwingTR->GetHealthPct() >= 79)
                    DeathwingTR->SetHealth(DeathwingTR->CountPctFromMaxHealth(60));
                else if (DeathwingTR->GetHealthPct() >= 59)
                    DeathwingTR->SetHealth(DeathwingTR->CountPctFromMaxHealth(40));
                else if (DeathwingTR->GetHealthPct() >= 39)
                    DeathwingTR->SetHealth(DeathwingTR->CountPctFromMaxHealth(20));
            }
        }

        void Reset()
        {
            DoZoneInCombat();
            me->RemoveAllAuras();
            events.Reset();
        }

        void UpdateAI(uint32 diff)
        {
            events.Update(diff);

            if (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                case EVENT_SUMMON_MUTATED_CORRUPTION:
                    events.ScheduleEvent(EVENT_SUMMON_MUTATED_CORRUPTION, 3000);

                    if (me->GetHealthPct() < 50)
                    {
                        switch (me->GetMap()->GetDifficulty())
                        {
                        case MAN10_DIFFICULTY:
                        case MAN10_HEROIC_DIFFICULTY:
                            {
                                if (me->FindNearestPlayer(15.0f)/* >= 8*/)
                                {
                                    events.CancelEvent(EVENT_SUMMON_MUTATED_CORRUPTION);
                                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM))
                                    {
                                        me->SummonCreature(NPC_MUTATED_CORRUPTION, MutatedCorruptionPos[3], TEMPSUMMON_CORPSE_DESPAWN, 120000);
                                    }
                                }
                                break;
                            }

                        case MAN25_DIFFICULTY:
                        case MAN25_HEROIC_DIFFICULTY:
                            {
                                if (me->FindNearestPlayer(15.0f/*, true*/)/* >= 20*/)
                                {
                                    events.CancelEvent(EVENT_SUMMON_MUTATED_CORRUPTION);
                                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM))
                                    {
                                        me->SummonCreature(NPC_MUTATED_CORRUPTION, MutatedCorruptionPos[3], TEMPSUMMON_CORPSE_DESPAWN, 120000);
                                    }
                                }
                                break;
                            }
                        }
                    }
                    break;
                default:
                    break;
                }
            }
        }
    private:
        EventMap events;
        InstanceScript* instance;
    };
};

class npc_mutated_corruption: public CreatureScript
{
public:
    npc_mutated_corruption() : CreatureScript("npc_mutated_corruption") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_mutated_corruptionAI(pCreature);
    }

    struct npc_mutated_corruptionAI : public ScriptedAI
    {
        npc_mutated_corruptionAI(Creature* pCreature) : ScriptedAI(pCreature)
        {             
            me->setActive(true);
            me->SetReactState(REACT_PASSIVE);
        }

        void EnterCombat(Unit* /*who*/)
        {
            events.ScheduleEvent(EVENT_CRUSH, 5000);
            events.ScheduleEvent(EVENT_IMPALE, 20000);
        }

        void Reset()
        {
            me->RemoveAllAuras();
            events.Reset();
        }

        void JustDied(Unit* /*killer*/)
        {
            me->DespawnOrUnsummon(3000);
        }

        void UpdateAI(uint32 diff)
        {
            events.Update(diff);

            if (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                case EVENT_CRUSH:
                    if (Unit* pTarget = SelectTarget(SELECT_TARGET_RANDOM, 0, 0.0f, true))
                        me->CastSpell(pTarget, SPELL_CRASH, true);
                    events.ScheduleEvent(EVENT_CRUSH, 5000);
                    break;

                case EVENT_IMPALE:
                    if (Unit* pTarget = SelectTarget(SELECT_TARGET_TOPAGGRO, 0, 0.0f, true))
                        me->CastSpell(pTarget, SPELL_IMPALE, true);
                    events.ScheduleEvent(EVENT_IMPALE, 20000);
                    break;
                default:
                    break;
                }
            }
        }
    private:
        EventMap events;
        InstanceScript* instance;
    };
};

class npc_madness_of_deathwing_jump_pad : public CreatureScript
{
public:
    npc_madness_of_deathwing_jump_pad() : CreatureScript("npc_madness_of_deathwing_jump_pad") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_madness_of_deathwing_jump_padAI(pCreature);
    }

    struct npc_madness_of_deathwing_jump_padAI : public Scripted_NoMovementAI
    {
        npc_madness_of_deathwing_jump_padAI(Creature* pCreature) : Scripted_NoMovementAI(pCreature)
        {
            me->SetReactState(REACT_PASSIVE);
            checkTimer = 500;
            pos = 0;
            spellId = 0;
        }

        void Reset()
        {
            if (me->GetPositionY() >= 12270.0f && me->GetPositionY() <= 12275.0f)
            {
                pos = 1;
                spellId = SPELL_CARRYING_WINDS_SCRIPT_1;
            }
            else if (me->GetPositionY() >= 12225.0f && me->GetPositionY() <= 12230.0f)
            {
                pos = 2;
                spellId = SPELL_CARRYING_WINDS_SCRIPT_2;
            }
            else if (me->GetPositionY() >= 12210.0f && me->GetPositionY() <= 12215.0f)
            {
                pos = 3;
                spellId = SPELL_CARRYING_WINDS_SCRIPT_3;
            }
            else if (me->GetPositionY() >= 12162.0f && me->GetPositionY() <= 12167.0f)
            {
                pos = 4;
                spellId = SPELL_CARRYING_WINDS_SCRIPT_4;
            }
            else if (me->GetPositionY() >= 12150.0f && me->GetPositionY() <= 12155.0f)
            {
                pos = 5;
                spellId = SPELL_CARRYING_WINDS_SCRIPT_5;
            }
            else if (me->GetPositionY() >= 12080.9f && me->GetPositionY() <= 12085.0f)
            {
                pos = 6;
                spellId = SPELL_CARRYING_WINDS_SCRIPT_6;
            }
        }

        void UpdateAI(uint32 diff)
        {
            if (!spellId)
                return;

            if (checkTimer <= diff)
            {
                std::list<Player*> players;
                PlayerCheck check(me);
                Trinity::PlayerListSearcher<PlayerCheck> searcher(me, players, check);
                me->VisitNearbyObject(32.0f, searcher);
                if (!players.empty())
                    for (std::list<Player*>::const_iterator itr = players.begin(); itr != players.end(); ++ itr)
                        DoCast((*itr), spellId, true);

                checkTimer = 500;
            }
            else
                checkTimer -= diff;
        }

    private:
        uint32 checkTimer;
        uint8 pos;
        uint32 spellId;

        class PlayerCheck
        {
        public:
            PlayerCheck(WorldObject const* obj) : _obj(obj) { }
            bool operator()(Player* u)
            {
                if (!u->isAlive())
                    return false;

                if (!u->IsFalling() || _obj->GetPositionZ() < (u->GetPositionZ() + 3.0f))
                    return false;

                if (!_obj->IsWithinDistInMap(u, 32.0f))
                    return false;

                return true;
            }

        private:
            WorldObject const* _obj;
        };
    };
};

class spell_madness_of_deathwing_carrying_winds_script : public SpellScriptLoader
{
public:
    spell_madness_of_deathwing_carrying_winds_script(const char* name, uint8 pos) : SpellScriptLoader(name), _pos(pos) { }

    class spell_madness_of_deathwing_carrying_winds_script_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_madness_of_deathwing_carrying_winds_script_SpellScript);

        void HandleDummy(SpellEffIndex /*effIndex*/)
        {
            if (!GetCaster() || !GetHitUnit())
                return;

            uint32 spellId = SPELL_CARRYING_WINDS_1;
            switch (_pos)
            {
            case 1: spellId = SPELL_CARRYING_WINDS_1; break;
            case 2: spellId = SPELL_CARRYING_WINDS_2; break;
            case 3: spellId = SPELL_CARRYING_WINDS_3; break;
            case 4: spellId = SPELL_CARRYING_WINDS_4; break;
            case 5: spellId = SPELL_CARRYING_WINDS_5; break;
            case 6: spellId = SPELL_CARRYING_WINDS_6; break;

            default:
                break;
            }

            GetHitUnit()->CastSpell(jumpPos[_pos - 1].GetPositionX(), jumpPos[_pos - 1].GetPositionY(), jumpPos[_pos - 1].GetPositionZ(), spellId, true);
        }

        void Register()
        {
            OnEffectHitTarget += SpellEffectFn(spell_madness_of_deathwing_carrying_winds_script_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
        }

    public:
        spell_madness_of_deathwing_carrying_winds_script_SpellScript(uint8 pos) : SpellScript(), _pos(pos) { } 

    private:
        uint8 _pos;
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_madness_of_deathwing_carrying_winds_script_SpellScript(_pos);
    }

private:
    uint8 _pos;
};

void AddSC_madness_of_deathwing()
{
    new boss_deathwing();
    new boss_deathwing_head();
    new npc_maelstrom_trall();
    new npc_arm_tentacle_one();
    new npc_arm_tentacle_two();
    new npc_wing_tentacle_one();
    new npc_wing_tentacle_two();
    new npc_mutated_corruption();
    new npc_madness_of_deathwing_jump_pad();
    new spell_madness_of_deathwing_carrying_winds_script("spell_madness_of_deathwing_carrying_winds_script_1", 1);
    new spell_madness_of_deathwing_carrying_winds_script("spell_madness_of_deathwing_carrying_winds_script_2", 2);
    new spell_madness_of_deathwing_carrying_winds_script("spell_madness_of_deathwing_carrying_winds_script_3", 3);
    new spell_madness_of_deathwing_carrying_winds_script("spell_madness_of_deathwing_carrying_winds_script_4", 4);
    new spell_madness_of_deathwing_carrying_winds_script("spell_madness_of_deathwing_carrying_winds_script_5", 5);
    new spell_madness_of_deathwing_carrying_winds_script("spell_madness_of_deathwing_carrying_winds_script_6", 6);
}