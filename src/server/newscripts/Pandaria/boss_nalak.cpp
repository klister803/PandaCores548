//UWoWCore
//World boss

#include "NewScriptPCH.h"

enum eSpells
{
    SPELL_ARC_NOVA          = 136338,
    SPELL_STATIC_SHIELD     = 136341,
    SPELL_LIGHTNING_TETHER  = 136339,
    SPELL_STORMCLOUD        = 136340,
};

enum eEvents
{
    EVENT_ARC_NOVA          = 1,
    EVENT_LIGHTNING_TETHER  = 2,
    EVENT_STORMCLOUD        = 3,
};

//69099
class boss_nalak : public CreatureScript
{
public:
    boss_nalak() : CreatureScript("boss_nalak") { }

    struct boss_nalakAI : public CreatureAI
    {
        boss_nalakAI(Creature* creature) : CreatureAI(creature)
        {
            me->SetCanFly(true);
            me->SetDisableGravity(true);
            me->SetByteFlag(UNIT_FIELD_BYTES_1, 3, UNIT_BYTE1_FLAG_ALWAYS_STAND | UNIT_BYTE1_FLAG_HOVER);
        }

        EventMap events;

        void Reset()
        {
            events.Reset();
            me->SetReactState(REACT_DEFENSIVE);
        }

        void EnterCombat(Unit* unit)
        {
            DoZoneInCombat(me, 75.0f);
            //DoCast(me, SPELL_STATIC_SHIELD, true);             not work
            events.ScheduleEvent(EVENT_ARC_NOVA,         42000);
          //events.ScheduleEvent(EVENT_LIGHTNING_TETHER, 35000); not work
            events.ScheduleEvent(EVENT_STORMCLOUD,       24000);
        }
        
        void UpdateAI(const uint32 diff)
        {
            if (!UpdateVictim() || me->HasUnitState(UNIT_STATE_CASTING))
                return;

            if (me->getVictim())
                if (!me->IsWithinMeleeRange(me->getVictim()))
                    me->GetMotionMaster()->MoveCharge(me->getVictim()->GetPositionX(), me->getVictim()->GetPositionY(), me->getVictim()->GetPositionZ() + 18.0f, 15.0f);

            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                case EVENT_ARC_NOVA:
                    DoCastAOE(SPELL_ARC_NOVA);
                    events.ScheduleEvent(EVENT_ARC_NOVA, 42000);
                    break;
                case EVENT_LIGHTNING_TETHER:
                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 40.0f, true))
                        DoCast(target, SPELL_LIGHTNING_TETHER);
                    events.ScheduleEvent(EVENT_LIGHTNING_TETHER, 35000);
                    break;
                case EVENT_STORMCLOUD:
                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 40.0f, true))
                        DoCast(target, SPELL_STORMCLOUD);
                    events.ScheduleEvent(EVENT_STORMCLOUD,  24000);
                    break;
                }
            }
            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new boss_nalakAI(creature);
    }
};

void AddSC_boss_nalak()
{
    new boss_nalak();
}
