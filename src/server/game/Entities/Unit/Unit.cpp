/*
 * Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2005-2009 MaNGOS <http://getmangos.com/>
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

#include "Common.h"
#include "CreatureAIImpl.h"
#include "Log.h"
#include "Opcodes.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "World.h"
#include "ObjectMgr.h"
#include "SpellMgr.h"
#include "Unit.h"
#include "QuestDef.h"
#include "Player.h"
#include "Creature.h"
#include "Spell.h"
#include "Group.h"
#include "SpellAuras.h"
#include "SpellAuraEffects.h"
#include "MapManager.h"
#include "ObjectAccessor.h"
#include "CreatureAI.h"
#include "Formulas.h"
#include "Pet.h"
#include "Util.h"
#include "Totem.h"
#include "Battleground.h"
#include "OutdoorPvP.h"
#include "InstanceSaveMgr.h"
#include "GridNotifiersImpl.h"
#include "CellImpl.h"
#include "CreatureGroups.h"
#include "PetAI.h"
#include "PassiveAI.h"
#include "TemporarySummon.h"
#include "Vehicle.h"
#include "Transport.h"
#include "InstanceScript.h"
#include "SpellInfo.h"
#include "MoveSplineInit.h"
#include "MoveSpline.h"
#include "ConditionMgr.h"
#include "UpdateFieldFlags.h"
#include "Battlefield.h"
#include "BattlefieldMgr.h"
#include <math.h>

float baseMoveSpeed[MAX_MOVE_TYPE] =
{
    2.5f,                  // MOVE_WALK
    7.0f,                  // MOVE_RUN
    4.5f,                  // MOVE_RUN_BACK
    4.722222f,             // MOVE_SWIM
    2.5f,                  // MOVE_SWIM_BACK
    3.141594f,             // MOVE_TURN_RATE
    7.0f,                  // MOVE_FLIGHT
    4.5f,                  // MOVE_FLIGHT_BACK
    3.14f                  // MOVE_PITCH_RATE
};

float playerBaseMoveSpeed[MAX_MOVE_TYPE] =
{
    2.5f,                  // MOVE_WALK
    7.0f,                  // MOVE_RUN
    4.5f,                  // MOVE_RUN_BACK
    4.722222f,             // MOVE_SWIM
    2.5f,                  // MOVE_SWIM_BACK
    3.141594f,             // MOVE_TURN_RATE
    7.0f,                  // MOVE_FLIGHT
    4.5f,                  // MOVE_FLIGHT_BACK
    3.14f                  // MOVE_PITCH_RATE
};

// Used for prepare can/can`t triggr aura
static bool InitTriggerAuraData();
// Define can trigger auras
static bool isTriggerAura[TOTAL_AURAS];
// Define can't trigger auras (need for disable second trigger)
static bool isNonTriggerAura[TOTAL_AURAS];
// Triggered always, even from triggered spells
static bool isAlwaysTriggeredAura[TOTAL_AURAS];
// Has cap to damage
static bool isDamageCapAura[TOTAL_AURAS];
// Prepare lists
static bool procPrepared = InitTriggerAuraData();

DamageInfo::DamageInfo(Unit* _attacker, Unit* _victim, uint32 _damage, SpellInfo const* _spellInfo, SpellSchoolMask _schoolMask, DamageEffectType _damageType, uint32 m_damageBeforeHit)
: m_attacker(_attacker), m_victim(_victim), m_damage(_damage), m_spellInfo(_spellInfo), m_schoolMask(_schoolMask),
m_damageType(_damageType), m_attackType(BASE_ATTACK), m_damageBeforeHit(m_damageBeforeHit)
{
    m_absorb = 0;
    m_resist = 0;
    m_block = 0;
    m_cleanDamage = 0;
    m_addpower = 0;
    m_addptype = -1;
}
DamageInfo::DamageInfo(CalcDamageInfo& dmgInfo)
: m_attacker(dmgInfo.attacker), m_victim(dmgInfo.target), m_damage(dmgInfo.damage), m_spellInfo(NULL), m_schoolMask(SpellSchoolMask(dmgInfo.damageSchoolMask)),
m_damageType(DIRECT_DAMAGE), m_attackType(dmgInfo.attackType), m_absorb(dmgInfo.absorb), m_resist(dmgInfo.resist), m_block(dmgInfo.blocked_amount), m_cleanDamage(dmgInfo.cleanDamage)
, m_damageBeforeHit(dmgInfo.damageBeforeHit), m_addptype(-1), m_addpower(0)
{
}
DamageInfo::DamageInfo(SpellNonMeleeDamage& dmgInfo, SpellInfo const* _spellInfo)
: m_attacker(dmgInfo.attacker), m_victim(dmgInfo.target), m_damage(dmgInfo.damage), m_spellInfo(_spellInfo), m_schoolMask(SpellSchoolMask(dmgInfo.schoolMask)),
m_damageType(DIRECT_DAMAGE), m_attackType(BASE_ATTACK), m_absorb(dmgInfo.absorb), m_resist(dmgInfo.resist), m_block(dmgInfo.blocked),
 m_cleanDamage(dmgInfo.cleanDamage), m_damageBeforeHit(dmgInfo.damageBeforeHit), m_addptype(-1), m_addpower(0)
{
}

void DamageInfo::ModifyDamage(int32 amount)
{
    amount = std::min(amount, int32(GetDamage()));
    m_damage += amount;
}

void DamageInfo::AbsorbDamage(int32 amount)
{
    amount = std::min(amount, int32(GetDamage()));
    m_absorb += amount;
    m_damage -= amount;
}

void DamageInfo::ResistDamage(uint32 amount)
{
    amount = std::min(amount, GetDamage());
    m_resist += amount;
    m_damage -= amount;
}

void DamageInfo::BlockDamage(uint32 amount)
{
    amount = std::min(amount, GetDamage());
    m_block += amount;
    m_damage -= amount;
}

ProcEventInfo::ProcEventInfo(Unit* actor, Unit* actionTarget, Unit* procTarget, uint32 typeMask, uint32 spellTypeMask, uint32 spellPhaseMask, uint32 hitMask, Spell* spell, DamageInfo* damageInfo, HealInfo* healInfo)
:_actor(actor), _actionTarget(actionTarget), _procTarget(procTarget), _typeMask(typeMask), _spellTypeMask(spellTypeMask), _spellPhaseMask(spellPhaseMask),
_hitMask(hitMask), _spell(spell), _damageInfo(damageInfo), _healInfo(healInfo)
{
}

// we can disable this warning for this since it only
// causes undefined behavior when passed to the base class constructor
#ifdef _MSC_VER
#pragma warning(disable:4355)
#endif
Unit::Unit(bool isWorldObject): WorldObject(isWorldObject)
    , m_movedPlayer(NULL)
    , m_lastSanctuaryTime(0)
    , m_TempSpeed(0.0f)
    , IsAIEnabled(false)
    , NeedChangeAI(false)
    , m_ControlledByPlayer(false)
    , movespline(new Movement::MoveSpline())
    , i_AI(NULL)
    , i_disabledAI(NULL)
    , m_AutoRepeatFirstCast(false)
    , m_procDeep(0)
    , m_removedAurasCount(0)
    , i_motionMaster(this)
    , m_ThreatManager(this)
    , m_vehicle(NULL)
    , m_vehicleKit(NULL)
    , m_unitTypeMask(UNIT_MASK_NONE)
    , m_HostileRefManager(this)
    , LastCharmerGUID(0)
    , _delayInterruptFlag(0)
    , m_onMount(false)
    , m_castCounter(0)
{
#ifdef _MSC_VER
#pragma warning(default:4355)
#endif
    m_objectType |= TYPEMASK_UNIT;
    m_objectTypeId = TYPEID_UNIT;

    m_updateFlag = UPDATEFLAG_LIVING;

    m_attackTimer[BASE_ATTACK] = 0;
    m_attackTimer[OFF_ATTACK] = 0;
    m_attackTimer[RANGED_ATTACK] = 0;
    m_modAttackSpeedPct[BASE_ATTACK] = 1.0f;
    m_modAttackSpeedPct[OFF_ATTACK] = 1.0f;
    m_modAttackSpeedPct[RANGED_ATTACK] = 1.0f;
    m_attackDist = MELEE_RANGE;
    Zliquid_status = LIQUID_MAP_NO_WATER;

    m_extraAttacks = 0;
    countCrit = 0;
    insightCount = 0;
    m_canDualWield = false;

    m_rootTimes = 0;
    m_timeForSpline = 0;

    m_state = 0;
    m_deathState = ALIVE;

    for (uint8 i = 0; i < CURRENT_MAX_SPELL; ++i)
        m_currentSpells[i] = NULL;

    m_addDmgOnce = 0;

    for (uint8 i = 0; i < MAX_SUMMON_SLOT; ++i)
        m_SummonSlot[i] = 0;

    for (uint8 i = 0; i < MAX_GAMEOBJECT_SLOT; ++i)
        m_ObjectSlot[i] = 0;

    m_auraUpdateIterator = m_ownedAuras.end();

    m_interruptMask = 0;
    m_transform = 0;
    m_canModifyStats = false;

    for (uint8 i = 0; i < MAX_SPELL_IMMUNITY; ++i)
        m_spellImmune[i].clear();

    for (uint8 i = 0; i < UNIT_MOD_END; ++i)
    {
        m_auraModifiersGroup[i][BASE_VALUE] = 0.0f;
        m_auraModifiersGroup[i][BASE_PCT] = 1.0f;
        m_auraModifiersGroup[i][TOTAL_VALUE] = 0.0f;
        m_auraModifiersGroup[i][TOTAL_PCT] = 1.0f;
    }
                                                            // implement 50% base damage from offhand
    m_auraModifiersGroup[UNIT_MOD_DAMAGE_OFFHAND][TOTAL_PCT] = 0.5f;

    for (uint8 i = 0; i < MAX_ATTACK; ++i)
    {
        m_weaponDamage[i][MINDAMAGE] = BASE_MINDAMAGE;
        m_weaponDamage[i][MAXDAMAGE] = BASE_MAXDAMAGE;
    }

    for (uint8 i = 0; i < MAX_STATS; ++i)
        m_createStats[i] = 0.0f;

    m_attacking = NULL;
    m_modMeleeHitChance = 0.0f;
    m_modRangedHitChance = 0.0f;
    m_modSpellHitChance = 0.0f;
    m_expertise = 0.0f;
    m_baseSpellCritChance = 5;
    m_anti_JupmTime = 0;                // Jump Time
    m_anti_FlightTime = 0;                // Jump Time

    m_CombatTimer = 0;

    for (uint8 i = 0; i < MAX_SPELL_SCHOOL; ++i)
        m_threatModifier[i] = 1.0f;

    m_isSorted = true;

    for (uint8 i = 0; i < MAX_MOVE_TYPE; ++i)
        m_speed_rate[i] = 1.0f;

    m_charmInfo = NULL;
    m_reducedThreatPercent = 0;
    m_misdirectionTargetGUID = 0;
    m_curTargetGUID = 0;

    // remove aurastates allowing special moves
    for (uint8 i = 0; i < MAX_REACTIVE; ++i)
        m_reactiveTimer[i] = 0;

    m_cleanupDone = false;
    m_duringRemoveFromWorld = false;

    m_serverSideVisibility.SetValue(SERVERSIDE_VISIBILITY_GHOST, GHOST_VISIBILITY_ALIVE);

    _focusSpell = NULL;
    _lastLiquid = NULL;
    _isWalkingBeforeCharm = false;
    _mount = NULL;

    m_IsInKillingProcess = false;
    m_VisibilityUpdScheduled = false;
    m_VisibilityUpdateTask = false;
    m_diffMode = GetMap() ? GetMap()->GetSpawnMode() : 0;
    m_SpecialTarget = 0;
    isMagnetSpellTarget = false;

    m_damage_counter_timer = 1 * IN_MILLISECONDS;
    for (int i = 0; i < MAX_DAMAGE_COUNTERS; ++i)
        m_damage_counters[i].push_front(0);

    m_baseRHastRatingPct = 1.0f;
    m_baseMHastRatingPct = 1.0f;
    m_baseHastRatingPct = 1.0f;
    m_modForHolyPowerSpell = 0;

    for (uint8 i = 0; i < MAX_COMBAT_RATING; i++)
        m_baseRatingValue[i] = 0;
}

////////////////////////////////////////////////////////////
// Methods of class GlobalCooldownMgr
bool GlobalCooldownMgr::HasGlobalCooldown(SpellInfo const* spellInfo) const
{
    GlobalCooldownList::const_iterator itr = m_GlobalCooldowns.find(spellInfo->StartRecoveryCategory);
    return itr != m_GlobalCooldowns.end() && itr->second.duration && getMSTimeDiff(itr->second.cast_time, getMSTime() + 120) < itr->second.duration;
}

void GlobalCooldownMgr::AddGlobalCooldown(SpellInfo const* spellInfo, uint32 gcd)
{
    m_GlobalCooldowns[spellInfo->StartRecoveryCategory] = GlobalCooldown(gcd, getMSTime());
}

void GlobalCooldownMgr::CancelGlobalCooldown(SpellInfo const* spellInfo)
{
    m_GlobalCooldowns[spellInfo->StartRecoveryCategory].duration = 0;
}

////////////////////////////////////////////////////////////
// Methods of class Unit
Unit::~Unit()
{
    // set current spells as deletable
    for (uint8 i = 0; i < CURRENT_MAX_SPELL; ++i)
        if (m_currentSpells[i])
        {
            m_currentSpells[i]->SetReferencedFromCurrent(false);
            m_currentSpells[i] = NULL;
        }

    _DeleteRemovedAuras();

    delete m_charmInfo;
    delete movespline;

    ASSERT(!m_duringRemoveFromWorld);
    ASSERT(!m_attacking);
    ASSERT(m_attackers.empty());
    ASSERT(m_sharedVision.empty());
    //ASSERT(m_Controlled.empty()); // @todo reimplement this
    ASSERT(m_appliedAuras.empty());
    ASSERT(m_ownedAuras.empty());
    ASSERT(m_removedAuras.empty());
    ASSERT(m_gameObj.empty());
    ASSERT(m_dynObj.empty());
    ASSERT(m_AreaObj.empty());
}

void Unit::Update(uint32 p_time)
{
    // WARNING! Order of execution here is important, do not change.
    // Spells must be processed with event system BEFORE they go to _UpdateSpells.
    // Or else we may have some SPELL_STATE_FINISHED spells stalled in pointers, that is bad.
    m_Events.Update(p_time);

    if (!IsInWorld())
        return;

    if (m_damage_counter_timer <= (int)p_time)
    {
        for (int i = 0; i < MAX_DAMAGE_COUNTERS; ++i)
        {
            m_damage_counters[i].push_front(0);
            while (m_damage_counters[i].size() > MAX_DAMAGE_LOG_SECS)
                m_damage_counters[i].pop_back();
        }

        m_damage_counter_timer = 1 * IN_MILLISECONDS;
    }
    else
        m_damage_counter_timer -= p_time;

    _UpdateSpells(p_time);

    // If this is set during update SetCantProc(false) call is missing somewhere in the code
    // Having this would prevent spells from being proced, so let's crash
    //ASSERT(!m_procDeep);
    if(m_procDeep)
        return;

    if (CanHaveThreatList() && getThreatManager().isNeedUpdateToClient(p_time))
        SendThreatListUpdate();

    // update combat timer only for players and pets (only pets with PetAI)
    if (isInCombat() && (GetTypeId() == TYPEID_PLAYER || (ToCreature()->isPet() && IsControlledByPlayer()) || isMonkClones()))
    {
        //Don`t claer combat state if instance in progress
        Map* map = GetMap();
        InstanceScript* instance = GetInstanceScript();
        bool leaveCombat = true;
        if (map && map->IsRaid() && instance && instance->IsEncounterInProgress())
            leaveCombat = false;

        // Check UNIT_STATE_MELEE_ATTACKING or UNIT_STATE_CHASE (without UNIT_STATE_FOLLOW in this case) so pets can reach far away
        // targets without stopping half way there and running off.
        // These flags are reset after target dies or another command is given.
        if (m_CombatTimer <= p_time) // m_CombatTimer set at aura start and it will be freeze until aura removing
        {
            if (m_HostileRefManager.isEmpty() && leaveCombat)
                ClearInCombat();
            else
                m_CombatTimer = 0;
        }
        else
            m_CombatTimer -= p_time;
    }

    // not implemented before 3.0.2
    if (uint32 base_att = getAttackTimer(BASE_ATTACK))
        setAttackTimer(BASE_ATTACK, (p_time >= base_att ? 0 : base_att - p_time));
    if (uint32 ranged_att = getAttackTimer(RANGED_ATTACK))
        setAttackTimer(RANGED_ATTACK, (p_time >= ranged_att ? 0 : ranged_att - p_time));
    if (uint32 off_att = getAttackTimer(OFF_ATTACK))
        setAttackTimer(OFF_ATTACK, (p_time >= off_att ? 0 : off_att - p_time));

    // update abilities available only for fraction of time
    UpdateReactives(p_time);

    if (isAlive())
    {
        ModifyAuraState(AURA_STATE_HEALTHLESS_20_PERCENT, HealthBelowPct(20));
        ModifyAuraState(AURA_STATE_HEALTHLESS_35_PERCENT, HealthBelowPct(35));
        ModifyAuraState(AURA_STATE_HEALTH_ABOVE_75_PERCENT, HealthAbovePct(75));
    }

    UpdateSplineMovement(p_time);
    i_motionMaster.UpdateMotion(p_time);
}

bool Unit::haveOffhandWeapon() const
{
    if (GetTypeId() == TYPEID_PLAYER)
        return ToPlayer()->GetWeaponForAttack(OFF_ATTACK, true);
    else
        return m_canDualWield;
}

void Unit::MonsterMoveWithSpeed(float x, float y, float z, float speed, bool generatePath, bool forceDestination)
{
    Movement::MoveSplineInit init(*this);
    init.MoveTo(x, y, z, generatePath, forceDestination);
    init.SetVelocity(speed);
    init.Launch();
}

uint32 const positionUpdateDelay = 400;

void Unit::UpdateSplineMovement(uint32 t_diff)
{
    if (movespline->Finalized())
        return;

    movespline->updateState(t_diff);
    bool arrived = movespline->Finalized();

    if (arrived)
        DisableSpline();

    m_movesplineTimer.Update(t_diff);
    if (m_movesplineTimer.Passed() || arrived)
        UpdateSplinePosition();
}

void Unit::UpdateSplinePosition(bool stop/* = false*/)
{
    m_movesplineTimer.Reset(positionUpdateDelay);
    Movement::Location loc = movespline->ComputePosition();
    if (GetTransGUID())
    {
        Position& pos = m_movementInfo.t_pos;
        pos.m_positionX = loc.x;
        pos.m_positionY = loc.y;
        pos.m_positionZ = loc.z;
        pos.SetOrientation(loc.orientation);
        if (Unit* vehicle = GetVehicleBase())
        if (TransportBase* transport = GetDirectTransport())
            transport->CalculatePassengerPosition(loc.x, loc.y, loc.z, loc.orientation);
    }

    if (HasUnitState(UNIT_STATE_CANNOT_TURN))
        loc.orientation = GetOrientation();

    //if (GetTypeId() == TYPEID_PLAYER)
        //sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "UpdateSplinePosition loc(%f %f %f)", loc.x, loc.y, loc.z);

    UpdatePosition(loc.x, loc.y, loc.z, loc.orientation, false, stop);
}

void Unit::DisableSpline()
{
    m_movementInfo.RemoveMovementFlag(MOVEMENTFLAG_FORWARD);
    movespline->_Interrupt();
}

void Unit::resetAttackTimer(WeaponAttackType type)
{
    m_attackTimer[type] = uint32(GetAttackTime(type) * m_modAttackSpeedPct[type]);
}

bool Unit::IsWithinCombatRange(const Unit* obj, float dist2compare) const
{
    if (!obj || !IsInMap(obj) || !InSamePhase(obj))
        return false;

    float dx = GetPositionX() - obj->GetPositionX();
    float dy = GetPositionY() - obj->GetPositionY();
    float dz = GetPositionZ() - obj->GetPositionZ();
    float distsq = dx * dx + dy * dy + dz * dz;

    float sizefactor = GetCombatReach() + obj->GetCombatReach();
    float maxdist = dist2compare + sizefactor;

    return distsq < maxdist * maxdist;
}

bool Unit::IsWithinMeleeRange(const Unit* obj, float dist) const
{
    if (!obj || !IsInMap(obj) || !InSamePhase(obj))
        return false;

    float dx = GetPositionX() - obj->GetPositionX();
    float dy = GetPositionY() - obj->GetPositionY();
    float dz = GetPositionZ() - obj->GetPositionZ();
    float distsq = dx*dx + dy*dy + dz*dz;

    float sizefactor = GetMeleeReach() + obj->GetMeleeReach();
    float maxdist = dist + sizefactor;

    return distsq < maxdist * maxdist;
}

void Unit::GetRandomContactPoint(const Unit* obj, float &x, float &y, float &z, float distance2dMin, float distance2dMax) const
{
    float combat_reach = GetCombatReach();
    if (combat_reach < 0.1f) // sometimes bugged for players
        combat_reach = DEFAULT_COMBAT_REACH;

    uint32 attacker_number = getAttackers().size();
    if (attacker_number > 0)
        --attacker_number;
    GetNearPoint(obj, x, y, z, obj->GetCombatReach(), distance2dMin+(distance2dMax-distance2dMin) * (float)rand_norm()
        , GetAngle(obj) + (attacker_number ? (static_cast<float>(M_PI/2) - static_cast<float>(M_PI) * (float)rand_norm()) * float(attacker_number) / combat_reach * 0.3f : 0));
}

void Unit::UpdateInterruptMask()
{
    m_interruptMask = 0;
    for (AuraApplicationList::const_iterator i = m_interruptableAuras.begin(); i != m_interruptableAuras.end(); ++i)
        m_interruptMask |= (*i)->GetBase()->GetSpellInfo()->AuraInterruptFlags;

    if (Spell* spell = m_currentSpells[CURRENT_CHANNELED_SPELL])
        if (spell->getState() == SPELL_STATE_CASTING)
            m_interruptMask |= spell->m_spellInfo->ChannelInterruptFlags;
}

bool Unit::HasAuraTypeWithFamilyFlags(AuraType auraType, uint32 familyName, uint32 familyFlags) const
{
    if (!HasAuraType(auraType))
        return false;
    AuraEffectList auras = GetAuraEffectsByType(auraType);
    for (AuraEffectList::const_iterator itr = auras.begin(); itr != auras.end(); ++itr)
        if (SpellInfo const* iterSpellProto = (*itr)->GetSpellInfo())
            if (iterSpellProto->SpellFamilyName == familyName && iterSpellProto->SpellFamilyFlags[0] & familyFlags)
                return true;
    return false;
}

bool Unit::HasCrowdControlAuraType(AuraType type, uint32 excludeAura) const
{
    AuraEffectList auras = GetAuraEffectsByType(type);
    for (AuraEffectList::const_iterator itr = auras.begin(); itr != auras.end(); ++itr)
        if ((!excludeAura || excludeAura != (*itr)->GetSpellInfo()->Id) && //Avoid self interrupt of channeled Crowd Control spells like Seduction
            ((*itr)->GetSpellInfo()->Attributes & SPELL_ATTR0_BREAKABLE_BY_DAMAGE || (*itr)->GetSpellInfo()->AuraInterruptFlags & (AURA_INTERRUPT_FLAG_TAKE_DAMAGE)))
            return true;
    return false;
}

bool Unit::HasCrowdControlAura(Unit* excludeCasterChannel) const
{
    uint32 excludeAura = 0;
    if (Spell* currentChanneledSpell = excludeCasterChannel ? excludeCasterChannel->GetCurrentSpell(CURRENT_CHANNELED_SPELL) : NULL)
        excludeAura = currentChanneledSpell->GetSpellInfo()->Id; //Avoid self interrupt of channeled Crowd Control spells like Seduction

    return ( HasCrowdControlAuraType(SPELL_AURA_MOD_CONFUSE, excludeAura)
            //|| HasCrowdControlAuraType(SPELL_AURA_MOD_FEAR, excludeAura)
            //|| HasCrowdControlAuraType(SPELL_AURA_MOD_FEAR_2, excludeAura)
            || HasCrowdControlAuraType(SPELL_AURA_MOD_STUN, excludeAura)
            /*|| HasCrowdControlAuraType(SPELL_AURA_MOD_ROOT, excludeAura)
            || HasCrowdControlAuraType(SPELL_AURA_TRANSFORM, excludeAura)*/);
}

void Unit::DealDamageMods(Unit* victim, uint32 &damage, uint32* absorb)
{
    if (!victim || !victim->isAlive() || victim->HasUnitState(UNIT_STATE_IN_FLIGHT) || (victim->GetTypeId() == TYPEID_UNIT && victim->ToCreature()->IsInEvadeMode()))
    {
        if (absorb)
            *absorb += damage;
        damage = 0;
    }
}

uint32 Unit::DealDamage(Unit* victim, uint32 damage, CleanDamage const* cleanDamage, DamageEffectType damagetype, SpellSchoolMask damageSchoolMask, SpellInfo const* spellProto, bool durabilityLoss)
{
    if(spellProto)
    {
        SpellBonusEntry const* bonus = sSpellMgr->GetSpellBonusData(spellProto->Id);
        if(bonus && bonus->damage_bonus)
            damage *= bonus->damage_bonus;
    }

    // Log damage > 1 000 000 on worldboss
    if (damage > 1000000 && GetTypeId() == TYPEID_PLAYER && victim->GetTypeId() == TYPEID_UNIT && victim->ToCreature()->GetCreatureTemplate()->rank)
        sLog->outWarn(LOG_FILTER_UNITS, "World Boss %u [%s] take more than 1M damage (%u) by player %u [%s] with spell %u", victim->GetEntry(), victim->GetName(), damage, GetGUIDLow(), GetName(), spellProto ? spellProto->Id : 0);

    // Leeching Poison - 112961 each attack heal the player for 10% of the damage
    if (GetTypeId() == TYPEID_PLAYER && getClass() == CLASS_ROGUE && damage != 0)
    {
        if (Aura* leechingPoison = victim->GetAura(112961))
        {
            if (leechingPoison->GetCaster())
            {
                if (leechingPoison->GetCaster()->GetGUID() == GetGUID())
                {
                    int32 bp = damage / 10;
                    CastCustomSpell(this, 112974, &bp, NULL, NULL, true);
                }
            }
        }
    }
    // Spirit Hunt - 58879 : Feral Spirit heal their owner for 150% of their damage
    if (GetOwner() && GetTypeId() == TYPEID_UNIT && GetEntry() == 29264 && damage > 0)
    {
        int32 basepoints = 0;

        // Glyph of Feral Spirit : +40% heal
        if (GetOwner()->HasAura(63271))
            basepoints = CalculatePct(damage, 190);
        else
            basepoints = CalculatePct(damage, 150);

        CastCustomSpell(GetOwner(), 58879, &basepoints, NULL, NULL, true, 0, NULL, GetGUID());
    }
    // Searing Flames - 77657 : Fire Elemental attacks or Searing Totem attacks
    if (GetOwner() && (GetTypeId() == TYPEID_UNIT && (GetEntry() == 15438 || GetEntry() == 61029) && !spellProto) || (isTotem() && GetEntry() == 2523))
        if (GetOwner()->HasAura(77657))
            GetOwner()->CastSpell(GetOwner(), 77661, true);

    if (victim->IsAIEnabled)
        victim->GetAI()->DamageTaken(this, damage);

    if (IsAIEnabled)
        GetAI()->DamageDealt(victim, damage, damagetype);

    if (victim->GetTypeId() == TYPEID_PLAYER)
    {
        if (victim->ToPlayer()->GetCommandStatus(CHEAT_GOD))
            return 0;
        
    // Signal to pets that their owner was attacked
    Pet* pet = victim->ToPlayer()->GetPet();

    if (pet && pet->isAlive())
        pet->AI()->OwnerDamagedBy(this);
    }

    if (victim->GetTypeId() != TYPEID_PLAYER && GetTypeId() != TYPEID_PLAYER && !victim->ToCreature()->isPet() && (victim->ToCreature()->GetCreatureTemplate()->flags_extra & CREATURE_FLAG_EXTRA_HP_80_PERC) && victim->HealthBelowPct(80))
        damage = 0;

    if (damagetype != NODAMAGE)
    {
        // interrupting auras with AURA_INTERRUPT_FLAG_DAMAGE before checking !damage (absorbed damage breaks that type of auras)
        if (spellProto)
        {
            if (!(spellProto->AttributesEx4 & SPELL_ATTR4_DAMAGE_DOESNT_BREAK_AURAS))
                if (damagetype == DOT && damage != 0 || damagetype != DOT)
                    victim->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_TAKE_DAMAGE, spellProto->Id);
        }
        else if (damagetype != SELF_DAMAGE)
        {
            victim->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_TAKE_DAMAGE, 0);
        }

        // We're going to call functions which can modify content of the list during iteration over it's elements
        // Let's copy the list so we can prevent iterator invalidation
        AuraEffectList vCopyDamageCopy(victim->GetAuraEffectsByType(SPELL_AURA_SHARE_DAMAGE_PCT));
        // copy damage to casters of this aura
        for (AuraEffectList::iterator i = vCopyDamageCopy.begin(); i != vCopyDamageCopy.end(); ++i)
        {
            // Check if aura was removed during iteration - we don't need to work on such auras
            if (!((*i)->GetBase()->IsAppliedOnTarget(victim->GetGUID())))
                continue;
            // check damage school mask
            if (((*i)->GetMiscValue() & damageSchoolMask) == 0)
                continue;

            Unit* shareDamageTarget = (*i)->GetCaster();
            if (!shareDamageTarget)
                continue;
            SpellInfo const* spell = (*i)->GetSpellInfo();

            uint32 share = CalculatePct(damage, (*i)->GetAmount());

            // TODO: check packets if damage is done by victim, or by attacker of victim
            DealDamageMods(shareDamageTarget, share, NULL);
            DealDamage(shareDamageTarget, share, NULL, NODAMAGE, spell->GetSchoolMask(), spell, false);
        }
    }

    // Rage from Damage made (only from direct weapon damage)
    if (cleanDamage && damagetype == DIRECT_DAMAGE && this != victim && getPowerType() == POWER_RAGE)
    {
        uint32 rage = uint32(GetAttackTime(cleanDamage->attackType) / 1000 * 8.125f);
        float ragewarrior = float(3.5f * ((GetAttackTime(cleanDamage->attackType)) / 1000.0f) * 0.5f);
        
        switch (cleanDamage->attackType)
        {
            case OFF_ATTACK:
                rage /= 2;
            case BASE_ATTACK:
            {
                if (Player* player = ToPlayer())
                {
                    switch (player->getClass())
                    {
                        case CLASS_WARRIOR:
                        {
                            RewardRage(ragewarrior, true);
                            break;
                        }
                        case CLASS_DRUID:
                        {
                            rage = 4;
                            RewardRage(rage, true);
                            break;
                        }
                    }
                    break;
                }
            }
            default:
                break;
        }
    }

    if (damagetype != NODAMAGE && (damage || (cleanDamage && cleanDamage->absorbed_damage) ))
    {
        if (victim != this && victim->GetTypeId() == TYPEID_PLAYER) // does not support creature push_back
        {
            if (damagetype != DOT || (spellProto && spellProto->IsChanneled()))
            {
                if (Spell* spell = victim->m_currentSpells[CURRENT_GENERIC_SPELL])
                    if (spell->getState() == SPELL_STATE_PREPARING)
                    {
                        uint32 interruptFlags = spell->m_spellInfo->InterruptFlags;
                        if (interruptFlags & SPELL_INTERRUPT_FLAG_ABORT_ON_DMG)
                            victim->InterruptNonMeleeSpells(false);
                    }
            }
        }
    }
    
    if (!damage)
    {
        // Rage from absorbed damage
        if (cleanDamage && cleanDamage->absorbed_damage && victim->getPowerType() == POWER_RAGE)
            victim->RewardRage(cleanDamage->absorbed_damage, false);

        return 0;
    }

    sLog->outDebug(LOG_FILTER_UNITS, "DealDamageStart");

    uint32 health = victim->GetHealth();
    sLog->outInfo(LOG_FILTER_UNITS, "deal dmg:%d to health:%d to %s", damage, health, GetString().c_str());

    // duel ends when player has 1 or less hp
    bool duel_hasEnded = false;
    bool duel_wasMounted = false;
    if (victim->GetTypeId() == TYPEID_PLAYER && victim->ToPlayer()->duel && damage >= (health-1))
    {
        // prevent kill only if killed in duel and killed by opponent or opponent controlled creature
        if (victim->ToPlayer()->duel->opponent == this || victim->ToPlayer()->duel->opponent->GetGUID() == GetOwnerGUID())
            damage = health - 1;

        duel_hasEnded = true;
    }
    else if (victim->IsVehicle() && damage >= (health-1) && victim->GetCharmer() && victim->GetCharmer()->GetTypeId() == TYPEID_PLAYER)
    {
        Player* victimRider = victim->GetCharmer()->ToPlayer();

        if (victimRider && victimRider->duel && victimRider->duel->isMounted)
        {
            // prevent kill only if killed in duel and killed by opponent or opponent controlled creature
            if (victimRider->duel->opponent == this || victimRider->duel->opponent->GetGUID() == GetCharmerGUID())
                damage = health - 1;

            duel_wasMounted = true;
            duel_hasEnded = true;
        }
    }

    if (GetTypeId() == TYPEID_PLAYER && this != victim)
    {
        Player* killer = ToPlayer();

        // in bg, count dmg if victim is also a player
        if (victim->GetTypeId() == TYPEID_PLAYER)
            if (Battleground* bg = killer->GetBattleground())
            {
                bg->UpdatePlayerScore(killer, SCORE_DAMAGE_DONE, damage);
                /** World of Warcraft Armory **/
                if (Battleground *bgV = ((Player*)victim)->GetBattleground())
                    bgV->UpdatePlayerScore(((Player*)victim), SCORE_DAMAGE_TAKEN, damage);
                /** World of Warcraft Armory **/
            }

        killer->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_DAMAGE_DONE, damage, 0, 0, victim);
        killer->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_HIT_DEALT, damage);
    }
    else if (GetTypeId() == TYPEID_UNIT && this != victim && isPet())
    {
        if (GetOwner() && GetOwner()->ToPlayer())
        {
            Player* killerOwner = GetOwner()->ToPlayer();

            if (victim->GetTypeId() == TYPEID_PLAYER)
                if (Battleground* bg = killerOwner->GetBattleground())
                    bg->UpdatePlayerScore(killerOwner, SCORE_DAMAGE_DONE, damage);
        }
    }

    if (victim->GetTypeId() == TYPEID_PLAYER)
        victim->ToPlayer()->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_HIT_RECEIVED, damage);
    else if (!victim->IsControlledByPlayer() || victim->IsVehicle())
    {
        if (!victim->ToCreature()->hasLootRecipient())
            victim->ToCreature()->SetLootRecipient(this);

        if (IsControlledByPlayer())
            victim->ToCreature()->LowerPlayerDamageReq(health < damage ?  health : damage);
    }

    if (damagetype == DIRECT_DAMAGE || damagetype == SPELL_DIRECT_DAMAGE)
    {
        m_damage_counters[DAMAGE_DONE_COUNTER][0] += health >= damage ? damage : health;
        victim->m_damage_counters[DAMAGE_TAKEN_COUNTER][0] += health >= damage ? damage : health;
    }

    if (health <= damage)
    {
        sLog->outDebug(LOG_FILTER_UNITS, "DealDamage: victim just died");

        if (victim->GetTypeId() == TYPEID_PLAYER && victim != this)
        {
            victim->ToPlayer()->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_TOTAL_DAMAGE_RECEIVED, health);

            // call before auras are removed
            if (Player* killer = GetCharmerOrOwnerPlayerOrPlayerItself())
                killer->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_SPECIAL_PVP_KILL, 1, 0, 0, victim);
        }

        if (spellProto && (spellProto->AttributesEx9 & SPELL_ATTR9_UNK28))
            victim->SetHealth(1);
        else
            Kill(victim, durabilityLoss, spellProto ? spellProto : NULL);
    }
    else
    {
        sLog->outDebug(LOG_FILTER_UNITS, "DealDamageAlive");

        if (victim->GetTypeId() == TYPEID_PLAYER)
            victim->ToPlayer()->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_TOTAL_DAMAGE_RECEIVED, damage);

        victim->ModifyHealth(- (int32)damage);

        if (damagetype == DIRECT_DAMAGE || damagetype == SPELL_DIRECT_DAMAGE)
            victim->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_DIRECT_DAMAGE, spellProto ? spellProto->Id : 0);

        if (victim->GetTypeId() != TYPEID_PLAYER)
            victim->AddThreat(this, float(damage), damageSchoolMask, spellProto);
        else                                                // victim is a player
        {
            // random durability for items (HIT TAKEN)
            if (roll_chance_f(sWorld->getRate(RATE_DURABILITY_LOSS_DAMAGE)))
            {
                EquipmentSlots slot = EquipmentSlots(urand(0, EQUIPMENT_SLOT_END-1));
                victim->ToPlayer()->DurabilityPointLossForEquipSlot(slot);
            }
        }

        // Rage from damage received
        if (this != victim && victim->getPowerType() == POWER_RAGE)
        {
            uint32 rage_damage = damage + (cleanDamage ? cleanDamage->absorbed_damage : 0);
            victim->RewardRage(rage_damage, false);
        }

        if (GetTypeId() == TYPEID_PLAYER)
        {
            // random durability for items (HIT DONE)
            if (roll_chance_f(sWorld->getRate(RATE_DURABILITY_LOSS_DAMAGE)))
            {
                EquipmentSlots slot = EquipmentSlots(urand(0, EQUIPMENT_SLOT_END-1));
                ToPlayer()->DurabilityPointLossForEquipSlot(slot);
            }
        }

        if (damagetype != NODAMAGE && damage)
        {
            if (victim != this && victim->GetTypeId() == TYPEID_PLAYER && // does not support creature push_back
                (!spellProto || !(spellProto->AttributesEx7 & SPELL_ATTR7_NO_PUSHBACK_ON_DAMAGE)))
            {
                if (damagetype != DOT)
                    if (Spell* spell = victim->m_currentSpells[CURRENT_GENERIC_SPELL])
                        if (spell->getState() == SPELL_STATE_PREPARING)
                        {
                            uint32 interruptFlags = spell->m_spellInfo->InterruptFlags;
                            if (interruptFlags & SPELL_INTERRUPT_FLAG_ABORT_ON_DMG)
                                victim->InterruptNonMeleeSpells(false);
                            else if (interruptFlags & SPELL_INTERRUPT_FLAG_PUSH_BACK)
                                spell->Delayed();
                        }

                if (Spell* spell = victim->m_currentSpells[CURRENT_CHANNELED_SPELL])
                    if (spell->getState() == SPELL_STATE_CASTING)
                    {
                        uint32 channelInterruptFlags = spell->m_spellInfo->ChannelInterruptFlags;
                        if (((channelInterruptFlags & CHANNEL_FLAG_DELAY) != 0) && (damagetype != DOT))
                            spell->DelayedChannel();
                    }
            }
        }

        // last damage from duel opponent
        if (duel_hasEnded)
        {
            Player* he = duel_wasMounted ? victim->GetCharmer()->ToPlayer() : victim->ToPlayer();

            ASSERT(he && he->duel);

            if (duel_wasMounted) // In this case victim==mount
                victim->SetHealth(1);
            else
                he->SetHealth(1);

            he->duel->opponent->CombatStopWithPets(true);
            he->CombatStopWithPets(true);

            he->CastSpell(he, 7267, true);                  // beg
            he->DuelComplete(DUEL_WON);
        }
    }

    sLog->outDebug(LOG_FILTER_UNITS, "DealDamageEnd returned %d damage", damage);

    return damage;
}

uint32 Unit::CalcStaggerDamage(uint32 damage, SpellSchoolMask damageSchoolMask, SpellInfo const* spellInfo)
{
    if (GetTypeId() == TYPEID_PLAYER && damageSchoolMask == SPELL_SCHOOL_MASK_NORMAL && damage > 0)
    {
        uint32 staggerBleed  = 124255;
        uint32 staggerGreen  = 124275;
        uint32 staggerYellow = 124274;
        uint32 staggerRed    = 124273;
        uint32 staggerDebuf  = 124275;
        float stagger = 0.0f;
        int32 bp0 = 0;
        int32 bp1 = 0;

        if (spellInfo && spellInfo->Id == staggerBleed)
        {
            if (Aura* staggerBleedAura = GetAura(staggerBleed))
            {
                AuraEffect* eff0 = staggerBleedAura->GetEffect(EFFECT_0);
                AuraEffect* eff1 = staggerBleedAura->GetEffect(EFFECT_1);
                bp0 = eff0->GetAmount();
                bp1 = eff1->GetAmount() - damage;

                if (bp1 <= 0)
                {
                    RemoveAurasDueToSpell(staggerBleed);
                    return damage;
                }
                else
                {
                    eff1->SetAmount(bp1);

                    if (bp1 > int32(CountPctFromMaxHealth(60)))
                        staggerDebuf = staggerRed;
                    else if (bp1 > int32(CountPctFromMaxHealth(30)))
                        staggerDebuf = staggerYellow;
                }
            }

            if (Aura* staggerGreenAura = GetAura(staggerGreen))
            {
                if (staggerDebuf != staggerGreen || bp1 <= 0)
                {
                    int32 olddur = staggerGreenAura->GetDuration();
                    RemoveAurasDueToSpell(staggerGreen);
                    CastCustomSpell(this, staggerDebuf, &bp0, &bp1, NULL, true);

                    if (Aura* staggerDebufAura = GetAura(staggerDebuf))
                    {
                        staggerDebufAura->SetDuration(olddur);
                        staggerDebufAura->SetMaxDuration(olddur);
                    }
                }
                else
                {
                    staggerGreenAura->GetEffect(EFFECT_1)->SetAmount(bp1);
                }
            }
            else if (Aura* staggerYellowAura = GetAura(staggerYellow))
            {
                if (staggerDebuf != staggerYellow || bp1 <= 0)
                {
                    int32 olddur = staggerYellowAura->GetDuration();
                    RemoveAurasDueToSpell(staggerYellow);
                    CastCustomSpell(this, staggerDebuf, &bp0, &bp1, NULL, true);

                    if (Aura* staggerDebufAura = GetAura(staggerDebuf))
                    {
                        staggerDebufAura->SetDuration(olddur);
                        staggerDebufAura->SetMaxDuration(olddur);
                    }
                }
                else
                {
                    staggerYellowAura->GetEffect(EFFECT_1)->SetAmount(bp1);
                }
            }
            else if (Aura* staggerRedAura = GetAura(staggerRed))
            {
                if (staggerDebuf != staggerRed || bp1 <= 0)
                {
                    int32 olddur = staggerRedAura->GetDuration();
                    RemoveAurasDueToSpell(staggerRed);
                    CastCustomSpell(this, staggerDebuf, &bp0, &bp1, NULL, true);

                    if (Aura* staggerDebufAura = GetAura(staggerDebuf))
                    {
                        staggerDebufAura->SetDuration(olddur);
                        staggerDebufAura->SetMaxDuration(olddur);
                    }
                }
                else
                {
                    staggerRedAura->GetEffect(EFFECT_1)->SetAmount(bp1);
                }
            }
        }
        else
        {
            if (HasAura(117967))
            {
                if (Aura* Stance_of_the_Sturdy_OxAura = GetAura(115069))
                    stagger += float(Stance_of_the_Sturdy_OxAura->GetSpellInfo()->Effects[EFFECT_7].BasePoints);

                if (AuraEffect const* mastery = GetAuraEffect(117906, EFFECT_0))
                    stagger += float(mastery->GetAmount());

                if (AuraEffect const* Fortifying_Brew = GetAuraEffect(120954, EFFECT_4))
                    stagger += float(Fortifying_Brew->GetAmount());

                if (AuraEffect const* Shuffle = GetAuraEffect(115307, EFFECT_1))
                    stagger += float(Shuffle->GetAmount());

                if (stagger > 100.0f)
                    stagger = 100.0f;

                
                bp0 = RoundingFloatValue(float(CalculatePct(damage, stagger) / 10.0f));
                bp1 = bp0 * 10;

                damage -= bp1;

                if (Aura* staggerBleedAura = GetAura(staggerBleed))
                {
                    AuraEffect* eff0 = staggerBleedAura->GetEffect(EFFECT_0);
                    AuraEffect* eff1 = staggerBleedAura->GetEffect(EFFECT_1);

                    bp0 = RoundingFloatValue(float(bp1 + eff1->GetAmount()) / 10.0f);
                    bp1 = bp0 * 10;

                    if (bp1 > int32(CountPctFromMaxHealth(60)))
                        staggerDebuf = staggerRed;
                    else if (bp1 > int32(CountPctFromMaxHealth(30)))
                        staggerDebuf = staggerYellow;

                    eff0->SetAmount(bp0);
                    eff1->SetAmount(bp1);

                    if (HasAura(staggerGreen))
                    {
                        if (staggerDebuf != staggerGreen)
                        {
                            RemoveAurasDueToSpell(staggerGreen);
                            CastCustomSpell(this, staggerDebuf, &bp0, &bp1, NULL, true);
                        }
                    }
                    else if (HasAura(staggerYellow))
                    {
                        if (staggerDebuf != staggerYellow)
                        {
                            RemoveAurasDueToSpell(staggerYellow);
                            CastCustomSpell(this, staggerDebuf, &bp0, &bp1, NULL, true);
                        }
                    }
                    else if (HasAura(staggerRed))
                    {
                        if (staggerDebuf != staggerRed)
                        {
                            RemoveAurasDueToSpell(staggerRed);
                            CastCustomSpell(this, staggerDebuf, &bp0, &bp1, NULL, true);
                        }
                    }

                    if (Aura* staggerDebufAura = GetAura(staggerDebuf))
                    {
                        staggerDebufAura->GetEffect(EFFECT_0)->SetAmount(bp0);
                        staggerDebufAura->GetEffect(EFFECT_1)->SetAmount(bp1);
                        int32 dur = 10000 - eff0->GetAmplitude() + eff0->GetPeriodicTimer();
                        staggerDebufAura->SetMaxDuration(dur);
                        staggerDebufAura->SetDuration(dur);
                    }
                }
                else
                {
                    if (bp1 > int32(CountPctFromMaxHealth(60)))
                        staggerDebuf = staggerRed;
                    else if (bp1 > int32(CountPctFromMaxHealth(30)))
                        staggerDebuf = staggerYellow;

                    if (bp0 > 0)
                        CastCustomSpell(this, staggerBleed, &bp0, &bp1, NULL, true);

                    CastCustomSpell(this, staggerDebuf, &bp0, &bp1, NULL, true);
                }
            }
        }
    }
    return damage;
}

void Unit::CastStop(uint32 except_spellid)
{
    for (uint32 i = CURRENT_FIRST_NON_MELEE_SPELL; i < CURRENT_MAX_SPELL; i++)
        if (m_currentSpells[i] && m_currentSpells[i]->m_spellInfo->Id != except_spellid)
            InterruptSpell(CurrentSpellTypes(i), false);
}

void Unit::CastSpell(SpellCastTargets const& targets, SpellInfo const* spellInfo, CustomSpellValues const* value, TriggerCastFlags triggerFlags, Item* castItem, AuraEffect const* triggeredByAura, uint64 originalCaster)
{
    if (!spellInfo)
    {
        sLog->outError(LOG_FILTER_UNITS, "CastSpell: unknown spell by caster: %s %u)", (GetTypeId() == TYPEID_PLAYER ? "player (GUID:" : "creature (Entry:"), (GetTypeId() == TYPEID_PLAYER ? GetGUIDLow() : GetEntry()));
        return;
    }

    // TODO: this is a workaround - not needed anymore, but required for some scripts :(
    if (!originalCaster && triggeredByAura)
        originalCaster = triggeredByAura->GetCasterGUID();

    Spell* spell = new Spell(this, spellInfo, triggerFlags, originalCaster);

    if (value)
        for (CustomSpellValues::const_iterator itr = value->begin(); itr != value->end(); ++itr)
            spell->SetSpellValue(itr->first, itr->second, itr->second);

    spell->m_CastItem = castItem;
    spell->prepare(&targets, triggeredByAura);
}

void Unit::CastSpell(Unit* victim, uint32 spellId, bool triggered, Item* castItem, AuraEffect const* triggeredByAura, uint64 originalCaster)
{
    CastSpell(victim, spellId, triggered ? TRIGGERED_FULL_MASK : TRIGGERED_NONE, castItem, triggeredByAura, originalCaster);
}

void Unit::CastSpell(Unit* victim, uint32 spellId, TriggerCastFlags triggerFlags /*= TRIGGER_NONE*/, Item* castItem /*= NULL*/, AuraEffect const* triggeredByAura /*= NULL*/, uint64 originalCaster /*= 0*/)
{
    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId);
    if (!spellInfo)
    {
        sLog->outError(LOG_FILTER_UNITS, "CastSpell: unknown spell id %u by caster: %s %u)", spellId, (GetTypeId() == TYPEID_PLAYER ? "player (GUID:" : "creature (Entry:"), (GetTypeId() == TYPEID_PLAYER ? GetGUIDLow() : GetEntry()));
        return;
    }

    CastSpell(victim, spellInfo, triggerFlags, castItem, triggeredByAura, originalCaster);
}

void Unit::CastSpell(Unit* victim, SpellInfo const* spellInfo, bool triggered, Item* castItem/*= NULL*/, AuraEffect const* triggeredByAura /*= NULL*/, uint64 originalCaster /*= 0*/)
{
    CastSpell(victim, spellInfo, triggered ? TRIGGERED_FULL_MASK : TRIGGERED_NONE, castItem, triggeredByAura, originalCaster);
}

void Unit::CastSpell(Unit* victim, SpellInfo const* spellInfo, TriggerCastFlags triggerFlags, Item* castItem, AuraEffect const* triggeredByAura, uint64 originalCaster)
{
    SpellCastTargets targets;
    targets.SetUnitTarget(victim);
    CastSpell(targets, spellInfo, NULL, triggerFlags, castItem, triggeredByAura, originalCaster);
}

void Unit::CastCustomSpell(Unit* target, uint32 spellId, int32 const* bp0, int32 const* bp1, int32 const* bp2, bool triggered, Item* castItem, AuraEffect const* triggeredByAura, uint64 originalCaster)
{
    CustomSpellValues values;
    if (bp0)
        values.AddSpellMod(SPELLVALUE_BASE_POINT0, *bp0);
    if (bp1)
        values.AddSpellMod(SPELLVALUE_BASE_POINT1, *bp1);
    if (bp2)
        values.AddSpellMod(SPELLVALUE_BASE_POINT2, *bp2);
    CastCustomSpell(spellId, values, target, triggered, castItem, triggeredByAura, originalCaster);
}

void Unit::CastCustomSpell(Unit* target, uint32 spellId, int32 const* bp0, int32 const* bp1, int32 const* bp2, int32 const* bp3, int32 const* bp4, int32 const* bp5, bool triggered, Item* castItem, AuraEffect const* triggeredByAura, uint64 originalCaster)
{
    CustomSpellValues values;
    if (bp0)
        values.AddSpellMod(SPELLVALUE_BASE_POINT0, *bp0);
    if (bp1)
        values.AddSpellMod(SPELLVALUE_BASE_POINT1, *bp1);
    if (bp2)
        values.AddSpellMod(SPELLVALUE_BASE_POINT2, *bp2);
    if (bp3)
        values.AddSpellMod(SPELLVALUE_BASE_POINT3, *bp3);
    if (bp4)
        values.AddSpellMod(SPELLVALUE_BASE_POINT4, *bp4);
    if (bp5)
        values.AddSpellMod(SPELLVALUE_BASE_POINT5, *bp5);
    CastCustomSpell(spellId, values, target, triggered, castItem, triggeredByAura, originalCaster);
}

void Unit::CastCustomSpell(uint32 spellId, SpellValueMod mod, int32 value, Unit* target, bool triggered, Item* castItem, AuraEffect const* triggeredByAura, uint64 originalCaster)
{
    CustomSpellValues values;
    values.AddSpellMod(mod, value);
    CastCustomSpell(spellId, values, target, triggered, castItem, triggeredByAura, originalCaster);
}

void Unit::CastCustomSpell(uint32 spellId, CustomSpellValues const& value, Unit* victim, bool triggered, Item* castItem, AuraEffect const* triggeredByAura, uint64 originalCaster)
{
    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId);
    if (!spellInfo)
    {
        sLog->outError(LOG_FILTER_UNITS, "CastSpell: unknown spell id %u by caster: %s %u)", spellId, (GetTypeId() == TYPEID_PLAYER ? "player (GUID:" : "creature (Entry:"), (GetTypeId() == TYPEID_PLAYER ? GetGUIDLow() : GetEntry()));
        return;
    }
    SpellCastTargets targets;
    targets.SetUnitTarget(victim);

    CastSpell(targets, spellInfo, &value, triggered ? TRIGGERED_FULL_MASK : TRIGGERED_NONE, castItem, triggeredByAura, originalCaster);
}

void Unit::CastSpell(float x, float y, float z, uint32 spellId, bool triggered, Item* castItem, AuraEffect const* triggeredByAura, uint64 originalCaster)
{
    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId);
    if (!spellInfo)
    {
        sLog->outError(LOG_FILTER_UNITS, "CastSpell: unknown spell id %u by caster: %s %u)", spellId, (GetTypeId() == TYPEID_PLAYER ? "player (GUID:" : "creature (Entry:"), (GetTypeId() == TYPEID_PLAYER ? GetGUIDLow() : GetEntry()));
        return;
    }
    SpellCastTargets targets;
    targets.SetDst(x, y, z, GetOrientation());

    CastSpell(targets, spellInfo, NULL, triggered ? TRIGGERED_FULL_MASK : TRIGGERED_NONE, castItem, triggeredByAura, originalCaster);
}

void Unit::CastSpell(float x, float y, float z, uint32 spellId, TriggerCastFlags triggeredCastFlags, Item* castItem, AuraEffect const* triggeredByAura, uint64 originalCaster)
{
    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId);
    if (!spellInfo)
    {
        sLog->outError(LOG_FILTER_UNITS, "CastSpell: unknown spell id %u by caster: %s %u)", spellId, (GetTypeId() == TYPEID_PLAYER ? "player (GUID:" : "creature (Entry:"), (GetTypeId() == TYPEID_PLAYER ? GetGUIDLow() : GetEntry()));
        return;
    }
    SpellCastTargets targets;
    targets.SetDst(x, y, z, GetOrientation());

    CastSpell(targets, spellInfo, NULL, triggeredCastFlags, castItem, triggeredByAura, originalCaster);
}

void Unit::CastSpell(GameObject* go, uint32 spellId, bool triggered, Item* castItem, AuraEffect* triggeredByAura, uint64 originalCaster)
{
    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId);
    if (!spellInfo)
    {
        sLog->outError(LOG_FILTER_UNITS, "CastSpell: unknown spell id %u by caster: %s %u)", spellId, (GetTypeId() == TYPEID_PLAYER ? "player (GUID:" : "creature (Entry:"), (GetTypeId() == TYPEID_PLAYER ? GetGUIDLow() : GetEntry()));
        return;
    }
    SpellCastTargets targets;
    targets.SetGOTarget(go);

    CastSpell(targets, spellInfo, NULL, triggered ? TRIGGERED_FULL_MASK : TRIGGERED_NONE, castItem, triggeredByAura, originalCaster);
}

void Unit::CalculateSpellDamageTaken(SpellNonMeleeDamage* damageInfo, int32 damage, SpellInfo const* spellInfo, uint32 effectMask, WeaponAttackType attackType, bool crit)
{
    if (damage < 0)
        return;

    Unit* victim = damageInfo->target;
    if (!victim || !victim->isAlive())
        return;

    SpellSchoolMask damageSchoolMask = SpellSchoolMask(damageInfo->schoolMask);

    if (IsDamageReducedByArmor(damageSchoolMask, spellInfo, effectMask))
        damage = CalcArmorReducedDamage(victim, damage, spellInfo, attackType);

    int32 sourceDamage = damage;

    bool blocked = false;
    // Per-school calc
    switch (spellInfo->DmgClass)
    {
        // Melee and Ranged Spells
        case SPELL_DAMAGE_CLASS_RANGED:
        case SPELL_DAMAGE_CLASS_MELEE:
        {
            // Physical Damage
            if (damageSchoolMask & SPELL_SCHOOL_MASK_NORMAL)
            {
                // Get blocked status
                blocked = isSpellBlocked(victim, spellInfo, attackType);
            }

            if (crit)
            {
                damageInfo->HitInfo |= SPELL_HIT_TYPE_CRIT;

                // Calculate crit bonus
                uint32 crit_bonus = damage;
                // Apply crit_damage bonus for melee spells
                if (Player* modOwner = GetSpellModOwner())
                    modOwner->ApplySpellMod(spellInfo->Id, SPELLMOD_CRIT_DAMAGE_BONUS, crit_bonus);
                damage += crit_bonus;

                // Apply SPELL_AURA_MOD_ATTACKER_RANGED_CRIT_DAMAGE or SPELL_AURA_MOD_ATTACKER_MELEE_CRIT_DAMAGE
                float critPctDamageMod = 0.0f;
                if (attackType == RANGED_ATTACK)
                    critPctDamageMod += victim->GetTotalAuraModifier(SPELL_AURA_MOD_ATTACKER_RANGED_CRIT_DAMAGE);
                else
                    critPctDamageMod += victim->GetTotalAuraModifier(SPELL_AURA_MOD_ATTACKER_MELEE_CRIT_DAMAGE);

                // Increase crit damage from SPELL_AURA_MOD_CRIT_DAMAGE_BONUS
                critPctDamageMod += (GetTotalAuraMultiplierByMiscMask(SPELL_AURA_MOD_CRIT_DAMAGE_BONUS, spellInfo->GetSchoolMask()) - 1.0f) * 100;

                if (critPctDamageMod != 0)
                    AddPct(damage, critPctDamageMod);
            }

            // Spell weapon based damage CAN BE crit & blocked at same time
            if (blocked)
            {
                // double blocked amount if block is critical
                uint32 value = victim->GetBlockPercent();
                if (victim->isBlockCritical())
                    value *= 2; // double blocked percent
                damageInfo->blocked = CalculatePct(damage, value);
                damage -= damageInfo->blocked;
                sourceDamage -= damageInfo->blocked;
            }

            ApplyResilience(victim, &damage, crit);
            break;
        }
        // Magical Attacks
        case SPELL_DAMAGE_CLASS_NONE:
        case SPELL_DAMAGE_CLASS_MAGIC:
        {
            // If crit add critical bonus
            if (crit)
            {
                damageInfo->HitInfo |= SPELL_HIT_TYPE_CRIT;
                damage = SpellCriticalDamageBonus(spellInfo, damage, victim);
            }

            ApplyResilience(victim, &damage, crit);
            break;
        }
        default:
            break;
    }

    if (victim->getClass() == CLASS_MONK)
        damage = victim->CalcStaggerDamage(damage, damageSchoolMask, spellInfo);

    if (spellInfo && spellInfo->AttributesEx6 & SPELL_ATTR6_NO_DONE_PCT_DAMAGE_MODS)
    {
        if (sourceDamage > 0)
        {
            CalcAbsorbResist(victim, damageSchoolMask, SPELL_DIRECT_DAMAGE, sourceDamage, &damageInfo->absorb, &damageInfo->resist, spellInfo);
            sourceDamage -= damageInfo->absorb + damageInfo->resist;
        }
        damageInfo->damage = sourceDamage;
        return;
    }

    // Calculate absorb resist
    if (damage > 0)
    {
        CalcAbsorbResist(victim, damageSchoolMask, SPELL_DIRECT_DAMAGE, damage, &damageInfo->absorb, &damageInfo->resist, spellInfo);
        damage -= damageInfo->absorb + damageInfo->resist;
    }
    else
        damage = 0;

    damageInfo->damage = damage;
}

void Unit::DealSpellDamage(SpellNonMeleeDamage* damageInfo, bool durabilityLoss)
{
    if (damageInfo == 0)
        return;

    Unit* victim = damageInfo->target;

    if (!victim)
        return;

    if (!victim->isAlive() || victim->HasUnitState(UNIT_STATE_IN_FLIGHT) || (victim->GetTypeId() == TYPEID_UNIT && victim->ToCreature()->IsInEvadeMode()))
        return;

    SpellInfo const* spellProto = sSpellMgr->GetSpellInfo(damageInfo->SpellID);
    if (spellProto == NULL)
    {
        sLog->outDebug(LOG_FILTER_UNITS, "Unit::DealSpellDamage has wrong damageInfo->SpellID: %u", damageInfo->SpellID);
        return;
    }

    // Call default DealDamage
    CleanDamage cleanDamage(damageInfo->cleanDamage, damageInfo->absorb, BASE_ATTACK, MELEE_HIT_NORMAL);
    DealDamage(victim, damageInfo->damage, &cleanDamage, SPELL_DIRECT_DAMAGE, SpellSchoolMask(damageInfo->schoolMask), spellProto, durabilityLoss);
}

// TODO for melee need create structure as in
void Unit::CalculateMeleeDamage(Unit* victim, uint32 damage, CalcDamageInfo* damageInfo, WeaponAttackType attackType)
{
    damageInfo->attacker         = this;
    damageInfo->target           = victim;
    damageInfo->damageSchoolMask = GetMeleeDamageSchoolMask();
    damageInfo->attackType       = attackType;
    damageInfo->damage           = 0;
    damageInfo->cleanDamage      = 0;
    damageInfo->damageBeforeHit  = 0;
    damageInfo->absorb           = 0;
    damageInfo->resist           = 0;
    damageInfo->blocked_amount   = 0;

    damageInfo->TargetState      = 0;
    damageInfo->HitInfo          = 0;
    damageInfo->procAttacker     = PROC_FLAG_NONE;
    damageInfo->procVictim       = PROC_FLAG_NONE;
    damageInfo->procEx           = PROC_EX_NONE;
    damageInfo->hitOutCome       = MELEE_HIT_EVADE;

    if (!victim)
        return;

    if (!isAlive() || !victim->isAlive())
        return;

    // Select HitInfo/procAttacker/procVictim flag based on attack type
    switch (attackType)
    {
        case BASE_ATTACK:
            damageInfo->procAttacker = PROC_FLAG_DONE_MELEE_AUTO_ATTACK | PROC_FLAG_DONE_MAINHAND_ATTACK;
            damageInfo->procVictim   = PROC_FLAG_TAKEN_MELEE_AUTO_ATTACK;
            break;
        case OFF_ATTACK:
            damageInfo->procAttacker = PROC_FLAG_DONE_MELEE_AUTO_ATTACK | PROC_FLAG_DONE_OFFHAND_ATTACK;
            damageInfo->procVictim   = PROC_FLAG_TAKEN_MELEE_AUTO_ATTACK;
            damageInfo->HitInfo      = HITINFO_OFFHAND;
            break;
        default:
            return;
    }

    // Physical Immune check
    if (damageInfo->target->IsImmunedToDamage(SpellSchoolMask(damageInfo->damageSchoolMask)))
    {
       damageInfo->HitInfo       |= HITINFO_NORMALSWING;
       damageInfo->TargetState    = VICTIMSTATE_IS_IMMUNE;

       damageInfo->procEx        |= PROC_EX_IMMUNE;
       damageInfo->damage         = 0;
       damageInfo->cleanDamage    = 0;
       return;
    }

    damage += CalculateDamage(damageInfo->attackType, false, true);

    // Add melee damage bonus
    damage = MeleeDamageBonusDone(damageInfo->target, damage, damageInfo->attackType);
    damageInfo->damageBeforeHit = damageInfo->target->MeleeDamageBonusForDamageBeforeHit(this, damage, damageInfo->attackType);
    damage = damageInfo->target->MeleeDamageBonusTaken(this, damage, damageInfo->attackType);

    // Calculate armor reduction
    if (IsDamageReducedByArmor((SpellSchoolMask)(damageInfo->damageSchoolMask)))
    {
        damageInfo->damage = CalcArmorReducedDamage(damageInfo->target, damage, NULL, damageInfo->attackType);
        damageInfo->cleanDamage += damage - damageInfo->damage;
    }
    else
        damageInfo->damage = damage;

    if (Unit* owner = GetAnyOwner()) //For pets chance calc from owner
        damageInfo->hitOutCome = owner->RollMeleeOutcomeAgainst(damageInfo->target, damageInfo->attackType);
    else
        damageInfo->hitOutCome = RollMeleeOutcomeAgainst(damageInfo->target, damageInfo->attackType);

    switch (damageInfo->hitOutCome)
    {
        case MELEE_HIT_EVADE:
            damageInfo->HitInfo        |= HITINFO_MISS | HITINFO_SWINGNOHITSOUND;
            damageInfo->TargetState     = VICTIMSTATE_EVADES;
            damageInfo->procEx         |= PROC_EX_EVADE;
            damageInfo->damage = 0;
            damageInfo->cleanDamage = 0;
            return;
        case MELEE_HIT_MISS:
            damageInfo->HitInfo        |= HITINFO_MISS;
            damageInfo->TargetState     = VICTIMSTATE_INTACT;
            damageInfo->procEx         |= PROC_EX_MISS;
            damageInfo->damage          = 0;
            damageInfo->cleanDamage     = 0;
            break;
        case MELEE_HIT_NORMAL:
            damageInfo->TargetState     = VICTIMSTATE_HIT;
            damageInfo->procEx         |= PROC_EX_NORMAL_HIT;
            break;
        case MELEE_HIT_CRIT:
        {
            damageInfo->HitInfo        |= HITINFO_CRITICALHIT;
            damageInfo->TargetState     = VICTIMSTATE_HIT;

            damageInfo->procEx         |= PROC_EX_CRITICAL_HIT;
            // Crit bonus calc
            damageInfo->damage += damageInfo->damage;
            damageInfo->damageBeforeHit += damageInfo->damageBeforeHit;
            float mod = 0.0f;
            // Apply SPELL_AURA_MOD_ATTACKER_RANGED_CRIT_DAMAGE or SPELL_AURA_MOD_ATTACKER_MELEE_CRIT_DAMAGE
            if (damageInfo->attackType == RANGED_ATTACK)
                mod += damageInfo->target->GetTotalAuraModifier(SPELL_AURA_MOD_ATTACKER_RANGED_CRIT_DAMAGE);
            else
                mod += damageInfo->target->GetTotalAuraModifier(SPELL_AURA_MOD_ATTACKER_MELEE_CRIT_DAMAGE);

            // Increase crit damage from SPELL_AURA_MOD_CRIT_DAMAGE_BONUS
            mod += (GetTotalAuraMultiplierByMiscMask(SPELL_AURA_MOD_CRIT_DAMAGE_BONUS, damageInfo->damageSchoolMask) - 1.0f) * 100;

            if (mod != 0)
                AddPct(damageInfo->damage, mod);
            break;
        }
        case MELEE_HIT_PARRY:
            damageInfo->TargetState  = VICTIMSTATE_PARRY;
            damageInfo->procEx      |= PROC_EX_PARRY;
            damageInfo->cleanDamage += damageInfo->damage;
            damageInfo->damage = 0;
            break;
        case MELEE_HIT_DODGE:
            damageInfo->TargetState  = VICTIMSTATE_DODGE;
            damageInfo->procEx      |= PROC_EX_DODGE;
            damageInfo->cleanDamage += damageInfo->damage;
            damageInfo->damage = 0;
            break;
        case MELEE_HIT_BLOCK:
            damageInfo->TargetState = VICTIMSTATE_HIT;
            damageInfo->HitInfo    |= HITINFO_BLOCK;
            damageInfo->procEx     |= PROC_EX_BLOCK | PROC_EX_NORMAL_HIT;
            // 30% damage blocked, double blocked amount if block is critical
            damageInfo->blocked_amount = CalculatePct(damageInfo->damage, damageInfo->target->isBlockCritical() ? damageInfo->target->GetBlockPercent() * 2 : damageInfo->target->GetBlockPercent());
            damageInfo->damage      -= damageInfo->blocked_amount;
            damageInfo->cleanDamage += damageInfo->blocked_amount;
            break;
        case MELEE_HIT_GLANCING:
        {
            damageInfo->HitInfo     |= HITINFO_GLANCING;
            damageInfo->TargetState  = VICTIMSTATE_HIT;
            damageInfo->procEx      |= PROC_EX_NORMAL_HIT;
            int32 leveldif = int32(victim->getLevel()) - int32(getLevel());
            if (leveldif > 3)
                leveldif = 3;
            float reducePercent = 1 - leveldif * 0.1f;
            damageInfo->cleanDamage += damageInfo->damage - uint32(reducePercent * damageInfo->damage);
            damageInfo->damage = uint32(reducePercent * damageInfo->damage);
            break;
        }
        case MELEE_HIT_CRUSHING:
            damageInfo->HitInfo     |= HITINFO_CRUSHING;
            damageInfo->TargetState  = VICTIMSTATE_HIT;
            damageInfo->procEx      |= PROC_EX_NORMAL_HIT;
            // 150% normal damage
            damageInfo->damage += (damageInfo->damage / 2);
            break;
        default:
            break;
    }

    // Always apply HITINFO_AFFECTS_VICTIM in case its not a miss
    if (!(damageInfo->HitInfo & HITINFO_MISS))
        damageInfo->HitInfo |= HITINFO_AFFECTS_VICTIM;

    int32 resilienceReduction = damageInfo->damage;
    ApplyResilience(victim, &resilienceReduction, damageInfo->hitOutCome == MELEE_HIT_CRIT);
    resilienceReduction = damageInfo->damage - resilienceReduction;
    damageInfo->damage      -= resilienceReduction;
    damageInfo->cleanDamage += resilienceReduction;

    if (victim->getClass() == CLASS_MONK)
        damageInfo->damage = victim->CalcStaggerDamage(damageInfo->damage, SpellSchoolMask(damageInfo->damageSchoolMask));

    // Calculate absorb resist
    if (int32(damageInfo->damage) > 0)
    {
        damageInfo->procVictim |= PROC_FLAG_TAKEN_DAMAGE;
        // Calculate absorb & resists
        CalcAbsorbResist(damageInfo->target, SpellSchoolMask(damageInfo->damageSchoolMask), DIRECT_DAMAGE, damageInfo->damage, &damageInfo->absorb, &damageInfo->resist);

        if (damageInfo->absorb)
        {
            damageInfo->HitInfo |= (damageInfo->damage - damageInfo->absorb == 0 ? HITINFO_FULL_ABSORB : HITINFO_PARTIAL_ABSORB);
            damageInfo->procEx  |= PROC_EX_ABSORB;
        }

        if (damageInfo->resist)
            damageInfo->HitInfo |= (damageInfo->damage - damageInfo->resist == 0 ? HITINFO_FULL_RESIST : HITINFO_PARTIAL_RESIST);

        damageInfo->damage -= damageInfo->absorb + damageInfo->resist;
    }
    else // Impossible get negative result but....
        damageInfo->damage = 0;
}

void Unit::DealMeleeDamage(CalcDamageInfo* damageInfo, bool durabilityLoss)
{
    Unit* victim = damageInfo->target;

    if (!victim->isAlive() || victim->HasUnitState(UNIT_STATE_IN_FLIGHT) || (victim->GetTypeId() == TYPEID_UNIT && victim->ToCreature()->IsInEvadeMode()))
        return;

    // Hmmmm dont like this emotes client must by self do all animations
    if (damageInfo->HitInfo & HITINFO_CRITICALHIT)
        victim->HandleEmoteCommand(EMOTE_ONESHOT_WOUND_CRITICAL);
    if (damageInfo->blocked_amount && damageInfo->TargetState != VICTIMSTATE_BLOCKS)
        victim->HandleEmoteCommand(EMOTE_ONESHOT_PARRY_SHIELD);

    if (damageInfo->TargetState == VICTIMSTATE_PARRY)
    {
        // Get attack timers
        float offtime  = float(victim->getAttackTimer(OFF_ATTACK));
        float basetime = float(victim->getAttackTimer(BASE_ATTACK));
        // Reduce attack time
        if (victim->haveOffhandWeapon() && offtime < basetime)
        {
            float percent20 = victim->GetAttackTime(OFF_ATTACK) * 0.20f;
            float percent60 = 3.0f * percent20;
            if (offtime > percent20 && offtime <= percent60)
                victim->setAttackTimer(OFF_ATTACK, uint32(percent20));
            else if (offtime > percent60)
            {
                offtime -= 2.0f * percent20;
                victim->setAttackTimer(OFF_ATTACK, uint32(offtime));
            }
        }
        else
        {
            float percent20 = victim->GetAttackTime(BASE_ATTACK) * 0.20f;
            float percent60 = 3.0f * percent20;
            if (basetime > percent20 && basetime <= percent60)
                victim->setAttackTimer(BASE_ATTACK, uint32(percent20));
            else if (basetime > percent60)
            {
                basetime -= 2.0f * percent20;
                victim->setAttackTimer(BASE_ATTACK, uint32(basetime));
            }
        }
    }

    // Call default DealDamage
    CleanDamage cleanDamage(damageInfo->cleanDamage, damageInfo->absorb, damageInfo->attackType, damageInfo->hitOutCome);
    DealDamage(victim, damageInfo->damage, &cleanDamage, DIRECT_DAMAGE, SpellSchoolMask(damageInfo->damageSchoolMask), NULL, durabilityLoss);

    // Custom MoP Script
    // Brewing : Elusive Brew - 128938
    if (GetTypeId() == TYPEID_PLAYER && getClass() == CLASS_MONK && HasAura(128938)
        && damageInfo->hitOutCome == MELEE_HIT_CRIT
        && (damageInfo->attackType == BASE_ATTACK || damageInfo->attackType == OFF_ATTACK))
        {
            float WeaponSpeed = GetAttackTime(damageInfo->attackType) / 1000.0f;
            float chance = 0.0f;
            if(WeaponSpeed > 2.6f)
            {
                CastSpell(this, 128939, true);
                chance = (((3.0f * WeaponSpeed) / 3.6f) - 1.0f) * 100.f;

                if(chance >= 100.0f)
                {
                    CastSpell(this, 128939, true);
                    chance -= 100.0f;
                }
                if(chance > 0.0f && roll_chance_f(chance))
                    CastSpell(this, 128939, true);
            }
            else
            {
                chance = (((1.5f * WeaponSpeed) / 2.6f) - 1.0f) * 100.f;
                if(chance > 0.0f && roll_chance_f(chance))
                    CastSpell(this, 128939, true);
                if(chance > 0.0f && roll_chance_f(chance))
                    CastSpell(this, 128939, true);
            }
        }

    // If this is a creature and it attacks from behind it has a probability to daze it's victim
    if ((damageInfo->hitOutCome == MELEE_HIT_CRIT || damageInfo->hitOutCome == MELEE_HIT_CRUSHING || damageInfo->hitOutCome == MELEE_HIT_NORMAL || damageInfo->hitOutCome == MELEE_HIT_GLANCING) &&
        GetTypeId() != TYPEID_PLAYER && !ToCreature()->IsControlledByPlayer() && !victim->HasInArc(M_PI, this)
        && (victim->GetTypeId() == TYPEID_PLAYER || !victim->ToCreature()->isWorldBoss()))
    {
        // -probability is between 0% and 40%
        // 20% base chance
        float Probability = 20.0f;

        // there is a newbie protection, at level 10 just 7% base chance; assuming linear function
        if (victim->getLevel() < 30)
            Probability = 0.65f * victim->getLevel() + 0.5f;

        uint32 VictimDefense = victim->GetMaxSkillValueForLevel(this);
        uint32 AttackerMeleeSkill = GetMaxSkillValueForLevel();

        Probability *= AttackerMeleeSkill/(float)VictimDefense;

        if (Probability > 40.0f)
            Probability = 40.0f;

        if (roll_chance_f(Probability))
            CastSpell(victim, 1604, true);
    }

    if (GetTypeId() == TYPEID_PLAYER)
        ToPlayer()->CastItemCombatSpell(victim, damageInfo->attackType, damageInfo->procVictim, damageInfo->procEx);

    // Do effect if any damage done to target
    if (damageInfo->damage)
    {
        // We're going to call functions which can modify content of the list during iteration over it's elements
        // Let's copy the list so we can prevent iterator invalidation
        AuraEffectList vDamageShieldsCopy(victim->GetAuraEffectsByType(SPELL_AURA_DAMAGE_SHIELD));
        for (AuraEffectList::const_iterator dmgShieldItr = vDamageShieldsCopy.begin(); dmgShieldItr != vDamageShieldsCopy.end(); ++dmgShieldItr)
        {
            SpellInfo const* i_spellProto = (*dmgShieldItr)->GetSpellInfo();
            // Damage shield can be resisted...
            if (SpellMissInfo missInfo = victim->SpellHitResult(this, i_spellProto, false, 1 << (*dmgShieldItr)->GetEffIndex()))
            {
                victim->SendSpellMiss(this, i_spellProto->Id, missInfo);
                continue;
            }

            // ...or immuned
            if (IsImmunedToDamage(i_spellProto))
            {
                victim->SendSpellDamageImmune(this, i_spellProto->Id);
                continue;
            }

            uint32 damage = (*dmgShieldItr)->GetAmount();

            if (Unit* caster = (*dmgShieldItr)->GetCaster())
            {
                damage = caster->SpellDamageBonusDone(this, i_spellProto, damage, SPELL_DIRECT_DAMAGE, (SpellEffIndex) (*dmgShieldItr)->GetEffIndex());
                damage = this->SpellDamageBonusTaken(caster, i_spellProto, damage);
            }

            // No Unit::CalcAbsorbResist here - opcode doesn't send that data - this damage is probably not affected by that
            victim->DealDamageMods(this, damage, NULL);

            // TODO: Move this to a packet handler
            ObjectGuid victimGuid = victim->GetGUID();
            ObjectGuid sourceGuid = GetGUID();
            int32 overkill = int32(damage) - int32(GetHealth());

            WorldPacket data(SMSG_SPELLDAMAGESHIELD, 8 + 8 + 4 + 4 + 4 + 4 + 4);

            data.WriteGuidMask<3>(sourceGuid);
            data.WriteBit(0);       // not has power data
            data.WriteGuidMask<1>(victimGuid);
            data.WriteGuidMask<1>(sourceGuid);

            data.WriteGuidMask<3, 5, 0>(victimGuid);
            data.WriteGuidMask<4>(sourceGuid);
            data.WriteGuidMask<7, 6, 2, 4>(victimGuid);
            data.WriteGuidMask<2, 7, 5, 0, 6>(sourceGuid);

            data.WriteGuidBytes<1>(sourceGuid);

            data.WriteGuidBytes<7>(victimGuid);
            data.WriteGuidBytes<6, 7>(sourceGuid);
            data.WriteGuidBytes<5, 6, 3, 1, 0>(victimGuid);
            data.WriteGuidBytes<5, 2>(sourceGuid);
            data << uint32(overkill > 0 ? overkill : 0);    // Overkill
            data << uint32(damage);                         // Damage
            data.WriteGuidBytes<4>(victimGuid);
            data << uint32(-1);                             // FIX ME: Send resisted damage, both fully resisted and partly resisted
            data << uint32(i_spellProto->SchoolMask);
            data.WriteGuidBytes<0>(sourceGuid);
            data << uint32(i_spellProto->Id);
            data.WriteGuidBytes<2>(victimGuid);
            data.WriteGuidBytes<3, 4>(sourceGuid);
            victim->SendMessageToSet(&data, true);

            victim->DealDamage(this, damage, 0, SPELL_DIRECT_DAMAGE, i_spellProto->GetSchoolMask(), i_spellProto, true);
        }
    }
}

void Unit::HandleEmoteCommand(uint32 anim_id)
{
    WorldPacket data(SMSG_EMOTE, 4 + 8);
    data << uint32(anim_id);
    data << uint64(GetGUID());
    SendMessageToSet(&data, true);
}

bool Unit::IsDamageReducedByArmor(SpellSchoolMask schoolMask, SpellInfo const* spellInfo, uint32 effectMask)
{
    // only physical spells damage gets reduced by armor
    if (schoolMask != SPELL_SCHOOL_MASK_NORMAL)
        return false;

    if (spellInfo)
    {
        // there are spells with no specific attribute but they have "ignores armor" in tooltip
        if ((spellInfo->AttributesCu & SPELL_ATTR0_CU_IGNORE_ARMOR))
            return false;

        for (int i = 0; i < MAX_SPELL_EFFECTS; ++i)
        {
            if ((effectMask & (1 << i)) && (spellInfo->Effects[i].ApplyAuraName == SPELL_AURA_PERIODIC_DAMAGE ||
                spellInfo->Effects[i].Effect == SPELL_EFFECT_SCHOOL_DAMAGE))
            {
                if (spellInfo->GetEffectMechanicMask(i) & (1 << MECHANIC_BLEED))
                    return false;
            }
        }
    }
    return true;
}

uint32 Unit::CalcArmorReducedDamage(Unit* victim, const uint32 damage, SpellInfo const* spellInfo, WeaponAttackType /*attackType*/)
{
    uint32 newdamage = 0;
    float armor = float(victim->GetArmor());

    // bypass enemy armor by SPELL_AURA_BYPASS_ARMOR_FOR_CASTER
    int32 armorBypassPct = 0;
    AuraEffectList const & reductionAuras = victim->GetAuraEffectsByType(SPELL_AURA_BYPASS_ARMOR_FOR_CASTER);
    for (AuraEffectList::const_iterator i = reductionAuras.begin(); i != reductionAuras.end(); ++i)
        if ((*i)->GetCasterGUID() == GetGUID())
        {
            if (spellInfo && spellInfo->Id == (*i)->GetId())
                continue;

            armorBypassPct += (*i)->GetAmount();
        }
    armor = CalculatePct(armor, 100 - std::min(armorBypassPct, 100));

    // Ignore enemy armor by SPELL_AURA_MOD_TARGET_RESISTANCE aura
    armor += GetTotalAuraModifierByMiscMask(SPELL_AURA_MOD_TARGET_RESISTANCE, SPELL_SCHOOL_MASK_NORMAL);

    if (spellInfo)
        if (Player* modOwner = GetSpellModOwner())
            modOwner->ApplySpellMod(spellInfo->Id, SPELLMOD_IGNORE_ARMOR, armor);

    AuraEffectList ResIgnoreAuras = GetAuraEffectsByType(SPELL_AURA_MOD_IGNORE_TARGET_RESIST);
    for (AuraEffectList::const_iterator j = ResIgnoreAuras.begin(); j != ResIgnoreAuras.end(); ++j)
    {
        if ((*j)->GetMiscValue() & SPELL_SCHOOL_MASK_NORMAL)
            armor = floor(AddPct(armor, -(*j)->GetAmount()));
    }

    AuraEffectList armorPenetrationPct = GetAuraEffectsByType(SPELL_AURA_MOD_ARMOR_PENETRATION_PCT);
    for (AuraEffectList::const_iterator j = armorPenetrationPct.begin(); j != armorPenetrationPct.end(); ++j)
    {
        if ((*j)->GetMiscValue() & SPELL_SCHOOL_MASK_NORMAL)
            armor -= CalculatePct(armor, (*j)->GetAmount());
    }

    if (armor < 0.0f)
        armor = 0.0f;

    float levelModifier = getLevel();
    if ( levelModifier > 85 )
 		levelModifier = levelModifier + (4.5 * (levelModifier - 59)) + (20 * (levelModifier - 80)) + (22 * (levelModifier - 85));
 	else if ( levelModifier > 80 )
 		levelModifier = levelModifier + (4.5 * (levelModifier - 59)) + (20 * (levelModifier - 80));
 	else if ( levelModifier > 59 )
 		levelModifier = levelModifier + (4.5 * (levelModifier - 59));

    float tmpvalue = 0.1f * armor / (8.5f * levelModifier + 40);
    tmpvalue = tmpvalue / (1.0f + tmpvalue);

    if (tmpvalue < 0.0f)
        tmpvalue = 0.0f;
    if (tmpvalue > 0.75f)
        tmpvalue = 0.75f;

    newdamage = uint32(damage - (damage * tmpvalue));

    return (newdamage > 1) ? newdamage : 1;
}

void Unit::CalcAbsorbResist(Unit* victim, SpellSchoolMask schoolMask, DamageEffectType damagetype, uint32 const damage, uint32 *absorb, uint32 *resist, SpellInfo const* spellInfo)
{
    if (!victim || !victim->isAlive() || !damage)
        return;

    DamageInfo dmgInfo = DamageInfo(this, victim, damage, spellInfo, schoolMask, damagetype, damage);

    // Magic damage, check for resists
    // Ignore spells that cant be resisted
    if ((schoolMask & SPELL_SCHOOL_MASK_NORMAL) == 0 && (!spellInfo || (spellInfo->AttributesEx4 & SPELL_ATTR4_IGNORE_RESISTANCES) == 0))
    {
        float victimResistance = float(victim->GetResistance(schoolMask));
        victimResistance += float(GetTotalAuraModifierByMiscMask(SPELL_AURA_MOD_TARGET_RESISTANCE, schoolMask));

        if (Player* player = ToPlayer())
            victimResistance -= float(player->GetSpellPenetrationItemMod());

        // Resistance can't be lower then 0.
        if (victimResistance < 0.0f)
            victimResistance = 0.0f;

        static uint32 const BOSS_LEVEL = 83;
        static float const BOSS_RESISTANCE_CONSTANT = 510.0f;
        uint32 level = victim->getLevel();
        float resistanceConstant = 0.0f;

        if (level == BOSS_LEVEL)
            resistanceConstant = BOSS_RESISTANCE_CONSTANT;
        else
            resistanceConstant = level * 5.0f;

        float averageResist = victimResistance / (victimResistance + resistanceConstant);
        float discreteResistProbability[11];
        for (uint32 i = 0; i < 11; ++i)
        {
            discreteResistProbability[i] = 0.5f - 2.5f * fabs(0.1f * i - averageResist);
            if (discreteResistProbability[i] < 0.0f)
                discreteResistProbability[i] = 0.0f;
        }

        if (averageResist <= 0.1f)
        {
            discreteResistProbability[0] = 1.0f - 7.5f * averageResist;
            discreteResistProbability[1] = 5.0f * averageResist;
            discreteResistProbability[2] = 2.5f * averageResist;
        }

        float r = float(rand_norm());
        uint32 i = 0;
        float probabilitySum = discreteResistProbability[0];

        while (r >= probabilitySum && i < 10)
            probabilitySum += discreteResistProbability[++i];

        float damageResisted = float(damage * i / 10);

        AuraEffectList ResIgnoreAuras = GetAuraEffectsByType(SPELL_AURA_MOD_IGNORE_TARGET_RESIST);
        for (AuraEffectList::const_iterator j = ResIgnoreAuras.begin(); j != ResIgnoreAuras.end(); ++j)
            if ((*j)->GetMiscValue() & schoolMask)
                AddPct(damageResisted, -(*j)->GetAmount());

        dmgInfo.ResistDamage(uint32(damageResisted));
    }

    // Ignore Absorption Auras
    float auraAbsorbMod = 0;
    AuraEffectList AbsIgnoreAurasA = GetAuraEffectsByType(SPELL_AURA_MOD_TARGET_ABSORB_SCHOOL);
    for (AuraEffectList::const_iterator itr = AbsIgnoreAurasA.begin(); itr != AbsIgnoreAurasA.end(); ++itr)
    {
        if (!((*itr)->GetMiscValue() & schoolMask))
            continue;

        if ((*itr)->GetAmount() > auraAbsorbMod)
            auraAbsorbMod = float((*itr)->GetAmount());
    }

    AuraEffectList AbsIgnoreAurasB = GetAuraEffectsByType(SPELL_AURA_MOD_TARGET_ABILITY_ABSORB_SCHOOL);
    for (AuraEffectList::const_iterator itr = AbsIgnoreAurasB.begin(); itr != AbsIgnoreAurasB.end(); ++itr)
    {
        if (!((*itr)->GetMiscValue() & schoolMask))
            continue;

        if (((*itr)->GetAmount() > auraAbsorbMod) && (*itr)->IsAffectingSpell(spellInfo))
            auraAbsorbMod = float((*itr)->GetAmount());
    }
    RoundToInterval(auraAbsorbMod, 0.0f, 100.0f);

    // We're going to call functions which can modify content of the list during iteration over it's elements
    // Let's copy the list so we can prevent iterator invalidation
    AuraEffectList vSchoolAbsorbCopy(victim->GetAuraEffectsByType(SPELL_AURA_SCHOOL_ABSORB));
    vSchoolAbsorbCopy.sort(Trinity::AbsorbAuraOrderPred());

    // absorb without mana cost
    for (AuraEffectList::iterator itr = vSchoolAbsorbCopy.begin(); (itr != vSchoolAbsorbCopy.end()) && (dmgInfo.GetDamage() > 0); ++itr)
    {
        AuraEffect* absorbAurEff = *itr;
        // Check if aura was removed during iteration - we don't need to work on such auras
        AuraApplication const* aurApp = absorbAurEff->GetBase()->GetApplicationOfTarget(victim->GetGUID());
        if (!aurApp)
            continue;

        uint8 _mask = schoolMask;
        _mask &= ~absorbAurEff->GetMiscValue();

        if (_mask != 0)
            continue;

        // get amount which can be still absorbed by the aura
        int32 currentAbsorb = absorbAurEff->GetAmount();
        // aura with infinite absorb amount - let the scripts handle absorbtion amount, set here to 0 for safety
        if (currentAbsorb < 0)
            currentAbsorb = 0;

        uint32 tempAbsorb = uint32(currentAbsorb);

        bool defaultPrevented = false;

        absorbAurEff->GetBase()->CallScriptEffectAbsorbHandlers(absorbAurEff, aurApp, dmgInfo, tempAbsorb, defaultPrevented);
        currentAbsorb = tempAbsorb;

        // Apply absorb mod auras
        AddPct(currentAbsorb, -auraAbsorbMod);

        // absorb must be smaller than the damage itself
        currentAbsorb = RoundToInterval(currentAbsorb, 0, int32(dmgInfo.GetDamage()));

        dmgInfo.AbsorbDamage(currentAbsorb);

        tempAbsorb = currentAbsorb;
        absorbAurEff->GetBase()->CallScriptEffectAfterAbsorbHandlers(absorbAurEff, aurApp, dmgInfo, tempAbsorb);

        if (defaultPrevented)
            continue;

        if (currentAbsorb != dmgInfo.GetAbsorb())
            currentAbsorb = dmgInfo.GetAbsorb();

        // Check if our aura is using amount to count damage
        if (absorbAurEff->GetAmount() >= 0 && currentAbsorb && !(absorbAurEff->GetSpellInfo()->AttributesEx6 & SPELL_ATTR6_NOT_LIMIT_ABSORB))
        {
            // Reduce shield amount
            absorbAurEff->SetAmount(absorbAurEff->GetAmount() - currentAbsorb);
            // Aura cannot absorb anything more - remove it
            if (absorbAurEff->GetAmount() <= 0) // Custom MoP Script - Stance of the Sturdy Ox shoudn't be removed at any damage
                absorbAurEff->GetBase()->Remove(AURA_REMOVE_BY_ENEMY_SPELL);
        }
    }

    // absorb by mana cost
    AuraEffectList vManaShieldCopy(victim->GetAuraEffectsByType(SPELL_AURA_MANA_SHIELD));
    for (AuraEffectList::const_iterator itr = vManaShieldCopy.begin(); (itr != vManaShieldCopy.end()) && (dmgInfo.GetDamage() > 0); ++itr)
    {
        AuraEffect* absorbAurEff = *itr;
        // Check if aura was removed during iteration - we don't need to work on such auras
        AuraApplication const* aurApp = absorbAurEff->GetBase()->GetApplicationOfTarget(victim->GetGUID());
        if (!aurApp)
            continue;
        // check damage school mask
        if (!(absorbAurEff->GetMiscValue() & schoolMask))
            continue;

        // get amount which can be still absorbed by the aura
        int32 currentAbsorb = absorbAurEff->GetAmount();
        // aura with infinite absorb amount - let the scripts handle absorbtion amount, set here to 0 for safety
        if (currentAbsorb < 0)
            currentAbsorb = 0;

        uint32 tempAbsorb = currentAbsorb;

        bool defaultPrevented = false;

        absorbAurEff->GetBase()->CallScriptEffectManaShieldHandlers(absorbAurEff, aurApp, dmgInfo, tempAbsorb, defaultPrevented);
        currentAbsorb = tempAbsorb;

        if (defaultPrevented)
            continue;

        AddPct(currentAbsorb, -auraAbsorbMod);

        // absorb must be smaller than the damage itself
        currentAbsorb = RoundToInterval(currentAbsorb, 0, int32(dmgInfo.GetDamage()));

        int32 manaReduction = currentAbsorb;

        // lower absorb amount by talents
        if (float manaMultiplier = absorbAurEff->GetSpellInfo()->GetEffect(absorbAurEff->GetEffIndex(), GetSpawnMode())->CalcValueMultiplier(absorbAurEff->GetCaster()))
            manaReduction = int32(float(manaReduction) * manaMultiplier);

        int32 manaTaken = -victim->ModifyPower(POWER_MANA, -manaReduction);

        // take case when mana has ended up into account
        currentAbsorb = currentAbsorb ? int32(float(currentAbsorb) * (float(manaTaken) / float(manaReduction))) : 0;

        dmgInfo.AbsorbDamage(currentAbsorb);

        tempAbsorb = currentAbsorb;
        absorbAurEff->GetBase()->CallScriptEffectAfterManaShieldHandlers(absorbAurEff, aurApp, dmgInfo, tempAbsorb);

        // Check if our aura is using amount to count damage
        if (absorbAurEff->GetAmount() >= 0)
        {
            absorbAurEff->SetAmount(absorbAurEff->GetAmount() - currentAbsorb);
            if ((absorbAurEff->GetAmount() <= 0))
                absorbAurEff->GetBase()->Remove(AURA_REMOVE_BY_ENEMY_SPELL);
        }
    }

    // split damage auras - only when not damaging self
    if (victim != this)
    {
        // We're going to call functions which can modify content of the list during iteration over it's elements
        // Let's copy the list so we can prevent iterator invalidation
        AuraEffectList vSplitDamagePctCopy(victim->GetAuraEffectsByType(SPELL_AURA_SPLIT_DAMAGE_PCT));
        for (AuraEffectList::iterator itr = vSplitDamagePctCopy.begin(), next; (itr != vSplitDamagePctCopy.end()) &&  (dmgInfo.GetDamage() > 0); ++itr)
        {
            // Check if aura was removed during iteration - we don't need to work on such auras
            if (!((*itr)->GetBase()->IsAppliedOnTarget(victim->GetGUID())))
                continue;
            // check damage school mask
            if (!((*itr)->GetMiscValue() & schoolMask))
                continue;

            // Damage can be splitted only if aura has an alive caster
            Unit* caster = (*itr)->GetCaster();
            if (!caster || (caster == victim) || !caster->IsInWorld() || !caster->isAlive())
                continue;

            int32 splitDamage = CalculatePct(dmgInfo.GetDamage(), (*itr)->GetAmount());

            // absorb must be smaller than the damage itself
            splitDamage = RoundToInterval(splitDamage, 0, int32(dmgInfo.GetDamage()));

            dmgInfo.AbsorbDamage(splitDamage);

            uint32 splitted = splitDamage;
            uint32 split_absorb = 0;

            SpellNonMeleeDamage damageInfo(this, caster, (*itr)->GetSpellInfo()->Id, schoolMask);

            if (caster->IsImmunedToDamage(schoolMask))
            {
                damageInfo.resist = splitted;
                splitted = 0;
            }

            damageInfo.damage = splitted;
            DealDamageMods(caster, damageInfo.damage, &damageInfo.absorb);
            SendSpellNonMeleeDamageLog(&damageInfo);

            uint32 m_procVictim = PROC_FLAG_DONE_MELEE_AUTO_ATTACK | PROC_FLAG_DONE_MAINHAND_ATTACK;
            uint32 m_procAttacker = PROC_FLAG_NONE;
            if(spellInfo)
            {
                switch (spellInfo->DmgClass)
                {
                    case SPELL_DAMAGE_CLASS_MELEE:
                        m_procVictim   = PROC_FLAG_TAKEN_SPELL_MELEE_DMG_CLASS;
                        break;
                    case SPELL_DAMAGE_CLASS_MAGIC:
                        m_procVictim   = PROC_FLAG_TAKEN_SPELL_MAGIC_DMG_CLASS_NEG;
                        break;
                    case SPELL_DAMAGE_CLASS_NONE:
                        m_procVictim |= PROC_FLAG_TAKEN_SPELL_MELEE_DMG_CLASS;
                        break;
                    case SPELL_DAMAGE_CLASS_RANGED:
                        m_procVictim   = PROC_FLAG_TAKEN_SPELL_RANGED_DMG_CLASS;
                        break;
                }
            }

            DamageInfo dmgInfoProc = DamageInfo(damageInfo, spellInfo);
            ProcDamageAndSpell(caster, m_procAttacker, m_procVictim, PROC_EX_NORMAL_HIT, &dmgInfoProc, BASE_ATTACK, spellInfo);

            CleanDamage cleanDamage = CleanDamage(splitted, 0, BASE_ATTACK, MELEE_HIT_NORMAL);
            DealDamage(caster, splitted, &cleanDamage, DIRECT_DAMAGE, schoolMask, (*itr)->GetSpellInfo(), false);
        }
    }

    if (victim && victim->ToPlayer())
    {
        if (victim->getClass() == CLASS_PALADIN && victim->HasAura(498))
        {
            if(Aura* aura = victim->GetAura(144580)) //Item - Paladin T16 Protection 2P Bonus
                aura->GetEffect(0)->SetAmount(aura->GetEffect(0)->GetAmount() + dmgInfo.GetDamage());
            if(Aura* aura = victim->GetAura(138244))
            {
                int32 _damage = dmgInfo.GetDamage();
                int32 _amount = aura->GetEffect(0)->GetAmount();
                int32 _hpperc = victim->CountPctFromMaxHealth(20);
                if((_damage + _amount) >= _hpperc) //Item - Paladin T15 Protection 4P Bonus
                {
                    victim->CastSpell(victim, 138248, true);
                    aura->GetEffect(0)->SetAmount(int32((_damage + _amount) - _hpperc));
                }
                else
                    aura->GetEffect(0)->SetAmount(_amount + _damage);
            }
        }
        if (victim->getClass() == CLASS_WARLOCK && victim->ToPlayer()->HasSpellCooldown(5484) && !victim->ToPlayer()->HasSpellCooldown(112891)) // Howl of Terror
        {
            victim->ToPlayer()->ModifySpellCooldown(5484, -1000);
            victim->ToPlayer()->AddSpellCooldown(112891, 0, getPreciseTime() + 1.0);
        }
    }

    *resist = dmgInfo.GetResist();
    *absorb = dmgInfo.GetAbsorb();
}

void Unit::CalcHealAbsorb(Unit* victim, const SpellInfo* healSpell, uint32 &healAmount, uint32 &absorb)
{
    if (!healAmount)
        return;

    int32 RemainingHeal = healAmount;

    // Need remove expired auras after
    bool existExpired = false;

    DamageInfo dmgInfo = DamageInfo(this, victim, healAmount, healSpell, SpellSchoolMask(healSpell->SchoolMask), SPELL_DIRECT_DAMAGE, 0);

    // absorb without mana cost
    AuraEffectList vHealAbsorb = victim->GetAuraEffectsByType(SPELL_AURA_SCHOOL_HEAL_ABSORB);
    for (AuraEffectList::const_iterator i = vHealAbsorb.begin(); i != vHealAbsorb.end() && RemainingHeal > 0; ++i)
    {
        AuraEffect* absorbAurEff = *i;
        AuraApplication const* aurApp = absorbAurEff->GetBase()->GetApplicationOfTarget(victim->GetGUID());
        if (!aurApp)
            continue;

        if (!((*i)->GetMiscValue() & healSpell->SchoolMask))
            continue;

        // Max Amount can be absorbed by this aura
        int32 currentAbsorb = (*i)->GetAmount();

        // Found empty aura (impossible but..)
        if (currentAbsorb <= 0)
        {
            existExpired = true;
            continue;
        }

        uint32 tempAbsorb = uint32(currentAbsorb);
        bool defaultPrevented = false;
        absorbAurEff->GetBase()->CallScriptEffectAbsorbHandlers(absorbAurEff, aurApp, dmgInfo, tempAbsorb, defaultPrevented);
        currentAbsorb = tempAbsorb;

        // currentAbsorb - damage can be absorbed by shield
        // If need absorb less damage
        if (RemainingHeal < currentAbsorb)
            currentAbsorb = RemainingHeal;

        RemainingHeal -= currentAbsorb;

        dmgInfo.AbsorbDamage(currentAbsorb);

        tempAbsorb = currentAbsorb;
        absorbAurEff->GetBase()->CallScriptEffectAfterAbsorbHandlers(absorbAurEff, aurApp, dmgInfo, tempAbsorb);

        // Reduce shield amount
        (*i)->SetAmount((*i)->GetAmount() - currentAbsorb);
        // Need remove it later
        if ((*i)->GetAmount() <= 0)
            existExpired = true;
    }

    // Remove all expired absorb auras
    if (existExpired)
    {
        for (AuraEffectList::const_iterator i = vHealAbsorb.begin(); i != vHealAbsorb.end();)
        {
            AuraEffect* auraEff = *i;
            ++i;
            if (auraEff->GetAmount() <= 0)
            {
                uint32 removedAuras = victim->m_removedAurasCount;
                auraEff->GetBase()->Remove(AURA_REMOVE_BY_ENEMY_SPELL);
                if (removedAuras+1 < victim->m_removedAurasCount)
                    i = vHealAbsorb.begin();
            }
        }
    }

    absorb = RemainingHeal > 0 ? (healAmount - RemainingHeal) : healAmount;
    healAmount = RemainingHeal;
}

uint32 Unit::CalcAbsorb(Unit* victim, SpellInfo const* spellProto, uint32 amount)
{
    if (!victim || !spellProto)
        return amount;

    if ((spellProto->AttributesEx3 & SPELL_ATTR3_NO_DONE_BONUS) || (spellProto->AttributesEx6 & SPELL_ATTR6_NO_DONE_PCT_DAMAGE_MODS))
        return amount;

    SpellBonusEntry const* bonus = sSpellMgr->GetSpellBonusData(spellProto->Id);

    if (bonus)
    {
        if (bonus->direct_damage > 0)
            amount += int32(bonus->direct_damage * GetSpellPowerDamage(spellProto->GetSchoolMask()));
        else if (bonus->ap_bonus > 0)
            amount += int32(bonus->ap_bonus * GetTotalAttackPowerValue(BASE_ATTACK));
    }

    AuraEffectList mAbsorbReducedDamage = victim->GetAuraEffectsByType(SPELL_AURA_MOD_ABSORB_AMOUNT);
    for (AuraEffectList::const_iterator i = mAbsorbReducedDamage.begin(); i != mAbsorbReducedDamage.end(); ++i)
        AddPct(amount, (*i)->GetAmount());

    amount *= CalcPvPPower(victim, 1.0f, true);

    return amount;
}

void Unit::AttackerStateUpdate(Unit* victim, WeaponAttackType attType, bool extra, uint32 replacementAttackTrigger, uint32 replacementAttackAura)
{
    if (HasUnitState(UNIT_STATE_CANNOT_AUTOATTACK) || HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PACIFIED))
        return;

    if (!victim->isAlive())
        return;

    if (!IsAIEnabled || !GetMap()->Instanceable())
        if ((attType == BASE_ATTACK || attType == OFF_ATTACK) && !IsWithinLOSInMap(victim))
            return;

    CombatStart(victim);
    RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_MELEE_ATTACK);

    if (attType != BASE_ATTACK && attType != OFF_ATTACK)
        return;                                             // ignore ranged case

    if (replacementAttackTrigger)
    {
        CastSpell(victim, replacementAttackTrigger, true);
    }
    else
    {
        // melee attack spell casted at main hand attack only - no normal melee dmg dealt
        if (attType == BASE_ATTACK && m_currentSpells[CURRENT_MELEE_SPELL] && !extra)
            m_currentSpells[CURRENT_MELEE_SPELL]->cast();
        else
        {
            // attack can be redirected to another target
            Unit* redirectVictim = GetMeleeHitRedirectTarget(victim);

            // Custom MoP Script
            // SPELL_AURA_STRIKE_SELF
            if (HasAuraType(SPELL_AURA_STRIKE_SELF))
            {
                // Dizzying Haze - 115180
                if (AuraApplication* aura = this->GetAuraApplication(116330))
                {
                    if (roll_chance_i(aura->GetBase()->GetEffect(1)->GetAmount()))
                    {
                        redirectVictim->CastSpell(this, 118022, true);
                        return;
                    }
                }
            }

            CalcDamageInfo damageInfo;
            CalculateMeleeDamage(redirectVictim, 0, &damageInfo, attType);
            // Send log damage message to client
            DealDamageMods(redirectVictim, damageInfo.damage, &damageInfo.absorb);
            SendAttackStateUpdate(&damageInfo);

            DealMeleeDamage(&damageInfo, true);

            //TriggerAurasProcOnEvent(damageInfo);
            DamageInfo dmgInfoProc = DamageInfo(damageInfo);
            ProcDamageAndSpell(redirectVictim, damageInfo.procAttacker, damageInfo.procVictim, damageInfo.procEx, &dmgInfoProc, damageInfo.attackType);
            if (redirectVictim != victim)
                ProcDamageAndSpell(victim, damageInfo.procAttacker, damageInfo.procVictim, damageInfo.procEx, &dmgInfoProc, damageInfo.attackType);

            if (GetTypeId() == TYPEID_PLAYER)
                sLog->outDebug(LOG_FILTER_UNITS, "AttackerStateUpdate: (Player) %u attacked %u (TypeId: %u) for %u dmg, absorbed %u, blocked %u, resisted %u.",
                    GetGUIDLow(), redirectVictim->GetGUIDLow(), redirectVictim->GetTypeId(), damageInfo.damage, damageInfo.absorb, damageInfo.blocked_amount, damageInfo.resist);
            else
                sLog->outDebug(LOG_FILTER_UNITS, "AttackerStateUpdate: (NPC)    %u attacked %u (TypeId: %u) for %u dmg, absorbed %u, blocked %u, resisted %u.",
                    GetGUIDLow(), redirectVictim->GetGUIDLow(), redirectVictim->GetTypeId(), damageInfo.damage, damageInfo.absorb, damageInfo.blocked_amount, damageInfo.resist);

            if (replacementAttackAura)
                RemoveAura(replacementAttackAura);
        }
    }
}

void Unit::HandleProcExtraAttackFor(Unit* victim)
{
    while (m_extraAttacks)
    {
        AttackerStateUpdate(victim, BASE_ATTACK, true);
        --m_extraAttacks;
    }
}

MeleeHitOutcome Unit::RollMeleeOutcomeAgainst(const Unit* victim, WeaponAttackType attType) const
{
    // This is only wrapper

    // Miss chance based on melee
    //float miss_chance = MeleeMissChanceCalc(victim, attType);
    float miss_chance = MeleeSpellMissChance(victim, attType, 0);

    // Critical hit chance
    float crit_chance = GetUnitCriticalChance(attType, victim);

    // stunned target cannot dodge and this is check in GetUnitDodgeChance() (returned 0 in this case)
    float dodge_chance = victim->GetUnitDodgeChance();
    float block_chance = victim->GetUnitBlockChance();
    float parry_chance = victim->GetUnitParryChance();

    // Useful if want to specify crit & miss chances for melee, else it could be removed
    sLog->outDebug(LOG_FILTER_UNITS, "MELEE OUTCOME: miss %f crit %f dodge %f parry %f block %f", miss_chance, crit_chance, dodge_chance, parry_chance, block_chance);

    return RollMeleeOutcomeAgainst(victim, attType, int32(crit_chance*100), int32(miss_chance*100), int32(dodge_chance*100), int32(parry_chance*100), int32(block_chance*100));
}

MeleeHitOutcome Unit::RollMeleeOutcomeAgainst (const Unit* victim, WeaponAttackType attType, int32 crit_chance, int32 miss_chance, int32 dodge_chance, int32 parry_chance, int32 block_chance) const
{
    if (victim->GetTypeId() == TYPEID_UNIT && victim->ToCreature()->IsInEvadeMode())
        return MELEE_HIT_EVADE;

    int32 attackerMaxSkillValueForLevel = GetMaxSkillValueForLevel(victim);
    int32 victimMaxSkillValueForLevel = victim->GetMaxSkillValueForLevel(this);

    // bonus from skills is 0.04%
    int32    skillBonus  = 4 * (attackerMaxSkillValueForLevel - victimMaxSkillValueForLevel);
    int32    sum = 0, tmp = 0;
    int32    roll = urand (0, 10000);

    sLog->outDebug(LOG_FILTER_UNITS, "RollMeleeOutcomeAgainst: skill bonus of %d for attacker", skillBonus);
    sLog->outDebug(LOG_FILTER_UNITS, "RollMeleeOutcomeAgainst: rolled %d, miss %d, dodge %d, parry %d, block %d, crit %d",
        roll, miss_chance, dodge_chance, parry_chance, block_chance, crit_chance);

    tmp = miss_chance;

    if (tmp > 0 && roll < (sum += tmp))
    {
        sLog->outDebug(LOG_FILTER_UNITS, "RollMeleeOutcomeAgainst: MISS");
        return MELEE_HIT_MISS;
    }

    // always crit against a sitting target (except 0 crit chance)
    if (victim->GetTypeId() == TYPEID_PLAYER && crit_chance > 0 && !victim->IsStandState())
    {
        sLog->outDebug(LOG_FILTER_UNITS, "RollMeleeOutcomeAgainst: CRIT (sitting victim)");
        return MELEE_HIT_CRIT;
    }

    // Dodge chance

    // only players can't dodge if attacker is behind
    if (victim->GetTypeId() == TYPEID_PLAYER && !victim->HasInArc(M_PI, this) && !victim->HasAuraType(SPELL_AURA_IGNORE_HIT_DIRECTION))
    {
        sLog->outDebug(LOG_FILTER_UNITS, "RollMeleeOutcomeAgainst: attack came from behind and victim was a player.");
    }
    else
    {
        // Reduce dodge chance by attacker expertise rating
        if (GetTypeId() == TYPEID_PLAYER)
            dodge_chance -= int32(ToPlayer()->GetExpertiseDodgeOrParryReduction(attType) * 100);
        else
        {
            dodge_chance -= m_expertise * 100.0f;
            dodge_chance -= GetTotalAuraModifier(SPELL_AURA_MOD_EXPERTISE) * 25;
        }

        // Modify dodge chance by attacker SPELL_AURA_MOD_COMBAT_RESULT_CHANCE
        dodge_chance+= GetTotalAuraModifierByMiscValue(SPELL_AURA_MOD_COMBAT_RESULT_CHANCE, VICTIMSTATE_DODGE) * 100;
        dodge_chance = int32 (float (dodge_chance) * GetTotalAuraMultiplier(SPELL_AURA_MOD_ENEMY_DODGE));

        tmp = dodge_chance;
        if ((tmp > 0)                                        // check if unit _can_ dodge
            && ((tmp -= skillBonus) > 0)
            && roll < (sum += tmp))
        {
            sLog->outDebug(LOG_FILTER_UNITS, "RollMeleeOutcomeAgainst: DODGE <%d, %d)", sum-tmp, sum);
            return MELEE_HIT_DODGE;
        }
    }

    // parry & block chances

    // check if attack comes from behind, nobody can parry or block if attacker is behind
    if (!victim->HasInArc(M_PI, this) && !victim->HasAuraType(SPELL_AURA_IGNORE_HIT_DIRECTION))
        sLog->outDebug(LOG_FILTER_UNITS, "RollMeleeOutcomeAgainst: attack came from behind.");
    else
    {
        // Reduce parry chance by attacker expertise rating
        if (GetTypeId() == TYPEID_PLAYER)
            parry_chance -= int32(ToPlayer()->GetExpertiseDodgeOrParryReduction(attType) * 100);
        else
        {
            parry_chance -= m_expertise * 100.0f;
            parry_chance -= GetTotalAuraModifier(SPELL_AURA_MOD_EXPERTISE) * 25;
        }

        if (victim->GetTypeId() == TYPEID_PLAYER || !(victim->ToCreature()->GetCreatureTemplate()->flags_extra & CREATURE_FLAG_EXTRA_NO_PARRY))
        {
            int32 tmp2 = int32(parry_chance);
            if (tmp2 > 0                                         // check if unit _can_ parry
                && (tmp2 -= skillBonus) > 0
                && roll < (sum += tmp2))
            {
                sLog->outDebug(LOG_FILTER_UNITS, "RollMeleeOutcomeAgainst: PARRY <%d, %d)", sum-tmp2, sum);
                return MELEE_HIT_PARRY;
            }
        }

        if (victim->GetTypeId() == TYPEID_PLAYER || !(victim->ToCreature()->GetCreatureTemplate()->flags_extra & CREATURE_FLAG_EXTRA_NO_BLOCK))
        {
            tmp = block_chance;
            if (tmp > 0                                          // check if unit _can_ block
                && (tmp -= skillBonus) > 0
                && roll < (sum += tmp))
            {
                sLog->outDebug(LOG_FILTER_UNITS, "RollMeleeOutcomeAgainst: BLOCK <%d, %d)", sum-tmp, sum);
                return MELEE_HIT_BLOCK;
            }
        }
    }

    // Critical chance
    tmp = crit_chance;

    if (tmp > 0 && roll < (sum += tmp))
    {
        sLog->outDebug(LOG_FILTER_UNITS, "RollMeleeOutcomeAgainst: CRIT <%d, %d)", sum-tmp, sum);
        if (GetTypeId() == TYPEID_UNIT && (ToCreature()->GetCreatureTemplate()->flags_extra & CREATURE_FLAG_EXTRA_NO_CRIT))
            sLog->outDebug(LOG_FILTER_UNITS, "RollMeleeOutcomeAgainst: CRIT DISABLED)");
        else
            return MELEE_HIT_CRIT;
    }

    // Max 40% chance to score a glancing blow against mobs that are higher level (can do only players and pets and not with ranged weapon)
    if (attType != RANGED_ATTACK &&
        (GetTypeId() == TYPEID_PLAYER || ToCreature()->isPet()) &&
        victim->GetTypeId() != TYPEID_PLAYER && !victim->ToCreature()->isPet() &&
        getLevel() < victim->getLevelForTarget(this))
    {
        // cap possible value (with bonuses > max skill)
        int32 skill = attackerMaxSkillValueForLevel;

        tmp = (10 + (victimMaxSkillValueForLevel - skill)) * 100;
        tmp = tmp > 4000 ? 4000 : tmp;
        if (roll < (sum += tmp))
        {
            sLog->outDebug(LOG_FILTER_UNITS, "RollMeleeOutcomeAgainst: GLANCING <%d, %d)", sum-4000, sum);
            return MELEE_HIT_GLANCING;
        }
    }

    // mobs can score crushing blows if they're 4 or more levels above victim
    if (getLevelForTarget(victim) >= victim->getLevelForTarget(this) + 4 &&
        // can be from by creature (if can) or from controlled player that considered as creature
        !IsControlledByPlayer() &&
        !(GetTypeId() == TYPEID_UNIT && ToCreature()->GetCreatureTemplate()->flags_extra & CREATURE_FLAG_EXTRA_NO_CRUSH))
    {
        // when their weapon skill is 15 or more above victim's defense skill
        tmp = victimMaxSkillValueForLevel;
        // tmp = mob's level * 5 - player's current defense skill
        tmp = attackerMaxSkillValueForLevel - tmp;
        if (tmp >= 15)
        {
            // add 2% chance per lacking skill point, min. is 15%
            tmp = tmp * 200 - 1500;
            if (roll < (sum += tmp))
            {
                sLog->outDebug(LOG_FILTER_UNITS, "RollMeleeOutcomeAgainst: CRUSHING <%d, %d)", sum-tmp, sum);
                return MELEE_HIT_CRUSHING;
            }
        }
    }

    sLog->outDebug(LOG_FILTER_UNITS, "RollMeleeOutcomeAgainst: NORMAL");
    return MELEE_HIT_NORMAL;
}

uint32 Unit::CalculateDamage(WeaponAttackType attType, bool normalized, bool addTotalPct)
{
    float min_damage, max_damage;

    if (GetTypeId() == TYPEID_PLAYER && (normalized || !addTotalPct))
        ToPlayer()->CalculateMinMaxDamage(attType, normalized, addTotalPct, min_damage, max_damage);
    else
    {
        switch (attType)
        {
            case RANGED_ATTACK:
                min_damage = GetFloatValue(UNIT_FIELD_MINRANGEDDAMAGE);
                max_damage = GetFloatValue(UNIT_FIELD_MAXRANGEDDAMAGE);
                break;
            case BASE_ATTACK:
                min_damage = GetFloatValue(UNIT_FIELD_MINDAMAGE);
                max_damage = GetFloatValue(UNIT_FIELD_MAXDAMAGE);
                break;
            case OFF_ATTACK:
                min_damage = GetFloatValue(UNIT_FIELD_MINOFFHANDDAMAGE);
                max_damage = GetFloatValue(UNIT_FIELD_MAXOFFHANDDAMAGE);
                break;
                // Just for good manner
            default:
                min_damage = 0.0f;
                max_damage = 0.0f;
                break;
        }
    }

    if (min_damage > max_damage)
        std::swap(min_damage, max_damage);

    if (max_damage == 0.0f)
        max_damage = 5.0f;

    return urand((uint32)min_damage, (uint32)max_damage);
}

float Unit::CalculateLevelPenalty(SpellInfo const* spellProto) const
{
    if (spellProto->SpellLevel <= 0 || spellProto->SpellLevel >= spellProto->MaxLevel)
        return 1.0f;

    float LvlPenalty = 0.0f;

    if (spellProto->SpellLevel < 20)
        LvlPenalty = 20.0f - spellProto->SpellLevel * 3.75f;
    float LvlFactor = (float(spellProto->SpellLevel) + 6.0f) / float(getLevel());
    if (LvlFactor > 1.0f)
        LvlFactor = 1.0f;

    return AddPct(LvlFactor, -LvlPenalty);
}

void Unit::SendMeleeAttackStart(Unit* victim)
{
    //! 5.4.1
    WorldPacket data(SMSG_ATTACKSTART, 8 + 8);

    ObjectGuid attackerGuid = GetGUID();
    ObjectGuid victimGuid = victim->GetGUID();

    data.WriteBit(attackerGuid[3]);
    data.WriteBit(victimGuid[3]);
    data.WriteBit(victimGuid[2]);
    data.WriteBit(attackerGuid[0]);
    data.WriteBit(attackerGuid[1]);
    data.WriteBit(attackerGuid[4]);
    data.WriteBit(victimGuid[7]);
    data.WriteBit(victimGuid[4]);
    data.WriteBit(attackerGuid[5]);
    data.WriteBit(victimGuid[1]);
    data.WriteBit(victimGuid[5]);
    data.WriteBit(attackerGuid[6]);
    data.WriteBit(attackerGuid[7]);
    data.WriteBit(victimGuid[0]);
    data.WriteBit(victimGuid[6]);
    data.WriteBit(attackerGuid[2]);

    data.WriteByteSeq(attackerGuid[4]);
    data.WriteByteSeq(attackerGuid[6]);
    data.WriteByteSeq(attackerGuid[2]);
    data.WriteByteSeq(attackerGuid[7]);
    data.WriteByteSeq(victimGuid[1]);
    data.WriteByteSeq(attackerGuid[0]);
    data.WriteByteSeq(attackerGuid[3]);
    data.WriteByteSeq(victimGuid[2]);
    data.WriteByteSeq(attackerGuid[5]);
    data.WriteByteSeq(victimGuid[0]);
    data.WriteByteSeq(victimGuid[4]);
    data.WriteByteSeq(victimGuid[3]);
    data.WriteByteSeq(attackerGuid[1]);
    data.WriteByteSeq(victimGuid[6]);
    data.WriteByteSeq(victimGuid[5]);
    data.WriteByteSeq(victimGuid[7]);

    SendMessageToSet(&data, true);
    sLog->outDebug(LOG_FILTER_UNITS, "WORLD: Sent SMSG_ATTACKSTART");
}

void Unit::SendMeleeAttackStop(Unit* victim)
{
    //! 5.4.1
    WorldPacket data(SMSG_ATTACKSTOP, (8+8+4));

    ObjectGuid attackerGuid = GetGUID();
    ObjectGuid victimGuid = victim ? victim->GetGUID() : NULL;

    data.WriteBit(victimGuid[3]);
    data.WriteBit(victimGuid[0]);
    data.WriteBit(attackerGuid[1]);
    data.WriteBit(victimGuid[1]);
    data.WriteBit(victimGuid[2]);
    data.WriteBit(victimGuid[6]);
    data.WriteBit(victimGuid[5]);
    data.WriteBit(attackerGuid[3]);
    data.WriteBit(attackerGuid[0]);
    data.WriteBit(attackerGuid[6]);
    data.WriteBit(victimGuid[4]);
    data.WriteBit(0);                   // Unk bit - updating rotation ?
    data.WriteBit(attackerGuid[5]);
    data.WriteBit(victimGuid[7]);
    data.WriteBit(attackerGuid[7]);
    data.WriteBit(attackerGuid[2]);
    data.WriteBit(attackerGuid[4]);

    data.FlushBits();

    data.WriteByteSeq(victimGuid[5]);
    data.WriteByteSeq(attackerGuid[0]);
    data.WriteByteSeq(attackerGuid[6]);
    data.WriteByteSeq(victimGuid[1]);
    data.WriteByteSeq(victimGuid[3]);
    data.WriteByteSeq(victimGuid[6]);
    data.WriteByteSeq(victimGuid[7]);
    data.WriteByteSeq(victimGuid[0]);
    data.WriteByteSeq(attackerGuid[4]);
    data.WriteByteSeq(attackerGuid[1]);
    data.WriteByteSeq(attackerGuid[7]);
    data.WriteByteSeq(victimGuid[4]);
    data.WriteByteSeq(attackerGuid[3]);
    data.WriteByteSeq(attackerGuid[5]);
    data.WriteByteSeq(victimGuid[2]);
    data.WriteByteSeq(attackerGuid[2]);
    SendMessageToSet(&data, true);
    sLog->outDebug(LOG_FILTER_UNITS, "WORLD: Sent SMSG_ATTACKSTOP");

    if (victim)
        sLog->outInfo(LOG_FILTER_UNITS, "%s %u stopped attacking %s %u", (GetTypeId() == TYPEID_PLAYER ? "Player" : "Creature"), GetGUIDLow(), (victim->GetTypeId() == TYPEID_PLAYER ? "player" : "creature"), victim->GetGUIDLow());
    else
        sLog->outInfo(LOG_FILTER_UNITS, "%s %u stopped attacking", (GetTypeId() == TYPEID_PLAYER ? "Player" : "Creature"), GetGUIDLow());
}

bool Unit::isSpellBlocked(Unit* victim, SpellInfo const* spellProto, WeaponAttackType /*attackType*/)
{
    // These spells can't be blocked
    if (spellProto && spellProto->Attributes & SPELL_ATTR0_IMPOSSIBLE_DODGE_PARRY_BLOCK)
        return false;

    if (victim->HasAuraType(SPELL_AURA_IGNORE_HIT_DIRECTION) || victim->HasInArc(M_PI, this))
    {
        // Check creatures flags_extra for disable block
        if (victim->GetTypeId() == TYPEID_UNIT &&
            victim->ToCreature()->GetCreatureTemplate()->flags_extra & CREATURE_FLAG_EXTRA_NO_BLOCK)
                return false;

        if (roll_chance_f(victim->GetUnitBlockChance()))
            return true;
    }
    return false;
}

bool Unit::isBlockCritical()
{
    if (roll_chance_i(GetTotalAuraModifier(SPELL_AURA_MOD_BLOCK_CRIT_CHANCE)))
    {
        // Critical Blocks enrage the warrior
        if (HasAura(76857))
            CastSpell(this, 12880, true);
        return true;
    }

    return false;
}

int32 Unit::GetMechanicResistChance(const SpellInfo* spell)
{
    if (!spell)
        return 0;
    int32 resist_mech = 0;
    for (uint8 eff = 0; eff < MAX_SPELL_EFFECTS; ++eff)
    {
        if (!spell->GetEffect(eff, GetSpawnMode())->IsEffect())
           break;
        int32 effect_mech = spell->GetEffectMechanic(eff);
        if (effect_mech)
        {
            int32 temp = GetTotalAuraModifierByMiscValue(SPELL_AURA_MOD_MECHANIC_RESISTANCE, effect_mech);
            if (resist_mech < temp)
                resist_mech = temp;
        }
    }
    return resist_mech;
}

// Melee based spells hit result calculations
SpellMissInfo Unit::MeleeSpellHitResult(Unit* victim, SpellInfo const* spell)
{
    // Spells with SPELL_ATTR3_IGNORE_HIT_RESULT will additionally fully ignore
    // resist and deflect chances
    if (spell->AttributesEx3 & SPELL_ATTR3_IGNORE_HIT_RESULT)
        return SPELL_MISS_NONE;

    WeaponAttackType attType = BASE_ATTACK;

    // Check damage class instead of attack type to correctly handle judgements
    // - they are meele, but can't be dodged/parried/deflected because of ranged dmg class
    if (spell->DmgClass == SPELL_DAMAGE_CLASS_RANGED)
        attType = RANGED_ATTACK;

    uint32 roll = urand (0, 10000);

    uint32 missChance = uint32(MeleeSpellMissChance(victim, attType, spell->Id) * 100.0f);
    // Roll miss
    uint32 tmp = missChance;
    if (roll < tmp)
        return SPELL_MISS_MISS;

    // Chance resist mechanic (select max value from every mechanic spell effect)
    int32 resist_mech = 0;
    // Get effects mechanic and chance
    for (uint8 eff = 0; eff < MAX_SPELL_EFFECTS; ++eff)
    {
        int32 effect_mech = spell->GetEffectMechanic(eff);
        if (effect_mech)
        {
            int32 temp = victim->GetTotalAuraModifierByMiscValue(SPELL_AURA_MOD_MECHANIC_RESISTANCE, effect_mech);
            if (resist_mech < temp*100)
                resist_mech = temp*100;
        }
    }
    // Roll chance
    tmp += resist_mech;
    if (roll < tmp)
        return SPELL_MISS_RESIST;

    bool canDodge = true;
    bool canParry = true;
    bool canBlock = spell->AttributesEx3 & SPELL_ATTR3_BLOCKABLE_SPELL;

    // Same spells cannot be parry/dodge
    if (spell->Attributes & SPELL_ATTR0_IMPOSSIBLE_DODGE_PARRY_BLOCK)
        return SPELL_MISS_NONE;

    // Chance resist mechanic
    int32 resist_chance = victim->GetMechanicResistChance(spell) * 100;
    tmp += resist_chance;
    if (roll < tmp)
        return SPELL_MISS_RESIST;

    // Ranged attacks can only miss, resist and deflect
    if (attType == RANGED_ATTACK)
    {
        // only if in front
        if (victim->HasInArc(M_PI, this) || victim->HasAuraType(SPELL_AURA_IGNORE_HIT_DIRECTION))
        {
            int32 deflect_chance = victim->GetTotalAuraModifier(SPELL_AURA_DEFLECT_SPELLS) * 100;
            tmp+=deflect_chance;
            if (roll < tmp)
                return SPELL_MISS_DEFLECT;
        }

        if (spell->EquippedItemClass > 0)
            canDodge = true;
        else 
            return SPELL_MISS_NONE;
    }

    // Check for attack from behind
    if (!victim->HasInArc(M_PI, this))
    {
        if (!victim->HasAuraType(SPELL_AURA_IGNORE_HIT_DIRECTION))
        {
            // Can`t dodge from behind in PvP (but its possible in PvE)
            if (victim->GetTypeId() == TYPEID_PLAYER)
                canDodge = false;
            // Can`t parry or block
            canParry = false;
            canBlock = false;
        }
        else // Only deterrence as of 3.3.5
        {
            if (spell->AttributesCu & SPELL_ATTR0_CU_REQ_CASTER_BEHIND_TARGET)
                canParry = false;
        }
    }
    // Check creatures flags_extra for disable parry
    if (victim->GetTypeId() == TYPEID_UNIT)
    {
        uint32 flagEx = victim->ToCreature()->GetCreatureTemplate()->flags_extra;
        if (flagEx & CREATURE_FLAG_EXTRA_NO_PARRY)
            canParry = false;
        // Check creatures flags_extra for disable block
        if (flagEx & CREATURE_FLAG_EXTRA_NO_BLOCK)
            canBlock = false;
    }

    switch (spell->Id)
    {
        case 24275: // Hammer of Wrath
        case 49998: // Death Strike
        {
            canParry = false;
            break;
        }
        default:
            break;
    }

    // Ignore combat result aura
    AuraEffectList ignore = GetAuraEffectsByType(SPELL_AURA_IGNORE_COMBAT_RESULT);
    for (AuraEffectList::const_iterator i = ignore.begin(); i != ignore.end(); ++i)
    {
        if (!(*i)->IsAffectingSpell(spell))
            continue;
        switch ((*i)->GetMiscValue())
        {
            case MELEE_HIT_DODGE: canDodge = false; break;
            case MELEE_HIT_BLOCK: canBlock = false; break;
            case MELEE_HIT_PARRY: canParry = false; break;
            default:
                sLog->outDebug(LOG_FILTER_UNITS, "Spell %u SPELL_AURA_IGNORE_COMBAT_RESULT has unhandled state %d", (*i)->GetId(), (*i)->GetMiscValue());
                break;
        }
    }

    if (canDodge)
    {
        int32 lvldiff = victim->getLevel() - getLevel();
        int32 dodgelvl = lvldiff * 150;
        // Roll dodge
        int32 dodgeChance = int32(victim->GetUnitDodgeChance() * 100.0f);
        dodgeChance += dodgelvl;
        // Reduce enemy dodge chance by SPELL_AURA_MOD_COMBAT_RESULT_CHANCE
        dodgeChance += GetTotalAuraModifierByMiscValue(SPELL_AURA_MOD_COMBAT_RESULT_CHANCE, VICTIMSTATE_DODGE) * 100;
        dodgeChance = int32(float(dodgeChance) * GetTotalAuraMultiplier(SPELL_AURA_MOD_ENEMY_DODGE));
        // Reduce dodge chance by attacker expertise rating
        if (GetTypeId() == TYPEID_PLAYER)
            dodgeChance -= int32(ToPlayer()->GetExpertiseDodgeOrParryReduction(attType) * 100.0f);
        else
        {
            dodgeChance -= m_expertise * 100.0f;
            dodgeChance -= GetTotalAuraModifier(SPELL_AURA_MOD_EXPERTISE) * 25;
        }
        if (dodgeChance < 0)
            dodgeChance = 0;

        if (roll < (tmp += dodgeChance))
            return SPELL_MISS_DODGE;
        else if (attType == RANGED_ATTACK)
            return SPELL_MISS_NONE;
    }

    if (canParry)
    {
        // Roll parry
        int32 parryChance = int32(victim->GetUnitParryChance() * 100.0f);
        // Reduce parry chance by attacker expertise rating
        if (GetTypeId() == TYPEID_PLAYER)
            parryChance -= int32(ToPlayer()->GetExpertiseDodgeOrParryReduction(attType) * 100.0f);
        else
        {
            parryChance -= m_expertise * 100.0f;
            parryChance -= GetTotalAuraModifier(SPELL_AURA_MOD_EXPERTISE) * 25;
        }
        if (parryChance < 0)
            parryChance = 0;

        tmp += parryChance;
        if (roll < tmp)
            return SPELL_MISS_PARRY;
    }

    if (canBlock)
    {
        int32 blockChance = int32(victim->GetUnitBlockChance() * 100.0f);
        if (blockChance < 0)
            blockChance = 0;
        tmp += blockChance;

        if (roll < tmp)
            return SPELL_MISS_BLOCK;
    }

    return SPELL_MISS_NONE;
}

// TODO need use unit spell resistances in calculations
SpellMissInfo Unit::MagicSpellHitResult(Unit* victim, SpellInfo const* spell)
{
    if (spell->AttributesEx3 & SPELL_ATTR3_IGNORE_HIT_RESULT)
        return SPELL_MISS_NONE;

    // Can`t miss on dead target (on skinning for example)
    if (!victim->isAlive() && victim->GetTypeId() != TYPEID_PLAYER)
        return SPELL_MISS_NONE;

    SpellSchoolMask schoolMask = spell->GetSchoolMask();
    int32 thisLevel = getLevelForTarget(victim);
    if (GetTypeId() == TYPEID_UNIT && ToCreature()->isTrigger())
        thisLevel = std::max<int32>(thisLevel, spell->SpellLevel);
    int32 leveldif = int32(victim->getLevelForTarget(this)) - thisLevel;

    // Base hit chance from attacker and victim levels

    // | caster | target | miss 
    //    90        90      6
    //    90        91      9
    //    90        92     12
    //    90        93     15

    int32 modHitChance = 94;
    modHitChance -= leveldif * 3;

    // Spellmod from SPELLMOD_RESIST_MISS_CHANCE
    if (Player* modOwner = GetSpellModOwner())
        modOwner->ApplySpellMod(spell->Id, SPELLMOD_RESIST_MISS_CHANCE, modHitChance);

    int32 bestVal = 0;
    bool firstCheck = false;
        
    for (uint8 i = SPELL_SCHOOL_NORMAL; i < MAX_SPELL_SCHOOL; ++i)
        if (schoolMask & (1 << i))
        {
            int32 amount = victim->GetTotalAuraModifierByMiscMask(SPELL_AURA_MOD_ATTACKER_SPELL_HIT_CHANCE, (1 << i)); // Chance hit from victim SPELL_AURA_MOD_ATTACKER_SPELL_HIT_CHANCE auras
            if (bestVal < amount || !firstCheck)
            {
                bestVal = amount;
                firstCheck = true;
            }
        }

    modHitChance += bestVal;

    int32 HitChance = modHitChance * 100;
    // Increase hit chance from attacker SPELL_AURA_MOD_SPELL_HIT_CHANCE and attacker ratings
    HitChance += int32((m_modSpellHitChance + m_expertise) * 100.0f);

    if (HitChance < 100)
        HitChance = 100;
    else if (HitChance > 10000)
        HitChance = 10000;

    int32 tmp = 10000 - HitChance;

    int32 rand = irand(0, 10000);

    if (rand < tmp)
        return SPELL_MISS_MISS;

    // Spells with SPELL_ATTR3_IGNORE_HIT_RESULT will additionally fully ignore
    // resist and deflect chances
    if (spell->AttributesEx3 & SPELL_ATTR3_IGNORE_HIT_RESULT)
        return SPELL_MISS_NONE;

    // Chance resist mechanic (select max value from every mechanic spell effect)
    int32 resist_chance = victim->GetMechanicResistChance(spell) * 100;
    tmp += resist_chance;

   // Roll chance
    if (rand < tmp)
        return SPELL_MISS_RESIST;

    // cast by caster in front of victim
    if (victim->HasInArc(M_PI, this) || victim->HasAuraType(SPELL_AURA_IGNORE_HIT_DIRECTION))
    {
        int32 deflect_chance = victim->GetTotalAuraModifier(SPELL_AURA_DEFLECT_SPELLS) * 100;
        tmp += deflect_chance;
        if (rand < tmp)
            return SPELL_MISS_DEFLECT;

        int32 deflectFromFront = victim->GetTotalAuraModifier(SPELL_AURA_MOD_DEFLECT_SPELLS_FROM_FRONT) * 100;
        tmp += deflectFromFront;
        if (rand < tmp)
            return SPELL_MISS_DEFLECT;
    }

    return SPELL_MISS_NONE;
}

// Calculate spell hit result can be:
// Every spell can: Evade/Immune/Reflect/Sucesful hit
// For melee based spells:
//   Miss
//   Dodge
//   Parry
// For spells
//   Resist
SpellMissInfo Unit::SpellHitResult(Unit* victim, SpellInfo const* spell, bool CanReflect, uint32 effectMask)
{
    // Check for immune
    if (this != victim)
    {
        if (victim->IsImmunedToSpell(spell))
            return SPELL_MISS_IMMUNE;
    }

    // All positive spells can`t miss
    // TODO: client not show miss log for this spells - so need find info for this in dbc and use it!
    bool positive = false;
    for (uint32 effIndex = 0; effIndex < MAX_SPELL_EFFECTS; ++effIndex)
        if (effectMask & (1 << effIndex))
            positive = spell->IsPositiveEffect(effIndex);

    if (positive && !IsHostileTo(victim))  // prevent from affecting enemy by "positive" spell
        return SPELL_MISS_NONE;

    // Check for immune
    if (victim->IsImmunedToDamage(spell))
        return SPELL_MISS_IMMUNE;

    if (this == victim)
        return SPELL_MISS_NONE;

    // Return evade for units in evade mode
    if (victim->GetTypeId() == TYPEID_UNIT && victim->ToCreature()->IsInEvadeMode())
        return SPELL_MISS_EVADE;

    // Try victim reflect spell
    if (CanReflect)
    {
        int32 reflectchance = victim->GetTotalAuraModifier(SPELL_AURA_REFLECT_SPELLS);
        Unit::AuraEffectList mReflectSpellsSchool = victim->GetAuraEffectsByType(SPELL_AURA_REFLECT_SPELLS_SCHOOL);
        for (Unit::AuraEffectList::const_iterator i = mReflectSpellsSchool.begin(); i != mReflectSpellsSchool.end(); ++i)
            if ((*i)->GetMiscValue() & spell->GetSchoolMask())
                reflectchance += (*i)->GetAmount();
        if (reflectchance > 0 && roll_chance_i(reflectchance))
        {
            // Start triggers for remove charges if need (trigger only for victim, and mark as active spell)
            //ProcDamageAndSpell(victim, PROC_FLAG_NONE, PROC_FLAG_TAKEN_SPELL_MAGIC_DMG_CLASS_NEG, PROC_EX_REFLECT, 1, BASE_ATTACK, spell);
            return SPELL_MISS_REFLECT;
        }
    }

    Unit* checker = (GetTypeId() == TYPEID_UNIT && GetAnyOwner()) ? GetAnyOwner() : this;
    switch (spell->DmgClass)
    {
        case SPELL_DAMAGE_CLASS_RANGED:
        case SPELL_DAMAGE_CLASS_MELEE:
            return checker->MeleeSpellHitResult(victim, spell);
        case SPELL_DAMAGE_CLASS_NONE:
            return SPELL_MISS_NONE;
        case SPELL_DAMAGE_CLASS_MAGIC:
            return checker->MagicSpellHitResult(victim, spell);
    }
    return SPELL_MISS_NONE;
}

float Unit::GetUnitDodgeChance() const
{
    if (!HasAuraWithAttribute(10, SPELL_ATTR10_CAN_DODGE_ON_CAST) && (IsNonMeleeSpellCasted(false) || HasUnitState(UNIT_STATE_CONTROLLED)))
        return 0.0f;

    if (GetTypeId() == TYPEID_PLAYER)
        return GetFloatValue(PLAYER_DODGE_PERCENTAGE);
    else
    {
        if (ToCreature()->isTotem())
            return 0.0f;
        else
        {
            float dodge = 3.0f;
            dodge += GetTotalAuraModifier(SPELL_AURA_MOD_DODGE_PERCENT);
            return dodge > 0.0f ? dodge : 0.0f;
        }
    }
}

float Unit::GetUnitParryChance() const
{
    if (!HasAuraWithAttribute(10, SPELL_ATTR10_CAN_PARRY_ON_CAST) && (IsNonMeleeSpellCasted(false) || HasUnitState(UNIT_STATE_CONTROLLED)))
        return 0.0f;

    float chance = 0.0f;

    if (Player const* player = ToPlayer())
    {
        if (player->CanParry())
        {
            Item* tmpitem = player->GetWeaponForAttack(BASE_ATTACK, true);
            if (!tmpitem)
                tmpitem = player->GetWeaponForAttack(OFF_ATTACK, true);

            if (tmpitem)
                chance = GetFloatValue(PLAYER_PARRY_PERCENTAGE);
        }
    }
    else if (GetTypeId() == TYPEID_UNIT)
    {
        if (GetCreatureType() == CREATURE_TYPE_HUMANOID)
        {
            chance = 5.0f;
            chance += GetTotalAuraModifier(SPELL_AURA_MOD_PARRY_PERCENT);
            chance += GetTotalAuraModifier(SPELL_AURA_MOD_PARRY_PERCENT2);
        }
    }

    return chance > 0.0f ? chance : 0.0f;
}

float Unit::GetUnitMissChance(WeaponAttackType attType) const
{
    float miss_chance = 3.00f;

    if (attType == RANGED_ATTACK)
        miss_chance -= GetTotalAuraModifier(SPELL_AURA_MOD_ATTACKER_RANGED_HIT_CHANCE);
    else
        miss_chance -= GetTotalAuraModifier(SPELL_AURA_MOD_ATTACKER_MELEE_HIT_CHANCE);

    return miss_chance;
}

float Unit::GetUnitBlockChance() const
{
    if (!HasAuraWithAttribute(10, SPELL_ATTR10_CAN_PARRY_ON_CAST) && (IsNonMeleeSpellCasted(false) || HasUnitState(UNIT_STATE_CONTROLLED)))
        return 0.0f;

    if (Player const* player = ToPlayer())
    {
        if (player->CanBlock())
        {
            Item* tmpitem = player->GetUseableItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_OFFHAND);
            if (tmpitem && !tmpitem->IsBroken())
                return GetFloatValue(PLAYER_BLOCK_PERCENTAGE);
        }
        // is player but has no block ability or no not broken shield equipped
        return 0.0f;
    }
    else
    {
        if (ToCreature()->isTotem())
            return 0.0f;
        else
        {
            float block = 5.0f;
            block += GetTotalAuraModifier(SPELL_AURA_MOD_BLOCK_PERCENT);
            return block > 0.0f ? block : 0.0f;
        }
    }
}

float Unit::GetUnitCriticalChance(WeaponAttackType attackType, const Unit* victim, SpellInfo const* spellProto) const
{
    float crit;

    if (GetTypeId() == TYPEID_PLAYER)
    {
        switch (attackType)
        {
            case BASE_ATTACK:
                crit = GetFloatValue(PLAYER_CRIT_PERCENTAGE);
                break;
            case OFF_ATTACK:
                crit = GetFloatValue(PLAYER_OFFHAND_CRIT_PERCENTAGE);
                break;
            case RANGED_ATTACK:
                crit = GetFloatValue(PLAYER_RANGED_CRIT_PERCENTAGE);
                break;
                // Just for good manner
            default:
                crit = 0.0f;
                break;
        }
    }
    else
    {
        crit = 5.0f;
        crit += GetTotalAuraModifier(SPELL_AURA_MOD_WEAPON_CRIT_PERCENT);
        crit += GetTotalAuraModifier(SPELL_AURA_MOD_CRIT_PCT);
    }

    // flat aura mods
    if (attackType == RANGED_ATTACK)
        crit += victim->GetTotalAuraModifier(SPELL_AURA_MOD_ATTACKER_RANGED_CRIT_CHANCE);
    else
        crit += victim->GetTotalAuraModifier(SPELL_AURA_MOD_ATTACKER_MELEE_CRIT_CHANCE);

    int32 critMod = victim->GetTotalAuraModifier(SPELL_AURA_MOD_ATTACKER_SPELL_AND_WEAPON_CRIT_CHANCE);

    if (spellProto && critMod < 0)
    {
        if (!spellProto->IsPositive())
            crit += critMod;
    }
    else
        crit += critMod;

    if (crit < 0.0f)
        crit = 0.0f;
    return countCrit ? countCrit: crit;
}

void Unit::_DeleteRemovedAuras()
{
    while (!m_removedAuras.empty())
    {
        delete m_removedAuras.front();
        m_removedAuras.pop_front();
    }
}

void Unit::_UpdateSpells(uint32 time)
{
    if (!IsInWorld())
        return;

    if (m_currentSpells[CURRENT_AUTOREPEAT_SPELL])
        _UpdateAutoRepeatSpell();

    // remove finished spells from current pointers
    for (uint32 i = 0; i < CURRENT_MAX_SPELL; ++i)
    {
        if (m_currentSpells[i] && m_currentSpells[i]->getState() == SPELL_STATE_FINISHED)
        {
            m_currentSpells[i]->SetReferencedFromCurrent(false);
            m_currentSpells[i] = NULL;                      // remove pointer
        }
    }

    // m_auraUpdateIterator can be updated in indirect called code at aura remove to skip next planned to update but removed auras
    for (m_auraUpdateIterator = m_ownedAuras.begin(); m_auraUpdateIterator != m_ownedAuras.end();)
    {
        Aura* i_aura = m_auraUpdateIterator->second;
        ++m_auraUpdateIterator;                            // need shift to next for allow update if need into aura update
        i_aura->UpdateOwner(time, this);
    }

    // remove expired auras - do that after updates(used in scripts?)
    for (AuraMap::iterator i = m_ownedAuras.begin(); i != m_ownedAuras.end();)
    {
        if (i->second->IsExpired())
            RemoveOwnedAura(i, AURA_REMOVE_BY_EXPIRE);
        else
            ++i;
    }

    for (VisibleAuraMap::iterator itr = m_visibleAuras.begin(); itr != m_visibleAuras.end(); ++itr)
        if (itr->second->IsNeedClientUpdate())
            itr->second->ClientUpdate();

    _DeleteRemovedAuras();

    if (!m_gameObj.empty())
    {
        GameObjectList::iterator itr;
        for (itr = m_gameObj.begin(); itr != m_gameObj.end();)
        {
            if (!(*itr)->isSpawned())
            {
                (*itr)->SetOwnerGUID(0);
                (*itr)->SetRespawnTime(0);
                (*itr)->Delete();
                m_gameObj.erase(itr++);
            }
            else
                ++itr;
        }
    }
}

void Unit::_UpdateAutoRepeatSpell()
{
    // check "real time" interrupts
    // don't cancel spells which are affected by a SPELL_AURA_CAST_WHILE_WALKING effect
    if (((GetTypeId() == TYPEID_PLAYER && ToPlayer()->isMoving()) || IsNonMeleeSpellCasted(false, false, true, m_currentSpells[CURRENT_AUTOREPEAT_SPELL]->m_spellInfo->Id == 75)) && 
        !HasAuraCastWhileWalking(m_currentSpells[CURRENT_AUTOREPEAT_SPELL]->m_spellInfo))
    {
        // cancel wand shoot
        if (m_currentSpells[CURRENT_AUTOREPEAT_SPELL]->m_spellInfo->Id != 75)
            InterruptSpell(CURRENT_AUTOREPEAT_SPELL);
        m_AutoRepeatFirstCast = true;
        return;
    }

    // apply delay (Auto Shot - 75 not affected)
    if (m_AutoRepeatFirstCast && getAttackTimer(RANGED_ATTACK) < 500 && m_currentSpells[CURRENT_AUTOREPEAT_SPELL]->m_spellInfo->Id != 75)
        setAttackTimer(RANGED_ATTACK, 500);
    m_AutoRepeatFirstCast = false;

    // castroutine
    if (isAttackReady(RANGED_ATTACK))
    {
        // Check if able to cast
        if (m_currentSpells[CURRENT_AUTOREPEAT_SPELL]->CheckCast(true) != SPELL_CAST_OK)
        {
            if (m_attacking)
            {
                m_attacking->_removeAttacker(this);
                m_attacking = NULL;
            }
            InterruptSpell(CURRENT_AUTOREPEAT_SPELL);
            return;
        }

        if (!m_attacking)
            if (Unit* curspellTarget = m_currentSpells[CURRENT_AUTOREPEAT_SPELL]->m_targets.GetUnitTarget())
            {
                m_attacking = curspellTarget;
                m_attacking->_addAttacker(this);
            }

        // we want to shoot
        Spell* spell = new Spell(this, m_currentSpells[CURRENT_AUTOREPEAT_SPELL]->m_spellInfo, TRIGGERED_FULL_MASK);
        spell->prepare(&(m_currentSpells[CURRENT_AUTOREPEAT_SPELL]->m_targets));

        // all went good, reset attack
        resetAttackTimer(RANGED_ATTACK);
    }
}

void Unit::SetCurrentCastedSpell(Spell* pSpell)
{
    ASSERT(pSpell);                                         // NULL may be never passed here, use InterruptSpell or InterruptNonMeleeSpells

    CurrentSpellTypes CSpellType = pSpell->GetCurrentContainer();

    if (pSpell == m_currentSpells[CSpellType])             // avoid breaking self
        return;

    // break same type spell if it is not delayed
    if (SpellInfo const* _spellInfo = pSpell->GetSpellInfo())
        if (!(_spellInfo->AttributesCu & SPELL_ATTR0_CU_DOESENT_INTERRUPT_CHANNELING))
            InterruptSpell(CSpellType, false);

    // special breakage effects:
    switch (CSpellType)
    {
        case CURRENT_GENERIC_SPELL:
        {
            // generic spells always break channeled not delayed spells
            InterruptSpell(CURRENT_CHANNELED_SPELL, false);

            // autorepeat breaking
            if (m_currentSpells[CURRENT_AUTOREPEAT_SPELL])
            {
                // break autorepeat if not Auto Shot
                if (m_currentSpells[CURRENT_AUTOREPEAT_SPELL]->m_spellInfo->Id != 75)
                    InterruptSpell(CURRENT_AUTOREPEAT_SPELL);
                m_AutoRepeatFirstCast = true;
            }
            if (pSpell->m_spellInfo->CalcCastTime(this) > 0)
                AddUnitState(UNIT_STATE_CASTING);

            break;
        }
        case CURRENT_CHANNELED_SPELL:
        {
            // channel spells always break generic non-delayed and any channeled spells
            InterruptSpell(CURRENT_GENERIC_SPELL, false);
            InterruptSpell(CURRENT_CHANNELED_SPELL);

            // it also does break autorepeat if not Auto Shot
            if (m_currentSpells[CURRENT_AUTOREPEAT_SPELL] &&
                m_currentSpells[CURRENT_AUTOREPEAT_SPELL]->m_spellInfo->Id != 75)
                InterruptSpell(CURRENT_AUTOREPEAT_SPELL);

            AddUnitState(UNIT_STATE_CASTING);
            if (pSpell->GetCaster()->ToCreature())
                if (!(pSpell->m_spellInfo->ChannelInterruptFlags & CHANNEL_INTERRUPT_FLAG_INTERRUPT))
                    AddUnitState(UNIT_STATE_MOVE_IN_CASTING);

            break;
        }
        case CURRENT_AUTOREPEAT_SPELL:
        {
            // only Auto Shoot does not break anything
            if (pSpell->m_spellInfo->Id != 75)
            {
                // generic autorepeats break generic non-delayed and channeled non-delayed spells
                InterruptSpell(CURRENT_GENERIC_SPELL, false);
                InterruptSpell(CURRENT_CHANNELED_SPELL, false);
            }
            // special action: set first cast flag
            m_AutoRepeatFirstCast = true;

            break;
        }
        default:
            break; // other spell types don't break anything now
    }

    // current spell (if it is still here) may be safely deleted now
    if (m_currentSpells[CSpellType])
        m_currentSpells[CSpellType]->SetReferencedFromCurrent(false);

    // set new current spell
    m_currentSpells[CSpellType] = pSpell;
    pSpell->SetReferencedFromCurrent(true);

    pSpell->m_selfContainer = &(m_currentSpells[pSpell->GetCurrentContainer()]);
}

void Unit::InterruptSpell(CurrentSpellTypes spellType, bool withDelayed, bool withInstant)
{
    ASSERT(spellType < CURRENT_MAX_SPELL);

    //sLog->outDebug(LOG_FILTER_UNITS, "Interrupt spell for unit %u.", GetEntry());
    Spell* spell = m_currentSpells[spellType];
    if (spell
        && (withDelayed || spell->getState() != SPELL_STATE_DELAYED)
        && (withInstant || spell->GetCastTime() > 0))
    {
        // for example, do not let self-stun aura interrupt itself
        if (!spell->IsInterruptable())
            return;

        // send autorepeat cancel message for autorepeat spells
        if (spellType == CURRENT_AUTOREPEAT_SPELL)
            if (GetTypeId() == TYPEID_PLAYER)
                ToPlayer()->SendAutoRepeatCancel(this);

        if (spell->getState() != SPELL_STATE_FINISHED)
            spell->cancel();

        m_currentSpells[spellType] = NULL;
        spell->SetReferencedFromCurrent(false);
    }
}

void Unit::FinishSpell(CurrentSpellTypes spellType, bool ok /*= true*/)
{
    Spell* spell = m_currentSpells[spellType];
    if (!spell)
        return;

    if (spellType == CURRENT_CHANNELED_SPELL)
        spell->SendChannelUpdate(0);

    spell->finish(ok);
}

bool Unit::IsNonMeleeSpellCasted(bool withDelayed, bool skipChanneled, bool skipAutorepeat, bool isAutoshoot, bool skipInstant) const
{
    // We don't do loop here to explicitly show that melee spell is excluded.
    // Maybe later some special spells will be excluded too.

    // if skipInstant then instant spells shouldn't count as being casted
    if (skipInstant && m_currentSpells[CURRENT_GENERIC_SPELL] && !m_currentSpells[CURRENT_GENERIC_SPELL]->GetCastTime())
        return false;

    // generic spells are casted when they are not finished and not delayed
    if (m_currentSpells[CURRENT_GENERIC_SPELL] &&
        (m_currentSpells[CURRENT_GENERIC_SPELL]->getState() != SPELL_STATE_FINISHED) &&
        (withDelayed || m_currentSpells[CURRENT_GENERIC_SPELL]->getState() != SPELL_STATE_DELAYED))
    {
        if (!isAutoshoot || !(m_currentSpells[CURRENT_GENERIC_SPELL]->m_spellInfo->AttributesEx2 & SPELL_ATTR2_NOT_RESET_AUTO_ACTIONS))
            return true;
    }
    // channeled spells may be delayed, but they are still considered casted
    else if (!skipChanneled && m_currentSpells[CURRENT_CHANNELED_SPELL] &&
        (m_currentSpells[CURRENT_CHANNELED_SPELL]->getState() != SPELL_STATE_FINISHED))
    {
        if (!isAutoshoot || !(m_currentSpells[CURRENT_CHANNELED_SPELL]->m_spellInfo->AttributesEx2 & SPELL_ATTR2_NOT_RESET_AUTO_ACTIONS))
            return true;
    }
    // autorepeat spells may be finished or delayed, but they are still considered casted
    else if (!skipAutorepeat && m_currentSpells[CURRENT_AUTOREPEAT_SPELL])
        return true;

    return false;
}

void Unit::InterruptNonMeleeSpells(bool withDelayed, uint32 spell_id, bool withInstant)
{
    // generic spells are interrupted if they are not finished or delayed
    if (m_currentSpells[CURRENT_GENERIC_SPELL] && (!spell_id || m_currentSpells[CURRENT_GENERIC_SPELL]->m_spellInfo->Id == spell_id))
        InterruptSpell(CURRENT_GENERIC_SPELL, withDelayed, withInstant);

    // autorepeat spells are interrupted if they are not finished or delayed
    if (m_currentSpells[CURRENT_AUTOREPEAT_SPELL] && (!spell_id || m_currentSpells[CURRENT_AUTOREPEAT_SPELL]->m_spellInfo->Id == spell_id))
        InterruptSpell(CURRENT_AUTOREPEAT_SPELL, withDelayed, withInstant);

    // channeled spells are interrupted if they are not finished, even if they are delayed
    if (m_currentSpells[CURRENT_CHANNELED_SPELL] && (!spell_id || m_currentSpells[CURRENT_CHANNELED_SPELL]->m_spellInfo->Id == spell_id))
        InterruptSpell(CURRENT_CHANNELED_SPELL, true, true);
}

Spell* Unit::FindCurrentSpellBySpellId(uint32 spell_id) const
{
    for (uint32 i = 0; i < CURRENT_MAX_SPELL; i++)
        if (m_currentSpells[i] && m_currentSpells[i]->m_spellInfo->Id == spell_id)
            return m_currentSpells[i];
    return NULL;
}

int32 Unit::GetCurrentSpellCastTime(uint32 spell_id) const
{
    if (Spell const* spell = FindCurrentSpellBySpellId(spell_id))
        return spell->GetCastTime();
    return 0;
}

bool Unit::isInFrontInMap(Unit const* target, float distance,  float arc) const
{
    return IsWithinDistInMap(target, distance) && HasInArc(arc, target);
}

bool Unit::isInBackInMap(Unit const* target, float distance, float arc) const
{
    return IsWithinDistInMap(target, distance) && !HasInArc(2 * M_PI - arc, target);
}

bool Unit::isInAccessiblePlaceFor(Creature const* c) const
{
    if(!c)
        return false;

    if (IsInWater())
        return c->canSwim();
    else
        return c->canWalk() || c->CanFly();
}

bool Unit::IsInWater() const
{
    return Zliquid_status & (LIQUID_MAP_UNDER_WATER | LIQUID_MAP_IN_WATER);
}

bool Unit::IsUnderWater() const
{
    return Zliquid_status & LIQUID_MAP_UNDER_WATER;
}

void Unit::UpdateUnderwaterState(Map* m, float x, float y, float z)
{
    if (!isPet() && !IsVehicle())
        return;

    Zliquid_status = m->getLiquidStatus(x, y, z, MAP_ALL_LIQUIDS, &liquid_status);
    if (!Zliquid_status)
    {
        if (_lastLiquid && _lastLiquid->SpellId)
            RemoveAurasDueToSpell(_lastLiquid->SpellId);

        RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_NOT_UNDERWATER);
        _lastLiquid = NULL;
        return;
    }

    if (uint32 liqEntry = liquid_status.entry)
    {
        LiquidTypeEntry const* liquid = sLiquidTypeStore.LookupEntry(liqEntry);
        if (_lastLiquid && _lastLiquid->SpellId && _lastLiquid->Id != liqEntry)
            RemoveAurasDueToSpell(_lastLiquid->SpellId);

        if (liquid && liquid->SpellId)
        {
            if (Zliquid_status & (LIQUID_MAP_UNDER_WATER | LIQUID_MAP_IN_WATER))
            {
                if (!HasAura(liquid->SpellId))
                    CastSpell(this, liquid->SpellId, true);
            }
            else
                RemoveAurasDueToSpell(liquid->SpellId);
        }

        RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_NOT_ABOVEWATER);
        _lastLiquid = liquid;
    }
    else if (_lastLiquid && _lastLiquid->SpellId)
    {
        RemoveAurasDueToSpell(_lastLiquid->SpellId);
        RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_NOT_UNDERWATER);
        _lastLiquid = NULL;
    }
}

void Unit::DeMorph()
{
    SetDisplayId(GetNativeDisplayId());
}

Aura* Unit::_TryStackingOrRefreshingExistingAura(SpellInfo const* newAura, uint32 effMask, Unit* caster, int32* baseAmount /*= NULL*/, Item* castItem /*= NULL*/, uint64 casterGUID /*= 0*/)
{
    ASSERT(casterGUID || caster);
    if (!casterGUID)
    {
        if(!caster->ToCreature() || caster->isAnySummons() || isAnySummons())
            casterGUID = caster->GetGUID();
        else if(!newAura->StackAmount)
            casterGUID = caster->GetGUID();
    }

    // passive and Incanter's Absorption and auras with different type can stack with themselves any number of times
    if (!newAura->IsMultiSlotAura())
    {
        // check if cast item changed
        uint64 castItemGUID = 0;
        if (castItem)
            castItemGUID = castItem->GetGUID();

        // find current aura from spell and change it's stackamount, or refresh it's duration
        Aura* foundAura = GetOwnedAura(newAura->Id, casterGUID, (newAura->AttributesCu & SPELL_ATTR0_CU_ENCHANT_PROC) ? castItemGUID : 0, 0);
        if (foundAura != NULL)
        {
            // effect masks do not match
            // extremely rare case
            // let's just recreate aura
            if (effMask != foundAura->GetEffectMask())
                return NULL;

            // update basepoints with new values - effect amount will be recalculated in ModStackAmount
            for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
            {
                if (!foundAura->HasEffect(i))
                    continue;

                int bp;
                if (baseAmount)
                    bp = *(baseAmount + i);
                else
                    bp = foundAura->GetSpellInfo()->GetEffect(i, GetSpawnMode())->BasePoints;

                int32* oldBP = const_cast<int32*>(&(foundAura->GetEffect(i)->m_baseAmount));
                int32* oldBPget = const_cast<int32*>(&(foundAura->GetEffect(i)->m_amount));
                int32* oldBPSave = const_cast<int32*>(&(foundAura->GetEffect(i)->m_oldbaseAmount));
                Unit** savetarget = const_cast<Unit**>(&(foundAura->GetEffect(i)->saveTarget));
                *savetarget = this;
                *oldBPSave = *oldBPget;
                *oldBP = bp;
            }

            // correct cast item guid if needed
            if (castItemGUID != foundAura->GetCastItemGUID())
            {
                uint64* oldGUID = const_cast<uint64 *>(&foundAura->m_castItemGuid);
                *oldGUID = castItemGUID;
            }

            // try to increase stack amount
            foundAura->ModStackAmount(1);
            return foundAura;
        }
    }

    return NULL;
}

void Unit::_AddAura(UnitAura* aura, Unit* caster)
{
    ASSERT(!m_cleanupDone);
    m_ownedAuras.insert(AuraMap::value_type(aura->GetId(), aura));

    _RemoveNoStackAurasDueToAura(aura);

    if (aura->IsRemoved())
        return;

    aura->SetIsSingleTarget(caster && (aura->GetSpellInfo()->IsSingleTarget(caster) || aura->HasEffectType(SPELL_AURA_CONTROL_VEHICLE)));
    if (aura->IsSingleTarget())
    {
        ASSERT((IsInWorld() && !IsDuringRemoveFromWorld()) || (aura->GetCasterGUID() == GetGUID()) ||
                (isBeingLoaded() && aura->HasEffectType(SPELL_AURA_CONTROL_VEHICLE)));
                /* @HACK: Player is not in world during loading auras.
                 *        Single target auras are not saved or loaded from database
                 *        but may be created as a result of aura links (player mounts with passengers)
                 */

        // register single target aura
        caster->GetSingleCastAuras().push_back(aura);
        // remove other single target auras
        Unit::AuraList& scAuras = caster->GetSingleCastAuras();
        for (Unit::AuraList::iterator itr = scAuras.begin(); itr != scAuras.end();)
        {
            if ((*itr) != aura &&
                (*itr)->IsSingleTargetWith(aura))
            {
                (*itr)->Remove();
                itr = scAuras.begin();
            }
            else
                ++itr;
        }
    }
    //if(caster)
        //caster->GetMyCastAuras().push_back(aura);
}

// creates aura application instance and registers it in lists
// aura application effects are handled separately to prevent aura list corruption
AuraApplication * Unit::_CreateAuraApplication(Aura* aura, uint32 effMask)
{
    // can't apply aura on unit which is going to be deleted - to not create a memory leak
    ASSERT(!m_cleanupDone);
    // aura musn't be removed
    ASSERT(!aura->IsRemoved());

    // aura mustn't be already applied on target
    ASSERT (!aura->IsAppliedOnTarget(GetGUID()) && "Unit::_CreateAuraApplication: aura musn't be applied on target");

    SpellInfo const* aurSpellInfo = aura->GetSpellInfo();
    uint32 aurId = aurSpellInfo->Id;

    // ghost spell check, allow apply any auras at player loading in ghost mode (will be cleanup after load)
    if (!isAlive() && !aurSpellInfo->IsDeathPersistent() &&
        (GetTypeId() != TYPEID_PLAYER || !ToPlayer()->GetSession()->PlayerLoading()))
        return NULL;

    Unit* caster = aura->GetCaster();

    AuraApplication * aurApp = new AuraApplication(this, caster, aura, effMask);
    m_appliedAuras.insert(AuraApplicationMap::value_type(aurId, aurApp));

    if (aurSpellInfo->AuraInterruptFlags)
    {
        m_interruptableAuras.push_back(aurApp);
        AddInterruptMask(aurSpellInfo->AuraInterruptFlags);
    }

    if (AuraStateType aState = aura->GetSpellInfo()->GetAuraState())
        m_auraStateAuras.insert(AuraStateAurasMap::value_type(aState, aurApp));

    aura->_ApplyForTarget(this, caster, aurApp);
    return aurApp;
}

void Unit::_ApplyAuraEffect(Aura* aura, uint32 effIndex)
{
    ASSERT(aura);
    ASSERT(aura->HasEffect(effIndex));
    AuraApplication * aurApp = aura->GetApplicationOfTarget(GetGUID());
    ASSERT(aurApp);
    if (!aurApp->GetEffectMask())
        _ApplyAura(aurApp, 1<<effIndex);
    else
        aurApp->_HandleEffect(effIndex, true);
}

// handles effects of aura application
// should be done after registering aura in lists
void Unit::_ApplyAura(AuraApplication * aurApp, uint32 effMask)
{
    Aura* aura = aurApp->GetBase();
    Unit* caster = aura->GetCaster();
    SpellInfo const* spellInfo = aura->GetSpellInfo();

    _RemoveNoStackAurasDueToAura(aura);

    if (aurApp->GetRemoveMode())
        return;

    // Update target aura state flag
    if (AuraStateType aState = spellInfo->GetAuraState())
        ModifyAuraState(aState, true);

    if (caster)
        caster->ModifyExcludeCasterAuraSpell(spellInfo->Id, true);

    if (aurApp->GetRemoveMode())
        return;

    // Sitdown on apply aura req seated
    if (spellInfo->AuraInterruptFlags & AURA_INTERRUPT_FLAG_NOT_SEATED && !IsSitState())
        SetStandState(UNIT_STAND_STATE_SIT);

    if (aurApp->GetRemoveMode())
        return;

    aura->HandleAuraSpecificMods(aurApp, caster, true, false);

    if (aura->GetCasterGUID() != GetGUID() && caster)
        caster->m_unitsHasCasterAura.insert(GetGUID());

    // Epicurean
    if (GetTypeId() == TYPEID_PLAYER &&
        getRace() == RACE_PANDAREN_ALLI ||
        getRace() == RACE_PANDAREN_HORDE ||
        getRace() == RACE_PANDAREN_NEUTRAL)
    {
        if (spellInfo->AttributesEx2 & SPELL_ATTR2_FOOD_BUFF)
        {
            for (int i = 0; i < MAX_SPELL_EFFECTS; ++i)
                if (aura->GetEffect(i))
                    aura->GetEffect(i)->SetAmount(spellInfo->GetEffect(i, GetSpawnMode())->BasePoints * 2);
        }
    }

    // apply effects of the aura
    for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
    {
        if (effMask & 1<<i && (!aurApp->GetRemoveMode()))
            aurApp->_HandleEffect(i, true);
    }
}

// removes aura application from lists and unapplies effects
void Unit::_UnapplyAura(AuraApplicationMap::iterator &i, AuraRemoveMode removeMode)
{
    AuraApplication * aurApp = i->second;
    ASSERT(aurApp);
    ASSERT(!aurApp->GetRemoveMode());
    ASSERT(aurApp->GetTarget() == this);

    aurApp->SetRemoveMode(removeMode);
    Aura* aura = aurApp->GetBase();
    sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "Aura %u now is remove mode %d", aura->GetId(), removeMode);

    // dead loop is killing the server probably
    ASSERT(m_removedAurasCount < 0xFFFFFFFF);

    ++m_removedAurasCount;

    Unit* caster = aura->GetCaster();

    // Remove all pointers from lists here to prevent possible pointer invalidation on spellcast/auraapply/auraremove
    m_appliedAuras.erase(i);

    if (aura->GetSpellInfo()->AuraInterruptFlags)
    {
        m_interruptableAuras.remove(aurApp);
        UpdateInterruptMask();
    }

    bool auraStateFound = false;
    AuraStateType auraState = aura->GetSpellInfo()->GetAuraState();
    if (auraState)
    {
        bool canBreak = false;
        // Get mask of all aurastates from remaining auras
        for (AuraStateAurasMap::iterator itr = m_auraStateAuras.lower_bound(auraState); itr != m_auraStateAuras.upper_bound(auraState) && !(auraStateFound && canBreak);)
        {
            if (itr->second == aurApp)
            {
                m_auraStateAuras.erase(itr);
                itr = m_auraStateAuras.lower_bound(auraState);
                canBreak = true;
                continue;
            }
            auraStateFound = true;
            ++itr;
        }
    }

    aurApp->_Remove();
    aura->_UnapplyForTarget(this, caster, aurApp);

    // remove effects of the spell - needs to be done after removing aura from lists
    for (uint8 itr = 0; itr < MAX_SPELL_EFFECTS; ++itr)
    {
        if (aurApp->HasEffect(itr))
            aurApp->_HandleEffect(itr, false);
    }

    // all effect mustn't be applied
    ASSERT(!aurApp->GetEffectMask());

    // Remove totem at next update if totem loses its aura
    if ((aurApp->GetRemoveMode() == AURA_REMOVE_BY_EXPIRE || aurApp->GetRemoveMode() == AURA_REMOVE_BY_DROP_CHARGERS) && GetTypeId() == TYPEID_UNIT && ToCreature()->isTotem() && ToTotem()->GetSummonerGUID() == aura->GetCasterGUID())
    {
        if (ToTotem()->GetSpell() == aura->GetId() && ToTotem()->GetTotemType() == TOTEM_PASSIVE)
            ToTotem()->setDeathState(JUST_DIED);
    }

    // Remove aurastates only if were not found
    if (!auraStateFound)
        ModifyAuraState(auraState, false);

    if (caster)
    {
        caster->ModifyExcludeCasterAuraSpell(aura->GetId(), false);

        aura->HandleAuraSpecificMods(aurApp, caster, false, false);

        if (!HasSomeCasterAura(caster->GetGUID()))
            caster->m_unitsHasCasterAura.erase(GetGUID());
    }

    // only way correctly remove all auras from list
    //if (removedAuras != m_removedAurasCount) new aura may be added
        i = m_appliedAuras.begin();
}

void Unit::_UnapplyAura(AuraApplication * aurApp, AuraRemoveMode removeMode)
{
    // aura can be removed from unit only if it's applied on it, shouldn't happen
    ASSERT(aurApp->GetBase()->GetApplicationOfTarget(GetGUID()) == aurApp);
    uint32 spellId = aurApp->GetBase()->GetId();
    for (AuraApplicationMap::iterator iter = m_appliedAuras.lower_bound(spellId); iter != m_appliedAuras.upper_bound(spellId);)
    {
        if (iter->second == aurApp)
        {
            _UnapplyAura(iter, removeMode);
            return;
        }
        else
            ++iter;
    }
    ASSERT(false);
}

void Unit::_RemoveNoStackAurasDueToAura(Aura* aura)
{
    SpellInfo const* spellProto = aura->GetSpellInfo();

    if(!spellProto)
        return;

    // passive spell special case (only non stackable with ranks)
    if (spellProto->IsPassiveStackableWithRanks())
        return;

    bool remove = false;
    for (AuraApplicationMap::iterator i = m_appliedAuras.begin(); i != m_appliedAuras.end(); ++i)
    {
        if (remove)
        {
            remove = false;
            i = m_appliedAuras.begin();
        }

        if (Aura* auraBase = i->second->GetBase())
        {
            if (aura->CanStackWith(auraBase))
                continue;

            // Hack fix remove seal by consecration
            if ((auraBase->GetId() == 105361 ||
                auraBase->GetId() == 101423 ||
                auraBase->GetId() == 31801 ||
                auraBase->GetId() == 20165 ||
                auraBase->GetId() == 20164)
                && spellProto->Id == 26573)
                continue;

            RemoveAura(i, AURA_REMOVE_BY_DEFAULT);
            if (i == m_appliedAuras.end())
                break;
            remove = true;
        }
    }
}

void Unit::_RegisterAuraEffect(AuraEffect* aurEff, bool apply)
{
    if (apply)
        m_modAuras[aurEff->GetAuraType()].emplace_back(aurEff);
    else if(!m_modAuras[aurEff->GetAuraType()].empty())
        m_modAuras[aurEff->GetAuraType()].remove(aurEff);
}

// All aura base removes should go threw this function!
void Unit::RemoveOwnedAura(AuraMap::iterator &i, AuraRemoveMode removeMode)
{
    Aura* aura = i->second;
    ASSERT(!aura->IsRemoved());

    // if unit currently update aura list then make safe update iterator shift to next
    if (m_auraUpdateIterator == i)
        ++m_auraUpdateIterator;

    m_ownedAuras.erase(i);
    m_removedAuras.push_back(aura);

    // Unregister single target aura
    if (aura->IsSingleTarget())
        aura->UnregisterSingleTarget();

    aura->_Remove(removeMode);

    aura->UnregisterCasterAuras();

    i = m_ownedAuras.begin();
}

void Unit::RemoveOwnedAura(uint32 spellId, uint64 casterGUID, uint32 reqEffMask, AuraRemoveMode removeMode)
{
    for (AuraMap::iterator itr = m_ownedAuras.lower_bound(spellId); itr != m_ownedAuras.upper_bound(spellId);)
        if (((itr->second->GetEffectMask() & reqEffMask) == reqEffMask) && (!casterGUID || itr->second->GetCasterGUID() == casterGUID))
        {
            RemoveOwnedAura(itr, removeMode);
            itr = m_ownedAuras.lower_bound(spellId);
        }
        else
            ++itr;
}

void Unit::RemoveOwnedAura(Aura* aura, AuraRemoveMode removeMode)
{
    if (aura->IsRemoved())
        return;

    ASSERT(aura->GetOwner() == this);

    uint32 spellId = aura->GetId();
    for (AuraMap::iterator itr = m_ownedAuras.lower_bound(spellId); itr != m_ownedAuras.upper_bound(spellId); ++itr)
        if (itr->second == aura)
        {
            RemoveOwnedAura(itr, removeMode);
            return;
        }
    ASSERT(false);
}

Aura* Unit::GetOwnedAura(uint32 spellId, uint64 casterGUID, uint64 itemCasterGUID, uint32 reqEffMask, Aura* except) const
{
    for (AuraMap::const_iterator itr = m_ownedAuras.lower_bound(spellId); itr != m_ownedAuras.upper_bound(spellId); ++itr)
        if (((itr->second->GetEffectMask() & reqEffMask) == reqEffMask) && (!casterGUID || itr->second->GetCasterGUID() == casterGUID) && (!itemCasterGUID || itr->second->GetCastItemGUID() == itemCasterGUID) && (!except || except != itr->second))
            return itr->second;
    return NULL;
}

void Unit::RemoveAura(AuraApplicationMap::iterator &i, AuraRemoveMode mode)
{
    AuraApplication * aurApp = i->second;
    // Do not remove aura which is already being removed
    if (aurApp->GetRemoveMode())
        return;
    Aura* aura = aurApp->GetBase();
    _UnapplyAura(i, mode);
    // Remove aura - for Area and Target auras
    if (aura->GetOwner() == this)
        aura->Remove(mode);
}

void Unit::RemoveAura(uint32 spellId, uint64 caster, uint32 reqEffMask, AuraRemoveMode removeMode)
{
    for (AuraApplicationMap::iterator iter = m_appliedAuras.lower_bound(spellId); iter != m_appliedAuras.upper_bound(spellId);)
    {
        Aura const* aura = iter->second->GetBase();
        if (((aura->GetEffectMask() & reqEffMask) == reqEffMask)
            && (!caster || aura->GetCasterGUID() == caster))
        {
            RemoveAura(iter, removeMode);
            return;
        }
        else
            ++iter;
    }
}

void Unit::RemoveSomeAuras()
{
    // SymbiosisAuras
    RemoveAura(110309);// Caster
    RemoveAura(110478);// Death Knight
    RemoveAura(110479);// Hunter
    RemoveAura(110482);// Mage
    RemoveAura(110483);// Monk
    RemoveAura(110484);// Paladin
    RemoveAura(110485);// Priest
    RemoveAura(110486);// Rogue
    RemoveAura(110488);// Shaman
    RemoveAura(110490);// Warlock
    RemoveAura(110491);// Warrior

    RemoveAura(104756);
    RemoveAura(104759);
    RemoveAura(123171);
}

void Unit::RemoveAura(AuraApplication * aurApp, AuraRemoveMode mode)
{
    // we've special situation here, RemoveAura called while during aura removal
    // this kind of call is needed only when aura effect removal handler
    // or event triggered by it expects to remove
    // not yet removed effects of an aura
    if (aurApp->GetRemoveMode())
    {
        // remove remaining effects of an aura
        for (uint8 itr = 0; itr < MAX_SPELL_EFFECTS; ++itr)
        {
            if (aurApp->HasEffect(itr))
                aurApp->_HandleEffect(itr, false);
        }
        return;
    }
    // no need to remove
    if (aurApp->GetBase()->GetApplicationOfTarget(GetGUID()) != aurApp || aurApp->GetBase()->IsRemoved())
        return;

    uint32 spellId = aurApp->GetBase()->GetId();

    if (spellId == 51713 && mode != AURA_REMOVE_BY_EXPIRE)
        return;

    for (AuraApplicationMap::iterator iter = m_appliedAuras.lower_bound(spellId); iter != m_appliedAuras.upper_bound(spellId);)
    {
        if (aurApp == iter->second)
        {
            RemoveAura(iter, mode);
            return;
        }
        else
            ++iter;
    }
}

void Unit::RemoveAura(Aura* aura, AuraRemoveMode mode)
{
    if (aura->IsRemoved())
        return;
    if (AuraApplication * aurApp = aura->GetApplicationOfTarget(GetGUID()))
        RemoveAura(aurApp, mode);
}

void Unit::RemoveAurasDueToSpell(uint32 spellId, uint64 casterGUID, uint32 reqEffMask, AuraRemoveMode removeMode)
{
    for (AuraApplicationMap::iterator iter = m_appliedAuras.lower_bound(spellId); iter != m_appliedAuras.upper_bound(spellId);)
    {
        Aura const* aura = iter->second->GetBase();
        if (((aura->GetEffectMask() & reqEffMask) == reqEffMask)
            && (!casterGUID || aura->GetCasterGUID() == casterGUID))
        {
            RemoveAura(iter, removeMode);
            iter = m_appliedAuras.lower_bound(spellId);
        }
        else
            ++iter;
    }
}

void Unit::RemoveAuraFromStack(uint32 spellId, uint64 casterGUID, AuraRemoveMode removeMode, int32 num)
{
    for (AuraMap::iterator iter = m_ownedAuras.lower_bound(spellId); iter != m_ownedAuras.upper_bound(spellId);)
    {
        Aura* aura = iter->second;
        if ((aura->GetType() == UNIT_AURA_TYPE)
            && (!casterGUID || aura->GetCasterGUID() == casterGUID))
        {
            aura->ModStackAmount(-num, removeMode);
            return;
        }
        else
            ++iter;
    }
}

void Unit::RemoveAurasDueToSpellByDispel(uint32 spellId, uint32 dispellerSpellId, uint64 casterGUID, Unit* dispeller, uint8 chargesRemoved/*= 1*/)
{
    for (AuraMap::iterator iter = m_ownedAuras.lower_bound(spellId); iter != m_ownedAuras.upper_bound(spellId);)
    {
        Aura* aura = iter->second;
        if (aura->GetCasterGUID() == casterGUID)
        {
            DispelInfo dispelInfo(dispeller, dispellerSpellId, chargesRemoved);

            // Call OnDispel hook on AuraScript
            aura->CallScriptDispel(&dispelInfo);

            if (aura->GetSpellInfo()->AttributesEx7 & SPELL_ATTR7_DISPEL_CHARGES)
                aura->ModCharges(-dispelInfo.GetRemovedCharges(), AURA_REMOVE_BY_ENEMY_SPELL);
            else
                aura->ModStackAmount(-dispelInfo.GetRemovedCharges(), AURA_REMOVE_BY_ENEMY_SPELL);

            // Call AfterDispel hook on AuraScript
            aura->CallScriptAfterDispel(&dispelInfo);

            return;
        }
        else
            ++iter;
    }
}

void Unit::RemoveAurasDueToSpellBySteal(uint32 spellId, uint64 casterGUID, Unit* stealer)
{
    for (AuraMap::iterator iter = m_ownedAuras.lower_bound(spellId); iter != m_ownedAuras.upper_bound(spellId);)
    {
        Aura* aura = iter->second;
        if (aura->GetCasterGUID() == casterGUID)
        {
            int32 damage[MAX_SPELL_EFFECTS];
            int32 baseDamage[MAX_SPELL_EFFECTS];
            uint32 effMask = 0;
            uint32 recalculateMask = 0;
            Unit* caster = aura->GetCaster();
            for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
            {
                if (aura->GetEffect(i))
                {
                    baseDamage[i] = aura->GetEffect(i)->GetBaseAmount();
                    damage[i] = aura->GetEffect(i)->GetAmount();
                    effMask |= (1<<i);
                    if (aura->GetEffect(i)->CanBeRecalculated())
                        recalculateMask |= (1<<i);
                }
                else
                {
                    baseDamage[i] = 0;
                    damage[i] = 0;
                }
            }

            bool stealCharge = aura->GetSpellInfo()->AttributesEx7 & SPELL_ATTR7_DISPEL_CHARGES;
            // Cast duration to unsigned to prevent permanent aura's such as Righteous Fury being permanently added to caster
            uint32 dur = std::min(2u * MINUTE * IN_MILLISECONDS, uint32(aura->GetDuration()));

            Aura* oldAura = stealer->GetAura(aura->GetId(), aura->GetCasterGUID());
            if (oldAura != NULL)
            {
                if (stealCharge)
                    oldAura->ModCharges(1);
                else
                    oldAura->ModStackAmount(1);
                oldAura->SetDuration(int32(dur));
            }
            else
            {
                // single target state must be removed before aura creation to preserve existing single target aura
                if (aura->IsSingleTarget())
                    aura->UnregisterSingleTarget();

                Aura* newAura = Aura::TryRefreshStackOrCreate(aura->GetSpellInfo(), effMask, stealer, NULL, &baseDamage[0], NULL, aura->GetCasterGUID());
                if (newAura != NULL)
                {
                    // created aura must not be single target aura,, so stealer won't loose it on recast
                    if (newAura->IsSingleTarget())
                    {
                        newAura->UnregisterSingleTarget();
                        // bring back single target aura status to the old aura
                        aura->SetIsSingleTarget(true);
                        caster->GetSingleCastAuras().push_back(aura);
                    }
                    // FIXME: using aura->GetMaxDuration() maybe not blizzlike but it fixes stealing of spells like Innervate
                    newAura->SetLoadedState(aura->GetMaxDuration(), int32(dur), stealCharge ? 1 : aura->GetCharges(), 1, recalculateMask, &damage[0]);
                    newAura->ApplyForTargets();

                    if (newAura->GetId() == 1022)
                    {
                        stealer->AddAura(25771, stealer);
                    }
                }
            }

            if (stealCharge)
                aura->ModCharges(-1, AURA_REMOVE_BY_ENEMY_SPELL);
            else
                aura->ModStackAmount(-1, AURA_REMOVE_BY_ENEMY_SPELL);

            return;
        }
        else
            ++iter;
    }
}

void Unit::RemoveAurasDueToItemSpell(Item* castItem, uint32 spellId)
{
    for (AuraApplicationMap::iterator iter = m_appliedAuras.lower_bound(spellId); iter != m_appliedAuras.upper_bound(spellId);)
    {
        if (!castItem || iter->second->GetBase()->GetCastItemGUID() == castItem->GetGUID())
        {
            RemoveAura(iter);
            iter = m_appliedAuras.upper_bound(spellId);          // overwrite by more appropriate
        }
        else
            ++iter;
    }
}

void Unit::RemoveAurasByType(AuraType auraType, uint64 casterGUID, Aura* except, bool negative, bool positive)
{
    for (AuraEffectList::iterator iter = m_modAuras[auraType].begin(), next; iter != m_modAuras[auraType].end();iter = next)
    {
        next = iter;
        Aura* aura = (*iter)->GetBase();

        if (!aura)
        {
            ++next;
            continue;
        }

        AuraApplication * aurApp = aura->GetApplicationOfTarget(GetGUID());

        if (!aurApp)
        {
            printf("CRASH ALERT : Unit::RemoveAurasByType no AurApp pointer for Aura Id %u\n", aura->GetId());
            ++next;
            continue;
        }

        ++next;
        if (aura != except && (!casterGUID || aura->GetCasterGUID() == casterGUID)
            && ((negative && !aurApp->IsPositive()) || (positive && aurApp->IsPositive())))
        {
            uint32 removedAuras = m_removedAurasCount;
            RemoveAura(aurApp);
            if (m_removedAurasCount > removedAuras + 1)
                next = m_modAuras[auraType].begin();
        }
    }
}

void Unit::RemoveAurasWithAttribute(uint32 flags)
{
    for (AuraApplicationMap::iterator iter = m_appliedAuras.begin(); iter != m_appliedAuras.end();)
    {
        SpellInfo const* spell = iter->second->GetBase()->GetSpellInfo();
        if (spell && spell->Attributes & flags)
            RemoveAura(iter);
        else
            ++iter;
    }
}

void Unit::RemoveNotOwnSingleTargetAuras(uint32 newPhase)
{
    // single target auras from other casters
    for (AuraApplicationMap::iterator iter = m_appliedAuras.begin(); iter != m_appliedAuras.end();)
    {
        AuraApplication const* aurApp = iter->second;
        Aura const* aura = aurApp->GetBase();
        Unit* caster = aura->GetCaster();

        if (aura->GetCasterGUID() != GetGUID() && aura->GetSpellInfo()->IsSingleTarget(caster))
        {
            if (!newPhase)
                RemoveAura(iter);
            else
            {
                if (!caster || !caster->InSamePhase(newPhase))
                    RemoveAura(iter);
                else
                    ++iter;
            }
        }
        else
            ++iter;
    }

    // single target auras at other targets
    AuraList& scAuras = GetSingleCastAuras();
    for (AuraList::iterator iter = scAuras.begin(); iter != scAuras.end();)
    {
        Aura* aura = *iter;
        if (aura && aura->GetOwner() && aura->GetUnitOwner() != this && !aura->GetUnitOwner()->InSamePhase(newPhase))
        {
            aura->Remove();
            iter = scAuras.begin();
        }
        else
            ++iter;
    }
}

uint32 Unit::RemoveAurasWithInterruptFlags(uint32 flag, uint32 except)
{
    if (!(m_interruptMask & flag))
        return 0;

    //sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "RemoveAurasWithInterruptFlags flag %u except %u, m_interruptMask %u", flag, except, m_interruptMask);

    uint32 count = 0;
    // interrupt auras
    for (AuraApplicationList::iterator iter = m_interruptableAuras.begin(); iter != m_interruptableAuras.end();)
    {
        Aura* aura = (*iter)->GetBase();
        ++iter;
        if ((aura->GetSpellInfo()->AuraInterruptFlags & flag) && (!except || aura->GetId() != except) &&
            ((flag & AURA_INTERRUPT_FLAG_MOVING) == 0 || !HasAuraCastWhileWalking(aura->GetSpellInfo())))
        {
            uint32 removedAuras = m_removedAurasCount;
            RemoveAura(aura);
            count++;
            if (m_removedAurasCount > removedAuras + 1)
                iter = m_interruptableAuras.begin();
        }
    }

    // interrupt channeled spell
    if (Spell* spell = m_currentSpells[CURRENT_CHANNELED_SPELL])
        if (spell->getState() == SPELL_STATE_CASTING
            && (spell->m_spellInfo->ChannelInterruptFlags & flag)
            && spell->m_spellInfo->Id != except
            && !(spell->m_spellInfo->Id == 120360 && flag == AURA_INTERRUPT_FLAG_MOVE)
            && ((flag & AURA_INTERRUPT_FLAG_MOVING) == 0 || !HasAuraCastWhileWalking(spell->m_spellInfo)))
            InterruptNonMeleeSpells(false);

    UpdateInterruptMask();
    return count;
}

void Unit::RemoveAurasWithFamily(SpellFamilyNames family, uint32 familyFlag1, uint32 familyFlag2, uint32 familyFlag3, uint64 casterGUID)
{
    for (AuraApplicationMap::iterator iter = m_appliedAuras.begin(); iter != m_appliedAuras.end();)
    {
        Aura const* aura = iter->second->GetBase();
        if (!casterGUID || aura->GetCasterGUID() == casterGUID)
        {
            SpellInfo const* spell = aura->GetSpellInfo();
            if (spell->SpellFamilyName == uint32(family) && spell->SpellFamilyFlags.HasFlag(familyFlag1, familyFlag2, familyFlag3))
            {
                RemoveAura(iter);
                continue;
            }
        }
        ++iter;
    }
}

void Unit::RemoveMovementImpairingAuras()
{
    RemoveAurasWithMechanic((1<<MECHANIC_SNARE)|(1<<MECHANIC_ROOT));
}

void Unit::RemoveAurasWithMechanic(uint32 mechanic_mask, AuraRemoveMode removemode, uint32 except)
{
    for (AuraApplicationMap::iterator iter = m_appliedAuras.begin(); iter != m_appliedAuras.end();)
    {
        Aura const* aura = iter->second->GetBase();
        if (!except || aura->GetId() != except)
        {
            if (aura->GetSpellInfo()->GetAllEffectsMechanicMask() & mechanic_mask)
            {
                RemoveAura(iter, removemode);
                continue;
            }
        }
        ++iter;
    }
}

void Unit::RemoveAreaAurasDueToLeaveWorld()
{
    // make sure that all area auras not applied on self are removed - prevent access to deleted pointer later
    for (AuraMap::iterator iter = m_ownedAuras.begin(); iter != m_ownedAuras.end();)
    {
        Aura* aura = iter->second;
        ++iter;
        Aura::ApplicationMap const& appMap = aura->GetApplicationMap();
        for (Aura::ApplicationMap::const_iterator itr = appMap.begin(); itr!= appMap.end();)
        {
            AuraApplication * aurApp = itr->second;
            ++itr;
            Unit* target = aurApp->GetTarget();
            if (!target || target == this)
                continue;

            target->RemoveAura(aurApp);
            // things linked on aura remove may apply new area aura - so start from the beginning
            iter = m_ownedAuras.begin();
        }
    }

    // remove area auras owned by others
    for (AuraApplicationMap::iterator iter = m_appliedAuras.begin(); iter != m_appliedAuras.end();)
    {
        if (iter->second->GetBase()->GetOwner() != this)
        {
            RemoveAura(iter);
        }
        else
            ++iter;
    }
}

void Unit::RemoveAllAuras()
{
    // this may be a dead loop if some events on aura remove will continiously apply aura on remove
    // we want to have all auras removed, so use your brain when linking events
    /*while (!m_appliedAuras.empty() || !m_ownedAuras.empty())
    {
        AuraApplicationMap::iterator aurAppIter;
        for (aurAppIter = m_appliedAuras.begin(); aurAppIter != m_appliedAuras.end();)
            _UnapplyAura(aurAppIter, AURA_REMOVE_BY_DEFAULT);

        AuraMap::iterator aurIter;
        for (aurIter = m_ownedAuras.begin(); aurIter != m_ownedAuras.end();)
            RemoveOwnedAura(aurIter);
    }*/
    if (!m_appliedAuras.empty())
    {
        AuraApplicationMap::iterator aurAppIter;
        for (aurAppIter = m_appliedAuras.begin(); aurAppIter != m_appliedAuras.end();)
            _UnapplyAura(aurAppIter, AURA_REMOVE_BY_DEFAULT);
    }

    if (!m_ownedAuras.empty())
    {
        AuraMap::iterator aurIter;
        for (aurIter = m_ownedAuras.begin(); aurIter != m_ownedAuras.end();)
            RemoveOwnedAura(aurIter);
    }
}

void Unit::RemoveNonPassivesAuras()
{
    // this may be a dead loop if some events on aura remove will continiously apply aura on remove
    // we want to have all auras removed, so use your brain when linking events
    for (AuraApplicationMap::iterator aurAppIter = m_appliedAuras.begin(); aurAppIter != m_appliedAuras.end();)
    {
        if (!aurAppIter->second->GetBase()->IsPassive())
            _UnapplyAura(aurAppIter, AURA_REMOVE_BY_DEFAULT);
        else
            ++aurAppIter;
    }

    for (AuraMap::iterator aurIter = m_ownedAuras.begin(); aurIter != m_ownedAuras.end();)
    {
        if (!aurIter->second->IsPassive())
            RemoveOwnedAura(aurIter);
        else
            ++aurIter;
    }
}

void Unit::RemoveArenaAuras()
{
    // in join, remove positive buffs, on end, remove negative
    // used to remove positive visible auras in arenas
    for (AuraApplicationMap::iterator iter = m_appliedAuras.begin(); iter != m_appliedAuras.end();)
    {
        AuraApplication const* aurApp = iter->second;
        Aura const* aura = aurApp->GetBase();
        if (!(aura->GetSpellInfo()->AttributesEx4 & SPELL_ATTR4_UNK21)  // don't remove stances, shadowform, pally/hunter auras
            && !aura->IsPassive()                                       // don't remove passive auras
            && (aurApp->IsPositive() || !(aura->GetSpellInfo()->AttributesEx3 & SPELL_ATTR3_DEATH_PERSISTENT)) // not negative death persistent auras
            || aura->GetSpellInfo()->AttributesEx2 & SPELL_ATTR2_UNK3)
            RemoveAura(iter);
        else
            ++iter;
    }
}

void Unit::RecalcArenaAuras()
{
    for (AuraApplicationMap::iterator iter = m_appliedAuras.begin(); iter != m_appliedAuras.end();)
    {
        AuraApplication const* aurApp = iter->second;
        if(Aura* aura = aurApp->GetBase())
        {
            //recalc only if need
            if ((aura->GetSpellInfo()->AttributesEx8 & SPELL_ATTR8_NOT_IN_BG_OR_ARENA) || (aura->GetSpellInfo()->AttributesEx4 & SPELL_ATTR4_NOT_USABLE_IN_ARENA_OR_RATED_BG))
                aura->RecalculateAmountOfEffects(true);
        }
        ++iter;
    }
}

void Unit::RemoveAllAurasOnDeath()
{
    // used just after dieing to remove all visible auras
    // and disable the mods for the passive ones
    for (AuraApplicationMap::iterator iter = m_appliedAuras.begin(); iter != m_appliedAuras.end();)
    {
        Aura const* aura = iter->second->GetBase();
        if (!aura->IsPassive() && !aura->IsDeathPersistent())
            _UnapplyAura(iter, AURA_REMOVE_BY_DEATH);
        else
            ++iter;
    }

    for (AuraMap::iterator iter = m_ownedAuras.begin(); iter != m_ownedAuras.end();)
    {
        Aura* aura = iter->second;
        if (!aura->IsPassive() && !aura->IsDeathPersistent())
            RemoveOwnedAura(iter, AURA_REMOVE_BY_DEATH);
        else
            ++iter;
    }
}

void Unit::RemoveAllAurasRequiringDeadTarget()
{
    for (AuraApplicationMap::iterator iter = m_appliedAuras.begin(); iter != m_appliedAuras.end();)
    {
        Aura const* aura = iter->second->GetBase();
        if (!aura->IsPassive() && aura->GetSpellInfo()->IsRequiringDeadTarget())
            _UnapplyAura(iter, AURA_REMOVE_BY_DEFAULT);
        else
            ++iter;
    }

    for (AuraMap::iterator iter = m_ownedAuras.begin(); iter != m_ownedAuras.end();)
    {
        Aura* aura = iter->second;
        if (!aura->IsPassive() && aura->GetSpellInfo()->IsRequiringDeadTarget())
            RemoveOwnedAura(iter, AURA_REMOVE_BY_DEFAULT);
        else
            ++iter;
    }
}

void Unit::RemoveAllAurasExceptType(AuraType type)
{
    for (AuraApplicationMap::iterator iter = m_appliedAuras.begin(); iter != m_appliedAuras.end();)
    {
        Aura const* aura = iter->second->GetBase();
        if (!aura->GetSpellInfo()->HasAura(type))
            _UnapplyAura(iter, AURA_REMOVE_BY_DEFAULT);
        else
            ++iter;
    }

    for (AuraMap::iterator iter = m_ownedAuras.begin(); iter != m_ownedAuras.end();)
    {
        Aura* aura = iter->second;
        if (!aura->GetSpellInfo()->HasAura(type))
            RemoveOwnedAura(iter, AURA_REMOVE_BY_DEFAULT);
        else
            ++iter;
    }
}

void Unit::RemoveAllAurasByType(AuraType type)
{
    for (AuraApplicationMap::iterator iter = m_appliedAuras.begin(); iter != m_appliedAuras.end();)
    {
        Aura const* aura = iter->second->GetBase();
        if (aura->GetSpellInfo()->HasAura(type))
            _UnapplyAura(iter, AURA_REMOVE_BY_DEFAULT);
        else
            ++iter;
    }

    for (AuraMap::iterator iter = m_ownedAuras.begin(); iter != m_ownedAuras.end();)
    {
        Aura* aura = iter->second;
        if (aura->GetSpellInfo()->HasAura(type))
            RemoveOwnedAura(iter, AURA_REMOVE_BY_DEFAULT);
        else
            ++iter;
    }
}

void Unit::DelayOwnedAuras(uint32 spellId, uint64 caster, int32 delaytime)
{
    for (AuraMap::iterator iter = m_ownedAuras.lower_bound(spellId); iter != m_ownedAuras.upper_bound(spellId);++iter)
    {
        Aura* aura = iter->second;
        if (!caster || aura->GetCasterGUID() == caster)
        {
            if (aura->GetDuration() < delaytime)
                aura->SetDuration(0);
            else
                aura->SetDuration(aura->GetDuration() - delaytime);

            // update for out of range group members (on 1 slot use)
            aura->SetNeedClientUpdateForTargets();
            sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "Aura %u partially interrupted on unit %u, new duration: %u ms", aura->GetId(), GetGUIDLow(), aura->GetDuration());
        }
    }
}

void Unit::_RemoveAllAuraStatMods()
{
    for (AuraApplicationMap::iterator i = m_appliedAuras.begin(); i != m_appliedAuras.end(); ++i)
        (*i).second->GetBase()->HandleAllEffects(i->second, AURA_EFFECT_HANDLE_STAT, false);
}

void Unit::_ApplyAllAuraStatMods()
{
    for (AuraApplicationMap::iterator i = m_appliedAuras.begin(); i != m_appliedAuras.end(); ++i)
        (*i).second->GetBase()->HandleAllEffects(i->second, AURA_EFFECT_HANDLE_STAT, true);
}

std::list<AuraEffect*> Unit::GetAuraEffectsByMechanic(uint32 mechanic_mask) const
{
    AuraEffectList list;
    for (AuraApplicationMap::const_iterator iter = m_appliedAuras.begin(); iter != m_appliedAuras.end(); ++iter)
    {
        Aura const* aura = iter->second->GetBase();
        for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
        {
            if (aura->GetSpellInfo()->GetEffectMechanicMask(i) & mechanic_mask)
            {
                if (iter->second)
                    if (iter->second->GetBase())
                        if (iter->second->GetBase()->GetEffect(i))
                            list.push_back(iter->second->GetBase()->GetEffect(i));
            }
        }
    }

    return list;
}

AuraEffect* Unit::GetAuraEffect(uint32 spellId, uint8 effIndex, uint64 caster) const
{
    for (AuraApplicationMap::const_iterator itr = m_appliedAuras.lower_bound(spellId); itr != m_appliedAuras.upper_bound(spellId); ++itr)
        if (itr->second->HasEffect(effIndex) && (!caster || itr->second->GetBase()->GetCasterGUID() == caster))
            return itr->second->GetBase()->GetEffect(effIndex);
    return NULL;
}

AuraEffect* Unit::GetAuraEffectOfRankedSpell(uint32 spellId, uint8 effIndex, uint64 caster) const
{
    uint32 rankSpell = sSpellMgr->GetFirstSpellInChain(spellId);
    while (rankSpell)
    {
        if (AuraEffect* aurEff = GetAuraEffect(rankSpell, effIndex, caster))
            return aurEff;
        rankSpell = sSpellMgr->GetNextSpellInChain(rankSpell);
    }
    return NULL;
}

AuraEffect* Unit::GetAuraEffect(AuraType type, SpellFamilyNames name, uint32 iconId, uint8 effIndex) const
{
    AuraEffectList auras = GetAuraEffectsByType(type);
    for (Unit::AuraEffectList::const_iterator itr = auras.begin(); itr != auras.end(); ++itr)
    {
        if (effIndex != (*itr)->GetEffIndex())
            continue;
        SpellInfo const* spell = (*itr)->GetSpellInfo();
        if (spell->SpellIconID == iconId && spell->SpellFamilyName == uint32(name) && !spell->SpellFamilyFlags)
            return *itr;
    }
    return NULL;
}

AuraEffect* Unit::GetAuraEffect(AuraType type, SpellFamilyNames family, uint32 familyFlag1, uint32 familyFlag2, uint32 familyFlag3, uint64 casterGUID)
{
    AuraEffectList auras = GetAuraEffectsByType(type);
    for (AuraEffectList::const_iterator i = auras.begin(); i != auras.end(); ++i)
    {
        SpellInfo const* spell = (*i)->GetSpellInfo();
        if (spell->SpellFamilyName == uint32(family) && spell->SpellFamilyFlags.HasFlag(familyFlag1, familyFlag2, familyFlag3))
        {
            if (casterGUID && (*i)->GetCasterGUID() != casterGUID)
                continue;
            return (*i);
        }
    }
    return NULL;
}

AuraApplication * Unit::GetAuraApplication(uint32 spellId, uint64 casterGUID, uint64 itemCasterGUID, uint32 reqEffMask, AuraApplication * except) const
{
    for (AuraApplicationMap::const_iterator itr = m_appliedAuras.lower_bound(spellId); itr != m_appliedAuras.upper_bound(spellId); ++itr)
    {
        Aura const* aura = itr->second->GetBase();
        if (((aura->GetEffectMask() & reqEffMask) == reqEffMask) && (!casterGUID || aura->GetCasterGUID() == casterGUID) && (!itemCasterGUID || aura->GetCastItemGUID() == itemCasterGUID) && (!except || except != itr->second))
            return itr->second;
    }
    return NULL;
}

Aura* Unit::GetAura(uint32 spellId, uint64 casterGUID, uint64 itemCasterGUID, uint32 reqEffMask) const
{
    AuraApplication * aurApp = GetAuraApplication(spellId, casterGUID, itemCasterGUID, reqEffMask);
    return aurApp ? aurApp->GetBase() : NULL;
}

AuraApplication * Unit::GetAuraApplicationOfRankedSpell(uint32 spellId, uint64 casterGUID, uint64 itemCasterGUID, uint32 reqEffMask, AuraApplication* except) const
{
    uint32 rankSpell = sSpellMgr->GetFirstSpellInChain(spellId);
    while (rankSpell)
    {
        if (AuraApplication * aurApp = GetAuraApplication(rankSpell, casterGUID, itemCasterGUID, reqEffMask, except))
            return aurApp;
        rankSpell = sSpellMgr->GetNextSpellInChain(rankSpell);
    }
    return NULL;
}

Aura* Unit::GetAuraOfRankedSpell(uint32 spellId, uint64 casterGUID, uint64 itemCasterGUID, uint32 reqEffMask) const
{
    AuraApplication * aurApp = GetAuraApplicationOfRankedSpell(spellId, casterGUID, itemCasterGUID, reqEffMask);
    return aurApp ? aurApp->GetBase() : NULL;
}

void Unit::GetDispellableAuraList(Unit* caster, uint32 dispelMask, DispelChargesList& dispelList)
{
    // we should not be able to dispel diseases if the target is affected by unholy blight
    if (dispelMask & (1 << DISPEL_DISEASE) && HasAura(50536))
        dispelMask &= ~(1 << DISPEL_DISEASE);

    AuraMap const& auras = GetOwnedAuras();
    for (AuraMap::const_iterator itr = auras.begin(); itr != auras.end(); ++itr)
    {
        Aura* aura = itr->second;
        AuraApplication * aurApp = aura->GetApplicationOfTarget(GetGUID());
        if (!aurApp)
            continue;

        // don't try to remove passive auras
        if (aura->IsPassive())
            continue;

        if (aura->GetSpellInfo()->GetDispelMask() & dispelMask)
        {
            if (aura->GetSpellInfo()->Dispel == DISPEL_MAGIC)
            {
                // do not remove positive auras if friendly target
                //               negative auras if non-friendly target
                if (aurApp->IsPositive() == IsFriendlyTo(caster))
                    continue;
            }

            // The charges / stack amounts don't count towards the total number of auras that can be dispelled.
            // Ie: A dispel on a target with 5 stacks of Winters Chill and a Polymorph has 1 / (1 + 1) -> 50% chance to dispell
            // Polymorph instead of 1 / (5 + 1) -> 16%.
            bool dispel_charges = aura->GetSpellInfo()->AttributesEx7 & SPELL_ATTR7_DISPEL_CHARGES;
            uint8 charges = dispel_charges ? aura->GetCharges() : aura->GetStackAmount();
            if (charges > 0)
                dispelList.push_back(std::make_pair(aura, charges));
        }
    }
}

bool Unit::HasAuraEffect(uint32 spellId, uint8 effIndex, uint64 caster) const
{
    for (AuraApplicationMap::const_iterator itr = m_appliedAuras.lower_bound(spellId); itr != m_appliedAuras.upper_bound(spellId); ++itr)
        if (itr->second->HasEffect(effIndex) && (!caster || itr->second->GetBase()->GetCasterGUID() == caster))
            return true;
    return false;
}

uint32 Unit::GetAuraCount(uint32 spellId) const
{
    uint32 count = 0;
    for (AuraApplicationMap::const_iterator itr = m_appliedAuras.lower_bound(spellId); itr != m_appliedAuras.upper_bound(spellId); ++itr)
    {
        if (!itr->second->GetBase()->GetStackAmount())
            count++;
        else
            count += (uint32)itr->second->GetBase()->GetStackAmount();
    }
    return count;
}

bool Unit::HasAura(uint32 spellId, uint64 casterGUID, uint64 itemCasterGUID, uint32 reqEffMask) const
{
    if (GetAuraApplication(spellId, casterGUID, itemCasterGUID, reqEffMask))
        return true;
    return false;
}

bool Unit::HasAuraType(AuraType auraType) const
{
    return (!m_modAuras[auraType].empty());
}

bool Unit::HasAuraTypeWithCaster(AuraType auratype, uint64 caster) const
{
    AuraEffectList mTotalAuraList = GetAuraEffectsByType(auratype);
    for (AuraEffectList::const_iterator i = mTotalAuraList.begin(), next; i != mTotalAuraList.end(); i = next)
    {
        next = i;
        ++next;
        if (caster == (*i)->GetCasterGUID())
            return true;
    }
    return false;
}

bool Unit::HasAuraTypeWithMiscvalue(AuraType auratype, int32 miscvalue) const
{
    AuraEffectList mTotalAuraList = GetAuraEffectsByType(auratype);
    for (AuraEffectList::const_iterator i = mTotalAuraList.begin(), next; i != mTotalAuraList.end(); i = next)
    {
        next = i;
        ++next;
        if (miscvalue == (*i)->GetMiscValue())
            return true;
    }
    return false;
}

bool Unit::HasAuraTypeWithAffectMask(AuraType auratype, SpellInfo const* affectedSpell) const
{
    AuraEffectList mTotalAuraList = GetAuraEffectsByType(auratype);
    for (AuraEffectList::const_iterator i = mTotalAuraList.begin(), next; i != mTotalAuraList.end(); i = next)
    {
        next = i;
        ++next;
        if ((*i)->IsAffectingSpell(affectedSpell))
            return true;
    }
    return false;
}

bool Unit::HasAuraTypeWithValue(AuraType auratype, int32 value) const
{
    AuraEffectList mTotalAuraList = GetAuraEffectsByType(auratype);
    for (AuraEffectList::const_iterator i = mTotalAuraList.begin(), next; i != mTotalAuraList.end(); i = next)
    {
        next = i;
        ++next;
        if (value == (*i)->GetAmount())
            return true;
    }
    return false;
}

bool Unit::HasNegativeAuraWithInterruptFlag(uint32 flag, uint64 guid)
{
    if (!(m_interruptMask & flag))
        return false;
    for (AuraApplicationList::iterator iter = m_interruptableAuras.begin(); iter != m_interruptableAuras.end(); ++iter)
    {
        if (!(*iter)->IsPositive() && (*iter)->GetBase()->GetSpellInfo()->AuraInterruptFlags & flag && (!guid || (*iter)->GetBase()->GetCasterGUID() == guid))
            return true;
    }
    return false;
}

bool Unit::HasNegativeAuraWithAttribute(uint32 flag, uint64 guid)
{
    for (AuraApplicationMap::iterator iter = m_appliedAuras.begin(); iter != m_appliedAuras.end(); ++iter)
    {
        Aura const* aura = iter->second->GetBase();
        if (!iter->second->IsPositive() && aura->GetSpellInfo()->Attributes & flag && (!guid || aura->GetCasterGUID() == guid))
            return true;
    }
    return false;
}

bool Unit::HasAuraWithAttribute(uint32 Attributes, uint32 flag) const
{
    for (AuraApplicationMap::const_iterator iter = m_appliedAuras.begin(); iter != m_appliedAuras.end(); ++iter)
    {
        Aura const* aura = iter->second->GetBase();
        switch(Attributes)
        {
            case 0:
                if(aura->GetSpellInfo()->Attributes & flag)
                    return true;
                break;
            case 1:
                if(aura->GetSpellInfo()->AttributesEx & flag)
                    return true;
                break;
            case 2:
                if(aura->GetSpellInfo()->AttributesEx2 & flag)
                    return true;
                break;
            case 3:
                if(aura->GetSpellInfo()->AttributesEx3 & flag)
                    return true;
                break;
            case 4:
                if(aura->GetSpellInfo()->AttributesEx4 & flag)
                    return true;
                break;
            case 5:
                if(aura->GetSpellInfo()->AttributesEx5 & flag)
                    return true;
                break;
            case 6:
                if(aura->GetSpellInfo()->AttributesEx6 & flag)
                    return true;
                break;
            case 7:
                if(aura->GetSpellInfo()->AttributesEx7 & flag)
                    return true;
                break;
            case 8:
                if(aura->GetSpellInfo()->AttributesEx8 & flag)
                    return true;
                break;
            case 9:
                if(aura->GetSpellInfo()->AttributesEx9 & flag)
                    return true;
                break;
            case 10:
                if(aura->GetSpellInfo()->AttributesEx10 & flag)
                    return true;
                break;
            case 11:
                if(aura->GetSpellInfo()->AttributesEx11 & flag)
                    return true;
                break;
            case 12:
                if(aura->GetSpellInfo()->AttributesEx12 & flag)
                    return true;
                break;
            case 13:
                if(aura->GetSpellInfo()->AttributesEx13 & flag)
                    return true;
                break;
        }
    }
    return false;
}

bool Unit::HasAuraWithMechanic(uint32 mechanicMask)
{
    for (AuraApplicationMap::iterator iter = m_appliedAuras.begin(); iter != m_appliedAuras.end(); ++iter)
    {
        SpellInfo const* spellInfo  = iter->second->GetBase()->GetSpellInfo();
        if (spellInfo->Mechanic && (mechanicMask & (1 << spellInfo->Mechanic)))
            return true;

        for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
            if (iter->second->HasEffect(i) && spellInfo->GetEffect(i, GetSpawnMode())->Effect && spellInfo->GetEffect(i, GetSpawnMode())->Mechanic)
                if (mechanicMask & (1 << spellInfo->GetEffect(i, GetSpawnMode())->Mechanic))
                    return true;
    }

    return false;
}

bool Unit::HasAuraCastWhileWalking(SpellInfo const* spellInfo)
{
    return HasAuraTypeWithAffectMask(SPELL_AURA_CAST_WHILE_WALKING, spellInfo) || 
           HasAuraTypeWithAffectMask(SPELL_AURA_MOD_CAST_TIME_WHILE_MOVING, spellInfo) || 
           (spellInfo->AttributesEx5 & SPELL_ATTR5_USABLE_WHILE_MOVING);
}

AuraEffect* Unit::IsScriptOverriden(SpellInfo const* spell, int32 script) const
{
    AuraEffectList auras = GetAuraEffectsByType(SPELL_AURA_OVERRIDE_CLASS_SCRIPTS);
    for (AuraEffectList::const_iterator i = auras.begin(); i != auras.end(); ++i)
    {
        if ((*i)->GetMiscValue() == script)
            if ((*i)->IsAffectingSpell(spell))
                return (*i);
    }
    return NULL;
}

uint32 Unit::GetDiseasesByCaster(uint64 casterGUID, bool remove)
{
    static const AuraType diseaseAuraTypes[] =
    {
        SPELL_AURA_PERIODIC_DAMAGE, // Frost Fever and Blood Plague
        SPELL_AURA_LINKED,          // Crypt Fever and Ebon Plague
        SPELL_AURA_NONE
    };

    uint32 diseases = 0;
    for (AuraType const* itr = &diseaseAuraTypes[0]; itr && itr[0] != SPELL_AURA_NONE; ++itr)
    {
        for (AuraEffectList::iterator i = m_modAuras[*itr].begin(); i != m_modAuras[*itr].end();)
        {
            // Get auras with disease dispel type by caster
            if ((*i)->GetSpellInfo()->Dispel == DISPEL_DISEASE
                && (*i)->GetCasterGUID() == casterGUID)
            {
                ++diseases;

                if (remove)
                {
                    RemoveAura((*i)->GetId(), (*i)->GetCasterGUID());
                    i = m_modAuras[*itr].begin();
                    continue;
                }
            }
            ++i;
        }
    }
    return diseases;
}

uint32 Unit::GetDoTsByCaster(uint64 casterGUID) const
{
    static const AuraType diseaseAuraTypes[] =
    {
        SPELL_AURA_PERIODIC_DAMAGE,
        SPELL_AURA_PERIODIC_DAMAGE_PERCENT,
        SPELL_AURA_NONE
    };

    uint32 dots = 0;
    for (AuraType const* itr = &diseaseAuraTypes[0]; itr && itr[0] != SPELL_AURA_NONE; ++itr)
    {
        Unit::AuraEffectList auras = GetAuraEffectsByType(*itr);
        for (AuraEffectList::const_iterator i = auras.begin(); i != auras.end(); ++i)
        {
            // Get auras by caster
            if ((*i)->GetCasterGUID() == casterGUID)
                ++dots;
        }
    }
    return dots;
}

std::list<AuraEffect*> Unit::GetTotalNotStuckAuraEffectByType(AuraType auratype) const
{
    AuraEffectList FinishedEffectList;
    std::multimap<SpellGroup, AuraEffect*> SameEffectSpellGroup;

    AuraEffectList mTotalAuraList = GetAuraEffectsByType(auratype);
    for (AuraEffectList::const_iterator i = mTotalAuraList.begin(), next; i != mTotalAuraList.end(); i = next)
    {
        next = i;
        ++next;
        if (AuraEffect* eff = (*i))
            if (!sSpellMgr->AddSameEffectStackRuleSpellGroups(eff->GetSpellInfo(), eff, SameEffectSpellGroup))
                FinishedEffectList.push_back(eff);
    }

    for (std::map<SpellGroup, AuraEffect*>::const_iterator itr = SameEffectSpellGroup.begin(); itr != SameEffectSpellGroup.end(); ++itr)
        FinishedEffectList.push_back(itr->second);

    return FinishedEffectList;
}

int32 Unit::GetTotalAuraModifier(AuraType auratype, bool raid) const
{
    std::map<SpellGroup, int32> SameEffectSpellGroup;
    int32 modifier = 0;
    int32 raidModifier = 0;

    AuraEffectList mTotalAuraList = GetAuraEffectsByType(auratype);
    for (AuraEffectList::const_iterator i = mTotalAuraList.begin(), next; i != mTotalAuraList.end(); i = next)
    {
        next = i;
        ++next;
        if (AuraEffect* eff = (*i))
        {
            if (raid && (eff->GetSpellInfo()->AttributesEx7 & SPELL_ATTR7_CONSOLIDATED_RAID_BUFF))
            {
                if (eff->GetAmount() > raidModifier)
                    raidModifier = eff->GetAmount();
            }
            else if (!sSpellMgr->AddSameEffectStackRuleSpellGroups(eff->GetSpellInfo(), eff->GetAmount(), SameEffectSpellGroup))
                modifier += eff->GetAmount();
        }
    }

    for (std::map<SpellGroup, int32>::const_iterator itr = SameEffectSpellGroup.begin(); itr != SameEffectSpellGroup.end(); ++itr)
        modifier += itr->second;

    return modifier + raidModifier;
}

int32 Unit::GetTotalForAurasModifier(std::list<AuraType> *auratypelist) const
{
    std::map<SpellGroup, int32> SameEffectSpellGroup;
    int32 modifier = 0;

    AuraEffectList mTotalAuraList;
    for (std::list<AuraType>::iterator auratype = auratypelist->begin(); auratype!= auratypelist->end(); ++auratype)
    {
        AuraEffectList swaps = GetAuraEffectsByType(*auratype);
        if (!swaps.empty())
            mTotalAuraList.insert(mTotalAuraList.end(), swaps.begin(), swaps.end());
    }

    if (!mTotalAuraList.empty())
    {
        for (AuraEffectList::const_iterator i = mTotalAuraList.begin(), next; i != mTotalAuraList.end(); i = next)
        {
            next = i;
            ++next;
            if (AuraEffect* eff = (*i))
                if (!sSpellMgr->AddSameEffectStackRuleSpellGroups(eff->GetSpellInfo(), eff->GetAmount(), SameEffectSpellGroup))
                    modifier += eff->GetAmount();
        }

        for (std::map<SpellGroup, int32>::const_iterator itr = SameEffectSpellGroup.begin(); itr != SameEffectSpellGroup.end(); ++itr)
            modifier += itr->second;
    }

    return modifier;
}

float Unit::GetTotalForAurasMultiplier(std::list<AuraType> *auratypelist) const
{
    std::map<SpellGroup, int32> SameEffectSpellGroup;
    float multiplier = 1.0f;

    AuraEffectList mTotalAuraList;
    for (std::list<AuraType>::iterator auratype = auratypelist->begin(); auratype!= auratypelist->end(); ++auratype)
    {
        AuraEffectList swaps = GetAuraEffectsByType(*auratype);
        if (!swaps.empty())
            mTotalAuraList.insert(mTotalAuraList.end(), swaps.begin(), swaps.end());
    }

    if (!mTotalAuraList.empty())
    {
        for (AuraEffectList::const_iterator i = mTotalAuraList.begin(), next; i != mTotalAuraList.end(); i = next)
        {
            next = i;
            ++next;
            if (AuraEffect* eff = (*i))
                if (!sSpellMgr->AddSameEffectStackRuleSpellGroups(eff->GetSpellInfo(), eff->GetAmount(), SameEffectSpellGroup))
                    AddPct(multiplier, eff->GetAmount());
        }

        for (std::map<SpellGroup, int32>::const_iterator itr = SameEffectSpellGroup.begin(); itr != SameEffectSpellGroup.end(); ++itr)
            AddPct(multiplier, itr->second);
    }

    return multiplier;
}

float Unit::GetTotalAuraMultiplier(AuraType auratype, bool raid) const
{
    std::map<SpellGroup, int32> SameEffectSpellGroup;
    float multiplier = 1.0f;
    int32 raidModifier = 0;

    AuraEffectList mTotalAuraList = GetAuraEffectsByType(auratype);
    for (AuraEffectList::const_iterator i = mTotalAuraList.begin(), next; i != mTotalAuraList.end(); i = next)
    {
        next = i;
        ++next;
        if (AuraEffect* eff = (*i))
            if (raid && (eff->GetSpellInfo()->AttributesEx7 & SPELL_ATTR7_CONSOLIDATED_RAID_BUFF))
            {
                if (eff->GetAmount() > raidModifier)
                    raidModifier = eff->GetAmount();
            }
            else if (!sSpellMgr->AddSameEffectStackRuleSpellGroups(eff->GetSpellInfo(), eff->GetAmount(), SameEffectSpellGroup))
                AddPct(multiplier, eff->GetAmount());
    }

    for (auto itr : SameEffectSpellGroup)
        AddPct(multiplier, itr.second);

    if (raidModifier)
        AddPct(multiplier, raidModifier);

    return multiplier;
}

float Unit::GetTotalPositiveAuraMultiplierByMiscMask(AuraType auratype, uint32 misc_mask) const
{
    std::map<SpellGroup, int32> SameEffectSpellGroup;
    float multiplier = 1.0f;

    AuraEffectList mTotalAuraList = GetAuraEffectsByType(auratype);
    for (AuraEffectList::const_iterator i = mTotalAuraList.begin(), next; i != mTotalAuraList.end(); i = next)
    {
        next = i;
        ++next;
        if (AuraEffect* eff = (*i))
            if ((eff->GetMiscValue() & misc_mask) && eff->GetAmount() > 0)
                if (!sSpellMgr->AddSameEffectStackRuleSpellGroups(eff->GetSpellInfo(), eff->GetAmount(), SameEffectSpellGroup))
                    AddPct(multiplier, eff->GetAmount());
    }

    for (std::map<SpellGroup, int32>::const_iterator itr = SameEffectSpellGroup.begin(); itr != SameEffectSpellGroup.end(); ++itr)
        AddPct(multiplier, itr->second);

    return multiplier;
}

int32 Unit::GetMaxPositiveAuraModifier(AuraType auratype)
{
    int32 modifier = 0;

    AuraEffectList mTotalAuraList = GetAuraEffectsByType(auratype);
    for (AuraEffectList::const_iterator i = mTotalAuraList.begin(), next; i != mTotalAuraList.end(); i = next)
    {
        next = i;
        ++next;
        if (AuraEffect* eff = (*i))
            if (eff->GetAmount() > modifier)
                modifier = eff->GetAmount();
    }

    return modifier;
}

int32 Unit::GetMaxNegativeAuraModifier(AuraType auratype) const
{
    int32 modifier = 0;

    AuraEffectList mTotalAuraList = GetAuraEffectsByType(auratype);
    for (AuraEffectList::const_iterator i = mTotalAuraList.begin(), next; i != mTotalAuraList.end(); i = next)
    {
        next = i;
        ++next;
        if (AuraEffect* eff = (*i))
            if (eff->GetAmount() < modifier)
                modifier = eff->GetAmount();
    }

    return modifier;
}

int32 Unit::GetTotalAuraModifierByMiscMask(AuraType auratype, uint32 misc_mask) const
{
    std::map<SpellGroup, int32> SameEffectSpellGroup;
    int32 modifier = 0;

    AuraEffectList mTotalAuraList = GetAuraEffectsByType(auratype);
    uint32 _sizeList = mTotalAuraList.size();

    for (AuraEffectList::const_iterator i = mTotalAuraList.begin(), next; i != mTotalAuraList.end(); i = next)
    {
        next = i;
        ++next;
        if (_sizeList != mTotalAuraList.size())
            break;

        if (i == mTotalAuraList.end())
            continue;

        if (AuraEffect* eff = (*i))
            if (eff->GetMiscValue() & misc_mask)
                if (!sSpellMgr->AddSameEffectStackRuleSpellGroups(eff->GetSpellInfo(), eff->GetAmount(), SameEffectSpellGroup))
                    modifier += eff->GetAmount();
    }

    for (std::map<SpellGroup, int32>::const_iterator itr = SameEffectSpellGroup.begin(); itr != SameEffectSpellGroup.end(); ++itr)
        modifier += itr->second;

    return modifier;
}

float Unit::GetTotalAuraMultiplierByMiscMask(AuraType auratype, uint32 misc_mask, bool raid, bool miscB) const
{
    std::map<SpellGroup, int32> SameEffectSpellGroup;
    float multiplier = 1.0f;
    int32 raidModifier = 0;

    AuraEffectList mTotalAuraList = GetAuraEffectsByType(auratype);
    for (AuraEffectList::const_iterator i = mTotalAuraList.begin(), next; i != mTotalAuraList.end(); i = next)
    {
        next = i;
        ++next;
        if (AuraEffect* eff = (*i))
            if (((miscB ? eff->GetMiscValueB() : eff->GetMiscValue()) & misc_mask))
            {
                if (raid && (eff->GetSpellInfo()->AttributesEx7 & SPELL_ATTR7_CONSOLIDATED_RAID_BUFF))
                {
                    if (eff->GetAmount() > raidModifier)
                        raidModifier = eff->GetAmount();
                }
                else if (!sSpellMgr->AddSameEffectStackRuleSpellGroups(eff->GetSpellInfo(), eff->GetAmount(), SameEffectSpellGroup))
                    AddPct(multiplier, eff->GetAmount());
            }
    }
    // Add the highest of the Same Effect Stack Rule SpellGroups to the multiplier
    for (std::map<SpellGroup, int32>::const_iterator itr = SameEffectSpellGroup.begin(); itr != SameEffectSpellGroup.end(); ++itr)
        AddPct(multiplier, itr->second);

    if (raidModifier)
        AddPct(multiplier, raidModifier);

    return multiplier;
}

int32 Unit::GetMaxPositiveAuraModifierByMiscMask(AuraType auratype, uint32 misc_mask, AuraEffect const* except) const
{
    int32 modifier = 0;

    AuraEffectList mTotalAuraList = GetAuraEffectsByType(auratype);
    for (AuraEffectList::const_iterator i = mTotalAuraList.begin(), next; i != mTotalAuraList.end(); i = next)
    {
        next = i;
        ++next;
        if (except != (*i) && (*i)->GetMiscValue()& misc_mask && (*i)->GetAmount() > modifier)
            modifier = (*i)->GetAmount();
    }

    return modifier;
}

int32 Unit::GetMaxNegativeAuraModifierByMiscMask(AuraType auratype, uint32 misc_mask) const
{
    int32 modifier = 0;

    AuraEffectList mTotalAuraList = GetAuraEffectsByType(auratype);
    for (AuraEffectList::const_iterator i = mTotalAuraList.begin(), next; i != mTotalAuraList.end(); i = next)
    {
        next = i;
        ++next;
        if (AuraEffect* eff = (*i))
            if (eff->GetMiscValue()& misc_mask && eff->GetAmount() < modifier)
                modifier = eff->GetAmount();
    }

    return modifier;
}

int32 Unit::GetTotalAuraModifierByMiscValue(AuraType auratype, int32 misc_value) const
{
    std::map<SpellGroup, int32> SameEffectSpellGroup;
    int32 modifier = 0;

    AuraEffectList mTotalAuraList = GetAuraEffectsByType(auratype);
    for (AuraEffectList::const_iterator i = mTotalAuraList.begin(), next; i != mTotalAuraList.end(); i = next)
    {
        next = i;
        ++next;
        if (AuraEffect* eff = (*i))
            if (eff->GetMiscValue() == misc_value)
                if (!sSpellMgr->AddSameEffectStackRuleSpellGroups(eff->GetSpellInfo(), eff->GetAmount(), SameEffectSpellGroup))
                    modifier += eff->GetAmount();
    }

    for (std::map<SpellGroup, int32>::const_iterator itr = SameEffectSpellGroup.begin(); itr != SameEffectSpellGroup.end(); ++itr)
        modifier += itr->second;

    return modifier;
}

float Unit::GetTotalAuraMultiplierByMiscValue(AuraType auratype, int32 misc_value) const
{
    std::map<SpellGroup, int32> SameEffectSpellGroup;
    float multiplier = 1.0f;

    AuraEffectList mTotalAuraList = GetAuraEffectsByType(auratype);
    for (AuraEffectList::const_iterator i = mTotalAuraList.begin(), next; i != mTotalAuraList.end(); i = next)
    {
        next = i;
        ++next;
        if (AuraEffect* eff = (*i))
            if (eff->GetMiscValue() == misc_value)
                if (!sSpellMgr->AddSameEffectStackRuleSpellGroups(eff->GetSpellInfo(), eff->GetAmount(), SameEffectSpellGroup))
                    AddPct(multiplier, eff->GetAmount());
    }

    for (std::map<SpellGroup, int32>::const_iterator itr = SameEffectSpellGroup.begin(); itr != SameEffectSpellGroup.end(); ++itr)
        AddPct(multiplier, itr->second);

    return multiplier;
}

float Unit::GetTotalAuraMultiplierByMiscValueB(AuraType auratype, int32 misc_value, int32 misc_valueB) const
{
    std::map<SpellGroup, int32> SameEffectSpellGroup;
    float multiplier = 1.0f;

    AuraEffectList mTotalAuraList = GetAuraEffectsByType(auratype);
    for (AuraEffectList::const_iterator i = mTotalAuraList.begin(), next; i != mTotalAuraList.end(); i = next)
    {
        next = i;
        ++next;
        if (AuraEffect* eff = (*i))
            if (eff->GetMiscValue() == misc_value)
                if (eff->GetMiscValueB() == 0 || eff->GetMiscValueB() == misc_valueB)
                    if (!sSpellMgr->AddSameEffectStackRuleSpellGroups(eff->GetSpellInfo(), eff->GetAmount(), SameEffectSpellGroup))
                        AddPct(multiplier, eff->GetAmount());
    }

    for (std::map<SpellGroup, int32>::const_iterator itr = SameEffectSpellGroup.begin(); itr != SameEffectSpellGroup.end(); ++itr)
        AddPct(multiplier, itr->second);

    return multiplier;
}

int32 Unit::GetMaxPositiveAuraModifierByMiscValue(AuraType auratype, int32 misc_value) const
{
    int32 modifier = 0;

    AuraEffectList mTotalAuraList = GetAuraEffectsByType(auratype);
    for (AuraEffectList::const_iterator i = mTotalAuraList.begin(), next; i != mTotalAuraList.end(); i = next)
    {
        next = i;
        ++next;
        if (AuraEffect* eff = (*i))
            if (eff->GetMiscValue() == misc_value && eff->GetAmount() > modifier)
                modifier = eff->GetAmount();
    }

    return modifier;
}

int32 Unit::GetMaxNegativeAuraModifierByMiscValue(AuraType auratype, int32 misc_value) const
{
    int32 modifier = 0;

    AuraEffectList mTotalAuraList = GetAuraEffectsByType(auratype);
    for (AuraEffectList::const_iterator i = mTotalAuraList.begin(), next; i != mTotalAuraList.end(); i = next)
    {
        next = i;
        ++next;
        if (AuraEffect* eff = (*i))
            if (eff->GetMiscValue() == misc_value && eff->GetAmount() < modifier)
                modifier = eff->GetAmount();
    }

    return modifier;
}

int32 Unit::GetTotalAuraModifierByAffectMask(AuraType auratype, SpellInfo const* affectedSpell) const
{
    std::map<SpellGroup, int32> SameEffectSpellGroup;
    int32 modifier = 0;

    AuraEffectList mTotalAuraList = GetAuraEffectsByType(auratype);
    for (AuraEffectList::const_iterator i = mTotalAuraList.begin(), next; i != mTotalAuraList.end(); i = next)
    {
        next = i;
        ++next;
        if (AuraEffect* eff = (*i))
            if (eff->IsAffectingSpell(affectedSpell))
                if (!sSpellMgr->AddSameEffectStackRuleSpellGroups(eff->GetSpellInfo(), eff->GetAmount(), SameEffectSpellGroup))
                    modifier += eff->GetAmount();
    }

    for (std::map<SpellGroup, int32>::const_iterator itr = SameEffectSpellGroup.begin(); itr != SameEffectSpellGroup.end(); ++itr)
        modifier += itr->second;

    return modifier;
}

float Unit::GetTotalAuraMultiplierByAffectMask(AuraType auratype, SpellInfo const* affectedSpell) const
{
    std::map<SpellGroup, int32> SameEffectSpellGroup;
    float multiplier = 1.0f;

    AuraEffectList mTotalAuraList = GetAuraEffectsByType(auratype);
    for (AuraEffectList::const_iterator i = mTotalAuraList.begin(), next; i != mTotalAuraList.end(); i = next)
    {
        next = i;
        ++next;
        if (AuraEffect* eff = (*i))
            if (eff->IsAffectingSpell(affectedSpell))
                if (!sSpellMgr->AddSameEffectStackRuleSpellGroups(eff->GetSpellInfo(), eff->GetAmount(), SameEffectSpellGroup))
                    AddPct(multiplier, eff->GetAmount());
    }

    for (std::map<SpellGroup, int32>::const_iterator itr = SameEffectSpellGroup.begin(); itr != SameEffectSpellGroup.end(); ++itr)
        AddPct(multiplier, itr->second);

    return multiplier;
}

int32 Unit::GetMaxPositiveAuraModifierByAffectMask(AuraType auratype, SpellInfo const* affectedSpell) const
{
    int32 modifier = 0;

    AuraEffectList mTotalAuraList = GetAuraEffectsByType(auratype);
    for (AuraEffectList::const_iterator i = mTotalAuraList.begin(), next; i != mTotalAuraList.end(); i = next)
    {
        next = i;
        ++next;
        if (AuraEffect* eff = (*i))
            if (eff->IsAffectingSpell(affectedSpell) && eff->GetAmount() > modifier)
                modifier = eff->GetAmount();
    }

    return modifier;
}

int32 Unit::GetMaxNegativeAuraModifierByAffectMask(AuraType auratype, SpellInfo const* affectedSpell) const
{
    int32 modifier = 0;

    AuraEffectList mTotalAuraList = GetAuraEffectsByType(auratype);
    for (AuraEffectList::const_iterator i = mTotalAuraList.begin(), next; i != mTotalAuraList.end(); i = next)
    {
        next = i;
        ++next;
        if (AuraEffect* eff = (*i))
            if (eff->IsAffectingSpell(affectedSpell) && eff->GetAmount() < modifier)
                modifier = eff->GetAmount();
    }

    return modifier;
}

void Unit::_RegisterDynObject(DynamicObject* dynObj)
{
    m_dynObj.push_back(dynObj);
}

void Unit::_UnregisterDynObject(DynamicObject* dynObj)
{
    m_dynObj.remove(dynObj);
}

DynamicObject* Unit::GetDynObject(uint32 spellId)
{
    if (m_dynObj.empty())
        return NULL;
    for (DynObjectList::const_iterator i = m_dynObj.begin(); i != m_dynObj.end();++i)
    {
        DynamicObject* dynObj = *i;
        if (dynObj->GetSpellId() == spellId)
            return dynObj;
    }
    return NULL;
}

int32 Unit::CountDynObject(uint32 spellId)
{
    int32 count = 0;

    if (m_dynObj.empty())
        return 0;
    for (DynObjectList::const_iterator i = m_dynObj.begin(); i != m_dynObj.end();++i)
    {
        DynamicObject* dynObj = *i;
        if (dynObj->GetSpellId() == spellId)
            count++;
    }
    return count;
}

void Unit::GetDynObjectList(std::list<DynamicObject*> &list, uint32 spellId)
{
    if (m_dynObj.empty())
        return;
    for (DynObjectList::const_iterator i = m_dynObj.begin(); i != m_dynObj.end();++i)
    {
        DynamicObject* dynObj = *i;
        if (dynObj->GetSpellId() == spellId)
            list.push_back(dynObj);
    }
}

void Unit::RemoveDynObject(uint32 spellId)
{
    if (m_dynObj.empty())
        return;
    for (DynObjectList::iterator i = m_dynObj.begin(); i != m_dynObj.end();)
    {
        DynamicObject* dynObj = *i;
        if (dynObj->GetSpellId() == spellId)
        {
            dynObj->Remove();
            i = m_dynObj.begin();
        }
        else
            ++i;
    }
}

void Unit::RemoveAllDynObjects()
{
    while (!m_dynObj.empty())
        m_dynObj.front()->Remove();
}

/*AreaTrigger*/
void Unit::_RegisterAreaObject(AreaTrigger* areaObj)
{
    m_AreaObj.push_back(areaObj);
}

void Unit::_UnregisterAreaObject(AreaTrigger* areaObj)
{
    m_AreaObj.remove(areaObj);
}

AreaTrigger* Unit::GetAreaObject(uint32 spellId)
{
    if (m_AreaObj.empty())
        return NULL;
    for (AreaObjectList::const_iterator i = m_AreaObj.begin(); i != m_AreaObj.end();++i)
    {
        AreaTrigger* areaObj = *i;
        if (areaObj->GetSpellId() == spellId)
            return areaObj;
    }
    return NULL;
}

int32 Unit::CountAreaObject(uint32 spellId)
{
    int32 count = 0;

    if (m_AreaObj.empty())
        return 0;
    for (AreaObjectList::const_iterator i = m_AreaObj.begin(); i != m_AreaObj.end();++i)
    {
        AreaTrigger* areaObj = *i;
        if (areaObj->GetSpellId() == spellId)
            count++;
    }
    return count;
}

void Unit::GetAreaObjectList(std::list<AreaTrigger*> &list, uint32 spellId)
{
    if (m_AreaObj.empty())
        return;
    for (AreaObjectList::const_iterator i = m_AreaObj.begin(); i != m_AreaObj.end();++i)
    {
        AreaTrigger* areaObj = *i;
        if (areaObj->GetSpellId() == spellId)
            list.push_back(areaObj);
    }
}

void Unit::RemoveAreaObject(uint32 spellId)
{
    if (m_AreaObj.empty())
        return;
    for (AreaObjectList::iterator i = m_AreaObj.begin(); i != m_AreaObj.end();)
    {
        AreaTrigger* areaObj = *i;
        if (areaObj->GetSpellId() == spellId)
        {
            areaObj->Despawn();
            i = m_AreaObj.begin();
        }
        else
            ++i;
    }
}

void Unit::RemoveAllAreaObjects()
{
    while (!m_AreaObj.empty())
        m_AreaObj.front()->Despawn();
}

GameObject* Unit::GetGameObject(uint32 spellId) const
{
    for (GameObjectList::const_iterator i = m_gameObj.begin(); i != m_gameObj.end(); ++i)
        if ((*i)->GetSpellId() == spellId)
            return *i;

    return NULL;
}

GameObject* Unit::GetGameObjectbyId(uint32 entry) const
{
    for (GameObjectList::const_iterator i = m_gameObj.begin(); i != m_gameObj.end(); ++i)
        if ((*i)->GetEntry() == entry)
            return *i;

    return NULL;
}


void Unit::AddGameObject(GameObject* gameObj)
{
    if (!gameObj || !gameObj->GetOwnerGUID() == 0)
        return;

    m_gameObj.push_back(gameObj);
    gameObj->SetOwnerGUID(GetGUID());

    if (GetTypeId() == TYPEID_PLAYER && gameObj->GetSpellId())
    {
        SpellInfo const* createBySpell = sSpellMgr->GetSpellInfo(gameObj->GetSpellId());
        // Need disable spell use for owner
        if (createBySpell && createBySpell->Attributes & SPELL_ATTR0_DISABLED_WHILE_ACTIVE)
            // note: item based cooldowns and cooldown spell mods with charges ignored (unknown existing cases)
            ToPlayer()->AddSpellAndCategoryCooldowns(createBySpell, 0, NULL, true);
    }
}

void Unit::RemoveGameObject(GameObject* gameObj, bool del)
{
    if (!gameObj || gameObj->GetOwnerGUID() != GetGUID())
        return;

    gameObj->SetOwnerGUID(0);

    for (uint8 i = 0; i < MAX_GAMEOBJECT_SLOT; ++i)
    {
        if (m_ObjectSlot[i] == gameObj->GetGUID())
        {
            m_ObjectSlot[i] = 0;
            break;
        }
    }

    // GO created by some spell
    if (uint32 spellid = gameObj->GetSpellId())
    {
        RemoveAurasDueToSpell(spellid);

        if (GetTypeId() == TYPEID_PLAYER)
        {
            SpellInfo const* createBySpell = sSpellMgr->GetSpellInfo(spellid);
            // Need activate spell use for owner
            if (createBySpell && createBySpell->Attributes & SPELL_ATTR0_DISABLED_WHILE_ACTIVE)
                // note: item based cooldowns and cooldown spell mods with charges ignored (unknown existing cases)
                ToPlayer()->SendCooldownEvent(createBySpell);
        }
    }

    m_gameObj.remove(gameObj);

    if (del)
    {
        gameObj->SetRespawnTime(0);
        gameObj->Delete();
    }
}

void Unit::RemoveGameObject(uint32 spellid, bool del)
{
    if (m_gameObj.empty())
        return;
    GameObjectList::iterator i, next;
    for (i = m_gameObj.begin(); i != m_gameObj.end(); i = next)
    {
        next = i;
        if (spellid == 0 || (*i)->GetSpellId() == spellid)
        {
            (*i)->SetOwnerGUID(0);
            if (del)
            {
                (*i)->SetRespawnTime(0);
                (*i)->Delete();
            }

            next = m_gameObj.erase(i);
        }
        else
            ++next;
    }
}

void Unit::RemoveAllGameObjects()
{
    // remove references to unit
    while (!m_gameObj.empty())
    {
        GameObjectList::iterator i = m_gameObj.begin();
        (*i)->SetOwnerGUID(0);
        (*i)->SetRespawnTime(0);
        (*i)->Delete();
        m_gameObj.erase(i);
    }
}

void Unit::SendSpellNonMeleeDamageLog(SpellNonMeleeDamage* log)
{
    uint32 newDamage = log->damage;
    if(log->SpellID)
    {
        SpellBonusEntry const* bonus = sSpellMgr->GetSpellBonusData(log->SpellID);
        if(bonus && bonus->damage_bonus)
            newDamage *= bonus->damage_bonus;
    }

    ObjectGuid casterGuid = log->attacker->GetObjectGuid();
    ObjectGuid targetGuid = log->target->GetObjectGuid();
    int32 overkill = newDamage - log->target->GetHealth();

    WorldPacket data(SMSG_SPELLNONMELEEDAMAGELOG, 8 + 8 + 1 + 1 + 1 + 7 * 4 + 1);
    data.WriteGuidMask<1, 6, 0>(targetGuid);
    data.WriteGuidMask<3>(casterGuid);

    data.WriteBit(0);                               // unk
    data.WriteGuidMask<4>(casterGuid);
    data.WriteGuidMask<3>(targetGuid);
    data.WriteBit(log->physicalLog);                // if 1, then client show spell name (example: %s's ranged shot hit %s for %u school or %s suffers %u school damage from %s's spell_name
    data.WriteGuidMask<2>(targetGuid);
    data.WriteGuidMask<7, 2>(casterGuid);
    data.WriteBit(0);                               // not has power data
    data.WriteGuidMask<7>(targetGuid);

    data.WriteGuidMask<1, 5>(casterGuid);

    data.WriteBit(0);                               // not has floats (extended data)

    data.WriteGuidMask<5, 4>(targetGuid);
    data.WriteGuidMask<0, 6>(casterGuid);

    data.WriteGuidBytes<7>(targetGuid);
    data << uint32(newDamage);                      // damage amount
    data.WriteGuidBytes<4, 6>(targetGuid);
    data << uint32(log->absorb);                    // AbsorbedDamage
    data.WriteGuidBytes<4>(casterGuid);
    data.WriteGuidBytes<2>(targetGuid);
    data << uint32(log->blocked);                   // blocked
    data << uint32(log->SpellID);
    data.WriteGuidBytes<1>(targetGuid);
    data.WriteGuidBytes<3>(casterGuid);
    data << uint8 (log->schoolMask);                // damage school
    data.WriteGuidBytes<7>(casterGuid);
    data << uint32(log->HitInfo);
    data.WriteGuidBytes<0>(targetGuid);
    data.WriteGuidBytes<0>(casterGuid);
    data.WriteGuidBytes<5>(targetGuid);
    data.WriteGuidBytes<6>(casterGuid);
    data << uint32(log->resist);                    // resist
    data.WriteGuidBytes<3>(targetGuid);
    data.WriteGuidBytes<5>(casterGuid);
    data << uint32(overkill > 0 ? overkill : -1);   // overkill
    data.WriteGuidBytes<2, 1>(casterGuid);

    SendMessageToSet(&data, true);
}

void Unit::SendSpellNonMeleeDamageLog(Unit* target, uint32 SpellID, uint32 Damage, SpellSchoolMask damageSchoolMask, uint32 AbsorbedDamage, uint32 Resist, bool PhysicalDamage, uint32 Blocked, bool CriticalHit)
{
    SpellNonMeleeDamage log(this, target, SpellID, damageSchoolMask);
    log.damage = Damage - AbsorbedDamage - Resist - Blocked;
    log.absorb = AbsorbedDamage;
    log.resist = Resist;
    log.physicalLog = PhysicalDamage;
    log.blocked = Blocked;
    log.HitInfo = SPELL_HIT_TYPE_UNK1 | SPELL_HIT_TYPE_UNK3 | SPELL_HIT_TYPE_UNK6;
    if (CriticalHit)
        log.HitInfo |= SPELL_HIT_TYPE_CRIT;
    SendSpellNonMeleeDamageLog(&log);
}

void Unit::ProcDamageAndSpell(Unit* victim, uint32 procAttacker, uint32 procVictim, uint32 procExtra, DamageInfo* dmgInfoProc, WeaponAttackType attType, SpellInfo const* procSpell, SpellInfo const* procAura, std::list<uint32>* mSpellModsList)
{
     // Not much to do if no flags are set.
    if (procAttacker)
        ProcDamageAndSpellFor(false, victim, procAttacker, procExtra, attType, procSpell, dmgInfoProc, procAura, mSpellModsList);
    // Now go on with a victim's events'n'auras
    // Not much to do if no flags are set or there is no victim
    if (victim && victim->isAlive() && procVictim)
        victim->ProcDamageAndSpellFor(true, this, procVictim, procExtra, attType, procSpell, dmgInfoProc, procAura, mSpellModsList);
}

void Unit::SendPeriodicAuraLog(SpellPeriodicAuraLogInfo* pInfo)
{
    AuraEffect const* aura = pInfo->auraEff;

    ObjectGuid casterGuid = aura->GetCasterGUID();
    ObjectGuid targetGuid = GetObjectGuid();
    WorldPacket data(SMSG_PERIODICAURALOG, 30);
    data.WriteBit(0);                   // not has power data

    data.WriteBits(1, 21);              // aura count
    data.WriteGuidMask<1, 6, 2, 5>(casterGuid);
    data.WriteGuidMask<5>(targetGuid);
    data.WriteGuidMask<4>(casterGuid);

    ByteBuffer buff;
    //for (var i = 0; i < count; ++i)
    {
        switch (aura->GetAuraType())
        {
            case SPELL_AURA_PERIODIC_DAMAGE:
            case SPELL_AURA_PERIODIC_DAMAGE_PERCENT:
            {
                uint32 schoolmask = aura->GetSpellInfo()->GetSchoolMask();

                data.WriteBit(pInfo->critical);                         // new 3.1.2 critical tick
                data.WriteBit(0);                                       // has overkill
                data.WriteBit(!pInfo->resist);                          // resist
                data.WriteBit(!schoolmask);
                data.WriteBit(!pInfo->absorb);                          // absorb

                if (pInfo->resist)
                    buff << uint32(pInfo->resist);                      // resist
                if (schoolmask)
                    buff << uint32(schoolmask);
                buff << uint32(pInfo->overDamage > 0 ? pInfo->overDamage : -1);
                buff << uint32(aura->GetAuraType());                    // auraId
                buff << uint32(pInfo->damage);                          // damage
                if (pInfo->absorb)
                    buff << uint32(pInfo->absorb);                      // absorb
                break;
            }
            case SPELL_AURA_PERIODIC_HEAL:
            case SPELL_AURA_OBS_MOD_HEALTH:
            {
                data.WriteBit(pInfo->critical);                         // new 3.1.2 critical tick
                data.WriteBit(0);                                       // has overkill
                data.WriteBit(1);                                       // not has resist
                data.WriteBit(1);                                       // not has school mask or power type
                data.WriteBit(!pInfo->absorb);                          // absorb

                buff << uint32(pInfo->overDamage > 0 ? pInfo->overDamage : -1);
                buff << uint32(aura->GetAuraType());                    // auraId
                buff << uint32(pInfo->damage);                          // amount
                if (pInfo->absorb)
                    buff << uint32(pInfo->absorb);                      // absorb
                break;
            }
            case SPELL_AURA_OBS_MOD_POWER:
            case SPELL_AURA_PERIODIC_ENERGIZE:
            {
                data.WriteBit(0);                                       // always non-critical
                data.WriteBit(1);                                       // not has overkill
                data.WriteBit(1);                                       // not has resist
                data.WriteBit(!aura->GetMiscValue());                   // power type
                data.WriteBit(1);                                       // not has absorb or power gain

                if (aura->GetMiscValue())
                    buff << uint32(aura->GetMiscValue());               // power type
                buff << uint32(aura->GetAuraType());                    // auraId
                buff << uint32(pInfo->damage);                          // amount
                break;
            }
            case SPELL_AURA_PERIODIC_MANA_LEECH:
            {
                data.WriteBit(0);                                       // always non-critical
                data.WriteBit(1);                                       // not has overkill
                data.WriteBit(1);                                       // not has resist
                data.WriteBit(!aura->GetMiscValue());                   // has power type
                data.WriteBit(pInfo->multiplier == 0.0f);               // multiplier

                if (aura->GetMiscValue())
                    buff << uint32(aura->GetMiscValue());               // power type
                buff << uint32(aura->GetAuraType());                    // auraId
                buff << uint32(pInfo->damage);                          // amount
                if (pInfo->multiplier != 0.0f)
                    buff << float(pInfo->multiplier);                   // gain multiplier
                break;
            }
            default:
                sLog->outError(LOG_FILTER_UNITS, "Unit::SendPeriodicAuraLog: unknown aura %u", uint32(aura->GetAuraType()));
                return;
        }
    }

    data.WriteGuidMask<6, 1>(targetGuid);
    data.WriteGuidMask<0>(casterGuid);
    data.WriteGuidMask<2, 4>(targetGuid);
    data.WriteGuidMask<3>(casterGuid);
    data.WriteGuidMask<3, 0>(targetGuid);
    data.WriteGuidMask<7>(casterGuid);
    data.WriteGuidMask<7>(targetGuid);

    if (!buff.empty())
    {
        data.FlushBits();
        data.append(buff);
    }

    data.WriteGuidBytes<1>(targetGuid);

    data << uint32(aura->GetId());                          // spellId

    data.WriteGuidBytes<2>(targetGuid);
    data.WriteGuidBytes<4, 7, 5, 0, 2, 3>(casterGuid);
    data.WriteGuidBytes<4, 6, 0>(targetGuid);
    data.WriteGuidBytes<1>(casterGuid);
    data.WriteGuidBytes<7>(targetGuid);
    data.WriteGuidBytes<6>(casterGuid);
    data.WriteGuidBytes<3, 5>(targetGuid);

    SendMessageToSet(&data, true);
}

void Unit::SendSpellMiss(Unit* target, uint32 spellID, SpellMissInfo missInfo)
{
    ObjectGuid casterGuid = GetObjectGuid();
    ObjectGuid targetGuid = target->GetObjectGuid();

    WorldPacket data(SMSG_SPELLLOGMISS, 8 + 8 + 1 + 1 + 1 + 3 + 4);
    data.WriteGuidMask<5, 2, 4>(casterGuid);
    data.WriteBit(0);           // not has power data
    data.WriteGuidMask<1>(casterGuid);

    data.WriteBits(1, 23);      // miss count
    //for (var i = 0; i < missCount; ++i)
    {
        data.WriteBit(0);       // not has floats
        data.WriteGuidMask<1, 0, 3, 4, 5, 7, 2, 6>(targetGuid);
    }

    data.WriteGuidMask<0, 6, 3, 7>(casterGuid);

    //for (var i = 0; i < missCount; ++i)
    {
        data.WriteGuidBytes<4, 0, 1, 7, 6, 3, 5>(targetGuid);
        data << uint8(missInfo);
        data.WriteGuidBytes<2>(targetGuid);
    }

    data.WriteGuidBytes<3, 6, 4, 2, 1, 7, 5, 0>(casterGuid);
    data << uint32(spellID);

    SendMessageToSet(&data, true);
}

void Unit::SendSpellDamageResist(Unit* target, uint32 spellId)
{
    WorldPacket data(SMSG_PROCRESIST, 8+8+4+1);
    data << uint64(GetGUID());
    data << uint64(target->GetGUID());
    data << uint32(spellId);
    data << uint8(0); // bool - log format: 0-default, 1-debug
    SendMessageToSet(&data, true);
}

void Unit::SendSpellDamageImmune(Unit* target, uint32 spellId)
{
    ObjectGuid casterGuid = GetObjectGuid();
    ObjectGuid targetGuid = target->GetObjectGuid();

    WorldPacket data(SMSG_SPELLORDAMAGE_IMMUNE, 8 + 8 + 4 + 1 + 1 + 1);
    data.WriteGuidMask<3, 1>(targetGuid);
    data.WriteGuidMask<1, 5>(casterGuid);
    data.WriteBit(1);       // 1 - spell immmune, 0 - damage? (1 seen only)
    data.WriteGuidMask<7>(targetGuid);
    data.WriteGuidMask<7, 2>(casterGuid);
    data.WriteGuidMask<5>(targetGuid);
    data.WriteGuidMask<3, 0>(casterGuid);
    data.WriteBit(0);       // not has power data
    data.WriteGuidMask<4>(casterGuid);
    data.WriteGuidMask<0>(targetGuid);
    data.WriteGuidMask<6>(casterGuid);
    data.WriteGuidMask<6>(targetGuid);

    data.WriteGuidMask<2, 4>(targetGuid);
    data.WriteGuidBytes<1, 4, 2>(casterGuid);
    data.WriteGuidBytes<4, 5>(targetGuid);
    data.WriteGuidBytes<0, 6>(casterGuid);
    data.WriteGuidBytes<2, 6>(targetGuid);
    data.WriteGuidBytes<7, 3, 5>(casterGuid);
    data << uint32(spellId);
    data.WriteGuidBytes<0, 7, 1, 3>(targetGuid);

    SendMessageToSet(&data, true);
}

void Unit::SendAttackStateUpdate(CalcDamageInfo* damageInfo)
{
    sLog->outDebug(LOG_FILTER_UNITS, "WORLD: Sending SMSG_ATTACKERSTATEUPDATE");

    uint32 count = 1;
    size_t maxsize = 4+5+5+4+4+1+4+4+4+4+4+1+4+4+4+4+4*12;
    WorldPacket data(SMSG_ATTACKERSTATEUPDATE, maxsize);    // we guess size
    data << uint32(damageInfo->HitInfo);
    data.append(damageInfo->attacker->GetPackGUID());
    data.append(damageInfo->target->GetPackGUID());
    data << uint32(damageInfo->damage);                     // Full damage
    int32 overkill = damageInfo->damage - damageInfo->target->GetHealth();
    data << uint32(overkill < 0 ? 0 : overkill);            // Overkill
    data << uint8(count);                                   // Sub damage count

    for (uint32 i = 0; i < count; ++i)
    {
        data << uint32(damageInfo->damageSchoolMask);       // School of sub damage
        data << float(damageInfo->damage);                  // sub damage
        data << uint32(damageInfo->damage);                 // Sub Damage
    }

    if (damageInfo->HitInfo & (HITINFO_FULL_ABSORB | HITINFO_PARTIAL_ABSORB))
    {
        for (uint32 i = 0; i < count; ++i)
            data << uint32(damageInfo->absorb);             // Absorb
    }

    if (damageInfo->HitInfo & (HITINFO_FULL_RESIST | HITINFO_PARTIAL_RESIST))
    {
        for (uint32 i = 0; i < count; ++i)
            data << uint32(damageInfo->resist);             // Resist
    }

    data << uint8(damageInfo->TargetState);
    data << uint32(0);  // Unknown attackerstate
    data << uint32(0);  // Melee spellid

    if (damageInfo->HitInfo & HITINFO_BLOCK)
        data << uint32(damageInfo->blocked_amount);

    if (damageInfo->HitInfo & HITINFO_RAGE_GAIN)
        data << uint32(0);

    //! Probably used for debugging purposes, as it is not known to appear on retail servers
    if (damageInfo->HitInfo & HITINFO_UNK1)
    {
        data << uint32(0);
        data << float(0);
        data << float(0);
        data << float(0);
        data << float(0);
        data << float(0);
        data << float(0);
        data << float(0);
        data << float(0);
        for (uint8 i = 0; i < 2; ++i)
        {
            data << float(0);
            data << float(0);
        }
        data << uint32(0);
    }

    if (damageInfo->HitInfo & 0x3000)
        data << float(0);

    SendMessageToSet(&data, true);
}

void Unit::SendAttackStateUpdate(uint32 HitInfo, Unit* target, uint8 /*SwingType*/, SpellSchoolMask damageSchoolMask, uint32 Damage, uint32 AbsorbDamage, uint32 Resist, VictimState TargetState, uint32 BlockedAmount)
{
    CalcDamageInfo dmgInfo;
    dmgInfo.HitInfo = HitInfo;
    dmgInfo.attacker = this;
    dmgInfo.target = target;
    dmgInfo.damage = Damage - AbsorbDamage - Resist - BlockedAmount;
    dmgInfo.damageSchoolMask = damageSchoolMask;
    dmgInfo.absorb = AbsorbDamage;
    dmgInfo.resist = Resist;
    dmgInfo.TargetState = TargetState;
    dmgInfo.blocked_amount = BlockedAmount;
    SendAttackStateUpdate(&dmgInfo);
}

bool Unit::HandleHasteAuraProc(Unit* victim, DamageInfo* dmgInfoProc, AuraEffect* triggeredByAura, SpellInfo const* /*procSpell*/, uint32 /*procFlag*/, uint32 /*procEx*/, double cooldown)
{
    uint32 damage = dmgInfoProc->GetDamage();
    SpellInfo const* hasteSpell = triggeredByAura->GetSpellInfo();

    Item* castItem = triggeredByAura->GetBase()->GetCastItemGUID() && GetTypeId() == TYPEID_PLAYER
        ? ToPlayer()->GetItemByGuid(triggeredByAura->GetBase()->GetCastItemGUID()) : NULL;

    uint32 triggered_spell_id = 0;
    Unit* target = victim;
    int32 basepoints0 = 0;

    // processed charge only counting case
    if (!triggered_spell_id)
        return true;

    SpellInfo const* triggerEntry = sSpellMgr->GetSpellInfo(triggered_spell_id);

    if (!triggerEntry)
    {
        sLog->outError(LOG_FILTER_UNITS, "Unit::HandleHasteAuraProc: Spell %u has non-existing triggered spell %u", hasteSpell->Id, triggered_spell_id);
        return false;
    }

    if (G3D::fuzzyGt(cooldown, 0.0) && GetTypeId() == TYPEID_PLAYER && ToPlayer()->HasSpellCooldown(triggered_spell_id))
        return false;

    if (basepoints0)
        CastCustomSpell(target, triggered_spell_id, &basepoints0, NULL, NULL, true, castItem, triggeredByAura);
    else
        CastSpell(target, triggered_spell_id, true, castItem, triggeredByAura);

    if (G3D::fuzzyGt(cooldown, 0.0) && GetTypeId() == TYPEID_PLAYER)
        ToPlayer()->AddSpellCooldown(triggered_spell_id, 0, getPreciseTime() + cooldown);

    return true;
}

bool Unit::HandleProcTriggerSpellCopy(Unit* victim, DamageInfo* dmgInfoProc, AuraEffect* triggeredByAura, SpellInfo const* procSpell, double cooldown)
{
    if (!procSpell || !victim)
        return false;

    for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
        if (procSpell->Effects[i].Effect)
            if (procSpell->Effects[i].HasRadius())
                return false;

    int32 procSpellId = procSpell->Id;
    int32 triggeredByAuraId = triggeredByAura->GetId();
    Player* plr = ToPlayer();
    uint32 roll = 0;

    Item* castItem = triggeredByAura->GetBase()->GetCastItemGUID() && GetTypeId() == TYPEID_PLAYER
        ? ToPlayer()->GetItemByGuid(triggeredByAura->GetBase()->GetCastItemGUID()) : NULL;

    if (G3D::fuzzyGt(cooldown, 0.0) && plr && plr->HasSpellCooldown(triggeredByAuraId))
        return false;

    if (plr)
    {
        switch (plr->GetSpecializationId(plr->GetActiveSpec()))
        {
            case SPEC_SHAMAN_ELEMENTAL:
            case SPEC_SHAMAN_RESTORATION:
                roll = 6;
                break;
            case SPEC_SHAMAN_ENHANCEMENT:
                roll = 30;
                break;
            default:
                break;
        }

        if (procSpell->Id == 117014)
            roll = 6;

        if (roll)
            if (!roll_chance_i(roll))
                return false;

        plr->RemoveSpellCooldown(procSpellId);
    }

    CastSpell(victim, procSpellId, true, castItem, triggeredByAura);

    if (G3D::fuzzyGt(cooldown, 0.0) && GetTypeId() == TYPEID_PLAYER)
        ToPlayer()->AddSpellCooldown(triggeredByAuraId, 0, getPreciseTime() + cooldown);

    return true;
}

bool Unit::HandleSpellCritChanceAuraProc(Unit* victim, DamageInfo* /*dmgInfoProc*/, AuraEffect* triggeredByAura, SpellInfo const* /*procSpell*/, uint32 /*procFlag*/, uint32 /*procEx*/, double cooldown)
{
    SpellInfo const* triggeredByAuraSpell = triggeredByAura->GetSpellInfo();

    Item* castItem = triggeredByAura->GetBase()->GetCastItemGUID() && GetTypeId() == TYPEID_PLAYER
        ? ToPlayer()->GetItemByGuid(triggeredByAura->GetBase()->GetCastItemGUID()) : NULL;

    uint32 triggered_spell_id = 0;
    Unit* target = victim;
    int32 basepoints0 = 0;

    // processed charge only counting case
    if (!triggered_spell_id)
        return true;

    SpellInfo const* triggerEntry = sSpellMgr->GetSpellInfo(triggered_spell_id);

    if (!triggerEntry)
    {
        sLog->outError(LOG_FILTER_UNITS, "Unit::HandleSpellCritChanceAuraProc: Spell %u has non-existing triggered spell %u", triggeredByAuraSpell->Id, triggered_spell_id);
        return false;
    }

    // default case
    if (!target || (target != this && !target->isAlive()))
        return false;

    if (G3D::fuzzyGt(cooldown, 0.0) && GetTypeId() == TYPEID_PLAYER && ToPlayer()->HasSpellCooldown(triggered_spell_id))
        return false;

    if (basepoints0)
        CastCustomSpell(target, triggered_spell_id, &basepoints0, NULL, NULL, true, castItem, triggeredByAura);
    else
        CastSpell(target, triggered_spell_id, true, castItem, triggeredByAura);

    if (G3D::fuzzyGt(cooldown, 0.0) && GetTypeId() == TYPEID_PLAYER)
        ToPlayer()->AddSpellCooldown(triggered_spell_id, 0, getPreciseTime() + cooldown);

    return true;
}

bool Unit::HandleAuraProcOnPowerAmount(Unit* victim, DamageInfo* /*dmgInfoProc*/, AuraEffect* triggeredByAura, SpellInfo const *procSpell, uint32 procFlag, uint32 /*procEx*/, double cooldown)
{
    int32 triggered_spell_id = triggeredByAura->GetId();

    if (G3D::fuzzyGt(cooldown, 0.0) && GetTypeId() == TYPEID_PLAYER && ToPlayer()->HasSpellCooldown(triggered_spell_id))
        return false;

    // Get triggered aura spell info
    SpellInfo const* spellProto = triggeredByAura->GetSpellInfo();
    int32 triggerAmount = triggeredByAura->GetAmount();
    Powers powerType = Powers(triggeredByAura->GetMiscValue());
    if (GetPowerIndex(powerType) == MAX_POWERS)
        return false;

    int32 powerAmount = GetPower(powerType);

    switch (spellProto->Id)
    {
        case 79577:         // Eclipse Mastery Driver Passive
        {
            if (!procSpell)
                return false;

            // forbid proc when not in balance spec or while in Celestial Alignment
            if (!HasSpell(78674) || HasAura(112071))
                return false;

            int32 powerMod = 0;
            bool isGlyph = false;
            bool aura54812 = HasAura(54812);

            bool aura48517 = HasAura(48517);
            bool aura48518 = HasAura(48518);

            bool aura67484 = HasAura(67484);
            bool aura67483 = HasAura(67483);

            
            switch(procSpell->Id)
            {
                case 2912:
                {
                    powerMod = (aura67483 || (!aura67483 && !aura67484)) ? procSpell->Effects[1].CalcValue(this) : 0;
                    break;
                }
                case 78674:
                {
                    powerMod = aura67483 ? procSpell->Effects[1].CalcValue(this) : -procSpell->Effects[1].CalcValue(this);
                    break;
                }
                case 5176:
                {
                    powerMod = (aura67484 || (!aura67483 && !aura67484)) ? -procSpell->Effects[1].CalcValue(this) : 0;
                    break;
                }
                case 99:
                case 339:
                case 5211:
                case 33786:
                case 61391:
                case 102359:
                case 102355:
                case 106707:
                case 102793:
                case 132469:
                {
                    if (aura54812)
                    {
                        if (!aura48518 && !aura48517)
                        {
                            powerMod = aura67483 ? 10 : -10;
                            isGlyph = true;
                        }
                    }
                    break;
                }
                default:
                    return false;
            }

            // while not in Eclipse State
            if (!aura48517 && !aura48518 && !isGlyph)
            {
                // search Euphoria
                if (HasAura(81062))
                    powerMod *= 2;

                // Item - Druid T12 Balance 4P Bonus
                if (HasAura(99049) && procSpell->Id != 78674)
                    powerMod += (procSpell->Id == 2912 ? 5 : -3);
            }

            // solar Eclipse Marker
            if (!aura67484 && !aura67483)
                AddAura(powerMod > 0 ? 67483 : 67484, this);

            //sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "HandleAuraProcOnPowerAmount: powerMod %i direction %i, powerAmount %i", powerMod, direction, powerAmount);
            // proc failed if wrong spell or spell direction does not match marker direction
            if (!powerMod)
                return false;

            if (powerMod > 0 && triggeredByAura->GetEffIndex() != EFFECT_0)
                return false;
            else if (powerMod < 0 && triggeredByAura->GetEffIndex() != EFFECT_1)
                return false;

            CastCustomSpell(this, 89265, &powerMod, NULL, NULL, true);
            //ModifyPower(powerType, powerMod);
            break;
        }
        default:
            break;
    }

    if (G3D::fuzzyGt(cooldown, 0.0) && GetTypeId() == TYPEID_PLAYER)
        ToPlayer()->AddSpellCooldown(triggered_spell_id, 0, getPreciseTime() + cooldown);

    return true;
}

//victim may be NULL
bool Unit::HandleDummyAuraProc(Unit* victim, DamageInfo* dmgInfoProc, AuraEffect* triggeredByAura, SpellInfo const* procSpell, uint32 procFlag, uint32 procEx, double cooldown)
{
    uint32 damage = dmgInfoProc->GetDamage();
    SpellInfo const* dummySpell = triggeredByAura->GetSpellInfo();
    uint32 effIndex = triggeredByAura->GetEffIndex();
    int32  triggerAmount = triggeredByAura->GetAmount();

    Item* castItem = triggeredByAura->GetBase()->GetCastItemGUID() && GetTypeId() == TYPEID_PLAYER
        ? ToPlayer()->GetItemByGuid(triggeredByAura->GetBase()->GetCastItemGUID()) : NULL;

    uint32 triggered_spell_id = triggeredByAura->GetTriggerSpell() ? triggeredByAura->GetTriggerSpell(): 0;
    uint32 cooldown_spell_id = 0; // for random trigger, will be one of the triggered spell to avoid repeatable triggers
                                  // otherwise, it's the triggered_spell_id by default
    Unit* target = victim;
    int32 basepoints0 = NULL;
    int32 basepoints1 = NULL;
    int32 basepoints2 = NULL;
    uint64 originalCaster = 0;
    Unit* procSpellCaster = dmgInfoProc->GetAttacker();
    uint64 procSpellCasterGUID = procSpellCaster ? procSpellCaster->GetGUID(): 0;

    switch (dummySpell->SpellFamilyName)
    {
        case SPELLFAMILY_GENERIC:
        {
            switch (dummySpell->Id)
            {
                case 139116: // Item - Attacks Proc Highest Rating
                {
                    if (Player* plr = ToPlayer())
                    {
                        switch (plr->GetSpecializationId(plr->GetActiveSpec()))
                        {
                            case SPEC_ROGUE_ASSASSINATION:
                            case SPEC_ROGUE_COMBAT:
                            case SPEC_ROGUE_SUBTLETY:
                            case SPEC_MONK_BREWMASTER:
                            case SPEC_MONK_WINDWALKER:
                            case SPEC_HUNTER_BEASTMASTER:
                            case SPEC_HUNTER_MARKSMAN:
                            case SPEC_HUNTER_SURVIVAL:
                            case SPEC_DRUID_CAT:
                            case SPEC_DRUID_BEAR:
                            case SPEC_SHAMAN_ENHANCEMENT:
                                break;
                            default:
                                return false;
                        }

                        int32 crit = plr->GetUInt32Value(PLAYER_FIELD_COMBAT_RATING_1 + CR_CRIT_MELEE);
                        int32 mastery = plr->GetUInt32Value(PLAYER_FIELD_COMBAT_RATING_1 + CR_MASTERY);
                        int32 haste = plr->GetUInt32Value(PLAYER_FIELD_COMBAT_RATING_1 + CR_HASTE_MELEE);

                        triggered_spell_id = 139117;

                        if (crit > mastery && crit > haste)
                            triggered_spell_id = 139117;
                        else if (haste > mastery && haste > crit)
                            triggered_spell_id = 139121;
                        else if (mastery > haste && mastery > crit)
                            triggered_spell_id = 139120;

                        switch (triggered_spell_id)
                        {
                            case 139117:
                            {
                                basepoints0 = (mastery + haste) * 2;
                                basepoints1 = -mastery;
                                basepoints2 = -haste;
                                break;
                            }
                            case 139120:
                            {
                                basepoints0 = (crit + haste) * 2;
                                basepoints1 = -crit;
                                basepoints2 = -haste;
                                break;
                            }
                            case 139121:
                            {
                                basepoints0 = (mastery + crit) * 2;
                                basepoints1 = -mastery;
                                basepoints2 = -crit;
                                break;
                            }
                        }
                        if (!basepoints0 && !basepoints1 && !basepoints2)
                            return false;
                    }
                    break;
                }
                case 146200: // Spirit of Chi-Ji
                {
                    if(!target)
                        return false;

                    basepoints0 = damage - (target->GetMaxHealth() - target->GetHealth());

                    if (basepoints0 > 0)
                        triggered_spell_id = 148009;
                    break;
                }
                case 146136: // Cleave
                {
                    int32 rollchance = urand(0, 10000);

                    if (rollchance > triggerAmount)
                        return false;

                    if (!damage)
                        return false;
                        
                    triggered_spell_id = 146137;
                    basepoints0 = damage;
                        
                    if (!procSpell)
                        break;

                    if (procSpell->GetSchoolMask() == SPELL_SCHOOL_MASK_NORMAL)
                        break;
                        
                    if (Player* _player = ToPlayer())
                    {
                        uint32 spec = _player->GetSpecializationId(_player->GetActiveSpec());

                        switch (_player->getClass())
                        {
                            case CLASS_MAGE:
                            {
                                if (spec == SPEC_MAGE_ARCANE)
                                    triggered_spell_id = 146166;
                                else
                                    triggered_spell_id = 146160;
                                break;
                            }
                            case CLASS_DRUID:
                            {
                                if (spec == SPEC_DRUID_BALANCE || spec == SPEC_DRUID_RESTORATION)
                                    triggered_spell_id = 146158;
                                break;
                            }
                            case CLASS_PRIEST:
                            {
                                if (spec == SPEC_PRIEST_SHADOW)
                                    triggered_spell_id = 146159;
                                else
                                    triggered_spell_id = 146157;
                                break;
                            }
                            case CLASS_PALADIN:
                            {
                                triggered_spell_id = 146157;
                                break;
                            }
                            case CLASS_WARLOCK:
                            case CLASS_DEATH_KNIGHT:
                            {
                                triggered_spell_id = 146159;
                                break;
                            }
                            case CLASS_MONK:
                            {
                                triggered_spell_id = 146172;
                                break;
                            }
                            case CLASS_SHAMAN:
                            {
                                triggered_spell_id = 146171;
                                break;
                            }
                            case CLASS_HUNTER:
                            {
                                triggered_spell_id =  146162;
                                break;
                            }
                            default:
                                break;
                        }
                    }
                    break;
                }
                case 146059: // Multistrike
                {
                    int32 rollchance = urand(0, 1000);

                    if (rollchance > triggerAmount)
                        return false;

                    if (!damage)
                        return false;
                        
                    triggered_spell_id = 146061;
                    basepoints0 = damage / 3;
                        
                    if (!procSpell)
                        break;

                    if (procSpell->GetSchoolMask() == SPELL_SCHOOL_MASK_NORMAL)
                        break;
                        
                    if (Player* _player = ToPlayer())
                    {
                        uint32 spec = _player->GetSpecializationId(_player->GetActiveSpec());

                        switch (_player->getClass())
                        {
                            case CLASS_MAGE:
                            {
                                if (spec == SPEC_MAGE_ARCANE)
                                    triggered_spell_id = 146070;
                                else
                                    triggered_spell_id = 146067;
                                break;
                            }
                            case CLASS_DRUID:
                            {
                                if (spec == SPEC_DRUID_BALANCE || spec == SPEC_DRUID_RESTORATION)
                                    triggered_spell_id = 146064;
                                break;
                            }
                            case CLASS_PRIEST:
                            {
                                if (spec == SPEC_PRIEST_SHADOW)
                                    triggered_spell_id = 146065;
                                else
                                    triggered_spell_id = 146063;
                                break;
                            }
                            case CLASS_PALADIN:
                            {
                                triggered_spell_id = 146063;
                                break;
                            }
                            case CLASS_WARLOCK:
                            case CLASS_DEATH_KNIGHT:
                            {
                                triggered_spell_id = 146065;
                                break;
                            }
                            case CLASS_MONK:
                            {
                                triggered_spell_id = 146075;
                                break;
                            }
                            case CLASS_SHAMAN:
                            {
                                triggered_spell_id = 146071;
                                break;
                            }
                            case CLASS_HUNTER:
                            {
                                triggered_spell_id =  146069;
                                break;
                            }
                            default:
                                break;
                        }
                    }
                    break;
                }
                case 148233: // Cleave Heal
                {
                    int32 rollchance = urand(0, 10000);

                    if (rollchance > triggerAmount)
                        return false;
                        
                    triggered_spell_id = 148234;
                        
                    if (!procSpell)
                        break;
                    
                    if (procSpell->GetSchoolMask() & SPELL_SCHOOL_MASK_NATURE)
                        triggered_spell_id = 148235;

                    basepoints0 = damage;
                    break;
                }
                case 146176: // Multistrike Heal
                {
                    int32 rollchance = urand(0, 1000);

                    if (rollchance > triggerAmount)
                        return false;
                        
                    triggered_spell_id = 146177;
                        
                    if (!procSpell)
                        break;
                    
                    if (procSpell->GetSchoolMask() & SPELL_SCHOOL_MASK_NATURE)
                        triggered_spell_id = 146178;

                    basepoints0 = damage / 3;
                    break;
                }
                case 104561: // Windsong
                {
                    triggered_spell_id = RAND(104423, 104510, 104509);
                    break;
                }
                case 104441: // River's Song
                {
                    triggered_spell_id = 116660;
                    break;
                }
                case 120033: // Jade Spirit (Passive)
                case 142536: // Spirit of Conquest (Passive)
                {
                    triggered_spell_id = 104993;
                    break;
                }
                case 134732: // Battle Fatigue
                {
                    if (Player* plr = ToPlayer())
                        if ((plr->InArena() || plr->InRBG()) && HasAura(134735))
                            return false;

                    if (Unit * owner = victim->GetOwner())
                        victim = owner;

                    if (victim->IsFriendlyTo(this) || victim->IsInPartyWith(this) || victim->GetTypeId() != TYPEID_PLAYER)
                        return false;

                    break;
                }
                // Concentration, Majordomo Staghelm
                case 98229:
                    SetPower(POWER_ALTERNATE_POWER, 0);
                    break;
                case 97138: // Matrix Restabilizer (391)
                case 96976: // Matrix Restabilizer (384)
                {

                    uint32 crit = this->ToPlayer()->GetUInt32Value(PLAYER_FIELD_COMBAT_RATING_1 + CR_CRIT_MELEE);
                    uint32 mastery = this->ToPlayer()->GetUInt32Value(PLAYER_FIELD_COMBAT_RATING_1 + CR_MASTERY);
                    uint32 haste = this->ToPlayer()->GetUInt32Value(PLAYER_FIELD_COMBAT_RATING_1 + CR_HASTE_MELEE);
                    
                    bool isheroic = (dummySpell->Id == 97138);
                    if (crit > mastery && crit > haste)
                        triggered_spell_id = isheroic ? 97140: 96978;
                    else if (haste > mastery && haste > crit)
                        triggered_spell_id = isheroic ? 97139: 96977;
                    else if (mastery > haste && mastery > crit)
                        triggered_spell_id = isheroic ? 97141: 96979;
                    break;
                }
                // Item - Dragon Soul - Proc - Agi Melee 1H
                case 109866:
                case 109873:
                case 107786:
                {
                    switch (dummySpell->Id)
                    {
                        case 109866:    // LFR
                            triggered_spell_id = RAND(109867, 109869, 109871);
                            break;
                        case 109873:    // Heroic
                            triggered_spell_id = RAND(109868, 109870, 109872);
                            break;
                        case 107786:
                            triggered_spell_id = RAND(107785, 107789, 107787);
                            break;
                        default:
                            break;
                    }
                    break;
                }
                case 26022:
                case 26023:
                {
                    if(!procSpell)
                        return false;
                    for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
                    {
                        switch(procSpell->GetEffect(i, GetSpawnMode())->ApplyAuraName)
                        {
                            case SPELL_AURA_MOD_FEAR:
                            case SPELL_AURA_MOD_STUN:
                            case SPELL_AURA_MOD_ROOT:
                                triggered_spell_id = 89024;
                            break;
                        }
                    }
                    break;
                }
                case 99262:
                {
                    if(target->HasAura(99252) && !HasAura(99263))
                    {
                        int32 newBp = 10;
                        newBp += triggeredByAura->GetBase()->GetStackAmount() * 5;
                        CastCustomSpell(this, 99263, &newBp, NULL, NULL, true, 0, 0, GetGUID());
                    }
                    return false;
                }
                case 99256:
                case 100230:
                case 100231:
                case 100232:
                {
                    if(target->HasAura(99263))
                        return false;

                    uint32 stack = triggeredByAura->GetBase()->GetStackAmount() / 3;
                    Aura* aura = target->GetAura(99262, target->GetGUID());
                    if (aura)
                        stack += aura->GetStackAmount();
                    else
                        aura = target->AddAura(99262, target);
                    if (aura && stack)
                    {
                        aura->SetStackAmount(stack);
                        aura->RefreshDuration();
                    }
                    return false;
                }
                // Bloodworms Health Leech
                case 50453:
                {
                    if (Unit* owner = GetOwner())
                    {
                        basepoints0 = int32(damage * 1.50f);
                        target = owner;
                        triggered_spell_id = 50454;
                        break;
                    }
                    return false;
                }
                // Eye for an Eye
                case 9799:
                case 25988:
                {
                    // return damage % to attacker but < 50% own total health
                    basepoints0 = int32(std::min(CalculatePct(damage, triggerAmount), CountPctFromMaxHealth(50)));
                    triggered_spell_id = 25997;
                    break;
                }
                // Sweeping Strikes
                case 18765:
                case 35429:
                {
                    target = SelectNearbyTarget(victim);
                    if (!target)
                        return false;

                    triggered_spell_id = 26654;
                    break;
                }
                // Unstable Power
                case 24658:
                {
                    if (!procSpell || procSpell->Id == 24659)
                        return false;
                    // Need remove one 24659 aura
                    RemoveAuraFromStack(24659);
                    return true;
                }
                // Restless Strength
                case 24661:
                {
                    // Need remove one 24662 aura
                    RemoveAuraFromStack(24662);
                    return true;
                }
                // Adaptive Warding (Frostfire Regalia set)
                case 28764:
                {
                    if (!procSpell)
                        return false;

                    // find Mage Armor
                    if (!GetAuraEffect(SPELL_AURA_MOD_MANA_REGEN_INTERRUPT, SPELLFAMILY_MAGE, 0x10000000, 0, 0))
                        return false;

                    switch (GetFirstSchoolInMask(procSpell->GetSchoolMask()))
                    {
                        case SPELL_SCHOOL_NORMAL:
                        case SPELL_SCHOOL_HOLY:
                            return false;                   // ignored
                        case SPELL_SCHOOL_FIRE:   triggered_spell_id = 28765; break;
                        case SPELL_SCHOOL_NATURE: triggered_spell_id = 28768; break;
                        case SPELL_SCHOOL_FROST:  triggered_spell_id = 28766; break;
                        case SPELL_SCHOOL_SHADOW: triggered_spell_id = 28769; break;
                        case SPELL_SCHOOL_ARCANE: triggered_spell_id = 28770; break;
                        default:
                            return false;
                    }

                    target = this;
                    break;
                }
                // Obsidian Armor (Justice Bearer`s Pauldrons shoulder)
                case 27539:
                {
                    if (!procSpell)
                        return false;

                    switch (GetFirstSchoolInMask(procSpell->GetSchoolMask()))
                    {
                        case SPELL_SCHOOL_NORMAL:
                            return false;                   // ignore
                        case SPELL_SCHOOL_HOLY:   triggered_spell_id = 27536; break;
                        case SPELL_SCHOOL_FIRE:   triggered_spell_id = 27533; break;
                        case SPELL_SCHOOL_NATURE: triggered_spell_id = 27538; break;
                        case SPELL_SCHOOL_FROST:  triggered_spell_id = 27534; break;
                        case SPELL_SCHOOL_SHADOW: triggered_spell_id = 27535; break;
                        case SPELL_SCHOOL_ARCANE: triggered_spell_id = 27540; break;
                        default:
                            return false;
                    }

                    target = this;
                    break;
                }
                // Mark of Malice
                case 33493:
                {
                    // Cast finish spell at last charge
                    if (triggeredByAura->GetBase()->GetCharges() > 1)
                        return false;

                    target = this;
                    triggered_spell_id = 33494;
                    break;
                }
                // Twisted Reflection (boss spell)
                case 21063:
                    triggered_spell_id = 21064;
                    break;
                // Vampiric Aura (boss spell)
                case 38196:
                {
                    basepoints0 = 3 * damage;               // 300%
                    if (basepoints0 < 0)
                        return false;

                    triggered_spell_id = 31285;
                    target = this;
                    break;
                }
                // Aura of Madness (Darkmoon Card: Madness trinket)
                //=====================================================
                // 39511 Sociopath: +35 strength (Paladin, Rogue, Druid, Warrior)
                // 40997 Delusional: +70 attack power (Rogue, Hunter, Paladin, Warrior, Druid)
                // 40998 Kleptomania: +35 agility (Warrior, Rogue, Paladin, Hunter, Druid)
                // 40999 Megalomania: +41 damage/healing (Druid, Shaman, Priest, Warlock, Mage, Paladin)
                // 41002 Paranoia: +35 spell/melee/ranged crit strike rating (All classes)
                // 41005 Manic: +35 haste (spell, melee and ranged) (All classes)
                // 41009 Narcissism: +35 intellect (Druid, Shaman, Priest, Warlock, Mage, Paladin, Hunter)
                // 41011 Martyr Complex: +35 stamina (All classes)
                // 41406 Dementia: Every 5 seconds either gives you +5% damage/healing. (Druid, Shaman, Priest, Warlock, Mage, Paladin)
                // 41409 Dementia: Every 5 seconds either gives you -5% damage/healing. (Druid, Shaman, Priest, Warlock, Mage, Paladin)
                case 39446:
                {
                    if (GetTypeId() != TYPEID_PLAYER || !isAlive())
                        return false;

                    // Select class defined buff
                    switch (getClass())
                    {
                        case CLASS_PALADIN:                 // 39511, 40997, 40998, 40999, 41002, 41005, 41009, 41011, 41409
                        case CLASS_DRUID:                   // 39511, 40997, 40998, 40999, 41002, 41005, 41009, 41011, 41409
                            triggered_spell_id = RAND(39511, 40997, 40998, 40999, 41002, 41005, 41009, 41011, 41409);
                            cooldown_spell_id = 39511;
                            break;
                        case CLASS_ROGUE:                   // 39511, 40997, 40998, 41002, 41005, 41011
                        case CLASS_WARRIOR:                 // 39511, 40997, 40998, 41002, 41005, 41011
                        case CLASS_DEATH_KNIGHT:
                            triggered_spell_id = RAND(39511, 40997, 40998, 41002, 41005, 41011);
                            cooldown_spell_id = 39511;
                            break;
                        case CLASS_PRIEST:                  // 40999, 41002, 41005, 41009, 41011, 41406, 41409
                        case CLASS_SHAMAN:                  // 40999, 41002, 41005, 41009, 41011, 41406, 41409
                        case CLASS_MAGE:                    // 40999, 41002, 41005, 41009, 41011, 41406, 41409
                        case CLASS_WARLOCK:                 // 40999, 41002, 41005, 41009, 41011, 41406, 41409
                            triggered_spell_id = RAND(40999, 41002, 41005, 41009, 41011, 41406, 41409);
                            cooldown_spell_id = 40999;
                            break;
                        case CLASS_HUNTER:                  // 40997, 40999, 41002, 41005, 41009, 41011, 41406, 41409
                            triggered_spell_id = RAND(40997, 40999, 41002, 41005, 41009, 41011, 41406, 41409);
                            cooldown_spell_id = 40997;
                            break;
                        default:
                            return false;
                    }

                    target = this;
                    if (roll_chance_i(10))
                        ToPlayer()->Say("This is Madness!", LANG_UNIVERSAL); // TODO: It should be moved to database, shouldn't it?
                    break;
                }
                // Sunwell Exalted Caster Neck (??? neck)
                // cast ??? Light's Wrath if Exalted by Aldor
                // cast ??? Arcane Bolt if Exalted by Scryers
                case 46569:
                    return false;                           // old unused version
                // Sunwell Exalted Caster Neck (Shattered Sun Pendant of Acumen neck)
                // cast 45479 Light's Wrath if Exalted by Aldor
                // cast 45429 Arcane Bolt if Exalted by Scryers
                case 45481:
                {
                    if (GetTypeId() != TYPEID_PLAYER)
                        return false;

                    // Get Aldor reputation rank
                    if (ToPlayer()->GetReputationRank(932) == REP_EXALTED)
                    {
                        target = this;
                        triggered_spell_id = 45479;
                        break;
                    }
                    // Get Scryers reputation rank
                    if (ToPlayer()->GetReputationRank(934) == REP_EXALTED)
                    {
                        // triggered at positive/self casts also, current attack target used then
                        if (target && IsFriendlyTo(target))
                        {
                            target = getVictim();
                            if (!target)
                            {
                                uint64 selected_guid = ToPlayer()->GetSelection();
                                target = ObjectAccessor::GetUnit(*this, selected_guid);
                                if (!target)
                                    return false;
                            }
                            if (IsFriendlyTo(target))
                                return false;
                        }

                        triggered_spell_id = 45429;
                        break;
                    }
                    return false;
                }
                // Sunwell Exalted Melee Neck (Shattered Sun Pendant of Might neck)
                // cast 45480 Light's Strength if Exalted by Aldor
                // cast 45428 Arcane Strike if Exalted by Scryers
                case 45482:
                {
                    if (GetTypeId() != TYPEID_PLAYER)
                        return false;

                    // Get Aldor reputation rank
                    if (ToPlayer()->GetReputationRank(932) == REP_EXALTED)
                    {
                        target = this;
                        triggered_spell_id = 45480;
                        break;
                    }
                    // Get Scryers reputation rank
                    if (ToPlayer()->GetReputationRank(934) == REP_EXALTED)
                    {
                        triggered_spell_id = 45428;
                        break;
                    }
                    return false;
                }
                // Sunwell Exalted Tank Neck (Shattered Sun Pendant of Resolve neck)
                // cast 45431 Arcane Insight if Exalted by Aldor
                // cast 45432 Light's Ward if Exalted by Scryers
                case 45483:
                {
                    if (GetTypeId() != TYPEID_PLAYER)
                        return false;

                    // Get Aldor reputation rank
                    if (ToPlayer()->GetReputationRank(932) == REP_EXALTED)
                    {
                        target = this;
                        triggered_spell_id = 45432;
                        break;
                    }
                    // Get Scryers reputation rank
                    if (ToPlayer()->GetReputationRank(934) == REP_EXALTED)
                    {
                        target = this;
                        triggered_spell_id = 45431;
                        break;
                    }
                    return false;
                }
                // Sunwell Exalted Healer Neck (Shattered Sun Pendant of Restoration neck)
                // cast 45478 Light's Salvation if Exalted by Aldor
                // cast 45430 Arcane Surge if Exalted by Scryers
                case 45484:
                {
                    if (GetTypeId() != TYPEID_PLAYER)
                        return false;

                    // Get Aldor reputation rank
                    if (ToPlayer()->GetReputationRank(932) == REP_EXALTED)
                    {
                        target = this;
                        triggered_spell_id = 45478;
                        break;
                    }
                    // Get Scryers reputation rank
                    if (ToPlayer()->GetReputationRank(934) == REP_EXALTED)
                    {
                        triggered_spell_id = 45430;
                        break;
                    }
                    return false;
                }
                // Living Seed
                case 48504:
                {
                    triggered_spell_id = 48503;
                    basepoints0 = triggerAmount;
                    target = this;
                    break;
                }
                // Kill command
                case 58914:
                {
                    // Remove aura stack from pet
                    RemoveAuraFromStack(58914);
                    Unit* owner = GetOwner();
                    if (!owner)
                        return true;
                    // reduce the owner's aura stack
                    owner->RemoveAuraFromStack(34027);
                    return true;
                }
                // Glyph of Scourge Strike
                case 58642:
                {
                    triggered_spell_id = 69961; // Glyph of Scourge Strike
                    break;
                }
                // Purified Shard of the Scale - Onyxia 10 Caster Trinket
                case 69755:
                {
                    triggered_spell_id = (procFlag & PROC_FLAG_DONE_SPELL_MAGIC_DMG_CLASS_POS) ? 69733 : 69729;
                    break;
                }
                // Shiny Shard of the Scale - Onyxia 25 Caster Trinket
                case 69739:
                {
                    triggered_spell_id = (procFlag & PROC_FLAG_DONE_SPELL_MAGIC_DMG_CLASS_POS) ? 69734 : 69730;
                    break;
                }
                case 71519: // Deathbringer's Will Normal
                {
                    if (GetTypeId() != TYPEID_PLAYER)
                        return false;

                    std::vector<uint32> RandomSpells;
                    switch (getClass())
                    {
                        case CLASS_WARRIOR:
                        case CLASS_PALADIN:
                        case CLASS_DEATH_KNIGHT:
                            RandomSpells.push_back(71484);
                            RandomSpells.push_back(71491);
                            RandomSpells.push_back(71492);
                            break;
                        case CLASS_SHAMAN:
                        case CLASS_ROGUE:
                            RandomSpells.push_back(71486);
                            RandomSpells.push_back(71485);
                            RandomSpells.push_back(71492);
                            break;
                        case CLASS_DRUID:
                            RandomSpells.push_back(71484);
                            RandomSpells.push_back(71485);
                            RandomSpells.push_back(71492);
                            break;
                        case CLASS_HUNTER:
                            RandomSpells.push_back(71486);
                            RandomSpells.push_back(71491);
                            RandomSpells.push_back(71485);
                            break;
                        default:
                            return false;
                    }
                    if (RandomSpells.empty()) // shouldn't happen
                        return false;

                    uint8 rand_spell = irand(0, (RandomSpells.size() - 1));
                    CastSpell(target, RandomSpells[rand_spell], true, castItem, triggeredByAura, originalCaster);
                    for (std::vector<uint32>::iterator itr = RandomSpells.begin(); itr != RandomSpells.end(); ++itr)
                    {
                        if (!ToPlayer()->HasSpellCooldown(*itr))
                            ToPlayer()->AddSpellCooldown(*itr, 0, getPreciseTime() + cooldown);
                    }
                    break;
                }
                case 71562: // Deathbringer's Will Heroic
                {
                    if (GetTypeId() != TYPEID_PLAYER)
                        return false;

                    std::vector<uint32> RandomSpells;
                    switch (getClass())
                    {
                        case CLASS_WARRIOR:
                        case CLASS_PALADIN:
                        case CLASS_DEATH_KNIGHT:
                            RandomSpells.push_back(71561);
                            RandomSpells.push_back(71559);
                            RandomSpells.push_back(71560);
                            break;
                        case CLASS_SHAMAN:
                        case CLASS_ROGUE:
                            RandomSpells.push_back(71558);
                            RandomSpells.push_back(71556);
                            RandomSpells.push_back(71560);
                            break;
                        case CLASS_DRUID:
                            RandomSpells.push_back(71561);
                            RandomSpells.push_back(71556);
                            RandomSpells.push_back(71560);
                            break;
                        case CLASS_HUNTER:
                            RandomSpells.push_back(71558);
                            RandomSpells.push_back(71559);
                            RandomSpells.push_back(71556);
                            break;
                        default:
                            return false;
                    }
                    if (RandomSpells.empty()) // shouldn't happen
                        return false;

                    uint8 rand_spell = irand(0, (RandomSpells.size() - 1));
                    CastSpell(target, RandomSpells[rand_spell], true, castItem, triggeredByAura, originalCaster);
                    for (std::vector<uint32>::iterator itr = RandomSpells.begin(); itr != RandomSpells.end(); ++itr)
                    {
                        if (!ToPlayer()->HasSpellCooldown(*itr))
                            ToPlayer()->AddSpellCooldown(*itr, 0, getPreciseTime() + cooldown);
                    }
                    break;
                }
                case 71875: // Item - Black Bruise: Necrotic Touch Proc
                case 71877:
                {
                    basepoints0 = CalculatePct(int32(damage), triggerAmount);
                    triggered_spell_id = 71879;
                    break;
                }
                // Item - Shadowmourne Legendary
                case 71903:
                {
                    if (!victim || !victim->isAlive() || HasAura(73422))  // cant collect shards while under effect of Chaos Bane buff
                        return false;

                    CastSpell(this, 71905, true, NULL, triggeredByAura);

                    // this can't be handled in AuraScript because we need to know victim
                    Aura const* dummy = GetAura(71905);
                    if (!dummy || dummy->GetStackAmount() < 10)
                        return false;

                    RemoveAurasDueToSpell(71905);
                    triggered_spell_id = 71904;
                    target = victim;
                    break;
                }
                // Shadow's Fate (Shadowmourne questline)
                case 71169:
                {
                    target = triggeredByAura->GetCaster();
                    if (!target)
                        return false;
                    Player* player = target->ToPlayer();
                    if (!player)
                        return false;
                    // not checking Infusion auras because its in targetAuraSpell of credit spell
                    if (player->GetQuestStatus(24749) == QUEST_STATUS_INCOMPLETE)       // Unholy Infusion
                    {
                        if (GetEntry() != 36678)                                        // Professor Putricide
                            return false;
                        CastSpell(target, 71518, true);                                 // Quest Credit
                        return true;
                    }
                    else if (player->GetQuestStatus(24756) == QUEST_STATUS_INCOMPLETE)  // Blood Infusion
                    {
                        if (GetEntry() != 37955)                                        // Blood-Queen Lana'thel
                            return false;
                        CastSpell(target, 72934, true);                                 // Quest Credit
                        return true;
                    }
                    else if (player->GetQuestStatus(24757) == QUEST_STATUS_INCOMPLETE)  // Frost Infusion
                    {
                        if (GetEntry() != 36853)                                        // Sindragosa
                            return false;
                        CastSpell(target, 72289, true);                                 // Quest Credit
                        return true;
                    }
                    else if (player->GetQuestStatus(24547) == QUEST_STATUS_INCOMPLETE)  // A Feast of Souls
                        triggered_spell_id = 71203;
                    break;
                }
                // Essence of the Blood Queen
                case 70871:
                {
                    basepoints0 = CalculatePct(int32(damage), triggerAmount);
                    CastCustomSpell(70872, SPELLVALUE_BASE_POINT0, basepoints0, this);
                    return true;
                }
                case 65032: // Boom aura (321 Boombot)
                {
                    if (victim->GetEntry() != 33343)   // Scrapbot
                        return false;

                    InstanceScript* instance = GetInstanceScript();
                    if (!instance)
                        return false;

                    instance->DoCastSpellOnPlayers(65037);  // Achievement criteria marker
                    break;
                }
                // Dark Hunger (The Lich King encounter)
                case 69383:
                {
                    basepoints0 = CalculatePct(int32(damage), 50);
                    triggered_spell_id = 69384;
                    break;
                }
                case 47020: // Enter vehicle XT-002 (Scrapbot)
                {
                    if (GetTypeId() != TYPEID_UNIT)
                        return false;

                    Unit* vehicleBase = GetVehicleBase();
                    if (!vehicleBase)
                        return false;

                    // Todo: Check if this amount is blizzlike
                    vehicleBase->ModifyHealth(int32(vehicleBase->CountPctFromMaxHealth(1)));
                    break;
                }
            }
            break;
        }
        case SPELLFAMILY_MAGE:
        {
            // Magic Absorption
            if (dummySpell->SpellIconID == 459)             // only this spell has SpellIconID == 459 and dummy aura
            {
                if (getPowerType() != POWER_MANA)
                    return false;

                // mana reward
                basepoints0 = CalculatePct(GetMaxPower(POWER_MANA), triggerAmount);
                target = this;
                triggered_spell_id = 29442;
                break;
            }
            // Master of Elements
            if (dummySpell->SpellIconID == 1920)
            {
                if (!procSpell)
                    return false;

                // mana cost save
                int32 cost = int32(procSpell->PowerCost + CalculatePct(GetCreateMana(), procSpell->PowerCostPercentage));
                basepoints0 = CalculatePct(cost, triggerAmount);
                if (basepoints0 <= 0)
                    return false;

                target = this;
                triggered_spell_id = 29077;
                break;
            }
            // Arcane Potency
            if (dummySpell->SpellIconID == 2120)
            {
                if (!procSpell)
                    return false;

                target = this;
                switch (dummySpell->Id)
                {
                    case 31571: triggered_spell_id = 57529; break;
                    case 31572: triggered_spell_id = 57531; break;
                    default:
                        sLog->outError(LOG_FILTER_UNITS, "Unit::HandleDummyAuraProc: non handled spell id: %u", dummySpell->Id);
                        return false;
                }
                break;
            }
            // Incanter's Regalia set (add trigger chance to Mana Shield)
            if (dummySpell->SpellFamilyFlags[0] & 0x8000)
            {
                if (GetTypeId() != TYPEID_PLAYER)
                    return false;

                target = this;
                triggered_spell_id = 37436;
                break;
            }
            switch (dummySpell->Id)
            {
                case 12846: // Mastery: Ignite
                {
                    if (effIndex != EFFECT_0)
                        return false;

                    basepoints0 = CalculatePct(damage, triggerAmount / 2);
                    triggered_spell_id = 12654;
                    break;
                }
                case 89926: // Glyph of Fire Blast
                {
                    if (target->HasAura(112948, GetGUID())) // for Frost Bomb
                    {
                        target->RemoveOwnedAura(112948);
                        CastSpell(target, 113092, true);
                    }
                    else if (target->HasAura(114923, GetGUID())) // for Nether Tempest
                    {
                        std::list<Unit*> targetList;
                        target->GetAttackableUnitListInRange(targetList, 10.0f);

                        for (std::list<Unit*>::const_iterator itr = targetList.begin(); itr != targetList.end(); ++itr)
                        {
                            if ((!this || !target) || !this->IsValidAttackTarget(*itr) || !(*itr)->IsWithinLOSInMap(this) ||
                                (*itr)->GetGUID() == this->GetGUID() || (*itr)->GetGUID() == target->GetGUID())
                                continue;

                            CastSpell(*itr, 114954, true);
                            CastSpell(*itr, 114924, true);
                            target->CastSpell(*itr, 114956, true);
                        }
                    }
                    break;
                }
                case 44448: // Pyroblast Clearcasting Driver
                {
                    for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
                        if (procSpell->Effects[i].Effect)
                            if (procSpell->Effects[i].HasRadius())
                                return false;

                    bool RemoveHeatingUp = HasAura(48107) ? true: false;

                    if (procEx & PROC_EX_CRITICAL_HIT)
                    {
                        uint32 aura = RemoveHeatingUp ? 48108: 48107;
                        CastSpell(this, aura, true);
                    }
                    if (RemoveHeatingUp)
                        RemoveAura(48107);

                    return true;
                }
                // Glyph of Polymorph
                case 56375:
                {
                    if (!target)
                        return false;
                    target->RemoveAurasByType(SPELL_AURA_PERIODIC_DAMAGE, 0, target->GetAura(32409)); // SW:D shall not be removed.
                    target->RemoveAurasByType(SPELL_AURA_PERIODIC_DAMAGE_PERCENT);
                    target->RemoveAurasByType(SPELL_AURA_PERIODIC_LEECH);
                    return true;
                }
                // Glyph of Icy Veins
                case 56374:
                {
                    RemoveAurasByType(SPELL_AURA_HASTE_SPELLS, 0, NULL, true, false);
                    RemoveAurasByType(SPELL_AURA_MOD_DECREASE_SPEED);
                    return true;
                }
                // Blessing of Ancient Kings (Val'anyr, Hammer of Ancient Kings)
                case 64411:
                {
                    if (!victim)
                        return false;
                    basepoints0 = int32(CalculatePct(damage, 15));
                    if (AuraEffect* aurEff = victim->GetAuraEffect(64413, 0, GetGUID()))
                    {
                        // The shield can grow to a maximum size of 20, 000 damage absorbtion
                        aurEff->SetAmount(std::min<int32>(aurEff->GetAmount() + basepoints0, 20000));

                        // Refresh and return to prevent replacing the aura
                        aurEff->GetBase()->RefreshDuration();
                        return true;
                    }
                    target = victim;
                    triggered_spell_id = 64413;
                    break;
                }
                // Fingers of Frost
                case 112965:
                {
                    if (!procSpell)
                        return false;

                    uint8 effIdx = triggeredByAura->GetEffIndex();
                    switch (procSpell->Id)
                    {
                        case 116:   // Frostbolt
                        case 44614: // Frostfire Bolt
                        case 84721: // Frozen Orb
                        {
                            if (effIdx != EFFECT_0)
                                return false;
                            break;
                        }
                        case 42208: // Blizzard
                        {
                            if (effIdx != EFFECT_1)
                                return false;
                            break;
                        }
                        case 2948:  // Scorch
                        {
                            if (effIdx != EFFECT_2)
                                return false;
                            break;
                        }
                        default:
                            return false;
                    }

                    if (!roll_chance_i(triggerAmount))
                        return false;

                    triggered_spell_id = 44544;
                    if (HasAura(triggered_spell_id))
                        CastSpell(this, 126084, true);  // cast second charge visual
                    break;
                }
            }
            break;
        }
        case SPELLFAMILY_WARRIOR:
        {
            switch (dummySpell->Id)
            {
                // Taste for Blood
                case 56636:
                {
                    if (cooldown && GetTypeId() == TYPEID_PLAYER && ToPlayer()->HasSpellCooldown(dummySpell->Id))
                        return false;

                    uint32 stack = 1;
                    if (procSpell && procSpell->Id == 12294 && (procEx & (PROC_EX_NORMAL_HIT|PROC_EX_CRITICAL_HIT)))
                        stack = 2;
                    else if (!(procEx & PROC_EX_DODGE))
                        return false;

                    triggered_spell_id = 60503;
                    Aura* aura = GetAura(triggered_spell_id);
                    if (!aura)
                    {
                        if(aura = AddAura(triggered_spell_id, this))
                            stack += aura->GetStackAmount() - 1;
                    }
                    else
                        stack += aura->GetStackAmount();
                    if (aura)
                    {
                        if(stack > aura->GetSpellInfo()->StackAmount)
                            stack = aura->GetSpellInfo()->StackAmount;

                        aura->SetStackAmount(stack);
                        aura->RefreshSpellMods();
                        aura->RefreshTimers();
                    }
                    if (cooldown && GetTypeId() == TYPEID_PLAYER)
                        ToPlayer()->AddSpellCooldown(dummySpell->Id, 0, time(NULL) + cooldown);
                    return true;
                }
                // Sweeping Strikes
                case 12328:
                {
                    if (!victim || !damage)
                        return false;

                    if (GetTypeId() != TYPEID_PLAYER)
                        return false;

                    if (!procSpell || procSpell->Id == 1464 || procSpell->Id == 76858 || procSpell->IsAffectingArea())
                        return false;

                    triggered_spell_id = 12723;

                    target = SelectNearbyTarget(victim, 5);

                    if (!target)
                        return false;

                    basepoints0 = CalculatePct(damage, triggerAmount);
                    break;
                }
                // Victorious
                case 32216:
                {
                    RemoveAura(dummySpell->Id);
                    return false;
                }
            }
            // Retaliation
            if (dummySpell->SpellFamilyFlags[1] & 0x8)
            {
                // check attack comes not from behind
                if (!HasInArc(M_PI, victim))
                    return false;

                triggered_spell_id = 22858;
                break;
            }
            // Second Wind
            if (dummySpell->SpellIconID == 1697)
            {
                // only for spells and hit/crit (trigger start always) and not start from self casted spells (5530 Mace Stun Effect for example)
                if (procSpell == 0 || !(procEx & (PROC_EX_NORMAL_HIT|PROC_EX_CRITICAL_HIT)) || this == victim)
                    return false;
                // Need stun or root mechanic
                if (!(procSpell->GetAllEffectsMechanicMask() & ((1<<MECHANIC_ROOT)|(1<<MECHANIC_STUN))))
                    return false;

                switch (dummySpell->Id)
                {
                    case 29838: triggered_spell_id=29842; break;
                    case 29834: triggered_spell_id=29841; break;
                    case 42770: triggered_spell_id=42771; break;
                    default:
                        sLog->outError(LOG_FILTER_UNITS, "Unit::HandleDummyAuraProc: non handled spell id: %u (SW)", dummySpell->Id);
                    return false;
                }

                target = this;
                break;
            }
            // Glyph of Sunder Armor
            if (dummySpell->Id == 58387)
            {
                if (!victim || !victim->isAlive() || !procSpell)
                    return false;

                target = SelectNearbyTarget(victim);
                if (!target)
                    return false;

                triggered_spell_id = 58567;
                break;
            }
            break;
        }
        case SPELLFAMILY_WARLOCK:
        {
            // Seed of Corruption (Mobs cast) - no die req
            if (dummySpell->SpellFamilyFlags.IsEqual(0, 0, 0) && dummySpell->SpellIconID == 1932)
            {
                // if damage is more than need deal finish spell
                if (triggeredByAura->GetAmount() <= int32(damage))
                {
                    // remember guid before aura delete
                    uint64 casterGuid = triggeredByAura->GetCasterGUID();

                    // Remove aura (before cast for prevent infinite loop handlers)
                    RemoveAurasDueToSpell(triggeredByAura->GetId());

                    // Cast finish spell (triggeredByAura already not exist!)
                    if (Unit* caster = GetUnit(*this, casterGuid))
                        caster->CastSpell(this, 32865, true, castItem);
                    return true;                            // no hidden cooldown
                }
                // Damage counting
                triggeredByAura->SetAmount(triggeredByAura->GetAmount() - damage);
                return true;
            }
            // Fel Synergy
            if (dummySpell->SpellIconID == 3222)
            {
                target = GetGuardianPet();
                if (!target)
                    return false;
                basepoints0 = CalculatePct(int32(damage), triggerAmount);
                triggered_spell_id = 54181;
                break;
            }
            switch (dummySpell->Id)
            {
                case 108370: // Soul Leech
                {
                    if (Player * warlock = ToPlayer())
                    {
                        triggered_spell_id = 108366;
                        int32 hasabsorb = 0;

                        if (Aura * aura = GetAura(triggered_spell_id))
                            hasabsorb = aura->GetEffect(EFFECT_0)->GetAmount();

                        if (warlock->GetSpecializationId(warlock->GetActiveSpec()) == SPEC_WARLOCK_AFFLICTION)
                            triggerAmount *= 2;

                        basepoints0 = CalculatePct(damage, triggerAmount) + hasabsorb;

                        if (basepoints0 > (int32)CountPctFromMaxHealth(15))
                            basepoints0 = CountPctFromMaxHealth(15);
                    } 
                    break;
                }
                case 108558: // Nightfall
                {
                    triggered_spell_id = 17941;
                    break;
                }
                case 108869: // Decimation
                {
                    if (target->GetHealthPct() < triggerAmount)
                    {
                        triggered_spell_id = 122355;
                    }
                    break;
                }
                case 108563: // Backlash
                {
                    if (GetTypeId() != TYPEID_PLAYER)
                        return false;

                    if (ToPlayer()->HasSpellCooldown(108563))
                        return false;

                    triggered_spell_id = 34936;
                    ToPlayer()->AddSpellCooldown(108563, 0, getPreciseTime() + 8.0);
                    break;
                }
                case 111397: // Blood Horror
                {
                    if (Unit* owner = target->GetOwner())
                        if (owner->GetTypeId() == TYPEID_PLAYER)
                            return false;

                    triggered_spell_id = 137143;
                    break;
                }
                // Glyph of Shadowflame
                case 63310:
                {
                    triggered_spell_id = 63311;
                    break;
                }
                // Soul Leech
                case 30293:
                case 30295:
                {
                    basepoints0 = CalculatePct(int32(damage), triggerAmount);
                    target = this;
                    triggered_spell_id = 30294;
                    // Replenishment
                    CastSpell(this, 57669, true, castItem, triggeredByAura);
                    break;
                }
                // Shadowflame (Voidheart Raiment set bonus)
                case 37377:
                {
                    triggered_spell_id = 37379;
                    break;
                }
                // Pet Healing (Corruptor Raiment or Rift Stalker Armor)
                case 37381:
                {
                    target = GetGuardianPet();
                    if (!target)
                        return false;

                    // heal amount
                    basepoints0 = CalculatePct(int32(damage), triggerAmount);
                    triggered_spell_id = 37382;
                    break;
                }
                // Shadowflame Hellfire (Voidheart Raiment set bonus)
                case 39437:
                {
                    triggered_spell_id = 37378;
                    break;
                }
                // Glyph of Succubus
                case 56250:
                {
                    if (!target)
                        return false;
                    target->RemoveAurasByType(SPELL_AURA_PERIODIC_DAMAGE, 0, target->GetAura(32409)); // SW:D shall not be removed.
                    target->RemoveAurasByType(SPELL_AURA_PERIODIC_DAMAGE_PERCENT);
                    target->RemoveAurasByType(SPELL_AURA_PERIODIC_LEECH);
                    return true;
                }
            }
            break;
        }
        case SPELLFAMILY_PRIEST:
        {
            // Vampiric Touch
            if (dummySpell->SpellFamilyFlags[1] & 0x00000400)
            {
                if (!victim || !victim->isAlive())
                    return false;

                if (effIndex != 0)
                    return false;

                // victim is caster of aura
                if (triggeredByAura->GetCasterGUID() != victim->GetGUID())
                    return false;

                // Energize 1% of max. mana
                victim->CastSpell(victim, 57669, true, castItem, triggeredByAura);
                return true;                                // no hidden cooldown
            }
            // Divine Aegis
            if (dummySpell->Id == 47515)
            {
                if (!target)
                    return false;

                basepoints0 = CalculatePct(int32(damage), triggerAmount);

                if(AuraEffect const* aurEff = target->GetAuraEffect(47753, EFFECT_0))
                {
                    basepoints0 += aurEff->GetAmount();
                    if(basepoints0 > int32(CountPctFromMaxHealth(40)))
                        basepoints0 = CountPctFromMaxHealth(40);
                }

                triggered_spell_id = 47753;
                break;
            }
            switch (dummySpell->Id)
            {
                case 118314: // Colossus
                {
                    triggered_spell_id = 116631;
                    break;
                }
                case 89489: // MoA Inner Focus
                {
                    triggered_spell_id = 96267;
                    break;
                }
                // Vampiric Embrace
                case 15286:
                {
                    if (!victim || !victim->isAlive() || procSpell->SpellFamilyFlags[1] & 0x80000)
                        return false;

                    // heal amount
                    int32 amount = CalculatePct(int32(damage), triggerAmount);
                    CastCustomSpell(this, 15290, &amount, &amount, NULL, true, castItem, triggeredByAura);
                    return true;                                // no hidden cooldown
                }
                // Priest Tier 6 Trinket (Ashtongue Talisman of Acumen)
                case 40438:
                {
                    // Shadow Word: Pain
                    if (procSpell->SpellFamilyFlags[0] & 0x8000)
                        triggered_spell_id = 40441;
                    // Renew
                    else if (procSpell->SpellFamilyFlags[0] & 0x40)
                        triggered_spell_id = 40440;
                    else
                        return false;

                    target = this;
                    break;
                }
                // Glyph of Prayer of Healing
                case 55680:
                {
                    triggered_spell_id = 56161;

                    SpellInfo const* GoPoH = sSpellMgr->GetSpellInfo(triggered_spell_id);
                    if (!GoPoH)
                        return false;

                    int32 tickcount = GoPoH->GetMaxDuration() / GoPoH->GetEffect(EFFECT_0, GetSpawnMode())->Amplitude;
                    basepoints0 = CalculatePct(int32(damage), triggerAmount) / tickcount;
                    break;
                }
                // Phantasm
                case 47569:
                case 47570:
                {
                    if (!roll_chance_i(triggerAmount))
                        return false;

                    RemoveMovementImpairingAuras();
                    break;
                }
                // Glyph of Dispel Magic
                case 55677:
                {
                    if (!target || !target->IsFriendlyTo(this))
                        return false;

                    basepoints0 = int32(target->CountPctFromMaxHealth(triggerAmount));
                    triggered_spell_id = 56131;
                    break;
                }
                // Oracle Healing Bonus ("Garments of the Oracle" set)
                case 26169:
                {
                    // heal amount
                    basepoints0 = int32(CalculatePct(damage, 10));
                    target = this;
                    triggered_spell_id = 26170;
                    break;
                }
                // Frozen Shadoweave (Shadow's Embrace set) warning! its not only priest set
                case 39372:
                {
                    if (!procSpell || (procSpell->GetSchoolMask() & (SPELL_SCHOOL_MASK_FROST | SPELL_SCHOOL_MASK_SHADOW)) == 0)
                        return false;

                    // heal amount
                    basepoints0 = CalculatePct(int32(damage), triggerAmount);
                    target = this;
                    triggered_spell_id = 39373;
                    break;
                }
                // Greater Heal (Vestments of Faith (Priest Tier 3) - 4 pieces bonus)
                case 28809:
                {
                    triggered_spell_id = 28810;
                    break;
                }
                // Priest T10 Healer 2P Bonus
                case 70770:
                    // Flash Heal
                    if (procSpell->SpellFamilyFlags[0] & 0x800)
                    {
                        triggered_spell_id = 70772;
                        SpellInfo const* blessHealing = sSpellMgr->GetSpellInfo(triggered_spell_id);
                        if (!blessHealing)
                            return false;
                        basepoints0 = int32(CalculatePct(damage, triggerAmount) / (blessHealing->GetMaxDuration() / blessHealing->GetEffect(0, GetSpawnMode())->Amplitude));
                    }
                    break;
            }
            break;
        }
        case SPELLFAMILY_DRUID:
        {
            switch (dummySpell->Id)
            {
                // Nature's Vigil
                case 124974:
                {
                    if (!procSpell)
                        return false;

                    for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
                        if (procSpell->Effects[i].Effect)
                            if (procSpell->Effects[i].HasRadius())
                                return false;

                    if (Aura* aura = GetAura(137009))
                    {
                        if (procSpell->IsPositive())
                        {
                            if (AuraEffect* eff = aura->GetEffect(EFFECT_3))
                                eff->SetAmount(eff->GetAmount() + (damage * triggerAmount / 100.0f));
                        }

                        if (AuraEffect* eff = aura->GetEffect(EFFECT_2))
                            eff->SetAmount(eff->GetAmount() + (damage * triggerAmount / 100.0f));
                    }
                    return true;
                }
                case 102351: // Cenarion Ward
                {
                    target = this;
                    break;
                }
                case 17007: // Leader of the Pack
                {
                    if (G3D::fuzzyGt(cooldown, 0.0) && GetTypeId() == TYPEID_PLAYER && ToPlayer()->HasSpellCooldown(dummySpell->Id))
                        return false;

                    cooldown_spell_id = dummySpell->Id;

                    CastSpell(this, 34299, true);
                    
                    basepoints0 = CountPctFromMaxMana(triggerAmount);
                    triggered_spell_id = 68285;
                    break;
                }
                // Sudden Eclipse
                case 46832:
                {
                    if (GetTypeId() != TYPEID_PLAYER || triggeredByAura->GetEffIndex() != EFFECT_0)
                        return false;

                    if (((procEx & PROC_EX_CRITICAL_HIT) == 0) || (procEx & PROC_EX_INTERNAL_HOT) != 0)
                        return false;

                    // ignore when in Solar and Lunar Eclipse, Celestial Alignment
                    if (HasAura(48517) || HasAura(48518)/* || HasAura(112071)*/)
                        return false;

                    triggered_spell_id = 95746;
                    basepoints0 = 20;
                    // check Lunar eclipse marker
                    if (HasAura(67484))
                        basepoints0 *= -1;
                    break;
                }
                // Glyph of Innervate
                case 54832:
                {
                    if (procSpell->SpellIconID != 62)
                        return false;

                    basepoints0 = int32(CalculatePct(GetCreatePowers(POWER_MANA), triggerAmount) / 5);
                    triggered_spell_id = 54833;
                    target = this;
                    break;
                }
                // Glyph of Starfire
                case 54845:
                {
                    triggered_spell_id = 54846;
                    break;
                }
                // Glyph of Bloodletting
                case 54815:
                {
                    if (!target)
                        return false;

                    // try to find spell Rip on the target
                    if (AuraEffect const* AurEff = target->GetAuraEffect(SPELL_AURA_PERIODIC_DAMAGE, SPELLFAMILY_DRUID, 0x00800000, 0x0, 0x0, GetGUID()))
                    {
                        // Rip's max duration, note: spells which modifies Rip's duration also counted
                        uint32 CountMin = AurEff->GetBase()->GetMaxDuration();

                        // just Rip's max duration without other spells
                        uint32 CountMax = AurEff->GetSpellInfo()->GetMaxDuration();

                        // add possible auras' and Glyph of Shred's max duration
                        CountMax += 3 * triggerAmount * IN_MILLISECONDS;      // Glyph of Bloodletting        -> +6 seconds
                        CountMax += HasAura(60141) ? 4 * IN_MILLISECONDS : 0; // Rip Duration/Lacerate Damage -> +4 seconds

                        // if min < max -> that means caster didn't cast 3 shred yet
                        // so set Rip's duration and max duration
                        if (CountMin < CountMax)
                        {
                            AurEff->GetBase()->SetDuration(AurEff->GetBase()->GetDuration() + triggerAmount * IN_MILLISECONDS);
                            AurEff->GetBase()->SetMaxDuration(CountMin + triggerAmount * IN_MILLISECONDS);
                            return true;
                        }
                    }
                    // if not found Rip
                    return false;
                }
                // Healing Touch (Dreamwalker Raiment set)
                case 28719:
                {
                    // mana back
                    basepoints0 = int32(CalculatePct(procSpell->PowerCost, 30));
                    target = this;
                    triggered_spell_id = 28742;
                    break;
                }
                // Healing Touch Refund (Idol of Longevity trinket)
                case 28847:
                {
                    target = this;
                    triggered_spell_id = 28848;
                    break;
                }
                // Mana Restore (Malorne Raiment set / Malorne Regalia set)
                case 37288:
                case 37295:
                {
                    target = this;
                    triggered_spell_id = 37238;
                    break;
                }
                // Druid Tier 6 Trinket
                case 40442:
                {
                    float  chance;

                    // Starfire
                    if (procSpell->SpellFamilyFlags[0] & 0x4)
                    {
                        triggered_spell_id = 40445;
                        chance = 25.0f;
                    }
                    // Rejuvenation
                    else if (procSpell->SpellFamilyFlags[0] & 0x10)
                    {
                        triggered_spell_id = 40446;
                        chance = 25.0f;
                    }
                    // Mangle (Bear) and Mangle (Cat)
                    else if (procSpell->SpellFamilyFlags[1] & 0x00000440)
                    {
                        triggered_spell_id = 40452;
                        chance = 40.0f;
                    }
                    else
                        return false;

                    if (!roll_chance_f(chance))
                        return false;

                    target = this;
                    break;
                }
                // Maim Interrupt
                case 44835:
                {
                    // Deadly Interrupt Effect
                    triggered_spell_id = 32747;
                    break;
                }
                // Item - Druid T10 Balance 4P Bonus
                case 70723:
                {
                    // Wrath & Starfire
                    if ((procSpell->SpellFamilyFlags[0] & 0x5) && (procEx & PROC_EX_CRITICAL_HIT))
                    {
                        triggered_spell_id = 71023;
                        SpellInfo const* triggeredSpell = sSpellMgr->GetSpellInfo(triggered_spell_id);
                        if (!triggeredSpell)
                            return false;
                        basepoints0 = CalculatePct(int32(damage), triggerAmount) / (triggeredSpell->GetMaxDuration() / triggeredSpell->GetEffect(0, GetSpawnMode())->Amplitude);
                    }
                    break;
                }
                // Item - Druid T10 Restoration 4P Bonus (Rejuvenation)
                case 70664:
                {
                    // Proc only from normal Rejuvenation
                    if (procSpell->SpellVisual[0] != 32)
                        return false;

                    Player* caster = ToPlayer();
                    if (!caster)
                        return false;
                    if (!caster->GetGroup() && victim == this)
                        return false;

                    CastCustomSpell(70691, SPELLVALUE_BASE_POINT0, damage, victim, true);
                    return true;
                }
            }
            // Living Seed
            if (dummySpell->SpellIconID == 2860)
            {
                triggered_spell_id = 48504;
                basepoints0 = CalculatePct(int32(damage), triggerAmount);
                break;
            }
            // King of the Jungle
            else if (dummySpell->SpellIconID == 2850)
            {
                // Effect 0 - mod damage while having Enrage
                if (effIndex == 0)
                {
                    if (!(procSpell->SpellFamilyFlags[0] & 0x00080000) || procSpell->SpellIconID != 961)
                        return false;
                    triggered_spell_id = 51185;
                    basepoints0 = triggerAmount;
                    target = this;
                    break;
                }
                // Effect 1 - Tiger's Fury restore energy
                else if (effIndex == 1)
                {
                    if (!(procSpell->SpellFamilyFlags[2] & 0x00000800) || procSpell->SpellIconID != 1181)
                        return false;
                    triggered_spell_id = 51178;
                    basepoints0 = triggerAmount;
                    target = this;
                    break;
                }
            }
            break;
        }
        case SPELLFAMILY_ROGUE:
        {
            switch (dummySpell->Id)
            {
                case 84654: // Bandit's Guile
                {
                    insightCount++;

                    if (insightCount > 3)
                    {
                        if       (HasAura(84745)) AddAura(84746, this);
                        else if  (HasAura(84746)) AddAura(84747, this);
                        else if (!HasAura(84747)) AddAura(84745, this);
                    }
                    else
                    {
                        if      (Aura *  GreenBuff = GetAura(84745))  GreenBuff->RefreshDuration();
                        else if (Aura * YellowBuff = GetAura(84746)) YellowBuff->RefreshDuration();
                    }
                    break;
                }
                case 51701: // Honor Among Thieves
                {
                    if (Unit* owner = (Unit *)(triggeredByAura->GetBase()->GetOwner()))
                    {
                        if (Player* rogue = owner->ToPlayer())
                        {
                            if (rogue->HasSpellCooldown(51699) || !rogue->isInCombat())
                                break;

                            if (rogue->GetComboPoints() >= 5 && owner->HasAura(114015))
                            {
                                owner->CastSpell(owner, 115189, true);
                                rogue->AddSpellCooldown(51699, NULL, getPreciseTime() + cooldown);
                                break;
                            }
                            if (rogue->GetComboTarget())
                            {
                                Unit * getComdoTarget = ObjectAccessor::GetUnit(*rogue, rogue->GetComboTarget());
                                rogue->CastSpell(getComdoTarget, 51699, true);
                                rogue->AddSpellCooldown(51699, NULL, getPreciseTime() + cooldown);
                                break;
                            }
                            if (rogue->GetSelectedUnit() && !rogue->GetSelectedUnit()->IsFriendlyTo(rogue))
                            {
                                rogue->CastSpell(rogue->GetSelectedUnit(), 51699, true);
                                rogue->AddSpellCooldown(51699, NULL, getPreciseTime() + cooldown);
                                break;
                            } 
                            if (target && !target->IsFriendlyTo(rogue))
                            {
                                rogue->CastSpell(target, 51699, true);
                                rogue->AddSpellCooldown(51699, NULL, getPreciseTime() + cooldown);
                                break;
                            }
                        }
                    }
                    break;
                }
                case 114015: // Anticipation
                {
                    if (GetTypeId() != TYPEID_PLAYER)
                        return false;

                    if (!procSpell)
                        return false;

                    if (procSpell->Id == 115190)
                        return false;

                    if(ToPlayer()->GetSelection() != target->GetGUID())
                        break;

                    if (ToPlayer()->GetComboPoints() < 5 && procSpell->Id != 27576) //Mutilate add 2 KP
                        return false;

                    if (ToPlayer()->GetComboPoints() < 4 && procSpell->Id == 27576) //Mutilate add 2 KP
                        return false;

                    CastSpell(this,115189,true);
                    return false;
                }
                // Cut to the Chase
                case 51667:
                {
                    return false;
                }
                case 57934: // Tricks of the Trade
                {
                    Unit* redirectTarget = GetMisdirectionTarget();
                    RemoveAura(57934);
                    if (!redirectTarget)
                        break;
                    CastSpell(this,59628,true);
                    CastSpell(redirectTarget,57933,true);
                    break;
                }
                case 76806:                                 // Main Gauche
                {
                    if (effIndex != EFFECT_0 || !roll_chance_i(triggerAmount))
                        return false;

                    triggered_spell_id = 86392;
                    break;
                }
            }

            switch (dummySpell->SpellIconID)
            {
                case 2963: // Deadly Brew
                {
                    triggered_spell_id = 3409;
                    break;
                }
            }
            break;
        }
        case SPELLFAMILY_HUNTER:
        {
            switch (dummySpell->SpellIconID)
            {
                // Improved Steady Shot
                case 3409:
                {
                    if (procSpell->Id != 56641) // not steady shot
                    {
                        if (!(procEx & (PROC_EX_INTERNAL_TRIGGERED | PROC_EX_INTERNAL_CANT_PROC))) // shitty procs
                            triggeredByAura->GetBase()->SetCharges(0);
                        return false;
                    }

                    // wtf bug
                    if (this == target)
                        return false;

                    if (triggeredByAura->GetBase()->GetCharges() <= 1)
                    {
                        triggeredByAura->GetBase()->SetCharges(2);
                        return true;
                    }
                    triggeredByAura->GetBase()->SetCharges(0);
                    CastSpell(this, 53220, true);
                    return true;
                }
                case 3560: // Rapid Recuperation
                {
                    // This effect only from Rapid Killing (focus regen)
                    if (!(procSpell->SpellFamilyFlags[1] & 0x01000000))
                        return false;

                    target = this;
                    triggered_spell_id = 58883;
                    basepoints0 = CalculatePct(GetMaxPower(POWER_FOCUS), triggerAmount);
                    break;
                }
            }

            switch (dummySpell->Id)
            {
                case 76659: // Mastery: Wild Quiver
                {
                    if (triggeredByAura->GetEffIndex() != EFFECT_0 || !procSpell)
                        return false;

                    if (!roll_chance_i(triggerAmount))
                        return false;

                    triggered_spell_id = 76663;
                    break;
                }
                case 82661: // Aspect of the Fox
                {
                    EnergizeBySpell(this, 82661, 2, POWER_FOCUS);
                    break;
                }
                case 34477: // Misdirection
                {
                    if (!GetMisdirectionTarget())
                        return false;
                    triggered_spell_id = 35079; // 4 sec buff on self
                    target = this;
                    break;
                }
            }
            break;
        }
        case SPELLFAMILY_PALADIN:
        {
            switch (dummySpell->Id)
            {
                case 86172: // Divine Purpose
                {
                    if (!RequiresCurrentSpellsToHolyPower(dummySpell))
                        return false;

                    if (!roll_chance_i(triggerAmount))
                        return false;

                    break;
                }
                case 54936: // Glyph of Word of Glory
                {
                    basepoints0 = triggerAmount * GetModForHolyPowerSpell();
                    triggered_spell_id = 115522;
                    break;
                }
                case 76672: // Mastery : Hand of Light
                {
                    if (effIndex != EFFECT_0)
                        return false;

                    triggered_spell_id = 96172;
                    basepoints0 = CalculatePct(dmgInfoProc->GetDamage() + dmgInfoProc->GetAbsorb(), triggerAmount);

                    if (Aura * aura = GetAura(84963))
                        if (AuraEffect * eff = aura->GetEffect(EFFECT_0))
                            basepoints0 += CalculatePct(basepoints0, eff->GetAmount());
                    break;
                }
                case 76669: // Illuminated Healing
                {
                    if (effIndex != EFFECT_0)
                        return false;

                    triggered_spell_id = 86273;
                    int32 maxAmt = GetMaxHealth() / 3;
                    if (AuraEffect const* oldEff = victim->GetAuraEffect(triggered_spell_id, EFFECT_0, GetGUID()))
                        basepoints0 = oldEff->GetAmount();

                    basepoints0 += int32(triggerAmount * damage / 100);
                    // Must not exceed 1/3 of paladin's health
                    if (basepoints0 > maxAmt)
                        basepoints0 = maxAmt;
                    break;
                }
                // Ancient Crusader (player)
                case 86701:
                {
                    if (GetTypeId() != TYPEID_PLAYER)
                        return false;

                    //if caster has no guardian of ancient kings aura then remove dummy aura
                    if (!HasAura(86698))
                    {
                        RemoveAurasDueToSpell(86701);
                        return false;
                    }

                    CastSpell(this, 86700, true);
                    return true;
                }
                // Ancient Crusader (guardian)
                case 86703:
                {
                    if (!GetOwner() || GetOwner()->GetTypeId() != TYPEID_PLAYER)
                        return false;

                    GetOwner()->CastSpell(this, 86700, true);
                    return true;
                }
                // Ancient Healer
                case 86674:
                {
                    if (GetTypeId() != TYPEID_PLAYER)
                        return false;

                    if (effIndex != 0)
                        return false;

                    // if caster has no guardian of ancient kings aura then remove dummy aura
                    if (!HasAura(86669))
                    {
                        RemoveAurasDueToSpell(86674);
                        return false;
                    }

                    // check for single target spell (TARGET_SINGLE_FRIEND, NO_TARGET)
                    if (!(procSpell->GetEffect(triggeredByAura->GetEffIndex(), GetSpawnMode())->TargetA.GetTarget() == TARGET_UNIT_TARGET_ALLY) &&
                        (procSpell->GetEffect(triggeredByAura->GetEffIndex(), GetSpawnMode())->TargetB.GetTarget() == 0))
                        return false;

                    std::list<Creature*> petlist;
                    GetCreatureListWithEntryInGrid(petlist, 46499, 100.0f);
                    if (!petlist.empty())
                    {
                        for (std::list<Creature*>::const_iterator itr = petlist.begin(); itr != petlist.end(); ++itr)
                        {
                            Unit* pPet = (*itr);
                            if (pPet->GetOwnerGUID() == GetGUID())
                            {
                                int32 bp0 = damage;
                                int32 bp1 = damage / 10;
                                pPet->CastCustomSpell(victim, 86678, &bp0, &bp1, NULL, true);
                            }
                        }
                    }

                    return true;
                }
            }
            // Seal of Command
            if (dummySpell->Id == 105361 &&  effIndex == 0)
            {
                triggered_spell_id = 118215;
                break;
            }
            // Seal of Righteousness
            if (dummySpell->Id == 20154)
            {
                triggered_spell_id = 101423;
                break;
            }
            // Light's Beacon - Beacon of Light
            if (dummySpell->Id == 53651)
            {
                // Get target of beacon of light
                if (Unit* beaconTarget = triggeredByAura->GetBase()->GetCaster())
                {
                    // do not proc when target of beacon of light is healed
                    if (!victim || beaconTarget->GetGUID() == GetGUID())
                        return false;

                    // check if it was heal by paladin which casted this beacon of light
                    if (beaconTarget->GetAura(53563, victim->GetGUID()))
                    {
                        if (beaconTarget->IsWithinLOSInMap(victim))
                        {
                            int32 percent = 0;
                            switch (procSpell->Id)
                            {
                                case 82327: // Holy Radiance
                                case 119952:// Light's Hammer
                                case 114871:// Holy Prism
                                case 85222: // Light of Dawn
                                    percent = 15; // 15% heal from these spells
                                    break;
                                case 635:   // Holy Light
                                    percent = triggerAmount * 2; // 100% heal from Holy Light
                                    break;
                                default:
                                    percent = triggerAmount; // 50% heal from all other heals
                                    break;
                            }
                            basepoints0 = CalculatePct(damage, percent);
                            victim->CastCustomSpell(beaconTarget, 53652, &basepoints0, NULL, NULL, true);
                            return true;
                        }
                    }
                }
                return false;
            }
            // Judgements of the Wise
            if (dummySpell->SpellIconID == 3017)
            {
                target = this;
                triggered_spell_id = 31930;
                break;
            }
            switch (dummySpell->Id)
            {
                // Holy Power (Redemption Armor set)
                case 28789:
                {
                    if (!victim)
                        return false;

                    // Set class defined buff
                    switch (victim->getClass())
                    {
                        case CLASS_PALADIN:
                        case CLASS_PRIEST:
                        case CLASS_SHAMAN:
                        case CLASS_DRUID:
                            triggered_spell_id = 28795;     // Increases the friendly target's mana regeneration by $s1 per 5 sec. for $d.
                            break;
                        case CLASS_MAGE:
                        case CLASS_WARLOCK:
                            triggered_spell_id = 28793;     // Increases the friendly target's spell damage and healing by up to $s1 for $d.
                            break;
                        case CLASS_HUNTER:
                        case CLASS_ROGUE:
                            triggered_spell_id = 28791;     // Increases the friendly target's attack power by $s1 for $d.
                            break;
                        case CLASS_WARRIOR:
                            triggered_spell_id = 28790;     // Increases the friendly target's armor
                            break;
                        default:
                            return false;
                    }
                    break;
                }
                // Paladin Tier 6 Trinket (Ashtongue Talisman of Zeal)
                case 40470:
                {
                    if (!procSpell)
                        return false;

                    float chance;

                    // Flash of light/Holy light
                    if (procSpell->SpellFamilyFlags[0] & 0xC0000000)
                    {
                        triggered_spell_id = 40471;
                        chance = 15.0f;
                    }
                    // Judgement (any)
                    else if (procSpell->GetSpellSpecific() == SPELL_SPECIFIC_JUDGEMENT)
                    {
                        triggered_spell_id = 40472;
                        chance = 50.0f;
                    }
                    else
                        return false;

                    if (!roll_chance_f(chance))
                        return false;

                    break;
                }
                // Item - Paladin T8 Holy 2P Bonus
                case 64890:
                {
                    triggered_spell_id = 64891;
                    basepoints0 = triggerAmount * damage / 300;
                    break;
                }
                case 71406: // Tiny Abomination in a Jar
                case 71545: // Tiny Abomination in a Jar (Heroic)
                {
                    if (!victim || !victim->isAlive())
                        return false;

                    CastSpell(this, 71432, true, NULL, triggeredByAura);

                    Aura const* dummy = GetAura(71432);
                    if (!dummy || dummy->GetStackAmount() < (dummySpell->Id == 71406 ? 8 : 7))
                        return false;

                    RemoveAurasDueToSpell(71432);
                    triggered_spell_id = 71433;  // default main hand attack
                    // roll if offhand
                    if (Player const* player = ToPlayer())
                        if (player->GetWeaponForAttack(OFF_ATTACK, true) && urand(0, 1))
                            triggered_spell_id = 71434;
                    target = victim;
                    break;
                }
                // Item - Icecrown 25 Normal Dagger Proc
                case 71880:
                {
                    switch (getPowerType())
                    {
                        case POWER_MANA:
                            triggered_spell_id = 71881;
                            break;
                        case POWER_RAGE:
                            triggered_spell_id = 71883;
                            break;
                        case POWER_ENERGY:
                            triggered_spell_id = 71882;
                            break;
                        case POWER_RUNIC_POWER:
                            triggered_spell_id = 71884;
                            break;
                        default:
                            return false;
                    }
                    break;
                }
                // Item - Icecrown 25 Heroic Dagger Proc
                case 71892:
                {
                    switch (getPowerType())
                    {
                        case POWER_MANA:
                            triggered_spell_id = 71888;
                            break;
                        case POWER_RAGE:
                            triggered_spell_id = 71886;
                            break;
                        case POWER_ENERGY:
                            triggered_spell_id = 71887;
                            break;
                        case POWER_RUNIC_POWER:
                            triggered_spell_id = 71885;
                            break;
                        default:
                            return false;
                    }
                    break;
                }
            }
            break;
        }
        case SPELLFAMILY_SHAMAN:
        {
            switch (dummySpell->Id)
            {
                case 144964:  // Flow of the Elements
                {
                    if (Player* plr = ToPlayer())
                    {
                        Item* weapon = NULL;
                        weapon = (procFlag & PROC_FLAG_DONE_OFFHAND_ATTACK) ? plr->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_MAINHAND) : plr->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_OFFHAND);

                        if (!weapon)
                            return false;

                        switch (weapon->GetEnchantmentId(TEMP_ENCHANTMENT_SLOT))
                        {
                            case 5:    triggered_spell_id = 73683; break;
                            case 2:    triggered_spell_id = 73682; break;
                            case 3021: triggered_spell_id = 73684; break;
                            case 283:  triggered_spell_id = 73681; break;
                            default:
                               break;
                        }
                    }
                    break;
                }
                case 144966:  // Item - Shaman T16 Enhancement 4P Bonus
                {
                    CastSpell(this, 144967, true);
                    CastSpell(this, 77661, true); 

                    if (Aura* aura = GetAura(77661))
                        aura->SetStackAmount(5);

                    if (Player* plr = ToPlayer())
                        plr->RemoveSpellCooldown(60103, true);
                    break;
                }
                case 145394:  // Item - Shaman T16 Restoration 4P Heal Trigger
                {
                    if(!procSpell || !victim || !procSpell->CalcCastTime())
                        return false;

                    Unit* pPet = NULL;
                    for (ControlList::const_iterator itr = m_Controlled.begin(); itr != m_Controlled.end(); ++itr) // Find Spirit Champion
                        if ((*itr)->GetEntry() == 72473)
                        {
                            pPet = *itr;
                            break;
                        }

                    if (pPet)
                        pPet->CastSpell(victim, procSpell->Id, true);
                    else
                        return false;
                    break;
                }
                case 51558:  // Ancestral Awakening
                {
                    for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
                        if (procSpell->Effects[i].Effect)
                            if (procSpell->Effects[i].HasRadius())
                                return false;
                    triggered_spell_id = 52759;
                    basepoints0 = CalculatePct(int32(damage), triggerAmount);
                    target = this;
                    break;
                }
                case 120676: // Stormlash Totem
                {
                    if (!procSpell)
                        return false;

                    if (Player * player = ToPlayer())
                    {
                        int32 AP = player->GetTotalAttackPowerValue(BASE_ATTACK);
                        int32 spellPower = player->GetSpellPowerDamage();
                        basepoints0 = (AP > spellPower) ? int32(0.2f * AP) : int32(0.3f * spellPower);
                        triggered_spell_id = 120687;
                        originalCaster = player->GetGUID();
                    }
                    break;
                }
                // Lightning Shield
                case 324:
                {
                    if (GetTypeId() != TYPEID_PLAYER)
                        return false;

                    Player* _plr = ToPlayer();

                    if (Aura* lightningShield = _plr->GetAura(324))
                    {
                        if (lightningShield->GetCharges() > 1 && !HasAura(88764))
                            lightningShield->DropCharge();

                        SpellInfo const* triggerEntry = sSpellMgr->GetSpellInfo(triggered_spell_id);
                        cooldown = triggerEntry->GetRecoveryTime() / 1000.0f;
                    }
                    break;
                }
                // Resurgence
                case 16196: // Resurgence
                {
                    if (!HasAura(52127))
                        return false;

                    triggered_spell_id = 101033;
                    SpellInfo const* triggerEntry = sSpellMgr->GetSpellInfo(triggered_spell_id);
                    float coeff = 1.0f;

                    basepoints0 = triggerEntry->GetEffect(EFFECT_0)->CalcValue(this);

                    switch (procSpell->Id)
                    {
                        case 8004: case 61295: case 73685:
                        {
                            coeff = 0.6f;
                            break;
                        }
                        case 1064:
                        {
                            coeff = 0.333f;
                            break;
                        }
                        default:
                            break;
                    }
                    basepoints0 *= coeff;

                    break;
                }
                // Totemic Power (The Earthshatterer set)
                case 28823:
                {
                    if (!victim)
                        return false;

                    // Set class defined buff
                    switch (victim->getClass())
                    {
                        case CLASS_PALADIN:
                        case CLASS_PRIEST:
                        case CLASS_SHAMAN:
                        case CLASS_DRUID:
                            triggered_spell_id = 28824;     // Increases the friendly target's mana regeneration by $s1 per 5 sec. for $d.
                            break;
                        case CLASS_MAGE:
                        case CLASS_WARLOCK:
                            triggered_spell_id = 28825;     // Increases the friendly target's spell damage and healing by up to $s1 for $d.
                            break;
                        case CLASS_HUNTER:
                        case CLASS_ROGUE:
                            triggered_spell_id = 28826;     // Increases the friendly target's attack power by $s1 for $d.
                            break;
                        case CLASS_WARRIOR:
                            triggered_spell_id = 28827;     // Increases the friendly target's armor
                            break;
                        default:
                            return false;
                    }
                    break;
                }
                // Lesser Healing Wave (Totem of Flowing Water Relic)
                case 28849:
                {
                    target = this;
                    triggered_spell_id = 28850;
                    break;
                }
                // Windfury Weapon (Passive) 1-8 Ranks
                case 33757:
                {
                    Player* player = ToPlayer();
                    if (!player || !castItem || !castItem->IsEquipped() || !victim || !victim->isAlive())
                        return false;

                    // custom cooldown processing case
                    if (G3D::fuzzyGt(cooldown, 0.0) && player->HasSpellCooldown(dummySpell->Id))
                        return false;

                    if (triggeredByAura->GetBase() && castItem->GetGUID() != triggeredByAura->GetBase()->GetCastItemGUID())
                        return false;

                    WeaponAttackType attType = WeaponAttackType(player->GetAttackBySlot(castItem->GetSlot()));
                    if ((attType != BASE_ATTACK && attType != OFF_ATTACK)
                        || (attType == BASE_ATTACK && procFlag & PROC_FLAG_DONE_OFFHAND_ATTACK)
                        || (attType == OFF_ATTACK && procFlag & PROC_FLAG_DONE_MAINHAND_ATTACK))
                         return false;

                    // Now compute real proc chance...
                    uint32 chance = 20;
                    player->ApplySpellMod(dummySpell->Id, SPELLMOD_CHANCE_OF_SUCCESS, chance);

                    Item* addWeapon = player->GetWeaponForAttack(attType == BASE_ATTACK ? OFF_ATTACK : BASE_ATTACK, true);
                    uint32 enchant_id_add = addWeapon ? addWeapon->GetEnchantmentId(EnchantmentSlot(TEMP_ENCHANTMENT_SLOT)) : 0;
                    SpellItemEnchantmentEntry const* pEnchant = sSpellItemEnchantmentStore.LookupEntry(enchant_id_add);
                    if (pEnchant && pEnchant->spellid[0] == dummySpell->Id)
                        chance += 14;

                    if (!roll_chance_i(chance))
                        return false;

                    // Now amount of extra power stored in 1 effect of Enchant spell
                    uint32 spellId = 8232;
                    SpellInfo const* windfurySpellInfo = sSpellMgr->GetSpellInfo(spellId);
                    if (!windfurySpellInfo)
                    {
                        sLog->outError(LOG_FILTER_UNITS, "Unit::HandleDummyAuraProc: non-existing spell id: %u (Windfury)", spellId);
                        return false;
                    }

                    int32 extra_attack_power = CalculateSpellDamage(victim, windfurySpellInfo, 1);

                    // Value gained from additional AP
                    basepoints0 = int32(extra_attack_power / 14.0f * GetAttackTime(attType) / 1000);

                    if (procFlag & PROC_FLAG_DONE_MAINHAND_ATTACK)
                        triggered_spell_id = 25504;

                    if (procFlag & PROC_FLAG_DONE_OFFHAND_ATTACK)
                        triggered_spell_id = 33750;

                    // apply cooldown before cast to prevent processing itself
                    if (G3D::fuzzyGt(cooldown, 0.0))
                        player->AddSpellCooldown(dummySpell->Id, 0, getPreciseTime() + cooldown);

                    if (HasAura(138141)) //Item - Shaman T15 Enhancement 4P Bonus
                        player->ModifySpellCooldown(51533, -8000);

                    // Attack Twice
                    for (uint32 i = 0; i < 3; ++i)
                        CastCustomSpell(victim, triggered_spell_id, &basepoints0, NULL, NULL, true, castItem, triggeredByAura);

                    return true;
                }
                // Shaman Tier 6 Trinket
                case 40463:
                {
                    if (!procSpell)
                        return false;

                    float chance;
                    if (procSpell->SpellFamilyFlags[0] & 0x1)
                    {
                        triggered_spell_id = 40465;         // Lightning Bolt
                        chance = 15.0f;
                    }
                    else if (procSpell->SpellFamilyFlags[0] & 0x80)
                    {
                        triggered_spell_id = 40465;         // Lesser Healing Wave
                        chance = 10.0f;
                    }
                    else if (procSpell->SpellFamilyFlags[1] & 0x00000010)
                    {
                        triggered_spell_id = 40466;         // Stormstrike
                        chance = 50.0f;
                    }
                    else
                        return false;

                    if (!roll_chance_f(chance))
                        return false;

                    target = this;
                    break;
                }
                // Glyph of Healing Wave
                case 55440:
                {
                    // Not proc from self heals
                    if (this == victim)
                        return false;
                    basepoints0 = CalculatePct(int32(damage), triggerAmount);
                    target = this;
                    triggered_spell_id = 55533;
                    break;
                }
                // Shaman T8 Elemental 4P Bonus
                case 64928:
                {
                    basepoints0 = CalculatePct(int32(damage), triggerAmount);
                    triggered_spell_id = 64930;            // Electrified
                    break;
                }
                // Shaman T9 Elemental 4P Bonus
                case 67228:
                {
                    // Lava Burst
                    if (procSpell->SpellFamilyFlags[1] & 0x1000)
                    {
                        triggered_spell_id = 71824;
                        SpellInfo const* triggeredSpell = sSpellMgr->GetSpellInfo(triggered_spell_id);
                        if (!triggeredSpell)
                            return false;
                        basepoints0 = CalculatePct(int32(damage), triggerAmount) / (triggeredSpell->GetMaxDuration() / triggeredSpell->GetEffect(0, GetSpawnMode())->Amplitude);
                    }
                    break;
                }
                // Item - Shaman T10 Restoration 4P Bonus
                case 70808:
                {
                    // Chain Heal
                    if ((procSpell->SpellFamilyFlags[0] & 0x100) && (procEx & PROC_EX_CRITICAL_HIT))
                    {
                        triggered_spell_id = 70809;
                        SpellInfo const* triggeredSpell = sSpellMgr->GetSpellInfo(triggered_spell_id);
                        if (!triggeredSpell)
                            return false;
                        basepoints0 = CalculatePct(int32(damage), triggerAmount) / (triggeredSpell->GetMaxDuration() / triggeredSpell->GetEffect(0, GetSpawnMode())->Amplitude);
                    }
                    break;
                }
                // Item - Shaman T10 Elemental 2P Bonus
                case 70811:
                {
                    // Lightning Bolt & Chain Lightning
                    if (procSpell->SpellFamilyFlags[0] & 0x3)
                    {
                        if (ToPlayer()->HasSpellCooldown(16166))
                        {
                            ToPlayer()->ModifySpellCooldown(16166, -2 * IN_MILLISECONDS);
                            return true;
                        }
                    }
                    return false;
                }
                // Item - Shaman T10 Elemental 4P Bonus
                case 70817:
                {
                    if (!target)
                        return false;
                    // try to find spell Flame Shock on the target
                    if (AuraEffect const* aurEff = target->GetAuraEffect(SPELL_AURA_PERIODIC_DAMAGE, SPELLFAMILY_SHAMAN, 0x10000000, 0x0, 0x0, GetGUID()))
                    {
                        Aura* flameShock  = aurEff->GetBase();
                        int32 maxDuration = flameShock->GetMaxDuration();
                        int32 newDuration = flameShock->GetDuration() + 2 * aurEff->GetAmplitude();

                        flameShock->SetDuration(newDuration);
                        // is it blizzlike to change max duration for FS?
                        if (newDuration > maxDuration)
                            flameShock->SetMaxDuration(newDuration);

                        return true;
                    }
                    // if not found Flame Shock
                    return false;
                }
                break;
            }
            // Frozen Power
            if (dummySpell->SpellIconID == 3780)
            {
                if (!target)
                    return false;
                if (GetDistance(target) < 15.0f)
                    return false;
                float chance = (float)triggerAmount;
                if (!roll_chance_f(chance))
                    return false;

                triggered_spell_id = 63685;
                break;
            }
            // Earth Shield
            if (dummySpell->Id == 974)
            {
                triggered_spell_id = 379;
                break;
            }
            // Flametongue Weapon (Passive)
            if (dummySpell->Id == 10400)
            {
                if (GetTypeId() != TYPEID_PLAYER  || !victim || !victim->isAlive() || !castItem || !castItem->IsEquipped())
                    return false;

                WeaponAttackType attType = WeaponAttackType(Player::GetAttackBySlot(castItem->GetSlot()));
                if ((attType != BASE_ATTACK && attType != OFF_ATTACK)
                    || (attType == BASE_ATTACK && procFlag & PROC_FLAG_DONE_OFFHAND_ATTACK)
                    || (attType == OFF_ATTACK && procFlag & PROC_FLAG_DONE_MAINHAND_ATTACK))
                    return false;

                SpellInfo const* _spellinfo = sSpellMgr->GetSpellInfo(8024);
                int32 dmg = 8625;
                float add_spellpower = GetSpellPowerDamage(SPELL_SCHOOL_MASK_FIRE) * _spellinfo->Effects[EFFECT_1].BonusMultiplier;

                // Enchant on Off-Hand and ready?
                if (castItem->GetSlot() == EQUIPMENT_SLOT_OFFHAND && procFlag & PROC_FLAG_DONE_OFFHAND_ATTACK)
                {
                    float BaseWeaponSpeed = GetAttackTime(OFF_ATTACK) / 1000.0f;
                    basepoints0 = dmg / (100 / BaseWeaponSpeed);
                    basepoints0 += add_spellpower;
                    AddPct(basepoints0, 50);
                    triggered_spell_id = 10444;
                }
                // Enchant on Main-Hand and ready?
                else if (castItem->GetSlot() == EQUIPMENT_SLOT_MAINHAND && procFlag & PROC_FLAG_DONE_MAINHAND_ATTACK)
                {
                    float BaseWeaponSpeed = GetAttackTime(BASE_ATTACK) / 1000.0f;
                    basepoints0 = dmg / (100 / BaseWeaponSpeed);
                    basepoints0 += add_spellpower;
                    AddPct(basepoints0, 50);
                    triggered_spell_id = 10444;
                }
                // If not ready, we should  return, shouldn't we?!
                else
                    return false;

                if (HasAura(131554))
                    CastSpell(target, 147732, true);
            }
            break;
        }
        case SPELLFAMILY_DEATHKNIGHT:
        {
            // Blood-Caked Blade
            if (dummySpell->SpellIconID == 138)
            {
                if (!target || !target->isAlive())
                    return false;

                triggered_spell_id = dummySpell->GetEffect(effIndex, GetSpawnMode())->TriggerSpell;
                break;
            }
            // Butchery
            if (dummySpell->SpellIconID == 2664)
            {
                basepoints0 = triggerAmount;
                triggered_spell_id = 50163;
                target = this;
                break;
            }
            // Dancing Rune Weapon
            if (dummySpell->Id == 49028)
            {
                int32 modify = 0;
                if (AuraEffect const* aurEff = GetAuraEffect(63330, 1))
                    modify = aurEff->GetAmount();

                // 1 dummy aura for dismiss rune blade
                if (effIndex != 1)
                    return false;

                Unit* pPet = NULL;
                for (ControlList::const_iterator itr = m_Controlled.begin(); itr != m_Controlled.end(); ++itr) // Find Rune Weapon
                    if ((*itr)->GetEntry() == 27893)
                    {
                        pPet = *itr;
                        break;
                    }

                if (pPet && pPet->getVictim() && damage)
                {
                    uint32 procDmg = damage / 2;
                    if(modify)
                        procDmg += int32((procDmg * modify) / 100);

                    if(procSpell) //< wtf?
                        pPet->CastSpell(pPet->getVictim(), procSpell->Id, true);
                    else
                    {
                        if (procSpell)
                            pPet->SendSpellNonMeleeDamageLog(pPet->getVictim(), procSpell->Id, procDmg, procSpell->GetSchoolMask(), 0, 0, false, 0, false);
                        pPet->DealDamage(pPet->getVictim(), procDmg);
                    }
                    break;
                }
                else
                    return false;
            }
            // Threat of Thassarian
            if (dummySpell->Id == 66192)
            {
                // Must Dual Wield
                if (!procSpell || !haveOffhandWeapon())
                    return false;

                switch (procSpell->Id)
                {
                    case 49020: triggered_spell_id = 66198; break; // Obliterate
                    case 49143: triggered_spell_id = 66196; break; // Frost Strike
                    case 45462: triggered_spell_id = 66216; break; // Plague Strike
                    case 49998: triggered_spell_id = 66188; break; // Death Strike
                    default:
                        return false;
                }
                break;
            }
            // Runic Power Back on Snare/Root
            if (dummySpell->Id == 61257)
            {
                // only for spells and hit/crit (trigger start always) and not start from self casted spells
                if (procSpell == 0 || !(procEx & (PROC_EX_NORMAL_HIT|PROC_EX_CRITICAL_HIT)) || this == victim)
                    return false;
                // Need snare or root mechanic
                if (!(procSpell->GetAllEffectsMechanicMask() & ((1<<MECHANIC_ROOT)|(1<<MECHANIC_SNARE))))
                    return false;
                triggered_spell_id = 61258;
                target = this;
                break;
            }
            break;
        }
        case SPELLFAMILY_POTION:
        {
            // alchemist's stone
            if (dummySpell->Id == 17619)
            {
                if (procSpell->SpellFamilyName == SPELLFAMILY_POTION)
                {
                    for (uint8 i = 0; i < MAX_SPELL_EFFECTS; i++)
                    {
                        if (procSpell->GetEffect(i, GetSpawnMode())->Effect == SPELL_EFFECT_HEAL)
                        {
                            triggered_spell_id = 21399;
                        }
                        else if (procSpell->GetEffect(i, GetSpawnMode())->Effect == SPELL_EFFECT_ENERGIZE)
                        {
                            triggered_spell_id = 21400;
                        }
                        else
                            continue;

                        basepoints0 = int32(CalculateSpellDamage(this, procSpell, i) * 0.4f);
                        CastCustomSpell(this, triggered_spell_id, &basepoints0, NULL, NULL, true, NULL, triggeredByAura);
                    }
                    return true;
                }
            }
            break;
        }
        case SPELLFAMILY_PET:
        {
            switch (dummySpell->SpellIconID)
            {
                // Guard Dog
                case 201:
                {
                    if (!victim)
                        return false;

                    triggered_spell_id = 54445;
                    target = this;
                    float addThreat = float(CalculatePct(procSpell->GetEffect(0, GetSpawnMode())->CalcValue(this), triggerAmount));
                    victim->AddThreat(this, addThreat);
                    break;
                }
                // Silverback
                case 1582:
                    triggered_spell_id = dummySpell->Id == 62765 ? 62801 : 62800;
                    target = this;
                    break;
            }
            break;
        }
        case SPELLFAMILY_MONK:
        {
            switch (dummySpell->Id)
            {
                case 137639: // Storm, Earth, and Fire
                {
                    for (uint8 i = 13; i < 16; ++i)
                        if (m_SummonSlot[i])
                            if (Creature* crt = GetMap()->GetCreature(m_SummonSlot[i]))
                                if (!crt->IsDespawn())
                                {
                                    if (Unit* cloneUnit = crt->ToUnit())
                                        if (cloneUnit->HasUnitState(UNIT_STATE_CASTING))
                                            continue;

                                    if (procSpell->Id != 113656 && procSpell->Id != 101546 && procSpell->Id != 116847)
                                        if (Unit* cloneTarget = crt->getVictim())
                                            if (target == cloneTarget)
                                                continue;

                                    if (Unit* cloneTarget = crt->getVictim())
                                        crt->CastSpell(cloneTarget, procSpell->Id, true);
                                }
                    return true;
                }
                case 116023: // Sparring
                {
                    if (dmgInfoProc->GetAttacker() != this)
                    {
                        if (!HasAura(116033) && !HasAura(116087) && isInFront(victim))
                        {
                            triggered_spell_id = 116033;
                            AddAura(116087, this);
                        }
                    }
                    else
                    {
                        if (HasAura(116033))
                            triggered_spell_id = 116033;
                    }
                    break;
                }
                case 124489: // Restless Pursuit
                {
                    RemoveAurasByType(SPELL_AURA_MOD_DECREASE_SPEED);
                    break;
                }
                // Afterlife
                case 116092:
                {
                    if (!victim)
                        return false;

                    if (GetTypeId() != TYPEID_PLAYER)
                        return false;

                    if (ToPlayer()->HasSpellCooldown(116092))
                        return false;

                    int32 chance = dummySpell->GetEffect(1, GetSpawnMode())->BasePoints;

                    if (!roll_chance_i(chance))
                        return false;

                    triggered_spell_id = 117032; // Healing Sphere
                    target = this;

                    if (procSpell && procSpell->Id == 100784)
                        triggered_spell_id = 121286; // Chi Sphere

                    // Prevent multiple spawn of Sphere
                    ToPlayer()->AddSpellCooldown(116092, 0, getPreciseTime() + 1.0);

                    break;
                }
            }
            break;
        }
        default:
            break;
    }

    // Misdirection
    if (dummySpell->Id == 110588)
    {
        if (!GetMisdirectionTarget())
            return false;
        triggered_spell_id = 35079; // 4 sec buff on self
        target = this;
    }

    // processed charge only counting case
    if (!triggered_spell_id)
        return true;

    SpellInfo const* triggerEntry = sSpellMgr->GetSpellInfo(triggered_spell_id);
    if (!triggerEntry)
    {
        sLog->outError(LOG_FILTER_UNITS, "Unit::HandleDummyAuraProc: Spell %u has non-existing triggered spell %u", dummySpell->Id, triggered_spell_id);
        return false;
    }

    if (cooldown_spell_id == 0)
        cooldown_spell_id = triggered_spell_id;

    if (G3D::fuzzyGt(cooldown, 0.0) && GetTypeId() == TYPEID_PLAYER && ToPlayer()->HasSpellCooldown(cooldown_spell_id))
        return false;

    if (basepoints0 || basepoints1 || basepoints2)
        CastCustomSpell(target, triggered_spell_id, &basepoints0, &basepoints1, &basepoints2, true, castItem, triggeredByAura, originalCaster);
    else
        CastSpell(target, triggered_spell_id, true, castItem, triggeredByAura, originalCaster);

    if (G3D::fuzzyGt(cooldown, 0.0) && GetTypeId() == TYPEID_PLAYER)
    {
        ToPlayer()->ApplySpellMod(cooldown_spell_id, SPELLMOD_COOLDOWN, cooldown);
        ToPlayer()->AddSpellCooldown(cooldown_spell_id, 0, getPreciseTime() + cooldown);
    }

    return true;
}

bool Unit::HandleObsModEnergyAuraProc(Unit* victim, DamageInfo* /*dmgInfoProc*/, AuraEffect* triggeredByAura, SpellInfo const* /*procSpell*/, uint32 /*procFlag*/, uint32 /*procEx*/, double cooldown)
{
    SpellInfo const* dummySpell = triggeredByAura->GetSpellInfo();
    //uint32 effIndex = triggeredByAura->GetEffIndex();
    //int32  triggerAmount = triggeredByAura->GetAmount();

    Item* castItem = triggeredByAura->GetBase()->GetCastItemGUID() && GetTypeId() == TYPEID_PLAYER
        ? ToPlayer()->GetItemByGuid(triggeredByAura->GetBase()->GetCastItemGUID()) : NULL;

    uint32 triggered_spell_id = 0;
    Unit* target = victim;
    int32 basepoints0 = 0;

    /*
    switch (dummySpell->SpellFamilyName)
    {

    }
    */
    // processed charge only counting case
    if (!triggered_spell_id)
        return true;

    SpellInfo const* triggerEntry = sSpellMgr->GetSpellInfo(triggered_spell_id);

    // Try handle unknown trigger spells
    if (!triggerEntry)
    {
        sLog->outError(LOG_FILTER_UNITS, "Unit::HandleObsModEnergyAuraProc: Spell %u has non-existing triggered spell %u", dummySpell->Id, triggered_spell_id);
        return false;
    }

    if (G3D::fuzzyGt(cooldown, 0.0) && GetTypeId() == TYPEID_PLAYER && ToPlayer()->HasSpellCooldown(triggered_spell_id))
        return false;
    if (basepoints0)
        CastCustomSpell(target, triggered_spell_id, &basepoints0, NULL, NULL, true, castItem, triggeredByAura);
    else
        CastSpell(target, triggered_spell_id, true, castItem, triggeredByAura);

    if (G3D::fuzzyGt(cooldown, 0.0) && GetTypeId() == TYPEID_PLAYER)
        ToPlayer()->AddSpellCooldown(triggered_spell_id, 0, getPreciseTime() + cooldown);
    return true;
}
bool Unit::HandleModDamagePctTakenAuraProc(Unit* victim, DamageInfo* dmgInfoProc, AuraEffect* triggeredByAura, SpellInfo const* /*procSpell*/, uint32 /*procFlag*/, uint32 procEx, double cooldown)
{
    SpellInfo const* dummySpell = triggeredByAura->GetSpellInfo();
    //uint32 effIndex = triggeredByAura->GetEffIndex();
    //int32  triggerAmount = triggeredByAura->GetAmount();

    Item* castItem = triggeredByAura->GetBase()->GetCastItemGUID() && GetTypeId() == TYPEID_PLAYER
        ? ToPlayer()->GetItemByGuid(triggeredByAura->GetBase()->GetCastItemGUID()) : NULL;

    uint32 triggered_spell_id = 0;
    Unit* target = victim;
    int32 basepoints0 = 0;
    uint32 damage = dmgInfoProc->GetDamage();

    switch (dummySpell->SpellFamilyName)
    {
        case SPELLFAMILY_MAGE:
        {
            switch (dummySpell->Id)
            {
                case 115610: // Temporal Shield
                {
                    if (procEx & PROC_EX_INTERNAL_HOT)
                        return false;

                    triggered_spell_id = 115611;
                    basepoints0 = damage / 3;
                    break;
                }
                default:
                    break;
            }
            break;
        }
    }

    // processed charge only counting case
    if (!triggered_spell_id)
        return true;

    SpellInfo const* triggerEntry = sSpellMgr->GetSpellInfo(triggered_spell_id);

    if (!triggerEntry)
    {
        sLog->outError(LOG_FILTER_UNITS, "Unit::HandleModDamagePctTakenAuraProc: Spell %u has non-existing triggered spell %u", dummySpell->Id, triggered_spell_id);
        return false;
    }

    if (G3D::fuzzyGt(cooldown, 0.0) && GetTypeId() == TYPEID_PLAYER && ToPlayer()->HasSpellCooldown(triggered_spell_id))
        return false;

    if (basepoints0)
        CastCustomSpell(target, triggered_spell_id, &basepoints0, NULL, NULL, true, castItem, triggeredByAura);
    else
        CastSpell(target, triggered_spell_id, true, castItem, triggeredByAura);

    if (G3D::fuzzyGt(cooldown, 0.0) && GetTypeId() == TYPEID_PLAYER)
        ToPlayer()->AddSpellCooldown(triggered_spell_id, 0, getPreciseTime() + cooldown);

    return true;
}

// Used in case when access to whole aura is needed
// All procs should be handled like this...
bool Unit::HandleAuraProc(Unit* victim, DamageInfo* /*dmgInfoProc*/, Aura* triggeredByAura, SpellInfo const* procSpell, uint32 /*procFlag*/, uint32 /*procEx*/, double cooldown, bool * handled)
{
    SpellInfo const* dummySpell = triggeredByAura->GetSpellInfo();

    switch (dummySpell->SpellFamilyName)
    {
        case SPELLFAMILY_GENERIC:
            switch (dummySpell->Id)
            {
                // Nevermelting Ice Crystal
                case 71564:
                    RemoveAuraFromStack(71564);
                    *handled = true;
                    break;
                // Gaseous Bloat
                case 70672:
                case 72455:
                case 72832:
                case 72833:
                {
                    *handled = true;
                    uint32 stack = triggeredByAura->GetStackAmount();
                    int32 const mod = (GetMap()->GetSpawnMode() & 1) ? 1500 : 1250;
                    int32 dmg = 0;
                    for (uint8 i = 1; i < stack; ++i)
                        dmg += mod * stack;
                    if (Unit* caster = triggeredByAura->GetCaster())
                        caster->CastCustomSpell(70701, SPELLVALUE_BASE_POINT0, dmg);
                    break;
                }
                // Ball of Flames Proc
                case 71756:
                case 72782:
                case 72783:
                case 72784:
                    RemoveAuraFromStack(dummySpell->Id);
                    *handled = true;
                    break;
                // Discerning Eye of the Beast
                case 59915:
                {
                    CastSpell(this, 59914, true);   // 59914 already has correct basepoints in DBC, no need for custom bp
                    *handled = true;
                    break;
                }
                // Swift Hand of Justice
                case 59906:
                {
                    int32 bp0 = CalculatePct(GetMaxHealth(), dummySpell->GetEffect(EFFECT_0, GetSpawnMode())-> CalcValue());
                    CastCustomSpell(this, 59913, &bp0, NULL, NULL, true);
                    *handled = true;
                    break;
                }
            }

            break;
        case SPELLFAMILY_PALADIN:
        {
            // Judgements of the Just
            if (dummySpell->SpellIconID == 3015)
            {
                *handled = true;
                CastSpell(victim, 68055, true);
                return true;
            }
            // Glyph of Divinity
            else if (dummySpell->Id == 54939)
            {
                *handled = true;
                // Check if we are the target and prevent mana gain
                if (victim && triggeredByAura->GetCasterGUID() == victim->GetGUID())
                    return false;
                // Lookup base amount mana restore
                for (uint8 i = 0; i < MAX_SPELL_EFFECTS; i++)
                {
                    if (procSpell->GetEffect(i, GetSpawnMode())->Effect == SPELL_EFFECT_ENERGIZE)
                    {
                        // value multiplied by 2 because you should get twice amount
                        int32 mana = procSpell->GetEffect(i, GetSpawnMode())->CalcValue() * 2;
                        CastCustomSpell(this, 54986, 0, &mana, NULL, true);
                    }
                }
                return true;
            }
            break;
        }
        case SPELLFAMILY_MAGE:
        {
            switch (dummySpell->Id)
            {
                // Empowered Fire
                case 31656:
                case 31657:
                case 31658:
                {
                    *handled = true;

                    SpellInfo const* spInfo = sSpellMgr->GetSpellInfo(67545);
                    if (!spInfo)
                        return false;

                    int32 bp0 = int32(CalculatePct(GetCreateMana(), spInfo->GetEffect(0, GetSpawnMode())->CalcValue()));
                    CastCustomSpell(this, 67545, &bp0, NULL, NULL, true, NULL, triggeredByAura->GetEffect(EFFECT_0), GetGUID());
                    return true;
                }
            }
            break;
        }
        case SPELLFAMILY_DEATHKNIGHT:
        {
            // Reaping
            // Blood Rites
            if (dummySpell->Id == 56835 || dummySpell->Id == 50034)
            {
                if (Player* plr = ToPlayer())
                {
                    SpellRuneCostEntry const* runeCostData = sSpellRuneCostStore.LookupEntry(procSpell->RuneCostID);
                    if (!runeCostData || (runeCostData->NoRuneCost()))
                        return false;

                    uint8 runeCost[NUM_RUNE_TYPES];
                    for (uint8 i = 0; i < NUM_RUNE_TYPES; ++i)
                        runeCost[i] = runeCostData->RuneCost[i];

                    for (uint8 i = 0; i < MAX_RUNES; ++i)
                    {
                        RuneType baseRune = plr->GetBaseRune(i);
                        if (plr->IsLastRuneUsed(i))
                        {
                            if (runeCost[baseRune])
                            {
                                plr->SetConvertIn(i, RUNE_DEATH);
                                plr->ConvertRune(i, RUNE_DEATH);
                            }
                        }
                    }
                }
                return false;
            }

            switch (dummySpell->Id)
            {
                // Bone Shield cooldown
                case 49222:
                {
                    *handled = true;
                    if (G3D::fuzzyGt(cooldown, 0.0) && GetTypeId() == TYPEID_PLAYER)
                    {
                        if (ToPlayer()->HasSpellCooldown(100000))
                            return false;
                        if (HasAura(138197)) //Item - Death Knight T15 Blood 4P Bonus
                            CastSpell(this, 138214, true);

                        ToPlayer()->AddSpellCooldown(100000, 0, getPreciseTime() + cooldown);
                    }
                    return true;
                }
                // Hungering Cold aura drop
                case 51209:
                    *handled = true;
                    // Drop only in not disease case
                    if (procSpell && procSpell->Dispel == DISPEL_DISEASE)
                        return false;
                    return true;
            }
            break;
        }
        case SPELLFAMILY_WARRIOR:
        {
            switch (dummySpell->Id)
            {
                // Item - Warrior T10 Protection 4P Bonus
                case 70844:
                {
                    int32 basepoints0 = CalculatePct(GetMaxHealth(), dummySpell->GetEffect(EFFECT_1, GetSpawnMode())-> CalcValue());
                    CastCustomSpell(this, 70845, &basepoints0, NULL, NULL, true);
                    break;
                }
                // Recklessness
                case 1719:
                {
                    //! Possible hack alert
                    //! Don't drop charges on proc, they will be dropped on SpellMod removal
                    //! Before this change, it was dropping two charges per attack, one in ProcDamageAndSpellFor, and one in RemoveSpellMods.
                    //! The reason of this behaviour is Recklessness having three auras, 2 of them can not proc (isTriggeredAura array) but the other one can, making the whole spell proc.
                    *handled = true;
                    break;
                }
                default:
                    break;
            }
            break;
        }
    }
    return false;
}

bool Unit::HandleProcTriggerSpell(Unit* victim, DamageInfo* dmgInfoProc, AuraEffect* triggeredByAura, SpellInfo const* procSpell, uint32 procFlags, uint32 procEx, double cooldown)
{
    // Get triggered aura spell info
    SpellInfo const* auraSpellInfo = triggeredByAura->GetSpellInfo();

    // Basepoints of trigger aura
    int32 triggerAmount = triggeredByAura->GetAmount();

    // Set trigger spell id, target, custom basepoints
    uint32 trigger_spell_id = auraSpellInfo->GetEffect(triggeredByAura->GetEffIndex(), GetSpawnMode())->TriggerSpell;

    Unit*  target = NULL;
    int32  basepoints0 = 0;
    uint32 damage = dmgInfoProc->GetDamage();
    uint16 stack_for_trigger = 0;

    if (triggeredByAura->GetAuraType() == SPELL_AURA_PROC_TRIGGER_SPELL_WITH_VALUE)
        basepoints0 = triggerAmount;

    Item* castItem = triggeredByAura->GetBase()->GetCastItemGUID() && GetTypeId() == TYPEID_PLAYER
        ? ToPlayer()->GetItemByGuid(triggeredByAura->GetBase()->GetCastItemGUID()) : NULL;

    // Try handle unknown trigger spells
    if (sSpellMgr->GetSpellInfo(trigger_spell_id) == NULL)
    {
        switch (auraSpellInfo->SpellFamilyName)
        {
            case SPELLFAMILY_GENERIC:
                switch (auraSpellInfo->Id)
                {
                    case 23780:             // Aegis of Preservation (Aegis of Preservation trinket)
                        trigger_spell_id = 23781;
                        break;
                    case 33896:             // Desperate Defense (Stonescythe Whelp, Stonescythe Alpha, Stonescythe Ambusher)
                        trigger_spell_id = 33898;
                        break;
                    case 43820:             // Charm of the Witch Doctor (Amani Charm of the Witch Doctor trinket)
                        // Pct value stored in dummy
                        basepoints0 = victim->GetCreateHealth() * auraSpellInfo->GetEffect(1, GetSpawnMode())->CalcValue() / 100;
                        target = victim;
                        break;
                    case 57345:             // Darkmoon Card: Greatness
                    {
                        float stat = 0.0f;
                        // strength
                        if (GetStat(STAT_STRENGTH) > stat) { trigger_spell_id = 60229;stat = GetStat(STAT_STRENGTH); }
                        // agility
                        if (GetStat(STAT_AGILITY)  > stat) { trigger_spell_id = 60233;stat = GetStat(STAT_AGILITY);  }
                        // intellect
                        if (GetStat(STAT_INTELLECT)> stat) { trigger_spell_id = 60234;stat = GetStat(STAT_INTELLECT);}
                        // spirit
                        if (GetStat(STAT_SPIRIT)   > stat) { trigger_spell_id = 60235;                               }
                        break;
                    }
                    case 64568:             // Blood Reserve
                    {
                        if (HealthBelowPctDamaged(35, damage))
                        {
                            CastCustomSpell(this, 64569, &triggerAmount, NULL, NULL, true);
                            RemoveAura(64568);
                        }
                        return false;
                    }
                    case 67702:             // Death's Choice, Item - Coliseum 25 Normal Melee Trinket
                    {
                        float stat = 0.0f;
                        // strength
                        if (GetStat(STAT_STRENGTH) > stat) { trigger_spell_id = 67708;stat = GetStat(STAT_STRENGTH); }
                        // agility
                        if (GetStat(STAT_AGILITY)  > stat) { trigger_spell_id = 67703;                               }
                        break;
                    }
                    case 67771:             // Death's Choice (heroic), Item - Coliseum 25 Heroic Melee Trinket
                    {
                        float stat = 0.0f;
                        // strength
                        if (GetStat(STAT_STRENGTH) > stat) { trigger_spell_id = 67773;stat = GetStat(STAT_STRENGTH); }
                        // agility
                        if (GetStat(STAT_AGILITY)  > stat) { trigger_spell_id = 67772;                               }
                        break;
                    }
                    // Mana Drain Trigger
                    case 27522:
                    case 40336:
                    {
                        // On successful melee or ranged attack gain $29471s1 mana and if possible drain $27526s1 mana from the target.
                        if (this && isAlive())
                            CastSpell(this, 29471, true, castItem, triggeredByAura);
                        if (victim && victim->isAlive())
                            CastSpell(victim, 27526, true, castItem, triggeredByAura);
                        return true;
                    }
                    // Evasive Maneuvers
                    case 50240:
                    {
                        // Remove a Evasive Charge
                        if(Aura* charge = GetAura(50241))
                            if (charge->ModStackAmount(-1, AURA_REMOVE_BY_ENEMY_SPELL))
                                RemoveAurasDueToSpell(50240);
                        break;
                    }
                    // Warrior - Vigilance, SPELLFAMILY_GENERIC
                    case 50720:
                    {
                        target = triggeredByAura->GetCaster();
                        if (!target)
                            return false;

                        break;
                    }
                }
            case 142598: //Apothecary: Volatile Poultice
                if (victim && victim->isAlive() && this->IsFriendlyTo(victim))
                    this->CastCustomSpell(142877, SPELLVALUE_BASE_POINT0, damage, victim, true); //Volatile Poultice
                break;
            case SPELLFAMILY_PRIEST:
            {
                switch (auraSpellInfo->Id)
                {
                    case 37594: // Greater Heal Refund
                    {
                        trigger_spell_id = 37595;
                        break;
                    }
                    case 108945: // Angelic Bulwark
                    {
                        if ((this->GetHealth() - damage) < CalculatePct(this->GetMaxHealth(), auraSpellInfo->Effects[0].BasePoints))
                        {
                            trigger_spell_id = 114214;
                            basepoints0 = CalculatePct(this->GetMaxHealth(), 20);
                            cooldown = 90.0;
                        }
                        break;
                    }
                    default:
                        break;
                }
                break;
            }
            case SPELLFAMILY_DRUID:
            {
                switch (auraSpellInfo->Id)
                {
                    // Druid Forms Trinket
                    case 37336:
                    {
                        switch (GetShapeshiftForm())
                        {
                            case FORM_NONE:     trigger_spell_id = 37344; break;
                            case FORM_CAT:      trigger_spell_id = 37341; break;
                            case FORM_BEAR:     trigger_spell_id = 37340; break;
                            case FORM_TREE:     trigger_spell_id = 37342; break;
                            case FORM_MOONKIN:  trigger_spell_id = 37343; break;
                            default:
                                return false;
                        }
                        break;
                    }
                    // Druid T9 Feral Relic (Lacerate, Swipe, Mangle, and Shred)
                    case 67353:
                    {
                        switch (GetShapeshiftForm())
                        {
                            case FORM_CAT:      trigger_spell_id = 67355; break;
                            case FORM_BEAR:     trigger_spell_id = 67354; break;
                            default:
                                return false;
                        }
                        break;
                    }
                    default:
                        break;
                }
                break;
            }
            case SPELLFAMILY_HUNTER:
            {
                if (auraSpellInfo->SpellIconID == 3247)     // Piercing Shots
                {
                    trigger_spell_id = 63468;

                    SpellInfo const* TriggerPS = sSpellMgr->GetSpellInfo(trigger_spell_id);
                    if (!TriggerPS)
                        return false;

                    basepoints0 = CalculatePct(int32(damage), triggerAmount) / (TriggerPS->GetMaxDuration() / TriggerPS->GetEffect(0, GetSpawnMode())->Amplitude);
                    break;
                }
                // Item - Hunter T9 4P Bonus
                if (auraSpellInfo->Id == 67151)
                {
                    trigger_spell_id = 68130;
                    target = this;
                    break;
                }
                break;
            }
            case SPELLFAMILY_PALADIN:
            {
                switch (auraSpellInfo->Id)
                {
                    // Healing Discount
                    case 37705:
                    {
                        trigger_spell_id = 37706;
                        target = this;
                        break;
                    }
                    // Soul Preserver
                    case 60510:
                    {
                        switch (getClass())
                        {
                            case CLASS_DRUID:
                                trigger_spell_id = 60512;
                                break;
                            case CLASS_PALADIN:
                                trigger_spell_id = 60513;
                                break;
                            case CLASS_PRIEST:
                                trigger_spell_id = 60514;
                                break;
                            case CLASS_SHAMAN:
                                trigger_spell_id = 60515;
                                break;
                        }

                        target = this;
                        break;
                    }
                    case 37657: // Lightning Capacitor
                    case 54841: // Thunder Capacitor
                    case 67712: // Item - Coliseum 25 Normal Caster Trinket
                    case 67758: // Item - Coliseum 25 Heroic Caster Trinket
                    {
                        if (!victim || !victim->isAlive() || GetTypeId() != TYPEID_PLAYER)
                            return false;

                        uint32 stack_spell_id = 0;
                        switch (auraSpellInfo->Id)
                        {
                            case 37657:
                                stack_spell_id = 37658;
                                trigger_spell_id = 37661;
                                break;
                            case 54841:
                                stack_spell_id = 54842;
                                trigger_spell_id = 54843;
                                break;
                            case 67712:
                                stack_spell_id = 67713;
                                trigger_spell_id = 67714;
                                break;
                            case 67758:
                                stack_spell_id = 67759;
                                trigger_spell_id = 67760;
                                break;
                        }

                        CastSpell(this, stack_spell_id, true, NULL, triggeredByAura);

                        Aura* dummy = GetAura(stack_spell_id);
                        if (!dummy || dummy->GetStackAmount() < triggerAmount)
                            return false;

                        RemoveAurasDueToSpell(stack_spell_id);
                        target = victim;
                        break;
                    }
                    default:
                        break;
                }
                break;
            }
            case SPELLFAMILY_SHAMAN:
            {
                switch (auraSpellInfo->Id)
                {
                    // Lightning Shield (The Ten Storms set)
                    case 23551:
                    {
                        trigger_spell_id = 23552;
                        target = victim;
                        break;
                    }
                    // Damage from Lightning Shield (The Ten Storms set)
                    case 23552:
                    {
                        trigger_spell_id = 27635;
                        break;
                    }
                    // Mana Surge (The Earthfury set)
                    case 23572:
                    {
                        if (!procSpell)
                            return false;
                        basepoints0 = int32(CalculatePct(procSpell->PowerCost, 35));
                        trigger_spell_id = 23571;
                        target = this;
                        break;
                    }
                    case 30881: // Nature's Guardian Rank 1
                    case 30883: // Nature's Guardian Rank 2
                    case 30884: // Nature's Guardian Rank 3
                    {
                        if (HealthBelowPctDamaged(30, damage))
                        {
                            basepoints0 = int32(CountPctFromMaxHealth(triggerAmount));
                            target = this;
                            trigger_spell_id = 31616;
                            if (victim && victim->isAlive())
                                victim->getThreatManager().modifyThreatPercent(this, -10);
                        }
                        else
                            return false;
                        break;
                    }
                }
                break;
            }
            case SPELLFAMILY_DEATHKNIGHT:
            {
                // Item - Death Knight T10 Melee 4P Bonus
                if (auraSpellInfo->Id == 70656)
                {
                    if (GetTypeId() != TYPEID_PLAYER || getClass() != CLASS_DEATH_KNIGHT)
                        return false;

                    for (uint8 i = 0; i < MAX_RUNES; ++i)
                        if (ToPlayer()->GetRuneCooldown(i) == 0)
                            return false;
                }
                break;
            }
            case SPELLFAMILY_ROGUE:
            {
                switch (auraSpellInfo->Id)
                {
                    // Rogue T10 2P bonus, should only proc on caster
                    case 70805:
                    {
                        if (victim != this)
                            return false;
                        break;
                    }
                    // Rogue T10 4P bonus, should proc on victim
                    case 70803:
                    {
                        target = victim;
                        break;
                    }
                }
                break;
            }
            default:
                 break;
        }
    }

    // All ok. Check current trigger spell
    SpellInfo const* triggerEntry = sSpellMgr->GetSpellInfo(trigger_spell_id);
    if (triggerEntry == NULL)
    {
        // Don't cast unknown spell
        // sLog->outError(LOG_FILTER_UNITS, "Unit::HandleProcTriggerSpell: Spell %u has 0 in EffectTriggered[%d]. Unhandled custom case?", auraSpellInfo->Id, triggeredByAura->GetEffIndex());
        if(SpellProcTriggered(victim, dmgInfoProc, triggeredByAura, procSpell, procFlags, procEx, cooldown))
            return true;
        return false;
    }

    // not allow proc extra attack spell at extra attack
    if (m_extraAttacks && triggerEntry->HasEffect(SPELL_EFFECT_ADD_EXTRA_ATTACKS))
        return false;

    // Custom requirements (not listed in procEx) Warning! damage dealing after this
    // Custom triggered spells
    switch (auraSpellInfo->Id)
    {
        case 45243: // Focused Will
        {
            if ((damage < CountPctFromMaxHealth(10) && !(procEx & PROC_EX_CRITICAL_HIT)) || ((procEx & PROC_EX_CRITICAL_HIT) && dmgInfoProc->GetDamageType() == DOT))
                return false;
            break;
        }
        case 122280: // Healing Elixirs (Talent)
        {
            trigger_spell_id = 0;

            if (HasAura(134563))
                if (GetHealth() - damage < CountPctFromMaxHealth(35))
                    trigger_spell_id = 122281;
            break;
        }
        case 134563: // Healing Elixirs
        {
            if (IsFullHealth())
                return false;
            break;
        }
        case 146199: // Spirit of Chi-Ji
        {
            for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
                if (procSpell->Effects[i].Effect)
                    if (procSpell->Effects[i].HasRadius())
                        return false;

            break;
        }
        case 146195: // Flurry of Xuen
        case 146197: // Essence of Yu'lon
        {
            if (!victim || victim->GetTypeId() == TYPEID_PLAYER)
                return false;

            if (Unit* owner = victim->GetOwner())
                if (owner->GetTypeId() == TYPEID_PLAYER)
                    return false;

            if (procSpell)
                for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
                    if (procSpell->Effects[i].Effect)
                        if (procSpell->Effects[i].HasRadius())
                            return false;
            break;
        }
        case 108215: // Paralysis
        {
            if (!victim || victim->HasAura(113953, GetGUID()))
                return false;
            break;
        }
        case 145672: // Riposte
        case 145676: // Riposte
        {
            if(Player* player = ToPlayer())
            {
                int32 CRraiting = player->GetUInt32Value(PLAYER_FIELD_COMBAT_RATING_1 + CR_PARRY) + player->GetUInt32Value(PLAYER_FIELD_COMBAT_RATING_1 + CR_DODGE);
                basepoints0 = int32(CRraiting * triggerAmount / 100);
            }
            break;
        }
        case 5118: // Aspect of the Cheetah
        {
            if (HasAura(119462) && ToPlayer())
            {
                ToPlayer()->AddSpellCooldown(5118, 0, getPreciseTime() + 4.0);
                ToPlayer()->AddSpellCooldown(13165, 0, getPreciseTime() + 4.0);
                ToPlayer()->AddSpellCooldown(13159, 0, getPreciseTime() + 4.0);
                ToPlayer()->AddSpellCooldown(82661, 0, getPreciseTime() + 4.0);
                RemoveAura(5118);
                return true;
            }
            break;
        }
        case 49509: // Scent of Blood
        {
            if (GetTypeId() != TYPEID_PLAYER)
                return false;

            if (getClass() != CLASS_DEATH_KNIGHT)
                return false;

            if (ToPlayer()->GetSpecializationId(ToPlayer()->GetActiveSpec()) != SPEC_DK_BLOOD)
                return false;

            if (!roll_chance_i(15))
                return false;

            break;
        }
        // Arcane Missiles !
        case 79684:
        {
            Player* player = ToPlayer();
            if (!player)
                return false;

            if (player->GetSpecializationId(player->GetActiveSpec()) != SPEC_MAGE_ARCANE)
                return false;

            if (!procSpell)
                return false;

            if (procSpell->Id == 4143 || procSpell->Id == 7268)
                return false;

            if (Aura* arcaneMissiles = GetAura(79683))
                CastSpell(this, 79808, true);
            break;
        }
        // Glyph of Avenging Wrath
        case 54927:
        {
            return false;
        }
        // Shooting Stars
        case 93399:
        {
            if (GetTypeId() != TYPEID_PLAYER)
                return false;

            float targetCount = 0.0f;

            for (std::set<uint64>::iterator iter = m_unitsHasCasterAura.begin(); iter != m_unitsHasCasterAura.end(); ++iter)
                if (Unit* _target = ObjectAccessor::GetUnit(*this, *iter))
                    if (_target->HasAura(8921, GetGUID()) || _target->HasAura(93402, GetGUID()))
                        targetCount++;

            int32 chance = (30.0f * pow(targetCount, 1.0f / 2.0f) / targetCount) * 100.0f;

            if (irand(0, 10000) > chance)
                return false;

            break;
        }
        // Selfless Healer
        case 85804:
        {
            if (!procSpell)
                return false;

            if (procSpell->Id != 20271)
                return false;

            break;
        }
        // Adaptation
        case 126046:
        {
            if (!procSpell)
                return false;

            if (GetTypeId() != TYPEID_PLAYER)
                return false;

            if (!(procSpell->GetAllEffectsMechanicMask() & (1 << MECHANIC_DISARM)))
                return false;

            break;
        }
        // Glyph of Blessed Life
        case 54943:
        {
            if (!procSpell)
                return false;

            if (GetTypeId() != TYPEID_PLAYER)
                return false;
            
            if (!(procSpell->GetAllEffectsMechanicMask() & ((1 << MECHANIC_ROOT) | (1 << MECHANIC_STUN) | (1 << MECHANIC_FEAR))))
                return false;

            target = this;
            break;
        }
        // Infusion of Light
        case 53576:
        {
            if (!procSpell)
                return false;

            if (GetTypeId() != TYPEID_PLAYER)
                return false;

            if (!(procSpell->Id == 25912) && !(procSpell->Id == 25914))
                return false;

            if (!(procEx & PROC_EX_CRITICAL_HIT))
                return false;

            break;
        }
        // Shadow infusion
        case 49572:
        {
            if (!procSpell)
                return false;

            if (procSpell->Id != 47632)
                return false;

            if (GetTypeId() != TYPEID_PLAYER)
                return false;

            if (Pet* pet = ToPlayer()->GetPet())
            {
                uint8 stackAmount = 0;
                if (Aura* aura = pet->GetAura(trigger_spell_id))
                    stackAmount = aura->GetStackAmount();

                if (stackAmount >= 4) // Apply Dark Transformation
                    CastSpell(this, 93426, true);
            }

            break;
        }
        // Glyph of Mind Spike
        case 33371:
        {
            if (!procSpell)
                return false;

            if (GetTypeId() != TYPEID_PLAYER)
                return false;

            if (!victim)
                return false;

            if (procSpell->Id != 73510)
                return false;

            break;
        }
        // Revenge (aura proc)
        case 5301:
        {
            if (!(procEx & PROC_EX_DODGE) && !(procEx & PROC_EX_PARRY))
                return false;

            if (GetTypeId() != TYPEID_PLAYER)
                return false;

            if (ToPlayer()->HasSpellCooldown(6572))
                ToPlayer()->RemoveSpellCooldown(6572, true);

            break;
        }
        // Meat Cleaver
        case 12950:
        {
            if (!procSpell || ! victim || GetTypeId() != TYPEID_PLAYER)
                return false;

            if (procSpell->Id != 1680 && procSpell->Id != 44949)
                return false;

            break;
        }
        // Glyph of Mind Blast
        case 87195:
        {
            if (!procSpell)
                return false;

            if (GetTypeId() != TYPEID_PLAYER)
                return false;

            if (!victim)
                return false;

            if (procSpell->Id != 8092)
                return false;

            if (!(procEx & PROC_EX_CRITICAL_HIT))
                return false;

            break;
        }
        // Twist of Fate
        case 109142:
        {
            if (!victim)
                return false;

            if (!procSpell)
                return false;

            if (victim->GetHealthPct() > 35.0f)
                return false;

            break;
        }
        case 76857:     // Mastery : Critical Block
        case 58410:     // Master Poisoner
        case 113043:    // Omen of Clarity (new)
            return false;
        // Combat Potency
        case 35551:
        {
            if (GetTypeId() != TYPEID_PLAYER || procSpell)
                return false;

            float offHandSpeed = GetAttackTime(OFF_ATTACK) / IN_MILLISECONDS;

            if (procFlags & PROC_FLAG_DONE_OFFHAND_ATTACK)
            {
                if (!roll_chance_f(20.0f * offHandSpeed / 1.4f))
                    return false;
            }
            else
                return false;
            break;
        }
        // Blindside
        case 121152:
        {
            if (!procSpell)
                return false;

            if (procSpell->Id != 5374 && procSpell->Id != 27576)
                return false;

            break;
        }
        // Teachings of the Monastery (Tiger Palm)
        case 118672:
        {
            if (!procSpell)
                return false;

            if (procSpell->Id != 100787)
                return false;

            break;
        }
        // Blazing Speed Trigger
        case 113857:
        {
            int32 health = GetMaxHealth();

            if (!(procFlags & PROC_FLAG_KILL) && int32(damage) * 100 < health * 2)
                return false;

            // check Blazing Speed cooldown
            if (Player* player = ToPlayer())
                if (player->HasSpellCooldown(108843))
                    return false;
            break;
        }
        // Backdraft
        case 117896:
        {
            if (!procSpell || (procSpell->Id != 17962 && procSpell->Id != 108685))
                return false;

            if (GetTypeId() != TYPEID_PLAYER || getClass() != CLASS_WARLOCK || ToPlayer()->GetSpecializationId(ToPlayer()->GetActiveSpec()) != SPEC_WARLOCK_DESTRUCTION)
                return false;

            break;
        }
        // Master Marksmann
        case 34487:
        {
            if (!procSpell || procSpell->Id != 56641) // Steady Shot
                return false;

            if (GetTypeId() != TYPEID_PLAYER || getClass() != CLASS_HUNTER)
                return false;

            Aura* aimed = GetAura(trigger_spell_id);
            //  After reaching 3 stacks, your next Aimed Shot's cast time and Focus cost are reduced by 100% for 10 sec
            if (aimed && aimed->GetStackAmount() >= 2)
            {
                RemoveAura(trigger_spell_id);
                CastSpell(this, 82926, true); // Fire !

                return false;
            }

            break;
        }
        // Will of the Necropolis
        case 81164:
        {
            if (GetTypeId() != TYPEID_PLAYER || getClass() != CLASS_DEATH_KNIGHT)
                return false;

            if (GetHealthPct() > 30.0f)
                return false;

            if (ToPlayer()->HasSpellCooldown(81164))
                return false;

            ToPlayer()->AddSpellCooldown(81164, 0, getPreciseTime() + 45.0);
            ToPlayer()->RemoveSpellCooldown(48982, true);

            break;
        }
        // Persistent Shield (Scarab Brooch trinket)
        // This spell originally trigger 13567 - Dummy Trigger (vs dummy efect)
        case 26467:
        {
            basepoints0 = int32(CalculatePct(damage, 15));
            target = victim;
            trigger_spell_id = 26470;
            break;
        }
        // Unyielding Knights (item exploit 29108\29109)
        case 38164:
        {
            if (!victim || victim->GetEntry() != 19457)  // Proc only if your target is Grillok
                return false;
            break;
        }
        // Deflection
        case 52420:
        {
            if (!HealthBelowPctDamaged(35, damage))
                return false;
            break;
        }

        // Cheat Death
        case 28845:
        {
            // When your health drops below 20%
            if (HealthBelowPctDamaged(20, damage) || HealthBelowPct(20))
                return false;
            break;
        }
        // Greater Heal Refund (Avatar Raiment set)
        case 37594:
        {
            if (!victim || !victim->isAlive())
                return false;

            // Doesn't proc if target already has full health
            if (victim->IsFullHealth())
                return false;
            // If your Greater Heal brings the target to full health, you gain $37595s1 mana.
            if (victim->GetHealth() + damage < victim->GetMaxHealth())
                return false;
            break;
        }
        // Bonus Healing (Crystal Spire of Karabor mace)
        case 40971:
        {
            // If your target is below $s1% health
            if (!victim || !victim->isAlive() || victim->HealthAbovePct(triggerAmount))
                return false;
            break;
        }
        // Rapid Recuperation
        case 53228:
        case 53232:
        {
            // This effect only from Rapid Fire (ability cast)
            if (!(procSpell->SpellFamilyFlags[0] & 0x20))
                return false;
            break;
        }
        // Decimation
        case 63156:
        case 63158:
            // Can proc only if target has hp below 25%
            if (!victim || !victim->HealthBelowPct(auraSpellInfo->GetEffect(EFFECT_1, GetSpawnMode())->CalcValue()))
                return false;
            break;
        // Deathbringer Saurfang - Blood Beast's Blood Link
        case 72176:
            basepoints0 = 3;
            break;
        // Professor Putricide - Ooze Spell Tank Protection
        case 71770:
            if (victim)
                victim->CastSpell(victim, trigger_spell_id, true);    // EffectImplicitTarget is self
            return true;
        case 45057: // Evasive Maneuvers (Commendation of Kael`thas trinket)
        case 71634: // Item - Icecrown 25 Normal Tank Trinket 1
        case 71640: // Item - Icecrown 25 Heroic Tank Trinket 1
        case 75475: // Item - Chamber of Aspects 25 Normal Tank Trinket
        case 75481: // Item - Chamber of Aspects 25 Heroic Tank Trinket
        {
            // Procs only if damage takes health below $s1%
            if (!HealthBelowPctDamaged(triggerAmount, damage))
                return false;
            break;
        }
        case 143574: //Swelling corruption (Immerseus HM)
            {
                if (ToCreature())
                {
                    CastSpell(victim, trigger_spell_id, true);
                    CastSpell(victim, 143581, true);
                    RemoveAuraFromStack(143574);
                    return true;
                }
            }
        default:
            break;
    }


    // Custom basepoints/target for exist spell
    // dummy basepoints or other customs
    switch (trigger_spell_id)
    {
        case 144999: // Всплеск стихий
        case 145180: // Empowered Shadows
        {
            basepoints0 = triggerAmount;
            break;
        }
        // Auras which should proc on area aura source (caster in this case):
        // Cast positive spell on enemy target
        case 7099:  // Curse of Mending
        case 39703: // Curse of Mending
        case 29494: // Temptation
        case 20233: // Improved Lay on Hands (cast on target)
        {
            target = victim;
            break;
        }
        case 146310: // Restless Agility
        case 146317: // Restless Spirit
        {
            target = this;
            stack_for_trigger = triggerEntry->StackAmount;
            break;
        }
        // Finish movies that add combo
        case 14189: // Seal Fate (Netherblade set)
        {
            if (!victim || victim == this)
                return false;
            if (HasAura(114015) && ToPlayer()->GetComboPoints() >= 5)
            {
                CastSpell(this,115189,true);
                return false;
            }
            // Need add combopoint AFTER finish movie (or they dropped in finish phase)
            break;
        }
        // Item - Druid T10 Balance 2P Bonus
        case 16870:
        {
            if (HasAura(70718))
                CastSpell(this, 70721, true);
            break;
        }
        // Enlightenment (trigger only from mana cost spells)
        case 35095:
        {
            if (!procSpell || procSpell->PowerType != POWER_MANA || (procSpell->PowerCost == 0 && procSpell->PowerCostPercentage == 0))
                return false;
            break;
        }
        case 46916:  // Slam! (Bloodsurge proc)
        case 52437:  // Sudden Death
        {
            // Item - Warrior T10 Melee 4P Bonus
            if (AuraEffect const* aurEff = GetAuraEffect(70847, 0))
            {
                if (!roll_chance_i(aurEff->GetAmount()))
                    break;
                CastSpell(this, 70849, true, castItem, triggeredByAura); // Extra Charge!
                CastSpell(this, 71072, true, castItem, triggeredByAura); // Slam GCD Reduced
                CastSpell(this, 71069, true, castItem, triggeredByAura); // Execute GCD Reduced
            }
            break;
        }
        // Sword and Board
        case 50227:
        {
            // Remove cooldown on Shield Slam
            if (GetTypeId() == TYPEID_PLAYER)
                ToPlayer()->RemoveSpellCategoryCooldown(1209, true);
            break;
        }
        // Maelstrom Weapon
        case 53817:
        {
            // Item - Shaman T10 Enhancement 4P Bonus
            if (AuraEffect const* aurEff = GetAuraEffect(70832, 0))
                if (Aura const* maelstrom = GetAura(53817))
                    if ((maelstrom->GetStackAmount() == maelstrom->GetSpellInfo()->StackAmount - 1) && roll_chance_i(aurEff->GetAmount()))
                        CastSpell(this, 70831, true, castItem, triggeredByAura);

            // Full Maelstrom Visual
            if (Aura const* maelstrom = GetAura(53817))
                if (maelstrom->GetStackAmount() >= 4)
                    CastSpell(this, 60349, true);

            break;
        }
        // Glyph of Death's Embrace
        case 58679:
        {
            // Proc only from healing part of Death Coil. Check is essential as all Death Coil spells have 0x2000 mask in SpellFamilyFlags
            if (!procSpell || !(procSpell->SpellFamilyName == SPELLFAMILY_DEATHKNIGHT && procSpell->SpellFamilyFlags[0] == 0x80002000))
                return false;
            break;
        }
        // Glyph of Death Grip
        case 58628:
        {
            // remove cooldown of Death Grip
            if (GetTypeId() == TYPEID_PLAYER)
                ToPlayer()->RemoveSpellCooldown(49576, true);
            return true;
        }
        // Savage Defense
        case 62606:
        {
            basepoints0 = CalculatePct(triggerAmount, GetTotalAttackPowerValue(BASE_ATTACK));
            break;
        }
        // Culling the Herd
        case 70893:
        {
            // check if we're doing a critical hit
            if (!(procSpell->SpellFamilyFlags[1] & 0x10000000) && (procEx != PROC_EX_CRITICAL_HIT))
                return false;
            // check if we're procced by Claw, Bite or Smack (need to use the spell icon ID to detect it)
            if (!(procSpell->SpellIconID == 262 || procSpell->SpellIconID == 1680 || procSpell->SpellIconID == 473))
                return false;
            break;
        }
        // Shadow's Fate (Shadowmourne questline)
        case 71169:
        {
            if (!victim || GetTypeId() != TYPEID_PLAYER)
                return false;

            Player* player = ToPlayer();
            if (player->GetQuestStatus(24749) == QUEST_STATUS_INCOMPLETE)       // Unholy Infusion
            {
                if (!player->HasAura(71516) || victim->GetEntry() != 36678)    // Shadow Infusion && Professor Putricide
                    return false;
            }
            else if (player->GetQuestStatus(24756) == QUEST_STATUS_INCOMPLETE)  // Blood Infusion
            {
                if (!player->HasAura(72154) || victim->GetEntry() != 37955)    // Thirst Quenched && Blood-Queen Lana'thel
                    return false;
            }
            else if (player->GetQuestStatus(24757) == QUEST_STATUS_INCOMPLETE)  // Frost Infusion
            {
                if (!player->HasAura(72290) || victim->GetEntry() != 36853)    // Frost-Imbued Blade && Sindragosa
                    return false;
            }
            else if (player->GetQuestStatus(24547) != QUEST_STATUS_INCOMPLETE)  // A Feast of Souls
                return false;

            if (victim->GetTypeId() != TYPEID_UNIT)
                return false;
            // critters are not allowed
            if (victim->GetCreatureType() == CREATURE_TYPE_CRITTER)
                return false;
            break;
        }
        // Death's Advance
        case 96268:
        {
            if (!ToPlayer())
                return false;
            if (!ToPlayer()->GetRuneCooldown(RUNE_UNHOLY*2) || !ToPlayer()->GetRuneCooldown(RUNE_UNHOLY*2+1))
                return false;
            break;
        }
        // Primal Fury
        case 16959:
        {
            if (procSpell)
            {
                if (procSpell->HasEffect(SPELL_EFFECT_ADD_COMBO_POINTS) || procSpell->Id == 33876)
                {
                    if(m_movedPlayer)
                        if (Unit* targetCP = m_movedPlayer->GetSelectedUnit())
                            if(targetCP == victim)
                                CastSpell(victim, 16953, true);
                }
            }
            break;
        }
        //Generate Rage - General Nazgrim[SO]
        case 144278:
        {
            if (victim->ToPlayer() && !victim->HasAura(143494)) //Sundering Blow
            {
                CastSpell(this, 143597, true); //Generate Rage - Energize
                return true;
            }
            break;
        }
    }

    if (G3D::fuzzyGt(cooldown, 0.0) && GetTypeId() == TYPEID_PLAYER && ToPlayer()->HasSpellCooldown(trigger_spell_id))
        return false;

    // try detect target manually if not set
    if (target == NULL)
        target = !(procFlags & (PROC_FLAG_DONE_SPELL_MAGIC_DMG_CLASS_POS | PROC_FLAG_DONE_SPELL_NONE_DMG_CLASS_POS)) && triggerEntry && triggerEntry->IsPositive() ? this : victim;

    if (basepoints0 && triggeredByAura->GetAuraType() == SPELL_AURA_PROC_TRIGGER_SPELL_WITH_VALUE)
        CastCustomSpell(target, trigger_spell_id, &basepoints0, &basepoints0, &basepoints0, true, castItem, triggeredByAura);
    else if (basepoints0)
        CastCustomSpell(target, trigger_spell_id, &basepoints0, NULL, NULL, true, castItem, triggeredByAura);
    else if (stack_for_trigger)
        AddAura(trigger_spell_id, target, castItem, stack_for_trigger);
    else
        CastSpell(target, trigger_spell_id, true, castItem, triggeredByAura);

    if (G3D::fuzzyGt(cooldown, 0.0) && GetTypeId() == TYPEID_PLAYER)
        ToPlayer()->AddSpellCooldown(trigger_spell_id, 0, getPreciseTime() + cooldown);

    return true;
}

bool Unit::HandleProcMelleTriggerSpell(Unit* victim, DamageInfo* dmgInfoProc, AuraEffect* triggeredByAura, SpellInfo const* procSpell, uint32 procFlags, uint32 procEx, double cooldown)
{
    // Get triggered aura spell info
    SpellInfo const* auraSpellInfo = triggeredByAura->GetSpellInfo();

    // Set trigger spell id, target, custom basepoints
    uint32 trigger_spell_id = auraSpellInfo->GetEffect(triggeredByAura->GetEffIndex(), GetSpawnMode())->TriggerSpell;

    Unit* target = victim ? victim : GetTargetUnit();

    Item* castItem = triggeredByAura->GetBase()->GetCastItemGUID() && GetTypeId() == TYPEID_PLAYER
        ? ToPlayer()->GetItemByGuid(triggeredByAura->GetBase()->GetCastItemGUID()) : NULL;

    // All ok. Check current trigger spell
    SpellInfo const* triggerEntry = sSpellMgr->GetSpellInfo(trigger_spell_id);
    if (triggerEntry == NULL)
    {
        // Don't cast unknown spell
        if(SpellProcTriggered(victim, dmgInfoProc, triggeredByAura, procSpell, procFlags, procEx, cooldown))
            return true;
        return false;
    }

    // not allow proc extra attack spell at extra attack
    if (m_extraAttacks && triggerEntry->HasEffect(SPELL_EFFECT_ADD_EXTRA_ATTACKS))
        return false;

    if (G3D::fuzzyGt(cooldown, 0.0) && GetTypeId() == TYPEID_PLAYER && ToPlayer()->HasSpellCooldown(trigger_spell_id))
        return false;

    // try detect target manually if not set
    if (target == NULL)
        target = !(procFlags & (PROC_FLAG_DONE_SPELL_MAGIC_DMG_CLASS_POS | PROC_FLAG_DONE_SPELL_NONE_DMG_CLASS_POS)) && triggerEntry && triggerEntry->IsPositive() ? this : victim;

    CastSpell(target, trigger_spell_id, true, castItem, triggeredByAura);

    if (G3D::fuzzyGt(cooldown, 0.0) && GetTypeId() == TYPEID_PLAYER)
        ToPlayer()->AddSpellCooldown(trigger_spell_id, 0, getPreciseTime() + cooldown);

    return true;
}

bool Unit::HandleOverrideClassScriptAuraProc(Unit* victim, DamageInfo* /*dmgInfoProc*/, AuraEffect* triggeredByAura, SpellInfo const* /*procSpell*/, double cooldown)
{
    int32 scriptId = triggeredByAura->GetMiscValue();

    if (!victim || !victim->isAlive())
        return false;

    Item* castItem = triggeredByAura->GetBase()->GetCastItemGUID() && GetTypeId() == TYPEID_PLAYER
        ? ToPlayer()->GetItemByGuid(triggeredByAura->GetBase()->GetCastItemGUID()) : NULL;

    uint32 triggered_spell_id = 0;

    switch (scriptId)
    {
        case 4533:                                          // Dreamwalker Raiment 2 pieces bonus
        {
            // Chance 50%
            if (!roll_chance_i(50))
                return false;

            switch (victim->getPowerType())
            {
                case POWER_MANA:   triggered_spell_id = 28722; break;
                case POWER_RAGE:   triggered_spell_id = 28723; break;
                case POWER_ENERGY: triggered_spell_id = 28724; break;
                default:
                    return false;
            }
            break;
        }
        case 4537:                                          // Dreamwalker Raiment 6 pieces bonus
            triggered_spell_id = 28750;                     // Blessing of the Claw
            break;
        default:
            break;
    }

    // not processed
    if (!triggered_spell_id)
        return false;

    // standard non-dummy case
    SpellInfo const* triggerEntry = sSpellMgr->GetSpellInfo(triggered_spell_id);

    if (!triggerEntry)
    {
        sLog->outError(LOG_FILTER_UNITS, "Unit::HandleOverrideClassScriptAuraProc: Spell %u triggering for class script id %u", triggered_spell_id, scriptId);
        return false;
    }

    if (G3D::fuzzyGt(cooldown, 0.0) && GetTypeId() == TYPEID_PLAYER && ToPlayer()->HasSpellCooldown(triggered_spell_id))
        return false;

    CastSpell(victim, triggered_spell_id, true, castItem, triggeredByAura);

    if (G3D::fuzzyGt(cooldown, 0.0) && GetTypeId() == TYPEID_PLAYER)
        ToPlayer()->AddSpellCooldown(triggered_spell_id, 0, getPreciseTime() + cooldown);

    return true;
}

void Unit::setPowerType(Powers new_powertype)
{
    SetFieldPowerType(new_powertype);

    if (GetTypeId() == TYPEID_PLAYER)
    {
        if (ToPlayer()->GetGroup())
            ToPlayer()->SetGroupUpdateFlag(GROUP_UPDATE_FLAG_POWER_TYPE);
    }
    else if (Pet* pet = ToCreature()->ToPet())
    {
        if (pet->isControlled())
        {
            Unit* owner = GetOwner();
            if (owner && (owner->GetTypeId() == TYPEID_PLAYER) && owner->ToPlayer()->GetGroup())
                owner->ToPlayer()->SetGroupUpdateFlag(GROUP_UPDATE_FLAG_PET_POWER_TYPE);
        }
    }

    switch (new_powertype)
    {
        default:
        case POWER_MANA:
            break;
        case POWER_RAGE:
            SetMaxPower(POWER_RAGE, GetCreatePowers(POWER_RAGE));
            SetPower(POWER_RAGE, 0);
            break;
        case POWER_FOCUS:
            SetMaxPower(POWER_FOCUS, GetCreatePowers(POWER_FOCUS));
            SetPower(POWER_FOCUS, GetCreatePowers(POWER_FOCUS));
            break;
        case POWER_ENERGY:
            SetMaxPower(POWER_ENERGY, GetCreatePowers(POWER_ENERGY));
            break;
    }
}

FactionTemplateEntry const* Unit::getFactionTemplateEntry() const
{
    FactionTemplateEntry const* entry = sFactionTemplateStore.LookupEntry(getFaction());
    if (!entry)
    {
        static uint64 guid = 0;                             // prevent repeating spam same faction problem

        if (GetGUID() != guid)
        {
            if (Player const* player = ToPlayer())
                sLog->outError(LOG_FILTER_UNITS, "Player %s has invalid faction (faction template id) #%u", player->GetName(), getFaction());
            else if (Creature const* creature = ToCreature())
                sLog->outError(LOG_FILTER_UNITS, "Creature (template id: %u) has invalid faction (faction template id) #%u", creature->GetCreatureTemplate()->Entry, getFaction());
            else
                sLog->outError(LOG_FILTER_UNITS, "Unit (name=%s, type=%u) has invalid faction (faction template id) #%u", GetName(), uint32(GetTypeId()), getFaction());

            guid = GetGUID();
        }
    }
    return entry;
}

// function based on function Unit::UnitReaction from 13850 client
ReputationRank Unit::GetReactionTo(Unit const* target) const
{
    // always friendly to self
    if (this == target)
        return REP_FRIENDLY;

    if (!target)
    	return REP_FRIENDLY;

    // always friendly to charmer or owner
    if (GetCharmerOrOwnerOrSelf() == target->GetCharmerOrOwnerOrSelf())
        return REP_FRIENDLY;

    if (HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PVP_ATTACKABLE) || target->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PVP_ATTACKABLE))
        {
            Player const* selfPlayerOwner = GetAffectingPlayer();
            Player const* targetPlayerOwner = target->GetAffectingPlayer();

            if (selfPlayerOwner && targetPlayerOwner)
            {
                // always friendly to other unit controlled by player, or to the player himself
                if (selfPlayerOwner == targetPlayerOwner)
                    return REP_FRIENDLY;

                // duel - always hostile to opponent
                if (selfPlayerOwner->duel && selfPlayerOwner->duel->opponent == targetPlayerOwner && selfPlayerOwner->duel->startTime != 0)
                    return REP_HOSTILE;

                // same group - checks dependant only on our faction - skip FFA_PVP for example
                if (selfPlayerOwner->IsInRaidWith(targetPlayerOwner))
                    return REP_FRIENDLY; // return true to allow config option AllowTwoSide.Interaction.Group to work
                    // however client seems to allow mixed group parties, because in 13850 client it works like:
                    // return GetFactionReactionTo(getFactionTemplateEntry(), target);

                if (selfPlayerOwner->GetByteValue(UNIT_FIELD_BYTES_2, 1) & UNIT_BYTE2_FLAG_FFA_PVP
                    && targetPlayerOwner->GetByteValue(UNIT_FIELD_BYTES_2, 1) & UNIT_BYTE2_FLAG_FFA_PVP)
                    return REP_HOSTILE;
            }

            // check FFA_PVP
            if (GetByteValue(UNIT_FIELD_BYTES_2, 1) & UNIT_BYTE2_FLAG_FFA_PVP
                && target->GetByteValue(UNIT_FIELD_BYTES_2, 1) & UNIT_BYTE2_FLAG_FFA_PVP)
                return REP_HOSTILE;

            if (selfPlayerOwner)
            {
                if (FactionTemplateEntry const* targetFactionTemplateEntry = target->getFactionTemplateEntry())
                {
                    if (ReputationRank const* repRank = selfPlayerOwner->GetReputationMgr().GetForcedRankIfAny(targetFactionTemplateEntry))
                        return *repRank;
                    if (!selfPlayerOwner->HasFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_IGNORE_REPUTATION))
                    {
                        if (FactionEntry const* targetFactionEntry = sFactionStore.LookupEntry(targetFactionTemplateEntry->faction))
                        {
                            if (targetFactionEntry->CanHaveReputation())
                            {
                                // check contested flags
                                if (targetFactionTemplateEntry->factionFlags & FACTION_TEMPLATE_FLAG_CONTESTED_GUARD
                                    && selfPlayerOwner->HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_CONTESTED_PVP))
                                    return REP_HOSTILE;

                                // if faction has reputation, hostile state depends only from AtWar state
                                if (selfPlayerOwner->GetReputationMgr().IsAtWar(targetFactionEntry))
                                    return REP_HOSTILE;
                                return REP_FRIENDLY;
                            }
                        }
                    }
                }
            }
        }

    // do checks dependant only on our faction
    return GetFactionReactionTo(getFactionTemplateEntry(), target);
}

ReputationRank Unit::GetFactionReactionTo(FactionTemplateEntry const* factionTemplateEntry, Unit const* target)
{
    // always neutral when no template entry found
    if (!factionTemplateEntry)
        return REP_NEUTRAL;

    FactionTemplateEntry const* targetFactionTemplateEntry = target->getFactionTemplateEntry();
    if (!targetFactionTemplateEntry)
        return REP_NEUTRAL;

    if (Player const* targetPlayerOwner = target->GetAffectingPlayer())
    {
        // check contested flags
        if (factionTemplateEntry->factionFlags & FACTION_TEMPLATE_FLAG_CONTESTED_GUARD
            && targetPlayerOwner->HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_CONTESTED_PVP))
            return REP_HOSTILE;
        if (ReputationRank const* repRank = targetPlayerOwner->GetReputationMgr().GetForcedRankIfAny(factionTemplateEntry))
            return *repRank;
        if (!target->HasFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_IGNORE_REPUTATION))
        {
            if (FactionEntry const* factionEntry = sFactionStore.LookupEntry(factionTemplateEntry->faction))
            {
                if (factionEntry->CanHaveReputation())
                {
                    // CvP case - check reputation, don't allow state higher than neutral when at war
                    ReputationRank repRank = targetPlayerOwner->GetReputationMgr().GetRank(factionEntry);
                    if (targetPlayerOwner->GetReputationMgr().IsAtWar(factionEntry))
                        repRank = std::min(REP_NEUTRAL, repRank);
                    return repRank;
                }
            }
        }
    }

    // common faction based check
    if (factionTemplateEntry->IsHostileTo(*targetFactionTemplateEntry))
        return REP_HOSTILE;
    if (factionTemplateEntry->IsFriendlyTo(*targetFactionTemplateEntry))
        return REP_FRIENDLY;
    if (targetFactionTemplateEntry->IsFriendlyTo(*factionTemplateEntry))
        return REP_FRIENDLY;
    if (factionTemplateEntry->factionFlags & FACTION_TEMPLATE_FLAG_HOSTILE_BY_DEFAULT)
        return REP_HOSTILE;
    // neutral by default
    return REP_NEUTRAL;
}

bool Unit::IsHostileTo(Unit const* unit) const
{
    return GetReactionTo(unit) <= REP_HOSTILE;
}

bool Unit::IsFriendlyTo(Unit const* unit) const
{
    return GetReactionTo(unit) >= REP_FRIENDLY;
}

bool Unit::IsHostileToPlayers() const
{
    FactionTemplateEntry const* my_faction = getFactionTemplateEntry();
    if (!my_faction || !my_faction->faction)
        return false;

    FactionEntry const* raw_faction = sFactionStore.LookupEntry(my_faction->faction);
    if (raw_faction && raw_faction->reputationListID >= 0)
        return false;

    return my_faction->IsHostileToPlayers();
}

bool Unit::IsNeutralToAll() const
{
    FactionTemplateEntry const* my_faction = getFactionTemplateEntry();
    if (!my_faction || !my_faction->faction)
        return true;

    FactionEntry const* raw_faction = sFactionStore.LookupEntry(my_faction->faction);
    if (raw_faction && raw_faction->reputationListID >= 0)
        return false;

    return my_faction->IsNeutralToAll();
}

bool Unit::Attack(Unit* victim, bool meleeAttack)
{
    if (!victim || victim == this)
        return false;

    // dead units can neither attack nor be attacked
    if (!isAlive() || !victim->IsInWorld() || !victim->isAlive())
        return false;

    // player cannot attack in mount state
    if (GetTypeId() == TYPEID_PLAYER && IsMounted())
        return false;

    // nobody can attack GM in GM-mode
    if (victim->GetTypeId() == TYPEID_PLAYER)
    {
        if (victim->ToPlayer()->isGameMaster())
            return false;
    }
    else
    {
        if (victim->ToCreature()->IsInEvadeMode())
            return false;
    }

    // remove SPELL_AURA_MOD_UNATTACKABLE at attack (in case non-interruptible spells stun aura applied also that not let attack)
    if (HasAuraType(SPELL_AURA_MOD_UNATTACKABLE))
        RemoveAurasByType(SPELL_AURA_MOD_UNATTACKABLE);

    if (m_attacking)
    {
        if (m_attacking == victim)
        {
            // switch to melee attack from ranged/magic
            if (meleeAttack)
            {
                if (!HasUnitState(UNIT_STATE_MELEE_ATTACKING))
                {
                    AddUnitState(UNIT_STATE_MELEE_ATTACKING);
                    SendMeleeAttackStart(victim);
                    return true;
                }
            }
            else if (HasUnitState(UNIT_STATE_MELEE_ATTACKING))
            {
                ClearUnitState(UNIT_STATE_MELEE_ATTACKING);
                SendMeleeAttackStop(victim);
                return true;
            }
            return false;
        }

        // switch target
        InterruptSpell(CURRENT_MELEE_SPELL);
        if (!meleeAttack)
            ClearUnitState(UNIT_STATE_MELEE_ATTACKING);
    }

    if (m_attacking)
        m_attacking->_removeAttacker(this);

    m_attacking = victim;
    m_attacking->_addAttacker(this);

    // Set our target
    SetTarget(victim->GetGUID());

    if (meleeAttack)
        AddUnitState(UNIT_STATE_MELEE_ATTACKING);

    // set position before any AI calls/assistance
    //if (GetTypeId() == TYPEID_UNIT)
    //    ToCreature()->SetCombatStartPosition(GetPositionX(), GetPositionY(), GetPositionZ());

    if (GetTypeId() == TYPEID_UNIT && !ToCreature()->isPet())
    {
        // should not let player enter combat by right clicking target - doesn't helps
        SetInCombatWith(victim);
        if (victim->GetTypeId() == TYPEID_PLAYER)
            victim->SetInCombatWith(this);
        AddThreat(victim, 0.0f);

        ToCreature()->SendAIReaction(AI_REACTION_HOSTILE);
        ToCreature()->CallAssistance();
    }

    // delay offhand weapon attack to next attack time
    if (haveOffhandWeapon())
        setAttackTimer(OFF_ATTACK, 1000);

    if (meleeAttack)
        SendMeleeAttackStart(victim);

    // Let the pet know we've started attacking someting. Handles melee attacks only
    // Spells such as auto-shot and others handled in WorldSession::HandleCastSpellOpcode
    if (this->GetTypeId() == TYPEID_PLAYER)
    {
        Pet* playerPet = this->ToPlayer()->GetPet();

        if (playerPet && playerPet->isAlive())
            playerPet->AI()->OwnerAttacked(victim);
    }

    return true;
}

bool Unit::AttackStop()
{
    if (!m_attacking)
        return false;

    Unit* victim = m_attacking;

    m_attacking->_removeAttacker(this);
    m_attacking = NULL;

    // Clear our target
    SetTarget(0);

    ClearUnitState(UNIT_STATE_MELEE_ATTACKING);

    InterruptSpell(CURRENT_MELEE_SPELL);

    // reset only at real combat stop
    if (Creature* creature = ToCreature())
    {
        creature->SetNoCallAssistance(false);

        if (creature->HasSearchedAssistance())
        {
            creature->SetNoSearchAssistance(false);
            UpdateSpeed(MOVE_RUN, false);
        }
    }

    SendMeleeAttackStop(victim);

    return true;
}

void Unit::CombatStop(bool includingCast)
{
    if (includingCast && IsNonMeleeSpellCasted(false))
        InterruptNonMeleeSpells(false);

    AttackStop();
    RemoveAllAttackers();
    if (GetTypeId() == TYPEID_PLAYER)
        ToPlayer()->SendAttackSwingResult(ATTACK_SWING_ERROR_DEAD_TARGET);     // melee and ranged forced attack cancel
    ClearInCombat();
}

void Unit::CombatStopWithPets(bool includingCast)
{
    if(!IsInWorld())
        return;

    CombatStop(includingCast);

    for (ControlList::const_iterator itr = m_Controlled.begin(), next; itr != m_Controlled.end(); itr = next)
    {
        next = itr;
        ++next;
        auto controlled = (*itr);
        if (!controlled || !controlled->IsInWorld())
            continue;

        controlled->CombatStop(includingCast);
    }
}

bool Unit::isAttackingPlayer() const
{
    if (HasUnitState(UNIT_STATE_ATTACK_PLAYER))
        return true;

    for (ControlList::const_iterator itr = m_Controlled.begin(); itr != m_Controlled.end(); ++itr)
        if ((*itr)->isAttackingPlayer())
            return true;

    for (uint8 i = 0; i < MAX_SUMMON_SLOT; ++i)
        if (m_SummonSlot[i])
            if (Creature* summon = GetMap()->GetCreature(m_SummonSlot[i]))
                if (summon->isAttackingPlayer())
                    return true;

    return false;
}

void Unit::RemoveAllAttackers()
{
    while (!m_attackers.empty())
    {
        AttackerSet::iterator iter = m_attackers.begin();
        if (!(*iter)->AttackStop())
        {
            sLog->outError(LOG_FILTER_UNITS, "WORLD: Unit has an attacker that isn't attacking it!");
            m_attackers.erase(iter);
        }
    }
}

void Unit::ModifyAuraState(AuraStateType flag, bool apply)
{
    if (apply)
    {
        if (!HasFlag(UNIT_FIELD_AURASTATE, 1<<(flag-1)))
        {
            SetFlag(UNIT_FIELD_AURASTATE, 1<<(flag-1));
            if (GetTypeId() == TYPEID_PLAYER)
            {
                PlayerSpellMap const& sp_list = ToPlayer()->GetSpellMap();
                for (PlayerSpellMap::const_iterator itr = sp_list.begin(); itr != sp_list.end(); ++itr)
                {
                    if (itr->second->state == PLAYERSPELL_REMOVED || itr->second->disabled)
                        continue;
                    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(itr->first);
                    if (!spellInfo || !spellInfo->IsPassive())
                        continue;
                    if (spellInfo->CasterAuraState == uint32(flag))
                        CastSpell(this, itr->first, true, NULL);
                }
            }
            else if (Pet* pet = ToCreature()->ToPet())
            {
                for (PetSpellMap::const_iterator itr = pet->m_spells.begin(); itr != pet->m_spells.end(); ++itr)
                {
                    if (itr->second.state == PETSPELL_REMOVED)
                        continue;
                    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(itr->first);
                    if (!spellInfo || !spellInfo->IsPassive())
                        continue;
                    if (spellInfo->CasterAuraState == uint32(flag))
                        CastSpell(this, itr->first, true, NULL);
                }
            }
        }
    }
    else
    {
        if (HasFlag(UNIT_FIELD_AURASTATE, 1<<(flag-1)))
        {
            RemoveFlag(UNIT_FIELD_AURASTATE, 1<<(flag-1));

            Unit::AuraApplicationMap& tAuras = GetAppliedAuras();
            for (Unit::AuraApplicationMap::iterator itr = tAuras.begin(); itr != tAuras.end();)
            {
                SpellInfo const* spellProto = (*itr).second->GetBase()->GetSpellInfo();
                if (spellProto->CasterAuraState == uint32(flag))
                    RemoveAura(itr);
                else
                    ++itr;
            }
        }
    }
}

void Unit::ModifyExcludeCasterAuraSpell(uint32 auraId, bool apply)
{
    if (Player* plr = ToPlayer())
    {
        if (apply)
        {
            std::list<uint32> removeAuraList;
            Unit::AuraApplicationMap& auras = GetAppliedAuras();
            for (Unit::AuraApplicationMap::iterator itr = auras.begin(); itr != auras.end(); ++itr)
            {
                SpellInfo const* spellProto = (*itr).second->GetBase()->GetSpellInfo();

                if (spellProto->Id == auraId)
                    continue;

                if (spellProto->ExcludeCasterAuraSpell == auraId && spellProto->IsPassive())
                    removeAuraList.push_back(spellProto->Id);
            }

            ItemSpellList itmSpells = plr->GetItemSpellList();
            for (ItemSpellList::iterator itr = itmSpells.begin(); itr != itmSpells.end(); ++itr)
            {
                SpellInfo const* spellProto = sSpellMgr->GetSpellInfo(*itr);

                if (!spellProto || spellProto->Id == auraId)
                    continue;

                if (spellProto->ExcludeCasterAuraSpell == auraId && spellProto->IsPassive())
                    removeAuraList.push_back(spellProto->Id);
            }

            for (std::list<uint32>::iterator itr = removeAuraList.begin(); itr != removeAuraList.end(); ++itr)
                RemoveAurasDueToSpell(*itr);
        }
        else
        {
            PlayerSpellMap const& sp_list = plr->GetSpellMap();
            for (PlayerSpellMap::const_iterator itr = sp_list.begin(); itr != sp_list.end(); ++itr)
            {
                if (!itr->second || itr->second->state == PLAYERSPELL_REMOVED || itr->second->disabled)
                    continue;

                SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(itr->first);

                if (!spellInfo)
                    continue;

                if (spellInfo->ExcludeCasterAuraSpell == auraId && !HasAura(spellInfo->Id) && spellInfo->IsPassive())
                    CastSpell(this, spellInfo->Id, true);
            }

            ItemSpellList itmSpells = plr->GetItemSpellList();
            for (ItemSpellList::iterator itr = itmSpells.begin(); itr != itmSpells.end(); ++itr)
            {
                SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(*itr);

                if (!spellInfo)
                    continue;

                if (spellInfo->ExcludeCasterAuraSpell == auraId && !HasAura(spellInfo->Id) && spellInfo->IsPassive())
                    CastSpell(this, spellInfo->Id, true);
            }
        }
    }
}

uint32 Unit::BuildAuraStateUpdateForTarget(Unit* target) const
{
    uint32 auraStates = GetUInt32Value(UNIT_FIELD_AURASTATE) &~(PER_CASTER_AURA_STATE_MASK);
    for (AuraStateAurasMap::const_iterator itr = m_auraStateAuras.begin(); itr != m_auraStateAuras.end(); ++itr)
        if ((1<<(itr->first-1)) & PER_CASTER_AURA_STATE_MASK)
            if (itr->second->GetBase()->GetCasterGUID() == target->GetGUID())
                auraStates |= (1<<(itr->first-1));

    return auraStates;
}

bool Unit::HasAuraState(AuraStateType flag, SpellInfo const* spellProto, Unit const* Caster) const
{
    if (Caster)
    {
        if (spellProto)
        {
            AuraEffectList stateAuras = Caster->GetAuraEffectsByType(SPELL_AURA_ABILITY_IGNORE_AURASTATE);
            for (AuraEffectList::const_iterator j = stateAuras.begin(); j != stateAuras.end(); ++j)
                if ((*j)->IsAffectingSpell(spellProto))
                    return true;
        }
        // Fix Brain Freeze (57761) - Frostfire Bolt (44614) act as if target has aurastate frozen
        if (spellProto && spellProto->Id == 44614 && Caster->HasAura(57761))
            return true;
        // Check per caster aura state
        // If aura with aurastate by caster not found return false
        // OO: Sha of Pride: 144363 hack fix.
        if ((1<<(flag-1)) & PER_CASTER_AURA_STATE_MASK && (!spellProto || spellProto->Id != 144363))
        {
            for (AuraStateAurasMap::const_iterator itr = m_auraStateAuras.lower_bound(flag); itr != m_auraStateAuras.upper_bound(flag); ++itr)
                if (itr->second->GetBase()->GetCasterGUID() == Caster->GetGUID())
                    return true;
            return false;
        }
    }

    return HasFlag(UNIT_FIELD_AURASTATE, 1<<(flag-1));
}

void Unit::SetOwnerGUID(uint64 owner)
{
    if (GetSummonedByGUID() == owner)
        return;

    SetUInt64Value(UNIT_FIELD_SUMMONEDBY, owner);
    if (!owner)
        return;

    // Update owner dependent fields
    Player* player = ObjectAccessor::GetPlayer(*this, owner);
    if (!player || !player->HaveAtClient(this)) // if player cannot see this unit yet, he will receive needed data with create object
        return;

    SetFieldNotifyFlag(UF_FLAG_OWNER);

    UpdateData udata(GetMapId());
    WorldPacket packet;
    BuildValuesUpdateBlockForPlayer(&udata, player);
    udata.BuildPacket(&packet);
    player->SendDirectMessage(&packet);

    RemoveFieldNotifyFlag(UF_FLAG_OWNER);
}

bool Unit::IsOwnerOrSelf(Unit* owner) const
{
    if(this == owner)
        return true;
    if(GetDemonCreatorGUID() == owner->GetGUID())
        return true;
    if(GetSummonedByGUID() == owner->GetGUID())
        return true;
    if(GetOwnerGUID() == owner->GetGUID())
        return true;
    if(GetCharmerGUID() == owner->GetGUID())
        return true;
    return false;
}

Unit* Unit::GetOwner() const
{
    if (uint64 ownerid = GetOwnerGUID())
    {
        return ObjectAccessor::GetUnit(*this, ownerid);
    }
    return NULL;
}

Unit* Unit::GetAnyOwner() const
{
    if(GetCharmerGUID())
        return GetCharmer();
    if(ToTempSummon())
        return ToTempSummon()->GetSummoner();
    if(GetOwner())
        return GetOwner();
    return NULL;
}

Unit* Unit::GetCharmer() const
{
    if (uint64 charmerid = GetCharmerGUID())
        return ObjectAccessor::GetUnit(*this, charmerid);
    return NULL;
}

Player* Unit::GetCharmerOrOwnerPlayerOrPlayerItself() const
{
    uint64 guid = GetCharmerOrOwnerGUID();
    if (IS_PLAYER_GUID(guid))
        return ObjectAccessor::GetPlayer(*this, guid);

    return GetTypeId() == TYPEID_PLAYER ? (Player*)this : NULL;
}

Player* Unit::GetAffectingPlayer() const
{
    if (!GetCharmerOrOwnerGUID())
        return GetTypeId() == TYPEID_PLAYER ? (Player*)this : NULL;

    if (Unit* owner = GetCharmerOrOwner())
        return owner->GetCharmerOrOwnerPlayerOrPlayerItself();
    return NULL;
}

Minion *Unit::GetFirstMinion() const
{
    if (uint64 pet_guid = GetMinionGUID())
    {
        if (Creature* pet = ObjectAccessor::GetCreatureOrPetOrVehicle(*this, pet_guid))
            if (pet->HasUnitTypeMask(UNIT_MASK_MINION))
                return (Minion*)pet;

        sLog->outError(LOG_FILTER_UNITS, "Unit::GetFirstMinion: Minion %u not exist.", GUID_LOPART(pet_guid));
        const_cast<Unit*>(this)->SetMinionGUID(0);
    }

    return NULL;
}

Guardian* Unit::GetGuardianPet() const
{
    if (uint64 pet_guid = GetPetGUID())
    {
        if (Creature* pet = ObjectAccessor::GetCreatureOrPetOrVehicle(*this, pet_guid))
            if (pet->HasUnitTypeMask(UNIT_MASK_GUARDIAN))
                return (Guardian*)pet;

        sLog->outFatal(LOG_FILTER_UNITS, "Unit::GetGuardianPet: Guardian " UI64FMTD " not exist.", pet_guid);
        const_cast<Unit*>(this)->SetPetGUID(0);
    }

    return NULL;
}

Unit* Unit::GetCharm() const
{
    if (uint64 charm_guid = GetCharmGUID())
    {
        if (Unit* pet = ObjectAccessor::GetUnit(*this, charm_guid))
            return pet;

        sLog->outError(LOG_FILTER_UNITS, "Unit::GetCharm: Charmed creature %u not exist.", GUID_LOPART(charm_guid));
        const_cast<Unit*>(this)->SetUInt64Value(UNIT_FIELD_CHARM, 0);
    }

    return NULL;
}

void Unit::SetMinion(Minion *minion, bool apply, bool stampeded)
{
    //sLog->outDebug(LOG_FILTER_PETS, "SetMinion %u for %u, apply %u, stampeded %i", minion->GetEntry(), GetEntry(), apply, stampeded);

    if (apply)
    {
        if (minion->GetSummonedByGUID())
            return;

        if (minion->HasUnitTypeMask(UNIT_MASK_CONTROLABLE_GUARDIAN))
            minion->SetOwnerGUID(GetGUID());

        if (!isMonkClones())
            m_Controlled.insert(minion);

        if (GetTypeId() == TYPEID_PLAYER)
        {
            minion->m_ControlledByPlayer = true;
            minion->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PVP_ATTACKABLE);
        }

        // Can only have one pet. If a new one is summoned, dismiss the old one.
        if (minion->IsGuardianPet() && !stampeded)
        {
            Guardian* oldPet = NULL;
            for (ControlList::iterator itr = m_Controlled.begin(); itr != m_Controlled.end(); ++itr)
            {
                if ((*itr)->GetGUID() == GetPetGUID())
                {
                    oldPet = (Guardian*)*itr;
                    break;
                }
            }
            //if (Guardian* oldPet = GetGuardianPet())
            if (oldPet)
            {
                if (oldPet != minion && (oldPet->isPet() || minion->isPet() || oldPet->GetEntry() != minion->GetEntry()))
                {
                    // remove existing minion pet
                    if (oldPet->isPet())
                        ((Pet*)oldPet)->Remove();
                    else
                        oldPet->UnSummon();
                    SetPetGUID(minion->GetGUID());
                    SetMinionGUID(0);
                }
            }
            else
            {
                SetPetGUID(minion->GetGUID());
                SetMinionGUID(0);
            }
        }
        
        //! Important part for pet slot system. Where we set curent pet number and set in slot array
        if (GetTypeId() == TYPEID_PLAYER && getClass() == CLASS_HUNTER && minion->GetCharmInfo() && !stampeded)
        {
            ToPlayer()->m_currentPetNumber = minion->GetCharmInfo()->GetPetNumber();
            ToPlayer()->setPetSlotWithStableMoveOrRealDelete(ToPlayer()->m_currentSummonedSlot, minion->GetCharmInfo()->GetPetNumber(), minion->isHunterPet()); // minion->isHunterPet() always true ;)
        }

        if (minion->HasUnitTypeMask(UNIT_MASK_CONTROLABLE_GUARDIAN))
            AddUInt64Value(UNIT_FIELD_SUMMON, minion->GetGUID());

        if (minion->m_Properties && minion->m_Properties->Type == SUMMON_TYPE_MINIPET)
            SetCritterGUID(minion->GetGUID());

        // PvP, FFAPvP
        minion->SetByteValue(UNIT_FIELD_BYTES_2, 1, GetByteValue(UNIT_FIELD_BYTES_2, 1));

        // FIXME: hack, speed must be set only at follow
        if (GetTypeId() == TYPEID_PLAYER && minion->isPet())
            for (uint8 i = 0; i < MAX_MOVE_TYPE; ++i)
                minion->SetSpeed(UnitMoveType(i), m_speed_rate[i], true);

        // Ghoul pets and Warlock's pets have energy instead of mana (is anywhere better place for this code?)
        if (minion->IsPetGhoul() || (minion->GetOwner() && minion->GetOwner()->getClass() == CLASS_WARLOCK))
            minion->setPowerType(POWER_ENERGY);

        if (GetTypeId() == TYPEID_PLAYER)
        {
            // Send infinity cooldown - client does that automatically but after relog cooldown needs to be set again
            SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(minion->GetUInt32Value(UNIT_CREATED_BY_SPELL));

            if (spellInfo && (spellInfo->Attributes & SPELL_ATTR0_DISABLED_WHILE_ACTIVE))
                ToPlayer()->AddSpellAndCategoryCooldowns(spellInfo, 0, NULL, true);
        }
    }
    else
    {
        if (minion->GetOwnerGUID() != GetGUID())
        {
            sLog->outFatal(LOG_FILTER_UNITS, "SetMinion: Minion %u is not the minion of owner %u", minion->GetEntry(), GetEntry());
            return;
        }

        m_Controlled.erase(minion);

        if (minion->m_Properties && minion->m_Properties->Type == SUMMON_TYPE_MINIPET)
        {
            if (GetCritterGUID() == minion->GetGUID())
                SetCritterGUID(0);
        }

        if (minion->IsGuardianPet() && !stampeded)
        {
            if (GetPetGUID() == minion->GetGUID())
                SetPetGUID(0);
        }
        else if (minion->isTotem())
        {
            // All summoned by totem minions must disappear when it is removed.
            if (SpellInfo const* spInfo = sSpellMgr->GetSpellInfo(minion->ToTotem()->GetSpell()))
            {
                for (int i = 0; i < MAX_SPELL_EFFECTS; ++i)
                {
                    if (spInfo->GetEffect(i, GetSpawnMode())->Effect != SPELL_EFFECT_SUMMON)
                        continue;

                    RemoveAllMinionsByEntry(spInfo->GetEffect(i, GetSpawnMode())->MiscValue);
                }
            }

            if (minion->GetEntry() == 15439 && minion->GetOwner())
            {
                if(minion->GetOwner()->HasAura(117013))
                    RemoveAllMinionsByEntry(61029);
                else
                    RemoveAllMinionsByEntry(15438);
            }
            else if (minion->GetEntry() == 15430 && minion->GetOwner())
            {
                if(minion->GetOwner()->HasAura(117013))
                    RemoveAllMinionsByEntry(61056);
                else
                    RemoveAllMinionsByEntry(15352);
            }
        }

        if (GetTypeId() == TYPEID_PLAYER)
        {
            SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(minion->GetUInt32Value(UNIT_CREATED_BY_SPELL));
            // Remove infinity cooldown
            if (spellInfo && (spellInfo->Attributes & SPELL_ATTR0_DISABLED_WHILE_ACTIVE))
                ToPlayer()->SendCooldownEvent(spellInfo);
        }

        //if (minion->HasUnitTypeMask(UNIT_MASK_GUARDIAN))
        {
            if (RemoveUInt64Value(UNIT_FIELD_SUMMON, minion->GetGUID()))
            {
                // Check if there is another minion
                for (ControlList::iterator itr = m_Controlled.begin(); itr != m_Controlled.end(); ++itr)
                {
                    // do not use this check, creature do not have charm guid
                    //if (GetCharmGUID() == (*itr)->GetGUID())
                    if (GetGUID() == (*itr)->GetCharmerGUID())
                        continue;

                    //ASSERT((*itr)->GetOwnerGUID() == GetGUID());
                    if ((*itr)->GetOwnerGUID() != GetGUID())
                    {
                        OutDebugInfo();
                        (*itr)->OutDebugInfo();
                        ASSERT(false);
                    }
                    ASSERT((*itr)->GetTypeId() == TYPEID_UNIT);

                    if (!(*itr)->HasUnitTypeMask(UNIT_MASK_CONTROLABLE_GUARDIAN))
                        continue;

                    if (AddUInt64Value(UNIT_FIELD_SUMMON, (*itr)->GetGUID()))
                    {
                        // show another pet bar if there is no charm bar
                        if (GetTypeId() == TYPEID_PLAYER && !GetCharmGUID())
                        {
                            if ((*itr)->isPet())
                                ToPlayer()->PetSpellInitialize();
                            else
                                ToPlayer()->CharmSpellInitialize();
                        }
                    }
                    break;
                }
            }
        }
    }

    uint32 count = 0;
    for (ControlList::iterator itr = m_Controlled.begin(); itr != m_Controlled.end(); ++itr)
    {
        float angle = PET_FOLLOW_ANGLE + (((PET_FOLLOW_ANGLE * 2) / m_Controlled.size()) * count);
        if (Creature* creature = (*itr)->ToCreature())
            creature->SetFollowAngle(angle);
        count++;
    }
}

Creature* Unit::GetMinionByEntry(uint32 entry)
{
    for (Unit::ControlList::iterator itr = m_Controlled.begin(); itr != m_Controlled.end();)
    {
        Unit* unit = *itr;
        ++itr;
        if (unit->GetEntry() == entry && unit->GetTypeId() == TYPEID_UNIT
            && unit->ToCreature()->isSummon()) // minion, actually
            return unit->ToCreature();
    }
    return NULL;
}

void Unit::GetAllMinionsByEntry(std::list<Creature*>& Minions, uint32 entry)
{
    for (Unit::ControlList::iterator itr = m_Controlled.begin(); itr != m_Controlled.end();)
    {
        Unit* unit = *itr;
        ++itr;
        if (unit->GetEntry() == entry && unit->GetTypeId() == TYPEID_UNIT
            && unit->ToCreature()->isSummon()) // minion, actually
            Minions.push_back(unit->ToCreature());
    }
}

void Unit::RemoveAllMinionsByEntry(uint32 entry)
{
    for (Unit::ControlList::iterator itr = m_Controlled.begin(); itr != m_Controlled.end();)
    {
        Unit* unit = *itr;
        ++itr;
        if (unit->GetEntry() == entry && unit->GetTypeId() == TYPEID_UNIT
            && unit->ToCreature()->isSummon()) // minion, actually
            unit->ToTempSummon()->UnSummon();
        // i think this is safe because i have never heard that a despawned minion will trigger a same minion
    }
}

void Unit::SetCharm(Unit* charm, bool apply)
{
    if (apply)
    {
        if (GetTypeId() == TYPEID_PLAYER)
        {
            if (!AddUInt64Value(UNIT_FIELD_CHARM, charm->GetGUID()))
                sLog->outFatal(LOG_FILTER_UNITS, "Player %s is trying to charm unit %u, but it already has a charmed unit " UI64FMTD "", GetName(), charm->GetEntry(), GetCharmGUID());

            charm->m_ControlledByPlayer = true;
            // TODO: maybe we can use this flag to check if controlled by player
            charm->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PVP_ATTACKABLE);
        }
        else
            charm->m_ControlledByPlayer = false;

        // PvP, FFAPvP
        charm->SetByteValue(UNIT_FIELD_BYTES_2, 1, GetByteValue(UNIT_FIELD_BYTES_2, 1));

        if (!charm->AddUInt64Value(UNIT_FIELD_CHARMEDBY, GetGUID()))
            sLog->outFatal(LOG_FILTER_UNITS, "Unit %u is being charmed, but it already has a charmer " UI64FMTD "", charm->GetEntry(), charm->GetCharmerGUID());

        _isWalkingBeforeCharm = charm->IsWalking();
        if (_isWalkingBeforeCharm)
        {
            charm->SetWalk(false);
            charm->SendMovementFlagUpdate();
        }

        m_Controlled.insert(charm);
    }
    else
    {
        if (GetTypeId() == TYPEID_PLAYER)
        {
            if (!RemoveUInt64Value(UNIT_FIELD_CHARM, charm->GetGUID()))
                sLog->outFatal(LOG_FILTER_UNITS, "Player %s is trying to uncharm unit %u, but it has another charmed unit " UI64FMTD "", GetName(), charm->GetEntry(), GetCharmGUID());
        }

        if (!charm->RemoveUInt64Value(UNIT_FIELD_CHARMEDBY, GetGUID()))
            sLog->outFatal(LOG_FILTER_UNITS, "Unit %u is being uncharmed, but it has another charmer " UI64FMTD "", charm->GetEntry(), charm->GetCharmerGUID());

        if (charm->GetTypeId() == TYPEID_PLAYER)
        {
            charm->m_ControlledByPlayer = true;
            charm->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PVP_ATTACKABLE);
            charm->ToPlayer()->UpdatePvPState();
        }
        else if (Player* player = charm->GetCharmerOrOwnerPlayerOrPlayerItself())
        {
            charm->m_ControlledByPlayer = true;
            charm->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PVP_ATTACKABLE);
            charm->SetByteValue(UNIT_FIELD_BYTES_2, 1, player->GetByteValue(UNIT_FIELD_BYTES_2, 1));
        }
        else
        {
            charm->m_ControlledByPlayer = false;
            charm->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PVP_ATTACKABLE);
            charm->SetByteValue(UNIT_FIELD_BYTES_2, 1, 0);
        }

        if (charm->IsWalking() != _isWalkingBeforeCharm)
        {
            charm->SetWalk(_isWalkingBeforeCharm);
            charm->SendMovementFlagUpdate(true); // send packet to self, to update movement state on player.
        }

        if (charm->GetTypeId() == TYPEID_PLAYER
            || !charm->ToCreature()->HasUnitTypeMask(UNIT_MASK_MINION)
            || charm->GetOwnerGUID() != GetGUID())
            m_Controlled.erase(charm);
    }
}

int32 Unit::DealHeal(Unit* victim, uint32 addhealth, SpellInfo const* spellProto)
{
    int32 gain = 0;

    if (victim->IsAIEnabled)
        victim->GetAI()->HealReceived(this, addhealth);

    if (IsAIEnabled)
        GetAI()->HealDone(victim, addhealth);

    if (addhealth)
        gain = victim->ModifyHealth(int32(addhealth));

    Unit* unit = this;

    if (GetTypeId() == TYPEID_UNIT && (ToCreature()->isTotem() || ToCreature()->GetEntry() == 60849))
        unit = GetOwner();

    // Custom MoP Script
    // Purification (passive) - 16213 : Increase maximum health by 10% of the amount healed up to a maximum of 10% of health
    if (unit && unit->GetTypeId() == TYPEID_PLAYER && addhealth != 0 && unit->HasAura(16213))
    {
        int32 bp = 0;
        bp = int32(addhealth / 10);

        if (bp > (victim->GetMaxHealth() * 0.1f))
            bp = (victim->GetMaxHealth() * 0.1f);

        // Ancestral Vigor - 105284
        victim->CastCustomSpell(victim, 105284, &bp, NULL, NULL, true);
    }

    if (Player* player = unit->ToPlayer())
    {
        if (Battleground* bg = player->GetBattleground())
            bg->UpdatePlayerScore(player, SCORE_HEALING_DONE, gain);

        // use the actual gain, as the overheal shall not be counted, skip gain 0 (it ignored anyway in to criteria)
        if (gain)
            player->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_HEALING_DONE, gain, 0, 0, victim);

        player->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_HEAL_CASTED, addhealth);
    }

    if (Player* player = victim->ToPlayer())
    {
        player->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_TOTAL_HEALING_RECEIVED, gain);
        player->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_HEALING_RECEIVED, addhealth);
        /** World of Warcraft Armory **/
        if (Battleground *bgV = player->GetBattleground())
            bgV->UpdatePlayerScore(player, SCORE_HEALING_TAKEN, gain);
        /** World of Warcraft Armory **/
    }

    if (gain)
        m_damage_counters[HEALING_DONE_COUNTER][0] += gain;

    return gain;
}

Unit* Unit::GetMagicHitRedirectTarget(Unit* victim, SpellInfo const* spellInfo)
{
    // Patch 1.2 notes: Spell Reflection no longer reflects abilities
    if (spellInfo->Attributes & SPELL_ATTR0_ABILITY || spellInfo->AttributesEx & SPELL_ATTR1_CANT_BE_REDIRECTED || spellInfo->Attributes & SPELL_ATTR0_UNAFFECTED_BY_INVULNERABILITY || spellInfo->AttributesEx4 & SPELL_ATTR4_UNK15)
        return victim;

    Unit::AuraEffectList magnetAuras = victim->GetAuraEffectsByType(SPELL_AURA_SPELL_MAGNET);
    for (Unit::AuraEffectList::const_iterator itr = magnetAuras.begin(); itr != magnetAuras.end(); ++itr)
    {
        if (Unit* magnet = (*itr)->GetBase()->GetCaster())
        {
            magnet->SetMagnetSpell(true);

            if (spellInfo->CheckExplicitTarget(this, magnet) == SPELL_CAST_OK
                && spellInfo->CheckTarget(this, magnet, false) == SPELL_CAST_OK
                && _IsValidAttackTarget(magnet, spellInfo))
            {
                // TODO: handle this charge drop by proc in cast phase on explicit target
                //(*itr)->GetBase()->DropCharge(AURA_REMOVE_BY_DROP_CHARGERS);
                return magnet;
            }
            else
                magnet->SetMagnetSpell(false);
        }
    }
    return victim;
}

Unit* Unit::GetMeleeHitRedirectTarget(Unit* victim, SpellInfo const* spellInfo)
{
    AuraEffectList hitTriggerAuras = victim->GetAuraEffectsByType(SPELL_AURA_ADD_CASTER_HIT_TRIGGER);
    for (AuraEffectList::const_iterator i = hitTriggerAuras.begin(); i != hitTriggerAuras.end(); ++i)
    {
        if (Unit* magnet = (*i)->GetBase()->GetCaster())
            if (_IsValidAttackTarget(magnet, spellInfo) && magnet->IsWithinLOSInMap(this)
                && (!spellInfo || (spellInfo->CheckExplicitTarget(this, magnet) == SPELL_CAST_OK
                && spellInfo->CheckTarget(this, magnet, false) == SPELL_CAST_OK)))
                {
                    (*i)->GetBase()->DropCharge(AURA_REMOVE_BY_DROP_CHARGERS);
                    return magnet;
                }
    }
    return victim;
}

Unit* Unit::GetFirstControlled() const
{
    // Sequence: charmed, pet, other guardians
    Unit* unit = GetCharm();
    if (!unit)
        if (uint64 guid = GetMinionGUID())
            unit = ObjectAccessor::GetUnit(*this, guid);

    return unit;
}

void Unit::RemoveAllControlled()
{
    // possessed pet and vehicle
    if (GetTypeId() == TYPEID_PLAYER)
        ToPlayer()->StopCastingCharm();

    while (!m_Controlled.empty())
    {
        Unit *target = *m_Controlled.begin();
        m_Controlled.erase(m_Controlled.begin());
        if (target->m_objectTypeId != TYPEID_OBJECT)
        {
            if (target->GetCharmerGUID() == GetGUID())
                target->RemoveCharmAuras();
            else if (target->GetOwnerGUID() == GetGUID() && target->isSummon())
                target->ToTempSummon()->UnSummon();
        }
        else if (target->ToTempSummon() && target->IsInWorld())
            target->ToTempSummon()->UnSummon();
        //else
            //sLog->outError(LOG_FILTER_UNITS, "Unit %u is trying to release unit %u which is neither charmed nor owned by it", GetEntry(), target->GetEntry());
    }

    if (getClass() == CLASS_MONK && (m_SummonSlot[13] || m_SummonSlot[14] || m_SummonSlot[15]))
    {
        std::set<Creature*> RemoveList;
        for (uint8 i = 13; i < 16; ++i)
            if (m_SummonSlot[i])
                if (Creature* crt = GetMap()->GetCreature(m_SummonSlot[i]))
                    RemoveList.insert(crt);

        for (std::set<Creature*>::iterator itr = RemoveList.begin(); itr != RemoveList.end(); ++itr)
            if (TempSummon* tempsum = (*itr)->ToTempSummon())
                tempsum->UnSummon();
    }

    /*if (GetPetGUID())
        sLog->outFatal(LOG_FILTER_UNITS, "Unit %u is not able to release its pet " UI64FMTD, GetEntry(), GetPetGUID());
    if (GetMinionGUID())
        sLog->outFatal(LOG_FILTER_UNITS, "Unit %u is not able to release its minion " UI64FMTD, GetEntry(), GetMinionGUID());
    if (GetCharmGUID())
        sLog->outFatal(LOG_FILTER_UNITS, "Unit %u is not able to release its charm " UI64FMTD, GetEntry(), GetCharmGUID());*/
}

Unit* Unit::GetNextRandomRaidMemberOrPet(float radius)
{
    Player* player = NULL;
    if (GetTypeId() == TYPEID_PLAYER)
        player = ToPlayer();
    // Should we enable this also for charmed units?
    else if (GetTypeId() == TYPEID_UNIT && ToCreature()->isPet())
        player = GetOwner()->ToPlayer();

    if (!player)
        return NULL;
    Group* group = player->GetGroup();
    // When there is no group check pet presence
    if (!group)
    {
        // We are pet now, return owner
        if (player != this)
            return IsWithinDistInMap(player, radius) ? player : NULL;
        Unit* pet = GetGuardianPet();
        // No pet, no group, nothing to return
        if (!pet)
            return NULL;
        // We are owner now, return pet
        return IsWithinDistInMap(pet, radius) ? pet : NULL;
    }

    std::vector<Unit*> nearMembers;
    // reserve place for players and pets because resizing vector every unit push is unefficient (vector is reallocated then)
    nearMembers.reserve(group->GetMembersCount() * 2);

    for (GroupReference* itr = group->GetFirstMember(); itr != NULL; itr = itr->next())
        if (Player* Target = itr->getSource())
        {
            // IsHostileTo check duel and controlled by enemy
            if (Target != this && Target->isAlive() && IsWithinDistInMap(Target, radius) && !IsHostileTo(Target))
                nearMembers.push_back(Target);

        // Push player's pet to vector
        if (Unit* pet = Target->GetGuardianPet())
            if (pet != this && pet->isAlive() && IsWithinDistInMap(pet, radius) && !IsHostileTo(pet))
                nearMembers.push_back(pet);
        }

    if (nearMembers.empty())
        return NULL;

    uint32 randTarget = urand(0, nearMembers.size()-1);
    return nearMembers[randTarget];
}

// only called in Player::SetSeer
// so move it to Player?
void Unit::AddPlayerToVision(Player* player)
{
    if (m_sharedVision.empty())
    {
        setActive(true);
        SetWorldObject(true);
    }
    m_sharedVision.push_back(player);
}

// only called in Player::SetSeer
void Unit::RemovePlayerFromVision(Player* player)
{
    m_sharedVision.remove(player);
    if (m_sharedVision.empty())
    {
        setActive(false);
        SetWorldObject(false);
    }
}

void Unit::RemoveBindSightAuras()
{
    RemoveAurasByType(SPELL_AURA_BIND_SIGHT);
}

void Unit::RemoveCharmAuras()
{
    RemoveAurasByType(SPELL_AURA_MOD_CHARM);
    RemoveAurasByType(SPELL_AURA_MOD_POSSESS_PET);
    RemoveAurasByType(SPELL_AURA_MOD_POSSESS);
    RemoveAurasByType(SPELL_AURA_AOE_CHARM);
}

void Unit::UnsummonAllTotems()
{
    for (uint8 i = 0; i < MAX_SUMMON_SLOT; ++i)
    {
        if (!m_SummonSlot[i])
            continue;

        if (Creature* OldTotem = GetMap()->GetCreature(m_SummonSlot[i]))
            if (OldTotem->isSummon())
                OldTotem->ToTempSummon()->UnSummon();
    }
}

void Unit::SendHealSpellLog(Unit* victim, uint32 SpellID, uint32 Damage, uint32 OverHeal, uint32 Absorb, bool critical)
{
    ObjectGuid casterGuid = GetObjectGuid();
    ObjectGuid targetGuid = victim->GetObjectGuid();

    WorldPacket data(SMSG_SPELLHEALLOG, 4 * 4 + 8 + 8 + 1 + 1 + 1);

    data.WriteGuidMask<4>(casterGuid);
    data.WriteGuidMask<5, 3>(targetGuid);
    data.WriteBit(0);       // not has float
    data.WriteGuidMask<3>(casterGuid);
    data.WriteGuidMask<7>(targetGuid);
    data.WriteGuidMask<0, 5>(casterGuid);
    data.WriteGuidMask<0>(targetGuid);
    data.WriteGuidMask<7, 1>(casterGuid);
    data.WriteBit(critical);
    data.WriteGuidMask<6>(casterGuid);
    data.WriteGuidMask<1, 6, 2>(targetGuid);
    data.WriteGuidMask<2>(casterGuid);
    data.WriteBit(0);       // not has float
    data.WriteGuidMask<4>(targetGuid);
    data.WriteBit(0);       // not has power data

    data << uint32(Damage);
    data.WriteGuidBytes<1>(targetGuid);
    data.WriteGuidBytes<7>(casterGuid);
    data.WriteGuidBytes<7>(targetGuid);
    data.WriteGuidBytes<3, 6, 5>(casterGuid);
    data.WriteGuidBytes<2>(targetGuid);
    data << uint32(OverHeal);
    data.WriteGuidBytes<0, 6>(targetGuid);
    data.WriteGuidBytes<4>(casterGuid);
    data.WriteGuidBytes<5>(targetGuid);
    data.WriteGuidBytes<4, 3>(targetGuid);
    data << uint32(SpellID);
    data << uint32(Absorb); // Absorb amount
    data.WriteGuidBytes<2, 1, 0>(casterGuid);

    SendMessageToSet(&data, true);
}

int32 Unit::HealBySpell(Unit* victim, SpellInfo const* spellInfo, uint32 addHealth, bool critical)
{
    uint32 absorb = 0;
    int32 gain = 0;
    // calculate heal absorb and reduce healing
    CalcHealAbsorb(victim, spellInfo, addHealth, absorb);

    if (spellInfo)
    {
        SpellBonusEntry const* bonus = sSpellMgr->GetSpellBonusData(spellInfo->Id);
        if(bonus && bonus->heal_bonus)
            addHealth *= bonus->heal_bonus;
        
        gain = DealHeal(victim, addHealth, spellInfo);
        SendHealSpellLog(victim, spellInfo->Id, addHealth, uint32(addHealth - gain), absorb, critical);
    }

    return gain;
}

void Unit::SendEnergizeSpellLog(Unit* victim, uint32 spellID, uint32 damage, Powers powerType)
{
    WorldPacket data(SMSG_SPELLENERGIZELOG, 8 + 8 + 1 + 1 + 4 + 4 + 4);

    ObjectGuid targetGuid = victim->GetObjectGuid();
    ObjectGuid sourceGuid = GetObjectGuid();

    data.WriteGuidMask<2>(targetGuid);
    data.WriteGuidMask<1>(sourceGuid);
    data.WriteGuidMask<1>(targetGuid);

    data.WriteBit(0);       // not has power data

    data.WriteGuidMask<3, 0>(targetGuid);
    data.WriteGuidMask<5, 3>(sourceGuid);
    data.WriteGuidMask<5, 6>(targetGuid);
    data.WriteGuidMask<4>(sourceGuid);
    data.WriteGuidMask<7>(targetGuid);
    data.WriteGuidMask<6>(sourceGuid);
    data.WriteGuidMask<4>(targetGuid);
    data.WriteGuidMask<7, 0, 2>(sourceGuid);

    data.WriteGuidBytes<1>(sourceGuid);
    data.WriteGuidBytes<3>(targetGuid);

    data.WriteGuidBytes<5>(sourceGuid);
    data.WriteGuidBytes<7, 1, 6, 2, 0, 4>(targetGuid);
    data << uint32(powerType);
    data.WriteGuidBytes<2>(sourceGuid);
    data << uint32(damage);
    data.WriteGuidBytes<7, 0, 3>(sourceGuid);
    data.WriteGuidBytes<5>(targetGuid);
    data << uint32(spellID);
    data.WriteGuidBytes<4, 6>(sourceGuid);

    SendMessageToSet(&data, true);
}

void Unit::EnergizeBySpell(Unit* victim, uint32 spellID, int32 damage, Powers powerType)
{
    SendEnergizeSpellLog(victim, spellID, damage, powerType);
    // needs to be called after sending spell log
    victim->ModifyPower(powerType, damage);

    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellID);
    victim->getHostileRefManager().threatAssist(this, float(damage) * 0.5f, spellInfo);
}

uint32 Unit::SpellDamageBonusDone(Unit* victim, SpellInfo const* spellProto, uint32 pdamage, DamageEffectType damagetype, SpellEffIndex effIndex, uint32 stack)
{
    if (!spellProto || !victim || damagetype == DIRECT_DAMAGE)
        return pdamage;

    // Some spells don't benefit from done mods
    if ((spellProto->AttributesEx3 & SPELL_ATTR3_NO_DONE_BONUS) || (spellProto->AttributesEx6 & SPELL_ATTR6_NO_DONE_PCT_DAMAGE_MODS))
        return pdamage;

    // small exception for Hemorrhage, can't find any general rule
    // should ignore ALL damage mods, they already calculated in trigger spell
    if (spellProto->Id == 89775) // Hemorrhage and Soul Link damage
        return pdamage;

    // For totems get damage bonus from owner
    if (GetTypeId() == TYPEID_UNIT && ToCreature()->isTotem())
        if (Unit* owner = GetOwner())
            return owner->SpellDamageBonusDone(victim, spellProto, pdamage, damagetype, effIndex);

    // Done total percent damage auras
    float DoneTotalMod = 1.0f;
    int32 DoneTotal = 0;
    float tmpDamage = 0.0f;

    {
        // Apply PowerPvP damage bonus
        DoneTotalMod = CalcPvPPower(victim, DoneTotalMod);

        // Chaos Bolt - 116858 and Soul Fire - 6353
        // damage is increased by your critical strike chance
        if (GetTypeId() == TYPEID_PLAYER && spellProto && (spellProto->Id == 116858 || spellProto->Id == 6353 || spellProto->Id == 104027))
        {
            float crit_chance;
            crit_chance = GetFloatValue(PLAYER_SPELL_CRIT_PERCENTAGE1 + GetFirstSchoolInMask(spellProto->GetSchoolMask()));
            int32 modif = victim->GetTotalAuraModifier(SPELL_AURA_MOD_ATTACKER_SPELL_AND_WEAPON_CRIT_CHANCE);
            if(modif < -100)
                crit_chance -= 50.0f;
            AddPct(DoneTotalMod, crit_chance);
        }

        // Pet damage?
        if (GetTypeId() == TYPEID_UNIT && !ToCreature()->isPet())
            DoneTotalMod *= ToCreature()->GetSpellDamageMod(ToCreature()->GetCreatureTemplate()->rank);

        AuraEffectList mModDamagePercentDone = GetTotalNotStuckAuraEffectByType(SPELL_AURA_MOD_DAMAGE_PERCENT_DONE);

        if (!mModDamagePercentDone.empty())
        {
            float best_bonus = 1.0f;
            bool firstCheck = false;

            for (uint8 j = SPELL_SCHOOL_NORMAL; j < MAX_SPELL_SCHOOL; ++j)
            {
                if (spellProto->GetSchoolMask() & (1 << j))
                {
                    float temp = 1.0f;
                    for (AuraEffectList::const_iterator i = mModDamagePercentDone.begin(); i != mModDamagePercentDone.end(); ++i)
                    {
                        // Mastery: Unshackled Fury
                        if ((*i)->GetId() == 76856 && !HasAuraState(AURA_STATE_ENRAGE))
                            continue;

                        if (ToPlayer() && ToPlayer()->HasItemFitToSpellRequirements((*i)->GetSpellInfo()) && (*i)->GetSpellInfo()->EquippedItemClass != -1 && spellProto->DmgClass == SPELL_DAMAGE_CLASS_MELEE)
                            AddPct(temp, (*i)->GetAmount());
                        else if ((*i)->GetMiscValue() & (1 << j))
                        {
                            if ((*i)->GetSpellInfo()->EquippedItemClass == -1)
                                AddPct(temp, (*i)->GetAmount());
                            else if (!((*i)->GetSpellInfo()->AttributesEx5 & SPELL_ATTR5_SPECIAL_ITEM_CLASS_CHECK) && ((*i)->GetSpellInfo()->EquippedItemSubClassMask == 0))
                                AddPct(temp, (*i)->GetAmount());
                        }
                    }

                    if (best_bonus < temp || !firstCheck)
                    {
                        best_bonus = temp;
                        firstCheck = true;
                    }
                }
            }
            DoneTotalMod *= best_bonus;
        }

        uint32 creatureTypeMask = victim->GetCreatureTypeMask();
        // Add flat bonus from spell damage versus
        DoneTotal += GetTotalAuraModifierByMiscMask(SPELL_AURA_MOD_FLAT_SPELL_DAMAGE_VERSUS, creatureTypeMask);
        AuraEffectList mDamageDoneVersus = GetAuraEffectsByType(SPELL_AURA_MOD_DAMAGE_DONE_VERSUS);
        for (AuraEffectList::const_iterator i = mDamageDoneVersus.begin(); i != mDamageDoneVersus.end(); ++i)
            if (creatureTypeMask & uint32((*i)->GetMiscValue()))
                AddPct(DoneTotalMod, (*i)->GetAmount());

        // bonus against aurastate
        AuraEffectList mDamageDoneVersusAurastate = GetAuraEffectsByType(SPELL_AURA_MOD_DAMAGE_DONE_VERSUS_AURASTATE);
        for (AuraEffectList::const_iterator i = mDamageDoneVersusAurastate.begin(); i != mDamageDoneVersusAurastate.end(); ++i)
            if (victim->HasAuraState(AuraStateType((*i)->GetMiscValue())))
                if (HasAura(144421) && GetPower(POWER_ALTERNATE_POWER))
                {
                    int32 pos = GetPower(POWER_ALTERNATE_POWER);
                    int32 mod = 0;
                    switch (pos)
                    {
                    case 25:
                        mod = -10;
                        break;
                    case 50:
                        mod = -25;
                        break;
                    case 75:
                        mod = -50;
                        break;
                    case 100:
                        mod = -75;
                        break;
                    default:
                        mod = 0;
                        break;
                    }
                    AddPct(DoneTotalMod, mod);
                }
                else
                    AddPct(DoneTotalMod, (*i)->GetAmount());

        // Add SPELL_AURA_MOD_DAMAGE_DONE_FOR_MECHANIC percent bonus
        AddPct(DoneTotalMod, GetTotalAuraModifierByMiscValue(SPELL_AURA_MOD_DAMAGE_DONE_FOR_MECHANIC, spellProto->Mechanic));

        // done scripted mod (take it from owner)
        Unit* owner = GetOwner() ? GetOwner() : this;
        AuraEffectList mOverrideClassScript= owner->GetAuraEffectsByType(SPELL_AURA_OVERRIDE_CLASS_SCRIPTS);
        for (AuraEffectList::const_iterator i = mOverrideClassScript.begin(); i != mOverrideClassScript.end(); ++i)
        {
            if (!(*i)->IsAffectingSpell(spellProto))
                continue;

            switch ((*i)->GetMiscValue())
            {
                case 4920: // Molten Fury
                case 4919:
                {
                    if (victim->HasAuraState(AURA_STATE_HEALTHLESS_35_PERCENT, spellProto, this))
                        AddPct(DoneTotalMod, (*i)->GetAmount());
                    break;
                }
                case 6917: // Death's Embrace damage effect
                case 6926:
                case 6928:
                {
                    // Health at 25% or less (25% stored at effect 2 of the spell)
                    if (victim->HealthBelowPct(CalculateSpellDamage(this, (*i)->GetSpellInfo(), EFFECT_2)))
                        AddPct(DoneTotalMod, (*i)->GetAmount());
                }
                case 6916: // Death's Embrace heal effect
                case 6925:
                case 6927:
                    if (HealthBelowPct(CalculateSpellDamage(this, (*i)->GetSpellInfo(), EFFECT_2)))
                        AddPct(DoneTotalMod, (*i)->GetAmount());
                    break;
                // Soul Siphon
                case 4992:
                case 4993:
                {
                    // effect 1 m_amount
                    int32 maxPercent = (*i)->GetAmount();
                    // effect 0 m_amount
                    int32 stepPercent = CalculateSpellDamage(this, (*i)->GetSpellInfo(), 0);
                    // count affliction effects and calc additional damage in percentage
                    int32 modPercent = 0;
                    AuraApplicationMap const& victimAuras = victim->GetAppliedAuras();
                    for (AuraApplicationMap::const_iterator itr = victimAuras.begin(); itr != victimAuras.end(); ++itr)
                    {
                        Aura const* aura = itr->second->GetBase();
                        SpellInfo const* spell = aura->GetSpellInfo();
                        if (spell->SpellFamilyName != SPELLFAMILY_WARLOCK || !(spell->SpellFamilyFlags[1] & 0x0004071B || spell->SpellFamilyFlags[0] & 0x8044C402))
                            continue;
                        modPercent += stepPercent * aura->GetStackAmount();
                        if (modPercent >= maxPercent)
                        {
                            modPercent = maxPercent;
                            break;
                        }
                    }
                    AddPct(DoneTotalMod, modPercent);
                    break;
                }
                case 5481: // Starfire Bonus
                {
                    if (victim->GetAuraEffect(SPELL_AURA_PERIODIC_DAMAGE, SPELLFAMILY_DRUID, 0x200002, 0, 0))
                        AddPct(DoneTotalMod, (*i)->GetAmount());
                    break;
                }
                case 4418: // Increased Shock Damage
                case 4554: // Increased Lightning Damage
                case 4555: // Improved Moonfire
                case 5142: // Increased Lightning Damage
                case 5147: // Improved Consecration / Libram of Resurgence
                case 5148: // Idol of the Shooting Star
                case 6008: // Increased Lightning Damage
                case 8627: // Totem of Hex
                {
                    DoneTotal += (*i)->GetAmount();
                    break;
                }
            }
        }

        // Custom scripted damage
        switch (spellProto->SpellFamilyName)
        {
            case SPELLFAMILY_ROGUE:
            {
                // Revealing Strike for direct damage abilities
                if (spellProto->AttributesEx & SPELL_ATTR1_REQ_COMBO_POINTS1 && damagetype != DOT)
                    if (AuraEffect* aurEff = victim->GetAuraEffect(84617, 2, GetGUID()))
                        AddPct(DoneTotalMod, aurEff->GetAmount());
                break;
            }
            case SPELLFAMILY_MAGE:
                // Ice Lance
                if (spellProto->SpellIconID == 186)
                {
                    if (victim->HasAuraState(AURA_STATE_FROZEN, spellProto, this))
                        DoneTotalMod *= 4.0f;

                    // search Fingers of Frost
                    if (AuraEffect const* aurEff = GetAuraEffect(44544, EFFECT_1))
                        AddPct(DoneTotalMod, aurEff->GetAmount());
                }

                // Torment the weak
                if (spellProto->GetSchoolMask() & SPELL_SCHOOL_MASK_ARCANE)
                {
                    if (victim->HasAuraWithMechanic((1<<MECHANIC_SNARE)|(1<<MECHANIC_COMBAT_SLOW)))
                    {
                        AuraEffectList mDumyAuras = GetAuraEffectsByType(SPELL_AURA_DUMMY);
                        for (AuraEffectList::const_iterator i = mDumyAuras.begin(); i != mDumyAuras.end(); ++i)
                        {
                            if ((*i)->GetSpellInfo()->SpellIconID == 2215)
                            {
                                AddPct(DoneTotalMod, (*i)->GetAmount());
                                break;
                            }
                        }
                    }
                }
                break;
            case SPELLFAMILY_PRIEST:
            {
                // Smite
                if (spellProto->SpellFamilyFlags[0] & 0x80)
                {
                    // Glyph of Smite
                    if (AuraEffect* aurEff = GetAuraEffect(55692, 0))
                        if (victim->GetAuraEffect(SPELL_AURA_PERIODIC_DAMAGE, SPELLFAMILY_PRIEST, 0x100000, 0, 0, GetGUID()))
                            AddPct(DoneTotalMod, aurEff->GetAmount());
                }
                break;
            }
            case SPELLFAMILY_WARLOCK:
            {
                // Fire and Brimstone
                if (spellProto->SpellFamilyFlags[1] & 0x00020040)
                    if (victim->HasAuraState(AURA_STATE_CONFLAGRATE))
                    {
                        AuraEffectList mDumyAuras = GetAuraEffectsByType(SPELL_AURA_DUMMY);
                        for (AuraEffectList::const_iterator i = mDumyAuras.begin(); i != mDumyAuras.end(); ++i)
                            if ((*i)->GetSpellInfo()->SpellIconID == 3173)
                            {
                                AddPct(DoneTotalMod, (*i)->GetAmount());
                                break;
                            }
                    }
                // Shadow Bite (30% increase from each dot)
                if (spellProto->SpellFamilyFlags[1] & 0x00400000 && isPet())
                    if (uint8 count = victim->GetDoTsByCaster(GetOwnerGUID()))
                        AddPct(DoneTotalMod, 30 * count);

                // Mastery: Emberstorm
                if (spellProto->Id == 17877 || spellProto->Id == 116858)
                {
                    if (AuraEffect const* aurEff = GetAuraEffect(77220, EFFECT_0))
                        AddPct(DoneTotalMod, aurEff->GetAmount());
                }
                break;
            }
            case SPELLFAMILY_DEATHKNIGHT:
                // Sigil of the Vengeful Heart
                if (spellProto->SpellFamilyFlags[0] & 0x2000)
                    if (AuraEffect* aurEff = GetAuraEffect(64962, EFFECT_1))
                        DoneTotal += aurEff->GetAmount();
                break;
        }

        // Done fixed damage bonus auras
        int32 DoneAdvertisedBenefit = GetSpellPowerDamage(spellProto->GetSchoolMask());
        // Pets just add their bonus damage to their spell damage
        // note that their spell damage is just gain of their own auras
        if (HasUnitTypeMask(UNIT_MASK_GUARDIAN) && spellProto->SchoolMask & SPELL_SCHOOL_MASK_MAGIC)
            DoneAdvertisedBenefit += ((Guardian*)this)->GetBonusDamage();

        // Check for table values
        float SPDCoeffMod = spellProto->GetEffect(effIndex, m_diffMode)->BonusMultiplier;
        float ApCoeffMod = spellProto->SpellAPBonusMultiplier;

        SpellBonusEntry const* bonus = sSpellMgr->GetSpellBonusData(spellProto->Id);
        if (bonus)
        {
            SPDCoeffMod = damagetype == DOT ? bonus->dot_damage : bonus->direct_damage;
            ApCoeffMod = damagetype == DOT ? bonus->ap_dot_bonus : bonus->ap_bonus;
        }

		bool calcSPDBonus = (SPDCoeffMod > 0) && getClass() != CLASS_MONK;

        if (ApCoeffMod > 0)
        {
            //code for bonus AP from dbc

            WeaponAttackType attType;

            if (Player* plr = ToPlayer())
                attType = plr->getClass() == CLASS_HUNTER ? RANGED_ATTACK: BASE_ATTACK;
            else
                attType = (spellProto->IsRangedWeaponSpell() && spellProto->DmgClass == SPELL_DAMAGE_CLASS_RANGED) ? RANGED_ATTACK : BASE_ATTACK;

            float APbonus = float(victim->GetTotalAuraModifier(attType == BASE_ATTACK ? SPELL_AURA_MELEE_ATTACK_POWER_ATTACKER_BONUS : SPELL_AURA_RANGED_ATTACK_POWER_ATTACKER_BONUS));
            APbonus += GetTotalAttackPowerValue(attType);

            if (calcSPDBonus)
                calcSPDBonus = DoneAdvertisedBenefit > APbonus;

            if (!calcSPDBonus || spellProto->CasterAuraState == AURA_STATE_JUDGEMENT)
            {
                DoneTotal += int32(stack * ApCoeffMod * APbonus);

                if (damagetype == DOT)
                {
                    sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "Unit::SpellDamageBonusDone DOT DoneTotal %i, APbonus %f", DoneTotal, APbonus);
                }
                else
                    sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "Unit::SpellDamageBonusDone !DOT DoneTotal %i, APbonus %f", DoneTotal, APbonus);
            }
        }
        // Default calculation
        if (calcSPDBonus || spellProto->CasterAuraState == AURA_STATE_JUDGEMENT)
        {
            float factorMod = CalculateLevelPenalty(spellProto);

            if (Player* modOwner = GetSpellModOwner())
            {
                SPDCoeffMod *= 100.0f;
                modOwner->ApplySpellMod(spellProto->Id, SPELLMOD_BONUS_MULTIPLIER, SPDCoeffMod);
                SPDCoeffMod /= 100.0f;
            }
            DoneTotal += int32(DoneAdvertisedBenefit * SPDCoeffMod * factorMod * stack);
        }

        if (getPowerType() == POWER_MANA)
        {
            Unit::AuraEffectList doneFromManaPctAuras = GetAuraEffectsByType(SPELL_AURA_MOD_DAMAGE_DONE_FROM_PCT_POWER);
            if (!doneFromManaPctAuras.empty())
            {
                float powerPct = 100.f * GetPower(POWER_MANA) / GetMaxPower(POWER_MANA);
                for (Unit::AuraEffectList::const_iterator itr = doneFromManaPctAuras.begin(); itr != doneFromManaPctAuras.end(); ++itr)
                {
                    if (spellProto->SchoolMask & (*itr)->GetMiscValue())
                        AddPct(DoneTotalMod, CalculatePct((*itr)->GetAmount(), powerPct));
                }
            }
        }

        // 76613 - Mastery: Frostburn
        if (GetTypeId() == TYPEID_PLAYER && spellProto->SpellFamilyName == SPELLFAMILY_MAGE &&
            getLevel() >= 80 && ToPlayer()->GetSpecializationId(ToPlayer()->GetActiveSpec()) == SPEC_MAGE_FROST)
        {
            if (AuraEffect const* aurEff = GetAuraEffect(76613, EFFECT_0))
                if (victim->isFrozen() && aurEff->IsAffectingSpell(spellProto))
                    AddPct(DoneTotalMod, aurEff->GetAmount());
        }

        // Pyroblast
        if (spellProto && spellProto->Id == 11366)
        {
            // Pyroblast!
            if (HasAura(48108))
                DoneTotalMod *= (100.0f + spellProto->Effects[2].CalcValue()) / 100.0f;
        }

        // Mastery: Master Demonologist
        if (Player* modOwner = GetSpellModOwner())
            if (AuraEffect const* aurEff = modOwner->GetAuraEffect(77219, EFFECT_0))
                AddPct(DoneTotalMod, GetShapeshiftForm() == FORM_METAMORPHOSIS ? aurEff->GetAmount() * 3 : aurEff->GetAmount());

        tmpDamage = (int32(pdamage) + DoneTotal) * DoneTotalMod;

        // apply spellmod to Done damage (flat and pct)
        if (Player* modOwner = GetSpellModOwner())
            modOwner->ApplySpellMod(spellProto->Id, damagetype == DOT ? SPELLMOD_DOT : SPELLMOD_DAMAGE, tmpDamage);

        CalculateFromDummy(victim, tmpDamage, spellProto, (1<<effIndex));

        if (getClass() == CLASS_PALADIN)
            if (RequiresCurrentSpellsToHolyPower(spellProto))
                tmpDamage *= GetModForHolyPowerSpell();
    }

    //sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "SpellDamageBonusDone spellid %u in effIndex %u tmpDamage %f, pdamage %i DoneTotalMod %f DoneTotal %i", spellProto ? spellProto->Id : 0, effIndex, tmpDamage, pdamage, DoneTotalMod, DoneTotal);

    return uint32(std::max(tmpDamage, 0.0f));
}

uint32 Unit::SpellDamageBonusTaken(Unit* caster, SpellInfo const* spellProto, uint32 pdamage)
{
    if (!spellProto)
        return pdamage;

    // Some spells don't benefit from done mods
    if (spellProto->AttributesEx6 & SPELL_ATTR6_NO_DONE_PCT_DAMAGE_MODS)
        return pdamage;

    int32 TakenTotal = 0;
    float TakenTotalMod = 1.0f;
    float TakenTotalCasterMod = 0.0f;
    SpellSchoolMask schoolMask = spellProto->GetSchoolMask();

    // get all auras from caster that allow the spell to ignore resistance (sanctified wrath)
    AuraEffectList IgnoreResistAuras = caster->GetAuraEffectsByType(SPELL_AURA_MOD_IGNORE_TARGET_RESIST);
    for (AuraEffectList::const_iterator i = IgnoreResistAuras.begin(); i != IgnoreResistAuras.end(); ++i)
    {
        if ((*i)->GetMiscValue() & schoolMask)
            TakenTotalCasterMod += (float((*i)->GetAmount()));
    }

    // from positive and negative SPELL_AURA_MOD_DAMAGE_PERCENT_TAKEN
    // multiplicative bonus, for example Dispersion + Shadowform (0.10*0.85=0.085)

    float bestVal = 1.0f;
    bool firstCheck = false;

    for (uint8 i = SPELL_SCHOOL_NORMAL; i < MAX_SPELL_SCHOOL; ++i)
        if (schoolMask & (1 << i))
        {
            float amount = GetTotalAuraMultiplierByMiscMask(SPELL_AURA_MOD_DAMAGE_PERCENT_TAKEN, (1 << i));
            if (bestVal < amount || !firstCheck)
            {
                bestVal = amount;
                firstCheck = true;
            }
        }

    TakenTotalMod *= bestVal;

    // From caster spells
    AuraEffectList mOwnerTaken = GetAuraEffectsByType(SPELL_AURA_MOD_DAMAGE_FROM_CASTER);
    for (AuraEffectList::const_iterator i = mOwnerTaken.begin(); i != mOwnerTaken.end(); ++i)
        if ((*i)->GetCasterGUID() == caster->GetGUID() && (*i)->IsAffectingSpell(spellProto))
            AddPct(TakenTotalMod, (*i)->GetAmount());

    // Mod damage from spell mechanic
    if (uint32 mechanicMask = spellProto->GetAllEffectsMechanicMask())
    {
        AuraEffectList mDamageDoneMechanic = GetAuraEffectsByType(SPELL_AURA_MOD_MECHANIC_DAMAGE_TAKEN_PERCENT);
        for (AuraEffectList::const_iterator i = mDamageDoneMechanic.begin(); i != mDamageDoneMechanic.end(); ++i)
            if (mechanicMask & uint32(1<<((*i)->GetMiscValue())))
                AddPct(TakenTotalMod, (*i)->GetAmount());
    }

    int32 bestVal1 = 0;
    firstCheck = false;

    for (uint8 i = SPELL_SCHOOL_NORMAL; i < MAX_SPELL_SCHOOL; ++i)
        if (schoolMask & (1 << i))
        {
            int32 amount = SpellBaseDamageBonusTaken(SpellSchoolMask(1 << i));
            if (bestVal1 < amount || !firstCheck)
            {
                bestVal1 = amount;
                firstCheck = true;
            }
        }

    TakenTotal += bestVal1;

    float tmpDamage = 0.0f;

    if (TakenTotalCasterMod)
    {
        if (TakenTotal < 0)
        {
            if (TakenTotalMod < 1)
                tmpDamage = ((float(CalculatePct(pdamage, TakenTotalCasterMod) + TakenTotal) * TakenTotalMod) + CalculatePct(pdamage, TakenTotalCasterMod));
            else
                tmpDamage = ((float(CalculatePct(pdamage, TakenTotalCasterMod) + TakenTotal) + CalculatePct(pdamage, TakenTotalCasterMod)) * TakenTotalMod);
        }
        else if (TakenTotalMod < 1)
            tmpDamage = ((CalculatePct(float(pdamage) + TakenTotal, TakenTotalCasterMod) * TakenTotalMod) + CalculatePct(float(pdamage) + TakenTotal, TakenTotalCasterMod));
    }
    if (!tmpDamage)
        tmpDamage = (float(pdamage) + TakenTotal) * TakenTotalMod;

    //sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "SpellDamageBonusTaken spellid %u tmpDamage %f, pdamage %i TakenTotalMod %f TakenTotal %i", spellProto ? spellProto->Id : 0, tmpDamage, pdamage, TakenTotalMod, TakenTotal);

    return uint32(std::max(tmpDamage, 0.0f));
}

uint32 Unit::SpellDamageBonusForDamageBeforeHit(Unit* caster, SpellInfo const* spellProto, uint32 damageBeforeHit)
{
    if (!spellProto)
        return damageBeforeHit;

    if (spellProto->AttributesEx6 & SPELL_ATTR6_NO_DONE_PCT_DAMAGE_MODS)
        return damageBeforeHit;

    damageBeforeHit *= GetTotalPositiveAuraMultiplierByMiscMask(SPELL_AURA_MOD_DAMAGE_PERCENT_TAKEN, spellProto->GetSchoolMask());

    AuraEffectList mOwnerTaken = GetAuraEffectsByType(SPELL_AURA_MOD_DAMAGE_FROM_CASTER);
    for (AuraEffectList::const_iterator i = mOwnerTaken.begin(); i != mOwnerTaken.end(); ++i)
        if ((*i)->GetCasterGUID() == caster->GetGUID() && (*i)->IsAffectingSpell(spellProto))
            if ((*i)->GetAmount() > 0)
                AddPct(damageBeforeHit, (*i)->GetAmount());

    if (uint32 mechanicMask = spellProto->GetAllEffectsMechanicMask())
    {
        AuraEffectList mDamageDoneMechanic = GetAuraEffectsByType(SPELL_AURA_MOD_MECHANIC_DAMAGE_TAKEN_PERCENT);
        for (AuraEffectList::const_iterator i = mDamageDoneMechanic.begin(); i != mDamageDoneMechanic.end(); ++i)
            if (mechanicMask & uint32(1 << ((*i)->GetMiscValue())))
                if ((*i)->GetAmount() > 0)
                    AddPct(damageBeforeHit, (*i)->GetAmount());
    }

    return uint32(std::max(float(damageBeforeHit), 0.0f));
}

int32 Unit::SpellBaseDamageBonusDone(SpellSchoolMask schoolMask, int32 baseBonus)
{
    int32 DoneAdvertisedBenefit = 0;

    AuraEffectList mDamageDone = GetAuraEffectsByType(SPELL_AURA_MOD_DAMAGE_DONE);
    for (AuraEffectList::const_iterator i = mDamageDone.begin(); i != mDamageDone.end(); ++i)
        if (((*i)->GetMiscValue() & schoolMask) != 0 &&
        (*i)->GetSpellInfo()->EquippedItemClass == -1 &&          // -1 == any item class (not wand then)  
        (*i)->GetSpellInfo()->EquippedItemInventoryTypeMask == 0) //  0 == any inventory type (not wand then)            
            DoneAdvertisedBenefit += (*i)->GetAmount();

    if (GetTypeId() == TYPEID_PLAYER)
    {
        // Base value
        DoneAdvertisedBenefit += baseBonus;

        // Spell power from SPELL_AURA_MOD_SPELL_POWER_PCT
        DoneAdvertisedBenefit *= GetTotalAuraMultiplier(SPELL_AURA_MOD_SPELL_POWER_PCT, true);

        // Damage bonus from stats
        AuraEffectList mDamageDoneOfStatPercent = GetAuraEffectsByType(SPELL_AURA_MOD_SPELL_DAMAGE_OF_STAT_PERCENT);
        for (AuraEffectList::const_iterator i = mDamageDoneOfStatPercent.begin(); i != mDamageDoneOfStatPercent.end(); ++i)
        {
            if ((*i)->GetMiscValue() & schoolMask)
            {
                // stat used stored in miscValueB for this aura
                Stats usedStat = Stats((*i)->GetMiscValueB());
                DoneAdvertisedBenefit += int32(CalculatePct(GetStat(usedStat), (*i)->GetAmount()));
            }
        }
    }
    return DoneAdvertisedBenefit;
}

int32 Unit::SpellBaseDamageBonusTaken(SpellSchoolMask schoolMask)
{
    int32 TakenAdvertisedBenefit = 0;

    AuraEffectList mDamageTaken = GetAuraEffectsByType(SPELL_AURA_MOD_DAMAGE_TAKEN);
    for (AuraEffectList::const_iterator i = mDamageTaken.begin(); i != mDamageTaken.end(); ++i)
        if (((*i)->GetMiscValue() & schoolMask) != 0)
            TakenAdvertisedBenefit += (*i)->GetAmount();

    return TakenAdvertisedBenefit;
}

int32 Unit::GetSpellPowerDamage(SpellSchoolMask schoolMask)
{
    int32 SPD = 0;

    switch (getClass())
    {
        case CLASS_HUNTER:
        case CLASS_ROGUE:
        case CLASS_WARRIOR:
        case CLASS_DEATH_KNIGHT:
            return SPD;
        default:
            break;
    }

    if (Player* plr = ToPlayer())
    {
        for (int i = SPELL_SCHOOL_HOLY; i < MAX_SPELL_SCHOOL; ++i)
        {
            int32 val = plr->GetUInt32Value(PLAYER_FIELD_MOD_DAMAGE_DONE_POS+i);
            if (SPD > val || !SPD)
                SPD = val;

            if ((1 << i) & schoolMask && schoolMask != SPELL_SCHOOL_MASK_NORMAL)
                return val;
        }
    }

    return SPD;
}

int32 Unit::GetSpellPowerHealing()
{
    int32 val = 0;

    if (Player* plr = ToPlayer())
        val = plr->GetUInt32Value(PLAYER_FIELD_MOD_HEALING_DONE_POS);

    return val;
}

bool Unit::isSpellCrit(Unit* victim, SpellInfo const* spellProto, SpellSchoolMask schoolMask, WeaponAttackType attackType, float &critChance) const
{
    Unit* owner = GetAnyOwner();
    //! Mobs can't crit with spells. Players, Pets, Totems can
    if (ToCreature() && !(owner && owner->GetTypeId() == TYPEID_PLAYER))
        return false;

    float crit_chance = 0.0f;
    // Pets have 100% of owner's crit_chance
    if (isAnySummons() && owner)
        owner->isSpellCrit(victim, spellProto, schoolMask, attackType, crit_chance);

    switch (spellProto->DmgClass)
    {
        case SPELL_DAMAGE_CLASS_NONE:
        case SPELL_DAMAGE_CLASS_MAGIC:
        {
            if (schoolMask == SPELL_SCHOOL_MASK_ALL)
            {
                float maxCrit = 0.0f;

                for (int8 i = 0; i < MAX_SPELL_SCHOOL; ++i)
                {
                    float _crit = GetFloatValue(PLAYER_SPELL_CRIT_PERCENTAGE1 + i);
                    if (_crit > maxCrit)
                        maxCrit = _crit;
                }
                    
                crit_chance = maxCrit;
            }
            else if (schoolMask & SPELL_SCHOOL_MASK_NORMAL)
                crit_chance = 0.0f;
            // For other schools
            else if (GetTypeId() == TYPEID_PLAYER)
                crit_chance = GetFloatValue(PLAYER_SPELL_CRIT_PERCENTAGE1 + GetFirstSchoolInMask(schoolMask));
            else if(!isAnySummons() || !owner)
            {
                crit_chance = (float)m_baseSpellCritChance;
                crit_chance += GetTotalAuraModifierByMiscMask(SPELL_AURA_MOD_SPELL_CRIT_CHANCE_SCHOOL, schoolMask);
            }
            // taken
            if (victim)
            {
                if (!spellProto->IsPositive())
                {
                    // Modify critical chance by victim SPELL_AURA_MOD_ATTACKER_SPELL_CRIT_CHANCE
                    crit_chance += victim->GetTotalAuraModifierByMiscMask(SPELL_AURA_MOD_ATTACKER_SPELL_CRIT_CHANCE, schoolMask);
                    // Modify critical chance by victim SPELL_AURA_MOD_ATTACKER_SPELL_AND_WEAPON_CRIT_CHANCE
                    crit_chance += victim->GetTotalAuraModifier(SPELL_AURA_MOD_ATTACKER_SPELL_AND_WEAPON_CRIT_CHANCE);
                }
                // scripted (increase crit chance ... against ... target by x%
                AuraEffectList mOverrideClassScript = GetAuraEffectsByType(SPELL_AURA_OVERRIDE_CLASS_SCRIPTS);
                for (AuraEffectList::const_iterator i = mOverrideClassScript.begin(); i != mOverrideClassScript.end(); ++i)
                {
                    if (!((*i)->IsAffectingSpell(spellProto)))
                        continue;
                    int32 modChance = 0;
                    switch ((*i)->GetMiscValue())
                    {
                        // Shatter
                        case  911:
                            if (!victim->HasAuraState(AURA_STATE_FROZEN, spellProto, this))
                                break;
                            crit_chance *= 2; // double the critical chance against frozen targets
                            crit_chance += 50.0f; // plus an additional 50%
                            break;
                        case 7917: // Glyph of Shadowburn
                            if (victim->HasAuraState(AURA_STATE_HEALTHLESS_35_PERCENT, spellProto, this))
                                crit_chance+=(*i)->GetAmount();
                            break;
                        case 7997: // Renewed Hope
                        case 7998:
                            if (victim->HasAura(6788))
                                crit_chance+=(*i)->GetAmount();
                            break;
                        default:
                            break;
                    }
                }
                // Custom crit by class
                switch (spellProto->SpellFamilyName)
                {
                    case SPELLFAMILY_MAGE:
                        // Glyph of Fire Blast
                        if (spellProto->SpellFamilyFlags[0] == 0x2 && spellProto->SpellIconID == 12)
                            if (victim->HasAuraWithMechanic((1<<MECHANIC_STUN) | (1<<MECHANIC_INCAPACITATE)))
                                if (AuraEffect const* aurEff = GetAuraEffect(56369, EFFECT_0))
                                    crit_chance += aurEff->GetAmount();
                        // Inferno Blast
                        if (spellProto->Id == 108853)
                        {
                            critChance = 100.0f;
                            return true;
                        }
                        break;
                    case SPELLFAMILY_PALADIN:
                    {
                        switch (spellProto->Id)
                        {
                            case 25912: // Holy Shock Damage
                            case 25914: // Holy Shock Heal
                            {
                                crit_chance += 25.0f;
                                break;
                            }
                            default:
                                break;
                        }
                        break;
                    }
                    case SPELLFAMILY_DRUID:
                        // Improved Faerie Fire
                        if (victim->HasAuraState(AURA_STATE_FAERIE_FIRE))
                            if (AuraEffect const* aurEff = GetDummyAuraEffect(SPELLFAMILY_DRUID, 109, 0))
                                crit_chance += aurEff->GetAmount();

                        // cumulative effect - don't break

                        // Starfire
                        if (spellProto->SpellFamilyFlags[0] & 0x4 && spellProto->SpellIconID == 1485)
                        {
                            // Improved Insect Swarm
                            if (AuraEffect const* aurEff = GetDummyAuraEffect(SPELLFAMILY_DRUID, 1771, 0))
                                if (victim->GetAuraEffect(SPELL_AURA_PERIODIC_DAMAGE, SPELLFAMILY_DRUID, 0x00000002, 0, 0))
                                    crit_chance += aurEff->GetAmount();
                           break;
                        }
                        // Regrowth
                        if (spellProto->Id == 8936)
                        {
                            // Regrowth has a 60% increased chance for a critical effect.
                            crit_chance += 60.0f;

                            // Glyph of Regrowth
                            if (HasAura(116218))
                            {
                                critChance = 100.0f;
                                return true; // Increases the critical strike chance of your Regrowth by 40%, but removes the periodic component of the spell.
                            }
                        }
                    break;
                    case SPELLFAMILY_SHAMAN:
                        // Lava Burst
                        if (spellProto->Id == 51505 || spellProto->Id == 77451)
                        {
                            critChance = 100.0f;
                            return true;
                        }
                    break;
                    case SPELLFAMILY_WARLOCK:
                    {
                        switch (spellProto->Id)
                        {
                            case 116858: // Chaos Bolt
                            case 104027: // Soul Fire (Metamorphosis)
                            case 6353:   // Soul Fire
                            case 31117:  // Unstable Affliction
                            {
                                critChance = 100.0f;
                                return true;
                            }
                            default:
                                break;
                        }
                        break;
                    }
                }
            }
            break;
        }
        case SPELL_DAMAGE_CLASS_MELEE:
        {
            if (victim)
            {
                crit_chance += GetUnitCriticalChance(attackType, victim);
                crit_chance += GetTotalAuraModifierByMiscMask(SPELL_AURA_MOD_SPELL_CRIT_CHANCE_SCHOOL, schoolMask);
                // Custom crit by class
                switch (spellProto->SpellFamilyName)
                {
                    case SPELLFAMILY_DRUID:
                    {
                        switch (spellProto->Id)
                        {
                            case 22568: // +25% crit chance for Ferocious Bite on bleeding targets
                            {
                                if (victim->HasAuraState(AURA_STATE_BLEEDING))
                                {
                                    crit_chance += 25.0f;
                                }
                                break;
                            }
                            case 6785:   // Ravage
                            case 102545: // Ravage!
                            {
                                if (victim->GetHealthPct() > 80.0f)
                                {
                                    crit_chance += 50.0f; // Ravage has a 50% increased chance to critically strike targets with over 80% health.
                                }
                                break;
                            }
                            default:
                                break;
                        }
                        break;
                    }
                    case SPELLFAMILY_WARRIOR:
                    {
                        // Victory Rush
                        if (spellProto->SpellFamilyFlags[1] & 0x100)
                        {
                            // Glyph of Victory Rush
                            if (AuraEffect const* aurEff = GetAuraEffect(58382, 0))
                                crit_chance += aurEff->GetAmount();
                            break;
                        }
                        switch (spellProto->Id)
                        {
                            case 118000: // Dragon Roar is always a critical hit
                            {
                                critChance = 100.0f;
                                return true;
                            }
                            case 23881: // Bloodthirst has double critical chance
                            {
                                crit_chance *= 2;
                                break;
                            }
                            case 7384: // Overpower
                            {
                                crit_chance += spellProto->Effects[2].BasePoints;
                                break;
                            }
                            default:
                                break;
                        }
                        break;
                    }
                }
            }
            break;
        }
        case SPELL_DAMAGE_CLASS_RANGED:
        {
            if (victim)
            {
                // Ranged Spell (hunters)
                switch (spellProto->Id)
                {
                    case 19434: // Aimed Shot
                    case 82928: // Aimed Shot (Master Marksman)
                    case 56641: // Steady Shot
                        if (HasAura(34483)) // Careful Aim
                            if (victim->GetHealthPct() > 80.0f)
                                crit_chance += 75.0f;
                        break;
                    default:
                        break;
                }
                crit_chance += GetUnitCriticalChance(attackType, victim);
                crit_chance += GetTotalAuraModifierByMiscMask(SPELL_AURA_MOD_SPELL_CRIT_CHANCE_SCHOOL, schoolMask);
            }
            break;
        }
        default:
            return false;
    }

    // not critting spell
    if ((spellProto->AttributesEx2 & SPELL_ATTR2_CANT_CRIT))
        return false;

    if (spellProto->AttributesEx6 & SPELL_ATTR6_NO_DONE_PCT_DAMAGE_MODS)
        if ((spellProto->AttributesEx10 & SPELL_ATTR10_STACK_DAMAGE_OR_HEAL) || GetGUID() == victim->GetGUID())
            return false;

    // percent done
    // only players use intelligence for critical chance computations
    if (Player* modOwner = GetSpellModOwner())
        modOwner->ApplySpellMod(spellProto->Id, SPELLMOD_CRITICAL_CHANCE, crit_chance);

    AuraEffectList critAuras = victim->GetAuraEffectsByType(SPELL_AURA_MOD_CRIT_CHANCE_FOR_CASTER);
    for (AuraEffectList::const_iterator i = critAuras.begin(); i != critAuras.end(); ++i)
        if ((*i)->GetCasterGUID() == GetGUID() && (*i)->IsAffectingSpell(spellProto))
            crit_chance += (*i)->GetAmount();

    CalculateFromDummy(victim, crit_chance, spellProto, 131071, false);

    crit_chance = crit_chance > 0.0f ? crit_chance : 0.0f;
    critChance = crit_chance;

    if (countCrit)
        crit_chance = countCrit;

    if (roll_chance_f(crit_chance))
        return true;
    return false;
}

uint32 Unit::SpellCriticalDamageBonus(SpellInfo const* spellProto, uint32 damage, Unit* victim)
{
    // Calculate critical bonus
    int32 crit_bonus = damage;
    float crit_mod = 0.0f;

    crit_bonus += damage; // 200% for all damage type

    crit_mod += (GetTotalAuraMultiplierByMiscMask(SPELL_AURA_MOD_CRIT_DAMAGE_BONUS, spellProto->GetSchoolMask()) - 1.0f) * 100;

    if (crit_bonus != 0)
        AddPct(crit_bonus, crit_mod);

    crit_bonus -= damage;

    // adds additional damage to crit_bonus (from talents)
    if (Player* modOwner = GetSpellModOwner())
        modOwner->ApplySpellMod(spellProto->Id, SPELLMOD_CRIT_DAMAGE_BONUS, crit_bonus);

    crit_bonus += damage;

    return crit_bonus;
}

uint32 Unit::SpellCriticalHealingBonus(SpellInfo const* /*spellProto*/, uint32 damage, Unit* victim)
{
    // Calculate critical bonus
    int32 crit_bonus = damage;

    damage += crit_bonus;

    damage = int32(float(damage) * GetTotalAuraMultiplier(SPELL_AURA_MOD_CRITICAL_HEALING_AMOUNT));

    return damage;
}

uint32 Unit::SpellHealingBonusDone(Unit* victim, SpellInfo const* spellProto, uint32 healamount, DamageEffectType damagetype, SpellEffIndex effIndex, uint32 stack)
{
    // For totems get healing bonus from owner (statue isn't totem in fact)
    if (GetTypeId() == TYPEID_UNIT && isTotem())
        if (Unit* owner = GetOwner())
            return owner->SpellHealingBonusDone(victim, spellProto, healamount, damagetype, effIndex, stack);

    // Some spells don't benefit from done mods
    if ((spellProto->AttributesEx3 & SPELL_ATTR3_NO_DONE_BONUS) || (spellProto->AttributesEx6 & SPELL_ATTR6_NO_DONE_PCT_DAMAGE_MODS))
        return healamount;

    // No bonus healing for potion spells
    if (spellProto->SpellFamilyName == SPELLFAMILY_POTION)
        return healamount;

    // No bonus for Temporal Ripples or Desperate Prayer or Conductivity
    if (spellProto->Id == 115611 || spellProto->Id == 19236 || spellProto->Id == 118800)
        return healamount;

    // No bonus for Devouring Plague heal or Atonement or Eminence
    if (spellProto->Id == 81751 || spellProto->Id == 117895)
        return healamount;

    // No bonus for Leader of the Pack or Soul Leech or Soul Link heal
    if (spellProto->Id == 108366)
        return healamount;

    // No bonus for Living Seed
    if (spellProto->Id == 48503)
        return healamount;

    // No bonus for Lifebloom : Final heal
    if (spellProto->Id == 33778)
        return healamount;

    // No bonus for Eminence (statue) and Eminence
    if (spellProto->Id == 117895 || spellProto->Id == 126890)
        return healamount;

    float DoneTotalMod = 1.0f;
    int32 DoneTotal = 0;

    // Healing done percent
    AuraEffectList mHealingDonePct = GetAuraEffectsByType(SPELL_AURA_MOD_HEALING_DONE_PERCENT);
    for (AuraEffectList::const_iterator i = mHealingDonePct.begin(); i != mHealingDonePct.end(); ++i)
        AddPct(DoneTotalMod, (*i)->GetAmount());

    if (victim)
    {
        AuraEffectList mHealingFromHealthPct = GetAuraEffectsByType(SPELL_AURA_MOD_HEALING_DONE_FROM_PCT_HEALTH);
        if (!mHealingFromHealthPct.empty())
        {
            float healthPct = std::max(0.0f, 1.0f - float(victim->GetHealth()) / victim->GetMaxHealth());
            for (AuraEffectList::const_iterator i = mHealingFromHealthPct.begin(); i != mHealingFromHealthPct.end(); ++i)
                if ((*i)->IsAffectingSpell(spellProto))
                    DoneTotalMod *= (100.0f + (*i)->GetAmount() * healthPct) / 100.0f;
        }
    }

    if (Map* m_map = GetMap())
        if (!m_map->IsDungeon())
            DoneTotalMod = CalcPvPPower(victim, DoneTotalMod, true);

    // done scripted mod (take it from owner)
    Unit* owner = GetOwner() ? GetOwner() : this;
    AuraEffectList mOverrideClassScript= owner->GetAuraEffectsByType(SPELL_AURA_OVERRIDE_CLASS_SCRIPTS);
    for (AuraEffectList::const_iterator i = mOverrideClassScript.begin(); i != mOverrideClassScript.end(); ++i)
    {
        if (!(*i)->IsAffectingSpell(spellProto))
            continue;
        switch ((*i)->GetMiscValue())
        {
            case 4415: // Increased Rejuvenation Healing
            case 4953:
            case 3736: // Hateful Totem of the Third Wind / Increased Lesser Healing Wave / LK Arena (4/5/6) Totem of the Third Wind / Savage Totem of the Third Wind
                DoneTotal += (*i)->GetAmount();
                break;
            case   21: // Test of Faith
            case 6935:
            case 6918:
                if (victim->HealthBelowPct(50))
                    AddPct(DoneTotalMod, (*i)->GetAmount());
                break;
            default:
                break;
        }
    }
    int32 DoneAdvertisedBenefit = GetSpellPowerHealing();
    int32 bonusDone = GetSpellPowerDamage(spellProto->GetSchoolMask());

    if (HasUnitTypeMask(UNIT_MASK_GUARDIAN))
        DoneAdvertisedBenefit += ((Guardian*)this)->GetBonusDamage();

    if (!DoneAdvertisedBenefit || DoneAdvertisedBenefit < bonusDone)
        DoneAdvertisedBenefit = bonusDone;

    // Check for table values
    SpellBonusEntry const* bonus = sSpellMgr->GetSpellBonusData(spellProto->Id);
    float dbccoeff = spellProto->GetEffect(effIndex, m_diffMode)->BonusMultiplier;
    float ApCoeffMod = spellProto->SpellAPBonusMultiplier;
    float coeff = 0;
    float factorMod = 1.0f;

    if (getClass() == CLASS_DRUID && spellProto->Id == 5185)
        if (HasAura(145162))
        {
            ApCoeffMod = dbccoeff;
            dbccoeff = 0;
        }

    if (bonus)
    {
        if (damagetype == DOT)
        {
            coeff = bonus->dot_damage;
            if (bonus->ap_dot_bonus > 0)
                DoneTotal += int32(bonus->ap_dot_bonus * stack * GetTotalAttackPowerValue(
                    (spellProto->IsRangedWeaponSpell() && spellProto->DmgClass !=SPELL_DAMAGE_CLASS_MELEE) ? RANGED_ATTACK : BASE_ATTACK));
        }
        else
        {
            coeff = bonus->direct_damage;
            if (bonus->ap_bonus > 0)
                DoneTotal += int32(bonus->ap_bonus * stack * GetTotalAttackPowerValue(BASE_ATTACK));
        }
    }
    else
    {
        if (dbccoeff/* && spellProto->SchoolMask & SPELL_SCHOOL_MASK_MAGIC*/)
            coeff = dbccoeff; // 77478 use in SCHOOL_MASK_PHYSICAL

        if (ApCoeffMod)
        {
            DoneTotal += int32(ApCoeffMod * stack * GetTotalAttackPowerValue(
                (spellProto->IsRangedWeaponSpell() && spellProto->DmgClass !=SPELL_DAMAGE_CLASS_MELEE) ? RANGED_ATTACK : BASE_ATTACK));
        }
    }

    // Default calculation
    if (DoneAdvertisedBenefit)
    {
        factorMod *= CalculateLevelPenalty(spellProto);

        if (Player* modOwner = GetSpellModOwner())
        {
            coeff *= 100.0f;
            modOwner->ApplySpellMod(spellProto->Id, SPELLMOD_BONUS_MULTIPLIER, coeff);
            coeff /= 100.0f;
        }

        DoneTotal += int32(DoneAdvertisedBenefit * coeff * factorMod * stack);
    }

    for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
    {
        switch (spellProto->GetEffect(i, GetSpawnMode())->ApplyAuraName)
        {
            // Bonus healing does not apply to these spells
            case SPELL_AURA_PERIODIC_LEECH:
            case SPELL_AURA_PERIODIC_HEALTH_FUNNEL:
                DoneTotal = 0;
                break;
        }
        if (spellProto->GetEffect(i, GetSpawnMode())->Effect == SPELL_EFFECT_HEALTH_LEECH)
            DoneTotal = 0;
    }

    switch (getClass())
    {
        case CLASS_SHAMAN:
        {
            if (Aura* aura = GetAura(118473))
                if (AuraEffect* eff = aura->GetEffect(EFFECT_0))
                {
                    if (spellProto->StartRecoveryTime)
                    {
                        bool singleTarget = false;
                        for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
                            if (spellProto->Effects[i].Effect)
                            {
                                if (!spellProto->Effects[i].HasRadius())
                                    singleTarget = true;
                            }
                            else break;

                        if (singleTarget)
                        {
                            AddPct(DoneTotalMod, eff->GetAmount());
                            RemoveAura(aura->GetId());
                        }
                        break;
                    }
                }
            break;
        }
        default:
            break;
    }

    // use float as more appropriate for negative values and percent applying
    float heal = float(int32(healamount) + DoneTotal) * DoneTotalMod;
    // apply spellmod to Done amount
    if (Player* modOwner = GetSpellModOwner())
        modOwner->ApplySpellMod(spellProto->Id, damagetype == DOT ? SPELLMOD_DOT : SPELLMOD_DAMAGE, heal);

    CalculateFromDummy(victim, heal, spellProto, (1<<effIndex));

    if (getClass() == CLASS_PALADIN)
        if (RequiresCurrentSpellsToHolyPower(spellProto))
            heal *= GetModForHolyPowerSpell();

    return uint32(std::max(heal, 0.0f));
}

uint32 Unit::SpellHealingBonusTaken(Unit* caster, SpellInfo const* spellProto, uint32 healamount, DamageEffectType damagetype, SpellEffIndex effIndex, uint32 stack)
{
    float TakenTotalMod = 1.0f;

    // Some spells don't benefit from done mods
    if ((spellProto->AttributesEx3 & SPELL_ATTR3_NO_DONE_BONUS) || (spellProto->AttributesEx6 & SPELL_ATTR6_NO_DONE_PCT_DAMAGE_MODS))
        return healamount;

    // No bonus
    switch (spellProto->Id)
    {
        // Eminence (statue) and Eminence
        case 117895:
        case 126890:
        //Living Seed
        case 48503:
        //Lifebloom : Final heal
        case 33778:
            return healamount;
    }

    // Healing taken percent
    float ModHealingPct = float(GetTotalAuraMultiplier(SPELL_AURA_MOD_HEALING_PCT));
    if (ModHealingPct != 1.0f)
        TakenTotalMod *= ModHealingPct;

    // Tenacity increase healing % taken
    if (AuraEffect const* Tenacity = GetAuraEffect(58549, 0))
        AddPct(TakenTotalMod, Tenacity->GetAmount());

    // Healing Done
    int32 TakenTotal = 0;

    // Taken fixed damage bonus auras
    int32 TakenAdvertisedBenefit = SpellBaseHealingBonusTaken(spellProto->GetSchoolMask());

    // Nourish heal boost
    if (spellProto->Id == 50464)
    {
        // Heals for an additional 20% if you have a Rejuvenation, Regrowth, Lifebloom, or Wild Growth effect active on the target.
        if (HasAura(48438, caster->GetGUID()) ||   // Wild Growth
            HasAura(33763, caster->GetGUID()) ||   // Lifebloom
            HasAura(8936, caster->GetGUID()) ||    // Regrowth
            HasAura(774, caster->GetGUID()))       // Rejuvenation
            AddPct(TakenTotalMod, 20);
    }

    // Check for table values
    SpellBonusEntry const* bonus = sSpellMgr->GetSpellBonusData(spellProto->Id);
    float dbccoeff = spellProto->GetEffect(effIndex, m_diffMode)->BonusMultiplier;
    float coeff = 0;
    float factorMod = 1.0f;
    if (bonus)
        coeff = (damagetype == DOT) ? bonus->dot_damage : bonus->direct_damage;
    else
    {
        coeff = dbccoeff;
        // No bonus healing for SPELL_DAMAGE_CLASS_NONE class spells by default
        if (spellProto->DmgClass == SPELL_DAMAGE_CLASS_NONE)
        {
            healamount = uint32(std::max((float(healamount) * TakenTotalMod), 0.0f));
            return healamount;
        }
    }

    // Default calculation
    if (TakenAdvertisedBenefit)
    {
        if ((!bonus && !dbccoeff) || coeff < 0)
            coeff = CalculateDefaultCoefficient(spellProto, damagetype) * 1.88f;  // As wowwiki says: C = (Cast Time / 3.5) * 1.88 (for healing spells)

        factorMod *= CalculateLevelPenalty(spellProto);
        if (Player* modOwner = GetSpellModOwner())
        {
            coeff *= 100.0f;
            modOwner->ApplySpellMod(spellProto->Id, SPELLMOD_BONUS_MULTIPLIER, coeff);
            coeff /= 100.0f;
        }

        TakenTotal += int32(TakenAdvertisedBenefit * coeff * factorMod * stack);
    }

    AuraEffectList mHealingGet= GetAuraEffectsByType(SPELL_AURA_MOD_HEALING_RECEIVED);
    for (AuraEffectList::const_iterator i = mHealingGet.begin(); i != mHealingGet.end(); ++i)
    {
        if (caster->GetGUID() == (*i)->GetCasterGUID() && (*i)->IsAffectingSpell(spellProto))
            AddPct(TakenTotalMod, (*i)->GetAmount());
        else if ((*i)->GetBase()->GetId() == 974) // Hack fix for Earth Shield
            AddPct(TakenTotalMod, (*i)->GetAmount());
    }

    AuraEffectList mHotPct = GetAuraEffectsByType(SPELL_AURA_MOD_HOT_PCT);
    for (AuraEffectList::const_iterator i = mHotPct.begin(); i != mHotPct.end(); ++i)
        if (damagetype == DOT)
            AddPct(TakenTotalMod, (*i)->GetAmount());

    for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
    {
        switch (spellProto->GetEffect(i, GetSpawnMode())->ApplyAuraName)
        {
            // Bonus healing does not apply to these spells
            case SPELL_AURA_PERIODIC_LEECH:
            case SPELL_AURA_PERIODIC_HEALTH_FUNNEL:
                TakenTotal = 0;
                break;
        }
        if (spellProto->GetEffect(i, GetSpawnMode())->Effect == SPELL_EFFECT_HEALTH_LEECH)
            TakenTotal = 0;
    }

    float heal = float(int32(healamount) + TakenTotal) * TakenTotalMod;
    return uint32(std::max(heal, 0.0f));
}

float Unit::CalcPvPPower(Unit* target, float amount, bool isHeal)
{
    if (!target)
        return amount;

    Unit* caster = this;

    if (target->GetTypeId() != TYPEID_PLAYER)
        if (Unit* owner = target->GetOwner())
        {
            if (owner->GetTypeId() != TYPEID_PLAYER)
                return amount;
        }
        else return amount;

    if (GetTypeId() != TYPEID_PLAYER)
        if (Unit* owner = GetOwner())
        {
            if (owner->GetTypeId() == TYPEID_PLAYER)
            {
                caster = owner;
            }
            else return amount;
        }
        else return amount;

    if (Player* plr = caster->ToPlayer())
    {
        float PowerPvP = plr->GetFloatValue(isHeal ? PLAYER_PVP_POWER_HEALING: PLAYER_PVP_POWER_DAMAGE);
        AddPct(amount, PowerPvP);
    }

    return amount;
}

int32 Unit::SpellBaseHealingBonusDone(SpellSchoolMask schoolMask, int32 baseBonus)
{
    int32 AdvertisedBenefit = 0;

    AuraEffectList mHealingDone2 = GetAuraEffectsByType(SPELL_AURA_MOD_HEALING_DONE2);
    for (AuraEffectList::const_iterator i = mHealingDone2.begin(); i != mHealingDone2.end(); ++i)
        if (!(*i)->GetMiscValue() || ((*i)->GetMiscValue() & schoolMask) != 0)
            AdvertisedBenefit += (*i)->GetAmount();

    // Healing bonus of spirit, intellect and strength
    if (GetTypeId() == TYPEID_PLAYER)
    {
        // Base value
        AdvertisedBenefit += baseBonus;

        AuraEffectList mHealingDone = GetAuraEffectsByType(SPELL_AURA_MOD_HEALING_DONE);
        for (AuraEffectList::const_iterator i = mHealingDone.begin(); i != mHealingDone.end(); ++i)
            if (!(*i)->GetMiscValue() || ((*i)->GetMiscValue() & schoolMask) != 0)
                AdvertisedBenefit += (*i)->GetAmount();

        AdvertisedBenefit *= GetTotalAuraMultiplier(SPELL_AURA_MOD_SPELL_POWER_PCT, true);

        // Healing bonus from stats
        AuraEffectList mHealingDoneOfStatPercent = GetAuraEffectsByType(SPELL_AURA_MOD_SPELL_HEALING_OF_STAT_PERCENT);
        for (AuraEffectList::const_iterator i = mHealingDoneOfStatPercent.begin(); i != mHealingDoneOfStatPercent.end(); ++i)
        {
            // stat used dependent from misc value (stat index)
            Stats usedStat = Stats((*i)->GetMiscValue());
            AdvertisedBenefit += int32(CalculatePct(GetStat(usedStat), (*i)->GetAmount()));
        }
    }
    return AdvertisedBenefit;
}

int32 Unit::SpellBaseHealingBonusTaken(SpellSchoolMask schoolMask)
{
    int32 AdvertisedBenefit = 0;

    AuraEffectList mDamageTaken = GetAuraEffectsByType(SPELL_AURA_MOD_HEALING);
    for (AuraEffectList::const_iterator i = mDamageTaken.begin(); i != mDamageTaken.end(); ++i)
        if (((*i)->GetMiscValue() & schoolMask) != 0)
            AdvertisedBenefit += (*i)->GetAmount();

    return AdvertisedBenefit;
}

bool Unit::IsImmunedToDamage(SpellSchoolMask shoolMask)
{
    // If m_immuneToSchool type contain this school type, IMMUNE damage.
    SpellImmuneList schoolList = m_spellImmune[IMMUNITY_SCHOOL];
    for (auto itr : schoolList)
        if (itr.type & shoolMask)
            return true;

    // If m_immuneToDamage type contain magic, IMMUNE damage.
    SpellImmuneList damageList = m_spellImmune[IMMUNITY_DAMAGE];
    for (auto itr : damageList)
        if (itr.type & shoolMask)
            return true;

    return false;
}

bool Unit::IsImmunedToDamage(SpellInfo const* spellInfo)
{
    if (spellInfo->Attributes & SPELL_ATTR0_UNAFFECTED_BY_INVULNERABILITY)
        return false;

    uint8 shoolMask = spellInfo->GetSchoolMask();
    uint8 tempMask = shoolMask;

    if (spellInfo->Id != 42292 && spellInfo->Id != 59752)
    {
        // If m_immuneToSchool type contain this school type, IMMUNE damage.
        SpellImmuneList schoolList = m_spellImmune[IMMUNITY_SCHOOL];
        for (SpellImmuneList::const_iterator itr = schoolList.begin(); itr != schoolList.end(); ++itr)
        {
            tempMask &= ~itr->type;
            if (itr->type & shoolMask && !spellInfo->CanPierceImmuneAura(sSpellMgr->GetSpellInfo(itr->spellId)))
            {
                if (!tempMask)
                    return true;
            }
        }
    }

    // If m_immuneToDamage type contain magic, IMMUNE damage.
    SpellImmuneList damageList = m_spellImmune[IMMUNITY_DAMAGE];
    for (SpellImmuneList::const_iterator itr = damageList.begin(); itr != damageList.end(); ++itr)
    {
        tempMask &= ~itr->type;
        if ((itr->type & shoolMask) && !tempMask)
            return true;
    }

    return false;
}

bool Unit::IsImmunedToSpell(SpellInfo const* spellInfo)
{
    if (!spellInfo)
        return false;

    // Single spell immunity.
    SpellImmuneList idList = m_spellImmune[IMMUNITY_ID];
    for (SpellImmuneList::const_iterator itr = idList.begin(); itr != idList.end(); ++itr)
        if (itr->type == spellInfo->Id)
            return true;

    if (spellInfo->Attributes & SPELL_ATTR0_UNAFFECTED_BY_INVULNERABILITY)
        return false;

    if (spellInfo->Dispel)
    {
        SpellImmuneList dispelList = m_spellImmune[IMMUNITY_DISPEL];
        for (SpellImmuneList::const_iterator itr = dispelList.begin(); itr != dispelList.end(); ++itr)
            if (itr->type == spellInfo->Dispel)
                return true;
    }

    // Spells that don't have effectMechanics.
    if (spellInfo->Mechanic)
    {
        SpellImmuneList mechanicList = m_spellImmune[IMMUNITY_MECHANIC];
        for (SpellImmuneList::const_iterator itr = mechanicList.begin(); itr != mechanicList.end(); ++itr)
            if (itr->type == spellInfo->Mechanic)
                return true;
    }

    bool immuneToAllEffects = true;
    for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
    {
        // State/effect immunities applied by aura expect full spell immunity
        // Ignore effects with mechanic, they are supposed to be checked separately
        if (!spellInfo->GetEffect(i, GetSpawnMode())->IsEffect())
            continue;
        if (!IsImmunedToSpellEffect(spellInfo, i))
        {
            immuneToAllEffects = false;
            break;
        }
    }

    if (immuneToAllEffects) //Return immune only if the target is immune to all spell effects.
        return true;

    if (spellInfo->Id != 42292 && spellInfo->Id != 59752)
    {
        uint8 mask = spellInfo->GetSchoolMask();
        SpellImmuneList schoolList = m_spellImmune[IMMUNITY_SCHOOL];
        for (SpellImmuneList::const_iterator itr = schoolList.begin(), next; itr != schoolList.end(); itr = next)
        {
            next = itr;
            ++next;
            mask &= ~itr->type;
            SpellInfo const* immuneSpellInfo = sSpellMgr->GetSpellInfo(itr->spellId);
            if ((itr->type & spellInfo->GetSchoolMask())
                && !(immuneSpellInfo && immuneSpellInfo->IsPositive() && spellInfo->IsPositive())
                && !spellInfo->CanPierceImmuneAura(immuneSpellInfo))
            {
                if (!mask)
                    return true;
            }
        }
    }

    return false;
}

bool Unit::IsImmunedToSpellEffect(SpellInfo const* spellInfo, uint32 index) const
{
    if (!spellInfo || !spellInfo->GetEffect(index, GetSpawnMode())->IsEffect())
        return false;

    // If m_immuneToEffect type contain this effect type, IMMUNE effect.
    uint32 effect = spellInfo->GetEffect(index, GetSpawnMode())->Effect;
    SpellImmuneList effectList = m_spellImmune[IMMUNITY_EFFECT];
    for (SpellImmuneList::const_iterator itr = effectList.begin(), next; itr != effectList.end(); itr = next)
    {
        next = itr;
        ++next;
        if (itr->type == effect)
            return true;
    }

    if (uint32 mechanic = spellInfo->GetEffect(index, GetSpawnMode())->Mechanic)
    {
        SpellImmuneList mechanicList = m_spellImmune[IMMUNITY_MECHANIC];
        for (SpellImmuneList::const_iterator itr = mechanicList.begin(), next; itr != mechanicList.end(); itr = next)
        {
            next = itr;
            ++next;
            if (itr->type == mechanic)
                return true;
        }
    }

    if (uint32 aura = spellInfo->GetEffect(index, GetSpawnMode())->ApplyAuraName)
    {
        SpellImmuneList list = m_spellImmune[IMMUNITY_STATE];
        for (SpellImmuneList::const_iterator itr = list.begin(), next; itr != list.end(); itr = next)
        {
            next = itr;
            ++next;
            if (itr->type == aura)
                if (!(spellInfo->AttributesEx3 & SPELL_ATTR3_IGNORE_HIT_RESULT))
                    return true;
        }
        // Check for immune to application of harmful magical effects
        AuraEffectList immuneAuraApply = GetAuraEffectsByType(SPELL_AURA_MOD_IMMUNE_AURA_APPLY_SCHOOL);
        for (AuraEffectList::const_iterator iter = immuneAuraApply.begin(); iter != immuneAuraApply.end(); ++iter)
        if (((*iter)->GetMiscValue() & spellInfo->GetSchoolMask()) && !spellInfo->IsPositiveEffect(index))        // Harmful && Magic effects
                return true;

        AuraEffectList immuneMechanicAuraApply = GetAuraEffectsByType(SPELL_AURA_MECHANIC_IMMUNITY_MASK);
        for(AuraEffectList::const_iterator i = immuneMechanicAuraApply.begin(); i != immuneMechanicAuraApply.end(); ++i)
            if(spellInfo->GetEffect(index, GetSpawnMode())->Mechanic && spellInfo->Mechanic && ((1 << (spellInfo->GetEffect(index, GetSpawnMode())->Mechanic)) & (*i)->GetMiscValue() ||
            (1 << (spellInfo->Mechanic)) & (*i)->GetMiscValue()))
                return true;
    }

    return false;
}

uint32 Unit::MeleeDamageBonusDone(Unit* victim, uint32 pdamage, WeaponAttackType attType, SpellInfo const* spellProto, uint32 effectMask)
{
    if (!victim || pdamage == 0)
        return 0;

    if (spellProto && (spellProto->AttributesEx6 & SPELL_ATTR6_NO_DONE_PCT_DAMAGE_MODS))
        return pdamage;

    uint32 creatureTypeMask = victim->GetCreatureTypeMask();

    // Done fixed damage bonus auras
    int32 DoneFlatBenefit = 0;

    // ..done
    AuraEffectList mDamageDoneCreature = GetAuraEffectsByType(SPELL_AURA_MOD_DAMAGE_DONE_CREATURE);
    for (AuraEffectList::const_iterator i = mDamageDoneCreature.begin(); i != mDamageDoneCreature.end(); ++i)
        if (creatureTypeMask & uint32((*i)->GetMiscValue()))
            DoneFlatBenefit += (*i)->GetAmount();

    // ..done
    // SPELL_AURA_MOD_DAMAGE_DONE included in weapon damage

    // ..done (base at attack power for marked target and base at attack power for creature type)
    int32 APbonus = 0;

    if (attType == RANGED_ATTACK)
    {
        APbonus += victim->GetTotalAuraModifier(SPELL_AURA_RANGED_ATTACK_POWER_ATTACKER_BONUS);

        // ..done (base at attack power and creature type)
        AuraEffectList mCreatureAttackPower = GetAuraEffectsByType(SPELL_AURA_MOD_RANGED_ATTACK_POWER_VERSUS);
        for (AuraEffectList::const_iterator i = mCreatureAttackPower.begin(); i != mCreatureAttackPower.end(); ++i)
            if (creatureTypeMask & uint32((*i)->GetMiscValue()))
                APbonus += (*i)->GetAmount();
    }
    else
    {
        APbonus += victim->GetTotalAuraModifier(SPELL_AURA_MELEE_ATTACK_POWER_ATTACKER_BONUS);

        // ..done (base at attack power and creature type)
        AuraEffectList mCreatureAttackPower = GetAuraEffectsByType(SPELL_AURA_MOD_MELEE_ATTACK_POWER_VERSUS);
        for (AuraEffectList::const_iterator i = mCreatureAttackPower.begin(); i != mCreatureAttackPower.end(); ++i)
            if (creatureTypeMask & uint32((*i)->GetMiscValue()))
                APbonus += (*i)->GetAmount();
    }

    if (APbonus != 0)                                       // Can be negative
    {
        bool normalized = false;
        if (spellProto)
            for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
                if (spellProto->GetEffect(i, GetSpawnMode())->Effect == SPELL_EFFECT_NORMALIZED_WEAPON_DMG)
                {
                    normalized = true;
                    break;
                }
        DoneFlatBenefit += int32(APbonus/14.0f * GetAPMultiplier(attType, normalized));
    }

    // Done total percent damage auras
    float DoneTotalMod = 1.0f;

    // Apply PowerPvP damage bonus
    DoneTotalMod = CalcPvPPower(victim, DoneTotalMod);

    // Some spells don't benefit from pct done mods
    if (spellProto)
    {
        AuraEffectList mModDamagePercentDone = GetTotalNotStuckAuraEffectByType(SPELL_AURA_MOD_DAMAGE_PERCENT_DONE);
        for (AuraEffectList::const_iterator i = mModDamagePercentDone.begin(); i != mModDamagePercentDone.end(); ++i)
        {
            // Mastery: Unshackled Fury
            if ((*i)->GetId() == 76856 && !HasAuraState(AURA_STATE_ENRAGE))
                continue;

            if ((*i)->GetMiscValue() & spellProto->GetSchoolMask() && !(spellProto->GetSchoolMask() & SPELL_SCHOOL_MASK_NORMAL))
            {
                if ((*i)->GetSpellInfo()->EquippedItemClass == -1)
                    AddPct(DoneTotalMod, (*i)->GetAmount());
                else if (!((*i)->GetSpellInfo()->AttributesEx5 & SPELL_ATTR5_SPECIAL_ITEM_CLASS_CHECK) && ((*i)->GetSpellInfo()->EquippedItemSubClassMask == 0))
                    AddPct(DoneTotalMod, (*i)->GetAmount());
                else if (ToPlayer() && ToPlayer()->HasItemFitToSpellRequirements((*i)->GetSpellInfo()))
                    AddPct(DoneTotalMod, (*i)->GetAmount());
            }
        }
    }

    AuraEffectList mDamageDoneVersus = GetAuraEffectsByType(SPELL_AURA_MOD_DAMAGE_DONE_VERSUS);
    for (AuraEffectList::const_iterator i = mDamageDoneVersus.begin(); i != mDamageDoneVersus.end(); ++i)
        if (creatureTypeMask & uint32((*i)->GetMiscValue()))
            AddPct(DoneTotalMod, (*i)->GetAmount());

    // bonus against aurastate
    AuraEffectList mDamageDoneVersusAurastate = GetAuraEffectsByType(SPELL_AURA_MOD_DAMAGE_DONE_VERSUS_AURASTATE);
    for (AuraEffectList::const_iterator i = mDamageDoneVersusAurastate.begin(); i != mDamageDoneVersusAurastate.end(); ++i)
        if (victim->HasAuraState(AuraStateType((*i)->GetMiscValue())))
            if (HasAura(144421) && GetPower(POWER_ALTERNATE_POWER))
            {
                int32 pos = GetPower(POWER_ALTERNATE_POWER);
                int32 mod = 0;
                switch (pos)
                {
                case 25:
                    mod = -10;
                    break;
                case 50:
                    mod = -25;
                    break;
                case 75:
                    mod = -50;
                    break;
                case 100:
                    mod = -75;
                    break;
                default:
                    mod = 0;
                    break;
                }
                AddPct(DoneTotalMod, mod);
            }
            else
                AddPct(DoneTotalMod, (*i)->GetAmount());

    // Add SPELL_AURA_MOD_DAMAGE_DONE_FOR_MECHANIC percent bonus
    if (spellProto)
        AddPct(DoneTotalMod, GetTotalAuraModifierByMiscValue(SPELL_AURA_MOD_DAMAGE_DONE_FOR_MECHANIC, spellProto->Mechanic));

    // Add SPELL_AURA_MOD_AUTOATTACK_DAMAGE percent bonus
    if (!spellProto)
    {
        AuraEffectList mAutoAttacksDamageBonus = GetAuraEffectsByType(SPELL_AURA_MOD_AUTOATTACK_DAMAGE);
        for (AuraEffectList::const_iterator i = mAutoAttacksDamageBonus.begin(); i != mAutoAttacksDamageBonus.end(); ++i)
            AddPct(DoneTotalMod, (*i)->GetAmount());
    }

    // done scripted mod (take it from owner)
    Unit* owner = GetOwner() ? GetOwner() : this;
    // AuraEffectList mOverrideClassScript = owner->GetAuraEffectsByType(SPELL_AURA_OVERRIDE_CLASS_SCRIPTS);

    float tmpDamage = float(int32(pdamage) + DoneFlatBenefit) * DoneTotalMod;

    // apply spellmod to Done damage
    if (spellProto)
    {
        if (Player* modOwner = GetSpellModOwner())
            modOwner->ApplySpellMod(spellProto->Id, SPELLMOD_DAMAGE, tmpDamage);
        CalculateFromDummy(victim, tmpDamage, spellProto, effectMask);
    }

    if (Player* modOwner = GetSpellModOwner())
    {
        // Mastery: Master Demonologist
        if (AuraEffect const* aurEff = modOwner->GetAuraEffect(77219, EFFECT_0))
            AddPct(DoneTotalMod, GetShapeshiftForm() == FORM_METAMORPHOSIS ? aurEff->GetAmount() * 3 : aurEff->GetAmount());
    }

    //sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "MeleeDamageBonusDone spellid %u in tmpDamage %f, pdamage %i DoneTotalMod %f DoneFlatBenefit %i", spellProto ? spellProto->Id : 0, tmpDamage, pdamage, DoneTotalMod, DoneFlatBenefit);

    // bonus result can be negative
    return uint32(std::max(tmpDamage, 0.0f));
}

uint32 Unit::MeleeDamageBonusTaken(Unit* attacker, uint32 pdamage, WeaponAttackType attType, SpellInfo const* spellProto)
{
    if (pdamage == 0)
        return 0;

    int32 TakenFlatBenefit = 0;
    float TakenTotalCasterMod = 0.0f;

    // get all auras from caster that allow the spell to ignore resistance (sanctified wrath)
    SpellSchoolMask attackSchoolMask = spellProto ? spellProto->GetSchoolMask() : SPELL_SCHOOL_MASK_NORMAL;
    AuraEffectList IgnoreResistAuras = attacker->GetAuraEffectsByType(SPELL_AURA_MOD_IGNORE_TARGET_RESIST);
    for (AuraEffectList::const_iterator i = IgnoreResistAuras.begin(); i != IgnoreResistAuras.end(); ++i)
    {
        if ((*i)->GetMiscValue() & attackSchoolMask)
            TakenTotalCasterMod += (float((*i)->GetAmount()));
    }

    // ..taken
    AuraEffectList mDamageTaken = GetAuraEffectsByType(SPELL_AURA_MOD_DAMAGE_TAKEN);
    for (AuraEffectList::const_iterator i = mDamageTaken.begin(); i != mDamageTaken.end(); ++i)
        if ((*i)->GetMiscValue() & GetMeleeDamageSchoolMask())
            TakenFlatBenefit += (*i)->GetAmount();

    if (attType != RANGED_ATTACK)
        TakenFlatBenefit += GetTotalAuraModifier(SPELL_AURA_MOD_MELEE_DAMAGE_TAKEN);
    else
        TakenFlatBenefit += GetTotalAuraModifier(SPELL_AURA_MOD_RANGED_DAMAGE_TAKEN);

    // Taken total percent damage auras
    float TakenTotalMod = 1.0f;

    // ..taken
    TakenTotalMod *= GetTotalAuraMultiplierByMiscMask(SPELL_AURA_MOD_DAMAGE_PERCENT_TAKEN, GetMeleeDamageSchoolMask());

    // .. taken pct (special attacks)
    if (spellProto)
    {
        // From caster spells
        AuraEffectList mOwnerTaken = GetAuraEffectsByType(SPELL_AURA_MOD_DAMAGE_FROM_CASTER);
        for (AuraEffectList::const_iterator i = mOwnerTaken.begin(); i != mOwnerTaken.end(); ++i)
            if ((*i)->GetCasterGUID() == attacker->GetGUID() && (*i)->IsAffectingSpell(spellProto))
                AddPct(TakenTotalMod, (*i)->GetAmount());

        // Mod damage from spell mechanic
        uint32 mechanicMask = spellProto->GetAllEffectsMechanicMask();

        // Shred, Maul - "Effects which increase Bleed damage also increase Shred damage"
        if (spellProto->SpellFamilyName == SPELLFAMILY_DRUID && spellProto->SpellFamilyFlags[0] & 0x00008800)
            mechanicMask |= (1<<MECHANIC_BLEED);

        if (mechanicMask)
        {
            AuraEffectList mDamageDoneMechanic = GetAuraEffectsByType(SPELL_AURA_MOD_MECHANIC_DAMAGE_TAKEN_PERCENT);
            for (AuraEffectList::const_iterator i = mDamageDoneMechanic.begin(); i != mDamageDoneMechanic.end(); ++i)
                if (mechanicMask & uint32(1<<((*i)->GetMiscValue())))
                    AddPct(TakenTotalMod, (*i)->GetAmount());
        }
    }
    else
    {
        AuraEffectList mMeleeDamageFromCaster = GetAuraEffectsByType(SPELL_AURA_MOD_AUTOATTACK_DAMAGE_TARGET);
        for (AuraEffectList::const_iterator i = mMeleeDamageFromCaster.begin(); i != mMeleeDamageFromCaster.end(); ++i)
            if ((*i)->GetCasterGUID() == attacker->GetGUID())
                AddPct(TakenTotalMod, (*i)->GetAmount());
    }

    // .. taken pct: class scripts
    //*AuraEffectList mclassScritAuras = GetAuraEffectsByType(SPELL_AURA_OVERRIDE_CLASS_SCRIPTS);
    //for (AuraEffectList::const_iterator i = mclassScritAuras.begin(); i != mclassScritAuras.end(); ++i)
    //{
    //    switch ((*i)->GetMiscValue())
    //    {
    //    }
    //}*/

    if (attType != RANGED_ATTACK)
    {
        AuraEffectList mModMeleeDamageTakenPercent = GetAuraEffectsByType(SPELL_AURA_MOD_MELEE_DAMAGE_TAKEN_PCT);
        for (AuraEffectList::const_iterator i = mModMeleeDamageTakenPercent.begin(); i != mModMeleeDamageTakenPercent.end(); ++i)
            AddPct(TakenTotalMod, (*i)->GetAmount());
    }
    else
    {
        AuraEffectList mModRangedDamageTakenPercent = GetAuraEffectsByType(SPELL_AURA_MOD_RANGED_DAMAGE_TAKEN_PCT);
        for (AuraEffectList::const_iterator i = mModRangedDamageTakenPercent.begin(); i != mModRangedDamageTakenPercent.end(); ++i)
            AddPct(TakenTotalMod, (*i)->GetAmount());
    }

    float tmpDamage = 0.0f;

    if (TakenTotalCasterMod)
    {
        if (TakenFlatBenefit < 0)
        {
            if (TakenTotalMod < 1)
                tmpDamage = ((float(CalculatePct(pdamage, TakenTotalCasterMod) + TakenFlatBenefit) * TakenTotalMod) + CalculatePct(pdamage, TakenTotalCasterMod));
            else
                tmpDamage = ((float(CalculatePct(pdamage, TakenTotalCasterMod) + TakenFlatBenefit) + CalculatePct(pdamage, TakenTotalCasterMod)) * TakenTotalMod);
        }
        else if (TakenTotalMod < 1)
            tmpDamage = ((CalculatePct(float(pdamage) + TakenFlatBenefit, TakenTotalCasterMod) * TakenTotalMod) + CalculatePct(float(pdamage) + TakenFlatBenefit, TakenTotalCasterMod));
    }
    if (!tmpDamage)
        tmpDamage = (float(pdamage) + TakenFlatBenefit) * TakenTotalMod;

    //sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "MeleeDamageBonusTaken spellid %u in tmpDamage %f, pdamage %i TakenTotalMod %f TakenFlatBenefit %i", spellProto ? spellProto->Id : 0, tmpDamage, pdamage, TakenTotalMod, TakenFlatBenefit);

    // bonus result can be negative
    return uint32(std::max(tmpDamage, 0.0f));
}

uint32 Unit::MeleeDamageBonusForDamageBeforeHit(Unit* attacker, uint32 damageBeforeHit, WeaponAttackType attType, SpellInfo const* spellProto)
{
    if (damageBeforeHit == 0)
        return 0;

    damageBeforeHit *= GetTotalPositiveAuraMultiplierByMiscMask(SPELL_AURA_MOD_DAMAGE_PERCENT_TAKEN, GetMeleeDamageSchoolMask());

    if (spellProto)
    {
        AuraEffectList mOwnerTaken = GetAuraEffectsByType(SPELL_AURA_MOD_DAMAGE_FROM_CASTER);
        for (AuraEffectList::const_iterator i = mOwnerTaken.begin(); i != mOwnerTaken.end(); ++i)
            if ((*i)->GetCasterGUID() == attacker->GetGUID() && (*i)->IsAffectingSpell(spellProto))
                if ((*i)->GetAmount() > 0)
                    AddPct(damageBeforeHit, (*i)->GetAmount());

        uint32 mechanicMask = spellProto->GetAllEffectsMechanicMask();

        if (mechanicMask)
        {
            AuraEffectList mDamageDoneMechanic = GetAuraEffectsByType(SPELL_AURA_MOD_MECHANIC_DAMAGE_TAKEN_PERCENT);
            for (AuraEffectList::const_iterator i = mDamageDoneMechanic.begin(); i != mDamageDoneMechanic.end(); ++i)
                if (mechanicMask & uint32(1 << ((*i)->GetMiscValue())))
                    if ((*i)->GetAmount() > 0)
                        AddPct(damageBeforeHit, (*i)->GetAmount());
        }
    }
    else
    {
        AuraEffectList mMeleeDamageFromCaster = GetAuraEffectsByType(SPELL_AURA_MOD_AUTOATTACK_DAMAGE_TARGET);
        for (AuraEffectList::const_iterator i = mMeleeDamageFromCaster.begin(); i != mMeleeDamageFromCaster.end(); ++i)
            if ((*i)->GetCasterGUID() == attacker->GetGUID())
                if ((*i)->GetAmount() > 0)
                    AddPct(damageBeforeHit, (*i)->GetAmount());
    }

    if (attType != RANGED_ATTACK)
    {
        AuraEffectList mModMeleeDamageTakenPercent = GetAuraEffectsByType(SPELL_AURA_MOD_MELEE_DAMAGE_TAKEN_PCT);
        for (AuraEffectList::const_iterator i = mModMeleeDamageTakenPercent.begin(); i != mModMeleeDamageTakenPercent.end(); ++i)
            if ((*i)->GetAmount() > 0)
                AddPct(damageBeforeHit, (*i)->GetAmount());
    }
    else
    {
        AuraEffectList mModRangedDamageTakenPercent = GetAuraEffectsByType(SPELL_AURA_MOD_RANGED_DAMAGE_TAKEN_PCT);
        for (AuraEffectList::const_iterator i = mModRangedDamageTakenPercent.begin(); i != mModRangedDamageTakenPercent.end(); ++i)
            if ((*i)->GetAmount() > 0)
                AddPct(damageBeforeHit, (*i)->GetAmount());
    }

    return uint32(std::max(float(damageBeforeHit), 0.0f));
}

void Unit::ApplyUberImmune(uint32 spellid, bool apply)
{
    if (apply)
        RemoveAurasWithMechanic(IMMUNE_TO_MOVEMENT_IMPAIRMENT_AND_LOSS_CONTROL_MASK, AURA_REMOVE_BY_DEFAULT, spellid);
    for (uint32 mech=MECHANIC_CHARM; mech!=MECHANIC_ENRAGED; ++mech)
    {
        if (mech == MECHANIC_DISARM)
            continue;
        if (1<<mech & IMMUNE_TO_MOVEMENT_IMPAIRMENT_AND_LOSS_CONTROL_MASK)
            ApplySpellImmune(spellid, IMMUNITY_MECHANIC, mech, apply);
    }
}

void Unit::ApplySpellImmune(uint32 spellId, uint32 op, uint32 type, bool apply)
{
    if (apply)
    {
        for (SpellImmuneList::iterator itr = m_spellImmune[op].begin(), next; itr != m_spellImmune[op].end(); itr = next)
        {
            next = itr; ++next;
            if (itr->spellId == spellId && itr->type == type)
            {
                m_spellImmune[op].erase(itr);
                next = m_spellImmune[op].begin();
            }
        }
        SpellImmune Immune;
        Immune.spellId = spellId;
        Immune.type = type;
        m_spellImmune[op].push_back(Immune);
    }
    else
    {
        for (SpellImmuneList::iterator itr = m_spellImmune[op].begin(); itr != m_spellImmune[op].end(); ++itr)
        {
            if (itr->spellId == spellId && itr->type == type)
            {
                m_spellImmune[op].erase(itr);
                break;
            }
        }
    }
}

void Unit::ApplySpellDispelImmunity(const SpellInfo* spellProto, DispelType type, bool apply)
{
    ApplySpellImmune(spellProto->Id, IMMUNITY_DISPEL, type, apply);

    if (apply && spellProto->AttributesEx & SPELL_ATTR1_DISPEL_AURAS_ON_IMMUNITY)
    {
        // Create dispel mask by dispel type
        uint32 dispelMask = SpellInfo::GetDispelMask(type);
        // Dispel all existing auras vs current dispel type
        AuraApplicationMap& auras = GetAppliedAuras();
        for (AuraApplicationMap::iterator itr = auras.begin(); itr != auras.end();)
        {
            SpellInfo const* spell = itr->second->GetBase()->GetSpellInfo();
            if (spell->GetDispelMask() & dispelMask)
            {
                // Dispel aura
                RemoveAura(itr);
            }
            else
                ++itr;
        }
    }
}

float Unit::GetWeaponProcChance() const
{
    // normalized proc chance for weapon attack speed
    // (odd formula...)
    if (isAttackReady(BASE_ATTACK))
        return (GetAttackTime(BASE_ATTACK) * 1.8f / 1000.0f);
    else if (haveOffhandWeapon() && isAttackReady(OFF_ATTACK))
        return (GetAttackTime(OFF_ATTACK) * 1.6f / 1000.0f);
    return 0;
}

float Unit::GetPPMProcChance(uint32 WeaponSpeed, float PPM, const SpellInfo* spellProto) const
{
    // proc per minute chance calculation
    if (PPM <= 0)
        return 0.0f;

    // Apply chance modifer aura
    if (spellProto)
        if (Player* modOwner = GetSpellModOwner())
            modOwner->ApplySpellMod(spellProto->Id, SPELLMOD_PROC_PER_MINUTE, PPM);

    return floor((WeaponSpeed * PPM) / 600.0f);   // result is chance in percents (probability = Speed_in_sec * (PPM / 60))
}

void Unit::Mount(uint32 mount, uint32 VehicleId, uint32 creatureEntry)
{
    if (mount)
        SetUInt32Value(UNIT_FIELD_MOUNTDISPLAYID, mount);

    m_onMount = true;

    if (Player* player = ToPlayer())
    {
        // mount as a vehicle
        if (VehicleId)
        {
            if (CreateVehicleKit(VehicleId, creatureEntry))
            {
                // Send others that we now have a vehicle
                WorldPacket data(SMSG_PLAYER_VEHICLE_DATA, 8 + 1 + 4);
                data << uint32(VehicleId);
                data.WriteGuidMask<5, 3, 6, 2, 1, 4, 0, 7>(GetObjectGuid());
                data.WriteGuidBytes<6, 0, 1, 3, 5, 7, 2, 4>(GetObjectGuid());
                SendMessageToSet(&data, true);

                data.Initialize(SMSG_ON_CANCEL_EXPECTED_RIDE_VEHICLE_AURA, 0);
                player->GetSession()->SendPacket(&data);

                // mounts can also have accessories
                GetVehicleKit()->InstallAllAccessories(false);
            }
        }

        // don't unsummon pet but SetFlag UNIT_FLAG_STUNNED to disable pet's interface
        if (Pet* pet = player->GetPet())
            pet->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_STUNNED);

        player->SendMovementSetCollisionHeight(player->GetCollisionHeight(true), mount);
    }

    RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_MOUNT);
}

void Unit::Dismount()
{
    if (!IsMounted())
        return;

    SetUInt32Value(UNIT_FIELD_MOUNTDISPLAYID, 0);
    m_onMount = false;

    if (Player* thisPlayer = ToPlayer())
        thisPlayer->SendMovementSetCollisionHeight(thisPlayer->GetCollisionHeight(false));

    WorldPacket data(SMSG_DISMOUNT, 8 + 1);
    data.WriteGuidMask<2, 3, 7, 1, 6, 4, 0, 5>(GetObjectGuid());
    data.WriteGuidBytes<5, 7, 2, 0, 3, 1, 4, 6>(GetObjectGuid());
    SendMessageToSet(&data, true);

    // dismount as a vehicle
    if (GetTypeId() == TYPEID_PLAYER && GetVehicleKit())
    {
        // Send other players that we are no longer a vehicle
        data.Initialize(SMSG_PLAYER_VEHICLE_DATA, 8 + 4 + 1);
        data << uint32(0);
        data.WriteGuidMask<5, 3, 6, 2, 1, 4, 0, 7>(GetObjectGuid());
        data.WriteGuidBytes<6, 0, 1, 3, 5, 7, 2, 4>(GetObjectGuid());
        ToPlayer()->SendMessageToSet(&data, true);
        // Remove vehicle from player
        RemoveVehicleKit();
    }

    RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_NOT_MOUNTED);

    // only resummon old pet if the player is already added to a map
    // this prevents adding a pet to a not created map which would otherwise cause a crash
    // (it could probably happen when logging in after a previous crash)
    if (Player* player = ToPlayer())
    {
        if (Pet* pPet = player->GetPet())
        {
            if (pPet->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_STUNNED) && !pPet->HasUnitState(UNIT_STATE_STUNNED))
                pPet->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_STUNNED);
        }
        else
            player->ResummonPetTemporaryUnSummonedIfAny();
    }
}

void Unit::UpdateMount()
{
    MountCapabilityEntry const* newMount = NULL;
    AuraEffect* effect = NULL;

    // First get the mount type
    MountTypeEntry const* mountType = NULL;
    {
        AuraEffectList auras = GetAuraEffectsByType(SPELL_AURA_MOUNTED);
        for (AuraEffectList::const_reverse_iterator itr = auras.rbegin(); itr != auras.rend(); ++itr)
        {
            AuraEffect* aura = *itr;
            aura->GetMiscValueB();
            mountType = sMountTypeStore.LookupEntry(uint32(aura->GetMiscValueB()));
            if (mountType)
            {
                effect = aura;
                break;
            }
        }
    }

    if (mountType)
    {
        uint32 zoneId, areaId;
        GetZoneAndAreaId(zoneId, areaId);

        uint32 ridingSkill = 5000;
        if (GetTypeId() == TYPEID_PLAYER)
            ridingSkill = ToPlayer()->GetSkillValue(SKILL_RIDING);

        // Find the currently allowed mount flags
        uint32 currentMountFlags;
        {
            AuraEffectList auras = GetAuraEffectsByType(SPELL_AURA_MOD_FLYING_RESTRICTIONS);
            if (!auras.empty())
            {
                currentMountFlags = 0;
                for (AuraEffectList::const_iterator itr = auras.begin(); itr != auras.end(); ++itr)
                    currentMountFlags |= (*itr)->GetMiscValue();
            }
            else
            {
                AreaTableEntry const* entry;
                entry = GetAreaEntryByAreaID(areaId);
                if (!entry)
                    entry = GetAreaEntryByAreaID(zoneId);

                if (entry)
                    currentMountFlags = entry->mountFlags;
            }
        }

        // Find the fitting mount
        for (uint32 i = MAX_MOUNT_CAPABILITIES-1; i < MAX_MOUNT_CAPABILITIES; --i)
        {
            uint32 id = mountType->MountCapability[i];
            if (!id)
                continue;

            MountCapabilityEntry const* mountCapability = sMountCapabilityStore.LookupEntry(id);
            if (!mountCapability)
                continue;

            if (ridingSkill < mountCapability->RequiredRidingSkill)
                continue;

            // Flags required to use this mount
            uint32 reqFlags = mountCapability->Flags;

            if (reqFlags&1 && !(currentMountFlags&1))
                continue;

            if (reqFlags&2 && !(currentMountFlags&2))
                continue;

            if (reqFlags&4 && !(currentMountFlags&4))
                continue;

            if (reqFlags&8 && !(currentMountFlags&8))
                continue;

            if (m_movementInfo.hasPitch)
            {
                if (!(reqFlags & MOUNT_FLAG_CAN_PITCH))
                    continue;
            }

            if (HasUnitMovementFlag(MOVEMENTFLAG_SWIMMING))
            {
                if (!(reqFlags & MOUNT_FLAG_CAN_SWIM))
                    continue;
            }

            //if (!(reqFlags & 3))
                //continue;

            if (mountCapability->RequiredMap != -1 && GetMapId() != uint32(mountCapability->RequiredMap))
                continue;

            if (mountCapability->RequiredArea && (mountCapability->RequiredArea != zoneId && mountCapability->RequiredArea != areaId))
                continue;

            if (mountCapability->RequiredAura && !HasAura(mountCapability->RequiredAura))
                continue;

            if (mountCapability->RequiredSpell && (GetTypeId() != TYPEID_PLAYER || !ToPlayer()->HasSpell(mountCapability->RequiredSpell)))
                continue;

            newMount = mountCapability;
            break;
        }
    }

    if (_mount != newMount)
    {
        uint32 oldSpell = _mount ? _mount->SpeedModSpell : 0;
        bool oldFlyer = _mount ? (_mount->Flags & 2) : false;
        uint32 newSpell = newMount ? newMount->SpeedModSpell : 0;
        bool newFlyer = newMount ? (newMount->Flags & 2) : false;

        // This is required for displaying speeds on aura
        if (effect)
            effect->SetAmount(newMount ? newMount->Id : 0);

        if (oldSpell != newSpell)
        {
            if (oldSpell)
                RemoveAurasDueToSpell(oldSpell);

            if (newSpell)
                CastSpell(this, newSpell, true, NULL, effect);
        }

        if (oldFlyer != newFlyer)
            SetCanFly(newFlyer);

        if (!(oldFlyer ^ newFlyer))
            SetCanFly(newFlyer);

        Player* player = ToPlayer();
        if (!player)
            player = m_movedPlayer;

        _mount = newMount;
    }
}

MountCapabilityEntry const* Unit::GetMountCapability(uint32 mountType) const
{
    if (!mountType)
        return NULL;

    MountTypeEntry const* mountTypeEntry = sMountTypeStore.LookupEntry(mountType);
    if (!mountTypeEntry)
        return NULL;

    uint32 zoneId, areaId;
    GetZoneAndAreaId(zoneId, areaId);
    uint32 ridingSkill = 5000;
    if (GetTypeId() == TYPEID_PLAYER)
        ridingSkill = ToPlayer()->GetSkillValue(SKILL_RIDING);

    for (uint32 i = MAX_MOUNT_CAPABILITIES; i > 0; --i)
    {
        MountCapabilityEntry const* mountCapability = sMountCapabilityStore.LookupEntry(mountTypeEntry->MountCapability[i - 1]);
        if (!mountCapability)
            continue;

        if (ridingSkill < mountCapability->RequiredRidingSkill)
            continue;

        /*if (HasExtraUnitMovementFlag(MOVEMENTFLAG2_FULL_SPEED_PITCHING))
        {
            if (!(mountCapability->Flags & MOUNT_FLAG_CAN_PITCH))
                continue;
        }*/
        else if (HasUnitMovementFlag(MOVEMENTFLAG_SWIMMING))
        {
            if (!(mountCapability->Flags & MOUNT_FLAG_CAN_SWIM))
                continue;
        }
        else if (!(mountCapability->Flags & 0x1))   // unknown flags, checked in 4.2.2 14545 client
        {
            if (!(mountCapability->Flags & 0x2))
                continue;
        }

        if (mountCapability->RequiredMap != -1 && int32(GetMapId()) != mountCapability->RequiredMap)
            continue;

        if (mountCapability->RequiredArea && (mountCapability->RequiredArea != zoneId && mountCapability->RequiredArea != areaId))
            continue;

        if (mountCapability->RequiredAura && !HasAura(mountCapability->RequiredAura))
            continue;

        if (mountCapability->RequiredSpell && (GetTypeId() != TYPEID_PLAYER || !ToPlayer()->HasSpell(mountCapability->RequiredSpell)))
            continue;

        return mountCapability;
    }

    return NULL;
}

void Unit::SetInCombatWith(Unit* enemy)
{
    Unit* eOwner = enemy->GetCharmerOrOwnerOrSelf();
    if (eOwner->IsPvP())
    {
        SetInCombatState(true, enemy);
        return;
    }

    // check for duel
    if (eOwner->GetTypeId() == TYPEID_PLAYER && eOwner->ToPlayer()->duel)
    {
        Unit const* myOwner = GetCharmerOrOwnerOrSelf();
        if (((Player const*)eOwner)->duel->opponent == myOwner)
        {
            SetInCombatState(true, enemy);
            return;
        }
    }
    SetInCombatState(false, enemy);
}

void Unit::CombatStart(Unit* target, bool initialAggro)
{
    if (initialAggro)
    {
        if (!target->IsStandState())
            target->SetStandState(UNIT_STAND_STATE_STAND);

        if (!target->isInCombat() && target->GetTypeId() != TYPEID_PLAYER
            && !target->ToCreature()->HasReactState(REACT_PASSIVE) && target->ToCreature()->IsAIEnabled)
        {
            target->ToCreature()->AI()->AttackStart(this);
        }

        SetInCombatWith(target);
        target->SetInCombatWith(this);
    }
    Unit* who = target->GetCharmerOrOwnerOrSelf();
    if (who->GetTypeId() == TYPEID_PLAYER)
      SetContestedPvP(who->ToPlayer());

    Player* me = GetCharmerOrOwnerPlayerOrPlayerItself();
    if (me && who->IsPvP()
        && (who->GetTypeId() != TYPEID_PLAYER
        || !me->duel || me->duel->opponent != who))
    {
        me->UpdatePvP(true);
        me->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_ENTER_PVP_COMBAT);
    }
    RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_ENTER_COMBAT);
}

void Unit::SetInCombatState(bool PvP, Unit* enemy)
{
    // only alive units can be in combat
    if (!isAlive())
        return;

    SetCombatTimer(5000);

    if (isInCombat() || HasUnitState(UNIT_STATE_EVADE))
        return;

    SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IN_COMBAT);

    if (Creature* creature = ToCreature())
    {
        // Set home position at place of engaging combat for escorted creatures
        if ((IsAIEnabled && creature->AI()->IsEscorted()) ||
            GetMotionMaster()->GetCurrentMovementGeneratorType() == WAYPOINT_MOTION_TYPE ||
            GetMotionMaster()->GetCurrentMovementGeneratorType() == POINT_MOTION_TYPE)
            creature->SetHomePosition(GetPositionX(), GetPositionY(), GetPositionZ(), GetOrientation());

        if (enemy)
        {
            if (IsAIEnabled)
            {
                creature->AI()->EnterCombat(enemy);
                RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC); // unit has engaged in combat, remove immunity so players can fight back
            }
            if (creature->GetFormation())
                creature->GetFormation()->MemberAttackStart(creature, enemy);
        }

        if (!(creature->GetCreatureTemplate()->type_flags & CREATURE_TYPEFLAGS_MOUNTED_COMBAT))
            Dismount();
    }

    for (Unit::ControlList::iterator itr = m_Controlled.begin(); itr != m_Controlled.end(); ++itr)
    {
        (*itr)->SetInCombatState(PvP, enemy);
        (*itr)->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PET_IN_COMBAT);
    }
}

void Unit::ClearInCombat()
{
    m_CombatTimer = 0;
    RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IN_COMBAT);

    // Player's state will be cleared in Player::UpdateContestedPvP
    if (Creature* creature = ToCreature())
    {
        if (creature->GetCreatureTemplate() && creature->GetCreatureTemplate()->unit_flags & UNIT_FLAG_IMMUNE_TO_PC)
            SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC); // set immunity state to the one from db on evade

        ClearUnitState(UNIT_STATE_ATTACK_PLAYER);
        if (HasFlag(OBJECT_FIELD_DYNAMIC_FLAGS, UNIT_DYNFLAG_TAPPED))
            SetUInt32Value(OBJECT_FIELD_DYNAMIC_FLAGS, creature->GetCreatureTemplate()->dynamicflags);

        if (CreatureAI* ai = creature->AI())
            ai->OutOfCombat();

        if (creature->isPet())
        {
            if (Unit* owner = GetOwner())
                for (uint8 i = 0; i < MAX_MOVE_TYPE; ++i)
                    if (owner->GetSpeedRate(UnitMoveType(i)) > GetSpeedRate(UnitMoveType(i)))
                        SetSpeed(UnitMoveType(i), owner->GetSpeedRate(UnitMoveType(i)), true);
        }
        else if (!isCharmed())
            return;
    }
    else
        ToPlayer()->UpdatePotionCooldown();

    RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PET_IN_COMBAT);
}

bool Unit::isTargetableForAttack(bool checkFakeDeath) const
{
    if (!isAlive())
        return false;

    if (HasFlag(UNIT_FIELD_FLAGS,
        UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_IMMUNE_TO_PC))
        return false;

    if (GetTypeId() == TYPEID_PLAYER && ToPlayer()->isGameMaster())
        return false;

    return !HasUnitState(UNIT_STATE_UNATTACKABLE) && (!checkFakeDeath || !HasUnitState(UNIT_STATE_DIED));
}

bool Unit::IsValidAttackTarget(Unit const* target) const
{
    return _IsValidAttackTarget(target, NULL);
}

// function based on function Unit::CanAttack from 13850 client
bool Unit::_IsValidAttackTarget(Unit const* target, SpellInfo const* bySpell, WorldObject const* obj) const
{
    ASSERT(target);

    // can't attack self
    if (this == target)
        return false;
    
    // Sha of anger mind control || Crawler mine (Iron Juggernaut)
    if (target->HasAura(119626) || target->HasAura(144718))
        return true;

    // can't attack unattackable units or GMs
    if (target->HasUnitState(UNIT_STATE_UNATTACKABLE)
        || (target->GetTypeId() == TYPEID_PLAYER && target->ToPlayer()->isGameMaster()))
        return false;

    // can't attack own vehicle or passenger
    if (m_vehicle)
        if (IsOnVehicle(target) || m_vehicle->GetBase()->IsOnVehicle(target))
            return false;

    // can't attack invisible (ignore stealth for aoe spells) also if the area being looked at is from a spell use the dynamic object created instead of the casting unit.
    if ((!bySpell || !(bySpell->AttributesEx6 & SPELL_ATTR6_CAN_TARGET_INVISIBLE)) && (obj ? !obj->canSeeOrDetect(target, bySpell && bySpell->IsAffectingArea()) : !canSeeOrDetect(target, bySpell && bySpell->IsAffectingArea())))
        return false;

    // can't attack dead
    if ((!bySpell || !bySpell->IsAllowingDeadTarget()) && !target->isAlive())
       return false;

    // can't attack untargetable
    if ((!bySpell || !(bySpell->AttributesEx6 & SPELL_ATTR6_CAN_TARGET_UNTARGETABLE))
        && target->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE))
        return false;

    if (Player const* playerAttacker = ToPlayer())
    {
        if (playerAttacker->HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_UBER))
            return false;
    }

    // check flags
    if (target->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_TAXI_FLIGHT | UNIT_FLAG_NOT_ATTACKABLE_1 | UNIT_FLAG_UNK_16)
        || (!HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PVP_ATTACKABLE) && target->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_NPC))
        || (!target->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PVP_ATTACKABLE) && HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_NPC)))
        return false;

    if ((!bySpell || !(bySpell->AttributesEx8 & SPELL_ATTR8_ATTACK_IGNORE_IMMUNE_TO_PC_FLAG))
        && (HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PVP_ATTACKABLE) && target->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC))
        // check if this is a world trigger cast - GOs are using world triggers to cast their spells, so we need to ignore their immunity flag here, this is a temp workaround, needs removal when go cast is implemented properly
        && GetEntry() != WORLD_TRIGGER)
        return false;

    // CvC case - can attack each other only when one of them is hostile
    if (!HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PVP_ATTACKABLE) && !target->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PVP_ATTACKABLE))
        return GetReactionTo(target) <= REP_HOSTILE || target->GetReactionTo(this) <= REP_HOSTILE;

    // PvP, PvC, CvP case
    // can't attack friendly targets
    if ( GetReactionTo(target) > REP_NEUTRAL
        || target->GetReactionTo(this) > REP_NEUTRAL)
        return false;

    // Not all neutral creatures can be attacked
    if (GetReactionTo(target) == REP_NEUTRAL &&
        target->GetReactionTo(this) == REP_NEUTRAL)
    {
        if  (!(target->GetTypeId() == TYPEID_PLAYER && GetTypeId() == TYPEID_PLAYER) &&
            !(target->GetTypeId() == TYPEID_UNIT && GetTypeId() == TYPEID_UNIT))
        {
            Player const* player = target->GetTypeId() == TYPEID_PLAYER ? target->ToPlayer() : ToPlayer();
            Unit const* creature = target->GetTypeId() == TYPEID_UNIT ? target : this;
            
            {
                if (FactionTemplateEntry const* factionTemplate = creature->getFactionTemplateEntry())
                    if (FactionEntry const* factionEntry = sFactionStore.LookupEntry(factionTemplate->faction))
                        if (FactionState const* repState = player->GetReputationMgr().GetState(factionEntry))
                            if (!(repState->Flags & FACTION_FLAG_AT_WAR))
                                return false;
            }
        }
    }

    Creature const* creatureAttacker = ToCreature();
    if (creatureAttacker && creatureAttacker->GetCreatureTemplate()->type_flags & CREATURE_TYPEFLAGS_TREAT_AS_RAID_UNIT)
        return false;

    Player const* playerAffectingAttacker = HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PVP_ATTACKABLE) ? GetAffectingPlayer() : NULL;
    Player const* playerAffectingTarget = target->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PVP_ATTACKABLE) ? target->GetAffectingPlayer() : NULL;

    // check duel - before sanctuary checks
    if (playerAffectingAttacker && playerAffectingTarget)
        if (playerAffectingAttacker->duel && playerAffectingAttacker->duel->opponent == playerAffectingTarget && playerAffectingAttacker->duel->startTime != 0)
            return true;

    // PvP case - can't attack when attacker or target are in sanctuary
    // however, 13850 client doesn't allow to attack when one of the unit's has sanctuary flag and is pvp
    if (target->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PVP_ATTACKABLE) && HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PVP_ATTACKABLE)
        && ((target->GetByteValue(UNIT_FIELD_BYTES_2, 1) & UNIT_BYTE2_FLAG_SANCTUARY) || (GetByteValue(UNIT_FIELD_BYTES_2, 1) & UNIT_BYTE2_FLAG_SANCTUARY)))
        return false;

    // additional checks - only PvP case
    if (playerAffectingAttacker && playerAffectingTarget)
    {
        if (target->GetByteValue(UNIT_FIELD_BYTES_2, 1) & UNIT_BYTE2_FLAG_PVP)
            return true;

        if (GetByteValue(UNIT_FIELD_BYTES_2, 1) & UNIT_BYTE2_FLAG_FFA_PVP
            && target->GetByteValue(UNIT_FIELD_BYTES_2, 1) & UNIT_BYTE2_FLAG_FFA_PVP)
            return true;

        return (GetByteValue(UNIT_FIELD_BYTES_2, 1) & UNIT_BYTE2_FLAG_UNK1)
            || (target->GetByteValue(UNIT_FIELD_BYTES_2, 1) & UNIT_BYTE2_FLAG_UNK1);
    }
    return true;
}

bool Unit::IsValidAssistTarget(Unit const* target) const
{
    return _IsValidAssistTarget(target, NULL);
}

// function based on function Unit::CanAssist from 13850 client
bool Unit::_IsValidAssistTarget(Unit const* target, SpellInfo const* bySpell) const
{
    ASSERT(target);

    // can assist to self
    if (this == target)
        return true;

    // can't assist unattackable units or GMs
    if (target->HasUnitState(UNIT_STATE_UNATTACKABLE)
        || (target->GetTypeId() == TYPEID_PLAYER && target->ToPlayer()->isGameMaster()))
        return false;

    // can't assist own vehicle or passenger
    if (m_vehicle)
        if (IsOnVehicle(target) || m_vehicle->GetBase()->IsOnVehicle(target))
            return false;

    // can't assist invisible
    if ((!bySpell || !(bySpell->AttributesEx6 & SPELL_ATTR6_CAN_TARGET_INVISIBLE)) && !canSeeOrDetect(target, bySpell && bySpell->IsAffectingArea()))
        return false;

    // can't assist dead
    if ((!bySpell || !bySpell->IsAllowingDeadTarget()) && !target->isAlive())
       return false;

    // can't assist untargetable
    if ((!bySpell || !(bySpell->AttributesEx6 & SPELL_ATTR6_CAN_TARGET_UNTARGETABLE))
        && target->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE))
        return false;

    if (!bySpell || !(bySpell->AttributesEx6 & SPELL_ATTR6_ASSIST_IGNORE_IMMUNE_FLAG))
    {
        if (HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PVP_ATTACKABLE))
        {
            if (target->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC))
                return false;
        }
        else
        {
            if (target->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_NPC))
                return false;
        }
    }

    //sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "_IsValidAssistTarget Id %i GetReactionTo %i", bySpell ? bySpell->Id : 0, GetReactionTo(target));
    // can't assist non-friendly targets
    if (GetReactionTo(target) <= REP_NEUTRAL
        && target->GetReactionTo(this) <= REP_NEUTRAL
        && (!ToCreature() || !(ToCreature()->GetCreatureTemplate()->type_flags & CREATURE_TYPEFLAGS_TREAT_AS_RAID_UNIT)))
        return false;

    //Check for pets(need for Wild Mushroom)
    if(target->GetOwner() == this)
        return true;

    // PvP case
    if (target->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PVP_ATTACKABLE))
    {
        Player const* targetPlayerOwner = target->GetAffectingPlayer();
        if (HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PVP_ATTACKABLE))
        {
            Player const* selfPlayerOwner = GetAffectingPlayer();
            if (selfPlayerOwner && targetPlayerOwner)
            {
                // can't assist player which is dueling someone
                if (selfPlayerOwner != targetPlayerOwner
                    && targetPlayerOwner->duel)
                    return false;
            }
            // can't assist player in ffa_pvp zone from outside
            if (!(bySpell->AttributesEx2 & SPELL_ATTR2_CAN_TARGET_NOT_IN_LOS))
                if ((target->GetByteValue(UNIT_FIELD_BYTES_2, 1) & UNIT_BYTE2_FLAG_FFA_PVP)
                    && !(GetByteValue(UNIT_FIELD_BYTES_2, 1) & UNIT_BYTE2_FLAG_FFA_PVP))
                    return false;
            // can't assist player out of sanctuary from sanctuary if has pvp enabled
            if (target->GetByteValue(UNIT_FIELD_BYTES_2, 1) & UNIT_BYTE2_FLAG_PVP)
                if ((GetByteValue(UNIT_FIELD_BYTES_2, 1) & UNIT_BYTE2_FLAG_SANCTUARY) && !(target->GetByteValue(UNIT_FIELD_BYTES_2, 1) & UNIT_BYTE2_FLAG_SANCTUARY))
                    return false;
        }
    }
    // PvC case - player can assist creature only if has specific type flags
    // !target->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PVP_ATTACKABLE) &&
    else if (HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PVP_ATTACKABLE)
        && (!bySpell || !(bySpell->AttributesEx6 & SPELL_ATTR6_ASSIST_IGNORE_IMMUNE_FLAG))
        && !((target->GetByteValue(UNIT_FIELD_BYTES_2, 1) & UNIT_BYTE2_FLAG_PVP)))
    {
        if (Creature const* creatureTarget = target->ToCreature())
            return creatureTarget->GetCreatureTemplate()->type_flags & CREATURE_TYPEFLAGS_TREAT_AS_RAID_UNIT || creatureTarget->GetCreatureTemplate()->type_flags & CREATURE_TYPEFLAGS_CAN_ASSIST;
    }
    return true;
}

int32 Unit::ModifyHealth(int32 dVal)
{
    int64 gain = 0;

    if (dVal == 0)
        return 0;

    int64 curHealth = (int32)GetHealth();

    int64 val = dVal + curHealth;
    if (val <= 0)
    {
        SetHealth(0);
        return -curHealth;
    }

    int64 maxHealth = (int64)GetMaxHealth();

    if (val < maxHealth)
    {
        SetHealth(val);
        gain = val - curHealth;
    }
    else if (curHealth != maxHealth)
    {
        SetHealth(maxHealth);
        gain = maxHealth - curHealth;
    }

    return gain;
}

int32 Unit::GetHealthGain(int32 dVal)
{
    int32 gain = 0;

    if (dVal == 0)
        return 0;

    int32 curHealth = (int32)GetHealth();

    int32 val = dVal + curHealth;
    if (val <= 0)
    {
        return -curHealth;
    }

    int32 maxHealth = (int32)GetMaxHealth();

    if (val < maxHealth)
        gain = dVal;
    else if (curHealth != maxHealth)
        gain = maxHealth - curHealth;

    return gain;
}

void Unit::VisualForPower(Powers power, int32 curentVal, int32 modVal)
{
    Player* player = ToPlayer();
    if(!player)
        return;

    int32 oldVal = GetPower(power);
    if(modVal > 0)
    {
        AuraEffectList mTotalAuraList = GetAuraEffectsByType(SPELL_AURA_PROC_ON_POWER_AMOUNT_2);
        for (AuraEffectList::const_iterator i = mTotalAuraList.begin(), next; i != mTotalAuraList.end(); i = next)
        {
            next = i;
            ++next;
            uint32 triggered_spell_id = (*i)->GetTriggerSpell() ? (*i)->GetTriggerSpell(): 0;
            SpellInfo const* triggerEntry = sSpellMgr->GetSpellInfo(triggered_spell_id);
            if ((*i)->GetMiscValue() == power && oldVal < (*i)->GetAmount() && curentVal >= (*i)->GetAmount() && (*i)->GetMiscValueB() == 0)
            {
                if(triggerEntry)
                    CastSpell(this, triggered_spell_id, true, NULL, (*i));
            }
        }
    }
    else
    {
        AuraEffectList mTotalAuraList = GetAuraEffectsByType(SPELL_AURA_PROC_ON_POWER_AMOUNT_2);
        for (AuraEffectList::const_iterator i = mTotalAuraList.begin(), next; i != mTotalAuraList.end(); i = next)
        {
            next = i;
            ++next;
            uint32 triggered_spell_id = (*i)->GetTriggerSpell() ? (*i)->GetTriggerSpell(): 0;
            SpellInfo const* triggerEntry = sSpellMgr->GetSpellInfo(triggered_spell_id);
            if ((*i)->GetMiscValue() == power && oldVal > (*i)->GetAmount() && curentVal <= (*i)->GetAmount() && (*i)->GetMiscValueB() == 1)
            {
                if(triggerEntry)
                {
                    if (Aura* aura = GetAura(triggered_spell_id))
                        aura->Remove();
                    else
                        CastSpell(this, triggered_spell_id, true, NULL, (*i));
                }
            }
        }
    }

    uint32 specId = player->GetSpecializationId(player->GetActiveSpec());

    switch(power)
    {
        case POWER_SOUL_SHARDS:
        {
            if(specId != SPEC_WARLOCK_AFFLICTION)
            {
                RemoveAura(104756);
                RemoveAura(104759);
                RemoveAura(123171);
                if (HasAura(56241))
                {
                    RemoveAura(123728);
                    RemoveAura(123730);
                    RemoveAura(123731);
                }
                break;
            }

            uint32 spellid[] = {104756, 104759, 123171};
            if(HasAura(56241)) // Glyph of Verdant Spheres (Affliction, Destruction)
            {
                spellid[0] = 123728;
                spellid[1] = 123730;
                spellid[2] = 123731;
            }

            switch(curentVal)
            {
                case 100:
                {
                    RemoveAura(spellid[1]);
                    RemoveAura(spellid[2]);
                    CastSpell(this, spellid[0], true);
                    break;
                }
                case 200:
                {
                    RemoveAura(spellid[0]);
                    RemoveAura(spellid[2]);
                    CastSpell(this, spellid[1], true);
                    break;
                }
                case 300:
                {
                    RemoveAura(spellid[2]);
                    CastSpell(this, spellid[1], true);
                    CastSpell(this, spellid[0], true);
                    break;
                }
                case 400:
                {
                    RemoveAura(spellid[0]);
                    CastSpell(this, spellid[2], true);
                    CastSpell(this, spellid[1], true);
                    break;
                }
                default:
                {
                    RemoveAura(spellid[0]);
                    break;
                }
            }
            break;
        }
        case POWER_CHI:
        {
            if (modVal < 0)
            {
                if (Aura* tigereyeBrew = GetAura(123980))
                    tigereyeBrew->SetScriptData(0, -modVal);
                else if (Aura* manaTea = GetAura(123766))
                    manaTea->SetScriptData(0, -modVal);
            }
            break;
        }
        case POWER_DEMONIC_FURY:
        {
            if(specId != SPEC_WARLOCK_DEMONOLOGY)
            {
                RemoveAura(122738);
                RemoveAura(131755);
                break;
            }

            if(curentVal <= 200 && oldVal > curentVal && GetShapeshiftForm() == FORM_METAMORPHOSIS)
                if (AuraEffect* aurEff = GetAuraEffect(109145, 0))
                    aurEff->ChangeAmount(-100);

            // Demonic Fury visuals
            if (curentVal == 1000)
                CastSpell(this, 131755, true);
            else if (curentVal >= 500)
            {
                if (!HasAura(122738))
                    CastSpell(this, 122738, true);

                RemoveAura(131755);
            }
            else
            {
                RemoveAura(122738);
                RemoveAura(131755);
            }
            break;
        }
        case POWER_BURNING_EMBERS:
        {
            if(specId != SPEC_WARLOCK_DESTRUCTION)
            {
                RemoveAura(116920);
                if (HasAura(56241))
                {
                    RemoveAura(123728);
                    RemoveAura(123730);
                    RemoveAura(123731);
                }
                break;
            }

            if(curentVal < 10)
                RemoveAura(108683);

            if(curentVal < 10 && oldVal > curentVal)
                if (AuraEffect* aurEff = GetAuraEffect(108647, 0))
                    aurEff->ChangeAmount(-100);

            if (curentVal >= 10 && curentVal > oldVal)
                if (AuraEffect* aurEff = GetAuraEffect(108647, 0))
                    aurEff->ChangeAmount(-100);

            if (curentVal >= 20 && !HasAura(116920))
                CastSpell(this, 116920, true);
            else if (curentVal < 20)
                RemoveAura(116920);

            if(HasAura(56241))
            {
                uint32 spellid[] = {123728, 123730, 123731};

                switch(curentVal)
                {
                    case 10:
                    {
                        RemoveAura(spellid[1]);
                        RemoveAura(spellid[2]);
                        CastSpell(this, spellid[0], true);
                        break;
                    }
                    case 20:
                    {
                        RemoveAura(spellid[0]);
                        RemoveAura(spellid[2]);
                        CastSpell(this, spellid[1], true);
                        break;
                    }
                    case 30:
                    {
                        RemoveAura(spellid[2]);
                        CastSpell(this, spellid[1], true);
                        CastSpell(this, spellid[0], true);
                        break;
                    }
                    case 40:
                    {
                        RemoveAura(spellid[0]);
                        CastSpell(this, spellid[2], true);
                        CastSpell(this, spellid[1], true);
                        break;
                    }
                    default:
                    {
                        RemoveAura(spellid[0]);
                        break;
                    }
                }
            }
            break;
        }
        case POWER_SHADOW_ORB:
        {
            if(specId != SPEC_PRIEST_SHADOW)
            {
                RemoveAura(77487);
                break;
            }

            if (curentVal > 0)
            {
                if (!HasAura(77487))
                    CastSpell(this, 77487, true);
            }
            else
                RemoveAura(77487);
            break;
        }
    }
}

// returns negative amount on power reduction
int32 Unit::ModifyPower(Powers power, int32 dVal, bool set)
{
    uint32 powerIndex = GetPowerIndexByClass(power, getClass());
    if (powerIndex == MAX_POWERS)
        return 0;

    int32 gain = 0;

    if (dVal == 0 && power != POWER_ENERGY) // The client will always regen energy if we don't send him the actual value
        return 0;

    int32 curPower = GetPower(power);

    int32 val = dVal + curPower;

    //Visualization for power
    VisualForPower(power, val, dVal);

    if (val <= GetMinPower(power))
    {
        if(set)
            SetPower(power, GetMinPower(power));
        else
            SetInt32Value(UNIT_FIELD_POWER1 + powerIndex, GetMinPower(power));
        if (power == POWER_ECLIPSE)
            TriggerEclipse(curPower);
        return -curPower;
    }

    int32 maxPower = GetMaxPower(power);

    if (val < maxPower)
    {
        if(set)
            SetPower(power, val);
        else
            SetInt32Value(UNIT_FIELD_POWER1 + powerIndex, val);
        gain = val - curPower;
    }
    else if (curPower != maxPower)
    {
        if(set)
            SetPower(power, maxPower);
        else
            SetInt32Value(UNIT_FIELD_POWER1 + powerIndex, maxPower);
        gain = maxPower - curPower;
    }

    if (gain && power == POWER_ECLIPSE)
        TriggerEclipse(curPower);

    if(power == POWER_BURNING_EMBERS)
    {
        if(val >= maxPower && curPower < maxPower)
            SetPower(power, val);
    }

    return gain;
}

void Unit::TriggerEclipse(int32 oldPower)
{
    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(79577);
    if (!spellInfo)
        return;

    int32 newPower = GetPower(POWER_ECLIPSE);
    if (newPower == oldPower)
        return;

    // Eclipse is cleared when eclipse power reaches 0
    if (newPower * oldPower <= 0)
    {
        RemoveAurasDueToSpell(spellInfo->Effects[EFFECT_0].TriggerSpell);
        RemoveAurasDueToSpell(spellInfo->Effects[EFFECT_1].TriggerSpell);
    }

    if (newPower != spellInfo->Effects[EFFECT_0].CalcValue(this) && newPower != spellInfo->Effects[EFFECT_1].CalcValue(this))
        return;

    uint32 effIdx = newPower > 0 ? EFFECT_0 : EFFECT_1;
    uint32 eclipseSpell = spellInfo->Effects[effIdx].TriggerSpell;
    if (!eclipseSpell)
        return;

    // cast Eclipse
    CastSpell(this, eclipseSpell, true);
}

// returns negative amount on power reduction
int32 Unit::ModifyPowerPct(Powers power, float pct, bool apply)
{
    float amount = (float)GetMaxPower(power);
    ApplyPercentModFloatVar(amount, pct, apply);

    return ModifyPower(power, (int32)amount - GetMaxPower(power));
}

bool Unit::IsAlwaysVisibleFor(WorldObject const* seer) const
{
    if (WorldObject::IsAlwaysVisibleFor(seer))
        return true;

    // Always seen by owner
    if (uint64 guid = GetCharmerOrOwnerGUID())
        if (seer->GetGUID() == guid)
            return true;

    if (Player const* seerPlayer = seer->ToPlayer())
        if (Unit* owner =  GetOwner())
            if (Player* ownerPlayer = owner->ToPlayer())
                if (ownerPlayer->IsGroupVisibleFor(seerPlayer))
                    return true;

    return false;
}

bool Unit::IsAlwaysDetectableFor(WorldObject const* seer) const
{
    if (WorldObject::IsAlwaysDetectableFor(seer))
        return true;

    if (HasAuraTypeWithCaster(SPELL_AURA_MOD_STALKED, seer->GetGUID()))
        return true;

    return false;
}

void Unit::UpdateSpeed(UnitMoveType mtype, bool forced)
{
    float old_speed = m_speed_rate[MOVE_RUN];
    int32 main_speed_mod  = 0;
    float stack_bonus     = 1.0f;
    float non_stack_bonus = 1.0f;

    switch (mtype)
    {
        // Only apply debuffs
        case MOVE_FLIGHT_BACK:
        case MOVE_RUN_BACK:
        case MOVE_SWIM_BACK:
            break;
        case MOVE_WALK:
            return;
        case MOVE_RUN:
        {
            if (IsMounted()) // Use on mount auras
            {
                main_speed_mod  = GetMaxPositiveAuraModifier(SPELL_AURA_MOD_INCREASE_MOUNTED_SPEED);
                stack_bonus     = GetTotalAuraMultiplier(SPELL_AURA_MOD_MOUNTED_SPEED_ALWAYS);
                non_stack_bonus += GetMaxPositiveAuraModifier(SPELL_AURA_MOD_MOUNTED_SPEED_NOT_STACK) / 100.0f;
            }
            else
            {
                main_speed_mod  = GetMaxPositiveAuraModifier(SPELL_AURA_MOD_INCREASE_SPEED) - GetMaxNegativeAuraModifier(SPELL_AURA_MOD_INCREASE_SPEED);
                stack_bonus     = GetTotalAuraMultiplier(SPELL_AURA_MOD_SPEED_ALWAYS);
                non_stack_bonus += GetMaxPositiveAuraModifier(SPELL_AURA_MOD_SPEED_NOT_STACK) / 100.0f;
            }
            break;
        }
        case MOVE_SWIM:
        {
            main_speed_mod  = GetMaxPositiveAuraModifier(SPELL_AURA_MOD_INCREASE_SWIM_SPEED);
            break;
        }
        case MOVE_FLIGHT:
        {
            if (GetTypeId() == TYPEID_UNIT && IsControlledByPlayer()) // not sure if good for pet
            {
                main_speed_mod  = GetMaxPositiveAuraModifier(SPELL_AURA_MOD_INCREASE_VEHICLE_FLIGHT_SPEED);
                stack_bonus     = GetTotalAuraMultiplier(SPELL_AURA_MOD_VEHICLE_SPEED_ALWAYS);

                // for some spells this mod is applied on vehicle owner
                int32 owner_speed_mod = 0;

                if (Unit* owner = GetCharmer())
                    owner_speed_mod = owner->GetMaxPositiveAuraModifier(SPELL_AURA_MOD_INCREASE_VEHICLE_FLIGHT_SPEED);

                main_speed_mod = std::max(main_speed_mod, owner_speed_mod);
            }
            else if (IsMounted())
            {
                main_speed_mod  = GetMaxPositiveAuraModifier(SPELL_AURA_MOD_INCREASE_MOUNTED_FLIGHT_SPEED);
                stack_bonus     = GetTotalAuraMultiplier(SPELL_AURA_MOD_MOUNTED_FLIGHT_SPEED_ALWAYS);
            }
            else             // Use not mount (shapeshift for example) auras (should stack)
                main_speed_mod  = GetTotalAuraModifier(SPELL_AURA_MOD_INCREASE_FLIGHT_SPEED) + GetTotalAuraModifier(SPELL_AURA_MOD_INCREASE_VEHICLE_FLIGHT_SPEED);

            non_stack_bonus += GetMaxPositiveAuraModifier(SPELL_AURA_MOD_FLIGHT_SPEED_NOT_STACK) / 100.0f;

            // Update speed for vehicle if available
            if (GetTypeId() == TYPEID_PLAYER && GetVehicle())
                GetVehicleBase()->UpdateSpeed(MOVE_FLIGHT, true);
            break;
        }
        default:
            sLog->outError(LOG_FILTER_UNITS, "Unit::UpdateSpeed: Unsupported move type (%d)", mtype);
            return;
    }

    // now we ready for speed calculation
    float speed = std::max(non_stack_bonus, stack_bonus);
    if (main_speed_mod)
        AddPct(speed, main_speed_mod);

    switch (mtype)
    {
        case MOVE_RUN:
        case MOVE_SWIM:
        case MOVE_FLIGHT:
        {
            // Set creature speed rate
            if (GetTypeId() == TYPEID_UNIT)
            {
                Unit* pOwner = GetCharmerOrOwner();
                if ((isPet() || isGuardian()) && !isInCombat() && pOwner && pOwner->IsMounted()) // Must check for owner or crash on "Tame Beast"
                {
                    // For every yard over 5, increase speed by 0.01
                    //  to help prevent pet from lagging behind and despawning
                    float dist = GetDistance(pOwner);
                    float base_rate = 1.00f; // base speed is 100% of owner speed

                    if (dist < 5)
                        dist = 5;

                    float mult = base_rate + ((dist - 5) * 0.01f);

                    speed *= pOwner->GetSpeedRate(mtype) * mult; // pets derive speed from owner when not in combat
                }
                else
                    speed *= ToCreature()->GetCreatureTemplate()->speed_run;    // at this point, MOVE_WALK is never reached
            }
            // Normalize speed by 191 aura SPELL_AURA_USE_NORMAL_MOVEMENT_SPEED if need
            // TODO: possible affect only on MOVE_RUN
            if (int32 normalization = GetMaxPositiveAuraModifier(SPELL_AURA_USE_NORMAL_MOVEMENT_SPEED))
            {
                // Use speed from aura
                float max_speed = normalization / (IsControlledByPlayer() ? playerBaseMoveSpeed[mtype] : baseMoveSpeed[mtype]);
                if (speed > max_speed)
                    speed = max_speed;
            }
            break;
        }
        default:
            break;
    }

    // for creature case, we check explicit if mob searched for assistance
    if (GetTypeId() == TYPEID_UNIT)
    {
        if (ToCreature()->HasSearchedAssistance())
            speed *= 0.66f;                                 // best guessed value, so this will be 33% reduction. Based off initial speed, mob can then "run", "walk fast" or "walk".
    }

    // Apply strongest slow aura mod to speed
    int32 slow = GetMaxNegativeAuraModifier(SPELL_AURA_MOD_DECREASE_SPEED) - GetMaxPositiveAuraModifier(SPELL_AURA_MOD_DECREASE_SPEED);
    if (slow)
        AddPct(speed, slow);

    if (float minSpeedMod = (float)GetMaxPositiveAuraModifier(SPELL_AURA_MOD_MINIMUM_SPEED))
    {
        float min_speed = minSpeedMod / 100.0f;
        if (speed < min_speed && mtype != MOVE_SWIM)
            speed = min_speed;
    }

    if (mtype == MOVE_SWIM)
    {
        if (float minSwimSpeedMod = (float)GetMaxPositiveAuraModifier(SPELL_AURA_INCREASE_MIN_SWIM_SPEED))
        {
            float min_speed = minSwimSpeedMod / 100.0f;
            if (speed < min_speed)
                speed = min_speed;
        }
    }
    if (speed <= 0) //crash client
        speed = 0.01f;

    if(old_speed > speed)
        m_anti_JupmTime = 2000;

    SetSpeed(mtype, speed, forced);
}

float Unit::GetSpeed(UnitMoveType mtype) const
{
    return m_speed_rate[mtype]*(IsControlledByPlayer() ? playerBaseMoveSpeed[mtype] : baseMoveSpeed[mtype]);
}

void Unit::SetSpeed(UnitMoveType mtype, float rate, bool forced)
{
    if (rate < 0)
        rate = 0.0f;

    // Update speed only on change
    if (m_speed_rate[mtype] == rate)
        return;

    m_speed_rate[mtype] = rate;

    propagateSpeedChange();

    WorldPacket data;
    ObjectGuid guid = GetGUID();
    if (!forced)
    {
        switch (mtype)
        {
            case MOVE_WALK:
            {
                //! 5.4.1
                data.Initialize(SMSG_SPLINE_MOVE_SET_WALK_SPEED, 8+4+1);
    
                data.WriteGuidMask<2, 0, 6, 3, 7, 5, 1, 4>(guid);
                data << float(GetSpeed(mtype));
                data.WriteGuidBytes<3, 1, 4, 2, 6, 0, 5, 7>(guid);
                break;
            }
            case MOVE_RUN:
            {
                //! 5.4.1
                data.Initialize(SMSG_SPLINE_MOVE_SET_RUN_SPEED, 1 + 8 + 4);
    
                data.WriteGuidMask<7, 0, 4, 6, 1, 2, 3, 5>(guid);
                data.WriteGuidBytes<2, 4, 6>(guid);
                data << float(GetSpeed(mtype));
                data.WriteGuidBytes< 0, 3, 7, 1, 5>(guid);
                break;
            }
            case MOVE_RUN_BACK:
            {
                //! 5.4.1
                data.Initialize(SMSG_SPLINE_MOVE_SET_RUN_BACK_SPEED, 1 + 8 + 4);
    
                data.WriteGuidMask<1, 7, 2, 5, 0, 6, 3, 4>(guid);               
                data.WriteGuidBytes<4, 7, 6, 0, 2, 3>(guid);
                data << float(GetSpeed(mtype));
                data.WriteGuidBytes<5, 1>(guid);
                break;
            }
            case MOVE_SWIM:
            {
                //! 5.4.1
                data.Initialize(SMSG_SPLINE_MOVE_SET_SWIM_SPEED, 1 + 8 + 4);
    
                data.WriteGuidMask<3, 2, 6, 1, 7, 0, 4, 5>(guid);                
                data.WriteGuidBytes<3>(guid);
                data << float(GetSpeed(mtype));
                data.WriteGuidBytes<4, 1, 2, 7, 0, 5, 6>(guid);
                break;
            }
            case MOVE_SWIM_BACK:
            {
                //! 5.4.1
                data.Initialize(SMSG_SPLINE_MOVE_SET_SWIM_BACK_SPEED, 1 + 8 + 4);
    
                data.WriteGuidMask<6, 0, 1, 3, 7, 2, 4, 5>(guid);                
                data.WriteGuidBytes<2, 5, 6, 4, 3, 1, 0>(guid);
                data << float(GetSpeed(mtype));
                data.WriteGuidBytes<7>(guid);
                break;
            }
            case MOVE_TURN_RATE:
            {
                //! 5.4.1
                data.Initialize(SMSG_SPLINE_MOVE_SET_TURN_RATE, 1 + 8 + 4);
    
                data.WriteGuidMask<5, 1, 7, 2, 0, 6, 3, 4>(guid);               
                data.WriteGuidBytes<4, 0, 3, 7, 5, 2>(guid);
                data << float(GetSpeed(mtype));
                data.WriteGuidBytes<1, 6>(guid);
                break;
            }
            case MOVE_FLIGHT:
            {
                //! 5.4.1
                data.Initialize(SMSG_SPLINE_MOVE_SET_FLIGHT_SPEED, 1 + 8 + 4);
    
                data.WriteGuidMask<2, 6, 0, 3, 4, 1, 5, 7>(guid);
                data << float(GetSpeed(mtype));
                data.WriteGuidBytes<7, 2, 3, 0, 5, 1, 4, 6>(guid);
                break;
            }
            case MOVE_FLIGHT_BACK:
            {
                //! 5.4.1
                data.Initialize(SMSG_SPLINE_MOVE_SET_FLIGHT_BACK_SPEED, 1 + 8 + 4);
    
                data.WriteGuidMask<6, 7, 1, 5, 2, 4, 3, 0>(guid);
                data << float(GetSpeed(mtype));
                data.WriteGuidBytes<4, 3, 6, 5, 0, 2, 1, 7>(guid);
                break;
            }
            case MOVE_PITCH_RATE:
            {
                //! 5.4.1
                data.Initialize(SMSG_SPLINE_MOVE_SET_PITCH_RATE, 1 + 8 + 4);
                data << float(GetSpeed(mtype));
                data.WriteGuidMask<4, 0, 5, 7, 2, 3, 6, 1>(guid);
                data.WriteGuidBytes<0, 3, 7, 4, 1, 2, 6, 5>(guid);
                break;
            }
            default:
                sLog->outError(LOG_FILTER_UNITS, "Unit::SetSpeed: Unsupported move type (%d), data not sent to client.", mtype);
                return;
        }

        SendMessageToSet(&data, true);
    }
    else
    {
        if (GetTypeId() == TYPEID_PLAYER)
        {
            // register forced speed changes for WorldSession::HandleForceSpeedChangeAck
            // and do it only for real sent packets and use run for run/mounted as client expected
            ++ToPlayer()->m_forced_speed_changes[mtype];

            if (!isInCombat())
                if (Pet* pet = ToPlayer()->GetPet())
                    pet->SetSpeed(mtype, m_speed_rate[mtype], forced);
        }

        switch (mtype)
        {
            case MOVE_WALK:
            {
                //! 5.4.1
                data.Initialize(SMSG_MOVE_SET_WALK_SPEED, 1 + 8 + 4 + 4);
   
                data << float(GetSpeed(mtype));
                data << uint32(0); // Unk Int32
                data.WriteGuidMask<5, 4, 3, 1, 0, 7, 2, 6>(guid);                
                data.WriteGuidBytes<7, 5, 3, 2, 4, 1, 6, 0>(guid);
                break;
            }
            case MOVE_RUN:
            {
                //! 5.4.1
                data.Initialize(SMSG_MOVE_SET_RUN_SPEED, 1 + 8 + 4 + 4);

                data.WriteGuidMask<3, 6, 7, 1, 4, 0, 2, 5>(guid);
                data.WriteGuidBytes<7, 5>(guid);
                data << float(GetSpeed(mtype));
                data.WriteGuidBytes<6, 2, 0, 3, 1, 4>(guid);
                data << uint32(0); // Unk Int32
                break;
            }
            case MOVE_RUN_BACK:
            {
                //! 5.4.1
                data.Initialize(SMSG_MOVE_SET_RUN_BACK_SPEED, 1 + 8 + 4 + 4);
                data.WriteGuidMask<1, 0, 5, 2, 4, 6, 7, 3>(guid);
                data.WriteGuidBytes<3, 0, 7>(guid);
                data << float(GetSpeed(mtype));
                data.WriteGuidBytes<2, 4>(guid);
                data << uint32(0); // Unk Int32
                data.WriteGuidBytes<6, 1, 5>(guid);
                break;
            }
            case MOVE_SWIM:
            {
                //! 5.4.1
                data.Initialize(SMSG_MOVE_SET_SWIM_SPEED, 1 + 8 + 4 + 4);
    
                data.WriteGuidMask<6, 3, 1, 2, 0, 4, 7, 5>(guid);
                data.WriteGuidBytes<2>(guid);
                data << float(GetSpeed(mtype));
                data.WriteGuidBytes<1, 6>(guid);
                data << uint32(0); // Unk Int32
                data.WriteGuidBytes<3, 4, 0, 7, 5>(guid);
                break;
            }
            case MOVE_SWIM_BACK:
            {
                //! 5.4.1
                data.Initialize(SMSG_MOVE_SET_SWIM_BACK_SPEED, 1 + 8 + 4 + 4);
    
                data.WriteGuidMask<0, 6, 5, 2, 1, 7, 4, 3>(guid);
                data.WriteGuidBytes<0, 3, 6>(guid);
                data << uint32(0); // Unk Int32
                data.WriteGuidBytes<5>(guid);
                data << float(GetSpeed(mtype));
                data.WriteGuidBytes<2, 1, 7, 4>(guid);
                break;
            }
            case MOVE_TURN_RATE:
            {
                //! 5.4.1
                data.Initialize(SMSG_MOVE_SET_TURN_RATE, 1 + 8 + 4 + 4);
                
                data.WriteGuidMask<1, 7, 3, 0, 5, 4, 6, 2>(guid);
                data << float(GetSpeed(mtype));
                data.WriteGuidBytes<2, 7, 1, 5, 6, 0, 4, 3>(guid);
                data << uint32(0); // Unk Int32
                break;
            }
            case MOVE_FLIGHT:
            {
                //! 5.4.1
                data.Initialize(SMSG_MOVE_SET_FLIGHT_SPEED, 1 + 8 + 4 + 4);
    
                data.WriteGuidMask<3, 0, 2, 4, 6, 1, 5, 7>(guid);
                data.WriteGuidBytes<2, 7, 1>(guid);
                data << float(GetSpeed(mtype));
                data.WriteGuidBytes<0, 4, 5>(guid);
                data << uint32(0); // Unk Int32
                data.WriteGuidBytes<6, 3>(guid);
                break;
            }
            case MOVE_FLIGHT_BACK:
            {
                //! 5.4.1
                data.Initialize(SMSG_MOVE_SET_FLIGHT_BACK_SPEED, 1 + 8 + 4 + 4);
    
                data.WriteGuidMask<1, 0, 7, 2, 3, 5, 4, 6>(guid);
                data << float(GetSpeed(mtype));
                data.WriteGuidBytes<1, 4, 0, 3, 6, 7, 2, 5>(guid);
                data << uint32(0); // Unk Int32
                break;
            }
            case MOVE_PITCH_RATE:
            {
                //! 5.4.1
                data.Initialize(SMSG_MOVE_SET_PITCH_RATE, 1 + 8 + 4 + 4);
                
                data.WriteGuidMask<7, 5, 2, 6, 1, 3, 0, 4>(guid);
                data.WriteGuidBytes<2, 6>(guid);
                data << float(GetSpeed(mtype));
                data.WriteGuidBytes<3, 1, 5, 4, 0>(guid);
                data << uint32(0); // Unk Int32
                data.WriteGuidBytes<7>(guid);
                break;
            }
            default:
                sLog->outError(LOG_FILTER_UNITS, "Unit::SetSpeed: Unsupported move type (%d), data not sent to client.", mtype);
                return;
        }
        SendMessageToSet(&data, true);
    }
}

void Unit::setDeathState(DeathState s)
{
    if (s != ALIVE && s != JUST_RESPAWNED)
    {
        CombatStop();
        DeleteThreatList();
        getHostileRefManager().deleteReferences();

        if (IsNonMeleeSpellCasted(false))
            InterruptNonMeleeSpells(false);

        ExitVehicle();                                      // Exit vehicle before calling RemoveAllControlled
                                                            // vehicles use special type of charm that is not removed by the next function
                                                            // triggering an assert
        UnsummonAllTotems();
        RemoveAllControlled();
        RemoveAllAurasOnDeath();
    }

    if (s == JUST_DIED)
    {
        ModifyAuraState(AURA_STATE_HEALTHLESS_20_PERCENT, false);
        ModifyAuraState(AURA_STATE_HEALTHLESS_35_PERCENT, false);
        // remove aurastates allowing special moves
        ClearAllReactives();
        ClearDiminishings();
        if (IsInWorld())
        {
            // Only clear MotionMaster for entities that exists in world
            // Avoids crashes in the following conditions :
            //  * Using 'call pet' on dead pets
            //  * Using 'call stabled pet'
            //  * Logging in with dead pets
            GetMotionMaster()->Clear(false);
            GetMotionMaster()->MoveIdle();
        }
        StopMoving();
        DisableSpline();
        // without this when removing IncreaseMaxHealth aura player may stuck with 1 hp
        // do not why since in IncreaseMaxHealth currenthealth is checked
        SetHealth(0);
        //SetPower(getPowerType(), 0);

        // players in instance don't have ZoneScript, but they have InstanceScript
        if (ZoneScript* zoneScript = GetZoneScript() ? GetZoneScript() : (ZoneScript*)GetInstanceScript())
            zoneScript->OnUnitDeath(this);

        if (isPet())
        {
            if (Unit* owner = GetOwner())
            {
                // Fix Demonic Rebirth
                if (owner->HasAura(108559) && !owner->HasAura(89140))
                {
                    owner->CastSpell(owner, 88448, true);
                    owner->CastSpell(owner, 89140, true); // Cooldown marker
                }
            }
        }
    }
    else if (s == JUST_RESPAWNED)
        RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_SKINNABLE); // clear skinnable for creature and player (at battleground)

    m_deathState = s;
}

/*########################################
########                          ########
########       AGGRO SYSTEM       ########
########                          ########
########################################*/
bool Unit::CanHaveThreatList() const
{
    // only creatures can have threat list
    if (GetTypeId() != TYPEID_UNIT)
        return false;

    // only alive units can have threat list
    if (!isAlive() || isDying())
        return false;

    // totems can not have threat list
    if (ToCreature()->isTotem())
        return false;

    // vehicles can not have threat list
    //if (ToCreature()->IsVehicle())
    //    return false;

    // summons can not have a threat list, unless they are controlled by a creature
    if (HasUnitTypeMask(UNIT_MASK_MINION | UNIT_MASK_GUARDIAN | UNIT_MASK_CONTROLABLE_GUARDIAN) && IS_PLAYER_GUID(((Pet*)this)->GetOwnerGUID()))
        return false;

    return true;
}

//======================================================================

float Unit::ApplyTotalThreatModifier(float fThreat, SpellSchoolMask schoolMask)
{
    if (!HasAuraType(SPELL_AURA_MOD_THREAT) || fThreat < 0)
        return fThreat;

    SpellSchools school = GetFirstSchoolInMask(schoolMask);

    return fThreat * m_threatModifier[school];
}

//======================================================================

void Unit::AddThreat(Unit* victim, float fThreat, SpellSchoolMask schoolMask, SpellInfo const* threatSpell)
{
    // Only mobs can manage threat lists
    if (CanHaveThreatList())
        m_ThreatManager.addThreat(victim, fThreat, schoolMask, threatSpell);
}

//======================================================================

void Unit::DeleteThreatList()
{
    if (CanHaveThreatList() && !m_ThreatManager.isThreatListEmpty())
        SendClearThreatListOpcode();
    m_ThreatManager.clearReferences();
    ClearSaveThreatTarget();
}

//======================================================================

void Unit::DeleteFromThreatList(Unit* victim)
{
    if (CanHaveThreatList() && !m_ThreatManager.isThreatListEmpty())
    {
        // remove unreachable target from our threat list
        // next tick we will select next possible target
        m_HostileRefManager.deleteReference(victim);
        m_ThreatManager.modifyThreatPercent(victim, -101);
       // _removeAttacker(victim);
    }
}

//======================================================================

void Unit::TauntApply(Unit* taunter)
{
    ASSERT(GetTypeId() == TYPEID_UNIT);

    if (!taunter || (taunter->GetTypeId() == TYPEID_PLAYER && taunter->ToPlayer()->isGameMaster()))
        return;

    if (!CanHaveThreatList())
        return;

    Creature* creature = ToCreature();

    if (creature->HasReactState(REACT_PASSIVE))
        return;

    Unit* target = getVictim();
    if (target && target == taunter)
        return;

    SetInFront(taunter);
    if (creature->IsAIEnabled)
        creature->AI()->AttackStart(taunter);

    //m_ThreatManager.tauntApply(taunter);
}

//======================================================================

void Unit::TauntFadeOut(Unit* taunter)
{
    ASSERT(GetTypeId() == TYPEID_UNIT);

    if (!taunter || (taunter->GetTypeId() == TYPEID_PLAYER && taunter->ToPlayer()->isGameMaster()))
        return;

    if (!CanHaveThreatList())
        return;

    Creature* creature = ToCreature();

    if (creature->HasReactState(REACT_PASSIVE))
        return;

    Unit* target = getVictim();
    if (!target || target != taunter)
        return;

    if (m_ThreatManager.isThreatListEmpty())
    {
        if (creature->IsAIEnabled)
            creature->AI()->EnterEvadeMode();
        return;
    }

    target = creature->SelectVictim();  // might have more taunt auras remaining

    if (target && target != taunter)
    {
        SetInFront(target);
        if (creature->IsAIEnabled)
            creature->AI()->AttackStart(target);
    }
}

//======================================================================

Unit* Creature::SelectVictim()
{
    // function provides main threat functionality
    // next-victim-selection algorithm and evade mode are called
    // threat list sorting etc.

    Unit* target = NULL;
    // First checking if we have some taunt on us
    AuraEffectList tauntAuras = GetAuraEffectsByType(SPELL_AURA_MOD_TAUNT);
    if (!tauntAuras.empty())
    {
        Unit* caster = tauntAuras.back()->GetCaster();

        // The last taunt aura caster is alive an we are happy to attack him
        if (caster && caster->isAlive())
            return getVictim();
        else if (tauntAuras.size() > 1)
        {
            // We do not have last taunt aura caster but we have more taunt auras,
            // so find first available target

            // Auras are pushed_back, last caster will be on the end
            AuraEffectList::const_iterator aura = --tauntAuras.end();
            do
            {
                --aura;
                caster = (*aura)->GetCaster();
                if (caster && canSeeOrDetect(caster, true) && IsValidAttackTarget(caster) && caster->isInAccessiblePlaceFor(ToCreature()))
                {
                    target = caster;
                    break;
                }
            } while (aura != tauntAuras.begin());
        }
        else
            target = getVictim();
    }

    if (CanHaveThreatList())
    {
        if (!target && !m_ThreatManager.isThreatListEmpty())
            // No taunt aura or taunt aura caster is dead standard target selection
            target = m_ThreatManager.getHostilTarget();

        //If target in agrolist check onli friend
        if (target && !IsFriendlyTo(target) && canCreatureAttack(target))
        {
            if (!HasUnitState(UNIT_STATE_CASTING))
                SetInFront(target);
            return target;
        }
    }
    else if (!HasReactState(REACT_PASSIVE))
    {
        // We have player pet probably
        target = getAttackerForHelper();
        if (!target && isSummon())
        {
            if (Unit* owner = ToTempSummon()->GetOwner())
            {
                if (owner->isInCombat())
                    target = owner->getAttackerForHelper();
                if (!target)
                {
                    for (ControlList::const_iterator itr = owner->m_Controlled.begin(); itr != owner->m_Controlled.end(); ++itr)
                    {
                        if ((*itr)->isInCombat())
                        {
                            target = (*itr)->getAttackerForHelper();
                            if (target)
                                break;
                        }
                    }
                }
            }
        }
    }
    else
        return NULL;

    if (target && _IsTargetAcceptable(target) && canCreatureAttack(target))
    {
        SetInFront(target);
        return target;
    }

    // last case when creature must not go to evade mode:
    // it in combat but attacker not make any damage and not enter to aggro radius to have record in threat list
    // for example at owner command to pet attack some far away creature
    // Note: creature does not have targeted movement generator but has attacker in this case
    for (AttackerSet::const_iterator itr = m_attackers.begin(); itr != m_attackers.end(); ++itr)
    {
        if ((*itr) && !canCreatureAttack(*itr) && (*itr)->GetTypeId() != TYPEID_PLAYER
        && !(*itr)->ToCreature()->HasUnitTypeMask(UNIT_MASK_CONTROLABLE_GUARDIAN))
            return NULL;
    }

    // TODO: a vehicle may eat some mob, so mob should not evade
    if (GetVehicle())
        return NULL;

    // search nearby enemy before enter evade mode
    if (HasReactState(REACT_AGGRESSIVE))
    {
        target = SelectNearestTargetInAttackDistance(m_CombatDistance ? m_CombatDistance : ATTACK_DISTANCE);

        if (target && _IsTargetAcceptable(target) && canCreatureAttack(target))
            return target;
    }

    Unit::AuraEffectList iAuras = GetAuraEffectsByType(SPELL_AURA_MOD_INVISIBILITY);
    if (!iAuras.empty())
    {
        for (Unit::AuraEffectList::const_iterator itr = iAuras.begin(); itr != iAuras.end(); ++itr)
        {
            if ((*itr)->GetBase()->IsPermanent())
            {
                AI()->EnterEvadeMode();
                break;
            }
        }
        return NULL;
    }

    // enter in evade mode in other case
    AI()->EnterEvadeMode();

    return NULL;
}

bool Unit::GetThreatTarget(uint64 targetGuid)
{
    for (std::list<uint64>::const_iterator itr = m_savethreatlist.begin(); itr != m_savethreatlist.end(); ++itr)
        if ((*itr) == targetGuid)
            return true;

    return false;
}

//======================================================================
//======================================================================
//======================================================================

float Unit::ApplyEffectModifiers(SpellInfo const* spellProto, uint8 effect_index, float value) const
{
    if (Player* modOwner = GetSpellModOwner())
    {
        modOwner->ApplySpellMod(spellProto->Id, SPELLMOD_ALL_EFFECTS, value);
        switch (effect_index)
        {
            case 0:
                modOwner->ApplySpellMod(spellProto->Id, SPELLMOD_EFFECT1, value);
                break;
            case 1:
                modOwner->ApplySpellMod(spellProto->Id, SPELLMOD_EFFECT2, value);
                break;
            case 2:
                modOwner->ApplySpellMod(spellProto->Id, SPELLMOD_EFFECT3, value);
                break;
            case 3:
                modOwner->ApplySpellMod(spellProto->Id, SPELLMOD_EFFECT4, value);
                break;
            case 4:
                modOwner->ApplySpellMod(spellProto->Id, SPELLMOD_EFFECT5, value);
                break;
        }
    }
    return value;
}

// function uses real base points (typically value - 1)
int32 Unit::CalculateSpellDamage(Unit const* target, SpellInfo const* spellProto, uint8 effect_index, int32 const* basePoints, Item* m_castitem, bool lockBasePoints) const
{
    return spellProto->GetEffect(effect_index, GetSpawnMode())->CalcValue(this, basePoints, target, m_castitem, lockBasePoints);
}

int32 Unit::CalcSpellDuration(SpellInfo const* spellProto)
{
    uint8 comboPoints = m_movedPlayer ? m_movedPlayer->GetComboPointsForDuration() : 0;
    uint8 holyPower   = 0;

    int32 minduration = spellProto->GetDuration();
    int32 maxduration = spellProto->GetMaxDuration();

    int32 duration;

    if (spellProto->PowerType == POWER_HOLY_POWER)
        holyPower = GetModForHolyPowerSpell();

    if (comboPoints && minduration != -1 && minduration != maxduration)
        duration = minduration + int32((maxduration - minduration) * comboPoints / 5);
    else if (holyPower && !(spellProto->AttributesEx8 & SPELL_ATTR8_HEALING_SPELL))
        duration = maxduration * holyPower;
    else
        duration = minduration;

    return duration;
}

int32 Unit::ModSpellDuration(SpellInfo const* spellProto, Unit const* target, int32 duration, bool positive, uint32 effectMask, Unit* caster)
{
    // don't mod permanent auras duration
    if (duration < 0)
        return duration;

    // some auras are not affected by duration modifiers
    if (spellProto->AttributesEx7 & SPELL_ATTR7_IGNORE_DURATION_MODS)
        return duration;

    if (caster)
    {
        // Skull Bash
        // need find other interrupt spells, whose interrupt duration is affected by spellmods and
        // implement proper code
        if (spellProto->Id == 93985)
            if (Player* modOwner = caster->GetSpellModOwner())
                modOwner->ApplySpellMod(spellProto->Id, SPELLMOD_DURATION, duration);
    }

    // cut duration only of negative effects
    if (!positive)
    {
        int32 mechanic = spellProto->GetSpellMechanicMaskByEffectMask(effectMask);

        int32 durationMod;
        int32 durationMod_always = 0;
        int32 durationMod_not_stack = 0;

        for (uint8 i = 1; i <= MECHANIC_ENRAGED; ++i)
        {
            if (!(mechanic & 1<<i))
                continue;
            // Find total mod value (negative bonus)
            int32 new_durationMod_always = target->GetTotalAuraModifierByMiscValue(SPELL_AURA_MECHANIC_DURATION_MOD, i);
            // Find max mod (negative bonus)
            int32 new_durationMod_not_stack = target->GetMaxNegativeAuraModifierByMiscValue(SPELL_AURA_MECHANIC_DURATION_MOD_NOT_STACK, i);
            // Check if mods applied before were weaker
            if (new_durationMod_always < durationMod_always)
                durationMod_always = new_durationMod_always;
            if (new_durationMod_not_stack < durationMod_not_stack)
                durationMod_not_stack = new_durationMod_not_stack;
        }

        // Select strongest negative mod
        if (durationMod_always > durationMod_not_stack)
            durationMod = durationMod_not_stack;
        else
            durationMod = durationMod_always;

        if (durationMod != 0)
            AddPct(duration, durationMod);

        // there are only negative mods currently
        durationMod_always = target->GetTotalAuraModifierByMiscValue(SPELL_AURA_MOD_AURA_DURATION_BY_DISPEL, spellProto->Dispel);
        durationMod_not_stack = target->GetMaxNegativeAuraModifierByMiscValue(SPELL_AURA_MOD_AURA_DURATION_BY_DISPEL_NOT_STACK, spellProto->Dispel);

        durationMod = 0;
        if (durationMod_always > durationMod_not_stack)
            durationMod += durationMod_not_stack;
        else
            durationMod += durationMod_always;

        if (durationMod != 0)
            AddPct(duration, durationMod);
    }
    else
    {
        // else positive mods here, there are no currently
        // when there will be, change GetTotalAuraModifierByMiscValue to GetTotalPositiveAuraModifierByMiscValue

        // Mixology - duration boost
        if (target->GetTypeId() == TYPEID_PLAYER)
        {
            if (/*spellProto->SpellFamilyName == SPELLFAMILY_POTION && */(
                sSpellMgr->IsSpellMemberOfSpellGroup(spellProto->Id, SPELL_GROUP_ELIXIR_BATTLE) ||
                sSpellMgr->IsSpellMemberOfSpellGroup(spellProto->Id, SPELL_GROUP_ELIXIR_GUARDIAN)))
            {
                if (target->HasAura(53042) && target->HasSpell(spellProto->GetEffect(0, GetSpawnMode())->TriggerSpell))
                    duration *= 2;
            }
        }
    }

    // Glyphs which increase duration of selfcasted buffs
    if (target == this)
    {
        switch (spellProto->SpellFamilyName)
        {
            case SPELLFAMILY_DRUID:
                if (spellProto->SpellFamilyFlags[0] & 0x100)
                {
                    // Glyph of Thorns
                    if (AuraEffect* aurEff = GetAuraEffect(57862, 0))
                        duration += aurEff->GetAmount() * MINUTE * IN_MILLISECONDS;
                }
                break;
        }
    }
    return std::max(duration, 0);
}

void Unit::ModSpellCastTime(SpellInfo const* spellProto, int32 & castTime, Spell* spell)
{
    if (!spellProto || castTime < 0)
        return;
    // called from caster
    if (Player* modOwner = GetSpellModOwner())
    {
        modOwner->ApplySpellMod(spellProto->Id, SPELLMOD_CASTING_TIME, castTime, spell);
        if(modOwner->HasAura(55442) && spellProto->Id == 118905) //Glyph of Capacitor Totem
            castTime -= 2000;
    }

    if (!(spellProto->Attributes & (SPELL_ATTR0_ABILITY|SPELL_ATTR0_TRADESPELL)) && ((GetTypeId() == TYPEID_PLAYER && spellProto->SpellFamilyName) || GetTypeId() == TYPEID_UNIT))
        castTime = int32(float(castTime) * GetFloatValue(UNIT_MOD_CAST_SPEED));
    else if (spellProto->Attributes & SPELL_ATTR0_REQ_AMMO && !(spellProto->AttributesEx2 & SPELL_ATTR2_AUTOREPEAT_FLAG))
        castTime = int32(float(castTime) * m_modAttackSpeedPct[RANGED_ATTACK]);
    else if (spellProto->SpellVisual[0] == 3881 && HasAura(67556)) // cooking with Chef Hat.
        castTime = 500;

    if(getClass() == CLASS_DEATH_KNIGHT)
    {
        if(AuraEffect const* aurEff = GetAuraEffect(77616, 0))
            if(aurEff->GetAmount() == spellProto->Id)
                castTime = 0;
    }
    if (isMoving())
    {
        float mod = 1.0f;
        AuraEffectList mTotalAuraList = GetAuraEffectsByType(SPELL_AURA_MOD_CAST_TIME_WHILE_MOVING);
        for (AuraEffectList::const_iterator i = mTotalAuraList.begin(), next; i != mTotalAuraList.end(); i = next)
        {
            next = i;
            ++next;
            if ((*i)->IsAffectingSpell(spellProto))
                AddPct(mod, (*i)->GetAmount());
        }

        castTime *= mod;
    }

    CalculateCastTimeFromDummy(castTime, spellProto);
}

DiminishingLevels Unit::GetDiminishing(DiminishingGroup group)
{
    for (Diminishing::iterator i = m_Diminishing.begin(); i != m_Diminishing.end(); ++i)
    {
        if (i->DRGroup != group)
            continue;

        if (!i->hitCount)
            return DIMINISHING_LEVEL_1;

        if (!i->hitTime)
            return DIMINISHING_LEVEL_1;

        // If last spell was casted more than 15 seconds ago - reset the count.
        if (i->stack == 0 && getMSTimeDiff(i->hitTime, getMSTime()) > DiminishingDuration())
        {
            i->hitCount = DIMINISHING_LEVEL_1;
            return DIMINISHING_LEVEL_1;
        }
        // or else increase the count.
        else
            return DiminishingLevels(i->hitCount);
    }
    return DIMINISHING_LEVEL_1;
}

uint32 Unit::DiminishingDuration() const
{
    uint32 MSTime = getMSTime();
    if (MSTime > 5000)
    {
        uint32 checktime = MSTime / 5000;
        checktime *= 5000;

        return MSTime - checktime + 15000;
    }
    else
        return MSTime + 15000;
}

void Unit::IncrDiminishing(DiminishingGroup group)
{
    // Checking for existing in the table
    for (Diminishing::iterator i = m_Diminishing.begin(); i != m_Diminishing.end(); ++i)
    {
        if (i->DRGroup != group)
            continue;
        if (int32(i->hitCount) < GetDiminishingReturnsMaxLevel(group))
            i->hitCount += 1;
        return;
    }
    m_Diminishing.push_back(DiminishingReturn(group, getMSTime(), DIMINISHING_LEVEL_2));
}

float Unit::ApplyDiminishingToDuration(DiminishingGroup group, int32 &duration, Unit* caster, DiminishingLevels Level, int32 limitduration)
{
    if (duration == -1 || group == DIMINISHING_NONE)
        return 1.0f;

    // test pet/charm masters instead pets/charmeds
    Unit const* targetOwner = GetCharmerOrOwner();
    Unit const* casterOwner = caster->GetCharmerOrOwner();

    // Duration of crowd control abilities on pvp target is limited by 10 sec. (2.2.0)
    if (limitduration > 0 && duration > limitduration)
    {
        Unit const* target = targetOwner ? targetOwner : this;
        Unit const* source = casterOwner ? casterOwner : caster;

        if ((target->GetTypeId() == TYPEID_PLAYER
            || ((Creature*)target)->GetCreatureTemplate()->flags_extra & CREATURE_FLAG_EXTRA_ALL_DIMINISH)
            && source->GetTypeId() == TYPEID_PLAYER)
            duration = limitduration;
    }

    float mod = 1.0f;

    if (group == DIMINISHING_TAUNT)
    {
        if (GetTypeId() == TYPEID_UNIT && (ToCreature()->GetCreatureTemplate()->flags_extra & CREATURE_FLAG_EXTRA_TAUNT_DIMINISH))
        {
            DiminishingLevels diminish = Level;
            switch (diminish)
            {
                case DIMINISHING_LEVEL_1: break;
                case DIMINISHING_LEVEL_2: mod = 0.65f; break;
                case DIMINISHING_LEVEL_3: mod = 0.4225f; break;
                case DIMINISHING_LEVEL_4: mod = 0.274625f; break;
                case DIMINISHING_LEVEL_TAUNT_IMMUNE: mod = 0.0f; break;
                default: break;
            }
        }
    }
    // Some diminishings applies to mobs too (for example, Stun)
    else if ((GetDiminishingReturnsGroupType(group) == DRTYPE_PLAYER
        && ((targetOwner ? (targetOwner->GetTypeId() == TYPEID_PLAYER) : (GetTypeId() == TYPEID_PLAYER))
        || (GetTypeId() == TYPEID_UNIT && ToCreature()->GetCreatureTemplate()->flags_extra & CREATURE_FLAG_EXTRA_ALL_DIMINISH)))
        || GetDiminishingReturnsGroupType(group) == DRTYPE_ALL)
    {
        DiminishingLevels diminish = Level;
        switch (diminish)
        {
            case DIMINISHING_LEVEL_1: break;
            case DIMINISHING_LEVEL_2: mod = 0.5f; break;
            case DIMINISHING_LEVEL_3: mod = 0.25f; break;
            case DIMINISHING_LEVEL_IMMUNE: mod = 0.0f; break;
            default: break;
        }
    }

    duration = int32(duration * mod);
    return mod;
}

void Unit::ApplyDiminishingAura(DiminishingGroup group, bool apply)
{
    // Checking for existing in the table
    for (Diminishing::iterator i = m_Diminishing.begin(); i != m_Diminishing.end(); ++i)
    {
        if (i->DRGroup != group)
            continue;

        if (apply)
            i->stack += 1;
        else if (i->stack)
        {
            i->stack -= 1;
            // Remember time after last aura from group removed
            if (i->stack == 0)
                i->hitTime = getMSTime();
        }
        break;
    }
}

float Unit::GetSpellMaxRangeForTarget(Unit const* target, SpellInfo const* spellInfo) const
{
    if (!spellInfo->RangeEntry)
        return 0;

    if (spellInfo->RangeEntry->maxRangeFriend == spellInfo->RangeEntry->maxRangeHostile)
        return spellInfo->GetMaxRange();

    if (!target)
    	return spellInfo->RangeEntry->maxRangeFriend;

    return spellInfo->GetMaxRange(!IsHostileTo(target));
}

float Unit::GetSpellMinRangeForTarget(Unit const* target, SpellInfo const* spellInfo) const
{
    if (!spellInfo->RangeEntry)
        return 0;
    if (spellInfo->RangeEntry->minRangeFriend == spellInfo->RangeEntry->minRangeHostile)
        return spellInfo->GetMinRange();
    return spellInfo->GetMinRange(!IsHostileTo(target));
}

Unit* Unit::GetUnit(WorldObject& object, uint64 guid)
{
    return ObjectAccessor::GetUnit(object, guid);
}

Player* Unit::GetPlayer(WorldObject& object, uint64 guid)
{
    return ObjectAccessor::GetPlayer(object, guid);
}

Creature* Unit::GetCreature(WorldObject& object, uint64 guid)
{
    return object.GetMap()->GetCreature(guid);
}

uint32 Unit::GetCreatureType() const
{
    if (GetTypeId() == TYPEID_PLAYER)
    {
        ShapeshiftForm form = GetShapeshiftForm();
        SpellShapeshiftFormEntry const* ssEntry = sSpellShapeshiftFormStore.LookupEntry(form);
        if (ssEntry && ssEntry->creatureType > 0)
            return ssEntry->creatureType;
        else
            return CREATURE_TYPE_HUMANOID;
    }
    else
        return ToCreature()->GetCreatureTemplate()->type;
}

/*#######################################
########                         ########
########       STAT SYSTEM       ########
########                         ########
#######################################*/

bool Unit::HandleStatModifier(UnitMods unitMod, UnitModifierType modifierType, float amount, bool apply)
{
    if (unitMod >= UNIT_MOD_END || modifierType >= MODIFIER_TYPE_END)
    {
        sLog->outError(LOG_FILTER_UNITS, "ERROR in HandleStatModifier(): non-existing UnitMods or wrong UnitModifierType!");
        return false;
    }

    switch (modifierType)
    {
        case BASE_VALUE:
        case TOTAL_VALUE:
            m_auraModifiersGroup[unitMod][modifierType] += apply ? amount : -amount;
            break;
        case BASE_PCT:
        case TOTAL_PCT:
            ApplyPercentModFloatVar(m_auraModifiersGroup[unitMod][modifierType], amount, apply);
            break;
        default:
            break;
    }

    if (!CanModifyStats())
        return false;

    switch (unitMod)
    {
        case UNIT_MOD_STAT_STRENGTH:
        case UNIT_MOD_STAT_AGILITY:
        case UNIT_MOD_STAT_STAMINA:
        case UNIT_MOD_STAT_INTELLECT:
        case UNIT_MOD_STAT_SPIRIT:         UpdateStats(GetStatByAuraGroup(unitMod));  break;

        case UNIT_MOD_ARMOR:               UpdateArmor();           break;
        case UNIT_MOD_HEALTH:              UpdateMaxHealth();       break;

        case UNIT_MOD_MANA:
        case UNIT_MOD_RAGE:
        case UNIT_MOD_FOCUS:
        case UNIT_MOD_ENERGY:
        case UNIT_MOD_RUNE:
        case UNIT_MOD_RUNIC_POWER:          UpdateMaxPower(GetPowerTypeByAuraGroup(unitMod));          break;

        case UNIT_MOD_RESISTANCE_HOLY:
        case UNIT_MOD_RESISTANCE_FIRE:
        case UNIT_MOD_RESISTANCE_NATURE:
        case UNIT_MOD_RESISTANCE_FROST:
        case UNIT_MOD_RESISTANCE_SHADOW:
        case UNIT_MOD_RESISTANCE_ARCANE:   UpdateResistances(GetSpellSchoolByAuraGroup(unitMod));      break;

        case UNIT_MOD_ATTACK_POWER:        UpdateAttackPowerAndDamage();         break;
        case UNIT_MOD_ATTACK_POWER_RANGED: UpdateAttackPowerAndDamage(true);     break;

        case UNIT_MOD_DAMAGE_MAINHAND:     UpdateDamagePhysical(BASE_ATTACK);    break;
        case UNIT_MOD_DAMAGE_OFFHAND:      UpdateDamagePhysical(OFF_ATTACK);     break;
        case UNIT_MOD_DAMAGE_RANGED:       UpdateDamagePhysical(RANGED_ATTACK);  break;

        default:
            break;
    }

    return true;
}

float Unit::GetModifierValue(UnitMods unitMod, UnitModifierType modifierType) const
{
    if (unitMod >= UNIT_MOD_END || modifierType >= MODIFIER_TYPE_END)
    {
        sLog->outError(LOG_FILTER_UNITS, "attempt to access non-existing modifier value from UnitMods!");
        return 0.0f;
    }

    if (modifierType == TOTAL_PCT && m_auraModifiersGroup[unitMod][modifierType] <= 0.0f)
        return 0.0f;

    return m_auraModifiersGroup[unitMod][modifierType];
}

float Unit::GetTotalStatValue(Stats stat) const
{
    UnitMods unitMod = UnitMods(UNIT_MOD_STAT_START + stat);

    if (m_auraModifiersGroup[unitMod][TOTAL_PCT] <= 0.0f)
        return 0.0f;

    // value = ((base_value * base_pct) + total_value) * total_pct
    float value  = m_auraModifiersGroup[unitMod][BASE_VALUE] + GetCreateStat(stat);
    value *= m_auraModifiersGroup[unitMod][BASE_PCT];
    value += m_auraModifiersGroup[unitMod][TOTAL_VALUE];
    value *= GetTotalAuraMultiplierByMiscMask(SPELL_AURA_MOD_TOTAL_STAT_PERCENTAGE, (1 << stat), true, true);

    return value;
}

float Unit::GetTotalAuraModValue(UnitMods unitMod) const
{
    if (unitMod >= UNIT_MOD_END)
    {
        sLog->outError(LOG_FILTER_UNITS, "attempt to access non-existing UnitMods in GetTotalAuraModValue()!");
        return 0.0f;
    }

    if (m_auraModifiersGroup[unitMod][TOTAL_PCT] <= 0.0f)
        return 0.0f;

    float value = m_auraModifiersGroup[unitMod][BASE_VALUE];
    value *= m_auraModifiersGroup[unitMod][BASE_PCT];
    value += m_auraModifiersGroup[unitMod][TOTAL_VALUE];
    value *= m_auraModifiersGroup[unitMod][TOTAL_PCT];

    return value;
}

SpellSchools Unit::GetSpellSchoolByAuraGroup(UnitMods unitMod) const
{
    SpellSchools school = SPELL_SCHOOL_NORMAL;

    switch (unitMod)
    {
        case UNIT_MOD_RESISTANCE_HOLY:     school = SPELL_SCHOOL_HOLY;          break;
        case UNIT_MOD_RESISTANCE_FIRE:     school = SPELL_SCHOOL_FIRE;          break;
        case UNIT_MOD_RESISTANCE_NATURE:   school = SPELL_SCHOOL_NATURE;        break;
        case UNIT_MOD_RESISTANCE_FROST:    school = SPELL_SCHOOL_FROST;         break;
        case UNIT_MOD_RESISTANCE_SHADOW:   school = SPELL_SCHOOL_SHADOW;        break;
        case UNIT_MOD_RESISTANCE_ARCANE:   school = SPELL_SCHOOL_ARCANE;        break;

        default:
            break;
    }

    return school;
}

Stats Unit::GetStatByAuraGroup(UnitMods unitMod) const
{
    Stats stat = STAT_STRENGTH;

    switch (unitMod)
    {
        case UNIT_MOD_STAT_STRENGTH:    stat = STAT_STRENGTH;      break;
        case UNIT_MOD_STAT_AGILITY:     stat = STAT_AGILITY;       break;
        case UNIT_MOD_STAT_STAMINA:     stat = STAT_STAMINA;       break;
        case UNIT_MOD_STAT_INTELLECT:   stat = STAT_INTELLECT;     break;
        case UNIT_MOD_STAT_SPIRIT:      stat = STAT_SPIRIT;        break;

        default:
            break;
    }

    return stat;
}

Powers Unit::GetPowerTypeByAuraGroup(UnitMods unitMod) const
{
    switch (unitMod)
    {
        case UNIT_MOD_RAGE:        return POWER_RAGE;
        case UNIT_MOD_FOCUS:       return POWER_FOCUS;
        case UNIT_MOD_ENERGY:      return POWER_ENERGY;
        case UNIT_MOD_RUNE:        return POWER_RUNES;
        case UNIT_MOD_RUNIC_POWER: return POWER_RUNIC_POWER;
        default:
        case UNIT_MOD_MANA:        return POWER_MANA;
    }
}

float Unit::GetTotalAttackPowerValue(WeaponAttackType attType) const
{
    if (attType == RANGED_ATTACK)
    {
        int32 ap = GetInt32Value(UNIT_FIELD_RANGED_ATTACK_POWER) + GetInt32Value(UNIT_FIELD_RANGED_ATTACK_POWER_MOD_POS) - GetInt32Value(UNIT_FIELD_RANGED_ATTACK_POWER_MOD_NEG);
        if (ap < 0)
            return 0.0f;
        return ap * (1.0f + GetFloatValue(UNIT_FIELD_RANGED_ATTACK_POWER_MULTIPLIER));
    }
    else
    {
        int32 ap = GetInt32Value(UNIT_FIELD_ATTACK_POWER) + GetInt32Value(UNIT_FIELD_ATTACK_POWER_MOD_POS) - GetInt32Value(UNIT_FIELD_ATTACK_POWER_MOD_NEG);
        if (ap < 0)
            return 0.0f;
        return ap * (1.0f + GetFloatValue(UNIT_FIELD_ATTACK_POWER_MULTIPLIER));
    }
}

float Unit::GetWeaponDamageRange(WeaponAttackType attType, WeaponDamageRange type) const
{
    if (attType == OFF_ATTACK && !haveOffhandWeapon())
        return 0.0f;

    return m_weaponDamage[attType][type];
}

void Unit::SetLevel(uint8 lvl)
{
    SetUInt32Value(UNIT_FIELD_LEVEL, lvl);

    // group update
    if (GetTypeId() == TYPEID_PLAYER && ToPlayer()->GetGroup())
        ToPlayer()->SetGroupUpdateFlag(GROUP_UPDATE_FLAG_LEVEL);

    if (GetTypeId() == TYPEID_PLAYER)
        sWorld->UpdateCharacterNameDataLevel(ToPlayer()->GetGUIDLow(), lvl);
}

void Unit::SetHealth(uint32 val)
{
    if (getDeathState() == JUST_DIED)
        val = 0;
    else if (GetTypeId() == TYPEID_PLAYER && getDeathState() == DEAD)
        val = 1;
    else
    {
        uint32 maxHealth = GetMaxHealth();
        if (maxHealth < val)
            val = maxHealth;
    }

    SetUInt32Value(UNIT_FIELD_HEALTH, val);

    // group update
    if (Player* player = ToPlayer())
    {
        if (player->GetGroup())
            player->SetGroupUpdateFlag(GROUP_UPDATE_FLAG_CUR_HP);
    }
    else if (Pet* pet = ToCreature()->ToPet())
    {
        if (pet->isControlled())
        {
            Unit* owner = GetOwner();
            if (owner && (owner->GetTypeId() == TYPEID_PLAYER) && owner->ToPlayer()->GetGroup())
                owner->ToPlayer()->SetGroupUpdateFlag(GROUP_UPDATE_FLAG_PET_CUR_HP);
        }
    }
}

void Unit::SetMaxHealth(uint32 val)
{
    if (!val)
        val = 1;

    uint32 health = GetHealth();
    SetUInt32Value(UNIT_FIELD_MAXHEALTH, val);

    // group update
    if (GetTypeId() == TYPEID_PLAYER)
    {
        if (ToPlayer()->GetGroup())
            ToPlayer()->SetGroupUpdateFlag(GROUP_UPDATE_FLAG_MAX_HP);
    }
    else if (Pet* pet = ToCreature()->ToPet())
    {
        if (pet->isControlled())
        {
            Unit* owner = GetOwner();
            if (owner && (owner->GetTypeId() == TYPEID_PLAYER) && owner->ToPlayer()->GetGroup())
                owner->ToPlayer()->SetGroupUpdateFlag(GROUP_UPDATE_FLAG_PET_MAX_HP);
        }
    }

    if (val < health)
        SetHealth(val);
}

uint32 Unit::GetPowerIndexByClass(uint32 powerId, uint32 classId) const
{
    if (GetTypeId() != TYPEID_PLAYER)
    {
        if (powerId == POWER_ALTERNATE_POWER)
            return 1;
        return 0;
    }

    ChrClassesEntry const* classEntry = sChrClassesStore.LookupEntry(classId);

    //ASSERT(classEntry && "Class not found");
    if(!classEntry)
        return 0;

    uint32 index = 0;
    for (uint32 i = 0; i <= sChrPowerTypesStore.GetNumRows(); ++i)
    {
        ChrPowerTypesEntry const* powerEntry = sChrPowerTypesStore.LookupEntry(i);
        if (!powerEntry)
            continue;

        if (powerEntry->classId != classId)
            continue;

        if (powerEntry->power == powerId)
            return index;

        ++index;
    }

    // return invalid value - this class doesn't use this power
    return MAX_POWERS;
};

int32 Unit::GetPower(Powers power) const
{
    uint32 powerIndex = GetPowerIndexByClass(power, getClass());
    if (powerIndex == MAX_POWERS)
        return 0;

    return GetInt32Value(UNIT_FIELD_POWER1 + powerIndex);
}

int32 Unit::GetMaxPower(Powers power) const
{
    uint32 powerIndex = GetPowerIndexByClass(power, getClass());
    if (powerIndex == MAX_POWERS)
        return 0;

    return GetInt32Value(UNIT_FIELD_MAXPOWER1 + powerIndex);
}

int32 Unit::GetPowerForReset(Powers power, bool maxpower) const
{
    switch (power)
    {
        case POWER_BURNING_EMBERS:
            return  10;
        case POWER_SOUL_SHARDS:
            return  400;
        case POWER_DEMONIC_FURY:
            return  200;
        case POWER_MANA:
        case POWER_ENERGY:
        case POWER_FOCUS:
        {
            if(maxpower)
                return GetMaxPower(power);
            else
                return 0;
        }
        default:
            break;
    }

    return 0;
}

void Unit::InitialPowers(bool maxpower)
{
    int32 classId = getClass();
    ChrClassesEntry const* classEntry = sChrClassesStore.LookupEntry(classId);

    if(!classEntry)
        return;

    int32 count = 0;
    for (uint32 i = 0; i <= sChrPowerTypesStore.GetNumRows(); ++i)
    {
        ChrPowerTypesEntry const* powerEntry = sChrPowerTypesStore.LookupEntry(i);
        if (!powerEntry)
            continue;

        if (powerEntry->classId != classId)
            continue;

        if (powerEntry->power == POWER_ALTERNATE_POWER)
            continue;

        if (powerEntry->power != POWER_MANA && (classId == CLASS_WARLOCK || maxpower)) //warlock not send power > 0
            continue;

        count++;
    }

    if(!count)
        return;

    WorldPacket data(SMSG_POWER_UPDATE, 8 + 4 + 1 + 4);
    ObjectGuid guid = GetGUID();
    data.WriteGuidMask<1>(guid);
    data.WriteBits(count, 21);
    data.WriteGuidMask<3, 6, 0, 4, 2, 5, 7>(guid);
    data.FlushBits();
    data.WriteGuidBytes<1, 2, 0>(guid);

    int32 powerIndex = 0;
    for (uint32 i = 0; i <= sChrPowerTypesStore.GetNumRows(); ++i)
    {
        ChrPowerTypesEntry const* powerEntry = sChrPowerTypesStore.LookupEntry(i);
        if (!powerEntry)
            continue;

        if (powerEntry->classId != classId)
            continue;

        Powers power = Powers(powerEntry->power);
        int32 curval = GetPowerForReset(power, maxpower);

        if (power != POWER_ALTERNATE_POWER)
        {
            int32 createval = GetCreatePowers(power);
            if(maxpower)
            {
                SetInt32Value(UNIT_FIELD_MAXPOWER1 + powerIndex, createval);
                SetInt32Value(UNIT_FIELD_POWER1 + powerIndex, curval);
            }
            else
                SetInt32Value(UNIT_FIELD_POWER1 + powerIndex, curval);
        }

        powerIndex++;

        if (power != POWER_MANA && (classId == CLASS_WARLOCK || maxpower)) //warlock not send power > 0
            continue;

        data << uint8(power);
        data << int32(curval);

    }

    data.WriteGuidBytes<7, 4, 5, 6, 3>(guid);
    SendMessageToSet(&data, GetTypeId() == TYPEID_PLAYER ? true : false);
}

void Unit::SetPower(Powers power, int32 val, bool send)
{
    uint32 powerIndex = GetPowerIndexByClass(power, getClass());
    if (powerIndex == MAX_POWERS)
        return;

    int32 maxPower = int32(GetMaxPower(power));
    if (maxPower < val)
        val = maxPower;

    //Visualization for power
    VisualForPower(power, val);

    SetInt32Value(UNIT_FIELD_POWER1 + powerIndex, val);

    if (IsInWorld() && send)
    {
        //! 5.4.1
        WorldPacket data(SMSG_POWER_UPDATE, 8 + 4 + 1 + 4);
        ObjectGuid guid = GetGUID();

        data.WriteGuidMask<1>(guid);
        int powerCounter = 1;
        data.WriteBits(powerCounter, 21);
        data.WriteGuidMask<3, 6, 0, 4, 2, 5, 7>(guid);
        
        data.FlushBits();
        
        data.WriteGuidBytes<1, 2, 0>(guid);
        data << uint8(power);
        data << int32(val);
        data.WriteGuidBytes<7, 4, 5, 6, 3>(guid);
        SendMessageToSet(&data, GetTypeId() == TYPEID_PLAYER ? true : false);
    }
    
    if (Player* player = ToPlayer())
    {
        if (player->GetGroup())
            player->SetGroupUpdateFlag(GROUP_UPDATE_FLAG_CUR_POWER);
    }
    else if (Pet* pet = ToCreature()->ToPet())
    {
        if (pet->isControlled())
        {
            Unit* owner = GetOwner();
            if (owner && (owner->GetTypeId() == TYPEID_PLAYER) && owner->ToPlayer()->GetGroup())
                owner->ToPlayer()->SetGroupUpdateFlag(GROUP_UPDATE_FLAG_PET_CUR_POWER);
        }
    }
}

void Unit::SetMaxPower(Powers power, int32 val)
{
    uint32 powerIndex = GetPowerIndexByClass(power, getClass());
   
    if (powerIndex == MAX_POWERS)
        return;

    int32 cur_power = GetPower(power);
    SetInt32Value(UNIT_FIELD_MAXPOWER1 + powerIndex, val);

    // group update
    if (GetTypeId() == TYPEID_PLAYER)
    {
        if (ToPlayer()->GetGroup())
            ToPlayer()->SetGroupUpdateFlag(GROUP_UPDATE_FLAG_MAX_POWER);
    }
    else if (Pet* pet = ToCreature()->ToPet())
    {
        if (pet->isControlled())
        {
            Unit* owner = GetOwner();
            if (owner && (owner->GetTypeId() == TYPEID_PLAYER) && owner->ToPlayer()->GetGroup())
                owner->ToPlayer()->SetGroupUpdateFlag(GROUP_UPDATE_FLAG_PET_MAX_POWER);
        }
    }

    // if (val < cur_power)
        // SetPower(power, val);
}

int32 Unit::GetCreatePowers(Powers power) const
{
    switch (power)
    {
        case POWER_MANA:
            return GetCreateMana();
        case POWER_RAGE:
            return 1000;
        case POWER_FOCUS:
            if (GetTypeId() == TYPEID_PLAYER && getClass() == CLASS_HUNTER)
                return 100;
            return (GetTypeId() == TYPEID_PLAYER || !((Creature const*)this)->isPet() || ((Pet const*)this)->getPetType() != HUNTER_PET ? 0 : 100);
        case POWER_ENERGY:
            return ((ToPet() && ToPet()->IsWarlockPet()) ? 200 : 100);
        case POWER_RUNIC_POWER:
            return 1000;
        case POWER_RUNES:
            return 0;
        case POWER_SHADOW_ORB:
            return 3;
        case POWER_BURNING_EMBERS:
            return 40;
        case POWER_DEMONIC_FURY:
            return 1000;
        case POWER_SOUL_SHARDS:
            return 400;
        case POWER_ECLIPSE:
            return 100; // Should be -100 to 100 this needs the power to be int32 instead of uint32
        case POWER_HOLY_POWER:
            return 3;
        case POWER_HEALTH:
            return 0;
        case POWER_CHI:
            return 4;
        default:
            break;
    }

    return 0;
}

float Unit::OCTRegenMPPerSpirit()
{
    uint8 level = getLevel();
    uint32 pclass = getClass();

    if (level > GT_MAX_LEVEL)
        level = GT_MAX_LEVEL;

    GtRegenMPPerSptEntry  const* moreRatio = sGtRegenMPPerSptStore.LookupEntry((pclass-1) * GT_MAX_LEVEL + level-1);
    if (!moreRatio)
        return 0.0f;

    // Formula get from PaperDollFrame script
    return GetStat(STAT_SPIRIT) * moreRatio->ratio;
}

float Unit::GetSpellCritFromIntellect()
{
    uint8 level = getLevel();
    uint32 pclass = getClass();

    if (level > GT_MAX_LEVEL)
        level = GT_MAX_LEVEL;

    GtChanceToSpellCritBaseEntry const* critBase = sGtChanceToSpellCritBaseStore.LookupEntry((pclass - 1) * GT_MAX_LEVEL + level - 1);
    GtChanceToSpellCritEntry const* critRatio = sGtChanceToSpellCritStore.LookupEntry((pclass - 1) * GT_MAX_LEVEL + level - 1);
    if (critBase == NULL || critRatio == NULL)
        return 0.0f;

    float crit = ((GetStat(STAT_INTELLECT) - 1) / (critRatio->ratio * 100) + critBase->base);
    return crit * 100.0f;
}

float Unit::GetRatingMultiplier(CombatRating cr) const
{
    uint8 level = getLevel();

    if (level > GT_MAX_LEVEL)
        level = GT_MAX_LEVEL;

    GtCombatRatingsEntry const* Rating = sGtCombatRatingsStore.LookupEntry(cr*GT_MAX_LEVEL+level-1);
    // gtOCTClassCombatRatingScalarStore.dbc starts with 1, CombatRating with zero, so cr+1
    GtOCTClassCombatRatingScalarEntry const* classRating = sGtOCTClassCombatRatingScalarStore.LookupEntry((getClass()-1)*GT_MAX_RATING+cr+1);

    if (cr == CR_RESILIENCE_PLAYER_DAMAGE_TAKEN)
        return Rating->ratio;

    if (!Rating || !classRating)
        return 1.0f;                                        // By default use minimum coefficient (not must be called)

    return classRating->ratio / Rating->ratio;
}

void Unit::AddToWorld()
{
    if (!IsInWorld())
    {
        WorldObject::AddToWorld();
    }
}

void Unit::RemoveFromWorld()
{
    // cleanup
    ASSERT(GetGUID());

    if (IsInWorld())
    {
        m_VisibilityUpdateTask  = false;
        m_VisibilityUpdScheduled = false;
        m_duringRemoveFromWorld = true;
        if (IsVehicle())
            RemoveVehicleKit();

        RemoveCharmAuras();
        RemoveBindSightAuras();
        RemoveNotOwnSingleTargetAuras();

        RemoveAllGameObjects();
        RemoveAllDynObjects();
        RemoveAllAreaObjects();

        ExitVehicle();  // Remove applied auras with SPELL_AURA_CONTROL_VEHICLE
        UnsummonAllTotems();

        if (!IsVehicle()) // should be remove in deathstate
            RemoveAllControlled();

        if (!ToCreature())
            RemoveAreaAurasDueToLeaveWorld();

        if (GetCharmerGUID())
        {
            sLog->outFatal(LOG_FILTER_UNITS, "Unit %u has charmer guid when removed from world", GetEntry());
            ASSERT(false);
        }

        if (Unit* owner = GetOwner())
        {
            if (owner->m_Controlled.find(this) != owner->m_Controlled.end())
            {
                //sLog->outFatal(LOG_FILTER_UNITS, "Unit %u is in controlled list of %u when removed from world", GetEntry(), owner->GetEntry());
                //ASSERT(false);
                owner->m_Controlled.erase(this);
            }
        }

        WorldObject::RemoveFromWorld();
        m_duringRemoveFromWorld = false;
    }
}

void Unit::CleanupBeforeRemoveFromMap(bool finalCleanup)
{
    // This needs to be before RemoveFromWorld to make GetCaster() return a valid pointer on aura removal
    InterruptNonMeleeSpells(true);
    RemoveAllAuras();

    if (IsInWorld())
        RemoveFromWorld();

    //! ==-- DOUBLE DRAGON --==
    RemoveAllAuras();   //remove auras witch was added while we where removing from world.

    ASSERT(m_appliedAuras.empty());
    ASSERT(m_ownedAuras.empty());
    ASSERT(GetGUID());

    if (finalCleanup)
        m_cleanupDone = true;

    // A unit may be in removelist and not in world, but it is still in grid
    // and may have some references during delete
    RemoveAllGameObjects();

    m_Events.KillAllEvents(false);                      // non-delatable (currently casted spells) will not deleted now but it will deleted at call in Map::RemoveAllObjectsInRemoveList
    CombatStop();
    ClearComboPointHolders();
    DeleteThreatList();
    getHostileRefManager().setOnlineOfflineState(false);
    GetMotionMaster()->Clear(false);                    // remove different non-standard movement generators.
}

void Unit::CleanupsBeforeDelete(bool finalCleanup)
{
    CleanupBeforeRemoveFromMap(finalCleanup);

    if (Creature* thisCreature = ToCreature())
        if (GetTransport())
            GetTransport()->RemovePassenger(thisCreature);
}

void Unit::UpdateCharmAI()
{
    if (GetTypeId() == TYPEID_PLAYER)
        return;

    if (i_disabledAI) // disabled AI must be primary AI
    {
        if (!isCharmed())
        {
            delete i_AI;
            i_AI = i_disabledAI;
            i_disabledAI = NULL;
        }
    }
    else
    {
        if (isCharmed())
        {
            i_disabledAI = i_AI;
            if (isPossessed() || IsVehicle())
                i_AI = new PossessedAI(ToCreature());
            else
                i_AI = new PetAI(ToCreature());
        }
    }
}

CharmInfo* Unit::InitCharmInfo()
{
    if (!m_charmInfo)
        m_charmInfo = new CharmInfo(this);

    return m_charmInfo;
}

void Unit::DeleteCharmInfo()
{
    if (!m_charmInfo)
        return;

    m_charmInfo->RestoreState();
    delete m_charmInfo;
    m_charmInfo = NULL;
}

CharmInfo::CharmInfo(Unit* unit)
: m_unit(unit), m_CommandState(COMMAND_FOLLOW), m_petnumber(0), m_barInit(false),
  m_isCommandAttack(false), m_isAtStay(false), m_isFollowing(false), m_isReturning(false),
  m_stayX(0.0f), m_stayY(0.0f), m_stayZ(0.0f)
{
    for (uint8 i = 0; i < MAX_SPELL_CHARM; ++i)
        m_charmspells[i].SetActionAndType(0, ACT_DISABLED);

    if (m_unit->GetTypeId() == TYPEID_UNIT)
    {
        m_oldReactState = m_unit->ToCreature()->GetReactState();
        m_unit->ToCreature()->SetReactState(REACT_PASSIVE);
    }
}

CharmInfo::~CharmInfo()
{
}

void CharmInfo::RestoreState()
{
    if (m_unit->GetTypeId() == TYPEID_UNIT)
        if (Creature* creature = m_unit->ToCreature())
            creature->SetReactState(m_oldReactState);
}

void CharmInfo::InitPetActionBar()
{
    // the first 3 SpellOrActions are attack, follow and move-to
    for (uint32 i = 0; i < ACTION_BAR_INDEX_PET_SPELL_START - ACTION_BAR_INDEX_START; ++i)
    {
        if (i < 2)
            SetActionBar(ACTION_BAR_INDEX_START + i, COMMAND_ATTACK - i, ACT_COMMAND);
        else
            SetActionBar(ACTION_BAR_INDEX_START + i, COMMAND_MOVE_TO, ACT_COMMAND);
    }

    // middle 4 SpellOrActions are spells/special attacks/abilities
    for (uint32 i = 0; i < ACTION_BAR_INDEX_PET_SPELL_END-ACTION_BAR_INDEX_PET_SPELL_START; ++i)
        SetActionBar(ACTION_BAR_INDEX_PET_SPELL_START + i, 0, ACT_PASSIVE);

    // last 3 SpellOrActions are reactions
    SetActionBar(ACTION_BAR_INDEX_PET_SPELL_END + 0, REACT_HELPER, ACT_REACTION);
    SetActionBar(ACTION_BAR_INDEX_PET_SPELL_END + 1, REACT_DEFENSIVE, ACT_REACTION);
    SetActionBar(ACTION_BAR_INDEX_PET_SPELL_END + 2, REACT_PASSIVE, ACT_REACTION);

    /*for (uint32 i = 0; i < ACTION_BAR_INDEX_END - ACTION_BAR_INDEX_PET_SPELL_END; ++i)
    {
        if (i != 2)
            SetActionBar(ACTION_BAR_INDEX_PET_SPELL_END + i, COMMAND_ATTACK - i, ACT_REACTION);
        else
            SetActionBar(ACTION_BAR_INDEX_PET_SPELL_END + i, REACT_HELPER, ACT_REACTION);
    }*/
}

void CharmInfo::InitEmptyActionBar(bool withAttack)
{
    if (withAttack)
        SetActionBar(ACTION_BAR_INDEX_START, COMMAND_ATTACK, ACT_COMMAND);
    else
        SetActionBar(ACTION_BAR_INDEX_START, 0, ACT_PASSIVE);
    for (uint32 x = ACTION_BAR_INDEX_START+1; x < ACTION_BAR_INDEX_END; ++x)
        SetActionBar(x, 0, ACT_PASSIVE);
}

void CharmInfo::InitPossessCreateSpells()
{
    InitEmptyActionBar();
    if (m_unit->GetTypeId() == TYPEID_UNIT)
    {
        for (uint32 i = 0; i < CREATURE_MAX_SPELLS; ++i)
        {
            uint32 spellId = m_unit->ToCreature()->m_temlate_spells[i];
            SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId);
            if (spellInfo && !(spellInfo->Attributes & SPELL_ATTR0_CASTABLE_WHILE_DEAD))
            {
                if (spellInfo->IsPassive())
                    m_unit->CastSpell(m_unit, spellInfo, true);
                else
                    AddSpellToActionBar(spellInfo, ACT_PASSIVE);
            }
        }
    }
}

void CharmInfo::InitCharmCreateSpells()
{
    if (m_unit->GetTypeId() == TYPEID_PLAYER)                // charmed players don't have spells
    {
        InitEmptyActionBar();
        return;
    }

    InitPetActionBar();

    for (uint32 x = 0; x < MAX_SPELL_CHARM; ++x)
    {
        uint32 spellId = m_unit->ToCreature()->m_temlate_spells[x];
        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId);

        if (!spellInfo || spellInfo->Attributes & SPELL_ATTR0_CASTABLE_WHILE_DEAD)
        {
            m_charmspells[x].SetActionAndType(spellId, ACT_DISABLED);
            continue;
        }

        if (spellInfo->IsPassive())
        {
            m_unit->CastSpell(m_unit, spellInfo, true);
            m_charmspells[x].SetActionAndType(spellId, ACT_PASSIVE);
        }
        else
        {
            m_charmspells[x].SetActionAndType(spellId, ACT_DISABLED);

            ActiveStates newstate = ACT_PASSIVE;

            if (!spellInfo->IsAutocastable())
                newstate = ACT_PASSIVE;
            else
            {
                if (spellInfo->NeedsExplicitUnitTarget())
                {
                    newstate = ACT_ENABLED;
                    ToggleCreatureAutocast(spellInfo, true);
                }
                else
                    newstate = ACT_DISABLED;
            }

            AddSpellToActionBar(spellInfo, newstate);
        }
    }
}

bool CharmInfo::AddSpellToActionBar(SpellInfo const* spellInfo, ActiveStates newstate)
{
    // pet cannot summon mob
    if (spellInfo->Id != 49297)
    {
        for (int j = 0; j < MAX_SPELL_EFFECTS; ++j)
        {
            switch (spellInfo->Effects[j].Effect)
            {
                case SPELL_EFFECT_SUMMON:
                    return false;
            }
        }
    }

    uint32 spell_id = spellInfo->Id;
    uint32 first_id = spellInfo->GetFirstRankSpell()->Id;

    // new spell rank can be already listed
    for (uint8 i = 0; i < MAX_UNIT_ACTION_BAR_INDEX; ++i)
    {
        if (uint32 action = PetActionBar[i].GetAction())
        {
            if (PetActionBar[i].IsActionBarForSpell() && sSpellMgr->GetFirstSpellInChain(action) == first_id)
            {
                PetActionBar[i].SetAction(spell_id);
                return true;
            }
        }
    }

    // or use empty slot in other case
    for (uint8 i = 0; i < MAX_UNIT_ACTION_BAR_INDEX; ++i)
    {
        if (!PetActionBar[i].GetAction() && PetActionBar[i].IsActionBarForSpell())
        {
            SetActionBar(i, spell_id, newstate == ACT_DECIDE ? spellInfo->IsAutocastable() ? ACT_DISABLED : ACT_PASSIVE : newstate);
            return true;
        }
    }
    return false;
}

bool CharmInfo::RemoveSpellFromActionBar(uint32 spell_id)
{
    uint32 first_id = sSpellMgr->GetFirstSpellInChain(spell_id);

    for (uint8 i = 0; i < MAX_UNIT_ACTION_BAR_INDEX; ++i)
    {
        if (uint32 action = PetActionBar[i].GetAction())
        {
            if (PetActionBar[i].IsActionBarForSpell() && sSpellMgr->GetFirstSpellInChain(action) == first_id)
            {
                SetActionBar(i, 0, ACT_PASSIVE);
                return true;
            }
        }
    }

    return false;
}

void CharmInfo::ToggleCreatureAutocast(SpellInfo const* spellInfo, bool apply)
{
    if (spellInfo->IsPassive())
        return;

    for (uint32 x = 0; x < MAX_SPELL_CHARM; ++x)
        if (spellInfo->Id == m_charmspells[x].GetAction())
            m_charmspells[x].SetType(apply ? ACT_ENABLED : ACT_DISABLED);
}

void CharmInfo::SetPetNumber(uint32 petnumber, bool statwindow)
{
    m_petnumber = petnumber;
    if (statwindow)
        m_unit->SetUInt32Value(UNIT_FIELD_PETNUMBER, m_petnumber);
    else
        m_unit->SetUInt32Value(UNIT_FIELD_PETNUMBER, 0);
}

void CharmInfo::LoadPetActionBar(const std::string& data)
{
    InitPetActionBar();

    Tokenizer tokens(data, ' ');

    if (tokens.size() != (ACTION_BAR_INDEX_END-ACTION_BAR_INDEX_START) * 2)
        return;                                             // non critical, will reset to default

    uint8 index = ACTION_BAR_INDEX_START;
    Tokenizer::const_iterator iter = tokens.begin();
    for (; index < ACTION_BAR_INDEX_END; ++iter, ++index)
    {
        // use unsigned cast to avoid sign negative format use at long-> ActiveStates (int) conversion
        ActiveStates type  = ActiveStates(atol(*iter));
        ++iter;
        uint32 action = uint32(atol(*iter));

        PetActionBar[index].SetActionAndType(action, type);

        // check correctness
        if (PetActionBar[index].IsActionBarForSpell())
        {
            SpellInfo const* spelInfo = sSpellMgr->GetSpellInfo(PetActionBar[index].GetAction());
            if (!spelInfo)
                SetActionBar(index, 0, ACT_PASSIVE);
            else if (!spelInfo->IsAutocastable())
                SetActionBar(index, PetActionBar[index].GetAction(), ACT_PASSIVE);
        }
    }
}

void CharmInfo::BuildActionBar(WorldPacket* data)
{
    for (uint32 i = 0; i < MAX_UNIT_ACTION_BAR_INDEX; ++i)
        *data << uint32(PetActionBar[i].packedData);
}

void CharmInfo::SetSpellAutocast(SpellInfo const* spellInfo, bool state)
{
    for (uint8 i = 0; i < MAX_UNIT_ACTION_BAR_INDEX; ++i)
    {
        if (spellInfo->Id == PetActionBar[i].GetAction() && PetActionBar[i].IsActionBarForSpell())
        {
            PetActionBar[i].SetType(state ? ACT_ENABLED : ACT_DISABLED);
            break;
        }
    }
}

bool Unit::isFrozen() const
{
    return HasAuraState(AURA_STATE_FROZEN);
}

struct ProcTriggeredData
{
    ProcTriggeredData(Aura* _aura)
        : aura(_aura)
    {
        effMask = 0;
        spellProcEvent = NULL;
        isProcOneEff = false;
    }
    SpellProcEventEntry const* spellProcEvent;
    Aura* aura;
    uint32 effMask;
    bool isProcOneEff;
};

typedef std::list< ProcTriggeredData > ProcTriggeredList;

// List of auras that CAN be trigger but may not exist in spell_proc_event
// in most case need for drop charges
// in some types of aura need do additional check
// for example SPELL_AURA_MECHANIC_IMMUNITY - need check for mechanic
bool InitTriggerAuraData()
{
    for (uint16 i = 0; i < TOTAL_AURAS; ++i)
    {
        isTriggerAura[i] = false;
        isNonTriggerAura[i] = false;
        isAlwaysTriggeredAura[i] = false;
        isDamageCapAura[i] = false;
    }
    isTriggerAura[SPELL_AURA_PROC_ON_POWER_AMOUNT] = true;
    isTriggerAura[SPELL_AURA_DUMMY] = true;
    isTriggerAura[SPELL_AURA_PERIODIC_DUMMY] = true;
    isTriggerAura[SPELL_AURA_MOD_CONFUSE] = true;
    isTriggerAura[SPELL_AURA_MOD_THREAT] = true;
    isTriggerAura[SPELL_AURA_MOD_STUN] = true; // Aura does not have charges but needs to be removed on trigger
    isTriggerAura[SPELL_AURA_MOD_DAMAGE_DONE] = true;
    isTriggerAura[SPELL_AURA_MOD_DAMAGE_TAKEN] = true;
    isTriggerAura[SPELL_AURA_MOD_RESISTANCE] = true;
    isTriggerAura[SPELL_AURA_MOD_STEALTH] = true;
    isTriggerAura[SPELL_AURA_MOD_FEAR] = true; // Aura does not have charges but needs to be removed on trigger
    isTriggerAura[SPELL_AURA_MOD_FEAR_2] = true;
    isTriggerAura[SPELL_AURA_MOD_ROOT] = true;
    isTriggerAura[SPELL_AURA_TRANSFORM] = true;
    isTriggerAura[SPELL_AURA_REFLECT_SPELLS] = true;
    isTriggerAura[SPELL_AURA_DAMAGE_IMMUNITY] = true;
    isTriggerAura[SPELL_AURA_PROC_TRIGGER_SPELL] = true;
    isTriggerAura[SPELL_AURA_PROC_MELEE_TRIGGER_SPELL] = true;
    isTriggerAura[SPELL_AURA_PROC_TRIGGER_DAMAGE] = true;
    isTriggerAura[SPELL_AURA_MOD_CASTING_SPEED_NOT_STACK] = true;
    isTriggerAura[SPELL_AURA_MOD_CASTING_SPEED] = true;
    isTriggerAura[SPELL_AURA_SCHOOL_ABSORB] = true; // Savage Defense untested
    isTriggerAura[SPELL_AURA_MOD_POWER_COST_SCHOOL_PCT] = true;
    isTriggerAura[SPELL_AURA_MOD_POWER_COST_SCHOOL] = true;
    isTriggerAura[SPELL_AURA_REFLECT_SPELLS_SCHOOL] = true;
    isTriggerAura[SPELL_AURA_MECHANIC_IMMUNITY] = true;
    isTriggerAura[SPELL_AURA_MOD_DAMAGE_PERCENT_TAKEN] = true;
    isTriggerAura[SPELL_AURA_SPELL_MAGNET] = true;
    isTriggerAura[SPELL_AURA_MOD_ATTACK_POWER] = true;
    isTriggerAura[SPELL_AURA_ADD_CASTER_HIT_TRIGGER] = true;
    isTriggerAura[SPELL_AURA_OVERRIDE_CLASS_SCRIPTS] = true;
    isTriggerAura[SPELL_AURA_MOD_MECHANIC_RESISTANCE] = true;
    isTriggerAura[SPELL_AURA_RANGED_ATTACK_POWER_ATTACKER_BONUS] = true;
    isTriggerAura[SPELL_AURA_MOD_MELEE_HASTE] = true;
    isTriggerAura[SPELL_AURA_MOD_MELEE_HASTE_3] = true;
    isTriggerAura[SPELL_AURA_MOD_ATTACKER_MELEE_HIT_CHANCE] = true;
    isTriggerAura[SPELL_AURA_RAID_PROC_FROM_CHARGE] = true;
    isTriggerAura[SPELL_AURA_RAID_PROC_FROM_CHARGE_WITH_VALUE2] = true;
    isTriggerAura[SPELL_AURA_RAID_PROC_FROM_CHARGE_WITH_VALUE] = true;
    isTriggerAura[SPELL_AURA_PROC_TRIGGER_SPELL_WITH_VALUE] = true;
    isTriggerAura[SPELL_AURA_MOD_DAMAGE_FROM_CASTER] = true;
    isTriggerAura[SPELL_AURA_MOD_SPELL_CRIT_CHANCE] = true;
    isTriggerAura[SPELL_AURA_ABILITY_IGNORE_AURASTATE] = true;
    isTriggerAura[SPELL_AURA_CAST_WHILE_WALKING] = true;
    isTriggerAura[SPELL_AURA_MOD_CAST_TIME_WHILE_MOVING] = true;
    isTriggerAura[SPELL_AURA_ADD_PCT_MODIFIER] = true;
    isTriggerAura[SPELL_AURA_ADD_FLAT_MODIFIER] = true;
    isTriggerAura[SPELL_AURA_OVERRIDE_ACTIONBAR_SPELLS] = true;
    isTriggerAura[SPELL_AURA_OVERRIDE_ACTIONBAR_SPELLS_2] = true;
    isTriggerAura[SPELL_AURA_MOD_HEALING_RECEIVED] = true;
    isTriggerAura[SPELL_AURA_IGNORE_CD] = true;

    isNonTriggerAura[SPELL_AURA_MOD_POWER_REGEN] = true;
    isNonTriggerAura[SPELL_AURA_REDUCE_PUSHBACK] = true;

    isAlwaysTriggeredAura[SPELL_AURA_OVERRIDE_CLASS_SCRIPTS] = true;
    isAlwaysTriggeredAura[SPELL_AURA_MOD_FEAR] = true;
    isAlwaysTriggeredAura[SPELL_AURA_MOD_FEAR_2] = true;
    isAlwaysTriggeredAura[SPELL_AURA_MOD_ROOT] = true;
    isAlwaysTriggeredAura[SPELL_AURA_MOD_STUN] = true;
    isAlwaysTriggeredAura[SPELL_AURA_TRANSFORM] = true;
    isAlwaysTriggeredAura[SPELL_AURA_SPELL_MAGNET] = true;
    isAlwaysTriggeredAura[SPELL_AURA_SCHOOL_ABSORB] = true;
    isAlwaysTriggeredAura[SPELL_AURA_MOD_STEALTH] = true;
    isAlwaysTriggeredAura[SPELL_AURA_CAST_WHILE_WALKING] = true;
    isAlwaysTriggeredAura[SPELL_AURA_MOD_CAST_TIME_WHILE_MOVING] = true;
    isAlwaysTriggeredAura[SPELL_AURA_WATER_WALK] = true;

    isDamageCapAura[SPELL_AURA_MOD_CONFUSE] = true;
    isDamageCapAura[SPELL_AURA_MOD_FEAR] = true;
    isDamageCapAura[SPELL_AURA_MOD_FEAR_2] = true;
    isDamageCapAura[SPELL_AURA_MOD_ROOT] = true;
    isDamageCapAura[SPELL_AURA_MOD_STUN] = true;
    isDamageCapAura[SPELL_AURA_TRANSFORM] = true;

    return true;
}

uint32 createProcExtendMask(SpellNonMeleeDamage* damageInfo, SpellMissInfo missCondition)
{
    uint32 procEx = PROC_EX_NONE;
    // Check victim state
    if (missCondition != SPELL_MISS_NONE)
        switch (missCondition)
        {
            case SPELL_MISS_MISS:    procEx|=PROC_EX_MISS;   break;
            case SPELL_MISS_RESIST:  procEx|=PROC_EX_RESIST; break;
            case SPELL_MISS_DODGE:   procEx|=PROC_EX_DODGE;  break;
            case SPELL_MISS_PARRY:   procEx|=PROC_EX_PARRY;  break;
            case SPELL_MISS_BLOCK:   procEx|=PROC_EX_BLOCK;  break;
            case SPELL_MISS_EVADE:   procEx|=PROC_EX_EVADE;  break;
            case SPELL_MISS_IMMUNE:  procEx|=PROC_EX_IMMUNE; break;
            case SPELL_MISS_IMMUNE2: procEx|=PROC_EX_IMMUNE; break;
            case SPELL_MISS_DEFLECT: procEx|=PROC_EX_DEFLECT;break;
            case SPELL_MISS_ABSORB:  procEx|=PROC_EX_ABSORB; break;
            case SPELL_MISS_REFLECT: procEx|=PROC_EX_REFLECT;break;
            default:
                break;
        }
    else
    {
        // On block
        if (damageInfo->blocked)
            procEx|=PROC_EX_BLOCK;
        // On absorb
        if (damageInfo->absorb)
            procEx|=PROC_EX_ABSORB;
        // On crit
        if (damageInfo->HitInfo & SPELL_HIT_TYPE_CRIT)
            procEx|=PROC_EX_CRITICAL_HIT;
        else
            procEx|=PROC_EX_NORMAL_HIT;
    }
    return procEx;
}

void Unit::ProcDamageAndSpellFor(bool isVictim, Unit* target, uint32 procFlag, uint32 procExtra, WeaponAttackType attType, SpellInfo const* procSpell, DamageInfo* dmgInfoProc, SpellInfo const* procAura, std::list<uint32>* mSpellModsList)
{
    // Player is loaded now - do not allow passive spell casts to proc
    if (GetTypeId() == TYPEID_PLAYER && ToPlayer()->GetSession()->PlayerLoading())
        return;

    //Check for periodick non proc spell
    if ((procFlag & (PROC_FLAG_DONE_PERIODIC | PROC_FLAG_TAKEN_PERIODIC)) && procSpell && !procSpell->CanSpellProc(target))
        return;

    // For melee/ranged based attack need update skills and set some Aura states if victim present
    if (procFlag & MELEE_BASED_TRIGGER_MASK && target)
    {
        // If exist crit/parry/dodge/block need update aura state (for victim and attacker)
        if (procExtra & (PROC_EX_CRITICAL_HIT|PROC_EX_PARRY|PROC_EX_DODGE|PROC_EX_BLOCK))
        {
            // for victim
            if (isVictim)
            {
                // if victim and dodge attack
                if (procExtra & PROC_EX_DODGE)
                {
                    // Update AURA_STATE on dodge
                    if (getClass() != CLASS_ROGUE) // skip Rogue Riposte
                    {
                        ModifyAuraState(AURA_STATE_DEFENSE, true);
                        StartReactiveTimer(REACTIVE_DEFENSE);
                    }
                }
                // if victim and parry attack
                if (procExtra & PROC_EX_PARRY)
                {
                    // For Hunters only Counterattack (skip Mongoose bite)
                    if (getClass() == CLASS_HUNTER)
                    {
                        ModifyAuraState(AURA_STATE_HUNTER_PARRY, true);
                        StartReactiveTimer(REACTIVE_HUNTER_PARRY);
                    }
                    else
                    {
                        ModifyAuraState(AURA_STATE_DEFENSE, true);
                        StartReactiveTimer(REACTIVE_DEFENSE);
                    }
                }
                // if and victim block attack
                if (procExtra & PROC_EX_BLOCK)
                {
                    ModifyAuraState(AURA_STATE_DEFENSE, true);
                    StartReactiveTimer(REACTIVE_DEFENSE);
                }
            }
            else // For attacker
            {
                // Overpower on victim dodge
                if (procExtra & PROC_EX_DODGE && GetTypeId() == TYPEID_PLAYER && getClass() == CLASS_WARRIOR)
                {
                    ToPlayer()->AddComboPoints(target, 1);
                    StartReactiveTimer(REACTIVE_OVERPOWER);
                    CastSpell(this, 119962, true);
                }
            }
        }
    }

    // Hack Fix Cobra Strikes - Drop charge
    if (GetTypeId() == TYPEID_UNIT && HasAura(53257) && !procSpell)
        if (Aura* aura = GetAura(53257))
            aura->ModStackAmount(-1);

    Unit* actor = isVictim ? target : this;
    Unit* actionTarget = !isVictim ? target : this;

    HealInfo healInfo = HealInfo(actor, actionTarget, dmgInfoProc->GetDamage(), procSpell, procSpell ? SpellSchoolMask(procSpell->SchoolMask) : SPELL_SCHOOL_MASK_NORMAL);
    ProcEventInfo eventInfo = ProcEventInfo(actor, actionTarget, target, procFlag, 0, 0, procExtra, NULL, dmgInfoProc, &healInfo);

    ProcTriggeredList procTriggered;
    // Fill procTriggered list
    for (AuraApplicationMap::const_iterator itr = GetAppliedAuras().begin(); itr!= GetAppliedAuras().end(); ++itr)
    {
        // Do not allow auras to proc from effect triggered by itself
        if (procAura && procAura->Id == itr->first)
            continue;
        ProcTriggeredData triggerData(itr->second->GetBase());

        // Defensive procs are active on absorbs (so absorption effects are not a hindrance)
        bool active = dmgInfoProc->GetDamage() || dmgInfoProc->GetAddPower() || (procExtra & PROC_EX_ON_CAST) || (procExtra & PROC_EX_BLOCK && isVictim) || (procExtra & PROC_EX_ABSORB);

        if(triggerData.aura->IsUsingCharges())
        {
            if((procFlag & SPELL_PROC_FROM_CAST_MASK) && !(procExtra & PROC_EX_ON_CAST))
                continue;
        }
        else if ((procFlag & SPELL_PROC_FROM_CAST_MASK) && (procExtra & PROC_EX_ON_CAST) && !(triggerData.aura->GetSpellInfo()->AttributesCu & SPELL_ATTR0_CU_PROC_ONLY_ON_CAST))
            continue;
        else if ((procFlag & SPELL_PROC_FROM_CAST_MASK) && !(procExtra & PROC_EX_ON_CAST) && (triggerData.aura->GetSpellInfo()->AttributesCu & SPELL_ATTR0_CU_PROC_ONLY_ON_CAST))
            continue;

        // only auras that has triggered spell should proc from fully absorbed damage
        SpellInfo const* spellProto = itr->second->GetBase()->GetSpellInfo();
        if ((procExtra & PROC_EX_ABSORB && isVictim) || ((procFlag & PROC_FLAG_DONE_SPELL_MAGIC_DMG_CLASS_NEG) && spellProto->DmgClass == SPELL_DAMAGE_CLASS_MAGIC))
        {
            bool triggerSpell = false;
            for (int i = 0; i < MAX_SPELL_EFFECTS; ++i)
                if (spellProto->GetEffect(i, GetSpawnMode())->TriggerSpell)
                    triggerSpell = true;

            if (dmgInfoProc->GetDamage() || triggerSpell)
                active = true;
        }

        // Custom MoP Script
        // Breath of Fire DoT shoudn't remove Breath of Fire disorientation - Hack Fix
        if (procSpell && procSpell->Id == 123725 && itr->first == 123393)
            continue;

        if (procSpell && !(procSpell->AuraInterruptFlags & (AURA_INTERRUPT_FLAG_TAKE_DAMAGE)))
            // time for hardcode! Some spells can proc on absorb
            if (triggerData.aura && triggerData.aura->GetSpellInfo() && (triggerData.aura->GetSpellInfo()->Id == 33757 || triggerData.aura->GetSpellInfo()->GetSpellSpecific() == SPELL_SPECIFIC_SEAL ||
                triggerData.aura->GetSpellInfo()->HasAura(SPELL_AURA_MOD_STEALTH) || triggerData.aura->GetSpellInfo()->HasAura(SPELL_AURA_MOD_INVISIBILITY)))
                active = true;

        if (isVictim)
        {
            if (triggerData.aura->GetSpellInfo()->HasAura(SPELL_AURA_MOD_STEALTH))
            {
                if (procSpell && procSpell->IsPositive())
                    continue;

                if (!dmgInfoProc->GetDamage() && !(procExtra & PROC_EX_ABSORB) && procSpell && !procSpell->HasAura(SPELL_AURA_MOD_STUN) && !procSpell->HasAura(SPELL_AURA_MOD_CONFUSE) &&
                    !procSpell->HasAura(SPELL_AURA_MOD_FEAR) && !procSpell->HasAura(SPELL_AURA_MOD_FEAR_2))
                    continue;

                active = true;
            }
            else if ((procSpell && procSpell->Mechanic == MECHANIC_DISARM) || (spellProto->AttributesEx3 & SPELL_ATTR3_CAN_PROC_WITH_TRIGGERED))
                active = true;
        }

        // do checks using conditions table
        ConditionList conditions = sConditionMgr->GetConditionsForNotGroupedEntry(CONDITION_SOURCE_TYPE_SPELL_PROC, spellProto->Id);
        ConditionSourceInfo condInfo = ConditionSourceInfo(eventInfo.GetActor(), eventInfo.GetActionTarget());
        if (!sConditionMgr->IsObjectMeetToConditions(condInfo, conditions))
            continue;

        // Triggered spells not triggering additional spells
        bool triggered = !(spellProto->AttributesEx3 & SPELL_ATTR3_CAN_PROC_WITH_TRIGGERED) ?
            (procExtra & PROC_EX_INTERNAL_TRIGGERED && !(procFlag & PROC_FLAG_DONE_TRAP_ACTIVATION)) : false;

        for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
        {
            if (itr->second->HasEffect(i))
            {
                if (!IsTriggeredAtSpellProcEvent(target, spellProto, procSpell, procFlag, procExtra, attType, isVictim, active, triggerData.spellProcEvent, i))
                    continue;
                AuraEffect* aurEff = itr->second->GetBase()->GetEffect(i);
                // Skip this auras
                if (isNonTriggerAura[aurEff->GetAuraType()])
                    continue;
                if (isDamageCapAura[aurEff->GetAuraType()])
                    triggerData.isProcOneEff = true;
                // If not trigger by default and spellProcEvent == NULL - skip
                if (!isTriggerAura[aurEff->GetAuraType()] && (triggerData.spellProcEvent == NULL || !(triggerData.spellProcEvent->effectMask & (1<<i))))
                    continue;
                if (!SpellProcCheck(target, spellProto, procSpell, i))
                    continue;
                // Some spells must always trigger
                if (!triggered || isAlwaysTriggeredAura[aurEff->GetAuraType()])
                    triggerData.effMask |= 1<<i;
            }
        }
        if (triggerData.effMask)
        {
            // If set trigger always but only one time
            if(triggerData.spellProcEvent && (triggerData.spellProcEvent->procEx & PROC_EX_EX_ONE_TIME_TRIGGER))
            {
                bool foundProc = false;
                for (ProcTriggeredList::const_iterator i = procTriggered.begin(); i != procTriggered.end(); ++i)
                    if(i->aura->GetId() == triggerData.aura->GetId())
                    {
                        foundProc = true;
                        break;
                    }
                if(!foundProc)
                    procTriggered.push_front(triggerData);
            }
            else
                procTriggered.push_front(triggerData);
        }
    }

    //sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "ProcDamageAndSpell: procSpell %u procTriggered %u procFlag %u procExtra %u isVictim %u", procSpell ? procSpell->Id : 0, procTriggered.size(), procFlag, procExtra, isVictim);

    // Nothing found
    if (procTriggered.empty())
        return;

    // Note: must SetCantProc(false) before return
    if (procExtra & (PROC_EX_INTERNAL_TRIGGERED | PROC_EX_INTERNAL_CANT_PROC))
        SetCantProc(true);

    // Handle effects proceed this time
    for (ProcTriggeredList::const_iterator i = procTriggered.begin(); i != procTriggered.end(); ++i)
    {
        // look for aura in auras list, it may be removed while proc event processing
        if (i->aura->IsRemoved())
            continue;

        bool isModifier = false;
        bool isReflect = false;

        bool useCharges  = i->aura->IsUsingCharges();
        // no more charges to use, prevent proc
        if (useCharges && !i->aura->GetCharges())
            continue;

        bool takeCharges = false;
        SpellInfo const* spellInfo = i->aura->GetSpellInfo();
        uint32 Id = i->aura->GetId();

        if(spellInfo->ProcFlags & (PROC_FLAG_DONE_SPELL_MAGIC_DMG_POS_NEG))
            useCharges = true;

        AuraApplication const* aurApp = i->aura->GetApplicationOfTarget(GetGUID());

        // For players set spell cooldown if need
        double cooldown = spellInfo->procTimeRec / 1000.0;
        if (GetTypeId() == TYPEID_PLAYER && i->spellProcEvent && G3D::fuzzyGt(i->spellProcEvent->cooldown, 0.0))
            cooldown = i->spellProcEvent->cooldown;

        // Note: must SetCantProc(false) before return
        //if (spellInfo->AttributesEx3 & SPELL_ATTR3_DISABLE_PROC)
            //SetCantProc(true);

        // This bool is needed till separate aura effect procs are still here
        bool handled = false;
        if (HandleAuraProc(target, dmgInfoProc, i->aura, procSpell, procFlag, procExtra, cooldown, &handled))
        {
            sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "ProcDamageAndSpell: casting spell %u (triggered with value by %s aura of spell %u)", spellInfo->Id, (isVictim?"a victim's":"an attacker's"), Id);
            takeCharges = true;
        }

        if (!handled)
        {
            for (uint8 effIndex = 0; effIndex < MAX_SPELL_EFFECTS; ++effIndex)
            {
                if (!(i->effMask & (1<<effIndex)))
                    continue;

                AuraEffect* triggeredByAura = i->aura->GetEffect(effIndex);
                ASSERT(triggeredByAura);

                //if aura has cap to damage, do not proc other auras
                if(i->isProcOneEff && !isDamageCapAura[triggeredByAura->GetAuraType()])
                    continue;

                bool prevented = i->aura->CallScriptEffectProcHandlers(triggeredByAura, aurApp, eventInfo);
                if (prevented)
                {
                    takeCharges = true;
                    continue;
                }

                // Proc chain chack. Not handle proc from curent effect in future prock from it.
                triggeredEffectList::iterator itr = m_triggeredEffect.find(triggeredByAura);
                if (itr != m_triggeredEffect.end())
                    continue;

                m_triggeredEffect.insert(triggeredByAura);

                switch (triggeredByAura->GetAuraType())
                {
                    case SPELL_AURA_PROC_MELEE_TRIGGER_SPELL:
                    {
                        sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "SPELL_AURA_PROC_MELEE_TRIGGER_SPELL: casting spell %u (triggered by %s aura of spell %u)", spellInfo->Id, (isVictim?"a victim's":"an attacker's"), triggeredByAura->GetId());
                        // Don`t drop charge or add cooldown for not started trigger
                        if (HandleProcMelleTriggerSpell(target, dmgInfoProc, triggeredByAura, procSpell, procFlag, procExtra, cooldown))
                            takeCharges = true;
                        break;
                    }
                    case SPELL_AURA_PROC_TRIGGER_SPELL:
                    case SPELL_AURA_PERIODIC_TRIGGER_SPELL:
                    {
                        sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "SPELL_AURA_PROC_TRIGGER_SPELL: casting spell %u (triggered by %s aura of spell %u)", spellInfo->Id, (isVictim?"a victim's":"an attacker's"), triggeredByAura->GetId());
                        // Don`t drop charge or add cooldown for not started trigger
                        if (HandleProcTriggerSpell(target, dmgInfoProc, triggeredByAura, procSpell, procFlag, procExtra, cooldown))
                            takeCharges = true;
                        break;
                    }
                    case SPELL_AURA_PROC_TRIGGER_DAMAGE:
                    {
                        // target has to be valid
                        if (!target)
                            break;

                        sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "SPELL_AURA_PROC_TRIGGER_DAMAGE: doing %u damage from spell id %u (triggered by %s aura of spell %u)", triggeredByAura->GetAmount(), spellInfo->Id, (isVictim?"a victim's":"an attacker's"), triggeredByAura->GetId());
                        SpellNonMeleeDamage damageInfo(this, target, spellInfo->Id, spellInfo->SchoolMask);
                        uint32 newDamage = SpellDamageBonusDone(target, spellInfo, triggeredByAura->GetAmount(), SPELL_DIRECT_DAMAGE, (SpellEffIndex) effIndex);
                        newDamage = target->SpellDamageBonusTaken(this, spellInfo, newDamage);
                        CalculateSpellDamageTaken(&damageInfo, newDamage, spellInfo, (1 << effIndex));
                        DealDamageMods(damageInfo.target, damageInfo.damage, &damageInfo.absorb);
                        SendSpellNonMeleeDamageLog(&damageInfo);
                        DealSpellDamage(&damageInfo, true);
                        takeCharges = true;
                        break;
                    }
                    case SPELL_AURA_MANA_SHIELD:
                    case SPELL_AURA_DUMMY:
                    case SPELL_AURA_PERIODIC_DUMMY:
                    {
                        sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "SPELL_AURA_DUMMY: casting spell id %u (triggered by %s dummy aura of spell %u), procSpell %u", spellInfo->Id, (isVictim?"a victim's":"an attacker's"), triggeredByAura->GetId(), (procSpell ? procSpell->Id : 0));
                        if(SpellProcTriggered(target, dmgInfoProc, triggeredByAura, procSpell, procFlag, procExtra, cooldown))
                            takeCharges = true;
                        else if (HandleDummyAuraProc(target, dmgInfoProc, triggeredByAura, procSpell, procFlag, procExtra, cooldown))
                            takeCharges = true;
                        break;
                    }
                    case SPELL_AURA_OVERRIDE_ACTIONBAR_SPELLS:
                    case SPELL_AURA_IGNORE_CD:
                    case SPELL_AURA_MOD_HEALING_RECEIVED:
                    {
                        if (!triggeredByAura->IsAffectingSpell(procSpell) && !triggeredByAura->IsAffectingSpell(procAura))
                            break;

                        sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "SPELL_AURA_OVERRIDE_ACTIONBAR_SPELLS: casting spell id %u (triggered by %s dummy aura of spell %u), procSpell %u", spellInfo->Id, (isVictim?"a victim's":"an attacker's"), triggeredByAura->GetId(), (procSpell ? procSpell->Id : 0));

                        SpellProcTriggered(target, dmgInfoProc, triggeredByAura, procSpell, procFlag, procExtra, cooldown);
                        takeCharges = true;
                        break;
                    }
                    case SPELL_AURA_PERIODIC_HEAL:
                    {
                        sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "SPELL_AURA_PERIODIC_HEAL: casting spell id %u (triggered by %s dummy aura of spell %u), procSpell %u", spellInfo->Id, (isVictim?"a victim's":"an attacker's"), triggeredByAura->GetId(), (procSpell ? procSpell->Id : 0));
                        if(SpellProcTriggered(target, dmgInfoProc, triggeredByAura, procSpell, procFlag, procExtra, cooldown))
                            takeCharges = true;
                        break;
                    }
                    case SPELL_AURA_OVERRIDE_ACTIONBAR_SPELLS_2:
                    {
                        sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "SPELL_AURA_OVERRIDE_ACTIONBAR_SPELLS_2: casting spell id %u (triggered by %s dummy aura of spell %u), procSpell %u", spellInfo->Id, (isVictim?"a victim's":"an attacker's"), triggeredByAura->GetId(), (procSpell ? procSpell->Id : 0));
                        if (procSpell && (procSpell->Id == triggeredByAura->GetAmount()))
                            takeCharges = true;
                        break;
                    }
                    case SPELL_AURA_PROC_ON_POWER_AMOUNT:
                    {
                        sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "SPELL_AURA_PROC_ON_POWER_AMOUNT: casting spell id %u (triggered by %s aura of spell %u), procSpell %u", spellInfo->Id, (isVictim?"a victim's":"an attacker's"), triggeredByAura->GetId(), (procSpell ? procSpell->Id : 0));
                        if (HandleAuraProcOnPowerAmount(target, dmgInfoProc, triggeredByAura, procSpell, procFlag, procExtra, cooldown))
                            takeCharges = true;
                        break;
                    }
                    case SPELL_AURA_OBS_MOD_POWER:
                        sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "SPELL_AURA_OBS_MOD_POWER: casting spell id %u (triggered by %s aura of spell %u), procSpell %u", spellInfo->Id, (isVictim?"a victim's":"an attacker's"), triggeredByAura->GetId(), (procSpell ? procSpell->Id : 0));
                        if (HandleObsModEnergyAuraProc(target, dmgInfoProc, triggeredByAura, procSpell, procFlag, procExtra, cooldown))
                            takeCharges = true;
                        break;
                    case SPELL_AURA_MOD_DAMAGE_PERCENT_TAKEN:
                        sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "SPELL_AURA_MOD_DAMAGE_PERCENT_TAKEN: casting spell id %u (triggered by %s aura of spell %u), procSpell %u", spellInfo->Id, (isVictim?"a victim's":"an attacker's"), triggeredByAura->GetId(), (procSpell ? procSpell->Id : 0));
                        if (HandleModDamagePctTakenAuraProc(target, dmgInfoProc, triggeredByAura, procSpell, procFlag, procExtra, cooldown))
                            takeCharges = true;
                        break;
                    case SPELL_AURA_MOD_MELEE_HASTE:
                    case SPELL_AURA_MOD_MELEE_HASTE_3:
                    {
                        sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "SPELL_AURA_MOD_MELEE_HASTE: casting spell id %u (triggered by %s haste aura of spell %u), procSpell %u", spellInfo->Id, (isVictim?"a victim's":"an attacker's"), triggeredByAura->GetId(), (procSpell ? procSpell->Id : 0));
                        if (HandleHasteAuraProc(target, dmgInfoProc, triggeredByAura, procSpell, procFlag, procExtra, cooldown))
                            takeCharges = true;
                        break;
                    }
                    case SPELL_AURA_PROC_TRIGGER_SPELL_COPY:
                    {
                        sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "ProcDamageAndSpell: casting copy spell %u (triggered by %s aura of spell %u)", spellInfo->Id, (isVictim?"a victim's":"an attacker's"), triggeredByAura->GetId());
                        if (HandleProcTriggerSpellCopy(target, dmgInfoProc, triggeredByAura, procSpell, cooldown))
                            takeCharges = true;
                        break;
                    }
                    case SPELL_AURA_OVERRIDE_CLASS_SCRIPTS:
                    {
                        sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "SPELL_AURA_OVERRIDE_CLASS_SCRIPTS: casting spell id %u (triggered by %s aura of spell %u), procSpell %u", spellInfo->Id, (isVictim?"a victim's":"an attacker's"), triggeredByAura->GetId(), (procSpell ? procSpell->Id : 0));
                        if (HandleOverrideClassScriptAuraProc(target, dmgInfoProc, triggeredByAura, procSpell, cooldown))
                            takeCharges = true;
                        break;
                    }
                    case SPELL_AURA_RAID_PROC_FROM_CHARGE_WITH_VALUE:
                    case SPELL_AURA_RAID_PROC_FROM_CHARGE_WITH_VALUE2:
                    {
                        sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "SPELL_AURA_RAID_PROC_FROM_CHARGE_WITH_VALUE: casting mending (triggered by %s dummy aura of spell %u), procSpell %u",
                            (isVictim?"a victim's":"an attacker's"), triggeredByAura->GetId(), (procSpell ? procSpell->Id : 0));

                        HandleAuraRaidProcFromChargeWithValue(triggeredByAura);
                        takeCharges = true;
                        break;
                    }
                    case SPELL_AURA_RAID_PROC_FROM_CHARGE:
                    {
                        sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "ProcDamageAndSpell: casting mending (triggered by %s dummy aura of spell %u)",
                            (isVictim?"a victim's":"an attacker's"), triggeredByAura->GetId());

                        HandleAuraRaidProcFromCharge(triggeredByAura);
                        takeCharges = true;
                        break;
                    }
                    case SPELL_AURA_PROC_TRIGGER_SPELL_WITH_VALUE:
                    {
                        sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "ProcDamageAndSpell: casting spell %u (triggered with value by %s aura of spell %u)", spellInfo->Id, (isVictim?"a victim's":"an attacker's"), triggeredByAura->GetId());

                        if (HandleProcTriggerSpell(target, dmgInfoProc, triggeredByAura, procSpell, procFlag, procExtra, cooldown))
                            takeCharges = true;
                        break;
                    }
                    case SPELL_AURA_MOD_CASTING_SPEED_NOT_STACK:
                    case SPELL_AURA_MOD_CASTING_SPEED:
                        // Skip melee hits or instant cast spells
                        if (procSpell && (procSpell->CalcCastTime() != 0 || procSpell->IsChanneled()))
                            takeCharges = true;
                        break;
                    case SPELL_AURA_REFLECT_SPELLS:
                        isReflect = true;
                        takeCharges = true;
                        break;
                    case SPELL_AURA_REFLECT_SPELLS_SCHOOL:
                        // Skip Melee hits and spells ws wrong school
                        if (procSpell && (triggeredByAura->GetMiscValue() & procSpell->SchoolMask))         // School check
                            takeCharges = true;
                        break;
                    case SPELL_AURA_SPELL_MAGNET:
                        // Skip Melee hits and targets with magnet aura
                        if (procSpell && (triggeredByAura->GetBase()->GetUnitOwner()->ToUnit() == ToUnit()))         // Magnet
                            takeCharges = true;
                        break;
                    case SPELL_AURA_MOD_POWER_COST_SCHOOL_PCT:
                    case SPELL_AURA_MOD_POWER_COST_SCHOOL:
                        // Skip melee hits and spells ws wrong school or zero cost
                        if (procSpell &&
                            (procSpell->PowerCost != 0 || procSpell->PowerCostPercentage != 0) && // Cost check
                            (triggeredByAura->GetMiscValue() & procSpell->SchoolMask))          // School check
                            takeCharges = true;
                        break;
                    case SPELL_AURA_MECHANIC_IMMUNITY:
                        // Compare mechanic
                        if (procSpell && (procSpell->GetAllEffectsMechanicMask() & uint32(1<<(triggeredByAura->GetMiscValue()))))
                            takeCharges = true;
                        break;
                    case SPELL_AURA_MOD_MECHANIC_RESISTANCE:
                        // Compare mechanic
                        if (procSpell && (procSpell->GetAllEffectsMechanicMask() & uint32(1<<(triggeredByAura->GetMiscValue()))))
                            takeCharges = true;
                        break;
                    case SPELL_AURA_MOD_DAMAGE_FROM_CASTER:
                        // Compare casters
                        if (triggeredByAura->GetCasterGUID() == target->GetGUID())
                            takeCharges = true;
                        break;
                    case SPELL_AURA_MOD_SPELL_CRIT_CHANCE:
                        sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "ProcDamageAndSpell: casting spell id %u (triggered by %s spell crit chance aura of spell %u)", spellInfo->Id, (isVictim?"a victim's":"an attacker's"), triggeredByAura->GetId());
                        if (procSpell && HandleSpellCritChanceAuraProc(target, dmgInfoProc, triggeredByAura, procSpell, procFlag, procExtra, cooldown))
                            takeCharges = true;
                        break;
                    // CC Auras which use their amount amount to drop
                    // Are there any more auras which need this?
                    case SPELL_AURA_MOD_CONFUSE:
                    case SPELL_AURA_MOD_FEAR:
                    case SPELL_AURA_MOD_FEAR_2:
                    case SPELL_AURA_MOD_STUN:
                    case SPELL_AURA_MOD_ROOT:
                    case SPELL_AURA_TRANSFORM:
                    {
                        if (procExtra & PROC_EX_INTERNAL_HOT)
                            break;

                        // chargeable mods are breaking on hit
                        if (useCharges && int32(dmgInfoProc->GetDamage() + dmgInfoProc->GetAbsorb()))
                            takeCharges = true;
                        else
                        {
                            // Spell own direct damage at apply wont break the CC
                            if (procSpell && (procSpell->Id == triggeredByAura->GetId()))
                            {
                                Aura* aura = triggeredByAura->GetBase();
                                // called from spellcast, should not have ticked yet
                                if (aura->GetDuration() == aura->GetMaxDuration())
                                    break;
                            }
                            int32 damageLeft = triggeredByAura->GetAmount();
                            // No damage left
                            if (!(procFlag & PROC_FLAG_TAKEN_SPELL_MAGIC_DMG_CLASS_POS))
                            {
                                if (damageLeft < int32(dmgInfoProc->GetDamage() + dmgInfoProc->GetAbsorb()))
                                {
                                    std::list<uint32> auras;
                                    auras.push_back(i->aura->GetId());
                                    SendDispelLog(target ? target->GetGUID() : 0, procSpell ? procSpell->Id : 0, auras, true, false);

                                    i->aura->Remove();
                                }
                                else
                                    triggeredByAura->SetAmount(damageLeft - dmgInfoProc->GetDamage() - dmgInfoProc->GetAbsorb());
                            }
                        }

                        switch (triggeredByAura->GetId())
                        {
                            case 1776: // Gouge
                            case 2094: // Blind
                            {
                                if (procExtra & PROC_EX_INTERNAL_DOT)
                                    if (procSpell->SpellFamilyName == SPELLFAMILY_ROGUE)
                                        if (Unit* rogue = dmgInfoProc->GetAttacker())
                                            if (rogue->HasAura(108216)) // Dirty Tricks
                                                if (triggeredByAura->GetCasterGUID() == rogue->GetGUID())
                                                    takeCharges = false;
                                break;
                            }
                            default:
                                break;
                        }
                        break;
                    }
                    case SPELL_AURA_MOD_STEALTH:
                    {
                        sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "ProcDamageAndSpell: SPELL_AURA_MOD_STEALTH casting spell id %u (triggered by %s spell spell %u), procSpell %u", spellInfo->Id, (isVictim?"a victim's":"an attacker's"), triggeredByAura->GetId(), (procSpell ? procSpell->Id : 0));
                        // chargeable mods are breaking on hit
                        if (spellInfo->Id == 115191)
                        {
                            if (procSpell)
                            {
                                if (!procSpell->IsBreakingStealth() && !isVictim || isVictim && procSpell->Id == 6770)
                                {
                                    takeCharges = isVictim ? true: false;
                                    break;
                                }
                            }

                            if (!HasAura(115192))
                                CastSpell(this, 115192, true);
                            takeCharges = false;
                            break;
                        }
                        takeCharges = true;
                        break;
                    }
                    case SPELL_AURA_CAST_WHILE_WALKING:
                    case SPELL_AURA_MOD_CAST_TIME_WHILE_MOVING:
                    {
                        if (!triggeredByAura->IsAffectingSpell(procSpell) && !triggeredByAura->IsAffectingSpell(procAura))
                            break;
                        if (HandleCastWhileWalkingAuraProc(target, dmgInfoProc, triggeredByAura, procSpell, procFlag, procExtra, cooldown))
                        {
                            takeCharges = true;
                            isModifier = true;
                        }
                        break;
                    }
                    case SPELL_AURA_ADD_FLAT_MODIFIER:
                    case SPELL_AURA_ADD_PCT_MODIFIER:
                    {
                        if (!triggeredByAura->IsAffectingSpell(procSpell) && !triggeredByAura->IsAffectingSpell(procAura))
                            break;

                        //take charges only if spell moded by this spell
                        if(procExtra & PROC_EX_ON_CAST)
                        {
                            uint8 modOp = triggeredByAura->GetMiscValue();

                            if (Player* plr = ToPlayer())
                                if (Spell* _spell = FindCurrentSpellBySpellId(procSpell->Id))
                                    _spell->ApplySpellMod(plr->TryFindMod(SpellModType(triggeredByAura->GetAuraType()), SpellModOp(modOp), triggeredByAura->GetId(), triggeredByAura->GetAmount()));

                            bool moded = false;
                            for (std::list<uint32>::iterator imods = mSpellModsList->begin(); imods != mSpellModsList->end(); ++imods)
                                if((*imods) == triggeredByAura->GetId())
                                    moded = true;

                            if (!moded && ((1 << modOp) & 0x00204420)) // SPELLMOD_COST SPELLMOD_GLOBAL_COOLDOWN SPELLMOD_RANGE SPELLMOD_CASTING_TIME 
                                break;

                            if(procSpell && procSpell->Id == 116858 && triggeredByAura->GetId() == 117828 && i->aura->GetCharges() > 2) // Chaos Bolt
                                i->aura->ModCharges(-2);
                        }

                        if (HandleSpellModAuraProc(target, dmgInfoProc, triggeredByAura, procSpell, procFlag, procExtra, cooldown))
                        {
                            takeCharges = true;
                        }
                        break;
                    }
                    case SPELL_AURA_ABILITY_IGNORE_AURASTATE:
                        if (HandleIgnoreAurastateAuraProc(target, dmgInfoProc, triggeredByAura, procSpell, procFlag, procExtra, cooldown))
                            takeCharges = true;
                        break;
                    default:
                        // nothing do, just charges counter
                        // Don't drop charge for Earth Shield because of second effect
                        if (triggeredByAura->GetId() == 974)
                            break;

                        takeCharges = true;
                        break;
                } // switch (triggeredByAura->GetAuraType())
                m_triggeredEffect.erase(triggeredByAura);
            } // for (uint8 effIndex = 0; effIndex < MAX_SPELL_EFFECTS; ++effIndex)
        } // if (!handled)

        // Remove charge (aura can be removed by triggers)
        if (useCharges && takeCharges && i->aura->GetId() != 324 // Custom MoP Script - Hack Fix for Lightning Shield and Hack Fix for Arcane Charges
           )
        {
            // Hack Fix for Tiger Strikes
            if (i->aura->GetId() == 120273)
            {
                if (target)
                {
                    if (attType == BASE_ATTACK)
                        CastSpell(target, 120274, true); // extra attack for MainHand
                    else if (attType == OFF_ATTACK)
                        CastSpell(target, 120278, true); // extra attack for OffHand
                }
            }

            i->aura->DropCharge();
            if(isReflect) // reflect take only one aura
                return;
            if(isModifier && (procExtra & PROC_EX_ON_CAST)) // same proc use charge on cast can take only one aura
                return;
        }

        //if (spellInfo->AttributesEx3 & SPELL_ATTR3_DISABLE_PROC)
            //SetCantProc(false);
    }

    // Cleanup proc requirements
    if (procExtra & (PROC_EX_INTERNAL_TRIGGERED | PROC_EX_INTERNAL_CANT_PROC))
        SetCantProc(false);
}

void Unit::GetProcAurasTriggeredOnEvent(std::list<AuraApplication*>& aurasTriggeringProc, std::list<AuraApplication*>* procAuras, ProcEventInfo eventInfo)
{
    // use provided list of auras which can proc
    if (procAuras)
    {
        for (std::list<AuraApplication*>::iterator itr = procAuras->begin(); itr!= procAuras->end(); ++itr)
        {
            ASSERT((*itr)->GetTarget() == this);
            if (!(*itr)->GetRemoveMode())
                if ((*itr)->GetBase()->IsProcTriggeredOnEvent(*itr, eventInfo))
                {
                    (*itr)->GetBase()->PrepareProcToTrigger(*itr, eventInfo);
                    aurasTriggeringProc.push_back(*itr);
                }
        }
    }
    // or generate one on our own
    else
    {
        for (AuraApplicationMap::iterator itr = GetAppliedAuras().begin(); itr!= GetAppliedAuras().end(); ++itr)
        {
            if (itr->second->GetBase()->IsProcTriggeredOnEvent(itr->second, eventInfo))
            {
                itr->second->GetBase()->PrepareProcToTrigger(itr->second, eventInfo);
                aurasTriggeringProc.push_back(itr->second);
            }
        }
    }
}

void Unit::TriggerAurasProcOnEvent(CalcDamageInfo& damageInfo)
{
    DamageInfo dmgInfo = DamageInfo(damageInfo);
    TriggerAurasProcOnEvent(NULL, NULL, damageInfo.target, damageInfo.procAttacker, damageInfo.procVictim, 0, 0, damageInfo.procEx, NULL, &dmgInfo, NULL);
}

void Unit::TriggerAurasProcOnEvent(std::list<AuraApplication*>* myProcAuras, std::list<AuraApplication*>* targetProcAuras, Unit* actionTarget, uint32 typeMaskActor, uint32 typeMaskActionTarget, uint32 spellTypeMask, uint32 spellPhaseMask, uint32 hitMask, Spell* spell, DamageInfo* damageInfo, HealInfo* healInfo)
{
    // prepare data for self trigger
    ProcEventInfo myProcEventInfo = ProcEventInfo(this, actionTarget, actionTarget, typeMaskActor, spellTypeMask, spellPhaseMask, hitMask, spell, damageInfo, healInfo);
    std::list<AuraApplication*> myAurasTriggeringProc;
    GetProcAurasTriggeredOnEvent(myAurasTriggeringProc, myProcAuras, myProcEventInfo);

    // prepare data for target trigger
    ProcEventInfo targetProcEventInfo = ProcEventInfo(this, actionTarget, this, typeMaskActionTarget, spellTypeMask, spellPhaseMask, hitMask, spell, damageInfo, healInfo);
    std::list<AuraApplication*> targetAurasTriggeringProc;
    if (typeMaskActionTarget)
        GetProcAurasTriggeredOnEvent(targetAurasTriggeringProc, targetProcAuras, targetProcEventInfo);

    TriggerAurasProcOnEvent(myProcEventInfo, myAurasTriggeringProc);

    if (typeMaskActionTarget)
        TriggerAurasProcOnEvent(targetProcEventInfo, targetAurasTriggeringProc);
}

void Unit::TriggerAurasProcOnEvent(ProcEventInfo& eventInfo, std::list<AuraApplication*>& aurasTriggeringProc)
{
    for (std::list<AuraApplication*>::iterator itr = aurasTriggeringProc.begin(); itr != aurasTriggeringProc.end(); ++itr)
    {
        if (!(*itr)->GetRemoveMode())
            (*itr)->GetBase()->TriggerProcOnEvent(*itr, eventInfo);
    }
}

SpellSchoolMask Unit::GetMeleeDamageSchoolMask() const
{
    return SPELL_SCHOOL_MASK_NORMAL;
}

Player* Unit::GetSpellModOwner() const
{
    if (GetTypeId() == TYPEID_PLAYER)
        return (Player*)this;
    if (ToCreature()->isPet() || ToCreature()->isTotem())
    {
        Unit* owner = GetOwner();
        if (owner && owner->GetTypeId() == TYPEID_PLAYER)
            return (Player*)owner;
    }
    else if (ToCreature()->isSummon())
    {
        if(isSummon())
            if(Unit* owner = ToTempSummon()->GetSummoner())
                if (owner && owner->GetTypeId() == TYPEID_PLAYER)
                    return (Player*)owner;
    }
    return NULL;
}

///----------Pet responses methods-----------------
void Unit::SendPetCastFail(uint32 spellid, SpellCastResult result)
{
    if (result == SPELL_CAST_OK)
        return;

    Unit* owner = GetCharmerOrOwner();
    if (!owner || owner->GetTypeId() != TYPEID_PLAYER)
        return;

    //sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "SendPetCastFail  Spell: %u result %u.", spellid, result);

    //! 5.4.1
    WorldPacket data(SMSG_PET_CAST_FAILED, 1 + 4 + 1);
    data << uint32(spellid);
    data << uint32(result);
    data << uint8(0);                                       // cast count
    data << uint8(0xC0);                                    // unk 2 bits
    //data.WriteBit(1);
    //data.WriteBit(1);
    owner->ToPlayer()->GetSession()->SendPacket(&data);
}

void Unit::SendPetActionFeedback(uint8 msg)
{
    Unit* owner = GetOwner();
    if (!owner || owner->GetTypeId() != TYPEID_PLAYER)
        return;

    bool send_spell = false;

    //! 5.4.1
    WorldPacket data(SMSG_PET_ACTION_FEEDBACK, 2);
    data << uint8(msg);
    data.WriteBit(!send_spell);
    data.FlushBits();
    if (send_spell)
        data << uint32(0);
    owner->ToPlayer()->GetSession()->SendPacket(&data);
}

void Unit::SendPetTalk(uint32 pettalk)
{
    Unit* owner = GetOwner();
    if (!owner || owner->GetTypeId() != TYPEID_PLAYER)
        return;

    ObjectGuid Guid = GetGUID();

    //! 5.4.1
    WorldPacket data(SMSG_PET_ACTION_SOUND, 8 + 4);
    data << uint32(pettalk);
    data.WriteGuidMask<2, 3, 1, 4, 5, 6, 0, 7>(Guid);
    data.WriteGuidBytes<6, 2, 7, 1, 3, 4, 5, 0>(Guid);
    owner->ToPlayer()->GetSession()->SendPacket(&data);
}

void Unit::SendPetAIReaction(uint64 guid)
{
    Unit* owner = GetOwner();
    if (!owner || owner->GetTypeId() != TYPEID_PLAYER)
        return;

    ObjectGuid guidd = guid;

    //! 5.4.1
    WorldPacket data(SMSG_AI_REACTION, 8 + 4);
    data.WriteGuidMask<1, 5, 4, 3, 7, 6, 0, 2>(guidd);
    data.WriteGuidBytes<0, 5, 6, 2, 7, 3, 1, 4>(guidd);
    data << uint32(AI_REACTION_HOSTILE);
    owner->ToPlayer()->GetSession()->SendPacket(&data);
}

///----------End of Pet responses methods----------

void Unit::StopMoving()
{
    ClearUnitState(UNIT_STATE_MOVING);

    // not need send any packets if not in world or not moving
    if (!IsInWorld() || movespline->Finalized())
        return;

    // Update position using old spline
    UpdateSplinePosition(true);
    Movement::MoveSplineInit(*this).Stop();
}

void Unit::SendMovementFlagUpdate(bool self /* = false */)
{
    if (GetTypeId() == TYPEID_PLAYER)
    {
        WorldPacket data(SMSG_PLAYER_MOVE);
        WriteMovementUpdate(data);
        SendMessageToSet(&data, self);
        return;
    }
    //creature will give update at next movement.
    //if creature not moving we can send update movement by stop.
    if (!isMoving())
        Movement::MoveSplineInit(*this).Stop();
}

bool Unit::IsSitState() const
{
    uint8 s = getStandState();
    return
        s == UNIT_STAND_STATE_SIT_CHAIR        || s == UNIT_STAND_STATE_SIT_LOW_CHAIR  ||
        s == UNIT_STAND_STATE_SIT_MEDIUM_CHAIR || s == UNIT_STAND_STATE_SIT_HIGH_CHAIR ||
        s == UNIT_STAND_STATE_SIT;
}

bool Unit::IsStandState() const
{
    uint8 s = getStandState();
    return !IsSitState() && s != UNIT_STAND_STATE_SLEEP && s != UNIT_STAND_STATE_KNEEL;
}

void Unit::SetStandState(uint8 state)
{
    SetByteValue(UNIT_FIELD_BYTES_1, 0, state);

    if (IsStandState())
       RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_NOT_SEATED);

    if (GetTypeId() == TYPEID_PLAYER)
    {
        //! 5.4.1
        WorldPacket data(SMSG_STANDSTATE_UPDATE, 1);
        data << (uint8)state;
        ToPlayer()->GetSession()->SendPacket(&data);
    }
}

bool Unit::IsPolymorphed() const
{
    uint32 transformId = getTransForm();
    if (!transformId)
        return false;

    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(transformId);
    if (!spellInfo)
        return false;

    return spellInfo->GetSpellSpecific() == SPELL_SPECIFIC_MAGE_POLYMORPH;
}

void Unit::SetDisplayId(uint32 modelId)
{
    SetUInt32Value(UNIT_FIELD_DISPLAYID, modelId);

    if (GetTypeId() == TYPEID_UNIT && ToCreature()->isPet())
    {
        Pet* pet = ToPet();
        if (!pet->isControlled())
            return;
        Unit* owner = GetOwner();
        if (owner && (owner->GetTypeId() == TYPEID_PLAYER) && owner->ToPlayer()->GetGroup())
            owner->ToPlayer()->SetGroupUpdateFlag(GROUP_UPDATE_FLAG_PET_MODEL_ID);
    }
}

void Unit::RestoreDisplayId()
{
    AuraEffect* handledAura = NULL;
    // try to receive model from transform auras
    Unit::AuraEffectList transforms = GetAuraEffectsByType(SPELL_AURA_TRANSFORM);
    if (!transforms.empty())
    {
        // iterate over already applied transform auras - from newest to oldest
        for (Unit::AuraEffectList::const_reverse_iterator i = transforms.rbegin(); i != transforms.rend(); ++i)
        {
            if (AuraApplication const* aurApp = (*i)->GetBase()->GetApplicationOfTarget(GetGUID()))
            {
                if (!handledAura)
                    handledAura = (*i);
                // prefer negative auras
                if (!aurApp->IsPositive())
                {
                    handledAura = (*i);
                    break;
                }
            }
        }
    }
    // transform aura was found
    if (handledAura)
        handledAura->HandleEffect(this, AURA_EFFECT_HANDLE_SEND_FOR_CLIENT, true);
    // we've found shapeshift
    else if (uint32 modelId = GetModelForForm(GetShapeshiftForm()))
        SetDisplayId(modelId);
    // no auras found - set modelid to default
    else
        SetDisplayId(GetNativeDisplayId());
}

void Unit::ClearComboPointHolders()
{
    while (!m_ComboPointHolders.empty())
    {
        uint32 lowguid = *m_ComboPointHolders.begin();

        Player* player = ObjectAccessor::FindPlayer(MAKE_NEW_GUID(lowguid, 0, HIGHGUID_PLAYER));
        if (player && player->GetComboTarget() == GetGUID())         // recheck for safe
            player->ClearComboPoints();                        // remove also guid from m_ComboPointHolders;
        else
            m_ComboPointHolders.erase(lowguid);             // or remove manually
    }
}

void Unit::ClearAllReactives()
{
    for (uint8 i = 0; i < MAX_REACTIVE; ++i)
        m_reactiveTimer[i] = 0;

    if (HasAuraState(AURA_STATE_DEFENSE))
        ModifyAuraState(AURA_STATE_DEFENSE, false);
    if (getClass() == CLASS_HUNTER && HasAuraState(AURA_STATE_HUNTER_PARRY))
        ModifyAuraState(AURA_STATE_HUNTER_PARRY, false);
    if (getClass() == CLASS_WARRIOR && GetTypeId() == TYPEID_PLAYER)
        ToPlayer()->ClearComboPoints();
}

void Unit::UpdateReactives(uint32 p_time)
{
    for (uint8 i = 0; i < MAX_REACTIVE; ++i)
    {
        ReactiveType reactive = ReactiveType(i);

        if (!m_reactiveTimer[reactive])
            continue;

        if (m_reactiveTimer[reactive] <= p_time)
        {
            m_reactiveTimer[reactive] = 0;

            switch (reactive)
            {
                case REACTIVE_DEFENSE:
                    if (HasAuraState(AURA_STATE_DEFENSE))
                        ModifyAuraState(AURA_STATE_DEFENSE, false);
                    break;
                case REACTIVE_HUNTER_PARRY:
                    if (getClass() == CLASS_HUNTER && HasAuraState(AURA_STATE_HUNTER_PARRY))
                        ModifyAuraState(AURA_STATE_HUNTER_PARRY, false);
                    break;
                case REACTIVE_OVERPOWER:
                    if (getClass() == CLASS_WARRIOR && GetTypeId() == TYPEID_PLAYER)
                        ToPlayer()->ClearComboPoints();
                    break;
                default:
                    break;
            }
        }
        else
        {
            m_reactiveTimer[reactive] -= p_time;
        }
    }
}

Unit* Unit::SelectNearbyTarget(Unit* exclude, float dist) const
{
    std::list<Unit*> targets;
    Trinity::AnyUnfriendlyUnitInObjectRangeCheck u_check(this, this, dist);
    Trinity::UnitListSearcher<Trinity::AnyUnfriendlyUnitInObjectRangeCheck> searcher(this, targets, u_check);
    VisitNearbyObject(dist, searcher);

    // remove current target
    if (getVictim())
        targets.remove(getVictim());

    if (exclude)
        targets.remove(exclude);

    // remove not LoS targets
    for (std::list<Unit*>::iterator tIter = targets.begin(); tIter != targets.end();)
    {
        if (!IsWithinLOSInMap(*tIter) || (*tIter)->isTotem() || (*tIter)->isSpiritService() || (*tIter)->GetCreatureType() == CREATURE_TYPE_CRITTER)
            targets.erase(tIter++);
        else
            ++tIter;
    }

    // no appropriate targets
    if (targets.empty())
        return NULL;

    // select random
    return Trinity::Containers::SelectRandomContainerElement(targets);
}

Unit* Unit::SelectNearbyAlly(Unit* exclude, float dist) const
{
    std::list<Unit*> targets;
    Trinity::AnyFriendlyUnitInObjectRangeCheck u_check(this, this, dist);
    Trinity::UnitListSearcher<Trinity::AnyFriendlyUnitInObjectRangeCheck> searcher(this, targets, u_check);
    VisitNearbyObject(dist, searcher);

    if (exclude)
        targets.remove(exclude);

    // remove not LoS targets
    for (std::list<Unit*>::iterator tIter = targets.begin(); tIter != targets.end();)
    {
        if (!IsWithinLOSInMap(*tIter) || (*tIter)->isTotem() || (*tIter)->isSpiritService() || (*tIter)->GetCreatureType() == CREATURE_TYPE_CRITTER)
            targets.erase(tIter++);
        else
            ++tIter;
    }

    // no appropriate targets
    if (targets.empty())
        return NULL;

    // select random
    return Trinity::Containers::SelectRandomContainerElement(targets);
}

Unit* Unit::GetNearbyVictim(Unit* exclude, float dist, bool IsInFront, bool IsNeutral) const
{
    Unit* Nearby    = NULL;
    float nearbydist = NULL;
    std::list<Unit*> targetList;
    GetAttackableUnitListInRange(targetList, dist);

    if (exclude) targetList.remove(exclude);

    if (!targetList.empty())
    {
        for (std::list<Unit*>::const_iterator itr = targetList.begin(); itr != targetList.end(); ++itr)
        {
            if (!(*itr)->IsWithinLOSInMap(this) || IsFriendlyTo(*itr))
                continue;

            if (IsInFront)
                if (!isInFront(*itr))
                    continue;

            if (IsNeutral)
                if ((*itr)->IsNeutralToAll())
                    continue;

            Position pos;

            if (Nearby)
            {
                (*itr)->GetPosition(&pos);
                float dist2 = GetDistance(pos);

                if (nearbydist > dist2)
                {
                    nearbydist = dist2;
                    Nearby = (*itr);
                }
                else continue;
            }
            else
            {
                Nearby = (*itr);
                Nearby->GetPosition(&pos);
                nearbydist = GetDistance(pos);
            }
        }
    }
    return Nearby;
}

void Unit::CalcAttackTimePercentMod(WeaponAttackType att, float val)
{
    uint32 attackTime = GetAttackTime(att);
    float remainingTimePct = (float)m_attackTimer[att] / (attackTime * m_modAttackSpeedPct[att]);
    
    m_modAttackSpeedPct[att] = val;
    SetAttackTime(att, attackTime);

    if (val && isPet())
        UpdateDamagePhysical(att);

    m_attackTimer[att] = uint32(GetAttackTime(att) * m_modAttackSpeedPct[att] * remainingTimePct);
}

uint32 Unit::GetCastingTimeForBonus(SpellInfo const* spellProto, DamageEffectType damagetype, uint32 CastingTime) const
{
    // Not apply this to creature casted spells with casttime == 0
    if (CastingTime == 0 && GetTypeId() == TYPEID_UNIT && !ToCreature()->isPet())
        return 3500;

    if (CastingTime > 7000) CastingTime = 7000;
    if (CastingTime < 1500) CastingTime = 1500;

    if (damagetype == DOT && !spellProto->IsChanneled())
        CastingTime = 3500;

    int32 overTime    = 0;
    uint8 effects     = 0;
    bool DirectDamage = false;
    bool AreaEffect   = false;

    for (uint32 i = 0; i < MAX_SPELL_EFFECTS; i++)
    {
        switch (spellProto->GetEffect(i, GetSpawnMode())->Effect)
        {
            case SPELL_EFFECT_SCHOOL_DAMAGE:
            case SPELL_EFFECT_POWER_DRAIN:
            case SPELL_EFFECT_HEALTH_LEECH:
            case SPELL_EFFECT_ENVIRONMENTAL_DAMAGE:
            case SPELL_EFFECT_POWER_BURN:
            case SPELL_EFFECT_HEAL:
                DirectDamage = true;
                break;
            case SPELL_EFFECT_APPLY_AURA:
                switch (spellProto->GetEffect(i, GetSpawnMode())->ApplyAuraName)
                {
                    case SPELL_AURA_PERIODIC_DAMAGE:
                    case SPELL_AURA_PERIODIC_HEAL:
                    case SPELL_AURA_PERIODIC_LEECH:
                        if (spellProto->GetDuration())
                            overTime = spellProto->GetDuration();
                        break;
                    default:
                        // -5% per additional effect
                        ++effects;
                        break;
                }
            default:
                break;
        }

        if (spellProto->GetEffect(i, GetSpawnMode())->IsTargetingArea())
            AreaEffect = true;
    }

    // Combined Spells with Both Over Time and Direct Damage
    if (overTime > 0 && CastingTime > 0 && DirectDamage)
    {
        // mainly for DoTs which are 3500 here otherwise
        uint32 OriginalCastTime = spellProto->CalcCastTime();
        if (OriginalCastTime > 7000) OriginalCastTime = 7000;
        if (OriginalCastTime < 1500) OriginalCastTime = 1500;
        // Portion to Over Time
        float PtOT = (overTime / 15000.0f) / ((overTime / 15000.0f) + (OriginalCastTime / 3500.0f));

        if (damagetype == DOT)
            CastingTime = uint32(CastingTime * PtOT);
        else if (PtOT < 1.0f)
            CastingTime  = uint32(CastingTime * (1 - PtOT));
        else
            CastingTime = 0;
    }

    // Area Effect Spells receive only half of bonus
    if (AreaEffect)
        CastingTime /= 2;

    // 50% for damage and healing spells for leech spells from damage bonus and 0% from healing
    for (uint8 j = 0; j < MAX_SPELL_EFFECTS; ++j)
    {
        if (spellProto->GetEffect(j, GetSpawnMode())->Effect == SPELL_EFFECT_HEALTH_LEECH ||
            (spellProto->GetEffect(j, GetSpawnMode())->Effect == SPELL_EFFECT_APPLY_AURA && spellProto->GetEffect(j, GetSpawnMode())->ApplyAuraName == SPELL_AURA_PERIODIC_LEECH))
        {
            CastingTime /= 2;
            break;
        }
    }

    // -5% of total per any additional effect
    for (uint8 i = 0; i < effects; ++i)
        CastingTime *= 0.95f;

    return CastingTime;
}

void Unit::UpdateAuraForGroup(uint8 slot)
{
    if (slot >= MAX_AURAS)                        // slot not found, return
        return;
    if (Player* player = ToPlayer())
    {
        if (player->GetGroup())
        {
            player->SetGroupUpdateFlag(GROUP_UPDATE_FLAG_AURAS);
            player->SetAuraUpdateMaskForRaid(slot);
        }
    }
    else if (GetTypeId() == TYPEID_UNIT && ToCreature()->isPet())
    {
        Pet* pet = ((Pet*)this);
        if (pet->isControlled())
        {
            Unit* owner = GetOwner();
            if (owner && (owner->GetTypeId() == TYPEID_PLAYER) && owner->ToPlayer()->GetGroup())
            {
                owner->ToPlayer()->SetGroupUpdateFlag(GROUP_UPDATE_FLAG_PET_AURAS);
                pet->SetAuraUpdateMaskForRaid(slot);
            }
        }
    }
}

float Unit::CalculateDefaultCoefficient(SpellInfo const *spellInfo, DamageEffectType damagetype) const
{
    // Damage over Time spells bonus calculation
    float DotFactor = 1.0f;
    if (damagetype == DOT)
    {

        int32 DotDuration = spellInfo->GetDuration();
        if (!spellInfo->IsChanneled() && DotDuration > 0)
            DotFactor = DotDuration / 15000.0f;

        if (uint32 DotTicks = spellInfo->GetMaxTicks())
            DotFactor /= DotTicks;
    }

    int32 CastingTime = spellInfo->IsChanneled() ? spellInfo->GetDuration() : spellInfo->CalcCastTime();
    // Distribute Damage over multiple effects, reduce by AoE
    CastingTime = GetCastingTimeForBonus(spellInfo, damagetype, CastingTime);

    // As wowwiki says: C = (Cast Time / 3.5)
    return (CastingTime / 3500.0f) * DotFactor;
}

float Unit::GetAPMultiplier(WeaponAttackType attType, bool normalized)
{
    if (!normalized || GetTypeId() != TYPEID_PLAYER)
        return float(GetAttackTime(attType)) / 1000.0f;

    Item* Weapon = ToPlayer()->GetWeaponForAttack(attType, true);
    if (!Weapon)
        return 2.4f;                                         // fist attack

    switch (Weapon->GetTemplate()->InventoryType)
    {
        case INVTYPE_2HWEAPON:
            return 3.3f;
        case INVTYPE_RANGED:
        case INVTYPE_RANGEDRIGHT:
        case INVTYPE_THROWN:
            return 2.8f;
        case INVTYPE_WEAPON:
        case INVTYPE_WEAPONMAINHAND:
        case INVTYPE_WEAPONOFFHAND:
        default:
            return Weapon->GetTemplate()->SubClass == ITEM_SUBCLASS_WEAPON_DAGGER ? 1.7f : 2.4f;
    }
}

void Unit::SetContestedPvP(Player* attackedPlayer)
{
    Player* player = GetCharmerOrOwnerPlayerOrPlayerItself();

    if (!player || (attackedPlayer && (attackedPlayer == player || (player->duel && player->duel->opponent == attackedPlayer))))
        return;

    player->SetContestedPvPTimer(30000);
    if (!player->HasUnitState(UNIT_STATE_ATTACK_PLAYER))
    {
        player->AddUnitState(UNIT_STATE_ATTACK_PLAYER);
        player->SetFlag(PLAYER_FLAGS, PLAYER_FLAGS_CONTESTED_PVP);
        // call MoveInLineOfSight for nearby contested guards
        UpdateObjectVisibility();
    }
    if (!HasUnitState(UNIT_STATE_ATTACK_PLAYER))
    {
        AddUnitState(UNIT_STATE_ATTACK_PLAYER);
        // call MoveInLineOfSight for nearby contested guards
        UpdateObjectVisibility();
    }
}

Pet* Unit::CreateTamedPetFrom(Creature* creatureTarget, uint32 spell_id)
{
    if (GetTypeId() != TYPEID_PLAYER)
        return NULL;

    Pet* pet = new Pet((Player*)this, HUNTER_PET);

    if (!pet->CreateBaseAtCreature(creatureTarget))
    {
        delete pet;
        return NULL;
    }

    uint8 level = creatureTarget->getLevel() + 5 < getLevel() ? (getLevel() - 5) : creatureTarget->getLevel();

    InitTamedPet(pet, level, spell_id);

    return pet;
}

Pet* Unit::CreateTamedPetFrom(uint32 creatureEntry, uint32 spell_id)
{
    if (GetTypeId() != TYPEID_PLAYER)
        return NULL;

    CreatureTemplate const* creatureInfo = sObjectMgr->GetCreatureTemplate(creatureEntry);
    if (!creatureInfo)
        return NULL;

    Pet* pet = new Pet((Player*)this, HUNTER_PET);

    if (!pet->CreateBaseAtCreatureInfo(creatureInfo, this) || !InitTamedPet(pet, getLevel(), spell_id))
    {
        delete pet;
        return NULL;
    }

    return pet;
}

bool Unit::InitTamedPet(Pet* pet, uint8 level, uint32 spell_id)
{
    pet->SetCreatorGUID(GetGUID());
    pet->setFaction(getFaction());
    pet->SetUInt32Value(UNIT_CREATED_BY_SPELL, spell_id);

    if (GetTypeId() == TYPEID_PLAYER)
        pet->SetUInt32Value(UNIT_FIELD_FLAGS, UNIT_FLAG_PVP_ATTACKABLE);

    if (!pet->InitStatsForLevel(level))
    {
        sLog->outError(LOG_FILTER_UNITS, "Pet::InitStatsForLevel() failed for creature (Entry: %u)!", pet->GetEntry());
        return false;
    }

    pet->GetCharmInfo()->SetPetNumber(sObjectMgr->GeneratePetNumber(), true);
    // this enables pet details window (Shift+P)
    pet->InitPetCreateSpells();
    pet->SetFullHealth();
    return true;
}

bool Unit::SpellProcTriggered(Unit* victim, DamageInfo* dmgInfoProc, AuraEffect* triggeredByAura, SpellInfo const* procSpell, uint32 procFlag, uint32 procEx, double cooldown)
{
    uint32 damage = dmgInfoProc->GetDamage();
    int32 addpowertype = dmgInfoProc->GetAddPType();
    SpellInfo const* dummySpell = triggeredByAura->GetSpellInfo();
    uint32 effIndex = triggeredByAura->GetEffIndex();
    int32  triggerAmount = triggeredByAura->GetAmount();
    int32 cooldown_spell_id = 0;
    uint32 triggered_spell_id = triggeredByAura->GetTriggerSpell() ? triggeredByAura->GetTriggerSpell(): 0;

    Item* castItem = triggeredByAura->GetBase()->GetCastItemGUID() && GetTypeId() == TYPEID_PLAYER
        ? ToPlayer()->GetItemByGuid(triggeredByAura->GetBase()->GetCastItemGUID()) : NULL;

    Unit* target = victim;
    int32 basepoints0 = NULL;
    int32 basepoints1 = NULL;
    int32 basepoints2 = NULL;
    uint64 originalCaster = 0;
    Unit* procSpellCaster = dmgInfoProc->GetAttacker();
    uint64 procSpellCasterGUID = procSpellCaster ? procSpellCaster->GetGUID(): 0;
    int32 schoolMask = procSpell ? SpellSchoolMask(procSpell->SchoolMask) : SPELL_SCHOOL_MASK_NORMAL;

    if (std::vector<SpellTriggered> const* spellTrigger = sSpellMgr->GetSpellTriggered(dummySpell->Id))
    {
        bool check = false;
        std::list<int32> groupList;
        for (std::vector<SpellTriggered>::const_iterator itr = spellTrigger->begin(); itr != spellTrigger->end(); ++itr)
        {
            if(itr->dummyId) //take amount from other spell
            {
                if(SpellInfo const* dummySpellInfo = sSpellMgr->GetSpellInfo(abs(itr->dummyId)))
                    triggerAmount = dummySpellInfo->Effects[itr->dummyEffect].BasePoints;
            }

            sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "SpellTriggered target %u, caster %u, spell_trigger %i, chance %i, triggerAmount %i, damage %i, GetAbsorb %i, GetResist %i, GetBlock %i",
            itr->target, itr->caster, itr->spell_trigger, itr->chance, triggerAmount, damage, dmgInfoProc->GetAbsorb(), dmgInfoProc->GetResist(), dmgInfoProc->GetBlock());
            sLog->outDebug(LOG_FILTER_SPELLS_AURAS, " group %i, effIndex %i, effectmask %i, option %i, (1<<effIndex) %i, procFlag %i addpowertype %i addptype %i procEx %i",
            itr->group, effIndex, itr->effectmask, itr->option, (1<<effIndex), procFlag, addpowertype, itr->addptype, procEx);

            if(itr->spell_cooldown)
                cooldown_spell_id = abs(itr->spell_cooldown);
            else
                cooldown_spell_id = abs(itr->spell_trigger);
            if (G3D::fuzzyGt(cooldown, 0.0) && GetTypeId() == TYPEID_PLAYER && ToPlayer()->HasSpellCooldown(cooldown_spell_id))
                return false;

            if (!(itr->effectmask & (1<<effIndex)))
                continue;

            if (itr->procFlags > 0 && !(itr->procFlags & procFlag))
                continue;
            else if (itr->procFlags < 0 && (abs(itr->procFlags) & procFlag))
                continue;

            if (itr->procEx > 0 && !(itr->procEx & procEx))
                continue;
            else if (itr->procEx < 0 && (abs(itr->procEx) & procEx))
                continue;

            if (itr->schoolMask > 0 && !(itr->schoolMask & schoolMask))
                continue;
            else if (itr->schoolMask < 0 && (abs(itr->schoolMask) & schoolMask))
                continue;

            if(itr->addptype != -1 && itr->addptype != addpowertype)
                continue;
            else if(itr->addptype == -1 && addpowertype != -1)
                continue;

            if (itr->check_spell_id > 0)
            {
                if (!procSpell)
                    continue;
                if (procSpell->Id != itr->check_spell_id)
                    continue;
            }
            else if (itr->check_spell_id < 0)
            {
                if (!procSpell)
                    continue;
                if (procSpell->Id == abs(itr->check_spell_id))
                    continue;
            }

            if(itr->chance != 0)
            {
                if(itr->chance > 100) // chance get from amount
                {
                    if(!roll_chance_i(triggerAmount))
                        continue;
                }
                else if(itr->chance == -1) // chance get from amount / 10
                {
                    int32 rollchance = urand(0, 1000);
                    if (rollchance > triggerAmount)
                        continue;
                }
                else if(itr->chance == -2) // chance get from amount / 100
                {
                    int32 rollchance = urand(0, 10000);
                    if (rollchance > triggerAmount)
                        continue;
                }
                else if(!roll_chance_i(itr->chance))
                    continue;
            }

            if(itr->group != 0 && !groupList.empty())
            {
                bool groupFind = false;
                for (std::list<int32>::const_iterator group_itr = groupList.begin(); group_itr != groupList.end(); ++group_itr)
                    if((*group_itr) == itr->group)
                        groupFind = true;
                if(groupFind)
                    continue;
            }

            if(itr->target == 8 && target == this) //not trigger spell for self
                continue;
            if(itr->target == 1 || itr->target == 6 || !target) //get target self
                target = this;
            if(itr->target == 3 && ToPlayer()) //get target owner
                if (Pet* pet = ToPlayer()->GetPet())
                    target = (Unit*)pet;
            if(itr->target == 4 && target->ToPlayer()) //get target pet
                if (Pet* pet = target->ToPlayer()->GetPet())
                    target = (Unit*)pet;
            if(itr->target == 5) //get target owner
                if (Unit* owner = GetOwner())
                    target = owner;
            if(itr->target == 7) //get target self
                target = triggeredByAura->GetCaster();

            Unit* _caster = this;
            Unit* _casterAura = triggeredByAura->GetCaster();
            Unit* _targetAura = this;

            if(itr->caster == 1) //get caster aura
                _caster = triggeredByAura->GetCaster();
            if(itr->caster == 3 && ToPlayer()) //get caster owner
                if (Pet* pet = ToPlayer()->GetPet())
                    _caster = (Unit*)pet;
            if(itr->caster == 4) //get caster owner
                if (Unit* owner = GetOwner())
                    _caster = owner;
            if(!_caster)
                _caster = this;

            if(itr->target == 9) //get target select
                if (Player* _player = _caster->ToPlayer())
                    if (Unit* _select = _player->GetSelectedUnit())
                        target = _select;

            if(itr->targetaura == 1) //get caster aura
                _targetAura = triggeredByAura->GetCaster();
            if(itr->targetaura == 2) //get target aura
                _targetAura = victim;
            if(itr->targetaura == 3) //get target select
                if (Player* _player = ToPlayer())
                    if (Unit* _select = _player->GetSelectedUnit())
                        _targetAura = _select;

            uint64 casterGuid = _targetAura != _casterAura ? (_casterAura ? _casterAura->GetGUID() : 0) : 0;

            if (itr->option != SPELL_TRIGGER_CHECK_PROCK && itr->option != SPELL_TRIGGER_COOLDOWN && itr->option != SPELL_TRIGGER_MODIFY_COOLDOWN)
            {
                if(itr->aura > 0 && !_targetAura->HasAura(itr->aura, casterGuid))
                {
                    check = true;
                    continue;
                }
                if(itr->aura < 0 && _targetAura->HasAura(abs(itr->aura), casterGuid))
                {
                    check = true;
                    continue;
                }
            }

            int32 bp0 = int32(itr->bp0);
            int32 bp1 = int32(itr->bp1);
            int32 bp2 = int32(itr->bp2);
            switch (itr->option)
            {
                case SPELL_TRIGGER_BP: //0
                {
                    if(itr->spell_trigger > 0)
                        basepoints0 = triggerAmount;
                    else
                        basepoints0 = -(triggerAmount);

                    triggered_spell_id = abs(itr->spell_trigger);
                    if(basepoints0)
                        _caster->CastCustomSpell(target, triggered_spell_id, &basepoints0, &basepoints0, &basepoints0, true, castItem, triggeredByAura, originalCaster);
                    else
                        _caster->CastSpell(target, triggered_spell_id, true);
                    if(itr->target == 6)
                    {
                        if (Guardian* pet = GetGuardianPet())
                        {
                            if(basepoints0)
                                _caster->CastCustomSpell(pet, triggered_spell_id, &basepoints0, &bp1, &bp2, true);
                            else
                                _caster->CastSpell(pet, triggered_spell_id, true);
                        }
                    }
                    check = true;
                }
                break;
                case SPELL_TRIGGER_BP_CUSTOM: //1
                {
                    triggered_spell_id = abs(itr->spell_trigger);
                    _caster->CastCustomSpell(target, triggered_spell_id, &bp0, &bp1, &bp2, true, castItem, triggeredByAura, originalCaster);
                    if(itr->target == 6)
                    {
                        if (Guardian* pet = GetGuardianPet())
                            _caster->CastCustomSpell(pet, triggered_spell_id, &basepoints0, &bp1, &bp2, true);
                    }
                    check = true;
                }
                break;
                case SPELL_TRIGGER_MANA_COST: //2
                {
                    if(!procSpell)
                        continue;
                    int32 cost = int32(procSpell->PowerCost + CalculatePct(GetCreateMana(), procSpell->PowerCostPercentage));
                    basepoints0 = CalculatePct(cost, bp0);

                    triggered_spell_id = abs(itr->spell_trigger);
                    _caster->CastCustomSpell(target, triggered_spell_id, &basepoints0, &bp1, &bp2, true, castItem, triggeredByAura, originalCaster);
                    if(itr->target == 6)
                    {
                        if (Guardian* pet = GetGuardianPet())
                            _caster->CastCustomSpell(pet, triggered_spell_id, &basepoints0, &bp1, &bp2, true);
                    }
                    check = true;
                }
                break;
                case SPELL_TRIGGER_DAM_HEALTH: //3
                {
                    float percent = 0.0f;
                    if (Player* plr = ToPlayer())
                    {
                        if (dummySpell->AttributesEx8 & SPELL_ATTR8_MASTERY_SPECIALIZATION)
                            percent = plr->GetFloatValue(PLAYER_MASTERY) * dummySpell->GetEffect(effIndex, m_diffMode)->BonusMultiplier;
                    }

                    if (!percent)
                        percent = triggerAmount;

                    if(bp0)
                        percent += bp0;
                    if(bp1)
                        percent /= bp1;
                    if(bp2)
                        percent *= bp2;

                    basepoints0 = CalculatePct(int32(dmgInfoProc->GetDamage() + dmgInfoProc->GetAbsorb()), percent);

                    triggered_spell_id = abs(itr->spell_trigger);
                    _caster->CastCustomSpell(target, triggered_spell_id, &basepoints0, &basepoints0, &basepoints0, true, castItem, triggeredByAura, originalCaster);
                    if(itr->target == 6)
                    {
                        if (Guardian* pet = GetGuardianPet())
                            _caster->CastCustomSpell(pet, triggered_spell_id, &basepoints0, &basepoints0, &basepoints0, true);
                    }
                    check = true;
                }
                break;
                case SPELL_TRIGGER_COOLDOWN: //4
                {
                    if(Player* player = target->ToPlayer())
                    {
                        uint32 spellid = abs(itr->spell_trigger);
                        if(itr->bp0 == 0.0f)
                            player->RemoveSpellCooldown(spellid, true);
                        else
                        {
                            if(itr->aura && !procSpell)
                                continue;
                            if(itr->aura && procSpell && itr->aura != procSpell->Id)
                                continue;

                            int32 delay = itr->bp0;
                            player->ModifySpellCooldown(spellid, delay);
                        }
                    }
                    check = true;
                }
                break;
                case SPELL_TRIGGER_UPDATE_DUR: //5
                {
                    if (itr->spell_trigger > 0)
                    {
                        if (Aura* aura = target->GetAura(abs(itr->spell_trigger), GetGUID()))
                            aura->RefreshTimers();
                    }
                    else
                    {
                        if (procSpell->Id == -(itr->spell_trigger))
                            if (procSpellCasterGUID == triggeredByAura->GetCasterGUID())
                                triggeredByAura->GetBase()->RefreshTimers();
                    }

                    check = true;
                    break;
                }
                case SPELL_TRIGGER_GET_DUR_AURA: //6
                {
                    if(Aura* aura = target->GetAura(itr->aura, GetGUID()))
                        basepoints0 = int32(aura->GetDuration() / 1000);
                    if(basepoints0)
                    {
                        triggered_spell_id = abs(itr->spell_trigger);
                        _caster->CastCustomSpell(this, triggered_spell_id, &basepoints0, &bp1, &bp2, true, castItem, triggeredByAura, originalCaster);
                        if(itr->target == 6)
                        {
                            if (Guardian* pet = GetGuardianPet())
                                _caster->CastCustomSpell(pet, triggered_spell_id, &basepoints0, &bp1, &bp2, true);
                        }
                    }
                    check = true;
                }
                break;
                case SPELL_TRIGGER_COMBOPOINTS_TO_CHANCE: //7
                {
                    if(!procSpell || !procSpell->NeedsComboPoints())
                    {
                        check = true;
                        continue;
                    }

                    int32 chance = 20 * ToPlayer()->GetComboPoints();
                    if (roll_chance_i(chance))
                    {
                        triggered_spell_id = abs(itr->spell_trigger);
                        _caster->CastCustomSpell(target, triggered_spell_id, &bp0, &bp1, &bp2, true, castItem, triggeredByAura, originalCaster);
                        if(itr->target == 6)
                        {
                            if (Guardian* pet = GetGuardianPet())
                                _caster->CastCustomSpell(pet, triggered_spell_id, &basepoints0, &bp1, &bp2, true);
                        }
                    }
                    check = true;
                }
                break;
                case SPELL_TRIGGER_UPDATE_DUR_TO_MAX: //8
                {
                    if(Aura* aura = target->GetAura(abs(itr->spell_trigger), GetGUID()))
                        aura->SetDuration(aura->GetSpellInfo()->GetMaxDuration(), true);
                    check = true;
                }
                break;
                case SPELL_TRIGGER_PERC_FROM_DAMGE: //9
                {
                    float amount = bp0;

                    if (!bp0)
                        amount = triggerAmount / 100.0f;

                    basepoints0 = CalculatePct(damage, amount);

                    triggered_spell_id = abs(itr->spell_trigger);
                    _caster->CastCustomSpell(target, triggered_spell_id, &basepoints0, &bp1, &bp2, true, castItem, triggeredByAura, originalCaster);
                    if(itr->target == 6)
                    {
                        if (Guardian* pet = GetGuardianPet())
                            _caster->CastCustomSpell(pet, triggered_spell_id, &basepoints0, &bp1, &bp2, true);
                    }
                    check = true;
                }
                break;
                case SPELL_TRIGGER_PERC_MAX_MANA: //10
                {
                    int32 percent = triggerAmount;
                    if(bp0)
                        percent += bp0;
                    if(bp1)
                        percent /= bp1;
                    if(bp2)
                        percent *= bp2;
                    basepoints0 = CalculatePct(target->GetMaxPower(POWER_MANA), percent);

                    triggered_spell_id = abs(itr->spell_trigger);
                    //target->EnergizeBySpell(target, triggered_spell_id, basepoints0, POWER_MANA);
                    _caster->CastCustomSpell(target, triggered_spell_id, &basepoints0, NULL, NULL, true, castItem, triggeredByAura, originalCaster);
                    check = true;
                }
                break;
                case SPELL_TRIGGER_PERC_BASE_MANA: //11
                {
                    int32 percent = triggerAmount;
                    if(bp0)
                        percent += bp0;
                    if(bp1)
                        percent /= bp1;
                    if(bp2)
                        percent *= bp2;
                    basepoints0 = CalculatePct(target->GetCreateMana(), percent);

                    triggered_spell_id = abs(itr->spell_trigger);
                    _caster->CastCustomSpell(target, triggered_spell_id, &basepoints0, NULL, NULL, true, castItem, triggeredByAura, originalCaster);
                    //target->EnergizeBySpell(target, triggered_spell_id, basepoints0, POWER_MANA);
                    check = true;
                }
                break;
                case SPELL_TRIGGER_PERC_CUR_MANA: //12
                {
                    basepoints0 = int32((itr->bp0 / 100.0f) * target->GetPower(POWER_MANA));

                    triggered_spell_id = abs(itr->spell_trigger);
                    target->EnergizeBySpell(target, triggered_spell_id, basepoints0, POWER_MANA);
                    check = true;
                }
                break;
                case SPELL_TRIGGER_CHECK_PROCK: //13
                {
                    if(!procSpell)
                    {
                        check = true;
                        continue;
                    }
                    triggered_spell_id = abs(itr->spell_trigger);

                    if(itr->aura == procSpell->Id)
                        _caster->CastSpell(target, triggered_spell_id, true);

                    check = true;
                }
                break;
                case SPELL_TRIGGER_CHECK_DAMAGE: //16
                {
                    triggered_spell_id = abs(itr->spell_trigger);

                    if(int32(damage) >= triggerAmount)
                    {
                        _caster->CastSpell(target, triggered_spell_id, true);
                        triggeredByAura->GetBase()->Remove(AURA_REMOVE_BY_DEFAULT);
                        check = true;
                        continue;
                    }

                    triggeredByAura->SetAmount(triggerAmount - damage);

                    check = true;
                }
                break;
                case SPELL_TRIGGER_CAST_OR_REMOVE: // 20
                {
                    triggered_spell_id = abs(itr->spell_trigger);

                    if (itr->spell_trigger > 0)
                        _caster->CastSpell(target, triggered_spell_id, true);
                    else
                        _caster->RemoveAurasDueToSpell(triggered_spell_id);

                    check = true;
                    break;
                }
                case SPELL_TRIGGER_UPDATE_DUR_TO_IGNORE_MAX: //21
                {
                    if(Aura* aura = target->GetAura(abs(itr->spell_trigger), GetGUID()))
                    {
                        int32 _duration = int32(aura->GetDuration() + int32(triggerAmount * 1000));
                        if(bp0 && bp0 < _duration)
                            _duration = int32(bp0);

                        aura->SetDuration(_duration, true);
                        if (_duration > aura->GetMaxDuration())
                            aura->SetMaxDuration(_duration);
                    }
                    check = true;
                }
                break;
                case SPELL_TRIGGER_ADD_DURATION: //22
                {
                    triggered_spell_id = abs(itr->spell_trigger);
                    if(Aura* aura = target->GetAura(triggered_spell_id))
                    {
                        int32 _duration = int32(aura->GetDuration() + triggerAmount);
                        if (_duration < aura->GetMaxDuration())
                            aura->SetDuration(_duration, true);
                    }
                    check = true;
                }
                break;
                case SPELL_TRIGGER_MODIFY_COOLDOWN: //23
                {
                    if(Player* player = target->ToPlayer())
                    {
                        uint32 triggered_spell_id = abs(itr->spell_trigger);
                        if(itr->aura && !procSpell)
                            continue;
                        if(itr->aura && procSpell && itr->aura != procSpell->Id)
                            continue;

                        int32 ChangeCooldown = triggerAmount;
                        if(ChangeCooldown < 100)
                            ChangeCooldown *= IN_MILLISECONDS;
                        player->ModifySpellCooldown(triggered_spell_id, -ChangeCooldown);
                    }
                    check = true;
                    break;
                }
                case SPELL_TRIGGER_VENGEANCE: // 24
                {
                    if (!target || target->GetCharmerOrOwnerPlayerOrPlayerItself() || (procSpell && procSpell->IsAffectingArea()))
                        return false;

                    if (!dmgInfoProc->GetDamageBeforeHit())
                        return false;

                    Creature* creature = target->ToCreature();

                    if (!creature)
                        return false;

                    uint64 creatureGUID = creature->GetGUID();
                     int32 alldamage = dmgInfoProc->GetDamageBeforeHit();
                     uint32 count = 1;
                    float creatureThreat = 0.0f;
                    ThreatContainer& _threatContainer = getThreatManager().getOnlineContainer();

                    if (!_threatContainer.empty())
                    {
                        HostileReference* refer = _threatContainer.getReferenceByTarget(creature);

                        if (!refer)
                            getThreatManager().resetAllAggro();
                    }

                    getThreatManager().addThreat(creature, float(alldamage), procSpell ? procSpell->GetSchoolMask() : SPELL_SCHOOL_MASK_NORMAL, procSpell);

                    std::list<HostileReference*>& _threatList = getThreatManager().getThreatList();

                    for (std::list<HostileReference*>::const_iterator _iter = _threatList.begin(); _iter != _threatList.end(); ++_iter)
                        if ((*_iter) && (*_iter)->getUnitGuid() == creatureGUID)
                        {
                            creatureThreat = (*_iter)->getThreat();
                            break;
                        }

                    for (std::list<HostileReference*>::const_iterator _iter = _threatList.begin(); _iter != _threatList.end(); ++_iter)
                    {
                        if ((*_iter)->getUnitGuid() == creatureGUID)
                            continue;

                        if ((*_iter)->getThreat() > creatureThreat)
                            count++;
                    }

                    triggered_spell_id = itr->spell_trigger;

                    float _percent = (triggerAmount / 100.0f);
                    int32 minBp = CalculatePct(alldamage, _percent * 5.0f);

                    int32 bp = 0;

                    if (Aura* oldAura = GetAura(triggered_spell_id, GetGUID()))
                    {
                        if (AuraEffect* oldEff = oldAura->GetEffect(EFFECT_0))
                        {
                            int32 oldamount = oldEff->GetAmount() * float(oldAura->GetDuration()) / float(oldAura->GetMaxDuration());
                            bp = CalculatePct(alldamage, _percent / count);
                            bp += oldamount;
                        }
                    }

                    if (bp < minBp)
                        bp = minBp;

                    int32 maxVal = int32(GetMaxHealth() * 0.5f);

                    if (bp > maxVal)
                        bp = maxVal;

                    basepoints0 = bp;
                    basepoints1 = bp;

                    _caster->CastCustomSpell(target, triggered_spell_id, &basepoints0, &basepoints1, &basepoints2, true, castItem, triggeredByAura, originalCaster);
                    if(itr->target == 6)
                    {
                        if (Guardian* pet = GetGuardianPet())
                            _caster->CastCustomSpell(pet, triggered_spell_id, &basepoints0, &basepoints1, &basepoints2, true);
                    }
                    check = true;
                    break;
                }
                case SPELL_TRIGGER_ADD_DURATION_OR_CAST: //25
                {
                    if(Aura* aura = target->GetAura(abs(itr->spell_trigger)))
                    {
                        int32 _duration = int32(aura->GetDuration() + aura->CalcMaxDuration(target));
                        aura->SetDuration(_duration, true);
                        if (_duration > aura->GetMaxDuration())
                            aura->SetMaxDuration(_duration);
                    }
                    else
                        _caster->CastSpell(target, itr->spell_trigger, true);
                    check = true;
                }
                break;
                case SPELL_TRIGGER_REMOVE_CD_RUNE: //26
                {
                    if(Player* _player = _caster->ToPlayer())
                    {
                        int32 runesRestor = 0;
                        for (int i = 0; i < MAX_RUNES ; i++)
                        {
                            if (_player->GetRuneCooldown(i) == RUNE_BASE_COOLDOWN && runesRestor < 1)
                            {
                                runesRestor++;
                                _player->SetRuneCooldown(i, 0);
                                _player->AddRunePower(i);
                            }
                        }
                    }
                    check = true;
                }
                break;
                case SPELL_TRIGGER_BP_SPELLID: //27
                {
                    if(!procSpell)
                    {
                        check = true;
                        continue;
                    }

                    triggered_spell_id = abs(itr->spell_trigger);
                    if(itr->bp0)
                        basepoints0 = procSpell->Id;
                    if(itr->bp1)
                        basepoints1 = procSpell->Id;
                    if(itr->bp2)
                        basepoints2 = procSpell->Id;
                    _caster->CastCustomSpell(target, triggered_spell_id, &basepoints0, &basepoints1, &basepoints2, true, castItem, triggeredByAura, originalCaster);
                    if(itr->target == 6)
                    {
                        if (Guardian* pet = GetGuardianPet())
                            _caster->CastCustomSpell(pet, triggered_spell_id, &basepoints0, &basepoints1, &basepoints2, true);
                    }
                    check = true;
                }
                break;
                case SPELL_TRIGGER_BP_SPD_AP: //28
                {
                    if(!procSpell)
                    {
                        check = true;
                        continue;
                    }

                    WeaponAttackType attType = BASE_ATTACK;
                    if (procSpell->DmgClass == SPELL_DAMAGE_CLASS_RANGED)
                        attType = RANGED_ATTACK;

                    int32 SPD = GetSpellPowerDamage(SPELL_SCHOOL_MASK_ALL);
                    int32 SPDH = GetSpellPowerHealing();
                    int32 AP = GetTotalAttackPowerValue(attType);

                    triggered_spell_id = abs(itr->spell_trigger);
                    switch (int32(itr->bp0))
                    {
                        case 1:
                            basepoints0 = SPD;
                            break;
                        case 2:
                            basepoints0 = SPDH;
                            break;
                        case 3:
                            basepoints0 = AP;
                            break;
                    }
                    switch (int32(itr->bp1))
                    {
                        case 1:
                            basepoints1 = SPD;
                            break;
                        case 2:
                            basepoints1 = SPDH;
                            break;
                        case 3:
                            basepoints1 = AP;
                            break;
                    }
                    switch (int32(itr->bp2))
                    {
                        case 1:
                            basepoints2 = SPD;
                            break;
                        case 2:
                            basepoints2 = SPDH;
                            break;
                        case 3:
                            basepoints2 = AP;
                            break;
                    }

                    _caster->CastCustomSpell(target, triggered_spell_id, &basepoints0, &basepoints1, &basepoints2, true, castItem, triggeredByAura, originalCaster);
                    if(itr->target == 6)
                    {
                        if (Guardian* pet = GetGuardianPet())
                            _caster->CastCustomSpell(pet, triggered_spell_id, &basepoints0, &basepoints1, &basepoints2, true);
                    }
                    check = true;
                }
                break;
                case SPELL_TRIGGER_COMBOPOINT_BP: //29
                {
                    if(!procSpell || !procSpell->NeedsComboPoints())
                    {
                        check = true;
                        continue;
                    }

                    int32 basepoints0 = triggerAmount * ToPlayer()->GetComboPoints();
                    triggered_spell_id = abs(itr->spell_trigger);

                    _caster->CastCustomSpell(target, triggered_spell_id, &basepoints0, &basepoints0, &basepoints0, true, castItem, triggeredByAura, originalCaster);
                    if(itr->target == 6)
                    {
                        if (Guardian* pet = GetGuardianPet())
                            _caster->CastCustomSpell(pet, triggered_spell_id, &basepoints0, &basepoints0, &basepoints0, true);
                    }
                    check = true;
                }
                break;
                case SPELL_TRIGGER_DAM_PERC_FROM_MAX_HP: //30
                {
                    basepoints0 = int32(float(dmgInfoProc->GetDamage() * 100.0f) / target->GetMaxHealth());
                    if(bp0)
                        basepoints0 *= bp0;

                    triggered_spell_id = abs(itr->spell_trigger);
                    _caster->CastCustomSpell(target, triggered_spell_id, &basepoints0, &basepoints0, &basepoints0, true, castItem, triggeredByAura, originalCaster);
                    if(itr->target == 6)
                    {
                        if (Guardian* pet = GetGuardianPet())
                            _caster->CastCustomSpell(pet, triggered_spell_id, &basepoints0, &basepoints0, &basepoints0, true);
                    }
                    check = true;
                }
                break;
                case SPELL_TRIGGER_SUMM_DAMAGE_PROC: //31
                {
                    int32 limited = 0;
                    int32 summ_damage = triggerAmount + dmgInfoProc->GetDamage();
                    triggered_spell_id = abs(itr->spell_trigger);
                    if (itr->bp0)
                        limited = int32(GetSpellPowerDamage(SPELL_SCHOOL_MASK_ALL) * itr->bp0);
                    else if (itr->bp1)
                        limited = int32(GetSpellPowerHealing() * itr->bp1);
                    else if (itr->bp2)
                    {
                        WeaponAttackType attType = BASE_ATTACK;
                        if (procSpell && procSpell->DmgClass == SPELL_DAMAGE_CLASS_RANGED)
                            attType = RANGED_ATTACK;
                        limited = int32(GetTotalAttackPowerValue(attType) * itr->bp2);
                    }

                    if(summ_damage < limited)
                    {
                        triggeredByAura->SetAmount(summ_damage);
                        check = true;
                        continue;
                    }
                    else
                        triggeredByAura->SetAmount(0);

                    _caster->CastSpell(target, triggered_spell_id, true);
                    if(itr->target == 6)
                    {
                        if (Guardian* pet = GetGuardianPet())
                            _caster->CastSpell(pet, triggered_spell_id, true);
                    }
                    check = true;
                }
                break;
                case SPELL_TRIGGER_ADDPOWER_PCT: //32
                {
                    int32 percent = triggerAmount;
                    if(bp0)
                        percent += bp0;
                    if(bp1)
                        percent /= bp1;
                    if(bp2)
                        percent *= bp2;

                    if(!dmgInfoProc->GetAddPower())
                    {
                        check = true;
                        continue;
                    }

                    basepoints0 = CalculatePct(dmgInfoProc->GetAddPower(), percent);

                    triggered_spell_id = abs(itr->spell_trigger);
                    _caster->CastCustomSpell(target, triggered_spell_id, &basepoints0, &basepoints0, &basepoints0, true, castItem, triggeredByAura, originalCaster);
                    if(itr->target == 6)
                    {
                        if (Guardian* pet = GetGuardianPet())
                            _caster->CastCustomSpell(pet, triggered_spell_id, &basepoints0, &basepoints0, &basepoints0, true);
                    }
                    check = true;
                }
                break;
                case SPELL_TRIGGER_ADD_ABSORB_PCT: //33
                {
                    int32 percent = triggerAmount;
                    if(bp0)
                        percent += bp0;
                    if(bp1)
                        percent /= bp1;
                    if(bp2)
                        percent *= bp2;

                    if(!dmgInfoProc->GetAbsorb())
                    {
                        check = true;
                        continue;
                    }

                    basepoints0 = CalculatePct(dmgInfoProc->GetAbsorb(), percent);

                    triggered_spell_id = abs(itr->spell_trigger);
                    _caster->CastCustomSpell(target, triggered_spell_id, &basepoints0, &basepoints0, &basepoints0, true, castItem, triggeredByAura, originalCaster);
                    if(itr->target == 6)
                    {
                        if (Guardian* pet = GetGuardianPet())
                            _caster->CastCustomSpell(pet, triggered_spell_id, &basepoints0, &basepoints0, &basepoints0, true);
                    }
                    check = true;
                }
                break;
                case SPELL_TRIGGER_ADD_BLOCK_PCT: //34
                {
                    int32 percent = triggerAmount;
                    if(bp0)
                        percent += bp0;
                    if(bp1)
                        percent /= bp1;
                    if(bp2)
                        percent *= bp2;

                    if(!dmgInfoProc->GetBlock())
                    {
                        check = true;
                        continue;
                    }

                    basepoints0 = CalculatePct(dmgInfoProc->GetBlock(), percent);

                    triggered_spell_id = abs(itr->spell_trigger);
                    _caster->CastCustomSpell(target, triggered_spell_id, &basepoints0, &basepoints0, &basepoints0, true, castItem, triggeredByAura, originalCaster);
                    if(itr->target == 6)
                    {
                        if (Guardian* pet = GetGuardianPet())
                            _caster->CastCustomSpell(pet, triggered_spell_id, &basepoints0, &basepoints0, &basepoints0, true);
                    }
                    check = true;
                }
                break;
                case SPELL_TRIGGER_NEED_COMBOPOINTS: //35
                {
                    if(!procSpell || !procSpell->NeedsComboPoints())
                    {
                        check = true;
                        continue;
                    }

                    triggered_spell_id = abs(itr->spell_trigger);
                    _caster->CastSpell(target, triggered_spell_id, true);
                    check = true;
                }
                break;
                case SPELL_TRIGGER_ADD_STACK: //17
                {
                    triggered_spell_id = abs(itr->spell_trigger);

                    if(Aura* aura = _caster->GetAura(triggered_spell_id))
                        if(aura->GetCharges() < (triggerAmount + bp1))
                            aura->ModStackAmount(bp0);

                    check = true;
                }
                break;
                case SPELL_TRIGGER_ADD_CHARGES: //18
                {
                    triggered_spell_id = abs(itr->spell_trigger);

                    if(Aura* aura = _caster->GetAura(triggered_spell_id))
                        if(aura->GetCharges() < (triggerAmount + bp1))
                            aura->ModCharges(bp0);

                    check = true;
                }
                break;
                case SPELL_TRIGGER_ADD_CHARGES_STACK: //19
                {
                    triggered_spell_id = abs(itr->spell_trigger);

                    if(Aura* aura = _caster->GetAura(triggered_spell_id))
                        if(aura->GetCharges() < (triggerAmount + bp1))
                        {
                            aura->ModCharges(bp0);
                            aura->ModStackAmount(bp0);
                        }

                    check = true;
                }
                break;
                case SPELL_TRIGGER_HOLYPOWER_BONUS: //36
                {
                    int32 percent = triggerAmount;
                    if(bp0)
                        percent += bp0;
                    if(bp1)
                        percent /= bp1;
                    if(bp2)
                        percent *= bp2;

                    basepoints0 = CalculatePct(int32(dmgInfoProc->GetDamage() + dmgInfoProc->GetAbsorb()), percent) / GetModForHolyPowerSpell();

                    triggered_spell_id = abs(itr->spell_trigger);
                    _caster->CastCustomSpell(target, triggered_spell_id, &basepoints0, &basepoints0, &basepoints0, true, castItem, triggeredByAura, originalCaster);
                    if(itr->target == 6)
                    {
                        if (Guardian* pet = GetGuardianPet())
                            _caster->CastCustomSpell(pet, triggered_spell_id, &basepoints0, &basepoints0, &basepoints0, true);
                    }
                    check = true;
                }
                break;
                case SPELL_TRIGGER_CAST_AFTER_MAX_STACK: // 37
                {
                    if (target->HasAura(itr->spell_trigger))
                    {
                        check = true;
                        continue;
                    }
                    if (itr->bp0 > 0 && !_targetAura->HasAura(itr->bp0))
                    {
                        check = true;
                        continue;
                    }
                    if (itr->bp0 < 0 && _targetAura->HasAura(abs(itr->bp0)))
                    {
                        check = true;
                        continue;
                    }

                    triggered_spell_id = itr->aura;

                    if (Aura* aura = _targetAura->GetAura(triggered_spell_id))
                    {
                        if (uint32((aura->GetStackAmount() + 1)) > (aura->GetSpellInfo()->StackAmount - 1))
                        {
                            RemoveAurasDueToSpell(triggered_spell_id);
                            triggered_spell_id = itr->spell_trigger;
                        }
                    }
                    _caster->CastSpell(target, triggered_spell_id, true, castItem);
                    break;
                }
            }
            if(itr->group != 0 && check)
                groupList.push_back(itr->group);
        }

        if (G3D::fuzzyGt(cooldown, 0.0) && GetTypeId() == TYPEID_PLAYER)
            ToPlayer()->AddSpellCooldown(cooldown_spell_id, 0, getPreciseTime() + cooldown);
        if(check)
            return true;
    }
    return false;
}

bool Unit::SpellProcCheck(Unit* victim, SpellInfo const* spellProto, SpellInfo const* procSpell, uint8 effect)
{
    bool procCheck = false;
    bool procCheckActiveted = false;
    bool procCheckSecond = false;
    bool procCheckSecondActiveted = false;
    uint64 casterGUID = GetGUID();
    int32 spellProcId = procSpell ? procSpell->Id : -1;
    uint32 procPowerType = procSpell ? procSpell->PowerType : 0;
    uint32 procDmgClass = procSpell ? procSpell->DmgClass : 0;
    uint32 Attributes = procSpell ? procSpell->Attributes : 0;
    uint32 AllEffectsMechanicMask = procSpell ? procSpell->GetAllEffectsMechanicMask() : 0;
    uint32 SpellTypeMask = procSpell ? procSpell->GetSpellTypeMask() : 1;
    uint32 NeedsComboPoints = procSpell ? procSpell->NeedsComboPoints() : 0;
    int32 duration = procSpell ? procSpell->GetDuration() : 0;
    int32 specCheckid = ToPlayer() ? ToPlayer()->GetSpecializationId(ToPlayer()->GetActiveSpec()) : 0;
    int32 deathstateMask = victim ? (1 << victim->getDeathState()) : 0;

    sLog->outDebug(LOG_FILTER_PROC, "SpellProcCheck: spellProto->Id %i, effect %i, spellProcId %i, procPowerType %i, procDmgClass %i, AllEffectsMechanicMask %i, specCheckid %i, SpellTypeMask %i duration %i",
    spellProto->Id, effect, spellProcId, procPowerType, procDmgClass, AllEffectsMechanicMask, specCheckid, SpellTypeMask, duration);

    if (std::vector<SpellPrcoCheck> const* spellCheck = sSpellMgr->GetSpellPrcoCheck(spellProto->Id))
    {
        for (std::vector<SpellPrcoCheck>::const_iterator itr = spellCheck->begin(); itr != spellCheck->end(); ++itr)
        {
            if (!(itr->effectmask & (1<<effect)))
                continue;
            Unit* _checkTarget = this;

            if(itr->target == 1 && victim)
                _checkTarget = victim;
            if(itr->target == 3 && ToPlayer()) //get target owner pet
                if (Pet* pet = ToPlayer()->GetPet())
                    _checkTarget = (Unit*)pet;
            if(itr->target == 4 && victim && victim->ToPlayer()) //get target pet
                if (Pet* pet = victim->ToPlayer()->GetPet())
                    _checkTarget = (Unit*)pet;

            //if this spell exist not proc
            if (itr->checkspell < 0)
            {
                procCheckActiveted = true;
                if (-(itr->checkspell) == spellProcId)
                {
                    if(itr->hastalent != 0 || itr->specId != 0 || itr->spellAttr0 != 0 || itr->targetTypeMask != 0 || itr->perchp != 0 || itr->fromlevel != 0 || itr->mechanicMask != 0 || itr->combopoints != 0)
                    {
                        if(itr->hastalent > 0 && _checkTarget->HasAura(itr->hastalent, casterGUID))
                        {
                            procCheck = true;
                            break;
                        }
                        else if(itr->hastalent < 0 && !_checkTarget->HasAura(-(itr->hastalent), casterGUID))
                        {
                            procCheck = true;
                            break;
                        }
                        else if(itr->specId > 0 && itr->specId != specCheckid)
                        {
                            procCheck = true;
                            break;
                        }
                        else if(itr->specId < 0 && itr->specId == specCheckid)
                        {
                            procCheck = true;
                            break;
                        }
                        else if(itr->spellAttr0 > 0 && !(Attributes & itr->spellAttr0))
                        {
                            procCheck = true;
                            break;
                        }
                        else if(itr->spellAttr0 < 0 && (Attributes & abs(itr->spellAttr0)))
                        {
                            procCheck = true;
                            break;
                        }
                        else if(itr->targetTypeMask != 0 && !(itr->targetTypeMask & (1 << _checkTarget->GetTypeId())))
                        {
                            procCheck = true;
                            break;
                        }
                        if(itr->mechanicMask != 0 && !(AllEffectsMechanicMask & itr->mechanicMask))
                        {
                            procCheck = true;
                            break;
                        }
                        if(itr->combopoints != 0 && !NeedsComboPoints)
                        {
                            procCheck = true;
                            break;
                        }
                        if(itr->spelltypeMask != 0 && !(SpellTypeMask & itr->spelltypeMask))
                        {
                            procCheck = true;
                            break;
                        }
                        if(itr->deathstateMask != 0 && !(deathstateMask & itr->deathstateMask))
                        {
                            procCheck = true;
                            break;
                        }
                        if(itr->fromlevel > 0 && _checkTarget->getLevel() >= itr->fromlevel)
                        {
                            procCheck = true;
                            break;
                        }
                        else if(itr->fromlevel < 0 && _checkTarget->getLevel() < abs(itr->fromlevel))
                        {
                            procCheck = true;
                            break;
                        }
                        if(itr->perchp > 0 && _checkTarget->GetHealthPct() >= itr->perchp)
                        {
                            procCheck = true;
                            break;
                        }
                        else if(itr->perchp < 0 && _checkTarget->GetHealthPct() < abs(itr->perchp))
                        {
                            procCheck = true;
                            break;
                        }
                        if(itr->hasDuration > 0 && duration < 0)
                        {
                            procCheck = true;
                            break;
                        }
                        else if(itr->hasDuration < 0 && duration > 0)
                        {
                            procCheck = true;
                            break;
                        }
                        if(itr->chance != 0 && !roll_chance_i(itr->chance) && procCheck)
                        {
                            procCheck = false;
                            continue;
                        }
                    }
                    else if(itr->chance != 0 && roll_chance_i(itr->chance))
                        procCheck = false;
                    else
                        procCheck = true;
                    break;
                }
            }
            //if this spell not exist not proc
            else if (itr->checkspell == spellProcId)
            {
                procCheckActiveted = true;
                if(itr->hastalent > 0 && !_checkTarget->HasAura(itr->hastalent, casterGUID))
                {
                    procCheck = true;
                    continue;
                }
                else if(itr->hastalent < 0 && _checkTarget->HasAura(-(itr->hastalent), casterGUID))
                {
                    procCheck = true;
                    continue;
                }
                if(itr->chance != 0 && !roll_chance_i(itr->chance))
                {
                    procCheck = true;
                    continue;
                }
                if(itr->specId > 0 && itr->specId != specCheckid)
                {
                    procCheck = true;
                    continue;
                }
                else if(itr->specId < 0 && itr->specId == specCheckid)
                {
                    procCheck = true;
                    continue;
                }
                if(itr->spellAttr0 > 0 && !(Attributes & itr->spellAttr0))
                {
                    procCheck = true;
                    continue;
                }
                else if(itr->spellAttr0 < 0 && (Attributes & abs(itr->spellAttr0)))
                {
                    procCheck = true;
                    continue;
                }
                if(itr->targetTypeMask != 0 && !(itr->targetTypeMask & (1 << _checkTarget->GetTypeId())))
                {
                    procCheck = true;
                    continue;
                }
                if(itr->mechanicMask != 0 && !(AllEffectsMechanicMask & itr->mechanicMask))
                {
                    procCheck = true;
                    continue;
                }
                if(itr->combopoints != 0 && !NeedsComboPoints)
                {
                    procCheck = true;
                    continue;
                }
                if(itr->spelltypeMask != 0 && !(SpellTypeMask & itr->spelltypeMask))
                {
                    procCheck = true;
                    continue;
                }
                if(itr->deathstateMask != 0 && !(deathstateMask & itr->deathstateMask))
                {
                    procCheck = true;
                    continue;
                }
                if(itr->fromlevel > 0 && _checkTarget->getLevel() < itr->fromlevel)
                {
                    procCheck = true;
                    continue;
                }
                else if(itr->fromlevel < 0 && _checkTarget->getLevel() > abs(itr->fromlevel))
                {
                    procCheck = true;
                    continue;
                }
                if(itr->perchp > 0 && _checkTarget->GetHealthPct() < itr->perchp)
                {
                    procCheck = true;
                    continue;
                }
                else if(itr->perchp < 0 && _checkTarget->GetHealthPct() > abs(itr->perchp))
                {
                    procCheck = true;
                    continue;
                }
                if(itr->hasDuration > 0 && duration < 0)
                {
                    procCheck = true;
                    continue;
                }
                else if(itr->hasDuration < 0 && duration > 0)
                {
                    procCheck = true;
                    continue;
                }
                procCheck = false;
                break;
            }
            //other check
            else if (itr->checkspell == 0)
            {
                procCheckSecondActiveted = true;
                if(itr->hastalent != 0)
                {
                    if(itr->hastalent > 0 && !_checkTarget->HasAura(itr->hastalent, casterGUID))
                    {
                        procCheckSecond = true;
                        continue;
                    }
                    else if(itr->hastalent < 0 && _checkTarget->HasAura(-(itr->hastalent), casterGUID))
                    {
                        procCheckSecond = true;
                        continue;
                    }
                }

                if(itr->specId > 0 && itr->specId != specCheckid)
                {
                    procCheckSecond = true;
                    continue;
                }
                else if(itr->specId < 0 && itr->specId == specCheckid)
                {
                    procCheckSecond = true;
                    continue;
                }
                if(procSpell && itr->spellAttr0 > 0 && !(procSpell->Attributes & itr->spellAttr0))
                {
                    procCheckSecond = true;
                    continue;
                }
                else if(procSpell && itr->spellAttr0 < 0 && (procSpell->Attributes & abs(itr->spellAttr0)))
                {
                    procCheckSecond = true;
                    continue;
                }
                if(itr->targetTypeMask != 0 && !(itr->targetTypeMask & (1 << _checkTarget->GetTypeId())))
                {
                    procCheckSecond = true;
                    continue;
                }
                if(itr->mechanicMask != 0 && !(AllEffectsMechanicMask & itr->mechanicMask))
                {
                    procCheckSecond = true;
                    continue;
                }
                if(itr->combopoints != 0 && !NeedsComboPoints)
                {
                    procCheckSecond = true;
                    continue;
                }
                if(itr->spelltypeMask != 0 && !(SpellTypeMask & itr->spelltypeMask))
                {
                    procCheckSecond = true;
                    continue;
                }
                if(itr->deathstateMask != 0 && !(deathstateMask & itr->deathstateMask))
                {
                    procCheckSecond = true;
                    continue;
                }
                if(itr->fromlevel > 0 && _checkTarget->getLevel() < itr->fromlevel)
                {
                    procCheckSecond = true;
                    continue;
                }
                else if(itr->fromlevel < 0 && _checkTarget->getLevel() >= abs(itr->fromlevel))
                {
                    procCheckSecond = true;
                    continue;
                }
                if(itr->perchp > 0 && _checkTarget->GetHealthPct() < itr->perchp)
                {
                    procCheckSecond = true;
                    continue;
                }
                else if(itr->perchp < 0 && _checkTarget->GetHealthPct() >= abs(itr->perchp))
                {
                    procCheckSecond = true;
                    continue;
                }
                if(itr->hasDuration > 0 && duration < 0)
                {
                    procCheckSecond = true;
                    continue;
                }
                else if(itr->hasDuration < 0 && duration > 0)
                {
                    procCheckSecond = true;
                    continue;
                }

                if(itr->powertype != -1 && itr->dmgclass != -1)
                {
                    if(itr->powertype != procPowerType || itr->dmgclass != procDmgClass)
                    {
                        procCheckSecond = true;
                        continue;
                    }
                }
                else if(itr->dmgclass != -1 && itr->dmgclass != procDmgClass)
                {
                    procCheckSecond = true;
                    continue;
                }
                else if(itr->powertype != -1 && itr->powertype != procPowerType)
                {
                    procCheckSecond = true;
                    continue;
                }
                if(itr->chance != 0 && !roll_chance_i(itr->chance))
                {
                    procCheckSecond = true;
                    break;
                }
                procCheckSecond = false;
                break;
            }
            else
                procCheck = true;
        }
    }
    //if check true false proc
    if(procCheck && !procCheckSecondActiveted)
    {
        sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "SpellProcCheck: spellProto->Id %i, effect %i, spellProcId %i procCheck", spellProto->Id, effect, spellProcId);
        return false;
    }
    if(procCheckSecond && !procCheckActiveted)
    {
        sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "SpellProcCheck: spellProto->Id %i, effect %i, spellProcId %i procCheckSecond", spellProto->Id, effect, spellProcId);
        return false;
    }
    if(procCheck && procCheckSecond)
    {
        sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "SpellProcCheck: spellProto->Id %i, effect %i, spellProcId %i procCheckSecond && procCheck", spellProto->Id, effect, spellProcId, procCheckSecond);
        return false;
    }

    return true;
}

void Unit::CalculateFromDummy(Unit* victim, float &amount, SpellInfo const* spellProto, uint32 mask, bool damage) const
{
    sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "Unit::CalculateFromDummy start GetId %i, amount %f, mask %i", spellProto->Id, amount, mask);

    if (std::vector<SpellAuraDummy> const* spellAuraDummy = sSpellMgr->GetSpellAuraDummy(spellProto->Id))
    {
        for (std::vector<SpellAuraDummy>::const_iterator itr = spellAuraDummy->begin(); itr != spellAuraDummy->end(); ++itr)
        {
            sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "Unit::CalculateFromDummy spellDummyId %i, effectmask %i, option %i, aura %i",
            itr->spellDummyId, itr->effectmask, itr->option, itr->aura);

            if (!(itr->effectmask & mask))
                continue;

            Unit* _caster = const_cast<Unit*>(this);
            Unit* _targetAura = const_cast<Unit*>(this);
            Unit* _target = victim;
            bool check = false;

            if(itr->caster == 1 && victim) //get caster as target
                _caster = victim;

            if(itr->targetaura == 2 && victim) //get target aura
                _targetAura = victim;

            if(itr->caster == 3) //get caster owner
                if (Unit* owner = GetAnyOwner())
                    _caster = owner;

            switch (itr->option)
            {
                case SPELL_DUMMY_CRIT_RESET: //5
                {
                    if(damage)
                        continue;
                    if(itr->aura > 0 && !_targetAura->HasAura(itr->aura))
                        continue;
                    if(itr->aura < 0 && _targetAura->HasAura(abs(itr->aura)))
                        continue;

                    if(itr->spellDummyId > 0 && !_caster->HasAura(itr->spellDummyId))
                    {
                        amount = 0.0f;
                        check = true;
                    }
                    if(itr->spellDummyId < 0 && _caster->HasAura(abs(itr->spellDummyId)))
                    {
                        amount = 0.0f;
                        check = true;
                    }
                    break;
                }
                case SPELL_DUMMY_CRIT_ADD_PERC: //6
                {
                    if(damage)
                        continue;
                    if(itr->aura > 0 && !_targetAura->HasAura(itr->aura))
                        continue;
                    if(itr->aura < 0 && _targetAura->HasAura(abs(itr->aura)))
                        continue;

                    if(itr->spellDummyId > 0 && _caster->HasAura(itr->spellDummyId))
                    {
                        if(AuraEffect const* dummyEff = _caster->GetAuraEffect(itr->spellDummyId, itr->effectDummy))
                        {
                            float bp = itr->custombp;
                            if(!bp)
                                bp = dummyEff->GetAmount();
                            amount += CalculatePct(amount, bp);
                            check = true;
                        }
                    }
                    if(itr->spellDummyId < 0 && _caster->HasAura(abs(itr->spellDummyId)))
                    {
                        if(AuraEffect const* dummyEff = _caster->GetAuraEffect(abs(itr->spellDummyId), itr->effectDummy))
                        {
                            float bp = itr->custombp;
                            if(!bp)
                                bp = dummyEff->GetAmount();
                            amount -= CalculatePct(amount, bp);
                            check = true;
                        }
                    }
                    break;
                }
                case SPELL_DUMMY_CRIT_ADD_VALUE: //7
                {
                    if(damage)
                        continue;
                    if(itr->aura > 0 && !_targetAura->HasAura(itr->aura))
                        continue;
                    if(itr->aura < 0 && _targetAura->HasAura(abs(itr->aura)))
                        continue;

                    if(itr->spellDummyId > 0 && _caster->HasAura(itr->spellDummyId))
                    {
                        if(AuraEffect const* dummyEff = _caster->GetAuraEffect(itr->spellDummyId, itr->effectDummy))
                        {
                            float bp = itr->custombp;
                            if(!bp)
                                bp = dummyEff->GetAmount();
                            amount += bp;
                            check = true;
                        }
                    }
                    if(itr->spellDummyId < 0 && _caster->HasAura(abs(itr->spellDummyId)))
                    {
                        if(AuraEffect const* dummyEff = _caster->GetAuraEffect(abs(itr->spellDummyId), itr->effectDummy))
                        {
                            float bp = itr->custombp;
                            if(!bp)
                                bp = dummyEff->GetAmount();
                            amount -= bp;
                            check = true;
                        }
                    }
                    break;
                }
                case SPELL_DUMMY_DAMAGE_ADD_PERC: //9
                {
                    if(!damage)
                        continue;
                    if(itr->aura > 0 && !_targetAura->HasAura(itr->aura))
                        continue;
                    if(itr->aura < 0 && _targetAura->HasAura(abs(itr->aura)))
                        continue;

                    if(itr->spellDummyId > 0 && _caster->HasAura(itr->spellDummyId))
                    {
                        if(AuraEffect const* dummyEff = _caster->GetAuraEffect(itr->spellDummyId, itr->effectDummy))
                        {
                            float bp = itr->custombp;
                            if(!bp)
                                bp = dummyEff->GetAmount();
                            amount += CalculatePct(amount, bp);
                            check = true;
                        }
                    }
                    if(itr->spellDummyId < 0 && _caster->HasAura(abs(itr->spellDummyId)))
                    {
                        if(AuraEffect const* dummyEff = _caster->GetAuraEffect(abs(itr->spellDummyId), itr->effectDummy))
                        {
                            float bp = itr->custombp;
                            if(!bp)
                                bp = dummyEff->GetAmount();
                            amount -= CalculatePct(amount, bp);
                            check = true;
                        }
                    }
                    break;
                }
                case SPELL_DUMMY_DAMAGE_ADD_VALUE: //10
                {
                    if(!damage)
                        continue;
                    if(itr->aura > 0 && !_targetAura->HasAura(itr->aura))
                        continue;
                    if(itr->aura < 0 && _targetAura->HasAura(abs(itr->aura)))
                        continue;

                    if(itr->spellDummyId > 0 && _caster->HasAura(itr->spellDummyId))
                    {
                        if(AuraEffect const* dummyEff = _caster->GetAuraEffect(itr->spellDummyId, itr->effectDummy))
                        {
                            float bp = itr->custombp;
                            if(!bp)
                                bp = dummyEff->GetAmount();
                            amount += bp;
                            check = true;
                        }
                    }
                    if(itr->spellDummyId < 0 && _caster->HasAura(abs(itr->spellDummyId)))
                    {
                        if(AuraEffect const* dummyEff = _caster->GetAuraEffect(abs(itr->spellDummyId), itr->effectDummy))
                        {
                            float bp = itr->custombp;
                            if(!bp)
                                bp = dummyEff->GetAmount();
                            amount -= bp;
                            check = true;
                        }
                    }
                    break;
                }
            }
            if(check && itr->removeAura)
                _caster->RemoveAurasDueToSpell(itr->removeAura);
        }
    }
}

void Unit::CalculateCastTimeFromDummy(int32& castTime, SpellInfo const* spellProto)
{
    if (std::vector<SpellAuraDummy> const* spellAuraDummy = sSpellMgr->GetSpellAuraDummy(spellProto->Id))
    {
        for (std::vector<SpellAuraDummy>::const_iterator itr = spellAuraDummy->begin(); itr != spellAuraDummy->end(); ++itr)
        {
            Unit* _caster = this;
            Unit* _targetAura = this;
            bool check = false;

            if(itr->caster == 2 && _caster->ToPlayer()) //get target pet
            {
                if (Pet* pet = _caster->ToPlayer()->GetPet())
                    _caster = (Unit*)pet;
            }
            if(itr->caster == 3) //get target owner
            {
                if (Unit* owner = _caster->GetOwner())
                    _caster = owner;
            }

            if(itr->targetaura == 1 && _caster->ToPlayer()) //get target pet
            {
                if (Pet* pet = _caster->ToPlayer()->GetPet())
                    _targetAura = (Unit*)pet;
            }
            if(itr->targetaura == 2) //get target owner
            {
                if (Unit* owner = _caster->GetOwner())
                    _targetAura = owner;
            }

            switch (itr->option)
            {
                case SPELL_DUMMY_CASTTIME_ADD_PERC: //13
                {
                    if(itr->aura > 0 && !_targetAura->HasAura(itr->aura))
                        continue;
                    if(itr->aura < 0 && _targetAura->HasAura(abs(itr->aura)))
                        continue;

                    if(itr->spellDummyId > 0 && _caster->HasAura(itr->spellDummyId))
                    {
                        if(SpellInfo const* dummyInfo = sSpellMgr->GetSpellInfo(itr->spellDummyId))
                        {
                            int32 bp = itr->custombp;
                            if(!bp)
                                bp = dummyInfo->Effects[itr->effectDummy].BasePoints;

                            castTime += CalculatePct(castTime, bp);
                            check = true;
                        }
                    }
                    if(itr->spellDummyId < 0 && _caster->HasAura(abs(itr->spellDummyId)))
                    {
                        if(SpellInfo const* dummyInfo = sSpellMgr->GetSpellInfo(abs(itr->spellDummyId)))
                        {
                            int32 bp = itr->custombp;
                            if(!bp)
                                bp = dummyInfo->Effects[itr->effectDummy].BasePoints;
                            castTime -= CalculatePct(castTime, bp);
                            check = true;
                        }
                    }
                    break;
                }
                case SPELL_DUMMY_CASTTIME_ADD_VALUE: //14
                {
                    if(itr->aura > 0 && !_targetAura->HasAura(itr->aura))
                        continue;
                    if(itr->aura < 0 && _targetAura->HasAura(abs(itr->aura)))
                        continue;

                    if(itr->spellDummyId > 0 && _caster->HasAura(itr->spellDummyId))
                    {
                        if(SpellInfo const* dummyInfo = sSpellMgr->GetSpellInfo(itr->spellDummyId))
                        {
                            int32 bp = itr->custombp;
                            if(!bp)
                                bp = dummyInfo->Effects[itr->effectDummy].BasePoints;
                            castTime += bp;
                            check = true;
                        }
                    }
                    if(itr->spellDummyId < 0 && _caster->HasAura(abs(itr->spellDummyId)))
                    {
                        if(SpellInfo const* dummyInfo = sSpellMgr->GetSpellInfo(abs(itr->spellDummyId)))
                        {
                            int32 bp = itr->custombp;
                            if(!bp)
                                bp = dummyInfo->Effects[itr->effectDummy].BasePoints;
                            castTime -= bp;
                            check = true;
                        }
                    }
                    break;
                }
            }
            if(check && itr->removeAura)
                _caster->RemoveAurasDueToSpell(itr->removeAura);
        }
    }
}

bool Unit::IsTriggeredAtSpellProcEvent(Unit* victim, SpellInfo const* spellProto, SpellInfo const* procSpell, uint32 procFlag, uint32 procExtra, WeaponAttackType attType, bool isVictim, bool active, SpellProcEventEntry const* & spellProcEvent, uint8 effect)
{
    if(!spellProto)
        return false;

    // let the aura be handled by new proc system if it has new entry
    //if (sSpellMgr->GetSpellProcEntry(spellProto->Id))
        //return false;

    // Get proc Event Entry
    //spellProcEvent = sSpellMgr->GetSpellProcEvent(spellProto->Id);
    if (std::vector<SpellProcEventEntry> const* spellproc = sSpellMgr->GetSpellProcEvent(spellProto->Id))
    {
        for (std::vector<SpellProcEventEntry>::const_iterator itr = spellproc->begin(); itr != spellproc->end(); ++itr)
        {
            if (itr->effectMask & (1 << effect))
            {
                spellProcEvent = &(*itr);
                break;
            }
        }
    }

    // Get EventProcFlag
    uint32 EventProcFlag;
    if (spellProcEvent && spellProcEvent->procFlags) // if exist get custom spellProcEvent->procFlags
        EventProcFlag = spellProcEvent->procFlags;
    else
        EventProcFlag = spellProto->ProcFlags;       // else get from spell proto
    // Continue if no trigger exist
    if (!EventProcFlag)
        return false;

    // Additional checks for triggered spells (ignore trap casts)
    if (procExtra & PROC_EX_INTERNAL_TRIGGERED && !(procFlag & PROC_FLAG_DONE_TRAP_ACTIVATION))
    {
        if (!(spellProto->AttributesEx3 & SPELL_ATTR3_CAN_PROC_WITH_TRIGGERED))
            return false;
        else if(spellProto->ProcFlags&(PROC_FLAG_DONE_SPELL_NONE_DMG_CLASS_NEG|PROC_FLAG_DONE_SPELL_NONE_DMG_CLASS_POS))// for triggered spell set to active
            active = true;
    }

    bool alredyCheck = false;
    //Spell can proc from HOT
    if (procFlag & PROC_FLAG_DONE_PERIODIC && EventProcFlag & PROC_FLAG_DONE_PERIODIC)
    {
        switch(spellProto->Id)
        {
            case 48544: // Revitalize
            case 70664: // Item - Druid T10 Restoration 4P Bonus (Rejuvenation)
            case 99013: // Item - Druid T12 Restoration 2P Bonus
            case 99190: // Item - Shaman T12 Restoration 2P Bonus
            case 144869: // Item - Druid T16 Restoration 2P Bonus
            case 145449: // Item - Monk T16 Mistweaver 4P Bonus
                if (procExtra & PROC_EX_INTERNAL_HOT)
                    alredyCheck = true;
                break;
        }
    }
    //Spell can`t proc from HOT
    if (procFlag & PROC_FLAG_TAKEN_PERIODIC && EventProcFlag & PROC_FLAG_TAKEN_PERIODIC)
    {
        switch(spellProto->Id)
        {
            case 33076: // Prayer of Mending
            case 41635: // Prayer of Mending
                if (procExtra & PROC_EX_INTERNAL_HOT)
                    return false;
                break;
        }
    }

    // Aura added by spell can`t trigger from self (prevent drop charges/do triggers)
    // But except periodic and kill triggers (can triggered from self)
    if (procSpell && procSpell->Id == spellProto->Id && !(spellProto->ProcFlags & (PROC_FLAG_KILL)))
        return false;

    if (spellProto->AttributesEx4 & SPELL_ATTR4_UNK19)
        if (procFlag & PROC_FLAG_TAKEN_PERIODIC && EventProcFlag & PROC_FLAG_TAKEN_PERIODIC)
            if (procExtra & (PROC_EX_INTERNAL_HOT | PROC_EX_ABSORB))
                return false;

    // Check spellProcEvent data requirements
    if (!alredyCheck && !sSpellMgr->IsSpellProcEventCanTriggeredBy(spellProcEvent, EventProcFlag, procSpell, procFlag, procExtra, active))
    {
        //sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "IsTriggeredAtSpellProcEvent: false procSpell %i, EventProcFlag %i, active %i, procExtra %i, isVictim %i procFlag %u Id %u", procSpell ? procSpell->Id : 0, EventProcFlag, active, procExtra, isVictim, procFlag, spellProto->Id);
        return false;
    }

    // In most cases req get honor or XP from kill
    if (EventProcFlag & PROC_FLAG_KILL && GetTypeId() == TYPEID_PLAYER)
    {
        bool allow = false;

        if (victim)
            allow = ToPlayer()->isHonorOrXPTarget(victim);

        // Shadow Word: Death - can trigger from every kill
        //if (aura->GetId() == 32409)
            //allow = true;
        if (!allow)
            return false;
    }

    // Check if current equipment allows aura to proc
    if (!isVictim && GetTypeId() == TYPEID_PLAYER)
    {
        Player* player = ToPlayer();
        if (spellProto->EquippedItemClass == ITEM_CLASS_WEAPON)
        {
            Item* item = NULL;
            if (attType == BASE_ATTACK)
                item = player->GetUseableItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_MAINHAND);
            else if (attType == OFF_ATTACK)
                item = player->GetUseableItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_OFFHAND);

            if (player->IsInFeralForm())
                return false;

            if (!item || item->IsBroken() || item->GetTemplate()->Class != ITEM_CLASS_WEAPON || !((1<<item->GetTemplate()->SubClass) & spellProto->EquippedItemSubClassMask))
                return false;
        }
        else if (spellProto->EquippedItemClass == ITEM_CLASS_ARMOR)
        {
            // Check if player is wearing shield
            Item* item = player->GetUseableItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_OFFHAND);
            if (!item || item->IsBroken() || item->GetTemplate()->Class != ITEM_CLASS_ARMOR || !((1<<item->GetTemplate()->SubClass) & spellProto->EquippedItemSubClassMask))
                return false;
        }
    }
    // Get chance from spell
    float chance = float(spellProto->ProcChance);
    // If in spellProcEvent exist custom chance, chance = spellProcEvent->customChance;
    if (spellProcEvent && spellProcEvent->customChance)
        chance = spellProcEvent->customChance;
    // If PPM exist calculate chance from PPM
    if (spellProcEvent && spellProcEvent->ppmRate != 0)
    {
        if (!isVictim)
        {
            uint32 WeaponSpeed = GetAttackTime(attType);
            chance = GetPPMProcChance(WeaponSpeed, spellProcEvent->ppmRate, spellProto);
        }
        else
        {
            uint32 WeaponSpeed = victim->GetAttackTime(attType);
            chance = victim->GetPPMProcChance(WeaponSpeed, spellProcEvent->ppmRate, spellProto);
        }
    }
    if (spellProto->procPerMinId)
    {
        float spellPPM = 0.0f;
        if(SpellProcsPerMinuteEntry const* procPPM = sSpellProcsPerMinuteStore.LookupEntry(spellProto->procPerMinId))
        {
            spellPPM = procPPM->ppmRate;

            if(Player* player = ToPlayer())
            {
                uint32 specId = player->GetSpecializationId(player->GetActiveSpec());
                if(std::list<uint32> const* modList = GetSpellProcsPerMinuteModList(spellProto->procPerMinId))
                    for (std::list<uint32>::const_iterator itr = modList->begin(); itr != modList->end(); ++itr)
                    {
                        if(SpellProcsPerMinuteModEntry const* procPPMmod = sSpellProcsPerMinuteModStore.LookupEntry((*itr)))
                            if(procPPMmod->specId == specId)
                                spellPPM *= procPPMmod->ppmRateMod + 1;
                    }

                double cooldown = player->GetPPPMSpellCooldownDelay(spellProto->Id); //base cap
                bool procked = player->GetRPPMProcChance(cooldown, spellPPM, spellProto);
                if(procked)
                {
                    player->SetLastSuccessfulProc(spellProto->Id, getPreciseTime());
                    player->AddRPPMSpellCooldown(spellProto->Id, 0, getPreciseTime() + cooldown);
                }
                return procked;
            }
        }
        return roll_chance_f(chance);
    }
    else
    {
        // Apply chance modifer aura
        if (Player* modOwner = GetSpellModOwner())
            modOwner->ApplySpellMod(spellProto->Id, SPELLMOD_CHANCE_OF_SUCCESS, chance);
        return roll_chance_f(chance);
    }
}

bool Unit::HandleAuraRaidProcFromChargeWithValue(AuraEffect* triggeredByAura)
{
    // aura can be deleted at casts
    SpellInfo const* spellProto = triggeredByAura->GetSpellInfo();
    int32 heal = triggeredByAura->GetAmount();
    uint64 caster_guid = triggeredByAura->GetCasterGUID();

    // Currently only Prayer of Mending
    if (!(spellProto->SpellFamilyName == SPELLFAMILY_PRIEST && spellProto->SpellFamilyFlags[1] & 0x20))
    {
        sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "Unit::HandleAuraRaidProcFromChargeWithValue, received not handled spell: %u", spellProto->Id);
        return false;
    }

    // jumps
    int32 jumps = triggeredByAura->GetBase()->GetCharges()-1;

    // current aura expire
    triggeredByAura->GetBase()->SetCharges(1);             // will removed at next charges decrease

    // next target selection
    if (jumps > 0)
    {
        if (Unit* caster = triggeredByAura->GetCaster())
        {
            float radius = triggeredByAura->GetSpellInfo()->GetEffect(triggeredByAura->GetEffIndex(), GetSpawnMode())->CalcRadius(caster);

            if (Unit* target = GetNextRandomRaidMemberOrPet(radius))
            {
                CastCustomSpell(target, spellProto->Id, &heal, NULL, NULL, true, NULL, triggeredByAura, caster_guid);
                Aura* aura = target->GetAura(spellProto->Id, caster->GetGUID());
                if (aura != NULL)
                    aura->SetCharges(jumps);
            }
            if(caster->HasAura(55685) && jumps == 3) // Glyph of Prayer of Mending
                heal *= 1.6f;
            if(caster->HasAura(109186) && roll_chance_i(15)) // hack for From Darkness, Comes Light
                CastSpell(this, 114255, true);
        }
    }

    // heal
    CastCustomSpell(this, 33110, &heal, NULL, NULL, true, NULL, NULL, caster_guid);
    return true;

}

bool Unit::HandleAuraRaidProcFromCharge(AuraEffect* triggeredByAura)
{
    // aura can be deleted at casts
    SpellInfo const* spellProto = triggeredByAura->GetSpellInfo();

    uint32 damageSpellId;
    switch (spellProto->Id)
    {
        case 57949:            // shiver
            damageSpellId = 57952;
            //animationSpellId = 57951; dummy effects for jump spell have unknown use (see also 41637)
            break;
        case 59978:            // shiver
            damageSpellId = 59979;
            break;
        case 43593:            // Cold Stare
            damageSpellId = 43594;
            break;
        default:
            sLog->outError(LOG_FILTER_UNITS, "Unit::HandleAuraRaidProcFromCharge, received unhandled spell: %u", spellProto->Id);
            return false;
    }

    uint64 caster_guid = triggeredByAura->GetCasterGUID();

    // jumps
    int32 jumps = triggeredByAura->GetBase()->GetCharges()-1;

    // current aura expire
    triggeredByAura->GetBase()->SetCharges(1);             // will removed at next charges decrease

    // next target selection
    if (jumps > 0)
    {
        if (Unit* caster = triggeredByAura->GetCaster())
        {
            float radius = triggeredByAura->GetSpellInfo()->GetEffect(triggeredByAura->GetEffIndex(), GetSpawnMode())->CalcRadius(caster);
            if (Unit* target= GetNextRandomRaidMemberOrPet(radius))
            {
                CastSpell(target, spellProto, true, NULL, triggeredByAura, caster_guid);
                Aura* aura = target->GetAura(spellProto->Id, caster->GetGUID());
                if (aura != NULL)
                    aura->SetCharges(jumps);
            }
        }
    }

    CastSpell(this, damageSpellId, true, NULL, triggeredByAura, caster_guid);

    return true;
}

void Unit::SendDurabilityLoss(Player* receiver, uint32 percent)
{
    WorldPacket data(SMSG_DURABILITY_DAMAGE_DEATH, 4);
    data << uint32(percent);
    receiver->GetSession()->SendPacket(&data);
}

//Check for 5.4.1
void Unit::SetAiAnimKit(uint32 id)
{
    ObjectGuid guidd(GetGUID());

    WorldPacket data(SMSG_SET_AI_ANIM_KIT, 7 + 2);
    data.WriteGuidMask<2, 5, 7, 4, 0, 1, 6, 3>(guidd);
    data.WriteGuidBytes<3, 0, 7, 6, 5, 1, 4>(guidd);
    data << uint16(id);
    data.WriteGuidBytes<2>(guidd);
    SendMessageToSet(&data, true);
}

void Unit::PlayOneShotAnimKit(uint32 id)
{
    ObjectGuid guidd(GetGUID());

    WorldPacket data(SMSG_PLAY_ONE_SHOT_ANIM_KIT, 7 + 2);
    data.WriteGuidMask<4, 3, 0, 7, 1, 6, 5, 2>(guidd);
    data.WriteGuidBytes<4, 6, 5, 1>(guidd);
    data << uint16(id);
    data.WriteGuidBytes<0, 7, 3, 2>(guidd);
    SendMessageToSet(&data, true);
}


void Unit::Kill(Unit* victim, bool durabilityLoss, SpellInfo const* spellProto)
{
    // Prevent killing unit twice (and giving reward from kill twice)
    if (!victim->GetHealth() || m_IsInKillingProcess)
        return;

    m_IsInKillingProcess = true;

    // find player: owner of controlled `this` or `this` itself maybe
    Player* player = GetCharmerOrOwnerPlayerOrPlayerItself();
    Creature* creature = victim->ToCreature();

    bool isRewardAllowed = true;
    if (creature)
    {
        isRewardAllowed = creature->IsDamageEnoughForLootingAndReward();
        if (!isRewardAllowed)
            creature->SetLootRecipient(NULL);
    }

    if (isRewardAllowed && creature && creature->GetLootRecipient())
        player = creature->GetLootRecipient();

    // Reward player, his pets, and group/raid members
    // call kill spell proc event (before real die and combat stop to triggering auras removed at death/combat stop)
    if (isRewardAllowed && player && player != victim)
    {
        ObjectGuid guid = player->GetGUID();
        ObjectGuid vGuid = victim->GetGUID();

        //! 5.4.1
        WorldPacket data(SMSG_PARTYKILLLOG, (8+8)); // send event PARTY_KILL
        data.WriteBit(0);
        data.WriteGuidMask<1>(guid);
        data.WriteGuidMask<4>(vGuid);
        data.WriteGuidMask<6, 0, 7, 3, 2, 4>(guid);
        data.WriteGuidMask<5, 7, 0, 2, 6>(vGuid);
        data.WriteGuidMask<5>(guid);
        data.WriteGuidMask<3, 1>(vGuid);

        data.FlushBits();

        data.WriteGuidBytes<7>(guid);
        data.WriteGuidBytes<5>(vGuid);
        data.WriteGuidBytes<4>(guid);
        data.WriteGuidBytes<0>(vGuid);
        data.WriteGuidBytes<5>(guid);
        data.WriteGuidBytes<4, 2>(vGuid);
        data.WriteGuidBytes<1>(guid);
        data.WriteGuidBytes<6>(vGuid);
        data.WriteGuidBytes<6>(guid);
        data.WriteGuidBytes<3, 1>(vGuid);
        data.WriteGuidBytes<0, 3>(guid);
        data.WriteGuidBytes<7>(vGuid);
        data.WriteGuidBytes<2>(guid);

        Player* looter = player;

        if (Group* group = player->GetGroup())
        {
            group->BroadcastPacket(&data, group->GetMemberGroup(player->GetGUID()));

            if (creature)
            {
                group->UpdateLooterGuid(creature, true);
                if (group->GetLooterGuid())
                {
                    looter = ObjectAccessor::FindPlayer(group->GetLooterGuid());
                    if (looter)
                    {
                        creature->SetLootRecipient(looter);   // update creature loot recipient to the allowed looter.
                        group->SendLooter(creature, looter);
                    }
                    else
                        group->SendLooter(creature, NULL);
                }
                else
                    group->SendLooter(creature, NULL);

                group->UpdateLooterGuid(creature);
            }
        }
        else
        {
            player->SendDirectMessage(&data);
        }

        if (creature)
        {
            Map* map = creature->GetMap();
            if(creature->IsPersonalLoot())
                GeneratePersonalLoot(creature, looter);
            else if(!map->IsLfr()) //Don`t loot in LFG
            {
                Loot* loot = &creature->loot;
                if (creature->lootForPickPocketed)
                    creature->lootForPickPocketed = false;

                loot->clear();
                loot->objType = 1;
                if (uint32 lootid = creature->GetCreatureTemplate()->lootid)
                    loot->FillLoot(lootid, LootTemplates_Creature, looter, false, false, creature);

                loot->generateMoneyLoot(creature->GetCreatureTemplate()->mingold, creature->GetCreatureTemplate()->maxgold);
            }
        }

        player->RewardPlayerAndGroupAtKill(victim, false);
    }

    // Do KILL and KILLED procs. KILL proc is called only for the unit who landed the killing blow (and its owner - for pets and totems) regardless of who tapped the victim
    if (isPet() || isTotem())
        if (Unit* owner = GetOwner())
        {
            DamageInfo dmgInfoProc = DamageInfo(owner, victim, 0, spellProto ? spellProto : NULL, spellProto ? SpellSchoolMask(spellProto->SchoolMask) : SPELL_SCHOOL_MASK_NORMAL, SPELL_DIRECT_DAMAGE, 0);
            owner->ProcDamageAndSpell(victim, PROC_FLAG_KILL, PROC_FLAG_NONE, PROC_EX_NONE, &dmgInfoProc);
        }

    if (victim->GetCreatureType() != CREATURE_TYPE_CRITTER)
    {
        DamageInfo dmgInfoProc = DamageInfo(this, victim, 0, spellProto ? spellProto : NULL, spellProto ? SpellSchoolMask(spellProto->SchoolMask) : SPELL_SCHOOL_MASK_NORMAL, SPELL_DIRECT_DAMAGE, 0);
        ProcDamageAndSpell(victim, PROC_FLAG_KILL, PROC_FLAG_KILLED, PROC_EX_NONE, &dmgInfoProc, BASE_ATTACK, spellProto ? spellProto : NULL, NULL);
    }

    // Proc auras on death - must be before aura/combat remove
    DamageInfo dmgInfoProc = DamageInfo(this, victim, 0, spellProto ? spellProto : NULL, spellProto ? SpellSchoolMask(spellProto->SchoolMask) : SPELL_SCHOOL_MASK_NORMAL, SPELL_DIRECT_DAMAGE, 0);
    victim->ProcDamageAndSpell(NULL, PROC_FLAG_DEATH, PROC_FLAG_NONE, PROC_EX_NONE, &dmgInfoProc, BASE_ATTACK, spellProto ? spellProto : NULL);

    // update get killing blow achievements, must be done before setDeathState to be able to require auras on target
    // and before Spirit of Redemption as it also removes auras
    if (player)
        player->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_GET_KILLING_BLOWS, 1, 0, 0, victim);

    // if talent known but not triggered (check priest class for speedup check)
    bool spiritOfRedemption = false;
    if (victim->GetTypeId() == TYPEID_PLAYER && victim->getClass() == CLASS_PRIEST)
    {
        if(AuraEffect const* aurEff = victim->GetAuraEffect(20711, 0))
        {
            // save value before aura remove
            uint32 ressSpellId = victim->GetUInt32Value(PLAYER_SELF_RES_SPELL);
            if (!ressSpellId)
                ressSpellId = victim->ToPlayer()->GetResurrectionSpellId();
            // Remove all expected to remove at death auras (most important negative case like DoT or periodic triggers)
            victim->RemoveAllAurasOnDeath();
            // restore for use at real death
            victim->SetUInt32Value(PLAYER_SELF_RES_SPELL, ressSpellId);

            // FORM_SPIRITOFREDEMPTION and related auras
            victim->CastSpell(victim, 27827, true, NULL, aurEff);
            spiritOfRedemption = true;
        }
    }

    if (!spiritOfRedemption)
    {
        sLog->outDebug(LOG_FILTER_UNITS, "SET JUST_DIED");
        victim->setDeathState(JUST_DIED);
    }

    if (Creature* crt = victim->ToCreature())
        if (CreatureAI* ai = crt->AI())
            ai->ComonOnHome();

    // Inform pets (if any) when player kills target)
    // MUST come after victim->setDeathState(JUST_DIED); or pet next target
    // selection will get stuck on same target and break pet react state
    if (player)
    {
        Pet* pet = player->GetPet();
        if (pet && pet->isAlive() && pet->isControlled())
            pet->AI()->KilledUnit(victim);
    }

    // 10% durability loss on death
    // clean InHateListOf
    if (Player* plrVictim = victim->ToPlayer())
    {
        // remember victim PvP death for corpse type and corpse reclaim delay
        // at original death (not at SpiritOfRedemtionTalent timeout)
        plrVictim->SetPvPDeath(player != NULL);

        // only if not player and not controlled by player pet. And not at BG
        if ((durabilityLoss && !player && !victim->ToPlayer()->InBattleground()) || (player && sWorld->getBoolConfig(CONFIG_DURABILITY_LOSS_IN_PVP)))
        {
            double baseLoss = sWorld->getRate(RATE_DURABILITY_LOSS_ON_DEATH);
            sLog->outDebug(LOG_FILTER_UNITS, "We are dead, losing %u percent durability", baseLoss);
            // Durability loss is calculated more accurately again for each item in Player::DurabilityLoss
            plrVictim->DurabilityLossAll(baseLoss, false, true);
            // durability lost message
            SendDurabilityLoss(plrVictim, baseLoss);
        }
        // Call KilledUnit for creatures
        if (GetTypeId() == TYPEID_UNIT && IsAIEnabled)
            ToCreature()->AI()->KilledUnit(victim);

        // last damage from non duel opponent or opponent controlled creature
        if (plrVictim->duel)
        {
            plrVictim->duel->opponent->CombatStopWithPets(true);
            plrVictim->CombatStopWithPets(true);
            plrVictim->DuelComplete(DUEL_INTERRUPTED);
        }
    }
    else                                                // creature died
    {
        sLog->outDebug(LOG_FILTER_UNITS, "DealDamageNotPlayer");

        if (!creature->isPet())
        {
            creature->DeleteThreatList();
            CreatureTemplate const* cInfo = creature->GetCreatureTemplate();
            if (cInfo && (cInfo->lootid || cInfo->maxgold > 0))
                creature->SetFlag(OBJECT_FIELD_DYNAMIC_FLAGS, UNIT_DYNFLAG_LOOTABLE);
        }

        // Call KilledUnit for creatures, this needs to be called after the lootable flag is set
        if (GetTypeId() == TYPEID_UNIT && IsAIEnabled)
            ToCreature()->AI()->KilledUnit(victim);

        // Call creature just died function
        if (creature->IsAIEnabled)
            creature->AI()->JustDied(this);

        if (TempSummon* summon = creature->ToTempSummon())
            if (Unit* summoner = summon->GetSummoner())
                if (summoner->ToCreature() && summoner->IsAIEnabled)
                    summoner->ToCreature()->AI()->SummonedCreatureDies(creature, this);

        // Call instance script.
        if (InstanceScript* script = creature->GetInstanceScript())
            script->CreatureDies(creature, this);

        // Dungeon specific stuff, only applies to players killing creatures
        if (creature->GetInstanceId())
        {
            Map* instanceMap = creature->GetMap();
            Player* creditedPlayer = GetCharmerOrOwnerPlayerOrPlayerItself();
            // TODO: do instance binding anyway if the charmer/owner is offline

            if (instanceMap->IsDungeon() && creditedPlayer)
            {
                if (instanceMap->IsRaidOrHeroicDungeon())
                {
                    if (creature->GetCreatureTemplate()->flags_extra & CREATURE_FLAG_EXTRA_INSTANCE_BIND)
                        ((InstanceMap*)instanceMap)->PermBindAllPlayers(creditedPlayer);
                }
            }
        }
    }

    // outdoor pvp things, do these after setting the death state, else the player activity notify won't work... doh...
    // handle player kill only if not suicide (spirit of redemption for example)
    if (player && this != victim)
    {
        if (OutdoorPvP* pvp = player->GetOutdoorPvP())
            pvp->HandleKill(player, victim);

        if (Battlefield* bf = sBattlefieldMgr->GetBattlefieldToZoneId(player->GetZoneId()))
            bf->HandleKill(player, victim);
    }

    //if (victim->GetTypeId() == TYPEID_PLAYER)
    //    if (OutdoorPvP* pvp = victim->ToPlayer()->GetOutdoorPvP())
    //        pvp->HandlePlayerActivityChangedpVictim->ToPlayer();

    // battleground things (do this at the end, so the death state flag will be properly set to handle in the bg->handlekill)
    if (player && player->InBattleground())
    {
        if (Battleground* bg = player->GetBattleground())
        {
            if (victim->GetTypeId() == TYPEID_PLAYER)
                bg->HandleKillPlayer((Player*)victim, player);
            else
                bg->HandleKillUnit(victim->ToCreature(), player);
        }
    }

    // achievement stuff
    if (victim->GetTypeId() == TYPEID_PLAYER)
    {
        if (GetTypeId() == TYPEID_UNIT)
            victim->ToPlayer()->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_KILLED_BY_CREATURE, GetEntry());
        else if (GetTypeId() == TYPEID_PLAYER && victim != this)
            victim->ToPlayer()->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_KILLED_BY_PLAYER, 1, ToPlayer()->GetTeam());
    }

    // Hook for OnPVPKill Event
    if (Player* killerPlr = ToPlayer())
    {
        if (Player* killedPlr = victim->ToPlayer())
            sScriptMgr->OnPVPKill(killerPlr, killedPlr);
        else if (Creature* killedCre = victim->ToCreature())
            sScriptMgr->OnCreatureKill(killerPlr, killedCre);
    }
    else if (Creature* killerCre = ToCreature())
    {
        if (Player* killed = victim->ToPlayer())
            sScriptMgr->OnPlayerKilledByCreature(killerCre, killed);
    }

    m_IsInKillingProcess = false;
}

void Unit::SetControlled(bool apply, UnitState state)
{
    if (apply)
    {
        if (HasUnitState(state))
            return;

        AddUnitState(state);
        switch (state)
        {
            case UNIT_STATE_STUNNED:
                SetStunned(true);
                CastStop();
                break;
            case UNIT_STATE_ROOT:
                if (!HasUnitState(UNIT_STATE_STUNNED))
                    SetRooted(true);
                break;
            case UNIT_STATE_CONFUSED:
                if (!HasUnitState(UNIT_STATE_STUNNED))
                {
                    ClearUnitState(UNIT_STATE_MELEE_ATTACKING);
                    SendMeleeAttackStop();
                    // SendAutoRepeatCancel ?
                    SetConfused(true);
                    CastStop();
                }
                break;
            case UNIT_STATE_FLEEING:
                if (!HasUnitState(UNIT_STATE_STUNNED | UNIT_STATE_CONFUSED))
                {
                    ClearUnitState(UNIT_STATE_MELEE_ATTACKING);
                    SendMeleeAttackStop();
                    // SendAutoRepeatCancel ?
                    SetFeared(true);
                    CastStop();
                }
                break;
            default:
                break;
        }
    }
    else
    {
        switch (state)
        {
            case UNIT_STATE_STUNNED: if (HasAuraType(SPELL_AURA_MOD_STUN))    return;
                                    else    SetStunned(false);    break;
            case UNIT_STATE_ROOT:    if (HasAuraType(SPELL_AURA_MOD_ROOT) || GetVehicle())    return;
                                    else    SetRooted(false);     break;
            case UNIT_STATE_CONFUSED:if (HasAuraType(SPELL_AURA_MOD_CONFUSE)) return;
                                    else    SetConfused(false);   break;
            case UNIT_STATE_FLEEING: if (HasAuraType(SPELL_AURA_MOD_FEAR))    return;
                                    else    SetFeared(false);     break;
            default: return;
        }

        ClearUnitState(state);

        if (HasUnitState(UNIT_STATE_STUNNED))
            SetStunned(true);
        else
        {
            if (HasUnitState(UNIT_STATE_ROOT))
                SetRooted(true);

            if (HasUnitState(UNIT_STATE_CONFUSED))
                SetConfused(true);
            else if (HasUnitState(UNIT_STATE_FLEEING))
                SetFeared(true);
        }
    }
}

void Unit::SendMoveRoot(uint32 value)
{
    ObjectGuid guid = GetGUID();

    //! 5.4.1
    WorldPacket data(SMSG_MOVE_ROOT, 1 + 8 + 4);
    data.WriteGuidMask<7, 1, 2, 6, 4, 3, 0, 5>(guid);
    data << uint32(value);
    data.WriteGuidBytes<5, 7, 2, 0, 4, 1, 6, 3>(guid);

    SendMessageToSet(&data, true);
}

void Unit::SendMoveUnroot(uint32 value)
{
    ObjectGuid guid = GetGUID();

    //! 5.4.1
    WorldPacket data(SMSG_MOVE_UNROOT, 1 + 8 + 4);
    data.WriteGuidMask<2, 0, 3, 6, 1, 5, 4, 7>(guid);
    data.WriteGuidBytes<1, 5, 2, 6, 4>(guid);
    data << uint32(value);
    data.WriteGuidBytes<7, 0, 3>(guid);

    SendMessageToSet(&data, true);
}

void Unit::SetStunned(bool apply)
{
    if (apply)
    {
        SetTarget(0);
        SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_STUNNED);

        // MOVEMENTFLAG_ROOT cannot be used in conjunction with MOVEMENTFLAG_MASK_MOVING (tested 3.3.5a)
        // this will freeze clients. That's why we remove MOVEMENTFLAG_MASK_MOVING before
        // setting MOVEMENTFLAG_ROOT
        RemoveUnitMovementFlag(MOVEMENTFLAG_MASK_MOVING);
        AddUnitMovementFlag(MOVEMENTFLAG_ROOT);

        // Creature specific
        if (GetTypeId() != TYPEID_PLAYER)
            ToCreature()->StopMoving();
        else
            SetStandState(UNIT_STAND_STATE_STAND);

        SendMoveRoot(0);

        CastStop();
    }
    else
    {
        if (isAlive() && getVictim())
            SetTarget(getVictim()->GetGUID());

        // don't remove UNIT_FLAG_STUNNED for pet when owner is mounted (disabled pet's interface)
        Unit* owner = GetOwner();
        if (!owner || (owner->GetTypeId() == TYPEID_PLAYER && !owner->ToPlayer()->IsMounted()))
            RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_STUNNED);

        if (!HasUnitState(UNIT_STATE_ROOT))         // prevent moving if it also has root effect
        {
            SendMoveUnroot(0);
            RemoveUnitMovementFlag(MOVEMENTFLAG_ROOT);
        }
    }
}

void Unit::SetRooted(bool apply)
{
    if (apply)
    {
        if (m_rootTimes > 0) // blizzard internal check?
            m_rootTimes++;

        // MOVEMENTFLAG_ROOT cannot be used in conjunction with MOVEMENTFLAG_MASK_MOVING (tested 3.3.5a)
        // this will freeze clients. That's why we remove MOVEMENTFLAG_MASK_MOVING before
        // setting MOVEMENTFLAG_ROOT
        RemoveUnitMovementFlag(MOVEMENTFLAG_MASK_MOVING);
        AddUnitMovementFlag(MOVEMENTFLAG_ROOT);

        if (GetTypeId() == TYPEID_PLAYER)
            SendMoveRoot(m_rootTimes);
        else
        {
            ObjectGuid guid = GetGUID();
            //! 5.4.1
            WorldPacket data(SMSG_SPLINE_MOVE_ROOT, 8);
   
            data.WriteGuidMask<0, 5, 2, 1, 4, 6, 3, 7>(guid);
            data.WriteGuidBytes<2, 7, 3, 0, 4, 6, 1, 5>(guid);
            SendMessageToSet(&data, true);
            StopMoving();
        }
    }
    else
    {
        if (!HasUnitState(UNIT_STATE_STUNNED))      // prevent moving if it also has stun effect
        {
            if (GetTypeId() == TYPEID_PLAYER)
                SendMoveUnroot(++m_rootTimes);
            else
            {
                ObjectGuid guid = GetGUID();
                //! 5.4.1
                WorldPacket data(SMSG_SPLINE_MOVE_UNROOT, 8);
                    
                data.WriteGuidMask<1, 0, 2, 6, 5, 4, 7>(guid);
                data.WriteGuidBytes<2, 4, 7, 3, 6, 5, 1, 0>(guid);
                SendMessageToSet(&data, true);
            }

            RemoveUnitMovementFlag(MOVEMENTFLAG_ROOT);
        }
    }
}

void Unit::SetFeared(bool apply)
{
    if (apply)
    {
        SetTarget(0);

        uint32 mechanic_mask = (1 << MECHANIC_FEAR) | (1 << MECHANIC_HORROR);

        Unit* caster = NULL;
        Unit::AuraEffectList fearAuras = GetAuraEffectsByMechanic(mechanic_mask);
        if (!fearAuras.empty())
            caster = ObjectAccessor::GetUnit(*this, fearAuras.front()->GetCasterGUID());
        if (!caster)
            caster = getAttackerForHelper();
        GetMotionMaster()->MoveFleeing(caster, fearAuras.empty() ? sWorld->getIntConfig(CONFIG_CREATURE_FAMILY_FLEE_DELAY) : 0);             // caster == NULL processed in MoveFleeing
    }
    else
    {
        if (isAlive())
        {
            if (GetMotionMaster()->GetCurrentMovementGeneratorType() == FLEEING_MOTION_TYPE)
                GetMotionMaster()->MovementExpired();
            if (getVictim())
                SetTarget(getVictim()->GetGUID());
        }
    }

    if (GetTypeId() == TYPEID_PLAYER)
        ToPlayer()->SetClientControl(this, !apply);
}

void Unit::SetConfused(bool apply)
{
    if (apply)
    {
        SetTarget(0);
        GetMotionMaster()->MoveConfused();
    }
    else
    {
        if (isAlive())
        {
            if (GetMotionMaster()->GetCurrentMovementGeneratorType() == CONFUSED_MOTION_TYPE)
                GetMotionMaster()->MovementExpired();
            if (getVictim())
                SetTarget(getVictim()->GetGUID());
        }
    }

    if (GetTypeId() == TYPEID_PLAYER)
    {
        ToPlayer()->SendMovementSetCollisionHeight(ToPlayer()->GetCollisionHeight(apply));
        ToPlayer()->SetClientControl(this, !apply);
    }
}

bool Unit::SetCharmedBy(Unit* charmer, CharmType type, AuraApplication const* aurApp)
{
    if (!charmer)
        return false;

    // dismount players when charmed
    if (GetTypeId() == TYPEID_PLAYER)
        RemoveAurasByType(SPELL_AURA_MOUNTED);

    if (charmer->GetTypeId() == TYPEID_PLAYER)
        charmer->RemoveAurasByType(SPELL_AURA_MOUNTED);

    //ASSERT(type != CHARM_TYPE_POSSESS || charmer->GetTypeId() == TYPEID_PLAYER);
    if(!(type != CHARM_TYPE_POSSESS || charmer->GetTypeId() == TYPEID_PLAYER))
        return false;
    //ASSERT((type == CHARM_TYPE_VEHICLE) == IsVehicle());
    if(!((type == CHARM_TYPE_VEHICLE) == IsVehicle()))
        return false;

    sLog->outDebug(LOG_FILTER_UNITS, "SetCharmedBy: charmer %u (GUID %u), charmed %u (GUID %u), type %u.", charmer->GetEntry(), charmer->GetGUIDLow(), GetEntry(), GetGUIDLow(), uint32(type));

    if (this == charmer)
    {
        sLog->outFatal(LOG_FILTER_UNITS, "Unit::SetCharmedBy: Unit %u (GUID %u) is trying to charm itself!", GetEntry(), GetGUIDLow());
        return false;
    }

    //if (HasUnitState(UNIT_STATE_UNATTACKABLE))
    //    return false;

    if (GetTypeId() == TYPEID_PLAYER && ToPlayer()->GetTransport())
    {
        sLog->outFatal(LOG_FILTER_UNITS, "Unit::SetCharmedBy: Player on transport is trying to charm %u (GUID %u)", GetEntry(), GetGUIDLow());
        return false;
    }

    // Already charmed
    if (GetCharmerGUID())
    {
        sLog->outFatal(LOG_FILTER_UNITS, "Unit::SetCharmedBy: %u (GUID %u) has already been charmed but %u (GUID %u) is trying to charm it!", GetEntry(), GetGUIDLow(), charmer->GetEntry(), charmer->GetGUIDLow());
        return false;
    }

    CastStop();
    CombatStop(); // TODO: CombatStop(true) may cause crash (interrupt spells)
    DeleteThreatList();

    // Charmer stop charming
    if (charmer->GetTypeId() == TYPEID_PLAYER)
    {
        charmer->ToPlayer()->StopCastingCharm();
        charmer->ToPlayer()->StopCastingBindSight();
    }

    // Charmed stop charming
    if (GetTypeId() == TYPEID_PLAYER)
    {
        ToPlayer()->StopCastingCharm();
        ToPlayer()->StopCastingBindSight();
    }

    // StopCastingCharm may remove a possessed pet?
    if (!IsInWorld())
    {
        sLog->outFatal(LOG_FILTER_UNITS, "Unit::SetCharmedBy: %u (GUID %u) is not in world but %u (GUID %u) is trying to charm it!", GetEntry(), GetGUIDLow(), charmer->GetEntry(), charmer->GetGUIDLow());
        return false;
    }

    // charm is set by aura, and aura effect remove handler was called during apply handler execution
    // prevent undefined behaviour
    if (aurApp && aurApp->GetRemoveMode())
        return false;

    // Set charmed
    Map* map = GetMap();
    if (!IsVehicle() || (IsVehicle() && map && !map->IsBattleground()))
        setFaction(charmer->getFaction());

    charmer->SetCharm(this, true);

    if (GetTypeId() == TYPEID_UNIT)
    {
        ToCreature()->AI()->OnCharmed(true);
        GetMotionMaster()->MoveIdle();
    }
    else
    {
        Player* player = ToPlayer();
        if (player->isAFK())
            player->ToggleAFK();
        player->SetClientControl(this, 0);
    }

    // charm is set by aura, and aura effect remove handler was called during apply handler execution
    // prevent undefined behaviour
    if (aurApp && aurApp->GetRemoveMode())
        return false;

    // Pets already have a properly initialized CharmInfo, don't overwrite it.
    if (type != CHARM_TYPE_VEHICLE && !GetCharmInfo())
    {
        InitCharmInfo();
        if (type == CHARM_TYPE_POSSESS)
            GetCharmInfo()->InitPossessCreateSpells();
        else
            GetCharmInfo()->InitCharmCreateSpells();
    }

    if (charmer->GetTypeId() == TYPEID_PLAYER)
    {
        switch (type)
        {
            case CHARM_TYPE_VEHICLE:
                SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PLAYER_CONTROLLED);
                charmer->ToPlayer()->SetClientControl(this, 1);
                charmer->ToPlayer()->SetMover(this);
                charmer->ToPlayer()->SetViewpoint(this, true);
                charmer->ToPlayer()->VehicleSpellInitialize();
                break;
            case CHARM_TYPE_POSSESS:
                AddUnitState(UNIT_STATE_POSSESSED);
                SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PLAYER_CONTROLLED);
                charmer->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
                charmer->ToPlayer()->SetClientControl(this, 1);
                charmer->ToPlayer()->SetMover(this);
                charmer->ToPlayer()->SetViewpoint(this, true);
                charmer->ToPlayer()->PossessSpellInitialize();
                break;
            case CHARM_TYPE_CHARM:
                if (GetTypeId() == TYPEID_UNIT && charmer->getClass() == CLASS_WARLOCK)
                {
                    CreatureTemplate const* cinfo = ToCreature()->GetCreatureTemplate();
                    if (cinfo && cinfo->type == CREATURE_TYPE_DEMON)
                    {
                        // to prevent client crash
                        SetClass(CLASS_MAGE);

                        // just to enable stat window
                        if (GetCharmInfo())
                            GetCharmInfo()->SetPetNumber(sObjectMgr->GeneratePetNumber(), true);

                        // if charmed two demons the same session, the 2nd gets the 1st one's name
                        SetUInt32Value(UNIT_FIELD_PET_NAME_TIMESTAMP, uint32(time(NULL))); // cast can't be helped
                    }
                }
                charmer->ToPlayer()->CharmSpellInitialize();
                break;
            default:
            case CHARM_TYPE_CONVERT:
                break;
        }
    }else if (type == CHARM_TYPE_CONVERT)
    {
        if (ToPlayer())
        {
            ToPlayer()->SetClientControl(charmer, 1);
            ToPlayer()->SetMover(charmer);
        }
    }

    return true;
}

void Unit::RemoveCharmedBy(Unit* charmer)
{
    if (!isCharmed())
        return;

    if (!charmer)
        charmer = GetCharmer();
    if (charmer != GetCharmer()) // one aura overrides another?
    {
//        sLog->outFatal(LOG_FILTER_UNITS, "Unit::RemoveCharmedBy: this: " UI64FMTD " true charmer: " UI64FMTD " false charmer: " UI64FMTD,
//            GetGUID(), GetCharmerGUID(), charmer->GetGUID());
//        ASSERT(false);
        return;
    }

    CharmType type;
    if (HasUnitState(UNIT_STATE_POSSESSED))
        type = CHARM_TYPE_POSSESS;
    else if (charmer && charmer->IsOnVehicle(this))
        type = CHARM_TYPE_VEHICLE;
    else
        type = CHARM_TYPE_CHARM;

    CastStop();
    CombatStop(); // TODO: CombatStop(true) may cause crash (interrupt spells)
    AttackStop();
    getHostileRefManager().deleteReferences();
    DeleteThreatList();
    Map* map = GetMap();
    if (!IsVehicle() || (IsVehicle() && map && !map->IsBattleground()))
        RestoreFaction();
    GetMotionMaster()->Clear(true);
    GetMotionMaster()->InitDefault();

    if (type == CHARM_TYPE_POSSESS)
    {
        ClearUnitState(UNIT_STATE_POSSESSED);
        RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PLAYER_CONTROLLED);
    }

    if (Creature* creature = ToCreature())
    {
        // Creature will restore its old AI on next update
        if (creature->AI())
            creature->AI()->OnCharmed(false);

        // Vehicle should not attack its passenger after he exists the seat
        if (type != CHARM_TYPE_VEHICLE)
            LastCharmerGUID = charmer->GetGUID();
    }
    else
        ToPlayer()->SetClientControl(this, 1);

    // If charmer still exists
    if (!charmer)
        return;

    ASSERT(type != CHARM_TYPE_POSSESS || charmer->GetTypeId() == TYPEID_PLAYER);
    ASSERT(type != CHARM_TYPE_VEHICLE || (GetTypeId() == TYPEID_UNIT && IsVehicle()));

    charmer->SetCharm(this, false);

    if (charmer->GetTypeId() == TYPEID_PLAYER)
    {
        switch (type)
        {
            case CHARM_TYPE_VEHICLE:
                charmer->ToPlayer()->SetClientControl(charmer, 1);
                charmer->ToPlayer()->SetViewpoint(this, false);
                charmer->ToPlayer()->SetClientControl(this, 0);
                if (GetTypeId() == TYPEID_PLAYER)
                    ToPlayer()->SetMover(this);
                break;
            case CHARM_TYPE_POSSESS:
                charmer->ToPlayer()->SetClientControl(charmer, 1);
                charmer->ToPlayer()->SetViewpoint(this, false);
                charmer->ToPlayer()->SetClientControl(this, 0);
                if (GetTypeId() == TYPEID_PLAYER)
                    ToPlayer()->SetMover(this);
                charmer->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
                break;
            case CHARM_TYPE_CHARM:
                if (GetTypeId() == TYPEID_UNIT && charmer->getClass() == CLASS_WARLOCK)
                {
                    CreatureTemplate const* cinfo = ToCreature()->GetCreatureTemplate();
                    if (cinfo && cinfo->type == CREATURE_TYPE_DEMON)
                    {
                        SetClass(cinfo->unit_class);
                        if (GetCharmInfo())
                            GetCharmInfo()->SetPetNumber(0, true);
                        else
                            sLog->outError(LOG_FILTER_UNITS, "Aura::HandleModCharm: target=" UI64FMTD " with typeid=%d has a charm aura but no charm info!", GetGUID(), GetTypeId());
                    }
                }
                break;
            default:
            case CHARM_TYPE_CONVERT:
                break;
        }
    }else if (type == CHARM_TYPE_CONVERT)
    {
        if (ToPlayer())
        {
            ToPlayer()->SetClientControl(charmer, 0);
            ToPlayer()->SetMover(this);
        }
    }

    // a guardian should always have charminfo
    if (charmer->GetTypeId() == TYPEID_PLAYER && this != charmer->GetFirstControlled())
        charmer->ToPlayer()->SendRemoveControlBar();
    else if (GetTypeId() == TYPEID_PLAYER || (GetTypeId() == TYPEID_UNIT && !ToCreature()->isGuardian()))
        DeleteCharmInfo();
}

void Unit::RestoreFaction()
{
    if (GetTypeId() == TYPEID_PLAYER)
        ToPlayer()->setFactionForRace(getRace());
    else
    {
        if (HasUnitTypeMask(UNIT_MASK_MINION))
        {
            if (Unit* owner = GetOwner())
            {
                setFaction(owner->getFaction());
                return;
            }
        }

        if (CreatureTemplate const* cinfo = ToCreature()->GetCreatureTemplate())  // normal creature
        {
            FactionTemplateEntry const* faction = getFactionTemplateEntry();
            setFaction(cinfo->faction);
        }
    }
}

bool Unit::CreateVehicleKit(uint32 id, uint32 creatureEntry, uint32 RecAura)
{
    VehicleEntry const* vehInfo = sVehicleStore.LookupEntry(id);
    if (!vehInfo)
        return false;

    m_vehicleKit = new Vehicle(this, vehInfo, creatureEntry, RecAura);
    m_updateFlag |= UPDATEFLAG_VEHICLE;
    m_unitTypeMask |= UNIT_MASK_VEHICLE;
    return true;
}

void Unit::RemoveVehicleKit()
{
    if (!m_vehicleKit)
        return;

    m_vehicleKit->Uninstall(true);
    delete m_vehicleKit;

    m_vehicleKit = NULL;

    m_updateFlag &= ~UPDATEFLAG_VEHICLE;
    m_unitTypeMask &= ~UNIT_MASK_VEHICLE;
    RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK);
    RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_PLAYER_VEHICLE);
}

Unit* Unit::GetVehicleBase() const
{
    return m_vehicle ? m_vehicle->GetBase() : NULL;
}

Creature* Unit::GetVehicleCreatureBase() const
{
    if (Unit* veh = GetVehicleBase())
        if (Creature* c = veh->ToCreature())
            return c;

    return NULL;
}

uint64 Unit::GetTransGUID() const
{
    if (GetVehicle())
        return GetVehicleBase()->GetGUID();
    if (GetTransport())
        return GetTransport()->GetGUID();

    return 0;
}

TransportBase* Unit::GetDirectTransport() const
{
    if (Vehicle* veh = GetVehicle())
        return veh;
    return GetTransport();
}

bool Unit::IsInPartyWith(Unit const* unit) const
{
    if (this == unit)
        return true;

    const Unit* u1 = GetCharmerOrOwnerOrSelf();
    const Unit* u2 = unit->GetCharmerOrOwnerOrSelf();
    if (u1 == u2)
        return true;

    if (u1->GetTypeId() == TYPEID_PLAYER && u2->GetTypeId() == TYPEID_PLAYER)
        return u1->ToPlayer()->IsInSameGroupWith(u2->ToPlayer());
    else if ((u2->GetTypeId() == TYPEID_PLAYER && u1->GetTypeId() == TYPEID_UNIT && u1->ToCreature()->GetCreatureTemplate()->type_flags & CREATURE_TYPEFLAGS_TREAT_AS_RAID_UNIT) ||
        (u1->GetTypeId() == TYPEID_PLAYER && u2->GetTypeId() == TYPEID_UNIT && u2->ToCreature()->GetCreatureTemplate()->type_flags & CREATURE_TYPEFLAGS_TREAT_AS_RAID_UNIT))
        return true;
    else
        return false;
}

bool Unit::IsInRaidWith(Unit const* unit) const
{
    if (this == unit)
        return true;

    const Unit* u1 = GetCharmerOrOwnerOrSelf();
    const Unit* u2 = unit->GetCharmerOrOwnerOrSelf();
    if (u1 == u2)
        return true;

    if (u1->GetTypeId() == TYPEID_PLAYER && u2->GetTypeId() == TYPEID_PLAYER)
        return u1->ToPlayer()->IsInSameRaidWith(u2->ToPlayer());
    else if ((u2->GetTypeId() == TYPEID_PLAYER && u1->GetTypeId() == TYPEID_UNIT && u1->ToCreature()->GetCreatureTemplate()->type_flags & CREATURE_TYPEFLAGS_TREAT_AS_RAID_UNIT) ||
            (u1->GetTypeId() == TYPEID_PLAYER && u2->GetTypeId() == TYPEID_UNIT && u2->ToCreature()->GetCreatureTemplate()->type_flags & CREATURE_TYPEFLAGS_TREAT_AS_RAID_UNIT))
        return true;
    else
        return false;
}

void Unit::GetPartyMembers(std::list<Unit*> &TagUnitMap)
{
    Unit* owner = GetCharmerOrOwnerOrSelf();
    Group* group = NULL;
    if (owner->GetTypeId() == TYPEID_PLAYER)
        group = owner->ToPlayer()->GetGroup();

    if (group)
    {
        uint8 subgroup = owner->ToPlayer()->GetSubGroup();

        for (GroupReference* itr = group->GetFirstMember(); itr != NULL; itr = itr->next())
        {
            Player* Target = itr->getSource();

            // IsHostileTo check duel and controlled by enemy
            if (Target && Target->GetSubGroup() == subgroup && !IsHostileTo(Target))
            {
                if (Target->isAlive() && IsInMap(Target))
                    TagUnitMap.push_back(Target);

                if (Guardian* pet = Target->GetGuardianPet())
                    if (pet->isAlive() && IsInMap(Target))
                        TagUnitMap.push_back(pet);
            }
        }
    }
    else
    {
        if (owner->isAlive() && (owner == this || IsInMap(owner)))
            TagUnitMap.push_back(owner);
        if (Guardian* pet = owner->GetGuardianPet())
            if (pet->isAlive() && (pet == this || IsInMap(pet)))
                TagUnitMap.push_back(pet);
    }
}

Aura* Unit::ToggleAura(uint32 spellId, Unit* target)
{
    if (!target)
        return NULL;

    if (target->HasAura(spellId))
    {
        target->RemoveAurasDueToSpell(spellId);
        return NULL;
    }
    else
        return target->AddAura(spellId, target);

    
    return NULL;
}

Aura* Unit::AddAura(uint32 spellId, Unit* target, Item* castItem, uint16 stackAmount)
{
    if (!target)
        return NULL;

    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId);
    if (!spellInfo)
        return NULL;

    if (!target->isAlive() && !(spellInfo->Attributes & SPELL_ATTR0_PASSIVE) && !(spellInfo->AttributesEx2 & SPELL_ATTR2_CAN_TARGET_DEAD))
        return NULL;

    return AddAura(spellInfo, MAX_EFFECT_MASK, target, castItem, stackAmount);
}

Aura* Unit::AddAura(SpellInfo const* spellInfo, uint32 effMask, Unit* target, Item* castItem, uint16 stackAmount)
{
    if (!spellInfo)
        return NULL;

    if (target->IsImmunedToSpell(spellInfo))
        return NULL;

    for (uint32 i = 0; i < MAX_SPELL_EFFECTS; ++i)
    {
        if (!(effMask & (1<<i)))
            continue;
        if (target->IsImmunedToSpellEffect(spellInfo, i))
            effMask &= ~(1<<i);
    }

    Aura* aura = Aura::TryRefreshStackOrCreate(spellInfo, effMask, target, this, NULL, castItem, NULL, NULL, stackAmount);
    if (aura != NULL)
    {
        aura->ApplyForTargets();
        return aura;
    }
    return NULL;
}

void Unit::SetAuraStack(uint32 spellId, Unit* target, uint32 stack)
{
    Aura* aura = target->GetAura(spellId, GetGUID());
    if (!aura)
        aura = AddAura(spellId, target);
    if (aura && stack)
        aura->ModStackAmount(stack);
}

void Unit::SendPlaySpellVisualKit(uint32 id, uint32 unkParam)
{
    ObjectGuid guid = GetGUID();

    WorldPacket data(SMSG_PLAY_SPELL_VISUAL_KIT, 4 + 4+ 4 + 8);
    data << uint32(0);
    data << uint32(unkParam);
    data << uint32(id); // SpellVisualKit.db2 index

    data.WriteGuidMask<5, 4, 6, 0, 1, 7, 3, 2>(guid);
    data.WriteGuidBytes<1, 7, 0, 3, 5, 4, 6, 2>(guid);

    SendMessageToSet(&data, true);
}

void Unit::ApplyResilience(Unit const* victim, int32* damage, bool isCrit) const
{
    // player mounted on multi-passenger mount is also classified as vehicle
    if (IsVehicle() && GetTypeId() != TYPEID_PLAYER || (victim->IsVehicle() && victim->GetTypeId() != TYPEID_PLAYER))
        return;

    // Resilience works only for players or pets against other players or pets
    if (GetTypeId() != TYPEID_PLAYER && (GetOwner() && GetOwner()->GetTypeId() != TYPEID_PLAYER))
        return;

    // Don't consider resilience if not in PvP - player or pet
    if (!GetCharmerOrOwnerPlayerOrPlayerItself())
        return;

    if (victim->GetOwnerGUID() && victim->GetOwnerGUID() == GetGUID())
        return;

    Unit const* target = NULL;
    if (victim->GetTypeId() == TYPEID_PLAYER)
        target = victim;
    else if (victim->GetTypeId() == TYPEID_UNIT && victim->GetOwner() && victim->GetOwner()->GetTypeId() == TYPEID_PLAYER)
        target = victim->GetOwner();

    if (!target)
        return;

    *damage -= target->GetDamageReduction(*damage);
}

// Melee based spells can be miss, parry or dodge on this step
// Crit or block - determined on damage calculation phase! (and can be both in some time)
float Unit::MeleeSpellMissChance(const Unit* victim, WeaponAttackType attType, uint32 spellId) const
{
    //calculate miss chance
    float missChance = victim->GetUnitMissChance(attType);

    // for example
    // | caster | target | miss 
    //    90        90      3
    //    90        91     4.5
    //    90        92      6
    //    90        93     7.5

    int16 level_diff = victim->getLevel() - getLevel();
    missChance += 1.5f * level_diff;


    if (!spellId && haveOffhandWeapon())
        missChance += 19;

    // Calculate hit chance
    float hitChance = 100.0f;

    // Spellmod from SPELLMOD_RESIST_MISS_CHANCE
    if (spellId)
    {
        if (Player* modOwner = GetSpellModOwner())
            modOwner->ApplySpellMod(spellId, SPELLMOD_RESIST_MISS_CHANCE, hitChance);
    }

    missChance += hitChance - 100.0f;

    if (attType == RANGED_ATTACK)
        missChance -= m_modRangedHitChance;
    else
        missChance -= m_modMeleeHitChance;

    // Limit miss chance from 0 to 60%
    if (missChance < 0.0f)
        return 0.0f;
    if (missChance > 100.0f)
        return 100.0f;
    return missChance;
}

void Unit::SetPhaseMask(uint32 newPhaseMask, bool update)
{
    if (newPhaseMask == GetPhaseMask())
        return;

    if (IsInWorld())
    {
        // modify hostile references for new phasemask, some special cases deal with hostile references themselves
        if (GetTypeId() == TYPEID_UNIT || (!ToPlayer()->isGameMaster() && !ToPlayer()->GetSession()->PlayerLogout()))
        {
            HostileRefManager& refManager = getHostileRefManager();
            HostileReference* ref = refManager.getFirst();

            while (ref)
            {
                if (Unit* unit = ref->getSource()->getOwner())
                    if (Creature* creature = unit->ToCreature())
                        refManager.setOnlineOfflineState(creature, creature->InSamePhase(newPhaseMask));

                ref = ref->next();
            }

            // modify threat lists for new phasemask
            if (GetTypeId() != TYPEID_PLAYER)
            {
                std::list<HostileReference*> threatList = getThreatManager().getThreatList();
                std::list<HostileReference*> offlineThreatList = getThreatManager().getOfflineThreatList();

                // merge expects sorted lists
                threatList.sort();
                offlineThreatList.sort();
                threatList.merge(offlineThreatList);

                for (std::list<HostileReference*>::const_iterator itr = threatList.begin(); itr != threatList.end(); ++itr)
                    if (Unit* unit = (*itr)->getTarget())
                        unit->getHostileRefManager().setOnlineOfflineState(ToCreature(), unit->InSamePhase(newPhaseMask));
            }
        }
    }

    WorldObject::SetPhaseMask(newPhaseMask, false);

    if (!IsInWorld())
        return;

    for (ControlList::const_iterator itr = m_Controlled.begin(); itr != m_Controlled.end(); ++itr)
    {
        Unit* u = *itr;
        if (!u)
            continue;

        if (u->GetTypeId() == TYPEID_UNIT)
            u->SetPhaseMask(newPhaseMask, true);
    }

    for (uint8 i = 0; i < MAX_SUMMON_SLOT; ++i)
        if (m_SummonSlot[i])
            if (Creature* summon = GetMap()->GetCreature(m_SummonSlot[i]))
                summon->SetPhaseMask(newPhaseMask, true);

    RemoveNotOwnSingleTargetAuras(newPhaseMask);            // we can lost access to caster or target

    // Update visibility after phasing pets and summons so they wont despawn
    if (update)
        UpdateObjectVisibility();
}

void Unit::SetPhaseId(uint32 newPhase, bool update)
{
    if (newPhase == GetPhaseId())
        return;

    if (IsInWorld())
    {
        // modify hostile references for new phasemask, some special cases deal with hostile references themselves
        if (GetTypeId() == TYPEID_UNIT || (!ToPlayer()->isGameMaster() && !ToPlayer()->GetSession()->PlayerLogout()))
        {
            HostileRefManager& refManager = getHostileRefManager();
            HostileReference* ref = refManager.getFirst();

            while (ref)
            {
                if (Unit* unit = ref->getSource()->getOwner())
                    if (Creature* creature = unit->ToCreature())
                        refManager.setOnlineOfflineState(creature, creature->InSamePhaseId(newPhase));

                ref = ref->next();
            }

            // modify threat lists for new phasemask
            if (GetTypeId() != TYPEID_PLAYER)
            {
                std::list<HostileReference*> threatList = getThreatManager().getThreatList();
                std::list<HostileReference*> offlineThreatList = getThreatManager().getOfflineThreatList();

                // merge expects sorted lists
                threatList.sort();
                offlineThreatList.sort();
                threatList.merge(offlineThreatList);

                for (std::list<HostileReference*>::const_iterator itr = threatList.begin(); itr != threatList.end(); ++itr)
                    if (Unit* unit = (*itr)->getTarget())
                        unit->getHostileRefManager().setOnlineOfflineState(ToCreature(), unit->InSamePhaseId(newPhase));
            }
        }
    }

    WorldObject::SetPhaseId(newPhase, false);

    if (!IsInWorld())
        return;

    for (ControlList::const_iterator itr = m_Controlled.begin(); itr != m_Controlled.end(); ++itr)
    {
        Unit* u = *itr;
        if (!u)
            continue;

        if (u->GetTypeId() == TYPEID_UNIT)
            u->SetPhaseId(newPhase, true);
    }

    for (uint8 i = 0; i < MAX_SUMMON_SLOT; ++i)
        if (m_SummonSlot[i])
            if (Creature* summon = GetMap()->GetCreature(m_SummonSlot[i]))
                summon->SetPhaseId(newPhase, true);

    // Update visibility after phasing pets and summons so they wont despawn
    if (update)
        UpdateObjectVisibility();
}

class Unit::AINotifyTask : public BasicEvent
{
    Unit& m_owner;
public:
    explicit AINotifyTask(Unit * me) : m_owner(*me) {}

    virtual bool Execute(uint64 , uint32)
    {
        Trinity::AIRelocationNotifier notifier(m_owner);
        m_owner.VisitNearbyObject(m_owner.GetVisibilityRange(), notifier);
        m_owner.m_VisibilityUpdScheduled = false;
        return true;
    }

    static void ScheduleAINotify(Unit* me)
    {
        if (!me->m_VisibilityUpdScheduled)
        {
            me->m_VisibilityUpdScheduled = true;
            me->m_Events.AddEvent(new AINotifyTask(me), me->m_Events.CalculateTime(World::Visibility_AINotifyDelay));
        }
    }
};

class Unit::VisibilityUpdateTask : public BasicEvent
{
    Unit& m_owner;
public:
    explicit VisibilityUpdateTask(Unit * me) : m_owner(*me) {}

    virtual bool Execute(uint64 , uint32)
    {
        UpdateVisibility(&m_owner);
        return true;
    }

    static void UpdateVisibility(Unit* me)
     {
        if (!me->m_sharedVision.empty())
            for (SharedVisionList::const_iterator it = me->m_sharedVision.begin();it!= me->m_sharedVision.end();)
            {
                Player * tmp = *it;
                ++it;
                tmp->UpdateVisibilityForPlayer();
            }
        if (me->isType(TYPEMASK_PLAYER))
            ((Player*)me)->UpdateVisibilityForPlayer();
        else
            me->WorldObject::UpdateObjectVisibility(true);
        me->m_VisibilityUpdateTask = false;
    }
};

void Unit::OnRelocated()
{
    if (m_VisibilityUpdateTask)
        return;

    if (!m_lastVisibilityUpdPos.IsInDist(this, World::Visibility_RelocationLowerLimit))
    {
        m_lastVisibilityUpdPos = *this;
        m_VisibilityUpdateTask = true;
        m_Events.AddEvent(new VisibilityUpdateTask(this), m_Events.CalculateTime(1));
    }
    AINotifyTask::ScheduleAINotify(this);
}

void Unit::UpdateObjectVisibility(bool forced)
{
    if (m_VisibilityUpdateTask)
        return;

    if (forced)
        VisibilityUpdateTask::UpdateVisibility(this);
    else
    {
        m_VisibilityUpdateTask = true;
        m_Events.AddEvent(new VisibilityUpdateTask(this), m_Events.CalculateTime(1));
    }
    AINotifyTask::ScheduleAINotify(this);
}

void Unit::SendMoveKnockBack(Player* player, float speedXY, float speedZ, float vcos, float vsin)
{
    AddUnitState(UNIT_STATE_JUMPING);
    m_TempSpeed = fabs(speedZ * 10.0f);

    ObjectGuid guid = GetGUID();
    //! 5.4.1
    WorldPacket data(SMSG_MOVE_KNOCK_BACK, (1+8+4+4+4+4+4));
    
    data << float(speedXY);
    data << float(vcos);
    data << float(speedZ);
    data << uint32(0);
    data << float(vsin);
    
    data.WriteGuidMask<7, 2, 4, 3, 0, 6, 1, 5>(guid);
    data.WriteGuidBytes<1, 4, 0, 2, 3, 5, 7, 6>(guid);

    player->GetSession()->SendPacket(&data);
}

void Unit::KnockbackFrom(float x, float y, float speedXY, float speedZ)
{
    Player* player = NULL;
    if (GetTypeId() == TYPEID_PLAYER)
        player = ToPlayer();
    else if (Unit* charmer = GetCharmer())
    {
        player = charmer->ToPlayer();
        if (player && player->m_mover != this)
            player = NULL;
    }

    if (!player)
        GetMotionMaster()->MoveKnockbackFrom(x, y, speedXY, speedZ);
    else
    {
        float vcos, vsin;
        GetSinCos(x, y, vsin, vcos);
        SendMoveKnockBack(player, speedXY, -speedZ, vcos, vsin);
    }
}

float Unit::GetCombatRatingReduction(CombatRating cr) const
{
    if (Player const* player = ToPlayer())
        return player->GetRatingBonusValue(cr);
    // Player's pet get resilience from owner
    else if (isPet() && GetOwner())
        if (Player* owner = GetOwner()->ToPlayer())
            return owner->GetRatingBonusValue(cr);

    return 0.0f;
}

uint32 Unit::GetCombatRatingDamageReduction(CombatRating cr, float cap, uint32 damage) const
{
    float percent = std::min(GetCombatRatingReduction(cr), cap);

    if (cr == CR_RESILIENCE_PLAYER_DAMAGE_TAKEN || cr == CR_RESILIENCE_CRIT_TAKEN)
    {
        if (Player* pl = GetCharmerOrOwnerPlayerOrPlayerItself())
        {
            float auraPercent = pl->GetFloatValue(PLAYER_FIELD_MOD_RESILIENCE_PCT);
            float scalar      = pl->GetRatingMultiplier(CR_RESILIENCE_PLAYER_DAMAGE_TAKEN) * 93.225806452f;
            uint32 resRating  = float(pl->GetUInt32Value(PLAYER_FIELD_COMBAT_RATING_1 + CR_RESILIENCE_PLAYER_DAMAGE_TAKEN));

            percent = ((scalar * 0.72f) + resRating) / (scalar + resRating) * 100.0f - 72.0f - auraPercent;
        }
    }

    return CalculatePct(damage, percent);
}

uint32 Unit::GetModelForForm(ShapeshiftForm form)
{
    switch (form)
    {
        case FORM_CAT:
        {
            // Hack for Druid of the Flame, Fandral's Flamescythe
            if (HasAura(99245) || HasAura(138927))
                return 38150;

            // check Incarnation
            bool epic = HasAura(102543);

            // Based on Hair color
            if (getRace() == RACE_NIGHTELF)
            {
                // Glyph of the Chameleon
                if (HasAura(107059))
                {
                    uint32 models[] = { 29405, 29406, 29407, 29408, 892 };
                    uint32 epicModels[] = { 43764, 43763, 43762, 43765, 43761 };
                    return epic ? epicModels[urand(0, 4)] : models[urand(0, 4)];
                }

                uint8 hairColor = GetByteValue(PLAYER_BYTES, 3);
                switch (hairColor)
                {
                    case 7: // Violet
                    case 8:
                        return epic ? 43764 : 29405;
                    case 3: // Light Blue
                        return epic ? 43763 : 29406;
                    case 0: // Green
                    case 1: // Light Green
                    case 2: // Dark Green
                        return epic ? 43762 : 29407;
                    case 4: // White
                        return epic ? 43765 : 29408;
                    default: // original - Dark Blue
                        return epic ? 43761 :892;
                }
            }
            else if (getRace() == RACE_TROLL)
            {
                // Glyph of the Chameleon
                if (HasAura(107059))
                {
                    uint32 models[] = { 33668, 33667, 33666, 33665, 33669 };
                    uint32 epicModels[] = { 43776, 43778, 43773, 43775, 43777 };
                    return epic ? epicModels[urand(0, 4)] : models[urand(0, 4)];
                }

                uint8 hairColor = GetByteValue(PLAYER_BYTES, 3);
                switch (hairColor)
                {
                    case 0: // Red
                    case 1:
                        return epic ? 43776 : 33668;
                    case 2: // Yellow
                    case 3:
                        return epic ? 43778 : 33667;
                    case 4: // Blue
                    case 5:
                    case 6:
                        return epic ? 43773 : 33666;
                    case 7: // Purple
                    case 10:
                        return epic ? 43775 : 33665;
                    default: // original - white
                        return epic ? 43777 : 33669;
                }
            }
            else if (getRace() == RACE_WORGEN)
            {
                // Glyph of the Chameleon
                if (HasAura(107059))
                {
                    uint32 models[] = { 33662, 33661, 33664, 33663, 33660 };
                    uint32 epicModels[] = { 43781, 43780, 43784, 43785, 43782 };
                    return epic ? epicModels[urand(0, 4)] : models[urand(0, 4)];
                }

                // Based on Skin color
                uint8 skinColor = GetByteValue(PLAYER_BYTES, 0);
                // Male
                if (getGender() == GENDER_MALE)
                {
                    switch (skinColor)
                    {
                        case 1: // Brown
                            return epic ? 43781 :33662;
                        case 2: // Black
                        case 7:
                            return epic ? 43780 : 33661;
                        case 4: // yellow
                            return epic ? 43784 : 33664;
                        case 3: // White
                        case 5:
                            return epic ? 43785 : 33663;
                        default: // original - Gray
                            return epic ? 43782 : 33660;
                    }
                }
                // Female
                else
                {
                    switch (skinColor)
                    {
                        case 5: // Brown
                        case 6:
                            return epic ? 43781 :33662;
                        case 7: // Black
                        case 8:
                            return epic ? 43780 : 33661;
                        case 3: // yellow
                        case 4:
                            return epic ? 43784 : 33664;
                        case 2: // White
                            return epic ? 43785 : 33663;
                        default: // original - Gray
                            return epic ? 43782 : 33660;
                    }
                }
            }
            // Based on Skin color
            else if (getRace() == RACE_TAUREN)
            {
                // Glyph of the Chameleon
                if (HasAura(107059))
                {
                    uint32 models[] = { 29409, 29410, 29411, 29412, 8571 };
                    uint32 epicModels[] = { 43769, 43770, 43768, 43766, 43767 };
                    return epic ? epicModels[urand(0, 4)] : models[urand(0, 4)];
                }

                uint8 skinColor = GetByteValue(PLAYER_BYTES, 0);
                // Male
                if (getGender() == GENDER_MALE)
                {
                    switch (skinColor)
                    {
                        case 12: // White
                        case 13:
                        case 14:
                        case 18: // Completly White
                            return epic ? 43769 : 29409;
                        case 9: // Light Brown
                        case 10:
                        case 11:
                            return epic ? 43770 : 29410;
                        case 6: // Brown
                        case 7:
                        case 8:
                            return epic ? 43768 : 29411;
                        case 0: // Dark
                        case 1:
                        case 2:
                        case 3: // Dark Grey
                        case 4:
                        case 5:
                            return epic ? 43766 : 29412;
                        default: // original - Grey
                            return epic ? 43767 : 8571;
                    }
                }
                // Female
                else
                {
                    switch (skinColor)
                    {
                        case 10: // White
                            return epic ? 43769 : 29409;
                        case 6: // Light Brown
                        case 7:
                            return epic ? 43770 : 29410;
                        case 4: // Brown
                        case 5:
                            return epic ? 43768 : 2941;
                        case 0: // Dark
                        case 1:
                        case 2:
                        case 3:
                            return epic ? 43766 : 29412;
                        default: // original - Grey
                            return epic ? 43767 : 8571;
                    }
                }
            }
            else if (Player::TeamForRace(getRace()) == ALLIANCE)
                return 892;
            else
                return 8571;
            break;
        }
        case FORM_BEAR:
        {
            // check Incarnation
            bool epic = HasAura(102558);

            // Based on Hair color
            if (getRace() == RACE_NIGHTELF)
            {
                // Glyph of the Chameleon
                if (HasAura(107059))
                {
                    uint32 models[] = { 29413, 29414, 29416, 29417, 2281 };
                    uint32 epicModels[] = { 43759, 43756, 43760, 43757, 43758 };
                    return epic ? epicModels[urand(0, 4)] : models[urand(0, 4)];
                }

                uint8 hairColor = GetByteValue(PLAYER_BYTES, 3);
                switch (hairColor)
                {
                    case 0: // Green
                    case 1: // Light Green
                    case 2: // Dark Green
                        return epic ? 43759 : 29413;
                    case 6: // Dark Blue
                        return epic ? 43756 : 29414;
                    case 4: // White
                        return epic ? 43760 : 29416;
                    case 3: // Light Blue
                        return epic ? 43757 : 29415;
                    default: // original - Violet
                        return epic ? 43758 : 2281;
                }
            }
            else if (getRace() == RACE_TROLL)
            {
                // Glyph of the Chameleon
                if (HasAura(107059))
                {
                    uint32 models[] = { 33657, 33659, 33656, 33658, 33655 };
                    uint32 epicModels[] = { 43748, 43750, 43747, 43749, 43746 };
                    return epic ? epicModels[urand(0, 4)] : models[urand(0, 4)];
                }

                uint8 hairColor = GetByteValue(PLAYER_BYTES, 3);
                switch (hairColor)
                {
                    case 0: // Red
                    case 1:
                        return epic ? 43748 : 33657;
                    case 2: // Yellow
                    case 3:
                        return epic ? 43750 : 33659;
                    case 7: // Purple
                    case 10:
                        return epic ? 43747 : 33656;
                    case 8: // White
                    case 9:
                    case 11:
                    case 12:
                        return epic ? 43749 : 33658;
                    default: // original - Blue
                        return epic ? 43746 : 33655;
                }
            }
            else if (getRace() == RACE_WORGEN)
            {
                // Glyph of the Chameleon
                if (HasAura(107059))
                {
                    uint32 models[] = { 33652, 33651, 33653, 33654, 33650 };
                    uint32 epicModels[] = { 43752, 43751, 43754, 43755, 43753 };
                    return epic ? epicModels[urand(0, 4)] : models[urand(0, 4)];
                }

                // Based on Skin color
                uint8 skinColor = GetByteValue(PLAYER_BYTES, 0);
                // Male
                if (getGender() == GENDER_MALE)
                {
                    switch (skinColor)
                    {
                        case 1: // Brown
                            return epic ? 43752 : 33652;
                        case 2: // Black
                        case 7:
                            return epic ? 43751 : 33651;
                        case 4: // Yellow
                            return epic ? 43754 : 33653;
                        case 3: // White
                        case 5:
                            return epic ? 43755 : 33654;
                        default: // original - Gray
                            return epic ? 43753 : 33650;
                    }
                }
                // Female
                else
                {
                    switch (skinColor)
                    {
                        case 5: // Brown
                        case 6:
                            return epic ? 43752 : 33652;
                        case 7: // Black
                        case 8:
                            return epic ? 43751 : 33651;
                        case 3: // yellow
                        case 4:
                            return epic ? 43755 : 33654;
                        case 2: // White
                            return epic ? 43754 : 33653;
                        default: // original - Gray
                            return epic ? 43753 : 33650;
                    }
                }
            }
            // Based on Skin color
            else if (getRace() == RACE_TAUREN)
            {
                // Glyph of the Chameleon
                if (HasAura(107059))
                {
                    uint32 models[] = { 29418, 29419, 29420, 29421, 2289 };
                    uint32 epicModels[] = { 43741, 43743, 43745, 43744, 43742 };
                    return epic ? epicModels[urand(0, 4)] : models[urand(0, 4)];
                }

                uint8 skinColor = GetByteValue(PLAYER_BYTES, 0);
                // Male
                if (getGender() == GENDER_MALE)
                {
                    switch (skinColor)
                    {
                        case 0: // Dark (Black)
                        case 1:
                        case 2:
                            return epic ? 43741 : 29418;
                        case 3: // White
                        case 4:
                        case 5:
                        case 12:
                        case 13:
                        case 14:
                            return epic ? 43743 : 29419;
                        case 9: // Light Brown/Grey
                        case 10:
                        case 11:
                        case 15:
                        case 16:
                        case 17:
                            return epic ? 43745 : 29420;
                        case 18: // Completly White
                            return epic ? 43744 : 29421;
                        default: // original - Brown
                            return epic ? 43742 : 2289;
                    }
                }
                // Female
                else
                {
                    switch (skinColor)
                    {
                        case 0: // Dark (Black)
                        case 1:
                            return epic ? 43741 : 29418;
                        case 2: // White
                        case 3:
                            return epic ? 43743 : 29419;
                        case 6: // Light Brown/Grey
                        case 7:
                        case 8:
                        case 9:
                            return epic ? 43745 : 29420;
                        case 10: // Completly White
                            return epic ? 43744 : 29421;
                        default: // original - Brown
                            return epic ? 43742 : 2289;
                    }
                }
            }
            else if (Player::TeamForRace(getRace()) == ALLIANCE)
                return 2281;
            else
                return 2289;
            break;
        }
        case FORM_FLIGHT:
            if (Player::TeamForRace(getRace()) == ALLIANCE)
                return 20857;
            return 20872;
        case FORM_FLIGHT_EPIC:
            if (Player::TeamForRace(getRace()) == HORDE)
            {
                if (getRace() == RACE_TROLL)
                    return 37730;
                else if (getRace() == RACE_TAUREN)
                    return 21244;
            }
            else if (Player::TeamForRace(getRace()) == ALLIANCE)
            {
                if (getRace() == RACE_NIGHTELF)
                    return 21243;
                else if (getRace() == RACE_WORGEN)
                    return 37729;
            }
            return 21244;
        case FORM_TRAVEL:
            // Glyph of the Cheetah
            if (HasAura(131113))
                return 918;
            if (Player::TeamForRace(getRace()) == ALLIANCE)
                return 40816;
            return 45339;
        case FORM_MOONKIN:
        {
            // Glyph of the Stars
            if (HasAura(114301))
                return 0;

            // check Incarnation
            bool epic = HasAura(102560);

            if (getRace() == RACE_TROLL)
                return epic ? 43789 : 37174;
            else if (getRace() == RACE_TAUREN)
                return epic ? 43786 : 15375;
            else if (getRace() == RACE_NIGHTELF)
                return epic ? 43790 : 15374;
            else if (getRace() == RACE_WORGEN)
                return epic ? 43787 : 37173;
            break;
        }
        case FORM_GHOSTWOLF:
            // Glyph of the Spectral Wolf
            if(HasAura(58135))
                return 21114;
            break;
        case FORM_AQUA:
            // Glyph of the Orca
            if (HasAura(114333))
                return 40815;
            break;
        case FORM_SPIRITOFREDEMPTION:
            // Glyph of the Val'kyr
            if(HasAura(126094))
                return 26101;
            break;
        default:
            break;
    }

    uint32 modelid = 0;
    SpellShapeshiftFormEntry const* formEntry = sSpellShapeshiftFormStore.LookupEntry(form);
    if (formEntry && formEntry->modelID_A)
    {
        // Take the alliance modelid as default
        if (GetTypeId() != TYPEID_PLAYER)
            return formEntry->modelID_A;
        else
        {
            if (Player::TeamForRace(getRace()) == ALLIANCE)
                modelid = formEntry->modelID_A;
            else
                modelid = formEntry->modelID_H;

            // If the player is horde but there are no values for the horde modelid - take the alliance modelid
            if (!modelid && Player::TeamForRace(getRace()) == HORDE)
                modelid = formEntry->modelID_A;
        }
    }

    return modelid;
}

uint32 Unit::GetModelForTotem(PlayerTotemType totemType)
    // TODO FIND for Pandaren horde/alliance
{
    if (totemType == 3211)
    {
        totemType = SUMMON_TYPE_TOTEM_FIRE;
        if(HasAura(147772)) //Glyph of Flaming Serpents
            return 46820;
    }

    switch (getRace())
    {
        case RACE_ORC:
        {
            switch (totemType)
            {
                case SUMMON_TYPE_TOTEM_FIRE:    // fire
                    return 30758;
                case SUMMON_TYPE_TOTEM_EARTH:   // earth
                    return 30757;
                case SUMMON_TYPE_TOTEM_WATER:   // water
                    return 30759;
                case SUMMON_TYPE_TOTEM_AIR:     // air
                    return 30756;
            }
            break;
        }
        case RACE_DWARF:
        {
            switch (totemType)
            {
                case SUMMON_TYPE_TOTEM_FIRE:    // fire
                    return 30754;
                case SUMMON_TYPE_TOTEM_EARTH:   // earth
                    return 30753;
                case SUMMON_TYPE_TOTEM_WATER:   // water
                    return 30755;
                case SUMMON_TYPE_TOTEM_AIR:     // air
                    return 30736;
            }
            break;
        }
        case RACE_TROLL:
        {
            switch (totemType)
            {
                case SUMMON_TYPE_TOTEM_FIRE:    // fire
                    return 30762;
                case SUMMON_TYPE_TOTEM_EARTH:   // earth
                    return 30761;
                case SUMMON_TYPE_TOTEM_WATER:   // water
                    return 30763;
                case SUMMON_TYPE_TOTEM_AIR:     // air
                    return 30760;
                case 3211: // Custom MoP Script - Hack Fix Searing Totem
                    return 30762;
            }
            break;
        }
        case RACE_TAUREN:
        {
            switch (totemType)
            {
                case SUMMON_TYPE_TOTEM_FIRE:    // fire
                    return 4589;
                case SUMMON_TYPE_TOTEM_EARTH:   // earth
                    return 4588;
                case SUMMON_TYPE_TOTEM_WATER:   // water
                    return 4587;
                case SUMMON_TYPE_TOTEM_AIR:     // air
                    return 4590;
            }
            break;
        }
        case RACE_DRAENEI:
        {
            switch (totemType)
            {
                case SUMMON_TYPE_TOTEM_FIRE:    // fire
                    return 19074;
                case SUMMON_TYPE_TOTEM_EARTH:   // earth
                    return 19073;
                case SUMMON_TYPE_TOTEM_WATER:   // water
                    return 19075;
                case SUMMON_TYPE_TOTEM_AIR:     // air
                    return 19071;
            }
            break;
        }
        case RACE_GOBLIN:
        {
            switch (totemType)
            {
                case SUMMON_TYPE_TOTEM_FIRE:    // fire
                    return 30783;
                case SUMMON_TYPE_TOTEM_EARTH:   // earth
                    return 30782;
                case SUMMON_TYPE_TOTEM_WATER:   // water
                    return 30784;
                case SUMMON_TYPE_TOTEM_AIR:     // air
                    return 30781;
            }
            break;
        }
        case RACE_PANDAREN_NEUTRAL:
        case RACE_PANDAREN_ALLI:
        case RACE_PANDAREN_HORDE:
        {
            switch (totemType)
            {
                case SUMMON_TYPE_TOTEM_FIRE:    // fire
                    return 41670;
                case SUMMON_TYPE_TOTEM_EARTH:   // earth
                    return 41669;
                case SUMMON_TYPE_TOTEM_WATER:   // water
                    return 41671;
                case SUMMON_TYPE_TOTEM_AIR:     // air
                    return 41668;
            }
            break;
        }
    }
    return 0;
}

void Unit::JumpTo(float speedXY, float speedZ, bool forward, float angle)
{
    float m_angle = angle ? angle : (forward ? 0 : M_PI);

    if (GetTypeId() == TYPEID_UNIT)
        GetMotionMaster()->MoveJumpTo(m_angle, speedXY, speedZ);
    else
    {
        float vcos = std::cos(m_angle + GetOrientation());
        float vsin = std::sin(m_angle + GetOrientation());
        SendMoveKnockBack(ToPlayer(), speedXY, -speedZ, vcos, vsin);
    }
}

void Unit::JumpTo(WorldObject* obj, float speedZ)
{
    float x, y, z;
    obj->GetContactPoint(this, x, y, z);
    float speedXY = GetExactDist2d(x, y) * 10.0f / speedZ;
    GetMotionMaster()->MoveJump(x, y, z, speedXY, speedZ);
}

bool Unit::HandleSpellClick(Unit* clicker, int8 seatId)
{
    bool res = false;
    uint32 spellClickEntry = GetVehicleKit() ? GetVehicleKit()->GetCreatureEntry() : GetEntry();
    SpellClickInfoMapBounds clickPair = sObjectMgr->GetSpellClickInfoMapBounds(spellClickEntry);
    for (SpellClickInfoContainer::const_iterator itr = clickPair.first; itr != clickPair.second; ++itr)
    {
        //! First check simple relations from clicker to clickee
        if (!itr->second.IsFitToRequirements(clicker, this))
            continue;

        //! Check database conditions
        ConditionList conds = sConditionMgr->GetConditionsForSpellClickEvent(spellClickEntry, itr->second.spellId);
        ConditionSourceInfo info = ConditionSourceInfo(clicker, this);
        if (!sConditionMgr->IsObjectMeetToConditions(info, conds))
            continue;

        Unit* caster = (itr->second.castFlags & NPC_CLICK_CAST_CASTER_CLICKER) ? clicker : this;
        Unit* target = (itr->second.castFlags & NPC_CLICK_CAST_TARGET_CLICKER) ? clicker : this;
        uint64 origCasterGUID = (itr->second.castFlags & NPC_CLICK_CAST_ORIG_CASTER_OWNER) ? GetOwnerGUID() : clicker->GetGUID();

        SpellInfo const* spellEntry = sSpellMgr->GetSpellInfo(itr->second.spellId);
        // if (!spellEntry) should be checked at npc_spellclick load

        if (seatId > -1)
        {
            uint8 i = 0;
            bool valid = false;
            while (i < MAX_SPELL_EFFECTS && !valid)
            {
                if (spellEntry->GetEffect(i, GetSpawnMode())->ApplyAuraName == SPELL_AURA_CONTROL_VEHICLE)
                {
                    valid = true;
                    break;
                }
                ++i;
            }

            if (!valid)
            {
                sLog->outError(LOG_FILTER_SQL, "Spell %u specified in npc_spellclick_spells is not a valid vehicle enter aura!", itr->second.spellId);
                continue;
            }

            if (IsInMap(caster))
                caster->CastCustomSpell(itr->second.spellId, SpellValueMod(SPELLVALUE_BASE_POINT0+i), seatId+1, target, false, NULL, NULL, origCasterGUID);
            else    // This can happen during Player::_LoadAuras
            {
                int32 bp0 = seatId;
                Aura::TryRefreshStackOrCreate(spellEntry, MAX_EFFECT_MASK, this, clicker, &bp0, NULL, origCasterGUID);
            }
            res = true;
        }
        else
        {
            if (IsInMap(caster))
                caster->CastSpell(target, spellEntry, false, NULL, NULL, origCasterGUID);
            else
                Aura::TryRefreshStackOrCreate(spellEntry, MAX_EFFECT_MASK, this, clicker, NULL, NULL, origCasterGUID);
            res = true;
        }
    }

    Creature* creature = ToCreature();
    if (creature && creature->IsAIEnabled)
        creature->AI()->OnSpellClick(clicker);

    if (!res)
        return false;

    return true;
}

void Unit::EnterVehicle(Unit* base, int8 seatId)
{
    CastCustomSpell(VEHICLE_SPELL_RIDE_HARDCODED, SPELLVALUE_BASE_POINT0, seatId+1, base, false);
}

void Unit::_EnterVehicle(Vehicle* vehicle, int8 seatId, AuraApplication const* aurApp)
{
    // Must be called only from aura handler
    if (!isAlive() || GetVehicleKit() == vehicle || vehicle->GetBase()->IsOnVehicle(this))
        return;

    if (m_vehicle)
    {
        if (m_vehicle == vehicle)
        {
            if (seatId >= 0 && seatId != GetTransSeat())
            {
                sLog->outDebug(LOG_FILTER_VEHICLES, "EnterVehicle: %u leave vehicle %u seat %d and enter %d.", GetEntry(), m_vehicle->GetBase()->GetEntry(), GetTransSeat(), seatId);
                ChangeSeat(seatId);
            }
            return;
        }
        else
        {
            sLog->outDebug(LOG_FILTER_VEHICLES, "EnterVehicle: %u exit %u and enter %u.", GetEntry(), m_vehicle->GetBase()->GetEntry(), vehicle->GetBase()->GetEntry());
            ExitVehicle();
        }
    }

    if (aurApp && aurApp->GetRemoveMode())
        return;

    if (Player* player = ToPlayer())
    {
        if (vehicle->GetBase()->GetTypeId() == TYPEID_PLAYER && player->isInCombat())
        {
            vehicle->GetBase()->RemoveAura(const_cast<AuraApplication*>(aurApp));
            return;
        }
    }

    ASSERT(!m_vehicle);

    (void)vehicle->AddPassenger(this, seatId);
}

void Unit::ChangeSeat(int8 seatId, bool next)
{
    if (!m_vehicle)
        return;

    // Don't change if current and new seat are identical
    if (seatId == GetTransSeat())
        return;

    SeatMap::const_iterator seat = (seatId < 0 ? m_vehicle->GetNextEmptySeat(GetTransSeat(), next) : m_vehicle->Seats.find(seatId));
    // The second part of the check will only return true if seatId >= 0. @Vehicle::GetNextEmptySeat makes sure of that.
    if (seat == m_vehicle->Seats.end() || seat->second.Passenger)
        return;

    // Todo: the functions below could be consolidated and refactored to take
    // SeatMap::const_iterator as parameter, to save redundant map lookups.
    m_vehicle->RemovePassenger(this);

    // Set m_vehicle to NULL before adding passenger as adding new passengers is handled asynchronously
    // and someone may call ExitVehicle again before passenger is added to new seat
    Vehicle* veh = m_vehicle;
    m_vehicle = NULL;
    if (!veh->AddPassenger(this, seatId))
        ASSERT(false);
}

void Unit::ExitVehicle(Position const* exitPosition)
{
    //! This function can be called at upper level code to initialize an exit from the passenger's side.
    if (!m_vehicle)
        return;

    GetVehicleBase()->RemoveAurasByType(SPELL_AURA_CONTROL_VEHICLE, GetGUID());

    if (m_vehicle)
        if (m_vehicle->ArePassengersSpawnedByAI())
            _ExitVehicle(exitPosition);

    //! To do:
    //! We need to allow SPELL_AURA_CONTROL_VEHICLE unapply handlers in spellscripts
    //! to specify exit coordinates and either store those per passenger, or we need to
    //! init spline movement based on those coordinates in unapply handlers, and
    //! relocate exiting passengers based on Unit::moveSpline data. Either way,
    //! Coming Soon(TM)
}

void Unit::_ExitVehicle(Position const* exitPosition)
{
    /// It's possible m_vehicle is NULL, when this function is called indirectly from @VehicleJoinEvent::Abort.
    /// In that case it was not possible to add the passenger to the vehicle. The vehicle aura has already been removed
    /// from the target in the aforementioned function and we don't need to do anything else at this point.
    if (!m_vehicle)
        return;

    m_vehicle->RemovePassenger(this);
    Player* player = ToPlayer();

    // If player is on mounted duel and exits the mount should immediately lose the duel
    if (player && player->duel && player->duel->isMounted)
        player->DuelComplete(DUEL_FLED);

    // This should be done before dismiss, because there may be some aura removal
    Vehicle* vehicle = m_vehicle;
    m_vehicle = NULL;

    SetControlled(false, UNIT_STATE_ROOT);      // SMSG_MOVE_FORCE_UNROOT, ~MOVEMENTFLAG_ROOT

    Position pos;
    if (!exitPosition)                          // Exit position not specified
        vehicle->GetBase()->GetPosition(&pos);  // This should use passenger's current position, leaving it as it is now
                                                // because we calculate positions incorrect (sometimes under map)
    else
        pos = *exitPosition;

    // Privent unomal relocation out of map while doing some spline at exit from vehicle
    DisableSpline();

    AddUnitState(UNIT_STATE_MOVE);

    if (GetTypeId() == TYPEID_PLAYER)
        ToPlayer()->SetFallInformation(0, GetPositionZ());
    else if (HasUnitMovementFlag(MOVEMENTFLAG_ROOT))
    {
        //! 5.4.1
        WorldPacket data(SMSG_SPLINE_MOVE_UNROOT, 8);
        ObjectGuid guid = GetGUID();
    
        data.WriteGuidMask<1, 0, 2, 6, 5, 4, 7>(guid);
        data.WriteGuidBytes<2, 4, 7, 3, 6, 5, 1, 0>(guid);
        SendMessageToSet(&data, false);
    }

    Movement::MoveSplineInit init(*this);
    init.MoveTo(pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ());
    init.SetFacing(GetOrientation());
    init.SetTransportExit();
    init.Launch();

    //GetMotionMaster()->MoveFall();            // Enable this once passenger positions are calculater properly (see above)

    if (player)
        player->ResummonPetTemporaryUnSummonedIfAny();

    if (vehicle->GetBase()->HasUnitTypeMask(UNIT_MASK_MINION) && vehicle->GetBase()->GetTypeId() == TYPEID_UNIT)
        if (((Minion*)vehicle->GetBase())->GetOwner() == this)
            vehicle->GetBase()->ToCreature()->DespawnOrUnsummon(1);

    if (HasUnitTypeMask(UNIT_MASK_ACCESSORY))
    {
        // Vehicle just died, we die too
        if (vehicle->GetBase()->getDeathState() == JUST_DIED)
            setDeathState(JUST_DIED);
        // If for other reason we as minion are exiting the vehicle (ejected, master dismounted) - unsummon
        else
            ToTempSummon()->UnSummon(2000); // Approximation
    }
}

void Unit::BuildMovementPacket(ByteBuffer *data) const
{
    *data << uint32(GetUnitMovementFlags());            // movement flags
    *data << uint16(GetExtraUnitMovementFlags());       // 2.3.0
    *data << uint32(getMSTime());                       // time / counter
    *data << GetPositionX();
    *data << GetPositionY();
    *data << GetPositionZMinusOffset();
    *data << GetOrientation();

    bool onTransport = m_movementInfo.t_guid != 0;
    bool hasInterpolatedMovement = m_movementInfo.flags2 & MOVEMENTFLAG2_INTERPOLATED_MOVEMENT;
    bool time3 = false;
    bool swimming = ((GetUnitMovementFlags() & (MOVEMENTFLAG_SWIMMING | MOVEMENTFLAG_FLYING))
        || (m_movementInfo.flags2 & MOVEMENTFLAG2_ALWAYS_ALLOW_PITCHING));
    bool interPolatedTurning = m_movementInfo.flags2 & MOVEMENTFLAG2_INTERPOLATED_TURNING;
    bool jumping = GetUnitMovementFlags() & MOVEMENTFLAG_FALLING;
    bool splineElevation = GetUnitMovementFlags() & MOVEMENTFLAG_SPLINE_ELEVATION;
    bool splineData = false;

    data->WriteBits(GetUnitMovementFlags(), 30);
    data->WriteBits(m_movementInfo.flags2, 12);
    data->WriteBit(onTransport);
    if (onTransport)
    {
        data->WriteBit(hasInterpolatedMovement);
        data->WriteBit(time3);
    }

    data->WriteBit(swimming);
    data->WriteBit(interPolatedTurning);
    if (interPolatedTurning)
        data->WriteBit(jumping);

    data->WriteBit(splineElevation);
    data->WriteBit(splineData);

    data->FlushBits(); // reset bit stream

    *data << uint64(GetGUID());
    *data << uint32(getMSTime());
    *data << float(GetPositionX());
    *data << float(GetPositionY());
    *data << float(GetPositionZ());
    *data << float(GetOrientation());

    if (onTransport)
    {
        if (m_vehicle)
            *data << uint64(m_vehicle->GetBase()->GetGUID());
        else if (GetTransport())
            *data << uint64(GetTransport()->GetGUID());
        else // probably should never happen
            *data << (uint64)0;

        *data << float (GetTransOffsetX());
        *data << float (GetTransOffsetY());
        *data << float (GetTransOffsetZ());
        *data << float (GetTransOffsetO());
        *data << uint8 (GetTransSeat());
        *data << uint32(GetTransTime());
        if (hasInterpolatedMovement)
            *data << int32(0); // Transport Time 2
        if (time3)
            *data << int32(0); // Transport Time 3
    }

    if (swimming)
        *data << (float)m_movementInfo.pitch;

    if (interPolatedTurning)
    {
        *data << (uint32)m_movementInfo.fallTime;
        *data << (float)m_movementInfo.j_zspeed;
        if (jumping)
        {
            *data << (float)m_movementInfo.j_sinAngle;
            *data << (float)m_movementInfo.j_cosAngle;
            *data << (float)m_movementInfo.j_xyspeed;
        }
    }

    if (splineElevation)
        *data << (float)m_movementInfo.splineElevation;
}

void Unit::NearTeleportTo(float x, float y, float z, float orientation, bool casting /*= false*/)
{
    DisableSpline();
    if (GetTypeId() == TYPEID_PLAYER)
    {
        m_anti_JupmTime = sWorld->GetUpdateTime() * 5;
        ToPlayer()->TeleportTo(GetMapId(), x, y, z, orientation, TELE_TO_NOT_LEAVE_TRANSPORT | TELE_TO_NOT_LEAVE_COMBAT | TELE_TO_NOT_UNSUMMON_PET | (casting ? TELE_TO_SPELL : 0));
    }
    else
    {
        Position pos = {x, y, z, orientation};
        SendTeleportPacket(pos);
        UpdatePosition(x, y, z, orientation, true);
        UpdateObjectVisibility();
    }
}

bool Unit::UpdatePosition(float x, float y, float z, float orientation, bool teleport, bool stop/* = false*/)
{
    // prevent crash when a bad coord is sent by the client
    if (!Trinity::IsValidMapCoord(x, y, z, orientation))
    {
        sLog->outDebug(LOG_FILTER_UNITS, "Unit::UpdatePosition(%f, %f, %f) .. bad coordinates!", x, y, z);
        return false;
    }

    bool turn = (GetOrientation() != orientation);
    bool relocated = (teleport || GetPositionX() != x || GetPositionY() != y || GetPositionZ() != z);

    if (turn && !stop)
        RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_TURNING);

    //if (GetTypeId() == TYPEID_PLAYER)
        //sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "UpdatePosition loc(%f %f %f) relocated %i, GetPosition(%f %f %f)", x, y, z, relocated, GetPositionX(), GetPositionY(), GetPositionZ());

    if (relocated)
    {
        if (!stop)
            RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_MOVE);

        // move and update visible state if need
        if (GetTypeId() == TYPEID_PLAYER)
            GetMap()->PlayerRelocation(ToPlayer(), x, y, z, orientation);
        else
            GetMap()->CreatureRelocation(ToCreature(), x, y, z, orientation);

        // code block for underwater state update
        UpdateUnderwaterState(GetMap(), x, y, z);
    }
    else if (turn)
        UpdateOrientation(orientation);

    return (relocated || turn);
}

//! Only server-side orientation update, does not broadcast to client
void Unit::UpdateOrientation(float orientation)
{
    SetOrientation(orientation);
    if (IsVehicle())
        GetVehicleKit()->RelocatePassengers();
}

//! Only server-side height update, does not broadcast to client
void Unit::UpdateHeight(float newZ)
{
    Relocate(GetPositionX(), GetPositionY(), newZ);
    if (IsVehicle())
        GetVehicleKit()->RelocatePassengers();
}

void Unit::SendThreatListUpdate()
{
    if (!getThreatManager().isThreatListEmpty())
    {
        uint32 count = getThreatManager().getThreatList().size();

        sLog->outDebug(LOG_FILTER_UNITS, "WORLD: Send SMSG_THREAT_UPDATE Message");

        ObjectGuid hostileGUID = 0;
        ObjectGuid guid = GetGUID(); 
        ByteBuffer dataBuffer;

        //! 5.4.1
        WorldPacket data(SMSG_THREAT_UPDATE);
        data.WriteGuidMask<3, 2, 1>(guid);
        data.WriteBits(count, 21);
        data.WriteGuidMask<0>(guid);
        std::list<HostileReference*>& tlist = getThreatManager().getThreatList();
        for (std::list<HostileReference*>::const_iterator itr = tlist.begin(); itr != tlist.end(); ++itr)
        {
            hostileGUID = (*itr)->getUnitGuid();
            data.WriteGuidMask<2, 4, 6, 1, 5, 3, 0, 7>(hostileGUID);

            dataBuffer << uint32((*itr)->getThreat() * 100);
            dataBuffer.WriteGuidBytes<0, 2, 5, 1, 4, 3, 6, 7>(hostileGUID);
        }
        data.WriteGuidMask<4, 7, 5, 6>(guid);
        data.FlushBits();
        data.WriteGuidBytes<0, 2>(guid);
        data.append(dataBuffer);
        data.WriteGuidBytes<4, 7, 5, 6, 1, 3>(guid);

        SendMessageToSet(&data, false);
    }
}

void Unit::SendChangeCurrentVictimOpcode(HostileReference* pHostileReference)
{
    if (!getThreatManager().isThreatListEmpty())
    {
        uint32 count = getThreatManager().getThreatList().size();

        sLog->outDebug(LOG_FILTER_UNITS, "WORLD: Send SMSG_HIGHEST_THREAT_UPDATE Message");

        ObjectGuid hostileGUID = 0;
        ObjectGuid HostileReferenceGUID = pHostileReference->getUnitGuid();
        ObjectGuid guid = GetGUID(); 
        ByteBuffer dataBuffer;

        //! 5.4.1
        WorldPacket data(SMSG_HIGHEST_THREAT_UPDATE);
        data.WriteGuidMask<2>(guid);
        data.WriteGuidMask<5>(HostileReferenceGUID);
        data.WriteGuidMask<5>(guid);
        data.WriteGuidMask<7>(HostileReferenceGUID);
        data.WriteGuidMask<7>(guid);
        data.WriteGuidMask<2>(HostileReferenceGUID);
        data.WriteGuidMask<6, 1, 4>(guid);
        data.WriteGuidMask<6, 1>(HostileReferenceGUID);
        data.WriteBits(count, 21);

        std::list<HostileReference*>& tlist = getThreatManager().getThreatList();
        for (std::list<HostileReference*>::const_iterator itr = tlist.begin(); itr != tlist.end(); ++itr)
        {
            hostileGUID = (*itr)->getUnitGuid();

            data.WriteGuidMask<5, 4, 7, 6, 2, 0, 1, 3>(hostileGUID);

            dataBuffer.WriteGuidBytes<2, 1, 5, 4, 3>(hostileGUID);
            dataBuffer << uint32((*itr)->getThreat());
            dataBuffer.WriteGuidBytes<0, 6, 7>(hostileGUID);
        }
        data.WriteGuidMask<0, 4>(HostileReferenceGUID);
        data.WriteGuidMask<3>(guid);
        data.WriteGuidMask<3>(HostileReferenceGUID);
        data.WriteGuidMask<0>(guid);

        data.FlushBits();

        data.append(dataBuffer);
        data.WriteGuidBytes<5>(guid);
        data.WriteGuidBytes<1>(HostileReferenceGUID);
        data.WriteGuidBytes<6, 4, 7>(guid);
        data.WriteGuidBytes<0>(HostileReferenceGUID);
        data.WriteGuidBytes<3>(guid);
        data.WriteGuidBytes<7>(HostileReferenceGUID);
        data.WriteGuidBytes<1, 0, 2>(guid);
        data.WriteGuidBytes<5, 2, 6, 4, 3>(HostileReferenceGUID);

        SendMessageToSet(&data, false);
    }
}

void Unit::SendClearThreatListOpcode()
{
    sLog->outDebug(LOG_FILTER_UNITS, "WORLD: Send SMSG_THREAT_CLEAR Message");

    ObjectGuid guid = GetGUID(); 

    //! 5.4.1
    WorldPacket data(SMSG_THREAT_CLEAR, 8);
    data.WriteGuidMask<0, 2, 5, 3, 1, 4, 6, 7>(guid);
    data.WriteGuidBytes<1, 2, 3, 7, 6, 0, 4, 5>(guid);
    SendMessageToSet(&data, false);
}

void Unit::SendRemoveFromThreatListOpcode(HostileReference* pHostileReference)
{
    sLog->outDebug(LOG_FILTER_UNITS, "WORLD: Send SMSG_THREAT_REMOVE Message");

    ObjectGuid guid = GetGUID(); 
    ObjectGuid RefGUID = pHostileReference->getUnitGuid(); 

    //! 5.4.1
    WorldPacket data(SMSG_THREAT_REMOVE, 8 + 8);
    data.WriteGuidMask<3>(guid);
    data.WriteGuidMask<5, 1, 3, 0>(RefGUID);
    data.WriteGuidMask<5>(guid);
    data.WriteGuidMask<2, 7>(RefGUID);
    data.WriteGuidMask<2, 6>(guid);
    data.WriteGuidMask<4>(RefGUID);
    data.WriteGuidMask<0, 7, 1, 4>(guid);
    data.WriteGuidMask<6>(RefGUID);

    data.WriteGuidBytes<0, 5>(guid);
    data.WriteGuidBytes<4>(RefGUID);
    data.WriteGuidBytes<4, 7>(guid);
    data.WriteGuidBytes<0, 1>(RefGUID);
    data.WriteGuidBytes<3, 1>(guid);
    data.WriteGuidBytes<6, 7, 2, 3, 5>(RefGUID);
    data.WriteGuidBytes<2, 6>(guid);

    SendMessageToSet(&data, false);
}

// baseRage means damage taken when attacker = false
void Unit::RewardRage(float baseRage, bool attacker)
{
    float addRage = 0.0f;

    if (attacker)
    {
        addRage = baseRage;
        // talent who gave more rage on attack
        AddPct(addRage, GetTotalAuraModifier(SPELL_AURA_MOD_RAGE_FROM_DAMAGE_DEALT));
    }

    addRage *= sWorld->getRate(RATE_POWER_RAGE_INCOME);

    ModifyPower(POWER_RAGE, uint32(addRage * 10));
}

void Unit::StopAttackFaction(uint32 faction_id)
{
    if (Unit* victim = getVictim())
    {
        if (victim->getFactionTemplateEntry()->faction == faction_id)
        {
            AttackStop();
            if (IsNonMeleeSpellCasted(false))
                InterruptNonMeleeSpells(false);

            // melee and ranged forced attack cancel
            if (GetTypeId() == TYPEID_PLAYER)
                ToPlayer()->SendAttackSwingResult(ATTACK_SWING_ERROR_DEAD_TARGET);
        }
    }

    AttackerSet const& attackers = getAttackers();
    for (AttackerSet::const_iterator itr = attackers.begin(); itr != attackers.end();)
    {
        if ((*itr)->getFactionTemplateEntry()->faction == faction_id)
        {
            (*itr)->AttackStop();
            itr = attackers.begin();
        }
        else
            ++itr;
    }

    getHostileRefManager().deleteReferencesForFaction(faction_id);

    for (ControlList::const_iterator itr = m_Controlled.begin(); itr != m_Controlled.end(); ++itr)
            (*itr)->StopAttackFaction(faction_id);
}

void Unit::OutDebugInfo() const
{
    sLog->outError(LOG_FILTER_UNITS, "Unit::OutDebugInfo");
    sLog->outInfo(LOG_FILTER_UNITS, "GUID " UI64FMTD ", entry %u, type %u, name %s", GetGUID(), GetEntry(), (uint32)GetTypeId(), GetName());
    sLog->outInfo(LOG_FILTER_UNITS, "OwnerGUID " UI64FMTD ", MinionGUID " UI64FMTD ", CharmerGUID " UI64FMTD ", CharmedGUID " UI64FMTD, GetOwnerGUID(), GetMinionGUID(), GetCharmerGUID(), GetCharmGUID());
    sLog->outInfo(LOG_FILTER_UNITS, "In world %u, unit type mask %u", (uint32)(IsInWorld() ? 1 : 0), m_unitTypeMask);
    if (IsInWorld())
        sLog->outInfo(LOG_FILTER_UNITS, "Mapid %u", GetMapId());

    std::ostringstream o;
    o << "Summon Slot: ";
    for (uint32 i = 0; i < MAX_SUMMON_SLOT; ++i)
        o << m_SummonSlot[i] << ", ";

    sLog->outInfo(LOG_FILTER_UNITS, "%s", o.str().c_str());
    o.str("");

    o << "Controlled List: ";
    for (ControlList::const_iterator itr = m_Controlled.begin(); itr != m_Controlled.end(); ++itr)
        o << (*itr)->GetGUID() << ", ";
    sLog->outInfo(LOG_FILTER_UNITS, "%s", o.str().c_str());
    o.str("");

    o << "Aura List: ";
    for (AuraApplicationMap::const_iterator itr = m_appliedAuras.begin(); itr != m_appliedAuras.end(); ++itr)
        o << itr->first << ", ";
    sLog->outInfo(LOG_FILTER_UNITS, "%s", o.str().c_str());
    o.str("");

    if (IsVehicle())
    {
        o << "Passenger List: ";
        for (SeatMap::iterator itr = GetVehicleKit()->Seats.begin(); itr != GetVehicleKit()->Seats.end(); ++itr)
            if (Unit* passenger = ObjectAccessor::GetUnit(*GetVehicleBase(), itr->second.Passenger))
                o << passenger->GetGUID() << ", ";
        sLog->outInfo(LOG_FILTER_UNITS, "%s", o.str().c_str());
    }

    if (GetVehicle())
        sLog->outInfo(LOG_FILTER_UNITS, "On vehicle %u.", GetVehicleBase()->GetEntry());
}

uint32 Unit::GetRemainingPeriodicAmount(uint64 caster, uint32 spellId, AuraType auraType, uint8 effectIndex) const
{
    uint32 amount = 0;
    AuraEffectList periodicAuras = GetAuraEffectsByType(auraType);
    for (AuraEffectList::const_iterator i = periodicAuras.begin(); i != periodicAuras.end(); ++i)
    {
        if ((*i)->GetCasterGUID() != caster || (*i)->GetId() != spellId || (*i)->GetEffIndex() != effectIndex || !(*i)->GetTotalTicks() || !(*i)->GetBase())
            continue;

        int32 duration = (*i)->GetBase()->GetDuration();
        int32 amplitude = (*i)->GetAmplitude();
        if(!duration || !amplitude)
            continue;

        uint32 ReallTicksleft = uint32(duration / amplitude);
        bool addTick = int32(ReallTicksleft * amplitude) < duration;

        if (addTick)
            ReallTicksleft++;

        amount += uint32(((*i)->GetAmount() * ReallTicksleft) / ((*i)->GetTotalTicks() + 1));
        break;
    }

    return amount;
}

void Unit::SendClearTarget()
{
    WorldPacket data(SMSG_BREAK_TARGET, 8 + 1);
    data.WriteGuidMask<5, 6, 7, 3, 2, 0, 4, 1>(GetObjectGuid());
    data.WriteGuidBytes<1, 6, 2, 4, 5, 3, 7, 0>(GetObjectGuid());
    SendMessageToSet(&data, false);
}

bool Unit::IsVisionObscured(Unit* victim)
{
    Aura* victimAura = NULL;
    Aura* myAura = NULL;
    Unit* victimCaster = NULL;
    Unit* myCaster = NULL;

    AuraEffectList vAuras = victim->GetAuraEffectsByType(SPELL_AURA_INTERFERE_TARGETTING);
    for (AuraEffectList::const_iterator i = vAuras.begin(); i != vAuras.end(); ++i)
    {
        victimAura = (*i)->GetBase();
        victimCaster = victimAura->GetCaster();
        break;
    }
    AuraEffectList myAuras = GetAuraEffectsByType(SPELL_AURA_INTERFERE_TARGETTING);
    for (AuraEffectList::const_iterator i = myAuras.begin(); i != myAuras.end(); ++i)
    {
        myAura = (*i)->GetBase();
        myCaster = myAura->GetCaster();
        break;
    }

    if ((myAura != NULL && myCaster == NULL) || (victimAura != NULL && victimCaster == NULL))
        return false; // Failed auras, will result in crash

    // E.G. Victim is in smoke bomb, and I'm not
    // Spells fail unless I'm friendly to the caster of victim's smoke bomb
    if (victimAura != NULL && myAura == NULL)
    {
        if (IsFriendlyTo(victimCaster))
            return false;
        else
            return true;
    }
    // Victim is not in smoke bomb, while I am
    // Spells fail if my smoke bomb aura's caster is my enemy
    else if (myAura != NULL && victimAura == NULL)
    {
        if (IsFriendlyTo(myCaster))
            return false;
        else
            return true;
    }
    return false;
}

uint32 Unit::GetResistance(SpellSchoolMask mask) const
{
    int32 resist = -1;
    for (int i = SPELL_SCHOOL_NORMAL; i < MAX_SPELL_SCHOOL; ++i)
        if (mask & (1 << i) && (resist < 0 || resist > int32(GetResistance(SpellSchools(i)))))
            resist = int32(GetResistance(SpellSchools(i)));

    // resist value will never be negative here
    return uint32(resist);
}

void CharmInfo::SetIsCommandAttack(bool val)
{
    m_isCommandAttack = val;
}

bool CharmInfo::IsCommandAttack()
{
    return m_isCommandAttack;
}

void CharmInfo::SaveStayPosition()
{
    //! At this point a new spline destination is enabled because of Unit::StopMoving()
    G3D::Vector3 const stayPos = m_unit->movespline->FinalDestination();
    m_stayX = stayPos.x;
    m_stayY = stayPos.y;
    m_stayZ = stayPos.z;
}

void CharmInfo::GetStayPosition(float &x, float &y, float &z)
{
    x = m_stayX;
    y = m_stayY;
    z = m_stayZ;
}

void CharmInfo::SetIsAtStay(bool val)
{
    m_isAtStay = val;
}

bool CharmInfo::IsAtStay()
{
    return m_isAtStay;
}

void CharmInfo::SetIsFollowing(bool val)
{
    m_isFollowing = val;
}

bool CharmInfo::IsFollowing()
{
    return m_isFollowing;
}

void CharmInfo::SetIsReturning(bool val)
{
    m_isReturning = val;
}

bool CharmInfo::IsReturning()
{
    return m_isReturning;
}

void Unit::SetInFront(Unit const* target)
{
    if (!HasUnitState(UNIT_STATE_CANNOT_TURN))
        SetOrientation(GetAngle(target));
}

void Unit::SetFacingTo(float ori)
{
    Movement::MoveSplineInit init(*this);
    init.MoveTo(GetPositionX(), GetPositionY(), GetPositionZMinusOffset());
    init.SetFacing(ori);
    init.Launch();
}

void Unit::SetFacingTo(Unit const* target)
{
    Movement::MoveSplineInit init(*this);
    init.MoveTo(GetPositionX(), GetPositionY(), GetPositionZMinusOffset());
    init.SetFacing(target);
    init.Launch();
}

void Unit::SetFacingToObject(WorldObject* object)
{
    // never face when already moving
    if (!IsStopped())
        return;

    // TODO: figure out under what conditions creature will move towards object instead of facing it where it currently is.
    SetFacingTo(GetAngle(object));
}

bool Unit::SetWalk(bool enable)
{
    if (enable == IsWalking())
        return false;

    if (enable)
        AddUnitMovementFlag(MOVEMENTFLAG_WALKING);
    else
        RemoveUnitMovementFlag(MOVEMENTFLAG_WALKING);

    return true;
}

bool Unit::SetSwim(bool enable)
{
    if (enable == HasUnitMovementFlag(MOVEMENTFLAG_SWIMMING))
        return false;

    if (enable)
        AddUnitMovementFlag(MOVEMENTFLAG_SWIMMING);
    else
        RemoveUnitMovementFlag(MOVEMENTFLAG_SWIMMING);
    return true;
}

bool Unit::SetDisableGravity(bool disable, bool /*packetOnly = false*/)
{
    if (disable == IsLevitating())
        return false;

    if (disable)
        AddUnitMovementFlag(MOVEMENTFLAG_DISABLE_GRAVITY);
    else
        RemoveUnitMovementFlag(MOVEMENTFLAG_DISABLE_GRAVITY);

    return true;
}

bool Unit::SetWaterWalking(bool enable, bool packetOnly)
{
    if (!packetOnly && enable == HasUnitMovementFlag(MOVEMENTFLAG_WATERWALKING))
        return false;

    if (GetTypeId() == TYPEID_PLAYER)
        ToPlayer()->SendMovementSetWaterWalking(enable);

    if (packetOnly)
        return false;

    if (enable)
        AddUnitMovementFlag(MOVEMENTFLAG_WATERWALKING);
    else
        RemoveUnitMovementFlag(MOVEMENTFLAG_WATERWALKING);
    return true;
}

bool Unit::SetCanFly(bool enable)
{
    if (enable == HasUnitMovementFlag(MOVEMENTFLAG_CAN_FLY))
        return false;

    if (GetTypeId() == TYPEID_PLAYER)
        ToPlayer()->SendMovementSetCanFly(enable);

    if (enable)
        AddUnitMovementFlag(MOVEMENTFLAG_CAN_FLY);
    else
        RemoveUnitMovementFlag(MOVEMENTFLAG_CAN_FLY | MOVEMENTFLAG_DESCENDING);

    return true;
}

bool Unit::SetFeatherFall(bool enable, bool packetOnly)
{
    if (!packetOnly && enable == HasUnitMovementFlag(MOVEMENTFLAG_FALLING_SLOW))
        return false;

    if (GetTypeId() == TYPEID_PLAYER)
        ToPlayer()->SendMovementSetFeatherFall(enable);

    if (packetOnly)
        return false;

    if (enable)
        AddUnitMovementFlag(MOVEMENTFLAG_FALLING_SLOW);
    else
        RemoveUnitMovementFlag(MOVEMENTFLAG_FALLING_SLOW);
    return true;
}

bool Unit::SetHover(bool enable, bool packetOnly)
{
    if (!packetOnly)
    {
        if (enable == HasUnitMovementFlag(MOVEMENTFLAG_HOVER))
            return false;

        if (GetTypeId() == TYPEID_PLAYER)
            ToPlayer()->SendMovementSetHover(enable);

        if (enable)
        {
            //! No need to check height on ascent
            AddUnitMovementFlag(MOVEMENTFLAG_HOVER);
            if (float hh = GetFloatValue(UNIT_FIELD_HOVERHEIGHT))
                UpdateHeight(GetPositionZ() + hh);
        }
        else
        {
            RemoveUnitMovementFlag(MOVEMENTFLAG_HOVER);
            if (float hh = GetFloatValue(UNIT_FIELD_HOVERHEIGHT))
            {
                float newZ = GetPositionZ() - hh;
                UpdateAllowedPositionZ(GetPositionX(), GetPositionY(), newZ);
                UpdateHeight(newZ);
            }
        }
    }

    return true;
}

void Unit::FocusTarget(Spell const* focusSpell, uint64 target)
{

    // already focused
    if (_focusSpell)
        return;
    _focusSpell = focusSpell;
    SetUInt64Value(UNIT_FIELD_TARGET, target);
    if (focusSpell->GetSpellInfo()->AttributesEx5 & SPELL_ATTR5_DONT_TURN_DURING_CAST)
        AddUnitState(UNIT_STATE_ROTATING);
}
void Unit::ReleaseFocus(Spell const* focusSpell)
{
    // focused to something else
    if (focusSpell != _focusSpell)
        return;
    _focusSpell = NULL;
    if (Unit* victim = getVictim())
        SetUInt64Value(UNIT_FIELD_TARGET, victim->GetGUID());
    else
        SetUInt64Value(UNIT_FIELD_TARGET, 0);
    if (focusSpell->GetSpellInfo()->AttributesEx5 & SPELL_ATTR5_DONT_TURN_DURING_CAST)
        ClearUnitState(UNIT_STATE_ROTATING);
}

Unit* Unit::GetTargetUnit() const
{
    if (m_curTargetGUID)
        return ObjectAccessor::GetUnit(*this, m_curTargetGUID);
    return NULL;
}

bool Unit::IsSplineEnabled() const
{
    return !movespline->Finalized();
}

void Unit::WriteMovementUpdate(WorldPacket &data) const
{
    WorldSession::WriteMovementInfo(data, const_cast<MovementInfo*>(&m_movementInfo), const_cast<Unit*>(this));
}

void Unit::RemoveSoulSwapDOT(Unit* target)
{
    _SoulSwapDOTList.clear();

    AuraEffectList const mPeriodic = target->GetAuraEffectsByType(SPELL_AURA_PERIODIC_DAMAGE);
    for (AuraEffectList::const_iterator iter = mPeriodic.begin(); iter != mPeriodic.end(); ++iter)
    {
        if (!(*iter)) // prevent crash
            continue;

        if ((*iter)->GetSpellInfo()->SpellFamilyName != SPELLFAMILY_WARLOCK ||
            (*iter)->GetCasterGUID() != GetGUID()) // only warlock spells
            continue;

        if (Aura* aura = (*iter)->GetBase())
        {
            SoulSwapDOT auraStatus;

            auraStatus.Id = aura->GetId();
            auraStatus.duration = aura->GetDuration();
            auraStatus.stackAmount = aura->GetStackAmount();
            auraStatus.amount = (*iter)->GetAmount();
            auraStatus.periodicTimer = (*iter)->GetPeriodicTimer();
            _SoulSwapDOTList.push_back(auraStatus);
        }
    }
}

void Unit::ApplySoulSwapDOT(Unit* target)
{
    for (AuraIdList::iterator iter = _SoulSwapDOTList.begin(); iter != _SoulSwapDOTList.end(); ++iter)
    {
        if (iter == _SoulSwapDOTList.end())
            continue;

        AddAura((*iter).Id, target);

        if (Aura* aura = target->GetAura((*iter).Id, GetGUID()))
        {
            aura->SetStackAmount((*iter).stackAmount);
            aura->SetMaxDuration((*iter).duration);
            aura->SetDuration((*iter).duration);
            if (AuraEffect* eff = aura->GetEffect(EFFECT_0))
            {
                eff->SetPeriodicTimer((*iter).periodicTimer);
                eff->SetCritAmount((*iter).amount * 2);
                eff->SetAmount((*iter).amount);
            }
            
            if (AuraEffect* eff = aura->GetEffect(EFFECT_1))
                eff->SetAmount((*iter).amount);
        }
    }

    _SoulSwapDOTList.clear();
}

void Unit::SendTeleportPacket(Position &destPos)
{
    ObjectGuid guid = GetGUID();
    ObjectGuid transGuid = GetTransGUID();

    WorldPacket data(SMSG_MOVE_TELEPORT, 38);
    data.WriteGuidMask<7>(guid);
    data.WriteBit(0);       // byte33
    data.WriteGuidMask<2, 0>(guid);
    data.WriteBit(transGuid != 0);
    if (transGuid)
        data.WriteGuidMask<4, 3, 5, 7, 0, 2, 6, 1>(transGuid);
    data.WriteGuidMask<5, 1, 3, 6, 4>(guid);

    data.WriteGuidBytes<0>(guid);
    if (transGuid)
        data.WriteGuidBytes<7, 6, 0, 2, 3, 1, 5, 4>(transGuid);
    data.WriteGuidBytes<6, 1>(guid);
    data << uint32(0);  // counter
    data.WriteGuidBytes<7, 5>(guid);
    data << float(destPos.GetPositionX());
    data.WriteGuidBytes<4, 3, 2>(guid);
    data << float(destPos.GetPositionY());
    data << float(NormalizeOrientation(destPos.GetOrientation()));
    data << float(destPos.GetPositionZ());//oldPos.GetPositionZMinusOffset()

    SendMessageToSet(&data, true);
}

bool Unit::HandleCastWhileWalkingAuraProc(Unit* victim, DamageInfo* /*dmgInfoProc*/, AuraEffect* triggeredByAura, SpellInfo const* procSpell, uint32 /*procFlag*/, uint32 /*procEx*/, double cooldown)
{
    SpellInfo const* triggeredByAuraSpell = triggeredByAura->GetSpellInfo();

    uint32 triggered_spell_id = 0;
    Unit* target = victim;
    int32 basepoints0 = 0;

    Item* castItem = triggeredByAura->GetBase()->GetCastItemGUID() && GetTypeId() == TYPEID_PLAYER
        ? ToPlayer()->GetItemByGuid(triggeredByAura->GetBase()->GetCastItemGUID()) : NULL;

    // processed charge only counting case
    if (!triggered_spell_id)
        return true;

    SpellInfo const* triggerEntry = sSpellMgr->GetSpellInfo(triggered_spell_id);

    if (!triggerEntry)
    {
        sLog->outError(LOG_FILTER_UNITS, "Unit::HandleSpellCritChanceAuraProc: Spell %u has non-existing triggered spell %u", triggeredByAuraSpell->Id, triggered_spell_id);
        return false;
    }

    if (G3D::fuzzyGt(cooldown, 0.0) && GetTypeId() == TYPEID_PLAYER && ToPlayer()->HasSpellCooldown(triggered_spell_id))
        return false;

    if (basepoints0)
        CastCustomSpell(target, triggered_spell_id, &basepoints0, NULL, NULL, true, castItem, triggeredByAura);
    else
        CastSpell(target, triggered_spell_id, true, castItem, triggeredByAura);

    if (G3D::fuzzyGt(cooldown, 0.0) && GetTypeId() == TYPEID_PLAYER)
        ToPlayer()->AddSpellCooldown(triggered_spell_id, 0, getPreciseTime() + cooldown);

    return true;
}

bool Unit::HandleSpellModAuraProc(Unit* victim, DamageInfo* /*dmgInfoProc*/, AuraEffect* triggeredByAura, SpellInfo const* procSpell, uint32 /*procFlag*/, uint32 /*procEx*/, double cooldown)
{
    SpellInfo const* triggeredByAuraSpell = triggeredByAura->GetSpellInfo();

    uint32 triggered_spell_id = 0;
    Unit* target = victim;
    int32 basepoints0 = 0;

    Item* castItem = triggeredByAura->GetBase()->GetCastItemGUID() && GetTypeId() == TYPEID_PLAYER
        ? ToPlayer()->GetItemByGuid(triggeredByAura->GetBase()->GetCastItemGUID()) : NULL;

    switch (triggeredByAuraSpell->Id)
    {
        case 77495:         // Mastery: Harmony
        {
            if (!procSpell)
                return false;

            int32 bp = triggeredByAura->GetAmount();
            CastCustomSpell(this, 100977, &bp, &bp, NULL, true);
            return true;
        }
    }

    // processed charge only counting case
    if (!triggered_spell_id)
        return true;

    SpellInfo const* triggerEntry = sSpellMgr->GetSpellInfo(triggered_spell_id);

    if (!triggerEntry)
    {
        sLog->outError(LOG_FILTER_UNITS, "Unit::HandleSpellCritChanceAuraProc: Spell %u has non-existing triggered spell %u", triggeredByAuraSpell->Id, triggered_spell_id);
        return false;
    }

    if (G3D::fuzzyGt(cooldown, 0.0) && GetTypeId() == TYPEID_PLAYER && ToPlayer()->HasSpellCooldown(triggered_spell_id))
        return false;

    if (basepoints0)
        CastCustomSpell(target, triggered_spell_id, &basepoints0, NULL, NULL, true, castItem, triggeredByAura);
    else
        CastSpell(target, triggered_spell_id, true, castItem, triggeredByAura);

    if (G3D::fuzzyGt(cooldown, 0.0) && GetTypeId() == TYPEID_PLAYER)
        ToPlayer()->AddSpellCooldown(triggered_spell_id, 0, getPreciseTime() + cooldown);

    return true;
}

bool Unit::HandleIgnoreAurastateAuraProc(Unit* victim, DamageInfo* /*dmgInfoProc*/, AuraEffect* triggeredByAura, SpellInfo const* procSpell, uint32 /*procFlag*/, uint32 /*procEx*/, double cooldown)
{
    SpellInfo const* triggeredByAuraSpell = triggeredByAura->GetSpellInfo();

    uint32 triggered_spell_id = 0;
    Unit* target = victim;
    int32 basepoints0 = 0;

    Item* castItem = triggeredByAura->GetBase()->GetCastItemGUID() && GetTypeId() == TYPEID_PLAYER
        ? ToPlayer()->GetItemByGuid(triggeredByAura->GetBase()->GetCastItemGUID()) : NULL;

    switch (triggeredByAuraSpell->Id)
    {
        case 44544:         // Fingers of Frost
        {
            RemoveAurasDueToSpell(126084);  // remove second visual
            if (triggeredByAura->GetBase()->ModStackAmount(-1))
                RemoveAura(triggeredByAura->GetBase());
            return true;
        }
    }

    // processed charge only counting case
    if (!triggered_spell_id)
        return true;

    SpellInfo const* triggerEntry = sSpellMgr->GetSpellInfo(triggered_spell_id);

    if (!triggerEntry)
    {
        sLog->outError(LOG_FILTER_UNITS, "Unit::HandleSpellCritChanceAuraProc: Spell %u has non-existing triggered spell %u", triggeredByAuraSpell->Id, triggered_spell_id);
        return false;
    }

    if (G3D::fuzzyGt(cooldown, 0.0) && GetTypeId() == TYPEID_PLAYER && ToPlayer()->HasSpellCooldown(triggered_spell_id))
        return false;

    if (basepoints0)
        CastCustomSpell(target, triggered_spell_id, &basepoints0, NULL, NULL, true, castItem, triggeredByAura);
    else
        CastSpell(target, triggered_spell_id, true, castItem, triggeredByAura);

    if (G3D::fuzzyGt(cooldown, 0.0) && GetTypeId() == TYPEID_PLAYER)
        ToPlayer()->AddSpellCooldown(triggered_spell_id, 0, getPreciseTime() + cooldown);

    return true;
}

void Trinity::BuildChatPacket(WorldPacket& data, ChatData& c, bool coded, bool empty)
{
    std::string message = c.message;
    uint8 langId = c.language;
    if (empty)
        message = "";
    else if (coded && !c.hasCoded)
    {
        c.hasCoded = true;
        c.codedMessage = CodeChatMessage(c.message, c.language);
        message = c.codedMessage;
    }

    if (message.empty())
        langId = LANG_UNIVERSAL;

    data.Initialize(SMSG_MESSAGECHAT);

    data.WriteBit(!langId);
    data.WriteBit(!c.sourceName.size());

    data.WriteBit(!c.groupGuid);
    data.WriteGuidMask<3, 7, 2, 6, 0, 4, 5, 1>(c.groupGuid);

    data.WriteBit(!c.achievementId);
    data.WriteBit(c.byte1495);
    data.WriteBit(!c.addonPrefix.size());

    data.WriteBit(!c.guildGuid);
    data.WriteBit(!c.targetName.size());
    data.WriteGuidMask<1, 6, 0, 5, 2, 4, 7, 3>(c.guildGuid);

    data.WriteBit(!c.targetGuid);
    data.WriteGuidMask<6, 1, 3, 5, 4, 2, 7, 0>(c.targetGuid);

    if (uint32 len = c.sourceName.size())
        data.WriteBits(len, 11);

    data.WriteBit(!c.channelName.size());
    data.WriteBit(c.byte1494);
    data.WriteBit(c.float1490 == 0.0f);
    data.WriteBit(!c.realmId);

    if (uint32 len = c.targetName.size())
        data.WriteBits(len, 11);

    if (uint32 len = c.channelName.size())
        data.WriteBits(len, 7);

    data.WriteBit(!message.size());
    if (uint32 len = message.size())
        data.WriteBits(len, 12);

    data.WriteBit(!c.sourceGuid);
    data.WriteGuidMask<4, 1, 3, 6, 2, 5, 0, 7>(c.sourceGuid);

    data.WriteBit(!c.chatTag);
    if (c.chatTag)
        data.WriteBits(c.chatTag, 9);

    if (uint32 len = c.addonPrefix.size())
        data.WriteBits(len, 5);

    data.WriteString(c.addonPrefix);

    data.WriteGuidBytes<4, 2, 7, 3, 6, 1, 5, 0>(c.groupGuid);

    data.WriteString(c.sourceName);

    data.WriteGuidBytes<7, 4, 1, 3, 0, 6, 5, 2>(c.targetGuid);

    data.WriteGuidBytes<5, 7, 3, 0, 4, 6, 1, 2>(c.guildGuid);

    if (langId)
        data << uint8(langId);

    data.WriteGuidBytes<7, 4, 0, 6, 3, 2, 5, 1>(c.sourceGuid);

    data.WriteString(c.channelName);

    data.WriteString(message);

    data << uint8(c.chatType);

    if (c.achievementId)
        data << uint32(c.achievementId);

    data.WriteString(c.targetName);

    if (c.float1490 != 0.0f)
        data << float(c.float1490);
    if (c.realmId)
        data << uint32(c.realmId);
}

uint32 GetWordWeight(std::string const& word)
{
    uint32 weight = 0;
    for (uint32 i = 0; i < word.size(); ++i)
        weight += (uint8)word[i];
    return weight;
}

bool isCaps(std::wstring wstr)
{
    if (wstr.empty())
        return false;

    uint32 upperCount = 0;
    for (uint32 i = 0; i < wstr.size(); ++i)
        if (std::iswupper(wstr[i]))
            ++upperCount;

    return upperCount * 2 >= wstr.size();
}

std::string Trinity::CodeChatMessage(std::string text, uint32 lang_id)
{
    LanguageWordsMap const* wordMap = GetLanguageWordMap(lang_id);
    if (!wordMap)
        return "";

    std::string convertedMessage;

    Tokenizer t(text, ' ');
    for (uint32 i = 0; i < t.size(); ++i)
    {
        std::string word = t[i];
        std::wstring wword;
        if (!Utf8toWStr(word, wword))
            continue;

        if (wword.empty())
            continue;

        if (std::vector<std::string> const* wordVector = GetLanguageWordsBySize(lang_id, wword.size()))
        {
            std::string replacer = wordVector->at(GetWordWeight(t[i]) % wordVector->size());
            if (isCaps(wword))
                std::transform(replacer.begin(), replacer.end(), replacer.begin(), toupper);

            convertedMessage += replacer + " ";
        }
    }

    return convertedMessage;
}

void Unit::SendDispelFailed(uint64 targetGuid, uint32 spellId, std::list<uint32>& spellList)
{
    ObjectGuid sourceGuid = GetObjectGuid();

    WorldPacket data(SMSG_DISPEL_FAILED, spellList.size() * 4 + 8 + 8 + 1 + 1 + 3);
    data.WriteGuidMask<6>(targetGuid);
    data.WriteBit(0);       // not has power data
    data.WriteGuidMask<3>(sourceGuid);

    data.WriteGuidMask<6>(sourceGuid);
    data.WriteBits(spellList.size(), 22);
    data.WriteGuidMask<0>(sourceGuid);
    data.WriteGuidMask<2>(targetGuid);
    data.WriteGuidMask<2, 4>(sourceGuid);
    data.WriteGuidMask<1>(targetGuid);
    data.WriteGuidMask<1, 5>(sourceGuid);
    data.WriteGuidMask<7, 3, 4, 0>(targetGuid);
    data.WriteGuidMask<7>(sourceGuid);
    data.WriteGuidMask<5>(targetGuid);

    data.WriteGuidBytes<4>(targetGuid);
    data.WriteGuidBytes<4, 2>(sourceGuid);
    data.WriteGuidBytes<2, 3, 7>(targetGuid);
    data.WriteGuidBytes<5>(sourceGuid);
    data.WriteGuidBytes<1>(targetGuid);
    data.WriteGuidBytes<7, 0>(sourceGuid);
    data.WriteGuidBytes<6>(targetGuid);
    data.WriteGuidBytes<1, 3>(sourceGuid);
    data.WriteGuidBytes<0, 5>(targetGuid);
    data.WriteGuidBytes<6>(sourceGuid);

    for (std::list<uint32>::const_iterator itr = spellList.begin(); itr != spellList.end(); ++itr)
        data << uint32(*itr);

    data << uint32(spellId);

    SendMessageToSet(&data, true);
}

void Unit::SendDispelLog(uint64 unitTargetGuid, uint32 spellId, std::list<uint32>& spellList, bool broke, bool stolen)
{
    ObjectGuid casterGuid = GetObjectGuid();

    WorldPacket data(SMSG_SPELLDISPELLOG, 4 + 4 + spellList.size() * 5 + 3 + 1);
    data.WriteGuidMask<1>(casterGuid);
    data.WriteBit(stolen);      // used in dispel, 0 - dispeled, 1 - stolen
    data.WriteGuidMask<7, 2>(casterGuid);
    data.WriteBit(broke);       // 0 - dispel, 1 - break
    data.WriteGuidMask<0>(casterGuid);
    data.WriteGuidMask<3>(unitTargetGuid);

    data.WriteBits(spellList.size(), 22);
    for (uint32 i = 0; i < spellList.size(); ++i)
    {
        data.WriteBit(0);
        data.WriteBit(!stolen);
        data.WriteBit(0);
    }

    data.WriteGuidMask<2, 0>(unitTargetGuid);
    data.WriteBit(0);           // not has power data
    data.WriteGuidMask<3>(casterGuid);

    data.WriteGuidMask<5, 4>(casterGuid);
    data.WriteGuidMask<1, 7, 4, 5, 6>(unitTargetGuid);
    data.WriteGuidMask<6>(casterGuid);

    data.WriteGuidBytes<5>(unitTargetGuid);
    data << uint32(spellId);

    for (std::list<uint32>::const_iterator itr = spellList.begin(); itr != spellList.end(); ++itr)
        data << uint32(*itr);

    data.WriteGuidBytes<3>(casterGuid);
    data.WriteGuidBytes<7, 2>(unitTargetGuid);
    data.WriteGuidBytes<1, 0>(casterGuid);
    data.WriteGuidBytes<3>(unitTargetGuid);
    data.WriteGuidBytes<7>(casterGuid);
    data.WriteGuidBytes<0, 4>(unitTargetGuid);
    data.WriteGuidBytes<2, 6>(casterGuid);
    data.WriteGuidBytes<1>(unitTargetGuid);
    data.WriteGuidBytes<4>(casterGuid);
    data.WriteGuidBytes<6>(unitTargetGuid);
    data.WriteGuidBytes<5>(casterGuid);

    SendMessageToSet(&data, true);
}

void Unit::SendMoveflag2_0x1000_Update(bool on)
{
    if (GetTypeId() != TYPEID_PLAYER)
        return;

    ObjectGuid guid = GetObjectGuid();
    if (on)
    {
        WorldPacket data(SMSG_SET_MOVEFLAG2_0x1000, 8 + 1 + 4);
        data << uint32(0);
        data.WriteGuidMask<5, 1, 3, 0, 2, 6, 7, 4>(guid);
        data.WriteGuidBytes<3, 1, 2, 7, 6, 0, 5, 4>(guid);
        ToPlayer()->SendDirectMessage(&data);
    }
    else
    {
        WorldPacket data(SMSG_UNSET_MOVEFLAG2_0x1000, 8 + 1 + 4);
        data.WriteGuidMask<5, 0, 3, 4, 7, 1, 2, 6>(guid);
        data.WriteGuidBytes<7>(guid);
        data << uint32(0);
        data.WriteGuidBytes<3, 4, 2, 0, 1, 5, 6>(guid);
        ToPlayer()->SendDirectMessage(&data);
    }
}

uint32 Unit::GetDamageCounterInPastSecs(uint32 secs, int type)
{
    if (type >= MAX_DAMAGE_COUNTERS)
        return 0;

    uint32 damage = 0;

    for (uint32 i = 0; i < secs && i < m_damage_counters[type].size(); ++i)
        damage += m_damage_counters[type][i];

    return damage;
}

bool Unit::CheckAndIncreaseCastCounter()
{
    uint32 maxCasts = sWorld->getIntConfig(CONFIG_MAX_SPELL_CASTS_IN_CHAIN);

    if (maxCasts && m_castCounter >= maxCasts)
        return false;

    ++m_castCounter;
    return true;
}

bool Unit::RequiresCurrentSpellsToHolyPower(SpellInfo const* spellProto)
{
    if (!spellProto)
        return false;

    if (spellProto->PowerType == POWER_HOLY_POWER)
        return true;
    else
    {
        for (uint8 i = 0; i < CURRENT_MAX_SPELL; ++i)
        {
            Spell* spell = m_currentSpells[i];

            if (!spell)
                continue;

            SpellInfo const* currentSpellInfo = spell->GetSpellInfo();
            if (currentSpellInfo && currentSpellInfo->PowerType == POWER_HOLY_POWER)
                return true;
        }
    }
    return false;
}

uint8 Unit::HandleHolyPowerCost(uint8 cost, uint8 baseCost)
{
    if (!baseCost)
        return 0;

    uint8 m_baseHolypower = 3;

    if (!cost)
    {
        m_modForHolyPowerSpell = m_baseHolypower / baseCost;
        return 0;
    }

    uint8 m_holyPower = GetPower(POWER_HOLY_POWER);

    if (!m_holyPower)
        return 0;

    if (m_holyPower < m_baseHolypower)
    {
        m_modForHolyPowerSpell = m_holyPower / baseCost;
        return m_holyPower;
    }
    else
    {
        m_modForHolyPowerSpell = m_baseHolypower / baseCost;
        return m_baseHolypower;
    }
}

void DelayCastEvent::Execute(Unit *caster)
{
    Unit* target = caster;

    if (TargetGUID)
        target = ObjectAccessor::GetUnit(*caster, TargetGUID);

    if (!target)
        return;

    caster->CastSpell(target, Spell, false);
};

void Unit::SendSpellCreateVisual(SpellInfo const* spellInfo, Position const* position, Unit* target, uint32 type, uint32 visualId)
{
    bool exist = false;
    uint32 visual = 0;
    uint16 unk1 = 0;
    uint16 unk2 = 0;
    float speed = spellInfo->Speed;
    float positionX = 0.0f;
    float positionY = 0.0f;
    float positionZ = 0.0f;
    bool positionFind = false;
    if (const std::vector<SpellVisual> *spell_visual = sSpellMgr->GetSpellVisual(spellInfo->Id))
    {
        float chance = 100.0f / spell_visual->size();
        for (std::vector<SpellVisual>::const_iterator i = spell_visual->begin(); i != spell_visual->end(); ++i)
        {
            if(i->type != type)
                continue;
            if(i->type == SPELL_VISUAL_TYPE_CUSTOM && i->visual != visualId)
                continue;

            visual = i->visual;
            unk1 = i->unk1;
            unk2 = i->unk2;
            if(i->speed)
                speed = i->speed;
            positionFind = i->position;
            exist = true;
            if(!type && roll_chance_f(chance))
                break;
        }
    }

    if(!exist)
        return;

    if(speed)
    {
        if (target && target != this)
        {
            positionX = target->GetPositionX();
            positionY = target->GetPositionY();
            positionZ = target->GetPositionZ();
        }
        else
        {
            positionX = position->GetPositionX();
            positionY = position->GetPositionY();
            positionZ = position->GetPositionZ();
        }
    }

    ObjectGuid casterGuid = GetObjectGuid();
    ObjectGuid targetGuid = target ? target->GetGUID() : NULL;
    WorldPacket data(SMSG_SPELL_CREATE_VISUAL, 50);
    data.WriteGuidMask<3, 0>(targetGuid);
    data.WriteGuidMask<2, 0>(casterGuid);
    data.WriteGuidMask<4>(targetGuid);
    data.WriteGuidMask<4, 3>(casterGuid);
    data.WriteGuidMask<7>(targetGuid);
    data.WriteGuidMask<6>(casterGuid);
    data.WriteGuidMask<5>(targetGuid);
    data.WriteBit(positionFind);            // hasPosition
    data.WriteGuidMask<5>(casterGuid);
    data.WriteGuidMask<2, 6, 1>(targetGuid);
    data.WriteGuidMask<7, 1>(casterGuid);

    data.WriteGuidBytes<3, 6>(casterGuid);
    data.WriteGuidBytes<5>(targetGuid);
    data.WriteGuidBytes<2>(casterGuid);
    data.WriteGuidBytes<4>(targetGuid);
    data << uint16(unk1);               // word10
    data.WriteGuidBytes<1>(casterGuid);
    data.WriteGuidBytes<0>(targetGuid);
    data << uint32(visual);             //Spell Visual dword14
    data << float(positionZ);           // z
    data << float(speed);               // speed
    data.WriteGuidBytes<3, 2>(targetGuid);
    data << uint16(unk2);               // word34
    data.WriteGuidBytes<0>(casterGuid);
    data.WriteGuidBytes<1, 7>(targetGuid);
    data << float(positionX);           // x
    data.WriteGuidBytes<6>(targetGuid);
    data.WriteGuidBytes<7, 4>(casterGuid);
    data << float(positionY);           // y
    data.WriteGuidBytes<5>(casterGuid);
    SendMessageToSet(&data, true);
}

void Unit::SendFakeAuraUpdate(uint32 auraId, uint32 flags, uint32 diration, uint32 _slot, bool remove)
{
    ObjectGuid targetGuid = GetObjectGuid();

    WorldPacket data(SMSG_AURA_UPDATE);
    data.WriteGuidMask<0>(targetGuid);
    data.WriteBit(0);   // has power unit
    data.WriteBit(0);   // full update
    data.WriteGuidMask<6>(targetGuid);
    /*
    if (hasPowerData) { }
    */
    data.WriteGuidMask<4, 7, 3>(targetGuid);
    data.WriteBits(1, 24);
    data.WriteGuidMask<1, 5, 2>(targetGuid);

    if (data.WriteBit(!remove))
    {
        if (data.WriteBit(!(flags & AFLAG_CASTER)))
            data.WriteGuidMask<2, 3, 4, 0, 1, 6, 7, 5>(targetGuid);
        data.WriteBits(0, 22);  // effect count 2
        data.WriteBits(0, 22);  // effect count
        data.WriteBit(flags & AFLAG_DURATION);  // has duration
        data.WriteBit(flags & AFLAG_DURATION);  // has max duration
    }

    if(remove)
        data << uint8(_slot);
    else
    {
        data << uint16(getLevel());
        if (!(flags & AFLAG_CASTER))
            data.WriteGuidBytes<0, 6, 1, 4, 5, 3, 2, 7>(targetGuid);
        data << uint8(flags);
        if (flags & AFLAG_DURATION)
            data << uint32(diration);

        data << uint8(0); // StackAmount
        data << uint32(1); // Effect mask
        if (flags & AFLAG_DURATION)
            data << uint32(diration);
        data << uint32(auraId);
        data << uint8(_slot);
    }

    /*
    if (hasPowerData) { }
    */

    data.WriteGuidBytes<7, 4, 2, 0, 6, 5, 1, 3>(targetGuid);

    SendMessageToSet(&data, true);
}

bool Unit::GetFreeAuraSlot(uint32& slot)
{
    VisibleAuraMap const* visibleAuras = GetVisibleAuras();
    // lookup for free slots in units visibleAuras
    VisibleAuraMap::const_iterator itr = visibleAuras->find(23);
    for (uint32 freeSlot = 23; freeSlot < MAX_AURAS; ++itr, ++freeSlot)
    {
        if (itr == visibleAuras->end() || itr->first != freeSlot)
        {
            slot = freeSlot;
            break;
        }
    }

    if (slot < MAX_AURAS)
        return true;

    return false;
}

void Unit::SendSpellCooldown(int32 spellId, int32 spell_cooldown, int32 cooldown)
{
    Player* player = ToPlayer();

    if (!player)
    {
        if(Unit* owner = GetAnyOwner())
            player = owner->ToPlayer();
    }

    if (!player)
        return;

    if (!cooldown)
    {
        if(SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spell_cooldown))
            cooldown = spellInfo->RecoveryTime;
    }

    if (!cooldown)
    {
        if(SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId))
            cooldown = spellInfo->RecoveryTime;
    }

    if (!cooldown)
        return;

    ObjectGuid guid = player->GetGUID();
    WorldPacket data(SMSG_SPELL_COOLDOWN, 8 + 1 + 3 + 4 + 4);
    data.WriteGuidMask<4, 7, 6>(guid);
    data.WriteBits(1, 21);
    data.WriteGuidMask<2, 3, 1, 0>(guid);
    data.WriteBit(1);                                  // !hasFlags
    data.WriteGuidMask<5>(guid);
    data.WriteGuidBytes<7, 2, 1, 6, 5, 4, 3, 0>(guid);
    data << uint32(cooldown);
    data << uint32(spellId);

    player->GetSession()->SendPacket(&data);
}

void Unit::SetDynamicPassiveSpells(uint32 spellId, uint32 slot)
{
    //from sniff 1-3 enable spell, 0-2 disable
    SetDynamicUInt32Value(UNIT_DYNAMIC_PASSIVE_SPELLS, slot, spellId);
}

void Unit::SetDynamicWorldEffects(uint32 effect, uint32 slot)
{
    SetDynamicUInt32Value(UNIT_DYNAMIC_WORLD_EFFECTS, slot, effect);
}

uint32 Unit::GetDynamicPassiveSpells(uint32 slot)
{
    //from sniff 1-3 enable spell, 0-2 disable
    return GetDynamicUInt32Value(UNIT_DYNAMIC_PASSIVE_SPELLS, slot);
}

/*
WorldPacket data(SMSG_CANCEL_SCENE);
data.WriteBit(!i->SceneInstanceID);
if (i->SceneInstanceID)
data << uint32(i->SceneInstanceID);
SendPacket(&data);
*/

void Unit::SendSpellScene(uint32 miscValue, Position* pos)
{
    if (GetTypeId() != TYPEID_PLAYER)
        return;

    ObjectGuid casterGuid = /*m_caster->GetObjectGuid()*/0; // not caster something else??? wrong val. could break scean.

    if (const std::vector<SpellScene> *spell_scene = sSpellMgr->GetSpellScene(miscValue))
    {
        for (std::vector<SpellScene>::const_iterator i = spell_scene->begin(); i != spell_scene->end(); ++i)
        {
            WorldPacket data(SMSG_PLAY_SCENE_DATA, 46);
            data.WriteBit(!i->MiscValue);
            data.WriteBit(!i->SceneInstanceID);
            data.WriteBit(!i->ScenePackageId);
            data.WriteBit(!i->hasO);
            data.WriteBit(!i->PlaybackFlags);
            data.WriteBit(!i->bit16);

            data.WriteGuidMask<0, 5, 1, 7, 4, 2, 6, 3>(casterGuid);
            data.WriteGuidBytes<1, 2, 5, 6, 0, 7, 3, 4>(casterGuid);

            data << float(pos->GetPositionY());            // Y

            if(i->SceneInstanceID)
                data << uint32(i->SceneInstanceID);                              // SceneInstanceID
            if(miscValue)
                data << uint32(miscValue);// SceneID
            if(i->hasO)
                data << float(pos->GetOrientation());      // Facing
            if(i->ScenePackageId)
                data << uint32(i->ScenePackageId);                   // SceneScriptPackageID
            if(i->PlaybackFlags)
                data << uint32(i->PlaybackFlags);                              // PlaybackFlags

            data << float(pos->GetPositionX());            // X
            data << float(pos->GetPositionZ());            // Z
            ToPlayer()->GetSession()->SendPacket(&data);
        }
    }
}

void Unit::SendMissileCancel(uint32 spellId, bool cancel)
{
    if (GetTypeId() != TYPEID_PLAYER)
        return;

    ObjectGuid guid = GetObjectGuid();

    WorldPacket data(SMSG_MISSILE_CANCEL, 13);
    data.WriteGuidMask<6, 0, 3, 7, 5, 1, 4>(guid);
    data.WriteBit(cancel);            // Reverse
    data.WriteGuidMask<2>(guid);
    data.WriteGuidBytes<4, 5, 7, 6, 1, 3>(guid);
    data << uint32(spellId);
    data.WriteGuidBytes<0, 2>(guid);
    ToPlayer()->GetSession()->SendPacket(&data);
}

void Unit::SendLossOfControl(Unit* caster, uint32 spellId, uint32 duraction, uint32 rmDuraction, uint32 mechanic, uint32 schoolMask, LossOfControlType type, bool apply)
{
    if (GetTypeId() != TYPEID_PLAYER || !caster)
        return;

    ObjectGuid guid = caster->GetObjectGuid();

    //sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "Unit::SendLossOfControl GetEffectMechanic %i, apply %i, GetId %i duraction %i rmDuraction %i schoolMask %i", mechanic, apply, spellId, duraction, rmDuraction, schoolMask);

    if(apply)
    {
        WorldPacket data(SMSG_ADD_LOSS_OF_CONTROL);
        data.WriteBits(mechanic, 8);     // Mechanic
        data.WriteBits(type, 8);     // Type (interrupt or some other) may be loss control = 0, silence = 0, interrupt = 1, disarm = 0, root = 0
        data.WriteGuidMask<2, 1, 4, 3, 5, 6, 7, 0>(guid);
        data.WriteGuidBytes<3, 1, 4>(guid);
        data << uint32(rmDuraction);        // RemainingDuration (контролирует блокировку баров, скажем если duration = 40000, а это число 10000, то как только останется 10 секунд, на барах пойдет прокрутка, иначе просто затеменено)
        data << uint32(duraction);        // Duration (время действия)
        data.WriteGuidBytes<0>(guid);
        data << uint32(spellId);     // SpellID
        data.WriteGuidBytes<2, 5, 6, 7>(guid);
        data << uint32(schoolMask);     // SchoolMask (для type == interrupt and other)
        ToPlayer()->GetSession()->SendPacket(&data);
    }
    else
    {
        WorldPacket data(SMSG_REMOVE_LOSS_OF_CONTROL);
        data.WriteGuidMask<1, 7, 0, 6, 2, 4, 5>(guid);
        data.WriteBits(type, 8); // Type
        data.WriteGuidMask<3>(guid);
        data.WriteGuidBytes<1, 0, 4, 6, 7>(guid);
        data << uint32(spellId); // SpellID
        data.WriteGuidBytes<3, 5, 2>(guid);
        ToPlayer()->GetSession()->SendPacket(&data);
    }
}

void Unit::SendDisplayToast(uint32 entry, uint8 hasDisplayToastMethod, bool isBonusRoll, uint32 count, uint8 type, Item* item)
{
    if (GetTypeId() != TYPEID_PLAYER)
        return;

    ObjectGuid guid = GetObjectGuid();

    //sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "Unit::SendDisplayToast entry %i, hasDisplayToastMethod %i, isBonusRoll %i count %i type %i", entry, hasDisplayToastMethod, isBonusRoll, count, type);

    WorldPacket data(SMSG_DISPLAY_TOAST);
    data.WriteBit(isBonusRoll);
    data.WriteBits(type, 2);
    data.WriteBit(!hasDisplayToastMethod);
    if (type == 1)
    {
        data.WriteBit(0); // Mailed?
        data << uint32(0); // lootSpecID
        data << uint32(item->GetUpgradeId()); // upgradeId
        data << uint32(0); // Unk Int32_3
        data << uint32(item->GetTemplate()->DisplayInfoID); // displayid
        data << uint32(item->GetItemSuffixFactor()); // Suffix factor
        data << uint32(entry); // itemId
    }
    data << uint32(count); // count
    if (hasDisplayToastMethod)
        data << uint8(hasDisplayToastMethod); // DisplayToastMethod - это принимает значения 1,2,3, 1 и 3 клиент запускает эвенты на лут и бонус лут ролл (EVENT_SHOW_LOOT_TOAST/EVENT_BONUS_ROLL_RESULT, на 2 - запускает чето связанное с батлпетами (EVENT_PET_BATTLE_LOOT_RECEIVED)
    if (type == 2)
        data << uint32(entry); // CurrencyID

    ToPlayer()->GetSession()->SendPacket(&data);
}

void Unit::GeneratePersonalLoot(Creature* creature, Player* anyLooter)
{
    Loot* cLoot = &creature->loot;
    if (creature->lootForPickPocketed)
        creature->lootForPickPocketed = false;
    cLoot->clear();
    uint32 lootid = creature->GetCreatureTemplate()->lootid;

    Map* map = creature->GetMap();
    uint32 spellForRep = 0;
    uint32 spellForBonusLoot = 0;
    uint32 cooldownid = creature->GetEntry();
    uint32 cooldowntype = TYPE_CREATURE;

    //Get boss personal loot template
    PersonalLootData const* plData = sObjectMgr->GetPersonalLootData(creature->GetEntry(), TYPE_CREATURE);
    if(plData)
    {
        spellForRep = plData->lootspellId;
        spellForBonusLoot = plData->bonusspellId;
        if(plData->cooldownid)
        {
            cooldownid = plData->cooldownid;
            lootid = plData->cooldownid;
            cooldowntype = plData->cooldowntype;
        }
    }

    //sLog->outDebug(LOG_FILTER_LOOT, "Unit::GeneratePersonalLoot spellForRep %i spellForBonusLoot %i lootid %i", spellForRep, spellForBonusLoot, lootid);

    //Generate loot for instance
    if(creature->GetInstanceId())
    {
        //Loot for LFR and Flex is personal
        if(map->IsLfr())
        {
            Map::PlayerList const& playerList = map->GetPlayers();
            if (playerList.isEmpty())
                return;

            for (Map::PlayerList::const_iterator itr = playerList.begin(); itr != playerList.end(); ++itr)
            {
                if (Player* player = itr->getSource())
                {
                    if(spellForBonusLoot) //Bonus roll
                        if(!player->IsPlayerLootCooldown(spellForBonusLoot, TYPE_SPELL, creature->GetMap()->GetDifficulty())) //Bonus loot
                            player->CastSpell(player, spellForBonusLoot, false);

                    if(player->IsPlayerLootCooldown(cooldownid, cooldowntype, creature->GetMap()->GetDifficulty()))
                        continue;

                    if(spellForRep) //Gain reputation
                        player->CastSpell(player, spellForRep, false);

                    Loot* loot = &player->personalLoot;
                    loot->clear();
                    loot->personal = true;
                    loot->objType = 1;
                    if(plData)
                        loot->chance = plData->chance;

                    //sLog->outDebug(LOG_FILTER_LOOT, "Unit::GeneratePersonalLoot GetEntry %i GetGUID %i player %i", creature->GetEntry(), loot->GetGUID(), player->GetGUID());

                    if (lootid)
                    {
                        switch(cooldowntype)
                        {
                            case TYPE_GO:
                                loot->FillLoot(lootid, LootTemplates_Gameobject, player, true, false, creature);
                                break;
                            case TYPE_CREATURE:
                                loot->FillLoot(lootid, LootTemplates_Creature, player, true, false, creature);
                                break;
                        }
                    }

                    if(creature->IsAutoLoot())
                        loot->AutoStoreItems();

                    player->AddPlayerLootCooldown(cooldownid, cooldowntype, true, creature->GetMap()->GetDifficulty());
                }
            }
            return;
        }
        else //Other difficulty is raid loot
        {
            //sLog->outDebug(LOG_FILTER_LOOT, "Unit::GeneratePersonalLoot Other difficulty is raid loot");
            Map::PlayerList const& playerList = map->GetPlayers();
            if (playerList.isEmpty())
                return;

            for (Map::PlayerList::const_iterator itr = playerList.begin(); itr != playerList.end(); ++itr)
            {
                if (Player* player = itr->getSource())
                {
                    if(spellForBonusLoot) //Bonus roll
                        if(!player->IsPlayerLootCooldown(spellForBonusLoot, TYPE_SPELL, creature->GetMap()->GetDifficulty())) //Bonus loot
                            player->CastSpell(player, spellForBonusLoot, false);
                }
            }

            cLoot->objType = 1;
            if (uint32 lootid = creature->GetCreatureTemplate()->lootid)
                cLoot->FillLoot(lootid, LootTemplates_Creature, anyLooter, false, false, creature);

            cLoot->generateMoneyLoot(creature->GetCreatureTemplate()->mingold, creature->GetCreatureTemplate()->maxgold);
            return;
        }
    }
    else if(creature->isWorldBoss())
    {
        //sLog->outDebug(LOG_FILTER_LOOT, "Unit::GeneratePersonalLoot isWorldBoss");

        if(spellForBonusLoot) //Bonus roll
            creature->CastSpell(creature, spellForBonusLoot, false);
        if(spellForRep) //Gain reputation
            creature->CastSpell(creature, spellForRep, false);
    }

    cLoot->unlootedCount = creature->GetSizeSaveThreat();
    //sLog->outDebug(LOG_FILTER_LOOT, "Unit::GeneratePersonalLoot unlootedCount %i", cLoot->unlootedCount);

    std::list<uint64>* savethreatlist = creature->GetSaveThreatList();
    for (std::list<uint64>::const_iterator itr = savethreatlist->begin(); itr != savethreatlist->end(); ++itr)
    {
        if (Player* looter = ObjectAccessor::GetPlayer(*creature, (*itr)))
        {
            if(looter->IsPlayerLootCooldown(cooldownid, cooldowntype, creature->GetMap()->GetDifficulty()) || creature->GetZoneId() != looter->GetZoneId())
            {
                --cLoot->unlootedCount;
                continue;
            }

            Loot* loot = &looter->personalLoot;
            loot->clear();
            loot->personal = true;
            loot->objType = 1;
            if(plData)
                loot->chance = plData->chance;

            if (lootid)
            {
                switch(cooldowntype)
                {
                    case TYPE_GO:
                        loot->FillLoot(lootid, LootTemplates_Gameobject, looter, true, false, creature);
                        break;
                    case TYPE_CREATURE:
                        loot->FillLoot(lootid, LootTemplates_Creature, looter, true, false, creature);
                        break;
                }
            }

            if(creature->IsAutoLoot())
            {
                //sLog->outDebug(LOG_FILTER_LOOT, "Unit::GeneratePersonalLoot IsAutoLoot lootGUID %i", loot->GetGUID());
                loot->AutoStoreItems();
                --cLoot->unlootedCount;
            }

            if(creature->isWorldBoss())
                looter->AddPlayerLootCooldown(cooldownid, cooldowntype, true, creature->GetMap()->GetDifficulty());

            //sLog->outDebug(LOG_FILTER_LOOT, "Unit::GeneratePersonalLoot lootGUID %i", loot->GetGUID());
        }
    }
}

void Unit::SendMovementForce(WorldObject* at, float windX, float windY, float windZ, float windSpeed, uint32 windType, bool apply)
{
    if (GetTypeId() != TYPEID_PLAYER)
        return;

    ObjectGuid guid = GetObjectGuid();
    uint32 TriggerGUID = MAKE_PAIR32(at->GetGUID(), 0x1000);

    //sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "Unit::SendMovementForce x %f, y %f, z %f o %f apply %i", windX, windY, windZ, windSpeed, apply);

    if(apply)
    {
        WorldPacket data(SMSG_MOVE_APPLY_MOVEMENT_FORCE);
        data.WriteGuidMask<6, 0, 1, 4, 3>(guid);
        data.WriteBits(windType, 2); // Type
        data.WriteGuidMask<5, 2, 7>(guid);
        data << uint32(TriggerGUID); //guid AT
        data << uint32(0); //Unk1 may TransportID
        data << float(windType ? at->GetPositionX() : windX);
        data.WriteGuidBytes<3>(guid);
        data << uint32(0); // Unk2 may be SequenceIndex
        data.WriteGuidBytes<6, 4>(guid);
        data << float(windType ? at->GetPositionZ() : windZ);
        data << float(windType ? at->GetPositionY() : windY);
        data.WriteGuidBytes<0, 7, 5, 1>(guid);
        data << float(windSpeed);
        data.WriteGuidBytes<2>(guid);
        ToPlayer()->GetSession()->SendPacket(&data);
    }
    else
    {
        WorldPacket data(SMSG_MOVE_REMOVE_MOVEMENT_FORCE);
        data.WriteGuidMask<0, 6, 1, 3, 2, 5, 7, 4>(guid);
        data.WriteGuidBytes<0, 5, 4>(guid);
        data << uint32(TriggerGUID); // guid AT
        data.WriteGuidBytes<2>(guid);
        data << uint32(1); //Unk2 may be SequenceIndex
        data.WriteGuidBytes<3, 6, 1, 7>(guid);
        ToPlayer()->GetSession()->SendPacket(&data);
    }
}

bool Unit::HasSomeCasterAura(uint64 guid)
{
    for (AuraApplicationMap::const_iterator itr = m_appliedAuras.begin(); itr != m_appliedAuras.end(); ++itr)
    {
        if (Aura const* aura = itr->second->GetBase())
            if (aura->GetCasterGUID() == guid)
                return true;
    }
    return false;
}

bool Unit::HasMyAura(uint32 spellId)
{
    AuraList my_Auras = m_my_Auras;

    for (AuraList::const_iterator itr = my_Auras.begin(); itr != my_Auras.end(); ++itr)
    {
        if (Aura const* aura = (*itr))
            if (aura->GetId() == spellId)
                return true;
    }
    return false;
}

bool Unit::HasMyAura(Aura const* hasAura, bool check)
{
    AuraList my_Auras = m_my_Auras;

    for (AuraList::const_iterator itr = my_Auras.begin(); itr != my_Auras.end(); ++itr)
    {
        if (Aura const* aura = (*itr))
            if (aura->GetId() == hasAura->GetId())
            {
                if(aura == hasAura && check)
                    return true;
                else if(aura != hasAura && !check)
                        return true;
            }
    }
    return false;
}

void Unit::RemovePetAndOwnerAura(uint32 spellId, Unit* owner)
{
    for (Unit::ControlList::iterator itr = m_Controlled.begin(); itr != m_Controlled.end();)
    {
        Unit* unit = *itr;
        ++itr;
        if (unit != owner && unit->IsInWorld())
            unit->RemoveAurasDueToSpell(spellId);
    }
    if (this != owner)
        RemoveAurasDueToSpell(spellId);
}

Unit* Unit::GetUnitForLinkedSpell(Unit* caster, Unit* target, uint8 type)
{
    switch (type)
    {
        case LINK_UNIT_TYPE_PET: //1
            return (Unit*)(ToPlayer() ? ToPlayer()->GetPet() : NULL);
            break;
        case LINK_UNIT_TYPE_OWNER: //2
            return GetAnyOwner();
            break;
        case LINK_UNIT_TYPE_CASTER: //3
            return caster;
            break;
        case LINK_UNIT_TYPE_SELECTED: //4
            return ToPlayer() ? ToPlayer()->GetSelectedUnit() : NULL;
            break;
        case LINK_UNIT_TYPE_TARGET: //5
            return target;
            break;
        case LINK_UNIT_TYPE_VICTIM: //6
            return getVictim();
            break;
        case LINK_UNIT_TYPE_ATTACKER: //7
        {
            if (Unit* owner = caster->GetAnyOwner())
                return owner->getAttackerForHelper();
            else
                return caster->getAttackerForHelper();
            break;
        }
        case LINK_UNIT_TYPE_NEARBY: //8
            return SelectNearbyTarget(target);
            break;
        case LINK_UNIT_TYPE_NEARBY_ALLY: //9
            return SelectNearbyAlly(target);
            break;
        case LINK_UNIT_TYPE_ORIGINALCASTER: //10
            return this;
            break;
    }
    return NULL;
}

bool Unit::HasAuraLinkedSpell(Unit* caster, Unit* target, uint8 type, int32 hastalent)
{
    switch (type)
    {
        case LINK_HAS_AURA_ON_CASTER: // 0
        {
            if(!caster)
                return true;
            if(hastalent > 0)
                return !caster->HasAura(hastalent);
            else if(hastalent < 0)
                return caster->HasAura(abs(hastalent));
        }
        case LINK_HAS_AURA_ON_TARGET: // 1
        {
            if(hastalent > 0)
                return target ? !target->HasAura(hastalent) : true ;
            else if(hastalent < 0)
                return target ? target->HasAura(abs(hastalent)) : true ;
        }
        case LINK_HAS_SPELL_ON_CASTER: // 2
        {
            if(!caster)
                return true;
            if(hastalent > 0)
                return !caster->HasSpell(hastalent);
            else if(hastalent < 0)
                return caster->HasSpell(abs(hastalent));
        }
        case LINK_HAS_AURA_ON_OWNER: // 3
        {
            if(!caster)
                return true;
            if(hastalent > 0)
                return caster->GetOwner() ? !caster->GetOwner()->HasAura(hastalent) : true;
            else if(hastalent < 0)
                return caster->GetOwner() ? caster->GetOwner()->HasAura(abs(hastalent)) : true;
        }
        case LINK_HAS_AURATYPE: // 4
            return target ? !target->HasAuraTypeWithCaster(AuraType(hastalent), caster ? caster->GetGUID() : NULL) : true;
        case LINK_HAS_MY_AURA_ON_CASTER: // 5
        {
            if(!caster)
                return false;
            if(hastalent > 0)
                return !caster->HasAura(hastalent, caster->GetGUID());
            else if(hastalent < 0)
                return caster->HasAura(abs(hastalent), caster->GetGUID());
        }
        case LINK_HAS_MY_AURA_ON_TARGET: // 6
        {
            if(hastalent > 0)
                return target ? !target->HasAura(hastalent, caster ? caster->GetGUID() : NULL) : true;
            else if(hastalent < 0)
                return target ? target->HasAura(abs(hastalent), caster ? caster->GetGUID() : NULL) : true;
        }
        case LINK_HAS_AURA_STATE: // 7
        {
            if(hastalent > 0)
                return target ? !target->HasAuraState(AuraStateType(hastalent)) : true;
            else if(hastalent < 0)
                return target ? target->HasAuraState(AuraStateType(abs(hastalent))) : true;
        }
        case LINK_HAS_SPECID: // 8
        {
            if(!caster)
                return true;
            Player* _player = caster->ToPlayer();
            if (!_player)
                return true;
            if(hastalent > 0)
                return _player->GetSpecializationId(_player->GetActiveSpec()) != hastalent;
            else if(hastalent < 0)
                return _player->GetSpecializationId(_player->GetActiveSpec()) == hastalent;
        }
    }
    return true;
}
