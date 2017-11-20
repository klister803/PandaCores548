/*
* Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
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

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "utgarde_pinnacle.h"

enum Spells
{
    SPELL_SVALA_TRANSFORMING1 = 54140,
    SPELL_SVALA_TRANSFORMING2 = 54205,
    SPELL_TRANSFORMING_CHANNEL = 54142,

    SPELL_CALL_FLAMES = 48258, // caster effect only, triggers event 17841

    SPELL_SINSTER_STRIKE = 15667,
    SPELL_HEROIC_SINSTER_STRIKE = 59409,

    SPELL_RITUAL_PREPARATION = 48267,
    SPELL_RITUAL_OF_THE_SWORD = 48276,
    SPELL_RITUAL_STRIKE_TRIGGER = 48331, // triggers 48277 & 59930, needs NPC_RITUAL_TARGET as spell_script_target
    SPELL_RITUAL_DISARM = 54159,
    SPELL_RITUAL_STRIKE_EFF_1 = 48277,
    SPELL_RITUAL_STRIKE_EFF_2 = 59930,

    //SPELL_SUMMONED_VIS                          = 64446,
    SPELL_RITUAL_CHANNELER_1 = 48271,
    SPELL_RITUAL_CHANNELER_2 = 48274,
    SPELL_RITUAL_CHANNELER_3 = 48275,

    // Ritual Channeler spells
    SPELL_PARALYZE = 48278,
    SPELL_SHADOWS_IN_THE_DARK = 59407,

    // Scourge Hulk spells
    SPELL_MIGHTY_BLOW = 48697,
    SPELL_VOLATILE_INFECTION = 56785,
    H_SPELL_VOLATILE_INFECTION = 59228,

    SPELL_TELEPORT = 12980,
    SPELL_IMAGE_OF_ARTHAS_VISUAL_EFFECT = 54134,
};

enum Yells
{
    // Svala
    SAY_SVALA_INTRO_0 = 0,

    // Svala Sorrowgrave
    SAY_SVALA_INTRO_1 = 0,
    SAY_SVALA_INTRO_2 = 1,
    SAY_AGGRO = 2,
    SAY_SLAY = 3,
    SAY_DEATH = 4,
    SAY_SACRIFICE_PLAYER = 5,

    // Image of Arthas
    SAY_DIALOG_OF_ARTHAS_1 = 0,
    SAY_DIALOG_OF_ARTHAS_2 = 1
};

enum Creatures
{
    CREATURE_ARTHAS = 29280, // Image of Arthas
    CREATURE_SVALA_SORROWGRAVE = 26668, // Svala after transformation
    CREATURE_SVALA = 29281, // Svala before transformation
    CREATURE_RITUAL_CHANNELER = 27281,
    CREATURE_SPECTATOR = 26667,
    CREATURE_RITUAL_TARGET = 27327,
    CREATURE_FLAME_BRAZIER = 27273,
    CREATURE_SCOURGE_HULK = 26555
};

enum Objects
{
    OBJECT_UTGARDE_MIRROR = 191745
};

enum SvalaPhase
{
    IDLE,
    INTRO,
    NORMAL,
    SACRIFICING,
    SVALADEAD
};

enum Events
{
    EVENT_INTRO = 1,
    EVENT_INTRO_2,
    EVENT_INTRO_3,
    EVENT_INTRO_4,
    EVENT_INTRO_5,
    EVENT_INTRO_6,
    EVENT_INTRO_7,
    EVENT_INTRO_8,
    EVENT_INTRO_9,
    EVENT_INTRO_10,

    EVENT_SINSTER_STRIKE,
    EVENT_CALL_FLAMES,
    EVENT_SACRIFICE_PREPARE,
    EVENT_SACRIFICE,
    EVENT_SACRIFICE_LAUNCH,
    EVENT_SACRIFICE_UPDATE,
    EVENT_RE_ATTACK,
};

#define DATA_INCREDIBLE_HULK 2043

static const float spectatorWP[2][3] =
{
    { 296.95f, -312.76f, 86.36f },
    { 297.69f, -275.81f, 86.36f }
};

uint32 const ritualchanneler[3] =
{
    SPELL_RITUAL_CHANNELER_1,
    SPELL_RITUAL_CHANNELER_2,
    SPELL_RITUAL_CHANNELER_3,
};

Position const dspos = { 296.689f, -346.504f, 108.548f, 0.0f };
Position const ritualtargetpos = { 296.651f, -346.293f, 91.3533f, 4.08407f };
static Position ArthasPos = { 296.2659f, -364.3349f, 92.92485f, 1.58825f };

//29281
class boss_svala : public CreatureScript
{
public:
    boss_svala() : CreatureScript("boss_svala") { }

    struct boss_svalaAI : public BossAI
    {
        boss_svalaAI(Creature* creature) : BossAI(creature, DATA_SVALA)
        {
            instance = creature->GetInstanceScript();
            me->ApplySpellImmune(0, IMMUNITY_ID, SPELL_RITUAL_STRIKE_EFF_1, true);
            me->ApplySpellImmune(0, IMMUNITY_ID, SPELL_RITUAL_STRIKE_EFF_2, true);
            Phase = IDLE;
            intro = false;
        }

        InstanceScript* instance;
        SvalaPhase Phase;
        uint64 arthasGUID;
        uint64 sacrificetargetGuid;
        uint32 sinsterStrikeTimer;
        uint32 callFlamesTimer;
        uint32 sacrificeTimer;
        bool sacrificed;
        bool intro;

        void Reset()
        {
            _Reset();
            events.Reset();
            me->SetCanFly(true);
            me->SetDisableGravity(true);
            me->SetReactState(REACT_DEFENSIVE);
            sacrificed = false;
            sacrificetargetGuid = 0;
            me->RemoveAllAuras();
            instance->SetData(DATA_SVALA_SORROWGRAVE_EVENT, NOT_STARTED);
        }

        Creature* GetArthas()
        {
            if (Creature* Arthas = me->GetCreature(*me, arthasGUID))
                return Arthas;

            return NULL;
        }

        void EnterCombat(Unit* /*who*/)
        {
            Talk(SAY_AGGRO);
            me->SetInCombatWithZone();
            events.ScheduleEvent(EVENT_SINSTER_STRIKE, 7000);
            events.ScheduleEvent(EVENT_CALL_FLAMES, 15000);
            instance->SetData(DATA_SVALA_SORROWGRAVE_EVENT, IN_PROGRESS);
        }

        void JustSummoned(Creature* summon)
        {
            summons.Summon(summon);
            summon->CastSpell(summon, SPELL_TELEPORT, true);
        }

        void DamageTaken(Unit* attacker, uint32 &damage)
        {
            if (HealthBelowPct(50) && !sacrificed)
            {
                sacrificed = true;
                events.Reset();
                me->InterruptNonMeleeSpells(true);
                events.ScheduleEvent(EVENT_SACRIFICE_PREPARE, 1000);
            }
        }

        void DoAction(int32 const action)
        {
            if (action == ACTION_RE_ATTACK)
                events.ScheduleEvent(EVENT_RE_ATTACK, 1000);
        }

        void SetData(uint32 type, uint32 data)
        {
            if (type == DATA_SVALA_INTRO && !intro)
            {
                intro = true;
                Phase = INTRO;
                me->SetReactState(REACT_PASSIVE);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);

                if (GameObject* mirror = GetClosestGameObjectWithEntry(me, OBJECT_UTGARDE_MIRROR, 100.0f))
                    mirror->SetGoState(GO_STATE_READY);

                if (Creature* Arthas = me->SummonCreature(CREATURE_ARTHAS, ArthasPos, TEMPSUMMON_MANUAL_DESPAWN))
                {
                    me->HandleEmoteCommand(EMOTE_ONESHOT_TALK);
                    Arthas->SetAttackStop(true);
                    Arthas->setFaction(14);
                    Arthas->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
                    Arthas->CastSpell(Arthas, SPELL_IMAGE_OF_ARTHAS_VISUAL_EFFECT, true);
                    Arthas->SetFacingTo(1.58825f);
                    arthasGUID = Arthas->GetGUID();
                    events.ScheduleEvent(EVENT_INTRO, 1000);
                }
            }
        }

        uint32 GetData(uint32 type)
        {
            if (type == DATA_SVALA_INTRO)
                return intro;

            return 0;
        }

        uint64 GetGUID(int32 id)
        {
            if (id == DATA_GET_SACRIFICE_TARGET)
                return sacrificetargetGuid;

            return 0;
        }

        void KilledUnit(Unit* victim)
        {
            if (victim->ToPlayer())
                Talk(SAY_SLAY);
        }

        void JustDied(Unit* /*killer*/)
        {
            Talk(SAY_DEATH);
            _JustDied();

            if (Phase == SACRIFICING)
                SetEquipmentSlots(false, EQUIP_UNEQUIP, EQUIP_NO_CHANGE, EQUIP_NO_CHANGE);

            me->HandleEmoteCommand(EMOTE_ONESHOT_FLYDEATH);
            instance->SetData(DATA_SVALA_SORROWGRAVE_EVENT, DONE);
        }

        void UpdateAI(uint32 diff)
        {
            if (!UpdateVictim() && Phase > INTRO)
                return;

            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    //Intro
                case EVENT_INTRO:
                    Talk(SAY_SVALA_INTRO_0);
                    events.ScheduleEvent(EVENT_INTRO_2, 8100);
                    break;
                case EVENT_INTRO_2:
                    if (Creature* Arthas = GetArthas())
                        Arthas->AI()->Talk(SAY_DIALOG_OF_ARTHAS_1);
                    events.ScheduleEvent(EVENT_INTRO_3, 10000);
                    break;
                case EVENT_INTRO_3:
                {
                    if (Creature* Arthas = GetArthas())
                        Arthas->CastSpell(me, SPELL_TRANSFORMING_CHANNEL, true);

                    Position pos;
                    me->GetPosition(&pos);
                    pos.m_positionZ += 8.0f;
                    me->SetCanFly(true);
                    me->SetDisableGravity(true);
                    me->GetMotionMaster()->MoveTakeoff(0, pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ());
                    std::list<Creature*> lspectatorList;
                    lspectatorList.clear();
                    GetCreatureListWithEntryInGrid(lspectatorList, me, CREATURE_SPECTATOR, 100.0f);
                    if (!lspectatorList.empty())
                    {
                        for (std::list<Creature*>::iterator itr = lspectatorList.begin(); itr != lspectatorList.end(); ++itr)
                        {
                            if ((*itr)->isAlive())
                            {
                                (*itr)->SetStandState(UNIT_STAND_STATE_STAND);
                                (*itr)->SetWalk(false);
                                (*itr)->GetMotionMaster()->MovePoint(1, spectatorWP[0][0], spectatorWP[0][1], spectatorWP[0][2]);
                            }
                        }
                    }
                    events.ScheduleEvent(EVENT_INTRO_4, 4200);
                    break;
                }
                case EVENT_INTRO_4:
                    me->CastSpell(me, SPELL_SVALA_TRANSFORMING1);
                    events.ScheduleEvent(EVENT_INTRO_5, 6200);
                    break;
                case EVENT_INTRO_5:
                    if (Creature* Arthas = GetArthas())
                    {
                        Arthas->InterruptNonMeleeSpells(true);
                        me->CastSpell(me, SPELL_SVALA_TRANSFORMING2);
                        me->RemoveAllAuras();
                        me->UpdateEntry(CREATURE_SVALA_SORROWGRAVE);
                        me->SetSheath(SHEATH_STATE_UNARMED);
                        me->SetReactState(REACT_DEFENSIVE);
                        me->SetFacingToObject(Arthas);
                        me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                    }
                    events.ScheduleEvent(EVENT_INTRO_6, 3200);
                    break;
                case EVENT_INTRO_6:
                    Talk(SAY_SVALA_INTRO_1);
                    events.ScheduleEvent(EVENT_INTRO_7, 10000);
                    break;
                case EVENT_INTRO_7:
                    if (Creature* Arthas = GetArthas())
                        Arthas->AI()->Talk(SAY_DIALOG_OF_ARTHAS_2);
                    events.ScheduleEvent(EVENT_INTRO_8, 7200);
                    break;
                case EVENT_INTRO_8:
                    Talk(SAY_SVALA_INTRO_2);
                    me->SetFacingTo(1.58f);
                    if (Creature* Arthas = GetArthas())
                    {
                        Arthas->SetVisible(false);
                        Arthas->DespawnOrUnsummon();
                    }
                    events.ScheduleEvent(EVENT_INTRO_9, 13800);
                    break;
                case EVENT_INTRO_9:
                {
                    Position pos;
                    me->GetPosition(&pos);
                    pos.m_positionZ = 94.54f;
                    me->SetHomePosition(pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), 1.58f);
                    me->GetMotionMaster()->MoveTakeoff(0, pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ());
                    events.ScheduleEvent(EVENT_INTRO_10, 3000);
                    break;
                }
                case EVENT_INTRO_10:
                    if (GameObject* mirror = GetClosestGameObjectWithEntry(me, OBJECT_UTGARDE_MIRROR, 100.0f))
                        mirror->SetGoState(GO_STATE_ACTIVE);

                    me->SetSheath(SHEATH_STATE_MELEE);
                    me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                    Phase = NORMAL;
                    break;
                    //Combat events
                case EVENT_SINSTER_STRIKE:
                    if (me->getVictim())
                        DoCast(me->getVictim(), SPELL_SINSTER_STRIKE);
                    events.ScheduleEvent(EVENT_SINSTER_STRIKE, 7000);
                    break;
                case EVENT_CALL_FLAMES:
                    DoCast(me, SPELL_CALL_FLAMES);
                    events.ScheduleEvent(EVENT_CALL_FLAMES, 15000);
                    break;
                case EVENT_SACRIFICE_PREPARE:
                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 80.0f, true))
                    {
                        me->SetAttackStop(true);
                        Talk(SAY_SACRIFICE_PLAYER);
                        instance->SetData64(DATA_SACRIFICED_PLAYER, target->GetGUID());
                        DoCast(target, SPELL_RITUAL_PREPARATION);
                        Phase = SACRIFICING;
                    }
                    events.ScheduleEvent(EVENT_SACRIFICE, 1000);
                    break;
                case EVENT_SACRIFICE:
                    for (uint8 n = 0; n < 3; n++)
                        me->CastSpell(me, ritualchanneler[n], true);

                    me->SetCanFly(true);
                    me->SetDisableGravity(true);
                    me->NearTeleportTo(dspos.GetPositionX(), dspos.GetPositionY(), dspos.GetPositionZ(), 0.0f);
                    events.ScheduleEvent(EVENT_SACRIFICE_LAUNCH, 500);
                    break;
                case EVENT_SACRIFICE_LAUNCH:
                    if (Creature* daggertarget = me->FindNearestCreature(NPC_RITUAL_TARGET, 80.0f, true))
                    {
                        daggertarget->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_PERIODIC_DAMAGE, true);
                        me->CastSpell(daggertarget, SPELL_RITUAL_STRIKE_TRIGGER, true);
                        me->AddAura(SPELL_RITUAL_DISARM, me);
                    }
                    events.ScheduleEvent(EVENT_SACRIFICE_UPDATE, 500);
                    break;
                case EVENT_SACRIFICE_UPDATE:
                    DoCast(me, SPELL_RITUAL_OF_THE_SWORD, true);
                    break;
                case EVENT_RE_ATTACK:
                    me->InterruptNonMeleeSpells(true);
                    me->ReAttackWithZone();
                    events.ScheduleEvent(EVENT_SINSTER_STRIKE, 7000);
                    events.ScheduleEvent(EVENT_CALL_FLAMES, 15000);
                    break;
                }
            }
            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new boss_svalaAI(creature);
    }
};

class npc_ritual_channeler : public CreatureScript
{
public:
    npc_ritual_channeler() : CreatureScript("npc_ritual_channeler") { }

    struct npc_ritual_channelerAI : public Scripted_NoMovementAI
    {
        npc_ritual_channelerAI(Creature* creature) :Scripted_NoMovementAI(creature)
        {
            instance = creature->GetInstanceScript();
            me->SetReactState(REACT_PASSIVE);
        }

        InstanceScript* instance;
        EventMap events;

        void Reset()
        {
            if (IsHeroic())
                DoCast(me, SPELL_SHADOWS_IN_THE_DARK);

            events.ScheduleEvent(EVENT_SACRIFICE, 500);
        }

        void JustDied(Unit* killer)
        {
            me->DespawnOrUnsummon(2000);
        }

        void UpdateAI(uint32 diff)
        {
            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                if (eventId == EVENT_SACRIFICE)
                {
                    DoCast(me, SPELL_PARALYZE, true);
                    if (Player* starget = me->GetPlayer(*me, instance->GetData64(DATA_SACRIFICED_PLAYER)))
                        me->SetFacingToObject(starget);
                }
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_ritual_channelerAI(creature);
    }
};

//27273
class npc_flame_brazler : public CreatureScript
{
public:
    npc_flame_brazler() : CreatureScript("npc_flame_brazler") { }

    struct npc_flame_brazlerAI : public ScriptedAI
    {
        npc_flame_brazlerAI(Creature* creature) : ScriptedAI(creature)
        {
            me->SetReactState(REACT_PASSIVE);
            me->setFaction(14);
            me->SetCanFly(true);
            me->SetDisableGravity(true);
            me->SetDisplayId(11686);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
        }

        void Reset(){}

        void DamageTaken(Unit* attacker, uint32 &damage)
        {
            damage = 0;
        }

        void UpdateAI(uint32 diff){}
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_flame_brazlerAI(creature);
    }
};

class npc_spectator : public CreatureScript
{
public:
    npc_spectator() : CreatureScript("npc_spectator") { }

    struct npc_spectatorAI : public ScriptedAI
    {
        npc_spectatorAI(Creature* creature) : ScriptedAI(creature) { }

        void Reset() {}

        void MovementInform(uint32 motionType, uint32 pointId)
        {
            if (motionType == POINT_MOTION_TYPE)
            {
                if (pointId == 1)
                    me->GetMotionMaster()->MovePoint(2, spectatorWP[1][0], spectatorWP[1][1], spectatorWP[1][2]);
                else if (pointId == 2)
                    me->DespawnOrUnsummon(1000);
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_spectatorAI(creature);
    }
};

class npc_scourge_hulk : public CreatureScript
{
public:
    npc_scourge_hulk() : CreatureScript("npc_scourge_hulk") { }

    struct npc_scourge_hulkAI : public ScriptedAI
    {
        npc_scourge_hulkAI(Creature* creature) : ScriptedAI(creature) { }

        uint32 mightyBlow;
        uint32 volatileInfection;

        void Reset()
        {
            mightyBlow = urand(4000, 9000);
            volatileInfection = urand(10000, 14000);
        }

        void UpdateAI(uint32 diff)
        {
            if (!UpdateVictim())
                return;

            if (mightyBlow <= diff)
            {
                if (Unit* victim = me->getVictim())
                    if (!victim->HasUnitState(UNIT_STATE_STUNNED))    // Prevent knocking back a ritual player
                        DoCast(victim, SPELL_MIGHTY_BLOW);
                mightyBlow = urand(12000, 17000);
            }
            else
                mightyBlow -= diff;

            if (volatileInfection <= diff)
            {
                DoCastVictim(SPELL_VOLATILE_INFECTION);
                volatileInfection = urand(13000, 17000);
            }
            else
                volatileInfection -= diff;

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_scourge_hulkAI(creature);
    }
};

class checkRitualTarget
{
public:
    explicit checkRitualTarget(Unit* target) : _target(target) { }

    bool operator() (WorldObject* unit)
    {
        if (unit->GetGUID() != _target->GetGUID())
            return true;

        return false;
    }
private:
    Unit* _target;
};

//48278
class spell_paralyze_pinnacle : public SpellScriptLoader
{
public:
    spell_paralyze_pinnacle() : SpellScriptLoader("spell_paralyze_pinnacle") { }

    class spell_paralyze_pinnacle_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_paralyze_pinnacle_SpellScript);

        void FilterTargets(std::list<WorldObject*>& unitList)
        {
            if (GetCaster())
            {
                if (InstanceScript* instance = GetCaster()->GetInstanceScript())
                    if (Player* starget = GetCaster()->GetPlayer(*GetCaster(), instance->GetData64(DATA_SACRIFICED_PLAYER)))
                        unitList.remove_if(checkRitualTarget(starget));

                if (unitList.size() > 1)
                    unitList.clear();
            }
        }

        void Register()
        {
            OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_paralyze_pinnacle_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENEMY);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_paralyze_pinnacle_SpellScript();
    }
};

class SvalaBallofFlameFilter
{
public:
    SvalaBallofFlameFilter(){}

    bool operator()(WorldObject* unit)
    {
        if (unit->ToPlayer())
            return false;

        return true;
    }
};

//48246
class spell_svala_ball_of_flame : public SpellScriptLoader
{
public:
    spell_svala_ball_of_flame() : SpellScriptLoader("spell_svala_ball_of_flame") { }

    class spell_svala_ball_of_flame_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_svala_ball_of_flame_SpellScript);

        void FilterTargets(std::list<WorldObject*>& targets)
        {
            targets.remove_if(SvalaBallofFlameFilter());
            if (targets.size() > 1)
                targets.resize(1);
        }

        void Register()
        {
            OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_svala_ball_of_flame_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENTRY);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_svala_ball_of_flame_SpellScript();
    }
};

//48277
class spell_svala_ritual_strike_explose : public SpellScriptLoader
{
public:
    spell_svala_ritual_strike_explose() : SpellScriptLoader("spell_svala_ritual_strike_explose") { }

    class spell_svala_ritual_strike_explose_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_svala_ritual_strike_explose_SpellScript);

        void HandleAfterCast()
        {
            if (GetCaster() && GetCaster()->ToCreature())
            {
                if (InstanceScript* instance = GetCaster()->GetInstanceScript())
                    instance->SetData(DATA_KILL_RITUAL_CHANNELER, 0);

                GetCaster()->ToCreature()->AI()->DoAction(ACTION_RE_ATTACK);
            }
        }

        void Register()
        {
            AfterCast += SpellCastFn(spell_svala_ritual_strike_explose_SpellScript::HandleAfterCast);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_svala_ritual_strike_explose_SpellScript();
    }
};

//5140
class at_svala_entrance : public AreaTriggerScript
{
public:
    at_svala_entrance() : AreaTriggerScript("at_svala_entrance") { }

    bool OnTrigger(Player* player, const AreaTriggerEntry* /*pAt*/, bool enter)
    {
        if (!enter)
            return true;

        if (InstanceScript* instance = player->GetInstanceScript())
            if (Creature* svala = player->GetCreature(*player, instance->GetData64(DATA_SVALA)))
                if (!svala->AI()->GetData(DATA_SVALA_INTRO))
                    svala->AI()->SetData(DATA_SVALA_INTRO, 0);

        return true;
    }
};

/*class achievement_incredible_hulk : public AchievementCriteriaScript
{
public:
    achievement_incredible_hulk() : AchievementCriteriaScript("achievement_incredible_hulk") { }

    bool OnCheck(Player* /*player, Unit* target)
    {
        if (!target)
            return false;

        if (Creature* hulk = target->ToCreature())
            if (npc_scourge_hulk::npc_scourge_hulkAI* hulkAI = CAST_AI(npc_scourge_hulk::npc_scourge_hulkAI, hulk->AI()))
                if (hulkAI->GetHulk())
                    return true;

        return false;
    }
};*/

void AddSC_boss_svala()
{
    new boss_svala();
    new npc_ritual_channeler();
    new npc_flame_brazler();
    new npc_spectator();
    new npc_scourge_hulk();
    new spell_paralyze_pinnacle();
    new spell_svala_ball_of_flame();
    new spell_svala_ritual_strike_explose();
    new at_svala_entrance();
    //new achievement_incredible_hulk();
}
