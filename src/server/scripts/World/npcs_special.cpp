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

/* ScriptData
SDName: Npcs_Special
SD%Complete: 100
SDComment: To be used for special NPCs that are located globally.
SDCategory: NPCs
EndScriptData
*/

/* ContentData
npc_air_force_bots       80%    support for misc (invisible) guard bots in areas where player allowed to fly. Summon guards after a preset time if tagged by spell
npc_lunaclaw_spirit      80%    support for quests 6001/6002 (Body and Heart)
npc_chicken_cluck       100%    support for quest 3861 (Cluck!)
npc_dancing_flames      100%    midsummer event NPC
npc_guardian            100%    guardianAI used to prevent players from accessing off-limits areas. Not in use by SD2
npc_garments_of_quests   80%    NPC's related to all Garments of-quests 5621, 5624, 5625, 5648, 565
npc_injured_patient     100%    patients for triage-quests (6622 and 6624)
npc_doctor              100%    Gustaf Vanhowzen and Gregory Victor, quest 6622 and 6624 (Triage)
npc_mount_vendor        100%    Regular mount vendors all over the world. Display gossip if player doesn't meet the requirements to buy
npc_rogue_trainer        80%    Scripted trainers, so they are able to offer item 17126 for class quest 6681
npc_sayge               100%    Darkmoon event fortune teller, buff player based on answers given
npc_snake_trap_serpents  80%    AI for snakes that summoned by Snake Trap
npc_locksmith            75%    list of keys needs to be confirmed
npc_firework            100%    NPC's summoned by rockets and rocket clusters, for making them cast visual
EndContentData */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "ScriptedEscortAI.h"
#include "ObjectMgr.h"
#include "ScriptMgr.h"
#include "World.h"
#include "PetAI.h"
#include "PassiveAI.h"
#include "CombatAI.h"
#include "GameEventMgr.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "Cell.h"
#include "CellImpl.h"
#include "SpellAuras.h"
#include "Vehicle.h"
#include "Player.h"

/*########
# npc_air_force_bots
#########*/

enum SpawnType
{
    SPAWNTYPE_TRIPWIRE_ROOFTOP,                             // no warning, summon Creature at smaller range
    SPAWNTYPE_ALARMBOT,                                     // cast guards mark and summon npc - if player shows up with that buff duration < 5 seconds attack
};

struct SpawnAssociation
{
    uint32 thisCreatureEntry;
    uint32 spawnedCreatureEntry;
    SpawnType spawnType;
};

enum eEnums
{
    SPELL_GUARDS_MARK               = 38067,
    AURA_DURATION_TIME_LEFT         = 5000
};

float const RANGE_TRIPWIRE          = 15.0f;
float const RANGE_GUARDS_MARK       = 50.0f;

SpawnAssociation spawnAssociations[] =
{
    {2614,  15241, SPAWNTYPE_ALARMBOT},                     //Air Force Alarm Bot (Alliance)
    {2615,  15242, SPAWNTYPE_ALARMBOT},                     //Air Force Alarm Bot (Horde)
    {21974, 21976, SPAWNTYPE_ALARMBOT},                     //Air Force Alarm Bot (Area 52)
    {21993, 15242, SPAWNTYPE_ALARMBOT},                     //Air Force Guard Post (Horde - Bat Rider)
    {21996, 15241, SPAWNTYPE_ALARMBOT},                     //Air Force Guard Post (Alliance - Gryphon)
    {21997, 21976, SPAWNTYPE_ALARMBOT},                     //Air Force Guard Post (Goblin - Area 52 - Zeppelin)
    {21999, 15241, SPAWNTYPE_TRIPWIRE_ROOFTOP},             //Air Force Trip Wire - Rooftop (Alliance)
    {22001, 15242, SPAWNTYPE_TRIPWIRE_ROOFTOP},             //Air Force Trip Wire - Rooftop (Horde)
    {22002, 15242, SPAWNTYPE_TRIPWIRE_ROOFTOP},             //Air Force Trip Wire - Ground (Horde)
    {22003, 15241, SPAWNTYPE_TRIPWIRE_ROOFTOP},             //Air Force Trip Wire - Ground (Alliance)
    {22063, 21976, SPAWNTYPE_TRIPWIRE_ROOFTOP},             //Air Force Trip Wire - Rooftop (Goblin - Area 52)
    {22065, 22064, SPAWNTYPE_ALARMBOT},                     //Air Force Guard Post (Ethereal - Stormspire)
    {22066, 22067, SPAWNTYPE_ALARMBOT},                     //Air Force Guard Post (Scryer - Dragonhawk)
    {22068, 22064, SPAWNTYPE_TRIPWIRE_ROOFTOP},             //Air Force Trip Wire - Rooftop (Ethereal - Stormspire)
    {22069, 22064, SPAWNTYPE_ALARMBOT},                     //Air Force Alarm Bot (Stormspire)
    {22070, 22067, SPAWNTYPE_TRIPWIRE_ROOFTOP},             //Air Force Trip Wire - Rooftop (Scryer)
    {22071, 22067, SPAWNTYPE_ALARMBOT},                     //Air Force Alarm Bot (Scryer)
    {22078, 22077, SPAWNTYPE_ALARMBOT},                     //Air Force Alarm Bot (Aldor)
    {22079, 22077, SPAWNTYPE_ALARMBOT},                     //Air Force Guard Post (Aldor - Gryphon)
    {22080, 22077, SPAWNTYPE_TRIPWIRE_ROOFTOP},             //Air Force Trip Wire - Rooftop (Aldor)
    {22086, 22085, SPAWNTYPE_ALARMBOT},                     //Air Force Alarm Bot (Sporeggar)
    {22087, 22085, SPAWNTYPE_ALARMBOT},                     //Air Force Guard Post (Sporeggar - Spore Bat)
    {22088, 22085, SPAWNTYPE_TRIPWIRE_ROOFTOP},             //Air Force Trip Wire - Rooftop (Sporeggar)
    {22090, 22089, SPAWNTYPE_ALARMBOT},                     //Air Force Guard Post (Toshley's Station - Flying Machine)
    {22124, 22122, SPAWNTYPE_ALARMBOT},                     //Air Force Alarm Bot (Cenarion)
    {22125, 22122, SPAWNTYPE_ALARMBOT},                     //Air Force Guard Post (Cenarion - Stormcrow)
    {22126, 22122, SPAWNTYPE_ALARMBOT}                      //Air Force Trip Wire - Rooftop (Cenarion Expedition)
};

class npc_storm_earth_and_fire : public CreatureScript
{
public:
    npc_storm_earth_and_fire() : CreatureScript("npc_storm_earth_and_fire") { }

    struct npc_storm_earth_and_fireAI : ScriptedAI
    {
        npc_storm_earth_and_fireAI(Creature* creature) : ScriptedAI(creature) { }

        bool addaura;
        bool jumpontarget;
        bool comeonhome;
        bool startattack;
        uint32 jump;
        uint32 entryId;
        uint64 firsttarget;
        Unit *owner;
        float oldHast;

        void InitializeAI()
        {
            owner = me->ToTempSummon()->GetSummoner();
            firsttarget = me->GetTargetGUID();
            addaura = true;
            jumpontarget = true;
            comeonhome = false;
            startattack = false;
            jump = 0;
            entryId = me->GetEntry();
            oldHast = 0;
        }

        void CheckWayOfTheMonkAura()
        {
            if (owner->HasAura(108977))
                me->AddAura(108977, me);
            else if (owner->HasAura(120275))
                me->AddAura(120275, me);
        }

        void RecalcDamage()
        {
            if (Player* plr = owner->ToPlayer())
            {
                Item* mainItem = plr->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_MAINHAND);
                Item* offItem = plr->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_OFFHAND);
                bool greenClone = entryId == 69792;

                float finaldmg = 0;
                float mainattackspeed = 0;
                float adddmg = 0;
                float offattackspeed = 0;

                if (mainItem)
                {
                    finaldmg = mainItem->GetTemplate()->DPS;
                    mainattackspeed = mainItem->GetTemplate()->Delay / 1000.0f;
                }
                if (offItem && mainItem)
                {
                    adddmg = offItem->GetTemplate()->DPS;
                    offattackspeed = mainItem->GetTemplate()->Delay / 1000.0f;

                    if (greenClone)
                        adddmg /= 2;
                }

                finaldmg += adddmg;

                if (!greenClone)
                {
                    float offfinaldmg = finaldmg / 2;
                    finaldmg -= offfinaldmg;

                    offfinaldmg = (offfinaldmg + me->GetTotalAttackPowerValue(BASE_ATTACK) / 14.0f) * (offattackspeed ? offattackspeed: mainattackspeed);
                    offfinaldmg *= me->GetModifierValue(UNIT_MOD_DAMAGE_OFFHAND, BASE_PCT);
                    offfinaldmg += me->GetModifierValue(UNIT_MOD_DAMAGE_OFFHAND, TOTAL_VALUE);
                    offfinaldmg *= me->GetModifierValue(UNIT_MOD_DAMAGE_OFFHAND, TOTAL_PCT);

                    me->SetStatFloatValue(UNIT_FIELD_MINOFFHANDDAMAGE, offfinaldmg);
                    me->SetStatFloatValue(UNIT_FIELD_MAXOFFHANDDAMAGE, offfinaldmg);
                }

                finaldmg = (finaldmg + me->GetTotalAttackPowerValue(BASE_ATTACK) / 14.0f) * mainattackspeed;
                finaldmg *= me->GetModifierValue(UNIT_MOD_DAMAGE_MAINHAND, BASE_PCT);
                finaldmg += me->GetModifierValue(UNIT_MOD_DAMAGE_MAINHAND, TOTAL_VALUE);
                finaldmg *= me->GetModifierValue(UNIT_MOD_DAMAGE_MAINHAND, TOTAL_PCT);

                me->SetStatFloatValue(UNIT_FIELD_MINDAMAGE, finaldmg);
                me->SetStatFloatValue(UNIT_FIELD_MAXDAMAGE, finaldmg);
            }
        }

        void RecalcHast()
        {
            if (Player* plr = owner->ToPlayer())
            {
                float amount = plr->GetRatingBonusValue(CR_HASTE_MELEE);
                amount += me->GetTotalAuraModifier(SPELL_AURA_MOD_MELEE_HASTE_3);

                if (amount == oldHast)
                    return;

                oldHast = amount;

                Item* mainItem = plr->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_MAINHAND);
                Item* offItem = plr->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_OFFHAND);
                float mainattackspeed = BASE_ATTACK_TIME;
                float offattackspeed = 0;

                if (mainItem)
                    mainattackspeed = mainItem->GetTemplate()->Delay;

                if (offItem)
                    offattackspeed = offItem->GetTemplate()->Delay;

                ApplyPercentModFloatVar(mainattackspeed, amount, false);
                ApplyPercentModFloatVar(offattackspeed, amount, false);

                if (entryId != 69792)
                    me->SetFloatValue(UNIT_FIELD_BASEATTACKTIME+OFF_ATTACK, offattackspeed ? offattackspeed: mainattackspeed);

                me->SetFloatValue(UNIT_FIELD_BASEATTACKTIME+BASE_ATTACK, mainattackspeed);
            }
        }

        void RecalcStats()
        {
            if (!owner || comeonhome)
                return;

            if (!me->IsWithinDist(owner, 100))
            {
                ComonOnHome();
                return;
            }

            if (Aura* aura = me->GetAura(138130))
            {
                if (AuraEffect* eff1 = aura->GetEffect(EFFECT_1))
                    if (Aura* ownerAura = owner->GetAura(137639))
                        if (ownerAura->GetEffect(EFFECT_1)->GetAmount() != eff1->GetAmount())
                        {
                            eff1->SetCanBeRecalculated(true);
                            eff1->RecalculateAmount(me);
                        }

                if (AuraEffect* eff4 = aura->GetEffect(EFFECT_4))
                {
                    if (eff4->GetAmount() != owner->GetTotalAttackPowerValue(BASE_ATTACK))
                    {
                        eff4->SetAmount(owner->GetTotalAttackPowerValue(BASE_ATTACK));
                        me->SetUInt32Value(UNIT_FIELD_ATTACK_POWER, me->GetMaxPositiveAuraModifier(SPELL_AURA_MOD_ATTACK_POWER));
                    }
                }

                RecalcHast();
                RecalcDamage();

                if (Player* plr = owner->ToPlayer())
                {
                    me->countCrit = plr->GetFloatValue(PLAYER_CRIT_PERCENTAGE);
                    me->m_modMeleeHitChance = plr->m_modMeleeHitChance;
                }
            }
        }

        void ComonOnHome()
        {
            if (!owner)
                return;

            comeonhome = true;

            if (me->isAlive())
            {
                me->CastSpell(owner, 124002, true);
                owner->m_Controlled.erase(me);
                me->DespawnOrUnsummon(500);
            }
            else
                me->DespawnOrUnsummon();

            if (Aura *aura = owner->GetAura(137639))
            {
                switch (aura->GetStackAmount())
                {
                    case 1: owner->RemoveAura(137639); break;
                    case 2: aura->SetStackAmount(1);   break;
                }
            }
        }

        void UpdateAI(uint32 diff)
        {
            if (!comeonhome)
            {
                if (!me->getVictim())
                {
                    if (addaura)
                    {
                        if (owner)
                        {
                            uint32 visualAuraId = 0;
                            switch (entryId)
                            {
                                case 69792: visualAuraId = 138083; break;
                                case 69680: visualAuraId = 138080; break;
                                case 69791: visualAuraId = 138081; break;
                            }
                            owner->CastSpell(me, 149716, true);
                            me->CastSpell(me, visualAuraId, true);
                            owner->AddAura(120272, me);
                            owner->AddAura(115043, me);
                            owner->AddAura(128595, me);
                            me->AddAura(138130, me);
                            me->AddAura(103985, me);
                            me->setPowerType(POWER_MANA);
                            me->SetMaxPower(POWER_MANA, 21262);
                            me->SetPower(POWER_MANA, 21262);
                            CheckWayOfTheMonkAura();
                        }
                        addaura = false;
                    }

                    if (!jumpontarget)
                        ComonOnHome();

                    if (jumpontarget)
                    {
                        jump += diff;
                        if (jump >= 500)
                        {
                            if(Unit* gettarget = ObjectAccessor::GetUnit(*me, firsttarget))
                            {
                                me->CastSpell(gettarget, 124002, true);
                                AttackStart(gettarget);
                                jumpontarget = false;
                            }
                        }
                    }
                }

                if (me->getVictim() && me->getVictim()->GetGUID() != firsttarget)
                    if (!jumpontarget && !comeonhome)
                        ComonOnHome();

                if (owner)
                {
                    if (me->HasUnitState(UNIT_STATE_CASTING))
                    {
                        startattack = false;
                        if (!owner->HasUnitState(UNIT_STATE_CASTING))
                            me->CastStop(113656);
                        else
                            return;
                    }
                }

                Unit *victim = me->getVictim();

                if (me->isAttackReady() && me->IsWithinMeleeRange(victim))
                {
                    me->AttackerStateUpdate(victim);
                    me->resetAttackTimer();

                    if (entryId != 69792 && !startattack)
                    {
                        me->setAttackTimer(OFF_ATTACK, 1000);
                        startattack = true;
                    }
                }

                if (entryId != 69792 && me->isAttackReady(OFF_ATTACK) && me->IsWithinMeleeRange(victim))
                {
                    me->AttackerStateUpdate(victim, OFF_ATTACK);
                    me->resetAttackTimer(OFF_ATTACK);
                }
            }
        }
    };

    ScriptedAI* GetAI(Creature* creature) const
    {
        return new npc_storm_earth_and_fireAI(creature);
    }
};

class npc_air_force_bots : public CreatureScript
{
    public:
        npc_air_force_bots() : CreatureScript("npc_air_force_bots") { }

        struct npc_air_force_botsAI : public ScriptedAI
        {
            npc_air_force_botsAI(Creature* creature) : ScriptedAI(creature)
            {
                SpawnAssoc = NULL;
                SpawnedGUID = 0;

                // find the correct spawnhandling
                static uint32 entryCount = sizeof(spawnAssociations) / sizeof(SpawnAssociation);

                for (uint8 i = 0; i < entryCount; ++i)
                {
                    if (spawnAssociations[i].thisCreatureEntry == creature->GetEntry())
                    {
                        SpawnAssoc = &spawnAssociations[i];
                        break;
                    }
                }

                if (!SpawnAssoc)
                    sLog->outError(LOG_FILTER_SQL, "TCSR: Creature template entry %u has ScriptName npc_air_force_bots, but it's not handled by that script", creature->GetEntry());
                else
                {
                    CreatureTemplate const* spawnedTemplate = sObjectMgr->GetCreatureTemplate(SpawnAssoc->spawnedCreatureEntry);

                    if (!spawnedTemplate)
                    {
                        sLog->outError(LOG_FILTER_SQL, "TCSR: Creature template entry %u does not exist in DB, which is required by npc_air_force_bots", SpawnAssoc->spawnedCreatureEntry);
                        SpawnAssoc = NULL;
                        return;
                    }
                }
            }

            SpawnAssociation* SpawnAssoc;
            uint64 SpawnedGUID;

            void Reset() {}

            Creature* SummonGuard()
            {
                Creature* summoned = me->SummonCreature(SpawnAssoc->spawnedCreatureEntry, 0.0f, 0.0f, 0.0f, 0.0f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 300000);

                if (summoned)
                    SpawnedGUID = summoned->GetGUID();
                else
                {
                    sLog->outError(LOG_FILTER_SQL, "TCSR: npc_air_force_bots: wasn't able to spawn Creature %u", SpawnAssoc->spawnedCreatureEntry);
                    SpawnAssoc = NULL;
                }

                return summoned;
            }

            Creature* GetSummonedGuard()
            {
                Creature* creature = Unit::GetCreature(*me, SpawnedGUID);

                if (creature && creature->isAlive())
                    return creature;

                return NULL;
            }

            void MoveInLineOfSight(Unit* who)
            {
                if (!SpawnAssoc)
                    return;

                if (me->IsValidAttackTarget(who))
                {
                    Player* playerTarget = who->ToPlayer();

                    // airforce guards only spawn for players
                    if (!playerTarget)
                        return;

                    if (!playerTarget->isAlive())
                        return;

                    Creature* lastSpawnedGuard = SpawnedGUID == 0 ? NULL : GetSummonedGuard();

                    // prevent calling Unit::GetUnit at next MoveInLineOfSight call - speedup
                    if (!lastSpawnedGuard)
                        SpawnedGUID = 0;

                    switch (SpawnAssoc->spawnType)
                    {
                        case SPAWNTYPE_ALARMBOT:
                        {
                            if (!who->IsWithinDistInMap(me, RANGE_GUARDS_MARK))
                                return;

                            Aura* markAura = who->GetAura(SPELL_GUARDS_MARK);
                            if (markAura)
                            {
                                // the target wasn't able to move out of our range within 25 seconds
                                if (!lastSpawnedGuard)
                                {
                                    lastSpawnedGuard = SummonGuard();

                                    if (!lastSpawnedGuard)
                                        return;
                                }

                                if (markAura->GetDuration() < AURA_DURATION_TIME_LEFT)
                                    if (!lastSpawnedGuard->getVictim())
                                        lastSpawnedGuard->AI()->AttackStart(who);
                            }
                            else
                            {
                                if (!lastSpawnedGuard)
                                    lastSpawnedGuard = SummonGuard();

                                if (!lastSpawnedGuard)
                                    return;

                                lastSpawnedGuard->CastSpell(who, SPELL_GUARDS_MARK, true);
                            }
                            break;
                        }
                        case SPAWNTYPE_TRIPWIRE_ROOFTOP:
                        {
                            if (!who->IsWithinDistInMap(me, RANGE_TRIPWIRE))
                                return;

                            if (!lastSpawnedGuard)
                                lastSpawnedGuard = SummonGuard();

                            if (!lastSpawnedGuard)
                                return;

                            // ROOFTOP only triggers if the player is on the ground
                            if (!playerTarget->IsFlying() && !lastSpawnedGuard->getVictim())
                                lastSpawnedGuard->AI()->AttackStart(who);

                            break;
                        }
                    }
                }
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_air_force_botsAI(creature);
        }
};

/*######
## npc_lunaclaw_spirit
######*/

enum
{
    QUEST_BODY_HEART_A      = 6001,
    QUEST_BODY_HEART_H      = 6002,

    TEXT_ID_DEFAULT         = 4714,
    TEXT_ID_PROGRESS        = 4715
};

#define GOSSIP_ITEM_GRANT   "You have thought well, spirit. I ask you to grant me the strength of your body and the strength of your heart."

class npc_lunaclaw_spirit : public CreatureScript
{
    public:
        npc_lunaclaw_spirit() : CreatureScript("npc_lunaclaw_spirit") { }

        bool OnGossipHello(Player* player, Creature* creature)
        {
            if (player->GetQuestStatus(QUEST_BODY_HEART_A) == QUEST_STATUS_INCOMPLETE || player->GetQuestStatus(QUEST_BODY_HEART_H) == QUEST_STATUS_INCOMPLETE)
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ITEM_GRANT, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);

            player->SEND_GOSSIP_MENU(TEXT_ID_DEFAULT, creature->GetGUID());
            return true;
        }

        bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action)
        {
            player->PlayerTalkClass->ClearMenus();
            if (action == GOSSIP_ACTION_INFO_DEF + 1)
            {
                player->SEND_GOSSIP_MENU(TEXT_ID_PROGRESS, creature->GetGUID());
                player->AreaExploredOrEventHappens(player->GetTeam() == ALLIANCE ? QUEST_BODY_HEART_A : QUEST_BODY_HEART_H);
            }
            return true;
        }
};

/*########
# npc_chicken_cluck
#########*/

#define EMOTE_HELLO         -1070004
#define EMOTE_CLUCK_TEXT    -1070006

#define QUEST_CLUCK         3861
#define FACTION_FRIENDLY    35
#define FACTION_CHICKEN     31

class npc_chicken_cluck : public CreatureScript
{
    public:
        npc_chicken_cluck() : CreatureScript("npc_chicken_cluck") { }

        struct npc_chicken_cluckAI : public ScriptedAI
        {
            npc_chicken_cluckAI(Creature* creature) : ScriptedAI(creature) {}

            uint32 ResetFlagTimer;

            void Reset()
            {
                ResetFlagTimer = 120000;
                me->setFaction(FACTION_CHICKEN);
                me->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER);
            }

            void EnterCombat(Unit* /*who*/) {}

            void UpdateAI(uint32 diff)
            {
                // Reset flags after a certain time has passed so that the next player has to start the 'event' again
                if (me->HasFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER))
                {
                    if (ResetFlagTimer <= diff)
                    {
                        EnterEvadeMode();
                        return;
                    }
                    else
                        ResetFlagTimer -= diff;
                }

                if (UpdateVictim())
                    DoMeleeAttackIfReady();
            }

            void ReceiveEmote(Player* player, uint32 emote)
            {
                switch (emote)
                {
                    case TEXT_EMOTE_CHICKEN:
                        if (player->GetQuestStatus(QUEST_CLUCK) == QUEST_STATUS_NONE && rand() % 30 == 1)
                        {
                            me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER);
                            me->setFaction(FACTION_FRIENDLY);
                            DoScriptText(EMOTE_HELLO, me);
                        }
                        break;
                    case TEXT_EMOTE_CHEER:
                        if (player->GetQuestStatus(QUEST_CLUCK) == QUEST_STATUS_COMPLETE)
                        {
                            me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER);
                            me->setFaction(FACTION_FRIENDLY);
                            DoScriptText(EMOTE_CLUCK_TEXT, me);
                        }
                        break;
                }
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_chicken_cluckAI(creature);
        }

        bool OnQuestAccept(Player* /*player*/, Creature* creature, Quest const* quest)
        {
            if (quest->GetQuestId() == QUEST_CLUCK)
                CAST_AI(npc_chicken_cluck::npc_chicken_cluckAI, creature->AI())->Reset();

            return true;
        }

        bool OnQuestComplete(Player* /*player*/, Creature* creature, Quest const* quest)
        {
            if (quest->GetQuestId() == QUEST_CLUCK)
                CAST_AI(npc_chicken_cluck::npc_chicken_cluckAI, creature->AI())->Reset();

            return true;
        }
};

/*######
## npc_dancing_flames
######*/

#define SPELL_BRAZIER       45423
#define SPELL_SEDUCTION     47057
#define SPELL_FIERY_AURA    45427

class npc_dancing_flames : public CreatureScript
{
    public:
        npc_dancing_flames() : CreatureScript("npc_dancing_flames") { }

        struct npc_dancing_flamesAI : public ScriptedAI
        {
            npc_dancing_flamesAI(Creature* creature) : ScriptedAI(creature) {}

            bool Active;
            uint32 CanIteract;

            void Reset()
            {
                Active = true;
                CanIteract = 3500;
                DoCast(me, SPELL_BRAZIER, true);
                DoCast(me, SPELL_FIERY_AURA, false);
                float x, y, z;
                me->GetPosition(x, y, z);
                me->Relocate(x, y, z + 0.94f);
                me->SetDisableGravity(true);
                me->HandleEmoteCommand(EMOTE_ONESHOT_DANCE);
                me->SendMovementFlagUpdate(false);  //send update position to client
            }

            void UpdateAI(uint32 diff)
            {
                if (!Active)
                {
                    if (CanIteract <= diff)
                    {
                        Active = true;
                        CanIteract = 3500;
                        me->HandleEmoteCommand(EMOTE_ONESHOT_DANCE);
                    }
                    else
                        CanIteract -= diff;
                }
            }

            void EnterCombat(Unit* /*who*/){}

            void ReceiveEmote(Player* player, uint32 emote)
            {
                if (me->IsWithinLOS(player->GetPositionX(), player->GetPositionY(), player->GetPositionZ()) && me->IsWithinDistInMap(player, 30.0f))
                {
                    me->SetInFront(player);
                    Active = false;

                    me->SendMovementFlagUpdate(false);
                    switch (emote)
                    {
                        case TEXT_EMOTE_KISS:
                            me->HandleEmoteCommand(EMOTE_ONESHOT_SHY);
                            break;
                        case TEXT_EMOTE_WAVE:
                            me->HandleEmoteCommand(EMOTE_ONESHOT_WAVE);
                            break;
                        case TEXT_EMOTE_BOW:
                            me->HandleEmoteCommand(EMOTE_ONESHOT_BOW);
                            break;
                        case TEXT_EMOTE_JOKE:
                            me->HandleEmoteCommand(EMOTE_ONESHOT_LAUGH);
                            break;
                        case TEXT_EMOTE_DANCE:
                            if (!player->HasAura(SPELL_SEDUCTION))
                                DoCast(player, SPELL_SEDUCTION, true);
                            break;
                    }
                }
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_dancing_flamesAI(creature);
        }
};

/*######
## Triage quest
######*/

//signed for 9623
#define SAY_DOC1    -1000201
#define SAY_DOC2    -1000202
#define SAY_DOC3    -1000203

#define DOCTOR_ALLIANCE     12939
#define DOCTOR_HORDE        12920
#define ALLIANCE_COORDS     7
#define HORDE_COORDS        6

struct Location
{
    float x, y, z, o;
};

static Location AllianceCoords[]=
{
    {-3757.38f, -4533.05f, 14.16f, 3.62f},                      // Top-far-right bunk as seen from entrance
    {-3754.36f, -4539.13f, 14.16f, 5.13f},                      // Top-far-left bunk
    {-3749.54f, -4540.25f, 14.28f, 3.34f},                      // Far-right bunk
    {-3742.10f, -4536.85f, 14.28f, 3.64f},                      // Right bunk near entrance
    {-3755.89f, -4529.07f, 14.05f, 0.57f},                      // Far-left bunk
    {-3749.51f, -4527.08f, 14.07f, 5.26f},                      // Mid-left bunk
    {-3746.37f, -4525.35f, 14.16f, 5.22f},                      // Left bunk near entrance
};

//alliance run to where
#define A_RUNTOX -3742.96f
#define A_RUNTOY -4531.52f
#define A_RUNTOZ 11.91f

static Location HordeCoords[]=
{
    {-1013.75f, -3492.59f, 62.62f, 4.34f},                      // Left, Behind
    {-1017.72f, -3490.92f, 62.62f, 4.34f},                      // Right, Behind
    {-1015.77f, -3497.15f, 62.82f, 4.34f},                      // Left, Mid
    {-1019.51f, -3495.49f, 62.82f, 4.34f},                      // Right, Mid
    {-1017.25f, -3500.85f, 62.98f, 4.34f},                      // Left, front
    {-1020.95f, -3499.21f, 62.98f, 4.34f}                       // Right, Front
};

//horde run to where
#define H_RUNTOX -1016.44f
#define H_RUNTOY -3508.48f
#define H_RUNTOZ 62.96f

uint32 const AllianceSoldierId[3] =
{
    12938,                                                  // 12938 Injured Alliance Soldier
    12936,                                                  // 12936 Badly injured Alliance Soldier
    12937                                                   // 12937 Critically injured Alliance Soldier
};

uint32 const HordeSoldierId[3] =
{
    12923,                                                  //12923 Injured Soldier
    12924,                                                  //12924 Badly injured Soldier
    12925                                                   //12925 Critically injured Soldier
};

/*######
## npc_doctor (handles both Gustaf Vanhowzen and Gregory Victor)
######*/

class npc_doctor : public CreatureScript
{
    public:
        npc_doctor() : CreatureScript("npc_doctor") {}

        struct npc_doctorAI : public ScriptedAI
        {
            npc_doctorAI(Creature* creature) : ScriptedAI(creature) {}

            uint64 PlayerGUID;

            uint32 SummonPatientTimer;
            uint32 SummonPatientCount;
            uint32 PatientDiedCount;
            uint32 PatientSavedCount;

            bool Event;

            std::list<uint64> Patients;
            std::vector<Location*> Coordinates;

            void Reset()
            {
                PlayerGUID = 0;

                SummonPatientTimer = 10000;
                SummonPatientCount = 0;
                PatientDiedCount = 0;
                PatientSavedCount = 0;

                Patients.clear();
                Coordinates.clear();

                Event = false;

                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            }

            void BeginEvent(Player* player)
            {
                PlayerGUID = player->GetGUID();

                SummonPatientTimer = 10000;
                SummonPatientCount = 0;
                PatientDiedCount = 0;
                PatientSavedCount = 0;

                switch (me->GetEntry())
                {
                    case DOCTOR_ALLIANCE:
                        for (uint8 i = 0; i < ALLIANCE_COORDS; ++i)
                            Coordinates.push_back(&AllianceCoords[i]);
                        break;
                    case DOCTOR_HORDE:
                        for (uint8 i = 0; i < HORDE_COORDS; ++i)
                            Coordinates.push_back(&HordeCoords[i]);
                        break;
                }

                Event = true;
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            }

            void PatientDied(Location* point)
            {
                Player* player = Unit::GetPlayer(*me, PlayerGUID);
                if (player && ((player->GetQuestStatus(6624) == QUEST_STATUS_INCOMPLETE) || (player->GetQuestStatus(6622) == QUEST_STATUS_INCOMPLETE)))
                {
                    ++PatientDiedCount;

                    if (PatientDiedCount > 5 && Event)
                    {
                        if (player->GetQuestStatus(6624) == QUEST_STATUS_INCOMPLETE)
                            player->FailQuest(6624);
                        else if (player->GetQuestStatus(6622) == QUEST_STATUS_INCOMPLETE)
                            player->FailQuest(6622);

                        Reset();
                        return;
                    }

                    Coordinates.push_back(point);
                }
                else
                    // If no player or player abandon quest in progress
                    Reset();
            }

            void PatientSaved(Creature* /*soldier*/, Player* player, Location* point)
            {
                if (player && PlayerGUID == player->GetGUID())
                {
                    if ((player->GetQuestStatus(6624) == QUEST_STATUS_INCOMPLETE) || (player->GetQuestStatus(6622) == QUEST_STATUS_INCOMPLETE))
                    {
                        ++PatientSavedCount;

                        if (PatientSavedCount == 15)
                        {
                            if (!Patients.empty())
                            {
                                std::list<uint64>::const_iterator itr;
                                for (itr = Patients.begin(); itr != Patients.end(); ++itr)
                                {
                                    if (Creature* patient = Unit::GetCreature((*me), *itr))
                                        patient->setDeathState(JUST_DIED);
                                }
                            }

                            if (player->GetQuestStatus(6624) == QUEST_STATUS_INCOMPLETE)
                                player->AreaExploredOrEventHappens(6624);
                            else if (player->GetQuestStatus(6622) == QUEST_STATUS_INCOMPLETE)
                                player->AreaExploredOrEventHappens(6622);

                            Reset();
                            return;
                        }

                        Coordinates.push_back(point);
                    }
                }
            }

            void UpdateAI(uint32 diff);

            void EnterCombat(Unit* /*who*/){}
        };

        bool OnQuestAccept(Player* player, Creature* creature, Quest const* quest)
        {
            if ((quest->GetQuestId() == 6624) || (quest->GetQuestId() == 6622))
                CAST_AI(npc_doctor::npc_doctorAI, creature->AI())->BeginEvent(player);

            return true;
        }

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_doctorAI(creature);
        }
};

/*#####
## npc_injured_patient (handles all the patients, no matter Horde or Alliance)
#####*/

class npc_injured_patient : public CreatureScript
{
    public:
        npc_injured_patient() : CreatureScript("npc_injured_patient") { }

        struct npc_injured_patientAI : public ScriptedAI
        {
            npc_injured_patientAI(Creature* creature) : ScriptedAI(creature) {}

            uint64 DoctorGUID;
            Location* Coord;

            void Reset()
            {
                DoctorGUID = 0;
                Coord = NULL;

                //no select
                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);

                //no regen health
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IN_COMBAT);

                //to make them lay with face down
                me->SetUInt32Value(UNIT_FIELD_BYTES_1, UNIT_STAND_STATE_DEAD);

                uint32 mobId = me->GetEntry();

                switch (mobId)
                {                                                   //lower max health
                    case 12923:
                    case 12938:                                     //Injured Soldier
                        me->SetHealth(me->CountPctFromMaxHealth(75));
                        break;
                    case 12924:
                    case 12936:                                     //Badly injured Soldier
                        me->SetHealth(me->CountPctFromMaxHealth(50));
                        break;
                    case 12925:
                    case 12937:                                     //Critically injured Soldier
                        me->SetHealth(me->CountPctFromMaxHealth(25));
                        break;
                }
            }

            void EnterCombat(Unit* /*who*/){}

            void SpellHit(Unit* caster, SpellInfo const* spell)
            {
                if (caster->GetTypeId() == TYPEID_PLAYER && me->isAlive() && spell->Id == 20804)
                {
                    if ((CAST_PLR(caster)->GetQuestStatus(6624) == QUEST_STATUS_INCOMPLETE) || (CAST_PLR(caster)->GetQuestStatus(6622) == QUEST_STATUS_INCOMPLETE))
                        if (DoctorGUID)
                            if (Creature* doctor = Unit::GetCreature(*me, DoctorGUID))
                                CAST_AI(npc_doctor::npc_doctorAI, doctor->AI())->PatientSaved(me, CAST_PLR(caster), Coord);

                    //make not selectable
                    me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);

                    //regen health
                    me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IN_COMBAT);

                    //stand up
                    me->SetUInt32Value(UNIT_FIELD_BYTES_1, UNIT_STAND_STATE_STAND);

                    DoScriptText(RAND(SAY_DOC1, SAY_DOC2, SAY_DOC3), me);

                    uint32 mobId = me->GetEntry();
                    me->SetWalk(false);

                    switch (mobId)
                    {
                        case 12923:
                        case 12924:
                        case 12925:
                            me->GetMotionMaster()->MovePoint(0, H_RUNTOX, H_RUNTOY, H_RUNTOZ);
                            break;
                        case 12936:
                        case 12937:
                        case 12938:
                            me->GetMotionMaster()->MovePoint(0, A_RUNTOX, A_RUNTOY, A_RUNTOZ);
                            break;
                    }
                }
            }

            void UpdateAI(uint32 /*diff*/)
            {
                //lower HP on every world tick makes it a useful counter, not officlone though
                if (me->isAlive() && me->GetHealth() > 6)
                    me->ModifyHealth(-5);

                if (me->isAlive() && me->GetHealth() <= 6)
                {
                    me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IN_COMBAT);
                    me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                    me->setDeathState(JUST_DIED);
                    me->SetFlag(OBJECT_FIELD_DYNAMIC_FLAGS, 32);

                    if (DoctorGUID)
                        if (Creature* doctor = Unit::GetCreature((*me), DoctorGUID))
                            CAST_AI(npc_doctor::npc_doctorAI, doctor->AI())->PatientDied(Coord);
                }
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_injured_patientAI(creature);
        }
};

void npc_doctor::npc_doctorAI::UpdateAI(uint32 diff)
{
    if (Event && SummonPatientCount >= 20)
    {
        Reset();
        return;
    }

    if (Event)
    {
        if (SummonPatientTimer <= diff)
        {
            if (Coordinates.empty())
                return;

            std::vector<Location*>::iterator itr = Coordinates.begin() + rand() % Coordinates.size();
            uint32 patientEntry = 0;

            switch (me->GetEntry())
            {
                case DOCTOR_ALLIANCE:
                    patientEntry = AllianceSoldierId[rand() % 3];
                    break;
                case DOCTOR_HORDE:
                    patientEntry = HordeSoldierId[rand() % 3];
                    break;
                default:
                    sLog->outError(LOG_FILTER_TSCR, "Invalid entry for Triage doctor. Please check your database");
                    return;
            }

            if (Location* point = *itr)
            {
                if (Creature* Patient = me->SummonCreature(patientEntry, point->x, point->y, point->z, point->o, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 5000))
                {
                    //303, this flag appear to be required for client side item->spell to work (TARGET_SINGLE_FRIEND)
                    Patient->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PVP_ATTACKABLE);

                    Patients.push_back(Patient->GetGUID());
                    CAST_AI(npc_injured_patient::npc_injured_patientAI, Patient->AI())->DoctorGUID = me->GetGUID();
                    CAST_AI(npc_injured_patient::npc_injured_patientAI, Patient->AI())->Coord = point;

                    Coordinates.erase(itr);
                }
            }
            SummonPatientTimer = 10000;
            ++SummonPatientCount;
        }
        else
            SummonPatientTimer -= diff;
    }
}

/*######
## npc_garments_of_quests
######*/

//TODO: get text for each NPC

enum eGarments
{
    SPELL_LESSER_HEAL_R2    = 2052,
    SPELL_FORTITUDE_R1      = 1243,

    QUEST_MOON              = 5621,
    QUEST_LIGHT_1           = 5624,
    QUEST_LIGHT_2           = 5625,
    QUEST_SPIRIT            = 5648,
    QUEST_DARKNESS          = 5650,

    ENTRY_SHAYA             = 12429,
    ENTRY_ROBERTS           = 12423,
    ENTRY_DOLF              = 12427,
    ENTRY_KORJA             = 12430,
    ENTRY_DG_KEL            = 12428,

    //used by 12429, 12423, 12427, 12430, 12428, but signed for 12429
    SAY_COMMON_HEALED       = -1000164,
    SAY_DG_KEL_THANKS       = -1000165,
    SAY_DG_KEL_GOODBYE      = -1000166,
    SAY_ROBERTS_THANKS      = -1000167,
    SAY_ROBERTS_GOODBYE     = -1000168,
    SAY_KORJA_THANKS        = -1000169,
    SAY_KORJA_GOODBYE       = -1000170,
    SAY_DOLF_THANKS         = -1000171,
    SAY_DOLF_GOODBYE        = -1000172,
    SAY_SHAYA_THANKS        = -1000173,
    SAY_SHAYA_GOODBYE       = -1000174, //signed for 21469
};

class npc_garments_of_quests : public CreatureScript
{
    public:
        npc_garments_of_quests() : CreatureScript("npc_garments_of_quests") { }

        struct npc_garments_of_questsAI : public npc_escortAI
        {
            npc_garments_of_questsAI(Creature* creature) : npc_escortAI(creature) {Reset();}

            uint64 CasterGUID;

            bool IsHealed;
            bool CanRun;

            uint32 RunAwayTimer;

            void Reset()
            {
                CasterGUID = 0;

                IsHealed = false;
                CanRun = false;

                RunAwayTimer = 5000;

                me->SetStandState(UNIT_STAND_STATE_KNEEL);
                //expect database to have RegenHealth=0
                me->SetHealth(me->CountPctFromMaxHealth(70));
            }

            void EnterCombat(Unit* /*who*/) {}

            void SpellHit(Unit* caster, SpellInfo const* Spell)
            {
                if (Spell->Id == SPELL_LESSER_HEAL_R2 || Spell->Id == SPELL_FORTITUDE_R1)
                {
                    //not while in combat
                    if (me->isInCombat())
                        return;

                    //nothing to be done now
                    if (IsHealed && CanRun)
                        return;

                    if (Player* player = caster->ToPlayer())
                    {
                        switch (me->GetEntry())
                        {
                            case ENTRY_SHAYA:
                                if (player->GetQuestStatus(QUEST_MOON) == QUEST_STATUS_INCOMPLETE)
                                {
                                    if (IsHealed && !CanRun && Spell->Id == SPELL_FORTITUDE_R1)
                                    {
                                        DoScriptText(SAY_SHAYA_THANKS, me, caster);
                                        CanRun = true;
                                    }
                                    else if (!IsHealed && Spell->Id == SPELL_LESSER_HEAL_R2)
                                    {
                                        CasterGUID = caster->GetGUID();
                                        me->SetStandState(UNIT_STAND_STATE_STAND);
                                        DoScriptText(SAY_COMMON_HEALED, me, caster);
                                        IsHealed = true;
                                    }
                                }
                                break;
                            case ENTRY_ROBERTS:
                                if (player->GetQuestStatus(QUEST_LIGHT_1) == QUEST_STATUS_INCOMPLETE)
                                {
                                    if (IsHealed && !CanRun && Spell->Id == SPELL_FORTITUDE_R1)
                                    {
                                        DoScriptText(SAY_ROBERTS_THANKS, me, caster);
                                        CanRun = true;
                                    }
                                    else if (!IsHealed && Spell->Id == SPELL_LESSER_HEAL_R2)
                                    {
                                        CasterGUID = caster->GetGUID();
                                        me->SetStandState(UNIT_STAND_STATE_STAND);
                                        DoScriptText(SAY_COMMON_HEALED, me, caster);
                                        IsHealed = true;
                                    }
                                }
                                break;
                            case ENTRY_DOLF:
                                if (player->GetQuestStatus(QUEST_LIGHT_2) == QUEST_STATUS_INCOMPLETE)
                                {
                                    if (IsHealed && !CanRun && Spell->Id == SPELL_FORTITUDE_R1)
                                    {
                                        DoScriptText(SAY_DOLF_THANKS, me, caster);
                                        CanRun = true;
                                    }
                                    else if (!IsHealed && Spell->Id == SPELL_LESSER_HEAL_R2)
                                    {
                                        CasterGUID = caster->GetGUID();
                                        me->SetStandState(UNIT_STAND_STATE_STAND);
                                        DoScriptText(SAY_COMMON_HEALED, me, caster);
                                        IsHealed = true;
                                    }
                                }
                                break;
                            case ENTRY_KORJA:
                                if (player->GetQuestStatus(QUEST_SPIRIT) == QUEST_STATUS_INCOMPLETE)
                                {
                                    if (IsHealed && !CanRun && Spell->Id == SPELL_FORTITUDE_R1)
                                    {
                                        DoScriptText(SAY_KORJA_THANKS, me, caster);
                                        CanRun = true;
                                    }
                                    else if (!IsHealed && Spell->Id == SPELL_LESSER_HEAL_R2)
                                    {
                                        CasterGUID = caster->GetGUID();
                                        me->SetStandState(UNIT_STAND_STATE_STAND);
                                        DoScriptText(SAY_COMMON_HEALED, me, caster);
                                        IsHealed = true;
                                    }
                                }
                                break;
                            case ENTRY_DG_KEL:
                                if (player->GetQuestStatus(QUEST_DARKNESS) == QUEST_STATUS_INCOMPLETE)
                                {
                                    if (IsHealed && !CanRun && Spell->Id == SPELL_FORTITUDE_R1)
                                    {
                                        DoScriptText(SAY_DG_KEL_THANKS, me, caster);
                                        CanRun = true;
                                    }
                                    else if (!IsHealed && Spell->Id == SPELL_LESSER_HEAL_R2)
                                    {
                                        CasterGUID = caster->GetGUID();
                                        me->SetStandState(UNIT_STAND_STATE_STAND);
                                        DoScriptText(SAY_COMMON_HEALED, me, caster);
                                        IsHealed = true;
                                    }
                                }
                                break;
                        }

                        //give quest credit, not expect any special quest objectives
                        if (CanRun)
                            player->TalkedToCreature(me->GetEntry(), me->GetGUID());
                    }
                }
            }

            void WaypointReached(uint32 /*waypointId*/)
            {

            }

            void UpdateAI(uint32 diff)
            {
                if (CanRun && !me->isInCombat())
                {
                    if (RunAwayTimer <= diff)
                    {
                        if (Unit* unit = Unit::GetUnit(*me, CasterGUID))
                        {
                            switch (me->GetEntry())
                            {
                                case ENTRY_SHAYA:
                                    DoScriptText(SAY_SHAYA_GOODBYE, me, unit);
                                    break;
                                case ENTRY_ROBERTS:
                                    DoScriptText(SAY_ROBERTS_GOODBYE, me, unit);
                                    break;
                                case ENTRY_DOLF:
                                    DoScriptText(SAY_DOLF_GOODBYE, me, unit);
                                    break;
                                case ENTRY_KORJA:
                                    DoScriptText(SAY_KORJA_GOODBYE, me, unit);
                                    break;
                                case ENTRY_DG_KEL:
                                    DoScriptText(SAY_DG_KEL_GOODBYE, me, unit);
                                    break;
                            }

                            Start(false, true, true);
                        }
                        else
                            EnterEvadeMode();                       //something went wrong

                        RunAwayTimer = 30000;
                    }
                    else
                        RunAwayTimer -= diff;
                }

                npc_escortAI::UpdateAI(diff);
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_garments_of_questsAI(creature);
        }
};

/*######
## npc_guardian
######*/

#define SPELL_DEATHTOUCH                5

class npc_guardian : public CreatureScript
{
    public:
        npc_guardian() : CreatureScript("npc_guardian") { }

        struct npc_guardianAI : public ScriptedAI
        {
            npc_guardianAI(Creature* creature) : ScriptedAI(creature) {}

            void Reset()
            {
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            }

            void EnterCombat(Unit* /*who*/)
            {
            }

            void UpdateAI(uint32 /*diff*/)
            {
                if (!UpdateVictim())
                    return;

                if (me->isAttackReady())
                {
                    DoCast(me->getVictim(), SPELL_DEATHTOUCH, true);
                    me->resetAttackTimer();
                }
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_guardianAI(creature);
        }
};

/*######
## npc_mount_vendor
######*/

class npc_mount_vendor : public CreatureScript
{
    public:
        npc_mount_vendor() : CreatureScript("npc_mount_vendor") { }

        bool OnGossipHello(Player* player, Creature* creature)
        {
            if (creature->isQuestGiver())
                player->PrepareQuestMenu(creature->GetGUID());

            bool canBuy = false;
            uint32 vendor = creature->GetEntry();
            uint8 race = player->getRace();

            switch (vendor)
            {
                case 384:                                           //Katie Hunter
                case 1460:                                          //Unger Statforth
                case 2357:                                          //Merideth Carlson
                case 4885:                                          //Gregor MacVince
                    if (player->GetReputationRank(72) != REP_EXALTED && race != RACE_HUMAN)
                        player->SEND_GOSSIP_MENU(5855, creature->GetGUID());
                    else canBuy = true;
                    break;
                case 1261:                                          //Veron Amberstill
                    if (player->GetReputationRank(47) != REP_EXALTED && race != RACE_DWARF)
                        player->SEND_GOSSIP_MENU(5856, creature->GetGUID());
                    else canBuy = true;
                    break;
                case 3362:                                          //Ogunaro Wolfrunner
                    if (player->GetReputationRank(76) != REP_EXALTED && race != RACE_ORC)
                        player->SEND_GOSSIP_MENU(5841, creature->GetGUID());
                    else canBuy = true;
                    break;
                case 3685:                                          //Harb Clawhoof
                    if (player->GetReputationRank(81) != REP_EXALTED && race != RACE_TAUREN)
                        player->SEND_GOSSIP_MENU(5843, creature->GetGUID());
                    else canBuy = true;
                    break;
                case 4730:                                          //Lelanai
                    if (player->GetReputationRank(69) != REP_EXALTED && race != RACE_NIGHTELF)
                        player->SEND_GOSSIP_MENU(5844, creature->GetGUID());
                    else canBuy = true;
                    break;
                case 4731:                                          //Zachariah Post
                    if (player->GetReputationRank(68) != REP_EXALTED && race != RACE_UNDEAD_PLAYER)
                        player->SEND_GOSSIP_MENU(5840, creature->GetGUID());
                    else canBuy = true;
                    break;
                case 7952:                                          //Zjolnir
                    if (player->GetReputationRank(530) != REP_EXALTED && race != RACE_TROLL)
                        player->SEND_GOSSIP_MENU(5842, creature->GetGUID());
                    else canBuy = true;
                    break;
                case 7955:                                          //Milli Featherwhistle
                    if (player->GetReputationRank(54) != REP_EXALTED && race != RACE_GNOME)
                        player->SEND_GOSSIP_MENU(5857, creature->GetGUID());
                    else canBuy = true;
                    break;
                case 16264:                                         //Winaestra
                    if (player->GetReputationRank(911) != REP_EXALTED && race != RACE_BLOODELF)
                        player->SEND_GOSSIP_MENU(10305, creature->GetGUID());
                    else canBuy = true;
                    break;
                case 17584:                                         //Torallius the Pack Handler
                    if (player->GetReputationRank(930) != REP_EXALTED && race != RACE_DRAENEI)
                        player->SEND_GOSSIP_MENU(10239, creature->GetGUID());
                    else canBuy = true;
                    break;
                case 48510:                                         //Kall Worthalon
                    if (player->GetReputationRank(1133) != REP_EXALTED && race != RACE_GOBLIN)
                        player->SEND_GOSSIP_MENU(30002, creature->GetGUID());
                    else canBuy = true;
                    break;
                case 65068:                                         //Old Whitenose
                    canBuy = true;
                    break;
                case 66022:                                         //Turtlemaster Odai
                    canBuy = true;
                    break;
            }

            if (canBuy)
            {
                if (creature->isVendor())
                    player->ADD_GOSSIP_ITEM(GOSSIP_ICON_VENDOR, GOSSIP_TEXT_BROWSE_GOODS, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_TRADE);
                player->SEND_GOSSIP_MENU(player->GetGossipTextId(creature), creature->GetGUID());
            }
            return true;
        }

        bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action)
        {
            player->PlayerTalkClass->ClearMenus();
            if (action == GOSSIP_ACTION_TRADE)
                player->GetSession()->SendListInventory(creature->GetGUID());

            return true;
        }
};

/*######
## npc_rogue_trainer
######*/

#define GOSSIP_HELLO_ROGUE1 "I wish to unlearn my talents"
#define GOSSIP_HELLO_ROGUE2 "<Take the letter>"
#define GOSSIP_HELLO_ROGUE3 "Purchase a Dual Talent Specialization."
#define GOSSIP_HELLO_ROGUE4 "I wish to unlearn my specialization"

class npc_rogue_trainer : public CreatureScript
{
    public:
        npc_rogue_trainer() : CreatureScript("npc_rogue_trainer") { }

        bool OnGossipHello(Player* player, Creature* creature)
        {
            if (creature->isQuestGiver())
                player->PrepareQuestMenu(creature->GetGUID());

            if (creature->isTrainer())
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_TRAINER, GOSSIP_TEXT_TRAIN, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_TRAIN);

            if (creature->isCanTrainingAndResetTalentsOf(player))
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_TRAINER, GOSSIP_HELLO_ROGUE1, GOSSIP_SENDER_MAIN, GOSSIP_OPTION_UNLEARNTALENTS);

            if (creature->isCanTrainingAndResetTalentsOf(player))
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_TRAINER, GOSSIP_HELLO_ROGUE4, GOSSIP_SENDER_MAIN, GOSSIP_OPTION_UNLEARNSPECIALIZATION);

            if (player->GetSpecsCount() == 1 && creature->isCanTrainingAndResetTalentsOf(player) && player->getLevel() >= sWorld->getIntConfig(CONFIG_MIN_DUALSPEC_LEVEL))
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_TRAINER, GOSSIP_HELLO_ROGUE3, GOSSIP_SENDER_MAIN, GOSSIP_OPTION_LEARNDUALSPEC);

            if (player->getClass() == CLASS_ROGUE && player->getLevel() >= 24 && !player->HasItemCount(17126) && !player->GetQuestRewardStatus(6681))
            {
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_HELLO_ROGUE2, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
                player->SEND_GOSSIP_MENU(5996, creature->GetGUID());
            } else
                player->SEND_GOSSIP_MENU(player->GetGossipTextId(creature), creature->GetGUID());

            return true;
        }

        bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action)
        {
            player->PlayerTalkClass->ClearMenus();
            switch (action)
            {
                case GOSSIP_ACTION_INFO_DEF + 1:
                    player->CLOSE_GOSSIP_MENU();
                    player->CastSpell(player, 21100, false);
                    break;
                case GOSSIP_ACTION_TRAIN:
                    player->GetSession()->SendTrainerList(creature->GetGUID());
                    break;
                case GOSSIP_OPTION_UNLEARNTALENTS:
                    player->CLOSE_GOSSIP_MENU();
                    player->SendTalentWipeConfirm(creature->GetGUID(), false);
                    break;
                case GOSSIP_OPTION_UNLEARNSPECIALIZATION:
                    player->CLOSE_GOSSIP_MENU();
                    player->SendTalentWipeConfirm(creature->GetGUID(), true);
                    break;
                case GOSSIP_OPTION_LEARNDUALSPEC:
                    if (player->GetSpecsCount() == 1 && !(player->getLevel() < sWorld->getIntConfig(CONFIG_MIN_DUALSPEC_LEVEL)))
                    {
                        if (!player->HasEnoughMoney(uint64(10000000)))
                        {
                            player->SendBuyError(BUY_ERR_NOT_ENOUGHT_MONEY, 0, 0, 0);
                            player->PlayerTalkClass->SendCloseGossip();
                            break;
                        }
                        else
                        {
                            player->ModifyMoney(int64(-10000000));

                            // Cast spells that teach dual spec
                            // Both are also ImplicitTarget self and must be cast by player
                            player->CastSpell(player, 63680, true, NULL, NULL, player->GetGUID());
                            player->CastSpell(player, 63624, true, NULL, NULL, player->GetGUID());

                            // Should show another Gossip text with "Congratulations..."
                            player->PlayerTalkClass->SendCloseGossip();
                        }
                    }
                    break;
            }
            return true;
        }
};

/*######
## npc_sayge
######*/

#define SPELL_DMG       23768                               //dmg
#define SPELL_RES       23769                               //res
#define SPELL_ARM       23767                               //arm
#define SPELL_SPI       23738                               //spi
#define SPELL_INT       23766                               //int
#define SPELL_STM       23737                               //stm
#define SPELL_STR       23735                               //str
#define SPELL_AGI       23736                               //agi
#define SPELL_FORTUNE   23765                               //faire fortune

#define GOSSIP_HELLO_SAYGE  "Yes"
#define GOSSIP_SENDACTION_SAYGE1    "Slay the Man"
#define GOSSIP_SENDACTION_SAYGE2    "Turn him over to liege"
#define GOSSIP_SENDACTION_SAYGE3    "Confiscate the corn"
#define GOSSIP_SENDACTION_SAYGE4    "Let him go and have the corn"
#define GOSSIP_SENDACTION_SAYGE5    "Execute your friend painfully"
#define GOSSIP_SENDACTION_SAYGE6    "Execute your friend painlessly"
#define GOSSIP_SENDACTION_SAYGE7    "Let your friend go"
#define GOSSIP_SENDACTION_SAYGE8    "Confront the diplomat"
#define GOSSIP_SENDACTION_SAYGE9    "Show not so quiet defiance"
#define GOSSIP_SENDACTION_SAYGE10   "Remain quiet"
#define GOSSIP_SENDACTION_SAYGE11   "Speak against your brother openly"
#define GOSSIP_SENDACTION_SAYGE12   "Help your brother in"
#define GOSSIP_SENDACTION_SAYGE13   "Keep your brother out without letting him know"
#define GOSSIP_SENDACTION_SAYGE14   "Take credit, keep gold"
#define GOSSIP_SENDACTION_SAYGE15   "Take credit, share the gold"
#define GOSSIP_SENDACTION_SAYGE16   "Let the knight take credit"
#define GOSSIP_SENDACTION_SAYGE17   "Thanks"

class npc_sayge : public CreatureScript
{
    public:
        npc_sayge() : CreatureScript("npc_sayge") { }

        bool OnGossipHello(Player* player, Creature* creature)
        {
            if (creature->isQuestGiver())
                player->PrepareQuestMenu(creature->GetGUID());

            if (player->HasSpellCooldown(SPELL_INT) ||
                player->HasSpellCooldown(SPELL_ARM) ||
                player->HasSpellCooldown(SPELL_DMG) ||
                player->HasSpellCooldown(SPELL_RES) ||
                player->HasSpellCooldown(SPELL_STR) ||
                player->HasSpellCooldown(SPELL_AGI) ||
                player->HasSpellCooldown(SPELL_STM) ||
                player->HasSpellCooldown(SPELL_SPI))
                player->SEND_GOSSIP_MENU(7393, creature->GetGUID());
            else
            {
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_HELLO_SAYGE, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
                player->SEND_GOSSIP_MENU(7339, creature->GetGUID());
            }

            return true;
        }

        void SendAction(Player* player, Creature* creature, uint32 action)
        {
            switch (action)
            {
                case GOSSIP_ACTION_INFO_DEF + 1:
                    player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_SENDACTION_SAYGE1,            GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2);
                    player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_SENDACTION_SAYGE2,            GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 3);
                    player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_SENDACTION_SAYGE3,            GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 4);
                    player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_SENDACTION_SAYGE4,            GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 5);
                    player->SEND_GOSSIP_MENU(7340, creature->GetGUID());
                    break;
                case GOSSIP_ACTION_INFO_DEF + 2:
                    player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_SENDACTION_SAYGE5,            GOSSIP_SENDER_MAIN + 1, GOSSIP_ACTION_INFO_DEF);
                    player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_SENDACTION_SAYGE6,            GOSSIP_SENDER_MAIN + 2, GOSSIP_ACTION_INFO_DEF);
                    player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_SENDACTION_SAYGE7,            GOSSIP_SENDER_MAIN + 3, GOSSIP_ACTION_INFO_DEF);
                    player->SEND_GOSSIP_MENU(7341, creature->GetGUID());
                    break;
                case GOSSIP_ACTION_INFO_DEF + 3:
                    player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_SENDACTION_SAYGE8,            GOSSIP_SENDER_MAIN + 4, GOSSIP_ACTION_INFO_DEF);
                    player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_SENDACTION_SAYGE9,            GOSSIP_SENDER_MAIN + 5, GOSSIP_ACTION_INFO_DEF);
                    player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_SENDACTION_SAYGE10,           GOSSIP_SENDER_MAIN + 2, GOSSIP_ACTION_INFO_DEF);
                    player->SEND_GOSSIP_MENU(7361, creature->GetGUID());
                    break;
                case GOSSIP_ACTION_INFO_DEF + 4:
                    player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_SENDACTION_SAYGE11,           GOSSIP_SENDER_MAIN + 6, GOSSIP_ACTION_INFO_DEF);
                    player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_SENDACTION_SAYGE12,           GOSSIP_SENDER_MAIN + 7, GOSSIP_ACTION_INFO_DEF);
                    player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_SENDACTION_SAYGE13,           GOSSIP_SENDER_MAIN + 8, GOSSIP_ACTION_INFO_DEF);
                    player->SEND_GOSSIP_MENU(7362, creature->GetGUID());
                    break;
                case GOSSIP_ACTION_INFO_DEF + 5:
                    player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_SENDACTION_SAYGE14,           GOSSIP_SENDER_MAIN + 5, GOSSIP_ACTION_INFO_DEF);
                    player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_SENDACTION_SAYGE15,           GOSSIP_SENDER_MAIN + 4, GOSSIP_ACTION_INFO_DEF);
                    player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_SENDACTION_SAYGE16,           GOSSIP_SENDER_MAIN + 3, GOSSIP_ACTION_INFO_DEF);
                    player->SEND_GOSSIP_MENU(7363, creature->GetGUID());
                    break;
                case GOSSIP_ACTION_INFO_DEF:
                    player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_SENDACTION_SAYGE17,           GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 6);
                    player->SEND_GOSSIP_MENU(7364, creature->GetGUID());
                    break;
                case GOSSIP_ACTION_INFO_DEF + 6:
                    creature->CastSpell(player, SPELL_FORTUNE, false);
                    player->SEND_GOSSIP_MENU(7365, creature->GetGUID());
                    break;
            }
        }

        bool OnGossipSelect(Player* player, Creature* creature, uint32 sender, uint32 action)
        {
            player->PlayerTalkClass->ClearMenus();
            switch (sender)
            {
                case GOSSIP_SENDER_MAIN:
                    SendAction(player, creature, action);
                    break;
                case GOSSIP_SENDER_MAIN + 1:
                    creature->CastSpell(player, SPELL_DMG, false);
                    player->AddSpellCooldown(SPELL_DMG, 0, getPreciseTime() + 7200.0);
                    SendAction(player, creature, action);
                    break;
                case GOSSIP_SENDER_MAIN + 2:
                    creature->CastSpell(player, SPELL_RES, false);
                    player->AddSpellCooldown(SPELL_RES, 0, getPreciseTime() + 7200.0);
                    SendAction(player, creature, action);
                    break;
                case GOSSIP_SENDER_MAIN + 3:
                    creature->CastSpell(player, SPELL_ARM, false);
                    player->AddSpellCooldown(SPELL_ARM, 0, getPreciseTime() + 7200.0);
                    SendAction(player, creature, action);
                    break;
                case GOSSIP_SENDER_MAIN + 4:
                    creature->CastSpell(player, SPELL_SPI, false);
                    player->AddSpellCooldown(SPELL_SPI, 0, getPreciseTime() + 7200.0);
                    SendAction(player, creature, action);
                    break;
                case GOSSIP_SENDER_MAIN + 5:
                    creature->CastSpell(player, SPELL_INT, false);
                    player->AddSpellCooldown(SPELL_INT, 0, getPreciseTime() + 7200.0);
                    SendAction(player, creature, action);
                    break;
                case GOSSIP_SENDER_MAIN + 6:
                    creature->CastSpell(player, SPELL_STM, false);
                    player->AddSpellCooldown(SPELL_STM, 0, getPreciseTime() + 7200.0);
                    SendAction(player, creature, action);
                    break;
                case GOSSIP_SENDER_MAIN + 7:
                    creature->CastSpell(player, SPELL_STR, false);
                    player->AddSpellCooldown(SPELL_STR, 0, getPreciseTime() + 7200.0);
                    SendAction(player, creature, action);
                    break;
                case GOSSIP_SENDER_MAIN + 8:
                    creature->CastSpell(player, SPELL_AGI, false);
                    player->AddSpellCooldown(SPELL_AGI, 0, getPreciseTime() + 7200.0);
                    SendAction(player, creature, action);
                    break;
            }
            return true;
        }
};

class npc_steam_tonk : public CreatureScript
{
    public:
        npc_steam_tonk() : CreatureScript("npc_steam_tonk") { }

        struct npc_steam_tonkAI : public ScriptedAI
        {
            npc_steam_tonkAI(Creature* creature) : ScriptedAI(creature) {}

            void Reset() {}
            void EnterCombat(Unit* /*who*/) {}

            void OnPossess(bool apply)
            {
                if (apply)
                {
                    // Initialize the action bar without the melee attack command
                    me->InitCharmInfo();
                    me->GetCharmInfo()->InitEmptyActionBar(false);

                    me->SetReactState(REACT_PASSIVE);
                }
                else
                    me->SetReactState(REACT_AGGRESSIVE);
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_steam_tonkAI(creature);
        }
};

#define SPELL_TONK_MINE_DETONATE 25099

class npc_tonk_mine : public CreatureScript
{
    public:
        npc_tonk_mine() : CreatureScript("npc_tonk_mine") { }

        struct npc_tonk_mineAI : public ScriptedAI
        {
            npc_tonk_mineAI(Creature* creature) : ScriptedAI(creature)
            {
                me->SetReactState(REACT_PASSIVE);
            }

            uint32 ExplosionTimer;

            void Reset()
            {
                ExplosionTimer = 3000;
            }

            void EnterCombat(Unit* /*who*/) {}
            void AttackStart(Unit* /*who*/) {}
            void MoveInLineOfSight(Unit* /*who*/) {}

            void UpdateAI(uint32 diff)
            {
                if (ExplosionTimer <= diff)
                {
                    DoCast(me, SPELL_TONK_MINE_DETONATE, true);
                    me->setDeathState(DEAD); // unsummon it
                }
                else
                    ExplosionTimer -= diff;
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_tonk_mineAI(creature);
        }
};

/*####
## npc_brewfest_reveler
####*/

class npc_brewfest_reveler : public CreatureScript
{
    public:
        npc_brewfest_reveler() : CreatureScript("npc_brewfest_reveler") { }

        struct npc_brewfest_revelerAI : public ScriptedAI
        {
            npc_brewfest_revelerAI(Creature* creature) : ScriptedAI(creature) {}
            void ReceiveEmote(Player* player, uint32 emote)
            {
                if (!IsHolidayActive(HOLIDAY_BREWFEST))
                    return;

                if (emote == TEXT_EMOTE_DANCE)
                    me->CastSpell(player, 41586, false);
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_brewfest_revelerAI(creature);
        }
};

/*####
## npc_brewfest_trigger
####*/
enum eBrewfestBarkQuests
{
    BARK_FOR_THE_THUNDERBREWS       = 11294,
    BARK_FOR_TCHALIS_VOODOO_BREWERY = 11408,
    BARK_FOR_THE_BARLEYBREWS        = 11293,
    BARK_FOR_DROHNS_DISTILLERY      = 11407
};

class npc_brewfest_trigger : public CreatureScript
{
public:
    npc_brewfest_trigger() : CreatureScript("npc_brewfest_trigger") { }

    struct npc_brewfest_triggerAI : public ScriptedAI
    {
        npc_brewfest_triggerAI(Creature* c) : ScriptedAI(c) {}
        void MoveInLineOfSight(Unit *who)
        {
            Player *pPlayer = who->ToPlayer();
            if (!pPlayer)
                return;
            if (pPlayer->GetQuestStatus(BARK_FOR_THE_THUNDERBREWS) == QUEST_STATUS_INCOMPLETE
                || pPlayer->GetQuestStatus(BARK_FOR_TCHALIS_VOODOO_BREWERY) == QUEST_STATUS_INCOMPLETE
                || pPlayer->GetQuestStatus(BARK_FOR_THE_BARLEYBREWS) == QUEST_STATUS_INCOMPLETE
                || pPlayer->GetQuestStatus(BARK_FOR_DROHNS_DISTILLERY) == QUEST_STATUS_INCOMPLETE)
                pPlayer->KilledMonsterCredit(me->GetEntry(),0);
        }
    };

    CreatureAI *GetAI(Creature *creature) const
    {
        return new npc_brewfest_triggerAI(creature);
    }
};

/*####
## npc_brewfest_apple_trigger
####*/

#define SPELL_RAM_FATIGUE    43052

class npc_brewfest_apple_trigger : public CreatureScript
{
public:
    npc_brewfest_apple_trigger() : CreatureScript("npc_brewfest_apple_trigger") { }

    struct npc_brewfest_apple_triggerAI : public ScriptedAI
    {
        npc_brewfest_apple_triggerAI(Creature* c) : ScriptedAI(c) {}
        void MoveInLineOfSight(Unit *who)
        {
            Player *pPlayer = who->ToPlayer();
            if (!pPlayer)
                return;
            if (pPlayer->HasAura(SPELL_RAM_FATIGUE) && me->GetDistance(pPlayer->GetPositionX(),pPlayer->GetPositionY(),pPlayer->GetPositionZ()) <= 7.5f)
                pPlayer->RemoveAura(SPELL_RAM_FATIGUE);
        }
    };

    CreatureAI *GetAI(Creature *creature) const
    {
        return new npc_brewfest_apple_triggerAI(creature);
    }
};

/*####
## npc_brewfest_keg_thrower
####*/

enum eBrewfestKegThrower
{
    SPELL_BREWFEST_RAM   = 43880,
    SPELL_THROW_KEG      = 43660,
    ITEM_BREWFEST_KEG    = 33797
};

class npc_brewfest_keg_thrower : public CreatureScript
{
public:
    npc_brewfest_keg_thrower() : CreatureScript("npc_brewfest_keg_thrower") { }

    struct npc_brewfest_keg_throwerAI : public ScriptedAI
    {
        npc_brewfest_keg_throwerAI(Creature* c) : ScriptedAI(c) {}
        void MoveInLineOfSight(Unit *who)
        {
            Player *pPlayer = who->ToPlayer();
            if (!pPlayer)
                return;
            if (pPlayer->HasAura(SPELL_BREWFEST_RAM) 
                && me->GetDistance(pPlayer->GetPositionX(),pPlayer->GetPositionY(),pPlayer->GetPositionZ()) <= 25.0f
                && !pPlayer->HasItemCount(ITEM_BREWFEST_KEG,1))
            {
                me->CastSpell(pPlayer,SPELL_THROW_KEG,false);
                me->CastSpell(pPlayer,42414,false);
            }
        }
    };

    CreatureAI *GetAI(Creature *creature) const
    {
        return new npc_brewfest_keg_throwerAI(creature);
    }
};

/*####
## npc_brewfest_keg_receiver
####*/

enum eBrewfestKegReceiver
{
    SPELL_CREATE_TICKETS            = 44501,
    QUEST_THERE_AND_BACK_AGAIN_A    = 11122,
    QUEST_THERE_AND_BACK_AGAIN_H    = 11412,
    NPC_BREWFEST_DELIVERY_BUNNY     = 24337   
};

class npc_brewfest_keg_receiver : public CreatureScript
{
public:
    npc_brewfest_keg_receiver() : CreatureScript("npc_brewfest_keg_receiver") { }

    struct npc_brewfest_keg_receiverAI : public ScriptedAI
    {
        npc_brewfest_keg_receiverAI(Creature* c) : ScriptedAI(c) {}
        void MoveInLineOfSight(Unit *who)
        {
            Player *pPlayer = who->ToPlayer();
            if (!pPlayer)
                return;
            if (pPlayer->HasAura(SPELL_BREWFEST_RAM) 
                && me->GetDistance(pPlayer->GetPositionX(),pPlayer->GetPositionY(),pPlayer->GetPositionZ()) <= 5.0f
                && pPlayer->HasItemCount(ITEM_BREWFEST_KEG,1)) 
            {
                pPlayer->CastSpell(me,SPELL_THROW_KEG,true);
                pPlayer->DestroyItemCount(ITEM_BREWFEST_KEG,1,true);
                pPlayer->GetAura(SPELL_BREWFEST_RAM)->SetDuration(pPlayer->GetAura(SPELL_BREWFEST_RAM)->GetDuration() + 30000);
                if (pPlayer->GetQuestRewardStatus(QUEST_THERE_AND_BACK_AGAIN_A) 
                    || pPlayer->GetQuestRewardStatus(QUEST_THERE_AND_BACK_AGAIN_H))
                {
                    pPlayer->CastSpell(pPlayer,SPELL_CREATE_TICKETS,true);
                }
                else
                {
                    pPlayer->KilledMonsterCredit(NPC_BREWFEST_DELIVERY_BUNNY,0);
                    if (pPlayer->GetQuestStatus(QUEST_THERE_AND_BACK_AGAIN_A) == QUEST_STATUS_INCOMPLETE) 
                        pPlayer->AreaExploredOrEventHappens(QUEST_THERE_AND_BACK_AGAIN_A);
                    if (pPlayer->GetQuestStatus(QUEST_THERE_AND_BACK_AGAIN_H) == QUEST_STATUS_INCOMPLETE) 
                        pPlayer->AreaExploredOrEventHappens(QUEST_THERE_AND_BACK_AGAIN_H);
                    if (pPlayer->GetQuestStatus(QUEST_THERE_AND_BACK_AGAIN_A) == QUEST_STATUS_COMPLETE
                        || pPlayer->GetQuestStatus(QUEST_THERE_AND_BACK_AGAIN_H) == QUEST_STATUS_COMPLETE)
                        pPlayer->RemoveAura(SPELL_BREWFEST_RAM);
                }
            }
        }
    };

    CreatureAI *GetAI(Creature *creature) const
    {
        return new npc_brewfest_keg_receiverAI(creature);
    }
};

/*####
## npc_brewfest_ram_master
####*/
#define GOSSIP_ITEM_RAM             "Do you have additional work?"
#define GOSSIP_ITEM_RAM_REINS       "Give me another Ram Racing Reins"
#define SPELL_BREWFEST_SUMMON_RAM   43720

class npc_brewfest_ram_master : public CreatureScript
{
public:
    npc_brewfest_ram_master() : CreatureScript("npc_brewfest_ram_master") { }

    bool OnGossipHello(Player *pPlayer, Creature *pCreature)
    {
        if (pCreature->isQuestGiver())
            pPlayer->PrepareQuestMenu(pCreature->GetGUID());

            if (pPlayer->HasSpellCooldown(SPELL_BREWFEST_SUMMON_RAM) 
                && !pPlayer->GetQuestRewardStatus(QUEST_THERE_AND_BACK_AGAIN_A) 
                && !pPlayer->GetQuestRewardStatus(QUEST_THERE_AND_BACK_AGAIN_H)
                && (pPlayer->GetQuestStatus(QUEST_THERE_AND_BACK_AGAIN_A) == QUEST_STATUS_INCOMPLETE
                || pPlayer->GetQuestStatus(QUEST_THERE_AND_BACK_AGAIN_H) == QUEST_STATUS_INCOMPLETE))
                pPlayer->RemoveSpellCooldown(SPELL_BREWFEST_SUMMON_RAM);

            if (!pPlayer->HasAura(SPELL_BREWFEST_RAM) && ((pPlayer->GetQuestStatus(QUEST_THERE_AND_BACK_AGAIN_A) == QUEST_STATUS_INCOMPLETE 
            || pPlayer->GetQuestStatus(QUEST_THERE_AND_BACK_AGAIN_H) == QUEST_STATUS_INCOMPLETE 
            || (!pPlayer->HasSpellCooldown(SPELL_BREWFEST_SUMMON_RAM) && 
                (pPlayer->GetQuestRewardStatus(QUEST_THERE_AND_BACK_AGAIN_A) 
                || pPlayer->GetQuestRewardStatus(QUEST_THERE_AND_BACK_AGAIN_H))))))
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ITEM_RAM, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1);

            if ((pPlayer->GetQuestRewardStatus(QUEST_THERE_AND_BACK_AGAIN_A) 
                || pPlayer->GetQuestRewardStatus(QUEST_THERE_AND_BACK_AGAIN_H))
                && !pPlayer->HasItemCount(33306,1,true))
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ITEM_RAM_REINS, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+2);



        pPlayer->SEND_GOSSIP_MENU(384, pCreature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* pPlayer, Creature* pCreature, uint32 /*uiSender*/, uint32 uiAction)
    {
        pPlayer->PlayerTalkClass->SendCloseGossip();
        if (uiAction == GOSSIP_ACTION_INFO_DEF+1)
        {
            if (pPlayer->HasItemCount(ITEM_BREWFEST_KEG,1)) 
                pPlayer->DestroyItemCount(ITEM_BREWFEST_KEG,1,true);
            pPlayer->CastSpell(pPlayer,SPELL_BREWFEST_SUMMON_RAM,true);
            pPlayer->AddSpellCooldown(SPELL_BREWFEST_SUMMON_RAM,0, getPreciseTime() + 64800.0);
        }
        if (uiAction == GOSSIP_ACTION_INFO_DEF+2)
        {
            pPlayer->CastSpell(pPlayer,44371,false);
        }
        return true;
    }
};

/*####
## npc_winter_reveler
####*/

enum WinterReveler
{
    SPELL_MISTLETOE_DEBUFF       = 26218,
    SPELL_CREATE_MISTLETOE       = 26206,
    SPELL_CREATE_HOLLY           = 26207,
    SPELL_CREATE_SNOWFLAKES      = 45036,
};

class npc_winter_reveler : public CreatureScript
{
    public:
        npc_winter_reveler() : CreatureScript("npc_winter_reveler") { }

        struct npc_winter_revelerAI : public ScriptedAI
        {
            npc_winter_revelerAI(Creature* creature) : ScriptedAI(creature) {}

            void ReceiveEmote(Player* player, uint32 emote)
            {
                if (player->HasAura(SPELL_MISTLETOE_DEBUFF))
                    return;

                if (!IsHolidayActive(HOLIDAY_FEAST_OF_WINTER_VEIL))
                    return;

                if (emote == TEXT_EMOTE_KISS)
                {
                    uint32 spellId = RAND<uint32>(SPELL_CREATE_MISTLETOE, SPELL_CREATE_HOLLY, SPELL_CREATE_SNOWFLAKES);
                    me->CastSpell(player, spellId, false);
                    me->CastSpell(player, SPELL_MISTLETOE_DEBUFF, false);
                }
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_winter_revelerAI(creature);
        }
};

/*####
## npc_snake_trap_serpents
####*/

#define SPELL_MIND_NUMBING_POISON    25810   //Viper
#define SPELL_DEADLY_POISON          34655   //Venomous Snake
#define SPELL_CRIPPLING_POISON       30981   //Viper

#define VENOMOUS_SNAKE_TIMER 1500
#define VIPER_TIMER 3000

#define C_VIPER 19921

class npc_snake_trap : public CreatureScript
{
    public:
        npc_snake_trap() : CreatureScript("npc_snake_trap_serpents") { }

        struct npc_snake_trap_serpentsAI : public ScriptedAI
        {
            npc_snake_trap_serpentsAI(Creature* creature) : ScriptedAI(creature) {}

            uint32 SpellTimer;
            bool IsViper;

            void EnterCombat(Unit* /*who*/) {}

            void Reset()
            {
                SpellTimer = 0;

                CreatureTemplate const* Info = me->GetCreatureTemplate();

                IsViper = Info->Entry == C_VIPER ? true : false;

                me->SetMaxHealth(uint32(107 * (me->getLevel() - 40) * 0.025f));
                //Add delta to make them not all hit the same time
                uint32 delta = (rand() % 7) * 100;
                me->SetStatFloatValue(UNIT_FIELD_BASEATTACKTIME, float(Info->baseattacktime + delta));
                me->SetInt32Value(UNIT_FIELD_ATTACK_POWER, int32(Info->attackpower));

                // Start attacking attacker of owner on first ai update after spawn - move in line of sight may choose better target
                if (!me->getVictim() && me->isSummon())
                    if (Unit* Owner = me->ToTempSummon()->GetSummoner())
                    {
                        if(Owner->HasAura(56849)) // Glyph of Snake Trap
                        {
                            me->AddAura(65220, me);
                            me->AddAura(111323, me);
                        }
                        if (Owner->getAttackerForHelper())
                            AttackStart(Owner->getAttackerForHelper());
                    }
            }

            //Redefined for random target selection:
            void MoveInLineOfSight(Unit* who)
            {
                if (!me->getVictim() && me->canCreatureAttack(who))
                {
                    if (me->GetDistanceZ(who) > CREATURE_Z_ATTACK_RANGE)
                        return;

                    float attackRadius = me->GetAttackDistance(who);
                    if (me->IsWithinDistInMap(who, attackRadius) && me->IsWithinLOSInMap(who))
                    {
                        if (!(rand() % 5))
                        {
                            me->setAttackTimer(BASE_ATTACK, (rand() % 10) * 100);
                            SpellTimer = (rand() % 10) * 100;
                            AttackStart(who);
                        }
                    }
                }
            }

            void UpdateAI(uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                if (me->getVictim()->HasCrowdControlAura(me))
                {
                    me->InterruptNonMeleeSpells(false);
                    return;
                }

                if (SpellTimer <= diff)
                {
                    if (IsViper) //Viper
                    {
                        if (urand(0, 2) == 0) //33% chance to cast
                        {
                            uint32 spell;
                            if (urand(0, 1) == 0)
                                spell = SPELL_MIND_NUMBING_POISON;
                            else
                                spell = SPELL_CRIPPLING_POISON;

                            DoCast(me->getVictim(), spell);
                        }

                        SpellTimer = VIPER_TIMER;
                    }
                    else //Venomous Snake
                    {
                        if (urand(0, 2) == 0) //33% chance to cast
                            DoCast(me->getVictim(), SPELL_DEADLY_POISON);
                        SpellTimer = VENOMOUS_SNAKE_TIMER + (rand() % 5) * 100;
                    }
                }
                else
                    SpellTimer -= diff;

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_snake_trap_serpentsAI(creature);
        }
};

#define SAY_RANDOM_MOJO0    "Now that's what I call froggy-style!"
#define SAY_RANDOM_MOJO1    "Your lily pad or mine?"
#define SAY_RANDOM_MOJO2    "This won't take long, did it?"
#define SAY_RANDOM_MOJO3    "I thought you'd never ask!"
#define SAY_RANDOM_MOJO4    "I promise not to give you warts..."
#define SAY_RANDOM_MOJO5    "Feelin' a little froggy, are ya?"
#define SAY_RANDOM_MOJO6a   "Listen, "
#define SAY_RANDOM_MOJO6b   ", I know of a little swamp not too far from here...."
#define SAY_RANDOM_MOJO7    "There's just never enough Mojo to go around..."

class mob_mojo : public CreatureScript
{
    public:
        mob_mojo() : CreatureScript("mob_mojo") { }

        struct mob_mojoAI : public ScriptedAI
        {
            mob_mojoAI(Creature* creature) : ScriptedAI(creature) {Reset();}
            uint32 hearts;
            uint64 victimGUID;
            void Reset()
            {
                victimGUID = 0;
                hearts = 15000;
                if (Unit* own = me->GetOwner())
                    me->GetMotionMaster()->MoveFollow(own, 0, 0);
            }

            void EnterCombat(Unit* /*who*/){}

            void UpdateAI(uint32 diff)
            {
                if (me->HasAura(20372))
                {
                    if (hearts <= diff)
                    {
                        me->RemoveAurasDueToSpell(20372);
                        hearts = 15000;
                    } hearts -= diff;
                }
            }

            void ReceiveEmote(Player* player, uint32 emote)
            {
                me->HandleEmoteCommand(emote);
                Unit* own = me->GetOwner();
                if (!own || own->GetTypeId() != TYPEID_PLAYER || CAST_PLR(own)->GetTeam() != player->GetTeam())
                    return;
                if (emote == TEXT_EMOTE_KISS)
                {
                    std::string whisp = "";
                    switch (rand() % 8)
                    {
                        case 0:
                            whisp.append(SAY_RANDOM_MOJO0);
                            break;
                        case 1:
                            whisp.append(SAY_RANDOM_MOJO1);
                            break;
                        case 2:
                            whisp.append(SAY_RANDOM_MOJO2);
                            break;
                        case 3:
                            whisp.append(SAY_RANDOM_MOJO3);
                            break;
                        case 4:
                            whisp.append(SAY_RANDOM_MOJO4);
                            break;
                        case 5:
                            whisp.append(SAY_RANDOM_MOJO5);
                            break;
                        case 6:
                            whisp.append(SAY_RANDOM_MOJO6a);
                            whisp.append(player->GetName());
                            whisp.append(SAY_RANDOM_MOJO6b);
                            break;
                        case 7:
                            whisp.append(SAY_RANDOM_MOJO7);
                            break;
                    }

                    me->MonsterWhisper(whisp.c_str(), player->GetGUID());
                    if (victimGUID)
                        if (Player* victim = Unit::GetPlayer(*me, victimGUID))
                            victim->RemoveAura(43906);//remove polymorph frog thing
                    me->AddAura(43906, player);//add polymorph frog thing
                    victimGUID = player->GetGUID();
                    DoCast(me, 20372, true);//tag.hearts
                    me->GetMotionMaster()->MoveFollow(player, 0, 0);
                    hearts = 15000;
                }
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new mob_mojoAI(creature);
        }
};

class npc_mirror_image : public CreatureScript
{
    public:
        npc_mirror_image() : CreatureScript("npc_mirror_image") { }

        struct npc_mirror_imageAI : CasterAI
        {
            npc_mirror_imageAI(Creature* creature) : CasterAI(creature) {}

            void InitializeAI()
            {
                CasterAI::InitializeAI();
                Unit* owner = me->GetOwner();
                if (!owner)
                    return;
                me->SetDisplayId(owner->GetDisplayId());
                me->SetReactState(REACT_DEFENSIVE);
                // Inherit Master's Threat List (not yet implemented)
                owner->CastSpell((Unit*)NULL, 58838, true);
                // here mirror image casts on summoner spell (not present in client dbc) 49866
                // here should be auras (not present in client dbc): 35657, 35658, 35659, 35660 selfcasted by mirror images (stats related?)
                // Clone Me!
                owner->CastSpell(me, 45204, true);
                me->SetCasterPet(true);
                me->SetAttackDist(40.0f);
                me->SetSpeed(MOVE_RUN, owner->GetSpeedRate(MOVE_RUN));
                me->SetSpeed(MOVE_SWIM, owner->GetSpeedRate(MOVE_SWIM));
                me->SetFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_MIRROR_IMAGE);

                if (owner->getVictim())
                    me->AI()->AttackStart(owner->getVictim());
            }

            void EnterCombat(Unit* who)
            {
                me->CastSpell(who, 59638, false);
                events.ScheduleEvent(59638, 2500);
            }

            void UpdateAI(uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                events.Update(diff);

                bool hasCC = false;
                if (me->GetCharmerOrOwnerGUID() && me->getVictim())
                    hasCC = me->getVictim()->HasNegativeAuraWithInterruptFlag(AURA_INTERRUPT_FLAG_TAKE_DAMAGE);

                if (hasCC)
                {
                    if (me->HasUnitState(UNIT_STATE_CASTING))
                        me->CastStop();

                    me->AI()->EnterEvadeMode();
                    return;
                }

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                if (uint32 spellId = events.ExecuteEvent())
                {
                    if (hasCC)
                    {
                        events.ScheduleEvent(spellId, 500);
                        return;
                    }
                    DoCast(spellId);
                    events.ScheduleEvent(spellId, 2500);
                }
            }

            // Do not reload Creature templates on evade mode enter - prevent visual lost
            void EnterEvadeMode()
            {
                if (me->IsInEvadeMode() || !me->isAlive())
                    return;

                Unit* owner = me->GetCharmerOrOwner();

                me->CombatStop(true);
                if (owner && !me->HasUnitState(UNIT_STATE_FOLLOW))
                {
                    me->GetMotionMaster()->Clear(false);
                    me->GetMotionMaster()->MoveFollow(owner, PET_FOLLOW_DIST, me->GetFollowAngle(), MOTION_SLOT_ACTIVE);
                }
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_mirror_imageAI(creature);
        }
};

class npc_ebon_gargoyle : public CreatureScript
{
    public:
        npc_ebon_gargoyle() : CreatureScript("npc_ebon_gargoyle") { }

        struct npc_ebon_gargoyleAI : CasterAI
        {
            npc_ebon_gargoyleAI(Creature* creature) : CasterAI(creature) {}

            uint32 despawnTimer;

            void InitializeAI()
            {
                CasterAI::InitializeAI();
                uint64 ownerGuid = me->GetOwnerGUID();
                if (!ownerGuid)
                    return;
                // Not needed to be despawned now
                despawnTimer = 0;
                // Find victim of Summon Gargoyle spell
                std::list<Unit*> targets;
                Trinity::AnyUnfriendlyUnitInObjectRangeCheck u_check(me, me, 30);
                Trinity::UnitListSearcher<Trinity::AnyUnfriendlyUnitInObjectRangeCheck> searcher(me, targets, u_check);
                me->VisitNearbyObject(30, searcher);
                for (std::list<Unit*>::const_iterator iter = targets.begin(); iter != targets.end(); ++iter)
                    if ((*iter)->GetAura(49206, ownerGuid))
                    {
                        me->Attack((*iter), false);
                        break;
                    }
            }

            void JustDied(Unit* /*killer*/)
            {
                // Stop Feeding Gargoyle when it dies
                if (Unit* owner = me->GetOwner())
                    owner->RemoveAurasDueToSpell(50514);
            }

            // Fly away when dismissed
            void SpellHit(Unit* source, SpellInfo const* spell)
            {
                if (spell->Id != 50515 || !me->isAlive())
                    return;

                Unit* owner = me->GetOwner();

                if (!owner || owner != source)
                    return;

                // Stop Fighting
                me->ApplyModFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE, true);
                // Sanctuary
                me->CastSpell(me, 54661, true);
                me->SetReactState(REACT_PASSIVE);

                //! HACK: Creature's can't have MOVEMENTFLAG_FLYING
                // Fly Away
                me->AddUnitMovementFlag(MOVEMENTFLAG_CAN_FLY|MOVEMENTFLAG_ASCENDING|MOVEMENTFLAG_FLYING);
                me->SetSpeed(MOVE_FLIGHT, 0.75f, true);
                me->SetSpeed(MOVE_RUN, 0.75f, true);
                float x = me->GetPositionX() + 20 * std::cos(me->GetOrientation());
                float y = me->GetPositionY() + 20 * std::sin(me->GetOrientation());
                float z = me->GetPositionZ() + 40;
                me->GetMotionMaster()->Clear(false);
                me->GetMotionMaster()->MovePoint(0, x, y, z);

                // Despawn as soon as possible
                despawnTimer = 4 * IN_MILLISECONDS;
            }

            void UpdateAI(uint32 diff)
            {
                if (despawnTimer > 0)
                {
                    if (despawnTimer > diff)
                        despawnTimer -= diff;
                    else
                        me->DespawnOrUnsummon();
                    return;
                }
                CasterAI::UpdateAI(diff);
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_ebon_gargoyleAI(creature);
        }
};

class npc_lightwell : public CreatureScript
{
    public:
        npc_lightwell() : CreatureScript("npc_lightwell") { }

        struct npc_lightwellAI : public PassiveAI
        {
            npc_lightwellAI(Creature* creature) : PassiveAI(creature)
            {
                if (TempSummon* summon = me->ToTempSummon())
                    if(Unit* owner = summon->GetSummoner())
                        me->SetLevel(owner->getLevel());
                //DoCast(me, 59907, false);
                me->SetAuraStack(59907, me, 17);
            }

            void EnterEvadeMode()
            {
                if (!me->isAlive())
                    return;

                me->DeleteThreatList();
                me->CombatStop(true);
                me->ResetPlayerDamageReq();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_lightwellAI(creature);
        }
};

class npc_lightwell_mop : public CreatureScript
{
    public:
        npc_lightwell_mop() : CreatureScript("npc_lightwell_mop") { }

        struct npc_lightwell_mopAI : public PassiveAI
        {
            npc_lightwell_mopAI(Creature* creature) : PassiveAI(creature)
            {
                if (TempSummon* summon = me->ToTempSummon())
                    if(Unit* owner = summon->GetSummoner())
                    {
                        me->SetLevel(owner->getLevel());
                        me->SetMaxHealth(CalculatePct(owner->GetMaxHealth(), 50));
                        me->SetFullHealth();
                        me->SetMaxPower(POWER_RAGE, 0);
                        me->SetFloatValue(UNIT_FIELD_COMBATREACH, 10);
                    }

                DoCast(me, 126138, true);
                me->SetAuraStack(126150, me, 17);
            }

            void EnterEvadeMode()
            {
                if (!me->isAlive())
                    return;

                me->DeleteThreatList();
                me->CombatStop(true);
                me->ResetPlayerDamageReq();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_lightwell_mopAI(creature);
        }
};

enum eTrainingDummy
{
    NPC_ADVANCED_TARGET_DUMMY                  = 2674,
    NPC_TARGET_DUMMY                           = 2673
};

class npc_training_dummy : public CreatureScript
{
    public:
        npc_training_dummy() : CreatureScript("npc_training_dummy") { }

        struct npc_training_dummyAI : Scripted_NoMovementAI
        {
            npc_training_dummyAI(Creature* creature) : Scripted_NoMovementAI(creature)
            {
                entry = creature->GetEntry();
            }

            uint32 entry;
            uint32 resetTimer;
            uint32 despawnTimer;

            void Reset()
            {
                me->SetControlled(true, UNIT_STATE_STUNNED);//disable rotate
                me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK, true);//imune to knock aways like blast wave

                resetTimer = 5000;
                despawnTimer = 15000;
            }
            
            void SpellHit(Unit* source, SpellInfo const* spell)
            {
                if(source)
                {
                    Player* player = source->ToPlayer();
                    if(!player)
                        return;
                    
                    switch (spell->Id)
                    {
                        case 100:
                        case 172:
                        case 589:
                        case 8921:
                        case 20271:
                        case 73899:
                        case 100787:
                        case 118215:
                        {
                            player->KilledMonsterCredit(44175, 0);
                            break;
                        }
                        default:
                            break;
                    }          
                }
            }
            
            void EnterEvadeMode()
            {
                if (!_EnterEvadeMode())
                    return;

                Reset();
            }

            void DamageTaken(Unit* doneBy, uint32& damage)
            {
                resetTimer = 5000;
                damage = 0;
            }

            void EnterCombat(Unit* who)
            {
                if (entry != NPC_ADVANCED_TARGET_DUMMY && entry != NPC_TARGET_DUMMY)
                    return;
            }

            void UpdateAI(uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                if (!me->HasUnitState(UNIT_STATE_STUNNED))
                    me->SetControlled(true, UNIT_STATE_STUNNED);//disable rotate

                if (entry != NPC_ADVANCED_TARGET_DUMMY && entry != NPC_TARGET_DUMMY)
                {
                    if (resetTimer <= diff)
                    {
                        EnterEvadeMode();
                        resetTimer = 5000;
                    }
                    else
                        resetTimer -= diff;
                    return;
                }
                else
                {
                    if (despawnTimer <= diff)
                        me->DespawnOrUnsummon();
                    else
                        despawnTimer -= diff;
                }
            }
            void MoveInLineOfSight(Unit* who){return;}
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_training_dummyAI(creature);
        }
};

/*######
# npc_fire_elemental
######*/

class npc_fire_elemental : public CreatureScript
{
    public:
        npc_fire_elemental() : CreatureScript("npc_fire_elemental") { }

        struct npc_fire_elementalAI : public ScriptedAI
        {
            npc_fire_elementalAI(Creature* creature) : ScriptedAI(creature) {}

            uint32 FireBlastTimer;

            void Reset()
            {
                me->ApplySpellImmune(0, IMMUNITY_SCHOOL, SPELL_SCHOOL_MASK_FIRE, true);
                FireBlastTimer = 6000;
            }

            void UpdateAI(uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                if (FireBlastTimer <= diff)
                {
                    me->CastSpell(me->getVictim(), 57984, true);
                    FireBlastTimer = 6000;
                }
                else
                    FireBlastTimer -= diff;

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_fire_elementalAI(creature);
        }
};

/*######
# npc_earth_elemental
######*/
#define SPELL_ANGEREDEARTH        36213

class npc_earth_elemental : public CreatureScript
{
    public:
        npc_earth_elemental() : CreatureScript("npc_earth_elemental") { }

        struct npc_earth_elementalAI : public ScriptedAI
        {
            npc_earth_elementalAI(Creature* creature) : ScriptedAI(creature) {}

            uint32 AngeredEarth_Timer;

            void Reset()
            {
                AngeredEarth_Timer = 0;
                me->ApplySpellImmune(0, IMMUNITY_SCHOOL, SPELL_SCHOOL_MASK_NATURE, true);
            }

            void UpdateAI(uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                if (AngeredEarth_Timer <= diff)
                {
                    DoCast(me->getVictim(), SPELL_ANGEREDEARTH);
                    AngeredEarth_Timer = 5000 + rand() % 15000; // 5-20 sec cd
                }
                else
                    AngeredEarth_Timer -= diff;

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_earth_elementalAI(creature);
        }
};

/*######
# npc_wormhole
######*/

#define GOSSIP_ENGINEERING1   "Borean Tundra"
#define GOSSIP_ENGINEERING2   "Howling Fjord"
#define GOSSIP_ENGINEERING3   "Sholazar Basin"
#define GOSSIP_ENGINEERING4   "Icecrown"
#define GOSSIP_ENGINEERING5   "Storm Peaks"
#define GOSSIP_ENGINEERING6   "Underground..."

enum WormholeSpells
{
    SPELL_BOREAN_TUNDRA         = 67834,
    SPELL_SHOLAZAR_BASIN        = 67835,
    SPELL_ICECROWN              = 67836,
    SPELL_STORM_PEAKS           = 67837,
    SPELL_HOWLING_FJORD         = 67838,
    SPELL_UNDERGROUND           = 68081,

    TEXT_WORMHOLE               = 907,

    DATA_SHOW_UNDERGROUND       = 1,
};

class npc_wormhole : public CreatureScript
{
    public:
        npc_wormhole() : CreatureScript("npc_wormhole") {}

        struct npc_wormholeAI : public PassiveAI
        {
            npc_wormholeAI(Creature* creature) : PassiveAI(creature) {}

            void InitializeAI()
            {
                _showUnderground = urand(0, 100) == 0; // Guessed value, it is really rare though
            }

            uint32 GetData(uint32 type)
            {
                return (type == DATA_SHOW_UNDERGROUND && _showUnderground) ? 1 : 0;
            }

        private:
            bool _showUnderground;
        };

        bool OnGossipHello(Player* player, Creature* creature)
        {
            if (creature->isSummon())
            {
                if (player == creature->ToTempSummon()->GetSummoner())
                {
                    player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ENGINEERING1, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
                    player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ENGINEERING2, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2);
                    player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ENGINEERING3, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 3);
                    player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ENGINEERING4, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 4);
                    player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ENGINEERING5, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 5);

                    if (creature->AI()->GetData(DATA_SHOW_UNDERGROUND))
                        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ENGINEERING6, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 6);

                    player->PlayerTalkClass->SendGossipMenu(TEXT_WORMHOLE, creature->GetGUID());
                }
            }

            return true;
        }

        bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action)
        {
            player->PlayerTalkClass->ClearMenus();

            switch (action)
            {
                case GOSSIP_ACTION_INFO_DEF + 1: // Borean Tundra
                    player->CLOSE_GOSSIP_MENU();
                    creature->CastSpell(player, SPELL_BOREAN_TUNDRA, false);
                    break;
                case GOSSIP_ACTION_INFO_DEF + 2: // Howling Fjord
                    player->CLOSE_GOSSIP_MENU();
                    creature->CastSpell(player, SPELL_HOWLING_FJORD, false);
                    break;
                case GOSSIP_ACTION_INFO_DEF + 3: // Sholazar Basin
                    player->CLOSE_GOSSIP_MENU();
                    creature->CastSpell(player, SPELL_SHOLAZAR_BASIN, false);
                    break;
                case GOSSIP_ACTION_INFO_DEF + 4: // Icecrown
                    player->CLOSE_GOSSIP_MENU();
                    creature->CastSpell(player, SPELL_ICECROWN, false);
                    break;
                case GOSSIP_ACTION_INFO_DEF + 5: // Storm peaks
                    player->CLOSE_GOSSIP_MENU();
                    creature->CastSpell(player, SPELL_STORM_PEAKS, false);
                    break;
                case GOSSIP_ACTION_INFO_DEF + 6: // Underground
                    player->CLOSE_GOSSIP_MENU();
                    creature->CastSpell(player, SPELL_UNDERGROUND, false);
                    break;
            }

            return true;
        }

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_wormholeAI(creature);
        }
};

/*######
## npc_pet_trainer
######*/

enum ePetTrainer
{
    TEXT_ISHUNTER               = 5838,
    TEXT_NOTHUNTER              = 5839,
    TEXT_PETINFO                = 13474,
    TEXT_CONFIRM                = 7722
};

#define GOSSIP_PET1             "How do I train my pet?"
#define GOSSIP_PET2             "I wish to untrain my pet."
#define GOSSIP_PET_CONFIRM      "Yes, please do."

class npc_pet_trainer : public CreatureScript
{
    public:
        npc_pet_trainer() : CreatureScript("npc_pet_trainer") { }

        bool OnGossipHello(Player* player, Creature* creature)
        {
            if (creature->isQuestGiver())
                player->PrepareQuestMenu(creature->GetGUID());

            if (player->getClass() == CLASS_HUNTER)
            {
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_PET1, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
                if (player->GetPet() && player->GetPet()->getPetType() == HUNTER_PET)
                    player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_PET2, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2);

                player->PlayerTalkClass->SendGossipMenu(TEXT_ISHUNTER, creature->GetGUID());
                return true;
            }
            player->PlayerTalkClass->SendGossipMenu(TEXT_NOTHUNTER, creature->GetGUID());
            return true;
        }

        bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action)
        {
            player->PlayerTalkClass->ClearMenus();
            switch (action)
            {
                case GOSSIP_ACTION_INFO_DEF + 1:
                    player->PlayerTalkClass->SendGossipMenu(TEXT_PETINFO, creature->GetGUID());
                    break;
                case GOSSIP_ACTION_INFO_DEF + 2:
                    {
                        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_PET_CONFIRM, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 3);
                        player->PlayerTalkClass->SendGossipMenu(TEXT_CONFIRM, creature->GetGUID());
                    }
                    break;
                case GOSSIP_ACTION_INFO_DEF + 3:
                    {
                        player->CLOSE_GOSSIP_MENU();
                    }
                    break;
            }
            return true;
        }
};

/*######
## npc_locksmith
######*/

enum eLockSmith
{
    QUEST_HOW_TO_BRAKE_IN_TO_THE_ARCATRAZ = 10704,
    QUEST_DARK_IRON_LEGACY                = 3802,
    QUEST_THE_KEY_TO_SCHOLOMANCE_A        = 5505,
    QUEST_THE_KEY_TO_SCHOLOMANCE_H        = 5511,
    QUEST_HOTTER_THAN_HELL_A              = 10758,
    QUEST_HOTTER_THAN_HELL_H              = 10764,
    QUEST_RETURN_TO_KHAGDAR               = 9837,
    QUEST_CONTAINMENT                     = 13159,
    QUEST_ETERNAL_VIGILANCE               = 11011,
    QUEST_KEY_TO_THE_FOCUSING_IRIS        = 13372,
    QUEST_HC_KEY_TO_THE_FOCUSING_IRIS     = 13375,

    ITEM_ARCATRAZ_KEY                     = 31084,
    ITEM_SHADOWFORGE_KEY                  = 11000,
    ITEM_SKELETON_KEY                     = 13704,
    ITEM_SHATTERED_HALLS_KEY              = 28395,
    ITEM_THE_MASTERS_KEY                  = 24490,
    ITEM_VIOLET_HOLD_KEY                  = 42482,
    ITEM_ESSENCE_INFUSED_MOONSTONE        = 32449,
    ITEM_KEY_TO_THE_FOCUSING_IRIS         = 44582,
    ITEM_HC_KEY_TO_THE_FOCUSING_IRIS      = 44581,

    SPELL_ARCATRAZ_KEY                    = 54881,
    SPELL_SHADOWFORGE_KEY                 = 54882,
    SPELL_SKELETON_KEY                    = 54883,
    SPELL_SHATTERED_HALLS_KEY             = 54884,
    SPELL_THE_MASTERS_KEY                 = 54885,
    SPELL_VIOLET_HOLD_KEY                 = 67253,
    SPELL_ESSENCE_INFUSED_MOONSTONE       = 40173,
};

#define GOSSIP_LOST_ARCATRAZ_KEY                "I've lost my key to the Arcatraz."
#define GOSSIP_LOST_SHADOWFORGE_KEY             "I've lost my key to the Blackrock Depths."
#define GOSSIP_LOST_SKELETON_KEY                "I've lost my key to the Scholomance."
#define GOSSIP_LOST_SHATTERED_HALLS_KEY         "I've lost my key to the Shattered Halls."
#define GOSSIP_LOST_THE_MASTERS_KEY             "I've lost my key to the Karazhan."
#define GOSSIP_LOST_VIOLET_HOLD_KEY             "I've lost my key to the Violet Hold."
#define GOSSIP_LOST_ESSENCE_INFUSED_MOONSTONE   "I've lost my Essence-Infused Moonstone."
#define GOSSIP_LOST_KEY_TO_THE_FOCUSING_IRIS    "I've lost my Key to the Focusing Iris."
#define GOSSIP_LOST_HC_KEY_TO_THE_FOCUSING_IRIS "I've lost my Heroic Key to the Focusing Iris."

class npc_locksmith : public CreatureScript
{
    public:
        npc_locksmith() : CreatureScript("npc_locksmith") { }

        bool OnGossipHello(Player* player, Creature* creature)
        {
            // Arcatraz Key
            if (player->GetQuestRewardStatus(QUEST_HOW_TO_BRAKE_IN_TO_THE_ARCATRAZ) && !player->HasItemCount(ITEM_ARCATRAZ_KEY, 1, true))
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_LOST_ARCATRAZ_KEY, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);

            // Shadowforge Key
            if (player->GetQuestRewardStatus(QUEST_DARK_IRON_LEGACY) && !player->HasItemCount(ITEM_SHADOWFORGE_KEY, 1, true))
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_LOST_SHADOWFORGE_KEY, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2);

            // Skeleton Key
            if ((player->GetQuestRewardStatus(QUEST_THE_KEY_TO_SCHOLOMANCE_A) || player->GetQuestRewardStatus(QUEST_THE_KEY_TO_SCHOLOMANCE_H)) &&
                !player->HasItemCount(ITEM_SKELETON_KEY, 1, true))
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_LOST_SKELETON_KEY, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 3);

            // Shatered Halls Key
            if ((player->GetQuestRewardStatus(QUEST_HOTTER_THAN_HELL_A) || player->GetQuestRewardStatus(QUEST_HOTTER_THAN_HELL_H)) &&
                !player->HasItemCount(ITEM_SHATTERED_HALLS_KEY, 1, true))
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_LOST_SHATTERED_HALLS_KEY, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 4);

            // Master's Key
            if (player->GetQuestRewardStatus(QUEST_RETURN_TO_KHAGDAR) && !player->HasItemCount(ITEM_THE_MASTERS_KEY, 1, true))
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_LOST_THE_MASTERS_KEY, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 5);

            // Violet Hold Key
            if (player->GetQuestRewardStatus(QUEST_CONTAINMENT) && !player->HasItemCount(ITEM_VIOLET_HOLD_KEY, 1, true))
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_LOST_VIOLET_HOLD_KEY, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 6);

            // Essence-Infused Moonstone
            if (player->GetQuestRewardStatus(QUEST_ETERNAL_VIGILANCE) && !player->HasItemCount(ITEM_ESSENCE_INFUSED_MOONSTONE, 1, true))
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_LOST_ESSENCE_INFUSED_MOONSTONE, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 7);

            // Key to the Focusing Iris
            if (player->GetQuestRewardStatus(QUEST_KEY_TO_THE_FOCUSING_IRIS) && !player->HasItemCount(ITEM_KEY_TO_THE_FOCUSING_IRIS, 1, true))
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_LOST_KEY_TO_THE_FOCUSING_IRIS, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 8);

            // Heroic Key to the Focusing Iris
            if (player->GetQuestRewardStatus(QUEST_HC_KEY_TO_THE_FOCUSING_IRIS) && !player->HasItemCount(ITEM_HC_KEY_TO_THE_FOCUSING_IRIS, 1, true))
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_LOST_HC_KEY_TO_THE_FOCUSING_IRIS, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 9);

            player->SEND_GOSSIP_MENU(player->GetGossipTextId(creature), creature->GetGUID());

            return true;
        }

        bool OnGossipSelect(Player* player, Creature* /*creature*/, uint32 /*sender*/, uint32 action)
        {
            player->PlayerTalkClass->ClearMenus();
            switch (action)
            {
                case GOSSIP_ACTION_INFO_DEF + 1:
                    player->CLOSE_GOSSIP_MENU();
                    player->CastSpell(player, SPELL_ARCATRAZ_KEY, false);
                    break;
                case GOSSIP_ACTION_INFO_DEF + 2:
                    player->CLOSE_GOSSIP_MENU();
                    player->CastSpell(player, SPELL_SHADOWFORGE_KEY, false);
                    break;
                case GOSSIP_ACTION_INFO_DEF + 3:
                    player->CLOSE_GOSSIP_MENU();
                    player->CastSpell(player, SPELL_SKELETON_KEY, false);
                    break;
                case GOSSIP_ACTION_INFO_DEF + 4:
                    player->CLOSE_GOSSIP_MENU();
                    player->CastSpell(player, SPELL_SHATTERED_HALLS_KEY, false);
                    break;
                case GOSSIP_ACTION_INFO_DEF + 5:
                    player->CLOSE_GOSSIP_MENU();
                    player->CastSpell(player, SPELL_THE_MASTERS_KEY, false);
                    break;
                case GOSSIP_ACTION_INFO_DEF + 6:
                    player->CLOSE_GOSSIP_MENU();
                    player->CastSpell(player, SPELL_VIOLET_HOLD_KEY, false);
                    break;
                case GOSSIP_ACTION_INFO_DEF + 7:
                    player->CLOSE_GOSSIP_MENU();
                    player->CastSpell(player, SPELL_ESSENCE_INFUSED_MOONSTONE, false);
                    break;
                case GOSSIP_ACTION_INFO_DEF + 8:
                    player->CLOSE_GOSSIP_MENU();
                    player->AddItem(ITEM_KEY_TO_THE_FOCUSING_IRIS,1);
                    break;
                case GOSSIP_ACTION_INFO_DEF + 9:
                    player->CLOSE_GOSSIP_MENU();
                    player->AddItem(ITEM_HC_KEY_TO_THE_FOCUSING_IRIS,1);
                    break;
            }
            return true;
        }
};

/*######
## npc_experience
######*/

#define EXP_COST                100000 //10 00 00 copper (10golds)
#define GOSSIP_TEXT_EXP         14736
#define GOSSIP_XP_OFF           "I no longer wish to gain experience."
#define GOSSIP_XP_ON            "I wish to start gaining experience again."

class npc_experience : public CreatureScript
{
    public:
        npc_experience() : CreatureScript("npc_experience") { }

        bool OnGossipHello(Player* player, Creature* creature)
        {
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_XP_OFF, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_XP_ON, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2);
            player->PlayerTalkClass->SendGossipMenu(GOSSIP_TEXT_EXP, creature->GetGUID());
            return true;
        }

        bool OnGossipSelect(Player* player, Creature* /*creature*/, uint32 /*sender*/, uint32 action)
        {
            player->PlayerTalkClass->ClearMenus();
            bool noXPGain = player->HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_NO_XP_GAIN);
            bool doSwitch = false;

            switch (action)
            {
                case GOSSIP_ACTION_INFO_DEF + 1://xp off
                    {
                        if (!noXPGain)//does gain xp
                            doSwitch = true;//switch to don't gain xp
                    }
                    break;
                case GOSSIP_ACTION_INFO_DEF + 2://xp on
                    {
                        if (noXPGain)//doesn't gain xp
                            doSwitch = true;//switch to gain xp
                    }
                    break;
            }
            if (doSwitch)
            {
                if (!player->HasEnoughMoney(uint64(EXP_COST)))
                    player->SendBuyError(BUY_ERR_NOT_ENOUGHT_MONEY, 0, 0, 0);
                else if (noXPGain)
                {
                    player->ModifyMoney(-int64(EXP_COST));
                    player->RemoveFlag(PLAYER_FLAGS, PLAYER_FLAGS_NO_XP_GAIN);
                }
                else if (!noXPGain)
                {
                    player->ModifyMoney(-EXP_COST);
                    player->SetFlag(PLAYER_FLAGS, PLAYER_FLAGS_NO_XP_GAIN);
                }
            }
            player->PlayerTalkClass->SendCloseGossip();
            return true;
        }
};

enum Fireworks
{
    NPC_OMEN                = 15467,
    NPC_MINION_OF_OMEN      = 15466,
    NPC_FIREWORK_BLUE       = 15879,
    NPC_FIREWORK_GREEN      = 15880,
    NPC_FIREWORK_PURPLE     = 15881,
    NPC_FIREWORK_RED        = 15882,
    NPC_FIREWORK_YELLOW     = 15883,
    NPC_FIREWORK_WHITE      = 15884,
    NPC_FIREWORK_BIG_BLUE   = 15885,
    NPC_FIREWORK_BIG_GREEN  = 15886,
    NPC_FIREWORK_BIG_PURPLE = 15887,
    NPC_FIREWORK_BIG_RED    = 15888,
    NPC_FIREWORK_BIG_YELLOW = 15889,
    NPC_FIREWORK_BIG_WHITE  = 15890,

    NPC_CLUSTER_BLUE        = 15872,
    NPC_CLUSTER_RED         = 15873,
    NPC_CLUSTER_GREEN       = 15874,
    NPC_CLUSTER_PURPLE      = 15875,
    NPC_CLUSTER_WHITE       = 15876,
    NPC_CLUSTER_YELLOW      = 15877,
    NPC_CLUSTER_BIG_BLUE    = 15911,
    NPC_CLUSTER_BIG_GREEN   = 15912,
    NPC_CLUSTER_BIG_PURPLE  = 15913,
    NPC_CLUSTER_BIG_RED     = 15914,
    NPC_CLUSTER_BIG_WHITE   = 15915,
    NPC_CLUSTER_BIG_YELLOW  = 15916,
    NPC_CLUSTER_ELUNE       = 15918,

    GO_FIREWORK_LAUNCHER_1  = 180771,
    GO_FIREWORK_LAUNCHER_2  = 180868,
    GO_FIREWORK_LAUNCHER_3  = 180850,
    GO_CLUSTER_LAUNCHER_1   = 180772,
    GO_CLUSTER_LAUNCHER_2   = 180859,
    GO_CLUSTER_LAUNCHER_3   = 180869,
    GO_CLUSTER_LAUNCHER_4   = 180874,

    SPELL_ROCKET_BLUE       = 26344,
    SPELL_ROCKET_GREEN      = 26345,
    SPELL_ROCKET_PURPLE     = 26346,
    SPELL_ROCKET_RED        = 26347,
    SPELL_ROCKET_WHITE      = 26348,
    SPELL_ROCKET_YELLOW     = 26349,
    SPELL_ROCKET_BIG_BLUE   = 26351,
    SPELL_ROCKET_BIG_GREEN  = 26352,
    SPELL_ROCKET_BIG_PURPLE = 26353,
    SPELL_ROCKET_BIG_RED    = 26354,
    SPELL_ROCKET_BIG_WHITE  = 26355,
    SPELL_ROCKET_BIG_YELLOW = 26356,
    SPELL_LUNAR_FORTUNE     = 26522,

    ANIM_GO_LAUNCH_FIREWORK = 3,
    ZONE_MOONGLADE          = 493,
};

Position omenSummonPos = {7558.993f, -2839.999f, 450.0214f, 4.46f};

class npc_firework : public CreatureScript
{
    public:
        npc_firework() : CreatureScript("npc_firework") { }

        struct npc_fireworkAI : public ScriptedAI
        {
            npc_fireworkAI(Creature* creature) : ScriptedAI(creature) {}

            bool isCluster()
            {
                switch (me->GetEntry())
                {
                    case NPC_FIREWORK_BLUE:
                    case NPC_FIREWORK_GREEN:
                    case NPC_FIREWORK_PURPLE:
                    case NPC_FIREWORK_RED:
                    case NPC_FIREWORK_YELLOW:
                    case NPC_FIREWORK_WHITE:
                    case NPC_FIREWORK_BIG_BLUE:
                    case NPC_FIREWORK_BIG_GREEN:
                    case NPC_FIREWORK_BIG_PURPLE:
                    case NPC_FIREWORK_BIG_RED:
                    case NPC_FIREWORK_BIG_YELLOW:
                    case NPC_FIREWORK_BIG_WHITE:
                        return false;
                    case NPC_CLUSTER_BLUE:
                    case NPC_CLUSTER_GREEN:
                    case NPC_CLUSTER_PURPLE:
                    case NPC_CLUSTER_RED:
                    case NPC_CLUSTER_YELLOW:
                    case NPC_CLUSTER_WHITE:
                    case NPC_CLUSTER_BIG_BLUE:
                    case NPC_CLUSTER_BIG_GREEN:
                    case NPC_CLUSTER_BIG_PURPLE:
                    case NPC_CLUSTER_BIG_RED:
                    case NPC_CLUSTER_BIG_YELLOW:
                    case NPC_CLUSTER_BIG_WHITE:
                    case NPC_CLUSTER_ELUNE:
                    default:
                        return true;
                }
            }

            GameObject* FindNearestLauncher()
            {
                GameObject* launcher = NULL;

                if (isCluster())
                {
                    GameObject* launcher1 = GetClosestGameObjectWithEntry(me, GO_CLUSTER_LAUNCHER_1, 0.5f);
                    GameObject* launcher2 = GetClosestGameObjectWithEntry(me, GO_CLUSTER_LAUNCHER_2, 0.5f);
                    GameObject* launcher3 = GetClosestGameObjectWithEntry(me, GO_CLUSTER_LAUNCHER_3, 0.5f);
                    GameObject* launcher4 = GetClosestGameObjectWithEntry(me, GO_CLUSTER_LAUNCHER_4, 0.5f);

                    if (launcher1)
                        launcher = launcher1;
                    else if (launcher2)
                        launcher = launcher2;
                    else if (launcher3)
                        launcher = launcher3;
                    else if (launcher4)
                        launcher = launcher4;
                }
                else
                {
                    GameObject* launcher1 = GetClosestGameObjectWithEntry(me, GO_FIREWORK_LAUNCHER_1, 0.5f);
                    GameObject* launcher2 = GetClosestGameObjectWithEntry(me, GO_FIREWORK_LAUNCHER_2, 0.5f);
                    GameObject* launcher3 = GetClosestGameObjectWithEntry(me, GO_FIREWORK_LAUNCHER_3, 0.5f);

                    if (launcher1)
                        launcher = launcher1;
                    else if (launcher2)
                        launcher = launcher2;
                    else if (launcher3)
                        launcher = launcher3;
                }

                return launcher;
            }

            uint32 GetFireworkSpell(uint32 entry)
            {
                switch (entry)
                {
                    case NPC_FIREWORK_BLUE:
                        return SPELL_ROCKET_BLUE;
                    case NPC_FIREWORK_GREEN:
                        return SPELL_ROCKET_GREEN;
                    case NPC_FIREWORK_PURPLE:
                        return SPELL_ROCKET_PURPLE;
                    case NPC_FIREWORK_RED:
                        return SPELL_ROCKET_RED;
                    case NPC_FIREWORK_YELLOW:
                        return SPELL_ROCKET_YELLOW;
                    case NPC_FIREWORK_WHITE:
                        return SPELL_ROCKET_WHITE;
                    case NPC_FIREWORK_BIG_BLUE:
                        return SPELL_ROCKET_BIG_BLUE;
                    case NPC_FIREWORK_BIG_GREEN:
                        return SPELL_ROCKET_BIG_GREEN;
                    case NPC_FIREWORK_BIG_PURPLE:
                        return SPELL_ROCKET_BIG_PURPLE;
                    case NPC_FIREWORK_BIG_RED:
                        return SPELL_ROCKET_BIG_RED;
                    case NPC_FIREWORK_BIG_YELLOW:
                        return SPELL_ROCKET_BIG_YELLOW;
                    case NPC_FIREWORK_BIG_WHITE:
                        return SPELL_ROCKET_BIG_WHITE;
                    default:
                        return 0;
                }
            }

            uint32 GetFireworkGameObjectId()
            {
                uint32 spellId = 0;

                switch (me->GetEntry())
                {
                    case NPC_CLUSTER_BLUE:
                        spellId = GetFireworkSpell(NPC_FIREWORK_BLUE);
                        break;
                    case NPC_CLUSTER_GREEN:
                        spellId = GetFireworkSpell(NPC_FIREWORK_GREEN);
                        break;
                    case NPC_CLUSTER_PURPLE:
                        spellId = GetFireworkSpell(NPC_FIREWORK_PURPLE);
                        break;
                    case NPC_CLUSTER_RED:
                        spellId = GetFireworkSpell(NPC_FIREWORK_RED);
                        break;
                    case NPC_CLUSTER_YELLOW:
                        spellId = GetFireworkSpell(NPC_FIREWORK_YELLOW);
                        break;
                    case NPC_CLUSTER_WHITE:
                        spellId = GetFireworkSpell(NPC_FIREWORK_WHITE);
                        break;
                    case NPC_CLUSTER_BIG_BLUE:
                        spellId = GetFireworkSpell(NPC_FIREWORK_BIG_BLUE);
                        break;
                    case NPC_CLUSTER_BIG_GREEN:
                        spellId = GetFireworkSpell(NPC_FIREWORK_BIG_GREEN);
                        break;
                    case NPC_CLUSTER_BIG_PURPLE:
                        spellId = GetFireworkSpell(NPC_FIREWORK_BIG_PURPLE);
                        break;
                    case NPC_CLUSTER_BIG_RED:
                        spellId = GetFireworkSpell(NPC_FIREWORK_BIG_RED);
                        break;
                    case NPC_CLUSTER_BIG_YELLOW:
                        spellId = GetFireworkSpell(NPC_FIREWORK_BIG_YELLOW);
                        break;
                    case NPC_CLUSTER_BIG_WHITE:
                        spellId = GetFireworkSpell(NPC_FIREWORK_BIG_WHITE);
                        break;
                    case NPC_CLUSTER_ELUNE:
                        spellId = GetFireworkSpell(urand(NPC_FIREWORK_BLUE, NPC_FIREWORK_WHITE));
                        break;
                }

                const SpellInfo* spellInfo = sSpellMgr->GetSpellInfo(spellId);

                if (spellInfo && spellInfo->Effects[0].Effect == SPELL_EFFECT_SUMMON_OBJECT_WILD)
                    return spellInfo->Effects[0].MiscValue;

                return 0;
            }

            void Reset()
            {
                if (GameObject* launcher = FindNearestLauncher())
                {
                    launcher->SendCustomAnim(ANIM_GO_LAUNCH_FIREWORK);
                    me->SetOrientation(launcher->GetOrientation() + M_PI/2);
                }
                else
                    return;

                if (isCluster())
                {
                    // Check if we are near Elune'ara lake south, if so try to summon Omen or a minion
                    if (me->GetZoneId() == ZONE_MOONGLADE)
                    {
                        if (!me->FindNearestCreature(NPC_OMEN, 100.0f, false) && me->GetDistance2d(omenSummonPos.GetPositionX(), omenSummonPos.GetPositionY()) <= 100.0f)
                        {
                            switch (urand(0,9))
                            {
                                case 0:
                                case 1:
                                case 2:
                                case 3:
                                    if (Creature* minion = me->SummonCreature(NPC_MINION_OF_OMEN, me->GetPositionX()+frand(-5.0f, 5.0f), me->GetPositionY()+frand(-5.0f, 5.0f), me->GetPositionZ(), 0.0f, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 20000))
                                        minion->AI()->AttackStart(me->SelectNearestPlayer(20.0f));
                                    break;
                                case 9:
                                    me->SummonCreature(NPC_OMEN, omenSummonPos);
                                    break;
                            }
                        }
                    }
                    if (me->GetEntry() == NPC_CLUSTER_ELUNE)
                        DoCast(SPELL_LUNAR_FORTUNE);

                    float displacement = 0.7f;
                    for (uint8 i = 0; i < 4; i++)
                        me->SummonGameObject(GetFireworkGameObjectId(), me->GetPositionX() + (i%2 == 0 ? displacement : -displacement), me->GetPositionY() + (i > 1 ? displacement : -displacement), me->GetPositionZ() + 4.0f, me->GetOrientation(), 0.0f, 0.0f, 0.0f, 0.0f, 1);
                }
                else
                    //me->CastSpell(me, GetFireworkSpell(me->GetEntry()), true);
                    me->CastSpell(me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), GetFireworkSpell(me->GetEntry()), true);
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_fireworkAI(creature);
        }
};

/*#####
# npc_spring_rabbit
#####*/

enum rabbitSpells
{
    SPELL_SPRING_FLING          = 61875,
    SPELL_SPRING_RABBIT_JUMP    = 61724,
    SPELL_SPRING_RABBIT_WANDER  = 61726,
    SPELL_SUMMON_BABY_BUNNY     = 61727,
    SPELL_SPRING_RABBIT_IN_LOVE = 61728,
    NPC_SPRING_RABBIT           = 32791
};

class npc_spring_rabbit : public CreatureScript
{
    public:
        npc_spring_rabbit() : CreatureScript("npc_spring_rabbit") { }

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_spring_rabbitAI(creature);
        }

        struct npc_spring_rabbitAI : public ScriptedAI
        {
            npc_spring_rabbitAI(Creature* creature) : ScriptedAI(creature) { }

            bool inLove;
            uint32 jumpTimer;
            uint32 bunnyTimer;
            uint32 searchTimer;
            uint64 rabbitGUID;

            void Reset()
            {
                inLove = false;
                rabbitGUID = 0;
                jumpTimer = urand(5000, 10000);
                bunnyTimer = urand(10000, 20000);
                searchTimer = urand(5000, 10000);
                if (Unit* owner = me->GetOwner())
                    me->GetMotionMaster()->MoveFollow(owner, PET_FOLLOW_DIST, PET_FOLLOW_ANGLE);
            }

            void EnterCombat(Unit * /*who*/) { }

            void DoAction(const int32 /*param*/)
            {
                inLove = true;
                if (Unit* owner = me->GetOwner())
                    owner->CastSpell(owner, SPELL_SPRING_FLING, true);
            }

            void UpdateAI(uint32 diff)
            {
                if (inLove)
                {
                    if (jumpTimer <= diff)
                    {
                        if (Unit* rabbit = Unit::GetUnit(*me, rabbitGUID))
                            DoCast(rabbit, SPELL_SPRING_RABBIT_JUMP);
                        jumpTimer = urand(5000, 10000);
                    } else jumpTimer -= diff;

                    if (bunnyTimer <= diff)
                    {
                        DoCast(SPELL_SUMMON_BABY_BUNNY);
                        bunnyTimer = urand(20000, 40000);
                    } else bunnyTimer -= diff;
                }
                else
                {
                    if (searchTimer <= diff)
                    {
                        if (Creature* rabbit = me->FindNearestCreature(NPC_SPRING_RABBIT, 10.0f))
                        {
                            if (rabbit == me || rabbit->HasAura(SPELL_SPRING_RABBIT_IN_LOVE))
                                return;

                            me->AddAura(SPELL_SPRING_RABBIT_IN_LOVE, me);
                            DoAction(1);
                            rabbit->AddAura(SPELL_SPRING_RABBIT_IN_LOVE, rabbit);
                            rabbit->AI()->DoAction(1);
                            rabbit->CastSpell(rabbit, SPELL_SPRING_RABBIT_JUMP, true);
                            rabbitGUID = rabbit->GetGUID();
                        }
                        searchTimer = urand(5000, 10000);
                    } else searchTimer -= diff;
                }
            }
        };
};

/*######
## npc_generic_harpoon_cannon
######*/

class npc_generic_harpoon_cannon : public CreatureScript
{
    public:
        npc_generic_harpoon_cannon() : CreatureScript("npc_generic_harpoon_cannon") { }

        struct npc_generic_harpoon_cannonAI : public ScriptedAI
        {
            npc_generic_harpoon_cannonAI(Creature* creature) : ScriptedAI(creature) {}

            void Reset()
            {
                me->SetUnitMovementFlags(MOVEMENTFLAG_ROOT);
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_generic_harpoon_cannonAI(creature);
        }
};

/*######
## npc_experience
######*/

#define GOSSIP_CHOOSE_FACTION     "I would like to choose my faction"
#define GOSSIP_TP_STORMIND        "I would like to go to Stormwind"
#define GOSSIP_TP_ORGRI           "I would like to go to Orgrimmar"

class npc_choose_faction : public CreatureScript
{
    public:
        npc_choose_faction() : CreatureScript("npc_choose_faction") { }

        bool OnGossipHello(Player* player, Creature* creature)
        {
            if (player->getRace() == RACE_PANDAREN_NEUTRAL)
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_CHOOSE_FACTION, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
            else if (player->getRace() == RACE_PANDAREN_ALLI)
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_TP_STORMIND, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2);
            else if (player->getRace() == RACE_PANDAREN_HORDE)
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_TP_ORGRI, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 3);

            player->PlayerTalkClass->SendGossipMenu(GOSSIP_TEXT_EXP, creature->GetGUID());
            return true;
        }

        bool OnGossipSelect(Player* player, Creature* /*creature*/, uint32 /*sender*/, uint32 action)
        {
            if (action == GOSSIP_ACTION_INFO_DEF + 1)
                player->ShowNeutralPlayerFactionSelectUI();
            else if (action == GOSSIP_ACTION_INFO_DEF + 2)
                player->TeleportTo(0, -8866.55f, 671.93f, 97.90f, 5.31f);
            else if (action == GOSSIP_ACTION_INFO_DEF + 3)
                player->TeleportTo(1, 1577.30f, -4453.64f, 15.68f, 1.84f);

            player->PlayerTalkClass->SendCloseGossip();
            return true;
        }
};

/*######
## npc_rate_xp_modifier
######*/

#define GOSSIP_TEXT_EXP_MODIF    1587
#define GOSSIP_TEXT_EXP_MODIF_OK 1588
#define GOSSIP_TEXT_EXP_NORMAL   1589
#define GOSSIP_ITEM_XP_CLOSE     "Good bye."

class npc_rate_xp_modifier : public CreatureScript
{
    public:
        npc_rate_xp_modifier() : CreatureScript("npc_rate_xp_modifier") { }

        bool OnGossipHello(Player *pPlayer, Creature *pCreature)
        {
            uint32 maxRates = sWorld->getRate(RATE_XP_KILL);

            for (uint32 i = 1; i < sWorld->getRate(RATE_XP_KILL); ++i)
            {
                std::ostringstream gossipText;
                gossipText << "I would like to change my rates" << i;
                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, gossipText.str(), GOSSIP_SENDER_MAIN, i);
            }

            if (pPlayer->GetPersonnalXpRate())
            {
                std::ostringstream gossipText;
                gossipText << "I would like to restore my rates (" << sWorld->getRate(RATE_XP_KILL) << ")";
                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, gossipText.str(), GOSSIP_SENDER_MAIN, 0);
            }

            pPlayer->PlayerTalkClass->SendGossipMenu(GOSSIP_TEXT_EXP_MODIF, pCreature->GetGUID());
            return true;
        }

        bool OnGossipSelect(Player* pPlayer, Creature* pCreature, uint32 /*uiSender*/, uint32 uiAction)
        {
            if (uiAction < 0 || uiAction >= sWorld->getRate(RATE_XP_KILL))
            {
                pPlayer->PlayerTalkClass->SendCloseGossip();
                return true;
            }
        
            pPlayer->SetPersonnalXpRate(float(uiAction));

            pPlayer->PlayerTalkClass->ClearMenus();
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ITEM_XP_CLOSE, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF);
            pPlayer->PlayerTalkClass->SendGossipMenu(GOSSIP_TEXT_EXP_MODIF_OK, pCreature->GetGUID());

            return true;
        }
};

/*######
## npc_capacitor_totem
######*/

class npc_capacitor_totem : public CreatureScript
{
    public:
        npc_capacitor_totem() : CreatureScript("npc_capacitor_totem") { }

    struct npc_capacitor_totemAI : public ScriptedAI
    {
        uint32 CastTimer;

        npc_capacitor_totemAI(Creature* creature) : ScriptedAI(creature)
        {
            CastTimer = 5000;

            if(Unit* owner = creature->GetOwner())
                if (owner->HasAura(55442))
                    CastTimer = 3000;
        }

        void UpdateAI(uint32 diff)
        {
            if (CastTimer <= diff)
            {
                if (me->GetEntry() == 61245)
                    me->CastSpell(me, 118905, true);
                CastTimer = 99999;
            }
            else
                CastTimer -= diff;

        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_capacitor_totemAI(creature);
    }
};

/*######
## npc_feral_spirit
######*/

class npc_feral_spirit : public CreatureScript
{
    public:
        npc_feral_spirit() : CreatureScript("npc_feral_spirit") { }

        struct npc_feral_spiritAI : public ScriptedAI
        {
            npc_feral_spiritAI(Creature* creature) : ScriptedAI(creature) {}

            uint32 SpiritBiteTimer;

            void Reset()
            {
                SpiritBiteTimer = 6000;
            }

            void UpdateAI(uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                if (SpiritBiteTimer <= diff)
                {
                    me->CastSpell(me->getVictim(), 58859, true);
                    SpiritBiteTimer = 6000;
                }
                else
                    SpiritBiteTimer -= diff;

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_feral_spiritAI(creature);
        }
};

/*######
## npc_spirit_link_totem
######*/

class npc_spirit_link_totem : public CreatureScript
{
    public:
        npc_spirit_link_totem() : CreatureScript("npc_spirit_link_totem") { }

    struct npc_spirit_link_totemAI : public ScriptedAI
    {
        uint32 CastTimer;

        npc_spirit_link_totemAI(Creature* creature) : ScriptedAI(creature)
        {
            CastTimer = 1000;

            if (creature->GetOwner() && creature->GetOwner()->GetTypeId() == TYPEID_PLAYER)
            {
                if (creature->GetEntry() == 53006)
                {
                    creature->CastSpell(creature, 98007, false);
                    creature->CastSpell(creature, 98017, true);
                }
            }
        }

        void UpdateAI(uint32 diff)
        {
            if (CastTimer >= diff)
            {
                if (me->GetOwner() && me->GetOwner()->GetTypeId() == TYPEID_PLAYER)
                {
                    if (me->GetEntry() == 53006)
                    {
                        me->CastSpell(me, 98007, false);
                        me->CastSpell(me, 98017, true);
                    }
                }
            }

            CastTimer = 0;
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_spirit_link_totemAI(creature);
    }
};

/*######
# npc_demoralizing_banner
######*/

class npc_demoralizing_banner : public CreatureScript
{
    public:
        npc_demoralizing_banner() : CreatureScript("npc_demoralizing_banner") { }

        struct npc_demoralizing_bannerAI : public ScriptedAI
        {
            uint32 demoralizingTimer;

            npc_demoralizing_bannerAI(Creature* creature) : ScriptedAI(creature)
            {
                demoralizingTimer = 1000;

                Unit* owner = creature->GetOwner();

                if (owner)
                    owner->CastSpell(creature, 114205, true);
            }

            void UpdateAI(uint32 diff)
            {
                Unit* owner = me->GetOwner();

                if (!owner)
                    return;

                if (demoralizingTimer <= diff)
                {
                    owner->CastSpell(me, 114205, true);
                    demoralizingTimer = 0;
                }
                else
                    demoralizingTimer -= diff;
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_demoralizing_bannerAI(creature);
        }
};

/*######
# npc_frozen_orb
######*/

class npc_frozen_orb : public CreatureScript
{
    public:
        npc_frozen_orb() : CreatureScript("npc_frozen_orb") { }

        struct npc_frozen_orbAI : public ScriptedAI
        {
            uint32 frozenOrbTimer;
            float x,y,z;

            npc_frozen_orbAI(Creature* creature) : ScriptedAI(creature)
            {
                Unit* owner = creature->GetOwner();

                if (owner)
                {
                    owner->CastSpell(creature, 84721, true);
                    if (owner->HasAura(44544))
                        owner->CastSpell(owner, 126084, true);
                    owner->CastSpell(owner, 44544, true);

                    float distance = 100.0f;
                    owner->GetNearPoint2D(x, y, distance, owner->GetOrientation());
                    z = me->GetMap()->GetHeight(x,y, me->GetPositionZ(), true, MAX_FALL_DISTANCE);
                    if(!me->IsWithinLOS(x, y, z))
                    {
                        for (uint8 j = 0; distance > 5.0f; ++j)
                        {
                            distance -= 5.0f;
                            owner->GetNearPoint2D(x, y, distance, owner->GetOrientation());
                            z = me->GetMap()->GetHeight(x,y, me->GetPositionZ(), true, MAX_FALL_DISTANCE);
                            if(me->IsWithinLOS(x, y, z))
                                break;
                        }
                    }
                    me->GetMotionMaster()->MovePoint(0, x, y, z);
                }

                me->SetReactState(REACT_PASSIVE);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                frozenOrbTimer = 1000;
            }

            void Reset()
            {
                me->GetMotionMaster()->MovePoint(0, x, y, z);
            }

            void UpdateAI(uint32 diff)
            {
                Unit* owner = me->GetOwner();

                if (!owner)
                    return;

                if (frozenOrbTimer <= diff)
                {
                    if (owner && owner->ToPlayer())
                        if (owner->ToPlayer()->HasSpellCooldown(84721))
                            owner->ToPlayer()->RemoveSpellCooldown(84721);

                    owner->CastSpell(me, 84721, true);

                    UnitList targets;
                    Trinity::AnyUnfriendlyUnitInObjectRangeCheck u_check(me, me, 10.0f);
                    Trinity::UnitListSearcher<Trinity::AnyUnfriendlyUnitInObjectRangeCheck> searcher(me, targets, u_check);
                    me->VisitNearbyObject(10.0f, searcher);
                    for (UnitList::iterator iter = targets.begin(); iter != targets.end(); ++iter)
                    {
                        me->SetSpeed(MOVE_WALK, 0.2f);
                        me->SetSpeed(MOVE_RUN, 0.2f);
                        break;
                    }
                    frozenOrbTimer = 1000;
                }
                else
                    frozenOrbTimer -= diff;
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_frozen_orbAI(creature);
        }
};

/*######
# npc_guardian_of_ancient_kings
######*/

enum GuardianSpellsAndEntries
{
    NPC_PROTECTION_GUARDIAN         = 46490,
    NPC_HOLY_GUARDIAN               = 46499,
    NPC_RETRI_GUARDIAN              = 46506,
    SPELL_ANCIENT_GUARDIAN_VISUAL   = 86657,
    SPELL_ANCIENT_HEALER            = 86674,
};

class npc_guardian_of_ancient_kings : public CreatureScript
{
    public:
        npc_guardian_of_ancient_kings() : CreatureScript("npc_guardian_of_ancient_kings") { }

        struct npc_guardian_of_ancient_kingsAI : public ScriptedAI
        {
            npc_guardian_of_ancient_kingsAI(Creature *creature) : ScriptedAI(creature) {}

            void Reset()
            {
                if (me->GetEntry() == NPC_RETRI_GUARDIAN || me->GetEntry() == NPC_HOLY_GUARDIAN)
                    me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_NON_ATTACKABLE);
                else if (me->GetEntry() == NPC_PROTECTION_GUARDIAN)
                    me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_DISABLE_MOVE);

                if (me->GetEntry() == NPC_RETRI_GUARDIAN)
                    me->SetReactState(REACT_DEFENSIVE);
                else
                    me->SetReactState(REACT_PASSIVE);

                if (me->GetEntry() == NPC_PROTECTION_GUARDIAN)
                {
                    if (me->GetOwner())
                        DoCast(me->GetOwner(), SPELL_ANCIENT_GUARDIAN_VISUAL);
                }
                else if (me->GetEntry() == NPC_RETRI_GUARDIAN)
                {
                    if (me->GetOwner())
                    {
                        float mindmg = me->GetOwner()->GetFloatValue(UNIT_FIELD_MINDAMAGE);
                        float maxdmg = me->GetOwner()->GetFloatValue(UNIT_FIELD_MAXDAMAGE);
                        me->SetBaseWeaponDamage(BASE_ATTACK, MINDAMAGE, mindmg);
                        me->SetBaseWeaponDamage(BASE_ATTACK, MAXDAMAGE, maxdmg);

                        if (me->GetOwner()->getVictim())
                            AttackStart(me->GetOwner()->getVictim());

                        DoCast(me, 86703, true);
                    }
                }
                else if (me->GetEntry() == NPC_HOLY_GUARDIAN)
                    if (me->GetOwner())
                        me->GetOwner()->CastSpell(me->GetOwner(), SPELL_ANCIENT_HEALER, true);
            }

            void UpdateAI(uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                if (me->GetOwner())
                    if (Unit* newVictim = me->GetOwner()->getVictim())
                        if (me->getVictim() != newVictim)
                            AttackStart(newVictim);

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_guardian_of_ancient_kingsAI(creature);
        }
};

/*######
# new npc_demonic_gateway_green
######*/

enum gateway_data
{
    SPELL_DEMONIC_GATEWAY_CHARGES           = 113901,
    //Add animation to AreaTrigger
    SPELL_DEMONIC_PORTAL_BIRTH_ANIM_DUMMY   = 143251,
    //Summon arrea trigger.
    SPELL_DEMONIC_GATEWAY                   = 113904,

    NPC_PURGE_GATE                          = 59271,
    NPC_GREEN_GATE                          = 59262,
};

uint32 DestEntry[2]= { NPC_PURGE_GATE, NPC_GREEN_GATE };

uint32 gwauras[2][5]=
{
    { 113903, 113911, 113912, 113913, 113914 },
    { 113915, 113916, 113917, 113918, 113919}
};

class npc_demonic_gateway : public CreatureScript
{
    public:
        npc_demonic_gateway() : CreatureScript("npc_demonic_gateway") { }

        struct npc_demonic_gatewayAI : public Scripted_NoMovementAI
        {
            bool gate;
            npc_demonic_gatewayAI(Creature* creature) : Scripted_NoMovementAI(creature)
            {
                me->SetReactState(REACT_PASSIVE);
            }

            void IsSummonedBy(Unit* summoner)
            {
                gate = me->GetEntry() == NPC_PURGE_GATE;
                me->CastSpell(me, SPELL_DEMONIC_GATEWAY, true);
                me->CastSpell(me, SPELL_DEMONIC_PORTAL_BIRTH_ANIM_DUMMY, false);
                if (gate)   //Only Purge gate do check for caster aura.
                {
                    if (summoner->HasAura(SPELL_DEMONIC_GATEWAY_CHARGES))
                        summoner->RemoveAurasDueToSpell(SPELL_DEMONIC_GATEWAY_CHARGES);
                    summoner->CastSpell(summoner, SPELL_DEMONIC_GATEWAY_CHARGES, false);
                }
            }

            void OnSpellClick(Unit* who)
            {
                if (!who || !me->ToTempSummon() || who->HasAura(113942))
                    return;
                if (Unit* owner = me->ToTempSummon()->GetSummoner())
                {
                    Player* ownerPlayer = owner->ToPlayer();
                    Player* whoPlayer = who->ToPlayer();
                    if(!ownerPlayer || !whoPlayer || !ownerPlayer->IsGroupVisibleFor(whoPlayer))
                        return;
                    if (Aura* aurastack = owner->GetAura(SPELL_DEMONIC_GATEWAY_CHARGES))
                    {
                        uint32 stacks = aurastack->GetEffect(EFFECT_0)->GetAmount();
                        if (!stacks)
                            return;

                        for (int32 i = 0; i < MAX_SUMMON_SLOT; ++i)
                        {
                            if (GUID_ENPART(owner->m_SummonSlot[i]) != DestEntry[gate])
                                continue;
                            if(Unit* uGate = ObjectAccessor::GetUnit(*me, owner->m_SummonSlot[i]))
                            {
                                aurastack->GetEffect(EFFECT_0)->ChangeAmount(stacks - 1);
                                aurastack->SetNeedClientUpdateForTargets();
                                who->CastSpell(uGate, gate ? 120729 : 113896, true);
                                for(int32 j = 5; j >= 0; --j)
                                {
                                    if (uGate->HasAura(gwauras[!gate][j]))
                                    {
                                        uGate->RemoveAurasDueToSpell(gwauras[!gate][j]);
                                        break;
                                    }
                                }
                                for(int32 j = 5; j >= 0; --j)
                                {
                                    if (me->HasAura(gwauras[gate][j]))
                                    {
                                        me->RemoveAurasDueToSpell(gwauras[gate][j]);
                                        break;
                                    }
                                }
                                break;
                            }
                        }
                    }
                }
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_demonic_gatewayAI(creature);
        }
};

/*######
# npc_xuen_the_white_tiger
######*/

#define CRACKLING_TIGER_LIGHTNING   123996

class npc_xuen_the_white_tiger : public CreatureScript
{
    public:
        npc_xuen_the_white_tiger() : CreatureScript("npc_xuen_the_white_tiger") { }

        struct npc_xuen_the_white_tigerAI : public ScriptedAI
        {
            uint32 CastTimer;

            npc_xuen_the_white_tigerAI(Creature *creature) : ScriptedAI(creature)
            {
                CastTimer = 6000;
                me->SetReactState(REACT_DEFENSIVE);
            }

            void UpdateAI(uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                if (me->getVictim())
                {
                    if (CastTimer <= diff)
                    {
                        DoCast(me->getVictim(), CRACKLING_TIGER_LIGHTNING, false);
                        CastTimer = 6000;
                    }
                    else
                        CastTimer -= diff;
                }

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_xuen_the_white_tigerAI(creature);
        }
};

/*######
# npc_murder_of_crows
######*/

class npc_murder_of_crows : public CreatureScript
{
    public:
        npc_murder_of_crows() : CreatureScript("npc_murder_of_crows") { }

        struct npc_murder_of_crowsAI : public ScriptedAI
        {
            npc_murder_of_crowsAI(Creature *creature) : ScriptedAI(creature)
            {
                me->SetReactState(REACT_DEFENSIVE);
            }

            void UpdateAI(uint32 diff)
            {
                if (me->GetReactState() != REACT_DEFENSIVE)
                    me->SetReactState(REACT_DEFENSIVE);

                if (!UpdateVictim())
                    return;

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_murder_of_crowsAI(creature);
        }
};

/*######
# npc_dire_beast
######*/

class npc_dire_beast : public CreatureScript
{
    public:
        npc_dire_beast() : CreatureScript("npc_dire_beast") { }

        struct npc_dire_beastAI : public ScriptedAI
        {
            npc_dire_beastAI(Creature *creature) : ScriptedAI(creature) {}

            void DamageDealt(Unit* target, uint32& damage, DamageEffectType damageType)
            {
                if (Unit* owner = me->GetOwner())
                    owner->ModifyPower(POWER_FOCUS, 5);
            }

        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_dire_beastAI(creature);
        }
};

#define FIREBOLT   104318

/*######
# npc_wild_imp
######*/

class npc_wild_imp : public CreatureScript
{
    public:
        npc_wild_imp() : CreatureScript("npc_wild_imp") { }

        struct npc_wild_impAI : public ScriptedAI
        {
            uint32 charges;

            npc_wild_impAI(Creature *creature) : ScriptedAI(creature)
            {
                charges = 10;
                me->SetReactState(REACT_AGGRESSIVE);
            }

            void Reset()
            {
                me->SetReactState(REACT_AGGRESSIVE);

                if (me->GetOwner())
                    if (me->GetOwner()->getVictim())
                        AttackStart(me->GetOwner()->getVictim());
            }

            void UpdateAI(uint32 diff)
            {
                if (me->GetReactState() != REACT_AGGRESSIVE)
                    me->SetReactState(REACT_AGGRESSIVE);

                if (!me->GetOwner())
                    return;

                if (!me->GetOwner()->ToPlayer())
                    return;

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                if (!charges)
                    me->DespawnOrUnsummon();

                if ((me->getVictim() || me->GetOwner()->getAttackerForHelper()))
                {
                    me->CastSpell(me->getVictim() ? me->getVictim() : me->GetOwner()->getAttackerForHelper(), FIREBOLT, false);
                    me->GetOwner()->EnergizeBySpell(me->GetOwner(), FIREBOLT, 5, POWER_DEMONIC_FURY);
                    charges--;
                    me->SetPower(POWER_ENERGY, charges);

                    if (me->GetOwner()->HasAura(122351))
                        if (roll_chance_i(8))
                            me->GetOwner()->CastSpell(me->GetOwner(), 122355, true);
                }
                else if (Pet* pet = me->GetOwner()->ToPlayer()->GetPet())
                {
                    if (pet->getAttackerForHelper())
                    {
                        me->CastSpell(me->getVictim() ? me->getVictim() : pet->getAttackerForHelper(), FIREBOLT, false);
                        me->GetOwner()->EnergizeBySpell(me->GetOwner(), FIREBOLT, 5, POWER_DEMONIC_FURY);
                        charges--;
                        me->SetPower(POWER_ENERGY, charges);

                        if (me->GetOwner()->HasAura(122351))
                            if (roll_chance_i(8))
                                me->GetOwner()->CastSpell(me->GetOwner(), 122355, true);
                    }
                }
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_wild_impAI(creature);
        }
};

/*######
## npc_earthgrab_totem
######*/

#define EARTHGRAB       116943

class npc_earthgrab_totem : public CreatureScript
{
    public:
        npc_earthgrab_totem() : CreatureScript("npc_earthgrab_totem") { }

    struct npc_earthgrab_totemAI : public ScriptedAI
    {
        npc_earthgrab_totemAI(Creature* creature) : ScriptedAI(creature)
        {
            creature->CastSpell(creature, EARTHGRAB, true);
        }

        void UpdateAI(uint32 diff)
        {
            if (!me->HasAura(EARTHGRAB))
                me->CastSpell(me, EARTHGRAB, true);
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_earthgrab_totemAI(creature);
    }
};

/*######
## npc_windwalk_totem
######*/

#define WINDWALK     114896

class npc_windwalk_totem : public CreatureScript
{
    public:
        npc_windwalk_totem() : CreatureScript("npc_windwalk_totem") { }

    struct npc_windwalk_totemAI : public ScriptedAI
    {
        npc_windwalk_totemAI(Creature* creature) : ScriptedAI(creature)
        {
            creature->CastSpell(creature, WINDWALK, true);
        }

        void UpdateAI(uint32 diff)
        {
            if (!me->HasAura(WINDWALK))
                me->CastSpell(me, WINDWALK, true);
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_windwalk_totemAI(creature);
    }
};

/*######
## npc_ring_of_frost
######*/

class npc_ring_of_frost : public CreatureScript
{
    public:
        npc_ring_of_frost() : CreatureScript("npc_ring_of_frost") { }

        struct npc_ring_of_frostAI : public Scripted_NoMovementAI
        {
            npc_ring_of_frostAI(Creature *c) : Scripted_NoMovementAI(c)
            {
                me->SetReactState(REACT_PASSIVE);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            }
        };

        CreatureAI* GetAI(Creature* pCreature) const
        {
            return new npc_ring_of_frostAI(pCreature);
        }
};

/*######
# npc_wild_mushroom
######*/

#define WILD_MUSHROOM_INVISIBILITY   92661

class npc_wild_mushroom : public CreatureScript
{
    public:
        npc_wild_mushroom() : CreatureScript("npc_wild_mushroom") { }

        struct npc_wild_mushroomAI : public ScriptedAI
        {
            uint32 CastTimer;
            bool stealthed;

            npc_wild_mushroomAI(Creature *creature) : ScriptedAI(creature)
            {
                CastTimer = 6000;
                stealthed = false;
                me->SetReactState(REACT_PASSIVE);
                me->SetMaxHealth(5);
            }

            void UpdateAI(uint32 diff)
            {
                if (CastTimer <= diff && !stealthed)
                {
                    DoCast(me, WILD_MUSHROOM_INVISIBILITY, true);
                    stealthed = true;
                }
                else
                {
                    CastTimer -= diff;

                    if (!stealthed)
                        me->RemoveAura(WILD_MUSHROOM_INVISIBILITY);
                }
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_wild_mushroomAI(creature);
        }
};

/*######
## npc_fungal_growth
######*/

#define FUNGAL_GROWTH_PERIODIC  81282
#define FUNGAL_GROWTH_AREA      94339

class npc_fungal_growth : public CreatureScript
{
    public:
        npc_fungal_growth() : CreatureScript("npc_fungal_growth") { }

        struct npc_fungal_growthAI : public Scripted_NoMovementAI
        {
            npc_fungal_growthAI(Creature *c) : Scripted_NoMovementAI(c)
            {
                me->SetReactState(REACT_PASSIVE);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            }

            void Reset()
            {
                me->SetReactState(REACT_PASSIVE);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            }

            void InitializeAI()
            {
                ScriptedAI::InitializeAI();
                Unit * owner = me->GetOwner();
                if (!owner || owner->GetTypeId() != TYPEID_PLAYER)
                    return;

                me->SetReactState(REACT_PASSIVE);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);

                me->CastSpell(me, FUNGAL_GROWTH_PERIODIC, true);    // Periodic Trigger spell : decrease speed
                me->CastSpell(me, FUNGAL_GROWTH_AREA, true);        // Persistent Area
            }

            void UpdateAI(uint32 diff)
            {
                if (!me->HasAura(FUNGAL_GROWTH_PERIODIC))
                    me->CastSpell(me, FUNGAL_GROWTH_PERIODIC, true);
            }
        };

        CreatureAI* GetAI(Creature* pCreature) const
        {
            return new npc_fungal_growthAI(pCreature);
        }
};
/*######
## npc_psyfiend -- 59190
######*/

enum PsyfiendSpells
{
    SPELL_PSYCHIC_HORROR    = 113792,
    SPELL_ROOT_FOR_EVER     = 31366,
    SPELL_SHADOW_FORM       = 34429,
};

class npc_psyfiend : public CreatureScript
{
    public:
        npc_psyfiend() : CreatureScript("npc_psyfiend") { }

        struct npc_psyfiendAI : public Scripted_NoMovementAI
        {
            npc_psyfiendAI(Creature* c) : Scripted_NoMovementAI(c)
            {
                me->SetReactState(REACT_AGGRESSIVE);
            }

            void Reset()
            {
                Unit * owner = me->GetOwner();
                if (!owner || owner->GetTypeId() != TYPEID_PLAYER)
                    return;

                if (!me->HasAura(SPELL_ROOT_FOR_EVER))
                    me->AddAura(SPELL_ROOT_FOR_EVER, me);

                if (!me->HasAura(SPELL_SHADOW_FORM))
                    me->AddAura(SPELL_SHADOW_FORM, me);

                me->SetLevel(owner->getLevel());
                me->SetMaxPower(POWER_RAGE, 0);
                me->SetMaxHealth(CalculatePct(owner->GetMaxHealth(), 2.5f));
                me->SetFullHealth();
                // Set no damage
                me->SetBaseWeaponDamage(BASE_ATTACK, MINDAMAGE, 0.0f);
                me->SetBaseWeaponDamage(BASE_ATTACK, MAXDAMAGE, 0.0f);
                
            }

            void UpdateAI(uint32 diff)
            {
                if (!me->HasUnitState(UNIT_STATE_CASTING))
                    DoCast(me, SPELL_PSYCHIC_HORROR, false);
            }
        };

        CreatureAI* GetAI(Creature *creature) const
        {
            return new npc_psyfiendAI(creature);
        }
};

/*######
## npc_void_tendrils -- 65282
######*/

enum voidTendrilsSpells
{
    PRIEST_SPELL_VOID_TENDRILS  = 114404,
};

class npc_void_tendrils : public CreatureScript
{
    public:
        npc_void_tendrils() : CreatureScript("npc_void_tendrils") { }

        struct npc_void_tendrilsAI : public Scripted_NoMovementAI
        {
            npc_void_tendrilsAI(Creature* c) : Scripted_NoMovementAI(c)
            {
                //me->SetReactState(REACT_AGGRESSIVE);
                if (Unit* m_target = me->GetTargetUnit())
                {
                    Aura* aura = me->AddAura(PRIEST_SPELL_VOID_TENDRILS, m_target);
                    if(m_target->GetTypeId() == TYPEID_PLAYER)
                    {
                        me->DespawnOrUnsummon(8000);
                        if(aura)
                            aura->SetDuration(8000);
                    }
                }
            }

            void IsSummonedBy(Unit* owner)
            {
                if (owner && owner->GetTypeId() == TYPEID_PLAYER)
                {
                    me->SetLevel(owner->getLevel());
                    me->SetMaxHealth(owner->CountPctFromMaxHealth(20));
                    me->SetHealth(me->GetMaxHealth());
                    // Set no damage
                    me->SetBaseWeaponDamage(BASE_ATTACK, MINDAMAGE, 0.0f);
                    me->SetBaseWeaponDamage(BASE_ATTACK, MAXDAMAGE, 0.0f);

                    me->AddAura(SPELL_ROOT_FOR_EVER, me);
                }
                else
                    me->DespawnOrUnsummon();
            }

            void Reset()
            {
                if (!me->HasAura(SPELL_ROOT_FOR_EVER))
                    me->AddAura(SPELL_ROOT_FOR_EVER, me);
            }

            void JustDied(Unit* killer)
            {
                if (Unit* m_target = me->GetTargetUnit())
                    m_target->RemoveAura(PRIEST_SPELL_VOID_TENDRILS);
            }
        };

        CreatureAI* GetAI(Creature *creature) const
        {
            return new npc_void_tendrilsAI(creature);
        }
};

/*######
## npc_spectral_guise -- 59607
######*/

enum spectralGuiseSpells
{
    SPELL_INITIALIZE_IMAGES         = 102284,
    SPELL_CLONE_CASTER              = 102288,
    SPELL_SPECTRAL_GUISE_CLONE      = 119012,
    SPELL_SPECTRAL_GUISE_CHARGES    = 119030,
    SPELL_SPECTRAL_GUISE_STEALTH    = 119032,
};

class npc_spectral_guise : public CreatureScript
{
    public:
        npc_spectral_guise() : CreatureScript("npc_spectral_guise") { }

        struct npc_spectral_guiseAI : public Scripted_NoMovementAI
        {
            npc_spectral_guiseAI(Creature* c) : Scripted_NoMovementAI(c)
            {
                me->SetReactState(REACT_PASSIVE);
            }

            void Reset()
            {
                if (!me->HasAura(SPELL_ROOT_FOR_EVER))
                    me->AddAura(SPELL_ROOT_FOR_EVER, me);
                if (Unit* owner = me->GetOwner())
                {
                    me->SetLevel(owner->getLevel());
                    me->SetMaxHealth(owner->GetMaxHealth() / 2);
                    me->SetHealth(me->GetMaxHealth());

                    owner->CastSpell(me, SPELL_CLONE_CASTER, true);
                    owner->CastSpell(me, SPELL_INITIALIZE_IMAGES, true);
                    me->CastSpell(me, SPELL_SPECTRAL_GUISE_CHARGES, true);
                    owner->CastSpell(me, 41055, true);
                    owner->AddAura(SPELL_SPECTRAL_GUISE_STEALTH, owner);

                    std::list<HostileReference*> threatList = owner->getThreatManager().getThreatList();
                    for (std::list<HostileReference*>::const_iterator itr = threatList.begin(); itr != threatList.end(); ++itr)
                        if (Unit* unit = (*itr)->getTarget())
                            if (unit->GetTypeId() == TYPEID_UNIT)
                                if (Creature* creature = unit->ToCreature())
                                    if (creature->canStartAttack(me, false))
                                        creature->Attack(me, true);

                    // Set no damage
                    me->SetBaseWeaponDamage(BASE_ATTACK, MINDAMAGE, 0.0f);
                    me->SetBaseWeaponDamage(BASE_ATTACK, MAXDAMAGE, 0.0f);

                    me->AddAura(SPELL_ROOT_FOR_EVER, me);
                    me->DespawnOrUnsummon(6000);
                }
            }

            void UpdateAI(uint32 diff) {}

            void EnterCombat(Unit* /*who*/) {}

            void EnterEvadeMode() {}
        };

        CreatureAI* GetAI(Creature *creature) const
        {
            return new npc_spectral_guiseAI(creature);
        }
};

/*######
## npc_bloodworm
######*/

#define BLOODWORM_BLOOD_GORGED  50453
#define BLOODWORM_BLOOD_STACKS  81277
#define BLOODWORM_BLOOD_BURST   81280

class npc_bloodworm : public CreatureScript
{
    public:
        npc_bloodworm() : CreatureScript("npc_bloodworm") { }

        struct npc_bloodwormAI : public ScriptedAI
        {
            npc_bloodwormAI(Creature* c) : ScriptedAI(c)
            {
            }

            uint32 uiBurstTimer;
            uint32 uiCheckBloodChargesTimer;

            void Burst()
            {
                if (Aura* bloodGorged = me->GetAura(BLOODWORM_BLOOD_STACKS))
                {
                    uint32 stacks = std::min<uint32>(bloodGorged->GetStackAmount(), 10);
                    int32 damage = stacks * (me->GetMaxHealth() * 0.25f);
                    me->CastCustomSpell(me, BLOODWORM_BLOOD_BURST, &damage, NULL, NULL, true);
                    me->DespawnOrUnsummon(500);
                }
            }

            void JustDied(Unit* killer)
            {
                Burst();
            }

            void Reset()
            {
                if (me->GetOwner())
                {
                    me->SetMaxHealth(0.15 * me->GetOwner()->GetHealth());
                    me->SetHealth(0.15 * me->GetOwner()->GetHealth());
                }
                
                DoCast(me, BLOODWORM_BLOOD_GORGED, true);

                uiBurstTimer = 19000;
                uiCheckBloodChargesTimer = 1500;
            }

            void UpdateAI(uint32 diff)
            {
                if (uiBurstTimer <= diff)
                    Burst();
                else
                    uiBurstTimer -= diff;

                if (uiCheckBloodChargesTimer <= diff)
                {
                    if (me->GetOwner())
                    {
                        if (Aura* bloodGorged = me->GetAura(BLOODWORM_BLOOD_STACKS))
                        {
                            // 10% per stack
                            int32 stacks = bloodGorged->GetStackAmount() * 10;
                            int32 masterPct = int32(100.0f - me->GetOwner()->GetHealthPct());
                            AddPct(stacks, masterPct);

                            if (stacks > 100)
                                stacks = 100;

                            if (roll_chance_i(stacks))
                                Burst();
                            else
                                uiCheckBloodChargesTimer = 1500;
                        }
                    }
                }
                else
                    uiCheckBloodChargesTimer -= diff;

                if (!UpdateVictim())
                {
                    if (Unit* target = me->SelectVictim())
                        me->Attack(target, true);
                    return;
                }

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature *creature) const
        {
            return new npc_bloodwormAI(creature);
        }
};

/*######
## npc_past_self
######*/

enum PastSelfSpells
{
    SPELL_FADING                    = 107550,
    SPELL_ALTER_TIME                = 110909,
    SPELL_ENCHANTED_REFLECTION      = 102284,
    SPELL_ENCHANTED_REFLECTION_2    = 102288,
};

struct auraData
{
    auraData(uint32 id, int32 duration) : m_id(id), m_duration(duration) {}
    uint32 m_id;
    int32 m_duration;
};

#define ACTION_ALTER_TIME   1

class npc_past_self : public CreatureScript
{
    public:
        npc_past_self() : CreatureScript("npc_past_self") { }

        struct npc_past_selfAI : public Scripted_NoMovementAI
        {
            npc_past_selfAI(Creature* c) : Scripted_NoMovementAI(c)
            {
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
                me->SetReactState(REACT_PASSIVE);
                me->SetMaxHealth(500);
                me->SetHealth(me->GetMaxHealth());
                mana = 0;
                health = 0;
                auras.clear();
            }

            int32 mana;
            int32 health;
            std::set<auraData*> auras;

            void Reset()
            {
                if (!me->HasAura(SPELL_FADING))
                    me->AddAura(SPELL_FADING, me);
            }

            void IsSummonedBy(Unit* owner)
            {
                if (owner && owner->GetTypeId() == TYPEID_PLAYER)
                {
                    Unit::AuraApplicationMap const& appliedAuras = owner->GetAppliedAuras();
                    for (Unit::AuraApplicationMap::const_iterator itr = appliedAuras.begin(); itr != appliedAuras.end(); ++itr)
                    {
                        if (Aura* aura = itr->second->GetBase())
                        {
                            SpellInfo const* auraInfo = aura->GetSpellInfo();
                            if (!auraInfo)
                                continue;

                            if (auraInfo->Id == SPELL_ALTER_TIME)
                                continue;

                            if (auraInfo->IsPassive())
                                continue;

                            auras.insert(new auraData(auraInfo->Id, aura->GetDuration()));
                        }
                    }

                    mana = owner->GetPower(POWER_MANA);
                    health = owner->GetHealth();

                    owner->AddAura(SPELL_ENCHANTED_REFLECTION, me);
                    owner->AddAura(SPELL_ENCHANTED_REFLECTION_2, me);
                }
                else
                    me->DespawnOrUnsummon();
            }

            void DoAction(const int32 action)
            {
                if (action == ACTION_ALTER_TIME)
                {
                    if (TempSummon* pastSelf = me->ToTempSummon())
                    {
                        if (Unit* m_owner = pastSelf->GetSummoner())
                        {
                            if (m_owner->ToPlayer())
                            {
                                if (!m_owner->isAlive())
                                    return;

                                //m_owner->RemoveNonPassivesAuras();

                                for (std::set<auraData*>::iterator itr = auras.begin(); itr != auras.end(); ++itr)
                                {
                                    Aura* aura = !m_owner->HasAura((*itr)->m_id) ? m_owner->AddAura((*itr)->m_id, m_owner) : m_owner->GetAura((*itr)->m_id);
                                    if (aura)
                                    {
                                        aura->SetDuration((*itr)->m_duration);
                                        aura->SetNeedClientUpdateForTargets();
                                    }

                                    delete (*itr);
                                }

                                auras.clear();

                                m_owner->SetPower(POWER_MANA, mana);
                                m_owner->SetHealth(health);

                                m_owner->NearTeleportTo(me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), me->GetOrientation(), true);
                                me->DespawnOrUnsummon(100);
                            }
                        }
                    }
                }
            }
        };

        CreatureAI* GetAI(Creature *creature) const
        {
            return new npc_past_selfAI(creature);
        }
};

/*######
## npc_transcendence_spirit -- 54569
######*/

enum TranscendenceSpiritSpells
{
    SPELL_VISUAL_SPIRIT     = 119053,
    SPELL_MEDITATE          = 124416,
};

enum transcendenceActions
{
    ACTION_TELEPORT     = 1,
};

class npc_transcendence_spirit : public CreatureScript
{
    public:
        npc_transcendence_spirit() : CreatureScript("npc_transcendence_spirit") { }

        struct npc_transcendence_spiritAI : public Scripted_NoMovementAI
        {
            npc_transcendence_spiritAI(Creature* c) : Scripted_NoMovementAI(c)
            {
                me->SetReactState(REACT_PASSIVE);
            }

            void Reset()
            {
                if (!me->HasAura(SPELL_MEDITATE))
                    me->AddAura(SPELL_MEDITATE, me);
            }

            void IsSummonedBy(Unit* owner)
            {
                if (owner && owner->GetTypeId() == TYPEID_PLAYER)
                {
                    me->SetMaxHealth(owner->GetMaxHealth() / 2);
                    me->SetHealth(me->GetMaxHealth());

                    me->CastSpell(me, SPELL_VISUAL_SPIRIT, true);
                    owner->CastSpell(me, SPELL_INITIALIZE_IMAGES, true);
                    owner->CastSpell(me, SPELL_CLONE_CASTER, true);
                    owner->AddAura(SPELL_MEDITATE, me);
                    me->AddAura(SPELL_ROOT_FOR_EVER, me);
                }
                else
                    me->DespawnOrUnsummon();
            }

            void MovementInform(uint32 type, uint32 id)
            {
                if (type != POINT_MOTION_TYPE)
                    return;

                if (id == 0)
                    me->SetSpeed(MOVE_RUN, 0.0f);
            }

            void DoAction(int32 const action)
            {
                switch (action)
                {
                    case ACTION_TELEPORT:
                        if (!me->GetOwner())
                        {
                            me->DespawnOrUnsummon(500);
                            break;
                        }

                        me->SetSpeed(MOVE_RUN, 10.0f);
                        me->GetMotionMaster()->MovePoint(0, me->GetOwner()->GetPositionX(), me->GetOwner()->GetPositionY(), me->GetOwner()->GetPositionZ());
                        me->SetOrientation(me->GetOwner()->GetOrientation());
                        break;
                    default:
                        break;
                }
            }
        };

        CreatureAI* GetAI(Creature *creature) const
        {
            return new npc_transcendence_spiritAI(creature);
        }
};

/*######
# npc_shahram
######*/

enum
{
    SPELL_FLAMES            = 16596,
    SPELL_CURSE             = 16597,
    SPELL_WILL              = 16598,
    SPELL_BLESSING          = 16599,
    SPELL_MIGHT             = 16600,
    SPELL_FIST              = 16601
};

static const uint32 aShahramCast[] = {SPELL_FLAMES, SPELL_CURSE, SPELL_WILL, SPELL_BLESSING, SPELL_MIGHT, SPELL_FIST};

class npc_shahram : public CreatureScript
{
    public:
        npc_shahram() : CreatureScript("npc_shahram") { }

    struct npc_shahramAI : public ScriptedAI
    {
        npc_shahramAI(Creature* creature) : ScriptedAI(creature) {}

        uint32 m_uiDeleteTimer;

        void Reset()
        {
            m_uiDeleteTimer = 2500;
            me->CastSpell(me, aShahramCast[urand(0, 5)], false);
        }

        void UpdateAI(uint32 uiDiff)
        {
            if (m_uiDeleteTimer < uiDiff)
                me->DespawnOrUnsummon();
            else
                m_uiDeleteTimer -= uiDiff;
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_shahramAI(creature);
    }
};

class npc_highwind_albatross : public CreatureScript
{
    public:
        npc_highwind_albatross() : CreatureScript("npc_highwind_albatross") { }

        struct npc_highwind_albatrossAI : Scripted_NoMovementAI
        {
            npc_highwind_albatrossAI(Creature* creature) : Scripted_NoMovementAI(creature)
            {
            }

            void SpellHit(Unit* source, SpellInfo const* /*spell*/)
            {
                if(source)
                {
                    Player* player = source->ToPlayer();
                    if(!player)
                        return;

                    int32 seatId = 1;
                    source->CastCustomSpell(me, 148764, &seatId, NULL, NULL, true);
                }
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_highwind_albatrossAI(creature);
        }
};

void AddSC_npcs_special()
{
    new npc_storm_earth_and_fire();
    new npc_air_force_bots();
    new npc_lunaclaw_spirit();
    new npc_chicken_cluck();
    new npc_dancing_flames();
    new npc_doctor();
    new npc_injured_patient();
    new npc_garments_of_quests();
    new npc_guardian();
    new npc_mount_vendor();
    new npc_rogue_trainer();
    new npc_sayge();
    new npc_steam_tonk();
    new npc_tonk_mine();
    new npc_winter_reveler();
    new npc_brewfest_reveler();
    new npc_snake_trap();
    new npc_mirror_image();
    new npc_ebon_gargoyle();
    new npc_lightwell();
    new npc_lightwell_mop();
    new mob_mojo();
    new npc_training_dummy();
    new npc_wormhole();
    new npc_pet_trainer();
    new npc_locksmith();
    new npc_experience();
    new npc_fire_elemental();
    new npc_earth_elemental();
    new npc_firework();
    new npc_spring_rabbit();
    new npc_generic_harpoon_cannon();
    new npc_choose_faction();
    new npc_rate_xp_modifier();
    new npc_capacitor_totem();
    new npc_feral_spirit();
    new npc_spirit_link_totem();
    new npc_demoralizing_banner();
    new npc_frozen_orb();
    new npc_guardian_of_ancient_kings();
    new npc_demonic_gateway();
    new npc_xuen_the_white_tiger();
    new npc_murder_of_crows();
    new npc_dire_beast();
    new npc_wild_imp();
    new npc_earthgrab_totem();
    new npc_windwalk_totem();
    new npc_ring_of_frost();
    new npc_wild_mushroom();
    new npc_fungal_growth();
    new npc_brewfest_trigger;
    new npc_brewfest_apple_trigger;
    new npc_brewfest_keg_thrower;
    new npc_brewfest_keg_receiver;
    new npc_brewfest_ram_master;
    new npc_psyfiend();
    new npc_void_tendrils();
    new npc_spectral_guise();
    new npc_bloodworm();
    new npc_transcendence_spirit();
    new npc_past_self();
    new npc_shahram();
    new npc_highwind_albatross();
}
