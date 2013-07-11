/*
 * Copyright © 2008-2012 Holystone Productions>
 * Copyright © 2008-2012 Northstrider>
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
Complete:   35%

Todo:
[100%]- Implement areatrigger for Intro
[100%]- Implement intro
[100%]- Fix stand state and visual of Ragnaros' pool phase
[100%]- Sulfuras Smash
[100%]- Magma Trap
[100%]- Hand of Ragnaros
[100%]- Engulfing Flames
[100%]- Splitting Blast
[100%]- Phase 1
[90%]-  Phase 2
[70%]-  Phase 3
[89%]-  Normal Mode Death at 10% hp
[75%]-  Switch Phase
[95%]-   World in Flames
[98%]-   Molten Seed
[0%]-   Living Meteor
[0%]-   Heroic Mode Switch
[??%]-  What's left todo ?
*/

#include "ObjectMgr.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "SpellAuraEffects.h"
#include "GridNotifiers.h"
#include "firelands.h"

enum Texts
{
    SAY_ARRIVE          = 0,
    SAY_DEATH_NORMAL    = 1,
    SAY_DEATH_HEROIC    = 2,
    SAY_SLAY            = 3,
    SAY_ANNOUNCE_SPLIT  = 4,
    SAY_SUBMERGE        = 5,
    SAY_EMERGE_ANNOUNCE = 6,
    SAY_EMERGE          = 7,
    SAY_MAGMA_TRAP      = 8,
    SAY_SULFURAS_SMASH  = 9,
    SAY_AGRRO           = 10,
};

enum Spells
{
    //Ragnaros
    SPELL_BASE_VISUAL                       = 98860,
    SPELL_BURNING_WOUNDS_AURA               = 99401,
    SPELL_SULFURAS_SMASH                    = 98710,
    SPELL_SUMMON_SULFURAS_SMASH_TARGET      = 98706,
    SPELL_SULFURAS_SMASH_EXPLOSION          = 98708,
    SPELL_SULFURAS_SMASH_VISUAL             = 98712,
    SPELL_MAGMA_TRAP                        = 98164,
    SPELL_MAGMA_BLAST                       = 98313,
    SPELL_HAND_OF_RAGNAROS                  = 98237,
    SPELL_WRATH_OF_RAGNAROS                 = 98263,
    SPELL_SUBMERGED                         = 98982,
    SPELL_DISABLE_ANIM                      = 16245, // not correct but a good temp solution
    SPELL_SPLITTING_BLOW_EAST               = 98953,
    SPELL_SPLITTING_BLOW_NORTH              = 98952,
    SPELL_SPLITTING_BLOW_WEST               = 98951,
    SPELL_ENGULFING_FLAMES_BOTTOM           = 99236,
    SPELL_ENGULFING_FLAMES_CENTER           = 99235,
    SPELL_ENGULFING_FLAMES_MELEE            = 99172,
    SPELL_ENGULFING_FLAMES_VISUAL_MELEE     = 99216,
    SPELL_ENGULFING_FLAMES_VISUAL_CENTER    = 99217,
    SPELL_ENGULFING_FLAMES_VISUAL_BOTTOM    = 99218,
    SPELL_ENGULFING_FLAMES_EXPLOSION        = 99225,
    SPELL_MOLTEN_SEED                       = 98498,
    SPELL_MOLTEN_SEED_VISUAL                = 98520,
    SPELL_MOLTEN_SEED_TRIGGER               = 98333,
    SPELL_MOLTEN_SEED_ELEMENTAL_SPAWN       = 100141,
    SPELL_MOLTEN_SEED_EXPLOGEN              = 98495,
    SPELL_MOLTEN_INFERNO                    = 98518,
    SPELL_LIVING_METEOR                     = 99268,

    //Magma Trap
    SPELL_MAGMA_TRAP_VISUAL                 = 98179,
    SPELL_MAGMA_ERRUPTION                   = 98175,

    //Sulfuras Hand of Ragnaros
    SPELL_SULFURAS_KNOCKBACK                = 100455,
    SPELL_FLAMES_OF_SULFURAS                = 99112,
    SPELL_SULFURAS_AURA                     = 100456,

    //Splitting Blow
    SPELL_CALL_SONS                         = 99054,
    SPELL_CALL_SONS_MISSILE                 = 99050,

    //Lava Wave
    SPELL_LAVA_WAVE_VISUAL                  = 98873,
    SPELL_LAVA_WAVE_DAMAGE                  = 98928,

    //Son of Flame
    SPELL_HIT_ME                            = 100446,
    SPELL_PRE_VISUAL                        = 100134,
    SPELL_BURNING_SPEED                     = 99414,

    //Living Meteor
    SPELL_LIVING_METEOR_COMBUSTION          = 99303,
    SPELL_LIVING_METEOR_BUFF                = 99287,
    SPELL_LIVING_METEOR_VISUAL              = 99215,
    SPELL_LIVING_METEOR_SPEED               = 100277,
    SPELL_LIVING_METEOR_FIXATE              = 99849,
};

enum Adds
{
    NPC_SULFURAS_SMASH_1            = 53266,
    NPC_SULFURAS_SMASH_2            = 53268,
    NPC_LAVA_SCION                  = 53231,
    NPC_SULFURAS_HAND_OF_RAGNAROS_1 = 53420,
    NPC_SULFURAS_HAND_OF_RAGNAROS_2 = 53419,
    NPC_PLATFORM_STALKER            = 53952,
    NPC_SPLITTING_BLOW              = 53393,
    NPC_MAGMA_TRAP                  = 53086,
    NPC_SON_OF_FLAME                = 53140,
    NPC_LAVA_WAVE                   = 53363,
    NPC_MOLTEN_SEED_CASTER          = 53186,
    NPC_ENGULFING_FLAMES            = 53485,
    NPC_MOLTEN_ELEMENTAL            = 53189,
    NPC_MAGMA                       = 53729,
    NPC_BLAZING_HEAT                = 53473,
    NPC_LIVING_METEOR               = 53500,
    NPC_DREADFLAME_SPAWN            = 54203,
    NPC_DREADFLAME                  = 54127,
    NPC_MAGMA_GEYSER                = 54184, // noot need in 4.3

    NPC_MALFURION_STORMRAGE         = 53875,
    NPC_CLOUDBURST                  = 54147,
    NPC_HAMUUL_RUNETOTEM            = 53876,
    NPC_ENTRAPPING_ROOTS            = 54074,
    NPC_CENARIUS                    = 53872,
    NPC_BREADTH_OF_FROST            = 53953,
};

enum Phases
{
    PHASE_INTRO             = 1,
    PHASE_1                 = 2,
    PHASE_2                 = 3,
    PHASE_3                 = 4,
    PHASE_SUBMERGED         = 5,
    PHASE_HEROIC            = 6,
    PHASE_MASK_NO_VICTIM    = 1 << PHASE_INTRO
};

enum Events
{
    //Intro
    EVENT_ARRIVE_1                  = 1,
    EVENT_ARRIVE_2                  = 2,

    //Encounter
    EVENT_SULFURAS_SMASH_TRIGGER    = 3,
    EVENT_SULFURAS_SMASH            = 4,
    EVENT_MAGMA_TRAP                = 5,
    EVENT_HAND_OF_RAGNAROS          = 6,
    EVENT_WRATH_OF_RAGNAROS         = 7,
    EVENT_EMERGE                    = 8,
    EVENT_SUBMERGE                  = 9,
    EVENT_SPLITTING_BLOW            = 10,
    EVENT_ATTACK                    = 11,
    EVENT_ENGULFING_FLAMES          = 12,
    EVENT_SUMMON_WAVES              = 13,
    EVENT_MOLTEN_SEED               = 14,
    EVENT_LIVING_METEOR             = 15,
    EVENT_DESPAWN                   = 16,
    EVENT_MOLTEN_SEED_START         = 17,
    EVENT_MOLTEN_SEED_VISUAL        = 18,
    EVENT_MOLTEN_SEED_END           = 19,
    //Magma Trap
    EVENT_PREPARE                   = 30,
    //Engulfing Flames
    EVENT_EXPLODE                   = 31,
    //Sulfuras
    EVENT_SUMMON_SONS               = 32,
    //Son of Flame
    EVENT_ACTIVATE                  = 33,
    EVENT_TRIGGER                   = 34,
    EVENT_BOOM                      = 35,
    EVENT_MAGMA_BLAST               = 36,

    //Living Meteor
    EVENT_CHANGE_TARGET             = 37,
    EVENT_CHANGE_SPEED              = 38,
    EVENT_LAVA_UP                   = 39,
    EVENT_LAVA_DOWN                 = 40,
    EVENT_ACTIVATE2                 = 41,
};

Position const RagnarosSummonPosition = {1075.201f, -57.84896f, 55.42427f,  3.159046f   };
Position const SplittingTriggerNorth  = {1023.55f,  -57.158f,   55.4215f,   3.12414f    };
Position const SplittingTriggerEast   = {1035.45f,  -25.3646f,  55.4924f,   2.49582f    };
Position const SplittingTriggerWest   = {1036.27f,  -89.2396f,  55.5098f,   3.83972f    };
Position const CachePosition          = {1016.043f, -57.436f,   55.333f,    3.151f      };

const Position HammerMiddleSummons[] =
{
    //West Side
    {1008.976f, -87.395f,  55.452f, 1.030f},
    {1037.130f, -101.037f, 55.544f, 2.130f},
    {1057.177f, -103.765f, 55.342f, 2.330f},
    {1076.355f, -101.017f, 55.342f, 2.677f},
    //East Side
    {1012.901f, -26.540f,  55.482f, 4.874f},
    {1037.587f, -13.490f,  55.555f, 4.658f},
    {1055.858f, -11.348f,  55.346f, 3.927f},
    {1074.467f, -12.893f, 55.342f,  3.715f},
};

const Position HammerWestSummons[] =
{
    //West Side
    {999.768f,  -69.833f,  55.485f, 5.887f},
    {1057.177f, -103.765f, 55.342f, 2.330f},
    {1076.355f, -101.017f, 55.342f, 2.677f},
    //East Side
    {999.505f,  -45.725f,  55.476f, 5.435f},
    {1012.901f, -26.540f,  55.482f, 4.874f},
    {1037.587f, -13.490f,  55.555f, 4.658f},
    {1055.858f, -11.348f,  55.346f, 3.927f},
    {1074.467f, -12.893f,  55.342f, 3.715f},
};

const Position HammerEastSummons[] =
{
    //West Side
    {999.768f,  -69.833f,  55.485f, 5.887f},
    {1008.976f, -87.395f,  55.452f, 1.030f},
    {1037.130f, -101.037f, 55.544f, 2.130f},
    {1057.177f, -103.765f, 55.342f, 2.330f},
    {1076.355f, -101.017f, 55.342f, 2.677f},
    //East Side
    {999.505f,  -45.725f,  55.476f, 5.435f},
    {1055.858f, -11.348f,  55.346f, 3.927f},
    {1074.467f, -12.893f, 55.342f,  3.715f},
};

const Position EngulfingFlamesMelee[] =
{
    {1086.55f, -18.0885f, 55.4228f, 1.57080f},
    {1091.83f, -21.9254f, 55.4241f, 4.71239f},
    {1092.52f, -92.3924f, 55.4241f, 4.71239f},
    {1079.15f, -15.5312f, 55.4230f, 4.71239f},
    {1078.01f, -97.7760f, 55.4227f, 1.57080f},
    {1065.44f, -17.7049f, 55.4250f, 5.00910f},
    {1063.59f, -97.0573f, 55.4934f, 1.23918f},
    {1051.80f, -24.0903f, 55.4258f, 5.41052f},
    {1049.27f, -90.6892f, 55.4259f, 0.89011f},
    {1042.34f, -32.1059f, 55.4254f, 5.68977f},
    {1041.26f, -81.4340f, 55.4240f, 0.57595f},
    {1036.82f, -44.3385f, 55.4425f, 6.02139f},
    {1036.34f, -69.8281f, 55.4425f, 0.31415f},
    {1034.76f, -63.9583f, 55.4397f, 6.26573f},
    {1033.93f, -57.0920f, 55.4225f, 6.26573f},
    {1086.42f, -96.7812f, 55.4226f, 1.57080f},
};

const Position EngulfingFlamesRange[] =
{
    {1035.17f, -125.646f, 55.4471f, 0.0f},
    {1032.48f,  13.2708f, 55.4469f, 0.0f},
    {1023.83f,  12.9774f, 55.4470f, 0.0f},
    {1023.05f, -128.257f, 55.4471f, 0.0f},
    {1019.60f,  7.76910f, 55.4470f, 0.0f},
    {1018.29f, -117.833f, 55.4471f, 0.0f},
    {1012.70f, -4.83333f, 55.6050f, 0.0f},
    {1009.56f, -108.161f, 55.4697f, 0.0f},
    {1005.80f, -8.81771f, 55.4672f, 0.0f},
    {1000.81f, -14.5069f, 55.4566f, 0.0f},
    {999.755f, -98.4792f, 55.4426f, 0.0f},
    {991.799f, -25.0955f, 55.4441f, 0.0f},
    {991.738f, -87.1632f, 55.4445f, 0.0f},
    {989.866f, -66.0868f, 55.4331f, 0.0f},
    {988.208f, -50.3646f, 55.4291f, 0.0f},
    {986.608f, -37.7656f, 55.4411f, 0.0f},
    {985.180f, -77.3785f, 55.4409f, 0.0f},
    {980.927f, -58.2656f, 55.4542f, 0.0f},
};

const Position EngulfingFlamesCenter[] =
{
    {1069.66f, -4.53993f, 55.4308f, 0.0f},
    {1062.94f, -4.34201f, 55.5682f, 0.0f},
    {1062.13f, -109.005f, 55.4259f, 0.0f},
    {1055.34f, 5.06771f,  55.4471f, 0.0f},
    {1057.03f, -4.10417f, 55.4258f, 0.0f},
    {1049.74f, -118.396f, 55.5661f, 0.0f},
    {1052.59f, -120.562f, 55.4563f, 0.0f},
    {1049.33f, 5.0434f,   55.4633f, 0.0f},
    {1049.98f, -7.22396f, 55.4537f, 0.0f},
    {1049.66f, -104.906f, 55.4556f, 0.0f},
    {1035.91f, 0.909722f, 55.4470f, 0.0f},
    {1035.56f, -114.156f, 55.4471f, 0.0f},
    {1038.53f, -100.254f, 55.6012f, 0.0f},
    {1036.90f, -14.6181f, 55.5715f, 0.0f},
    {1016.99f, -57.5642f, 55.4133f, 0.0f},
    {1030.22f, -92.8403f, 55.4344f, 0.0f},
    {1024.45f, -8.13889f, 55.4470f, 0.0f},
    {1024.45f, -8.13889f, 55.4470f, 0.0f},
    {1024.91f, -106.852f, 55.4471f, 0.0f},
    {1025.34f, -25.8472f, 55.4069f, 0.0f},
    {1025.29f, -86.2326f, 55.4071f, 0.0f},
    {1021.84f, -33.7483f, 55.4239f, 0.0f},
    {1021.49f, -79.6076f, 55.4261f, 0.0f},
    {1018.47f, -43.7674f, 55.4218f, 0.0f},
    {1069.91f, -109.651f, 55.4277f, 0.0f},
    {1014.15f, -17.3281f, 55.4629f, 0.0f},
    {1018.29f, -70.1875f, 55.4231f, 0.0f},
    {1012.09f, -97.5122f, 55.4570f, 0.0f},
    {1006.10f, -27.3681f, 55.4277f, 0.0f},
    {1005.49f, -86.4566f, 55.4275f, 0.0f},
    {1002.72f, -40.7431f, 55.4063f, 0.0f},
    {1003.44f, -74.0243f, 55.4063f, 0.0f},
    {1003.44f, -74.0243f, 55.4063f, 0.0f},
    {1003.07f, -66.4913f, 55.4067f, 0.0f},
    {1002.21f, -49.7049f, 55.4075f, 0.0f},
    {1002.00f, -58.2396f, 55.4331f, 0.0f},
};

class at_sulfuron_keep : public AreaTriggerScript
{
    public:
        at_sulfuron_keep() : AreaTriggerScript("at_sulfuron_keep") { }

        bool OnTrigger(Player* player, AreaTriggerEntry const* /*areaTrigger*/)
        {
            if (InstanceScript* instance = player->GetInstanceScript())
                if (!ObjectAccessor::GetCreature(*player, instance->GetData64(DATA_RAGNAROS)))
                    player->SummonCreature(NPC_RAGNAROS, RagnarosSummonPosition, TEMPSUMMON_MANUAL_DESPAWN, 0);
            return true;
        }
};

class boss_ragnaros_cata : public CreatureScript
{
public:
    boss_ragnaros_cata() : CreatureScript("boss_ragnaros_cata") { }

    struct boss_ragnaros_cataAI : public BossAI
    {
        boss_ragnaros_cataAI(Creature* creature) : BossAI(creature, DATA_RAGNAROS)
        {
            instance = creature->GetInstanceScript();
            Arrived = false;
            Killed = false;
            Submerged = 0;
        }

        void DespawnCreatures(uint32 entry, float distance)
        {
            std::list<Creature*> creatures;
            GetCreatureListWithEntryInGrid(creatures, me, entry, distance);

            if (creatures.empty())
                return;

            for (std::list<Creature*>::iterator iter = creatures.begin(); iter != creatures.end(); ++iter)
                (*iter)->DespawnOrUnsummon();
        }

        InstanceScript* instance;
        bool Arrived;
        bool Killed;
        uint8 Submerged;

        void Reset()
        {
            if (Arrived)
            {
                me->AddAura(SPELL_BASE_VISUAL, me);
                events.SetPhase(PHASE_1);
            }

            DespawnCreatures(53500, 300.0f);
            DespawnCreatures(53813, 300.0f);
            DespawnCreatures(53814, 300.0f);
            DespawnCreatures(53815, 300.0f);

            _Reset();
            instance->SetBossState(DATA_RAGNAROS, NOT_STARTED);
            instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
            me->AddAura(SPELL_BURNING_WOUNDS_AURA, me);
            me->AddAura(SPELL_BASE_VISUAL, me);
            me->SetReactState(REACT_PASSIVE);
            me->HandleEmoteCommand(0);
            me->SetByteValue(UNIT_FIELD_BYTES_1, 3, UNIT_BYTE1_FLAG_ALWAYS_STAND | UNIT_BYTE1_FLAG_HOVER);
            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            Submerged = 0;
            Killed = false;
            events.Reset();
            summons.DespawnAll();
        }

        void EnterCombat(Unit* who)
        {
            if(who && who->GetTypeId() != TYPEID_PLAYER)
                me->Kill(who);

            Talk(SAY_AGRRO);
            me->SetReactState(REACT_AGGRESSIVE);
            instance->SetBossState(DATA_RAGNAROS, IN_PROGRESS);
            instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me);
            events.ScheduleEvent(EVENT_SULFURAS_SMASH_TRIGGER, 30000, 0, PHASE_1);
            events.ScheduleEvent(EVENT_MAGMA_TRAP, 15000, 0, PHASE_1);
            events.ScheduleEvent(EVENT_HAND_OF_RAGNAROS, 25000, 0, PHASE_1);
            events.ScheduleEvent(EVENT_WRATH_OF_RAGNAROS, 6000, 0, PHASE_1);
            events.ScheduleEvent(EVENT_MAGMA_BLAST, 2000);
            _EnterCombat();
        }

        void JustDied(Unit* /*killer*/)
        {
            if (IsHeroic())
            {
                Talk(SAY_DEATH_HEROIC);
                instance->SetBossState(DATA_RAGNAROS, DONE);
                instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
            }
            events.Reset();
            summons.DespawnAll();
        }

        void IsSummonedBy(Unit* /*summoner*/)
        {
            if (!Arrived)
            {
                me->SetVisible(false);
                me->setActive(true);
                me->SetDisableGravity(true);
                me->SetByteValue(UNIT_FIELD_BYTES_1, 3, UNIT_BYTE1_FLAG_ALWAYS_STAND | UNIT_BYTE1_FLAG_HOVER);
                events.SetPhase(PHASE_INTRO);
                events.ScheduleEvent(EVENT_ARRIVE_1, 100, 0, PHASE_INTRO);
                Arrived = true;
            }
        }

        void JustSummoned(Creature* summon)
        {
            summons.Summon(summon);
            switch (summon->GetEntry())
            {
                case NPC_SULFURAS_SMASH_TARGET_1:
                    summon->SetReactState(REACT_PASSIVE);
                    summon->SetDisplayId(summon->GetCreatureTemplate()->Modelid2);
                    summon->AddAura(SPELL_SULFURAS_SMASH_VISUAL, summon);
                    break;
                case NPC_MAGMA_TRAP:
                    summon->AddAura(SPELL_MAGMA_TRAP_VISUAL, summon);
                    break;
                case NPC_SPLITTING_BLOW_TRIGGER:
                    summon->SetReactState(REACT_PASSIVE);
                    summon->SetDisplayId(summon->GetCreatureTemplate()->Modelid2);
                    break;
                case NPC_ENGULFING_FLAMES_TRIGGER:
                    summon->AddAura(SPELL_ENGULFING_FLAMES_VISUAL_MELEE, summon);
                    summon->SetDisplayId(summon->GetCreatureTemplate()->Modelid2);
                    summon->SetReactState(REACT_PASSIVE);
                    break;
            }
        }

        void DamageTaken(Unit* attacker, uint32& damage)
        {
            if (me->HealthBelowPct(10))
            {
                if (!Killed)
                {
                    Talk(SAY_DEATH_NORMAL);
                    me->AttackStop();
                    me->CastStop();
                    me->SetReactState(REACT_PASSIVE);
                    me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
                    me->RemoveAllAuras();
                    me->SummonGameObject(GO_CACHE_OF_THE_FIRELORD, 1016.043f, -57.436f, 55.333f, 3.151f, 0, 0, 0, 0, 70000);
                    instance->SetBossState(DATA_RAGNAROS, DONE);
                    events.ScheduleEvent(EVENT_DESPAWN, 2000);
                    me->HandleEmoteCommand(EMOTE_ONESHOT_SUBMERGE); // Temp until i got the correct animkit id
                    Killed = true;
                    Map::PlayerList const& players = me->GetMap()->GetPlayers();
                    if (!players.isEmpty())
                    {
                        for (Map::PlayerList::const_iterator itr = players.begin(); itr != players.end(); ++itr)
                        {
                            Player* player = itr->getSource();
                            if (player)
                                me->GetMap()->ToInstanceMap()->PermBindAllPlayers(player);
                        }
                    }
                }
            }
        }

        void KilledUnit(Unit* /*Killed*/)
        {
            Talk(SAY_SLAY);
        }

        void UpdateAI(uint32 const diff)
        {
            if (!(events.GetPhaseMask() & PHASE_SUBMERGED))
                if (!UpdateVictim())
                    return;

            if(me->getVictim() && me->getVictim()->GetTypeId() != TYPEID_PLAYER)
                me->Kill(me->getVictim());

            if (me->HealthBelowPct(70))
            {
                if (Submerged == 0)
                {
                    Talk(SAY_ANNOUNCE_SPLIT);
                    Talk(SAY_SUBMERGE);
                    switch (urand(0, 2))
                    {
                        case 0: // Splitting Blow East
                        {
                            me->CastStop();
                            if (Creature* trigger = me->SummonCreature(NPC_SPLITTING_BLOW_TRIGGER, SplittingTriggerEast, TEMPSUMMON_TIMED_DESPAWN, 12000))
                            {
                                me->SetFacingToObject(trigger);
                                DoCastAOE(SPELL_SPLITTING_BLOW_EAST, false);
                                for (uint32 x = 0; x<8; ++x)
                                    me->SummonCreature(NPC_SON_OF_FLAME, HammerEastSummons[x], TEMPSUMMON_TIMED_DESPAWN, 45000);
                            }
                            break;
                        }
                        case 1: // Splitting Blow West
                        {
                            me->CastStop();
                            if (Creature* trigger = me->SummonCreature(NPC_SPLITTING_BLOW_TRIGGER, SplittingTriggerWest, TEMPSUMMON_TIMED_DESPAWN, 12000))
                            {
                                me->SetFacingToObject(trigger);
                                DoCastAOE(SPELL_SPLITTING_BLOW_WEST, false);
                                for (uint32 x = 0; x<8; ++x)
                                    me->SummonCreature(NPC_SON_OF_FLAME, HammerWestSummons[x], TEMPSUMMON_TIMED_DESPAWN, 45000);
                            }
                            break;
                        }
                        case 2: // Splitting Blow North
                        {
                            me->CastStop();
                            if (Creature* trigger = me->SummonCreature(NPC_SPLITTING_BLOW_TRIGGER, SplittingTriggerNorth, TEMPSUMMON_TIMED_DESPAWN, 12000))
                            {
                                me->SetFacingToObject(trigger);
                                DoCastAOE(SPELL_SPLITTING_BLOW_NORTH, false);
                                for (uint32 x = 0; x<8; ++x)
                                    me->SummonCreature(NPC_SON_OF_FLAME, HammerMiddleSummons[x], TEMPSUMMON_TIMED_DESPAWN, 45000);
                            }
                            break;
                        }
                    }

                    me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                    me->SetReactState(REACT_PASSIVE);
                    me->AttackStop();
                    me->SetTarget(0);
                    events.SetPhase(PHASE_SUBMERGED);
                    events.ScheduleEvent(EVENT_SUBMERGE, 12000, 0, PHASE_SUBMERGED);
                    events.ScheduleEvent(EVENT_EMERGE, 47000, 0, PHASE_SUBMERGED);
                    events.CancelEvent(EVENT_MAGMA_BLAST);
                    Submerged++;
                }
            }

            if (me->HealthBelowPct(40))
            {
                if (Submerged == 1)
                {
                    me->SummonCreature(NPC_LAVA_SCION, 1026.86f, 5.89583f, 55.447f, 4.90438f, TEMPSUMMON_TIMED_DESPAWN, 45000);
                    me->SummonCreature(NPC_LAVA_SCION, 1027.31f, -121.746f, 55.4471f, 1.36136f, TEMPSUMMON_TIMED_DESPAWN, 45000);

                    Talk(SAY_ANNOUNCE_SPLIT);
                    Talk(SAY_SUBMERGE);
                    switch (urand(0, 2))
                    {
                        case 0: // Splitting Blow East
                        {
                            me->CastStop();
                            if (Creature* trigger = me->SummonCreature(NPC_SPLITTING_BLOW_TRIGGER, SplittingTriggerEast, TEMPSUMMON_TIMED_DESPAWN, 12000))
                            {
                                me->SetFacingToObject(trigger);
                                DoCastAOE(SPELL_SPLITTING_BLOW_EAST, false);
                                for (uint32 x = 0; x<8; ++x)
                                    me->SummonCreature(NPC_SON_OF_FLAME, HammerEastSummons[x], TEMPSUMMON_TIMED_DESPAWN, 45000);
                            }
                            break;
                        }
                        case 1: // Splitting Blow West
                        {
                            me->CastStop();
                            if (Creature* trigger = me->SummonCreature(NPC_SPLITTING_BLOW_TRIGGER, SplittingTriggerWest, TEMPSUMMON_TIMED_DESPAWN, 12000))
                            {
                                me->SetFacingToObject(trigger);
                                DoCastAOE(SPELL_SPLITTING_BLOW_WEST, false);
                                for (uint32 x = 0; x<8; ++x)
                                    me->SummonCreature(NPC_SON_OF_FLAME, HammerWestSummons[x], TEMPSUMMON_TIMED_DESPAWN, 45000);
                            }
                            break;
                        }
                        case 2: // Splitting Blow North
                        {
                            me->CastStop();
                            if (Creature* trigger = me->SummonCreature(NPC_SPLITTING_BLOW_TRIGGER, SplittingTriggerNorth, TEMPSUMMON_TIMED_DESPAWN, 12000))
                            {
                                me->SetFacingToObject(trigger);
                                DoCastAOE(SPELL_SPLITTING_BLOW_NORTH, false);
                                for (uint32 x = 0; x<8; ++x)
                                    me->SummonCreature(NPC_SON_OF_FLAME, HammerMiddleSummons[x], TEMPSUMMON_TIMED_DESPAWN, 45000);
                            }
                            break;
                        }
                    }

                    me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                    me->SetReactState(REACT_PASSIVE);
                    me->AttackStop();
                    me->SetTarget(0);
                    events.SetPhase(PHASE_SUBMERGED);
                    events.ScheduleEvent(EVENT_SUBMERGE, 12000, 0, PHASE_SUBMERGED);
                    events.ScheduleEvent(EVENT_EMERGE, 47000, 0, PHASE_SUBMERGED);
                    events.CancelEvent(EVENT_MAGMA_BLAST);
                    Submerged++;
                }
            }

            events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_ARRIVE_1:
                        me->SetVisible(true);
                        me->PlayOneShotAnimKit(1467);
                        Talk(SAY_ARRIVE);
                        events.ScheduleEvent(EVENT_ARRIVE_2, 6500, 0, PHASE_INTRO);
                        break;
                    case EVENT_ARRIVE_2:
                        me->PlayOneShotAnimKit(1468);
                        me->AddAura(SPELL_BASE_VISUAL, me);
                        me->AddAura(SPELL_BURNING_WOUNDS_AURA, me);
                        events.SetPhase(PHASE_1);
                        break;
                    case EVENT_SULFURAS_SMASH_TRIGGER:
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 0, true, 0))
                        {
                            float distance = me->GetExactDist2d(target);
                            if (distance == 0)
                                distance = 0.01f;
                            float x = me->GetPositionX();
                            x = x + (target->GetPositionX() - x) * me->GetCombatReach() / distance;
                            float y = me->GetPositionY();
                            y = y + (target->GetPositionY() - y) * me->GetCombatReach() / distance;
                            Creature* trigger = me->SummonCreature(NPC_SULFURAS_SMASH_TARGET_1, x, y, target->GetPositionZ(), me->GetOrientation(), TEMPSUMMON_TIMED_DESPAWN, 20000);
                            me->SetFacingToObject(trigger);
                        }
                        Talk(SAY_SULFURAS_SMASH);
                        if (Submerged == 0)
                        {
                            events.ScheduleEvent(EVENT_SULFURAS_SMASH_TRIGGER, 30000, 0, PHASE_1);
                            events.ScheduleEvent(EVENT_SULFURAS_SMASH, 100, 0, PHASE_1);
                        }
                        else if (Submerged == 1)
                        {
                            events.ScheduleEvent(EVENT_SULFURAS_SMASH_TRIGGER, 40000, 0, PHASE_2);
                            events.ScheduleEvent(EVENT_SULFURAS_SMASH, 100, 0, PHASE_2);
                        }
                        else if (Submerged == 2)
                        {
                            events.ScheduleEvent(EVENT_SULFURAS_SMASH_TRIGGER, 40000, 0, PHASE_3);
                            events.ScheduleEvent(EVENT_SULFURAS_SMASH, 100, 0, PHASE_3);
                        }
                        break;
                    case EVENT_SULFURAS_SMASH:
                        if (Creature* trigger = me->FindNearestCreature(NPC_SULFURAS_SMASH_TARGET_1, 100.0f))
                        {
                            me->AttackStop();
                            me->SetReactState(REACT_PASSIVE);
                            me->SetFacingToObject(trigger);
                            DoCast(trigger, SPELL_SULFURAS_SMASH);
                            events.ScheduleEvent(EVENT_ATTACK, 7000);
                        }
                        events.ScheduleEvent(EVENT_SUMMON_WAVES, 4000);
                        break;
                    case EVENT_SUMMON_WAVES:
                        if (Creature* summoner = me->FindNearestCreature(NPC_SULFURAS_SMASH_TARGET_1, 100.0f))
                        {
                            if (Creature* wave1 = me->SummonCreature(NPC_LAVA_WAVE, summoner->GetPositionX(), summoner->GetPositionY(), summoner->GetPositionZ(), summoner->GetOrientation(), TEMPSUMMON_TIMED_DESPAWN, 10000))
                            {
                                wave1->setFaction(14);
                                wave1->SetReactState(REACT_PASSIVE);
                                wave1->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
                                wave1->SetDisplayId(wave1->GetCreatureTemplate()->Modelid2);
                                wave1->AddAura(SPELL_LAVA_WAVE_VISUAL, wave1);
                                wave1->GetMotionMaster()->MovePoint(0, wave1->GetPositionX()+cos(wave1->GetOrientation())*50, wave1->GetPositionY()+sin(wave1->GetOrientation())*50, wave1->GetPositionZ());
                            }

                            if (Creature* wave2 = me->SummonCreature(NPC_LAVA_WAVE, summoner->GetPositionX(), summoner->GetPositionY(), summoner->GetPositionZ(), summoner->GetOrientation() + M_PI/2, TEMPSUMMON_TIMED_DESPAWN, 10000))
                            {
                                wave2->setFaction(14);
                                wave2->SetReactState(REACT_PASSIVE);
                                wave2->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
                                wave2->SetDisplayId(wave2->GetCreatureTemplate()->Modelid2);
                                wave2->AddAura(SPELL_LAVA_WAVE_VISUAL, wave2);
                                wave2->GetMotionMaster()->MovePoint(0, wave2->GetPositionX()+cos(wave2->GetOrientation())*60, wave2->GetPositionY()+sin(wave2->GetOrientation())*60, wave2->GetPositionZ());
                            }

                            if (Creature* wave3 = me->SummonCreature(NPC_LAVA_WAVE, summoner->GetPositionX(), summoner->GetPositionY(), summoner->GetPositionZ(), summoner->GetOrientation() - M_PI/2, TEMPSUMMON_TIMED_DESPAWN, 10000))
                            {
                                wave3->setFaction(14);
                                wave3->SetReactState(REACT_PASSIVE);
                                wave3->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
                                wave3->SetDisplayId(wave3->GetCreatureTemplate()->Modelid2);
                                wave3->AddAura(SPELL_LAVA_WAVE_VISUAL, wave3);
                                wave3->GetMotionMaster()->MovePoint(0, wave3->GetPositionX()+cos(wave3->GetOrientation())*60, wave3->GetPositionY()+sin(wave3->GetOrientation())*60, wave3->GetPositionZ());
                            }
                        }
                        break;
                    case EVENT_MAGMA_TRAP:
                        if (Unit* target = SelectTarget(SELECT_TARGET_FARTHEST, 0))
                            DoCast(target, SPELL_MAGMA_TRAP);
                        Talk(SAY_MAGMA_TRAP);
                        events.ScheduleEvent(EVENT_MAGMA_TRAP, 25000, 0, PHASE_1);
                        break;
                    case EVENT_HAND_OF_RAGNAROS:
                        DoCastAOE(SPELL_HAND_OF_RAGNAROS);
                        events.ScheduleEvent(EVENT_HAND_OF_RAGNAROS, 25000, 0, PHASE_1);
                        break;
                    case EVENT_WRATH_OF_RAGNAROS:
                        DoCastAOE(SPELL_WRATH_OF_RAGNAROS);
                        events.ScheduleEvent(EVENT_WRATH_OF_RAGNAROS, 36000, 0, PHASE_1);
                        break;
                    case EVENT_SUBMERGE:
                        me->AddAura(SPELL_DISABLE_ANIM, me);
                        me->AddAura(SPELL_SUBMERGED, me);
                        me->RemoveAurasDueToSpell(SPELL_BASE_VISUAL);
                        break;
                    case EVENT_EMERGE:
                        me->RemoveAllAuras();
                        me->SetByteValue(UNIT_FIELD_BYTES_1, 3, UNIT_BYTE1_FLAG_ALWAYS_STAND | UNIT_BYTE1_FLAG_HOVER);
                        me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                        me->PlayOneShotAnimKit(1465);
                        if (Submerged == 1)
                        {
                            events.SetPhase(PHASE_2);
                            events.ScheduleEvent(EVENT_ATTACK, 4500, 0, PHASE_2);
                            events.ScheduleEvent(EVENT_ENGULFING_FLAMES, 40000, 0, PHASE_2);
                            events.ScheduleEvent(EVENT_SULFURAS_SMASH_TRIGGER, 15000, 0, PHASE_2);
                            events.ScheduleEvent(EVENT_MOLTEN_SEED_START, 20000, 0, PHASE_2);
                            events.ScheduleEvent(EVENT_MAGMA_BLAST, 2000);

                        }
                        if (Submerged == 2)
                        {
                            events.SetPhase(PHASE_3);
                            events.ScheduleEvent(EVENT_ATTACK, 4500, 0, PHASE_3);
                            events.ScheduleEvent(EVENT_ENGULFING_FLAMES, 30000, 0, PHASE_3);
                            events.ScheduleEvent(EVENT_SULFURAS_SMASH_TRIGGER, 15000, 0, PHASE_3);
                            events.ScheduleEvent(EVENT_LIVING_METEOR, 51000, PHASE_3);
                            events.ScheduleEvent(EVENT_MAGMA_BLAST, 2000);
                        }
                        Talk(SAY_EMERGE);
                        break;
                    case EVENT_MOLTEN_SEED_START:
                        DoCastAOE(SPELL_MOLTEN_SEED);
                        DoCastAOE(SPELL_MOLTEN_SEED_EXPLOGEN);
                        DoCastAOE(SPELL_MOLTEN_SEED_TRIGGER);
                        events.ScheduleEvent(EVENT_MOLTEN_SEED_START, 60000, 0, PHASE_2);
                        break;
                    case EVENT_ATTACK:
                        me->SetReactState(REACT_AGGRESSIVE);
                        me->AddAura(SPELL_BASE_VISUAL, me);
                        me->AddAura(SPELL_BURNING_WOUNDS_AURA, me);
                        if (Creature* hammer = me->FindNearestCreature(NPC_SULFURAS_HAMMER, 50.0f))
                            hammer->DespawnOrUnsummon(1);
                        break;
                    case EVENT_ENGULFING_FLAMES:
                        switch (urand(0, 2))
                        {
                            case 0: // Melee
                                {
                                    for (uint32 x = 0; x<16; ++x)
                                        me->SummonCreature(NPC_ENGULFING_FLAMES_TRIGGER, EngulfingFlamesMelee[x], TEMPSUMMON_TIMED_DESPAWN, 20000);
                                    DoCastAOE(SPELL_ENGULFING_FLAMES_MELEE);
                                    break;
                                }
                            case 1: // Range
                                {
                                    for (uint32 x = 0; x<19; ++x)
                                        me->SummonCreature(NPC_ENGULFING_FLAMES_TRIGGER, EngulfingFlamesRange[x], TEMPSUMMON_TIMED_DESPAWN, 20000);
                                    DoCastAOE(SPELL_ENGULFING_FLAMES_BOTTOM);
                                    break;
                                }
                            case 2: // Center
                                {
                                    for (uint32 x = 0; x<36; ++x)
                                        me->SummonCreature(NPC_ENGULFING_FLAMES_TRIGGER, EngulfingFlamesCenter[x], TEMPSUMMON_TIMED_DESPAWN, 20000);
                                    DoCastAOE(SPELL_ENGULFING_FLAMES_CENTER);
                                    break;
                                }
                                break;
                        }
                        if (Submerged == 1)
                            events.ScheduleEvent(EVENT_ENGULFING_FLAMES, 40000, 0, PHASE_2);
                        else if (Submerged == 2)
                            events.ScheduleEvent(EVENT_ENGULFING_FLAMES, 30000, 0, PHASE_3);
                        break;
                    case EVENT_DESPAWN:
                        me->AddAura(SPELL_DISABLE_ANIM, me);
                        me->AddAura(SPELL_SUBMERGED, me);
                        me->DespawnOrUnsummon(1000);
                        break;
                    case EVENT_MAGMA_BLAST:
                        if(Unit* target = me->getVictim())
                            if(me->GetDistance2d(target) > 0.0f)
                                DoCast(target, SPELL_MAGMA_BLAST);
                        events.ScheduleEvent(EVENT_MAGMA_BLAST, 2000);
                        break;
                    case EVENT_LIVING_METEOR:
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 0, true, 0))
                            DoCast(target, SPELL_LIVING_METEOR);
                        events.ScheduleEvent(EVENT_LIVING_METEOR, 45000, PHASE_3);
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
        return new boss_ragnaros_cataAI(creature);
    }
};

class npc_molten_seed : public CreatureScript
{
    public:
        npc_molten_seed() :  CreatureScript("npc_molten_seed") { }

        struct npc_molten_seedAI : public ScriptedAI
        {
            npc_molten_seedAI(Creature* creature) : ScriptedAI(creature)
            {
                exploded = false;
                summoned = false;
            }

            void IsSummonedBy(Unit*)
            {
                if (!summoned)
                {
                    summoned = true;
                    DoCast(SPELL_MOLTEN_SEED_VISUAL);
                    events.ScheduleEvent(EVENT_MOLTEN_SEED_END, 10000);
                }
            }

             void UpdateAI(uint32 const diff)
            {
                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                    case EVENT_MOLTEN_SEED_END:
                        DoCast(SPELL_MOLTEN_INFERNO);
                        me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
                        if (Unit* summon = me->ToTempSummon()->GetSummoner())
                            if (Creature* ellement = summon->SummonCreature(NPC_MOLTEN_ELEMENTAR, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), me->GetOrientation(), TEMPSUMMON_TIMED_DESPAWN, 45000))
                                DoZoneInCombat(ellement);
                        me->DespawnOrUnsummon(1000);
                        break;
                    }
                }
             }
                private:
            EventMap events;
            bool exploded;
            bool ready;
            bool summoned;

        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_molten_seedAI(creature);
        }
};

class npc_magma_trap : public CreatureScript
{
    public:
        npc_magma_trap() :  CreatureScript("npc_magma_trap") { }

        struct npc_magma_trapAI : public ScriptedAI
        {
            npc_magma_trapAI(Creature* creature) : ScriptedAI(creature)
            {
                exploded = false;
                ready = false;
            }

            void IsSummonedBy(Unit* /*summoner*/)
            {
                me->SetSpeed(MOVE_RUN, 0.001f);
                me->SetSpeed(MOVE_WALK, 0.001f);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
                events.ScheduleEvent(EVENT_PREPARE, 5000);
            }

            void MoveInLineOfSight(Unit* who)
            {
                if (ready && !exploded && me->HasAura(SPELL_MAGMA_TRAP_VISUAL) && me->IsWithinDistInMap(who, 7.0f) && who->GetTypeId() == TYPEID_PLAYER)
                {
                    DoCastAOE(SPELL_MAGMA_ERRUPTION);
                    me->RemoveAurasDueToSpell(SPELL_MAGMA_TRAP_VISUAL);
                    me->DespawnOrUnsummon(5000);
                    exploded = true;
                }
            }

            void UpdateAI(uint32 const diff)
            {
                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_PREPARE:
                            ready = true;
                            break;
                    }
                }
            }

        private:
            EventMap events;
            bool exploded;
            bool ready;

        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_magma_trapAI(creature);
        }
};

class npc_engulfing_flame : public CreatureScript
{
    public:
        npc_engulfing_flame() :  CreatureScript("npc_engulfing_flame") { }

        struct npc_engulfing_flameAI : public ScriptedAI
        {
            npc_engulfing_flameAI(Creature* creature) : ScriptedAI(creature)
            {
            }

            void IsSummonedBy(Unit* /*summoner*/)
            {
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
                events.ScheduleEvent(EVENT_EXPLODE, 3000);
            }

            void UpdateAI(uint32 const diff)
            {
                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_EXPLODE:
                            DoCastAOE(SPELL_ENGULFING_FLAMES_EXPLOSION);
                            break;
                    }
                }
            }

        private:
            EventMap events;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_engulfing_flameAI(creature);
        }
};

class npc_sulfuras_hammer : public CreatureScript
{
    public:
        npc_sulfuras_hammer() :  CreatureScript("npc_sulfuras_hammer") { }

        struct npc_sulfuras_hammerAI : public ScriptedAI
        {
            npc_sulfuras_hammerAI(Creature* creature) : ScriptedAI(creature)
            {
                summoned = false;
            }

            void IsSummonedBy(Unit* /*summoner*/)
            {
                if (!summoned)
                {
                    summoned = true;
                    DoCastAOE(SPELL_SULFURAS_KNOCKBACK);
                    me->SetReactState(REACT_PASSIVE);
                    me->AddAura(SPELL_SULFURAS_AURA, me);
                    me->setFaction(14);
                    me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
                    me->SetDisplayId(me->GetCreatureTemplate()->Modelid2);
                    me->DespawnOrUnsummon(70000);
                }
            }

            void UpdateAI(uint32 const diff)
            {
                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_SUMMON_SONS:
                            break;
                    }
                }
            }

        private:
            EventMap events;
            bool summoned;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_sulfuras_hammerAI(creature);
        }
};

class npc_son_of_flame : public CreatureScript
{
    public:
        npc_son_of_flame() :  CreatureScript("npc_son_of_flame") { }

        struct npc_son_of_flameAI : public ScriptedAI
        {
            npc_son_of_flameAI(Creature* creature) : ScriptedAI(creature)
            {
                slowed = false;
            }

            void IsSummonedBy(Unit* /*summoner*/)
            {
                me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_GRIP, true);
                me->SetReactState(REACT_PASSIVE);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NON_ATTACKABLE);
                DoCastAOE(SPELL_PRE_VISUAL);
                events.ScheduleEvent(EVENT_TRIGGER, 9000);
            }

            void MovementInform(uint32 type, uint32 pointId)
            {
                switch (pointId)
                {
                    case POINT_HAMMER:
                        events.ScheduleEvent(EVENT_BOOM, 1);
                        break;
                }
            }

            void DamageTaken(Unit* /*attacker*/, uint32& damage)
            {
                float percen = me->GetHealthPct();
                if (percen > 50.0f)
                {
                    percen -= 50.0f;
                    uint32 stack = uint32(percen / 5);
                    uint32 counthealth = me->CountPctFromMaxHealth(50 + (stack * 5));
                    uint32 gethealth = me->GetHealth();
                    if(gethealth > counthealth)
                    {
                        gethealth -= damage;
                        if(gethealth < counthealth)
                            me->SetAuraStack(SPELL_BURNING_SPEED, me, stack);
                    }
                }
            }

            void UpdateAI(uint32 const diff)
            {
                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_TRIGGER:
                            {
                                if (Creature* trigger = me->FindNearestCreature(NPC_SULFURAS_HAMMER, 100.0f))
                                    trigger->CastSpell(me, SPELL_CALL_SONS_MISSILE);
                            }
                            events.ScheduleEvent(EVENT_ACTIVATE, 8000);
                            events.ScheduleEvent(EVENT_ACTIVATE2, 6000);
                            break;
                        case EVENT_ACTIVATE:
                            if(AuraPtr aura = me->AddAura(SPELL_BURNING_SPEED, me))
                                aura->SetStackAmount(10);
                            if (Creature* target = me->FindNearestCreature(NPC_SULFURAS_HAMMER, 100.0f))
                                me->GetMotionMaster()->MovePoint(POINT_HAMMER, target->GetPositionX(), target->GetPositionY(), target->GetPositionZ());
                            break;
                        case EVENT_BOOM:
                            if (Creature* target = me->FindNearestCreature(NPC_SULFURAS_HAMMER, 100.0f))
                                target->CastSpell(target, SPELL_FLAMES_OF_SULFURAS);
                            me->DespawnOrUnsummon(1000);
                            break;
                        case EVENT_ACTIVATE2:
                            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NON_ATTACKABLE);
                            me->RemoveAllAuras();
                            break;
                    }
                }

                if (me->HealthBelowPct(50) && !slowed)
                {
                    me->RemoveAllAuras();
                    me->SetWalk(true);
                    slowed = true;
                }
            }

        private:
            EventMap events;
            bool slowed;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_son_of_flameAI(creature);
        }
};

class npc_living_meteor : public CreatureScript
{
    public:
        npc_living_meteor() :  CreatureScript("npc_living_meteor") { }

        struct npc_living_meteorAI : public ScriptedAI
        {
            npc_living_meteorAI(Creature* creature) : ScriptedAI(creature)
            {
                change = true;
            }

            bool change;
            float movespeed;

            void IsSummonedBy(Unit* /*summoner*/)
            {
                movespeed = 0.8f;
                me->SetSpeed(MOVE_RUN, movespeed);
                me->SetSpeed(MOVE_WALK, movespeed);
                me->AddAura(SPELL_LIVING_METEOR_VISUAL, me);
                DoZoneInCombat();
                if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, NonTankTargetSelector(me)))
                {
                    me->AddThreat(target, 0.0f);
                    me->AI()->AttackStart(target);
                    me->AddAura(SPELL_LIVING_METEOR_FIXATE, target);
                }
                events.ScheduleEvent(EVENT_CHANGE_SPEED, 5000);
            }

            void KilledUnit(Unit* victim)
            {
                DoZoneInCombat();
                if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, NonTankTargetSelector(me)))
                {
                    me->AddThreat(target, 0.0f);
                    me->AI()->AttackStart(target);
                    me->AddAura(SPELL_LIVING_METEOR_FIXATE, target);
                }
            }

            void DamageTaken(Unit* attacker, uint32& damage)
            {
                if (damage > 0 && change)
                {
                    me->RemoveAllAuras();
                    movespeed = 0.8f;
                    me->SetSpeed(MOVE_RUN, movespeed);
                    me->SetSpeed(MOVE_WALK, movespeed);
                    me->AddAura(SPELL_LIVING_METEOR_VISUAL, me);
                    if(Unit* victim = me->getVictim())
                    {
                        victim->RemoveAurasDueToSpell(SPELL_LIVING_METEOR_FIXATE);
                        me->CastSpell(victim, SPELL_LIVING_METEOR_COMBUSTION, true);
                    }
                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, NonTankTargetSelector(me)))
                    {
                        me->AddThreat(target, 0.0f);
                        me->AI()->AttackStart(target);
                        me->AddAura(SPELL_LIVING_METEOR_FIXATE, target);
                    }
                    change = false;
                    events.ScheduleEvent(EVENT_CHANGE_TARGET, 5000);
                }
            }

            void MoveInLineOfSight(Unit* who)
            {
                if (who && me->IsWithinDistInMap(who, 4.0f) && who->GetTypeId() == TYPEID_PLAYER)
                    DoCastAOE(SPELL_LIVING_METEOR_BUFF);
            }

            void UpdateAI(uint32 const diff)
            {
                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_CHANGE_TARGET:
                            change = true;
                            break;
                        case EVENT_CHANGE_SPEED:
                            DoCast(me, SPELL_LIVING_METEOR_SPEED);
                            movespeed += movespeed * 0.1f;
                            me->SetSpeed(MOVE_RUN, movespeed);
                            me->SetSpeed(MOVE_WALK, movespeed);
                            events.ScheduleEvent(EVENT_CHANGE_SPEED, 3000);
                            break;
                    }
                }
            }

        private:
            EventMap events;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_living_meteorAI(creature);
        }
};

class npc_ragnarog_lava : public CreatureScript
{
public:
    npc_ragnarog_lava() : CreatureScript("npc_ragnarog_lava") { }

    struct npc_ragnarog_lavaAI : public ScriptedAI
    {
        npc_ragnarog_lavaAI(Creature* creature) : ScriptedAI(creature) {}

        void Reset()
        {
            me->SetReactState(REACT_PASSIVE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
            events.Reset();
            me->SetCanFly(true);
        }

        void MovementInform(uint32 type, uint32 id)
        {
            if (type == POINT_MOTION_TYPE)
            {
                switch (id)
                {
                    case POINT_RAGNAROS_UP:
                        events.ScheduleEvent(EVENT_LAVA_DOWN, 15000);
                        break;
                }
            }
        }

        void SpellHit(Unit* Hitter, const SpellInfo* Spell)
        {
            if (Spell->Id == 99503)
            {
                me->GetMotionMaster()->MovePoint(POINT_RAGNAROS_UP, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ()+8);
            }
        }

        void UpdateAI(const uint32 diff)
        {
            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_LAVA_DOWN:
                        me->GetMotionMaster()->MovePoint(POINT_RAGNAROS_DOWN, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ()-8);
                        break;
                }
            }
        }

        private:
            EventMap events;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_ragnarog_lavaAI (creature);
    }
};

class spell_splitting_blow : public SpellScriptLoader
{
public:
    spell_splitting_blow() : SpellScriptLoader("spell_splitting_blow") { }

    class spell_splitting_blow_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_splitting_blow_SpellScript);

        bool Validate(SpellInfo const* spellEntry)
        {
            if (!sSpellMgr->GetSpellInfo(99056))
                return false;
            return true;
        }

        void HandleScriptEffect(SpellEffIndex /*effIndex*/)
        {
            GetHitUnit()->CastSpell(GetHitUnit(), 99056, true);
        }

        void Register()
        {
            OnEffectHitTarget += SpellEffectFn(spell_splitting_blow_SpellScript::HandleScriptEffect, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_splitting_blow_SpellScript();
    }
};

class spell_boss_molten_inferno : public SpellScriptLoader
{
    public:
        spell_boss_molten_inferno() : SpellScriptLoader("spell_boss_molten_inferno") { }

        class spell_boss_molten_inferno_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_boss_molten_inferno_SpellScript);

            void CalculateDamage(SpellEffIndex /*effIndex*/)
            {
                if(Unit* target = GetHitUnit())
                {
                    if(Unit* caster = GetCaster())
                    {
                        uint32 dist = uint32(caster->GetDistance2d(target) / 3) + 1;
                        int32 damage = GetHitDamage();
                        damage = int32(damage / dist);
                        SetHitDamage(int32(damage));
                    }
                }
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_boss_molten_inferno_SpellScript::CalculateDamage, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_boss_molten_inferno_SpellScript();
        }
};

class spell_boss_raise_lava : public SpellScriptLoader
{
    public:
        spell_boss_raise_lava() : SpellScriptLoader("spell_boss_raise_lava") { }

        class spell_boss_raise_lava_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_boss_raise_lava_SpellScript);

            void FilterTargets(std::list<WorldObject*>& unitList)
            {
                for (std::list<WorldObject*>::iterator itr = unitList.begin(); itr != unitList.end();)
                {
                    if ((*itr)->GetEntry() == 53585)
                        ++itr;
                    else
                        unitList.erase(itr++);
                }
                if (unitList.size() > 3)
                    Trinity::Containers::RandomResizeList(unitList, 3);
            }

            void Register()
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_boss_raise_lava_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENTRY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_boss_raise_lava_SpellScript();
        }
};

class spell_boss_lava_bolt : public SpellScriptLoader
{
    public:
        spell_boss_lava_bolt() : SpellScriptLoader("spell_boss_lava_bolt") { }

        class spell_boss_lava_bolt_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_boss_lava_bolt_SpellScript);

            void FilterTargets(std::list<WorldObject*>& unitList)
            {
                uint32 maxSize = uint32(GetCaster()->GetMap()->GetSpawnMode() & 1 ? 10 : 4);
                if (unitList.size() > maxSize)
                    Trinity::Containers::RandomResizeList(unitList, maxSize);
            }

            void Register()
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_boss_lava_bolt_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENEMY);
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_boss_lava_bolt_SpellScript::FilterTargets, EFFECT_1, TARGET_UNIT_SRC_AREA_ENEMY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_boss_lava_bolt_SpellScript();
        }
};

void AddSC_boss_ragnaros_firelands()
{
    new at_sulfuron_keep();
    new boss_ragnaros_cata();
    new npc_molten_seed();
    new npc_magma_trap();
    new npc_engulfing_flame();
    new npc_sulfuras_hammer();
    new npc_son_of_flame();
    new npc_living_meteor();
    new npc_ragnarog_lava();
    new spell_splitting_blow();
    new spell_boss_molten_inferno();
    new spell_boss_raise_lava();
    new spell_boss_lava_bolt();
}