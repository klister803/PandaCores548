/*
    Dungeon : Mogu'shan palace 87-89
    Xin the weaponmaster
    Jade servers
*/

#include "ScriptMgr.h"
#include "ScriptedCreature.h"


class spell_dart : public SpellScriptLoader
{
    public:
        spell_dart() : SpellScriptLoader("spell_dart") { }

        class spell_dart_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_dart_SpellScript);

            bool Validate(SpellInfo const* spell)
            {
                return true;
            }

            // set up initial variables and check if caster is creature
            // this will let use safely use ToCreature() casts in entire script
            bool Load()
            {
                return true;
            }

            bool intersectionCircleSegment(float x0, float y0, float x1, float y1, float cx, float cy, float r)
            {
                float a = (x1 - x0) * (x1 - x0) + (y1 - y0) * (y1 - y0);
                float b = 2 * ((x1 - x0) * (x0 - cx) + (y1 - y0) * (y0 - cy));
                float c = cx * cx + cy * cy + x0 * x0 + y0 * y0 - 2 * (cx * x0 + cy * y0) - r * r;
                float det = b * b - 4 * a * c;
                
                if (det < 0)
                    return false;
                else
                {
                    // The circle is in the segment ?
                    float sdet = sqrt(det);
                    float r1 = (-b + sdet) / (2 * a);
                    float r2 = (-b - sdet) / (2 * a);
                    
                    if ((r1 < 0 || r1 > 1) && (r2 < 0 || r2 > 1))
                        if ((r1 < 0 && r2 < 0) || (r1 > 1 && r2 > 1))
                            return false;
                        else
                            return true;
                    else
                        return true;
                }
            }

            void SelectTarget(std::list<WorldObject*>& targetList)
            {
                if (targetList.empty())
                {
                    FinishCast(SPELL_FAILED_NO_VALID_TARGETS);
                    return;
                }
                //Select the two targets.
                std::list<WorldObject*> targets = targetList;
                for (auto object : targets)
                {
                    if (object->ToCreature() && object->GetGUID() != GetCaster()->GetGUID())
                    {
                        if (object->ToCreature()->GetEntry() != 61679 || !GetCaster()->isInFront(object, M_PI / 5))
                            targetList.remove(object);
                    }
                    else
                        targetList.remove(object);
                }
                std::list<WorldObject*> lines = targetList;
                //See if we intersect with any players.
                for (auto object : targets)
                {
                    if (object->ToPlayer())
                    {
                        for (auto line : lines)
                        {
                            if (intersectionCircleSegment(GetCaster()->GetPositionX(), GetCaster()->GetPositionY(),
                                line->GetPositionX(), line->GetPositionY(), object->GetPositionX(), object->GetPositionY(), 2.5f))
                                GetCaster()->DealDamage(object->ToPlayer(), GetSpellInfo()->Effects[0].BasePoints, 0, SPELL_DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, GetSpellInfo());
                        }
                    }
                }
            }

            void Register()
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_dart_SpellScript::SelectTarget, EFFECT_0, TARGET_SRC_CASTER);
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_dart_SpellScript::SelectTarget, EFFECT_0, TARGET_UNIT_SRC_AREA_ENTRY);
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_dart_SpellScript::SelectTarget, EFFECT_0, TARGET_UNIT_SRC_AREA_ENEMY);
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_dart_SpellScript::SelectTarget, EFFECT_0, TARGET_UNIT_CONE_ENEMY_104);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_dart_SpellScript();
        }
};

class mob_animated_staff : public CreatureScript
{
    public:
        mob_animated_staff() : CreatureScript("mob_animated_staff") { }

        CreatureAI* GetAI(Creature* creature) const
        {
            return new mob_animated_staff_AI(creature);
        }

        enum eSpells
        {
            SPELL_PERMANENT_FEIGN_DEATH = 29266,
            SPELL_RING_OF_FIRE_0 = 119544,
            SPELL_RING_OF_FIRE_1 = 119590,
        };

        enum eActions
        {
            ACTION_ACTIVATE,
        };

        enum eEvents
        {
            EVENT_SUMMON_RING_OF_FIRE = 1,
            EVENT_UNSUMMON = 2,
            EVENT_SUMMON_RING_TRIGGER = 3,
        };

        enum eCreatures
        {
            CREATURE_RING_OF_FIRE = 61499,
        };

        struct mob_animated_staff_AI : public ScriptedAI
        {
            mob_animated_staff_AI(Creature* creature) : ScriptedAI(creature)
            {
                me->SetDisplayId(42195);
                me->SetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_ID, 76364);
                me->AddAura(SPELL_PERMANENT_FEIGN_DEATH, me);
            
                _x = 0.f;
                _y = 0.f;
                point = 0.f;
            }
            EventMap events;
            float _x;
            float _y;
            float point;

            void Reset()
            {
                me->GetMotionMaster()->MoveTargetedHome();
                me->AddAura(SPELL_PERMANENT_FEIGN_DEATH, me);
            }

            void EnterCombat(Unit* unit)
            {
            }

            void DoAction(const int32 action)
            {
                switch (action)
                {
                case ACTION_ACTIVATE:
                    me->RemoveAura(SPELL_PERMANENT_FEIGN_DEATH);
                    events.ScheduleEvent(EVENT_SUMMON_RING_OF_FIRE, 500);
                    break;
                }
            }

            void UpdateAI(const uint32 diff)
            {
                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                    case EVENT_SUMMON_RING_OF_FIRE:
                        {
                            events.ScheduleEvent(EVENT_UNSUMMON, 6000);
                            Unit* target = nullptr;
                            std::list<Unit*> units;
                            Map::PlayerList const& PlayerList = me->GetMap()->GetPlayers();

                            if (!PlayerList.isEmpty())
                            {
                                for (Map::PlayerList::const_iterator i = PlayerList.begin(); i != PlayerList.end(); ++i)
                                {
                                    Player* plr = i->getSource();
                                    if( !plr)
                                        continue;
                                    if (plr->isAlive() && !plr->isGameMaster())
                                        units.push_back(plr);
                                }
                            }
                            if (units.empty())
                                return;
                            std::list<Unit*>::iterator itr = units.begin();
                            std::advance(itr, units.size() - 1);
                            target = *itr;
                            if (!target)
                                return;
                            me->GetMotionMaster()->MoveFollow(target, 6.0f, frand(0, 2 * M_PI));
                            _x = target->GetPositionX();
                            _y = target->GetPositionY();
                            point = 1.f;

                            float x = _x + 5.0f * cos(0);
                            float y = _y + 5.0f * sin(0);
                            me->SummonCreature(CREATURE_RING_OF_FIRE, x, y, me->GetMap()->GetHeight(0, x, y, me->GetPositionZ()));
                            events.ScheduleEvent(EVENT_SUMMON_RING_TRIGGER, 400);
                        }
                        break;
                    case EVENT_UNSUMMON:
                        Reset();
                        break;
                    case EVENT_SUMMON_RING_TRIGGER:
                        {
                            if (point == 21)
                            {
                                TempSummon* tmp = me->SummonCreature(CREATURE_RING_OF_FIRE, _x, _y, me->GetMap()->GetHeight(0, _x, _y, me->GetPositionZ()));
                                if (tmp)
                                {
                                    tmp->RemoveAura(SPELL_RING_OF_FIRE_0);
                                    tmp->CastSpell(tmp, SPELL_RING_OF_FIRE_1, false);
                                }
                                return;
                            }
                            float x = _x + 5.0f * cos(point * 2 * M_PI / 20);
                            float y = _y + 5.0f * sin(point * 2 * M_PI / 20);
                            me->SummonCreature(CREATURE_RING_OF_FIRE, x, y, me->GetMap()->GetHeight(0, x, y, me->GetPositionZ()));
                            point++;
                            if (point <= 21)
                                events.ScheduleEvent(EVENT_SUMMON_RING_TRIGGER, 400);
                        }
                        break;
                    }
                }
            }
        };
};

class mob_ring_of_fire : public CreatureScript
{
    public:
        mob_ring_of_fire() : CreatureScript("mob_ring_of_fire") { }

        CreatureAI* GetAI(Creature* creature) const
        {
            return new mob_ring_of_fire_AI(creature);
        }

        enum eSpells
        {
            SPELL_RING_OF_FIRE_0 = 119544,
            SPELL_RING_OF_FIRE_1 = 119590,
        };

        struct mob_ring_of_fire_AI : public ScriptedAI
        {
            mob_ring_of_fire_AI(Creature* creature) : ScriptedAI(creature)
            {
                me->setFaction(14);
                me->SetReactState(REACT_PASSIVE);
                me->AddAura(SPELL_RING_OF_FIRE_0, me);
                me->DespawnOrUnsummon(10000);
            }
        };
};

class boss_xin_the_weaponmaster : public CreatureScript
{
    public:
        boss_xin_the_weaponmaster() : CreatureScript("boss_xin_the_weaponmaster") { }

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_xin_the_weaponmaster_AI(creature);
        }

        enum eBosses
        {
            BOSS_XIN_THE_WEAPONMASTER,
        };

        enum eEvents
        {
            EVENT_RING_OF_FIRE = 1,
        };

        struct boss_xin_the_weaponmaster_AI : public BossAI
        {
            boss_xin_the_weaponmaster_AI(Creature* creature) : BossAI(creature, BOSS_XIN_THE_WEAPONMASTER)
            {
            }

            void EnterCombat(Unit* who)
            {
                events.ScheduleEvent(EVENT_RING_OF_FIRE, 3000);
            }

            void UpdateAI(const uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                events.Update(diff);

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                    case EVENT_RING_OF_FIRE:
                        if (me->GetInstanceScript())
                            me->GetInstanceScript()->SetData(18, 0); //TYPE_ACTIVATE_ANIMATED_STAFF
                        events.ScheduleEvent(EVENT_RING_OF_FIRE, 10000);
                        break;
                    }
                }
                DoMeleeAttackIfReady();
            }
        };
};

void AddSC_boss_xin_the_weaponmaster()
{
    new spell_dart();
    new mob_animated_staff();
    new mob_ring_of_fire();
    new boss_xin_the_weaponmaster();
}