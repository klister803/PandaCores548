#include "ScriptPCH.h"
#include "dragon_soul.h"

enum eZonozz
{
    SPELL_BERSERK                  = 26662,
    SPELL_VOID_OF_THE_UNMAKING     = 103571,
    SPELL_VOID_DIFFUSION_1         = 104031,
    SPELL_BLACK_BLOOD              = 104378,
    SPELL_BLACK_BLOOD_H            = 104377,
    SPELL_TANTRUM                  = 103953,
    SPELL_DERKNESS                 = 109413,

    SPELL_FOCUSED_ANGER_10N        = 104543,
    SPELL_FOCUSED_ANGER_10H        = 109410,
    SPELL_FOCUSED_ANGER_25N        = 109409,
    SPELL_FOCUSED_ANGER_25H        = 109411,
    SPELL_FOCUSED_ANGER_LFR        = 109409,

    SPELL_PSYCHIC_DRAIN_10N        = 104322,
    SPELL_PSYCHIC_DRAIN_10H        = 104607,
    SPELL_PSYCHIC_DRAIN_25N        = 104606,
    SPELL_PSYCHIC_DRAIN_25H        = 104608,
    SPELL_PSYCHIC_DRAIN_LFR        = 104606,

    SPELL_DISRUPTING_SHADOWS_10N   = 103434,
    SPELL_DISRUPTING_SHADOWS_10H   = 104600,
    SPELL_DISRUPTING_SHADOWS_25N   = 104599,
    SPELL_DISRUPTING_SHADOWS_25H   = 104601,
    SPELL_DISRUPTING_SHADOWS_LFR   = 104599,

    EVENT_BERSERK                  = 1,
    EVENT_FOCUSED_ANGER            = 2,
    EVENT_PSYCHIC_DRAIN            = 3,
    EVENT_DISRUPTING_SHADOWS       = 4,
    EVENT_VOID_OF_THE_UNMAKING     = 5,
    EVENT_PHASE_2                  = 6,
    EVENT_CHECK_POSITION           = 7,

    ACTION_VOID_HITS               = 1,

    YELL_INTRO                     = -2000325,
    WHISPER_INTRO                  = -2000326,
    YELL_AGGRO                     = -2000327,
    WHISPER_AGGRO                  = -2000328,
    YELL_DISRUPTING_SHADOWS_1      = -2000329,
    YELL_DISRUPTING_SHADOWS_2      = -2000330,
    YELL_DISRUPTING_SHADOWS_3      = -2000331,
    WHISPER_DISRUPTING_SHADOWS_1   = -2000332,
    WHISPER_DISRUPTING_SHADOWS_2   = -2000333,
    WHISPER_DISRUPTING_SHADOWS_3   = -2000334,
    YELL_BLACK_BLOOD               = -2000335,
    WHISPER_BLACK_BLOOD            = -2000336,
    YELL_VOID_OF_THE_UNMAKING      = -2000337,
    WHISPER_VOID_OF_THE_UNMAKING   = -2000338,
    YELL_KILL_1                    = -2000339,
    YELL_KILL_2                    = -2000340,
    YELL_KILL_3                    = -2000341,
    WHISPER_KILL_1                 = -2000342,
    WHISPER_KILL_2                 = -2000343,
    WHISPER_KILL_3                 = -2000344,
    YELL_DEATH                     = -2000345,
    WHISPER_DEATH                  = -2000346,
};

enum Phases
{
    PHASE_1                 = 1,
    PHASE_2                 = 2,
};

#define MAX_SUMMONS_G 14
#define MAX_SUMMONS_N 8

Position const summonpos_g[MAX_SUMMONS_G] =
{
    {-1701.138f, -1883.397f, -221.331f, 3.599924f},
    {-1696.089f, -1940.856f, -221.2506f, 2.806672f},
    {-1733.703f, -1984.603f, -221.3326f, 2.040907f},
    {-1791.600f, -1989.465f, -221.296f, 1.228019f},
    {-1835.749f, -1952.043f, -221.2816f, 0.4543397f},
    {-1840.347f, -1894.612f, -221.3248f, 5.963980f},
    {-1803.147f, -1850.224f, -221.3028f, 5.174665f},
    {-1745.112f, -1845.156f, -221.2553f, 4.408912f},
    {-1805.304f, -1889.113f, -225.4476f, 5.410419f},
    {-1731.457f, -1875.717f, -225.2117f, 1.076788f},
    {-1803.66f, -1959.571f, -225.2737f, 0.78052f},
    {-1765.915f, -1973.526f, -225.1206f, 1.569820f},
    {-1799.044f, -1926.346f, -226.2367f, 0.128618f},
    {-1724.930f, -1914.536f, -226.3537f, 3.325189f},
};

Position const summonpos_n[MAX_SUMMONS_N] =
{
    {-1701.138f, -1883.397f, -221.331f, 3.599924f},
    {-1696.089f, -1940.856f, -221.2506f, 2.806672f},
    {-1840.347f, -1894.612f, -221.3248f, 5.963980f},
    {-1803.147f, -1850.224f, -221.3028f, 5.174665f},
    {-1745.112f, -1845.156f, -221.2553f, 4.408912f},
    {-1805.304f, -1889.113f, -225.4476f, 5.410419f},
    {-1731.457f, -1875.717f, -225.2117f, 1.076788f},
    {-1724.930f, -1914.536f, -226.3537f, 3.325189f},
};

class boss_warlord_zonozz : public CreatureScript
{
public:
    boss_warlord_zonozz() : CreatureScript("boss_warlord_zonozz") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new boss_warlord_zonozzAI (creature);
    }

    struct boss_warlord_zonozzAI : public BossAI
    {
        boss_warlord_zonozzAI(Creature* c) : BossAI(c, DATA_WARLORD_ZONOZZ)
        {
        }

        EventMap events;

        uint8 phase;

        void Reset() 
        {
            _Reset();
            events.Reset();
            summons.DespawnAll();

            phase = 1;
            events.SetPhase(PHASE_1);
            me->SetReactState(REACT_AGGRESSIVE);

            events.ScheduleEvent(EVENT_BERSERK, 6*MINUTE*IN_MILLISECONDS);
            events.ScheduleEvent(EVENT_FOCUSED_ANGER, 6*IN_MILLISECONDS, 0, PHASE_1);
            events.ScheduleEvent(EVENT_PSYCHIC_DRAIN, 15*IN_MILLISECONDS);
            events.ScheduleEvent(EVENT_DISRUPTING_SHADOWS, 25*IN_MILLISECONDS, 0, PHASE_1);
            events.ScheduleEvent(EVENT_VOID_OF_THE_UNMAKING, 6*IN_MILLISECONDS);
            events.ScheduleEvent(EVENT_CHECK_POSITION, 5*IN_MILLISECONDS);
        }

        void EnterCombat(Unit* /*who*/) 
        {
            _EnterCombat();
            DoScriptText(YELL_AGGRO, me);
            WhisperToAllPlayerInZone(WHISPER_AGGRO, me);
        }

        void JustDied(Unit* /*victim*/) 
        {
             _JustDied();
            summons.DespawnAll();

            DoScriptText(YELL_DEATH, me);
            WhisperToAllPlayerInZone(WHISPER_DEATH, me);
        }

        void SummonedCreatureDies(Creature* summon, Unit* /*killer*/)
        {
            if(summon && (summon->GetEntry() == 55416 || summon->GetEntry() == 55417 || summon->GetEntry() == 55418))
            {
                Map::PlayerList const& players = me->GetMap()->GetPlayers();
                if (!players.isEmpty())
                    for (Map::PlayerList::const_iterator itr = players.begin(); itr != players.end(); ++itr)
                        if(Player* player = itr->getSource())
                        {
                            if (AuraPtr aura = player->GetAura(SPELL_BLACK_BLOOD_H))
                                aura->SetStackAmount(aura->GetStackAmount() - 1);
                        }
            }
        }

        void JustSummoned(Creature* summon) 
        {
            summons.Summon(summon);
        }

        void KilledUnit(Unit* /*victim*/) 
        {
            uint8 r = urand(0, 3);
            DoScriptText(YELL_KILL_1 - r, me);
            WhisperToAllPlayerInZone(WHISPER_KILL_1 - r, me);
        }

        void DoAction(int32 const action)
        {
            phase = 2;

            DoCast(me, SPELL_DERKNESS);
            DoCast(me, SPELL_TANTRUM);
            //DoCast(me, SPELL_VOID_DIFFUSION_1);
            uint32 uStack = action;
            if (AuraPtr aura = me->GetAura(SPELL_VOID_DIFFUSION_1))
                uStack += aura->GetStackAmount();

            me->RemoveAurasDueToSpell(SPELL_VOID_DIFFUSION_1);
            me->SetAuraStack(SPELL_VOID_DIFFUSION_1, me, uStack);

            if (IsHeroic())
            {
                if(Is25ManRaid())
                {
                    for (uint8 i = 0; i < MAX_SUMMONS_G; ++i)
                        if (Creature* summon = me->SummonCreature(i < 8 ? 55416 : i < 12 ? 55417 : 55418, summonpos_g[i], TEMPSUMMON_TIMED_DESPAWN, 6*MINUTE*IN_MILLISECONDS))
                        {
                            summon->AddUnitState(UNIT_STATE_ROOT);
                            DoCast(me, SPELL_BLACK_BLOOD_H, true);
                        }
                }
                else
                {
                    for (uint8 i = 0; i < MAX_SUMMONS_N; ++i)
                        if (Creature* summon = me->SummonCreature(i < 5 ? 55416 : i < 7 ? 55417 : 55418, summonpos_n[i], TEMPSUMMON_TIMED_DESPAWN, 6*MINUTE*IN_MILLISECONDS))
                        {
                            summon->AddUnitState(UNIT_STATE_ROOT);
                            DoCast(me, SPELL_BLACK_BLOOD_H, true);
                        }
                }
            }

            events.ScheduleEvent(EVENT_PHASE_2, 30*IN_MILLISECONDS);
            me->RemoveAurasDueToSpell(SPELL_FOCUSED_ANGER_10N);
            me->RemoveAurasDueToSpell(SPELL_FOCUSED_ANGER_10H);
            me->RemoveAurasDueToSpell(SPELL_FOCUSED_ANGER_25N);
            me->RemoveAurasDueToSpell(SPELL_FOCUSED_ANGER_25H);
            if (!IsHeroic())
                me->SetReactState(REACT_PASSIVE);
            me->AttackStop();
            me->SetTarget(0);
            events.SetPhase(PHASE_2);
            me->GetMotionMaster()->MovePoint(1, me->GetHomePosition());
        }

        void MovementInform(uint32 type, uint32 id)
        {
            if(type != POINT_MOTION_TYPE)
                return;

            if (id == 1 && !IsHeroic())
                DoCast(me, SPELL_BLACK_BLOOD);
        }

        void UpdateAI(const uint32 diff) 
        {
            if (!UpdateVictim())
                return;

            events.Update(diff);

            switch (events.ExecuteEvent())
            {
                case EVENT_BERSERK:
                    DoCast(me, SPELL_BERSERK);
                    events.ScheduleEvent(EVENT_BERSERK, 100*IN_MILLISECONDS);
                    break;
                case EVENT_FOCUSED_ANGER:
                    if (phase == 1)
                        DoCast(me, RAID_MODE(SPELL_FOCUSED_ANGER_10N, SPELL_FOCUSED_ANGER_10H, SPELL_FOCUSED_ANGER_25N, SPELL_FOCUSED_ANGER_25H));
                    events.ScheduleEvent(EVENT_FOCUSED_ANGER, 6*IN_MILLISECONDS, 0, PHASE_1);
                    break;
                case EVENT_PSYCHIC_DRAIN:
                    if (phase == 1)
                        DoCastVictim(RAID_MODE(SPELL_PSYCHIC_DRAIN_10N, SPELL_PSYCHIC_DRAIN_10H, SPELL_PSYCHIC_DRAIN_25N, SPELL_PSYCHIC_DRAIN_25H));
                    events.ScheduleEvent(EVENT_PSYCHIC_DRAIN, 20*IN_MILLISECONDS, 0, PHASE_1);
                    break;
                case EVENT_DISRUPTING_SHADOWS:
                {
                    if (phase == 1)
                    {
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, NonTankTargetSelector(me)))
                            DoCast(target, RAID_MODE(SPELL_DISRUPTING_SHADOWS_10N, SPELL_DISRUPTING_SHADOWS_10H, SPELL_DISRUPTING_SHADOWS_25N, SPELL_DISRUPTING_SHADOWS_25H), true);
                        uint8 r = urand(0, 3);
                        DoScriptText(YELL_DISRUPTING_SHADOWS_1 - r, me);
                        WhisperToAllPlayerInZone(WHISPER_DISRUPTING_SHADOWS_1 - r, me);
                    }
                    events.ScheduleEvent(EVENT_DISRUPTING_SHADOWS, 25*IN_MILLISECONDS, 0, PHASE_1);
                    break;
                }
                case EVENT_VOID_OF_THE_UNMAKING:
                {
                    if (phase == 1)
                    {
                        float x = me->GetPositionX() + cos(me->GetOrientation())*20;
                        float y = me->GetPositionY() + sin(me->GetOrientation())*20;
                        float z = me->GetPositionZ();

                        me->UpdateGroundPositionZ(x, y, z);

                        me->CastSpell(x, y, z, SPELL_VOID_OF_THE_UNMAKING, true);
                        DoScriptText(YELL_VOID_OF_THE_UNMAKING, me);
                        WhisperToAllPlayerInZone(WHISPER_VOID_OF_THE_UNMAKING, me);
                    }
                    events.ScheduleEvent(EVENT_VOID_OF_THE_UNMAKING, 90*MINUTE*IN_MILLISECONDS);
                    break;
                }
                case EVENT_PHASE_2:
                {
                    if (phase == 2)
                    {
                        phase = 1;

                        me->SetReactState(REACT_AGGRESSIVE);
                        events.SetPhase(PHASE_1);
                        events.ScheduleEvent(EVENT_FOCUSED_ANGER, 6*IN_MILLISECONDS, 0, PHASE_1);
                        events.ScheduleEvent(EVENT_PSYCHIC_DRAIN, 20*IN_MILLISECONDS, 0, PHASE_1);
                        events.ScheduleEvent(EVENT_DISRUPTING_SHADOWS, 25*IN_MILLISECONDS, 0, PHASE_1);
                        events.ScheduleEvent(EVENT_VOID_OF_THE_UNMAKING, 6*IN_MILLISECONDS);
                    }
                    events.ScheduleEvent(EVENT_PHASE_2, 30*MINUTE*IN_MILLISECONDS);
                    break;
                }
                case EVENT_CHECK_POSITION:
                {
                    if (me->GetDistance2d(me->GetHomePosition().GetPositionX(), me->GetHomePosition().GetPositionY()) > 60.0f)
                        DoCast(me, SPELL_BERSERK);
                    if (me->GetDistance2d(me->GetHomePosition().GetPositionX(), me->GetHomePosition().GetPositionY()) > 120.0f)
                        EnterEvadeMode();
                    events.ScheduleEvent(EVENT_CHECK_POSITION, 5*IN_MILLISECONDS);
                    break;
                }
            }

            DoMeleeAttackIfReady();
        }
    };
};

enum eVoidOfTheUnmaking
{
    EVENT_MOVE              = 1,
    EVENT_DELAY             = 2,

    SPELL_VOID_DIFFUSION    = 104605,
    SPELL_VOID_DIFFUSION_2  = 106836,
    SPELL_BLACK_BLOOD_E     = 108799,
};

class npc_void_of_the_unmaking : public CreatureScript
{
public:
    npc_void_of_the_unmaking() : CreatureScript("npc_void_of_the_unmaking") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_void_of_the_unmakingAI (creature);
    }

    struct npc_void_of_the_unmakingAI : public ScriptedAI
    {
        npc_void_of_the_unmakingAI(Creature* c) : ScriptedAI(c) { }

        EventMap events;
        bool delay;
        
        void Reset() 
        {
            events.Reset();
            delay = false;

            events.ScheduleEvent(EVENT_MOVE, 2000);

            Position pos;
            pos.m_positionX = me->GetPositionX() + 10*cos(me->ToTempSummon()->GetSummoner()->GetOrientation());
            pos.m_positionY = me->GetPositionY() + 10*sin(me->ToTempSummon()->GetSummoner()->GetOrientation());
            pos.m_positionZ = me->GetBaseMap()->GetHeight(me->GetPhaseMask(), pos.m_positionX, pos.m_positionY, me->GetPositionZ(), true);

            me->GetMotionMaster()->MovePoint(1, pos);
        }

        void UpdateMovement()
        {
            Position pos;
            pos.m_positionX = me->GetPositionX() + 10*cos(me->GetOrientation());
            pos.m_positionY = me->GetPositionY() + 10*sin(me->GetOrientation());
            pos.m_positionZ = me->GetBaseMap()->GetHeight(me->GetPhaseMask(), pos.m_positionX, pos.m_positionY, me->GetPositionZ(), true);

            if (me->GetDistance2d(-1769.0f, -1917.0f) > 95.0f)
            {
                pos.m_positionX = -1769.0f;
                pos.m_positionY = -1917.0f;
                pos.m_positionZ = me->GetPositionZ();

                DoCast(me, SPELL_BLACK_BLOOD_E);
            }

            me->GetMotionMaster()->MovePoint(1, pos);

            events.RescheduleEvent(EVENT_MOVE, 1.5*IN_MILLISECONDS);
        }

        void MoveInLineOfSight(Unit* who)
        {
            if (who && me->IsWithinDistInMap(who, 3.0f))
            {
                if (!delay && who->GetTypeId() == TYPEID_PLAYER)
                {
                    // Sphere
                    Position S;
                    me->GetPosition(&S);
                    // Player
                    Position P;
                    who->GetPosition(&P);

                    // get normal of P
                    P.m_orientation = Position::NormalizeOrientation(P.m_orientation - M_PI/2);

                    // move S into linear coords where P = 0
                    S.m_positionX -= P.m_positionX;
                    S.m_positionY -= P.m_positionY;

                    // rotate S to where P[o] = 0
                    float sin_ = sin(P.m_orientation);
                    float cos_ = cos(P.m_orientation);
                    //auto newX = S.m_positionX*sin_ + S.m_positionY*cos_;
                    float newY = S.m_positionX*cos_ - S.m_positionY*sin_;
                    //S.m_positionX = newX;
                    S.m_positionY = newY;
                    S.m_orientation = Position::NormalizeOrientation(S.m_orientation - P.m_orientation);

                    // move S[o] into [-PI,+PI]
                    S.m_orientation -= M_PI;

                    // check if we're on the right side of surface
                    if ((S.m_orientation > 0.0f) == (S.m_positionY > 0.0f))
                        return;

                    // perform the reflection
                    S.m_orientation = -S.m_orientation;

                    // move S[o] back into [0,2PI]
                    S.m_orientation += M_PI;

                    // rotate S back
                    S.m_orientation = Position::NormalizeOrientation(S.m_orientation + P.m_orientation);

                    me->SetOrientation(S.m_orientation);
                    UpdateMovement();

                    DoCast(who, SPELL_VOID_DIFFUSION, true);

                    delay = true;
                    events.ScheduleEvent(EVENT_DELAY, 5000);
                }
                else if (who->GetGUID() == me->ToTempSummon()->GetSummoner()->GetGUID())
                {
                    if (AuraPtr aura = me->GetAura(SPELL_VOID_DIFFUSION_2))
                        who->GetAI()->DoAction(aura->GetStackAmount());
                    else
                        who->GetAI()->DoAction(1);
                    me->DespawnOrUnsummon();
                }
            }
        }

        void UpdateAI(const uint32 diff) 
        {
            events.Update(diff);

            switch (events.ExecuteEvent())
            {
                case EVENT_MOVE:
                {
                    UpdateMovement();
                    events.ScheduleEvent(EVENT_MOVE, 2000);
                        break;
                }
                case EVENT_DELAY:
                {
                    if (delay)
                        delay = false;
                    break;
                }
            }
        }
    };
};

enum eGorath
{
    EVENT_CAST          = 1,
    EVENT_CAST2         = 2,
    
    SPELL_SHADOW_GAZE   = 109391,
    SPELL_WILD_FLAIL    = 109199,
    SPELL_OOZE_SPIT     = 109396,
    SPELL_SLUDGE_SPEW   = 110297,

    NPC_EYE_OF_GORATH   = 55416,
    NPC_FLAIL_OF_GORATH = 55417,
    NPC_CLAW_OF_GORATH  = 55418,
};

class mobs_of_gorath : public CreatureScript
{
public:
    mobs_of_gorath() : CreatureScript("mobs_of_gorath") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new mobs_of_gorathAI (creature);
    }

    struct mobs_of_gorathAI : public ScriptedAI
    {
        mobs_of_gorathAI(Creature* c) : ScriptedAI(c) { }

        EventMap events;

        void Reset() 
        {
            events.Reset();
            events.ScheduleEvent(EVENT_CAST, 5*IN_MILLISECONDS);
            if(me->GetEntry() == NPC_FLAIL_OF_GORATH)
                events.ScheduleEvent(EVENT_CAST2, 1500);
            DoZoneInCombat();
        }

        void UpdateAI(const uint32 diff) 
        {
            events.Update(diff);

            switch (events.ExecuteEvent())
            {
                case EVENT_CAST:
                {
                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0))
                    {
                        switch (me->GetEntry())
                        {
                            case NPC_EYE_OF_GORATH:
                                DoCast(target, SPELL_SHADOW_GAZE);
                                break;
                            case NPC_FLAIL_OF_GORATH:
                                DoCast(target, SPELL_WILD_FLAIL);
                                break;
                            case NPC_CLAW_OF_GORATH:
                                if(Unit* victim = me->getVictim())
                                    if(me->GetDistance2d(victim) > 4.0f)
                                        DoCast(victim, SPELL_OOZE_SPIT);
                                break;
                        }
                    }
                    else
                    {
                        switch (me->GetEntry())
                        {
                            case NPC_CLAW_OF_GORATH:
                                if(Unit* victim = me->getVictim())
                                    if(me->GetDistance2d(victim) > 4.0f)
                                        DoCast(victim, SPELL_OOZE_SPIT);
                                break;
                        }
                    }
                    events.ScheduleEvent(EVENT_CAST, 5*IN_MILLISECONDS);
                    break;
                }
                case EVENT_CAST2:
                {
                    if(Unit* victim = me->getVictim())
                        DoCast(victim, SPELL_SLUDGE_SPEW);
                    events.ScheduleEvent(EVENT_CAST2, 1500);
                    break;
                }
            }
        }
    };
};

class spell_zonozz_disrupting_shadows : public SpellScriptLoader
{
    public:
        spell_zonozz_disrupting_shadows() : SpellScriptLoader("spell_zonozz_disrupting_shadows") { }

        class spell_zonozz_disrupting_shadows_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_zonozz_disrupting_shadows_SpellScript);

            void FilterTargets(std::list<WorldObject*>& unitList)
            {
                Unit* caster = GetCaster();
                if(caster)
                {
                    Unit* victim = caster->getVictim();
                    uint8 playerCount = caster->GetMap()->Is25ManRaid() ? 7 : 3;
                    for (std::list<WorldObject*>::iterator itr = unitList.begin() ; itr != unitList.end();)
                    {
                        if (victim && victim->GetGUID() != (*itr)->GetGUID())
                            itr++;
                        else
                            itr = unitList.erase(itr);
                    }
                    if (unitList.size() > playerCount)
                        Trinity::Containers::RandomResizeList(unitList, playerCount);
                }
            }

            void Register()
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_zonozz_disrupting_shadows_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENEMY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_zonozz_disrupting_shadows_SpellScript();
        }
};

void AddSC_boss_warlord_zonozz()
{
    new boss_warlord_zonozz();
    new npc_void_of_the_unmaking();
    new mobs_of_gorath();
    new spell_zonozz_disrupting_shadows();
}
