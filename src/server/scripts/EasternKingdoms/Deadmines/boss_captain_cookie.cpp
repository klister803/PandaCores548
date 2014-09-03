#include "ScriptPCH.h"
#include "deadmines.h"

enum Spells
{
    SPELL_WHO_IS_THAT       = 89339,
    SPELL_SETIATED          = 89267,
    SPELL_SETIATED_H        = 92834,
    SPELL_NAUSEATED         = 89732,
    SPELL_NAUSEATED_H       = 92066,
    SPELL_ROTTEN_AURA       = 89734,
    SPELL_ROTTEN_AURA_H     = 95513,
    SPELL_ROTTEN_AURA_DMG   = 89734,
    SPELL_ROTTEN_AURA_DMG_H = 92065,
    SPELL_CAULDRON          = 89250,
    SPELL_CAULDRON_VISUAL   = 89251,
    SPELL_CAULDRON_FIRE     = 89252,


    SPELL_THROW_FOOD_01     = 90557,
    SPELL_THROW_FOOD_01_H   = 92059,
    SPELL_THROW_FOOD_02     = 90560,
    SPELL_THROW_FOOD_02_H   = 92060,
    SPELL_THROW_FOOD_03     = 90603,
    SPELL_THROW_FOOD_03_H   = 92840,
    SPELL_THROW_FOOD_04     = 89739,
    SPELL_THROW_FOOD_04_H   = 92838,
    SPELL_THROW_FOOD_05     = 90605,
    SPELL_THROW_FOOD_05_H   = 92844,
    SPELL_THROW_FOOD_06     = 90556,
    SPELL_THROW_FOOD_06_H   = 92058,
    SPELL_THROW_FOOD_07     = 90680,
    SPELL_THROW_FOOD_07_H   = 92842,
    SPELL_THROW_FOOD_08     = 90559,
    SPELL_THROW_FOOD_08_H   = 92062,
    SPELL_THROW_FOOD_09     = 90602,
    SPELL_THROW_FOOD_09_H   = 92841,
    SPELL_THROW_FOOD_10     = 89263,
    SPELL_THROW_FOOD_10_H   = 92057,
    SPELL_THROW_FOOD_11     = 90604,
    SPELL_THROW_FOOD_11_H   = 92839,
    SPELL_THROW_FOOD_12     = 90555,
    SPELL_THROW_FOOD_12_H   = 92063,
    SPELL_THROW_FOOD_13     = 90606,
    SPELL_THROW_FOOD_13_H   = 92843,
};

enum Adds
{
    NPC_BABY_MURLOC         = 48672,

    NPC_CAULDRON            = 47754,

    NPC_BUN                 = 48301,
    NPC_MISTERY_MEAT        = 48297,
    NPC_BREAD_LOAF          = 48300,
    NPC_STEAK               = 48296,
    NPC_CORN                = 48006,
    NPC_MELON               = 48294,

    NPC_ROTTEN_SNEAK        = 48295,
    NPC_ROTTEN_CORN         = 48276,
    NPC_ROTTEN_LOAF         = 48299,
    NPC_ROTTEN_MELON        = 48293,
    NPC_ROTTEN_MISTERY_MEAT = 48298,
    NPC_ROTTEN_BUN          = 48302,
};

enum Events
{
    EVENT_THROW_FOOD    = 1,
};

const Position notePos = {-74.3611f, -820.014f, 40.3714f, 0.0f};

const Position captaincookiePos = {-64.07f, -820.27f, 41.17f, 0.0f};

class boss_captain_cookie : public CreatureScript
{
    public:
        boss_captain_cookie() : CreatureScript("boss_captain_cookie") { }

        CreatureAI* GetAI(Creature* pCreature) const
        {
            return new boss_captain_cookieAI (pCreature);
        }

        struct boss_captain_cookieAI : public BossAI
        {
            boss_captain_cookieAI(Creature* pCreature) : BossAI(pCreature, DATA_CAPTAIN)
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
                me->SetReactState(REACT_PASSIVE);
                me->setActive(true);
            }

            void Reset()
            {
                _Reset();
                DoCast(SPELL_WHO_IS_THAT);
            }

            void MoveInLineOfSight(Unit* who)
            {
                if (instance->GetBossState(DATA_ADMIRAL) != DONE)
                    return;

                if (me->GetDistance(who) > 5.0f)
                    return;

                DoZoneInCombat();
            }

            void EnterCombat(Unit* who)
            {
                me->RemoveAurasDueToSpell(SPELL_WHO_IS_THAT);
                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                //me->GetMotionMaster()->MoveJump(captaincookiePos.GetPositionX(), captaincookiePos.GetPositionY(), captaincookiePos.GetPositionZ(), 40.0f, 20.0f);
                me->CastSpell(centershipPos.GetPositionX(), centershipPos.GetPositionY(), centershipPos.GetPositionZ(), SPELL_CAULDRON, true);
                events.ScheduleEvent(EVENT_THROW_FOOD, 5000);
                instance->SetBossState(DATA_CAPTAIN, IN_PROGRESS);
            }

            void JustDied(Unit* killer)
            {
                _JustDied();
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
                        case EVENT_THROW_FOOD:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM))
                            {
                                switch (urand(1, 13))
                                {
                                case 1:
                                    DoCast(target, DUNGEON_MODE(SPELL_THROW_FOOD_01, SPELL_THROW_FOOD_01_H));
                                    break;
                                case 2:
                                    DoCast(target, DUNGEON_MODE(SPELL_THROW_FOOD_02, SPELL_THROW_FOOD_02_H));
                                    break;
                                case 3:
                                    DoCast(target, DUNGEON_MODE(SPELL_THROW_FOOD_03, SPELL_THROW_FOOD_03_H));
                                    break;
                                case 4:
                                    DoCast(target, DUNGEON_MODE(SPELL_THROW_FOOD_04, SPELL_THROW_FOOD_04_H));
                                    break;
                                case 5:
                                    DoCast(target, DUNGEON_MODE(SPELL_THROW_FOOD_05, SPELL_THROW_FOOD_05_H));
                                    break;
                                case 6:
                                    DoCast(target, DUNGEON_MODE(SPELL_THROW_FOOD_06, SPELL_THROW_FOOD_06_H));
                                    break;
                                case 7:
                                    // саммонится непонятный мурлок, пока отключу
                                    //DoCast(target, DUNGEON_MODE(SPELL_THROW_FOOD_07, SPELL_THROW_FOOD_07_H));
                                    //break;
                                case 8:
                                    DoCast(target, DUNGEON_MODE(SPELL_THROW_FOOD_08, SPELL_THROW_FOOD_08_H));
                                    break;
                                case 9:
                                    DoCast(target, DUNGEON_MODE(SPELL_THROW_FOOD_09, SPELL_THROW_FOOD_09_H));
                                    break;
                                case 10:
                                    DoCast(target, DUNGEON_MODE(SPELL_THROW_FOOD_10, SPELL_THROW_FOOD_10_H));
                                    break;
                                case 11:
                                    DoCast(target, DUNGEON_MODE(SPELL_THROW_FOOD_11, SPELL_THROW_FOOD_11_H));
                                    break;
                                case 12:
                                    DoCast(target, DUNGEON_MODE(SPELL_THROW_FOOD_12, SPELL_THROW_FOOD_12_H));
                                    break;
                                case 13:
                                    DoCast(target, DUNGEON_MODE(SPELL_THROW_FOOD_13, SPELL_THROW_FOOD_13_H));
                                    break;
                                }
                            }
                            events.ScheduleEvent(EVENT_THROW_FOOD, 2100);
                            break;
                    }
                }
            }
        };
};

class npc_captain_cookie_cauldron : public CreatureScript
{
    public:
        npc_captain_cookie_cauldron() : CreatureScript("npc_captain_cookie_cauldron") { }

        CreatureAI* GetAI(Creature* pCreature) const
        {
            return new npc_captain_cookie_cauldronAI (pCreature);
        }

        struct npc_captain_cookie_cauldronAI : public ScriptedAI
        {
            npc_captain_cookie_cauldronAI(Creature* pCreature) : ScriptedAI(pCreature)
            {
                me->SetReactState(REACT_PASSIVE);
            }

            bool bReady;
            uint32 uiSeatTimer;

            void Reset() 
            {
                DoCast(me, SPELL_CAULDRON_VISUAL, true);
                DoCast(me, SPELL_CAULDRON_FIRE);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_STUNNED);
                bReady = false;
                uiSeatTimer = 1000;
            }

            void EnterCombat(Unit* /*who*/) {}

            void UpdateAI(uint32 diff) 
            {
                if (!bReady)
                {
                    if (uiSeatTimer <= diff)
                    {
                        bReady = true;
                        if (me->GetOwner())
                            me->GetOwner()->EnterVehicle(me);
                    }
                    else
                        uiSeatTimer -= diff;
                }
            }
        };
};

class npc_captain_cookie_good_food : public CreatureScript
{
    public:
        npc_captain_cookie_good_food() : CreatureScript("npc_captain_cookie_good_food") { }
     
        CreatureAI* GetAI(Creature* pCreature) const
        {
            return new npc_captain_cookie_good_foodAI (pCreature);
        }

        bool OnGossipHello(Player* pPlayer, Creature* pCreature)
        {
            InstanceScript* pInstance = pCreature->GetInstanceScript();
            if (!pInstance)
                return false;
            if (pInstance->GetData(DATA_CAPTAIN) != IN_PROGRESS)
                return false;
            switch (pInstance->instance->GetDifficulty())
            {
            case REGULAR_DIFFICULTY:
                pPlayer->CastSpell(pPlayer, SPELL_SETIATED, true);
                break;
            case HEROIC_DIFFICULTY:
                pPlayer->CastSpell(pPlayer, SPELL_SETIATED_H, true);
                break;
            }
            pCreature->DespawnOrUnsummon();
            return false;
        }

        struct npc_captain_cookie_good_foodAI : public ScriptedAI
        {
            npc_captain_cookie_good_foodAI(Creature *c) : ScriptedAI(c) 
            {
                pInstance = c->GetInstanceScript();
            }
     
            InstanceScript* pInstance;
            EventMap events;

            void Reset()
            {
                if (!pInstance)
                    return;

                DoCast(DUNGEON_MODE(SPELL_ROTTEN_AURA, SPELL_ROTTEN_AURA_H));
            }

            void JustDied(Unit* killer)
            {
                if (!pInstance)
                    return;

                me->DespawnOrUnsummon();
            }
            void UpdateAI(uint32 diff)
            {
                if (!pInstance)
                    return;

                if (pInstance->GetData(DATA_CAPTAIN) != IN_PROGRESS)
                    me->DespawnOrUnsummon();
            }
     
        };
};

class npc_captain_cookie_bad_food : public CreatureScript
{
    public:
        npc_captain_cookie_bad_food() : CreatureScript("npc_captain_cookie_bad_food") { }
     
        CreatureAI* GetAI(Creature* pCreature) const
        {
            return new npc_captain_cookie_bad_foodAI (pCreature);
        }
     
        bool OnGossipHello(Player* pPlayer, Creature* pCreature)
        {
            InstanceScript* pInstance = pCreature->GetInstanceScript();
            if (!pInstance)
                return false;
            if (pInstance->GetData(DATA_CAPTAIN) != IN_PROGRESS)
                return false;
            switch (pInstance->instance->GetDifficulty())
            {
                case REGULAR_DIFFICULTY:
                    pPlayer->CastSpell(pPlayer, SPELL_NAUSEATED, true);
                    break;
                case HEROIC_DIFFICULTY:
                    pPlayer->CastSpell(pPlayer, SPELL_NAUSEATED_H, true);
                    break;
            }
            pCreature->DespawnOrUnsummon();
            return false;
        }

        struct npc_captain_cookie_bad_foodAI : public ScriptedAI
        {
            npc_captain_cookie_bad_foodAI(Creature *c) : ScriptedAI(c) 
            {
                pInstance = c->GetInstanceScript();
            }
     
            InstanceScript* pInstance;
            EventMap events;

            void Reset()
            {
                if (!pInstance)
                    return;
            }

            void JustDied(Unit* killer)
            {
                if (!pInstance)
                    return;

                me->DespawnOrUnsummon();
            }
            void UpdateAI(uint32 diff)
            {
                if (!pInstance)
                    return;

                if (pInstance->GetData(DATA_CAPTAIN) != IN_PROGRESS)
                    me->DespawnOrUnsummon();
            }
     
        };
};

class spell_captain_cookie_setiated : public SpellScriptLoader
{
    public:
        spell_captain_cookie_setiated() : SpellScriptLoader("spell_captain_cookie_setiated") { }


        class spell_captain_cookie_setiated_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_captain_cookie_setiated_SpellScript);


            void HandleScript(SpellEffIndex /*effIndex*/)
            {
                if (!GetCaster() || !GetHitUnit())
                    return;
                GetHitUnit()->RemoveAuraFromStack(SPELL_NAUSEATED);
                GetHitUnit()->RemoveAuraFromStack(SPELL_NAUSEATED_H);
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_captain_cookie_setiated_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_captain_cookie_setiated_SpellScript();
        }
};

class spell_captain_cookie_nauseated : public SpellScriptLoader
{
    public:
        spell_captain_cookie_nauseated() : SpellScriptLoader("spell_captain_cookie_nauseated") { }


        class spell_captain_cookie_nauseated_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_captain_cookie_nauseated_SpellScript);


            void HandleScript(SpellEffIndex /*effIndex*/)
            {
                if (!GetCaster() || !GetHitUnit())
                    return;
                GetHitUnit()->RemoveAuraFromStack(SPELL_SETIATED);
                GetHitUnit()->RemoveAuraFromStack(SPELL_SETIATED_H);
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_captain_cookie_nauseated_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_captain_cookie_nauseated_SpellScript();
        }
};

void AddSC_boss_captain_cookie()
{
    new boss_captain_cookie();
    new npc_captain_cookie_cauldron();
    new npc_captain_cookie_good_food();
    new npc_captain_cookie_bad_food();
    new spell_captain_cookie_setiated();
    new spell_captain_cookie_nauseated();
}