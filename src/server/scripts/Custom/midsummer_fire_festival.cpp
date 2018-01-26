
#include "ScriptPCH.h"
#include "SpellAuras.h"
#include "SpellAuraEffects.h"
#include "Spell.h"

#define ACHIEVEMENT_FIRE_DANCING 271
#define ACHIEVEMENT_TORCH_JUGGLER 272
#define SPELL_JUGGLE_TORCH 45638
#define SPELL_TORCHES_CAUGHT 45693
#define GO_RIBBON_POLE 181605
const Position mfPos[22] =
{
    // Snarler spawn
    {-3924.37f, 6303.53f, 17.59f, 1.88f},
    {-4011.98f, 6416.34f, 14.75f, 3.73f},
    {-4097.92f, 6458.10f, 14.80f, 3.19f},
    {-4170.17f, 6503.14f, 13.41f, 1.55f},
    {-4266.80f, 6521.71f, 14.39f, 2.68f},
    {-4318.40f, 6601.40f, 9.853f, 1.47f},
    {-4056.26f, 6664.42f, 13.22f, 2.99f},
    {-4009.05f, 6561.04f, 17.15f, 4.37f},
    {-3932.42f, 6584.56f, 12.91f, 3.70f},
    {-3838.57f, 6461.64f, 11.91f, 3.92f},
    {-4268.83f, 6678.51f, 9.731f, 4.84f},
    // Dreadhowl spawn
    {-4225.82f, 6556.37f, 14.61f, 5.84f},
    {-4141.07f, 6523.72f, 16.81f, 6.06f},
    {-4073.94f, 6580.90f, 16.70f, 0.27f},
    {-3957.37f, 6617.45f, 12.66f, 0.43f},
    {-3865.21f, 6524.91f, 18.89f, 2.94f},
    {-3872.48f, 6498.26f, 17.90f, 3.39f},
    {-3914.52f, 6398.61f, 13.61f, 4.04f},
    {-4038.38f, 6514.68f, 13.36f, 3.01f},
    {-4344.90f, 6583.72f, 10.64f, 1.75f},
    {-4193.76f, 6122.50f, 13.00f, 6.06f},
    {-4082.68f, 6121.38f, 17.41f, 5.37f},
};

// 71992 - Moonfang <Darkmoon Den Mother>
class boss_darkmoon_moonfang_mother : public CreatureScript
{

enum eSay
{
    SAY_SUMM_SNARLER    = 0,
    SAY_SUMM_DREADHOWL  = 1,
    SAY_SUMM_MOTHER     = 2,
};

enum eCreatures
{
    NPC_MOONFANG_SNARLER        = 56160,
    NPC_MOONFANG_DREADHOWL      = 71982,
};

enum eSpells
{
    SPELL_LEAP_FOR_THE_KILL     = 144546,
    SPELL_FANGS_OF_THE_MOON     = 144700,
    SPELL_MOONFANG_TEARS        = 144702,
    SPELL_CALL_THE_PACK         = 144602,
    SPELL_MOONFANG_CURSE        = 144590,
};

    public:
        boss_darkmoon_moonfang_mother() : CreatureScript("boss_darkmoon_moonfang_mother") { }

        struct boss_darkmoon_moonfang_motherAI : public ScriptedAI
        {
            boss_darkmoon_moonfang_motherAI(Creature* creature) : ScriptedAI(creature), summons(me) 
            {
                me->SetVisible(false);
                prevEvent1 = true;
                prevEvent2 = false;
                sDiedCount = 0;
            }

            EventMap events;
            SummonList summons;

            bool prevEvent1;
            bool prevEvent2;
            uint8 sDiedCount;

            void Reset()
            {
                events.Reset();
                summons.DespawnAll();

                if (prevEvent1)
                {
                    SummonMoonfang();
                    ZoneTalk(SAY_SUMM_SNARLER);
                }
            }

            void SummonMoonfang()
            {
                if (prevEvent1)
                {
                    for (uint8 i = 0; i < 11; i++)
                    {
                        me->SummonCreature(NPC_MOONFANG_SNARLER, mfPos[i], TEMPSUMMON_CORPSE_TIMED_DESPAWN, 10000);
                    }
                }

                if (prevEvent2)
                {
                    for (uint8 i = 11; i < 22; i++)
                    {
                        me->SummonCreature(NPC_MOONFANG_DREADHOWL, mfPos[i], TEMPSUMMON_CORPSE_TIMED_DESPAWN, 10000);
                    }
                }
            }

            void SummonedCreatureDies(Creature* summon, Unit* /*killer*/)
            {
                if (prevEvent1 || prevEvent2)
                    sDiedCount++;
                
                if (sDiedCount == 11)
                {
                    prevEvent1 = false;
                    prevEvent2 = true;
                    ZoneTalk(SAY_SUMM_DREADHOWL);
                    SummonMoonfang();
                }
                if (sDiedCount == 22)
                {
                    prevEvent2 = false;
                    me->SetVisible(true);
                    ZoneTalk(SAY_SUMM_MOTHER);
                }
            }

            void EnterCombat(Unit* /*who*/)
            {
                events.ScheduleEvent(EVENT_1, 0);       // cast leap
                events.ScheduleEvent(EVENT_2, 10000);   // cast stuns the target
                events.ScheduleEvent(EVENT_3, 8000);    // cast tears AOE
                events.ScheduleEvent(EVENT_4, 64000);   // summon moonfangs
                events.ScheduleEvent(EVENT_5, 180000);  // cast mind control
            }

            void JustSummoned(Creature* summon)
            {
                summons.Summon(summon);
            }

            void UpdateAI(uint32 diff)
            {
                if (!UpdateVictim() && me->isInCombat())
                    return;
                
                events.Update(diff);
    
                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;
    
                if (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_1:
                            if (Unit* pTarget = SelectTarget(SELECT_TARGET_RANDOM, 1, 40.0f, true))
                                DoCast(pTarget, SPELL_LEAP_FOR_THE_KILL);
                            events.ScheduleEvent(EVENT_1, 12000);
                            break;
                        case EVENT_2:
                            if (Unit* pTarget = me->getVictim())
                                DoCast(pTarget, SPELL_FANGS_OF_THE_MOON);
                            events.ScheduleEvent(EVENT_2, 10000);
                            break;
                        case EVENT_3:
                            DoCast(SPELL_MOONFANG_TEARS);
                            events.ScheduleEvent(EVENT_3, 22000);
                            break;
                        case EVENT_4:
                            DoCast(SPELL_CALL_THE_PACK);
                            events.ScheduleEvent(EVENT_4, 64000);
                            break;
                        case EVENT_5:
                            DoCast(SPELL_MOONFANG_CURSE);
                            events.ScheduleEvent(EVENT_5, 180000);
                            break;
                        default:
                            break;
                    }
                }
                DoMeleeAttackIfReady();
            }
        };
        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_darkmoon_moonfang_motherAI(creature);
        }
};

// Darkmoon Faire Gnolls - 54444, 54466, 54549
class npc_darkmoon_faire_gnolls : public CreatureScript
{
    enum Spells
    {
        SPELL_WHACK             = 102022,
        SPELL_GNOLL_KILL_CREDIT = 101835,
    };

    public:
        npc_darkmoon_faire_gnolls() : CreatureScript("npc_darkmoon_faire_gnolls") { }

        struct npc_darkmoon_faire_gnollsAI : public Scripted_NoMovementAI
        {
            npc_darkmoon_faire_gnollsAI(Creature* c) : Scripted_NoMovementAI(c)
            {
                me->SetReactState(REACT_PASSIVE);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            }

            EventMap events;

            void Reset() 
            {
                me->DespawnOrUnsummon(3000);
            }

            void SpellHit(Unit* pCaster, const SpellInfo* Spell)
            {
                if (Spell->Id == SPELL_WHACK)
                {
                    DoCast(pCaster, SPELL_GNOLL_KILL_CREDIT);
                    me->Kill(me);
                }
            }
        };

        CreatureAI* GetAI(Creature *creature) const
        {
            return new npc_darkmoon_faire_gnollsAI(creature);
        }
};

// Darkmoon Faire Gnoll Holder - 54547
class npc_darkmoon_faire_gnoll_holder : public CreatureScript
{
    enum Events
    {
        EVENT_SUMMON_GNOLL = 1,
    };
    
    enum Spells
    {
        SPELL_SUMMON_GNOLL          = 102036,
        SPELL_SUMMON_GNOLL_BABY     = 102043,
        SPELL_SUMMON_GNOLL_BONUS    = 102044,
    };

    public:
        npc_darkmoon_faire_gnoll_holder() : CreatureScript("npc_darkmoon_faire_gnoll_holder") { }

        struct npc_darkmoon_faire_gnoll_holderAI : public Scripted_NoMovementAI
        {
            npc_darkmoon_faire_gnoll_holderAI(Creature* c) : Scripted_NoMovementAI(c)
            {
                me->SetReactState(REACT_PASSIVE);
            }
            
            EventMap events;

            void Reset() 
            {
                events.ScheduleEvent(EVENT_SUMMON_GNOLL, 3000);
            }
            
            void UpdateAI(uint32 diff) 
            {
                events.Update(diff);

                if (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_SUMMON_GNOLL:
                        {
                            std::list<Creature*> creatures;
                            me->GetCreatureListWithEntryInGrid(creatures, 54546, 40.0f);
                            if (!creatures.empty())
                            {
                                Trinity::Containers::RandomResizeList(creatures, 1);
                                uint32 Spells[3] = 
                                    {
                                        SPELL_SUMMON_GNOLL,
                                        SPELL_SUMMON_GNOLL_BABY,
                                        SPELL_SUMMON_GNOLL_BONUS,
                                    };
                                    uint8 rand = urand(0, 3);
                                if (Creature* pTarget = creatures.front())
                                    DoCast(pTarget, Spells[rand], true);
                            }
                            events.ScheduleEvent(EVENT_SUMMON_GNOLL, 2000);
                            break;
                        }
                    }
                }
            }
        };

        CreatureAI* GetAI(Creature *creature) const
        {
            return new npc_darkmoon_faire_gnoll_holderAI(creature);
        }
};

// Rinling - 14841
class npc_darkmoon_faire_rinling : public CreatureScript
{
    enum Events
    {
        EVENT_SHOTEVENT_ACTIVE  = 1,
        EVENT_SHOTEVENT_PAUSE   = 2,
    };
    
    enum Npcs
    {
        NPC_DARKMOON_FAIRE_TARGET   = 24171,
    };

    enum Spells
    {
        SPELL_GOSSIP_CREATE_RIFLE   = 101991,
        SPELL_INDICATOR             = 43313,
        SPELL_INDICATOR_QUICK_SHOT  = 101010,
    };

    public:
        npc_darkmoon_faire_rinling() : CreatureScript("npc_darkmoon_faire_rinling") { }

        bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action)
        {
            player->CLOSE_GOSSIP_MENU();

            if (action == 1)
                creature->CastSpell(player, SPELL_GOSSIP_CREATE_RIFLE, true);

            return true;
        }

        struct npc_darkmoon_faire_rinlingAI : public Scripted_NoMovementAI
        {
            npc_darkmoon_faire_rinlingAI(Creature* c) : Scripted_NoMovementAI(c)
            {
                me->SetReactState(REACT_PASSIVE);
            }
            
            EventMap events;

            void Reset() 
            {
                events.ScheduleEvent(EVENT_SHOTEVENT_ACTIVE, 3000);
            }
            
            void UpdateAI(uint32 diff) 
            {
                events.Update(diff);

                if (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_SHOTEVENT_ACTIVE:
                        {
                            std::list<Creature*> creatures;
                            me->GetCreatureListWithEntryInGrid(creatures, NPC_DARKMOON_FAIRE_TARGET, 15.0f);
                            if (!creatures.empty())
                            {
                                Trinity::Containers::RandomResizeList(creatures, 1);

                                if (Creature* pTarget = creatures.front())
                                {
                                    pTarget->CastSpell(pTarget, SPELL_INDICATOR, true);
                                    pTarget->CastSpell(pTarget, SPELL_INDICATOR_QUICK_SHOT, true);
                                }
                            }
                            events.ScheduleEvent(EVENT_SHOTEVENT_PAUSE, 3000);
                            break;
                        }
                        case EVENT_SHOTEVENT_PAUSE:
                        {
                            std::list<Creature*> creatures;
                            me->GetCreatureListWithEntryInGrid(creatures, NPC_DARKMOON_FAIRE_TARGET, 15.0f);
                            if (!creatures.empty())
                            {
                                for (std::list<Creature*>::iterator iter = creatures.begin(); iter != creatures.end(); ++iter)
                                    (*iter)->RemoveAura(SPELL_INDICATOR);
                            }
                            events.ScheduleEvent(EVENT_SHOTEVENT_ACTIVE, 3000);
                            break;
                        }
                    }
                }
            }
        };

        CreatureAI* GetAI(Creature *creature) const
        {
            return new npc_darkmoon_faire_rinlingAI(creature);
        }
};

// spell 45418
class spell_fire_dancing : public SpellScriptLoader
{
    public:
        spell_fire_dancing() : SpellScriptLoader("spell_fire_dancing") { }

        class spell_fire_dancingAuraScript : public AuraScript
        {
            PrepareAuraScript(spell_fire_dancingAuraScript);

            void HandleEffectRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                Unit* caster = GetCaster();
                // caster may be not avalible (logged out for example)
                if (!caster)
                    return;

                if (GameObject* Cage = caster->FindNearestGameObject(GO_RIBBON_POLE, 10))
                     if (AchievementEntry const *AchievWC = sAchievementStore.LookupEntry(ACHIEVEMENT_FIRE_DANCING))
                        caster->ToPlayer()->CompletedAchievement(AchievWC);
            }

            // function registering
            void Register()
            {
                OnEffectRemove += AuraEffectRemoveFn(spell_fire_dancingAuraScript::HandleEffectRemove, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL_OR_REAPPLY_MASK);
            }
        };

        // function which creates AuraScript
        AuraScript* GetAuraScript() const
        {
            return new spell_fire_dancingAuraScript();
        }
};

// spell 45693 - даем ачивку
class spell_torches_caught : public SpellScriptLoader
{
    public:
        spell_torches_caught() : SpellScriptLoader("spell_torches_caught") { }

        class spell_torches_caughtAuraScript : public AuraScript
        {
            PrepareAuraScript(spell_torches_caughtAuraScript);

            void OnStackChange(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                Unit* caster = GetCaster();
                // caster may be not avalible (logged out for example)
                if (caster)
                    if(GetStackAmount() >= 39)
                        if (AchievementEntry const *Achiev = sAchievementStore.LookupEntry(ACHIEVEMENT_TORCH_JUGGLER))
                            caster->ToPlayer()->CompletedAchievement(Achiev);
            }

            // function registering
            void Register()
            {
                OnEffectApply += AuraEffectApplyFn(spell_torches_caughtAuraScript::OnStackChange, EFFECT_1, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL_OR_REAPPLY_MASK);
            }
        };

        // function which creates AuraScript
        AuraScript* GetAuraScript() const
        {
            return new spell_torches_caughtAuraScript();
        }
};

// spell 45669
class spell_throw_torch : public SpellScriptLoader
{
    public:
        spell_throw_torch() : SpellScriptLoader("spell_throw_torch") { }

        class spell_throw_torch_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_throw_torch_SpellScript);

            void CheckTarget(SpellEffIndex effIndex)
            {
                PreventHitDefaultEffect(EFFECT_0);

                Unit* caster = GetCaster();
                Unit* target = caster->ToPlayer()->GetSelectedUnit();
                if(!caster)
                    return;
                if(target && target->GetTypeId() == TYPEID_PLAYER && target != caster)
                {
                    caster->CastSpell(caster, SPELL_TORCHES_CAUGHT, true);
                    target->CastSpell(caster, SPELL_JUGGLE_TORCH, true);
                }
            }

            void Register()
            {
                OnEffectHit += SpellEffectFn(spell_throw_torch_SpellScript::CheckTarget, EFFECT_0, SPELL_EFFECT_TRIGGER_MISSILE);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_throw_torch_SpellScript();
        }
};

class item_juggling_torch : public ItemScript
{
public:
    item_juggling_torch() : ItemScript("item_juggling_torch") { }

    bool OnUse(Player *pPlayer, Item *pItem, SpellCastTargets const& /*targets*/)
    {
        pPlayer->CastSpell(pPlayer->GetSelectedUnit(), 45669, true);

        pPlayer->DestroyItemCount(pItem->GetEntry(), 1, true);
        return true;
    }
};

// spell 45819
class spell_throw_torch2 : public SpellScriptLoader
{
    public:
        spell_throw_torch2() : SpellScriptLoader("spell_throw_torch2") { }

        class spell_throw_torch2_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_throw_torch2_SpellScript);

            void CheckTarget(SpellEffIndex effIndex)
            {
                PreventHitDefaultEffect(EFFECT_0);

                Unit* caster = GetCaster();
                if(!caster)
                    return;
                Unit* target = caster->getVictim();
                // caster may be not avalible (logged out for example)
                if(target)
                    caster->CastSpell(target, 45669, true);
            }

            void Register()
            {
                OnEffectHit += SpellEffectFn(spell_throw_torch2_SpellScript::CheckTarget, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_throw_torch2_SpellScript();
        }
};

// spell 45693 - даем ачивку
class spell_hawka : public SpellScriptLoader
{
    public:
        spell_hawka() : SpellScriptLoader("spell_hawka") { }

        class spell_hawkaAuraScript : public AuraScript
        {
            PrepareAuraScript(spell_hawkaAuraScript);

            void OnStackChange(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                Unit * target = GetTarget();
                // caster may be not avalible (logged out for example)
                if (target)
                    if(GetStackAmount() >= 5)
                    {
                        switch (GetId())
                        {
                            case 61841:
                                target->CastSpell(target, 66372, true);
                                break;
                            case 61842:
                                target->CastSpell(target, 66373, true);
                                break;
                            case 61843:
                                target->CastSpell(target, 66375, true);
                                break;
                            case 61844:
                                target->CastSpell(target, 66376, true);
                                break;
                            case 61845:
                                target->CastSpell(target, 66374, true);
                                break;
                            default:
                                break;
                        }
                        if(Aura* aura1 = target->GetAura(61841))
                        if(Aura* aura2 = target->GetAura(61842))
                        if(Aura* aura3 = target->GetAura(61843))
                        if(Aura* aura4 = target->GetAura(61844))
                        if(Aura* aura5 = target->GetAura(61845))
                        {
                            if(aura1->GetStackAmount() >= 5 && aura2->GetStackAmount() >= 5 && aura3->GetStackAmount() >= 5 && aura4->GetStackAmount() >= 5 && aura5->GetStackAmount() >= 5)
                            {
                                target->CastSpell(target, 61849, true);
                                aura1->SetStackAmount(0);
                                aura2->SetStackAmount(0);
                                aura3->SetStackAmount(0);
                                aura4->SetStackAmount(0);
                                aura5->SetStackAmount(0);
                            }
                        }
                    }
            }

            // function registering
            void Register()
            {
                OnEffectApply += AuraEffectApplyFn(spell_hawkaAuraScript::OnStackChange, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL_OR_REAPPLY_MASK);
            }
        };

        // function which creates AuraScript
        AuraScript* GetAuraScript() const
        {
            return new spell_hawkaAuraScript();
        }
};

// 62014
class spell_turkey_tracker : public SpellScriptLoader
{
    public:
        spell_turkey_tracker() : SpellScriptLoader("spell_turkey_tracker") { }

        class spell_turkey_trackerAuraScript : public AuraScript
        {
            PrepareAuraScript(spell_turkey_trackerAuraScript);

            void OnStackChange(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if (GetStackAmount() >= 39)
                    if (Unit* caster = GetCaster())
                        if (Player* player = caster->ToPlayer())
                        {
                            player->CastSpell(player, 62021, true);
                            GetAura()->Remove();
                        }
            }

            void Register()
            {
                OnEffectApply += AuraEffectApplyFn(spell_turkey_trackerAuraScript::OnStackChange, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL_OR_REAPPLY_MASK);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_turkey_trackerAuraScript();
        }
};

class spell_pass_the_turkey : public SpellScriptLoader
{
    public:
        spell_pass_the_turkey() : SpellScriptLoader("spell_pass_the_turkey") { }

        class spell_pass_the_turkey_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_pass_the_turkey_SpellScript);

            void Dummy(SpellEffIndex effIndex)
            {
                PreventHitDefaultEffect(EFFECT_0);

                if(Unit* caster = GetCaster())
                    if (Player* player = caster->GetCharmerOrOwnerPlayerOrPlayerItself())
                        if (AchievementEntry const *Achiev = sAchievementStore.LookupEntry(3579))
                            if (!player->HasAchieved(Achiev->ID))
                                player->CompletedAchievement(Achiev);
            }

            void Register()
            {
                OnEffectHit += SpellEffectFn(spell_pass_the_turkey_SpellScript::Dummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_pass_the_turkey_SpellScript();
        }
};

class spell_achiev_snow : public SpellScriptLoader
{
    public:
        spell_achiev_snow() : SpellScriptLoader("spell_achiev_snow") { }

        class spell_achiev_snow_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_achiev_snow_SpellScript);

            void Dummy(SpellEffIndex effIndex)
            {
                PreventHitDefaultEffect(EFFECT_0);

                Unit* target = GetHitUnit();
                if(!target || target->GetEntry() != 42928)
                    return;

                if(Unit* caster = GetCaster())
                    if (Player* player = caster->GetCharmerOrOwnerPlayerOrPlayerItself())
                        if (AchievementEntry const *Achiev = sAchievementStore.LookupEntry(1255))
                            player->CompletedAchievement(Achiev);
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_achiev_snow_SpellScript::Dummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_achiev_snow_SpellScript();
        }
};

class spell_darkmoon_cannon_prep : public SpellScriptLoader
{
    enum misc
    {
        NPC_DARKMOON_FAIRE_CANNON   = 15218,

        SPELL_DARKMOON_MAGIC_WINGS  = 102116,
    };

    public:
        spell_darkmoon_cannon_prep() : SpellScriptLoader("spell_darkmoon_cannon_prep") { }

        class spell_darkmoon_cannon_prep_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_darkmoon_cannon_prep_AuraScript);

            void OnApply(AuraEffect const* aurEff, AuraEffectHandleModes /*mode*/)
            {
                if (Unit* pCaster = GetCaster())
                    pCaster->SetControlled(true, UNIT_STATE_ROOT);
            }
            
            void OnRemove(AuraEffect const* aurEff, AuraEffectHandleModes /*mode*/)
            {
                if (Unit* pCaster = GetCaster())
                {
                    pCaster->SetControlled(false, UNIT_STATE_ROOT);

                    if (Creature* CanonTrigger = pCaster->FindNearestCreature(NPC_DARKMOON_FAIRE_CANNON, 30.0f))
                        CanonTrigger->CastSpell(pCaster, SPELL_DARKMOON_MAGIC_WINGS, true);
                }
            }

            void Register()
            {
                OnEffectApply += AuraEffectApplyFn(spell_darkmoon_cannon_prep_AuraScript::OnApply, EFFECT_0, SPELL_AURA_TRANSFORM, AURA_EFFECT_HANDLE_REAL);
                AfterEffectRemove += AuraEffectRemoveFn(spell_darkmoon_cannon_prep_AuraScript::OnRemove, EFFECT_0, SPELL_AURA_TRANSFORM, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_darkmoon_cannon_prep_AuraScript();
        }
};

//62244
class spell_darkmoon_cannonball : public SpellScriptLoader
{
    enum misc
    {
        NPC_DARKMOON_FAIRE_CANNON_TARGET    = 33068,

        SPELL_LANDING_RESULT_KILL_CREDIT    = 100962,
        SPELL_BULLSEYE                      = 62173,
        SPELL_GREAT_SHOT                    = 62175,
        SPELL_POOR_SHOT                     = 62179,
        SPELL_CANNONBALL                    = 62244,
    };

    public:
        spell_darkmoon_cannonball() : SpellScriptLoader("spell_darkmoon_cannonball") { }

        class spell_darkmoon_cannonball_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_darkmoon_cannonball_AuraScript);

            void HandlePeriodicTick(AuraEffect const* /*aurEff*/)
            {
                Unit* pCaster = GetCaster();
                if (!pCaster)
                    return;

                if (pCaster->IsInWater())
                    if (Creature* CanonTrigger = pCaster->FindNearestCreature(NPC_DARKMOON_FAIRE_CANNON_TARGET, 30.0f))
                    {
                        pCaster->CastSpell(pCaster, GetSpellInfo()->Effects[EFFECT_0]->BasePoints, true);

                        if (pCaster->IsInRange(CanonTrigger, 0.0f, 1.0f))
                        {
                            for (uint8 i = 0; i < 5; ++i)
                                pCaster->CastSpell(pCaster, SPELL_LANDING_RESULT_KILL_CREDIT, true);

                            pCaster->CastSpell(pCaster, SPELL_BULLSEYE, true);
                        }
                        else if (pCaster->IsInRange(CanonTrigger, 1.0f, 4.0f))
                        {
                            for (uint8 i = 0; i < 3; ++i)
                                pCaster->CastSpell(pCaster, SPELL_LANDING_RESULT_KILL_CREDIT, true);

                            pCaster->CastSpell(pCaster, SPELL_GREAT_SHOT, true);
                        }
                        else if (pCaster->IsInRange(CanonTrigger, 4.0f, 15.0f))
                        {
                            pCaster->CastSpell(pCaster, SPELL_POOR_SHOT, true);
                        }

                        pCaster->RemoveAura(SPELL_CANNONBALL);
                    }
            }

            void Register()
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_darkmoon_cannonball_AuraScript::HandlePeriodicTick, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_darkmoon_cannonball_AuraScript();
        }
};

// Cannon - 102292
class spell_darkmoon_faire_cannon : public SpellScriptLoader
{
    enum misc
    {
        NPC_TONK_TARGET    = 33081,

        SPELL_TONK_TARGET_ = 62265,
    };

    public:
        spell_darkmoon_faire_cannon() : SpellScriptLoader("spell_darkmoon_faire_cannon") { }

        class spell_darkmoon_faire_cannon_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_darkmoon_faire_cannon_SpellScript);

            void HandleHitTarget(SpellEffIndex /*effIndex*/)
            {
                PreventHitDefaultEffect(EFFECT_0);

                Unit* caster = GetCaster()->GetOwner();
                Unit* target = GetHitUnit()->ToCreature();

                if(!caster || !target)
                    return;

                if (target->IsAlive() && target->GetEntry() == NPC_TONK_TARGET)
                    caster->CastSpell(caster, SPELL_TONK_TARGET_, true);
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_darkmoon_faire_cannon_SpellScript::HandleHitTarget, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_darkmoon_faire_cannon_SpellScript();
        }
};

// Ring Toss - 101695
class spell_darkmoon_ring_toss : public SpellScriptLoader
{
    enum misc
    {
        NPC_DUBENKO     = 54490,

        SPELL_RING_TOSS_HIT     = 101699,
        SPELL_RING_TOSS_MISS    = 101697,
        SPELL_RING_TOSS_MISS_2  = 101698,
    };

    public:
        spell_darkmoon_ring_toss() : SpellScriptLoader("spell_darkmoon_ring_toss") { }

        class spell_darkmoon_ring_toss_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_darkmoon_ring_toss_SpellScript);

            void HandleHitTarget(SpellEffIndex /*effIndex*/)
            {
                PreventHitDefaultEffect(EFFECT_0);

                Unit* caster = GetCaster();
                Unit* target = GetHitUnit()->ToCreature();
                if(!caster || !target)
                    return;

                if (target->GetEntry() == NPC_DUBENKO)
                {
                    float distance = target->GetDistance2d(GetExplTargetDest()->GetPositionX(), GetExplTargetDest()->GetPositionY());

                    if (distance < 0.5f)
                        caster->CastSpell(target, SPELL_RING_TOSS_HIT, true);

                    else if (distance < 2.0f)
                        caster->CastSpell(target, SPELL_RING_TOSS_MISS, true);

                    else if (distance < 4.0f)
                        caster->CastSpell(target, SPELL_RING_TOSS_MISS_2, true);
                }
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_darkmoon_ring_toss_SpellScript::HandleHitTarget, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_darkmoon_ring_toss_SpellScript();
        }
};

class spell_darkmoon_deathmatch_teleport : public SpellScriptLoader
{
    enum Spells
    {
        SPELL_DARKMOON_DEATHMATCH_TELE_1 = 108919,
        SPELL_DARKMOON_DEATHMATCH_TELE_2 = 113212,
        SPELL_DARKMOON_DEATHMATCH_TELE_3 = 113213,
        SPELL_DARKMOON_DEATHMATCH_TELE_4 = 113216,
        SPELL_DARKMOON_DEATHMATCH_TELE_5 = 113219,
        SPELL_DARKMOON_DEATHMATCH_TELE_6 = 113224,
        SPELL_DARKMOON_DEATHMATCH_TELE_7 = 113227,
        SPELL_DARKMOON_DEATHMATCH_TELE_8 = 113228,
    };

    public:
        spell_darkmoon_deathmatch_teleport() : SpellScriptLoader("spell_darkmoon_deathmatch_teleport") { }

        class spell_darkmoon_deathmatch_teleport_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_darkmoon_deathmatch_teleport_SpellScript);
            
            void HandleScript(SpellEffIndex effIndex)
            {
                Player* pTarget = GetHitUnit()->ToPlayer();
                if (!pTarget)
                    return;

                uint32 Spells[8] = 
                {
                    SPELL_DARKMOON_DEATHMATCH_TELE_1,
                    SPELL_DARKMOON_DEATHMATCH_TELE_2,
                    SPELL_DARKMOON_DEATHMATCH_TELE_3,
                    SPELL_DARKMOON_DEATHMATCH_TELE_4,
                    SPELL_DARKMOON_DEATHMATCH_TELE_5,
                    SPELL_DARKMOON_DEATHMATCH_TELE_6,
                    SPELL_DARKMOON_DEATHMATCH_TELE_7,
                    SPELL_DARKMOON_DEATHMATCH_TELE_8,
                };
                uint8 rand = urand(0, 8);

                pTarget->CastSpell(pTarget, Spells[rand]);
            }
            
            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_darkmoon_deathmatch_teleport_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_darkmoon_deathmatch_teleport_SpellScript();
        }
};

void AddSC_midsummer_fire_festival()
{
    new boss_darkmoon_moonfang_mother();
    new npc_darkmoon_faire_gnolls();
    new npc_darkmoon_faire_gnoll_holder();
    new npc_darkmoon_faire_rinling();
    new spell_fire_dancing;
    new spell_torches_caught;
    new spell_throw_torch;
    new spell_throw_torch2;
    new item_juggling_torch;
    new spell_hawka;
    new spell_turkey_tracker();
    new spell_pass_the_turkey();
    new spell_achiev_snow();
    new spell_darkmoon_cannon_prep();
    new spell_darkmoon_cannonball();
    new spell_darkmoon_faire_cannon();
    new spell_darkmoon_ring_toss();
    new spell_darkmoon_deathmatch_teleport();
}
