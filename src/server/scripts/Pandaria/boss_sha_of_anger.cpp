/*
Pandaria
World boss
Antoine Vallee for Pandashan Servers

*/
#include "ScriptMgr.h"
#include "ScriptedCreature.h"

enum eBosses
{
    BOSS_SHA_OF_ANGER,
};

enum eSpells
{
    SPELL_SEETHE                = 105546,
    SPELL_ENDLESS_RAGE          = 119446,
    SPELL_BITTER_THOUGHTS       = 119601,
    SPELL_BERSERK               = 47008,
    SPELL_DOMINATE_MIND         = 119626,
    SPELL_CLOUD                 = 119446,
    SPELL_SEETHE_AURA           = 119487,
};

enum eEvents
{
    EVENT_GROWING_ANGER=1,
    EVENT_UNLEASHED_WRATH=2,
    EVENT_BERSERK=3,
    EVENT_DESPAWN=4,
    EVENT_SPAWN=5,
    EVENT_UPDATE_RAGE=6,
};

enum eCreatures
{
    CREATURE_SHA_OF_ANGER           = 56439,
};


class boss_sha_of_anger : public CreatureScript
{
public:
    boss_sha_of_anger() : CreatureScript("boss_sha_of_anger") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new boss_sha_of_anger_AI(creature);
    }

    struct boss_sha_of_anger_AI : public BossAI
    {
        boss_sha_of_anger_AI(Creature* creature) : BossAI(creature, BOSS_SHA_OF_ANGER)
        {
        }
        int _targetCount;
        int _cloudCount;
        int _maxTargetCount;
        uint8 _dominateMindCount;
        uint32 timer;
        bool phase1;

        void Reset()
        {
            phase1 = true;
            _dominateMindCount = 5;
            _cloudCount = 10;
            _targetCount = 0;
            _maxTargetCount = 12;
            Talk(3);
            events.Reset();
            _Reset();
        }

        void KilledUnit(Unit* u)
        {
            Talk(4);
        }

        void JustDied(Unit* u)
        {
        }

        void EnterCombat(Unit* unit)
        {
            Talk(5);
            events.ScheduleEvent(EVENT_GROWING_ANGER,1500);
            events.ScheduleEvent(EVENT_SPAWN,5000);
            events.ScheduleEvent(EVENT_UNLEASHED_WRATH,52000);
            events.ScheduleEvent(EVENT_BERSERK,900000);
            events.ScheduleEvent(EVENT_UPDATE_RAGE,1000);
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
                case EVENT_UNLEASHED_WRATH:
                    {
                        phase1 = false;
                        for (uint8 i=0; i<10;i++)
                        {
                            Unit* target = SelectTarget(SELECT_TARGET_RANDOM);
                            if (target->GetAuraCount(SPELL_SEETHE_AURA) < 6)
                            {
                                target->CastSpell(target,SPELL_SEETHE,false);
                                target->AddAura(SPELL_SEETHE_AURA,target);
                            } 
                        }
                        if (_targetCount < _maxTargetCount)
                        {
                            if (_targetCount == 0)
                            {
                                Talk(0);
                            }

                            _targetCount++;
                            events.ScheduleEvent(EVENT_UNLEASHED_WRATH, 2000);
                        }
                        else
                        {
                            events.ScheduleEvent(EVENT_UNLEASHED_WRATH, 50000);
                            phase1 = true;
                            _targetCount = 0;
                        }
                    }
                    break;
                case EVENT_GROWING_ANGER:
                    {
                        Talk(1);     
                        for (uint8 i = 0; i < _dominateMindCount; i++)
                        {
                           // me->CastSpell(SelectTarget(SELECT_TARGET_RANDOM), SPELL_DOMINATE_MIND, false);
                        }
                        events.ScheduleEvent(EVENT_GROWING_ANGER, 25000);
                    }
                    break;
                case EVENT_SPAWN:
                    {
                        Talk(2);    
                        for (uint8 i=0; i<_cloudCount;i++)
                        {
                            Unit* target = SelectTarget(SELECT_TARGET_RANDOM);
                            me->CastSpell(target, SPELL_CLOUD, false);
                        }
                        events.ScheduleEvent(EVENT_SPAWN, 15000);
                    }
                    break;
                case EVENT_BERSERK:
                    {
                        me->CastSpell(me,SPELL_BERSERK,false);
                    }
                    break;   
                case EVENT_UPDATE_RAGE:
                    {
                        if(phase1)
                            timer = timer + 2;
                        else
                            timer = timer - 2;

                        me->SetPower(POWER_RAGE,timer);
                        events.ScheduleEvent(EVENT_UPDATE_RAGE,1000);
                    }
                    break;
                }
            }
            DoMeleeAttackIfReady();
        }
    };
};

class mob_cloud : public CreatureScript
{
public:
    mob_cloud() : CreatureScript("mob_cloud") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new mob_cloud_AI(creature);
    }

    struct mob_cloud_AI : public ScriptedAI
    {
        mob_cloud_AI(Creature* creature) : ScriptedAI(creature)
        {
        }
        EventMap events;

        void JustDied(Unit* killer)
        {
        }

        void Reset()
        {
            me->CastSpell(me,SPELL_BITTER_THOUGHTS,false);
            events.ScheduleEvent(EVENT_DESPAWN, 60000);
        }

        void EnterCombat(Unit* u)
        {
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
                case EVENT_DESPAWN:
                    {
                        me->DespawnOrUnsummon();
                    }
                    break;
                }
            }
            DoMeleeAttackIfReady();
        }
    };
};

void AddSC_boss_sha_of_anger()
{
    new boss_sha_of_anger();
    new mob_cloud();
}