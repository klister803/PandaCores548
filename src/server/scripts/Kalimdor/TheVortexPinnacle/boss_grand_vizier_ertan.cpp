#include"ScriptPCH.h"
#include"the_vortex_pinnacle.h"
#include "ScriptedEscortAI.h"

enum ScriptTexts
{
    SAY_AGGRO    = 0,
    SAY_KILL    = 1,
    SAY_DEATH    = 2,
};

enum Spells
{
    SPELL_CYCLONE_SHIELD        = 86267,
    SPELL_CYCLONE_SHIELD_DMG    = 86292,
    SPELL_CYCLONE_SHIELD_DMG_H    = 93991,
    SPELL_SUMMON_TEMPEST        = 86340,
    SPELL_STORM_EDGE            = 86309,
    SPELL_STORM_EDGE_H            = 93992,
    SPELL_LIGHTNING_BOLT        = 86331,
    SPELL_LIGHTNING_BOLT_H        = 93990,
};

enum Events
{
    EVENT_LIGHTNING_BOLT    = 1,
    EVENT_STORM_EDGE        = 2,
    EVENT_CALL_VORTEX        = 3,
    EVENT_RESET_VORTEX        = 4,
};

enum Adds
{
    NPC_ERTAN_VORTEX    = 46007,
    NPC_SLIPSTREAM        = 45455,
};

const Position ertanvortexPos_1[8] = 
{
    {-702.109985f, -13.500000f, 635.669983f, 0.0f},
    {-719.549988f, -21.190001f, 635.669983f, 0.0f},
    {-737.419983f, -13.970000f, 635.669983f, 0.0f},
    {-745.000000f, 3.990000f,635.669983f, 0.0f},
    {-737.650024f, 21.790001f, 635.669983f, 0.0f},
    {-720.190002f, 29.540001f, 635.669983f, 0.0f},
    {-702.070007f, 22.150000f, 635.669983f, 0.0f},
    {-694.539978f, 4.250000f,635.669983f, 0.0f},
};

class boss_grand_vizier_ertan : public CreatureScript
{
    public:
        boss_grand_vizier_ertan() : CreatureScript("boss_grand_vizier_ertan") { }
        
        CreatureAI* GetAI(Creature* pCreature) const
        {
            return new boss_grand_vizier_ertanAI(pCreature);
        }
        struct boss_grand_vizier_ertanAI : public BossAI
        {
            boss_grand_vizier_ertanAI(Creature* pCreature) : BossAI(pCreature, DATA_ERTAN)
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

            Creature* _vortexes[8];
            float _distance;
    
            void Reset()
            {
                _Reset();
                memset(_vortexes, NULL, sizeof(_vortexes));
                me->SetReactState(REACT_AGGRESSIVE);
                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
            }
    
            void EnterCombat(Unit* /*pWho*/)
            {
                for (uint8 i = 0; i < 8; i++)
                {
                    _vortexes[i] = me->SummonCreature(NPC_ERTAN_VORTEX, ertanvortexPos_1[i]);
                    if(_vortexes[i]->AI())
                        _vortexes[i]->AI()->DoAction(i);
                }

                events.ScheduleEvent(EVENT_LIGHTNING_BOLT, 3000);
                events.ScheduleEvent(EVENT_CALL_VORTEX, urand(18000, 21000));
                events.ScheduleEvent(EVENT_STORM_EDGE, 5000);
                Talk(SAY_AGGRO);
                DoZoneInCombat();
                _EnterCombat();
            }

            void KilledUnit(Unit* /*pWho*/)
            {
                Talk(SAY_KILL);                
            }

            void JustDied(Unit* /*pWho*/)
            {
                _JustDied();
                Talk(SAY_DEATH);
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
                        case EVENT_LIGHTNING_BOLT:
                            if (me->HasUnitState(UNIT_STATE_CASTING))
                                return;
                            DoCast(me->getVictim(), SPELL_LIGHTNING_BOLT);
                            events.ScheduleEvent(EVENT_LIGHTNING_BOLT, 2000);
                            break;
                        case EVENT_CALL_VORTEX:
                            for (uint8 i = 0; i < 8; i++)
                                if (_vortexes[i] && _vortexes[i]->AI())
                                    _vortexes[i]->AI()->DoAction(15);
                            events.ScheduleEvent(EVENT_RESET_VORTEX, urand(14000, 17000));
                            break;
                        case EVENT_RESET_VORTEX:
                            for (uint8 i = 0; i < 8; i++)
                                if (_vortexes[i] && _vortexes[i]->AI())
                                    _vortexes[i]->AI()->DoAction(16);
                            events.ScheduleEvent(EVENT_CALL_VORTEX, urand(20000, 25000));
                            break;
                        case EVENT_STORM_EDGE:
                            _distance = me->GetDistance2d(_vortexes[1]);
                            if (me->GetMap()->GetPlayers().isEmpty())
                                return;
                            for (Map::PlayerList::const_iterator itr = me->GetMap()->GetPlayers().begin(); itr != me->GetMap()->GetPlayers().end(); ++itr)
                            {
                                if (Player* pPlayer = itr->getSource())
                                {
                                    if (me->GetDistance2d(pPlayer) > _distance)
                                    {
                                        uint8 i = urand(0, 7);
                                        if (_vortexes[i])
                                            _vortexes[i]->CastSpell(itr->getSource(), SPELL_STORM_EDGE, true);
                                        DoCast(pPlayer, SPELL_STORM_EDGE, true);
                                    }
                                }
                            }
                            events.ScheduleEvent(EVENT_STORM_EDGE, 2000);
                            break;
                    }
                }
                DoMeleeAttackIfReady();
            }
        };
};

class npc_ertan_vortex : public CreatureScript
{
    public:
        npc_ertan_vortex() : CreatureScript("npc_ertan_vortex") { }
        
        CreatureAI* GetAI(Creature* pCreature) const
        {
            return new npc_ertan_vortexAI(pCreature);
        }
        struct npc_ertan_vortexAI : public npc_escortAI
        {
            npc_ertan_vortexAI(Creature* pCreature) : npc_escortAI(pCreature)
            {
                Reset();
            }

            EventMap events;
            uint8 _mobid;
            Unit* _owner;
            float x, y, z;

            void Reset()
            {
                _owner = NULL;
                x = y = z = 0.0f;
                _mobid = 0;
                DoCast(me, SPELL_CYCLONE_SHIELD);
                events.Reset();
            }

            void InitWaypoint()
            {
                AddWaypoint(0, -702.109985f, -13.500000f, 635.669983f);
                AddWaypoint(1, -702.109985f, -13.500000f, 635.669983f);
                AddWaypoint(2, -719.549988f, -21.190001f, 635.669983f);
                AddWaypoint(3, -737.419983f, -13.970000f, 635.669983f);
                AddWaypoint(4, -745.000000f, 3.990000f, 635.669983f);
                AddWaypoint(5, -737.650024f, 21.790001f, 635.669983f);
                AddWaypoint(6, -720.190002f, 29.540001f, 635.669983f);
                AddWaypoint(7, -702.070007f, 22.150000f, 635.669983f);
                AddWaypoint(8, -694.539978f, 4.250000f, 635.669983f);
            }

            void WaypointReached(uint32 i)
            {
                if(i == 8)
                    SetCurentWP(0);
            }

            void IsSummonedBy(Unit* owner)
            {
                _owner = owner;
            }

            void DoAction(const int32 action)
            {
                if(action < 10)
                {
                    _mobid = action + 1;
                    InitWaypoint();
                    Start(false, false);
                    SetCurentWP(_mobid);
                    //SetDespawnAtFar(false);
                }
                else if(action == 15)
                {
                    SetEscortPaused(true);
                    events.ScheduleEvent(EVENT_CALL_VORTEX, 1000);
                }
                else if(action == 16)
                {
                    SetEscortPaused(true);
                    events.ScheduleEvent(EVENT_RESET_VORTEX, 1000);
                }
            }

            void UpdateAI(const uint32 diff)
            {
                events.Update(diff);
                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_CALL_VORTEX:
                        {
                            if(_owner)
                            {
                                float _angle;
                                Position _pos;
                                x = me->GetPositionX();
                                y = me->GetPositionY();
                                z = me->GetPositionZ();
                                _angle = _owner->GetAngle(me->GetPositionX(), me->GetPositionY());
                                _owner->GetNearPosition(_pos, 5.0f, _angle);
                                me->GetMotionMaster()->MovementExpired(false);
                                me->GetMotionMaster()->MovePoint(GetCurentWP(), _pos);
                            }
                            break;
                        }
                        case EVENT_RESET_VORTEX:
                        {
                            //me->GetMotionMaster()->MovementExpired(false);
                            //me->GetMotionMaster()->MovePoint(2, x, y, z);
                            SetEscortPaused(false);
                            break;
                        }
                    }
                }

                npc_escortAI::UpdateAI(diff);
            }
     };
};

void AddSC_boss_grand_vizier_ertan()
{
    new boss_grand_vizier_ertan();
    new npc_ertan_vortex();
}