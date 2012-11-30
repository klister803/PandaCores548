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
        };

        enum eActions
        {
            ACTION_ACTIVATE,
        };

        enum eEvents
        {
            EVENT_SUMMON_RING_OF_FIRE = 1,
            EVENT_UNSUMMON = 2,
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
            }
            EventMap events;

            void Reset()
            {
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
                            Unit* target = SelectTarget(SELECT_TARGET_RANDOM);
                            if (!target)
                                return;
                            float x = target->GetPositionX() + 5.f * cos(0);
                            float y = target->GetPositionY() + 5.f * sin(0);
                            me->SummonCreature(CREATURE_RING_OF_FIRE, x, y, me->GetMap()->GetHeight(0, x, y, target->GetPositionZ()));
                        }
                        break;
                    case EVENT_UNSUMMON:
                        me->DespawnOrUnsummon();
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
        };

        enum eEvents
        {
            EVENT_SUMMON_RING_OF_FIRE = 1,
            EVENT_UNSUMMON = 2,
        };

        struct mob_ring_of_fire_AI : public ScriptedAI
        {
            mob_ring_of_fire_AI(Creature* creature) : ScriptedAI(creature)
            {
                me->AddAura(SPELL_RING_OF_FIRE_0, me);
                _x = me->GetPositionX() - 5.0f * cos(0);
                _y = me->GetPositionY() - 5.0f * sin(0);
                _point = 0;

                me->GetMotionMaster()->MovePoint(++_point, _x + 5.0f * cos(_point * 2 * M_PI / 20), _y + 5.0f * sin(_point * 2 * M_PI / 20),
                    me->GetMap()->GetHeight(0, _x + 5.0f * cos(_point * 2 * M_PI / 20), _y + 5.0f * sin(_point * 2 * M_PI / 20), me->GetPositionZ()));
            }
            float _x;
            float _y;
            uint32 _point;

            void MovementInform(uint32 motionType, uint32 pointId)
            {
                if (pointId == 20)
                {
                    me->GetMotionMaster()->MovePoint(21, _x, _y, me->GetMap()->GetHeight(0, _x, _y, me->GetPositionZ()));
                    return;
                }
                me->GetMotionMaster()->MovePoint(++_point, _x + 5.0f * cos(_point * 2 * M_PI / 20), _y + 5.0f * sin(_point * 2 * M_PI / 20),
                    me->GetMap()->GetHeight(0, _x + 5.0f * cos(_point * 2 * M_PI / 20), _y + 5.0f * sin(_point * 2 * M_PI / 20), me->GetPositionZ()));
            }
        };
};

void AddSC_boss_xin_the_weaponmaster()
{
    new spell_dart();
    new mob_animated_staff();
}