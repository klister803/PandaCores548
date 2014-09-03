#include "ScriptPCH.h"
#include "deadmines.h"


//todo: сделать туман, правильный спавн аддов и касты аддов

enum ScriptTexts
{
    SAY_AGGRO   = 7,
    SAY_KILL    = 1,
    SAY_DEATH   = 0,
    SAY_SPELL1  = 2,
    SAY_SPELL2  = 3,
    SAY_SPELL3  = 4,
    SAY_SPELL4  = 5,
    SAY_SPELL5  = 6,
};
enum Spells
{
    SPELL_THRIST_OF_BLOOD       = 88736,
    SPELL_THRIST_OF_BLOOD_AURA  = 88737,
    SPELL_SWIPE                 = 88839,
    SPELL_SWIPE_H               = 91859,
    SPELL_FOG                   = 88768,
    SPELL_GO_FOR_THE_THROAT     = 88836,
    SPELL_GO_FOR_THE_THROAT_H   = 91863,
    SPELL_CONDENSATION          = 92013,
    SPELL_VAPOR                 = 88831,
    SPELL_FOG_1                 = 88768,
    SPELL_FOG_2                 = 88755,
};

enum Adds
{
    NPC_VAPOR   = 47714, 

    NPC_DUMMY_2 = 45979, // 88755
    NPC_DUMMY_1 = 47272, // 88768
};

enum Events
{
    EVENT_SWIPE    = 1,
};

const Position dummyPos[38] = 
{
    {-62.6927f, -814.031f, 41.3843f},
    {-67.6128f, -814.219f, 40.944f},
    {-68.1302f, -822.918f, 40.888f},
    {-62.6597f, -823.653f, 41.4015f},
    {-56.901f, -818.264f, 41.954f},
    {-72.9931f, -825.99f, 40.4834f},
    {-68.934f, -829.365f, 40.8772f},
    {-60.9792f, -835.979f, 41.5982f},
    {-65.401f, -833.352f, 41.2017f},
    {-56.9514f, -832.986f, 41.9731f},
    {-53.2882f, -810.453f, 42.2844f},
    {-73.9705f, -819.941f, 40.3588f},
    {-61.9878f, -829.885f, 41.5003f},
    {-56.4115f, -827.823f, 42.0163f},
    {-73.6458f, -814.604f, 40.402f},
    {-58.0365f, -812.931f, 41.8374f},
    {-51.8299f, -829.847f, 42.455f},
    {-47.4358f, -831.122f, 42.8763f},
    {-57.3889f, -822.721f, 41.915f},
    {-51.8837f, -833.491f, 42.4604f},
    {-75.5295f, -831.818f, 40.2749f},
    {-82.5642f, -830.498f, 39.7044f},
    {-47.6163f, -808.856f, 42.8273f},
    {-55.0469f, -805.922f, 42.1087f},
    {-60.0f, -807.571f, 41.6455f},
    {-66.6615f, -805.149f, 41.0334f},
    {-72.8733f, -808.346f, 40.4807f},
    {-79.3229f, -806.665f, 39.93f},
    {-85.6667f, -808.549f, 39.4228f},
    {-94.2292f, -809.929f, 38.7469f},
    {-101.122f, -813.936f, 38.2438f},
    {-104.384f, -819.276f, 38.0203f},
    {-101.174f, -824.677f, 38.2535f},
    {-99.684f, -818.974f, 38.3542f},
    {-95.3628f, -828.319f, 38.6867f},
    {-88.1024f, -831.63f, 39.2559f},
    {-79.2431f, -834.884f, 39.9758f},
    {-70.941f, -835.523f, 40.6859f}
};

class boss_admiral_ripsnarl : public CreatureScript
{
    public:
        boss_admiral_ripsnarl() : CreatureScript("boss_admiral_ripsnarl") { }

        CreatureAI* GetAI(Creature* pCreature) const
        {
            return new boss_admiral_ripsnarlAI (pCreature);
        }

        struct boss_admiral_ripsnarlAI : public BossAI
        {
            boss_admiral_ripsnarlAI(Creature* pCreature) : BossAI(pCreature, DATA_ADMIRAL)
            {
                me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK, true);
                me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_GRIP, true);
                me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_STUN, true);
                me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_FEAR, true);
                me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_ROOT, true);
                me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_FREEZE, true);
                me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_POLYMORPH, true);
                me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_HORROR, true);
                me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_SAPPED, true);
                me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_CHARM, true);
                me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_DISORIENTED, true);
                me->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_MOD_CONFUSE, true);
                me->setActive(true);
            }

            uint8 stage;

            void Reset()
            {
                _Reset();
                stage = 0;
            }

            void UpdateAI(uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                events.Update(diff);

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                if (me->HealthBelowPct(75) && stage == 0)
                {
                    stage = 1;
                    DoResetThreat();
                    me->RemoveAurasDueToSpell(SPELL_THRIST_OF_BLOOD_AURA);
                    for (uint8 i = 0; i < 3; i++)
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 0.0f, true))
                            DoCast(target, SPELL_VAPOR);
                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 0.0f, true))
                        DoCast(target, SPELL_GO_FOR_THE_THROAT);
                    Talk(SAY_SPELL4);
                    for (uint8 i = 0; i < 38; i++)
                        me->SummonCreature(NPC_DUMMY_1, dummyPos[i]);
                    return;
                }
                if (me->HealthBelowPct(50) && stage == 1)
                {
                    stage = 2;
                    DoResetThreat();
                    me->RemoveAurasDueToSpell(SPELL_THRIST_OF_BLOOD_AURA);
                    for (uint8 i = 0; i < 3; i++)
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 0.0f, true))
                            DoCast(target, SPELL_VAPOR);
                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 0.0f, true))
                        DoCast(target, SPELL_GO_FOR_THE_THROAT);
                    Talk(SAY_SPELL4);
                    return;
                }
                if (me->HealthBelowPct(25) && stage == 2)
                {
                    stage = 3;
                    DoResetThreat();
                    me->RemoveAurasDueToSpell(SPELL_THRIST_OF_BLOOD_AURA);
                    for (uint8 i = 0; i < 3; i++)
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 0.0f, true))
                            DoCast(target, SPELL_VAPOR);
                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 0.0f, true))
                        DoCast(target, SPELL_GO_FOR_THE_THROAT);
                    Talk(SAY_SPELL4);
                    me->SummonCreature(NPC_DUMMY_2, centershipPos);
                    return;
                }

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_SWIPE:
                            DoCast(me->getVictim(), SPELL_SWIPE);
                            events.ScheduleEvent(EVENT_SWIPE, urand(8000, 10000));
                            break;
                    }
                }

                DoMeleeAttackIfReady();
            }

            void EnterCombat(Unit* who) 
            {
                DoCast(me, SPELL_THRIST_OF_BLOOD);
                events.ScheduleEvent(EVENT_SWIPE, urand(5000, 10000));
                Talk(SAY_AGGRO);
                DoZoneInCombat();
                instance->SetBossState(DATA_ADMIRAL, IN_PROGRESS);
            }

            void KilledUnit(Unit * victim)
            {
                Talk(SAY_KILL);
            }


            void JustDied(Unit* killer)
            {
                _JustDied();
                Talk(SAY_DEATH);
            }
        };
};

class npc_vapor : public CreatureScript
{
    public:
        npc_vapor() : CreatureScript("npc_vapor") { }
     
        CreatureAI* GetAI(Creature* pCreature) const
        {
            return new npc_vaporAI (pCreature);
        }
     
        struct npc_vaporAI : public ScriptedAI
        {
            npc_vaporAI(Creature *c) : ScriptedAI(c) 
            {
            }
     
            void UpdateAI(uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                DoMeleeAttackIfReady();
            }
        };
};



void AddSC_boss_admiral_ripsnarl()
{
    new boss_admiral_ripsnarl();
    new npc_vapor();
}