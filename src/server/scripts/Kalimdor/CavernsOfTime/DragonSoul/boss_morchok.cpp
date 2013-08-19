#include "ScriptPCH.h"
#include "dragon_soul.h"

enum ScriptedTexts
{
    SAY_AGGRO       = 0,
    SAY_DEATH       = 1,
    SAY_GROUND1     = 6,
    SAY_GROUND2     = 7,
    SAY_CRYSTAL     = 9,
    SAY_KILL        = 10,
    SAY_KOHCROM     = 11,
};

enum Spells
{
    SPELL_BERSERK                           = 47008,
    SPELL_STOMP                             = 103414,
    SPELL_CRUSH_ARMOR                       = 103687,
    SPELL_RESONATING_CRYSTAL                = 103640,
    SPELL_RESONATING_CRYSTAL_SUMMON         = 103639,
    SPELL_RESONATING_CRYSTAL_DMG            = 103545,
    SPELL_RESONATING_CRYSTAL_AURA           = 103494,
    SPELL_TARGET_SELECTION                  = 103528,
    SPELL_DANGER                            = 103534,
    SPELL_WARNING                           = 103536,
    SPELL_SAFE                              = 103541,
    SPELL_FURIOUS                           = 103846,

    SPELL_BLACK_BLOOD_OF_THE_EARTH_DUMMY    = 103180,
    SPELL_BLACK_BLOOD_OF_THE_EARTH          = 103851,
    SPELL_BLACK_BLOOD_OF_THE_EARTH_DMG      = 103785,

    SPELL_EARTHEN_VORTEX                    = 103821,
    SPELL_EARTHEN_VORTEX_SUMMON             = 104512,
    SPELL_EARTHEN_VORTEX_VEHICLE            = 109615,

    SPELL_SUMMON_KOHCROM                    = 109017,
    SPELL_MORCHOK_JUMP                      = 109070,

    SPELL_MORCHOK_SIEGE_MISSILE_1           = 107541,
    SPELL_MORCHOK_SIEGE_MISSILE_2           = 110307,

    SPELL_EARTHS_VENGEANCE                  = 103176,
    SPELL_EARTHS_VENGEANCE_MISSILE          = 103177,
    SPELL_EARTHS_VENGEANCE_DMG              = 103178,
};

enum Events
{
    EVENT_EARTHS_VENGEANCE      = 1,
    EVENT_BLACK_BLOOD           = 2,
    EVENT_RESONATING_CRYSTAL    = 3,
    EVENT_BERSERK               = 4,
    EVENT_EARTHEN_VORTEX        = 5,
    EVENT_CONTINUE              = 6,
    EVENT_STOMP                 = 7,
    EVENT_CRUSH_ARMOR           = 8,
    EVENT_EXPLODE               = 9,
    EVENT_CHECK_PLAYERS         = 10,
    EVENT_CONTINUE_1            = 11,
    EVENT_UPDATE_HEALTH         = 12,
}; 

enum Adds
{
    NPC_EARTHEN_VORTEX      = 55723,
    NPC_RESONATING_CRYSTAL  = 55346,
};

enum Actions
{
    ACTION_KOHCROM_STOMP                = 1,
    ACTION_KOHCROM_RESONATING_CRYSTAL   = 2,
    ACTION_KOHCROM_EARTHEN_VORTEX       = 3,
};
enum MiscData
{
    DATA_GUID_1         = 1,
    DATA_GUID_2         = 2,
    DATA_KOHCROM_DONE   = 3,
};

class boss_morchok: public CreatureScript
{
    public:
        boss_morchok() : CreatureScript("boss_morchok") { }

        CreatureAI* GetAI(Creature* pCreature) const
        {
            return new boss_morchokAI(pCreature);
        }

        struct boss_morchokAI : public BossAI
        {
            boss_morchokAI(Creature* pCreature) : BossAI(pCreature, DATA_MORCHOK)
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

            void Reset()
            {
                _Reset();
                
                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_DANGER);
                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_WARNING);
                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_SAFE);

                me->SetReactState(REACT_AGGRESSIVE);

                me->LowerPlayerDamageReq(me->GetHealth());

                _stompguid1 = 0;
                _stompguid2 = 0;
                bEnrage = false;
                bKohcrom = false;
                bAchieve = true;
                bFirstStomp = false;
                bFirstCrystal = false;
            }

            void EnterCombat(Unit* who)
            {
                Talk(SAY_AGGRO);

                events.ScheduleEvent(EVENT_STOMP, urand(12000, 14000));
                events.ScheduleEvent(EVENT_EARTHEN_VORTEX, urand(56000, 60000));
                events.ScheduleEvent(EVENT_RESONATING_CRYSTAL, urand(19000, 20000));
                events.ScheduleEvent(EVENT_BERSERK, 7 * MINUTE * IN_MILLISECONDS);
                if (!IsHeroic())
                    events.ScheduleEvent(EVENT_CRUSH_ARMOR, urand(6000, 12000));

                _stompguid1 = 0;
                _stompguid2 = 0;
                bEnrage = false;
                bKohcrom = false;
                bAchieve = true;
                bFirstStomp = false;
                bFirstCrystal = false;

                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_DANGER);
                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_WARNING);
                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_SAFE);

                DoZoneInCombat();
                instance->SetBossState(DATA_MORCHOK, IN_PROGRESS);
            }

            void JustDied(Unit* /*killer*/)
            {
                _JustDied();

                Talk(SAY_DEATH);

                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_DANGER);
                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_WARNING);
                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_SAFE);
            }

            void SetGUID(uint64 guid, int32 type)
            {
                if (type == DATA_GUID_1)
                    _stompguid1 = guid;
                else if (type == DATA_GUID_2)
                    _stompguid2 = guid;
            }

            uint64 GetGUID(int32 type)
            {
                if (type == DATA_GUID_1)
                    return _stompguid1;
                else if (type == DATA_GUID_2)
                    return _stompguid2;

                return 0;
            }

            void JustSummoned(Creature* summon)
            {
                BossAI::JustSummoned(summon);
                if (summon->GetEntry() == NPC_KOHCROM)
                {
                    summon->SetMaxHealth(me->GetMaxHealth());
                    summon->SetHealth(me->GetHealth());
                }
            }
            void KilledUnit(Unit* victim)
            {
                if (victim && victim->GetTypeId() == TYPEID_PLAYER)
                    Talk(SAY_KILL);
            }

            bool AllowAchieve()
            {
                return bAchieve;
            }

            uint32 GetData(uint32 type)
            {
                if (type == DATA_KOHCROM_DONE)
                    return uint32(bKohcrom);

                return 0;
            }

            void UpdateAI(const uint32 diff)
            {
                if (!UpdateVictim())
                        return;

                if (me->GetDistance(me->GetHomePosition()) > 500.0f)
                {
                    EnterEvadeMode();
                    return;
                }

                events.Update(diff);

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                if (IsHeroic() && me->HealthBelowPct(90) && !bKohcrom)
                {
                    bKohcrom = true;
                    me->SetReactState(REACT_PASSIVE);
                    me->AttackStop();
                    
                    Talk(SAY_KOHCROM);

                    DoCast(me, SPELL_MORCHOK_JUMP);
                    DoCast(me, SPELL_SUMMON_KOHCROM, true);

                    bFirstCrystal = true;

                    events.DelayEvents(10000);
                    events.ScheduleEvent(EVENT_CONTINUE, 5000);
                    events.ScheduleEvent(EVENT_UPDATE_HEALTH, 8000);
                    return;
                }

                if (me->HealthBelowPct(20) && !bEnrage)
                {
                    bEnrage = true;
                    DoCast(me, SPELL_FURIOUS);
                    return;
                }

                if (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_UPDATE_HEALTH:
                            if (me->isAlive())
                            {
                                if (Creature* pKohcrom = ObjectAccessor::GetCreature(*me, instance->GetData64(DATA_KOHCROM)))
                                {
                                    if (!pKohcrom->isAlive())
                                        break;

                                    if (me->GetHealth() < 500000 || pKohcrom->GetHealth() < 500000)
                                        break;

                                    uint32 new_health = (me->GetHealth() + pKohcrom->GetHealth()) / 2;
                                    std::min(new_health, me->GetMaxHealth());
                                    me->SetHealth(new_health);
                                    pKohcrom->SetHealth(new_health);
                                }
                                events.ScheduleEvent(EVENT_UPDATE_HEALTH, 3000);
                            }
                            
                            break;
                        case EVENT_BERSERK:
                            DoCast(me, SPELL_BERSERK);
                            break;
                        case EVENT_CRUSH_ARMOR:
                            DoCastVictim(SPELL_CRUSH_ARMOR);
                            events.ScheduleEvent(EVENT_CRUSH_ARMOR, urand(12000, 15000));
                            break;
                        case EVENT_STOMP:
                        {
                            _stompguid1 = 0;
                            _stompguid2 = 0;

                            int32 tim = int32(events.GetNextEventTime(EVENT_EARTHEN_VORTEX)) - int32(events.GetTimer());
                            if (tim <= 7000)
                                break;

                            DoCast(me, SPELL_STOMP);
                            events.ScheduleEvent(EVENT_STOMP, urand(12000, 14000));
                            if (bKohcrom && !bFirstStomp)
                            {
                                if (Creature* pKohcrom = ObjectAccessor::GetCreature(*me, instance->GetData64(DATA_KOHCROM)))
                                    pKohcrom->AI()->DoAction(ACTION_KOHCROM_STOMP);
                            }
                            bFirstStomp = false;
                            break;
                        }
                        case EVENT_RESONATING_CRYSTAL:
                        {
                            int32 tim = int32(events.GetNextEventTime(EVENT_EARTHEN_VORTEX)) - int32(events.GetTimer());
                            if (tim <= 17000)
                                break;
                            
                            Talk(SAY_CRYSTAL);
                            Position pos;
                            me->GetNearPosition(pos, frand(25.0f, 45.0f), frand(0, 2 * M_PI));
                            me->CastSpell(pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), SPELL_RESONATING_CRYSTAL, true);
                            events.ScheduleEvent(EVENT_RESONATING_CRYSTAL, urand(12000, 14000));
                            if (bKohcrom && !bFirstCrystal)
                            {
                                if (Creature* pKohcrom = ObjectAccessor::GetCreature(*me, instance->GetData64(DATA_KOHCROM)))
                                    pKohcrom->AI()->DoAction(ACTION_KOHCROM_RESONATING_CRYSTAL);
                            }
                            bFirstCrystal = false;
                            break;
                        }
                        case EVENT_EARTHEN_VORTEX:
                            Talk(SAY_GROUND1);
                            me->SetReactState(REACT_PASSIVE);
                            me->AttackStop();
                            
                            events.CancelEvent(EVENT_RESONATING_CRYSTAL);
                            events.CancelEvent(EVENT_STOMP);
                            events.CancelEvent(EVENT_CRUSH_ARMOR);

                            DoCastAOE(SPELL_EARTHEN_VORTEX, true);
                            DoCast(me, SPELL_EARTHS_VENGEANCE);
                            
                            if (bKohcrom)
                                if (Creature* pKohcrom = ObjectAccessor::GetCreature(*me, instance->GetData64(DATA_KOHCROM)))
                                    pKohcrom->AI()->DoAction(ACTION_KOHCROM_EARTHEN_VORTEX);

                            events.ScheduleEvent(EVENT_BLACK_BLOOD, 5000);
                            events.ScheduleEvent(EVENT_EARTHEN_VORTEX, urand(94000, 97000));
                            break;
                        case EVENT_BLACK_BLOOD:
                            Talk(SAY_GROUND2);
                            DoCast(me, SPELL_BLACK_BLOOD_OF_THE_EARTH);
                            events.ScheduleEvent(EVENT_CONTINUE_1, 18000);
                            break;
                        case EVENT_CONTINUE:
                            me->SetReactState(REACT_AGGRESSIVE);
                            AttackStart(me->getVictim());
                            break;
                        case EVENT_CONTINUE_1:
                            me->SetReactState(REACT_AGGRESSIVE);
                            AttackStart(me->getVictim());
                            bFirstCrystal = true;
                            events.ScheduleEvent(EVENT_STOMP, 18000);
                            events.ScheduleEvent(EVENT_RESONATING_CRYSTAL, 25000);
                            if (!IsHeroic())
                                events.ScheduleEvent(EVENT_CRUSH_ARMOR, urand(6000, 12000));
                            break;
                        default:
                            break;
                    }
                }

                DoMeleeAttackIfReady();
            }
        private:

            uint64 _stompguid1;
            uint64 _stompguid2;
            bool bEnrage;
            bool bKohcrom;
            bool bAchieve;
            bool bFirstStomp;
            bool bFirstCrystal;
        };
};

class npc_morchok_kohcrom: public CreatureScript
{
    public:
        npc_morchok_kohcrom() : CreatureScript("npc_morchok_kohcrom") { }

        CreatureAI* GetAI(Creature* pCreature) const
        {
            return new npc_morchok_kohcromAI(pCreature);
        }

        struct npc_morchok_kohcromAI : public ScriptedAI
        {
            npc_morchok_kohcromAI(Creature* pCreature) : ScriptedAI(pCreature), summons(me)
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
                me->SetReactState(REACT_PASSIVE);
                pInstance = me->GetInstanceScript();
            }

            void Reset()
            {
                events.Reset();
                summons.DespawnAll();

                _stompguid1 = 0;
                _stompguid2 = 0;
                bEnrage = false;
            }

            void SetGUID(uint64 guid, int32 type)
            {
                if (type == DATA_GUID_1)
                    _stompguid1 = guid;
                else if (type == DATA_GUID_2)
                    _stompguid2 = guid;
            }

            uint64 GetGUID(int32 type)
            {
                if (type == DATA_GUID_1)
                    return _stompguid1;
                else if (type == DATA_GUID_2)
                    return _stompguid2;

                return 0;
            }

            void IsSummonedBy(Unit* /*owner*/)
            {
                DoCast(me, SPELL_MORCHOK_JUMP, true);
                events.ScheduleEvent(EVENT_CONTINUE, 5000);
            }

            void DoAction(const int32 action)
            {
                switch (action)
                {
                    case ACTION_KOHCROM_STOMP:
                        events.ScheduleEvent(EVENT_STOMP, urand(4000, 6000));
                        break;
                    case ACTION_KOHCROM_RESONATING_CRYSTAL:
                        events.ScheduleEvent(EVENT_RESONATING_CRYSTAL, urand(4000, 6000));
                        break;
                    case ACTION_KOHCROM_EARTHEN_VORTEX:
                        events.ScheduleEvent(EVENT_EARTHEN_VORTEX, 1);
                        break;
                }
            }

            void JustSummoned(Creature* summon)
            {
                summons.Summon(summon);
                if (me->isInCombat())
                    DoZoneInCombat(summon);
            }
            
            void SummonedCreatureDespawn(Creature* summon)
            {
                summons.Despawn(summon);
            }

            void UpdateAI(const uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                events.Update(diff);

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                if (me->HealthBelowPct(20) && !bEnrage)
                {
                    bEnrage = true;
                    DoCast(me, SPELL_FURIOUS);
                    return;
                }

                if (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_STOMP:
                            _stompguid1 = 0;
                            _stompguid2 = 0;
                            DoCast(me, SPELL_STOMP);
                            break;
                        case EVENT_RESONATING_CRYSTAL:
                        {
                            Position pos;
                            me->GetNearPosition(pos, frand(25.0f, 45.0f), frand(0, 2 * M_PI));
                            me->CastSpell(pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), SPELL_RESONATING_CRYSTAL, true);
                            break;
                        }
                        case EVENT_EARTHEN_VORTEX:
                            me->SetReactState(REACT_PASSIVE);
                            me->AttackStop();
                            DoCastAOE(SPELL_EARTHEN_VORTEX, true);
                            DoCast(me, SPELL_EARTHS_VENGEANCE);
                            events.ScheduleEvent(EVENT_BLACK_BLOOD, 5000);
                            break;
                        case EVENT_BLACK_BLOOD:
                            DoCast(me, SPELL_BLACK_BLOOD_OF_THE_EARTH);
                            events.ScheduleEvent(EVENT_CONTINUE, 18000);
                            break;
                        case EVENT_CONTINUE:
                            me->SetReactState(REACT_AGGRESSIVE);
                            AttackStart(me->getVictim());
                            break;
                        default:
                            break;
                    }
                }

                DoMeleeAttackIfReady();
            }
        private:
            
            EventMap events;
            SummonList summons;
            InstanceScript* pInstance;
            uint64 _stompguid1;
            uint64 _stompguid2;
            bool bEnrage;
        };
};

class npc_morchok_resonating_crystal : public CreatureScript
{
    public:
        npc_morchok_resonating_crystal() : CreatureScript("npc_morchok_resonating_crystal") { }

        CreatureAI* GetAI(Creature* pCreature) const
        {
            return new npc_morchok_resonating_crystalAI(pCreature);
        }

        struct npc_morchok_resonating_crystalAI : public Scripted_NoMovementAI
        {
            npc_morchok_resonating_crystalAI(Creature* pCreature) : Scripted_NoMovementAI(pCreature)
            {
                me->SetReactState(REACT_PASSIVE);
            }

            EventMap events; 

            void Reset()
            {
            }

            void IsSummonedBy(Unit* /*owner*/)
            {
                events.ScheduleEvent(EVENT_EXPLODE, 10000);
            }

            void UpdateAI(const uint32 diff)
            {
                events.Update(diff);

                if (uint32 eventId = events.ExecuteEvent())
                {
                    if (eventId == EVENT_EXPLODE)
                    {
                        me->RemoveAura(SPELL_RESONATING_CRYSTAL);
                        DoCastAOE(SPELL_RESONATING_CRYSTAL_DMG);
                        me->DespawnOrUnsummon(500);
                    }
                }                
            }
        };
};

        
class spell_morchok_target_selected : public SpellScriptLoader
{ 
    public:
        spell_morchok_target_selected() : SpellScriptLoader("spell_morchok_target_selected") { }

        class spell_morchok_target_selected_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_morchok_target_selected_SpellScript);

            void FilterTargets(std::list<WorldObject*>& targets)
            {
                if (targets.empty())
                    return;
                
                uint32 max_size = (GetCaster()->GetMap()->Is25ManRaid() ? 7 : 3);

                if (targets.size() > max_size)
                {
                    targets.sort(DistanceOrderPred(GetCaster()));
                }

                max_size = std::min((uint32)targets.size(), max_size);
                
                uint32 count = 0;
                for (std::list<WorldObject*>::const_iterator itr = targets.begin(); itr != targets.end(); ++itr)
                {
                    temp_targets.push_back((*itr));
                    count++;
                    if (count >= max_size)
                        break;
                }
            }

            void HandleScript(SpellEffIndex /*effIndex*/)
            {
                if (!GetCaster() || !GetHitUnit())
                    return;

                bool bFound = false;
                for (std::vector<WorldObject*>::const_iterator itr = temp_targets.begin(); itr != temp_targets.end(); ++itr)
                    if ((*itr)->GetGUID() == GetHitUnit()->GetGUID())
                    {
                        bFound = true;
                        break;
                    }

                if (!bFound)
                {
                    GetHitUnit()->RemoveAura(SPELL_DANGER);
                    GetHitUnit()->RemoveAura(SPELL_WARNING);
                    GetHitUnit()->RemoveAura(SPELL_SAFE);
                }
                else
                {
                    float dist = GetCaster()->GetDistance(GetHitUnit());
                    if (dist > 30.0f) 
                        GetCaster()->CastSpell(GetHitUnit(), SPELL_DANGER, true);
                    else if (dist > 15.0f) 
                        GetCaster()->CastSpell(GetHitUnit(), SPELL_WARNING, true);
                    else 
                        GetCaster()->CastSpell(GetHitUnit(), SPELL_SAFE, true);
                }
            }
        
            void Register()
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_morchok_target_selected_SpellScript::FilterTargets, EFFECT_1, TARGET_UNIT_DEST_AREA_ENEMY);
                OnEffectHitTarget += SpellEffectFn(spell_morchok_target_selected_SpellScript::HandleScript, EFFECT_1, SPELL_EFFECT_DUMMY);
            }

        private:
            
            std::vector<WorldObject*> temp_targets;

            class DistanceOrderPred
            {
                public:
                    DistanceOrderPred(Unit* searcher) : _searcher(searcher) { }
                    bool operator() (WorldObject* a, WorldObject* b) const
                    {
                        float rA = _searcher->GetDistance(a);
                        float rB = _searcher->GetDistance(b);
                        return rA < rB;
                    }
                private:
                    Unit* _searcher;
            };
        };
       
        SpellScript* GetSpellScript() const
        {
            return new spell_morchok_target_selected_SpellScript();
        }

};

class spell_morchok_resonating_crystal_dmg : public SpellScriptLoader
{
   
    public:
        spell_morchok_resonating_crystal_dmg() : SpellScriptLoader("spell_morchok_resonating_crystal_dmg") { }

        class spell_morchok_resonating_crystal_dmg_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_morchok_resonating_crystal_dmg_SpellScript);

            void FilterTargets(std::list<WorldObject*>& targets)
            {
                if (targets.empty())
                    return;

                targets.remove_if(AurasCheck());
            }

            void Register()
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_morchok_resonating_crystal_dmg_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_DEST_AREA_ENEMY);
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_morchok_resonating_crystal_dmg_SpellScript::FilterTargets, EFFECT_1, TARGET_UNIT_DEST_AREA_ENEMY);
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_morchok_resonating_crystal_dmg_SpellScript::FilterTargets, EFFECT_2, TARGET_UNIT_DEST_AREA_ENEMY);
            }

        private:

            class AurasCheck
            {
                public:
                    AurasCheck() {}
            
                    bool operator()(WorldObject* unit)
                    {
                        return (!unit->ToUnit() || !(unit->ToUnit()->HasAura(SPELL_DANGER) || unit->ToUnit()->HasAura(SPELL_WARNING) || unit->ToUnit()->HasAura(SPELL_SAFE)));
                    }
            };
        };
             
        SpellScript* GetSpellScript() const
        {
            return new spell_morchok_resonating_crystal_dmg_SpellScript();
        }
};

class spell_morchok_black_blood_of_the_earth : public SpellScriptLoader
{
    public:
        spell_morchok_black_blood_of_the_earth() : SpellScriptLoader("spell_morchok_black_blood_of_the_earth") { }

        class spell_morchok_black_blood_of_the_earth_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_morchok_black_blood_of_the_earth_AuraScript);

            void HandlePeriodicTick(AuraEffect const* aurEff)
            {
                float ticks = aurEff->GetTickNumber();
                int counter = (int)floor(ticks / 5) + 1;

                if(Unit* me = GetCaster())
                if(ticks == 1 || ticks == 5 || ticks == 10 || ticks == 15)
                {
                    float _angle;
                    float radius = 10.0f;
                    Position _pos;
                    std::list<GameObject*> fallingfragments;
                    GetGameObjectListWithEntryInGrid(fallingfragments, me, GO_INNER_WALL, 60.0f);
                    for (int i = 0; i < counter; i++)
                    {
                        //blackblood 1
                        _angle = me->GetOrientation() + M_PI/4;
                        me->GetNearPosition(_pos, radius, _angle);
                        if(BlackBloodPositionSelect(fallingfragments, _pos.GetPositionX(), _pos.GetPositionY(), me))
                            me->CastSpell(_pos.GetPositionX(), _pos.GetPositionY(), _pos.GetPositionZ(), SPELL_BLACK_BLOOD_OF_THE_EARTH_DUMMY, true);
                        //blackblood 2
                        _angle = me->GetOrientation() + M_PI/2;
                        me->GetNearPosition(_pos, radius, _angle);
                        if(BlackBloodPositionSelect(fallingfragments, _pos.GetPositionX(), _pos.GetPositionY(), me))
                            me->CastSpell(_pos.GetPositionX(), _pos.GetPositionY(), _pos.GetPositionZ(), SPELL_BLACK_BLOOD_OF_THE_EARTH_DUMMY, true);
                        //blackblood 3
                        _angle = me->GetOrientation() + (3*M_PI)/4;
                        me->GetNearPosition(_pos, radius, _angle);
                        if(BlackBloodPositionSelect(fallingfragments, _pos.GetPositionX(), _pos.GetPositionY(), me))
                            me->CastSpell(_pos.GetPositionX(), _pos.GetPositionY(), _pos.GetPositionZ(), SPELL_BLACK_BLOOD_OF_THE_EARTH_DUMMY, true);
                        //blackblood 4
                        _angle = me->GetOrientation() + M_PI;
                        me->GetNearPosition(_pos, radius, _angle);
                        if(BlackBloodPositionSelect(fallingfragments, _pos.GetPositionX(), _pos.GetPositionY(), me))
                            me->CastSpell(_pos.GetPositionX(), _pos.GetPositionY(), _pos.GetPositionZ(), SPELL_BLACK_BLOOD_OF_THE_EARTH_DUMMY, true);
                        //blackblood 5
                        _angle = me->GetOrientation() + M_PI/4 + M_PI;
                        me->GetNearPosition(_pos, radius, _angle);
                        if(BlackBloodPositionSelect(fallingfragments, _pos.GetPositionX(), _pos.GetPositionY(), me))
                            me->CastSpell(_pos.GetPositionX(), _pos.GetPositionY(), _pos.GetPositionZ(), SPELL_BLACK_BLOOD_OF_THE_EARTH_DUMMY, true);
                        //blackblood 6
                        _angle = me->GetOrientation() + M_PI/2 + M_PI;
                        me->GetNearPosition(_pos, radius, _angle);
                        if(BlackBloodPositionSelect(fallingfragments, _pos.GetPositionX(), _pos.GetPositionY(), me))
                            me->CastSpell(_pos.GetPositionX(), _pos.GetPositionY(), _pos.GetPositionZ(), SPELL_BLACK_BLOOD_OF_THE_EARTH_DUMMY, true);
                        //blackblood 7
                        _angle = me->GetOrientation() + (3*M_PI)/4 + M_PI;
                        me->GetNearPosition(_pos, radius, _angle);
                        if(BlackBloodPositionSelect(fallingfragments, _pos.GetPositionX(), _pos.GetPositionY(), me))
                            me->CastSpell(_pos.GetPositionX(), _pos.GetPositionY(), _pos.GetPositionZ(), SPELL_BLACK_BLOOD_OF_THE_EARTH_DUMMY, true);
                        //blackblood 8
                        _angle = me->GetOrientation() + M_PI*2;
                        me->GetNearPosition(_pos, radius, _angle);
                        if(BlackBloodPositionSelect(fallingfragments, _pos.GetPositionX(), _pos.GetPositionY(), me))
                            me->CastSpell(_pos.GetPositionX(), _pos.GetPositionY(), _pos.GetPositionZ(), SPELL_BLACK_BLOOD_OF_THE_EARTH_DUMMY, true);
                        radius += 10.0f;
                    }
                }
            }

            bool BlackBloodPositionSelect(std::list<GameObject*> const& collisionList, float x, float y, Unit* me)
            {
                for (std::list<GameObject*>::const_iterator itr = collisionList.begin(); itr != collisionList.end(); ++itr)
                    if ((*itr)->IsInBetween(me, x, y, 5.0f))
                        return false;

                return true;
            }

            void Register()
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_morchok_black_blood_of_the_earth_AuraScript::HandlePeriodicTick, EFFECT_0, SPELL_AURA_PERIODIC_TRIGGER_SPELL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_morchok_black_blood_of_the_earth_AuraScript();
        }
};

class FallingFragmentTargetSelector
{
    public:
        FallingFragmentTargetSelector(Unit* caster, std::list<GameObject*> const& collisionList) : _caster(caster), _collisionList(collisionList) { }

        bool operator()(WorldObject* unit)
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

class spell_morchok_black_blood_of_the_earth_dmg : public SpellScriptLoader
{
   
    public:
        spell_morchok_black_blood_of_the_earth_dmg() : SpellScriptLoader("spell_morchok_black_blood_of_the_earth_dmg") { }

        class spell_morchok_black_blood_of_the_earth_dmg_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_morchok_black_blood_of_the_earth_dmg_SpellScript);

            void FilterTargets(std::list<WorldObject*>& unitList)
            {
                if (!GetCaster())
                    return;

                if (unitList.empty())
                    return;

                if (AuraEffect const* aurEff = GetCaster()->GetAuraEffect(SPELL_BLACK_BLOOD_OF_THE_EARTH, EFFECT_0))
                {
                    float ticks = aurEff->GetTickNumber();
                    int counter = (int)floor(ticks / 5) + 1;
                    unitList.remove_if(DistanceCheck(GetCaster(), float(counter * 15)));
                }

                std::list<GameObject*> fallingfragments;
                GetGameObjectListWithEntryInGrid(fallingfragments, GetCaster(), GO_INNER_WALL, 200.0f);
                unitList.remove_if (FallingFragmentTargetSelector(GetCaster(), fallingfragments));
            }

            void Register()
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_morchok_black_blood_of_the_earth_dmg_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_DEST_AREA_ENEMY);
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_morchok_black_blood_of_the_earth_dmg_SpellScript::FilterTargets, EFFECT_1, TARGET_UNIT_DEST_AREA_ENEMY);
            }

        private:
            class DistanceCheck
            {
                public:
                    DistanceCheck(Unit* searcher, float distance) : _searcher(searcher), _distance(distance) {}

                    bool operator()(WorldObject* unit)
                    {
                        return (_searcher->GetDistance2d(unit) > _distance);
                    }

                private:
                    Unit* _searcher;
                    float _distance;
            };
        };
             
        SpellScript* GetSpellScript() const
        {
            return new spell_morchok_black_blood_of_the_earth_dmg_SpellScript();
        }
};

class spell_morchok_stomp : public SpellScriptLoader
{
    public:
        spell_morchok_stomp() : SpellScriptLoader("spell_morchok_stomp") { }

        class spell_morchok_stomp_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_morchok_stomp_SpellScript);

            void FilterTargets(std::list<WorldObject*>& targets)
            {
                if (!GetCaster())
                    return;

                if (targets.empty())
                    return;

                if (Creature* pMorchok = GetCaster()->ToCreature())
                {
                    targets.sort(DistanceOrderPred(GetCaster()));
                    std::list<WorldObject*>::const_iterator itr = targets.begin();
                    pMorchok->AI()->SetGUID((*itr)->GetGUID(), DATA_GUID_1);
                    if (targets.size() > 1)
                    {
                        ++itr;
                        pMorchok->AI()->SetGUID((*itr)->GetGUID(), DATA_GUID_2);
                    }
                }
            }

            void Register()
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_morchok_stomp_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_DEST_AREA_ENEMY);
            }

        private:
            class DistanceOrderPred
            {
                public:
                    DistanceOrderPred(Unit* searcher) : _searcher(searcher) { }
                    bool operator() (WorldObject* a, WorldObject* b) const
                    {
                        float rA = _searcher->GetExactDist(a);
                        float rB = _searcher->GetExactDist(b);
                        return rA < rB;
                    }

                private:
                    Unit* _searcher;
            };
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_morchok_stomp_SpellScript();
        }
};

typedef boss_morchok::boss_morchokAI MorchokAI;

class achievement_dont_stay_so_close_to_me : public AchievementCriteriaScript
{
    public:
        achievement_dont_stay_so_close_to_me() : AchievementCriteriaScript("achievement_dont_stay_so_close_to_me") { }

        bool OnCheck(Player* source, Unit* target)
        {
            if (!target)
                return false;

            if (MorchokAI* morchokAI = CAST_AI(MorchokAI, target->GetAI()))
                return morchokAI->AllowAchieve();

            return false;
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

            void OnPeriodic(AuraEffect const* /*aurEff*/)
            {
                if(amount > 0)
                    amount -= 20;
                if (AuraEffect* effect = GetAura()->GetEffect(EFFECT_1))
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
    new npc_morchok_kohcrom();
    new npc_morchok_resonating_crystal();
    new spell_morchok_target_selected();
    new spell_morchok_resonating_crystal_dmg();
    new spell_morchok_black_blood_of_the_earth();
    new spell_morchok_black_blood_of_the_earth_dmg();
    new spell_morchok_stomp();
    new spell_morchok_resonating_crystal();
    new achievement_dont_stay_so_close_to_me();
}