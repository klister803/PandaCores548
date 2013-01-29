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
    SPELL_DOMINATE_MIND_WARNING = 119622,
    SPELL_DOMINATE_MIND         = 119626,
    SPELL_CLOUD                 = 119446,
    SPELL_SEETHE_AURA           = 119487,
};

enum eEvents
{
    EVENT_GROWING_ANGER_WARNING = 1,
    EVENT_GROWING_ANGER         = 2,
    EVENT_UNLEASHED_WRATH       = 3,
    EVENT_BERSERK               = 4,
    EVENT_DESPAWN               = 5,
    EVENT_SPAWN                 = 6,
    EVENT_UPDATE_RAGE           = 7,
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
        {}

        int _targetCount;
        int _cloudCount;
        int _maxTargetCount;
        uint8 _dominateMindCount;
        uint32 timer;
        bool phase1;

        std::list<uint64> targetedDominationPlayerGuids;

        void Reset()
        {
            me->setPowerType(POWER_RAGE);

            phase1 = true;
            _dominateMindCount = 3;
            _cloudCount = 10;
            _targetCount = 0;
            _maxTargetCount = 12;
            timer = 0;
            Talk(3);
            events.Reset();
            _Reset();

            targetedDominationPlayerGuids.clear();
        }

        void KilledUnit(Unit* u)
        {
            Talk(4);
        }

        void EnterCombat(Unit* unit)
        {
            Talk(5);
            events.ScheduleEvent(EVENT_GROWING_ANGER_WARNING, 19000);
            events.ScheduleEvent(EVENT_SPAWN,5000);
            events.ScheduleEvent(EVENT_UNLEASHED_WRATH,52000);
            events.ScheduleEvent(EVENT_BERSERK,900000);
            events.ScheduleEvent(EVENT_UPDATE_RAGE,1000);
        }

        void JustSummoned(Creature* summon)
        {
            // Clouds
            if (summon->GetEntry() == 61523)
            {
                summon->CastSpell(summon, SPELL_BITTER_THOUGHTS, true);
                summon->DespawnOrUnsummon(60000);
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
                    case EVENT_UNLEASHED_WRATH:
                    {
                        phase1 = false;
                        for (uint8 i=0; i<10;i++)
                        {
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM))
                            {
                                if (target->GetAuraCount(SPELL_SEETHE_AURA) < 6)
                                {
                                    target->CastSpell(target,SPELL_SEETHE,false);
                                    target->AddAura(SPELL_SEETHE_AURA,target);
                                }
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

                        break;
                    }
                    case EVENT_GROWING_ANGER_WARNING:
                    {
                        Talk(1);
                        for (uint8 i = 0; i < _dominateMindCount; ++i)
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM))
                            {
                                targetedDominationPlayerGuids.push_back(target->GetGUID());
                                me->CastSpell(target, SPELL_DOMINATE_MIND_WARNING, true);
                            }

                        events.ScheduleEvent(EVENT_GROWING_ANGER, 6000);
                        break;
                    }
                    case EVENT_GROWING_ANGER:
                    {
                        for (auto guid : targetedDominationPlayerGuids)
                            if (Player* target = ObjectAccessor::GetPlayer(*me, guid))
                                me->CastSpell(target, SPELL_DOMINATE_MIND, false);

                        events.ScheduleEvent(EVENT_GROWING_ANGER_WARNING, 19000);
                        break;
                    }
                    case EVENT_SPAWN:
                    {
                        Talk(2);
                        for (uint8 i=0; i<_cloudCount;++i)
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM))
                                me->CastSpell(target, SPELL_CLOUD, false);

                        events.ScheduleEvent(EVENT_SPAWN, 15000);
                        break;
                    }
                    case EVENT_UPDATE_RAGE:
                    {
                        if(phase1)
                            timer = timer + 20;
                        else
                            timer = timer - 20;

                        me->SetPower(POWER_RAGE,timer);
                        events.ScheduleEvent(EVENT_UPDATE_RAGE,1000);
                        break;
                    }
                    case EVENT_BERSERK:
                    {
                        me->CastSpell(me,SPELL_BERSERK,false);
                        break;
                    }
                }
            }
            DoMeleeAttackIfReady();
        }
    };
};

class spell_sha_of_anger_aggressive_behaviour : public SpellScriptLoader
{
    public:
        spell_sha_of_anger_aggressive_behaviour() : SpellScriptLoader("spell_sha_of_anger_aggressive_behaviour") { }

        class spell_sha_of_anger_aggressive_behaviour_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_sha_of_anger_aggressive_behaviour_AuraScript);
            uint32 factionSave;
            bool pvpFlag;

            void HandlePeriodicTick(constAuraEffectPtr /*aurEff*/)
            {
                PreventDefaultAction();
                if (Unit* target = GetTarget())
                    if (target->GetHealthPct() < 50.0f)
                        this->Remove(AURA_REMOVE_BY_DEFAULT);
            }

            void OnApply(constAuraEffectPtr /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                pvpFlag = false;
                if(Unit* target = GetTarget())
                {
                    if (!target->ToPlayer())
                        return;
                    
                    target->SetPvP(true);
                    target->setFaction(16);
                    target->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PLAYER_CONTROLLED);
                }

            }

            void OnRemove(constAuraEffectPtr /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if(Unit* target = GetTarget())
                {
                    target->setFaction(target->RestoreFaction);
                    target->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PLAYER_CONTROLLED);
                }
            }

            void Register()
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_sha_of_anger_aggressive_behaviour_AuraScript::HandlePeriodicTick, EFFECT_5, SPELL_AURA_PERIODIC_DUMMY);
                OnEffectApply += AuraEffectApplyFn(spell_sha_of_anger_aggressive_behaviour_AuraScript::OnApply, EFFECT_5, SPELL_AURA_PERIODIC_DUMMY, AURA_EFFECT_HANDLE_REAL);
                OnEffectRemove += AuraEffectRemoveFn(spell_sha_of_anger_aggressive_behaviour_AuraScript::OnRemove, EFFECT_5, SPELL_AURA_PERIODIC_DUMMY, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_sha_of_anger_aggressive_behaviour_AuraScript();
        }
};

void AddSC_boss_sha_of_anger()
{
    new boss_sha_of_anger();
    new spell_sha_of_anger_aggressive_behaviour();
}
