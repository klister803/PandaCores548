/*
 * Copyright (C) 2008-2011 TrinityCore <http://www.trinitycore.org/>
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
SDName: faction_champions
SD%Complete: ??%
SDComment: Scripts by Selector, modified by /dev/rsa
SDCategory: Crusader Coliseum
EndScriptData */

// Known bugs:
// All - untested
// Pets aren't being summoned by their masters

#include "ScriptPCH.h"
#include "trial_of_the_crusader.h"

enum eYell
{
    SAY_GARROSH_KILL_ALLIANCE_PLAYER4 = -1649118,
    SAY_VARIAN_KILL_HORDE_PLAYER4     = -1649123,
};

enum eAIs
{
    AI_MELEE    = 0,
    AI_RANGED   = 1,
    AI_HEALER   = 2,
    AI_PET      = 3,
};

enum eSpells
{
    SPELL_ANTI_AOE      = 68595,
    SPELL_PVP_TRINKET   = 42292,
};

class boss_toc_champion_controller : public CreatureScript
{
public:
    boss_toc_champion_controller() : CreatureScript("boss_toc_champion_controller") { }

    CreatureAI* GetAI(Creature *pCreature) const
    {
        return new boss_toc_champion_controllerAI (pCreature);
    }

    struct boss_toc_champion_controllerAI : public ScriptedAI
    {
        boss_toc_champion_controllerAI(Creature* pCreature) : ScriptedAI(pCreature), Summons(me)
        {
            m_pInstance = (InstanceScript *) pCreature->GetInstanceScript();
        }

        InstanceScript* m_pInstance;
        SummonList Summons;
        uint32 m_uiChampionsNotStarted;
        uint32 m_uiChampionsFailed;
        uint32 m_uiChampionsKilled;
        bool   m_bInProgress;

        void Reset()
        {
            m_uiChampionsNotStarted = 0;
            m_uiChampionsFailed = 0;
            m_uiChampionsKilled = 0;
            m_bInProgress = false;
        }

        std::vector<uint32> SelectChampions(Team playerTeam)
        {
            std::vector<uint32> vHealersEntries;
            vHealersEntries.clear();
            vHealersEntries.push_back(playerTeam == ALLIANCE ? NPC_HORDE_DRUID_RESTORATION : NPC_ALLIANCE_DRUID_RESTORATION);
            vHealersEntries.push_back(playerTeam == ALLIANCE ? NPC_HORDE_PALADIN_HOLY : NPC_ALLIANCE_PALADIN_HOLY);
            vHealersEntries.push_back(playerTeam == ALLIANCE ? NPC_HORDE_PRIEST_DISCIPLINE : NPC_ALLIANCE_PRIEST_DISCIPLINE);
            vHealersEntries.push_back(playerTeam == ALLIANCE ? NPC_HORDE_SHAMAN_RESTORATION : NPC_ALLIANCE_SHAMAN_RESTORATION);

            std::vector<uint32> vOtherEntries;
            vOtherEntries.clear();
            vOtherEntries.push_back(playerTeam == ALLIANCE ? NPC_HORDE_DEATH_KNIGHT : NPC_ALLIANCE_DEATH_KNIGHT);
            vOtherEntries.push_back(playerTeam == ALLIANCE ? NPC_HORDE_HUNTER : NPC_ALLIANCE_HUNTER);
            vOtherEntries.push_back(playerTeam == ALLIANCE ? NPC_HORDE_MAGE : NPC_ALLIANCE_MAGE);
            vOtherEntries.push_back(playerTeam == ALLIANCE ? NPC_HORDE_ROGUE : NPC_ALLIANCE_ROGUE);
            vOtherEntries.push_back(playerTeam == ALLIANCE ? NPC_HORDE_WARLOCK : NPC_ALLIANCE_WARLOCK);
            vOtherEntries.push_back(playerTeam == ALLIANCE ? NPC_HORDE_WARRIOR : NPC_ALLIANCE_WARRIOR);

            uint8 healersSubtracted = 2;
            if (m_pInstance->instance->GetSpawnMode() == MAN25_DIFFICULTY || m_pInstance->instance->GetSpawnMode() == MAN25_HEROIC_DIFFICULTY)
                healersSubtracted = 1;
            for (uint8 i = 0; i < healersSubtracted; ++i)
            {
                uint8 pos = urand(0, vHealersEntries.size()-1);
                switch (vHealersEntries[pos])
                {
                    case NPC_ALLIANCE_DRUID_RESTORATION:
                        vOtherEntries.push_back(NPC_ALLIANCE_DRUID_BALANCE);
                        break;
                    case NPC_HORDE_DRUID_RESTORATION:
                        vOtherEntries.push_back(NPC_HORDE_DRUID_BALANCE);
                        break;
                    case NPC_ALLIANCE_PALADIN_HOLY:
                        vOtherEntries.push_back(NPC_ALLIANCE_PALADIN_RETRIBUTION);
                        break;
                    case NPC_HORDE_PALADIN_HOLY:
                        vOtherEntries.push_back(NPC_HORDE_PALADIN_RETRIBUTION);
                        break;
                    case NPC_ALLIANCE_PRIEST_DISCIPLINE:
                        vOtherEntries.push_back(NPC_ALLIANCE_PRIEST_SHADOW);
                        break;
                    case NPC_HORDE_PRIEST_DISCIPLINE:
                        vOtherEntries.push_back(NPC_HORDE_PRIEST_SHADOW);
                        break;
                    case NPC_ALLIANCE_SHAMAN_RESTORATION:
                        vOtherEntries.push_back(NPC_ALLIANCE_SHAMAN_ENHANCEMENT);
                        break;
                    case NPC_HORDE_SHAMAN_RESTORATION:
                        vOtherEntries.push_back(NPC_HORDE_SHAMAN_ENHANCEMENT);
                        break;
                }
                vHealersEntries.erase(vHealersEntries.begin()+pos);
            }

            if (m_pInstance->instance->GetSpawnMode() == MAN10_DIFFICULTY || m_pInstance->instance->GetSpawnMode() == MAN10_HEROIC_DIFFICULTY)
                for (uint8 i = 0; i < 4; ++i)
                    vOtherEntries.erase(vOtherEntries.begin()+urand(0, vOtherEntries.size()-1));

            std::vector<uint32> vChampionEntries;
            vChampionEntries.clear();
            for (uint8 i = 0; i < vHealersEntries.size(); ++i)
                vChampionEntries.push_back(vHealersEntries[i]);
            for (uint8 i = 0; i < vOtherEntries.size(); ++i)
                vChampionEntries.push_back(vOtherEntries[i]);

            return vChampionEntries;
        }

        void SummonChampions(Team playerTeam)
        {
            std::vector<Position> vChampionJumpOrigin;
            if (playerTeam == ALLIANCE)
                for (uint8 i = 0; i < 5; i++)
                    vChampionJumpOrigin.push_back(FactionChampionLoc[i]);
            else
                for (uint8 i = 5; i < 10; i++)
                    vChampionJumpOrigin.push_back(FactionChampionLoc[i]);

            std::vector<Position> vChampionJumpTarget;
            for (uint8 i = 10; i < 20; i++)
                vChampionJumpTarget.push_back(FactionChampionLoc[i]);
            std::vector<uint32> vChampionEntries = SelectChampions(playerTeam);

            for (uint8 i = 0; i < vChampionEntries.size(); ++i)
            {
                uint8 pos = urand(0, vChampionJumpTarget.size()-1);
                if (Creature* pTemp = me->SummonCreature(vChampionEntries[i], vChampionJumpOrigin[urand(0, vChampionJumpOrigin.size()-1)], TEMPSUMMON_MANUAL_DESPAWN))
                {
                    Summons.Summon(pTemp);
                    pTemp->SetReactState(REACT_PASSIVE);
                    pTemp->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_IMMUNE_TO_PC);
                    if (playerTeam == ALLIANCE)
                    {
                        pTemp->SetHomePosition(vChampionJumpTarget[pos].GetPositionX(), vChampionJumpTarget[pos].GetPositionY(), vChampionJumpTarget[pos].GetPositionZ(), 0);
                        pTemp->GetMotionMaster()->MoveJump(vChampionJumpTarget[pos].GetPositionX(), vChampionJumpTarget[pos].GetPositionY(), vChampionJumpTarget[pos].GetPositionZ(), 20.0f, 20.0f);
                        pTemp->SetOrientation(0);
                    }
                    else
                    {
                        pTemp->SetHomePosition((ToCCommonLoc[1].GetPositionX()*2)-vChampionJumpTarget[pos].GetPositionX(), vChampionJumpTarget[pos].GetPositionY(), vChampionJumpTarget[pos].GetPositionZ(), 3);
                        pTemp->GetMotionMaster()->MoveJump((ToCCommonLoc[1].GetPositionX()*2)-vChampionJumpTarget[pos].GetPositionX(), vChampionJumpTarget[pos].GetPositionY(), vChampionJumpTarget[pos].GetPositionZ(), 20.0f, 20.0f);
                        pTemp->SetOrientation(3);
                    }
                }
                vChampionJumpTarget.erase(vChampionJumpTarget.begin()+pos);
            }
        }

        void SetData(uint32 uiType, uint32 uiData)
        {
            switch (uiType)
            {
                case 0:
                    SummonChampions((Team)uiData);
                    break;
                case 1:
                    for (std::list<uint64>::iterator i = Summons.begin(); i != Summons.end(); ++i)
                    {
                        if (Creature* pTemp = Unit::GetCreature(*me, *i))
                        {
                            pTemp->SetReactState(REACT_AGGRESSIVE);
                            pTemp->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_IMMUNE_TO_PC);
                        }
                    }
                    break;
                case 2:
                    switch (uiData)
                    {
                        case FAIL:
                            m_uiChampionsFailed++;
                            if (m_uiChampionsFailed + m_uiChampionsKilled >= Summons.size())
                            {
                                m_pInstance->SetData(TYPE_CRUSADERS, FAIL);
                                Summons.DespawnAll();
                                me->DespawnOrUnsummon();
                            }
                            break;
                        case IN_PROGRESS:
                            if (!m_bInProgress)
                            {
                                m_uiChampionsNotStarted = 0;
                                m_uiChampionsFailed = 0;
                                m_uiChampionsKilled = 0;
                                m_bInProgress = true;
                                Summons.DoZoneInCombat();
                                m_pInstance->SetData(TYPE_CRUSADERS, IN_PROGRESS);
                            }
                            break;
                        case DONE:
                            m_uiChampionsKilled++;
                            if (m_uiChampionsKilled == 1)
                                m_pInstance->SetData(TYPE_CRUSADERS, SPECIAL);
                            else if (m_uiChampionsKilled >= Summons.size())
                            {
                                m_pInstance->SetData(TYPE_CRUSADERS, DONE);
                                Summons.DespawnAll();
                                me->DespawnOrUnsummon();
                            }
                            break;
                    }
                    break;
            }
        }
    };

};

struct boss_faction_championsAI : public ScriptedAI
{
    boss_faction_championsAI(Creature* pCreature, uint32 aitype) : ScriptedAI(pCreature)
    {
        m_pInstance = (InstanceScript *) pCreature->GetInstanceScript();
        mAIType = aitype;
    }

    InstanceScript* m_pInstance;

    uint64 championControllerGUID;
    uint32 mAIType;
    uint32 ThreatTimer;
    
    void Reset()
    {
        championControllerGUID = 0;
        ThreatTimer = 5000;
        
    }

    void JustReachedHome()
    {
        if (m_pInstance)
            if (Creature* pChampionController = Unit::GetCreature((*me), m_pInstance->GetData64(NPC_CHAMPIONS_CONTROLLER)))
                pChampionController->AI()->SetData(2, FAIL);
        me->DespawnOrUnsummon();
    }

    float CalculateThreat(float distance, float armor, uint32 health)
    {
        float dist_mod = (mAIType == AI_MELEE || mAIType == AI_PET) ? 15.0f/(15.0f + distance) : 1.0f;
        float armor_mod = (mAIType == AI_MELEE || mAIType == AI_PET) ? armor / 16635.0f : 0.0f;
        float eh = (health+1) * (1.0f + armor_mod);
        return dist_mod * 30000.0f / eh;
    }

    void UpdateThreat()
    {
        std::list<HostileReference*> const& tList = me->getThreatManager().getThreatList();
        for (std::list<HostileReference*>::const_iterator itr = tList.begin(); itr != tList.end(); ++itr)
        {
            Unit* pUnit = Unit::GetUnit((*me), (*itr)->getUnitGuid());
            if (pUnit && me->getThreatManager().getThreat(pUnit))
            {
                if (pUnit->GetTypeId()==TYPEID_PLAYER)
                {
                    float threat = CalculateThreat(me->GetDistance2d(pUnit), (float)pUnit->GetArmor(), pUnit->GetHealth());
                    me->getThreatManager().modifyThreatPercent(pUnit, -100);
                    me->AddThreat(pUnit, 1000000.0f * threat);
                }
            }
        }
    }

    void UpdatePower()
    {
        if (me->getPowerType() == POWER_MANA)
            me->ModifyPower(POWER_MANA, me->GetMaxPower(POWER_MANA) / 3);
        //else if (me->getPowerType() == POWER_ENERGY)
        //    me->ModifyPower(POWER_ENERGY, 100);
    }

    void JustDied(Unit* /*killer*/)
    {
        if (mAIType != AI_PET)
            if (m_pInstance)
                if (Creature* pChampionController = Unit::GetCreature((*me), m_pInstance->GetData64(NPC_CHAMPIONS_CONTROLLER)))
                    pChampionController->AI()->SetData(2, DONE);
    }

    void EnterCombat(Unit* /*who*/)
    {
        DoCast(me, SPELL_ANTI_AOE, true);
        me->SetInCombatWithZone();
        if (m_pInstance)
            if (Creature* pChampionController = Unit::GetCreature((*me), m_pInstance->GetData64(NPC_CHAMPIONS_CONTROLLER)))
                pChampionController->AI()->SetData(2, IN_PROGRESS);
    }

    void KilledUnit(Unit* who)
    {
        if (who->GetTypeId() == TYPEID_PLAYER)
        {
            Map::PlayerList const &players = me->GetMap()->GetPlayers();
            uint32 TeamInInstance = 0;

            if (!players.isEmpty())
                if (Player* pPlayer = players.begin()->getSource())
                    TeamInInstance = pPlayer->GetTeam();

            if (m_pInstance)
            {
                if (TeamInInstance == ALLIANCE)
                {
                    if (Creature* pTemp = Unit::GetCreature(*me, m_pInstance->GetData64(NPC_VARIAN)))
                        DoScriptText(SAY_VARIAN_KILL_HORDE_PLAYER4+urand(0, 3), pTemp); // + cause we are on negative
                }
                else
                    if (Creature* pTemp = me->FindNearestCreature(NPC_GARROSH, 300.f))
                        DoScriptText(SAY_GARROSH_KILL_ALLIANCE_PLAYER4+urand(0, 3), pTemp); // + cause we are on negative

                m_pInstance->SetData(DATA_TRIBUTE_TO_IMMORTALITY_ELEGIBLE, 0);
            }
        }
    }

    Creature* SelectRandomFriendlyMissingBuff(uint32 spell)
    {
        std::list<Creature *> lst = DoFindFriendlyMissingBuff(40.0f, spell);
        std::list<Creature *>::const_iterator itr = lst.begin();
        if (lst.empty())
            return NULL;
        advance(itr, rand()%lst.size());
        return (*itr);
    }

    Unit* SelectEnemyCaster(bool /*casting*/)
    {
        std::list<HostileReference*> const& tList = me->getThreatManager().getThreatList();
        std::list<HostileReference*>::const_iterator iter;
        Unit *target;
        for (iter = tList.begin(); iter!=tList.end(); ++iter)
        {
            target = Unit::GetUnit((*me), (*iter)->getUnitGuid());
            if (target && target->getPowerType() == POWER_MANA)
                return target;
        }
        return NULL;
    }

    uint32 EnemiesInRange(float distance)
    {
        std::list<HostileReference*> const& tList = me->getThreatManager().getThreatList();
        std::list<HostileReference*>::const_iterator iter;
        uint32 count = 0;
        Unit *target;
        for (iter = tList.begin(); iter!=tList.end(); ++iter)
        {
            target = Unit::GetUnit((*me), (*iter)->getUnitGuid());
                if (target && me->GetDistance2d(target) < distance)
                    ++count;
        }
        return count;
    }

    void AttackStart(Unit* pWho)
    {
        if (!pWho) return;

        if (me->Attack(pWho, true))
        {
            me->AddThreat(pWho, 30.0f);
            me->SetInCombatWith(pWho);
            pWho->SetInCombatWith(me);
            
            if (mAIType == AI_MELEE || mAIType == AI_PET)
            {
                DoStartMovement(pWho);
            SetCombatMovement(true);
            }

            if (mAIType == AI_RANGED || mAIType == AI_HEALER)
            {
                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                DoStartMovement(pWho, 20.0f);
                SetCombatMovement(false);
            }
           
        }
    }

    void UpdateAI(uint32 uiDiff)
    {
        if (ThreatTimer < uiDiff)
        {
            UpdatePower();
            UpdateThreat();
            ThreatTimer = 4000;
        }
        else ThreatTimer -= uiDiff;

       if (mAIType == AI_MELEE || mAIType == AI_PET) DoMeleeAttackIfReady();
    }
};

/********************************************************************
                            HEALERS
********************************************************************/
enum eDruidSpells
{
    SPELL_LIFEBLOOM         = 66093,
    SPELL_NOURISH           = 66066,
    SPELL_REGROWTH          = 66067,
    SPELL_REJUVENATION      = 66065,
    SPELL_TRANQUILITY       = 66086,
    SPELL_BARKSKIN          = 65860, 
    SPELL_THORNS            = 66068,
    SPELL_NATURE_GRASP      = 66071, 
};

class mob_toc_druid : public CreatureScript
{
public:
    mob_toc_druid() : CreatureScript("mob_toc_druid") { }

    CreatureAI* GetAI(Creature *pCreature) const
    {
        return new mob_toc_druidAI (pCreature);
    }

    struct mob_toc_druidAI : public boss_faction_championsAI
    {
        mob_toc_druidAI(Creature* pCreature) : boss_faction_championsAI(pCreature, AI_HEALER) {}

        uint32 m_uiNatureGraspTimer;
        uint32 m_uiTranquilityTimer;
        uint32 m_uiBarkskinTimer;
        uint32 m_uiRegrowthTimer;
        uint32 m_uiLifebloomTimer;
        uint32 m_uiRejuvenationTimer;
        uint32 m_uiNourishTimer;
        uint32 m_uiDamageCount;
        uint32 m_uiPvpTrinketTimer;
        bool m_applyBarkskin;
        bool m_Tranq;
        bool Trinket;



        void Reset()
        {
            boss_faction_championsAI::Reset();
            m_uiDamageCount = 0;
            m_uiNourishTimer = 8*IN_MILLISECONDS;
            m_uiNatureGraspTimer = 40*IN_MILLISECONDS;
            m_uiRejuvenationTimer = 10*IN_MILLISECONDS;
            m_uiRegrowthTimer = 6*IN_MILLISECONDS;
            m_uiLifebloomTimer = 4*IN_MILLISECONDS;
            m_uiTranquilityTimer = 0;
            m_uiBarkskinTimer = 0;
            m_uiPvpTrinketTimer = 0;
            m_applyBarkskin = false;
            Trinket = true;
            m_Tranq = true;
            SetEquipmentSlots(false, 51799, EQUIP_NO_CHANGE, EQUIP_NO_CHANGE);
        }

        void DamageTaken(Unit* target, uint32 &damage)
        {
            m_uiDamageCount += damage;

            if (m_uiDamageCount >= 50000)
                m_applyBarkskin = true;
        }

        void SelfHeal(const uint32 uiDiff)
        {
            if (m_uiRejuvenationTimer <= uiDiff)
            {
                if (HealthBelowPct(50))
                    DoCast(me, SPELL_REJUVENATION);
                m_uiRejuvenationTimer = 10*IN_MILLISECONDS;
            } else m_uiRejuvenationTimer -= uiDiff;

            if (m_uiRegrowthTimer <= uiDiff)
            {
                if (HealthBelowPct(50))
                    DoCast(me, SPELL_REGROWTH);
                m_uiRegrowthTimer = 6*IN_MILLISECONDS;
            } else m_uiRegrowthTimer -= uiDiff;

            if (m_uiNourishTimer <= uiDiff)
            {
                if (HealthBelowPct(50))
                    DoCast(me, SPELL_NOURISH);
                m_uiNourishTimer = 8*IN_MILLISECONDS;
            } else m_uiNourishTimer -= uiDiff;
        }
        
        void PartyHeal(const uint32 uiDiff)
        {
            if (m_uiRejuvenationTimer <= uiDiff)
            {
                if (Unit* target = DoSelectLowestHpFriendly(40.0f))
                    if (target->HealthBelowPct(95))
                        DoCast(target, SPELL_REJUVENATION);
                m_uiRejuvenationTimer = 10*IN_MILLISECONDS;
            } else m_uiRejuvenationTimer -= uiDiff;

            if (m_uiNourishTimer <= uiDiff)
            {
                if (Unit* target = DoSelectLowestHpFriendly(40.0f))
                    if (target->HealthBelowPct(95))
                        DoCast(target, SPELL_NOURISH);
                m_uiNourishTimer = 8*IN_MILLISECONDS;
            } else m_uiNourishTimer -= uiDiff;

            if (m_uiRegrowthTimer <= uiDiff)
            {
                if (Unit* target = DoSelectLowestHpFriendly(40.0f))
                    if (target->HealthBelowPct(95))
                        DoCast(target, SPELL_REGROWTH, true);
                m_uiRegrowthTimer = 6*IN_MILLISECONDS;
            } else m_uiRegrowthTimer -= uiDiff;

            if (m_uiLifebloomTimer <= uiDiff)
            {
                if (Unit* target = DoSelectLowestHpFriendly(40.0f))
                    if (target->HealthBelowPct(95))
                        DoCast(target, SPELL_LIFEBLOOM);
                m_uiLifebloomTimer = 4*IN_MILLISECONDS;
            } else m_uiLifebloomTimer -= uiDiff;
        }
        
        void UpdateAI(uint32 uiDiff)
        {
            if (!UpdateVictim()) return;

            if (m_uiBarkskinTimer)
            {
                if (m_uiBarkskinTimer <= uiDiff)
                    m_uiBarkskinTimer = 0;
                else 
                    m_uiBarkskinTimer -= uiDiff;
            }

            if (m_uiTranquilityTimer)
            {
                if (m_uiTranquilityTimer <= uiDiff)
                {
                    m_Tranq = true;
                    m_uiTranquilityTimer = 0;
                }
                else m_uiTranquilityTimer -= uiDiff;
            }

            if (m_applyBarkskin)
            {
                if (!m_uiBarkskinTimer)
                {
                    DoCast(me, SPELL_BARKSKIN);
                    m_uiBarkskinTimer = 20*IN_MILLISECONDS;
                    m_uiDamageCount = 0;
                }
            }

            if (HealthBelowPct(50) && m_Tranq)
            {
                DoCastAOE(SPELL_TRANQUILITY);
                m_Tranq = false;
                m_uiTranquilityTimer = 30*IN_MILLISECONDS;
            }

            if (m_uiNatureGraspTimer <= uiDiff)
            {
                if (!me->HasAura(SPELL_NATURE_GRASP))
                DoCast(me, SPELL_NATURE_GRASP);
                m_uiNatureGraspTimer = 40*IN_MILLISECONDS;
            } else m_uiNatureGraspTimer -= uiDiff;
            
            if (me->HasAuraType(SPELL_AURA_MOD_STUN) || me->HasAuraType(SPELL_AURA_MOD_FEAR) || me->HasAuraType(SPELL_AURA_MOD_CHARM) || me->HasAuraType(SPELL_AURA_MOD_ROOT) || me->HasAuraType(SPELL_AURA_MOD_CONFUSE))
            {
                if (Trinket)
                {
                    DoCast(me, SPELL_PVP_TRINKET, true);
                    Trinket = false;
                    m_uiPvpTrinketTimer = 120*IN_MILLISECONDS;
                }
            }

            if (m_uiPvpTrinketTimer)
            {
                if (m_uiPvpTrinketTimer <= uiDiff)
                {
                    Trinket = true;
                    m_uiPvpTrinketTimer = 0;
                }
                else m_uiPvpTrinketTimer -= uiDiff;
            }

            if (HealthBelowPct(50))
                SelfHeal(uiDiff);
            else
                PartyHeal(uiDiff);
           

            boss_faction_championsAI::UpdateAI(uiDiff);
        }
    };

};

enum eShamanSpells
{
    SPELL_HEALING_WAVE          = 66055,
    SPELL_RIPTIDE               = 66053, //cd 6sec
    SPELL_SPIRIT_CLEANSE        = 66056, 
    SPELL_HEROISM               = 65983, //cd 5min
    SPELL_BLOODLUST             = 65980, //cd 5min
    SPELL_HEX                   = 66054, //cd 45 sec
    SPELL_EARTH_SHIELD          = 66063,
    SPELL_EARTH_SHOCK           = 65973, //cd 6sec
    AURA_EXHAUSTION             = 57723,
    AURA_SATED                  = 57724,
};

class mob_toc_shaman : public CreatureScript
{
public:
    mob_toc_shaman() : CreatureScript("mob_toc_shaman") { }

    CreatureAI* GetAI(Creature *pCreature) const
    {
        return new mob_toc_shamanAI (pCreature);
    }

    struct mob_toc_shamanAI : public boss_faction_championsAI
    {
        mob_toc_shamanAI(Creature *pCreature) : boss_faction_championsAI(pCreature, AI_HEALER) {}

        uint32 m_uiHeroismOrBloodlustTimer;
        uint32 m_uiHexTimer;
        uint32 m_uiHealingWaveTimer;
        uint32 m_uiRiptideTimer;
        uint32 m_uiEarthShieldTimer;
        uint32 m_uiEarthShockTimer;
        uint32 m_uiPvpTrinketTimer;
        bool Trinket;

        void Reset()
        {
            boss_faction_championsAI::Reset();
            m_uiEarthShockTimer = 15*IN_MILLISECONDS;
            m_uiEarthShieldTimer = 10*IN_MILLISECONDS;
            m_uiRiptideTimer = 6*IN_MILLISECONDS;
            m_uiHealingWaveTimer = 6*IN_MILLISECONDS;
            m_uiHeroismOrBloodlustTimer = IN_MILLISECONDS;
            m_uiHexTimer = 45*IN_MILLISECONDS;
            m_uiPvpTrinketTimer = 0;
            Trinket = true;
            SetEquipmentSlots(false, 45620, 34986, EQUIP_NO_CHANGE);
        }

        void EnterCombat(Unit* /*who*/)
        {
            DoCast(me, SPELL_EARTH_SHIELD);
        }

        void SelfHeal(const uint32 uiDiff)
        {
            if (m_uiRiptideTimer <= uiDiff)
            {
                if (HealthBelowPct(50))
                    DoCast(me, SPELL_RIPTIDE, true);
                m_uiRiptideTimer = 6*IN_MILLISECONDS;
            } else m_uiRiptideTimer -= uiDiff;

            if (m_uiHealingWaveTimer <= uiDiff)
            {
                if (HealthBelowPct(50))
                    DoCast(me, SPELL_HEALING_WAVE, true);
                m_uiHealingWaveTimer = 6*IN_MILLISECONDS;
            } else m_uiHealingWaveTimer -= uiDiff;
        }

        void PartyHeal(const uint32 uiDiff)
        {
            if (m_uiRiptideTimer <= uiDiff)
            {
                if (Unit* target = DoSelectLowestHpFriendly(40.0f))
                    if (target->HealthBelowPct(95))
                        DoCast(target, SPELL_RIPTIDE, true);
                m_uiRiptideTimer = 6*IN_MILLISECONDS;
            } else m_uiRiptideTimer -= uiDiff;

            if (m_uiHealingWaveTimer <= uiDiff)
            {
                if (Unit* target = DoSelectLowestHpFriendly(40.0f))
                    if (target->HealthBelowPct(95))
                        DoCast(target, SPELL_HEALING_WAVE, true);
                m_uiHealingWaveTimer = 6*IN_MILLISECONDS;
            } else m_uiHealingWaveTimer -= uiDiff;
        }

        void UpdateAI(uint32 uiDiff)
        {
            if (!UpdateVictim()) return;

            if (m_uiHeroismOrBloodlustTimer <= uiDiff)
            {
                if (me->getFaction()) //Am i alliance?
                {
                    if (!me->HasAura(AURA_EXHAUSTION))
                        DoCastAOE(SPELL_HEROISM);
                }
                else
                    if (!me->HasAura(AURA_SATED))
                        DoCastAOE(SPELL_BLOODLUST);
                m_uiHeroismOrBloodlustTimer = 300*IN_MILLISECONDS;
            } else m_uiHeroismOrBloodlustTimer -= uiDiff;

            if (m_uiEarthShieldTimer <= uiDiff)
            {
                if (!me->HasAura(SPELL_EARTH_SHIELD))
                    DoCast(me, SPELL_EARTH_SHIELD);
                m_uiEarthShieldTimer = 10*IN_MILLISECONDS;
            } else m_uiEarthShieldTimer -= uiDiff;

            if (m_uiEarthShockTimer <= uiDiff)
            {
                if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0))
                    DoCast(target, SPELL_EARTH_SHOCK);
                m_uiEarthShockTimer = 15*IN_MILLISECONDS;
            } else m_uiEarthShockTimer -= uiDiff;

            if (m_uiHexTimer <= uiDiff)
            {
                if (Unit *pTarget = SelectTarget(SELECT_TARGET_RANDOM, 1))
                    DoCast(pTarget, SPELL_HEX);
                m_uiHexTimer = 45*IN_MILLISECONDS;
            } else m_uiHexTimer -= uiDiff;

            if (me->HasAuraType(SPELL_AURA_MOD_STUN) || me->HasAuraType(SPELL_AURA_MOD_FEAR) || me->HasAuraType(SPELL_AURA_MOD_CHARM) || me->HasAuraType(SPELL_AURA_MOD_ROOT) || me->HasAuraType(SPELL_AURA_MOD_CONFUSE))
            {
                if (Trinket)
                {
                    DoCast(me, SPELL_PVP_TRINKET, true);
                    Trinket = false;
                    m_uiPvpTrinketTimer = 120*IN_MILLISECONDS;
                }
            }

            if (m_uiPvpTrinketTimer)
            {
                if (m_uiPvpTrinketTimer <= uiDiff)
                {
                    Trinket = true;
                    m_uiPvpTrinketTimer = 0;
                }
                else m_uiPvpTrinketTimer -= uiDiff;
            }

            if (HealthBelowPct(50))
                SelfHeal(uiDiff);
            else
                PartyHeal(uiDiff);
            
            boss_faction_championsAI::UpdateAI(uiDiff);
        }
    };

};

enum ePaladinSpells
{
    SPELL_AVENGING_WRATH_HOLY = 66011,
    SPELL_HAND_OF_FREEDOM     = 68757, 
    SPELL_BUBBLE              = 66010,  //cd 5min
    SPELL_CLEANSE             = 66116,
    SPELL_FLASH_OF_LIGHT      = 66113, 
    SPELL_HOLY_LIGHT          = 66112,
    SPELL_HOLY_SHOCK          = 66114, //cd 6sec
    SPELL_HAND_OF_PROTECTION  = 66009, //cd 5min
    SPELL_HAMMER_OF_JUSTICE   = 66613, //cd 40sec
};

class mob_toc_paladin : public CreatureScript
{
public:
    mob_toc_paladin() : CreatureScript("mob_toc_paladin") { }

    CreatureAI* GetAI(Creature *pCreature) const
    {
        return new mob_toc_paladinAI (pCreature);
    }

    struct mob_toc_paladinAI : public boss_faction_championsAI
    {
        mob_toc_paladinAI(Creature *pCreature) : boss_faction_championsAI(pCreature, AI_HEALER) {}

        uint32 m_uiBubbleTimer;
        uint32 m_uiHandOfProtectionTimer;
        uint32 m_uiHolyShockTimer;
        uint32 m_uiHandOfFreedomTimer;
        uint32 m_uiHammerOfJusticeTimer;
        uint32 m_uiHolyLightTimer;
        uint32 m_uiAvengingWrathHolyTimer;
        uint32 m_uiFlashOfLightTimer;
        uint32 m_uiPvpTrinketTimer;
        bool Trinket;
        bool bubble;

        void Reset()
        {
            boss_faction_championsAI::Reset();
            m_uiFlashOfLightTimer = 4*IN_MILLISECONDS;
            m_uiAvengingWrathHolyTimer = 360*IN_MILLISECONDS;
            m_uiHolyLightTimer = 6*IN_MILLISECONDS;
            m_uiHandOfProtectionTimer = 300*IN_MILLISECONDS;
            m_uiHolyShockTimer = 8*IN_MILLISECONDS;
            m_uiHandOfFreedomTimer = urand(25*IN_MILLISECONDS, 40*IN_MILLISECONDS);
            m_uiHammerOfJusticeTimer = 40*IN_MILLISECONDS;
            m_uiPvpTrinketTimer = 0;
            m_uiBubbleTimer = 0;
            Trinket = true;
            bubble = true;
            SetEquipmentSlots(false, 28771, 32255, EQUIP_NO_CHANGE);
        }

        void EnterCombat(Unit* /*who*/)
        {
            DoCast(me, SPELL_AVENGING_WRATH_HOLY);
        }

        void SelfHeal(const uint32 uiDiff)
        {
            if (m_uiFlashOfLightTimer <= uiDiff)
            {
                if (HealthBelowPct(50))
                DoCast(me, SPELL_FLASH_OF_LIGHT);
                m_uiFlashOfLightTimer = 4*IN_MILLISECONDS;
            } else m_uiFlashOfLightTimer -= uiDiff;
            
            if (m_uiHolyLightTimer <=uiDiff)
            {
                if (HealthBelowPct(50))
                    DoCast(me, SPELL_HOLY_LIGHT);
                m_uiHolyLightTimer = 6*IN_MILLISECONDS;
            } else m_uiHolyLightTimer -= uiDiff;

            if (m_uiHolyShockTimer <= uiDiff)
            {
                if (HealthBelowPct(50))
                    DoCast(me, SPELL_HOLY_SHOCK);
                m_uiHolyShockTimer = 8*IN_MILLISECONDS;
            } else m_uiHolyShockTimer -= uiDiff;
        }
        
        void PartyHeal(const uint32 uiDiff)
        {
           if (m_uiFlashOfLightTimer <= uiDiff)
            {
              if (Unit* target = DoSelectLowestHpFriendly(40.0f))
                  if(target->HealthBelowPct(95))
                      DoCast(target, SPELL_FLASH_OF_LIGHT);
              m_uiFlashOfLightTimer = 4*IN_MILLISECONDS;
            } else m_uiFlashOfLightTimer -= uiDiff;

            if (m_uiHolyLightTimer <= uiDiff)
            {
              if (Unit* target = DoSelectLowestHpFriendly(40.0f))
                  if (target->HealthBelowPct(95))
                      DoCast(target, SPELL_HOLY_LIGHT);
                    m_uiHolyLightTimer = 6*IN_MILLISECONDS;
            } else m_uiHolyLightTimer -= uiDiff;

            if (m_uiHolyShockTimer <= uiDiff)
            {
              if (Unit* target = DoSelectLowestHpFriendly(40.0f))
                  if (target->HealthBelowPct(95))
                      DoCast(target, SPELL_HOLY_SHOCK);
              m_uiHolyShockTimer = 8*IN_MILLISECONDS;
            } else m_uiHolyShockTimer -= uiDiff;
        }
        
        void UpdateAI(uint32 uiDiff)
        {
            if (!UpdateVictim()) return;

            if (m_uiBubbleTimer)
            {
                if (m_uiBubbleTimer <= uiDiff)
                {
                    bubble = true;
                    m_uiBubbleTimer = 0;
                } else m_uiBubbleTimer -= uiDiff;
            }

            if (m_uiAvengingWrathHolyTimer <= uiDiff)
            {
                DoCast(me, SPELL_AVENGING_WRATH_HOLY);
                    m_uiAvengingWrathHolyTimer = 360*IN_MILLISECONDS;
            } else m_uiAvengingWrathHolyTimer -= uiDiff;

            if (m_uiHandOfProtectionTimer <= uiDiff)
            {
                if (Unit* target = DoSelectLowestHpFriendly(40.0f))
                    if (target->HealthBelowPct(25))
                        DoCast(target, SPELL_HAND_OF_PROTECTION);
                m_uiHandOfProtectionTimer = urand(0*IN_MILLISECONDS, 360*IN_MILLISECONDS);
            } else m_uiHandOfProtectionTimer -= uiDiff;

            if (m_uiHandOfFreedomTimer <= uiDiff)
            {
                if (Unit* target = SelectRandomFriendlyMissingBuff(SPELL_HAND_OF_FREEDOM))
                    DoCast(target, SPELL_HAND_OF_FREEDOM);
                m_uiHandOfFreedomTimer = urand(25*IN_MILLISECONDS, 40*IN_MILLISECONDS);
            } else m_uiHandOfFreedomTimer -= uiDiff;

            if (m_uiHammerOfJusticeTimer <= uiDiff)
            {
                if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0))
                    DoCast(target, SPELL_HAMMER_OF_JUSTICE);
                m_uiHammerOfJusticeTimer = 40*IN_MILLISECONDS;
            } else m_uiHammerOfJusticeTimer -= uiDiff;

            if (HealthBelowPct(25) && bubble)
            {
                DoCast(SPELL_BUBBLE);
                bubble = false;
                m_uiBubbleTimer = 300*IN_MILLISECONDS;
            }
            
            if (me->HasAuraType(SPELL_AURA_MOD_STUN) || me->HasAuraType(SPELL_AURA_MOD_FEAR) || me->HasAuraType(SPELL_AURA_MOD_CHARM) || me->HasAuraType(SPELL_AURA_MOD_ROOT) || me->HasAuraType(SPELL_AURA_MOD_CONFUSE))
            {
                if (Trinket)
                {
                    DoCast(me, SPELL_PVP_TRINKET, true);
                    Trinket = false;
                    m_uiPvpTrinketTimer = 120*IN_MILLISECONDS;
                }
            }

            if (m_uiPvpTrinketTimer)
            {
                if (m_uiPvpTrinketTimer <= uiDiff)
                {
                    Trinket = true;
                    m_uiPvpTrinketTimer = 0;
                }
                else m_uiPvpTrinketTimer -= uiDiff;
            }

            if (me->HasAura(SPELL_BUBBLE))
            {
                SelfHeal(uiDiff);
            }

            if(HealthBelowPct(50))
                SelfHeal(uiDiff);
            else
                PartyHeal(uiDiff);

            boss_faction_championsAI::UpdateAI(uiDiff);
        }
    };

};

enum ePriestSpells
{
    SPELL_PENANCE           = 66098,
    SPELL_RENEW             = 66177,
    SPELL_SHIELD            = 66099,
    SPELL_FLASH_HEAL        = 66104,
    SPELL_DISPEL            = 65546,
    SPELL_PSYCHIC_SCREAM    = 65543, //cd 30sec
    SPELL_MANA_BURN         = 66100, //cd 10sec
};

class mob_toc_priest : public CreatureScript
{
public:
    mob_toc_priest() : CreatureScript("mob_toc_priest") { }

    CreatureAI* GetAI(Creature *pCreature) const
    {
        return new mob_toc_priestAI (pCreature);
    }

    struct mob_toc_priestAI : public boss_faction_championsAI
    {
        mob_toc_priestAI(Creature *pCreature) : boss_faction_championsAI(pCreature, AI_HEALER) {}

        uint32 m_uiPsychicScreamTimer;
        uint32 m_uiDispelTimer;
        uint32 m_uiFlashHealTimer;
        uint32 m_uiCommonTimer;
        uint32 m_uiRenewTimer;
        uint32 m_uiShieldTimer;
        uint32 m_uiManaBurnTimer;
        uint32 m_uiPenanceTimer;
        uint32 m_uiPvpTrinketTimer;
        bool Trinket;


        void Reset()
        {
            boss_faction_championsAI::Reset();
            m_uiPenanceTimer = 4*IN_MILLISECONDS;
            m_uiManaBurnTimer = 13*IN_MILLISECONDS;
            m_uiDispelTimer = 5*IN_MILLISECONDS;
            m_uiFlashHealTimer = 6*IN_MILLISECONDS;
            m_uiShieldTimer = 2*IN_MILLISECONDS;
            m_uiRenewTimer = 8*IN_MILLISECONDS;
            m_uiPsychicScreamTimer = 30*IN_MILLISECONDS;
            m_uiPvpTrinketTimer = 0;
            Trinket = true;
            SetEquipmentSlots(false, 49992, EQUIP_NO_CHANGE, EQUIP_NO_CHANGE);
        }
        
        void EnterCombat(Unit* /*who*/)
        {
            DoCast(me, SPELL_SHIELD, true);
        }

        void SelfHeal(const uint32 uiDiff)
        {
           if (m_uiPenanceTimer <= uiDiff)
           {
               if (HealthBelowPct(50))
                   DoCast(me, SPELL_PENANCE);
               m_uiPenanceTimer = 4*IN_MILLISECONDS;
           } else m_uiPenanceTimer -= uiDiff;

           if (m_uiFlashHealTimer <= uiDiff)
           {
               if (HealthBelowPct(50))
                   DoCast(me, SPELL_FLASH_HEAL);
               m_uiFlashHealTimer = 6*IN_MILLISECONDS;
           } else m_uiFlashHealTimer -= uiDiff;

           if (m_uiRenewTimer <= uiDiff)
           {
               if (HealthBelowPct(50))
                   DoCast(me, SPELL_RENEW);
               m_uiRenewTimer = 8*IN_MILLISECONDS;
           } else m_uiRenewTimer -= uiDiff;
        }

        void PartyHeal(const uint32 uiDiff)
        {
           if (m_uiPenanceTimer <= uiDiff)
            {
                if (Unit* target = DoSelectLowestHpFriendly(40.0f))
                    if (target->HealthBelowPct(95))
                        DoCast(target, SPELL_PENANCE);
                m_uiPenanceTimer = 4*IN_MILLISECONDS;
            } else m_uiPenanceTimer -= uiDiff;

           if (m_uiFlashHealTimer <= uiDiff)
            {
                if (Unit* target = DoSelectLowestHpFriendly(40.0f))
                    if(target->HealthBelowPct(95))
                        DoCast(target, SPELL_FLASH_HEAL);
                m_uiFlashHealTimer = 6*IN_MILLISECONDS;
            } else m_uiFlashHealTimer -= uiDiff;
           
           if (m_uiRenewTimer <= uiDiff)
            {
                if (Unit* target = DoSelectLowestHpFriendly(40.0f))
                    if (target->HealthBelowPct(95))
                        DoCast(target, SPELL_RENEW);
                m_uiRenewTimer = 8*IN_MILLISECONDS;
            } else m_uiRenewTimer -= uiDiff;
        }
        
        void UpdateAI(uint32 uiDiff)
        {
            if (!UpdateVictim()) return;

            if (m_uiShieldTimer <= uiDiff)
            {
                if (me->HasAura(SPELL_SHIELD))
                {
                    if (Unit* target = DoSelectLowestHpFriendly(40.0f))
                        if (target->HealthBelowPct(95))
                            DoCast(target, SPELL_SHIELD, true);
                    m_uiShieldTimer = 2*IN_MILLISECONDS;
                } 
                else
                    DoCast(me, SPELL_SHIELD, true);
                m_uiShieldTimer = 2*IN_MILLISECONDS;
            } else m_uiShieldTimer -= uiDiff;
            
            if (m_uiPsychicScreamTimer <= uiDiff)
            {
                if (EnemiesInRange(10.0f) > 2)
                    DoCastAOE(SPELL_PSYCHIC_SCREAM);
                m_uiPsychicScreamTimer = 30*IN_MILLISECONDS;
            } else m_uiPsychicScreamTimer -= uiDiff;

            if (m_uiManaBurnTimer <= uiDiff)
            {
                if (Unit* target = SelectEnemyCaster(false))
                    DoCast(target, SPELL_MANA_BURN);
                m_uiManaBurnTimer = 13*IN_MILLISECONDS;
            } else m_uiManaBurnTimer -= uiDiff;

            if (me->HasAuraType(SPELL_AURA_MOD_STUN) || me->HasAuraType(SPELL_AURA_MOD_FEAR) || me->HasAuraType(SPELL_AURA_MOD_CHARM) || me->HasAuraType(SPELL_AURA_MOD_ROOT) || me->HasAuraType(SPELL_AURA_MOD_CONFUSE))
            {
                if (Trinket)
                {
                    DoCast(me, SPELL_PVP_TRINKET, true);
                    Trinket = false;
                    m_uiPvpTrinketTimer = 120*IN_MILLISECONDS;
                }
            }

            if (m_uiPvpTrinketTimer)
            {
                if (m_uiPvpTrinketTimer <= uiDiff)
                {
                    Trinket = true;
                    m_uiPvpTrinketTimer = 0;
                }
                else m_uiPvpTrinketTimer -= uiDiff;
            }

            if (HealthBelowPct(50))
                SelfHeal(uiDiff);
            else
                PartyHeal(uiDiff);
            
            boss_faction_championsAI::UpdateAI(uiDiff);
        }
    };

};

/********************************************************************
                            RANGED
********************************************************************/
enum eShadowPriestSpells
{
    SPELL_SILENCE           = 65542, //cd 45sec
    SPELL_VAMPIRIC_TOUCH    = 65490,
    SPELL_SW_PAIN           = 65541,
    SPELL_MIND_FLAY         = 65488,
    SPELL_MIND_BLAST        = 65492, //cd 8sec
    SPELL_HORROR            = 65545, //cd 2min
    SPELL_DISPERSION        = 65544, //cd 3min
    SPELL_SHADOWFORM        = 16592,
};

class mob_toc_shadow_priest : public CreatureScript
{
public:
    mob_toc_shadow_priest() : CreatureScript("mob_toc_shadow_priest") { }

    CreatureAI* GetAI(Creature *pCreature) const
    {
        return new mob_toc_shadow_priestAI (pCreature);
    }

    struct mob_toc_shadow_priestAI : public boss_faction_championsAI
    {
        mob_toc_shadow_priestAI(Creature *pCreature) : boss_faction_championsAI(pCreature, AI_RANGED) {}

        uint32 m_uiPsychicScreamTimer;
        uint32 m_uiDispersionTimer;
        uint32 m_uiSilenceTimer;
        uint32 m_uiSwPainTimer;
        uint32 m_uiCommonTimer;
        uint32 m_uiHorrorTimer;
        uint32 m_uiPvpTrinketTimer;
        bool Trinket;
        bool Dispers;


        void Reset()
        {
            boss_faction_championsAI::Reset();
            Trinket = true;
            Dispers = true;
            m_uiPvpTrinketTimer = 0;
            m_uiHorrorTimer = 120*IN_MILLISECONDS;
            m_uiCommonTimer = 4*IN_MILLISECONDS;
            m_uiSwPainTimer = 7*IN_MILLISECONDS;
            m_uiPsychicScreamTimer = urand(10*IN_MILLISECONDS, 15*IN_MILLISECONDS);
            m_uiDispersionTimer = 0;
            m_uiSilenceTimer = 45*IN_MILLISECONDS;
            SetEquipmentSlots(false, 50040, EQUIP_NO_CHANGE, EQUIP_NO_CHANGE);
            DoCast(me, SPELL_SHADOWFORM);
        }

        void EnterCombat(Unit *pWho)
        {
            boss_faction_championsAI::EnterCombat(pWho);
        }

        void UpdateAI(uint32 uiDiff)
        {
            if (!UpdateVictim()) return;

            if (m_uiDispersionTimer)
            { 
                if (m_uiDispersionTimer <= uiDiff)
                {
                    Dispers = true;
                    m_uiDispersionTimer = 0;
                } else m_uiDispersionTimer -= uiDiff;
            } 

            if (HealthBelowPct(30) && Dispers)
            {
                DoCast(me, SPELL_DISPERSION);
                Dispers = false;
                m_uiDispersionTimer = 180*IN_MILLISECONDS;
            }

            if (m_uiPsychicScreamTimer <= uiDiff)
            {
                if (EnemiesInRange(10.0f) >= 1)
                    DoCastAOE(SPELL_PSYCHIC_SCREAM);
                m_uiPsychicScreamTimer = urand(10*IN_MILLISECONDS, 15*IN_MILLISECONDS);
            } else m_uiPsychicScreamTimer -= uiDiff;

            if (m_uiHorrorTimer <= uiDiff)
            {
                if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0))
                    DoCast(target, SPELL_HORROR);
                m_uiHorrorTimer = 120*IN_MILLISECONDS;
            } else m_uiHorrorTimer -= uiDiff;

            if (m_uiSwPainTimer <= uiDiff)
            {
                if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0))
                    if (!target->HasAura(SPELL_SW_PAIN))
                         DoCast(target, SPELL_SW_PAIN);
                         m_uiSwPainTimer = 7*IN_MILLISECONDS;
            } else m_uiSwPainTimer -= uiDiff;

            if (m_uiSilenceTimer <= uiDiff)
            {
                if (Unit* target = SelectEnemyCaster(false))
                    DoCast(target, SPELL_SILENCE);
                m_uiSilenceTimer = 45*IN_MILLISECONDS;
            } else m_uiSilenceTimer -= uiDiff;

            
            if (m_uiCommonTimer <= uiDiff)
            {
                switch(urand(0, 2))
                {
                case 0:
                    DoCastVictim(SPELL_MIND_BLAST);
                    break;
                case 1:
                    DoCastVictim(SPELL_MIND_FLAY);
                    break;
                case 2:
                    {
                    if (Unit * target = SelectTarget(SELECT_TARGET_RANDOM, 0))
                        if (!target->HasAura(SPELL_VAMPIRIC_TOUCH))
                            DoCast(target, SPELL_VAMPIRIC_TOUCH);
                        else
                            DoCast(target, SPELL_MIND_BLAST);
                    }
                    break;
                }
                m_uiCommonTimer = 4*IN_MILLISECONDS;
            } else m_uiCommonTimer -= uiDiff;

            if (me->HasAuraType(SPELL_AURA_MOD_STUN) || me->HasAuraType(SPELL_AURA_MOD_FEAR) || me->HasAuraType(SPELL_AURA_MOD_CHARM) || me->HasAuraType(SPELL_AURA_MOD_ROOT) || me->HasAuraType(SPELL_AURA_MOD_CONFUSE))
            {
                if (Trinket)
                {
                    DoCast(me, SPELL_PVP_TRINKET, true);
                    Trinket = false;
                    m_uiPvpTrinketTimer = 120*IN_MILLISECONDS;
                }
            }

            if (m_uiPvpTrinketTimer)
            {
                if (m_uiPvpTrinketTimer <= uiDiff)
                {
                    Trinket = true;
                    m_uiPvpTrinketTimer = 0;
                }
                else m_uiPvpTrinketTimer -= uiDiff;
            }


            boss_faction_championsAI::UpdateAI(uiDiff);
        }
    };

};

enum eWarlockSpells
{
    SPELL_HELLFIRE              = 65816,
    SPELL_CORRUPTION            = 65810,
    SPELL_CURSE_OF_AGONY        = 65814,
    SPELL_FEAR                  = 65809, //cd 8sec 
    SPELL_SEARING_PAIN          = 65819,
    SPELL_SHADOW_BOLT           = 65821,
    SPELL_UNSTABLE_AFFLICTION   = 65812, //cd8sec
    SPELL_SUMMON_FELHUNTER      = 67514,
    H_SPELL_UNSTABLE_AFFLICTION = 68155, 
};

class mob_toc_warlock : public CreatureScript
{
public:
    mob_toc_warlock() : CreatureScript("mob_toc_warlock") { }

    CreatureAI* GetAI(Creature *pCreature) const
    {
        return new mob_toc_warlockAI (pCreature);
    }

    struct mob_toc_warlockAI : public boss_faction_championsAI
    {
        mob_toc_warlockAI(Creature *pCreature) : boss_faction_championsAI(pCreature, AI_RANGED), Summons(me) {}

        SummonList Summons;

        uint32 m_uiFearTimer;
        uint32 m_uiHellfireTimer;
        uint32 m_uiSummonPetTimer;
        uint32 m_uiCorruptionTimer;
        uint32 m_uiCurseOfAgonyTimer;
        uint32 m_uiCommonTimer;
        uint32 m_uiPvpTrinketTimer;
        bool Trinket;
        bool Hellfire;

        void Reset()
        {
            boss_faction_championsAI::Reset();
            Hellfire = true;
            Trinket = true;
            m_uiPvpTrinketTimer = 0;
            m_uiCorruptionTimer = 3*IN_MILLISECONDS;
            m_uiCommonTimer = 4*IN_MILLISECONDS;
            m_uiCurseOfAgonyTimer = 6*IN_MILLISECONDS;
            m_uiFearTimer = 8*IN_MILLISECONDS;
            m_uiSummonPetTimer = urand(15*IN_MILLISECONDS, 30*IN_MILLISECONDS);
            m_uiHellfireTimer = 0;
            SetEquipmentSlots(false, 49992, EQUIP_NO_CHANGE, EQUIP_NO_CHANGE);
            DoCast(SPELL_SUMMON_FELHUNTER);
        }

        void UpdateAI(uint32 uiDiff)
        {
            if (!UpdateVictim()) return;

            if (m_uiHellfireTimer)
            {
                if (m_uiHellfireTimer <= uiDiff)
                {
                    Hellfire = true;
                    m_uiHellfireTimer = 0;
                } else m_uiHellfireTimer -= uiDiff;
            }

            if (Hellfire && EnemiesInRange(10.0f) >= 3)
            {
                DoCastAOE(SPELL_HELLFIRE, true);
                Hellfire = false;
                m_uiHellfireTimer = 60*IN_MILLISECONDS;
            }
            
            if (m_uiFearTimer <= uiDiff)
            {
                if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0))
                    DoCast(target, SPELL_FEAR);
                m_uiFearTimer = 8*IN_MILLISECONDS;
            } else m_uiFearTimer -= uiDiff;

            if (m_uiCurseOfAgonyTimer <= uiDiff)
            {
                if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0))
                    if (!target->HasAura(SPELL_CURSE_OF_AGONY))
                        DoCast(target, SPELL_CURSE_OF_AGONY);
                m_uiCurseOfAgonyTimer = 6*IN_MILLISECONDS;
            } else m_uiCurseOfAgonyTimer -= uiDiff;  

            if (m_uiCorruptionTimer <= uiDiff)
            {
                if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0))
                    if (!target->HasAura(SPELL_CORRUPTION))
                        DoCast(target, SPELL_CORRUPTION);
                m_uiCorruptionTimer = 3*IN_MILLISECONDS;
            } else m_uiCorruptionTimer -= uiDiff;
                        
            if (m_uiCommonTimer <= uiDiff)
            {
                switch (urand(0, 2))
                {
                case 0:
                    DoCastVictim(SPELL_SHADOW_BOLT);
                    break;
                case 1:
                    {
                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0))
                        if (!target->HasAura(SPELL_UNSTABLE_AFFLICTION))
                            DoCast(target, SPELL_UNSTABLE_AFFLICTION);
                        else
                            DoCast(target, SPELL_SHADOW_BOLT);
                    }
                    break;
                case 2:
                    DoCastVictim(SPELL_SEARING_PAIN);
                    break;
                }
                m_uiCommonTimer = 4*IN_MILLISECONDS;
            } else m_uiCommonTimer -= uiDiff;
            
            if (me->HasAuraType(SPELL_AURA_MOD_STUN) || me->HasAuraType(SPELL_AURA_MOD_FEAR) || me->HasAuraType(SPELL_AURA_MOD_CHARM) || me->HasAuraType(SPELL_AURA_MOD_ROOT) || me->HasAuraType(SPELL_AURA_MOD_CONFUSE))
            {
                if (Trinket)
                {
                    DoCast(me, SPELL_PVP_TRINKET, true);
                    Trinket = false;
                    m_uiPvpTrinketTimer = 120*IN_MILLISECONDS;
                }
            }

            if (m_uiPvpTrinketTimer)
            {
                if (m_uiPvpTrinketTimer <= uiDiff)
                {
                    Trinket = true;
                    m_uiPvpTrinketTimer = 0;
                }
                else m_uiPvpTrinketTimer -= uiDiff;
            }

            boss_faction_championsAI::UpdateAI(uiDiff);
        }
    };

};

enum eMageSpells
{
    SPELL_ARCANE_BARRAGE    = 65799, //cd 3sec
    SPELL_ARCANE_BLAST      = 65791,
    SPELL_ARCANE_EXPLOSION  = 65800,
    SPELL_BLINK             = 65793, //cd 15sec
    SPELL_COUNTERSPELL      = 65790, //cd 24sec
    SPELL_FROST_NOVA        = 65792, //cd 25sec
    SPELL_FROSTBOLT         = 65807,
    SPELL_ICE_BLOCK         = 65802, //cd 5min
    SPELL_POLYMORPH         = 65801, //cd 15sec
};

class mob_toc_mage : public CreatureScript
{
public:
    mob_toc_mage() : CreatureScript("mob_toc_mage") { }

    CreatureAI* GetAI(Creature *pCreature) const
    {
        return new mob_toc_mageAI (pCreature);
    }

    struct mob_toc_mageAI : public boss_faction_championsAI
    {
        mob_toc_mageAI(Creature *pCreature) : boss_faction_championsAI(pCreature, AI_RANGED) {}

        uint32 m_uiCounterspellTimer;
        uint32 m_uiBlinkTimer;
        uint32 m_uiIceBlockTimer;
        uint32 m_uiPolymorphTimer;
        uint32 m_uiFrostNovaTimer;
        uint32 m_uiFrostNovaTimerVirt;
        uint32 m_ArcaneExplosionTimer;
        uint32 m_uiCommonTimer;
        uint32 m_uiPvpTrinketTimer;
        bool Trinket;
        bool IceBlock;

        void Reset()
        {
            boss_faction_championsAI::Reset();
            Trinket = true;
            IceBlock = true;
            m_uiPvpTrinketTimer = 0;
            m_uiCommonTimer = 4*IN_MILLISECONDS;
            m_ArcaneExplosionTimer = urand(12*IN_MILLISECONDS, 14*IN_MILLISECONDS);
            m_uiFrostNovaTimer = 25*IN_MILLISECONDS;
            m_uiCounterspellTimer = urand(5*IN_MILLISECONDS, 15*IN_MILLISECONDS);
            m_uiBlinkTimer = 0;
            m_uiIceBlockTimer = 0;
            m_uiFrostNovaTimerVirt = 0;
            m_uiPolymorphTimer = 15*IN_MILLISECONDS;
            SetEquipmentSlots(false, 47524, EQUIP_NO_CHANGE, EQUIP_NO_CHANGE);
        }

        void UpdateAI(uint32 uiDiff)
        {
            if (!UpdateVictim()) return;

            if (m_uiCounterspellTimer <= uiDiff)
            {
                if (Unit* target = SelectEnemyCaster(false))
                    DoCast(target, SPELL_COUNTERSPELL);
                m_uiCounterspellTimer = urand(5*IN_MILLISECONDS, 15*IN_MILLISECONDS);
            } else m_uiCounterspellTimer -= uiDiff;

            if (m_uiBlinkTimer)
            {
                if (m_uiBlinkTimer <= uiDiff)
                {
                    DoCast(me, SPELL_BLINK);
                    m_uiBlinkTimer = 0;
                }
            } else m_uiBlinkTimer -= uiDiff;

            if (m_uiIceBlockTimer)
            {
                if (m_uiIceBlockTimer  <= uiDiff)
                {
                   IceBlock = true;
                   m_uiIceBlockTimer = 0;
                } else m_uiIceBlockTimer -= uiDiff;
            }

            if (HealthBelowPct(50) && IceBlock)
            {
                DoCast(me, SPELL_ICE_BLOCK);
                IceBlock = false;
                m_uiIceBlockTimer = 300*IN_MILLISECONDS;
                m_uiFrostNovaTimerVirt = 1*IN_MILLISECONDS;
                m_uiBlinkTimer = 1.5*IN_MILLISECONDS;
            }

            if (me->HasAura(SPELL_ICE_BLOCK))
                return;

            if (m_uiCounterspellTimer <= uiDiff)
            {
                if (Unit* target = SelectEnemyCaster(false))
                    DoCast(target, SPELL_COUNTERSPELL);
                m_uiCounterspellTimer = urand(5*IN_MILLISECONDS, 15*IN_MILLISECONDS);
            } else m_uiCounterspellTimer -= uiDiff;

            if (m_uiFrostNovaTimer <= uiDiff)
            {
                if(EnemiesInRange(10.0f) >= 2)
                {   
                    DoCastAOE(SPELL_FROST_NOVA);
                    m_uiBlinkTimer = 25*IN_MILLISECONDS;
                }
                m_uiFrostNovaTimer = 3*IN_MILLISECONDS;
            } else m_uiFrostNovaTimer -= uiDiff;

            if (m_uiFrostNovaTimerVirt)
            {
                if (m_uiFrostNovaTimerVirt <= uiDiff)
                {
                    DoCastAOE(SPELL_FROST_NOVA);
                    m_uiFrostNovaTimerVirt = 0;
                }
                else m_uiFrostNovaTimerVirt -= uiDiff;
            }

            if (m_ArcaneExplosionTimer <=uiDiff)
            {
                if (EnemiesInRange(10.0f) > 3)
                    DoCastAOE(SPELL_ARCANE_EXPLOSION);
                m_ArcaneExplosionTimer = urand(12*IN_MILLISECONDS, 14*IN_MILLISECONDS);
            } else m_ArcaneExplosionTimer -= uiDiff;

            if (m_uiPolymorphTimer <= uiDiff)
            {
                if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0))
                    DoCast(target, SPELL_POLYMORPH);
                m_uiPolymorphTimer = 15*IN_MILLISECONDS;
            } else m_uiPolymorphTimer -= uiDiff;
            
            if (m_uiCommonTimer <= uiDiff)
            {
                switch (urand(0, 2))
                {
                case 0: 
                    DoCastVictim(SPELL_ARCANE_BARRAGE);
                    break;
                case 1:
                    DoCastVictim(SPELL_ARCANE_BLAST);
                    break;
                case 2:
                    DoCastVictim(SPELL_FROSTBOLT);
                    break;
                }
                m_uiCommonTimer = 4*IN_MILLISECONDS;
            } else m_uiCommonTimer -= uiDiff;
            
            if (me->HasAuraType(SPELL_AURA_MOD_STUN) || me->HasAuraType(SPELL_AURA_MOD_FEAR) || me->HasAuraType(SPELL_AURA_MOD_CHARM) || me->HasAuraType(SPELL_AURA_MOD_ROOT) || me->HasAuraType(SPELL_AURA_MOD_CONFUSE))
            {
                if (Trinket)
                {
                    DoCast(me, SPELL_PVP_TRINKET, true);
                    Trinket = false;
                    m_uiPvpTrinketTimer = 120*IN_MILLISECONDS;
                }
            }

            if (m_uiPvpTrinketTimer)
            {
                if (m_uiPvpTrinketTimer <= uiDiff)
                {
                    Trinket = true;
                    m_uiPvpTrinketTimer = 0;
                }
                else m_uiPvpTrinketTimer -= uiDiff;
            }

            boss_faction_championsAI::UpdateAI(uiDiff);
        }
    };

};

enum eHunterSpells
{
    SPELL_AIMED_SHOT        = 65883,
    SPELL_DETERRENCE        = 65871, //cd 1.5min
    SPELL_DISENGAGE         = 65869, //cd 30sec
    SPELL_EXPLOSIVE_SHOT    = 65866,
    SPELL_FROST_TRAP        = 65880, 
    SPELL_SHOOT             = 65868, 
    SPELL_STEADY_SHOT       = 65867, //cd 3sec
    SPELL_WING_CLIP         = 66207, //cd 6sec
    SPELL_WYVERN_STING      = 65877, 
    SPELL_CALL_PET          = 67777,
};

class mob_toc_hunter : public CreatureScript
{
public:
    mob_toc_hunter() : CreatureScript("mob_toc_hunter") { }

    CreatureAI* GetAI(Creature *pCreature) const
    {
        return new mob_toc_hunterAI (pCreature);
    }

    struct mob_toc_hunterAI : public boss_faction_championsAI
    {
        mob_toc_hunterAI(Creature *pCreature) : boss_faction_championsAI(pCreature, AI_RANGED), Summons(me) {}

        SummonList Summons;

        uint32 m_uiDisengageTimer;
        uint32 m_uiDeterrenceTimer;
        uint32 m_uiWyvernStingTimer;
        uint32 m_uiFrostTrapTimer;
        uint32 m_uiWingClipTimer;
        uint32 m_uiCommonTimer;
        uint32 m_uiSummonPetTimer;
        uint32 m_uiPvpTrinketTimer;
        bool Trinket;
        bool Deterrence;


        void Reset()
        {
            boss_faction_championsAI::Reset();
            me->SetBaseWeaponDamage(RANGED_ATTACK, MINDAMAGE, 2500);
            me->SetBaseWeaponDamage(RANGED_ATTACK, MAXDAMAGE, 3000);
            Trinket = true;
            Deterrence = true;
            m_uiPvpTrinketTimer = 0;
            m_uiDisengageTimer = 30*IN_MILLISECONDS;
            m_uiDeterrenceTimer = 0;
            m_uiWyvernStingTimer = urand(7*IN_MILLISECONDS, 60*IN_MILLISECONDS);
            m_uiFrostTrapTimer = 1*IN_MILLISECONDS;
            m_uiWingClipTimer = 6*IN_MILLISECONDS;
            m_uiCommonTimer = 2*IN_MILLISECONDS;
            DoCast(SPELL_CALL_PET);
            SetEquipmentSlots(false, 47156, EQUIP_NO_CHANGE, 48711);

            m_uiSummonPetTimer = urand(15*IN_MILLISECONDS, 30*IN_MILLISECONDS);
            DoCast(SPELL_CALL_PET);
        }

        void MeleeCombat(const uint32 uiDiff)
        {
        
            if ( Deterrence && EnemiesInRange(8.0f) >= 1)
            {
                DoCast(SPELL_DETERRENCE);
                Deterrence = false;
                m_uiDeterrenceTimer = 90*IN_MILLISECONDS;
            } 

            if (m_uiFrostTrapTimer <= uiDiff)
            {
                if (EnemiesInRange(8.0f) >= 1)
                DoCast(SPELL_FROST_TRAP);
                m_uiFrostTrapTimer = 60*IN_MILLISECONDS;
            } else m_uiFrostTrapTimer -= uiDiff;

           if (m_uiWingClipTimer <= uiDiff)
            {
                if (EnemiesInRange(5.0f) >= 1)
                    DoCastVictim(SPELL_WING_CLIP);
                m_uiWingClipTimer = 6*IN_MILLISECONDS;
            } else m_uiWingClipTimer -= uiDiff;

           if (m_uiDisengageTimer <= uiDiff)
           {
               if (EnemiesInRange(8.0f) >= 1)
                   DoCast(SPELL_DISENGAGE);
               m_uiDisengageTimer = 30*IN_MILLISECONDS;
           } else m_uiDisengageTimer -= uiDiff;

        }

        void RangeCombat(const uint32 uiDiff)
        {
           if (m_uiCommonTimer <= uiDiff)
            {
                if (me->getVictim()->HasAura(SPELL_WYVERN_STING))
                    return;

                switch (urand(0, 3))

                {
                    case 0:
                        DoCastVictim(SPELL_WYVERN_STING);
                        break;
                    case 1:
                        DoCastVictim(SPELL_SHOOT);
                        break;
                    case 2:
                        DoCastVictim(SPELL_EXPLOSIVE_SHOT);
                        break;
                    case 3:
                        DoCastVictim(SPELL_AIMED_SHOT);
                        break;
                }
                m_uiCommonTimer = 2*IN_MILLISECONDS;
            } else m_uiCommonTimer -= uiDiff;
        }
        
        void UpdateAI(uint32 uiDiff)
        {
			if (!UpdateVictim()) return;

            if (m_uiDeterrenceTimer)
            {
                if (m_uiDeterrenceTimer <= uiDiff)
                {
                    Deterrence = true;
                m_uiDeterrenceTimer = 0;
                } else m_uiDeterrenceTimer -= uiDiff;
            }

            if (me->HasAuraType(SPELL_AURA_MOD_STUN) || me->HasAuraType(SPELL_AURA_MOD_FEAR) || me->HasAuraType(SPELL_AURA_MOD_CHARM) || me->HasAuraType(SPELL_AURA_MOD_ROOT) || me->HasAuraType(SPELL_AURA_MOD_CONFUSE))
            {
                if (Trinket)
                {
                    DoCast(me, SPELL_PVP_TRINKET, true);
                    Trinket = false;
                    m_uiPvpTrinketTimer = 120*IN_MILLISECONDS;
                }
            }

            if (m_uiPvpTrinketTimer)
            {
                if (m_uiPvpTrinketTimer <= uiDiff)
                {
                    Trinket = true;
                    m_uiPvpTrinketTimer = 0;
                }
                else m_uiPvpTrinketTimer -= uiDiff;
            }

           if (EnemiesInRange(8.0f) >= 1)
               MeleeCombat(uiDiff);
           else
               RangeCombat(uiDiff);

            boss_faction_championsAI::UpdateAI(uiDiff);
        }
    };

};

enum eBoomkinSpells
{
    SPELL_CYCLONE           = 65859, //cd 6sec
    SPELL_ENTANGLING_ROOTS  = 65857, //cd 10sec
    SPELL_FAERIE_FIRE       = 65863,
    SPELL_FORCE_OF_NATURE   = 65861, //cd 3min
    SPELL_INSECT_SWARM      = 65855,
    SPELL_MOONFIRE          = 65856, //cd 10sec
    SPELL_STARFIRE          = 65854,
    SPELL_WRATH             = 65862,
};

class mob_toc_boomkin : public CreatureScript
{
public:
    mob_toc_boomkin() : CreatureScript("mob_toc_boomkin") { }

    CreatureAI* GetAI(Creature *pCreature) const
    {
        return new mob_toc_boomkinAI (pCreature);
    }

    struct mob_toc_boomkinAI : public boss_faction_championsAI
    {
        mob_toc_boomkinAI(Creature *pCreature) : boss_faction_championsAI(pCreature, AI_RANGED) {}

        uint32 m_uiBarkskinTimer;
        uint32 m_uiCycloneTimer;
        uint32 m_uiEntanglingRootsTimer;
        uint32 m_uiFaerieFireTimer;
        uint32 m_uiCommonTimer;
        uint32 m_uiForceOfNatureTimer;
        uint32 m_uiPvpTrinketTimer;
        bool Trinket;
        bool Barkskin;

        void Reset()
        {
            boss_faction_championsAI::Reset();
            Trinket = true;
            Barkskin = true;
            m_uiBarkskinTimer = 0;
            m_uiPvpTrinketTimer = 0;
            m_uiCycloneTimer = urand(10*IN_MILLISECONDS, 40*IN_MILLISECONDS);
            m_uiEntanglingRootsTimer = urand(5*IN_MILLISECONDS, 40*IN_MILLISECONDS);
            m_uiFaerieFireTimer = urand(10*IN_MILLISECONDS, 20*IN_MILLISECONDS);
            m_uiForceOfNatureTimer = 300*IN_MILLISECONDS;
            m_uiCommonTimer = 3*IN_MILLISECONDS;
            SetEquipmentSlots(false, 50966, EQUIP_NO_CHANGE, EQUIP_NO_CHANGE);
        }

        void EnterCombat(Unit* /*who*/)
        {
            DoCastVictim(SPELL_FAERIE_FIRE);
            DoCastVictim(SPELL_INSECT_SWARM);
        }

        void UpdateAI(uint32 uiDiff)
        {
            if (!UpdateVictim()) return;
            
            if (m_uiBarkskinTimer)
            {
                if (m_uiBarkskinTimer <= uiDiff)
                {
                    Barkskin = true;
                    m_uiBarkskinTimer = 0;
                } else m_uiBarkskinTimer -= uiDiff;
            }

            if (Barkskin && EnemiesInRange(8.0f) >= 1)
            {
                DoCast(me, SPELL_BARKSKIN);
                Barkskin = false;
                m_uiBarkskinTimer = 30*IN_MILLISECONDS;
            }

            if (m_uiFaerieFireTimer <= uiDiff)
            {
                if (!me->getVictim()->HasAura(SPELL_FAERIE_FIRE))
                    DoCastVictim(SPELL_FAERIE_FIRE);
                m_uiFaerieFireTimer = urand(10*IN_MILLISECONDS, 20*IN_MILLISECONDS);
            } else m_uiFaerieFireTimer -= uiDiff;
                 
            
            if (m_uiCycloneTimer <= uiDiff)
            {
                if (Unit* target = SelectEnemyCaster(false))
                    DoCast(target, SPELL_CYCLONE);
                m_uiCycloneTimer = urand(10*IN_MILLISECONDS, 15*IN_MILLISECONDS);
            } else m_uiCycloneTimer -= uiDiff;

            if (m_uiEntanglingRootsTimer <= uiDiff)
            {
                if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0))
                    DoCast(target, SPELL_ENTANGLING_ROOTS);
                m_uiEntanglingRootsTimer = urand(5*IN_MILLISECONDS, 40*IN_MILLISECONDS);
            } else m_uiEntanglingRootsTimer -= uiDiff;

            if (m_uiForceOfNatureTimer <= uiDiff)
            {
                DoCastVictim(SPELL_FORCE_OF_NATURE);
                m_uiForceOfNatureTimer = 300*IN_MILLISECONDS;
            } else m_uiForceOfNatureTimer -= uiDiff;

            if (m_uiCommonTimer <= uiDiff)
            {
                switch (urand(0, 3))
                {
                    case 0: 
                        DoCastVictim(SPELL_MOONFIRE, true);
                        break;
                    case 1:
                        {
                            if (!me->getVictim()->HasAura(SPELL_INSECT_SWARM))
                                DoCastVictim(SPELL_INSECT_SWARM, true);
                            else 
                                DoCastVictim(SPELL_STARFIRE, true);
                        }
                        break;
                    case 2:
                        DoCastVictim(SPELL_STARFIRE, true);
                        break;
                    case 3: 
                        DoCastVictim(SPELL_WRATH, true);
                        break;
                }
                m_uiCommonTimer = 3*IN_MILLISECONDS;
            } else m_uiCommonTimer -= uiDiff;

            if (me->HasAuraType(SPELL_AURA_MOD_STUN) || me->HasAuraType(SPELL_AURA_MOD_FEAR) || me->HasAuraType(SPELL_AURA_MOD_CHARM) || me->HasAuraType(SPELL_AURA_MOD_ROOT) || me->HasAuraType(SPELL_AURA_MOD_CONFUSE))
            {
                if (Trinket)
                {
                    DoCast(me, SPELL_PVP_TRINKET, true);
                    Trinket = false;
                    m_uiPvpTrinketTimer = 120*IN_MILLISECONDS;
                }
            }

            if (m_uiPvpTrinketTimer)
            {
                if (m_uiPvpTrinketTimer <= uiDiff)
                {
                    Trinket = true;
                    m_uiPvpTrinketTimer = 0;
                }
                else m_uiPvpTrinketTimer -= uiDiff;
            }

            boss_faction_championsAI::UpdateAI(uiDiff);
        }
    };

};

/********************************************************************
                            MELEE
********************************************************************/
enum eWarriorSpells
{
    SPELL_REND                  = 59691,
    SPELL_HAMSTRING             = 62845,
    SPELL_BLADESTORM            = 65947, //cd 1.5min
    SPELL_INTIMIDATING_SHOUT    = 65930, //cd 2min
    SPELL_MORTAL_STRIKE         = 65926, //cd 6sec
    SPELL_CHARGE                = 68764, //cd 15sec
    SPELL_DISARM                = 65935, //cd 1min
    SPELL_OVERPOWER             = 65924,
    SPELL_SUNDER_ARMOR          = 65936,
    SPELL_RETALIATION           = 65932,
};

class mob_toc_warrior : public CreatureScript
{
public:
    mob_toc_warrior() : CreatureScript("mob_toc_warrior") { }

    CreatureAI* GetAI(Creature *pCreature) const
    {
        return new mob_toc_warriorAI (pCreature);
    }

    struct mob_toc_warriorAI : public boss_faction_championsAI
    {
        mob_toc_warriorAI(Creature *pCreature) : boss_faction_championsAI(pCreature, AI_MELEE)
        {
            me->setPowerType(POWER_RAGE);
            me->SetPower(POWER_RAGE,0);
        }

        uint32 m_uiBladestormTimer;
        uint32 m_uiIntimidatingShoutTimer;
        uint32 m_uiChargeTimer;
        uint32 m_uiRetaliationTimer;
        uint32 m_uiDisarmTimer;
        uint32 m_uiHamstringTimer;
        uint32 m_uiCommonTimer;
        uint32 m_uiPvpTrinketTimer;
        bool Trinket;

        void Reset()
        {
            boss_faction_championsAI::Reset();
            Trinket = true;
            m_uiPvpTrinketTimer = 0;
            m_uiHamstringTimer = 2*IN_MILLISECONDS;
            m_uiCommonTimer = 4*IN_MILLISECONDS;
            m_uiBladestormTimer = 10*IN_MILLISECONDS;
            m_uiIntimidatingShoutTimer = 16*IN_MILLISECONDS;
            m_uiChargeTimer = 1*IN_MILLISECONDS;
            m_uiRetaliationTimer = 20*IN_MILLISECONDS;
            m_uiDisarmTimer = 13*IN_MILLISECONDS;
            SetEquipmentSlots(false, 31984, EQUIP_NO_CHANGE, EQUIP_NO_CHANGE);
        }

        void EnterCombat(Unit* /*who*/)
        {
            DoCastVictim(SPELL_CHARGE);
        }

        void Range(const uint32 uiDiff)
        {
            if (m_uiChargeTimer <= uiDiff)
            {
                if (me->IsInRange(me->getVictim(), 10.0f, 25.0f, false))
                    DoCastVictim(SPELL_CHARGE);
                m_uiChargeTimer = 15*IN_MILLISECONDS;
            } else m_uiChargeTimer -= uiDiff;

            if (m_uiHamstringTimer <= uiDiff)
            {
                DoCastVictim(SPELL_HAMSTRING, true);
                m_uiHamstringTimer = 2*IN_MILLISECONDS;
            } else m_uiHamstringTimer -= uiDiff;
        }
        
        void Melee(const uint32 uiDiff)
        {

            if (m_uiIntimidatingShoutTimer <= uiDiff)
            {
                if (EnemiesInRange(8.0f) >= 2)
                    DoCastVictim(SPELL_INTIMIDATING_SHOUT);
                m_uiIntimidatingShoutTimer = 60*IN_MILLISECONDS;
            } else m_uiIntimidatingShoutTimer -= uiDiff;

            if (m_uiBladestormTimer <= uiDiff)
            {
                if (EnemiesInRange(8.0f) >= 2)
                    DoCastVictim(SPELL_BLADESTORM);
                m_uiBladestormTimer = 90*IN_MILLISECONDS;
            } else m_uiBladestormTimer -= uiDiff;

            if (m_uiDisarmTimer <= uiDiff)
            {
                DoCastVictim(SPELL_DISARM);
                m_uiDisarmTimer = 60*IN_MILLISECONDS;
            } else m_uiDisarmTimer -= uiDiff;

            if (m_uiRetaliationTimer <= uiDiff)
            {
                if(me->IsInRange(me->getVictim(), 5.0f, 6.0f, false))
                    DoCast(me , SPELL_RETALIATION);
                m_uiRetaliationTimer = 20*IN_MILLISECONDS;
            } else m_uiRetaliationTimer -= uiDiff;
                  
            if (m_uiCommonTimer <= uiDiff)
            {
                switch(urand(0, 4))
                {
                case 0:
                    DoCastVictim(SPELL_MORTAL_STRIKE, true);
                    break;
                case 1:
                    DoCastVictim(SPELL_HAMSTRING, true);
                    break;
                case 2:
                    DoCastVictim(SPELL_OVERPOWER, true);
                    break;
                case 3:
                    DoCastVictim(SPELL_SUNDER_ARMOR, true);
                    break;
                case 4:
                    DoCastVictim(SPELL_REND);
                    break;
                } 
                m_uiCommonTimer = 4*IN_MILLISECONDS;
            } else m_uiCommonTimer -= uiDiff;
        }
            
        void UpdateAI(uint32 uiDiff)
        {
            if (!UpdateVictim()) return;
            
            if (me->HasAuraType(SPELL_AURA_MOD_STUN) || me->HasAuraType(SPELL_AURA_MOD_FEAR) || me->HasAuraType(SPELL_AURA_MOD_CHARM) || me->HasAuraType(SPELL_AURA_MOD_ROOT) || me->HasAuraType(SPELL_AURA_MOD_CONFUSE))
            {
                if (Trinket)
                {
                    DoCast(me, SPELL_PVP_TRINKET, true);
                    Trinket = false;
                    m_uiPvpTrinketTimer = 120*IN_MILLISECONDS;
                }
            }

            if (m_uiPvpTrinketTimer)
            {
                if (m_uiPvpTrinketTimer <= uiDiff)
                {
                    Trinket = true;
                    m_uiPvpTrinketTimer = 0;
                }
                else m_uiPvpTrinketTimer -= uiDiff;
            }

            if (me->IsInRange(me->getVictim(), 0.0f, 5.0f, false))
                Melee(uiDiff);
            else
                Range(uiDiff);

            boss_faction_championsAI::UpdateAI(uiDiff);
        }
    };

};

enum eDeathKnightSpells
{
    SPELL_DUAL_WIELD_DK       = 42459,
    SPELL_CHAINS_OF_ICE       = 66020, //cd 8sec
    SPELL_DEATH_COIL          = 66019, //cd 5sec
    SPELL_DEATH_GRIP          = 66017, //cd 35sec
    SPELL_FROST_STRIKE        = 66047, //cd 6sec
    SPELL_ICEBOUND_FORTITUDE  = 66023, //cd 1min
    SPELL_ICY_TOUCH           = 66021, //cd 8sec
    SPELL_STRANGULATE         = 66018, //cd 2min
};

class mob_toc_dk : public CreatureScript
{
public:
    mob_toc_dk() : CreatureScript("mob_toc_dk") { }

    CreatureAI* GetAI(Creature *pCreature) const
    {
        return new mob_toc_dkAI (pCreature);
    }

    struct mob_toc_dkAI : public boss_faction_championsAI
    {
        mob_toc_dkAI(Creature *pCreature) : boss_faction_championsAI(pCreature, AI_MELEE)
        {
            me->setPowerType(POWER_RUNES);
            me->SetPower(POWER_RUNES,0); 
            me->setPowerType(POWER_RUNIC_POWER);
            me->SetPower(POWER_RUNIC_POWER,1000);
            me->SetMaxPower(POWER_RUNIC_POWER,1000);
        }
        
        uint32 m_uiIceboundFortitudeTimer;
        uint32 m_uiChainsOfIceTimer;
        uint32 m_uiStrangulateTimer;
        uint32 m_uiDeathGripTimer;
        uint32 m_uiCommonTimer;
        uint32 m_uiPvpTrinketTimer;
        bool Trinket;
        bool Icebound;


        void Reset()
        {
            boss_faction_championsAI::Reset();
            Trinket = true;
            Icebound = true;
            me->SetBaseWeaponDamage(OFF_ATTACK, MINDAMAGE, 1061);
            me->SetBaseWeaponDamage(OFF_ATTACK, MAXDAMAGE, 1368);
            m_uiPvpTrinketTimer = 0;
            m_uiIceboundFortitudeTimer = 0;
            m_uiCommonTimer = 3*IN_MILLISECONDS;
            m_uiChainsOfIceTimer = 8*IN_MILLISECONDS;
            m_uiStrangulateTimer = 120*IN_MILLISECONDS;
            m_uiDeathGripTimer = 35*IN_MILLISECONDS,
            DoCast(me, SPELL_DUAL_WIELD_DK, true );
            SetEquipmentSlots(false, 35101, 35101, EQUIP_NO_CHANGE);
        }

        void SpellHitTarget(Unit * target, const SpellInfo * sp)
        {
            SpellRuneCostEntry const* runeCostData = sSpellRuneCostStore.LookupEntry(sp->RuneCostID);
            
            if (!runeCostData || (runeCostData->NoRuneCost() && runeCostData->NoRunicPowerGain()))
                return;

            if (int32 rp = int32(runeCostData->runePowerGain))
                me->ModifyPower(POWER_RUNIC_POWER, int32(rp));
        }

        void EnterCombat(Unit* /*who*/)
        {
            DoCastVictim(SPELL_DEATH_GRIP);
            DoCastVictim(SPELL_CHAINS_OF_ICE);
        }

        void Range(const uint32 uiDiff)
        {
            if (m_uiDeathGripTimer <= uiDiff)
            {
                if (me->IsInRange(me->getVictim(), 8.0f, 30.0f, false))
                    DoCastVictim(SPELL_DEATH_GRIP);
                m_uiDeathGripTimer = 35*IN_MILLISECONDS;
            } else m_uiDeathGripTimer -= uiDiff;

            if (m_uiChainsOfIceTimer <= uiDiff)
            {
               DoCastVictim(SPELL_CHAINS_OF_ICE);
                m_uiChainsOfIceTimer = 8*IN_MILLISECONDS;
            } else m_uiChainsOfIceTimer -= uiDiff;

        }

        void Melee(const uint32 uiDiff)
        {
            if (m_uiStrangulateTimer <= uiDiff)
            {
                if (Unit* target = SelectEnemyCaster(false))
                    DoCast(target, SPELL_STRANGULATE);
                m_uiStrangulateTimer = 120*IN_MILLISECONDS;
            } else m_uiStrangulateTimer -= uiDiff;

            if (m_uiCommonTimer <= uiDiff)
            {
                switch(urand(0, 2))
                {
                case 0:
                    DoCastVictim(SPELL_FROST_STRIKE);
                    break;
                case 1:
                    DoCastVictim(SPELL_ICY_TOUCH);
                    break;
                case 2:
                    DoCastVictim(SPELL_DEATH_COIL);
                    break;
                }
                m_uiCommonTimer = 3*IN_MILLISECONDS;
            } else m_uiCommonTimer -= uiDiff;

        }
        
        void UpdateAI(uint32 uiDiff)
        {
            if (!UpdateVictim()) return;

            if (m_uiIceboundFortitudeTimer)
            {
                if (m_uiIceboundFortitudeTimer <= uiDiff)
                {
                    Icebound = true;
                    m_uiIceboundFortitudeTimer = 0;
                }
                else m_uiIceboundFortitudeTimer -= uiDiff;
            }
            
            if (Icebound && HealthBelowPct(50))
            {     
                    DoCast(me, SPELL_ICEBOUND_FORTITUDE);
                    Icebound = false;
                m_uiIceboundFortitudeTimer = 60*IN_MILLISECONDS;
            }

            if (me->HasAuraType(SPELL_AURA_MOD_STUN) || me->HasAuraType(SPELL_AURA_MOD_FEAR) || me->HasAuraType(SPELL_AURA_MOD_CHARM) || me->HasAuraType(SPELL_AURA_MOD_ROOT) || me->HasAuraType(SPELL_AURA_MOD_CONFUSE))
            {
                if (Trinket)
                {
                    DoCast(me, SPELL_PVP_TRINKET, true);
                    Trinket = false;
                    m_uiPvpTrinketTimer = 120*IN_MILLISECONDS;
                }
            }

            if (m_uiPvpTrinketTimer)
            {
                if (m_uiPvpTrinketTimer <= uiDiff)
                {
                    Trinket = true;
                    m_uiPvpTrinketTimer = 0;
                }
                else m_uiPvpTrinketTimer -= uiDiff;
            }

            if (me->IsInRange(me->getVictim(), 0.0f, 8.0f, false))
                Melee(uiDiff);
            else 
                Range(uiDiff);

            boss_faction_championsAI::UpdateAI(uiDiff);
        }
    };

};

enum eRogueSpells
{
    SPELL_DUAL_WIELD_RG         = 42459,
    SPELL_GETHIT                = 62627,
    SPELL_WOUND_POISON          = 65962,
    SPELL_DEADLY_POISON_AURA    = 67711,
    SPELL_DEADLY_POISON         = 67710,
    SPELL_MUTILATE              = 48666,
    SPELL_FAN_OF_KNIVES         = 65955, 
    SPELL_BLIND                 = 65960, //cd 2min
    SPELL_CLOAK                 = 65961, //cd 1.5min 
    SPELL_BLADE_FLURRY          = 65956, //cd 2min
    SPELL_HEMORRHAGE            = 65954,
    SPELL_EVISCERATE            = 65957,
};

class mob_toc_rogue : public CreatureScript
{
public:
    mob_toc_rogue() : CreatureScript("mob_toc_rogue") { }

    CreatureAI* GetAI(Creature *pCreature) const
    {
        return new mob_toc_rogueAI (pCreature);
    }

    struct mob_toc_rogueAI : public boss_faction_championsAI
    {
        mob_toc_rogueAI(Creature *pCreature) : boss_faction_championsAI(pCreature, AI_MELEE)
        {
            me->setPowerType(POWER_ENERGY);
            me->SetPower(POWER_ENERGY,100);
        }
        
        uint32 m_uiMutilateTimer;
        uint32 m_uiFanOfKnivesTimer;
        uint32 m_uiHemorrhageTimer;
        uint32 m_uiEviscerateTimer;
        uint32 m_uiBlindTimer;
        uint32 m_uiCloakTimer;
        uint32 m_uiBladeFlurryTimer;
        uint32 m_uiPvpTrinketTimer;
        bool Cloak;
        bool Trinket;
     
        void Reset()
        {
            boss_faction_championsAI::Reset();
            me->SetBaseWeaponDamage(OFF_ATTACK, MINDAMAGE, 904);
            me->SetBaseWeaponDamage(OFF_ATTACK, MAXDAMAGE, 1096);
            m_uiPvpTrinketTimer = 0;
            Cloak = true;
            Trinket = true;
            m_uiMutilateTimer = urand(3*IN_MILLISECONDS, 6*IN_MILLISECONDS);
            m_uiFanOfKnivesTimer = urand(8*IN_MILLISECONDS, 12*IN_MILLISECONDS);
            m_uiHemorrhageTimer = urand(2*IN_MILLISECONDS, 4*IN_MILLISECONDS);
            m_uiEviscerateTimer = urand(15*IN_MILLISECONDS, 25*IN_MILLISECONDS);
            m_uiBlindTimer = 90*IN_MILLISECONDS;
            m_uiCloakTimer = 0;
            m_uiBladeFlurryTimer = urand(12*IN_MILLISECONDS, 120*IN_MILLISECONDS);
            DoCast(me, SPELL_DUAL_WIELD_RG, true);
            DoCast(me, SPELL_GETHIT);
            SetEquipmentSlots(false,28226 ,28226 , EQUIP_NO_CHANGE);
        }
        
        void EnterCombat(Unit* /*who*/)
        { 
            DoCast(me, SPELL_DEADLY_POISON_AURA, true);
            DoCastVictim(SPELL_BLIND, true);
            me->RemoveAura(SPELL_GETHIT);
            m_uiBlindTimer = 90*IN_MILLISECONDS;
        } 

        void SpellHitTarget(Unit* target, const SpellInfo* sp)
        {
            if (sp->Id == SPELL_DEADLY_POISON)
                me->CastSpell(target, SPELL_WOUND_POISON, true);
        }

        void UpdateAI(uint32 uiDiff)
        {
            if (!UpdateVictim()) return;
            
            if (m_uiFanOfKnivesTimer <= uiDiff)
            {
                if (EnemiesInRange(15.0f) > 2)
                        DoCastAOE(SPELL_FAN_OF_KNIVES);
                    m_uiFanOfKnivesTimer = urand(8*IN_MILLISECONDS, 10*IN_MILLISECONDS);
                } else m_uiFanOfKnivesTimer -= uiDiff;
            
            if (m_uiMutilateTimer <= uiDiff)
            {
                DoCastVictim(SPELL_MUTILATE,true);
                m_uiMutilateTimer = urand(3*IN_MILLISECONDS, 6*IN_MILLISECONDS);
            } else m_uiMutilateTimer -= uiDiff;
            
            if (m_uiHemorrhageTimer <= uiDiff)
            {
                DoCastVictim(SPELL_HEMORRHAGE);
                m_uiHemorrhageTimer = urand(2*IN_MILLISECONDS, 4*IN_MILLISECONDS);
            } else m_uiHemorrhageTimer -= uiDiff;
            
            if (m_uiEviscerateTimer <= uiDiff)
            {
                DoCastVictim(SPELL_EVISCERATE);
                m_uiEviscerateTimer = urand(15*IN_MILLISECONDS, 20*IN_MILLISECONDS);
            } else m_uiEviscerateTimer -= uiDiff;
            
            if (m_uiBlindTimer <= uiDiff)
            {
                if (Unit * target = SelectTarget(SELECT_TARGET_RANDOM, 1, 40, true))
                    DoCast(target, SPELL_BLIND, true);
                    m_uiBlindTimer = 120*IN_MILLISECONDS;
            } else m_uiBlindTimer -= uiDiff;
            
            if (HealthBelowPct(50) && Cloak)
            {
                Cloak = false;
                DoCast(me, SPELL_CLOAK);
                m_uiCloakTimer = 90*IN_MILLISECONDS;
            }

            if (m_uiCloakTimer)
            {
                if (m_uiCloakTimer <= uiDiff)
                {
                    Cloak = true;
                    m_uiCloakTimer = 0;
                }
                else m_uiCloakTimer -= uiDiff;
            }
            
            if (m_uiBladeFlurryTimer <= uiDiff)
            {
                DoCastVictim(SPELL_BLADE_FLURRY);
                m_uiBladeFlurryTimer = 120*IN_MILLISECONDS;
            } else m_uiBladeFlurryTimer -= uiDiff;

            if (me->HasAuraType(SPELL_AURA_MOD_STUN) || me->HasAuraType(SPELL_AURA_MOD_FEAR) || me->HasAuraType(SPELL_AURA_MOD_CHARM) || me->HasAuraType(SPELL_AURA_MOD_ROOT) || me->HasAuraType(SPELL_AURA_MOD_CONFUSE))
            {
                if (Trinket)
                {
                    DoCast(me, SPELL_PVP_TRINKET, true);
                    Trinket = false;
                    m_uiPvpTrinketTimer = 120*IN_MILLISECONDS;
                }
            }

            if (m_uiPvpTrinketTimer)
            {
                if (m_uiPvpTrinketTimer <= uiDiff)
                {
                    Trinket = true;
                    m_uiPvpTrinketTimer = 0;
                }
                else m_uiPvpTrinketTimer -= uiDiff;
            }
            
            boss_faction_championsAI::UpdateAI(uiDiff);
        }
                
            
    };

};

enum eEnhShamanSpells
{
    SPELL_DUAL_WIELD         = 42459,
    SPELL_WINDFURY           = 65976,
    SPELL_LIGHTNING_BOLT     = 65987,
    SPELL_EARTH_SHOCK_ENH    = 65973,
    SPELL_LAVA_LASH          = 65974,
    SPELL_STORMSTRIKE        = 65970, //cd 8sec
};

class mob_toc_enh_shaman : public CreatureScript
{
public:
    mob_toc_enh_shaman() : CreatureScript("mob_toc_enh_shaman") { }

    CreatureAI* GetAI(Creature *pCreature) const
    {
        return new mob_toc_enh_shamanAI (pCreature);
    }

    struct mob_toc_enh_shamanAI : public boss_faction_championsAI
    {
        mob_toc_enh_shamanAI(Creature *pCreature) : boss_faction_championsAI(pCreature, AI_MELEE), Summons(me) {}

        SummonList Summons;

        uint32 m_uiCommonTimer;
        uint32 m_uiWindfuryTimer;
        uint32 m_uiHeroismOrBloodlustTimer;
        uint32 m_uiDeployTotemTimer;
        uint32 m_uiPvpTrinketTimer;
        bool Trinket;
        uint8  m_uiTotemCount;
        float  m_fTotemOldCenterX, m_fTotemOldCenterY;
        

        void Reset()
        {
            boss_faction_championsAI::Reset();
            me->SetBaseWeaponDamage(OFF_ATTACK, MINDAMAGE, 749);
            me->SetBaseWeaponDamage(OFF_ATTACK, MAXDAMAGE, 953);
            m_uiPvpTrinketTimer = 0;
            Trinket = true;
            m_uiWindfuryTimer = 8*IN_MILLISECONDS;
            m_uiCommonTimer = 3*IN_MILLISECONDS;
            m_uiHeroismOrBloodlustTimer = 5*IN_MILLISECONDS;
            m_uiDeployTotemTimer = urand(1*IN_MILLISECONDS, 3*IN_MILLISECONDS);
            m_uiTotemCount = 0;
            DoCast(me, SPELL_DUAL_WIELD, true);
            m_fTotemOldCenterX = me->GetPositionX();
            m_fTotemOldCenterY = me->GetPositionY();
            SetEquipmentSlots(false, 51515, 51515, EQUIP_NO_CHANGE);
            Summons.DespawnAll();
        }

       void JustSummoned(Creature* summoned)
        {
            Summons.Summon(summoned);
        }

        void SummonedCreatureDespawn(Creature* /*pSummoned*/)
        {
            --m_uiTotemCount;
        }

        void DeployTotem()
        {
            m_uiTotemCount = 4;
            m_fTotemOldCenterX = me->GetPositionX();
            m_fTotemOldCenterY = me->GetPositionY();
            /*
            -Windfury (16% melee haste)
            -Grounding (redirects one harmful magic spell to the totem)

            -Healing Stream (unable to find amount of healing in our logs)

            -Tremor (prevents fear effects)
            -Strength of Earth (155 strength and agil for the opposing team)

            -Searing (average ~3500 damage on a random target every ~3.5 seconds)
            */
        }
        
        void JustDied(Unit* killer)
        {
            boss_faction_championsAI::JustDied(killer);
            Summons.DespawnAll();
        }

        void UpdateAI(uint32 uiDiff)
        {
            if (!UpdateVictim()) return;

            if (m_uiHeroismOrBloodlustTimer <= uiDiff)
            {
                if (me->getFaction()) //Am i alliance?
                {
                    if (!me->HasAura(AURA_EXHAUSTION))
                        DoCastAOE(SPELL_HEROISM);
                }
                else
                    if (!me->HasAura(AURA_SATED))
                        DoCastAOE(SPELL_BLOODLUST);
                m_uiHeroismOrBloodlustTimer = 300*IN_MILLISECONDS;
            } else m_uiHeroismOrBloodlustTimer -= uiDiff;
            
            if (m_uiDeployTotemTimer <= uiDiff)
            {
                if (m_uiTotemCount < 4 || me->GetDistance2d(m_fTotemOldCenterX, m_fTotemOldCenterY) > 20.0f)
                    DeployTotem();
                m_uiDeployTotemTimer = urand(1*IN_MILLISECONDS, 3*IN_MILLISECONDS);
            } else m_uiDeployTotemTimer -= uiDiff;

           if (m_uiWindfuryTimer <= uiDiff)
           {
               DoCast(me, SPELL_WINDFURY);
               m_uiWindfuryTimer = 8*IN_MILLISECONDS;
           } else m_uiWindfuryTimer -= uiDiff;

           if (m_uiCommonTimer <= uiDiff)
           {
               switch(urand(0, 3))
               {
               case 0:
                   DoCastVictim(SPELL_LIGHTNING_BOLT );
                   break;
               case 1:
                   DoCastVictim(SPELL_EARTH_SHOCK_ENH);
                   break;
               case 2:
                   DoCastVictim(SPELL_STORMSTRIKE);
                   break;
               case 3:
                   DoCastVictim(SPELL_LAVA_LASH);
                   break;
               }
               m_uiCommonTimer = 3*IN_MILLISECONDS;
           } else m_uiCommonTimer -= uiDiff;
           
           if (me->HasAuraType(SPELL_AURA_MOD_STUN) || me->HasAuraType(SPELL_AURA_MOD_FEAR) || me->HasAuraType(SPELL_AURA_MOD_CHARM) || me->HasAuraType(SPELL_AURA_MOD_ROOT) || me->HasAuraType(SPELL_AURA_MOD_CONFUSE))
           {
                if (Trinket)
                {
                    DoCast(me, SPELL_PVP_TRINKET, true);
                    Trinket = false;
                    m_uiPvpTrinketTimer = 120*IN_MILLISECONDS;
                }
            }

            if (m_uiPvpTrinketTimer)
            {
                if (m_uiPvpTrinketTimer <= uiDiff)
                {
                    Trinket = true;
                    m_uiPvpTrinketTimer = 0;
                }
                else m_uiPvpTrinketTimer -= uiDiff;
            }
           
           boss_faction_championsAI::UpdateAI(uiDiff);
        }
    };

};

enum eRetroPaladinSpells
{
    SPELL_BUBBLE_RET             = 66010, //cd 5min
    SPELL_AVENGING_WRATH         = 66011, //cd 3min 
    SPELL_CRUSADER_STRIKE        = 66003, //cd 6sec
    SPELL_DIVINE_STORM           = 66006, //cd 10sec
    SPELL_HAMMER_OF_JUSTICE_RET  = 66613, //cd 40sec
    SPELL_HAND_OF_PROTECTION_RET = 66009, //cd 5min
    SPELL_JUDGEMENT_OF_COMMAND   = 66005, 
    SPELL_REPENTANCE             = 20066, //cd 15sec
    SPELL_SEAL_OF_COMMAND        = 66004, 
};

class mob_toc_retro_paladin : public CreatureScript
{
public:
    mob_toc_retro_paladin() : CreatureScript("mob_toc_retro_paladin") { }

    CreatureAI* GetAI(Creature *pCreature) const
    {
        return new mob_toc_retro_paladinAI (pCreature);
    }

    struct mob_toc_retro_paladinAI : public boss_faction_championsAI
    {
        mob_toc_retro_paladinAI(Creature *pCreature) : boss_faction_championsAI(pCreature, AI_MELEE) {}

        uint32 m_uiRepentanceTimer;
        uint32 m_uiCrusaderStrikeTimer;
        uint32 m_uiAvengingWrathTimer;
        uint32 m_uiDivineStormTimer;
        uint32 m_uiJudgementOfCommandTimer;
        uint32 m_uiHammerOfJusticeRetTimer;
        uint32 m_uiHandOfProtectionRetdTimer;
        uint32 m_uiPvpTrinketTimer;
        uint32 m_uiBubbleRetTimer;
        bool Trinket;
        bool m_uiBubble;
        bool m_uiSaveFriend;

        void Reset()
        {
            boss_faction_championsAI::Reset();
            m_uiHammerOfJusticeRetTimer = urand(10*IN_MILLISECONDS, 15*IN_MILLISECONDS);
            m_uiRepentanceTimer = urand(10*IN_MILLISECONDS, 15*IN_MILLISECONDS);
            m_uiCrusaderStrikeTimer = urand(6*IN_MILLISECONDS, 18*IN_MILLISECONDS);
            m_uiAvengingWrathTimer = 360*IN_MILLISECONDS;
            m_uiDivineStormTimer = 10*IN_MILLISECONDS;
            m_uiJudgementOfCommandTimer = urand(8*IN_MILLISECONDS, 15*IN_MILLISECONDS);
            m_uiHandOfProtectionRetdTimer = 0;
            m_uiPvpTrinketTimer = 0;
            m_uiBubbleRetTimer = 0;
            Trinket = true;
            m_uiBubble = true;
            m_uiSaveFriend = true;
            SetEquipmentSlots(false, 49498, EQUIP_NO_CHANGE, EQUIP_NO_CHANGE);
        }

        void EnterCombat(Unit* /*who*/)
        {
            DoCastVictim(SPELL_REPENTANCE, true);
            DoCast(me, SPELL_SEAL_OF_COMMAND);
            DoCast(me, SPELL_AVENGING_WRATH );
        }

        void UpdateAI(uint32 uiDiff)
        {
            if (!UpdateVictim()) return;

            if (m_uiBubbleRetTimer)
            {
                if (m_uiBubbleRetTimer <= uiDiff)
                {
                    m_uiBubble = true;
                    m_uiBubbleRetTimer = 0;
                } else m_uiBubbleRetTimer -= uiDiff;
            }  

            if (HealthBelowPct(25) && m_uiBubble)
            {
                DoCast(SPELL_BUBBLE_RET);
                m_uiBubble = false;
                m_uiBubbleRetTimer = 300*IN_MILLISECONDS;
            }

            if (m_uiRepentanceTimer <= uiDiff)
            {
                if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0))
                    DoCast(target, SPELL_REPENTANCE);
                m_uiRepentanceTimer = 60*IN_MILLISECONDS;
            } else m_uiRepentanceTimer -= uiDiff;

            if (m_uiHammerOfJusticeRetTimer <= uiDiff)
            {
                DoCastVictim(SPELL_HAMMER_OF_JUSTICE_RET);
                m_uiHammerOfJusticeRetTimer = urand(10*IN_MILLISECONDS, 15*IN_MILLISECONDS);
            } else m_uiHammerOfJusticeRetTimer -= uiDiff;

            if (m_uiCrusaderStrikeTimer <= uiDiff)
            {
                DoCastVictim(SPELL_CRUSADER_STRIKE);
                m_uiCrusaderStrikeTimer = urand(6*IN_MILLISECONDS, 18*IN_MILLISECONDS);
            } else m_uiCrusaderStrikeTimer -= uiDiff;

            if (m_uiAvengingWrathTimer <= uiDiff)
            {
                DoCastVictim(SPELL_AVENGING_WRATH);
                m_uiAvengingWrathTimer = 360*IN_MILLISECONDS;
            } else m_uiAvengingWrathTimer -= uiDiff;

            if (m_uiHandOfProtectionRetdTimer)
            {
                if (m_uiHandOfProtectionRetdTimer <= uiDiff)
                {
                    m_uiHandOfProtectionRetdTimer = 0;
                    m_uiSaveFriend = true;
                } else m_uiHandOfProtectionRetdTimer -= uiDiff;
            }

            if (m_uiDivineStormTimer <= uiDiff)
            {
                DoCastVictim(SPELL_DIVINE_STORM);
                m_uiDivineStormTimer = 10*IN_MILLISECONDS;
            } else m_uiDivineStormTimer -= uiDiff;
            
            if (m_uiJudgementOfCommandTimer <= uiDiff)
            {
                DoCastVictim(SPELL_JUDGEMENT_OF_COMMAND);
                m_uiJudgementOfCommandTimer = urand(8*IN_MILLISECONDS, 15*IN_MILLISECONDS);
            } else m_uiJudgementOfCommandTimer -= uiDiff;
            
            if (Unit* target = DoSelectLowestHpFriendly(40.0f))
            {
                if (target->HealthBelowPct(30) && m_uiSaveFriend)
                {
                    if (target->GetGUID() == me->GetGUID())
                        return;

                    DoCast(target, SPELL_HAND_OF_PROTECTION_RET);
                m_uiSaveFriend = false;
                m_uiHandOfProtectionRetdTimer = 300*IN_MILLISECONDS;
                }
            }

            if (me->HasAuraType(SPELL_AURA_MOD_STUN) || me->HasAuraType(SPELL_AURA_MOD_FEAR) || me->HasAuraType(SPELL_AURA_MOD_CHARM) || me->HasAuraType(SPELL_AURA_MOD_ROOT) || me->HasAuraType(SPELL_AURA_MOD_CONFUSE))
            {
                if (Trinket)
                {
                    DoCast(me, SPELL_PVP_TRINKET, true);
                    Trinket = false;
                    m_uiPvpTrinketTimer = 120*IN_MILLISECONDS;
                }
            }

            if (m_uiPvpTrinketTimer)
            {
                if (m_uiPvpTrinketTimer <= uiDiff)
                {
                    Trinket = true;
                    m_uiPvpTrinketTimer = 0;
                }
                else m_uiPvpTrinketTimer -= uiDiff;
            }
            
            boss_faction_championsAI::UpdateAI(uiDiff);
        }
    };

};

enum eWarlockPetSpells
{
    SPELL_DEVOUR_MAGIC  = 67518,
    SPELL_SPELL_LOCK  = 67519,
};

class mob_toc_pet_warlock : public CreatureScript
{
public:
    mob_toc_pet_warlock() : CreatureScript("mob_toc_pet_warlock") { }

    CreatureAI* GetAI(Creature *pCreature) const
    {
        return new mob_toc_pet_warlockAI (pCreature);
    }

    struct mob_toc_pet_warlockAI : public boss_faction_championsAI
    {
        mob_toc_pet_warlockAI(Creature *pCreature) : boss_faction_championsAI(pCreature, AI_PET) {}

        uint32 m_uiDevourMagicTimer;
        uint32 m_uiSpellLockTimer;

        void Reset()
        {
            boss_faction_championsAI::Reset();
            m_uiDevourMagicTimer = urand(15*IN_MILLISECONDS, 30*IN_MILLISECONDS);
            m_uiSpellLockTimer = urand(15*IN_MILLISECONDS, 30*IN_MILLISECONDS);
        }

        void UpdateAI(uint32 uiDiff)
        {
            if (!UpdateVictim()) return;

            if (m_uiDevourMagicTimer <= uiDiff)
            {
                DoCastVictim(SPELL_DEVOUR_MAGIC);
                m_uiDevourMagicTimer = urand(15*IN_MILLISECONDS, 30*IN_MILLISECONDS);
            } else m_uiDevourMagicTimer -= uiDiff;

            if (m_uiSpellLockTimer <= uiDiff)
            {
                DoCastVictim(SPELL_SPELL_LOCK);
                m_uiSpellLockTimer = urand(15*IN_MILLISECONDS, 30*IN_MILLISECONDS);
            } else m_uiSpellLockTimer -= uiDiff;

            boss_faction_championsAI::UpdateAI(uiDiff);
        }
    };

};

enum eHunterPetSpells
{
    SPELL_CLAW  = 67793,
};

class mob_toc_pet_hunter : public CreatureScript
{
public:
    mob_toc_pet_hunter() : CreatureScript("mob_toc_pet_hunter") { }

    CreatureAI* GetAI(Creature *pCreature) const
    {
        return new mob_toc_pet_hunterAI (pCreature);
    }

    struct mob_toc_pet_hunterAI : public boss_faction_championsAI
    {
        mob_toc_pet_hunterAI(Creature *pCreature) : boss_faction_championsAI(pCreature, AI_PET) {}

        uint32 m_uiClawTimer;

        void Reset()
        {
            boss_faction_championsAI::Reset();
            m_uiClawTimer = urand(5*IN_MILLISECONDS, 10*IN_MILLISECONDS);
        }

        void UpdateAI(uint32 uiDiff)
        {
            if (!UpdateVictim()) return;

            if (m_uiClawTimer <= uiDiff)
            {
                DoCastVictim(SPELL_CLAW);
                m_uiClawTimer = urand(5*IN_MILLISECONDS, 10*IN_MILLISECONDS);
            } else m_uiClawTimer -= uiDiff;

            boss_faction_championsAI::UpdateAI(uiDiff);
        }
    };

};

/*========================================================*/

void AddSC_boss_faction_champions()
{
    new boss_toc_champion_controller();
    new mob_toc_druid();
    new mob_toc_shaman();
    new mob_toc_paladin();
    new mob_toc_priest();
    new mob_toc_shadow_priest();
    new mob_toc_mage();
    new mob_toc_warlock();
    new mob_toc_hunter();
    new mob_toc_boomkin();
    new mob_toc_warrior();
    new mob_toc_dk();
    new mob_toc_rogue();
    new mob_toc_enh_shaman();
    new mob_toc_retro_paladin();
    new mob_toc_pet_warlock();
    new mob_toc_pet_hunter();
}
