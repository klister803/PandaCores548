/*
    Dungeon : Template of the Jade Serpent 85-87
    Wise mari first boss
    Jade servers
*/

#include "ScriptMgr.h"
#include "ScriptedCreature.h"

enum eBoss
{
    BOSS_WASE_MARI = 1,
};

enum eSpells
{
    SPELL_WATER_BUBBLE      = 106062,
    SPELL_CALL_WATER        = 106526,
    SPELL_CORRUPTED_FOUTAIN = 106518,
};

enum eTexts
{
    TEXT_INTRO            = 0,
    TEXT_AGGRO            = 1,
    TEXT_BOSS_EMOTE_AGGRO = 2,
};

enum eEvents
{
    EVENT_CALL_WATER = 1,
};

enum eCreatures
{
    CREATURE_FOUTAIN_TRIGGER    = 56586,
};

enum eTimers
{
    TIMER_CALL_WATTER           = 29000,
};

static const float fountainTriggerPos[4][3] = 
{
    {1022.743f, -2544.295f, 173.7757f},
    {1023.314f, -2569.695f, 176.0339f},
    {1059.943f, -2581.648f, 176.1427f},
    {1075.231f, -2561.335f, 173.8758f},
};

class boss_wase_mari : public CreatureScript
{
    public:
        boss_wase_mari() : CreatureScript("boss_wase_mari") { }

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_wise_mari_AI(creature);
        }

        struct boss_wise_mari_AI : public BossAI
        {
            boss_wise_mari_AI(Creature* creature) : BossAI(creature, BOSS_WASE_MARI)
            {
                creature->AddUnitState(UNIT_STATE_ROOT);
            }

            bool ennemyInArea;
            bool intro;
            uint8 phase;
            uint8 foutainCount;
            uint64 foutainTrigger[4];

            void Reset()
            {

                for (uint8 i = 0; i < 4; i++)
                    foutainTrigger[i] = 0;

                foutainCount = 0;
                phase = 0;
                ennemyInArea= false;
                intro = false;
                me->RemoveAurasDueToSpell(SPELL_WATER_BUBBLE);

                _Reset();
            }

            void EnterCombat(Unit* /*who*/)
            {
                std::list<Creature*> searcher;
                GetCreatureListWithEntryInGrid(searcher, me, CREATURE_FOUTAIN_TRIGGER, 50.0f);
                uint8 tab = 0;
                for (auto itr : searcher)
                {
                    if (!itr)
                        continue;

                    itr->RemoveAllAuras();

                    foutainTrigger[++tab] = itr->GetGUID();
                }

                me->SetInCombatWithZone();
                me->CastSpell(me, SPELL_WATER_BUBBLE, true);
                Talk(TEXT_AGGRO);
                Talk(TEXT_BOSS_EMOTE_AGGRO);
                intro = true;
                phase = 1;
                events.ScheduleEvent(EVENT_CALL_WATER, 8000);

                _EnterCombat();
            }

            void DoAction(const int32 action)
            {
            }

            void KilledUnit(Unit* /*victim*/)
            {
            }

            void JustDied(Unit* /*killer*/)
            {
                _JustDied();
            }

            void DamageTaken(Unit* /*attacker*/, uint32& /*damage*/)
            {

            }

            void MoveInLineOfSight(Unit* who)
            {
                if(who->GetTypeId() != TYPEID_PLAYER)
                    return;

                if(intro)
                    return;

                if(!ennemyInArea)
                {
                    Talk(TEXT_INTRO);
                    ennemyInArea = true;
                    return;
                }
                
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
                        case EVENT_CALL_WATER:
                            if(phase == 1)
                            {
                                Creature* trigger = me->GetCreature(*me, foutainTrigger[++foutainCount]);
                                if (trigger)
                                {
                                    me->CastSpell(trigger, SPELL_CALL_WATER, true);
                                    trigger->AddAura(SPELL_CORRUPTED_FOUTAIN, trigger);
                                }

                                if (foutainCount == 4)
                                {
                                    phase = 2;
                                }
                            }
                            events.ScheduleEvent(EVENT_CALL_WATER, TIMER_CALL_WATTER + rand() % 6000);
                            break;
                    }
                }

            }
        };
};

void AddSC_boss_wise_mari()
{
    new boss_wase_mari();
}
