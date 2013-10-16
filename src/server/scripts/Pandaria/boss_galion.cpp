/*
Pandaria
World boss
Antoine Vallee for Pandashan Servers

*/
#include "ScriptPCH.h"

enum eBosses
{
    BOSS_GALION,
};

enum eSpells
{
    SPELL_STOMP                 = 121787,
    SPELL_CANNON_BARRAGE        = 121577, 
    SPELL_FIRE_SHOT             = 121673, 
    SPELL_EMPALLING_PULL        = 121747,
    SPELL_BERSERK               =  47008,
};

enum eEvents
{
    EVENT_STOMP                 = 1,
    EVENT_CANNON                = 2,
    EVENT_FIRE_SHOT             = 3,
    EVENT_EMPALLING             = 4,
    EVENT_SPAWN                 = 6,
    EVENT_BERSERK               = 7,
};

enum eCreatures
{
    CREATURE_GALION           = 62351,
};


class boss_galion : public CreatureScript
{
public:
    boss_galion() : CreatureScript("boss_galion") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new boss_galion_AI(creature);
    }

    struct boss_galion_AI : public BossAI
    {
        boss_galion_AI(Creature* creature) : BossAI(creature, BOSS_GALION)
        {}
        void Reset()
        {
            events.Reset();
            _Reset();
        }

        void EnterCombat(Unit* unit)
        {
            events.ScheduleEvent(EVENT_STOMP, 40000);
            events.ScheduleEvent(EVENT_CANNON, 25000);
            events.ScheduleEvent(EVENT_SPAWN, 60000);
            events.ScheduleEvent(EVENT_FIRE_SHOT, 10000);
            events.ScheduleEvent(EVENT_BERSERK, 900000);
        }

        void UpdateAI(const uint32 diff)
        {
            if (!UpdateVictim())
                return;

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_STOMP:
                        DoCast(me, SPELL_STOMP);
                        events.ScheduleEvent(EVENT_STOMP, 40000);
                        break;
                    case EVENT_CANNON:
                        DoCast(me, SPELL_CANNON_BARRAGE);
                        events.ScheduleEvent(EVENT_CANNON, 50000);
                        break;
                    case EVENT_FIRE_SHOT:
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1))
                            DoCast(target, SPELL_FIRE_SHOT);
                        events.ScheduleEvent(EVENT_FIRE_SHOT, 5000);
                        break;
                    case EVENT_SPAWN:
                        for (uint8 i = 0; i < 6; ++i)
                            me->SummonCreature(CREATURE_GALION, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ());

                        events.ScheduleEvent(EVENT_SPAWN, 60000);
                        break;
                    case EVENT_BERSERK:
                        me->CastSpell(me, SPELL_BERSERK);
                        break;
                }
            }
            DoMeleeAttackIfReady();
        }
    };
};

class npc_galion : public CreatureScript
{
    public:
        npc_galion() : CreatureScript("npc_galion") { }

        struct npc_galionAI : public ScriptedAI
        {
            npc_galionAI(Creature* creature) : ScriptedAI(creature)
            {
            }
        EventMap events;
        void Reset()
        {
            events.Reset();
        }
        void EnterCombat(Unit* unit)
        {
            events.ScheduleEvent(EVENT_EMPALLING, 50000);
        }
        void UpdateAI(const uint32 diff)
        {
            if (!UpdateVictim())
                return;

            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_EMPALLING:
                    {
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1))
                            me->CastSpell(target,SPELL_EMPALLING_PULL,true);
                        events.ScheduleEvent(EVENT_EMPALLING, 60000);
                        break;
                    }
                }
            }
            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_galionAI (creature);
    }
};

void AddSC_boss_galion()
{
    new boss_galion();
    new npc_galion();
}
