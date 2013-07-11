
#include <vector>

#include "ScriptPCH.h"
#include "dragon_soul.h"

enum eMorchok
{
    SPELL_BERSERK            = 26662,
    SPELL_FURIOUS            = 103846,
    SPELL_BLACK_BLOOD        = 103851,
    SPELL_BLACK_BLOOD_VISUAL = 103180,
    SPELL_CRUSH_ARMOR        = 103687,
    SPELL_FALLING_FRAGMENTS  = 103176,
    SPELL_RESONATING_CRYSTAL = 103640,
    SPELL_SUMMON_KOHCROM     = 109017,
    SPELL_SIEGE_MISSILE      = 107541,

    SPELL_RES_CRYSTAL_10     = 103494,
    SPELL_RES_CRYSTAL_25     = 108572,

    SPELL_STOMP              = 103414,
    SPELL_EARTHEN_VORTEX     = 103821,
    SPELL_FALLING_FRAGME_SUM = 103178,

    EVENT_BLACK_BLOOD        = 1,
    EVENT_CRUSH_ARMOR        = 2,
    EVENT_STOMP              = 3,
    EVENT_RESONATING_CRYSTAL = 4,
    EVENT_BERSERK            = 5,

    YELL_TOWER_ATTACK_1      = -2000300,
    YELL_TOWER_ATTACK_2      = -2000301,
    YELL_TOWER_ATTACK_3      = -2000302,
    YELL_TOWER_ATTACK_4      = -2000303,
    YELL_AGGRO               = -2000304,
    YELL_KILL_1              = -2000305,
    EMOTE_RESONATING_CRYSTAL = -2000306,
    YELL_RESONATING_CRYSTAL  = -2000307,
    YELL_FALLING_FRAGMENTS_1 = -2000308,
    YELL_FALLING_FRAGMENTS_2 = -2000309,
    YELL_BLACK_BLOOD_1       = -2000310,
    YELL_DIED                = -2000311,
    YELL_SUMMON_KOHCROM      = -2000312,

    INNER_WALL               = 209596,
};

class boss_morchok : public CreatureScript
{
public:
    boss_morchok() : CreatureScript("boss_morchok") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new boss_morchokAI (creature);
    }

    struct boss_morchokAI : public BossAI
    {
        boss_morchokAI(Creature* c) : BossAI(c, DATA_MORCHOK)
        {
        }

        EventMap events;
        EventMap event;

        bool FuriousActivate;
        bool KohcromActivate;
        bool BlackBloodActive;
        
        uint8 BlackBloodPhase;
        uint8 BlackBloodPhase2;
        uint32 BlackBloodTimer;
        uint32 BlackBloodTimer2;
        uint32 SiegeMissileTimer;

        void Reset() 
        {
            _Reset();
            events.Reset();
            event.Reset();
            summons.DespawnAll();

            FuriousActivate = false;
            KohcromActivate = false;
            BlackBloodActive = false;

            BlackBloodPhase = 1;
            BlackBloodPhase2 = 1;
            BlackBloodTimer = 0;
            BlackBloodTimer2 = 0;
            SiegeMissileTimer = 3000;

            events.ScheduleEvent(EVENT_BLACK_BLOOD, 54*IN_MILLISECONDS);
            if (!IsHeroic())
                events.ScheduleEvent(EVENT_CRUSH_ARMOR, urand(6*IN_MILLISECONDS, 8*IN_MILLISECONDS));
            events.ScheduleEvent(EVENT_STOMP, 12*IN_MILLISECONDS);
            events.ScheduleEvent(EVENT_RESONATING_CRYSTAL, 19*IN_MILLISECONDS);
            events.ScheduleEvent(EVENT_BERSERK, 10*MINUTE*IN_MILLISECONDS);

            me->RemoveGameObject(SPELL_FALLING_FRAGME_SUM, true);
            me->setActive(true);
            me->SetReactState(REACT_AGGRESSIVE);
        }

        void EnterCombat(Unit* /*who*/) 
        {
            _EnterCombat();
            DoScriptText(YELL_AGGRO, me);

            DoZoneInCombat();
        }

        void JustDied(Unit* /*victim*/) 
        {
             _JustDied();
            summons.DespawnAll();

            DoScriptText(YELL_DIED, me);

            if (Creature* npc = ObjectAccessor::GetCreature(*me, instance->GetData64(NPC_VALEERA)))
                npc->SetUInt32Value(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);

            if (Creature* npc = ObjectAccessor::GetCreature(*me, instance->GetData64(NPC_EIENDORMI)))
                npc->SetUInt32Value(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
        }

        void JustSummoned(Creature* summon) 
        {
            summons.Summon(summon);
            if(summon && summon->GetEntry() == NPC_MORCHOK_COPY)
                summon->GetMotionMaster()->MoveJump(-2038.10f, -2411.98f, 72.5f, 40.0f, 20.0f);
        }

        void KilledUnit(Unit* /*victim*/) 
        {
            DoScriptText(YELL_KILL_1, me);
        }

        void UpdateAI(const uint32 diff) 
        {
            if (!me->isInCombat())
            {
                if (SiegeMissileTimer <= diff)
                {
                    me->CastSpell(-1867.922f, -2436.486f, 92.39846f, SPELL_SIEGE_MISSILE, true);
                    SiegeMissileTimer = 3000;
                } else SiegeMissileTimer -= diff;
            }
            
            if (!UpdateVictim())
                return;

            if (!BlackBloodActive)
            {
                events.Update(diff);

                switch (events.ExecuteEvent())
                {
                    case EVENT_BLACK_BLOOD:
                    if (!IsHeroic())
                        events.RescheduleEvent(EVENT_CRUSH_ARMOR, 3*IN_MILLISECONDS);
                        BlackBloodActive = true;
                        BlackBloodPhase = 1;
                        events.ScheduleEvent(EVENT_BLACK_BLOOD, 74*IN_MILLISECONDS);
                        break;
                    case EVENT_CRUSH_ARMOR:
                        DoCastVictim(SPELL_CRUSH_ARMOR);
                        events.ScheduleEvent(EVENT_CRUSH_ARMOR, urand(6*IN_MILLISECONDS, 8*IN_MILLISECONDS));
                        break;
                    case EVENT_STOMP:
                    if (!IsHeroic())
                        events.RescheduleEvent(EVENT_CRUSH_ARMOR, 3*IN_MILLISECONDS);
                        DoCast(me, SPELL_STOMP, false);
                        events.ScheduleEvent(EVENT_STOMP, 12*IN_MILLISECONDS);
                        break;
                    case EVENT_RESONATING_CRYSTAL:
                    if (!IsHeroic())
                        events.RescheduleEvent(EVENT_CRUSH_ARMOR, 3*IN_MILLISECONDS);
                        if (Unit* target = SelectTarget(SELECT_TARGET_NEAREST, 0, NonTankTargetSelector(me)))
                        {
                            float _angle;
                            Position _pos;
                            _angle = me->GetAngle(target->GetPositionX(), target->GetPositionY());
                            me->GetNearPosition(_pos, 25.0f, _angle);
                            me->CastSpell(_pos.GetPositionX(), _pos.GetPositionY(), _pos.GetPositionZ(), SPELL_RESONATING_CRYSTAL, true);
                        }
                        DoScriptText(EMOTE_RESONATING_CRYSTAL, me);
                        DoScriptText(YELL_RESONATING_CRYSTAL, me);
                        events.ScheduleEvent(EVENT_RESONATING_CRYSTAL, 12*IN_MILLISECONDS);
                        break;
                    case EVENT_BERSERK:
                        DoCast(me, SPELL_BERSERK);
                        events.ScheduleEvent(EVENT_BERSERK, 100*IN_MILLISECONDS);
                        break;
                }

                if (!FuriousActivate && me->HealthBelowPct(21))
                {
                    DoCast(me, SPELL_FURIOUS);
                    FuriousActivate = true;
                }

                if (!KohcromActivate && me->GetMap()->IsHeroic() && me->HealthBelowPct(91))
                {
                    events.RescheduleEvent(EVENT_STOMP, 3*IN_MILLISECONDS);
                    DoScriptText(YELL_SUMMON_KOHCROM, me);
                    DoCast(me, SPELL_SUMMON_KOHCROM, true);
                    KohcromActivate = true;
                }

                DoMeleeAttackIfReady();
            }
            else
            {
                if (BlackBloodTimer <= diff)
                {
                    switch (BlackBloodPhase)
                    {
                        case 1:
                            DoCastVictim(SPELL_EARTHEN_VORTEX, true);
                            BlackBloodTimer = 5000;
                            break;
                        case 2:
                            me->SetReactState(REACT_PASSIVE);
                            me->AttackStop();
                            DoScriptText(RAND(YELL_FALLING_FRAGMENTS_1, YELL_FALLING_FRAGMENTS_2), me);
                            DoCast(me, SPELL_FALLING_FRAGMENTS, true);
                            BlackBloodTimer = 5000;
                            BlackBloodTimer2 = 5500;
                            break;
                        case 3:
                            DoScriptText(YELL_BLACK_BLOOD_1, me);
                            DoCast(me, SPELL_BLACK_BLOOD, true);
                            SummonBlackBlood(BlackBloodPhase2);
                            BlackBloodTimer2 = 5500;
                            BlackBloodTimer = 17000;
                            BlackBloodPhase2++;
                            break;
                        case 4:
                            me->RemoveGameObject(SPELL_FALLING_FRAGME_SUM, true);
                            BlackBloodTimer = 0;
                            BlackBloodTimer2 = 0;
                            BlackBloodPhase2 = 1;
                            me->SetReactState(REACT_AGGRESSIVE);
                            BlackBloodActive = false;
                            break;
                    }

                    BlackBloodPhase++;
                } else BlackBloodTimer -= diff;

                if (BlackBloodTimer2 <= diff)
                {
                    SummonBlackBlood(BlackBloodPhase2);
                    BlackBloodPhase2++;
                    BlackBloodTimer2 = 5500;
                } else BlackBloodTimer2 -= diff;
            }
        }

        void SummonBlackBlood(int counter)
        {
            float _angle;
            float radius = 10.0f;
            Position _pos;
            std::list<GameObject*> fallingfragments;
            GetGameObjectListWithEntryInGrid(fallingfragments, me, INNER_WALL, 60.0f);
            for (int i = 0; i < counter; i++)
            {
                //blackblood 1
                _angle = me->GetOrientation() + M_PI/4;
                me->GetNearPosition(_pos, radius, _angle);
                if(BlackBloodPositionSelect(fallingfragments, _pos.GetPositionX(), _pos.GetPositionY()))
                    me->CastSpell(_pos.GetPositionX(), _pos.GetPositionY(), _pos.GetPositionZ(), SPELL_BLACK_BLOOD_VISUAL, true);
                //blackblood 2
                _angle = me->GetOrientation() + M_PI/2;
                me->GetNearPosition(_pos, radius, _angle);
                if(BlackBloodPositionSelect(fallingfragments, _pos.GetPositionX(), _pos.GetPositionY()))
                    me->CastSpell(_pos.GetPositionX(), _pos.GetPositionY(), _pos.GetPositionZ(), SPELL_BLACK_BLOOD_VISUAL, true);
                //blackblood 3
                _angle = me->GetOrientation() + (3*M_PI)/4;
                me->GetNearPosition(_pos, radius, _angle);
                if(BlackBloodPositionSelect(fallingfragments, _pos.GetPositionX(), _pos.GetPositionY()))
                    me->CastSpell(_pos.GetPositionX(), _pos.GetPositionY(), _pos.GetPositionZ(), SPELL_BLACK_BLOOD_VISUAL, true);
                //blackblood 4
                _angle = me->GetOrientation() + M_PI;
                me->GetNearPosition(_pos, radius, _angle);
                if(BlackBloodPositionSelect(fallingfragments, _pos.GetPositionX(), _pos.GetPositionY()))
                    me->CastSpell(_pos.GetPositionX(), _pos.GetPositionY(), _pos.GetPositionZ(), SPELL_BLACK_BLOOD_VISUAL, true);
                //blackblood 5
                _angle = me->GetOrientation() + M_PI/4 + M_PI;
                me->GetNearPosition(_pos, radius, _angle);
                if(BlackBloodPositionSelect(fallingfragments, _pos.GetPositionX(), _pos.GetPositionY()))
                    me->CastSpell(_pos.GetPositionX(), _pos.GetPositionY(), _pos.GetPositionZ(), SPELL_BLACK_BLOOD_VISUAL, true);
                //blackblood 6
                _angle = me->GetOrientation() + M_PI/2 + M_PI;
                me->GetNearPosition(_pos, radius, _angle);
                if(BlackBloodPositionSelect(fallingfragments, _pos.GetPositionX(), _pos.GetPositionY()))
                    me->CastSpell(_pos.GetPositionX(), _pos.GetPositionY(), _pos.GetPositionZ(), SPELL_BLACK_BLOOD_VISUAL, true);
                //blackblood 7
                _angle = me->GetOrientation() + (3*M_PI)/4 + M_PI;
                me->GetNearPosition(_pos, radius, _angle);
                if(BlackBloodPositionSelect(fallingfragments, _pos.GetPositionX(), _pos.GetPositionY()))
                    me->CastSpell(_pos.GetPositionX(), _pos.GetPositionY(), _pos.GetPositionZ(), SPELL_BLACK_BLOOD_VISUAL, true);
                //blackblood 8
                _angle = me->GetOrientation() + M_PI*2;
                me->GetNearPosition(_pos, radius, _angle);
                if(BlackBloodPositionSelect(fallingfragments, _pos.GetPositionX(), _pos.GetPositionY()))
                    me->CastSpell(_pos.GetPositionX(), _pos.GetPositionY(), _pos.GetPositionZ(), SPELL_BLACK_BLOOD_VISUAL, true);
                radius += 10.0f;
            }
        }

        bool BlackBloodPositionSelect(std::list<GameObject*> const& collisionList, float x, float y)
        {
            for (std::list<GameObject*>::const_iterator itr = collisionList.begin(); itr != collisionList.end(); ++itr)
                if ((*itr)->IsInBetween(me, x, y, 5.0f))
                    return false;

            return true;
        }
    };
};

class boss_kohcrom : public CreatureScript
{
public:
    boss_kohcrom() : CreatureScript("boss_kohcrom") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new boss_kohcromAI (creature);
    }

    struct boss_kohcromAI : public ScriptedAI
    {
        boss_kohcromAI(Creature* c) : ScriptedAI(c) { }
        
        EventMap events;
        
        bool FuriousActivate;
        bool BlackBloodActive;
        
        uint8 BlackBloodPhase;
        uint8 BlackBloodPhase2;
        uint32 BlackBloodTimer;
        uint32 BlackBloodTimer2;

        void Reset() 
        {
            events.Reset();

            FuriousActivate = false;
            BlackBloodActive = false;

            BlackBloodPhase = 1;
            BlackBloodPhase2 = 1;
            BlackBloodTimer = 0;
            BlackBloodTimer2 = 0;

            events.ScheduleEvent(EVENT_BLACK_BLOOD, 54*IN_MILLISECONDS);
            if (!IsHeroic())
                events.ScheduleEvent(EVENT_CRUSH_ARMOR, urand(6*IN_MILLISECONDS, 8*IN_MILLISECONDS));
            events.ScheduleEvent(EVENT_STOMP, 12*IN_MILLISECONDS);
            events.ScheduleEvent(EVENT_RESONATING_CRYSTAL, 19*IN_MILLISECONDS);
            events.ScheduleEvent(EVENT_BERSERK, 10*MINUTE*IN_MILLISECONDS);
            me->setActive(true);
            me->SetReactState(REACT_AGGRESSIVE);
        }

        void EnterCombat(Unit* /*who*/) 
        {
            DoScriptText(YELL_AGGRO, me);
        }

        void JustDied(Unit* /*victim*/) 
        {
            DoScriptText(YELL_DIED, me);
        }

        void KilledUnit(Unit* /*victim*/) 
        {
            DoScriptText(YELL_KILL_1, me);
        }

        void UpdateAI(const uint32 diff) 
        {
            if (!UpdateVictim())
                return;

            if (!BlackBloodActive)
            {
                events.Update(diff);

                switch (events.ExecuteEvent())
                {
                    case EVENT_BLACK_BLOOD:
                    if (!IsHeroic())
                        events.RescheduleEvent(EVENT_CRUSH_ARMOR, 3*IN_MILLISECONDS);
                        BlackBloodActive = true;
                        BlackBloodPhase = 1;
                        events.ScheduleEvent(EVENT_BLACK_BLOOD, 74*IN_MILLISECONDS);
                        break;
                    case EVENT_CRUSH_ARMOR:
                        DoCastVictim(SPELL_CRUSH_ARMOR);
                        events.ScheduleEvent(EVENT_CRUSH_ARMOR, urand(6*IN_MILLISECONDS, 8*IN_MILLISECONDS));
                        break;
                    case EVENT_STOMP:
                    if (!IsHeroic())
                        events.RescheduleEvent(EVENT_CRUSH_ARMOR, 3*IN_MILLISECONDS);
                        DoCast(me, SPELL_STOMP, false);
                        events.ScheduleEvent(EVENT_STOMP, 12*IN_MILLISECONDS);
                        break;
                    case EVENT_RESONATING_CRYSTAL:
                    if (!IsHeroic())
                        events.RescheduleEvent(EVENT_CRUSH_ARMOR, 3*IN_MILLISECONDS);
                        if (Unit* target = SelectTarget(SELECT_TARGET_NEAREST, 0, NonTankTargetSelector(me)))
                        {
                            float _angle;
                            Position _pos;
                            _angle = me->GetAngle(target->GetPositionX(), target->GetPositionY());
                            me->GetNearPosition(_pos, 25.0f, _angle);
                            me->CastSpell(_pos.GetPositionX(), _pos.GetPositionY(), _pos.GetPositionZ(), SPELL_RESONATING_CRYSTAL, true);
                        }
                        DoScriptText(EMOTE_RESONATING_CRYSTAL, me);
                        DoScriptText(YELL_RESONATING_CRYSTAL, me);
                        events.ScheduleEvent(EVENT_RESONATING_CRYSTAL, 12*IN_MILLISECONDS);
                        break;
                    case EVENT_BERSERK:
                        DoCast(me, SPELL_BERSERK);
                        events.ScheduleEvent(EVENT_BERSERK, 100*IN_MILLISECONDS);
                        break;
                }

                if (!FuriousActivate && me->HealthBelowPct(21))
                {
                    DoCast(me, SPELL_FURIOUS);
                    FuriousActivate = true;
                }

                DoMeleeAttackIfReady();
            }
            else
            {
                if (BlackBloodTimer <= diff)
                {
                    switch (BlackBloodPhase)
                    {
                        case 1:
                            DoCastVictim(SPELL_EARTHEN_VORTEX, true);
                            BlackBloodTimer = 5000;
                            break;
                        case 2:
                            me->SetReactState(REACT_PASSIVE);
                            me->AttackStop();
                            DoScriptText(RAND(YELL_FALLING_FRAGMENTS_1, YELL_FALLING_FRAGMENTS_2), me);
                            DoCast(me, SPELL_FALLING_FRAGMENTS, true);
                            BlackBloodTimer = 5000;
                            BlackBloodTimer2 = 5500;
                            break;
                        case 3:
                            DoScriptText(YELL_BLACK_BLOOD_1, me);
                            DoCast(me, SPELL_BLACK_BLOOD, true);
                            SummonBlackBlood(BlackBloodPhase2);
                            BlackBloodTimer2 = 5500;
                            BlackBloodTimer = 17000;
                            BlackBloodPhase2++;
                            break;
                        case 4:
                            me->RemoveGameObject(SPELL_FALLING_FRAGME_SUM, true);
                            BlackBloodTimer = 0;
                            BlackBloodTimer2 = 0;
                            BlackBloodPhase2 = 1;
                            me->SetReactState(REACT_AGGRESSIVE);
                            BlackBloodActive = false;
                            break;
                    }

                    BlackBloodPhase++;
                } else BlackBloodTimer -= diff;

                if (BlackBloodTimer2 <= diff)
                {
                    SummonBlackBlood(BlackBloodPhase2);
                    BlackBloodPhase2++;
                    BlackBloodTimer2 = 5500;
                } else BlackBloodTimer2 -= diff;
            }
        }

        void SummonBlackBlood(int counter)
        {
            float _angle;
            float radius = 10.0f;
            Position _pos;
            std::list<GameObject*> fallingfragments;
            GetGameObjectListWithEntryInGrid(fallingfragments, me, INNER_WALL, 60.0f);
            for (int i = 0; i < counter; i++)
            {
                //blackblood 1
                _angle = me->GetOrientation() + M_PI/4;
                me->GetNearPosition(_pos, radius, _angle);
                if(BlackBloodPositionSelect(fallingfragments, _pos.GetPositionX(), _pos.GetPositionY()))
                    me->CastSpell(_pos.GetPositionX(), _pos.GetPositionY(), _pos.GetPositionZ(), SPELL_BLACK_BLOOD_VISUAL, true);
                //blackblood 2
                _angle = me->GetOrientation() + M_PI/2;
                me->GetNearPosition(_pos, radius, _angle);
                if(BlackBloodPositionSelect(fallingfragments, _pos.GetPositionX(), _pos.GetPositionY()))
                    me->CastSpell(_pos.GetPositionX(), _pos.GetPositionY(), _pos.GetPositionZ(), SPELL_BLACK_BLOOD_VISUAL, true);
                //blackblood 3
                _angle = me->GetOrientation() + (3*M_PI)/4;
                me->GetNearPosition(_pos, radius, _angle);
                if(BlackBloodPositionSelect(fallingfragments, _pos.GetPositionX(), _pos.GetPositionY()))
                    me->CastSpell(_pos.GetPositionX(), _pos.GetPositionY(), _pos.GetPositionZ(), SPELL_BLACK_BLOOD_VISUAL, true);
                //blackblood 4
                _angle = me->GetOrientation() + M_PI;
                me->GetNearPosition(_pos, radius, _angle);
                if(BlackBloodPositionSelect(fallingfragments, _pos.GetPositionX(), _pos.GetPositionY()))
                    me->CastSpell(_pos.GetPositionX(), _pos.GetPositionY(), _pos.GetPositionZ(), SPELL_BLACK_BLOOD_VISUAL, true);
                //blackblood 5
                _angle = me->GetOrientation() + M_PI/4 + M_PI;
                me->GetNearPosition(_pos, radius, _angle);
                if(BlackBloodPositionSelect(fallingfragments, _pos.GetPositionX(), _pos.GetPositionY()))
                    me->CastSpell(_pos.GetPositionX(), _pos.GetPositionY(), _pos.GetPositionZ(), SPELL_BLACK_BLOOD_VISUAL, true);
                //blackblood 6
                _angle = me->GetOrientation() + M_PI/2 + M_PI;
                me->GetNearPosition(_pos, radius, _angle);
                if(BlackBloodPositionSelect(fallingfragments, _pos.GetPositionX(), _pos.GetPositionY()))
                    me->CastSpell(_pos.GetPositionX(), _pos.GetPositionY(), _pos.GetPositionZ(), SPELL_BLACK_BLOOD_VISUAL, true);
                //blackblood 7
                _angle = me->GetOrientation() + (3*M_PI)/4 + M_PI;
                me->GetNearPosition(_pos, radius, _angle);
                if(BlackBloodPositionSelect(fallingfragments, _pos.GetPositionX(), _pos.GetPositionY()))
                    me->CastSpell(_pos.GetPositionX(), _pos.GetPositionY(), _pos.GetPositionZ(), SPELL_BLACK_BLOOD_VISUAL, true);
                //blackblood 8
                _angle = me->GetOrientation() + M_PI*2;
                me->GetNearPosition(_pos, radius, _angle);
                if(BlackBloodPositionSelect(fallingfragments, _pos.GetPositionX(), _pos.GetPositionY()))
                    me->CastSpell(_pos.GetPositionX(), _pos.GetPositionY(), _pos.GetPositionZ(), SPELL_BLACK_BLOOD_VISUAL, true);
                radius += 10.0f;
            }
        }

        bool BlackBloodPositionSelect(std::list<GameObject*> const& collisionList, float x, float y)
        {
            for (std::list<GameObject*>::const_iterator itr = collisionList.begin(); itr != collisionList.end(); ++itr)
                if ((*itr)->IsInBetween(me, x, y, 5.0f))
                    return false;

            return true;
        }
    };
};

// actually not less
bool less_(std::pair<Unit*, float> const& one, std::pair<Unit*, float> const& two)
{
    return one.second > two.second;
}

class spell_stomp : public SpellScriptLoader
{
    public:
        spell_stomp() : SpellScriptLoader("spell_stomp") { }

        class spell_stompSpellScript : public SpellScript
        {
            PrepareSpellScript(spell_stompSpellScript);

            bool Validate(SpellInfo const* spell)
            {
                return true;
            }

            uint8 count;
            std::vector<std::pair<Unit*, float> > data;

            bool Load()
            {
                return true;
            }

            void HandleTargets(std::list<Unit*>& targetList)
            {
                count = targetList.size() ? targetList.size() : 1;

                data.resize(2);
                memset(&data[0], 0, sizeof(std::pair<Unit*, float>) * data.size());

                int entries = 0;
                for (std::list<Unit*>::iterator itr = targetList.begin(); itr != targetList.end(); ++itr)
                {
                    Unit* unit = *itr;

                    if (unit)
                    {
                        std::pair<Unit*, float> pair = std::pair<Unit*, float>(unit,
                            GetCaster()->GetExactDist2dSq(unit->GetPositionX(), unit->GetPositionY()));

                        if (entries < int(data.size()))
                        {
                            data[entries] = pair;

                            ++entries;

                            if (entries == int(data.size()))
                                std::sort(data.begin(), data.end(), less_);
                        }
                        else if (less_(data[0], pair))
                        {
                            data[0] = pair;

                            for (int i = 0; i < int(data.size())-1; i++)
                            {
                                if (less_(data[i], data[i+1]))
                                    std::swap(data[i], data[i+1]);
                                else
                                    break;
                            }
                        }
                    }
                }
            }

            void RecalculateDamage()
            {
                for (uint8 i = 0; i < 2; i++)
                {
                    std::pair<Unit*, float>& pair = data[i];

                    if (pair.first == GetHitUnit())
                    {
                        SetHitDamage(GetHitDamage() / count * 2);
                        return;
                    }
                }

                SetHitDamage(GetHitDamage() / count);
            }

            void Register()
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_stompSpellScript::HandleTargets, EFFECT_0, TARGET_UNIT_DEST_AREA_ENEMY);
                OnHit += SpellHitFn(spell_stompSpellScript::RecalculateDamage);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_stompSpellScript();
        }
};

class spell_falling_fragments : public SpellScriptLoader
{
    public:
        spell_falling_fragments() : SpellScriptLoader("spell_falling_fragments") { }

        class spell_falling_fragments_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_falling_fragments_AuraScript);

            void HandlePeriodicTick(constAuraEffectPtr /*aurEff*/)
            {
                PreventDefaultAction();

                uint8 radius = urand(25, 30);
                float angle = frand(0.0f, 2*M_PI);
                float x = GetCaster()->GetPositionX() + cos(angle)*radius;
                float y = GetCaster()->GetPositionY() + sin(angle)*radius;
                float z = GetCaster()->GetPositionZ();

                GetCaster()->UpdateGroundPositionZ(x, y, z);
                
                GetCaster()->CastSpell(x, y, z, 103177, true);
            }

            void Register()
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_falling_fragments_AuraScript::HandlePeriodicTick, EFFECT_0, SPELL_AURA_PERIODIC_TRIGGER_SPELL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_falling_fragments_AuraScript();
        }
};

enum eCrystal
{
    SPELL_SAFE              = 103541,
    SPELL_WARNING           = 103536,
    SPELL_DANGER            = 103534,

    EVENT_DESPAWN           = 1,
};

class mobs_resonating_crystal : public CreatureScript
{
public:
    mobs_resonating_crystal() : CreatureScript("mobs_resonating_crystal") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new mobs_resonating_crystalAI (creature);
    }

    struct mobs_resonating_crystalAI : public PassiveAI
    {
        mobs_resonating_crystalAI(Creature* c) : PassiveAI(c) { }
        
        EventMap events;

        void Reset() 
        {
            events.Reset();

            events.ScheduleEvent(EVENT_DESPAWN, 10*IN_MILLISECONDS);
        }

        void UpdateAI(const uint32 diff) 
        {
            events.Update(diff);

            switch (events.ExecuteEvent())
            {
                case EVENT_DESPAWN:
                    DoCast(me, 103545, true);
                    me->DespawnOrUnsummon();
                    break;
            }
        }
    };
};

class spell_target_selection : public SpellScriptLoader
{
    public:
        spell_target_selection() : SpellScriptLoader("spell_target_selection") { }

        class spell_target_selection_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_target_selection_SpellScript);

            void HandleDummy(SpellEffIndex effIndex)
            {
                std::list<Player*> list;

                Trinity::AnyPlayerInObjectRangeCheck checker(GetCaster(), 100.0f);
                Trinity::PlayerListSearcher<Trinity::AnyPlayerInObjectRangeCheck> searcher(GetCaster(), list, checker);
                GetCaster()->VisitNearbyWorldObject(100.0f, searcher);

                uint8 playerCount = GetCaster()->GetMap()->Is25ManRaid() ? 7 : 3;
                std::vector<std::pair<Player*, float> > data(playerCount);
                memset(&data[0], 0, sizeof(std::pair<Player*, float>) * data.size());

                int entries = 0;
                for (std::list<Player*>::iterator itr = list.begin(); itr != list.end(); ++itr)
                {
                    Player* unit = *itr;

                    if (unit)
                    {
                        std::pair<Player*, float> pair = std::pair<Player*, float>(unit,
                            GetCaster()->GetExactDist2dSq(unit->GetPositionX(), unit->GetPositionY()));

                        if (entries < int(data.size()))
                        {
                            data[entries] = pair;

                            ++entries;

                            if (entries == int(data.size()))
                                std::sort(data.begin(), data.end(), less_);
                        }
                        else if (less_(data[0], pair))
                        {
                            data[0] = pair;

                            for (int i = 0; i < int(data.size())-1; i++)
                            {
                                if (less_(data[i], data[i+1]))
                                    std::swap(data[i], data[i+1]);
                                else
                                    break;
                            }
                        }
                        if(unit->HasAura(SPELL_SAFE))
                            unit->RemoveAurasDueToSpell(SPELL_SAFE);
                        if(unit->HasAura(SPELL_WARNING))
                            unit->RemoveAurasDueToSpell(SPELL_WARNING);
                        if(unit->HasAura(SPELL_DANGER))
                            unit->RemoveAurasDueToSpell(SPELL_DANGER);
                    }
                }

                for (uint8 i = 0; i < playerCount; i++)
                {
                    std::pair<Player*, float>& pair = data[i];

                    if (pair.first)
                    {
                        float distSq = pair.second;
                        if (distSq < float(10*10))
                            GetCaster()->CastSpell(pair.first, SPELL_SAFE);
                        else if (distSq < float(20*20))
                            GetCaster()->CastSpell(pair.first, SPELL_WARNING);
                        else
                            GetCaster()->CastSpell(pair.first, SPELL_DANGER);
                    }
                }
            }

            void Register() 
            {
                OnEffectLaunch += SpellEffectFn(spell_target_selection_SpellScript::HandleDummy, EFFECT_1, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript * GetSpellScript() const
        {
            return new spell_target_selection_SpellScript();
        }
};

class spell_resonating_crystal : public SpellScriptLoader
{
    public:
        spell_resonating_crystal() : SpellScriptLoader("spell_resonating_crystal") { }

        class spell_resonating_crystal_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_resonating_crystal_SpellScript);

            float dis;
            std::list<Unit*> List;

            bool Validate(SpellInfo const* spell) 
            {
                return true;
            }

            bool Load() 
            {
                return true;
            }

            /*void HandleTargets(std::list<Unit*>& unitList)
            {
                std::list<Player*> list;
                unitList.clear();
                dis = 0.0f;
                //Unit* victim = GetCaster()->getVictim();

                Trinity::AnyPlayerInObjectRangeCheck checker(GetCaster(), 100.0f);
                Trinity::PlayerListSearcher<Trinity::AnyPlayerInObjectRangeCheck> searcher(GetCaster(), list, checker);
                GetCaster()->VisitNearbyWorldObject(100.0f, searcher);

                uint8 playerCount = GetCaster()->GetMap()->Is25ManRaid() ? 7 : 3;
                std::vector<std::pair<Player*, float> > data(playerCount);
                memset(&data[0], 0, sizeof(std::pair<Player*, float>) * data.size());

                int entries = 0;
                for (std::list<Player*>::iterator itr = list.begin(); itr != list.end(); ++itr)
                {
                    Player* unit = *itr;

                    //if (unit && (!victim || victim->GetGUID() != unit->GetGUID()) && unit->IsWithinDist(GetCaster(), 40))
                    if (unit && (unit->HasAura(SPELL_SAFE) || unit->HasAura(SPELL_WARNING) || unit->HasAura(SPELL_DANGER)))
                    {
                        std::pair<Player*, float> pair = std::pair<Player*, float>(unit,
                            GetCaster()->GetExactDist2dSq(unit->GetPositionX(), unit->GetPositionY()));

                        if (entries < int(data.size()))
                        {
                            data[entries] = pair;

                            ++entries;

                            if (entries == data.size())
                                std::sort(data.begin(), data.end(), less_);
                        }
                        else if (less_(data[0], pair))
                        {
                            data[0] = pair;

                            for (int i = 0; i < int(data.size())-1; i++)
                            {
                                if (less_(data[i], data[i+1]))
                                    std::swap(data[i], data[i+1]);
                                else
                                    break;
                            }
                        }
                    }
                }

                for (uint8 i = 0; i < playerCount; i++)
                {
                    std::pair<Player*, float>& pair = data[i];

                    if (pair.first)
                    {
                        if (dis < pair.second)
                            dis = pair.second;

                        unitList.push_back(pair.first);
                    }
                }

                dis = sqrt(dis);
            }*/

            void HandleTargets(std::list<Unit*>& unitList)
            {
                Unit* caster = GetCaster();
                if(caster)
                {
                    for (std::list<Unit*>::iterator itr = unitList.begin() ; itr != unitList.end();)
                    {
                        Unit* unit = *itr;
                        if (unit->HasAura(SPELL_SAFE) || unit->HasAura(SPELL_WARNING) || unit->HasAura(SPELL_DANGER))
                        {
                            itr++;
                            float dist = caster->GetExactDist2dSq(unit->GetPositionX(), unit->GetPositionY());
                            if (dis < dist)
                                dis = dist;

                            unit->RemoveAurasDueToSpell(SPELL_SAFE);
                            unit->RemoveAurasDueToSpell(SPELL_WARNING);
                            unit->RemoveAurasDueToSpell(SPELL_DANGER);
                        }
                        else
                            itr = unitList.erase(itr);
                    }
                }
                dis = sqrt(dis);
                List = unitList;
            }

            void FilterTargets(std::list<Unit*>& unitList)
            {
                unitList = List;
            }

            void RecalculateDamage()
            {
                SetHitDamage(7200*(dis < 1.0f ? 1.0f : dis));
            }

            void Register() 
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_resonating_crystal_SpellScript::HandleTargets, EFFECT_0, TARGET_UNIT_DEST_AREA_ENEMY);
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_resonating_crystal_SpellScript::FilterTargets, EFFECT_1, TARGET_UNIT_DEST_AREA_ENEMY);
                OnHit += SpellHitFn(spell_resonating_crystal_SpellScript::RecalculateDamage);
            }
        };

        SpellScript * GetSpellScript() const
        {
            return new spell_resonating_crystal_SpellScript();
        }
};

class FallingFragmentTargetSelector
{
    public:
        FallingFragmentTargetSelector(Unit* caster, std::list<GameObject*> const& collisionList) : _caster(caster), _collisionList(collisionList) { }

        bool operator()(Unit* unit)
        {
            for (std::list<GameObject*>::const_iterator itr = _collisionList.begin(); itr != _collisionList.end(); ++itr)
                if ((*itr)->IsInBetween(_caster, unit, 5.0f))
                    return true;

            return false;
        }

    private:
        Unit* _caster;
        std::list<GameObject*> const& _collisionList;
};

class spell_morchok_black_blood : public SpellScriptLoader
{
    public:
        spell_morchok_black_blood() : SpellScriptLoader("spell_morchok_black_blood") { }

        class spell_morchok_black_blood_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_morchok_black_blood_SpellScript);

            void FilterTargets(std::list<Unit*>& unitList)
            {
                std::list<GameObject*> fallingfragments;
                GetGameObjectListWithEntryInGrid(fallingfragments, GetCaster(), INNER_WALL, 200.0f);
                unitList.remove_if (FallingFragmentTargetSelector(GetCaster(), fallingfragments));
            }

            void Register()
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_morchok_black_blood_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_DEST_AREA_ENEMY);
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_morchok_black_blood_SpellScript::FilterTargets, EFFECT_1, TARGET_UNIT_DEST_AREA_ENEMY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_morchok_black_blood_SpellScript();
        }
};

class spell_morchok_resonating_crystal : public SpellScriptLoader
{
    public:
        spell_morchok_resonating_crystal() : SpellScriptLoader("spell_morchok_resonating_crystal") { }

        class spell_morchok_resonating_crystal_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_morchok_resonating_crystal_AuraScript);

            int32 amount;

            bool Load()
            {
                amount = 200;
                return true;
            }

            void OnPeriodic(constAuraEffectPtr /*aurEff*/)
            {
                if(amount > 0)
                    amount -= 20;
                if (AuraEffectPtr effect = GetAura()->GetEffect(EFFECT_1))
                    effect->ChangeAmount(amount);
            }

            // function registering
            void Register()
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_morchok_resonating_crystal_AuraScript::OnPeriodic, EFFECT_0, SPELL_AURA_PERIODIC_TRIGGER_SPELL);
            }
        };

        // function which creates AuraScript
        AuraScript* GetAuraScript() const
        {
            return new spell_morchok_resonating_crystal_AuraScript();
        }
};

void AddSC_boss_morchok()
{
    new boss_morchok();
    new boss_kohcrom();
    new spell_stomp();
    new spell_falling_fragments();
    new mobs_resonating_crystal();
    new spell_target_selection();
    new spell_resonating_crystal();
    new spell_morchok_black_blood();
    new spell_morchok_resonating_crystal();
}
