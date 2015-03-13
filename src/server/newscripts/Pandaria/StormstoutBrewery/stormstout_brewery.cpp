/*
    Dungeon : Stormstout Brewery 85-87
    Instance General Script
*/

#include "NewScriptPCH.h"
#include "stormstout_brewery.h"

class npc_golden_hopling : public CreatureScript
{
    public:
        npc_golden_hopling() : CreatureScript("npc_golden_hopling") {}

        struct npc_golden_hoplingAI : public Scripted_NoMovementAI
        {
            npc_golden_hoplingAI(Creature* creature) : Scripted_NoMovementAI(creature)
            {
                instance = creature->GetInstanceScript();
                OneClick = false;
            }

            InstanceScript* instance;
            bool OneClick;
            
            void OnSpellClick(Unit* /*clicker*/)
            {
                if (instance && !OneClick)
                {
                    OneClick = true;
                    uint32 HoplingCount = instance->GetData(DATA_GOLDEN_HOPLING) + 1;
                    instance->SetData(DATA_GOLDEN_HOPLING, HoplingCount);
                    if (HoplingCount >= 30)
                        DoCast(SPELL_ACHIEV_CREDIT);
                    me->DespawnOrUnsummon();
                }
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_golden_hoplingAI(creature);
        }
};

class npc_big_ol_hammer : public CreatureScript
{
    public:
        npc_big_ol_hammer() : CreatureScript("npc_big_ol_hammer") {}

        struct npc_big_ol_hammerAI : public Scripted_NoMovementAI
        {
            npc_big_ol_hammerAI(Creature* creature) : Scripted_NoMovementAI(creature)
            {
                instance = creature->GetInstanceScript();
                OneClick = false;
            }

            InstanceScript* instance;
            bool OneClick;
            
            void OnSpellClick(Unit* clicker)
            {
                if (instance && !OneClick)
                {
                    OneClick = true;

                    for (uint8 i = 0; i < 3; ++i)
                        clicker->CastSpell(clicker, SPELL_SMASH_OVERRIDE);

                    me->DespawnOrUnsummon();
                }
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_big_ol_hammerAI(creature);
        }
};

class spell_stormstout_brewery_habanero_beer : public SpellScriptLoader
{
    public:
        spell_stormstout_brewery_habanero_beer() : SpellScriptLoader("spell_stormstout_brewery_habanero_beer") { }

        class spell_stormstout_brewery_habanero_beer_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_stormstout_brewery_habanero_beer_SpellScript);

            void HandleInstaKill(SpellEffIndex /*effIndex*/)
            {
                if (!GetCaster())
                    return;

                std::list<Creature*> creatureList;

                GetCreatureListWithEntryInGrid(creatureList, GetCaster(), NPC_BARREL, 10.0f);

                GetCaster()->RemoveAurasDueToSpell(SPELL_PROC_EXPLOSION);

                for (std::list<Creature*>::const_iterator itr = creatureList.begin(); itr != creatureList.end(); ++itr)
                {
                    if ((*itr)->HasAura(SPELL_PROC_EXPLOSION))
                    {
                        (*itr)->RemoveAurasDueToSpell(SPELL_PROC_EXPLOSION);
                        (*itr)->CastSpell(*itr, GetSpellInfo()->Id, true);
                    }
                }
            }

            void HandleAfterCast()
            {
                if (Unit* caster = GetCaster())
                    if (caster->ToCreature())
                        caster->ToCreature()->ForcedDespawn(1000);
            }

            void Register()
            {
                OnEffectHit += SpellEffectFn(spell_stormstout_brewery_habanero_beer_SpellScript::HandleInstaKill, EFFECT_1, SPELL_EFFECT_INSTAKILL);
                AfterCast += SpellCastFn(spell_stormstout_brewery_habanero_beer_SpellScript::HandleAfterCast);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_stormstout_brewery_habanero_beer_SpellScript();
        }
};

void AddSC_stormstout_brewery()
{
    new npc_golden_hopling();
    new npc_big_ol_hammer();
    new spell_stormstout_brewery_habanero_beer();
}