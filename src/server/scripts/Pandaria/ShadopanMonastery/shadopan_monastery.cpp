/*
    Dungeon : Shandopan Monastery 87-89
    Instance General Script
*/

#include "shadopan_monastery.h"

enum eSpells
{
    SPELL_CRISE                 = 128248,
    SPELL_ICE_ARROW             = 126114,
    SPELL_EXPLOSION_DAMAGE      = 106966,
    SPELL_PURIFICATION_RITUAL   = 111690,
};

class npc_shado_pan_ambusher : public CreatureScript
{
    public:
        npc_shado_pan_ambusher() :  CreatureScript("npc_shado_pan_ambusher") { }

        struct npc_shado_pan_ambusherAI : public ScriptedAI
        {
            npc_shado_pan_ambusherAI(Creature* creature) : ScriptedAI(creature) {}

            uint32 criseTimer;
            bool inFight;

            void Reset()
            {
                criseTimer = 5000;
                inFight = false;
            }

            void MoveInLineOfSight(Unit* who)
            {
                if (!inFight && me->GetDistance(who) < 35.0f)
                {
                    inFight = true;
                    me->GetMotionMaster()->MoveJump(who->GetPositionX(), who->GetPositionY(), who->GetPositionZ(), 20.0f, 20.0f);
                }
            }

            void UpdateAI(const uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                if (criseTimer <= diff)
                {
                    DoZoneInCombat();
                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 0, true))
                    {
                        me->getThreatManager().addThreat(target, 1000000.0f);
                        me->CastSpell(me, SPELL_CRISE, true);
                    }

                    criseTimer = 500;
                }
                else criseTimer -= diff;
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_shado_pan_ambusherAI(creature);
        }
};

class npc_shado_pan_archery : public CreatureScript
{
    public:
        npc_shado_pan_archery() :  CreatureScript("npc_shado_pan_archery") { }

        struct npc_shado_pan_archeryAI : public ScriptedAI
        {
            npc_shado_pan_archeryAI(Creature* creature) : ScriptedAI(creature)
            {
                pInstance = creature->GetInstanceScript();
            }

            uint16 fireTimer;
            InstanceScript* pInstance;

            void Reset()
            {
                fireTimer = urand(2000, 4000);
                me->setActive(true);
            }

            void UpdateAI(const uint32 diff)
            {
                if (!pInstance || !pInstance->GetData(DATA_ARCHERY))
                    return;
                
                if (fireTimer <= diff)
                {
                    uint64 targetGuid = 0;

                    if (pInstance->GetData(DATA_ARCHERY) == 1 && me->GetEntry() == NPC_ARCHERY_FIRST)
                        targetGuid = pInstance->GetData64(NPC_ARCHERY_TARGET);
                    else if (pInstance->GetData(DATA_ARCHERY) == 2 && me->GetEntry() == NPC_ARCHERY_SECOND)
                    {
                        Map::PlayerList const& playerList = pInstance->instance->GetPlayers();
                        Map::PlayerList::const_iterator Itr = playerList.begin();

                        uint8 advance = urand(0, playerList.getSize() - 1);
                        for (uint8 i = 0; i < advance; ++i, ++Itr);

                        if (Player* player = Itr->getSource())
                            targetGuid = player->GetGUID();
                    }

                    if (Unit* target = ObjectAccessor::FindUnit(targetGuid))
                        me->CastSpell(target, SPELL_ICE_ARROW, false);

                    fireTimer = urand(1000, 2000);
                }
                else fireTimer -= diff;
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_shado_pan_archeryAI(creature);
        }
};

class spell_shadopan_explosion : public SpellScriptLoader
{
    public:
        spell_shadopan_explosion() : SpellScriptLoader("spell_shadopan_explosion") { }

        class spell_shadopan_explosion_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_shadopan_explosion_AuraScript);

            void OnRemove(constAuraEffectPtr, AuraEffectHandleModes)
            {
                if (GetTargetApplication()->GetRemoveMode() == AURA_REMOVE_BY_DEATH)
                    if (Unit* caster = GetCaster())
                        caster->CastSpell(caster, SPELL_EXPLOSION_DAMAGE, true);
            }

            void Register()
            {
                OnEffectRemove += AuraEffectRemoveFn(spell_shadopan_explosion_AuraScript::OnRemove, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_shadopan_explosion_AuraScript();
        }
};

class spell_shadopan_apparitions : public SpellScriptLoader
{
    public:
        spell_shadopan_apparitions() : SpellScriptLoader("spell_shadopan_apparitions") { }

        class spell_shadopan_apparitions_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_shadopan_apparitions_AuraScript);

            void OnPeriodic(constAuraEffectPtr aurEff)
            {
                PreventDefaultAction();

                if (Unit* caster = GetCaster())
                {
                    if (caster->HasAura(SPELL_PURIFICATION_RITUAL))
                    {
                        GetAura()->Remove();
                        return;
                    }

                    std::list<Creature*> hatredList;
                    
                    caster->GetCreatureListWithEntryInGridAppend(hatredList, NPC_RESIDUAL_OF_HATRED, 20.0f);
                    caster->GetCreatureListWithEntryInGridAppend(hatredList, NPC_VESTIGE_OF_HATRED,  20.0f);
                    caster->GetCreatureListWithEntryInGridAppend(hatredList, NPC_FRAGMENT_OF_HATRED, 20.0f);

                    for (auto hatred: hatredList)
                        if (hatred->isAlive())
                            hatred->CastSpell(hatred, GetSpellInfo()->Effects[EFFECT_0].TriggerSpell, true);
                }
            }

            void Register()
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_shadopan_apparitions_AuraScript::OnPeriodic, EFFECT_0, SPELL_AURA_PERIODIC_TRIGGER_SPELL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_shadopan_apparitions_AuraScript();
        }
};

enum areaTrigger
{
    AREATRIGGER_ARCHERY_FIRST_BEGIN     = 8271,
    AREATRIGGER_ARCHERY_FIRST_END       = 8272,
    AREATRIGGER_ARCHERY_SECOND_FIRST    = 7121,
    AREATRIGGER_ARCHERY_SECOND_END      = 7126,
};

class areatrigger_at_shadopan_archery : public AreaTriggerScript
{
    public:

        areatrigger_at_shadopan_archery() : AreaTriggerScript("areatrigger_at_shadopan_archery") {}

        bool OnTrigger(Player* player, AreaTriggerEntry const* trigger)
        {
            InstanceScript* pInstance = player->GetInstanceScript();

            if (!pInstance)
                return false;

            switch(trigger->id)
            {
                case AREATRIGGER_ARCHERY_FIRST_BEGIN:
                    pInstance->SetData(DATA_ARCHERY, 1);
                    break;
                case AREATRIGGER_ARCHERY_FIRST_END:
                    pInstance->SetData(DATA_ARCHERY, 0);
                    break;
                case AREATRIGGER_ARCHERY_SECOND_FIRST:
                    pInstance->SetData(DATA_ARCHERY, 2);
                    break;
                case AREATRIGGER_ARCHERY_SECOND_END:
                    pInstance->SetData(DATA_ARCHERY, 0);
                    break;
            }

            return false;
        }
};

void AddSC_shadopan_monastery()
{
    new npc_shado_pan_ambusher();
    new npc_shado_pan_archery();
    new spell_shadopan_explosion();
    new spell_shadopan_apparitions();
    new areatrigger_at_shadopan_archery();
}
