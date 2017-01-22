/*
 * Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2006-2009 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "NewScriptPCH.h"
#include "throne_of_thunder.h"

enum eSpells
{
    //JiKun
    SPELL_CAW_SEARCHER            = 138923,
    SPELL_CAW_DMG                 = 138926,
    SPELL_QUILLS                  = 134380,
    SPELL_TALON_RAKE              = 134366,
    SPELL_INFECTED_TALONS         = 140092,
    SPELL_DOWN_DRAFT_AT           = 134370,
    SPELL_FEED_YOUNG              = 137528, //main spell

    //Fly buffs and spells
    SPELL_LESSON_OF_ICARUS        = 140571,
    SPELL_DAEDALIAN_WINGS         = 134339, //ovveride spells aura
    SPELL_FEATHER_AT              = 134338,
    SPELL_JUMP_TO_B_PLATFORM      = 138360,
    SPELL_JI_KUN_FEATHER_AURA     = 140014,
    SPELL_JI_KUN_FEATHER_USE_EF   = 140013,

    //Incubate
    SPELL_INCUBATE_ZONE           = 137526,
    SPELL_INCUBATE_TARGET_AURA    = 134347,

    //Feed for nest
    SPELL_PRIMAL_NUTRIMENT        = 140741, //buff if catch slimed
    SPELL_SLIMED_AT               = 134255, //feed fly AT (catch feed)
    SPELL_FOOD_FALL_VISUAL        = 140788, //visual feed, then fly
    SPELL_JUMPS_DOWN_TO_HATCHLING = 138904,
    SPELL_FEED_POOL_SPAWN_N       = 139284, //pool in nest visual

    //Feed for platform
    SPELL_FEED_POOL_SPAWN_P       = 138854, //pool on platform visual
    SPELL_FEED_POOL_DMG           = 138319,
    SPELL_JUMP_DOWN_TO_PLATFORM   = 140575,
    SPELL_SLIMED_DMG              = 134256,
    SPELL_SLIMED_DEBUFF           = 138309,

    //Young Hatchling
    SPELL_CHEEP_LOW               = 139296,
    SPELL_EAT_CHANNEL             = 134321,

    //Fallcatcher
    SPELL_SAFE_NET_TRIGGER        = 136524, //summon fallcatcher
    SPELL_GENTLE_YET_FIRM         = 139168,
    SPELL_CAST_FILTER             = 141062,
    SPELL_PARACHUTE               = 45472,
    SPELL_PARACHUTE_BUFF          = 44795,

    //Other
    SPELL_SPAWN_FEED              = 138918, //change color
};

enum eEvents
{
    EVENT_CAW                     = 1,
    EVENT_QUILLS                  = 2,
    EVENT_TALON_RAKE              = 3,
    EVENT_ACTIVE_NEST             = 4,
    EVENT_FALLCATCHER_IN_POS      = 5,
    EVENT_DOWN_DRAFT              = 6,
    EVENT_FEED_YOUNG              = 7,
    EVENT_FIND_PLAYER             = 8,
    EVENT_CHEEP                   = 10,
};

class boss_jikun : public CreatureScript
{
public:
    boss_jikun() : CreatureScript("boss_jikun") {}

    struct boss_jikunAI : public BossAI
    {
        boss_jikunAI(Creature* creature) : BossAI(creature, DATA_JI_KUN)
        {
            instance = creature->GetInstanceScript();
            //me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
        }
        InstanceScript* instance;

        void Reset()
        {
            _Reset();
            me->SetReactState(REACT_DEFENSIVE);
            //me->SetReactState(REACT_PASSIVE);
            instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_SLIMED_DEBUFF);
        }

        void EnterCombat(Unit* who)
        {
            _EnterCombat();
            //After pool active first nest, add feed 3 second timer
            //events.ScheduleEvent(EVENT_ACTIVE_NEST, 10000);
            events.ScheduleEvent(EVENT_CAW, 14000);
            events.ScheduleEvent(EVENT_TALON_RAKE, 20000);
            events.ScheduleEvent(EVENT_QUILLS, 42000);
            events.ScheduleEvent(EVENT_DOWN_DRAFT, 97000);
        }

        void JustDied(Unit* /*killer*/)
        {
            _JustDied();
            instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_SLIMED_DEBUFF);
        }

        void UpdateAI(uint32 diff)
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
                case EVENT_ACTIVE_NEST:
                    //instance->SetData(DATA_ACTIVE_NEXT_NEST, 0);
                    //events.ScheduleEvent(EVENT_FEED_YOUNG, 3000);
                    break;
                case EVENT_FEED_YOUNG:
                    DoCast(me, SPELL_FEED_YOUNG);
                    break;
                case EVENT_TALON_RAKE:
                    if (me->getVictim())
                    {
                        if (me->getVictim()->GetTypeId() == TYPEID_PLAYER)
                        {
                            uint8 pos = urand(0, 1);
                            switch (pos)
                            {
                            case 0:
                                DoCast(me->getVictim(), SPELL_TALON_RAKE);
                                break;
                            case 1:
                                DoCast(me->getVictim(), SPELL_INFECTED_TALONS);
                                break;
                            }
                        }
                    }
                    events.ScheduleEvent(EVENT_TALON_RAKE, 20000);
                    break;
                case EVENT_QUILLS:
                    DoCast(me, SPELL_QUILLS);
                    events.ScheduleEvent(EVENT_QUILLS, 62500);
                    break;
                case EVENT_CAW:
                    DoCast(me, SPELL_CAW_SEARCHER);
                    events.ScheduleEvent(EVENT_CAW, 20000);
                    break;
                case EVENT_DOWN_DRAFT:
                    DoCast(me, SPELL_DOWN_DRAFT_AT);
                    events.ScheduleEvent(EVENT_DOWN_DRAFT, 97000);
                    break;
                }
            }
            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new boss_jikunAI(creature);
    }
};

//69626
class npc_incubater : public CreatureScript
{
public:
    npc_incubater() : CreatureScript("npc_incubater") {}

    struct npc_incubaterAI : public ScriptedAI
    {
        npc_incubaterAI(Creature* creature) : ScriptedAI(creature), summon(me)
        {
            pInstance = creature->GetInstanceScript();
            me->SetDisplayId(11686);
            me->SetReactState(REACT_PASSIVE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_DISABLE_MOVE);
        }
        InstanceScript* pInstance;
        SummonList summon;

        void Reset(){}

        void JustSummoned(Creature* sum)
        {
            summon.Summon(sum);
        }

        void DamageTaken(Unit* attacker, uint32 &damage)
        {
            if (damage >= me->GetHealth())
                damage = 0;
        }

        void SetData(uint32 type, uint32 data)
        {
            switch (type)
            {
            case DATA_ACTIVE_NEST:
            {
                DoCast(me, SPELL_INCUBATE_ZONE, true);
                std::list<Creature*> egglist;
                egglist.clear();
                GetCreatureListWithEntryInGrid(egglist, me, NPC_YOUNG_EGG_OF_JIKUN, 20.0f);
                uint8 maxsize = me->GetMap()->Is25ManRaid() ? 5 : 4;
                if (egglist.size() > maxsize)
                    egglist.resize(maxsize);
                if (!egglist.empty())
                {
                    for (std::list<Creature*>::const_iterator itr = egglist.begin(); itr != egglist.end(); itr++)
                    {
                        (*itr)->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
                        (*itr)->CastSpell(*itr, SPELL_INCUBATE_TARGET_AURA, true);
                    }
                }
            }
            break;
            case DATA_RESET_NEST:
                summon.DespawnAll();
                me->RemoveAurasDueToSpell(SPELL_INCUBATE_ZONE);
                std::list<Creature*> egglist;
                egglist.clear();
                GetCreatureListWithEntryInGrid(egglist, me, NPC_YOUNG_EGG_OF_JIKUN, 20.0f);
                GetCreatureListWithEntryInGrid(egglist, me, NPC_MATURE_EGG_OF_JIKUN, 20.0f);

                if (!egglist.empty())
                {
                    for (std::list<Creature*>::const_iterator itr = egglist.begin(); itr != egglist.end(); itr++)
                    {
                        if (!(*itr)->isAlive())
                            (*itr)->Respawn();
                        else
                        {
                            (*itr)->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
                            (*itr)->RemoveAurasDueToSpell(SPELL_INCUBATE_TARGET_AURA);
                        }
                    }
                }

                std::list<GameObject*> _egglist;
                _egglist.clear();
                GetGameObjectListWithEntryInGrid(_egglist, me, GO_JIKUN_EGG, 20.0f);
                if (!_egglist.empty())
                    for (std::list<GameObject*>::const_iterator Itr = _egglist.begin(); Itr != _egglist.end(); Itr++)
                        (*Itr)->SetGoState(GO_STATE_READY);

                std::list<AreaTrigger*> atlist;
                atlist.clear();
                me->GetAreaTriggersWithEntryInRange(atlist, 4628, me->GetGUID(), 15.0f);
                if (!atlist.empty())
                    for (std::list<AreaTrigger*>::const_iterator itr = atlist.begin(); itr != atlist.end(); itr++)
                        (*itr)->RemoveFromWorld();
                break;
            }
        }

        void EnterCombat(Unit* who){}

        void EnterEvadeMode(){}

        void UpdateAI(uint32 diff){}
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_incubaterAI(creature);
    }
};

//68194
class npc_young_egg_of_jikun : public CreatureScript
{
public:
    npc_young_egg_of_jikun() : CreatureScript("npc_young_egg_of_jikun") {}

    struct npc_young_egg_of_jikunAI : public ScriptedAI
    {
        npc_young_egg_of_jikunAI(Creature* creature) : ScriptedAI(creature)
        {
            pInstance = creature->GetInstanceScript();
            me->SetReactState(REACT_PASSIVE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
        }
        InstanceScript* pInstance;

        void Reset()
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
        }

        void JustDied(Unit* killer)
        {
            me->RemoveAurasDueToSpell(SPELL_INCUBATE_TARGET_AURA);
            if (killer != me)  //egg destroy
            {
                if (Creature* incubate = me->FindNearestCreature(NPC_INCUBATER, 20.0f, true))
                {
                    float x, y;
                    GetPosInRadiusWithRandomOrientation(me, 2.0f, x, y);
                    incubate->CastSpell(x, y, incubate->GetPositionZ(), SPELL_FEATHER_AT, true);
                }
            }
            else               //incubate complete
            {
                if (Creature* incubate = me->FindNearestCreature(NPC_INCUBATER, 20.0f, true))
                {
                    float x, y;
                    GetPosInRadiusWithRandomOrientation(me, 2.0f, x, y);
                    incubate->SummonCreature(NPC_HATCHLING, x, y, me->GetPositionZ() + 1.0f);
                }
            }
        }

        void EnterCombat(Unit* who){}

        void UpdateAI(uint32 diff){}
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_young_egg_of_jikunAI(creature);
    }
};

//68192
class npc_hatchling : public CreatureScript
{
public:
    npc_hatchling() : CreatureScript("npc_hatchling") { }

    struct npc_hatchlingAI : public ScriptedAI
    {
        npc_hatchlingAI(Creature* pCreature) : ScriptedAI(pCreature)
        {
            me->SetReactState(REACT_DEFENSIVE);
        }

        EventMap events;

        void IsSummonedBy(Unit* summoner)
        {
            if (Player* player = me->FindNearestPlayer(20.0f, true))
            {
                me->Attack(player, true);
                events.ScheduleEvent(EVENT_CHEEP, 5000);
            }
        }

        void JustDied(Unit* killer)
        {
            if (Creature* incubate = me->FindNearestCreature(NPC_INCUBATER, 30.0f, true))
                incubate->CastSpell(me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), SPELL_FEATHER_AT, true);
        }

        void UpdateAI(uint32 diff)
        {
            if (!UpdateVictim())
                return;

            events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            while (uint32 eventId = events.ExecuteEvent())
            {
                if (eventId == EVENT_CHEEP)
                {
                    if (me->getVictim())
                        DoCastVictim(SPELL_CHEEP_LOW);
                    events.ScheduleEvent(EVENT_CHEEP, 5000);
                }
            }
            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_hatchlingAI(pCreature);
    }
};

//69628
class npc_mature_egg_of_jikun : public CreatureScript
{
public:
    npc_mature_egg_of_jikun() : CreatureScript("npc_mature_egg_of_jikun") {}

    struct npc_mature_egg_of_jikunAI : public ScriptedAI
    {
        npc_mature_egg_of_jikunAI(Creature* creature) : ScriptedAI(creature)
        {
            pInstance = creature->GetInstanceScript();
            me->SetReactState(REACT_PASSIVE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
        }
        InstanceScript* pInstance;

        void Reset()
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
        }

        void EnterCombat(Unit* who){}

        void UpdateAI(uint32 diff){}
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_mature_egg_of_jikunAI(creature);
    }
};

//69885
class npc_jump_to_boss_platform : public CreatureScript
{
public:
    npc_jump_to_boss_platform() : CreatureScript("npc_jump_to_boss_platform") {}

    struct npc_jump_to_boss_platformAI : public ScriptedAI
    {
        npc_jump_to_boss_platformAI(Creature* creature) : ScriptedAI(creature)
        {
            pInstance = creature->GetInstanceScript();
            me->SetDisplayId(11686);
            me->SetReactState(REACT_PASSIVE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE | UNIT_FLAG_NOT_SELECTABLE);
        }
        InstanceScript* pInstance;

        void Reset()
        {
            DoCast(me, SPELL_JUMP_TO_B_PLATFORM, true);
        }

        void DamageTaken(Unit* attacker, uint32 &damage)
        {
            if (damage >= me->GetHealth())
                damage = 0;
        }

        void EnterCombat(Unit* who){}

        void UpdateAI(uint32 diff){}
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_jump_to_boss_platformAI(creature);
    }
};

class npc_fall_catcher : public CreatureScript
{
public:
    npc_fall_catcher() : CreatureScript("npc_fall_catcher") {}

    struct npc_fall_catcherAI : public ScriptedAI
    {
        npc_fall_catcherAI(Creature* creature) : ScriptedAI(creature)
        {
            pInstance = creature->GetInstanceScript();
            me->SetDisplayId(11686);
            me->SetReactState(REACT_PASSIVE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE | UNIT_FLAG_NOT_SELECTABLE);
        }
        InstanceScript* pInstance;
        EventMap events;

        void Reset(){}

        void DamageTaken(Unit* attacker, uint32 &damage)
        {
            if (damage >= me->GetHealth())
                damage = 0;
        }

        void MovementInform(uint32 type, uint32 pointId)
        {
            switch (pointId)
            {
            case 1:
                if (Creature* jikun = me->GetCreature(*me, pInstance->GetData64(NPC_JI_KUN)))
                {
                    me->SetFacingToObject(jikun);
                    events.ScheduleEvent(EVENT_FALLCATCHER_IN_POS, 500);
                }
                break;
            case 2:
                if (Vehicle* fallcatcher = me->GetVehicleKit())
                {
                    if (Unit* target = fallcatcher->GetPassenger(0))
                    {
                        target->ExitVehicle();
                        target->SetCanFly(false);
                        target->RemoveAurasDueToSpell(SPELL_PARACHUTE_BUFF);
                        target->RemoveAurasDueToSpell(SPELL_PARACHUTE);
                        target->RemoveAurasDueToSpell(SPELL_SAFE_NET_TRIGGER);
                        target->RemoveAurasDueToSpell(SPELL_GENTLE_YET_FIRM);
                        target->RemoveAurasDueToSpell(SPELL_CAST_FILTER);
                        target->RemoveFlag(PLAYER_FLAGS, PLAYER_ALLOW_ONLY_ABILITY);
                        me->DespawnOrUnsummon();
                    }
                }
                break;
            }
        }


        void PassengerBoarded(Unit* who, int8 /*seatId*/, bool apply)
        {
            me->GetMotionMaster()->MoveCharge(me->GetPositionX(), me->GetPositionY(), -28.29f, 20.0f, 1);
        }

        void EnterCombat(Unit* who){}

        void UpdateAI(uint32 diff)
        {
            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                if (eventId == EVENT_FALLCATCHER_IN_POS)
                {
                    if (Creature* jikun = me->GetCreature(*me, pInstance->GetData64(NPC_JI_KUN)))
                    {
                        float ang = me->GetAngle(jikun);
                        float dist = me->GetExactDist2d(jikun);
                        float destdist = dist - 35.0f;
                        float x, y;
                        GetPositionWithDistInOrientation(me, destdist, ang, x, y);
                        me->GetMotionMaster()->MoveJump(x, y, -30.86f, 20.0f, 20.0f, 2);
                    }
                }
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_fall_catcherAI(creature);
    }
};

//68178
class npc_jikun_feed : public CreatureScript
{
public:
    npc_jikun_feed() : CreatureScript("npc_jikun_feed") {}

    struct npc_jikun_feedAI : public ScriptedAI
    {
        npc_jikun_feedAI(Creature* creature) : ScriptedAI(creature)
        {
            pInstance = creature->GetInstanceScript();
            me->SetCanFly(true);
            me->SetDisableGravity(true);
            me->SetDisplayId(11686);
            me->SetReactState(REACT_PASSIVE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
        }
        InstanceScript* pInstance;

        void Reset()
        {
            DoCast(me, SPELL_FOOD_FALL_VISUAL, true);
            DoCast(me, SPELL_SLIMED_AT, true);
        }

        void MovementInform(uint32 type, uint32 pointId)
        {
            if (pointId == SPELL_JUMPS_DOWN_TO_HATCHLING)
                me->DespawnOrUnsummon();
        }

        void DamageTaken(Unit* attacker, uint32 &damage)
        {
            if (damage >= me->GetHealth())
                damage = 0;
        }

        void EnterCombat(Unit* who){}

        void UpdateAI(uint32 diff){}
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_jikun_feedAI(creature);
    }
};

//68188
class npc_jikun_feed_pool : public CreatureScript
{
public:
    npc_jikun_feed_pool() : CreatureScript("npc_jikun_feed_pool") {}

    struct npc_jikun_feed_poolAI : public ScriptedAI
    {
        npc_jikun_feed_poolAI(Creature* creature) : ScriptedAI(creature)
        {
            pInstance = creature->GetInstanceScript();
            me->SetDisplayId(11686);
            me->SetReactState(REACT_PASSIVE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE /*| UNIT_FLAG_NOT_SELECTABLE*/);
        }
        InstanceScript* pInstance;
        EventMap events;

        void Reset()
        {
            DoCast(me, SPELL_FEED_POOL_SPAWN_P, true);
            events.ScheduleEvent(EVENT_FIND_PLAYER, 1000);
        }

        void DamageTaken(Unit* attacker, uint32 &damage)
        {
            if (damage >= me->GetHealth())
                damage = 0;
        }

        void EnterCombat(Unit* who){}

        void UpdateAI(uint32 diff)
        {
            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                if (eventId == EVENT_FIND_PLAYER)
                {
                    std::list<Player*> pllist;
                    pllist.clear();
                    GetPlayerListInGrid(pllist, me, 3.0f);
                    if (!pllist.empty())
                        for (std::list<Player*>::const_iterator itr = pllist.begin(); itr != pllist.end(); itr++)
                            if (!(*itr)->HasAura(SPELL_FEED_POOL_DMG) && me->GetExactDist2d(*itr) <= 5.5f)
                                me->CastSpell(*itr, SPELL_FEED_POOL_DMG, true);
                    events.ScheduleEvent(EVENT_FIND_PLAYER, 1000);
                }
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_jikun_feed_poolAI(creature);
    }
};

//218543
class go_ji_kun_feather : public GameObjectScript
{
public:
    go_ji_kun_feather() : GameObjectScript("go_ji_kun_feather") { }

    bool OnGossipHello(Player* player, GameObject* go)
    {
        InstanceScript* pInstance = (InstanceScript*)go->GetInstanceScript();
        if (!pInstance)
            return false;

        if (!player->HasAura(SPELL_JIKUN_FLY))
            player->CastSpell(player, SPELL_JIKUN_FLY, true);

        return true;
    }
};

//133755
class spell_jikun_fly : public SpellScriptLoader
{
public:
    spell_jikun_fly() : SpellScriptLoader("spell_jikun_fly") { }

    class spell_jikun_fly_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_jikun_fly_SpellScript);

        SpellCastResult CheckCast()
        {
            if (GetCaster())
            {
                if (GetCaster()->GetMap() && GetCaster()->GetMap()->GetId() == 1098) //Throne of Thunder
                    if (InstanceScript* instance = GetCaster()->GetInstanceScript())
                        if (instance->GetBossState(DATA_JI_KUN) == IN_PROGRESS)
                            return SPELL_CAST_OK;

                //Check cast failed, remove aura
                GetCaster()->RemoveAurasDueToSpell(SPELL_DAEDALIAN_WINGS);
            }
            return SPELL_FAILED_NOT_HERE;
        }

        void HandleAfterCast()
        {
            if (GetCaster())
            {
                if (Aura* aura = GetCaster()->GetAura(SPELL_DAEDALIAN_WINGS))
                {
                    if (aura->GetStackAmount() > 1)
                        aura->SetStackAmount(aura->GetStackAmount() - 1);
                    else
                        aura->Remove();
                }
            }
        }

        void Register()
        {
            OnCheckCast += SpellCheckCastFn(spell_jikun_fly_SpellScript::CheckCast);
            AfterCast += SpellCastFn(spell_jikun_fly_SpellScript::HandleAfterCast);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_jikun_fly_SpellScript();
    }
};

//134339
class spell_daedalian_wings : public SpellScriptLoader
{
public:
    spell_daedalian_wings() : SpellScriptLoader("spell_daedalian_wings") { }

    class spell_daedalian_wings_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_daedalian_wings_AuraScript);

        void OnApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
        {
            if (GetTarget())
                if (Aura* aura = GetTarget()->GetAura(SPELL_DAEDALIAN_WINGS))
                    if (aura->GetStackAmount() < 4)
                        aura->SetStackAmount(4);
        }

        void Register()
        {
            OnEffectApply += AuraEffectApplyFn(spell_daedalian_wings_AuraScript::OnApply, EFFECT_0, SPELL_AURA_OVERRIDE_SPELLS, AURA_EFFECT_HANDLE_REAL);
        }
    };

    AuraScript* GetAuraScript() const
    {
        return new spell_daedalian_wings_AuraScript();
    }
};

//134347
class spell_incubated : public SpellScriptLoader
{
public:
    spell_incubated() : SpellScriptLoader("spell_incubated") { }

    class spell_incubated_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_incubated_AuraScript);

        void HandleEffectRemove(AuraEffect const * /*aurEff*/, AuraEffectHandleModes mode)
        {
            if (GetTarget() && GetTargetApplication()->GetRemoveMode() == AURA_REMOVE_BY_EXPIRE)
                if (GetTarget()->ToCreature())
                    GetTarget()->Kill(GetTarget());
        }

        void Register()
        {
            OnEffectRemove += AuraEffectRemoveFn(spell_incubated_AuraScript::HandleEffectRemove, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
        }
    };

    AuraScript* GetAuraScript() const
    {
        return new spell_incubated_AuraScript();
    }
};

class TankFilter
{
public:
    bool operator()(WorldObject* unit)
    {
        if (Player* target = unit->ToPlayer())
            if (target->GetRoleForGroup(target->GetSpecializationId(target->GetActiveSpec())) != ROLES_TANK)
                return false;
        return true;
    }
};

//138923
class spell_caw_searcher : public SpellScriptLoader
{
public:
    spell_caw_searcher() : SpellScriptLoader("spell_caw_searcher") { }

    class spell_caw_searcher_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_caw_searcher_SpellScript);

        void FilterTargets(std::list<WorldObject*> &targets)
        {
            if (GetCaster())
            {
                targets.remove_if(TankFilter());
                uint8 targetcount = GetCaster()->GetMap()->Is25ManRaid() ? 5 : 2;
                if (targets.size() > targetcount)
                    targets.resize(targetcount);
            }
        }

        void HandleScript(SpellEffIndex effIndex)
        {
            if (GetCaster() && GetHitUnit())
                GetCaster()->CastSpell(GetHitUnit(), SPELL_CAW_DMG, true);
        }

        void Register()
        {
            OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_caw_searcher_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENEMY);
            OnEffectHitTarget += SpellEffectFn(spell_caw_searcher_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_caw_searcher_SpellScript();
    }
};

//134385
class spell_regurgitate : public SpellScriptLoader
{
public:
    spell_regurgitate() : SpellScriptLoader("spell_regurgitate") { }

    class spell_regurgitate_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_regurgitate_SpellScript);

        //this only test version
        void HandleScript(SpellEffIndex effIndex)
        {
            if (GetCaster() && GetCaster()->ToCreature())
            {
                if (InstanceScript* instance = GetCaster()->GetInstanceScript())
                {
                    /*if (Creature* incubate = GetCaster()->GetCreature(*GetCaster(), instance->GetData64(DATA_GET_FIRST_NEST)))
                    {
                        if (Creature* feed = GetCaster()->SummonCreature(NPC_FEED, GetCaster()->GetPositionX(), GetCaster()->GetPositionY(), GetCaster()->GetPositionZ() + 15.0f, 0.0f))
                            feed->CastSpell(incubate, SPELL_JUMPS_DOWN_TO_HATCHLING, true);
                    }*/
                }
            }
        }

        void Register()
        {
            OnEffectHitTarget += SpellEffectFn(spell_regurgitate_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_regurgitate_SpellScript();
    }
};

//138319
class spell_jikun_feed_platform_p_dmg : public SpellScriptLoader
{
public:
    spell_jikun_feed_platform_p_dmg() : SpellScriptLoader("spell_jikun_feed_platform_p_dmg") { }

    class spell_jikun_feed_platform_p_dmg_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_jikun_feed_platform_p_dmg_AuraScript);

        void OnTick(AuraEffect const* aurEff)
        {
            if (GetCaster() && GetCaster()->ToCreature() && GetTarget())
            {
                if (aurEff->GetTickNumber() > 3)
                {
                    GetCaster()->ToCreature()->DespawnOrUnsummon();
                    GetTarget()->RemoveAurasDueToSpell(SPELL_FEED_POOL_DMG);
                    GetTarget()->CastSpell(GetTarget(), SPELL_SLIMED_DMG, true);
                    return;
                }
                if (GetTarget()->GetExactDist2d(GetCaster()) > 6.0f)
                    GetTarget()->RemoveAurasDueToSpell(SPELL_FEED_POOL_DMG);
            }
        }

        

        void Register()
        {
            OnEffectPeriodic += AuraEffectPeriodicFn(spell_jikun_feed_platform_p_dmg_AuraScript::OnTick, EFFECT_0, SPELL_AURA_PERIODIC_DAMAGE);
        }
    };

    AuraScript* GetAuraScript() const
    {
        return new spell_jikun_feed_platform_p_dmg_AuraScript();
    }
};

//134256
class spell_jikun_slimed_aura : public SpellScriptLoader
{
public:
    spell_jikun_slimed_aura() : SpellScriptLoader("spell_jikun_slimed_aura") { }

    class spell_jikun_slimed_aura_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_jikun_slimed_aura_AuraScript);

        void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
        {
            if (GetTargetApplication()->GetRemoveMode() != AURA_REMOVE_BY_DEATH)
                GetTarget()->CastSpell(GetTarget(), SPELL_SLIMED_DEBUFF, true);
        }

        void Register()
        {
            OnEffectRemove += AuraEffectRemoveFn(spell_jikun_slimed_aura_AuraScript::OnRemove, EFFECT_0, SPELL_AURA_PERIODIC_DAMAGE, AURA_EFFECT_HANDLE_REAL);
        }
    };

    AuraScript* GetAuraScript() const
    {
        return new spell_jikun_slimed_aura_AuraScript();
    }
};

//8848
class at_jikun_precipice : public AreaTriggerScript
{
public:
    at_jikun_precipice() : AreaTriggerScript("at_jikun_precipice") { }

    bool OnTrigger(Player* player, AreaTriggerEntry const* areaTrigger, bool enter)
    {
        if (enter)
        {
            player->RemoveUnitMovementFlag(MOVEMENTFLAG_FALLING);
            player->SetCanFly(true);
            player->StopMoving();
            player->GetMotionMaster()->Clear(true);
            player->RemoveAurasDueToSpell(SPELL_PARACHUTE_BUFF);
            player->RemoveAurasDueToSpell(SPELL_PARACHUTE);
            player->CastSpell(player, SPELL_SAFE_NET_TRIGGER, true);
            player->SetFlag(PLAYER_FLAGS, PLAYER_ALLOW_ONLY_ABILITY); //for safe(block all ability)
        }
        return true;
    }
};

void AddSC_boss_jikun()
{
    new boss_jikun();
    new npc_incubater();
    new npc_young_egg_of_jikun();
    new npc_hatchling();
    new npc_mature_egg_of_jikun();
    new npc_jump_to_boss_platform();
    new npc_fall_catcher();
    new npc_jikun_feed();
    new npc_jikun_feed_pool();
    new go_ji_kun_feather();
    new spell_jikun_fly();
    new spell_daedalian_wings();
    new spell_incubated();
    new spell_caw_searcher();
    new spell_regurgitate();
    new spell_jikun_feed_platform_p_dmg();
    new spell_jikun_slimed_aura();
    new at_jikun_precipice();
}
