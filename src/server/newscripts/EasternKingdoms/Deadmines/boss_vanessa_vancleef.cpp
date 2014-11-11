#include "NewScriptPCH.h"
#include "deadmines.h"

enum Spells
{
    SPELL_DEADLY_BLADES         = 92622,
    SPELL_BACKSLASH_TARGETING   = 92620,
    SPELL_BACKSLASH             = 92619,
    SPELL_DEFLECTION            = 92614,
    SPELL_VENGEANCE_OF_VANCLEEF = 95542,
    SPELL_POWDER_EXPLOSION      = 96283,
    SPELL_SITTING               = 89279,
};

enum Adds
{
    NPC_ROPE                        = 49550, // 95527

    // event
    NPC_VANESSA_SITTING             = 49429,
    NPC_VANESSA_EVENT               = 49671, // 48143 69676

    NPC_VANESSA_TRAP_BUNNY          = 49454,

    NPC_GLUBTOK_EVENT               = 49670,

    NPC_HELIX_EVENT                 = 49674,

    NPC_CHATTERING_HORROR           = 49495,
    NPC_DARKWEB_DEVOURER            = 49494,

    NPC_FOEREAPER_EVENT             = 49681,
    NPC_VANESSA_LIGHTNING_PLATTER   = 49520,
    NPC_VANESSA_LIGHTNING_STALKER   = 49521,

    NPC_RIPSNARL_EVENT              = 49682,
    NPC_ENRAGED_WORGEN              = 49532,
    NPC_ERIK_HARRINGTON             = 49535, // 2
    NPC_EMME_HARRINGTON             = 49534, // 1
    NPC_CALISSA_HARRINGTON          = 49536, // 3 92608
    NPC_JAMES_HARRINGTON            = 49539,
};

const Position vanessaeventPos[4] = 
{
    {-230.716f, -563.013f, 51.32f, 1.04f}

};

class boss_vanessa_vancleef : public CreatureScript
{
    public:
        boss_vanessa_vancleef() : CreatureScript("boss_vanessa_vancleef") { }

        CreatureAI* GetAI(Creature* pCreature) const
        {
            return GetAIForInstance< boss_vanessa_vancleefAI >(pCreature, DMScriptName);
        }

        struct boss_vanessa_vancleefAI : public BossAI
        {
            boss_vanessa_vancleefAI(Creature* pCreature) : BossAI(pCreature, DATA_VANESSA)
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
            }

            void EnterCombat(Unit* /*who*/) 
            {
                //Talk(SAY_AGGRO);
                DoZoneInCombat();
                instance->SetBossState(DATA_VANESSA, IN_PROGRESS);
            }

            void KilledUnit(Unit * victim)
            {
                //Talk(SAY_KILL);
            }

            void JustDied(Unit* /*killer*/)
            {
                _JustDied();
                //Talk(SAY_DEATH);
            }

            void UpdateAI(uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                events.Update(diff);

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                DoMeleeAttackIfReady();
            }
        };
};

void AddSC_boss_vanessa_vancleef()
{
    new boss_vanessa_vancleef();
}