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
#include "DatabaseEnv.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "Opcodes.h"
#include "Log.h"
#include "UpdateMask.h"
#include "World.h"
#include "ObjectMgr.h"
#include "SpellMgr.h"
#include "Player.h"
#include "Pet.h"
#include "Unit.h"
#include "Totem.h"
#include "Spell.h"
#include "DynamicObject.h"
#include "Group.h"
#include "UpdateData.h"
#include "MapManager.h"
#include "ObjectAccessor.h"
#include "CellImpl.h"
#include "SharedDefines.h"
#include "LootMgr.h"
#include "VMapFactory.h"
#include "Battleground.h"
#include "Util.h"
#include "TemporarySummon.h"
#include "Vehicle.h"
#include "SpellAuraEffects.h"
#include "ScriptMgr.h"
#include "ConditionMgr.h"
#include "DisableMgr.h"
#include "SpellScript.h"
#include "InstanceScript.h"
#include "SpellInfo.h"
#include "DB2Stores.h"
#include "Battlefield.h"
#include "BattlefieldMgr.h"
#include "GuildMgr.h"
#include "ScenarioMgr.h"

extern pEffect SpellEffects[TOTAL_SPELL_EFFECTS];

SpellDestination::SpellDestination()
{
    _position.Relocate(0, 0, 0, 0);
    _transportGUID = 0;
    _transportOffset.Relocate(0, 0, 0, 0);
}

SpellDestination::SpellDestination(float x, float y, float z, float orientation, uint32 mapId)
{
    _position.Relocate(x, y, z, orientation);
    _transportGUID = 0;
    _position.m_mapId = mapId;
}

SpellDestination::SpellDestination(Position const& pos)
{
    _position.Relocate(pos);
    _transportGUID = 0;
}

SpellDestination::SpellDestination(WorldObject const& wObj)
{
    _transportGUID = wObj.GetTransGUID();
    _transportOffset.Relocate(wObj.GetTransOffsetX(), wObj.GetTransOffsetY(), wObj.GetTransOffsetZ(), wObj.GetTransOffsetO());
    _position.Relocate(wObj);
    _position.SetOrientation(wObj.GetOrientation());
}


SpellCastTargets::SpellCastTargets() : m_elevation(0), m_speed(0), m_strTarget()
{
    m_objectTarget = NULL;
    m_itemTarget = NULL;

    m_objectTargetGUID   = 0;
    m_itemTargetGUID   = 0;
    m_itemTargetEntry  = 0;

    m_targetMask = 0;
}

SpellCastTargets::~SpellCastTargets()
{
}

uint64 SpellCastTargets::GetUnitTargetGUID() const
{
    switch (GUID_HIPART(m_objectTargetGUID))
    {
        case HIGHGUID_PLAYER:
        case HIGHGUID_VEHICLE:
        case HIGHGUID_UNIT:
        case HIGHGUID_PET:
            return m_objectTargetGUID;
        default:
            return 0LL;
    }
}

Unit* SpellCastTargets::GetUnitTarget() const
{
    if (m_objectTarget)
        return m_objectTarget->ToUnit();
    return NULL;
}

void SpellCastTargets::SetUnitTarget(Unit* target)
{
    if (!target)
        return;

    m_objectTarget = target;
    m_objectTargetGUID = target->GetGUID();
    m_targetMask |= TARGET_FLAG_UNIT;
}

uint64 SpellCastTargets::GetGOTargetGUID() const
{
    switch (GUID_HIPART(m_objectTargetGUID))
    {
        case HIGHGUID_TRANSPORT:
        case HIGHGUID_MO_TRANSPORT:
        case HIGHGUID_GAMEOBJECT:
            return m_objectTargetGUID;
        default:
            return 0LL;
    }
}

GameObject* SpellCastTargets::GetGOTarget() const
{
    if (m_objectTarget)
        return m_objectTarget->ToGameObject();
    return NULL;
}


void SpellCastTargets::SetGOTarget(GameObject* target)
{
    if (!target)
        return;

    m_objectTarget = target;
    m_objectTargetGUID = target->GetGUID();
    m_targetMask |= TARGET_FLAG_GAMEOBJECT;
}

uint64 SpellCastTargets::GetCorpseTargetGUID() const
{
    switch (GUID_HIPART(m_objectTargetGUID))
    {
        case HIGHGUID_CORPSE:
            return m_objectTargetGUID;
        default:
            return 0LL;
    }
}

Corpse* SpellCastTargets::GetCorpseTarget() const
{
    if (m_objectTarget)
        return m_objectTarget->ToCorpse();
    return NULL;
}

WorldObject* SpellCastTargets::GetObjectTarget() const
{
    return m_objectTarget;
}

uint64 SpellCastTargets::GetObjectTargetGUID() const
{
    return m_objectTargetGUID;
}

void SpellCastTargets::RemoveObjectTarget()
{
    m_objectTarget = NULL;
    m_objectTargetGUID = 0LL;
    m_targetMask &= ~(TARGET_FLAG_UNIT_MASK | TARGET_FLAG_CORPSE_MASK | TARGET_FLAG_GAMEOBJECT_MASK);
}

void SpellCastTargets::SetItemTarget(Item* item)
{
    if (!item)
        return;

    m_itemTarget = item;
    m_itemTargetGUID = item->GetGUID();
    m_itemTargetEntry = item->GetEntry();
    m_targetMask |= TARGET_FLAG_ITEM;
}

void SpellCastTargets::SetTradeItemTarget(Player* caster)
{
    m_itemTargetGUID = uint64(TRADE_SLOT_NONTRADED);
    m_itemTargetEntry = 0;
    m_targetMask |= TARGET_FLAG_TRADE_ITEM;

    Update(caster);
}

void SpellCastTargets::UpdateTradeSlotItem()
{
    if (m_itemTarget && (m_targetMask & TARGET_FLAG_TRADE_ITEM))
    {
        m_itemTargetGUID = m_itemTarget->GetGUID();
        m_itemTargetEntry = m_itemTarget->GetEntry();
    }
}

SpellDestination const* SpellCastTargets::GetSrc() const
{
    return &m_src;
}

Position const* SpellCastTargets::GetSrcPos() const
{
    return &m_src._position;
}

void SpellCastTargets::SetSrc(float x, float y, float z)
{
    m_src = SpellDestination(x, y, z);
    m_targetMask |= TARGET_FLAG_SOURCE_LOCATION;
}

void SpellCastTargets::SetSrc(Position const& pos)
{
    m_src = SpellDestination(pos);
    m_targetMask |= TARGET_FLAG_SOURCE_LOCATION;
}

void SpellCastTargets::SetSrc(WorldObject const& wObj)
{
    m_src = SpellDestination(wObj);
    m_targetMask |= TARGET_FLAG_SOURCE_LOCATION;
}

void SpellCastTargets::ModSrc(Position const& pos)
{
    ASSERT(m_targetMask & TARGET_FLAG_SOURCE_LOCATION);

    if (m_src._transportGUID)
    {
        Position offset;
        m_src._position.GetPositionOffsetTo(pos, offset);
        m_src._transportOffset.RelocateOffset(offset);
    }
    m_src._position.Relocate(pos);
}

void SpellCastTargets::RemoveSrc()
{
    m_targetMask &= ~(TARGET_FLAG_SOURCE_LOCATION);
}

SpellDestination const* SpellCastTargets::GetDst() const
{
    return &m_dst;
}

WorldLocation const* SpellCastTargets::GetDstPos() const
{
    return &m_dst._position;
}

void SpellCastTargets::SetDst(float x, float y, float z, float orientation, uint32 mapId)
{
    m_dst = SpellDestination(x, y, z, orientation, mapId);
    m_targetMask |= TARGET_FLAG_DEST_LOCATION;
}

void SpellCastTargets::SetDst(Position const& pos)
{
    m_dst = SpellDestination(pos);
    m_targetMask |= TARGET_FLAG_DEST_LOCATION;
}

void SpellCastTargets::SetDst(WorldObject const& wObj)
{
    m_dst = SpellDestination(wObj);
    m_targetMask |= TARGET_FLAG_DEST_LOCATION;
}

void SpellCastTargets::SetDst(SpellCastTargets const& spellTargets)
{
    m_dst = spellTargets.m_dst;
    m_targetMask |= TARGET_FLAG_DEST_LOCATION;
}

void SpellCastTargets::ModDst(Position const& pos)
{
    ASSERT(m_targetMask & TARGET_FLAG_DEST_LOCATION);

    if (m_dst._transportGUID)
    {
        Position offset;
        m_dst._position.GetPositionOffsetTo(pos, offset);
        m_dst._transportOffset.RelocateOffset(offset);
    }
    m_dst._position.Relocate(pos);
}

void SpellCastTargets::RemoveDst()
{
    m_targetMask &= ~(TARGET_FLAG_DEST_LOCATION);
}

void SpellCastTargets::Update(Unit* caster)
{
    m_objectTarget = m_objectTargetGUID ? ((m_objectTargetGUID == caster->GetGUID()) ? caster : ObjectAccessor::GetWorldObject(*caster, m_objectTargetGUID)) : NULL;

    m_itemTarget = NULL;
    if (caster->GetTypeId() == TYPEID_PLAYER)
    {
        Player* player = caster->ToPlayer();
        if (m_targetMask & TARGET_FLAG_ITEM)
            m_itemTarget = player->GetItemByGuid(m_itemTargetGUID);
        else if (m_targetMask & TARGET_FLAG_TRADE_ITEM)
            if (m_itemTargetGUID == TRADE_SLOT_NONTRADED) // here it is not guid but slot. Also prevents hacking slots
                if (TradeData* pTrade = player->GetTradeData())
                    m_itemTarget = pTrade->GetTraderData()->GetItem(TRADE_SLOT_NONTRADED);

        if (m_itemTarget)
            m_itemTargetEntry = m_itemTarget->GetEntry();
    }

    // update positions by transport move
    if (HasSrc() && m_src._transportGUID)
    {
        if (WorldObject* transport = ObjectAccessor::GetWorldObject(*caster, m_src._transportGUID))
        {
            m_src._position.Relocate(transport);
            m_src._position.RelocateOffset(m_src._transportOffset);
        }
    }

    if (HasDst() && m_dst._transportGUID)
    {
        if (WorldObject* transport = ObjectAccessor::GetWorldObject(*caster, m_dst._transportGUID))
        {
            m_dst._position.Relocate(transport);
            m_dst._position.RelocateOffset(m_dst._transportOffset);
        }
    }
}

void SpellCastTargets::OutDebug() const
{
    if (!m_targetMask)
        sLog->outInfo(LOG_FILTER_SPELLS_AURAS, "No targets");

    sLog->outInfo(LOG_FILTER_SPELLS_AURAS, "target mask: %u", m_targetMask);
    if (m_targetMask & (TARGET_FLAG_UNIT_MASK | TARGET_FLAG_CORPSE_MASK | TARGET_FLAG_GAMEOBJECT_MASK))
        sLog->outInfo(LOG_FILTER_SPELLS_AURAS, "Object target: " UI64FMTD, m_objectTargetGUID);
    if (m_targetMask & TARGET_FLAG_ITEM)
        sLog->outInfo(LOG_FILTER_SPELLS_AURAS, "Item target: " UI64FMTD, m_itemTargetGUID);
    if (m_targetMask & TARGET_FLAG_TRADE_ITEM)
        sLog->outInfo(LOG_FILTER_SPELLS_AURAS, "Trade item target: " UI64FMTD, m_itemTargetGUID);
    if (m_targetMask & TARGET_FLAG_SOURCE_LOCATION)
        sLog->outInfo(LOG_FILTER_SPELLS_AURAS, "Source location: transport guid:" UI64FMTD " trans offset: %s position: %s", m_src._transportGUID, m_src._transportOffset.ToString().c_str(), m_src._position.ToString().c_str());
    if (m_targetMask & TARGET_FLAG_DEST_LOCATION)
        sLog->outInfo(LOG_FILTER_SPELLS_AURAS, "Destination location: transport guid:" UI64FMTD " trans offset: %s position: %s", m_dst._transportGUID, m_dst._transportOffset.ToString().c_str(), m_dst._position.ToString().c_str());
    if (m_targetMask & TARGET_FLAG_STRING)
        sLog->outInfo(LOG_FILTER_SPELLS_AURAS, "String: %s", m_strTarget.c_str());
    sLog->outInfo(LOG_FILTER_SPELLS_AURAS, "speed: %f", m_speed);
    sLog->outInfo(LOG_FILTER_SPELLS_AURAS, "elevation: %f", m_elevation);
}

SpellValue::SpellValue(SpellInfo const* proto, uint8 diff)
{
    for (uint32 i = 0; i < MAX_SPELL_EFFECTS; ++i)
    {
        EffectBasePoints[i] = proto->GetEffect(i, diff)->BasePoints;
        LockBasePoints[i] = false;
    }
    MaxAffectedTargets = proto->GetMaxAffectedTargets(diff);
    RadiusMod = 1.0f;
    AuraStackAmount = 1;
}

Spell::Spell(Unit* caster, SpellInfo const* info, TriggerCastFlags triggerFlags, uint64 originalCasterGUID, bool skipCheck, bool replaced) :
m_spellInfo(info),
m_caster((info->AttributesEx6 & SPELL_ATTR6_CAST_BY_CHARMER && caster->GetCharmerOrOwner()) ? caster->GetCharmerOrOwner() : caster),
m_customError(SPELL_CUSTOM_ERROR_NONE), m_skipCheck(skipCheck), m_spellMissMask(0), m_selfContainer(NULL), m_spellDynObjGuid(NULL),
m_referencedFromCurrentSpell(false), m_executedCurrently(false), m_needComboPoints(info->NeedsComboPoints()), hasPredictedDispel(NULL),
m_comboPointGain(0), m_delayStart(0), m_delayAtDamageCount(0), m_count_dispeling(0), m_applyMultiplierMask(0), m_auraScaleMask(0),
m_CastItem(NULL), m_castItemGUID(0), unitTarget(NULL), m_originalTarget(NULL), itemTarget(NULL), gameObjTarget(NULL), focusObject(NULL),
m_cast_count(0), m_glyphIndex(0), m_preCastSpell(0), m_triggeredByAuraSpell(NULL), m_spellAura(NULL), find_target(false), m_spellState(SPELL_STATE_NULL),
m_runesState(0), m_powerCost(0), m_casttime(0), m_timer(0), m_channelTargetEffectMask(0), _triggeredCastFlags(triggerFlags), m_spellValue(NULL), m_currentExecutedEffect(SPELL_EFFECT_NONE),
m_absorb(0), m_resist(0), m_blocked(0), m_interupted(false), m_effect_targets(NULL), m_replaced(replaced), m_triggeredByAura(NULL), m_originalTargetGUID(0)
{
    m_diffMode = m_caster->GetMap() ? m_caster->GetMap()->GetSpawnMode() : 0;
    m_spellValue = new SpellValue(m_spellInfo, m_diffMode);

    LoadAttrDummy();

    // Get data for type of attack
    switch (m_spellInfo->DmgClass)
    {
        case SPELL_DAMAGE_CLASS_MELEE:
            if (AttributesCustomEx3 & SPELL_ATTR3_REQ_OFFHAND)
                m_attackType = OFF_ATTACK;
            else
                m_attackType = BASE_ATTACK;
            break;
        case SPELL_DAMAGE_CLASS_RANGED:
            m_attackType = m_spellInfo->IsRangedWeaponSpell() ? RANGED_ATTACK : BASE_ATTACK;
            break;
        default:
                                                            // Wands
            if (AttributesCustomEx2 & SPELL_ATTR2_AUTOREPEAT_FLAG)
                m_attackType = RANGED_ATTACK;
            else
                m_attackType = BASE_ATTACK;
            break;
    }

    m_spellSchoolMask = info->GetSchoolMask();           // Can be override for some spell (wand shoot for example)

    if (m_attackType == RANGED_ATTACK)
        // wand case
        if ((m_caster->getClassMask() & CLASSMASK_WAND_USERS) != 0 && m_caster->GetTypeId() == TYPEID_PLAYER)
            if (Item* pItem = m_caster->ToPlayer()->GetWeaponForAttack(RANGED_ATTACK))
                m_spellSchoolMask = SpellSchoolMask(1 << pItem->GetTemplate()->DamageType);

    if (originalCasterGUID)
        m_originalCasterGUID = originalCasterGUID;
    else
        m_originalCasterGUID = m_caster->GetGUID();

    if (m_originalCasterGUID == m_caster->GetGUID())
        m_originalCaster = m_caster;
    else
    {
        m_originalCaster = ObjectAccessor::GetUnit(*m_caster, m_originalCasterGUID);
        if (m_originalCaster && !m_originalCaster->IsInWorld())
            m_originalCaster = NULL;
    }

    if (!m_replaced && info->AttributesEx4 & SPELL_ATTR4_TRIGGERED)
        _triggeredCastFlags |= TRIGGERED_FULL_MASK;
    if(m_replaced || AttributesCustomEx2 & SPELL_ATTR2_AUTOREPEAT_FLAG) //If spell casted as replaced, enable proc from him
        _triggeredCastFlags &= ~TRIGGERED_DISALLOW_PROC_EVENTS;
    if (info->AttributesCu & SPELL_ATTR0_CU_DOESENT_INTERRUPT_CHANNELING)
        _triggeredCastFlags |= TRIGGERED_CAST_DIRECTLY;

    //Auto Shot & Shoot (wand)
    m_autoRepeat = m_spellInfo->IsAutoRepeatRangedSpell();

    canHitTargetInLOS = AttributesCustomEx2 & SPELL_ATTR2_CAN_TARGET_NOT_IN_LOS;

    // Determine if spell can be reflected back to the caster
    // Patch 1.2 notes: Spell Reflection no longer reflects abilities
    m_canReflect = m_spellInfo->DmgClass == SPELL_DAMAGE_CLASS_MAGIC && !(AttributesCustom & SPELL_ATTR0_ABILITY)
        && !(AttributesCustomEx & SPELL_ATTR1_CANT_BE_REFLECTED) && !(AttributesCustom & SPELL_ATTR0_UNAFFECTED_BY_INVULNERABILITY)
        && !(AttributesCustomEx3 & SPELL_ATTR3_ONLY_TARGET_PLAYERS)
        && !m_spellInfo->IsPassive() && !m_spellInfo->IsPositive();

    CleanupTargetList();
    memset(m_effectExecuteData, 0, MAX_SPELL_EFFECTS * sizeof(EffectExecuteData*));

    for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
    {
        damageCalculate[i] = false;
        saveDamageCalculate[i] = 0;
        m_destTargets[i] = SpellDestination(*m_caster);
    }
    m_damage = 0;
    m_healing = 0;
    m_final_damage = 0;
    m_absorb = 0;
    m_resist = 0;
    m_blocked = 0;
    m_addpower = 0;
    m_addptype = -1;

    m_itemEntry = 0;
    m_caster->GetPosition(&visualPos);
}

Spell::~Spell()
{
    // unload scripts
    while (!m_loadedScripts.empty())
    {
        std::list<SpellScript*>::iterator itr = m_loadedScripts.begin();
        (*itr)->_Unload();
        delete (*itr);
        m_loadedScripts.erase(itr);
    }

    if (m_referencedFromCurrentSpell && m_selfContainer && *m_selfContainer == this)
    {
        // Clean the reference to avoid later crash.
        // If this error is repeating, we may have to add an ASSERT to better track down how we get into this case.
        sLog->outError(LOG_FILTER_SPELLS_AURAS, "SPELL: deleting spell for spell ID %u. However, spell still referenced.", m_spellInfo->Id);
        *m_selfContainer = NULL;
    }

    if (m_caster && m_caster->GetTypeId() == TYPEID_PLAYER)
        ASSERT(m_caster->ToPlayer()->m_spellModTakingSpell != this);
    delete m_spellValue;

    CheckEffectExecuteData();
}

void Spell::InitExplicitTargets(SpellCastTargets const& targets)
{
    m_targets = targets;
    m_originalTarget = targets.GetUnitTarget();
    m_originalTargetGUID = m_originalTarget ? m_originalTarget->GetGUID() : 0;
    // this function tries to correct spell explicit targets for spell
    // client doesn't send explicit targets correctly sometimes - we need to fix such spells serverside
    // this also makes sure that we correctly send explicit targets to client (removes redundant data)
    uint32 neededTargets = m_spellInfo->GetExplicitTargetMask();

    if (WorldObject* target = m_targets.GetObjectTarget())
    {
        // check if object target is valid with needed target flags
        // for unit case allow corpse target mask because player with not released corpse is a unit target
        if ((target->ToUnit() && !(neededTargets & (TARGET_FLAG_UNIT_MASK | TARGET_FLAG_CORPSE_MASK)))
            || (target->ToGameObject() && !(neededTargets & TARGET_FLAG_GAMEOBJECT_MASK))
            || (target->ToCorpse() && !(neededTargets & TARGET_FLAG_CORPSE_MASK)))
            m_targets.RemoveObjectTarget();

        if (Unit* _target = m_targets.GetUnitTarget())
            if (target->GetGUID() != m_caster->GetGUID())
            {
                bool canChangeTarget = false;
                for (uint8 i = 0; i < 5; ++i)
                {
                    if (!m_spellInfo->Effects[i].Effect)
                        break;

                    if ((m_spellInfo->Effects[i].TargetA.GetTarget() == TARGET_UNIT_TARGET_ANY ||
                        m_spellInfo->Effects[i].TargetA.GetTarget() == TARGET_DEST_TARGET_ANY) &&
                        m_spellInfo->IsPositiveEffect(i))
                    {
                        canChangeTarget = true;
                        break;
                    } 
                }

                if (canChangeTarget && !(AttributesCustomEx2 & SPELL_ATTR2_CAN_TARGET_NOT_IN_LOS) && !m_caster->IsWithinLOSInMap(_target) && !_target->IsFriendlyTo(m_caster))
                    m_targets.SetUnitTarget(m_caster);
            }
    }
    else
    {
        // try to select correct unit target if not provided by client or by serverside cast
        if (neededTargets & (TARGET_FLAG_UNIT_MASK))
        {
            Unit* unit = NULL;
            // try to use player selection as a target
            if (Player* playerCaster = m_caster->ToPlayer())
            {
                // selection has to be found and to be valid target for the spell
                if (Unit* selectedUnit = ObjectAccessor::GetUnit(*m_caster, playerCaster->GetSelection()))
                    if (m_spellInfo->CheckExplicitTarget(m_caster, selectedUnit) == SPELL_CAST_OK)
                        unit = selectedUnit;
            }
            // try to use attacked unit as a target
            else if ((m_caster->GetTypeId() == TYPEID_UNIT) && neededTargets & (TARGET_FLAG_UNIT_ENEMY | TARGET_FLAG_UNIT))
                unit = m_caster->getVictim();

            // didn't find anything - let's use self as target
            if (!unit && neededTargets & (TARGET_FLAG_UNIT_RAID | TARGET_FLAG_UNIT_PARTY | TARGET_FLAG_UNIT_ALLY))
                unit = m_caster;

            if(unit)
                m_targets.SetUnitTarget(unit);
        }
    }

    // check if spell needs dst target
    if (neededTargets & TARGET_FLAG_DEST_LOCATION)
    {
        // and target isn't set
        if (!m_targets.HasDst())
        {
            // try to use unit target if provided
            if (WorldObject* target = targets.GetObjectTarget())
                m_targets.SetDst(*target);
            // or use self if not available
            else
                m_targets.SetDst(*m_caster);
        }
    }
    else
        m_targets.RemoveDst();

    if (neededTargets & TARGET_FLAG_SOURCE_LOCATION)
    {
        if (!targets.HasSrc())
            m_targets.SetSrc(*m_caster);
    }
    else
        m_targets.RemoveSrc();
}

void Spell::SelectExplicitTargets()
{
    // here go all explicit target changes made to explicit targets after spell prepare phase is finished
    if (Unit* target = m_targets.GetUnitTarget())
    {
        // check for explicit target redirection, for Grounding Totem for example
        if (m_spellInfo->GetExplicitTargetMask() & TARGET_FLAG_UNIT_ENEMY
            || (m_spellInfo->GetExplicitTargetMask() & TARGET_FLAG_UNIT && !m_spellInfo->IsPositive()))
        {
            Unit* redirect;
            switch (m_spellInfo->DmgClass)
            {
                case SPELL_DAMAGE_CLASS_MAGIC:
                    redirect = m_caster->GetMagicHitRedirectTarget(target, m_spellInfo);
                    break;
                case SPELL_DAMAGE_CLASS_MELEE:
                case SPELL_DAMAGE_CLASS_RANGED:
                    redirect = m_caster->GetMeleeHitRedirectTarget(target, m_spellInfo);
                    break;
                default:
                    redirect = NULL;
                    break;
            }
            if (redirect && (redirect != target))
            {
                canHitTargetInLOS = true;
                m_targets.SetUnitTarget(redirect);
            }
        }
    }
}

void Spell::SelectSpellTargets()
{
    // select targets for cast phase
    SelectExplicitTargets();

    uint32 processedAreaEffectsMask = 0;
    for (uint32 i = 0; i < MAX_SPELL_EFFECTS; ++i)
    {
        // not call for empty effect.
        // Also some spells use not used effect targets for store targets for dummy effect in triggered spells
        if (!m_spellInfo->GetEffect(i, m_diffMode)->IsEffect())
            continue;

        if (m_spellMissMask & ((1 << SPELL_MISS_MISS) | (1 << SPELL_MISS_IMMUNE)))
            if (m_spellInfo->GetEffect(i, m_diffMode)->TargetA.GetTarget() == TARGET_UNIT_CASTER)
                continue;

        // set expected type of implicit targets to be sent to client
        uint32 implicitTargetMask = GetTargetFlagMask(m_spellInfo->GetEffect(i, m_diffMode)->TargetA.GetObjectType()) | GetTargetFlagMask(m_spellInfo->GetEffect(i, m_diffMode)->TargetB.GetObjectType());
        if (implicitTargetMask & TARGET_FLAG_UNIT)
            m_targets.SetTargetFlag(TARGET_FLAG_UNIT);
        if (implicitTargetMask & (TARGET_FLAG_GAMEOBJECT | TARGET_FLAG_GAMEOBJECT_ITEM))
            m_targets.SetTargetFlag(TARGET_FLAG_GAMEOBJECT);

        SelectEffectImplicitTargets(SpellEffIndex(i), m_spellInfo->GetEffect(i, m_diffMode)->TargetA, processedAreaEffectsMask);
        SelectEffectImplicitTargets(SpellEffIndex(i), m_spellInfo->GetEffect(i, m_diffMode)->TargetB, processedAreaEffectsMask);

        // Select targets of effect based on effect type
        // those are used when no valid target could be added for spell effect based on spell target type
        // some spell effects use explicit target as a default target added to target map (like SPELL_EFFECT_LEARN_SPELL)
        // some spell effects add target to target map only when target type specified (like SPELL_EFFECT_WEAPON)
        // some spell effects don't add anything to target map (confirmed with sniffs) (like SPELL_EFFECT_DESTROY_ALL_TOTEMS)
        SelectEffectTypeImplicitTargets(i);

        if (m_targets.HasDst())
            AddDestTarget(*m_targets.GetDst(), i);
        
        for (std::list<TargetInfo>::iterator ihit = m_UniqueTargetInfo.begin(); ihit != m_UniqueTargetInfo.end(); ++ihit)
            m_spellMissMask |= (1 << ihit->missCondition);

        if (m_spellInfo->IsChanneled())
        {
            uint8 mask = (1 << i);
            for (std::list<TargetInfo>::iterator ihit = m_UniqueTargetInfo.begin(); ihit != m_UniqueTargetInfo.end(); ++ihit)
            {
                if (ihit->effectMask & mask)
                {
                    m_channelTargetEffectMask |= mask;
                    break;
                }
            }
        }
        else if (m_auraScaleMask)
        {
            bool checkLvl = !m_UniqueTargetInfo.empty();
            for (std::list<TargetInfo>::iterator ihit = m_UniqueTargetInfo.begin(); ihit != m_UniqueTargetInfo.end();)
            {
                // remove targets which did not pass min level check
                if (m_auraScaleMask && ihit->effectMask == m_auraScaleMask)
                {
                    // Do not check for selfcast
                    if (!ihit->scaleAura && ihit->targetGUID != m_caster->GetGUID())
                    {
                         m_UniqueTargetInfo.erase(ihit++);
                         continue;
                    }
                }
                ++ihit;
            }
            if (checkLvl && m_UniqueTargetInfo.empty())
            {
                SendCastResult(SPELL_FAILED_LOWLEVEL);
                finish(false);
            }
        }
    }

    if (m_targets.HasDst())
    {
        if (m_targets.HasTraj())
        {
            float speed = m_targets.GetSpeedXY();
            if (speed > 0.0f)
                m_delayMoment = (uint64)floor(m_targets.GetDist2d() / speed * 1000.0f);
        }
        else if (m_spellInfo->Speed > 0.0f)
        {
            float dist = m_caster->GetDistance(*m_targets.GetDstPos());

            if (!(AttributesCustomEx9 & SPELL_ATTR9_SPECIAL_DELAY_CALCULATION))
                m_delayMoment = uint64(floor(dist / m_spellInfo->Speed * 1000.0f));
            else
                m_delayMoment = uint64(m_spellInfo->Speed * 1000.0f);
        }
    }
}

void Spell::SelectEffectImplicitTargets(SpellEffIndex effIndex, SpellImplicitTargetInfo const& targetType, uint32& processedEffectMask)
{
    if (!targetType.GetTarget())
        return;

    uint32 effectMask = 1 << effIndex;
    // set the same target list for all effects
    // some spells appear to need this, however this requires more research
    switch (targetType.GetSelectionCategory())
    {
        case TARGET_SELECT_CATEGORY_NEARBY:
        case TARGET_SELECT_CATEGORY_CONE:
        case TARGET_SELECT_CATEGORY_AREA:
            // targets for effect already selected
            if (effectMask & processedEffectMask)
                return;
            // choose which targets we can select at once
            for (uint32 j = effIndex + 1; j < MAX_SPELL_EFFECTS; ++j)
            {
                if (GetSpellInfo()->GetEffect(effIndex, m_diffMode)->TargetA.GetTarget() == GetSpellInfo()->Effects[j].TargetA.GetTarget() &&
                    GetSpellInfo()->GetEffect(effIndex, m_diffMode)->TargetB.GetTarget() == GetSpellInfo()->Effects[j].TargetB.GetTarget() &&
                    GetSpellInfo()->Effects[effIndex].ImplicitTargetConditions == GetSpellInfo()->Effects[j].ImplicitTargetConditions &&
                    GetSpellInfo()->GetEffect(effIndex, m_diffMode)->CalcRadius(m_caster) == GetSpellInfo()->Effects[j].CalcRadius(m_caster) &&
                    GetSpellInfo()->Id != 119072)
                    effectMask |= 1 << j;
            }
            processedEffectMask |= effectMask;
            break;
        default:
            break;
    }

    switch (targetType.GetSelectionCategory())
    {
        case TARGET_SELECT_CATEGORY_GOTOMOVE:
            SelectImplicitGotoMoveTargets(effIndex, targetType, effectMask);
            break;
        case TARGET_SELECT_CATEGORY_BETWEEN:
            SelectImplicitBetweenTargets(effIndex, targetType, effectMask);
            break;
        case TARGET_SELECT_CATEGORY_CHANNEL:
            SelectImplicitChannelTargets(effIndex, targetType);
            break;
        case TARGET_SELECT_CATEGORY_NEARBY:
            SelectImplicitNearbyTargets(effIndex, targetType, effectMask);
            break;
        case TARGET_SELECT_CATEGORY_CONE:
            SelectImplicitConeTargets(effIndex, targetType, effectMask);
            break;
        case TARGET_SELECT_CATEGORY_AREA:
            SelectImplicitAreaTargets(effIndex, targetType, effectMask);
            break;
        case TARGET_SELECT_CATEGORY_THREAD:
            SelectImplicitTargetsFromThreadList(effIndex, targetType, effectMask);
            break;
        case TARGET_SELECT_CATEGORY_DEFAULT:
            switch (targetType.GetObjectType())
            {
                case TARGET_OBJECT_TYPE_SRC:
                    switch (targetType.GetReferenceType())
                    {
                        case TARGET_REFERENCE_TYPE_CASTER:
                            m_targets.SetSrc(*m_caster);
                            break;
                        default:
                            ASSERT(false && "Spell::SelectEffectImplicitTargets: received not implemented select target reference type for TARGET_TYPE_OBJECT_SRC");
                            break;
                    }
                    break;
                case TARGET_OBJECT_TYPE_DEST:
                case TARGET_OBJECT_TYPE_OBJ_AND_DEST:
                     switch (targetType.GetReferenceType())
                     {
                         case TARGET_REFERENCE_TYPE_CASTER:
                             SelectImplicitCasterDestTargets(effIndex, targetType);
                             break;
                         case TARGET_REFERENCE_TYPE_TARGET:
                             SelectImplicitTargetDestTargets(effIndex, targetType);
                             break;
                         case TARGET_REFERENCE_TYPE_DEST:
                             SelectImplicitDestDestTargets(effIndex, targetType);
                             break;
                         default:
                             ASSERT(false && "Spell::SelectEffectImplicitTargets: received not implemented select target reference type for TARGET_TYPE_OBJECT_DEST");
                             break;
                     }
                     break;
                default:
                    switch (targetType.GetReferenceType())
                    {
                        case TARGET_REFERENCE_TYPE_CASTER:
                            SelectImplicitCasterObjectTargets(effIndex, targetType);
                            break;
                        case TARGET_REFERENCE_TYPE_TARGET:
                            SelectImplicitTargetObjectTargets(effIndex, targetType);
                            break;
                        default:
                            ASSERT(false && "Spell::SelectEffectImplicitTargets: received not implemented select target reference type for TARGET_TYPE_OBJECT");
                            break;
                    }
                    break;
            }
            break;
        case TARGET_SELECT_CATEGORY_NYI:
            sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "SPELL: target type %u, found in spellID %u, effect %u is not implemented yet!", m_spellInfo->Id, effIndex, targetType.GetTarget());
            break;
        default:
            printf("Spell::SelectEffectImplicitTargets: received not implemented select target category / Spell ID = %u and Effect = %u and target type = %u \n", m_spellInfo->Id, effIndex, targetType.GetTarget());
            //ASSERT(false && "Spell::SelectEffectImplicitTargets: received not implemented select target category");
            break;
    }
}

void Spell::SelectImplicitChannelTargets(SpellEffIndex effIndex, SpellImplicitTargetInfo const& targetType)
{
    if (targetType.GetReferenceType() != TARGET_REFERENCE_TYPE_CASTER)
    {
        ASSERT(false && "Spell::SelectImplicitChannelTargets: received not implemented target reference type");
        return;
    }

    Spell* channeledSpell = m_originalCaster->GetCurrentSpell(CURRENT_CHANNELED_SPELL);
    if (!channeledSpell)
    {
        sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "Spell::SelectImplicitChannelTargets: cannot find channel spell for spell ID %u, effect %u", m_spellInfo->Id, effIndex);
        return;
    }
    switch (targetType.GetTarget())
    {
        case TARGET_UNIT_CHANNEL_TARGET:
        {
            WorldObject* target = ObjectAccessor::GetUnit(*m_caster, m_originalCaster->GetUInt64Value(UNIT_FIELD_CHANNEL_OBJECT));
            CallScriptObjectTargetSelectHandlers(target, effIndex);

            /*if (target && m_caster && !m_caster->canSeeOrDetect(target))
            {
                m_caster->InterruptSpell(CURRENT_CHANNELED_SPELL, false);
                return;
            }*/
            // unit target may be no longer avalible - teleported out of map for example
            if (target && target->ToUnit())
                AddUnitTarget(target->ToUnit(), 1 << effIndex);
            else
                sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "SPELL: cannot find channel spell target for spell ID %u, effect %u", m_spellInfo->Id, effIndex);
            break;
        }
        case TARGET_DEST_CHANNEL_TARGET:
            if (channeledSpell->m_targets.HasDst())
                m_targets.SetDst(channeledSpell->m_targets);
            else if (WorldObject* target = ObjectAccessor::GetWorldObject(*m_caster, m_originalCaster->GetUInt64Value(UNIT_FIELD_CHANNEL_OBJECT)))
            {
                CallScriptObjectTargetSelectHandlers(target, effIndex);
                if (target)
                    m_targets.SetDst(*target);
            }
            else
                sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "SPELL: cannot find channel spell destination for spell ID %u, effect %u", m_spellInfo->Id, effIndex);
            break;
        case TARGET_DEST_CHANNEL_CASTER:
            m_targets.SetDst(*channeledSpell->GetCaster());
            break;
        default:
            ASSERT(false && "Spell::SelectImplicitChannelTargets: received not implemented target type");
            break;
    }
}

void Spell::SelectImplicitNearbyTargets(SpellEffIndex effIndex, SpellImplicitTargetInfo const& targetType, uint32 effMask)
{
    if (targetType.GetReferenceType() != TARGET_REFERENCE_TYPE_CASTER)
    {
        ASSERT(false && "Spell::SelectImplicitNearbyTargets: received not implemented target reference type");
        return;
    }

    float range = 0.0f;
    switch (targetType.GetCheckType())
    {
        case TARGET_CHECK_ENEMY:
            range = m_spellInfo->GetMaxRange(false, m_caster, this);
            break;
        case TARGET_CHECK_ALLY:
        case TARGET_CHECK_PARTY:
        case TARGET_CHECK_RAID:
        case TARGET_CHECK_RAID_CLASS:
        case TARGET_CHECK_SUMMON:
            range = m_spellInfo->GetMaxRange(true, m_caster, this);
            break;
        case TARGET_CHECK_ENTRY:
        case TARGET_CHECK_DEFAULT:
            range = m_spellInfo->GetMaxRange(m_spellInfo->IsPositive(), m_caster, this);
            break;
        default:
            ASSERT(false && "Spell::SelectImplicitNearbyTargets: received not implemented selection check type");
            break;
    }

    ConditionList* condList = m_spellInfo->Effects[effIndex].ImplicitTargetConditions;

    // handle emergency case - try to use other provided targets if no conditions provided
    if (targetType.GetCheckType() == TARGET_CHECK_ENTRY && (!condList || condList->empty()))
    {
        switch (targetType.GetObjectType())
        {
            case TARGET_OBJECT_TYPE_GOBJ:
                if (m_spellInfo->RequiresSpellFocus)
                {
                    if (focusObject)
                        AddGOTarget(focusObject, effMask);
                    return;
                }
                break;
            case TARGET_OBJECT_TYPE_DEST:
                if (m_spellInfo->RequiresSpellFocus)
                {
                    if (focusObject)
                        m_targets.SetDst(*focusObject);
                    return;
                }
                // A lot off new spells for dungeons not have target entry. Just use spell target position system.
                // it's right way and no need all time write hacks.
                if (SpellTargetPosition const* st = sSpellMgr->GetSpellTargetPosition(m_spellInfo->Id))
                {
                    // TODO: fix this check
                    if (m_spellInfo->HasEffect(SPELL_EFFECT_TELEPORT_UNITS))
                        m_targets.SetDst(st->target_X, st->target_Y, st->target_Z, st->target_Orientation, (int32)st->target_mapId);
                    else if (st->target_mapId == m_caster->GetMapId())
                        m_targets.SetDst(st->target_X, st->target_Y, st->target_Z, st->target_Orientation);
                    return;
                }else
                    sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "Spell::SelectImplicitNearbyTargets: no target destination on db: spell_target_position of spell ID %u, effect %u - selecting default targets", m_spellInfo->Id, effIndex);
                break;
            default:
                break;
        }
        sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "Spell::SelectImplicitNearbyTargets: no conditions entry for target with TARGET_CHECK_ENTRY of spell ID %u, effect %u - selecting default targets", m_spellInfo->Id, effIndex);
    }

    WorldObject* target = SearchNearbyTarget(range, targetType.GetObjectType(), targetType.GetCheckType(), condList);
    if (!target)
    {
        sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "Spell::SelectImplicitNearbyTargets: cannot find nearby target for spell ID %u, effect %u", m_spellInfo->Id, effIndex);
        return;
    }

    //sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "Spell::SelectImplicitNearbyTargets spell id %u caster %u target %u entry %i", m_spellInfo->Id,  m_caster->GetGUID(), target->GetGUID(), target->GetEntry());

    CallScriptObjectTargetSelectHandlers(target, effIndex);

    switch (targetType.GetObjectType())
    {
        case TARGET_OBJECT_TYPE_UNIT:
            if (Unit* unitTarget = target->ToUnit())
            {
                AddUnitTarget(unitTarget, effMask, condList ? false : true, false);
                m_targets.SetUnitTarget(unitTarget);
            }
            break;
        case TARGET_OBJECT_TYPE_GOBJ:
            if (GameObject* gobjTarget = target->ToGameObject())
                AddGOTarget(gobjTarget, effMask);
            break;
        case TARGET_OBJECT_TYPE_DEST:
            m_targets.SetDst(*target);
            break;
        default:
            ASSERT(false && "Spell::SelectImplicitNearbyTargets: received not implemented target object type");
            break;
    }

    SelectImplicitChainTargets(effIndex, targetType, target, effMask);
}

void Spell::SelectImplicitTargetsFromThreadList(SpellEffIndex effIndex, SpellImplicitTargetInfo const& targetType, uint32 effMask)
{
    if (targetType.GetReferenceType() != TARGET_REFERENCE_TYPE_CASTER)
    {
        ASSERT(false && "Spell::SelectImplicitTargetsFromThreadList: received not implemented target reference type");
        return;
    }

    std::list<uint64>* savethreatlist = m_caster->GetSaveThreatList();
    for (std::list<uint64>::const_iterator itr = savethreatlist->begin(); itr != savethreatlist->end(); ++itr)
    {
        if (Player* target = ObjectAccessor::GetPlayer(*m_caster, (*itr)))
        {
            if(target->IsPlayerLootCooldown(m_spellInfo->Id, TYPE_SPELL, target->GetMap()->GetDifficulty())) //Don`t add player if exist CD
                continue;

            if(m_caster->GetZoneId() == target->GetZoneId()) //Check target if this zone
            {
                AddUnitTarget((Unit*)target, effMask, false, false);
                m_targets.SetUnitTarget((Unit*)target);
            }
        }
    }
}

void Spell::SelectImplicitBetweenTargets(SpellEffIndex effIndex, SpellImplicitTargetInfo const& targetType, uint32 effMask)
{
    std::list<WorldObject*> targets;
    SpellTargetObjectTypes objectType = targetType.GetObjectType();
    SpellTargetCheckTypes selectionType = targetType.GetCheckType();
    ConditionList* condList = m_spellInfo->Effects[effIndex].ImplicitTargetConditions;
    float width = 5.0f;
    float coneAngle = targetType.CalcDirectionAngle();
    float radius = m_spellInfo->GetEffect(effIndex, m_diffMode)->CalcRadius(m_caster) * m_spellValue->RadiusMod;

    if (uint32 containerTypeMask = GetSearcherTypeMask(objectType, condList))
    {
        Unit* referer = NULL;
        switch (targetType.GetReferenceType())
        {
            case TARGET_REFERENCE_TYPE_TARGET:
                referer = m_targets.GetUnitTarget();
                break;
            case TARGET_REFERENCE_TYPE_LAST:
            {
                // find last added target for this effect
                for (std::list<TargetInfo>::reverse_iterator ihit = m_UniqueTargetInfo.rbegin(); ihit != m_UniqueTargetInfo.rend(); ++ihit)
                {
                    if (ihit->effectMask & (1<<effIndex))
                    {
                        referer = ObjectAccessor::GetUnit(*m_caster, ihit->targetGUID);
                        break;
                    }
                }
                break;
            }
            default:
                referer = m_caster;
                break;
        }

        Position const* center = NULL;
        switch (targetType.GetReferenceType())
        {
            case TARGET_REFERENCE_TYPE_NEAR_DEST:
            {
                Position position;
                m_caster->GetNearPosition(position, radius, coneAngle);
                center = &position;
                break;
            }
            case TARGET_REFERENCE_TYPE_SRC:
                center = m_targets.GetSrcPos();
                break;
            case TARGET_REFERENCE_TYPE_DEST:
                center = m_targets.GetDstPos();
                break;
             default:
                center = referer;
                break;
        }

        Trinity::WorldObjectSpellBetweenTargetCheck check(width, radius, m_caster, center, referer, m_spellInfo, selectionType, condList, effIndex);
        Trinity::WorldObjectListSearcher<Trinity::WorldObjectSpellBetweenTargetCheck> searcher(m_caster, targets, check, containerTypeMask);
        SearchTargets<Trinity::WorldObjectListSearcher<Trinity::WorldObjectSpellBetweenTargetCheck> >(searcher, containerTypeMask, m_caster, m_caster, radius);

        sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "Spell::SelectImplicitBetweenTargets coneAngle %f, radius %f, x %f, y %f, Id %u, targets.size %u", coneAngle, radius, center->GetPositionX(), center->GetPositionY(), m_spellInfo->Id, targets.size());

        CallScriptObjectAreaTargetSelectHandlers(targets, effIndex, targetType.GetTarget());

        if (!targets.empty())
        {
            // Other special target selection goes here
            if (uint32 maxTargets = m_spellValue->MaxAffectedTargets)
                Trinity::Containers::RandomResizeList(targets, maxTargets);

            // for compability with older code - add only unit and go targets
            // TODO: remove this
            std::list<Unit*> unitTargets;
            std::list<GameObject*> gObjTargets;

            for (std::list<WorldObject*>::iterator itr = targets.begin(); itr != targets.end(); ++itr)
            {
                if (Unit* unitTarget = (*itr)->ToUnit())
                    unitTargets.push_back(unitTarget);
                else if (GameObject* gObjTarget = (*itr)->ToGameObject())
                    gObjTargets.push_back(gObjTarget);
            }

            for (std::list<Unit*>::iterator itr = unitTargets.begin(); itr != unitTargets.end(); ++itr)
                AddUnitTarget(*itr, effMask, false);

            for (std::list<GameObject*>::iterator itr = gObjTargets.begin(); itr != gObjTargets.end(); ++itr)
                AddGOTarget(*itr, effMask);
        }
    }
}

void Spell::SelectImplicitConeTargets(SpellEffIndex effIndex, SpellImplicitTargetInfo const& targetType, uint32 effMask)
{
    if (targetType.GetReferenceType() != TARGET_REFERENCE_TYPE_CASTER)
    {
        ASSERT(false && "Spell::SelectImplicitConeTargets: received not implemented target reference type");
        return;
    }
    std::list<WorldObject*> targets;
    SpellTargetObjectTypes objectType = targetType.GetObjectType();
    SpellTargetCheckTypes selectionType = targetType.GetCheckType();
    ConditionList* condList = m_spellInfo->Effects[effIndex].ImplicitTargetConditions;
    float coneAngle = M_PI/2;

    switch (m_spellInfo->GetEffect(effIndex, m_diffMode)->TargetA.GetTarget())
    {
        case TARGET_UNIT_CONE_ENEMY_54:
            coneAngle = M_PI;
            break;
        case TARGET_UNIT_CONE_ENEMY_110:
            coneAngle = M_PI/6;
            break;
    }

    switch (m_spellInfo->Id)
    {
        case 118094:
            coneAngle = M_PI/2;
            break;
        case 118105:
            coneAngle = M_PI/4;
            break;
        case 118106:
            coneAngle = M_PI/6;
            break;
    }

    float radius = m_spellInfo->GetEffect(effIndex, m_diffMode)->CalcRadius(m_caster) * m_spellValue->RadiusMod;

    // Fists of Fury
    // it`s bugged, dont know how, temp fix
    if (GetSpellInfo()->Id == 120086 || GetSpellInfo()->Id == 117418)
        radius = m_caster->HasAura(131562) ? 10.f : 5.f;

    if (uint32 containerTypeMask = GetSearcherTypeMask(objectType, condList))
    {
        Trinity::WorldObjectSpellConeTargetCheck check(coneAngle, radius, m_caster, m_spellInfo, selectionType, condList, effIndex);
        Trinity::WorldObjectListSearcher<Trinity::WorldObjectSpellConeTargetCheck> searcher(m_caster, targets, check, containerTypeMask);
        SearchTargets<Trinity::WorldObjectListSearcher<Trinity::WorldObjectSpellConeTargetCheck> >(searcher, containerTypeMask, m_caster, m_caster, radius);

        CallScriptObjectAreaTargetSelectHandlers(targets, effIndex, targetType.GetTarget());

        if (!targets.empty())
        {
            // Other special target selection goes here
            if (uint32 maxTargets = m_spellValue->MaxAffectedTargets)
                Trinity::Containers::RandomResizeList(targets, maxTargets);

            // for compability with older code - add only unit and go targets
            // TODO: remove this
            std::list<Unit*> unitTargets;
            std::list<GameObject*> gObjTargets;

            for (std::list<WorldObject*>::iterator itr = targets.begin(); itr != targets.end(); ++itr)
            {
                if (Unit* unitTarget = (*itr)->ToUnit())
                    unitTargets.push_back(unitTarget);
                else if (GameObject* gObjTarget = (*itr)->ToGameObject())
                    gObjTargets.push_back(gObjTarget);
            }

            for (std::list<Unit*>::iterator itr = unitTargets.begin(); itr != unitTargets.end(); ++itr)
                AddUnitTarget(*itr, effMask, false);

            for (std::list<GameObject*>::iterator itr = gObjTargets.begin(); itr != gObjTargets.end(); ++itr)
                AddGOTarget(*itr, effMask);
        }
    }
}

void Spell::SelectImplicitAreaTargets(SpellEffIndex effIndex, SpellImplicitTargetInfo const& targetType, uint32 effMask)
{
    Unit* referer = NULL;
    switch (targetType.GetReferenceType())
    {
        case TARGET_REFERENCE_TYPE_SRC:
        case TARGET_REFERENCE_TYPE_DEST:
        case TARGET_REFERENCE_TYPE_CASTER:
            referer = m_caster;
            break;
        case TARGET_REFERENCE_TYPE_TARGET:
            referer = m_targets.GetUnitTarget();
            break;
        case TARGET_REFERENCE_TYPE_LAST:
        {
            // find last added target for this effect
            for (std::list<TargetInfo>::reverse_iterator ihit = m_UniqueTargetInfo.rbegin(); ihit != m_UniqueTargetInfo.rend(); ++ihit)
            {
                if (ihit->effectMask & (1<<effIndex))
                {
                    referer = ObjectAccessor::GetUnit(*m_caster, ihit->targetGUID);
                    break;
                }
            }
            break;
        }
        default:
            ASSERT(false && "Spell::SelectImplicitAreaTargets: received not implemented target reference type");
            return;
    }
    if (!referer)
        return;

    Position const* center = NULL;
    switch (targetType.GetReferenceType())
    {
        case TARGET_REFERENCE_TYPE_SRC:
            center = m_targets.GetSrcPos();
            break;
        case TARGET_REFERENCE_TYPE_DEST:
            center = m_targets.GetDstPos();
            break;
        case TARGET_REFERENCE_TYPE_CASTER:
        case TARGET_REFERENCE_TYPE_TARGET:
        case TARGET_REFERENCE_TYPE_LAST:
            center = referer;
            break;
         default:
             ASSERT(false && "Spell::SelectImplicitAreaTargets: received not implemented target reference type");
             return;
    }
    std::list<WorldObject*> targets;
    float radius = m_spellInfo->GetEffect(effIndex, m_diffMode)->CalcRadius(m_caster) * m_spellValue->RadiusMod;
    if(radius <= 0)
        radius = 5000.0f;

    SearchAreaTargets(targets, radius, center, referer, targetType.GetObjectType(), targetType.GetCheckType(), m_spellInfo->Effects[effIndex].ImplicitTargetConditions, effIndex);

    sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "Spell::SelectImplicitAreaTargets %u, radius %f, GetObjectType %u, targets count %u, effIndex %i, Conditions %i", m_spellInfo->Id, radius, targetType.GetObjectType(), targets.size(), effIndex, m_spellInfo->Effects[effIndex].ImplicitTargetConditions);

    CallScriptObjectAreaTargetSelectHandlers(targets, effIndex, targetType.GetTarget());

    sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "Spell::SelectImplicitAreaTargets after filter %u, radius %f, GetObjectType %u, targets count %u, GetCheckType %i, X %f, Y %f",
    m_spellInfo->Id, radius, targetType.GetObjectType(), targets.size(), targetType.GetCheckType(), center->GetPositionX(), center->GetPositionY());

    std::list<Unit*> unitTargets;
    std::list<GameObject*> gObjTargets;
    // for compability with older code - add only unit and go targets
    // TODO: remove this
    if (!targets.empty())
    for (std::list<WorldObject*>::iterator itr = targets.begin(); itr != targets.end(); ++itr)
    {
        if (Unit* unitTarget = (*itr)->ToUnit())
            unitTargets.push_back(unitTarget);
        else if (GameObject* gObjTarget = (*itr)->ToGameObject())
            gObjTargets.push_back(gObjTarget);
    }

    if (!unitTargets.empty())
    {
        // Special target selection for smart heals and energizes
        uint32 maxSize = 0;
        int32 power = -1;
        bool checkOnFullHealth = false;

        switch (m_spellInfo->SpellFamilyName)
        {
            case SPELLFAMILY_GENERIC:
                switch (m_spellInfo->Id)
                {
                    case 148008: // Essence of Yu'lon
                    {
                        maxSize = 5;
                        break;
                    }
                    case 113828: // Healing Touch
                    {
                        unitTargets.push_back(m_caster);
                        maxSize = 1;
                        power = POWER_HEALTH;
                        break;
                    }
                    case 52759: // Ancestral Awakening
                    case 71610: // Echoes of Light (Althor's Abacus normal version)
                    case 71641: // Echoes of Light (Althor's Abacus heroic version)
                        maxSize = 1;
                        power = POWER_HEALTH;
                        break;
                    case 54968: // Glyph of Holy Light
                        maxSize = m_spellInfo->GetMaxAffectedTargets(m_diffMode);
                        power = POWER_HEALTH;
                        break;
                    case 57669: // Replenishment
                        // In arenas Replenishment may only affect the caster
                        if (m_caster->GetTypeId() == TYPEID_PLAYER && m_caster->ToPlayer()->InArena())
                        {
                            unitTargets.clear();
                            unitTargets.push_back(m_caster);
                            break;
                        }
                        maxSize = 10;
                        power = POWER_MANA;
                        break;
                    default:
                        break;
                }
                break;
            case SPELLFAMILY_PRIEST:
                if (m_spellInfo->SpellFamilyFlags[0] == 0x10000000) // Circle of Healing
                {
                    maxSize = m_caster->HasAura(55675) ? 6 : 5; // Glyph of Circle of Healing
                    power = POWER_HEALTH;
                }
                else if (m_spellInfo->Id == 64844) // Divine Hymn
                {
                    maxSize = 3;
                    power = POWER_HEALTH;
                }
                else if (m_spellInfo->Id == 64904) // Hymn of Hope
                {
                    maxSize = 3;
                    power = POWER_MANA;
                }
                else
                    break;

                // Remove targets outside caster's raid
                for (std::list<Unit*>::iterator itr = unitTargets.begin(); itr != unitTargets.end();)
                {
                    if (!(*itr)->IsInRaidWith(m_caster))
                        itr = unitTargets.erase(itr);
                    else
                        ++itr;
                }
                break;
            case SPELLFAMILY_PALADIN:
                // Holy Wrath
                if (m_spellInfo->Id == 119072 && effIndex == EFFECT_0)
                {
                    static const uint8 types_noglyph[] = {CREATURE_TYPE_DEMON, CREATURE_TYPE_UNDEAD, 0};
                    static const uint8 types_glyph[] = {CREATURE_TYPE_DEMON, CREATURE_TYPE_UNDEAD, CREATURE_TYPE_ELEMENTAL, CREATURE_TYPE_DRAGONKIN, 0};
                    const uint8 *types = m_caster->HasAura(54923) ? types_glyph: types_noglyph;

                    for (std::list<Unit*>::iterator itr = unitTargets.begin() ; itr != unitTargets.end();)
                    {
                        bool found = false;
                        uint8 types_i = 0;
                        do
                        {
                            if ((*itr)->GetCreatureType() == types[types_i])
                            {
                                found = true;
                                break;
                            }
                        }
                        while (types[++types_i]);

                        if (found)
                            ++itr;
                        else
                            itr = unitTargets.erase(itr);
                    }
                }
                break;
            case SPELLFAMILY_MONK:
            {
                switch (m_spellInfo->Id)
                {
                    case 120086: // Fists of Fury (stun)
                    {
                        for (std::list<Unit*>::iterator itr = unitTargets.begin(); itr != unitTargets.end();)
                            if ((*itr)->HasAura(m_spellInfo->Id))
                                itr = unitTargets.erase(itr);
                        break;
                    }
                    default:
                        break;
                }
                break;
            }
            case SPELLFAMILY_DRUID:
            {
                bool shouldCheck = false;
                switch (m_spellInfo->Id)
                {
                    case 145110: // Ysera's Gift
                    {
                        checkOnFullHealth = true;
                        maxSize = 1;
                        power = POWER_HEALTH;
                        break;
                    }
                    case 48438:  // Wild Growth
                    {
                        shouldCheck = true;
                        maxSize = m_caster->HasAura(62970) ? 6 : 5; // Glyph of Wild Growth
                        power = POWER_HEALTH;
                        break;
                    }
                    default:
                        break;
                }

                // Remove targets outside caster's raid
                // WTF?????????? May be better to change targets?
                if (shouldCheck)
                {
                    for (std::list<Unit*>::iterator itr = unitTargets.begin(); itr != unitTargets.end();)
                        if (!(*itr)->IsInRaidWith(m_caster))
                            itr = unitTargets.erase(itr);
                        else
                            ++itr;
                }
                break;
            }
            default:
                break;
        }

        if (targetType.GetTarget() == TARGET_UNIT_FRIEND_OR_RAID)
        {
            if (m_originalTarget)
            {
                if (!m_caster->IsInRaidWith(m_originalTarget))
                {
                    unitTargets.clear();
                    unitTargets.push_back(m_originalTarget);
                }
            }
        }

        if (maxSize && power != -1)
        {
            if (Powers(power) == POWER_HEALTH)
            {
                if (unitTargets.size() > maxSize)
                {
                    unitTargets.sort(Trinity::HealthPctOrderPred());
                    unitTargets.resize(maxSize);
                }
            }
            else
            {
                for (std::list<Unit*>::iterator itr = unitTargets.begin(); itr != unitTargets.end();)
                    if ((*itr)->getPowerType() != (Powers)power)
                        itr = unitTargets.erase(itr);
                    else
                        ++itr;

                if (unitTargets.size() > maxSize)
                {
                    unitTargets.sort(Trinity::PowerPctOrderPred((Powers)power));
                    unitTargets.resize(maxSize);
                }
            }
        }

        if (checkOnFullHealth)
        {
            for (std::list<Unit*>::iterator itr = unitTargets.begin(); itr != unitTargets.end();)
                if ((*itr)->IsFullHealth())
                    itr = unitTargets.erase(itr);
                else
                    ++itr;
        }

        // todo: move to scripts, but we must call it before resize list by MaxAffectedTargets
        // Intimidating Shout
        if (m_spellInfo->Id == 5246 && effIndex != EFFECT_0)
            unitTargets.remove(m_targets.GetUnitTarget());

        // Custom MoP Script
        // 117640 - Spinning Crane Kick : Give 1 Chi if the spell hits at least 3 targets
        if (m_caster->GetTypeId() == TYPEID_PLAYER)
        {
            if (m_spellInfo->Id == 117640 && unitTargets.size() >= 3 && !m_caster->ToPlayer()->HasSpellCooldown(129881))
            {
                m_caster->CastSpell(m_caster, 129881, true);
                m_caster->ToPlayer()->AddSpellCooldown(129881, 0, getPreciseTime() + 6.0);
            }
        }

        // Other special target selection goes here
        if (uint32 maxTargets = m_spellValue->MaxAffectedTargets)
            Trinity::Containers::RandomResizeList(unitTargets, maxTargets);

        for (std::list<Unit*>::iterator itr = unitTargets.begin(); itr != unitTargets.end(); ++itr)
            AddUnitTarget(*itr, effMask);
    }

    if (!gObjTargets.empty())
    {
        if (uint32 maxTargets = m_spellValue->MaxAffectedTargets)
            Trinity::Containers::RandomResizeList(gObjTargets, maxTargets);

        for (std::list<GameObject*>::iterator itr = gObjTargets.begin(); itr != gObjTargets.end(); ++itr)
            AddGOTarget(*itr, effMask);
    }
}

void Spell::SelectImplicitCasterDestTargets(SpellEffIndex effIndex, SpellImplicitTargetInfo const& targetType)
{
    m_targets.SetDst(*m_caster);
    switch (targetType.GetTarget())
    {
        case TARGET_DEST_HOME:
            if (Player* playerCaster = m_caster->ToPlayer())
                m_targets.SetDst(playerCaster->m_homebindX, playerCaster->m_homebindY, playerCaster->m_homebindZ, playerCaster->GetOrientation(), playerCaster->m_homebindMapId);
            return;
        case TARGET_DEST_DB:
            if (SpellTargetPosition const* st = sSpellMgr->GetSpellTargetPosition(m_spellInfo->Id))
            {
                // TODO: fix this check
                if (m_spellInfo->HasEffect(SPELL_EFFECT_TELEPORT_UNITS))
                    m_targets.SetDst(st->target_X, st->target_Y, st->target_Z, st->target_Orientation, (int32)st->target_mapId);
                else if (st->target_mapId == m_caster->GetMapId())
                    m_targets.SetDst(st->target_X, st->target_Y, st->target_Z, st->target_Orientation);
            }
            else
            {
                sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "SPELL: unknown target coordinates for spell ID %u", m_spellInfo->Id);
                WorldObject* target = m_targets.GetObjectTarget();
                m_targets.SetDst(target ? *target : *m_caster);
            }
            return;
        case TARGET_DEST_CASTER_FISHING:
        {
             float min_dis = m_spellInfo->GetMinRange(true);
             float max_dis = m_spellInfo->GetMaxRange(true);
             float dis = (float)rand_norm() * (max_dis - min_dis) + min_dis;
             float x, y, z, angle;
             angle = (float)rand_norm() * static_cast<float>(M_PI * 35.0f / 180.0f) - static_cast<float>(M_PI * 17.5f / 180.0f);
             m_caster->GetClosePoint(x, y, z, DEFAULT_WORLD_OBJECT_SIZE, dis, angle);
             m_targets.SetDst(x, y, z, m_caster->GetOrientation());
             return;
        }
        default:
            break;
    }

    if(targetType.GetDirectionType() == TARGET_DIR_NONE)
        return;

    float dist = 0.0f;
    float angle = targetType.CalcDirectionAngle();
    float objSize = m_caster->GetObjectSize();
    if (targetType.GetTarget() == TARGET_DEST_CASTER_SUMMON)
        dist = PET_FOLLOW_DIST;
    else
        dist = m_spellInfo->GetEffect(effIndex, m_diffMode)->CalcRadius(m_caster);

    if (targetType.GetTarget() == TARGET_DEST_MAX_RANGE)
        dist = m_spellInfo->GetMaxRange();

    if (dist < objSize)
        dist = objSize;
    else if (targetType.GetTarget() == TARGET_DEST_CASTER_RANDOM)
        dist = objSize + (dist - objSize) * (float)rand_norm();

    Position pos;
    switch (targetType.GetTarget())
    {
        case TARGET_DEST_CASTER_FRONT_LEAP:
        case TARGET_DEST_CASTER_FRONT_RIGHT:
        case TARGET_DEST_CASTER_BACK_RIGHT:
        case TARGET_DEST_CASTER_BACK_LEFT:
        case TARGET_DEST_CASTER_FRONT_LEFT:
        case TARGET_DEST_CASTER_FRONT:
        case TARGET_DEST_CASTER_BACK:
        case TARGET_DEST_CASTER_RIGHT:
        case TARGET_DEST_CASTER_LEFT:
        case TARGET_DEST_MAX_RANGE:
            if (canHitTargetInLOS && m_caster->ToCreature() && dist < 200.0f)
                m_caster->GetNearPoint2D(pos, dist, angle);
            else
                m_caster->GetFirstCollisionPosition(pos, dist, angle);
            break;
        default:
            m_caster->GetNearPoint2D(pos, dist, angle);
            break;
    }
    m_targets.ModDst(pos);
}

void Spell::SelectImplicitGotoMoveTargets(SpellEffIndex effIndex, SpellImplicitTargetInfo const& /*targetType*/, uint32 /*effMask*/)
{
    float dist = m_spellInfo->GetEffect(effIndex, m_diffMode)->CalcRadius(m_caster);
    float angle = 0.0f;

    if(m_caster->HasUnitMovementFlag(MOVEMENTFLAG_FORWARD) && m_caster->HasUnitMovementFlag(MOVEMENTFLAG_STRAFE_LEFT))
        angle = static_cast<float>(M_PI/4);
    else if(m_caster->HasUnitMovementFlag(MOVEMENTFLAG_FORWARD) && m_caster->HasUnitMovementFlag(MOVEMENTFLAG_STRAFE_RIGHT))
        angle = static_cast<float>(-M_PI/4);
    else if(m_caster->HasUnitMovementFlag(MOVEMENTFLAG_BACKWARD) && m_caster->HasUnitMovementFlag(MOVEMENTFLAG_STRAFE_LEFT))
        angle = static_cast<float>(3*M_PI/4);
    else if(m_caster->HasUnitMovementFlag(MOVEMENTFLAG_BACKWARD) && m_caster->HasUnitMovementFlag(MOVEMENTFLAG_STRAFE_RIGHT))
        angle = static_cast<float>(-3*M_PI/4);
    else if(m_caster->HasUnitMovementFlag(MOVEMENTFLAG_BACKWARD))
        angle = static_cast<float>(M_PI);
    else if(m_caster->HasUnitMovementFlag(MOVEMENTFLAG_STRAFE_LEFT))
        angle = static_cast<float>(M_PI/2);
    else if(m_caster->HasUnitMovementFlag(MOVEMENTFLAG_STRAFE_RIGHT))
        angle = static_cast<float>(-M_PI/2);
    float objSize = m_caster->GetObjectSize();
    if (dist < objSize)
        dist = objSize;

    Position pos;
    m_caster->GetFirstCollisionPosition(pos, dist, angle);
    m_targets.SetDst(*m_caster);
    m_targets.ModDst(pos);
}

void Spell::SelectImplicitTargetDestTargets(SpellEffIndex effIndex, SpellImplicitTargetInfo const& targetType)
{
    WorldObject* target = m_targets.GetObjectTarget();
    switch (targetType.GetTarget())
    {
        case TARGET_DEST_TARGET_ANY:
        case TARGET_DEST_CHANNEL_CASTER:
            m_targets.SetDst(*target);
            return;
        default:
            break;
    }
    if(targetType.GetDirectionType() == TARGET_DIR_NONE)
    {
        m_targets.SetDst(*target);
        return;
    }

    float angle = targetType.CalcDirectionAngle();
    float objSize = target->GetObjectSize();
    float dist = m_spellInfo->GetEffect(effIndex, m_diffMode)->CalcRadius(m_caster);
    if (dist < objSize)
        dist = objSize;
    else if (targetType.GetTarget() == TARGET_DEST_TARGET_RANDOM)
        dist = objSize + (dist - objSize) * (float)rand_norm();

    Position pos;
    if (canHitTargetInLOS && m_caster->ToCreature() && dist < 200.0f)
        target->GetNearPoint2D(pos, dist, angle);
    else
        target->GetNearPosition(pos, dist, angle);
    m_targets.SetDst(*target);
    m_targets.ModDst(pos);
}

void Spell::SelectImplicitDestDestTargets(SpellEffIndex effIndex, SpellImplicitTargetInfo const& targetType)
{
    // set destination to caster if no dest provided
    // can only happen if previous destination target could not be set for some reason
    // (not found nearby target, or channel target for example
    // maybe we should abort the spell in such case?
    if (SpellTargetPosition const* st = sSpellMgr->GetSpellTargetPosition(m_spellInfo->Id))
    {
        // TODO: fix this check
        if (m_spellInfo->HasEffect(SPELL_EFFECT_TELEPORT_UNITS))
            m_targets.SetDst(st->target_X, st->target_Y, st->target_Z, st->target_Orientation, (int32)st->target_mapId);
        else if (st->target_mapId == m_caster->GetMapId())
            m_targets.SetDst(st->target_X, st->target_Y, st->target_Z, st->target_Orientation);
        return;
    }

    bool exit = false;

    switch (targetType.GetTarget())
    {
        case TARGET_DEST_TARGET_ENEMY:
            if (Unit* target = m_targets.GetUnitTarget())
            {
                AddUnitTarget(target, 1 << effIndex);
                m_targets.SetDst(*target);
            }
            exit = true;
            break;
        case TARGET_DEST_TARGET_SELECT:
            if (Unit* target = m_targets.GetUnitTarget())
                m_targets.SetDst(*target);
            exit = true;
            break;
        case TARGET_DEST_NEARBY_ENTRY:
            if (WorldObject* target = m_targets.GetObjectTarget())
                m_targets.SetDst(*target);
            exit = true;
            break;
        case TARGET_DEST_DYNOBJ_ENEMY:
        case TARGET_DEST_DYNOBJ_ALLY:
        case TARGET_DEST_DYNOBJ_NONE:
        case TARGET_DEST_DEST:
        case TARGET_UNK_128:
            exit = true;
            break;
        case TARGET_DEST_TRAJ:
            SelectImplicitTrajTargets();
            exit = true;
            break;
        default:
            break;
    }

    if (!m_targets.HasDst())
        m_targets.SetDst(*m_caster);

    if (exit)
        return;

    if (targetType.GetDirectionType() == TARGET_DIR_NONE)
        return;

    float angle = targetType.CalcDirectionAngle();
    float dist = m_spellInfo->GetEffect(effIndex, m_diffMode)->CalcRadius(m_caster);
    if (targetType.GetTarget() == TARGET_DEST_DEST_RANDOM)
        dist *= (float)rand_norm();

    Position pos = *m_targets.GetDstPos();
    if (canHitTargetInLOS && m_caster->ToCreature() && dist < 200.0f)
        m_caster->GetNearPoint2D(pos, dist, angle);
    else
        m_caster->MovePosition(pos, dist, angle);
    m_targets.ModDst(pos);
}

void Spell::SelectImplicitCasterObjectTargets(SpellEffIndex effIndex, SpellImplicitTargetInfo const& targetType)
{
    WorldObject* target = NULL;
    bool checkIfValid = true;

    switch (targetType.GetTarget())
    {
        case TARGET_UNIT_CASTER:
            target = m_caster;
            checkIfValid = false;
            break;
        case TARGET_UNIT_MASTER:
            target = m_caster->GetCharmerOrOwner();
            checkIfValid = false;
            break;
        case TARGET_UNIT_PET:
            if(m_caster->isPet())
                target = m_caster;
            else
                target = m_caster->GetGuardianPet();
            break;
        case TARGET_UNIT_VEHICLE:
            target = m_caster->GetVehicleBase();
            break;
        case TARGET_UNIT_PASSENGER_0:
        case TARGET_UNIT_PASSENGER_1:
        case TARGET_UNIT_PASSENGER_2:
        case TARGET_UNIT_PASSENGER_3:
        case TARGET_UNIT_PASSENGER_4:
        case TARGET_UNIT_PASSENGER_5:
        case TARGET_UNIT_PASSENGER_6:
        case TARGET_UNIT_PASSENGER_7:
            if (m_caster->GetTypeId() == TYPEID_UNIT && m_caster->ToCreature()->IsVehicle())
                target = m_caster->GetVehicleKit()->GetPassenger(targetType.GetTarget() - TARGET_UNIT_PASSENGER_0);
            break;
        case TARGET_UNIT_TARGET_PASSENGER:
            if (m_caster->GetTypeId() == TYPEID_UNIT && m_caster->ToCreature()->IsVehicle())
                target = m_caster->GetVehicleKit()->GetPassenger(0);
            break;
        default:
            break;
    }

    CallScriptObjectTargetSelectHandlers(target, effIndex);

    if (target && target->ToUnit())
        AddUnitTarget(target->ToUnit(), 1 << effIndex, checkIfValid);
}

void Spell::SelectImplicitTargetObjectTargets(SpellEffIndex effIndex, SpellImplicitTargetInfo const& targetType)
{
    //sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "Spell::SelectImplicitTargetObjectTargets Id %u effIndex %u", m_spellInfo->Id, effIndex);

    //ASSERT((m_targets.GetObjectTarget() || m_targets.GetItemTarget()) && "Spell::SelectImplicitTargetObjectTargets - no explicit object or item target available!");
    if(!m_targets.GetObjectTarget() && !m_targets.GetItemTarget())
        return;

    WorldObject* target = m_targets.GetObjectTarget();

    CallScriptObjectTargetSelectHandlers(target, effIndex);

    if (target)
    {
        if (Unit* unit = target->ToUnit())
            AddUnitTarget(unit, 1 << effIndex, true, false);
        else if (GameObject* gobj = target->ToGameObject())
            AddGOTarget(gobj, 1 << effIndex);

        SelectImplicitChainTargets(effIndex, targetType, target, 1 << effIndex);
    }
    // Script hook can remove object target and we would wrongly land here
    else if (Item* item = m_targets.GetItemTarget())
        AddItemTarget(item, 1 << effIndex);
}

void Spell::SelectImplicitChainTargets(SpellEffIndex effIndex, SpellImplicitTargetInfo const& targetType, WorldObject* target, uint32 effMask)
{
    int32 maxTargets = m_spellInfo->GetEffect(effIndex, m_diffMode)->ChainTarget;
    if (Player* modOwner = m_caster->GetSpellModOwner())
        modOwner->ApplySpellMod(m_spellInfo->Id, SPELLMOD_JUMP_TARGETS, maxTargets, this);
    if(maxTargets < 0)
        maxTargets = m_spellInfo->GetEffect(effIndex, m_diffMode)->ChainTarget;

    // Havoc
    if (Aura* _aura = m_caster->GetAura(80240))
    {
        std::list<Unit*> _targetUnitList;
        Trinity::AnyUnitHavingBuffInObjectRangeCheck u_check(m_caster, m_caster, 45, 80240, false);
        Trinity::UnitListSearcher<Trinity::AnyUnitHavingBuffInObjectRangeCheck> searcher(m_caster, _targetUnitList, u_check);
        m_caster->VisitNearbyObject(40, searcher);

        if ((m_spellInfo->Id == 116858 && _aura->GetStackAmount() > 2) || m_spellInfo->Id != 116858)
        {
            for (std::list<Unit*>::const_iterator itr = _targetUnitList.begin(); itr != _targetUnitList.end(); ++itr)
            {
                if (Unit* unitTarget = (*itr)->ToUnit())
                {
                    AddUnitTarget(unitTarget, effMask, false);
                    find_target = true;
                }
            }
        }
    }

    if (maxTargets > 1)
    {
        // mark damage multipliers as used
        for (uint32 k = effIndex; k < MAX_SPELL_EFFECTS; ++k)
            if (effMask & (1 << k))
                m_damageMultipliers[k] = 1.0f;
        m_applyMultiplierMask |= effMask;

        std::list<WorldObject*> targets;
        SearchChainTargets(targets, maxTargets - 1, target, targetType.GetObjectType(), targetType.GetCheckType()
            , m_spellInfo->Effects[effIndex].ImplicitTargetConditions, targetType.GetTarget() == TARGET_UNIT_TARGET_CHAINHEAL_ALLY);

        // Chain primary target is added earlier
        CallScriptObjectAreaTargetSelectHandlers(targets, effIndex, targetType.GetTarget());

        // for backward compability
        std::list<Unit*> unitTargets;
        for (std::list<WorldObject*>::iterator itr = targets.begin(); itr != targets.end(); ++itr)
            if (Unit* unitTarget = (*itr)->ToUnit())
                unitTargets.push_back(unitTarget);

        for (std::list<Unit*>::iterator itr = unitTargets.begin(); itr != unitTargets.end(); ++itr)
            AddUnitTarget(*itr, effMask, false);
    }
    switch (m_spellInfo->Id)
    {
        case 120755: // Glave Toss for visual back(off-like)
        case 120756: // Glave Toss for visual back(off-like)
        case 31935: // Avenger's Shield
            AddTargetVisualHit(m_caster);
            break;
        default:
            break;
    }
}

float tangent(float x)
{
    x = tan(x);
    //if (x < std::numeric_limits<float>::max() && x > -std::numeric_limits<float>::max()) return x;
    //if (x >= std::numeric_limits<float>::max()) return std::numeric_limits<float>::max();
    //if (x <= -std::numeric_limits<float>::max()) return -std::numeric_limits<float>::max();
    if (x < 100000.0f && x > -100000.0f) return x;
    if (x >= 100000.0f) return 100000.0f;
    if (x <= 100000.0f) return -100000.0f;
    return 0.0f;
}

#define DEBUG_TRAJ(a) //a

void Spell::SelectImplicitTrajTargets()
{
    if (!m_targets.HasTraj())
        return;

    float dist2d = m_targets.GetDist2d();
    if (!dist2d)
        return;

    float srcToDestDelta = m_targets.GetDstPos()->m_positionZ - m_targets.GetSrcPos()->m_positionZ;

    std::list<WorldObject*> targets;
    Trinity::WorldObjectSpellTrajTargetCheck check(dist2d, m_targets.GetSrcPos(), m_caster, m_spellInfo);
    Trinity::WorldObjectListSearcher<Trinity::WorldObjectSpellTrajTargetCheck> searcher(m_caster, targets, check, GRID_MAP_TYPE_MASK_ALL);
    SearchTargets<Trinity::WorldObjectListSearcher<Trinity::WorldObjectSpellTrajTargetCheck> > (searcher, GRID_MAP_TYPE_MASK_ALL, m_caster, m_targets.GetSrcPos(), dist2d);
    if (targets.empty())
        return;

    targets.sort(Trinity::ObjectDistanceOrderPred(m_caster));

    float b = tangent(m_targets.GetElevation());
    float a = (srcToDestDelta - dist2d * b) / (dist2d * dist2d);
    if (a > -0.0001f)
        a = 0;
    DEBUG_TRAJ(sLog->outError(LOG_FILTER_SPELLS_AURAS, "Spell::SelectTrajTargets: a %f b %f", a, b);)

    float bestDist = m_spellInfo->GetMaxRange(false);

    std::list<WorldObject*>::const_iterator itr = targets.begin();
    for (; itr != targets.end(); ++itr)
    {
        if (Unit* unitTarget = (*itr)->ToUnit())
            if (m_caster == *itr || m_caster->IsOnVehicle(unitTarget) || (unitTarget)->GetVehicle())//(*itr)->IsOnVehicle(m_caster))
                continue;

        const float size = std::max((*itr)->GetObjectSize() * 0.7f, 1.0f); // 1/sqrt(3)
        // TODO: all calculation should be based on src instead of m_caster
        const float objDist2d = m_targets.GetSrcPos()->GetExactDist2d(*itr) * std::cos(m_targets.GetSrcPos()->GetRelativeAngle(*itr));
        const float dz = (*itr)->GetPositionZ() - m_targets.GetSrcPos()->m_positionZ;

        DEBUG_TRAJ(sLog->outError(LOG_FILTER_SPELLS_AURAS, "Spell::SelectTrajTargets: check %u, dist between %f %f, height between %f %f.", (*itr)->GetEntry(), objDist2d - size, objDist2d + size, dz - size, dz + size);)

        float dist = objDist2d - size;
        float height = dist * (a * dist + b);
        DEBUG_TRAJ(sLog->outError(LOG_FILTER_SPELLS_AURAS, "Spell::SelectTrajTargets: dist %f, height %f.", dist, height);)
        if (dist < bestDist && height < dz + size && height > dz - size)
        {
            bestDist = dist > 0 ? dist : 0;
            break;
        }

#define CHECK_DIST {\
            DEBUG_TRAJ(sLog->outError(LOG_FILTER_SPELLS_AURAS, "Spell::SelectTrajTargets: dist %f, height %f.", dist, height);)\
            if (dist > bestDist)\
                continue;\
            if (dist < objDist2d + size && dist > objDist2d - size)\
            {\
                bestDist = dist;\
                break;\
            }\
        }

        if (!a)
        {
            height = dz - size;
            dist = height / b;
            CHECK_DIST;

            height = dz + size;
            dist = height / b;
            CHECK_DIST;

            continue;
        }

        height = dz - size;
        float sqrt1 = b * b + 4 * a * height;
        if (sqrt1 > 0)
        {
            sqrt1 = sqrt(sqrt1);
            dist = (sqrt1 - b) / (2 * a);
            CHECK_DIST;
        }

        height = dz + size;
        float sqrt2 = b * b + 4 * a * height;
        if (sqrt2 > 0)
        {
            sqrt2 = sqrt(sqrt2);
            dist = (sqrt2 - b) / (2 * a);
            CHECK_DIST;

            dist = (-sqrt2 - b) / (2 * a);
            CHECK_DIST;
        }

        if (sqrt1 > 0)
        {
            dist = (-sqrt1 - b) / (2 * a);
            CHECK_DIST;
        }
    }

    if (m_targets.GetSrcPos()->GetExactDist2d(m_targets.GetDstPos()) > bestDist)
    {
        float x = m_targets.GetSrcPos()->m_positionX + std::cos(m_caster->GetOrientation()) * bestDist;
        float y = m_targets.GetSrcPos()->m_positionY + std::sin(m_caster->GetOrientation()) * bestDist;
        float z = m_targets.GetSrcPos()->m_positionZ + bestDist * (a * bestDist + b);

        if (itr != targets.end())
        {
            float distSq = (*itr)->GetExactDistSq(x, y, z);
            float sizeSq = (*itr)->GetObjectSize();
            sizeSq *= sizeSq;
            DEBUG_TRAJ(sLog->outError(LOG_FILTER_SPELLS_AURAS, "Initial %f %f %f %f %f", x, y, z, distSq, sizeSq);)
            if (distSq > sizeSq)
            {
                float factor = 1 - sqrt(sizeSq / distSq);
                x += factor * ((*itr)->GetPositionX() - x);
                y += factor * ((*itr)->GetPositionY() - y);
                z += factor * ((*itr)->GetPositionZ() - z);

                distSq = (*itr)->GetExactDistSq(x, y, z);
                DEBUG_TRAJ(sLog->outError(LOG_FILTER_SPELLS_AURAS, "Initial %f %f %f %f %f", x, y, z, distSq, sizeSq);)
            }
        }

        Position trajDst;
        trajDst.Relocate(x, y, z, m_caster->GetOrientation());
        m_targets.ModDst(trajDst);
    }
}

void Spell::SelectEffectTypeImplicitTargets(uint8 effIndex)
{
    // special case for SPELL_EFFECT_SUMMON_RAF_FRIEND and SPELL_EFFECT_SUMMON_PLAYER
    // TODO: this is a workaround - target shouldn't be stored in target map for those spells
    switch (m_spellInfo->GetEffect(effIndex, m_diffMode)->Effect)
    {
        case SPELL_EFFECT_SUMMON_RAF_FRIEND:
        case SPELL_EFFECT_SUMMON_PLAYER:
            if (m_caster->GetTypeId() == TYPEID_PLAYER && m_caster->ToPlayer()->GetSelection())
            {
                WorldObject* target = ObjectAccessor::FindPlayer(m_caster->ToPlayer()->GetSelection());

                CallScriptObjectTargetSelectHandlers(target, SpellEffIndex(effIndex));

                if (target && target->ToPlayer())
                    AddUnitTarget(target->ToUnit(), 1 << effIndex, false);
            }
            return;
        default:
            break;
    }

    // select spell implicit targets based on effect type
    if (!m_spellInfo->GetEffect(effIndex, m_diffMode)->GetImplicitTargetType())
        return;

    uint32 targetMask = m_spellInfo->GetEffect(effIndex, m_diffMode)->GetMissingTargetMask();

    if (!targetMask)
        return;

    WorldObject* target = NULL;

    switch (m_spellInfo->GetEffect(effIndex, m_diffMode)->GetImplicitTargetType())
    {
        // add explicit object target or self to the target map
        case EFFECT_IMPLICIT_TARGET_EXPLICIT:
            // player which not released his spirit is Unit, but target flag for it is TARGET_FLAG_CORPSE_MASK
            if (targetMask & (TARGET_FLAG_UNIT_MASK | TARGET_FLAG_CORPSE_MASK))
            {
                if (Unit* unitTarget = m_targets.GetUnitTarget())
                    target = unitTarget;
                else if (targetMask & TARGET_FLAG_CORPSE_MASK)
                {
                    if (Corpse* corpseTarget = m_targets.GetCorpseTarget())
                    {
                        // TODO: this is a workaround - corpses should be added to spell target map too, but we can't do that so we add owner instead
                        if (Player* owner = ObjectAccessor::FindPlayer(corpseTarget->GetOwnerGUID()))
                            target = owner;
                    }
                }
                else //if (targetMask & TARGET_FLAG_UNIT_MASK)
                    target = m_caster;
            }
            if (targetMask & TARGET_FLAG_ITEM_MASK)
            {
                if (Item* itemTarget = m_targets.GetItemTarget())
                    AddItemTarget(itemTarget, 1 << effIndex);
                return;
            }
            if (targetMask & TARGET_FLAG_GAMEOBJECT_MASK)
                target = m_targets.GetGOTarget();
            break;
        // add self to the target map
        case EFFECT_IMPLICIT_TARGET_CASTER:
            if (targetMask & TARGET_FLAG_UNIT_MASK)
                target = m_caster;
            break;
        default:
            break;
    }

    CallScriptObjectTargetSelectHandlers(target, SpellEffIndex(effIndex));

    if (target)
    {
        if (target->ToUnit())
            AddUnitTarget(target->ToUnit(), 1 << effIndex, false);
        else if (target->ToGameObject())
            AddGOTarget(target->ToGameObject(), 1 << effIndex);
    }
}

uint32 Spell::GetSearcherTypeMask(SpellTargetObjectTypes objType, ConditionList* condList)
{
    // this function selects which containers need to be searched for spell target
    uint32 retMask = GRID_MAP_TYPE_MASK_ALL;

    // filter searchers based on searched object type
    switch (objType)
    {
        case TARGET_OBJECT_TYPE_UNIT:
        case TARGET_OBJECT_TYPE_UNIT_AND_DEST:
        case TARGET_OBJECT_TYPE_CORPSE:
        case TARGET_OBJECT_TYPE_CORPSE_ENEMY:
        case TARGET_OBJECT_TYPE_CORPSE_ALLY:
            retMask &= GRID_MAP_TYPE_MASK_PLAYER | GRID_MAP_TYPE_MASK_CORPSE | GRID_MAP_TYPE_MASK_CREATURE;
            break;
        case TARGET_OBJECT_TYPE_GOBJ:
        case TARGET_OBJECT_TYPE_GOBJ_ITEM:
            retMask &= GRID_MAP_TYPE_MASK_GAMEOBJECT;
            break;
        default:
            break;
    }
    if (!(AttributesCustomEx2 & SPELL_ATTR2_CAN_TARGET_DEAD))
        retMask &= ~GRID_MAP_TYPE_MASK_CORPSE;
    if (AttributesCustomEx3 & SPELL_ATTR3_ONLY_TARGET_PLAYERS)
        retMask &= GRID_MAP_TYPE_MASK_CORPSE | GRID_MAP_TYPE_MASK_PLAYER;
    if (AttributesCustomEx3 & SPELL_ATTR3_ONLY_TARGET_GHOSTS)
        retMask &= GRID_MAP_TYPE_MASK_PLAYER;

    if (condList)
        retMask &= sConditionMgr->GetSearcherTypeMaskForConditionList(*condList);
    return retMask;
}

template<class SEARCHER>
void Spell::SearchTargets(SEARCHER& searcher, uint32 containerMask, Unit* referer, Position const* pos, float radius)
{
    if (!containerMask)
        return;

    // search world and grid for possible targets
    bool searchInGrid = containerMask & (GRID_MAP_TYPE_MASK_CREATURE | GRID_MAP_TYPE_MASK_GAMEOBJECT);
    bool searchInWorld = containerMask & (GRID_MAP_TYPE_MASK_CREATURE | GRID_MAP_TYPE_MASK_PLAYER | GRID_MAP_TYPE_MASK_CORPSE);
    if (searchInGrid || searchInWorld)
    {
        float x,y;
        x = pos->GetPositionX();
        y = pos->GetPositionY();

        CellCoord p(Trinity::ComputeCellCoord(x, y));
        Cell cell(p);
        cell.SetNoCreate();

        Map& map = *(referer->GetMap());

        if (searchInWorld)
        {
            TypeContainerVisitor<SEARCHER, WorldTypeMapContainer> world_object_notifier(searcher);
            cell.Visit(p, world_object_notifier, map, radius, x, y);
        }
        if (searchInGrid)
        {
            TypeContainerVisitor<SEARCHER, GridTypeMapContainer >  grid_object_notifier(searcher);
            cell.Visit(p, grid_object_notifier, map, radius, x , y);
        }
    }
}

WorldObject* Spell::SearchNearbyTarget(float range, SpellTargetObjectTypes objectType, SpellTargetCheckTypes selectionType, ConditionList* condList, SpellEffIndex effIndex)
{
    WorldObject* target = NULL;
    uint32 containerTypeMask = GetSearcherTypeMask(objectType, condList);
    if (!containerTypeMask)
        return NULL;
    Trinity::WorldObjectSpellNearbyTargetCheck check(range, m_caster, m_spellInfo, selectionType, condList, effIndex);
    Trinity::WorldObjectLastSearcher<Trinity::WorldObjectSpellNearbyTargetCheck> searcher(m_caster, target, check, containerTypeMask);
    SearchTargets<Trinity::WorldObjectLastSearcher<Trinity::WorldObjectSpellNearbyTargetCheck> > (searcher, containerTypeMask, m_caster, m_caster, range);
    return target;
}

void Spell::SearchAreaTargets(std::list<WorldObject*>& targets, float range, Position const* position, Unit* referer, SpellTargetObjectTypes objectType, SpellTargetCheckTypes selectionType, ConditionList* condList, SpellEffIndex effIndex)
{
    uint32 containerTypeMask = GetSearcherTypeMask(objectType, condList);
    if (!containerTypeMask)
        return;
    Unit* caster = m_originalCaster ? m_originalCaster : m_caster;
    Trinity::WorldObjectSpellAreaTargetCheck check(range, position, caster, referer, m_spellInfo, selectionType, condList, effIndex);
    Trinity::WorldObjectListSearcher<Trinity::WorldObjectSpellAreaTargetCheck> searcher(caster, targets, check, containerTypeMask);
    SearchTargets<Trinity::WorldObjectListSearcher<Trinity::WorldObjectSpellAreaTargetCheck> > (searcher, containerTypeMask, caster, position, range);
}

void Spell::SearchChainTargets(std::list<WorldObject*>& targets, uint32 chainTargets, WorldObject* target, SpellTargetObjectTypes objectType, SpellTargetCheckTypes selectType, ConditionList* condList, bool isChainHeal, SpellEffIndex effIndex)
{
    // max dist for jump target selection
    float jumpRadius = 0.0f;
    switch (m_spellInfo->DmgClass)
    {
        case SPELL_DAMAGE_CLASS_RANGED:
            // 7.5y for multi shot
            jumpRadius = 7.5f;
            break;
        case SPELL_DAMAGE_CLASS_MELEE:
            // 5y for swipe, cleave and similar
            jumpRadius = 5.0f;
            break;
        case SPELL_DAMAGE_CLASS_NONE:
        case SPELL_DAMAGE_CLASS_MAGIC:
            // 12.5y for chain heal spell since 3.2 patch
            if (isChainHeal)
                jumpRadius = 12.5f;
            // 10y as default for magic chain spells
            else
                jumpRadius = 10.0f;
            break;
    }

    if (Player* modOwner = m_caster->GetSpellModOwner())
        modOwner->ApplySpellMod(m_spellInfo->Id, SPELLMOD_JUMP_DISTANCE, jumpRadius);

    // chain lightning/heal spells and similar - allow to jump at larger distance and go out of los
    bool isBouncingFar = (AttributesCustomEx4 & SPELL_ATTR4_AREA_TARGET_CHAIN
        || m_spellInfo->DmgClass == SPELL_DAMAGE_CLASS_NONE
        || m_spellInfo->DmgClass == SPELL_DAMAGE_CLASS_MAGIC);

//     switch (m_spellInfo->Id)
//     {
//         case 102359: // Mass Entanglement
//             isBouncingFar = false;
//             break;
//         default:
//             break;
//     }

    // max dist which spell can reach
    std::list<uint64> tempGUIDs;
    WorldObject* nextTarget = target;
    while (chainTargets)
    {
        std::list<WorldObject*> tempTargets;
        SearchAreaTargets(tempTargets, jumpRadius, nextTarget, m_caster, objectType, selectType, condList);

        tempGUIDs.push_back(nextTarget->GetGUID());

        std::list<WorldObject*> removeTargets;

        for (std::list<uint64>::iterator itr = tempGUIDs.begin(); itr != tempGUIDs.end(); ++itr)
            for (std::list<WorldObject*>::iterator i = tempTargets.begin(); i != tempTargets.end(); ++i)
                if (Unit* unitTarget = (*i)->ToUnit())
                    if (*itr == unitTarget->GetGUID())
                        removeTargets.push_back(*i);

        for (std::list<WorldObject*>::iterator i = removeTargets.begin(); i != removeTargets.end(); ++i)
            tempTargets.remove(*i);

        if (!isBouncingFar)
        {
            for (std::list<WorldObject*>::iterator itr = tempTargets.begin(); itr != tempTargets.end();)
            {
                std::list<WorldObject*>::iterator checkItr = itr++;
                if (!m_caster->HasInArc(static_cast<float>(M_PI), *checkItr))
                    tempTargets.erase(checkItr);
            }
        }

        WorldObject* foundItr = NULL;

        if (isChainHeal)
        {
            uint32 maxHPDeficit = 0;
            for (std::list<WorldObject*>::iterator itr = tempTargets.begin(); itr != tempTargets.end(); ++itr)
            {
                if (Unit* unitTarget = (*itr)->ToUnit())
                {
                    uint32 deficit = unitTarget->GetMaxHealth() - unitTarget->GetHealth();
                    if ((deficit > maxHPDeficit || !foundItr) && nextTarget->IsWithinDist(unitTarget, jumpRadius) && nextTarget->IsWithinLOSInMap(unitTarget))
                    {
                        foundItr = unitTarget;
                        maxHPDeficit = deficit;
                    }
                }
            }
        }
        else
        {
            for (std::list<WorldObject*>::iterator itr = tempTargets.begin(); itr != tempTargets.end(); ++itr)
            {
                if (!foundItr)
                {
                    // isBouncingFar allow hit not in los target & IsWithinDist already checked at SearchAreaTargets
                    if (isBouncingFar || target->IsWithinLOSInMap(*itr))
                        foundItr = *itr;
                }
                else if (target->GetDistanceOrder(*itr, foundItr) && target->IsWithinLOSInMap(*itr))
                    foundItr = *itr;
            }
        }

        if (!foundItr)
            break;

        nextTarget = foundItr;
        targets.push_back(nextTarget);
        --chainTargets;
    }
}

void Spell::prepareDataForTriggerSystem(AuraEffect const* /*triggeredByAura**/)
{
    //==========================================================================================
    // Now fill data for trigger system, need know:
    // can spell trigger another or not (m_canTrigger)
    // Create base triggers flags for Attacker and Victim (m_procAttacker, m_procVictim and m_procEx)
    //==========================================================================================

    m_procVictim = m_procAttacker = 0;
    m_procEx = PROC_EX_NONE;

    /* Effects which are result of aura proc from triggered spell cannot proc
        to prevent chain proc of these spells */

    // Hellfire Effect - trigger as DOT
    if (m_spellInfo->SpellFamilyName == SPELLFAMILY_WARLOCK && m_spellInfo->SpellFamilyFlags[0] & 0x00000040)
    {
        m_procAttacker = PROC_FLAG_DONE_PERIODIC;
        m_procVictim   = PROC_FLAG_TAKEN_PERIODIC;
    }

    // Ranged autorepeat attack is set as triggered spell - ignore it
    if (!(m_procAttacker & PROC_FLAG_DONE_RANGED_AUTO_ATTACK))
    {
        if (_triggeredCastFlags & TRIGGERED_DISALLOW_PROC_EVENTS &&
            (AttributesCustomEx2 & SPELL_ATTR2_TRIGGERED_CAN_TRIGGER_PROC ||
            AttributesCustomEx3 & SPELL_ATTR3_TRIGGERED_CAN_TRIGGER_PROC_2))
            m_procEx |= PROC_EX_INTERNAL_CANT_PROC;
        else if (_triggeredCastFlags & TRIGGERED_DISALLOW_PROC_EVENTS)
            m_procEx |= PROC_EX_INTERNAL_TRIGGERED;
    }
}

void Spell::CleanupTargetList()
{
    m_UniqueTargetInfo.clear();
    m_UniqueGOTargetInfo.clear();
    m_UniqueItemInfo.clear();
    m_delayMoment = 0;
}

void Spell::AddUnitTarget(Unit* target, uint32 effectMask, bool checkIfValid /*= true*/, bool implicit /*= true*/)
{
    if(!target || !m_spellInfo)
        return;

    volatile uint32 spellid = m_spellInfo->Id;

    for (uint32 effIndex = 0; effIndex < MAX_SPELL_EFFECTS; ++effIndex)
        if (!m_spellInfo->GetEffect(effIndex, m_diffMode)->IsEffect() || !CheckEffectTarget(target, effIndex) || CheckEffFromDummy(target, effIndex))
            effectMask &= ~(1 << effIndex);

    // no effects left
    if (!effectMask)
        return;

    if (checkIfValid)
        if (m_spellInfo->CheckTarget(m_caster, target, implicit) != SPELL_CAST_OK)
            return;

    if (AttributesCustomEx7 & SPELL_ATTR7_CONSOLIDATED_RAID_BUFF)
        if (target->GetTypeId() != TYPEID_PLAYER && target != m_caster)
            if (target->GetOwner() && target->GetOwner()->GetTypeId() == TYPEID_PLAYER)
                return;

    // Check for effect immune skip if immuned
    for (uint32 effIndex = 0; effIndex < MAX_SPELL_EFFECTS; ++effIndex)
        if (target->IsImmunedToSpellEffect(m_spellInfo, effIndex))
            effectMask &= ~(1 << effIndex);

    uint64 targetGUID = target->GetGUID();

    // Lookup target in already in list
    for (std::list<TargetInfo>::iterator ihit = m_UniqueTargetInfo.begin(); ihit != m_UniqueTargetInfo.end(); ++ihit)
    {
        if (targetGUID == ihit->targetGUID)             // Found in list
        {
            ihit->effectMask |= effectMask;             // Immune effects removed from mask
            ihit->scaleAura = false;
            if (m_auraScaleMask && ihit->effectMask == m_auraScaleMask && m_caster != target)
            {
                SpellInfo const* auraSpell = sSpellMgr->GetSpellInfo(sSpellMgr->GetFirstSpellInChain(m_spellInfo->Id));
                if (uint32(target->getLevel() + 10) >= auraSpell->SpellLevel)
                    ihit->scaleAura = true;
            }
            return;
        }
    }

    // This is new target calculate data for him

    // Get spell hit result on target
    TargetInfo targetInfo;
    targetInfo.targetGUID       = targetGUID;                         // Store target GUID
    targetInfo.timeDelay        = 0LL;
    targetInfo.effectMask       = effectMask;                         // Store all effects not immune
    targetInfo.processed        = false;                              // Effects not apply on target
    targetInfo.alive            = target->isAlive();
    targetInfo.damage           = 0;
    targetInfo.damageBeforeHit  = 0;
    targetInfo.crit             = false;
    targetInfo.scaleAura        = false;

    if (m_auraScaleMask && targetInfo.effectMask == m_auraScaleMask && m_caster != target)
    {
        SpellInfo const* auraSpell = sSpellMgr->GetSpellInfo(sSpellMgr->GetFirstSpellInChain(m_spellInfo->Id));
        if (uint32(target->getLevel() + 10) >= auraSpell->SpellLevel)
            targetInfo.scaleAura = true;
    }

    // Calculate hit result
    if (m_originalCaster)
    {
        targetInfo.missCondition = m_originalCaster->SpellHitResult(target, m_spellInfo, m_canReflect, effectMask);
        if (m_skipCheck && targetInfo.missCondition != SPELL_MISS_IMMUNE)
            targetInfo.missCondition = SPELL_MISS_NONE;
    }
    else
        targetInfo.missCondition = SPELL_MISS_EVADE; //SPELL_MISS_NONE;

    // Spell have speed - need calculate incoming time
    // Incoming time is zero for self casts. At least I think so.
    if (m_spellInfo->Speed > 0.0f && m_caster != target)
    {
        float mindist = 5.0f;
        switch (m_spellInfo->Id)
        {
            case 132467: // Chi Wave Neg
            case 132464: // Chi Wave Pos
                mindist = 0.0f;
                break;
            default:
                break;
        }
        // calculate spell incoming interval
        // TODO: this is a hack
        float dist = m_caster->GetDistance(target->GetPositionX(), target->GetPositionY(), target->GetPositionZ());

        if (dist < mindist)
            dist = mindist;

        if (!(AttributesCustomEx9 & SPELL_ATTR9_SPECIAL_DELAY_CALCULATION))
            targetInfo.timeDelay = uint64(floor(dist / m_spellInfo->Speed * 1000.0f));
        else
            targetInfo.timeDelay = uint64(m_spellInfo->Speed * 1000.0f);

        // Calculate minimum incoming time
        if (m_delayMoment == 0 || m_delayMoment > targetInfo.timeDelay)
            m_delayMoment = targetInfo.timeDelay;
    }
    else if((m_caster->GetTypeId() == TYPEID_PLAYER || (m_caster->ToCreature() && m_caster->ToCreature()->isPet())) && m_caster != target)
    {
        // TimeDelay for crowd control effects
        if (IsCCSpell(m_spellInfo, 0, false) && m_spellInfo->ExplicitTargetMask != TARGET_FLAG_DEST_LOCATION)
        {
            targetInfo.timeDelay = 200LL;
            m_delayMoment = 200LL;
        }
        if (!m_spellInfo->IsPositive() && (!_triggeredCastFlags || m_spellInfo->SpellIconID == 156) && m_spellInfo->ExplicitTargetMask != TARGET_FLAG_DEST_LOCATION)
        {
            switch(m_spellInfo->Effects[0].Effect)
            {
                case SPELL_EFFECT_INTERRUPT_CAST:
                case SPELL_EFFECT_APPLY_AURA:
                case SPELL_EFFECT_DISPEL:
                case SPELL_EFFECT_SCHOOL_DAMAGE:
                case SPELL_EFFECT_POWER_BURN:
                case SPELL_EFFECT_WEAPON_DAMAGE_NOSCHOOL:
                case SPELL_EFFECT_WEAPON_PERCENT_DAMAGE:
                case SPELL_EFFECT_WEAPON_DAMAGE:
                case SPELL_EFFECT_NORMALIZED_WEAPON_DMG:
                {
                    targetInfo.timeDelay = 200LL;
                    m_delayMoment = 200LL;
                }
                default:
                    break;
            }
        }
    }
    else
        targetInfo.timeDelay = 0LL;

    if (m_spellInfo->AttributesCu & SPELL_ATTR0_CU_HAVE_STABLE_FLYTIME)
    {
        targetInfo.timeDelay = 200LL;
        m_delayMoment = 200LL;
    }

    // If target reflect spell back to caster
    if (targetInfo.missCondition == SPELL_MISS_REFLECT)
    {
        // process reflect removal (not delayed)
        if (!targetInfo.timeDelay)
        {
            DamageInfo dmgInfoProc = DamageInfo(m_caster, target, 1, m_spellInfo, m_spellInfo ? SpellSchoolMask(m_spellInfo->SchoolMask) : SPELL_SCHOOL_MASK_NORMAL, SPELL_DIRECT_DAMAGE, targetInfo.damageBeforeHit);
            m_caster->ProcDamageAndSpell(target, PROC_FLAG_NONE, PROC_FLAG_TAKEN_SPELL_MAGIC_DMG_CLASS_NEG, PROC_EX_REFLECT, &dmgInfoProc, BASE_ATTACK, m_spellInfo);
        }

        // Calculate reflected spell result on caster
        targetInfo.reflectResult = m_caster->SpellHitResult(m_caster, m_spellInfo, m_canReflect, effectMask);

        if (targetInfo.reflectResult == SPELL_MISS_REFLECT)     // Impossible reflect again, so simply deflect spell
            targetInfo.reflectResult = SPELL_MISS_PARRY;

        // Increase time interval for reflected spells by 1.5
        targetInfo.timeDelay += targetInfo.timeDelay >> 1;
    }
    else
        targetInfo.reflectResult = SPELL_MISS_NONE;

    // Add target to list
    m_UniqueTargetInfo.push_back(targetInfo);
}

void Spell::AddTargetVisualHit(Unit* target)
{
    if(!m_spellInfo)
        return;

    // Get spell hit result on target
    TargetInfo targetInfo;
    targetInfo.targetGUID = target->GetGUID();                         // Store target GUID
    targetInfo.effectMask = 7;                         // Store all effects not immune
    targetInfo.processed  = false;                              // Effects not apply on target
    targetInfo.alive      = target->isAlive();
    targetInfo.damage     = 0;
    targetInfo.crit       = false;
    targetInfo.scaleAura  = false;

    // Add target to list
    m_VisualHitTargetInfo.push_back(targetInfo);
}

void Spell::AddGOTarget(GameObject* go, uint32 effectMask)
{
    for (uint32 effIndex = 0; effIndex < MAX_SPELL_EFFECTS; ++effIndex)
    {
        if (!m_spellInfo->GetEffect(effIndex, m_diffMode)->IsEffect())
            effectMask &= ~(1 << effIndex);
        else
        {
            switch (m_spellInfo->GetEffect(effIndex, m_diffMode)->Effect)
            {
            case SPELL_EFFECT_GAMEOBJECT_DAMAGE:
            case SPELL_EFFECT_GAMEOBJECT_REPAIR:
            case SPELL_EFFECT_GAMEOBJECT_SET_DESTRUCTION_STATE:
                if (go->GetGoType() != GAMEOBJECT_TYPE_DESTRUCTIBLE_BUILDING)
                    effectMask &= ~(1 << effIndex);
                break;
            default:
                break;
            }
        }
    }

    if (!effectMask)
        return;

    uint64 targetGUID = go->GetGUID();

    // Lookup target in already in list
    for (std::list<GOTargetInfo>::iterator ihit = m_UniqueGOTargetInfo.begin(); ihit != m_UniqueGOTargetInfo.end(); ++ihit)
    {
        if (targetGUID == ihit->targetGUID)                 // Found in list
        {
            ihit->effectMask |= effectMask;                 // Add only effect mask
            return;
        }
    }

    // This is new target calculate data for him

    GOTargetInfo target;
    target.targetGUID = targetGUID;
    target.effectMask = effectMask;
    target.processed  = false;                              // Effects not apply on target

    // Spell have speed - need calculate incoming time
    if (m_spellInfo->Speed > 0.0f)
    {
        // calculate spell incoming interval
        float dist = m_caster->GetDistance(go->GetPositionX(), go->GetPositionY(), go->GetPositionZ());
        if (dist < 5.0f)
            dist = 5.0f;
       if (!(AttributesCustomEx9 & SPELL_ATTR9_SPECIAL_DELAY_CALCULATION))
           target.timeDelay = uint64(floor(dist / m_spellInfo->Speed * 1000.0f));
       else
           target.timeDelay = uint64(m_spellInfo->Speed * 1000.0f);

        if (m_delayMoment == 0 || m_delayMoment > target.timeDelay)
            m_delayMoment = target.timeDelay;
    }
    else
        target.timeDelay = 0LL;

    // Add target to list
    m_UniqueGOTargetInfo.push_back(target);
}

void Spell::AddItemTarget(Item* item, uint32 effectMask)
{
    for (uint32 effIndex = 0; effIndex < MAX_SPELL_EFFECTS; ++effIndex)
        if (!m_spellInfo->GetEffect(effIndex, m_diffMode)->IsEffect())
            effectMask &= ~(1 << effIndex);

    // no effects left
    if (!effectMask)
        return;

    // Lookup target in already in list
    for (std::list<ItemTargetInfo>::iterator ihit = m_UniqueItemInfo.begin(); ihit != m_UniqueItemInfo.end(); ++ihit)
    {
        if (item == ihit->item)                            // Found in list
        {
            ihit->effectMask |= effectMask;                 // Add only effect mask
            return;
        }
    }

    // This is new target add data

    ItemTargetInfo target;
    target.item       = item;
    target.effectMask = effectMask;

    m_UniqueItemInfo.push_back(target);
}

void Spell::AddDestTarget(SpellDestination const& dest, uint32 effIndex)
{
    m_destTargets[effIndex] = dest;
}

void Spell::DoAllEffectOnTarget(TargetInfo* target)
{
    if (!target || target->processed)
        return;

    target->processed = true;                               // Target checked in apply effects procedure

    // Get mask of effects for target
    uint32 mask = target->effectMask;

    // Reset damage/healing counter
    m_damage = target->damage;
    m_healing = -target->damage;

    Unit* unit = m_caster->GetGUID() == target->targetGUID ? m_caster : ObjectAccessor::GetUnit(*m_caster, target->targetGUID);
    if (!unit)
    {
        uint32 farMask = 0;
        // create far target mask
        for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
            if (m_spellInfo->GetEffect(i, m_diffMode)->IsFarUnitTargetEffect())
                if ((1 << i) & mask)
                    farMask |= (1 << i);

        if (!farMask)
            return;
        // find unit in world
        unit = ObjectAccessor::FindUnit(target->targetGUID);
        if (!unit)
            return;

        // do far effects on the unit
        // can't use default call because of threading, do stuff as fast as possible
        for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
            if (farMask & (1 << i))
                HandleEffects(unit, NULL, NULL, i, SPELL_EFFECT_HANDLE_HIT_TARGET);
        return;
    }

    if (unit->isAlive() != target->alive)
        return;

    //if (m_spellInfo)
    //if (getState() == SPELL_STATE_DELAYED && !m_spellInfo->IsPositive() && (getMSTime() - target->timeDelay) <= unit->m_lastSanctuaryTime)
    //return;                                             // No missinfo in that case

    // Some spells should remove Camouflage after hit (traps, some spells that have casting time)
    if (target->targetGUID != m_caster->GetGUID() && m_spellInfo && m_spellInfo->IsBreakingCamouflageAfterHit())
    {
        if (TempSummon* summon = m_caster->ToTempSummon())
        {
            if (Unit* owner = summon->GetSummoner())
                owner->RemoveAurasByType(SPELL_AURA_MOD_CAMOUFLAGE);
        }
        else
            m_caster->RemoveAurasByType(SPELL_AURA_MOD_CAMOUFLAGE);
    }

    // Get original caster (if exist) and calculate damage/healing from him data
    Unit* caster = m_originalCaster ? m_originalCaster : m_caster;

    // Skip if m_originalCaster not avaiable
    if (!caster)
        return;

    SpellMissInfo missInfo = target->missCondition;

    // Need init unitTarget by default unit (can changed in code on reflect)
    // Or on missInfo != SPELL_MISS_NONE unitTarget undefined (but need in trigger subsystem)
    unitTarget = unit;

    // Reset damage/healing counter
    m_absorb = 0;

    // Fill base trigger info
    uint32 procAttacker = m_procAttacker;
    uint32 procVictim = m_procVictim;
    uint32 procEx = m_procEx;

    m_spellAura = NULL; // Set aura to null for every target-make sure that pointer is not used for unit without aura applied

    //Check can or not triggered
    bool canEffectTrigger = m_spellInfo->CanSpellProc(unitTarget, mask, m_CastItem);
    Unit* spellHitTarget = NULL;

    if (missInfo == SPELL_MISS_NONE)                          // In case spell hit target, do all effect on that target
        spellHitTarget = unit;
    else if (missInfo == SPELL_MISS_REFLECT)                // In case spell reflect from target, do all effect on caster (if hit)
    {
        if (target->reflectResult == SPELL_MISS_NONE)       // If reflected spell hit caster -> do all effect on him
        {
            spellHitTarget = m_caster;
            if (m_caster->GetTypeId() == TYPEID_UNIT)
                m_caster->ToCreature()->LowerPlayerDamageReq(target->damage);
        }
    }

    if (m_spellInfo->Id == 143339 || m_spellInfo->Id == 142315) //Injection & Caustic Blood(Paragons of Klaxxi)
    {
        if (spellHitTarget)
        {
            if (spellHitTarget->HasAura(132403) || spellHitTarget->HasAura(132404) //Evade Auras
                || spellHitTarget->HasAura(132402) || spellHitTarget->HasAura(115308) || spellHitTarget->HasAura(77535))
            {
                m_damage = 0;
                spellHitTarget = NULL;
            }
        }
    }

    if (spellHitTarget)
    {
        SpellMissInfo missInfo2 = DoSpellHitOnUnit(spellHitTarget, mask, target->scaleAura);
        if (missInfo2 != SPELL_MISS_NONE)
        {
            if (missInfo2 != SPELL_MISS_MISS)
                m_caster->SendSpellMiss(unit, m_spellInfo->Id, missInfo2);
            m_damage = 0;
            spellHitTarget = NULL;
        }
    }

    // Do not take combo points on dodge and miss
    if (missInfo != SPELL_MISS_NONE && m_needComboPoints &&
            m_targets.GetUnitTargetGUID() == target->targetGUID)
    {
        m_needComboPoints = false;
        // Restore spell mods for a miss/dodge/parry Cold Blood
        // TODO: check how broad this rule should be
        if (m_caster->GetTypeId() == TYPEID_PLAYER && (missInfo == SPELL_MISS_MISS ||
                missInfo == SPELL_MISS_DODGE || missInfo == SPELL_MISS_PARRY))
            m_caster->ToPlayer()->RestoreSpellMods(this, 14177);
    }

    // Trigger info was not filled in spell::preparedatafortriggersystem - we do it now
    bool positive = true;
    if (canEffectTrigger && !procAttacker && !procVictim)
    {
        // Hunter trap spells - activation proc for Lock and Load, Entrapment and Misdirection
        if (m_spellInfo->SpellFamilyName == SPELLFAMILY_HUNTER &&
            (m_spellInfo->SpellFamilyFlags[0] & 0x18 ||     // Freezing and Frost Trap, Freezing Arrow
            m_spellInfo->Id == 57879 || m_spellInfo->Id == 13810 ||                     // Snake Trap - done this way to avoid double proc
            m_spellInfo->SpellFamilyFlags[2] & 0x00024000)) // Explosive and Immolation Trap
            procAttacker |= PROC_FLAG_DONE_TRAP_ACTIVATION;

        bool dmgSpell = bool(target->damage || target->damageBeforeHit);

        if (m_damage > 0)
            positive = false;
        else if (!m_healing)
        {
            for (uint8 i = 0; i< MAX_SPELL_EFFECTS; ++i)
                // If at least one effect negative spell is negative hit
                if (mask & (1<<i))
                {
                    if (!m_spellInfo->IsPositiveEffect(i))
                    {
                        if(m_spellInfo->Effects[i].ApplyAuraName == SPELL_AURA_PERIODIC_DAMAGE)
                            dmgSpell = true;
                        positive = false;
                    }
                    else if (!dmgSpell)
                        dmgSpell = m_spellInfo->Effects[i].ApplyAuraName == SPELL_AURA_SCHOOL_ABSORB;
                    break;
                }
        }
        switch (m_spellInfo->DmgClass)
        {
            case SPELL_DAMAGE_CLASS_MAGIC:
            {
                procAttacker |= PROC_FLAG_DONE_SPELL_DMG_POS_NEG;
                if (positive)
                {
                    procAttacker |= dmgSpell ? PROC_FLAG_DONE_SPELL_MAGIC_DMG_CLASS_POS : PROC_FLAG_DONE_SPELL_NONE_DMG_CLASS_POS;
                    procVictim   |= dmgSpell ? PROC_FLAG_TAKEN_SPELL_MAGIC_DMG_CLASS_POS : PROC_FLAG_TAKEN_SPELL_NONE_DMG_CLASS_POS;
                }
                else
                {
                    procAttacker |= dmgSpell ? PROC_FLAG_DONE_SPELL_MAGIC_DMG_CLASS_NEG : PROC_FLAG_DONE_SPELL_NONE_DMG_CLASS_NEG;
                    procVictim   |= dmgSpell ? PROC_FLAG_TAKEN_SPELL_MAGIC_DMG_CLASS_NEG : PROC_FLAG_TAKEN_SPELL_NONE_DMG_CLASS_NEG;
                }
            }
            break;
            case SPELL_DAMAGE_CLASS_MELEE:
            {
                procAttacker |= PROC_FLAG_DONE_SPELL_DMG_POS_NEG;
                procAttacker |= dmgSpell ? PROC_FLAG_DONE_SPELL_MELEE_DMG_CLASS : PROC_FLAG_DONE_SPELL_NONE_DMG_CLASS_NEG;

                if (dmgSpell)
                    procVictim |= m_caster->IsWithinMeleeRange(spellHitTarget) ? PROC_FLAG_TAKEN_SPELL_MELEE_DMG_CLASS : PROC_FLAG_TAKEN_SPELL_RANGED_DMG_CLASS;
                else
                    procVictim |= PROC_FLAG_TAKEN_SPELL_NONE_DMG_CLASS_NEG;

                if (m_attackType == BASE_ATTACK)
                    procAttacker |= PROC_FLAG_DONE_MAINHAND_ATTACK;
                else
                    procAttacker |= PROC_FLAG_DONE_OFFHAND_ATTACK;
                break;
            }
            case SPELL_DAMAGE_CLASS_NONE:
            {
                if (m_spellInfo->GetSchoolMask() & SPELL_SCHOOL_MASK_MAGIC)
                {
                    if (positive)
                    {
                        procAttacker |= dmgSpell ? PROC_FLAG_DONE_SPELL_MAGIC_DMG_CLASS_POS : PROC_FLAG_DONE_SPELL_NONE_DMG_CLASS_POS;
                        procVictim   |= dmgSpell ? PROC_FLAG_TAKEN_SPELL_MAGIC_DMG_CLASS_POS : PROC_FLAG_TAKEN_SPELL_NONE_DMG_CLASS_POS;
                    }
                    else
                    {
                        procAttacker |= dmgSpell ? PROC_FLAG_DONE_SPELL_MAGIC_DMG_CLASS_NEG : PROC_FLAG_DONE_SPELL_NONE_DMG_CLASS_NEG;
                        procVictim   |= dmgSpell ? PROC_FLAG_TAKEN_SPELL_MAGIC_DMG_CLASS_NEG : PROC_FLAG_TAKEN_SPELL_NONE_DMG_CLASS_NEG;
                    }
                }
                else
                {
                    if (positive)
                    {
                        procAttacker |= PROC_FLAG_DONE_SPELL_NONE_DMG_CLASS_POS;
                        procVictim   |= PROC_FLAG_TAKEN_SPELL_NONE_DMG_CLASS_POS;
                    }
                    else
                    {
                        procAttacker |= PROC_FLAG_DONE_SPELL_NONE_DMG_CLASS_NEG;
                        procVictim   |= PROC_FLAG_TAKEN_SPELL_NONE_DMG_CLASS_NEG;
                    }
                }
                break;
            }
            case SPELL_DAMAGE_CLASS_RANGED:
                // Auto attack
                if (AttributesCustomEx2 & SPELL_ATTR2_AUTOREPEAT_FLAG)
                {
                    procAttacker |= PROC_FLAG_DONE_RANGED_AUTO_ATTACK;
                    procVictim   |= PROC_FLAG_TAKEN_RANGED_AUTO_ATTACK;
                }
                else // Ranged spell attack
                {
                    procAttacker |= dmgSpell ? PROC_FLAG_DONE_SPELL_RANGED_DMG_CLASS : PROC_FLAG_DONE_SPELL_NONE_DMG_CLASS_NEG;
                    procVictim   |= dmgSpell ? PROC_FLAG_TAKEN_SPELL_RANGED_DMG_CLASS : PROC_FLAG_TAKEN_SPELL_NONE_DMG_CLASS_NEG;
                }
                break;
        }
    }
    CallScriptOnHitHandlers();

    //sLog->outDebug(LOG_FILTER_PROC, "DoAllEffectOnTarget: m_spellInfo->Id %i, mask %i, m_healing %i, canEffectTrigger %i, m_damage %i, m_addpower %i, procEx %i procAttacker %u procVictim %u damage %i damageBeforeHit %i",
    //m_spellInfo->Id, mask, m_healing, canEffectTrigger, m_damage, m_addpower, procEx, procAttacker, procVictim, target->damage, target->damageBeforeHit);

    // All calculated do it!
    // Do healing and triggers
    if (m_healing > 0)
    {
        uint32 addhealth = m_healing;
        if (target->crit)
        {
            procEx |= PROC_EX_CRITICAL_HIT;
            addhealth = caster->SpellCriticalHealingBonus(m_spellInfo, addhealth, NULL);
        }
        else
            procEx |= PROC_EX_NORMAL_HIT;

        int32 gain = caster->HealBySpell(unitTarget, m_spellInfo, addhealth, target->crit);
        unitTarget->getHostileRefManager().threatAssist(caster, float(gain) * 0.5f, m_spellInfo);
        m_healing = gain;

        // Do triggers for unit (reflect triggers passed on hit phase for correct drop charge)
        if (canEffectTrigger && missInfo != SPELL_MISS_REFLECT)
        {
            DamageInfo dmgInfoProc = DamageInfo(m_caster, unitTarget, addhealth, m_spellInfo, SpellSchoolMask(m_spellInfo->SchoolMask), SPELL_DIRECT_DAMAGE, target->damageBeforeHit);
            caster->ProcDamageAndSpell(unitTarget, procAttacker, procVictim, procEx, &dmgInfoProc, m_attackType, m_spellInfo, m_triggeredByAuraSpell);
        }
    }
    else if (m_addpower != 0)
    {
        // Do triggers for unit (reflect triggers passed on hit phase for correct drop charge)
        if (canEffectTrigger && missInfo != SPELL_MISS_REFLECT)
        {
            procEx |= PROC_EX_NORMAL_HIT;
            DamageInfo dmgInfoProc = DamageInfo(m_caster, m_targets.GetUnitTarget() ? m_targets.GetUnitTarget() : m_caster, 0, m_spellInfo, SpellSchoolMask(m_spellInfo->SchoolMask), SPELL_DIRECT_DAMAGE, target->damageBeforeHit);
            dmgInfoProc.SetAddPower(m_addpower);
            dmgInfoProc.SetAddPType(m_addptype);
            caster->ProcDamageAndSpell(unitTarget, procAttacker, procVictim, procEx, &dmgInfoProc, m_attackType, m_spellInfo, m_triggeredByAuraSpell);
        }
    }
    // Do damage and triggers
    else if (m_damage > 0)
    {
        // Fill base damage struct (unitTarget - is real spell target)
        SpellNonMeleeDamage damageInfo(caster, unitTarget, m_spellInfo->Id, m_spellSchoolMask);

        damageInfo.damageBeforeHit = target->damageBeforeHit;
        // Add bonuses and fill damageInfo struct
        caster->CalculateSpellDamageTaken(&damageInfo, m_damage, m_spellInfo, mask, m_attackType, target->crit);
        caster->DealDamageMods(damageInfo.target, damageInfo.damage, &damageInfo.absorb);

        m_absorb = damageInfo.absorb;

        // Send log damage message to client
        caster->SendSpellNonMeleeDamageLog(&damageInfo);

        procEx |= createProcExtendMask(&damageInfo, missInfo);
        procVictim |= PROC_FLAG_TAKEN_DAMAGE;

        m_damage = damageInfo.damage;

        caster->DealSpellDamage(&damageInfo, true);

        // Do triggers for unit (reflect triggers passed on hit phase for correct drop charge)
        if (canEffectTrigger && missInfo != SPELL_MISS_REFLECT)
        {
            DamageInfo dmgInfoProc = DamageInfo(damageInfo, m_spellInfo);

            caster->ProcDamageAndSpell(unitTarget, procAttacker, procVictim, procEx, &dmgInfoProc, m_attackType, m_spellInfo, m_triggeredByAuraSpell);
            if (caster->GetTypeId() == TYPEID_PLAYER && (AttributesCustom & SPELL_ATTR0_STOP_ATTACK_TARGET) == 0 &&
               (m_spellInfo->DmgClass == SPELL_DAMAGE_CLASS_MELEE || m_spellInfo->DmgClass == SPELL_DAMAGE_CLASS_RANGED))
                caster->ToPlayer()->CastItemCombatSpell(unitTarget, m_attackType, procVictim, procEx);
        }

        m_final_damage = damageInfo.damage;
        m_absorb = damageInfo.absorb;
        m_resist = damageInfo.resist;
        m_blocked = damageInfo.blocked;

        // Hunter's pet special attacks
        if (m_spellInfo->SpellFamilyName == SPELLFAMILY_HUNTER && m_spellInfo->SpellFamilyFlags[0] & 0x00080000)
            if (Unit * owner = caster->GetOwner())
            {
                // Cobra Strikes
                if (Aura* pAura = owner->GetAura(53257))
                    pAura->ModStackAmount(-1);
            }
    }
    // Passive spell hits/misses or active spells only misses (only triggers)
    else
    {
        // Fill base damage struct (unitTarget - is real spell target)
        SpellNonMeleeDamage damageInfo(caster, unitTarget, m_spellInfo->Id, m_spellSchoolMask);
        damageInfo.damageBeforeHit = target->damageBeforeHit;
        procEx |= createProcExtendMask(&damageInfo, missInfo);
        // Do triggers for unit (reflect triggers passed on hit phase for correct drop charge)
        if (canEffectTrigger && missInfo != SPELL_MISS_REFLECT)
        {
            DamageInfo dmgInfoProc = DamageInfo(m_caster, unit, 0, m_spellInfo, SpellSchoolMask(m_spellInfo->SchoolMask), SPELL_DIRECT_DAMAGE, damageInfo.damageBeforeHit);
            caster->ProcDamageAndSpell(unit, procAttacker, procVictim, procEx, &dmgInfoProc, m_attackType, m_spellInfo, m_triggeredByAuraSpell);
        }

        // Failed Pickpocket, reveal rogue
        if (missInfo == SPELL_MISS_RESIST && AttributesCustomCu & SPELL_ATTR0_CU_PICKPOCKET && unitTarget->GetTypeId() == TYPEID_UNIT)
        {
            m_caster->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_TALK);
            if (unitTarget->ToCreature()->IsAIEnabled)
                unitTarget->ToCreature()->AI()->AttackStart(m_caster);
        }
        m_absorb = damageInfo.absorb;
        m_resist = damageInfo.resist;
        m_blocked = damageInfo.blocked;
    }

    // process reflect removal (delayed)
    if (missInfo == SPELL_MISS_REFLECT && target->timeDelay)
    {
        DamageInfo dmgInfoProc = DamageInfo(m_caster, unit, 1, m_spellInfo, SpellSchoolMask(m_spellInfo->SchoolMask), SPELL_DIRECT_DAMAGE, target->damageBeforeHit);
        caster->ProcDamageAndSpell(unit, procAttacker, PROC_FLAG_TAKEN_SPELL_MAGIC_DMG_CLASS_NEG, procEx, &dmgInfoProc, BASE_ATTACK, m_spellInfo);
    }

    if (missInfo != SPELL_MISS_EVADE && m_caster->IsValidAttackTarget(unit) && (!m_spellInfo->IsPositive() || m_spellInfo->HasEffect(SPELL_EFFECT_DISPEL)))
    {
        m_caster->CombatStart(unit, !(AttributesCustomEx3 & SPELL_ATTR3_NO_INITIAL_AGGRO) && !((AttributesCustomEx & SPELL_ATTR1_NOT_BREAK_STEALTH) && !m_damage));

        if (AttributesCustomCu & SPELL_ATTR0_CU_AURA_CC)
            if (!unit->IsStandState())
                unit->SetStandState(UNIT_STAND_STATE_STAND);
    }

    if (spellHitTarget)
    {
        //AI functions
        if (spellHitTarget->GetTypeId() == TYPEID_UNIT)
        {
            if (spellHitTarget->ToCreature()->IsAIEnabled)
                spellHitTarget->ToCreature()->AI()->SpellHit(m_caster, m_spellInfo);

            // cast at creature (or GO) quest objectives update at successful cast finished (+channel finished)
            // ignore pets or autorepeat/melee casts for speed (not exist quest for spells (hm...)
            if (m_originalCaster && m_originalCaster->IsControlledByPlayer() && !spellHitTarget->ToCreature()->isPet() && !IsAutoRepeat() && !IsNextMeleeSwingSpell() && !IsChannelActive())
                if (Player* p = m_originalCaster->GetCharmerOrOwnerPlayerOrPlayerItself())
                    p->CastedCreatureOrGO(spellHitTarget->GetEntry(), spellHitTarget->GetGUID(), m_spellInfo->Id);
        }

        if (m_caster->GetTypeId() == TYPEID_UNIT && m_caster->ToCreature()->IsAIEnabled)
            m_caster->ToCreature()->AI()->SpellHitTarget(spellHitTarget, m_spellInfo);

        // Needs to be called after dealing damage/healing to not remove breaking on damage auras
        DoTriggersOnSpellHit(spellHitTarget, mask);

        // if target is fallged for pvp also flag caster if a player
        if (unit->IsPvP() && m_caster->GetTypeId() == TYPEID_PLAYER)
            m_caster->ToPlayer()->UpdatePvP(true);

        CallScriptAfterHitHandlers();
    }
}

TargetInfo* Spell::GetTargetInfo(uint64 targetGUID)
{
    TargetInfo* infoTarget = NULL;
    for (std::list<TargetInfo>::iterator ihit = m_UniqueTargetInfo.begin(); ihit != m_UniqueTargetInfo.end(); ++ihit)
        if (ihit->targetGUID == targetGUID)
            infoTarget = &(*ihit);

    return infoTarget;
}

SpellMissInfo Spell::DoSpellHitOnUnit(Unit* unit, uint32 effectMask, bool scaleAura)
{
    if (!unit || !effectMask)
        return SPELL_MISS_EVADE;

    // For delayed spells immunity may be applied between missile launch and hit - check immunity for that case
    if (m_spellInfo->Speed && (unit->IsImmunedToDamage(m_spellInfo) || unit->IsImmunedToSpell(m_spellInfo)))
        return SPELL_MISS_IMMUNE;

    // disable effects to which unit is immune
    SpellMissInfo returnVal = SPELL_MISS_IMMUNE;
    for (uint32 effectNumber = 0; effectNumber < MAX_SPELL_EFFECTS; ++effectNumber)
        if (effectMask & (1 << effectNumber))
            if (unit->IsImmunedToSpellEffect(m_spellInfo, effectNumber))
                effectMask &= ~(1 << effectNumber);

    switch (m_spellInfo->Id)
    {
        case 12051:     // Evocation
        {
            // Glyph of Evocation, Invocation
            if (!m_caster->HasAura(56380) || m_caster->HasSpell(114003))
                effectMask &= ~(1 << EFFECT_1);
            break;
        }
    }

    if (!effectMask)
        return returnVal;

    PrepareScriptHitHandlers();
    CallScriptBeforeHitHandlers();

    LinkedSpell(unit, unit, SPELL_LINK_BEFORE_HIT);

    if (unit->GetTypeId() == TYPEID_PLAYER)
    {
        unit->ToPlayer()->GetAchievementMgr().StartTimedAchievement(ACHIEVEMENT_TIMED_TYPE_SPELL_TARGET, m_spellInfo->Id);
        unit->ToPlayer()->GetAchievementMgr().StartTimedAchievement(ACHIEVEMENT_TIMED_TYPE_SPELL_TARGET2, m_spellInfo->Id);
        unit->ToPlayer()->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_BE_SPELL_TARGET, m_spellInfo->Id, 0, 0, m_caster);
        unit->ToPlayer()->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_BE_SPELL_TARGET2, m_spellInfo->Id);

        Map* map = unit->GetMap();
        // Update scenario/challenge criterias
        if (uint32 instanceId =  map ? map->GetInstanceId() : 0)
            if (ScenarioProgress* progress = sScenarioMgr->GetScenarioProgress(instanceId))
            {
                progress->GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_BE_SPELL_TARGET, m_spellInfo->Id, 0, 0, m_caster, unit->ToPlayer());
                progress->GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_BE_SPELL_TARGET, m_spellInfo->Id, 0, 0, NULL, unit->ToPlayer());
            }
    }

    if (m_caster->GetTypeId() == TYPEID_PLAYER)
    {
        m_caster->ToPlayer()->GetAchievementMgr().StartTimedAchievement(ACHIEVEMENT_TIMED_TYPE_SPELL_CASTER, m_spellInfo->Id);
        m_caster->ToPlayer()->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_CAST_SPELL2, m_spellInfo->Id, 0, 0, unit);
    }

    if (m_caster != unit)
    {
        // Recheck  UNIT_FLAG_NON_ATTACKABLE for delayed spells
        if (m_spellInfo->Speed > 0.0f && unit->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE) && unit->GetCharmerOrOwnerGUID() != m_caster->GetGUID())
            return SPELL_MISS_EVADE;

        if (m_caster->_IsValidAttackTarget(unit, m_spellInfo))
        {
            unit->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_HITBYSPELL);
        }
        else if (m_caster->IsFriendlyTo(unit))
        {
            bool positive = false;
            for (uint32 effIndex = 0; effIndex < MAX_SPELL_EFFECTS; ++effIndex)
                if (effectMask & (1 << effIndex))
                    positive = m_spellInfo->IsPositiveEffect(effIndex);
            // for delayed spells ignore negative spells (after duel end) for friendly targets
            // TODO: this cause soul transfer bugged
            if (m_spellInfo->Speed > 0.0f && unit->GetTypeId() == TYPEID_PLAYER && !positive)
                return SPELL_MISS_EVADE;

            // assisting case, healing and resurrection
            if (unit->HasUnitState(UNIT_STATE_ATTACK_PLAYER))
            {
                m_caster->SetContestedPvP();
                if (m_caster->GetTypeId() == TYPEID_PLAYER)
                    m_caster->ToPlayer()->UpdatePvP(true);
            }
            if (unit->isInCombat() && !(AttributesCustomEx3 & SPELL_ATTR3_NO_INITIAL_AGGRO) && !((AttributesCustomEx & SPELL_ATTR1_NOT_BREAK_STEALTH) && !m_damage))
            {
                m_caster->SetInCombatState(unit->GetCombatTimer() > 0, unit);
                unit->getHostileRefManager().threatAssist(m_caster, 0.0f);
            }
        }
    }

    // Get Data Needed for Diminishing Returns, some effects may have multiple auras, so this must be done on spell hit, not aura add
    m_diminishGroup = GetDiminishingReturnsGroupForSpell(m_spellInfo, m_triggeredByAuraSpell);
    if (m_diminishGroup)
    {
        m_diminishLevel = DIMINISHING_LEVEL_1;

        m_diminishLevel = unit->GetDiminishing(m_diminishGroup);
        DiminishingReturnsType type = GetDiminishingReturnsGroupType(m_diminishGroup);
        // Increase Diminishing on unit, current informations for actually casts will use values above
        if ((type == DRTYPE_PLAYER && unit->GetCharmerOrOwnerPlayerOrPlayerItself()) || type == DRTYPE_ALL)
            unit->IncrDiminishing(m_diminishGroup);
    }

    uint32 aura_effmask = 0;
    for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
        if (effectMask & (1 << i) && m_spellInfo->GetEffect(i, m_diffMode)->IsUnitOwnedAuraEffect())
            aura_effmask |= 1 << i;

    if (aura_effmask)
    {
        // Select rank for aura with level requirements only in specific cases
        // Unit has to be target only of aura effect, both caster and target have to be players, target has to be other than unit target
        SpellInfo const* aurSpellInfo = m_spellInfo;
        float basePoints[32];
        if (scaleAura)
        {
            aurSpellInfo = m_spellInfo->GetAuraRankForLevel(unitTarget->getLevel());
            ASSERT(aurSpellInfo);
            for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
            {
                basePoints[i] = aurSpellInfo->GetEffect(i, m_diffMode)->BasePoints;
                if (m_spellInfo->GetEffect(i, m_diffMode)->Effect != aurSpellInfo->GetEffect(i, m_diffMode)->Effect)
                {
                    aurSpellInfo = m_spellInfo;
                    break;
                }
            }
        }

        if (m_originalCaster)
        {
            bool refresh = false;
            m_spellAura = Aura::TryRefreshStackOrCreate(aurSpellInfo, effectMask, unit,
                m_originalCaster, (aurSpellInfo == m_spellInfo)? &m_spellValue->EffectBasePoints[0] : &basePoints[0], m_CastItem, 0, &refresh, 0, this);
            if (m_spellAura)
            {
                // Set aura stack amount to desired value
                if (m_spellValue->AuraStackAmount > 1)
                {
                    if (!refresh)
                        m_spellAura->SetStackAmount(m_spellValue->AuraStackAmount);
                    else
                        m_spellAura->ModStackAmount(m_spellValue->AuraStackAmount);
                }

                // Now Reduce spell duration using data received at spell hit
                int32 duration = m_spellAura->GetMaxDuration();
                int32 limitduration = GetDiminishingReturnsLimitDuration(m_diminishGroup, aurSpellInfo);
                float diminishMod = unit->ApplyDiminishingToDuration(m_diminishGroup, duration, m_originalCaster, m_diminishLevel, limitduration);

                // unit is immune to aura if it was diminished to 0 duration
                if (diminishMod == 0.0f)
                {
                    m_spellAura->Remove();
                    bool found = false;
                    for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
                        if (effectMask & (1 << i) && m_spellInfo->GetEffect(i, m_diffMode)->Effect != SPELL_EFFECT_APPLY_AURA)
                            found = true;
                    if (!found)
                        return SPELL_MISS_IMMUNE;
                }
                else
                {
                    ((UnitAura*)m_spellAura)->SetDiminishGroup(m_diminishGroup);

                    bool positive = m_spellAura->GetSpellInfo()->IsPositive();
                    if (AuraApplication* aurApp = m_spellAura->GetApplicationOfTarget(m_originalCaster->GetGUID()))
                        positive = aurApp->IsPositive();

                    duration = m_originalCaster->ModSpellDuration(aurSpellInfo, unit, duration, positive, effectMask);

                    if (duration > 0)
                    {
                        // Haste modifies duration of channeled spells
                        if (m_spellInfo->IsChanneled())
                        {
                            if (AttributesCustomEx5 & SPELL_ATTR5_HASTE_AFFECT_TICK_AND_CASTTIME)
                                m_originalCaster->ModSpellCastTime(aurSpellInfo, duration, this);
                            else if (AttributesCustomEx8 & SPELL_ATTR8_HASTE_AFFECT_DURATION)
                                duration = int32(duration * m_originalCaster->GetFloatValue(UNIT_MOD_CAST_HASTE));
                        }
                        else if (AttributesCustomEx5 & SPELL_ATTR5_HASTE_AFFECT_TICK_AND_CASTTIME)
                        {
                            int32 origDuration = duration;
                            for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
                                if (AuraEffect const* eff = m_spellAura->GetEffect(i))
                                    if (int32 amplitude = eff->GetAmplitude())  // amplitude is hastened by UNIT_MOD_CAST_SPEED
                                    {
                                        int32 gettotalticks = origDuration / amplitude;
                                        int32 rest = origDuration - (gettotalticks * amplitude);
                                        int32 timer = eff->GetPeriodicTimer() == amplitude ? 0: eff->GetPeriodicTimer();

                                        if (rest > (amplitude / 2))
                                            gettotalticks++;

                                        duration = gettotalticks * amplitude + timer;
                                    }
                        }
                    }

                    if (duration != m_spellAura->GetMaxDuration())
                    {
                        bool periodicDamage = false;
                        for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
                            if (m_spellAura->GetEffect(i))
                                if (m_spellAura->GetEffect(i)->GetAuraType() == SPELL_AURA_PERIODIC_DAMAGE)
                                    periodicDamage = true;

                        // Fix Pandemic
                        if (periodicDamage && refresh && m_originalCaster->HasAura(131973))
                        {
                            int32 newDuration = (duration + m_spellAura->GetDuration()) <= (int32(m_spellInfo->GetMaxDuration() * 1.5f)) ?
                                duration + m_spellAura->GetDuration() : int32(m_spellInfo->GetMaxDuration() * 1.5f);
                            int32 newMaxDuration = (duration + m_spellAura->GetMaxDuration()) <= (int32(m_spellInfo->GetMaxDuration() * 1.5f)) ?
                                duration + m_spellAura->GetMaxDuration() : int32(m_spellInfo->GetMaxDuration() * 1.5f);

                            m_spellAura->SetMaxDuration(newMaxDuration);
                            m_spellAura->SetDuration(newDuration);
                        }
                        else
                        {
                            m_spellAura->SetMaxDuration(duration);
                            m_spellAura->SetDuration(duration);
                        }
                    }

                    m_spellAura->_RegisterForTargets();

                    std::list<uint64> list_efftarget = GetEffectTargets();
                    if(!list_efftarget.empty())
                        m_spellAura->SetEffectTargets(list_efftarget);
                    if(m_triggeredByAura)
                        m_spellAura->SetTriggeredAuraEff(m_triggeredByAura);
                    if(uint64 dynObjGuid = GetSpellDynamicObject())
                        m_spellAura->SetSpellDynamicObject(dynObjGuid);
                    if (m_targets.HasDst())
                        AddDst(m_targets.GetDstPos());
                    if(!_positions.empty())
                        m_spellAura->SetDstVector(_positions);
                }
            }
        }
    }

    for (uint32 effectNumber = 0; effectNumber < MAX_SPELL_EFFECTS; ++effectNumber)
        if (effectMask & (1 << effectNumber))
            HandleEffects(unit, NULL, NULL, effectNumber, SPELL_EFFECT_HANDLE_HIT_TARGET);

    if (unit->IsMagnetSpellTarget())
        unit->SetMagnetSpell(false);

    return SPELL_MISS_NONE;
}

void Spell::DoTriggersOnSpellHit(Unit* unit, uint32 effMask)
{
    // Apply additional spell effects to target
    // TODO: move this code to scripts
    if (m_preCastSpell)
    {
        if (sSpellMgr->GetSpellInfo(m_preCastSpell))
            // Blizz seems to just apply aura without bothering to cast
            m_caster->AddAura(m_preCastSpell, unit);
    }

    // handle SPELL_AURA_ADD_TARGET_TRIGGER auras
    // this is executed after spell proc spells on target hit
    // spells are triggered for each hit spell target
    // info confirmed with retail sniffs of permafrost and shadow weaving

    if (!m_hitTriggerSpells.empty())
    {
        int _duration = 0;
        for (HitTriggerSpellList::const_iterator i = m_hitTriggerSpells.begin(); i != m_hitTriggerSpells.end(); ++i)
        {
            if (CanExecuteTriggersOnHit(effMask, i->triggeredByAura) && roll_chance_i(i->chance) && !m_spellInfo->IsPassive())
            {
                m_caster->CastSpell(unit, i->triggeredSpell, true);
                sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "Spell %d triggered spell %d by SPELL_AURA_ADD_TARGET_TRIGGER aura", m_spellInfo->Id, i->triggeredSpell->Id);

                // SPELL_AURA_ADD_TARGET_TRIGGER auras shouldn't trigger auras without duration
                // set duration of current aura to the triggered spell
                if (i->triggeredSpell->GetDuration() == -1)
                {
                    Aura* triggeredAur = unit->GetAura(i->triggeredSpell->Id, m_caster->GetGUID());
                    if (triggeredAur != NULL)
                    {
                        // get duration from aura-only once
                        if (!_duration)
                        {
                            Aura* aur = unit->GetAura(m_spellInfo->Id, m_caster->GetGUID());
                            _duration = aur ? aur->GetDuration() : -1;
                        }
                        triggeredAur->SetDuration(_duration);
                    }
                }
            }
        }
        m_hitTriggerSpells.clear();
    }

    // Predatory Swiftness
    if (Aura* aura = m_caster->GetAura(69369))
        if (aura->GetEffect(EFFECT_0)->IsAffectingSpell(GetSpellInfo()))
            aura->Remove();

    // trigger linked auras remove/apply
    // TODO: remove/cleanup this, as this table is not documented and people are doing stupid things with it
    LinkedSpell(unit, unit, SPELL_LINK_ON_HIT);
}

void Spell::DoAllEffectOnTarget(GOTargetInfo* target)
{
    if (target->processed)                                  // Check target
        return;
    target->processed = true;                               // Target checked in apply effects procedure

    uint32 effectMask = target->effectMask;
    if (!effectMask)
        return;

    GameObject* go = m_caster->GetMap()->GetGameObject(target->targetGUID);
    if (!go)
        return;

    PrepareScriptHitHandlers();
    CallScriptBeforeHitHandlers();

    for (uint32 effectNumber = 0; effectNumber < MAX_SPELL_EFFECTS; ++effectNumber)
        if (effectMask & (1 << effectNumber))
            HandleEffects(NULL, NULL, go, effectNumber, SPELL_EFFECT_HANDLE_HIT_TARGET);

    CallScriptOnHitHandlers();

    // cast at creature (or GO) quest objectives update at successful cast finished (+channel finished)
    // ignore autorepeat/melee casts for speed (not exist quest for spells (hm...)
    if (m_originalCaster && m_originalCaster->IsControlledByPlayer() && !IsAutoRepeat() && !IsNextMeleeSwingSpell() && !IsChannelActive())
        if (Player* p = m_originalCaster->GetCharmerOrOwnerPlayerOrPlayerItself())
            p->CastedCreatureOrGO(go->GetEntry(), go->GetGUID(), m_spellInfo->Id);
    CallScriptAfterHitHandlers();
}

void Spell::DoAllEffectOnTarget(ItemTargetInfo* target)
{
    uint32 effectMask = target->effectMask;
    if (!target->item || !effectMask)
        return;

    PrepareScriptHitHandlers();
    CallScriptBeforeHitHandlers();

    for (uint32 effectNumber = 0; effectNumber < MAX_SPELL_EFFECTS; ++effectNumber)
        if (effectMask & (1 << effectNumber))
            HandleEffects(NULL, target->item, NULL, effectNumber, SPELL_EFFECT_HANDLE_HIT_TARGET);

    CallScriptOnHitHandlers();

    CallScriptAfterHitHandlers();
}

bool Spell::UpdateChanneledTargetList()
{
    // Automatically forces player to face target
    if((m_spellInfo->AttributesEx & SPELL_ATTR1_CHANNEL_TRACK_TARGET) && !m_caster->HasInArc(static_cast<float>(M_PI), m_targets.GetUnitTarget()))
        m_caster->SetInFront(m_targets.GetUnitTarget());

    // Not need check return true
    if (m_channelTargetEffectMask == 0)
        return true;

    uint32 channelTargetEffectMask = m_channelTargetEffectMask;
    uint32 channelAuraMask = 0;
    for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
    {
        if (m_spellInfo->GetEffect(i, m_diffMode)->Effect == SPELL_EFFECT_APPLY_AURA)
            channelAuraMask |= 1<<i;
        if (m_spellInfo->GetEffect(i, m_diffMode)->Effect == SPELL_EFFECT_PERSISTENT_AREA_AURA)
            return true;
    }

    channelAuraMask &= channelTargetEffectMask;

    float range = 0;
    if (channelAuraMask)
    {
        range = m_spellInfo->GetMaxRange(m_spellInfo->IsPositive());
        if (Player* modOwner = m_caster->GetSpellModOwner())
            modOwner->ApplySpellMod(m_spellInfo->Id, SPELLMOD_RANGE, range, this);
    }

    for (std::list<TargetInfo>::iterator ihit= m_UniqueTargetInfo.begin(); ihit != m_UniqueTargetInfo.end(); ++ihit)
    {
        if (ihit->missCondition == SPELL_MISS_NONE && (channelTargetEffectMask & ihit->effectMask))
        {
            Unit* unit = m_caster->GetGUID() == ihit->targetGUID ? m_caster : ObjectAccessor::GetUnit(*m_caster, ihit->targetGUID);

            if (!unit)
                continue;

            if (IsValidDeadOrAliveTarget(unit))
            {
                if (channelAuraMask & ihit->effectMask)
                {
                    if (AuraApplication * aurApp = unit->GetAuraApplication(m_spellInfo->Id, m_originalCasterGUID))
                    {
                        if (m_caster != unit && !m_caster->IsWithinDistInMap(unit, range))
                        {
                            ihit->effectMask &= ~aurApp->GetEffectMask();
                            unit->RemoveAura(aurApp);
                            continue;
                        }
                    }
                    else // aura is dispelled
                        continue;
                }

                channelTargetEffectMask &= ~ihit->effectMask;   // remove from need alive mask effect that have alive target
            }
        }
    }

    // is all effects from m_needAliveTargetMask have alive targets
    return channelTargetEffectMask == 0;
}

void Spell::prepare(SpellCastTargets const* targets, AuraEffect const* triggeredByAura)
{
    if (m_CastItem)
        m_castItemGUID = m_CastItem->GetGUID();
    else
        m_castItemGUID = 0;

    if (m_CastItem && !m_itemEntry)
        m_itemEntry = m_CastItem->GetEntry();

    if (triggeredByAura)
    {
        m_triggeredByAuraSpell  = triggeredByAura->GetSpellInfo();
        m_triggeredByAura = triggeredByAura;
    }

    InitExplicitTargets(*targets);

    // Fill aura scaling information
    if (m_caster->IsControlledByPlayer() && !m_spellInfo->IsPassive() && m_spellInfo->SpellLevel && !m_spellInfo->IsChanneled() && !(_triggeredCastFlags & TRIGGERED_IGNORE_AURA_SCALING))
    {
        for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
        {
            if (m_spellInfo->GetEffect(i, m_diffMode)->Effect == SPELL_EFFECT_APPLY_AURA)
            {
                // Change aura with ranks only if basepoints are taken from spellInfo and aura is positive
                if (m_spellInfo->IsPositiveEffect(i))
                {
                    m_auraScaleMask |= (1 << i);
                    if (m_spellValue->EffectBasePoints[i] != m_spellInfo->GetEffect(i, m_diffMode)->BasePoints)
                    {
                        m_auraScaleMask = 0;
                        break;
                    }
                }
            }
        }
    }

    m_spellState = SPELL_STATE_PREPARING;

    // create and add update event for this spell
    SpellEvent* Event = new SpellEvent(this);
    m_caster->m_Events.AddEvent(Event, m_caster->m_Events.CalculateTime(1));

    //Prevent casting at cast another spell (ServerSide check)
    if (!(_triggeredCastFlags & TRIGGERED_IGNORE_CAST_IN_PROGRESS) && m_caster->IsNonMeleeSpellCasted(false, true, true) && m_cast_count)
    {
        SendCastResult(SPELL_FAILED_SPELL_IN_PROGRESS);
        finish(false);
        return;
    }

    if (DisableMgr::IsDisabledFor(DISABLE_TYPE_SPELL, m_spellInfo->Id, m_caster))
    {
        SendCastResult(SPELL_FAILED_SPELL_UNAVAILABLE);
        finish(false);
        return;
    }
    LoadScripts();

    if (m_caster->GetTypeId() == TYPEID_PLAYER)
        m_caster->ToPlayer()->SetSpellModTakingSpell(this, true);
    // Fill cost data (not use power for item casts
    m_powerCost = m_CastItem ? 0 : m_spellInfo->CalcPowerCost(m_caster, m_spellSchoolMask);
    if (m_caster->GetTypeId() == TYPEID_PLAYER)
        m_caster->ToPlayer()->SetSpellModTakingSpell(this, false);

    // Set combo point requirement
    if ((_triggeredCastFlags & TRIGGERED_IGNORE_COMBO_POINTS) || m_CastItem || !m_caster->m_movedPlayer)
        m_needComboPoints = false;

    LinkedSpell(m_caster, m_targets.GetUnitTarget(), SPELL_LINK_BEFORE_CHECK);

    SpellCastResult result = CheckCast(true);
    if (result != SPELL_CAST_OK && !IsAutoRepeat())          //always cast autorepeat dummy for triggering
    {
        //#ifdef WIN32
        //sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "Spell::prepare::checkcast fail. spell id %u res %u source %u caster %d customCastFlags %u mask %u", m_spellInfo->Id, result, m_caster->GetEntry(), m_originalCaster ? m_originalCaster->GetEntry() : -1, _triggeredCastFlags, m_targets.GetTargetMask());
        //#endif
        // Periodic auras should be interrupted when aura triggers a spell which can't be cast
        // for example bladestorm aura should be removed on disarm as of patch 3.3.5
        // channeled periodic spells should be affected by this (arcane missiles, penance, etc)
        // a possible alternative sollution for those would be validating aura target on unit state change
        if (triggeredByAura && triggeredByAura->IsPeriodic() && !triggeredByAura->GetBase()->IsPassive())
        {
            SendChannelUpdate(0);
            triggeredByAura->GetBase()->SetDuration(0);
        }

        SendCastResult(result);

        finish(false);
        return;
    }

    // Prepare data for triggers
    prepareDataForTriggerSystem(triggeredByAura);

    if (m_caster->GetTypeId() == TYPEID_PLAYER)
        m_caster->ToPlayer()->SetSpellModTakingSpell(this, true);
    // calculate cast time (calculated after first CheckCast check to prevent charge counting for first CheckCast fail)
    m_casttime = m_spellInfo->CalcCastTime(m_caster, this);
    if (m_caster->GetTypeId() == TYPEID_PLAYER)
    { 
        m_caster->ToPlayer()->SetSpellModTakingSpell(this, false);

        // Set cast time to 0 if .cheat cast time is enabled.
        if (m_caster->ToPlayer()->GetCommandStatus(CHEAT_CASTTIME))
             m_casttime = 0;
    }

    // don't allow channeled spells / spells with cast time to be casted while moving
    // (even if they are interrupted on moving, spells with almost immediate effect get to have their effect processed before movement interrupter kicks in)
    // don't cancel spells which are affected by a SPELL_AURA_CAST_WHILE_WALKING effect
    if (((m_spellInfo->IsChanneled() || m_casttime) && m_caster->GetTypeId() == TYPEID_PLAYER && m_caster->isMoving() && 
        m_spellInfo->InterruptFlags & SPELL_INTERRUPT_FLAG_MOVEMENT) && !m_caster->HasAuraCastWhileWalking(m_spellInfo) && !(_triggeredCastFlags & TRIGGERED_IGNORE_AURA_INTERRUPT_FLAGS))
    {
        //sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "Spell::prepare::checkcast fail. spell id %u res %u source %u customCastFlags %u mask %u, InterruptFlags %i", m_spellInfo->Id, SPELL_FAILED_MOVING, m_caster->GetEntry(), _triggeredCastFlags, m_targets.GetTargetMask(), m_spellInfo->InterruptFlags);
        SendCastResult(SPELL_FAILED_MOVING);
        finish(false);
        return;
    }

    // set timer base at cast time
    ReSetTimer();

    sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "Spell::prepare: spell id %u source %u caster %d customCastFlags %u mask %u target %s", m_spellInfo->Id, m_caster->GetEntry(), m_originalCaster ? m_originalCaster->GetEntry() : -1, _triggeredCastFlags, m_targets.GetTargetMask(), m_targets.GetUnitTarget() ? m_targets.GetUnitTarget()->GetString().c_str() : "-");

    LinkedSpell(m_caster, m_targets.GetUnitTarget(), SPELL_LINK_PREPARE_CAST);

    //Containers for channeled spells have to be set
    //TODO:Apply this to all casted spells if needed
    // Why check duration? 29350: channeled triggers channeled
    if ((_triggeredCastFlags & TRIGGERED_CAST_DIRECTLY) && !(m_spellInfo->AttributesCu & SPELL_ATTR0_CU_HAVE_STABLE_FLYTIME) && (!m_spellInfo->IsChanneled() || !m_spellInfo->GetMaxDuration()))
        cast(true);
    else
    {
        // stealth must be removed at cast starting (at show channel bar)
        // skip triggered spell (item equip spell casting and other not explicit character casts/item uses)
        if (!(_triggeredCastFlags & TRIGGERED_IGNORE_AURA_INTERRUPT_FLAGS) && m_spellInfo->IsBreakingStealth() && (!m_caster->HasAuraType(SPELL_AURA_MOD_CAMOUFLAGE) || m_spellInfo->IsBreakingCamouflage()))
        {
            m_caster->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_CAST);
            for (uint32 i = 0; i < MAX_SPELL_EFFECTS; ++i)
                if (m_spellInfo->GetEffect(i, m_diffMode)->GetUsedTargetObjectType() == TARGET_OBJECT_TYPE_UNIT)
                {
                    m_caster->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_SPELL_ATTACK);
                    break;
                }
        }

        CallScriptBeforeStartCastHandlers();

        m_caster->SetCurrentCastedSpell(this);
        SendSpellPendingCast(); //Send activation spell
        SendSpellStart();

        // set target for proper facing
        if ((m_casttime || m_spellInfo->IsChanneled()) && !(_triggeredCastFlags & TRIGGERED_IGNORE_SET_FACING))
            if (m_targets.GetObjectTargetGUID() && m_caster->GetGUID() != m_targets.GetObjectTargetGUID() && m_caster->GetTypeId() == TYPEID_UNIT)
                m_caster->FocusTarget(this, m_targets.GetObjectTargetGUID());

        if (!(_triggeredCastFlags & TRIGGERED_IGNORE_GCD) && result == SPELL_CAST_OK)
            TriggerGlobalCooldown();

        //item: first cast may destroy item and second cast causes crash
        if (!m_casttime && !m_spellInfo->StartRecoveryTime && !m_castItemGUID && GetCurrentContainer() == CURRENT_GENERIC_SPELL && !(m_spellInfo->AttributesCu & SPELL_ATTR0_CU_HAVE_STABLE_FLYTIME))
            cast(true);
    }
}

void Spell::cancel()
{
    if (m_spellState == SPELL_STATE_FINISHED)
        return;

    uint32 oldState = m_spellState;
    m_spellState = SPELL_STATE_FINISHED;

    m_autoRepeat = false;
    switch (oldState)
    {
        case SPELL_STATE_PREPARING:
            CancelGlobalCooldown();
            if (m_caster->GetTypeId() == TYPEID_PLAYER)
                m_caster->ToPlayer()->RestoreSpellMods(this);
        case SPELL_STATE_DELAYED:
            SendInterrupted(0);
            SendCastResult(SPELL_FAILED_INTERRUPTED);
            break;

        case SPELL_STATE_CASTING:
            if (!m_UniqueTargetInfo.empty())
                for (std::list<TargetInfo>::const_iterator ihit = m_UniqueTargetInfo.begin(); ihit != m_UniqueTargetInfo.end(); ++ihit)
                    if ((*ihit).missCondition == SPELL_MISS_NONE)
                        if (Unit* unit = m_caster->GetGUID() == ihit->targetGUID ? m_caster : ObjectAccessor::GetUnit(*m_caster, ihit->targetGUID))
                            unit->RemoveOwnedAura(m_spellInfo->Id, m_originalCasterGUID, 0, AURA_REMOVE_BY_CANCEL);

            SendChannelUpdate(0);
            SendInterrupted(0);
            SendCastResult(SPELL_FAILED_INTERRUPTED);

            // spell is canceled-take mods and clear list
            if (m_caster->GetTypeId() == TYPEID_PLAYER)
                m_caster->ToPlayer()->RemoveSpellMods(this);

            m_appliedMods.clear();
            break;

        default:
            break;
    }

    SetReferencedFromCurrent(false);
    if (m_selfContainer && *m_selfContainer == this)
        *m_selfContainer = NULL;

    m_caster->RemoveDynObject(m_spellInfo->Id);
    if (m_spellInfo->IsChanneled()) // if not channeled then the object for the current cast wasn't summoned yet
        m_caster->RemoveGameObject(m_spellInfo->Id, true);

    //set state back so finish will be processed
    m_spellState = oldState;

    finish(false);
}

void Spell::cast(bool skipCheck)
{
    if (!m_caster->CheckAndIncreaseCastCounter())
    {
        if (m_triggeredByAuraSpell)
            sLog->outError(LOG_FILTER_GENERAL, "Spell %u triggered by aura spell %u too deep in cast chain for cast. Cast not allowed for prevent overflow stack crash.", m_spellInfo->Id);
        else
            sLog->outError(LOG_FILTER_GENERAL, "Spell %u too deep in cast chain for cast. Cast not allowed for prevent overflow stack crash.", m_spellInfo->Id);

        SendCastResult(SPELL_FAILED_ERROR);
        finish(false);
        return;
    }

    volatile uint32 spellid = m_spellInfo->Id;

    if (spellid == 144229) //SoP fix exploit(double click)
    {
        if (m_targets.GetObjectTarget()->ToGameObject())
        {
            if (m_targets.GetObjectTarget()->ToGameObject()->HasFlag(GAMEOBJECT_FLAGS, GO_FLAG_NOT_SELECTABLE))
            {
                SendInterrupted(0);
                SendCastResult(SPELL_FAILED_INTERRUPTED);
                finish(false);
                return;
            }
            else
                m_targets.GetObjectTarget()->ToGameObject()->SetFlag(GAMEOBJECT_FLAGS, GO_FLAG_NOT_SELECTABLE);
        }
    }

    // update pointers base at GUIDs to prevent access to non-existed already object
    UpdatePointers();

    // cancel at lost explicit target during cast
    if (m_targets.GetObjectTargetGUID() && !m_targets.GetObjectTarget())
    {
        cancel();
        m_caster->DecreaseCastCounter();
        return;
    }

    if (Player* playerCaster = m_caster->ToPlayer())
    {
        // now that we've done the basic check, now run the scripts
        // should be done before the spell is actually executed
        sScriptMgr->OnPlayerSpellCast(playerCaster, this, skipCheck);

        // As of 3.0.2 pets begin attacking their owner's target immediately
        // Let any pets know we've attacked something. Check DmgClass for harmful spells only
        // This prevents spells such as Hunter's Mark from triggering pet attack
        if (this->GetSpellInfo()->DmgClass != SPELL_DAMAGE_CLASS_NONE && !m_spellInfo->HasAura(SPELL_AURA_MOD_CONFUSE))
            if (Pet* playerPet = playerCaster->GetPet())
                if (playerPet->isAlive() && playerPet->isControlled() && (m_targets.GetTargetMask() & TARGET_FLAG_UNIT))
                    playerPet->AI()->OwnerAttacked(m_targets.GetObjectTarget()->ToUnit());
    }
    SetExecutedCurrently(true);

    if (m_caster->GetTypeId() != TYPEID_PLAYER && m_targets.GetUnitTarget() && m_targets.GetUnitTarget() != m_caster)
        m_caster->SetInFront(m_targets.GetUnitTarget());

    // Should this be done for original caster?
    if (m_caster->GetTypeId() == TYPEID_PLAYER)
    {
        // Set spell which will drop charges for triggered cast spells
        // if not successfully casted, will be remove in finish(false)
        m_caster->ToPlayer()->SetSpellModTakingSpell(this, true);
    }

    CallScriptBeforeCastHandlers();
    LinkedSpell(m_caster, m_targets.GetUnitTarget(), SPELL_LINK_BEFORE_CAST);

    // skip check if done already (for instant cast spells for example)
    if (!skipCheck)
    {
        SpellCastResult castResult = CheckCast(false);
        if (castResult != SPELL_CAST_OK)
        {
            #ifdef WIN32
            sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "Spell::cast::checkcast fail. spell id %u res %u source %u caster %d customCastFlags %u mask %u", m_spellInfo->Id, castResult, m_caster->GetEntry(), m_originalCaster ? m_originalCaster->GetEntry() : -1, _triggeredCastFlags, m_targets.GetTargetMask());
            #endif
            SendCastResult(castResult);
            SendInterrupted(0);
            //restore spell mods
            if (m_caster->GetTypeId() == TYPEID_PLAYER)
            {
                m_caster->ToPlayer()->RestoreSpellMods(this);
                // cleanup after mod system
                // triggered spell pointer can be not removed in some cases
                m_caster->ToPlayer()->SetSpellModTakingSpell(this, false);
            }
            finish(false);
            SetExecutedCurrently(false);
            m_caster->DecreaseCastCounter();
            return;
        }

        // additional check after cast bar completes (must not be in CheckCast)
        // if trade not complete then remember it in trade data
        if (m_targets.GetTargetMask() & TARGET_FLAG_TRADE_ITEM)
        {
            if (m_caster->GetTypeId() == TYPEID_PLAYER)
            {
                if (TradeData* my_trade = m_caster->ToPlayer()->GetTradeData())
                {
                    if (!my_trade->IsInAcceptProcess())
                    {
                        // Spell will be casted at completing the trade. Silently ignore at this place
                        my_trade->SetSpell(m_spellInfo->Id, m_CastItem);
                        SendCastResult(SPELL_FAILED_DONT_REPORT);
                        SendInterrupted(0);
                        m_caster->ToPlayer()->RestoreSpellMods(this);
                        // cleanup after mod system
                        // triggered spell pointer can be not removed in some cases
                        m_caster->ToPlayer()->SetSpellModTakingSpell(this, false);
                        finish(false);
                        SetExecutedCurrently(false);
                        m_caster->DecreaseCastCounter();
                        return;
                    }
                }
            }
        }
    }

    SelectSpellTargets();

    // Spell may be finished after target map check
    if (m_spellState == SPELL_STATE_FINISHED)
    {
        SendInterrupted(0);
        //restore spell mods
        if (m_caster->GetTypeId() == TYPEID_PLAYER)
        {
            m_caster->ToPlayer()->RestoreSpellMods(this);
            // cleanup after mod system
            // triggered spell pointer can be not removed in some cases
            m_caster->ToPlayer()->SetSpellModTakingSpell(this, false);
        }
        finish(false);
        SetExecutedCurrently(false);
        m_caster->DecreaseCastCounter();
        return;
    }

    PrepareTriggersExecutedOnHit();

    CallScriptOnCastHandlers();

    // traded items have trade slot instead of guid in m_itemTargetGUID
    // set to real guid to be sent later to the client
    m_targets.UpdateTradeSlotItem();

    if (Player* plrCaster = m_caster->ToPlayer())
    {
        if (!(_triggeredCastFlags & TRIGGERED_IGNORE_CAST_ITEM) && m_CastItem)
        {
            //plrCaster->GetAchievementMgr().StartTimedAchievement(ACHIEVEMENT_TIMED_TYPE_ITEM, m_CastItem->GetEntry());
            plrCaster->GetAchievementMgr().StartTimedAchievement(ACHIEVEMENT_TIMED_TYPE_ITEM2, m_CastItem->GetEntry());
            plrCaster->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_USE_ITEM, m_CastItem->GetEntry());
        }

        plrCaster->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_CAST_SPELL, m_spellInfo->Id);

        plrCaster->TakeSpellCharge(m_spellInfo);
    }

    // CAST SPELL
    SendSpellCooldown();

    PrepareScriptHitHandlers();

    HandleLaunchPhase();

    // Powers have to be taken before SendSpellGo
    if (!(_triggeredCastFlags & TRIGGERED_IGNORE_POWER_AND_REAGENT_COST))
        TakePower();

    m_caster->SendSpellCreateVisual(m_spellInfo, &visualPos, m_targets.GetUnitTarget());
    // we must send smsg_spell_go packet before m_castItem delete in TakeCastItem()...
    SendSpellGo();

    //test fix for take some charges from aura mods
    if (m_caster->GetTypeId() == TYPEID_PLAYER)
        m_caster->ToPlayer()->RemoveSpellMods(this, true);

    bool hasDeley = true;

    // Okay, everything is prepared. Now we need to distinguish between immediate and evented delayed spells
    if (((m_spellInfo->Speed > 0.0f || m_delayMoment) && !m_spellInfo->IsChanneled() && !m_spellInfo->IsNonNeedDelay() && m_spellInfo->Id != 114157)  || m_spellInfo->AttributesEx4 & SPELL_ATTR4_UNK4 || m_spellInfo->Id == 54957)
    {
        // Remove used for cast item if need (it can be already NULL after TakeReagents call
        // in case delayed spell remove item at cast delay start
        TakeCastItem();

        // Okay, maps created, now prepare flags
        m_immediateHandled = false;
        m_spellState = SPELL_STATE_DELAYED;
        SetDelayStart(0);

        if (m_caster->HasUnitState(UNIT_STATE_CASTING) && !m_caster->IsNonMeleeSpellCasted(false, false, true))
            m_caster->ClearUnitState(UNIT_STATE_CASTING);
    }
    else
        hasDeley = false;

    Unit* procTarget = m_targets.GetUnitTarget() ? m_targets.GetUnitTarget() : m_caster;
    uint32 procAttacker = PROC_EX_NONE;
    uint32 procVictim   = PROC_EX_NONE;
    TargetInfo* infoTarget = GetTargetInfo(procTarget ? procTarget->GetGUID() : 0);
    //sLog->outDebug(LOG_FILTER_PROC, "Spell::cast Id %i, m_UniqueTargetInfo %i, procAttacker %i, target %u, infoTarget %u",
    //m_spellInfo->Id, m_UniqueTargetInfo.size(), procAttacker, procTarget ? procTarget->GetGUID() : 0, infoTarget ? infoTarget->targetGUID : 0);

    if (!procAttacker)
    {
        Unit* caster = m_originalCaster ? m_originalCaster : m_caster;
        uint32 mask = 7;
        bool canEffectTrigger = m_spellInfo->CanSpellProc(procTarget, mask, m_CastItem);
        //sLog->outDebug(LOG_FILTER_PROC, "Spell::cast Id %i, mask %i, canEffectTrigger %i", m_spellInfo->Id, mask, canEffectTrigger);

        if(canEffectTrigger)
        {
            int32 procDamage = m_damage;
            uint32 procEx = m_procEx;

            if(infoTarget)
            {
                procDamage = infoTarget->crit ? infoTarget->damage * 2 : infoTarget->damage;
                if (infoTarget->crit)
                    procEx |= PROC_EX_CRITICAL_HIT;
                else
                    procEx |= PROC_EX_NORMAL_HIT;
            }
            else
                procEx |= PROC_EX_NORMAL_HIT;

            bool positive = true;
            bool dmgSpell = procDamage;
            if (procDamage > 0)
                positive = false;
            else if (procDamage < 0)
                procDamage = -procDamage;
            else if (!procDamage)
            {
                for (uint8 i = 0; i< MAX_SPELL_EFFECTS; ++i)
                    // If at least one effect negative spell is negative hit
                    if (mask & (1 << i))
                    {
                        if (!m_spellInfo->IsPositiveEffect(i))
                        {
                            if (m_spellInfo->CanHaveDamageNegOnEff(i))
                                dmgSpell = true;
                            positive = false;
                        }
                        else if (!dmgSpell)
                            dmgSpell = m_spellInfo->Effects[i].ApplyAuraName == SPELL_AURA_SCHOOL_ABSORB;
                        break;
                    }
            }

            // Hunter trap spells - activation proc for Lock and Load, Entrapment and Misdirection
            if (m_spellInfo->SpellFamilyName == SPELLFAMILY_HUNTER &&
                (m_spellInfo->SpellFamilyFlags[0] & 0x18 ||     // Freezing and Frost Trap, Freezing Arrow
                m_spellInfo->Id == 57879 || m_spellInfo->Id == 13810 ||                     // Snake Trap - done this way to avoid double proc
                m_spellInfo->SpellFamilyFlags[2] & 0x00024000)) // Explosive and Immolation Trap
                procAttacker |= PROC_FLAG_DONE_TRAP_ACTIVATION;

            switch (m_spellInfo->DmgClass)
            {
                case SPELL_DAMAGE_CLASS_MELEE:
                {
                    procAttacker |= PROC_FLAG_DONE_SPELL_DMG_POS_NEG;
                    procAttacker |= dmgSpell ? PROC_FLAG_DONE_SPELL_MELEE_DMG_CLASS : PROC_FLAG_DONE_SPELL_NONE_DMG_CLASS_NEG;

                    if (m_attackType == BASE_ATTACK)
                        procAttacker |= PROC_FLAG_DONE_MAINHAND_ATTACK;
                    else
                        procAttacker |= PROC_FLAG_DONE_OFFHAND_ATTACK;
                    break;
                }
                case SPELL_DAMAGE_CLASS_MAGIC:
                {
                    procAttacker |= PROC_FLAG_DONE_SPELL_DMG_POS_NEG;
                    if (!positive)
                        procAttacker |= dmgSpell ? PROC_FLAG_DONE_SPELL_MAGIC_DMG_CLASS_NEG : PROC_FLAG_DONE_SPELL_NONE_DMG_CLASS_NEG;
                    else
                        procAttacker |= dmgSpell ? PROC_FLAG_DONE_SPELL_MAGIC_DMG_CLASS_POS : PROC_FLAG_DONE_SPELL_NONE_DMG_CLASS_POS;
                    break;
                }
                case SPELL_DAMAGE_CLASS_NONE:
                {
                    if (m_spellInfo->GetSchoolMask() & SPELL_SCHOOL_MASK_MAGIC)
                    {
                        if (!positive)
                            procAttacker |= dmgSpell ? PROC_FLAG_DONE_SPELL_MAGIC_DMG_CLASS_NEG : PROC_FLAG_DONE_SPELL_NONE_DMG_CLASS_NEG;
                        else
                            procAttacker |= dmgSpell ? PROC_FLAG_DONE_SPELL_MAGIC_DMG_CLASS_POS : PROC_FLAG_DONE_SPELL_NONE_DMG_CLASS_POS;
                    }
                    else
                    {
                        if (positive)
                            procAttacker |= PROC_FLAG_DONE_SPELL_NONE_DMG_CLASS_POS;
                        else
                            procAttacker |= PROC_FLAG_DONE_SPELL_NONE_DMG_CLASS_NEG;
                    }
                    break;
                }
                case SPELL_DAMAGE_CLASS_RANGED:
                {
                    // Auto attack
                    if (!(AttributesCustomEx2 & SPELL_ATTR2_AUTOREPEAT_FLAG))
                        procAttacker |= dmgSpell ? PROC_FLAG_DONE_SPELL_RANGED_DMG_CLASS : PROC_FLAG_DONE_SPELL_NONE_DMG_CLASS_NEG;
                    break;
                }
            }

            if (!positive && procTarget->GetGUID() != m_caster->GetGUID() && m_spellInfo->StartRecoveryCategory)
                m_caster->SetLastCastTargetGUID(procTarget->GetGUID());

            //if (!(_triggeredCastFlags & TRIGGERED_DISALLOW_PROC_EVENTS))
                procEx |= PROC_EX_ON_CAST;

            sLog->outDebug(LOG_FILTER_PROC, "Cast m_spellInfo->Id %i, m_damage %i, procDamage %i, procEx %i", m_spellInfo->Id, m_damage, procDamage, procEx);

            if(procAttacker)
            {
                if(infoTarget)
                {
                    SpellNonMeleeDamage damageInfo(caster, procTarget, m_spellInfo->Id, m_spellSchoolMask);
                    procEx |= createProcExtendMask(&damageInfo, infoTarget->missCondition);
                    DamageInfo dmgInfoProc = DamageInfo(caster, procTarget, procDamage, m_spellInfo, SpellSchoolMask(m_spellInfo->SchoolMask), SPELL_DIRECT_DAMAGE, procDamage);
                    caster->ProcDamageAndSpell(procTarget, procAttacker, procVictim, procEx, &dmgInfoProc, m_attackType, m_spellInfo, m_triggeredByAuraSpell, GetSpellMods());
                }
                else
                {
                    DamageInfo dmgInfoProc = DamageInfo(caster, procTarget, procDamage, m_spellInfo, SpellSchoolMask(m_spellInfo->SchoolMask), SPELL_DIRECT_DAMAGE, procDamage);
                    caster->ProcDamageAndSpell(procTarget, procAttacker, procVictim, procEx, &dmgInfoProc, m_attackType, m_spellInfo, m_triggeredByAuraSpell, GetSpellMods());
                }
            }
        }
    }
    if (!hasDeley) // Immediate spell, no big deal
        handle_immediate();

    CallScriptAfterCastHandlers();

    if (!(_triggeredCastFlags & TRIGGERED_IGNORE_POWER_AND_REAGENT_COST))
        TakeReagents();
    else if (Item* targetItem = m_targets.GetItemTarget())
    {
        /// Not own traded item (in trader trade slot) req. reagents including triggered spell case
        if (targetItem->GetOwnerGUID() != m_caster->GetGUID())
            TakeReagents();
    }

    LinkedSpell(m_caster, m_targets.GetUnitTarget());

    if (Player* plr = m_caster->ToPlayer())
    { 
        plr->SetSpellModTakingSpell(this, false);
        //Clear spell cooldowns after every spell is cast if .cheat cooldown is enabled.
        if (plr->GetCommandStatus(CHEAT_COOLDOWN))
            plr->RemoveSpellCooldown(m_spellInfo->Id, true);
    }

    SetExecutedCurrently(false);
    m_caster->DecreaseCastCounter();
}

void Spell::handle_immediate()
{
    PrepareTargetProcessing();

    // process immediate effects (items, ground, etc.) also initialize some variables
    _handle_immediate_phase();

    for (std::list<TargetInfo>::iterator ihit= m_UniqueTargetInfo.begin(); ihit != m_UniqueTargetInfo.end(); ++ihit)
        DoAllEffectOnTarget(&(*ihit));

    for (std::list<GOTargetInfo>::iterator ihit= m_UniqueGOTargetInfo.begin(); ihit != m_UniqueGOTargetInfo.end(); ++ihit)
        DoAllEffectOnTarget(&(*ihit));

    FinishTargetProcessing();

    // spell is finished, perform some last features of the spell here
    _handle_finish_phase();

    // Remove used for cast item if need (it can be already NULL after TakeReagents call
    TakeCastItem();

    // handle ammo consumption for thrown weapons
    if (m_spellInfo->IsRangedWeaponSpell() && m_spellInfo->IsChanneled())
        TakeAmmo();

    if (m_spellState != SPELL_STATE_CASTING)
        finish(true);                                       // successfully finish spell cast (not last in case autorepeat or channel spell)
}

uint64 Spell::handle_delayed(uint64 t_offset)
{
    UpdatePointers();

    if (m_caster->GetTypeId() == TYPEID_PLAYER)
        m_caster->ToPlayer()->SetSpellModTakingSpell(this, true);

    uint64 next_time = 0;

    PrepareTargetProcessing();

    if (!m_immediateHandled)
    {
        _handle_immediate_phase();
        m_immediateHandled = true;
    }

    bool single_missile = (m_targets.HasDst());

    // now recheck units targeting correctness (need before any effects apply to prevent adding immunity at first effect not allow apply second spell effect and similar cases)
    for (std::list<TargetInfo>::iterator ihit= m_UniqueTargetInfo.begin(); ihit != m_UniqueTargetInfo.end(); ++ihit)
    {
        if (ihit->processed == false)
        {
            if (single_missile || ihit->timeDelay <= t_offset)
            {
                ihit->timeDelay = t_offset;
                DoAllEffectOnTarget(&(*ihit));
            }
            else if (next_time == 0 || ihit->timeDelay < next_time)
                next_time = ihit->timeDelay;
        }
    }

    // now recheck gameobject targeting correctness
    for (std::list<GOTargetInfo>::iterator ighit= m_UniqueGOTargetInfo.begin(); ighit != m_UniqueGOTargetInfo.end(); ++ighit)
    {
        if (ighit->processed == false)
        {
            if (single_missile || ighit->timeDelay <= t_offset)
                DoAllEffectOnTarget(&(*ighit));
            else if (next_time == 0 || ighit->timeDelay < next_time)
                next_time = ighit->timeDelay;
        }
    }

    FinishTargetProcessing();

    if (m_caster->GetTypeId() == TYPEID_PLAYER)
        m_caster->ToPlayer()->SetSpellModTakingSpell(this, false);

    // All targets passed - need finish phase
    if (next_time == 0)
    {
        // spell is finished, perform some last features of the spell here
        _handle_finish_phase();

        finish(true);                                       // successfully finish spell cast

        // return zero, spell is finished now
        return 0;
    }
    else
    {
        // spell is unfinished, return next execution time
        return next_time;
    }
}

void Spell::_handle_immediate_phase()
{
    m_spellAura = NULL;
    // initialize Diminishing Returns Data
    m_diminishLevel = DIMINISHING_LEVEL_1;
    m_diminishGroup = DIMINISHING_NONE;

    // handle some immediate features of the spell here
    HandleThreatSpells();

    PrepareScriptHitHandlers();

    // handle effects with SPELL_EFFECT_HANDLE_HIT mode
    for (uint32 j = 0; j < MAX_SPELL_EFFECTS; ++j)
    {
        // don't do anything for empty effect
        if (!m_spellInfo->Effects[j].IsEffect())
            continue;

        // call effect handlers to handle destination hit
        HandleEffects(NULL, NULL, NULL, j, SPELL_EFFECT_HANDLE_HIT);
    }

    // start channeling if applicable
    if (m_spellInfo->IsChanneled())
    {
        int32 duration = m_spellInfo->GetDuration();
        if (duration)
        {
            // First mod_duration then haste - see Missile Barrage
            // Apply duration mod
            if (Player* modOwner = m_caster->GetSpellModOwner())
                modOwner->ApplySpellMod(m_spellInfo->Id, SPELLMOD_DURATION, duration);

            // Apply haste mods
            if (AttributesCustomEx5 & SPELL_ATTR5_HASTE_AFFECT_TICK_AND_CASTTIME)
                m_caster->ModSpellCastTime(m_spellInfo, duration, this);
            if (AttributesCustomEx8 & SPELL_ATTR8_HASTE_AFFECT_DURATION)
                m_caster->ModSpellCastTime(m_spellInfo, duration, this);

            if(m_spellInfo->Id == 115294)
            {
                if (Aura* manaTeaStacks = m_caster->GetAura(115867))
                    duration = 500 * manaTeaStacks->GetStackAmount();
            }

            m_spellState = SPELL_STATE_CASTING;
            m_caster->AddInterruptMask(m_spellInfo->ChannelInterruptFlags);
            SendChannelStart(duration);
        }
        else if (duration == -1)
        {
            m_spellState = SPELL_STATE_CASTING;
            m_caster->AddInterruptMask(m_spellInfo->ChannelInterruptFlags);
            SendChannelStart(duration);
        }
    }

    // process items
    for (std::list<ItemTargetInfo>::iterator ihit= m_UniqueItemInfo.begin(); ihit != m_UniqueItemInfo.end(); ++ihit)
        DoAllEffectOnTarget(&(*ihit));

    if (!m_originalCaster)
        return;
    // Handle procs on cast
    // TODO: finish new proc system:P

    uint32 procAttacker = m_procAttacker;
    if (!procAttacker)
    {
        // Hunter trap spells - activation proc for Lock and Load, Entrapment and Misdirection
        if (m_spellInfo->SpellFamilyName == SPELLFAMILY_HUNTER &&
            (m_spellInfo->SpellFamilyFlags[0] & 0x18 ||     // Freezing and Frost Trap, Freezing Arrow
            m_spellInfo->Id == 57879 || m_spellInfo->Id == 13810 ||                     // Snake Trap - done this way to avoid double proc
            m_spellInfo->SpellFamilyFlags[2] & 0x00024000)) // Explosive and Immolation Trap
            procAttacker |= PROC_FLAG_DONE_TRAP_ACTIVATION;

        //procAttacker |= PROC_FLAG_DONE_SPELL_MAGIC_DMG_CLASS_POS;
        procAttacker |= PROC_FLAG_DONE_SPELL_DMG_POS_NEG;
    }

    if (m_UniqueTargetInfo.empty())
    {
        if (m_targets.HasDst())
        {
            // Proc the spells that have DEST target
            DamageInfo dmgInfoProc = DamageInfo(m_caster, NULL, 0, m_spellInfo, m_spellInfo ? SpellSchoolMask(m_spellInfo->SchoolMask) : SPELL_SCHOOL_MASK_NORMAL, SPELL_DIRECT_DAMAGE, 0);
            m_originalCaster->ProcDamageAndSpell(NULL, procAttacker, 0, m_procEx | PROC_EX_NORMAL_HIT, &dmgInfoProc, BASE_ATTACK, m_spellInfo, m_triggeredByAuraSpell);
        }
    }
    else
    {
        if (m_spellInfo->StartRecoveryTime && m_spellInfo->DmgClass == SPELL_DAMAGE_CLASS_MAGIC)
        {
            std::vector<uint32> spellId;
            std::list<AuraType> auralist;

            auralist.push_back(SPELL_AURA_MOD_CASTING_SPEED_NOT_STACK);
            auralist.push_back(SPELL_AURA_HASTE_SPELLS);
            auralist.push_back(SPELL_AURA_MELEE_SLOW);

            for (std::list<AuraType>::iterator auratype = auralist.begin(); auratype != auralist.end(); ++auratype)
            {
                Unit::AuraEffectList const& mModCastingSpeedNotStack = m_caster->GetAuraEffectsByType(*auratype);
                for (Unit::AuraEffectList::const_iterator i = mModCastingSpeedNotStack.begin(); i != mModCastingSpeedNotStack.end(); ++i)
                    if (SpellInfo const* sinfo = (*i)->GetSpellInfo())
                        if (sinfo->ProcCharges && !(*i)->HasSpellClassMask())
                            spellId.push_back(sinfo->Id);
            }

            for (std::vector<uint32>::iterator itr = spellId.begin(); itr != spellId.end(); ++itr)
                m_caster->RemoveAura(*itr);
        }
    }
}

void Spell::_handle_finish_phase()
{
    if (m_caster->m_movedPlayer)
    {
        // Take for real after all targets are processed
        if (m_needComboPoints)
        {
            m_caster->m_movedPlayer->ClearComboPoints();

            // Anticipation
            if (Player* _player = m_caster->ToPlayer())
            {
                if (_player->HasAura(115189))
                {
                    float basepoints0 = _player->GetAura(115189) ? _player->GetAura(115189)->GetStackAmount() : 0;
                    _player->CastCustomSpell(m_caster->getVictim(), 115190, &basepoints0, NULL, NULL, true);

                    if (basepoints0)
                        _player->RemoveAura(115189);
                }
            }
            if(uint8 count = m_caster->m_movedPlayer->GetSaveComboPoints())
            {
                if (Unit* target = m_targets.GetUnitTarget())
                    m_caster->m_movedPlayer->AddComboPoints(target, count, this);
                m_caster->m_movedPlayer->SaveAddComboPoints(-count);
            }
        }

        // Real add combo points from effects
        if (m_comboPointGain)
            m_caster->m_movedPlayer->GainSpellComboPoints(m_comboPointGain);
    }

    if (m_caster->m_extraAttacks && GetSpellInfo()->HasEffect(SPELL_EFFECT_ADD_EXTRA_ATTACKS))
        m_caster->HandleProcExtraAttackFor(m_caster->getVictim());

    // TODO: trigger proc phase finish here
}

void Spell::SendSpellCooldown()
{
    if (m_caster->GetTypeId() != TYPEID_PLAYER)
        return;

    Player* _player = (Player*)m_caster;

    // mana/health/etc potions, disabled by client (until combat out as declarate)
    if (m_CastItem && (m_CastItem->IsPotion() || m_CastItem->GetEntry() == 77589))
    {
        // need in some way provided data for Spell::finish SendCooldownEvent
        _player->SetLastPotionId(m_CastItem->GetEntry());
        return;
    }

    // have infinity cooldown but set at aura apply                  // do not set cooldown for triggered spells (needed by reincarnation)
    if (AttributesCustom & (SPELL_ATTR0_DISABLED_WHILE_ACTIVE | SPELL_ATTR0_PASSIVE) || (_triggeredCastFlags & TRIGGERED_IGNORE_SPELL_AND_CATEGORY_CD))
        return;

    _player->AddSpellAndCategoryCooldowns(m_spellInfo, m_CastItem ? m_CastItem->GetEntry() : 0, this);
}

void Spell::update(uint32 difftime)
{
    // update pointers based at it's GUIDs
    UpdatePointers();

    if (m_targets.GetUnitTargetGUID() && !m_targets.GetUnitTarget())
    {
        sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "Spell %u is cancelled due to removal of target.", m_spellInfo->Id);
        cancel();
        return;
    }

    // check if the player caster has moved before the spell finished
    // with the exception of spells affected with SPELL_AURA_CAST_WHILE_WALKING effect
    if ((m_caster->GetTypeId() == TYPEID_PLAYER && m_timer != 0) &&
        m_caster->isMoving() && (m_spellInfo->InterruptFlags & SPELL_INTERRUPT_FLAG_MOVEMENT) &&
        (m_spellInfo->Effects[0].Effect != SPELL_EFFECT_STUCK || !m_caster->HasUnitMovementFlag(MOVEMENTFLAG_FALLING_FAR)) &&
        !m_caster->HasAuraCastWhileWalking(m_spellInfo))
    {
        // don't cancel for melee, autorepeat, triggered and instant spells
        if (!IsNextMeleeSwingSpell() && !IsAutoRepeat() && !IsTriggered())
            cancel();
    }

    switch (m_spellState)
    {
        case SPELL_STATE_PREPARING:
        {
            if (m_timer > 0)
            {
                if (difftime >= (uint32)m_timer)
                    m_timer = 0;
                else
                    m_timer -= difftime;
            }

            if (m_timer == 0 && !IsNextMeleeSwingSpell() && !IsAutoRepeat())
                // don't CheckCast for instant spells - done in spell::prepare, skip duplicate checks, needed for range checks for example
                cast(!m_casttime);
            break;
        }
        case SPELL_STATE_CASTING:
        {
            if (m_timer)
            {
                // check if there are alive targets left
                if (!UpdateChanneledTargetList())
                {
                    sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "Channeled spell %d is removed due to lack of targets", m_spellInfo->Id);
                    SendChannelUpdate(0);
                    finish();
                }

                if (m_timer > 0)
                {
                    if (difftime >= (uint32)m_timer)
                        m_timer = 0;
                    else
                        m_timer -= difftime;
                }
            }

            if (m_timer == 0)
            {
                SendChannelUpdate(0);

                // channeled spell processed independently for quest targeting
                // cast at creature (or GO) quest objectives update at successful cast channel finished
                // ignore autorepeat/melee casts for speed (not exist quest for spells (hm...)
                if (!IsAutoRepeat() && !IsNextMeleeSwingSpell())
                {
                    if (Player* p = m_caster->GetCharmerOrOwnerPlayerOrPlayerItself())
                    {
                        for (std::list<TargetInfo>::iterator ihit = m_UniqueTargetInfo.begin(); ihit != m_UniqueTargetInfo.end(); ++ihit)
                        {
                            TargetInfo* target = &*ihit;
                            if (!IS_CRE_OR_VEH_GUID(target->targetGUID))
                                continue;

                            Unit* unit = m_caster->GetGUID() == target->targetGUID ? m_caster : ObjectAccessor::GetUnit(*m_caster, target->targetGUID);
                            if (unit == NULL)
                                continue;

                            p->CastedCreatureOrGO(unit->GetEntry(), unit->GetGUID(), m_spellInfo->Id);
                        }

                        for (std::list<GOTargetInfo>::iterator ihit = m_UniqueGOTargetInfo.begin(); ihit != m_UniqueGOTargetInfo.end(); ++ihit)
                        {
                            GOTargetInfo* target = &*ihit;

                            GameObject* go = m_caster->GetMap()->GetGameObject(target->targetGUID);
                            if (!go)
                                continue;

                            p->CastedCreatureOrGO(go->GetEntry(), go->GetGUID(), m_spellInfo->Id);
                        }
                    }
                }

                finish();
            }
            break;
        }
        default:
            break;
    }
}

void Spell::finish(bool ok)
{
    if (!m_caster)
        return;

    if (m_spellState == SPELL_STATE_FINISHED)
        return;
    m_spellState = SPELL_STATE_FINISHED;

    if (m_spellInfo->IsChanneled())
        m_caster->UpdateInterruptMask();

    if (m_caster->HasUnitState(UNIT_STATE_CASTING) && !m_caster->IsNonMeleeSpellCasted(false, false, true))
        m_caster->ClearUnitState(UNIT_STATE_CASTING | UNIT_STATE_MOVE_IN_CASTING);

    // Unsummon summon as possessed creatures on spell cancel
    if (m_spellInfo->IsChanneled() && m_caster->GetTypeId() == TYPEID_PLAYER)
    {
        if (Unit* charm = m_caster->GetCharm())
            if (charm->GetTypeId() == TYPEID_UNIT
                && charm->ToCreature()->HasUnitTypeMask(UNIT_MASK_PUPPET)
                && charm->GetUInt32Value(UNIT_CREATED_BY_SPELL) == m_spellInfo->Id)
                ((Puppet*)charm)->UnSummon();
    }

    if (m_caster->GetTypeId() == TYPEID_UNIT)
        m_caster->ReleaseFocus(this);

    if (!ok)
        return;

    if (m_caster->GetTypeId() == TYPEID_UNIT && m_caster->ToCreature()->isSummon())
    {
        // Unsummon statue
        uint32 spell = m_caster->GetUInt32Value(UNIT_CREATED_BY_SPELL);
        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spell);
        if (spellInfo && spellInfo->SpellIconID == 2056)
        {
            sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "Statue %d is unsummoned in spell %d finish", m_caster->GetGUIDLow(), m_spellInfo->Id);
            m_caster->setDeathState(JUST_DIED);
            return;
        }
    }

    if (IsAutoActionResetSpell())
    {
        bool found = false;
        Unit::AuraEffectList const& vIgnoreReset = m_caster->GetAuraEffectsByType(SPELL_AURA_IGNORE_MELEE_RESET);
        for (Unit::AuraEffectList::const_iterator i = vIgnoreReset.begin(); i != vIgnoreReset.end(); ++i)
        {
            if ((*i)->IsAffectingSpell(m_spellInfo))
            {
                found = true;
                break;
            }
        }
        if (!found && !(AttributesCustomEx2 & SPELL_ATTR2_NOT_RESET_AUTO_ACTIONS))
        {
            m_caster->resetAttackTimer(BASE_ATTACK);
            if (m_caster->haveOffhandWeapon())
                m_caster->resetAttackTimer(OFF_ATTACK);
            m_caster->resetAttackTimer(RANGED_ATTACK);
        }
    }

    // potions disabled by client, send event "not in combat" if need
    if (m_caster->GetTypeId() == TYPEID_PLAYER)
    {
        if (!m_triggeredByAuraSpell)
            m_caster->ToPlayer()->UpdatePotionCooldown(this);

        // triggered spell pointer can be not set in some cases
        // this is needed for proper apply of triggered spell mods
        m_caster->ToPlayer()->SetSpellModTakingSpell(this, true);

        // Take mods after trigger spell (needed for 14177 to affect 48664)
        // mods are taken only on succesfull cast and independantly from targets of the spell
        m_caster->ToPlayer()->RemoveSpellMods(this);
        m_caster->ToPlayer()->SetSpellModTakingSpell(this, false);
    }

    // Stop Attack for some spells
    if (AttributesCustom & SPELL_ATTR0_STOP_ATTACK_TARGET)
    {
        m_caster->AttackStop();
    }
    
    // Hack Shadowmeld
    if (m_spellInfo->Id == 58984)
    {
        m_caster->CombatStop();
    }
    
    if (m_castItemGUID && m_caster->GetTypeId() == TYPEID_PLAYER)
        if (Item* item = m_caster->ToPlayer()->GetItemByGuid(m_castItemGUID))
            if (item->IsEquipable() && !item->IsEquipped())
                m_caster->ToPlayer()->ApplyItemEquipSpell(item, false);

    sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "Spell::finish Spell: %u", m_spellInfo->Id);

    // Hack codes
    switch (m_spellInfo->Id)
    {
        case 53351: // Kill Shot
        {
            if (!unitTarget || !unitTarget->isAlive() || unitTarget->GetHealthPct() >= 20.0f || m_caster->HasAura(90967))
                break;

            m_caster->CastSpell(m_caster, 90967, true); // Effect cooldown marker
            m_caster->ToPlayer()->RemoveSpellCooldown(m_spellInfo->Id, true);

            break;
        }
        case 348:
        case 29722:
        case 6789:
        case 17877:
        case 77799:
        case 17962:
        case 109466:
        case 116858:
        {
            if (find_target)
            {
                // Havoc
                if (Aura* _aura = m_caster->GetAura(80240))
                {
                    // Allow only one Chaos Bolt to be duplicated ...
                    if (m_spellInfo->Id == 116858)
                        _aura->ModStackAmount(-3);
                    // ... or allow three next single target spells to be duplicated
                    else
                        _aura->ModStackAmount(-1);
                }
            }
            break;
        }
        case 49576: // Glyph of Resilient Grip
        {
            if (!unitTarget || !m_caster->ToPlayer() || !m_caster->HasAura(59309))
                break;

            for (std::list<TargetInfo>::iterator ihit = m_UniqueTargetInfo.begin(); ihit != m_UniqueTargetInfo.end(); ++ihit)
            {
                if (ihit->missCondition == SPELL_MISS_IMMUNE)
                    m_caster->CastSpell(m_caster, 90289, true);
            }
            break;
        }
        case 90289: // Glyph of Resilient Grip
        {
            if (Player* _player = m_caster->ToPlayer())
                _player->RemoveSpellCooldown(49576, true);
            break;
        }
        default:
            break;
    }

    if (m_caster->getClass() == CLASS_DEATH_KNIGHT && m_spellInfo->RuneCostID)
        if (Player* plr = m_caster->ToPlayer())
        {
            for (uint8 i = 0; i < MAX_RUNES; i++)
            {
                if (!plr->IsBlockedRuneConvert(i) && plr->IsLastRuneUsed(i))
                {
                    if (plr->GetCurrentRune(i) != plr->GetConvertIn(i))
                    {
                        plr->ConvertRune(i, plr->GetConvertIn(i));
                    }
                    else if (plr->GetCurrentRune(i) == RUNE_DEATH)
                    {
                        plr->SendDeathRuneUpdate(i);
                    }
                }
            }
        }

    LinkedSpell(m_caster, unitTarget, SPELL_LINK_FINISH_CAST);
}

void Spell::SendCastResult(SpellCastResult result)
{
    if (result == SPELL_CAST_OK)
        return;

    if (m_caster->GetTypeId() != TYPEID_PLAYER)
        return;

    if (m_caster->ToPlayer()->GetSession()->PlayerLoading())  // don't send cast results at loading time
        return;

    SendCastResult(m_caster->ToPlayer(), m_spellInfo, m_cast_count, result, m_customError);
}

void Spell::SendCastResult(Player* caster, SpellInfo const* spellInfo, uint8 cast_count, SpellCastResult result, SpellCustomErrors customError /*= SPELL_CUSTOM_ERROR_NONE*/)
{
    sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "SendCastResult  Spell: %u result %u.", spellInfo->Id, result);

    if (result == SPELL_CAST_OK)
        return;

    uint32 param1 = 0;
    uint32 param2 = 0;

    switch (result)
    {
        case SPELL_FAILED_NOT_READY:
        {
            param1 = 0;                                    // unknown (value 1 update cooldowns on client flag)                       
            break;
        }
        case SPELL_FAILED_REQUIRES_SPELL_FOCUS:
        {
            param1 = spellInfo->RequiresSpellFocus;        // SpellFocusObject.dbc id
            break;
        }
        case SPELL_FAILED_REQUIRES_AREA:                    // AreaTable.dbc id
        {
            // hardcode areas limitation case
            switch (spellInfo->Id)
            {
                case 41617:                                 // Cenarion Mana Salve
                case 41619:                                 // Cenarion Healing Salve
                    param1 = 3905;
                    break;
                case 41618:                                 // Bottled Nethergon Energy
                case 41620:                                 // Bottled Nethergon Vapor
                    param1 = 3842;
                    break;
                case 45373:                                 // Bloodberry Elixir
                    param1 = 4075;
                    break;
                default:                                    // default case (don't must be)
                    break;
            }
            break;
        }
        case SPELL_FAILED_TOTEMS:
        {
            param1 = spellInfo->Totem[0];
            param2 = spellInfo->Totem[1];
            break;
        }
        case SPELL_FAILED_TOTEM_CATEGORY:
        {
            param1 = spellInfo->TotemCategory[0];
            param2 = spellInfo->TotemCategory[1];

            //if (hasBit1 && hasBit2 && (!spellInfo->TotemCategory[0] || !spellInfo->TotemCategory[1]))
            //    sLog->OutCS(">>> SPELL_FAILED_TOTEM_CATEGORY ");

            break;
        }
        case SPELL_FAILED_EQUIPPED_ITEM_CLASS:
        case SPELL_FAILED_EQUIPPED_ITEM_CLASS_MAINHAND:
        case SPELL_FAILED_EQUIPPED_ITEM_CLASS_OFFHAND:
        {
            param1 = spellInfo->EquippedItemClass;
            param2 = spellInfo->EquippedItemSubClassMask;
            break;
        }
        case SPELL_FAILED_TOO_MANY_OF_ITEM:
        {
            for (uint32 eff = 0; eff < MAX_SPELL_EFFECTS; ++eff)
                if (uint32 item = spellInfo->Effects[eff].ItemType)
                    if (ItemTemplate const* proto = sObjectMgr->GetItemTemplate(item))
                    {
                        param1 = proto->ItemLimitCategory;
                        break;
                    }
            break;
        }
        case SPELL_FAILED_PREVENTED_BY_MECHANIC:
        {
            param1 = spellInfo->GetAllEffectsMechanicMask();   // SpellMechanic.dbc id
            break;
        }
        case SPELL_FAILED_NEED_EXOTIC_AMMO:
        {
            param1 = spellInfo->EquippedItemSubClassMask;          // seems correct...
            break;
        }
        case SPELL_FAILED_NEED_MORE_ITEMS:
        {
            //param1 = 0;                              // Item count?
            //param2 = 0;                              // Item id
            break;
        }
        case SPELL_FAILED_MIN_SKILL:
        {
            //param1 = 0;                              // SkillLine.dbc id
            //param2 = 0;                              // required skill value
            break;
        }
        case SPELL_FAILED_FISHING_TOO_LOW:
        {
            //param1 = 0;                              // required fishing skill
            break;
        }
        case SPELL_FAILED_CUSTOM_ERROR:
        {
            param1 = customError;
            break;
        }
        case SPELL_FAILED_SILENCED:
        {
            //param1 = 0;                              // Unknown
            break;
        }
        case SPELL_FAILED_REAGENTS:
        {
            for (uint32 i = 0; i < MAX_SPELL_REAGENTS; i++)
            {
                if (spellInfo->Reagent[i] <= 0)
                    continue;

                uint32 itemid    = spellInfo->Reagent[i];
                uint32 itemcount = spellInfo->ReagentCount[i];

                if (!caster->HasItemCount(itemid, itemcount))
                {
                    param1 = itemid;
                    param2 = itemcount;
                    break;
                }
            }

            if(!param1 && spellInfo->ReagentCurrency != 0)
            {
                uint32 currencyId    = spellInfo->ReagentCurrency;
                uint32 currencyCount = spellInfo->ReagentCurrencyCount;

                if (!caster->HasCurrency(currencyId, currencyCount))
                {
                    param1 = currencyId;
                    param2 = currencyCount;
                }
            }
            break;
        }
        // TODO: SPELL_FAILED_NOT_STANDING
        default:
            break;
    }

    //! 5.4.1
    WorldPacket data(SMSG_CAST_FAILED, 4 * 4 + 1 + 1);
    data.WriteBit(!param1);
    data.WriteBit(!param2);
    data << uint32(result);                                 // cast result
    data << uint32(spellInfo->Id);                          // spellId
    if (param1)
        data << uint32(param1);
    data << uint8(cast_count);                              // single cast or multi 2.3 (0/1)
    if (param2)
        data << uint32(param2);
    caster->GetSession()->SendPacket(&data);
}

void Spell::SendSpellStart()
{
    if (!IsNeedSendToClient())
        return;

    //sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "Sending SMSG_SPELL_START id=%u", m_spellInfo->Id);

    uint32 castFlags = CAST_FLAG_HAS_TRAJECTORY;

    if ((IsTriggered() && !m_spellInfo->IsAutoRepeatRangedSpell() && !(AttributesCustomEx4 & SPELL_ATTR4_TRIGGERED) && !(AttributesCustomCu & SPELL_ATTR0_CU_DOESENT_INTERRUPT_CHANNELING)) || m_triggeredByAuraSpell)
        castFlags |= CAST_FLAG_PENDING;

    if ((m_caster->GetTypeId() == TYPEID_PLAYER ||
        (m_caster->GetTypeId() == TYPEID_UNIT && (m_caster->ToCreature()->isPet() || m_caster->ToCreature()->IsVehicle())))
        && !GetSpellInfo()->HasPower(POWER_HEALTH))
        castFlags |= CAST_FLAG_POWER_LEFT_SELF;

    if (m_spellInfo->RuneCostID && GetSpellInfo()->HasPower(POWER_RUNES))
        castFlags |= CAST_FLAG_UNKNOWN_19;

    if(m_spellInfo->Id == 125084)
        castFlags = 262913;
    //sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: SMSG_SPELL_START, castCount: %u, spellId: %u, castFlags: %u", m_cast_count, m_spellInfo->Id, castFlags);

    ObjectGuid guid1A0 = ObjectGuid();
    ObjectGuid casterGuid = m_caster->GetObjectGuid();
    ObjectGuid itemCasterGuid = m_CastItem ? m_CastItem->GetObjectGuid() : casterGuid;
    ObjectGuid targetGuid;
    ObjectGuid itemTargetGuid;

    bool hasRuneState = castFlags & CAST_FLAG_RUNE_LIST;
    bool hasCastImmunities = false;
    bool hasCastSchoolImmunities = false;
    uint8 byte180 = 0;
    bool hasPowerUnit = false/*castFlags & CAST_FLAG_POWER_LEFT_SELF*/;
    bool hasPredictedType = false;
    bool hasAdjustMissile = false;
    bool hasPredictedHeal = false;
    bool hasVisualChain = false;
    uint8 AmmoInventoryType = 0;
    uint32 AmmoDisplayId = 0;

    uint32 powerCount = 0;
    uint32 missCount2 = 0;
    uint32 extraTargetsCount = 0;
    std::vector<ObjectGuid> extraTargetGuids(extraTargetsCount);

    if(m_caster->GetTypeId() == TYPEID_UNIT && m_spellInfo->EquippedItemClass == ITEM_CLASS_WEAPON && m_spellInfo->EquippedItemSubClassMask & 0x0004000C/*BOW, GUN, CROSSBOW*/)
    {
        castFlags |= CAST_FLAG_PROJECTILE;
        WriteProjectile(AmmoInventoryType, AmmoDisplayId);
    }

    uint32 targetMask = m_targets.GetTargetMask();
    if (targetMask & (TARGET_FLAG_UNIT | TARGET_FLAG_CORPSE_ALLY | TARGET_FLAG_GAMEOBJECT | TARGET_FLAG_CORPSE_ENEMY | TARGET_FLAG_UNIT_MINIPET))
        targetGuid = m_targets.m_objectTargetGUID;

    if (targetMask & (TARGET_FLAG_ITEM | TARGET_FLAG_TRADE_ITEM))
        if (Item* item = m_targets.GetItemTarget())
            itemTargetGuid = item->GetGUID();
        else
            targetMask &= ~(TARGET_FLAG_ITEM | TARGET_FLAG_TRADE_ITEM);

    WorldPacket data(SMSG_SPELL_START, (8+8+4+4+2));
    data.WriteBit(!hasRuneState);                               // has rune state after
    data.WriteBit(!hasCastImmunities);                          // !dword198
    data.WriteBit(!hasCastSchoolImmunities);                    // !dword194
    data.WriteGuidMask<5>(casterGuid);
    data.WriteBits(powerCount, 21);                             // power count
    data.WriteGuidMask<7>(casterGuid);
    data.WriteBits(missCount2, 25);                             // miss count 2

    data.WriteGuidMask<7>(itemCasterGuid);
    data.WriteBit(!byte180);                                    // !byte180

    data.WriteBits(extraTargetsCount, 20);                      // extra targets count
    data.WriteGuidMask<0>(casterGuid);

    data.WriteBit(!guid1A0);                                    // guid1A0 marker
    data.WriteBit(hasPowerUnit);
    if (hasPowerUnit)
    {
        data.WriteGuidMask<7, 3, 1, 5, 4, 2, 0>(casterGuid);
        data.WriteBits(1, 21);                                  // power type count
        data.WriteGuidMask<6>(casterGuid);
    }
    data.WriteBit(!hasPredictedType);                           // !byte1AC
    data.WriteBit(!hasAdjustMissile);                           // !has delay moment
    data.WriteBit(!hasRuneState);                               // has rune state before

    for (uint32 i = 0; i < extraTargetsCount; ++i)
        data.WriteGuidMask<3, 1, 7, 6, 2, 4, 5, 0>(extraTargetGuids[i]);

    data.WriteBit(!AmmoInventoryType);                          // !AmmoInventoryType
    data.WriteGuidMask<2>(itemCasterGuid);
    data.WriteBits(0, 24);                                      // hit count

    for (uint32 i = 0; i < missCount2; ++i)
    {
        uint32 hitResult = 0;
        data.WriteBits(hitResult, 4);                           // unk bits
        if (hitResult == SPELL_MISS_REFLECT)
            data.WriteBits(0, 4);                               // Reflect result
    }

    data.WriteGuidMask<3>(itemCasterGuid);
    data.WriteGuidMask<6>(casterGuid);
    data.WriteBits(0, 13);                                      // dword2C
    data.WriteGuidMask<4>(itemCasterGuid);
    data.WriteBit(!hasPredictedHeal);                           // !has predicted healing
    data.WriteGuidMask<0>(itemCasterGuid);
    data.WriteBit(hasVisualChain);                              // byte17C
    data.WriteBit(m_targets.HasSrc());
    data.WriteGuidMask<1>(itemCasterGuid);
    data.WriteGuidMask<4, 5, 6, 7, 3, 1, 2, 0>(guid1A0);
    data.WriteGuidMask<2>(casterGuid);

    uint8 runeCooldownCount = 0;
    if (castFlags & CAST_FLAG_RUNE_LIST)
        runeCooldownCount = MAX_RUNES;
    data.WriteBits(runeCooldownCount, 3);
    data.WriteBit(m_targets.HasDst());
    if (m_targets.HasDst())
    {
        ObjectGuid destTransportGuid = m_targets.GetDst()->_transportGUID;
        data.WriteGuidMask<4, 1, 6, 2, 3, 0, 7, 5>(destTransportGuid);
    }
    if (m_targets.HasSrc())
    {
        ObjectGuid srcTransportGuid = m_targets.GetSrc()->_transportGUID;
        data.WriteGuidMask<4, 5, 6, 0, 7, 1, 2, 3>(srcTransportGuid);
    }
    data.WriteGuidMask<1>(casterGuid);
    data.WriteBit(!AmmoDisplayId);                            // !dword16C
    data.WriteGuidMask<4>(casterGuid);
    data.WriteBit((targetMask & TARGET_FLAG_STRING) == 0);
    if (targetMask & TARGET_FLAG_STRING)
        data.WriteBits(m_targets.m_strTarget.length(), 7);
    data.WriteBit(!targetMask);
    data.WriteBits(0, 24);                                      // miss count
    data.WriteGuidMask<6, 5>(itemCasterGuid);

    data.WriteBit(!targetGuid);
    data.WriteGuidMask<7, 5, 3, 6, 1, 4, 0, 2>(targetGuid);

    if (targetMask)
        data.WriteBits(targetMask, 20);

    data.WriteBit(!itemTargetGuid);
    data.WriteGuidMask<0, 1, 2, 5, 4, 7, 3, 6>(itemTargetGuid);
    data.WriteBit(!hasAdjustMissile);                           // !has elevation
    data.WriteGuidMask<3>(casterGuid);

    data.FlushBits();

    data.WriteGuidBytes<5, 1>(itemCasterGuid);
    if (m_targets.HasSrc())
    {
        ObjectGuid srcTransportGuid = m_targets.GetSrc()->_transportGUID;
        Position const& pos = srcTransportGuid ? m_targets.GetSrc()->_transportOffset : m_targets.GetSrc()->_position;

        data.WriteGuidBytes<1, 3>(srcTransportGuid);
        data << float(pos.GetPositionZ());
        data.WriteGuidBytes<7, 5>(srcTransportGuid);
        data << float(pos.GetPositionX());
        data.WriteGuidBytes<2, 0, 4>(srcTransportGuid);
        data << float(pos.GetPositionY());
        data.WriteGuidBytes<6>(srcTransportGuid);
    }

    for (uint32 i = 0; i < extraTargetsCount; ++i)
    {
        data.WriteGuidBytes<5, 2>(extraTargetGuids[i]);
        data << float(0.0f);    // dst Y
        data << float(0.0f);    // dst X
        data.WriteGuidBytes<7>(extraTargetGuids[i]);
        data << float(0.0f);    // dst Z
        data.WriteGuidBytes<3, 1, 6, 4, 0>(extraTargetGuids[i]);
    }

    if (AmmoInventoryType)
        data << uint8(AmmoInventoryType);

    data.WriteGuidBytes<4, 2, 5, 6, 3, 1, 7, 0>(targetGuid);

    if (m_targets.HasDst())
    {
        ObjectGuid dstTransportGuid = m_targets.GetDst()->_transportGUID;
        Position const& pos = dstTransportGuid ? m_targets.GetDst()->_transportOffset : m_targets.GetDst()->_position;

        data << float(pos.GetPositionX());
        data.WriteGuidBytes<0>(dstTransportGuid);
        data << float(pos.GetPositionZ());
        data.WriteGuidBytes<7, 4, 1, 5, 3, 2>(dstTransportGuid);
        data << float(pos.GetPositionY());
        data.WriteGuidBytes<6>(dstTransportGuid);
    }

    if (hasAdjustMissile)
        data << uint32(0);                                      // delay moment

    if (hasRuneState)
    {
        Player* player = m_caster->ToPlayer();
        data << uint8(player->GetRunesState());                 // runes state after
    }

    data.WriteGuidBytes<3, 7, 0, 2, 1, 6, 4, 5>(guid1A0);

    if (hasPowerUnit)
    {
        Powers powertype = POWER_MANA;
        SpellPowerEntry power;
        if (GetSpellInfo()->GetSpellPowerByCasterPower(m_caster, power))
            powertype = (Powers)power.powerType;

        data << uint32(m_caster->GetHealth());
        data.WriteGuidBytes<7, 2>(m_caster->GetObjectGuid());
        data << uint32(m_caster->GetPower(powertype));
        data << uint32(powertype);
        data << uint32(m_caster->GetTotalAttackPowerValue(BASE_ATTACK));
        data << uint32(m_caster->GetSpellPowerDamage());
        data.WriteGuidBytes<6, 4, 5, 0, 1, 3>(m_caster->GetObjectGuid());
    }

    data.WriteGuidBytes<2>(itemCasterGuid);
    if (hasRuneState)
        data << uint8(m_runesState);                            // runes state before
    data.WriteGuidBytes<1, 7, 6, 5, 2, 3, 0, 4>(itemTargetGuid);

    data.WriteGuidBytes<1>(casterGuid);
    if (targetMask & TARGET_FLAG_STRING)
        data.WriteString(m_targets.m_strTarget);

    for (uint32 i = 0; i < powerCount; ++i)
    {
        // not sure about this ...
        data << uint8((Powers)GetSpellInfo()->PowerType);
        data << uint32(m_caster->GetPower((Powers)GetSpellInfo()->PowerType));
    }

    data << uint32(m_spellInfo->Id);                            // spellId

    if (hasPredictedType)
        data << uint8(0);

    data << uint8(m_cast_count);                                // pending spell cast?

    for (int i = 0; i < runeCooldownCount; ++i)
    {
        Player* player = m_caster->ToPlayer();
        float baseCd = float(RUNE_BASE_COOLDOWN);
        data << uint8((baseCd - float(player->GetRuneCooldown(i))) / baseCd * 255); // rune cooldown passed
    }

    data.WriteGuidBytes<6>(itemCasterGuid);
    if (AmmoDisplayId)
        data << uint32(AmmoDisplayId);
    data.WriteGuidBytes<0>(itemCasterGuid);
    data.WriteGuidBytes<4>(casterGuid);

    if (hasAdjustMissile)
        data << float(0.0f);                                    // elevation

    if (hasVisualChain)
    {
        data << uint32(0);
        data << uint32(0);
    }

    data << uint32(m_casttime);
    data.WriteGuidBytes<3>(itemCasterGuid);

    if (hasCastSchoolImmunities)
        data << uint32(0);

    if (byte180)
        data << uint8(byte180);

    if (hasCastImmunities)
        data << uint32(0);

    data.WriteGuidBytes<7>(casterGuid);

    if (hasPredictedHeal)
        data << uint32(0);                                      // amount

    data.WriteGuidBytes<4>(itemCasterGuid);
    data.WriteGuidBytes<5>(casterGuid);
    data << uint32(castFlags);                                  // dword28
    data.WriteGuidBytes<3>(casterGuid);
    data.WriteGuidBytes<7>(itemCasterGuid);
    data.WriteGuidBytes<6, 0, 2>(casterGuid);

    m_caster->SendMessageToSet(&data, true);
}

void Spell::SendSpellPendingCast()
{
    uint32 _spellId = 0;
    Player* player = m_caster->ToPlayer();
    if(!player)
        return;

    if (const std::vector<SpellPendingCast> *spell_pending = sSpellMgr->GetSpellPendingCast(m_spellInfo->Id))
    {
        bool check = false;
        for (std::vector<SpellPendingCast>::const_iterator i = spell_pending->begin(); i != spell_pending->end(); ++i)
        {
            switch (i->option)
            {
                case 0: // Check Spec
                {
                    if(player->GetLootSpecID() == i->check)
                    {
                        _spellId = i->pending_id;
                        check = true;
                    }
                    break;
                }

            }

            if (check)
                break;
        }
    }

    if(!_spellId)
        return;

    WorldPacket data(SMSG_SPELL_SCRIPT_CAST, 4);
    data << uint32(_spellId);           //Spell Id
    player->GetSession()->SendPacket(&data);
}

void Spell::SendSpellGo()
{
    // not send invisible spell casting
    if (!IsNeedSendToClient())
        return;

    //sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "Sending SMSG_SPELL_GO id=%u", m_spellInfo->Id);

    uint32 castFlags = CAST_FLAG_UNKNOWN_9;
    // triggered spells with spell visual != 0
    if ((IsTriggered() && !m_spellInfo->IsAutoRepeatRangedSpell() && !(AttributesCustomEx4 & SPELL_ATTR4_TRIGGERED) && !(AttributesCustomCu & SPELL_ATTR0_CU_DOESENT_INTERRUPT_CHANNELING)) || m_triggeredByAuraSpell)
        castFlags |= CAST_FLAG_PENDING;

    if ((m_caster->GetTypeId() == TYPEID_PLAYER ||
        (m_caster->GetTypeId() == TYPEID_UNIT && (m_caster->ToCreature()->isAnySummons() || m_caster->ToCreature()->IsVehicle())))
        && !GetSpellInfo()->HasPower(POWER_HEALTH))
        castFlags |= CAST_FLAG_POWER_LEFT_SELF; // should only be sent to self, but the current messaging doesn't make that possible

    if ((m_caster->GetTypeId() == TYPEID_PLAYER)
        && (m_caster->getClass() == CLASS_DEATH_KNIGHT)
        && m_spellInfo->RuneCostID
        && GetSpellInfo()->HasPower(POWER_RUNES))
    {
        castFlags |= CAST_FLAG_UNKNOWN_19;                   // same as in SMSG_SPELL_START
        castFlags |= CAST_FLAG_RUNE_LIST;                    // rune cooldowns list
    }

    if (m_spellInfo->HasEffect(SPELL_EFFECT_ACTIVATE_RUNE))
    {
        castFlags |= CAST_FLAG_UNKNOWN_19;                   // same as in SMSG_SPELL_START
        castFlags |= CAST_FLAG_RUNE_LIST;                    // rune cooldowns list
    }

    if (m_targets.HasTraj())
        castFlags |= CAST_FLAG_ADJUST_MISSILE;

    ObjectGuid guid1A0;
    ObjectGuid casterGuid = m_caster->GetObjectGuid();
    ObjectGuid itemCasterGuid = m_CastItem ? m_CastItem->GetObjectGuid() : casterGuid;
    ObjectGuid targetGuid;
    ObjectGuid itemTargetGuid;

    uint32 extraTargetsCount = 0;
    std::vector<ObjectGuid> extraTargetGuids(extraTargetsCount);

    SpellPowerEntry power;
    uint32 powerCount = GetSpellInfo()->GetSpellPowerByCasterPower(m_caster, power) ? 1 : 0;
    Powers powerType = Powers(power.powerType);

    bool hasPowerUnit = false/*castFlags & CAST_FLAG_POWER_LEFT_SELF*/;
    bool hasPredictedHeal = castFlags & CAST_FLAG_HEAL_PREDICTION;
    uint8 byte180 = 0;
    bool hasCastSchoolImmunities = false;
    bool hasPredictedType = false;
    bool hasRuneState = castFlags & CAST_FLAG_RUNE_LIST;
    bool hasAdjustMissile = castFlags & CAST_FLAG_ADJUST_MISSILE;
    bool hasVisualChain = false;
    bool hasCastImmunities = false;
    uint32 AmmoDisplayId = 0;
    uint8 AmmoInventoryType = 0;

    if(m_caster->GetTypeId() == TYPEID_UNIT && m_spellInfo->EquippedItemClass == ITEM_CLASS_WEAPON && m_spellInfo->EquippedItemSubClassMask & 0x0004000C/*BOW, GUN, CROSSBOW*/)
    {
        castFlags |= CAST_FLAG_PROJECTILE;
        WriteProjectile(AmmoInventoryType, AmmoDisplayId);
    }

    uint32 targetMask = m_targets.GetTargetMask();
    if (targetMask & (TARGET_FLAG_UNIT | TARGET_FLAG_CORPSE_ALLY | TARGET_FLAG_GAMEOBJECT | TARGET_FLAG_CORPSE_ENEMY | TARGET_FLAG_UNIT_MINIPET))
        targetGuid = m_targets.m_objectTargetGUID;

    if (targetMask & (TARGET_FLAG_ITEM | TARGET_FLAG_TRADE_ITEM))
        if (Item* item = m_targets.GetItemTarget())
            itemTargetGuid = item->GetGUID();
        else
            targetMask &= ~(TARGET_FLAG_ITEM | TARGET_FLAG_TRADE_ITEM);

    uint32 miss = 0, hit = 0;
    for (std::list<TargetInfo>::iterator ihit = m_UniqueTargetInfo.begin(); ihit != m_UniqueTargetInfo.end() && hit <= 255; ++ihit)
    {
        if (ihit->effectMask == 0)                      // No effect apply - all immuned add state
            ihit->missCondition = SPELL_MISS_IMMUNE2;

        if (ihit->missCondition == SPELL_MISS_NONE)
        {
            m_channelTargetEffectMask |= ihit->effectMask;
            ++hit;
        }
        else
            ++miss;
    }
    for (std::list<TargetInfo>::iterator ihit = m_VisualHitTargetInfo.begin(); ihit != m_VisualHitTargetInfo.end() && hit <= 255; ++ihit)
        ++hit;

    // Reset m_needAliveTargetMask for non channeled spell
    if (!m_spellInfo->IsChanneled())
        m_channelTargetEffectMask = 0;

    for (std::list<GOTargetInfo>::const_iterator ighit = m_UniqueGOTargetInfo.begin(); ighit != m_UniqueGOTargetInfo.end() && hit <= 255; ++ighit)
        ++hit;

    //sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: SMSG_SPELL_GO, castCount: %u, spellId: %u, castFlags: %u", m_cast_count, m_spellInfo->Id, castFlags);

    WorldPacket data(SMSG_SPELL_GO, 50);                        // guess size
    data.WriteBits(miss, 25);                                   // miss count 2
    data.WriteGuidMask<5>(casterGuid);
    data.WriteGuidMask<1>(itemCasterGuid);
    data.WriteBits(extraTargetsCount, 20);                      // extra targets count
    data.WriteGuidMask<0>(itemCasterGuid);
    data.WriteBit(!guid1A0);                                    // "Guid 1A0 marker"
    data.WriteGuidMask<0>(casterGuid);

    for (uint32 i = 0; i < extraTargetsCount; ++i)
        data.WriteGuidMask<4, 1, 5, 2, 0, 3, 6, 7>(extraTargetGuids[i]);

    data.WriteBit(!AmmoDisplayId);                              // !AmmoDisplayId
    data.WriteGuidMask<0, 1, 4, 6, 5, 3, 2, 7>(guid1A0);
    data.WriteBit(!byte180);                                     // !byte180
    data.WriteGuidMask<3>(casterGuid);
    data.WriteBit((targetMask & TARGET_FLAG_STRING) == 0);
    data.WriteGuidMask<4>(itemCasterGuid);
    data.WriteBit(!hasCastSchoolImmunities);                    // !dword194
    data.WriteBits(miss, 24);                                   // miss count

    data.WriteBit(!targetGuid);
    data.WriteGuidMask<3, 1, 5, 4, 0, 2, 6, 7>(targetGuid);

    // misses
    uint32 counter = 0;
    for (std::list<TargetInfo>::const_iterator ihit = m_UniqueTargetInfo.begin(); ihit != m_UniqueTargetInfo.end() && counter <= 255; ++ihit)
    {
        if (ihit->missCondition != SPELL_MISS_NONE)
        {
            data.WriteGuidMask<0, 5, 2, 7, 3, 6, 4, 1>(ihit->targetGUID);
            ++counter;
        }
    }

    data.WriteBit(!hasPredictedHeal);                           // !has predicted healing
    data.WriteBit(hasPowerUnit);                                // has power unit
    data.WriteBit(!hasPredictedType);                           // !byte1AC
    counter = 0;
    for (std::list<TargetInfo>::const_iterator ihit = m_UniqueTargetInfo.begin(); ihit != m_UniqueTargetInfo.end() && counter <= 255; ++ihit)
    {
        if (ihit->missCondition == SPELL_MISS_NONE)
            continue;

        data.WriteBits(ihit->missCondition, 4);
        if (ihit->missCondition == SPELL_MISS_REFLECT)
            data.WriteBits(ihit->reflectResult, 4);
        ++counter;
    }
    data.WriteGuidMask<2>(itemCasterGuid);

    if (hasPowerUnit)
    {
        ObjectGuid powerGuid = m_caster->GetObjectGuid();

        data.WriteGuidMask<5>(powerGuid);
        data.WriteBits(1, 21);                                  // power count
        data.WriteGuidMask<1, 7, 4, 2, 3, 6, 0>(powerGuid);
    }
    data.WriteGuidMask<3>(itemCasterGuid);
    uint8 runeCooldownCount = 0;
    if (hasRuneState)
        runeCooldownCount = MAX_RUNES;
    data.WriteBits(runeCooldownCount, 3);
    data.WriteGuidMask<2>(casterGuid);
    data.WriteBit(!hasRuneState);                               // has rune state before
    if (targetMask & TARGET_FLAG_STRING)
        data.WriteBits(m_targets.m_strTarget.length(), 7);
    data.WriteGuidMask<6>(itemCasterGuid);
    data.WriteBit(!hasAdjustMissile);                           // !has delay moment
    data.WriteGuidMask<6>(casterGuid);
    data.WriteBit(!targetMask);
    data.WriteBits(0, 13);                                      // dword2C
    data.WriteGuidMask<5>(itemCasterGuid);
    data.WriteBits(hasRuneState ? 1 : powerCount, 21);          // powerCount
    data.WriteBit(!AmmoInventoryType);                          // !AmmoInventoryType
    data.WriteGuidMask<7>(casterGuid);
    data.WriteBit(m_targets.HasDst());
    data.WriteBit(hasVisualChain);                              // byte17C

    if (m_targets.HasDst())
    {
        ObjectGuid dstTransportGuid = m_targets.GetDst()->_transportGUID;

        data.WriteGuidMask<0, 4, 3, 5, 2, 7, 6, 1>(dstTransportGuid);
    }
    data.WriteBit(!hasCastImmunities);                          // !dword198
    if (targetMask)
        data.WriteBits(targetMask, 20);
    data.WriteGuidMask<4>(casterGuid);
    data.WriteBit(!hasRuneState);                               // has rune state after
    data.WriteBit(!hasAdjustMissile);                           // has elevation
    data.WriteGuidMask<1>(casterGuid);
    data.WriteBit(m_targets.HasSrc());
    if (m_targets.HasSrc())
    {
        ObjectGuid srcTransportGuid = m_targets.GetSrc()->_transportGUID;

        data.WriteGuidMask<3, 5, 1, 7, 2, 6, 0, 4>(srcTransportGuid);
    }
    data.WriteBits(hit, 24);
    counter = 0;
    // hits
    for (std::list<TargetInfo>::const_iterator ihit = m_UniqueTargetInfo.begin(); ihit != m_UniqueTargetInfo.end() && counter <= 255; ++ihit)
    {
        if (ihit->missCondition == SPELL_MISS_NONE)
        {
            data.WriteGuidMask<2, 3, 7, 1, 5, 4, 6, 0>(ihit->targetGUID);
            ++counter;
        }
    }
    for (std::list<TargetInfo>::iterator ihit = m_VisualHitTargetInfo.begin(); ihit != m_VisualHitTargetInfo.end() && counter <= 255; ++ihit)
    {
        data.WriteGuidMask<2, 3, 7, 1, 5, 4, 6, 0>(ihit->targetGUID);
        ++counter;
    }
    for (std::list<GOTargetInfo>::const_iterator ighit = m_UniqueGOTargetInfo.begin(); ighit != m_UniqueGOTargetInfo.end() && counter <= 255; ++ighit)
    {
        data.WriteGuidMask<2, 3, 7, 1, 5, 4, 6, 0>(ighit->targetGUID);
        ++counter;
    }
    data.WriteGuidMask<7>(itemCasterGuid);
    data.WriteBit(!itemTargetGuid);
    data.WriteGuidMask<4, 3, 7, 0, 1, 6, 5, 2>(itemTargetGuid);

    data.FlushBits();

    data.WriteGuidBytes<7, 4, 3, 2, 1, 0, 6, 5>(itemTargetGuid);

    data.WriteGuidBytes<3, 1, 6, 5, 0, 7, 2, 4>(targetGuid);

    if (hasRuneState)
    {
        data << uint32(0);
        data << uint8(POWER_RUNES);
    }
    else
    {
        //for (uint32 i = 0; i < powerCount; ++i)
        if(powerCount)
        {
            data << uint32(m_caster->GetPower(powerType));
            data << uint8(powerType); //Power
        }
    }

    counter = 0;
    // hits
    for (std::list<TargetInfo>::const_iterator ihit = m_UniqueTargetInfo.begin(); ihit != m_UniqueTargetInfo.end() && counter <= 255; ++ihit)
    {
        if (ihit->missCondition == SPELL_MISS_NONE)
        {
            data.WriteGuidBytes<6, 5, 0, 3, 2, 1, 4, 7>(ihit->targetGUID);
            ++counter;
        }
    }
    for (std::list<TargetInfo>::iterator ihit = m_VisualHitTargetInfo.begin(); ihit != m_VisualHitTargetInfo.end() && hit <= 255; ++ihit)
    {
        data.WriteGuidBytes<6, 5, 0, 3, 2, 1, 4, 7>(ihit->targetGUID);
        ++counter;
    }
    for (std::list<GOTargetInfo>::const_iterator ighit = m_UniqueGOTargetInfo.begin(); ighit != m_UniqueGOTargetInfo.end() && counter <= 255; ++ighit)
    {
        data.WriteGuidBytes<6, 5, 0, 3, 2, 1, 4, 7>(ighit->targetGUID);
        ++counter;
    }

    for (uint32 i = 0; i < extraTargetsCount; ++i) // TARGET_FLAG_EXTRA_TARGETS
    {
        data.WriteGuidBytes<6, 7, 2, 5, 0>(extraTargetGuids[i]);
        data << float(0.0f);        // dst Y
        data.WriteGuidBytes<4, 1>(extraTargetGuids[i]);
        data << float(0.0f);        // dst Z
        data.WriteGuidBytes<3>(extraTargetGuids[i]);
        data << float(0.0f);        // dst X
    }

    if (m_targets.HasSrc())
    {
        ObjectGuid srcTransportGuid = m_targets.GetSrc()->_transportGUID;
        Position const& pos = srcTransportGuid ? m_targets.GetSrc()->_transportOffset : m_targets.GetSrc()->_position;

        data.WriteGuidBytes<4, 3>(srcTransportGuid);
        data << float(pos.GetPositionX());
        data.WriteGuidBytes<6, 2, 1, 5>(srcTransportGuid);
        data << float(pos.GetPositionY());
        data << float(pos.GetPositionZ());
        data.WriteGuidBytes<7, 0>(srcTransportGuid);
    }

    data.WriteGuidBytes<2, 7, 5, 1, 0, 3, 6, 4>(guid1A0);

    if (m_targets.HasDst())
    {
        ObjectGuid dstTransportGuid = m_targets.GetDst()->_transportGUID;
        Position const& pos = dstTransportGuid ? m_targets.GetDst()->_transportOffset : m_targets.GetDst()->_position;

        data << float(pos.GetPositionY());
        data.WriteGuidBytes<3>(dstTransportGuid);
        data << float(pos.GetPositionZ());
        data.WriteGuidBytes<4, 6, 7>(dstTransportGuid);
        data << float(pos.GetPositionX());
        data.WriteGuidBytes<0, 2, 5, 1>(dstTransportGuid);
    }

    if (hasPowerUnit)
    {
        ObjectGuid powerGuid = m_caster->GetObjectGuid();
        Powers powertype = POWER_MANA;
        SpellPowerEntry power;
        if (GetSpellInfo()->GetSpellPowerByCasterPower(m_caster, power))
            powertype = (Powers)power.powerType;

        data.WriteGuidBytes<2, 1, 3>(powerGuid);
        data << uint32(m_caster->GetHealth());
        data.WriteGuidBytes<5, 6, 7>(powerGuid);
        data << uint32(m_caster->GetTotalAttackPowerValue(BASE_ATTACK));
        data.WriteGuidBytes<0>(powerGuid);
        data << uint32(m_caster->GetPower(powertype));
        data << uint32(powertype);
        data.WriteGuidBytes<4>(powerGuid);
        data << uint32(m_caster->GetSpellPowerDamage());
    }

    if (hasCastImmunities)
        data << uint32(0);

    if (hasRuneState)
        data << uint8(m_runesState);                        // runes state before

    if (byte180)
        data << uint8(byte180);

    // misses
    counter = 0;
    for (std::list<TargetInfo>::const_iterator ihit = m_UniqueTargetInfo.begin(); ihit != m_UniqueTargetInfo.end() && counter <= 255; ++ihit)
    {
        if (ihit->missCondition != SPELL_MISS_NONE)
        {
            data.WriteGuidBytes<7, 3, 1, 6, 5, 2, 0, 4>(ihit->targetGUID);
            ++counter;
        }
    }
    data << uint32(castFlags);
    data.WriteGuidBytes<2>(itemCasterGuid);
    data.WriteGuidBytes<4>(casterGuid);
    if (hasVisualChain)
    {
        data << uint32(0);
        data << uint32(0);
    }

    if (hasPredictedHeal)
        data << uint32(0);

    data.WriteGuidBytes<6, 1>(casterGuid);

    if (hasAdjustMissile)
        data << m_targets.GetElevation();

    data.WriteGuidBytes<5>(itemCasterGuid);
    data.WriteGuidBytes<7>(casterGuid);

    if (hasPredictedType)
        data << uint8(0);

    if (AmmoInventoryType)
        data << uint8(AmmoInventoryType);

    data.WriteGuidBytes<3>(itemCasterGuid);
    data.WriteGuidBytes<3, 2>(casterGuid);
    data << uint8(m_cast_count);                            // pending spell cast?
    if (hasAdjustMissile)
        data << uint32(m_delayMoment);
    data.WriteGuidBytes<6, 4>(itemCasterGuid);
    for (int i = 0; i < runeCooldownCount; ++i)
    {
        Player* player = m_caster->ToPlayer();
        float baseCd = float(RUNE_BASE_COOLDOWN);
        data << uint8((baseCd - float(player->GetRuneCooldown(i))) / baseCd * 255); // rune cooldown passed
    }
    if (hasRuneState)
    {
        Player* player = m_caster->ToPlayer();
        data << uint8(player->GetRunesState());             // runes state after
    }

    data.WriteGuidBytes<0, 1>(itemCasterGuid);

    if (hasCastSchoolImmunities)
        data << uint32(0);

    if (AmmoDisplayId)
        data << uint32(AmmoDisplayId);

    data.WriteGuidBytes<7>(itemCasterGuid);

    data << uint32(getMSTime());                            // timestamp
    data.WriteGuidBytes<0>(casterGuid);
    data << uint32(m_spellInfo->Id);                        // spellId
    data.WriteGuidBytes<5>(casterGuid);
    if (targetMask & TARGET_FLAG_STRING)
        data.WriteString(m_targets.m_strTarget);

    m_caster->SendMessageToSet(&data, true);
}

void Spell::SendLogExecute()
{
    ObjectGuid casterGuid = m_caster->GetObjectGuid();

    WorldPacket data(SMSG_SPELLLOGEXECUTE, 8 + 4 + 4 + 4 + 4 + 8);
    data.WriteGuidMask<7, 0, 6, 3, 1, 5>(casterGuid);

    uint32 effectCount = 0;
    uint32 bitpos = data.bitwpos();
    data.WriteBits(effectCount, 19);

    ByteBuffer buff;
    for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
    {
        if (!m_effectExecuteData[i])
            continue;

        uint32 effect = m_spellInfo->GetEffect(i, m_diffMode)->Effect;

        switch (effect)
        {
            case SPELL_EFFECT_POWER_DRAIN:
            case SPELL_EFFECT_POWER_BURN:
            {
                data.WriteBits(0, 24);      // generic count
                data.WriteBits(0, 22);      // trade count
                data.WriteBit(0);           // no power data
                data.WriteBits(0, 22);      // feed count
                data.WriteBits(0, 21);      // extra count
                data.WriteBits(0, 21);      // durability count
                uint32 count = m_effectExecuteData[i]->guids.size();
                data.WriteBits(count, 20);  // power count
                for (uint32 j = 0; j < count; ++j)
                {
                    data.WriteGuidMask<0, 7, 5, 2, 3, 6, 1, 4>(m_effectExecuteData[i]->guids[j]);

                    buff.WriteGuidBytes<3, 0, 7, 1>(m_effectExecuteData[i]->guids[j]);
                    buff << uint32(m_effectExecuteData[i]->param2[j]);
                    buff << float(m_effectExecuteData[i]->floatParam[j]);
                    buff.WriteGuidBytes<6, 5>(m_effectExecuteData[i]->guids[j]);
                    buff << uint32(m_effectExecuteData[i]->param1[j]);
                    buff.WriteGuidBytes<4, 2>(m_effectExecuteData[i]->guids[j]);
                }
                break;
            }
            case SPELL_EFFECT_ADD_EXTRA_ATTACKS:
            {
                data.WriteBits(0, 24);      // generic count
                data.WriteBits(0, 22);      // trade count
                data.WriteBit(0);           // no power data
                data.WriteBits(0, 22);      // feed count
                uint32 count = m_effectExecuteData[i]->guids.size();
                data.WriteBits(count, 21);  // extra count
                for (uint32 j = 0; j < count; ++j)
                {
                    data.WriteGuidMask<7, 3, 4, 5, 1, 0, 2, 6>(m_effectExecuteData[i]->guids[j]);

                    buff.WriteGuidBytes<3, 2, 4>(m_effectExecuteData[i]->guids[j]);
                    buff << uint32(m_effectExecuteData[i]->param1[j]);
                    buff.WriteGuidBytes<6, 0, 7, 5, 1>(m_effectExecuteData[i]->guids[j]);
                }
                data.WriteBits(0, 21);      // durability count
                data.WriteBits(0, 20);      // power count

                break;
            }
            case SPELL_EFFECT_DURABILITY_DAMAGE:
            {
                data.WriteBits(0, 24);      // generic count
                data.WriteBits(0, 22);      // trade count
                data.WriteBit(0);           // no power data
                data.WriteBits(0, 22);      // feed count
                data.WriteBits(0, 21);      // extra count
                uint32 count = m_effectExecuteData[i]->guids.size();
                data.WriteBits(count, 21);  // durability count
                for (uint32 j = 0; j < count; ++j)
                {
                    data.WriteGuidMask<5, 0, 7, 6, 1, 3, 2, 4>(m_effectExecuteData[i]->guids[j]);

                    buff.WriteGuidBytes<6>(m_effectExecuteData[i]->guids[j]);
                    buff << uint32(m_effectExecuteData[i]->param2[j]);
                    buff.WriteGuidBytes<4, 5>(m_effectExecuteData[i]->guids[j]);
                    buff << uint32(m_effectExecuteData[i]->param1[j]);
                    buff.WriteGuidBytes<2, 1, 3, 0, 7>(m_effectExecuteData[i]->guids[j]);
                }
                data.WriteBits(0, 20);      // power count
                break;
            }
            case SPELL_EFFECT_RESURRECT:
            case SPELL_EFFECT_SUMMON:
            case SPELL_EFFECT_OPEN_LOCK:
            case SPELL_EFFECT_TRANS_DOOR:
            case SPELL_EFFECT_SUMMON_PET:
            case SPELL_EFFECT_SUMMON_OBJECT_WILD:
            case SPELL_EFFECT_CREATE_HOUSE:
            case SPELL_EFFECT_DUEL:
            case SPELL_EFFECT_DISMISS_PET:
            case SPELL_EFFECT_SUMMON_OBJECT_SLOT:
            case SPELL_EFFECT_OBJECT_WITH_PERSONAL_VISIBILITY:
            case SPELL_EFFECT_RESURRECT_WITH_AURA:
            case SPELL_EFFECT_RESURRECT_NEW:
            {
                uint32 count = m_effectExecuteData[i]->guids.size();
                data.WriteBits(count, 24);  // generic count
                for (uint32 j = 0; j < count; ++j)
                {
                    data.WriteGuidMask<3, 4, 5, 7, 0, 2, 6, 1>(m_effectExecuteData[i]->guids[j]);

                    buff.WriteGuidBytes<3, 5, 1, 0, 4, 7, 6, 2>(m_effectExecuteData[i]->guids[j]);
                }
                data.WriteBits(0, 22);      // trade count
                data.WriteBit(0);           // no power data
                data.WriteBits(0, 22);      // feed count
                data.WriteBits(0, 21);      // extra count
                data.WriteBits(0, 21);      // durability count
                data.WriteBits(0, 20);      // power count
                break;
            }
            case SPELL_EFFECT_CREATE_ITEM:
            case SPELL_EFFECT_CREATE_RANDOM_ITEM:
            case SPELL_EFFECT_CREATE_ITEM_2:
            case SPELL_EFFECT_LOOT_BONUS:
            {
                data.WriteBits(0, 24);      // generic count
                uint32 count = m_effectExecuteData[i]->param1.size();
                data.WriteBits(count, 22);  // trade count
                data.WriteBit(0);           // no power data
                data.WriteBits(0, 22);      // feed count
                data.WriteBits(0, 21);      // extra count
                data.WriteBits(0, 21);      // durability count
                data.WriteBits(0, 20);      // power count

                for (uint32 j = 0; j < count; ++j)
                    buff << uint32(m_effectExecuteData[i]->param1[j]);
                break;
            }
            case SPELL_EFFECT_FEED_PET:
            {
                data.WriteBits(0, 24);      // generic count
                data.WriteBits(0, 22);      // trade count
                data.WriteBit(0);           // no power data
                uint32 count = m_effectExecuteData[i]->param1.size();
                data.WriteBits(count, 22);  // feed count
                data.WriteBits(0, 21);      // extra count
                data.WriteBits(0, 21);      // durability count
                data.WriteBits(0, 20);      // power count

                for (uint32 j = 0; j < count; ++j)
                    buff << uint32(m_effectExecuteData[i]->param1[j]);
                break;
            }
            default:
            {
                sLog->outError(LOG_FILTER_SPELLS_AURAS, "Spell::SendLogExecute: Effect %u should not have data, skipping", effect);
                delete m_effectExecuteData[i];
                m_effectExecuteData[i] = NULL;
                continue;
            }
        }

        ++effectCount;
        buff << uint32(effect);             // spell effect

        delete m_effectExecuteData[i];
        m_effectExecuteData[i] = NULL;
    }

    data.WriteGuidMask<2, 4>(casterGuid);

    if (!buff.empty())
    {
        data.FlushBits();
        data.append(buff);
    }

    data.WriteGuidBytes<4, 6, 0>(casterGuid);
    data << uint32(m_spellInfo->Id);
    data.WriteGuidBytes<2, 5, 1, 3, 7>(casterGuid);

    data.PutBits<uint32>(bitpos, effectCount, 19);

    m_caster->SendMessageToSet(&data, true);
}

void Spell::ExecuteLogEffectGeneric(uint8 effIndex, uint64 guid)
{
    InitEffectExecuteData(effIndex);

    m_effectExecuteData[effIndex]->guids.push_back(guid);
}

void Spell::ExecuteLogEffectPowerDrain(uint8 effIndex, uint64 guid, uint32 powerType, uint32 powerTaken, float gainMultiplier)
{
    InitEffectExecuteData(effIndex);

    m_effectExecuteData[effIndex]->guids.push_back(guid);
    m_effectExecuteData[effIndex]->param1.push_back(powerTaken);    // v may be swapped
    m_effectExecuteData[effIndex]->param2.push_back(powerType);     // ^
    m_effectExecuteData[effIndex]->floatParam.push_back(gainMultiplier);
}

void Spell::ExecuteLogEffectExtraAttacks(uint8 effIndex, uint64 guid, uint32 attCount)
{
    InitEffectExecuteData(effIndex);

    m_effectExecuteData[effIndex]->guids.push_back(guid);
    m_effectExecuteData[effIndex]->param1.push_back(attCount);
}

void Spell::ExecuteLogEffectDurabilityDamage(uint8 effIndex, uint64 guid, uint32 /*itemslot*/, uint32 damage)
{
    InitEffectExecuteData(effIndex);

    m_effectExecuteData[effIndex]->guids.push_back(guid);
    m_effectExecuteData[effIndex]->param1.push_back(damage);
    m_effectExecuteData[effIndex]->param2.push_back(-1);
}

void Spell::ExecuteLogEffectTradeSkillItem(uint8 effIndex, uint32 entry)
{
    InitEffectExecuteData(effIndex);

    m_effectExecuteData[effIndex]->param1.push_back(entry);
}

void Spell::ExecuteLogEffectFeedPet(uint8 effIndex, uint32 entry)
{
    InitEffectExecuteData(effIndex);

    m_effectExecuteData[effIndex]->param1.push_back(entry);
}

void Spell::SendInterrupted(uint8 result)
{
    ObjectGuid guid = m_caster->GetGUID();

    //! 5.4.1
    WorldPacket data(SMSG_SPELL_FAILURE, (8+4+1));
    data << uint32(m_spellInfo->Id);
    data << uint8(m_cast_count);
    data << uint8(result);
    data.WriteGuidMask<6, 1, 3, 7, 0, 4, 2, 5>(guid);
    data.WriteGuidBytes<3, 2, 1, 6, 5, 0, 4, 7>(guid);
    m_caster->SendMessageToSet(&data, true);

    //! 5.4.1
    data.Initialize(SMSG_SPELL_FAILED_OTHER, (8+4));
    data.WriteGuidMask<0, 4, 6, 2, 7, 3, 5, 1>(guid);
    data.WriteGuidBytes<7, 3, 5>(guid);
    data << uint8(m_cast_count);
    data.WriteGuidBytes<4, 1>(guid);
    data << uint32(m_spellInfo->Id);
    data << uint8(result);
    data.WriteGuidBytes<0, 6, 2>(guid);
    m_caster->SendMessageToSet(&data, true);
}

void Spell::SendChannelUpdate(uint32 time)
{
    if (time == 0)
    {
        m_caster->SetUInt64Value(UNIT_FIELD_CHANNEL_OBJECT, 0);
        m_caster->SetUInt32Value(UNIT_CHANNEL_SPELL, 0);
    }

    //! 5.4.1
    WorldPacket data(SMSG_CHANNEL_UPDATE, 8+4);
    ObjectGuid guid = m_caster->GetGUID();
    data.WriteGuidMask<7, 1, 2, 6, 4, 3, 0, 5>(guid);
    data << uint32(time);
    data.WriteGuidBytes<5, 7, 2, 0, 4, 1, 6, 3>(guid);

    m_caster->SendMessageToSet(&data, true);
}

void Spell::SendChannelStart(uint32 duration)
{
    uint64 channelTarget = m_targets.GetObjectTargetGUID();
    uint64 dynObjGuid = GetSpellDynamicObject();
    uint64 channelGuid = m_caster->GetUInt64Value(UNIT_FIELD_CHANNEL_OBJECT);

    if (!channelTarget && !m_spellInfo->NeedsExplicitUnitTarget())
        if (m_UniqueTargetInfo.size() + m_UniqueGOTargetInfo.size() == 1)   // this is for TARGET_SELECT_CATEGORY_NEARBY
            if (!m_UniqueTargetInfo.empty())
                channelTarget = !m_UniqueTargetInfo.empty() ? m_UniqueTargetInfo.front().targetGUID : m_UniqueGOTargetInfo.front().targetGUID;

    /*if(Unit* target = m_targets.GetUnitTarget())
    {
        //sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "SendChannelStart target %u", target->GetGUID());
        if(m_spellInfo->AttributesEx & SPELL_ATTR1_CHANNEL_TRACK_TARGET)
            m_caster->SetFacingTo(m_targets.GetUnitTarget());
    }*/

    //sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "SendChannelStart id %u channelTarget %i dynObjGuid %i channelGuid %i ObjectTargetGUID %i", m_spellInfo->Id, channelTarget, dynObjGuid, channelGuid, m_targets.GetObjectTargetGUID());

    ObjectGuid casterGuid = m_caster->GetObjectGuid();
    WorldPacket data(SMSG_CHANNEL_START, (8+4+4));
    data.WriteGuidMask<7, 5, 2>(casterGuid);
    data.WriteBit(0);                   // not has heal prediction
    data.WriteGuidMask<6>(casterGuid);

    data.WriteGuidMask<3, 4, 0, 1>(casterGuid);
    data.WriteBit(0);                   // not has cast immunities

    data.WriteGuidBytes<0>(casterGuid);
    data.WriteGuidBytes<6>(casterGuid);
    data << uint32(m_spellInfo->Id);
    data.WriteGuidBytes<5, 1>(casterGuid);
    data << uint32(duration);
    data.WriteGuidBytes<3, 7, 2>(casterGuid);
    data.WriteGuidBytes<4>(casterGuid);

    m_caster->SendMessageToSet(&data, true);

    m_timer = duration;
    if(!channelGuid)
    {
        if(dynObjGuid)
            m_caster->SetUInt64Value(UNIT_FIELD_CHANNEL_OBJECT, dynObjGuid);
        else if (channelTarget)
            m_caster->SetUInt64Value(UNIT_FIELD_CHANNEL_OBJECT, channelTarget);
    }

    if (m_spellInfo->Id != 101546)
        m_caster->SetUInt32Value(UNIT_CHANNEL_SPELL, m_spellInfo->Id);
}

void Spell::SendResurrectRequest(Player* target)
{
    // get ressurector name for creature resurrections, otherwise packet will be not accepted
    // for player resurrections the name is looked up by guid
    std::string resurrectorName = m_caster->GetTypeId() == TYPEID_PLAYER ? "" : m_caster->GetNameForLocaleIdx(target->GetSession()->GetSessionDbLocaleIndex());

    ObjectGuid guid = m_caster->GetObjectGuid();
    WorldPacket data(SMSG_RESURRECT_REQUEST, 8 + 1 + 1 + 4 + 4 + 4 + resurrectorName.size());
    data.WriteBits(resurrectorName.size(), 6);
    data.WriteBit(m_caster->GetTypeId() == TYPEID_PLAYER ? 0 : 1);          // "you'll be afflicted with resurrection sickness"
                                                                            // override delay sent with SMSG_CORPSE_RECLAIM_DELAY, set instant resurrection for spells with this attribute
    data.WriteBit(0);                                                       // use timer according to client symbols
    data.WriteGuidMask<7, 2, 4, 5, 3, 0, 6, 1>(guid);
    data.WriteGuidBytes<4>(guid);
    data << uint32(realmID);
    data.WriteGuidBytes<0>(guid);
    data << uint32(0);                                                      // pet counter?
    data.WriteGuidBytes<3>(guid);
    // 4.2.2 edit : id of the spell used to resurect. (used client-side for Mass Resurect)
    data << uint32(m_spellInfo->Id);
    data.WriteGuidBytes<6, 2, 5>(guid);
    data.WriteString(resurrectorName);
    data.WriteGuidBytes<1, 7>(guid);

    target->GetSession()->SendPacket(&data);
}

void Spell::TakeCastItem()
{
    // not remove cast item at triggered spell (equipping, weapon damage, etc)
    if (_triggeredCastFlags & TRIGGERED_IGNORE_CAST_ITEM)
        return;

    // need find better way to handle this
    // after item swap m_castItem will be null so it cause exploits
    if (!m_CastItem && m_castItemGUID && m_itemEntry)
    {
        if (auto player = m_caster->ToPlayer())
        {
            auto count = 1;
            player->DestroyItemCount(m_itemEntry, count, true);
            return;
        }
    }

    if (!m_CastItem || m_caster->GetTypeId() != TYPEID_PLAYER)
        return;

    auto proto = m_CastItem->GetTemplate();
    if (!proto)
    {
        // This code is to avoid a crash
        // I'm not sure, if this is really an error, but I guess every item needs a prototype
        sLog->outError(LOG_FILTER_SPELLS_AURAS, "Cast item has no item prototype highId=%d, lowId=%d", m_CastItem->GetGUIDHigh(), m_CastItem->GetGUIDLow());
        return;
    }

    bool expendable = false;
    bool withoutCharges = false;

    for (int i = 0; i < MAX_ITEM_PROTO_SPELLS; ++i)
    {
        if (proto->Spells[i].SpellId)
        {
            // item has limited charges
            if (proto->Spells[i].SpellCharges)
            {
                if (proto->Spells[i].SpellCharges < 0)
                    expendable = true;

                int32 charges = m_CastItem->GetSpellCharges(i);

                // item has charges left
                if (charges)
                {
                    (charges > 0) ? --charges : ++charges;  // abs(charges) less at 1 after use
                    if (proto->Stackable == 1)
                        m_CastItem->SetSpellCharges(i, charges);
                    m_CastItem->SetState(ITEM_CHANGED, (Player*)m_caster);
                }

                // all charges used
                withoutCharges = (charges == 0);
            }
        }
    }

    if (expendable && withoutCharges)
    {
        uint32 count = 1;
        m_caster->ToPlayer()->DestroyItemCount(m_CastItem, count, true);

        // prevent crash at access to deleted m_targets.GetItemTarget
        if (m_CastItem == m_targets.GetItemTarget())
            m_targets.SetItemTarget(NULL);

        m_CastItem = NULL;
        m_itemEntry = 0;
    }
}

void Spell::TakePower()
{
    if (m_CastItem || m_triggeredByAuraSpell)
        return;
    //Don't take power if the spell is cast while .cheat power is enabled.
    if (m_caster->GetTypeId() == TYPEID_PLAYER)
    {
        if (m_caster->ToPlayer()->GetCommandStatus(CHEAT_POWER))
            return;
    }

    SpellPowerEntry power;
    if (!GetSpellInfo()->GetSpellPowerByCasterPower(m_caster, power))
        return;

    Powers powerType = Powers(power.powerType);
    int32  ifMissedPowerCost = m_powerCost;
    bool hit = true;

    if (m_caster->GetTypeId() == TYPEID_PLAYER)
    {
        if (uint64 targetGUID = m_targets.GetUnitTargetGUID())
            for (std::list<TargetInfo>::iterator ihit= m_UniqueTargetInfo.begin(); ihit != m_UniqueTargetInfo.end(); ++ihit)
                if (ihit->targetGUID == targetGUID)
                {
                    if (ihit->missCondition != SPELL_MISS_NONE && ihit->missCondition != SPELL_MISS_IMMUNE)
                    {
                        switch (powerType)
                        {
                        case POWER_CHI:
                        case POWER_HOLY_POWER:
                        case POWER_SHADOW_ORB:
                        case POWER_SOUL_SHARDS:
                            ifMissedPowerCost = 0;
                            break;
                        case POWER_ENERGY:
                        case POWER_RAGE:
                        case POWER_RUNES:
                            ifMissedPowerCost = CalculatePct(m_powerCost, 20);
                            break;
                        default:
                            break;
                        }
                        hit = false;
                        //lower spell cost on fail (by talent aura)
                        if (Player* modOwner = m_caster->ToPlayer()->GetSpellModOwner())
                            modOwner->ApplySpellMod(m_spellInfo->Id, SPELLMOD_SPELL_COST_REFUND_ON_FAIL, m_powerCost);
                    }
                    break;
                }
    }

    if (powerType == POWER_CHI)
    {
        if (!m_UniqueTargetInfo.empty())
            if (Aura* aur = m_caster->GetAura(123980))
            {
                uint8 chipower = m_spellInfo->PowerCost;
                uint8 maxamount = aur->GetSpellInfo()->Effects[EFFECT_0].BasePoints;
                int8 amount = aur->GetEffect(EFFECT_0)->GetAmount();
                amount -= chipower;

                if (amount <= 0)
                {
                    m_caster->CastSpell(m_caster, 125195, true);
                    amount += maxamount;
                }
                aur->GetEffect(EFFECT_0)->SetAmount(amount);
            }
    }
    if (powerType == POWER_RUNES)
    {
        TakeRunePower(hit);
        return;
    }

    CallScriptTakePowerHandlers(powerType, m_powerCost);

    sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "Spell::TakePower hit %i, powerType %i, m_powerCost %i", hit, powerType, m_powerCost);

    if (!m_powerCost)
        return;

    // health as power used
    if (powerType == POWER_HEALTH)
    {
        m_caster->ModifyHealth(-(int32)m_powerCost);
        return;
    }

    if (powerType >= MAX_POWERS)
    {
        sLog->outError(LOG_FILTER_SPELLS_AURAS, "Spell::TakePower: Unknown power type '%d'", powerType);
        return;
    }

    if (hit)
        m_caster->ModifyPower(powerType, -m_powerCost);
    else
        m_caster->ModifyPower(powerType, -ifMissedPowerCost);
}

void Spell::TakeAmmo()
{
    if (m_attackType == RANGED_ATTACK && m_caster->GetTypeId() == TYPEID_PLAYER)
    {
        Item* pItem = m_caster->ToPlayer()->GetWeaponForAttack(RANGED_ATTACK);

        // wands don't have ammo
        if (!pItem  || pItem->IsBroken() || pItem->GetTemplate()->SubClass == ITEM_SUBCLASS_WEAPON_WAND)
            return;

        if (pItem->GetTemplate()->InventoryType == INVTYPE_THROWN)
        {
            if (pItem->GetMaxStackCount() == 1)
            {
                // decrease durability for non-stackable throw weapon
                m_caster->ToPlayer()->DurabilityPointLossForEquipSlot(EQUIPMENT_SLOT_MAINHAND);
            }
            else
            {
                // decrease items amount for stackable throw weapon
                uint32 count = 1;
                m_caster->ToPlayer()->DestroyItemCount(pItem, count, true);
            }
        }
    }
}

SpellCastResult Spell::CheckRuneCost(uint32 runeCostID)
{
    if (!GetSpellInfo()->HasPower(POWER_RUNES) || !runeCostID)
        return SPELL_CAST_OK;

    if (m_caster->GetTypeId() != TYPEID_PLAYER)
        return SPELL_CAST_OK;

    Player* player = (Player*)m_caster;

    if (player->getClass() != CLASS_DEATH_KNIGHT)
        return SPELL_CAST_OK;

    SpellRuneCostEntry const* src = sSpellRuneCostStore.LookupEntry(runeCostID);

    if (!src)
        return SPELL_CAST_OK;

    if (src->NoRuneCost())
        return SPELL_CAST_OK;

    int32 runeCost[NUM_RUNE_TYPES];                         // blood, frost, unholy, death

    for (uint32 i = 0; i < NUM_RUNE_TYPES; ++i)
    {
        runeCost[i] = src->RuneCost[i];
        if (Player* modOwner = m_caster->GetSpellModOwner())
            modOwner->ApplySpellMod(m_spellInfo->Id, SPELLMOD_COST, runeCost[i], this);
    }

    for (uint32 i = 0; i < MAX_RUNES; ++i)
    {
        RuneType rune = player->GetCurrentRune(i);
        if ((player->GetRuneCooldown(i) == 0) && (runeCost[rune] > 0))
            runeCost[rune]--;
    }

    for (uint32 i = 0; i < RUNE_DEATH; ++i)
    {
        if (runeCost[i] > 0)
            runeCost[RUNE_DEATH] += runeCost[i];
    }

    if (runeCost[RUNE_DEATH] > MAX_RUNES)
        return SPELL_FAILED_NO_POWER;

    return SPELL_CAST_OK;
}

void Spell::TakeRunePower(bool didHit)
{
    if (m_caster->GetTypeId() != TYPEID_PLAYER || m_caster->getClass() != CLASS_DEATH_KNIGHT)
        return;

    SpellRuneCostEntry const* runeCostData = sSpellRuneCostStore.LookupEntry(m_spellInfo->RuneCostID);
    if (!runeCostData || (runeCostData->NoRuneCost() && runeCostData->NoRunicPowerGain()))
        return;

    Player* player = m_caster->ToPlayer();
    m_runesState = player->GetRunesState();                 // store previous state

    int32 runeCost[NUM_RUNE_TYPES];                         // blood, frost, unholy, death

    for (uint32 i = 0; i < NUM_RUNE_TYPES; ++i)
    {
        runeCost[i] = runeCostData->RuneCost[i];
        if (Player* modOwner = m_caster->GetSpellModOwner())
        {
            modOwner->ApplySpellMod(m_spellInfo->Id, SPELLMOD_COST, runeCost[i], this);

            if (runeCost[i] < 0)
                runeCost[i] = 0;
        }
    }

    bool canUse[MAX_RUNES];
    bool isDeathRunes[MAX_RUNES];

    for (uint8 i = 0; i < MAX_RUNES; ++i)
    {
        canUse[i] = false;
        isDeathRunes[i] = false;
        player->SetLastRuneUsed(i, false);
        RuneType rune = player->GetCurrentRune(i);
        RuneType baseRune = player->GetBaseRune(i);

        isDeathRunes[i] = rune == RUNE_DEATH ? true : false;

        if (player->GetRuneCooldown(i) || !runeCost[baseRune])
        {
            canUse[i] = false;
            continue;
        }
        else
        {
            if (!isDeathRunes[i])
            {
                runeCost[baseRune] = 0;
                player->SetLastRuneUsed(i, true);
            }

            canUse[i] = true;
        }
    }

    uint8 _runesCost = 0;

    for (uint8 i = 0; i < NUM_RUNE_TYPES; ++i)
        _runesCost += runeCost[i];

    if (_runesCost)
    {
        for (uint8 i = 0; i < MAX_RUNES; ++i)
        {
            RuneType baseRune = player->GetBaseRune(i);

            if (canUse[i] && isDeathRunes[i] && runeCost[baseRune])
            {
                runeCost[baseRune] = 0;
                player->SetLastRuneUsed(i, true);
                isDeathRunes[i] = false;
            }
        }
        _runesCost = 0;

        for (uint8 i = 0; i < NUM_RUNE_TYPES; ++i)
            _runesCost += runeCost[i];

        if (_runesCost)
            for (uint8 i = 0; i < MAX_RUNES; ++i)
            {
                if (isDeathRunes[i] && !player->GetRuneCooldown(i) && _runesCost)
                {
                    player->SetLastRuneUsed(i, true);
                    isDeathRunes[i] = false;
                    _runesCost--;
                }
            }
    }

    for (uint8 i = 0; i < MAX_RUNES; ++i)
        if (player->IsLastRuneUsed(i))
        {
            uint32 cooldown = ((m_spellInfo->SpellFamilyFlags[0] & SPELLFAMILYFLAG_DK_DEATH_STRIKE) > 0 || didHit) ? RUNE_BASE_COOLDOWN : uint32(RUNE_MISS_COOLDOWN);
            player->SetRuneCooldown(i, cooldown);
            if (!player->IsBlockedRuneConvert(i))
                player->SetConvertIn(i, player->GetBaseRune(i));
        }

    // you can gain some runic power when use runes
    if (didHit)
        if (int32 rp = int32(runeCostData->runePowerGain * sWorld->getRate(RATE_POWER_RUNICPOWER_INCOME)))
        {
            Unit::AuraEffectList const& ModRuneRegen = player->GetAuraEffectsByType(SPELL_AURA_MOD_RUNE_REGEN_SPEED);
            for (Unit::AuraEffectList::const_iterator i = ModRuneRegen.begin(); i != ModRuneRegen.end(); ++i)
                AddPct(rp, (*i)->GetAmount());

            player->ModifyPower(POWER_RUNIC_POWER, int32(rp));
        }
}

void Spell::LinkedSpell(Unit* _caster, Unit* _target, SpellLinkedType type)
{
    if (const std::vector<SpellLinked> *spell_triggered = sSpellMgr->GetSpellLinked(m_spellInfo->Id + type))
    {
        std::set<uint32> spellBanList;
        std::set<uint32> groupLock;
        for (std::vector<SpellLinked>::const_iterator i = spell_triggered->begin(); i != spell_triggered->end(); ++i)
        {
            if (i->hitmask)
                if (!(m_spellMissMask & i->hitmask))
                    continue;

            if (!spellBanList.empty())
            {
                std::set<uint32>::iterator itr = spellBanList.find(abs(i->effect));
                if (itr != spellBanList.end())
                    continue;
            }

            if(i->group != 0 && !groupLock.empty())
            {
                std::set<uint32>::iterator itr = groupLock.find(i->group);
                if (itr != groupLock.end())
                    continue;
            }

            if(!_target)
                _target = m_targets.GetUnitTarget();
            _caster = m_caster;

            if (i->target)
                _target = (m_originalCaster ? m_originalCaster : m_caster)->GetUnitForLinkedSpell(_caster, _target, i->target, abs(i->effect));

            if (i->caster)
                _caster = (m_originalCaster ? m_originalCaster : m_caster)->GetUnitForLinkedSpell(_caster, _target, i->caster, abs(i->effect));

            if(!_caster)
                continue;

            if(i->targetCount != -1)
            {
                switch (i->targetCountType)
                {
                    case LINK_TARGET_DEFAULT:
                    {
                        if(GetTargetCount() < i->targetCount)
                            continue;
                        break;
                    }
                    case LINK_TARGET_FROM_EFFECT:
                    {
                        if(GetEffectTargets().size() < i->targetCount)
                            continue;
                        break;
                    }
                }
            }

            if(i->hastalent)
                if(m_caster->HasAuraLinkedSpell(_caster, _target, i->hastype, i->hastalent))
                    continue;

            if (i->hastalent2)
                if(m_caster->HasAuraLinkedSpell(_caster, _target, i->hastype2, i->hastalent2))
                    continue;

            if (i->effect < 0)
            {
                switch (i->actiontype)
                {
                    case LINK_ACTION_DEFAULT:
                        _caster->RemoveAurasDueToSpell(abs(i->effect));
                        break;
                    case LINK_ACTION_LEARN:
                    {
                        if(Player* _lplayer = _caster->ToPlayer())
                            _lplayer->removeSpell(abs(i->effect));
                        break;
                    }
                    case LINK_ACTION_AURATYPE:
                        if(_target)
                            _target->RemoveAurasByType(AuraType(abs(i->effect)), 0, _target->GetAura(i->chance));
                        break;
                    case LINK_ACTION_CHANGE_STACK:
                        if (Aura* aura = (_target ? _target : _caster)->GetAura(abs(i->effect)))
                            aura->ModStackAmount(-1);
                        break;
                }
            }
            else
            {
                if(i->chance != 0 && !roll_chance_i(i->chance))
                    continue;
                if(i->cooldown != 0 && _caster->GetTypeId() == TYPEID_PLAYER && _caster->ToPlayer()->HasSpellCooldown(i->effect))
                    continue;

                switch (i->actiontype)
                {
                    case LINK_ACTION_DEFAULT: //0
                    {
                        _caster->CastSpell(_target ? _target : _caster, i->effect, true);
                        spellBanList.insert(i->effect); // Triggered once for a cycle
                        break;
                    }
                    case LINK_ACTION_LEARN: //1
                    {
                        if (Player* _lplayer = _caster->ToPlayer())
                            _lplayer->learnSpell(i->effect, false);
                        break;
                    }
                    case LINK_ACTION_SEND_COOLDOWN: // 3
                        _caster->SendSpellCooldown(i->effect, m_spellInfo->Id);
                        break;
                    case LINK_ACTION_CAST_NO_TRIGGER: //4
                        _caster->CastSpell(_target ? _target : _caster, i->effect, false);
                        break;
                    case LINK_ACTION_ADD_AURA: //5
                        _caster->AddAura(i->effect, _target ? _target : _caster);
                        break;
                    case LINK_ACTION_CHANGE_STACK: //7
                        if (Aura* aura = (_target ? _target : _caster)->GetAura(i->effect))
                            aura->ModStackAmount(1);
                        else
                            _caster->CastSpell(_target ? _target : _caster, i->effect, true);
                        break;
                    case LINK_ACTION_REMOVE_COOLDOWN: //8
                        if (Player* _lplayer = _caster->ToPlayer())
                            _lplayer->RemoveSpellCooldown(i->effect, true);
                        break;
                    case LINK_ACTION_REMOVE_MOVEMENT: //9
                        (_target ? _target : _caster)->RemoveMovementImpairingAuras();
                        break;
                    case LINK_ACTION_CHANGE_DURATION: //10
                    {
                        if(Aura* aura = (_target ? _target : _caster)->GetAura(i->effect, _caster->GetGUID()))
                        {
                            if (!i->duration)
                                aura->RefreshTimers();
                            else
                            {
                                int32 _duration = int32(aura->GetDuration() + i->duration);
                                if (_duration < aura->GetMaxDuration())
                                    aura->SetDuration(_duration, true);
                            }
                        }
                        else
                            _caster->CastSpell(_target ? _target : _caster, i->effect, true);
                        break;
                    }
                    case LINK_UNIT_TYPE_CAST_DEST: //11
                    {
                        if (m_targets.HasDst())
                        {
                            Position pos = *m_targets.GetDstPos();
                            _caster->CastSpell(pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), i->effect, true);
                        }
                        break;
                    }
                }

                if(i->group != 0)
                    groupLock.insert(i->group);

                if(i->cooldown != 0 && _caster->GetTypeId() == TYPEID_PLAYER)
                    _caster->ToPlayer()->AddSpellCooldown(i->effect, 0, getPreciseTime() + (double)i->cooldown);
            }
        }
    }
}

void Spell::TakeReagents()
{
    if (m_caster->GetTypeId() != TYPEID_PLAYER)
        return;

    ItemTemplate const* castItemTemplate = m_CastItem ? m_CastItem->GetTemplate() : NULL;

    // do not take reagents for these item casts
    if (castItemTemplate && castItemTemplate->Flags & ITEM_PROTO_FLAG_TRIGGERED_CAST)
        return;

    Player* p_caster = m_caster->ToPlayer();
    if (p_caster->CanNoReagentCast(m_spellInfo))
        return;

    for (uint32 x = 0; x < MAX_SPELL_REAGENTS; ++x)
    {
        if (m_spellInfo->Reagent[x] <= 0)
            continue;

        uint32 itemid = m_spellInfo->Reagent[x];
        uint32 itemcount = m_spellInfo->ReagentCount[x];

        // if CastItem is also spell reagent
        if (castItemTemplate && castItemTemplate->ItemId == itemid)
        {
            for (int s = 0; s < MAX_ITEM_PROTO_SPELLS; ++s)
            {
                // CastItem will be used up and does not count as reagent
                int32 charges = m_CastItem->GetSpellCharges(s);
                if (castItemTemplate->Spells[s].SpellCharges < 0 && abs(charges) < 2)
                {
                    ++itemcount;
                    break;
                }
            }

            m_CastItem = NULL;
            m_itemEntry = 0;
        }

        // if GetItemTarget is also spell reagent
        if (m_targets.GetItemTargetEntry() == itemid)
            m_targets.SetItemTarget(NULL);

        p_caster->DestroyItemCount(itemid, itemcount, true);
    }
    if(m_spellInfo->ReagentCurrency != 0)
        p_caster->ModifyCurrency(m_spellInfo->ReagentCurrency, -int32(m_spellInfo->ReagentCurrencyCount));
}

void Spell::HandleThreatSpells()
{
    if (m_UniqueTargetInfo.empty())
        return;

    if ((AttributesCustomEx  & SPELL_ATTR1_NO_THREAT) ||
        (AttributesCustomEx3 & SPELL_ATTR3_NO_INITIAL_AGGRO))
        return;

    float threat = 0.0f;
    if (SpellThreatEntry const* threatEntry = sSpellMgr->GetSpellThreatEntry(m_spellInfo->Id))
    {
        if (threatEntry->apPctMod != 0.0f)
            threat += threatEntry->apPctMod * m_caster->GetTotalAttackPowerValue(BASE_ATTACK);

        threat += threatEntry->flatMod;
    }
    else if ((AttributesCustomCu & SPELL_ATTR0_CU_NO_INITIAL_THREAT) == 0)
        threat += m_spellInfo->SpellLevel;

    // past this point only multiplicative effects occur
    if (threat == 0.0f)
        return;

    // since 2.0.1 threat from positive effects also is distributed among all targets, so the overall caused threat is at most the defined bonus
    threat /= m_UniqueTargetInfo.size();

    for (std::list<TargetInfo>::iterator ihit = m_UniqueTargetInfo.begin(); ihit != m_UniqueTargetInfo.end(); ++ihit)
    {
        if (ihit->missCondition != SPELL_MISS_NONE)
            continue;

        Unit* target = ObjectAccessor::GetUnit(*m_caster, ihit->targetGUID);
        if (!target)
            continue;

        // positive spells distribute threat among all units that are in combat with target, like healing
        if (m_spellInfo->_IsPositiveSpell())
            target->getHostileRefManager().threatAssist(m_caster, threat, m_spellInfo);
        // for negative spells threat gets distributed among affected targets
        else
        {
            if (!target->CanHaveThreatList())
                continue;

            target->AddThreat(m_caster, threat, m_spellInfo->GetSchoolMask(), m_spellInfo);
        }
    }
    sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "Spell %u, added an additional %f threat for %s %u target(s)", m_spellInfo->Id, threat, m_spellInfo->_IsPositiveSpell() ? "assisting" : "harming", uint32(m_UniqueTargetInfo.size()));
}

void Spell::HandleEffects(Unit* pUnitTarget, Item* pItemTarget, GameObject* pGOTarget, uint32 i, SpellEffectHandleMode mode)
{
    effectHandleMode = mode;
    unitTarget = pUnitTarget;
    itemTarget = pItemTarget;
    gameObjTarget = pGOTarget;
    destTarget = &m_destTargets[i]._position;

    uint16 eff = m_spellInfo->GetEffect(i, m_diffMode)->Effect;

    if(!damageCalculate[i])
    {
        damage = CalculateDamage(i, unitTarget);
        saveDamageCalculate[i] = damage;
        damageCalculate[i] = true;
    }
    else
        damage = saveDamageCalculate[i];

    #ifdef WIN32
    sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "Spell: %u Effect : %u, damage %i, mode %i", m_spellInfo->Id, eff, damage, mode);
    #endif

    bool preventDefault = CallScriptEffectHandlers((SpellEffIndex)i, mode);
    if(!preventDefault)
        preventDefault = CheckEffFromDummy(unitTarget, i);

    if (!preventDefault && eff < TOTAL_SPELL_EFFECTS)
    {
        m_currentExecutedEffect = eff;
        (this->*SpellEffects[eff])((SpellEffIndex)i);
        m_currentExecutedEffect = SPELL_EFFECT_NONE;
    }
}

bool Spell::CheckEffFromDummy(Unit* /*target*/, uint32 eff)
{
    bool prevent = false;
    if (std::vector<SpellAuraDummy> const* spellAuraDummy = sSpellMgr->GetSpellAuraDummy(m_spellInfo->Id))
    {
        for (std::vector<SpellAuraDummy>::const_iterator itr = spellAuraDummy->begin(); itr != spellAuraDummy->end(); ++itr)
        {
            Unit* _caster = m_caster;
            Unit* _targetAura = m_caster;
            bool check = false;

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
                case SPELL_DUMMY_MOD_EFFECT_MASK: //4
                {
                    if(itr->aura > 0 && !_targetAura->HasAura(itr->aura))
                        continue;
                    if(itr->aura < 0 && _targetAura->HasAura(abs(itr->aura)))
                        continue;

                    if(itr->spellDummyId > 0 && !_targetAura->HasAura(itr->spellDummyId))
                    {
                        if(itr->effectmask & (1 << eff))
                            prevent = true;
                        check = true;
                    }
                    if(itr->spellDummyId < 0 && _targetAura->HasAura(abs(itr->spellDummyId)))
                    {
                        if(itr->effectmask & (1 << eff))
                            prevent = true;
                        check = true;
                    }
                    break;
                }
            }
            if(check && itr->removeAura)
                _caster->RemoveAurasDueToSpell(itr->removeAura);
        }
    }

    //sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "Spell::CheckEffFromDummy: eff %i, prevent %i, Id %u", eff, prevent, m_spellInfo->Id);

    return prevent;
}

SpellCastResult Spell::CheckCast(bool strict)
{
    //Massive Ressurection(guild ability) : Can not use this ability then encounter in progress!!
    if (m_spellInfo->Id == 83968)
        if (m_caster->GetMap()->IsDungeon())
            if (InstanceScript* instance = m_caster->GetInstanceScript())
                if (instance->IsEncounterInProgress())
                    return SPELL_FAILED_AFFECTING_COMBAT;

    // Gloves S12 - Druid
    if (m_spellInfo->Id == 33830 && m_caster->HasAura(33830))
        return SPELL_FAILED_DONT_REPORT;

    // Gloves S12 - Shaman
    if (m_spellInfo->Id == 32973 && m_caster->HasAura(32973))
        return SPELL_FAILED_DONT_REPORT;

    // check death state
    if (!m_caster->isAlive() && !(AttributesCustom & SPELL_ATTR0_PASSIVE) && !((AttributesCustom & SPELL_ATTR0_CASTABLE_WHILE_DEAD) || (IsTriggered() && !m_triggeredByAuraSpell)))
        return SPELL_FAILED_CASTER_DEAD;

    if (m_spellInfo->Effects[EFFECT_0].Effect == SPELL_EFFECT_UNLEARN_TALENT)
    {
        if (Player* plr = m_caster->ToPlayer())
        {
            PlayerTalentMap* Talents = plr->GetTalentMap(plr->GetActiveSpec());
            for (PlayerTalentMap::iterator itr = Talents->begin(); itr != Talents->end(); ++itr)
            {
                SpellInfo const* spell = sSpellMgr->GetSpellInfo(itr->first);
                if (!spell)
                    continue;

                if (spell->talentId != m_glyphIndex)
                    continue;

                if (plr->HasSpellCooldown(spell->Id))
                    return SPELL_FAILED_CANT_DO_THAT_RIGHT_NOW;
                else
                    break;
            }
        }
    }

    // check cooldowns to prevent cheating
    if (m_caster->GetTypeId() == TYPEID_PLAYER && !(AttributesCustom & SPELL_ATTR0_PASSIVE))
    {
        //can cast triggered (by aura only?) spells while have this flag
        if (!(_triggeredCastFlags & TRIGGERED_IGNORE_CASTER_AURASTATE) && m_caster->HasFlag(PLAYER_FLAGS, PLAYER_ALLOW_ONLY_ABILITY))
        {
            bool allow = false;
            Unit::AuraEffectList const& auras = m_caster->GetAuraEffectsByType(SPELL_AURA_ALLOW_ONLY_ABILITY);
            for (Unit::AuraEffectList::const_iterator itr = auras.begin(); itr != auras.end(); ++itr)
            {
                if ((*itr)->IsAffectingSpell(m_spellInfo))
                {
                    allow = true;
                    break;
                }
            }

            if (!allow)
                return SPELL_FAILED_SPELL_IN_PROGRESS;
        }

        Player* playerCaster = m_caster->ToPlayer();
        if (playerCaster->HasSpellCooldown(m_spellInfo->Id))
        {
            if (m_triggeredByAuraSpell)
                return SPELL_FAILED_DONT_REPORT;
            else if(!m_caster->HasAuraTypeWithAffectMask(SPELL_AURA_IGNORE_CD, m_spellInfo))
                return SPELL_FAILED_NOT_READY;
        }

        if (!playerCaster->HasChargesForSpell(m_spellInfo))
            return m_triggeredByAuraSpell ? SPELL_FAILED_DONT_REPORT : SPELL_FAILED_NO_CHARGES_REMAIN;
    }

    if (AttributesCustomEx7 & SPELL_ATTR7_IS_CHEAT_SPELL && !m_caster->HasFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_ALLOW_CHEAT_SPELLS))
    {
        m_customError = SPELL_CUSTOM_ERROR_GM_ONLY;
        return SPELL_FAILED_CUSTOM_ERROR;
    }

    if (AttributesCustomCu & SPELL_ATTR0_CU_CANT_CAST_ROOTED && m_caster->HasUnitState(UNIT_STATE_ROOT))
    {
        if (m_caster->GetTypeId() == TYPEID_PLAYER)
            return SPELL_FAILED_ROOTED;
        else
            return SPELL_FAILED_DONT_REPORT;
    }

    // Check global cooldown
    if (strict && !(_triggeredCastFlags & TRIGGERED_IGNORE_GCD) && HasGlobalCooldown())
        if(!m_caster->HasAuraTypeWithAffectMask(SPELL_AURA_IGNORE_CD, m_spellInfo))
            return SPELL_FAILED_NOT_READY;

    // only triggered spells can be processed an ended battleground
    if (!IsTriggered() && m_caster->GetTypeId() == TYPEID_PLAYER)
        if (Battleground* bg = m_caster->ToPlayer()->GetBattleground())
            if (bg->GetStatus() == STATUS_WAIT_LEAVE)
                return SPELL_FAILED_DONT_REPORT;

    if (m_caster->GetTypeId() == TYPEID_PLAYER && VMAP::VMapFactory::createOrGetVMapManager()->isLineOfSightCalcEnabled())
    {
        if (AttributesCustom & SPELL_ATTR0_OUTDOORS_ONLY && !m_caster->vmapData->IsOutdoor())
            return SPELL_FAILED_ONLY_OUTDOORS;

        if (AttributesCustom & SPELL_ATTR0_INDOORS_ONLY && m_caster->vmapData->IsOutdoor())
            return SPELL_FAILED_ONLY_INDOORS;
    }

    // only check at first call, Stealth auras are already removed at second call
    // for now, ignore triggered spells
    if (strict && !(_triggeredCastFlags & TRIGGERED_IGNORE_SHAPESHIFT))
    {
        bool checkForm = true;
        // Ignore form req aura
        Unit::AuraEffectList const& ignore = m_caster->GetAuraEffectsByType(SPELL_AURA_MOD_IGNORE_SHAPESHIFT);
        for (Unit::AuraEffectList::const_iterator i = ignore.begin(); i != ignore.end(); ++i)
        {
            if (!(*i)->IsAffectingSpell(m_spellInfo))
                continue;
            checkForm = false;

            // Heart of the Wild (non-restoration buff)
            if ((*i)->GetId() == 108293)
            {
                if (Player* player = m_caster->ToPlayer())
                    // only Guardian spec is allowed to cast Rejuvenation in any form
                    if (player->GetSpecializationId(player->GetActiveSpec()) != SPEC_DRUID_BEAR)
                        checkForm = true;
            }
            break;
        }
        if (checkForm)
        {
            // Cannot be used in this stance/form
            SpellCastResult shapeError = m_spellInfo->CheckShapeshift(m_caster->GetShapeshiftForm());
            if (shapeError != SPELL_CAST_OK)
                return shapeError;

            if ((AttributesCustom & SPELL_ATTR0_ONLY_STEALTHED) && !(m_caster->HasStealthAura()))
                return SPELL_FAILED_ONLY_STEALTHED;
        }
    }

    Unit::AuraEffectList const& blockSpells = m_caster->GetAuraEffectsByType(SPELL_AURA_BLOCK_SPELL_FAMILY);
    for (Unit::AuraEffectList::const_iterator blockItr = blockSpells.begin(); blockItr != blockSpells.end(); ++blockItr)
        if (uint32((*blockItr)->GetMiscValue()) == m_spellInfo->SpellFamilyName && (*blockItr)->GetSpellInfo()->Effects[(*blockItr)->GetEffIndex()].TriggerSpell != m_spellInfo->Id)
            return SPELL_FAILED_SPELL_UNAVAILABLE;

    bool reqCombat = true;
    Unit::AuraEffectList const& stateAuras = m_caster->GetAuraEffectsByType(SPELL_AURA_ABILITY_IGNORE_AURASTATE);
    for (Unit::AuraEffectList::const_iterator j = stateAuras.begin(); j != stateAuras.end(); ++j)
    {
        if ((*j)->IsAffectingSpell(m_spellInfo))
        {
            m_needComboPoints = false;
            if ((*j)->GetMiscValue() == 1)
            {
                reqCombat=false;
                break;
            }
        }
    }

    // caster state requirements
    // not for triggered spells (needed by execute)
    if (!(_triggeredCastFlags & TRIGGERED_IGNORE_CASTER_AURASTATE))
    {
        if (m_spellInfo->CasterAuraState && !m_caster->HasAuraState(AuraStateType(m_spellInfo->CasterAuraState), m_spellInfo, m_caster))
            return SPELL_FAILED_CASTER_AURASTATE;
        if (m_spellInfo->CasterAuraStateNot && m_caster->HasAuraState(AuraStateType(m_spellInfo->CasterAuraStateNot), m_spellInfo, m_caster))
            return SPELL_FAILED_CASTER_AURASTATE;

        // Note: spell 62473 requres casterAuraSpell = triggering spell
        if (!((m_spellInfo->Id == 48020 || m_spellInfo->Id == 114794 || m_spellInfo->Id == 104136) && m_spellInfo->CasterAuraSpell == 62388) && m_spellInfo->CasterAuraSpell && !m_caster->HasAura(sSpellMgr->GetSpellIdForDifficulty(m_spellInfo->CasterAuraSpell, m_caster)))
            return SPELL_FAILED_CASTER_AURASTATE;
        if (m_spellInfo->ExcludeCasterAuraSpell && m_caster->HasAura(sSpellMgr->GetSpellIdForDifficulty(m_spellInfo->ExcludeCasterAuraSpell, m_caster)))
            return SPELL_FAILED_CASTER_AURASTATE;

        if (reqCombat && m_caster->isInCombat() && !m_spellInfo->CanBeUsedInCombat())
            return SPELL_FAILED_AFFECTING_COMBAT;
    }

    // cancel autorepeat spells if cast start when moving
    // (not wand currently autorepeat cast delayed to moving stop anyway in spell update code)
    // Do not cancel spells which are affected by a SPELL_AURA_CAST_WHILE_WALKING effect
    if (m_caster->GetTypeId() == TYPEID_PLAYER && m_caster->ToPlayer()->isMoving() && !m_caster->HasAuraCastWhileWalking(m_spellInfo))
    {
        // skip stuck spell to allow use it in falling case and apply spell limitations at movement
        if ((!m_caster->HasUnitMovementFlag(MOVEMENTFLAG_FALLING_FAR) || m_spellInfo->Effects[0].Effect != SPELL_EFFECT_STUCK) &&
            (IsAutoRepeat() || (m_spellInfo->AuraInterruptFlags & AURA_INTERRUPT_FLAG_NOT_SEATED) != 0))
            return SPELL_FAILED_MOVING;
    }

    Vehicle* vehicle = m_caster->GetVehicle();
    if (m_caster->GetTypeId() == TYPEID_PLAYER && vehicle && !(_triggeredCastFlags & TRIGGERED_IGNORE_CASTER_MOUNTED_OR_ON_VEHICLE))
    {
        uint16 checkMask = 0;
        for (uint8 effIndex = EFFECT_0; effIndex < MAX_SPELL_EFFECTS; ++effIndex)
        {
            SpellEffectInfo const* effInfo = m_spellInfo->GetEffect(effIndex, m_diffMode);
            if (effInfo->ApplyAuraName == SPELL_AURA_MOD_SHAPESHIFT)
            {
                SpellShapeshiftFormEntry const* shapeShiftEntry = sSpellShapeshiftFormStore.LookupEntry(effInfo->MiscValue);
                if (shapeShiftEntry && (shapeShiftEntry->flags1 & 1) == 0)  // unk flag
                    checkMask |= VEHICLE_SEAT_FLAG_UNCONTROLLED;
                break;
            }
        }

        if (m_spellInfo->HasAura(SPELL_AURA_MOUNTED))
            checkMask |= VEHICLE_SEAT_FLAG_CAN_CAST_MOUNT_SPELL;

        if (!checkMask)
            checkMask = VEHICLE_SEAT_FLAG_CAN_ATTACK;

        VehicleSeatEntry const* vehicleSeat = vehicle->GetSeatForPassenger(m_caster);
        if (!(AttributesCustomEx6 & SPELL_ATTR6_CASTABLE_WHILE_ON_VEHICLE) && !(AttributesCustom & SPELL_ATTR0_CASTABLE_WHILE_MOUNTED)
            && (vehicleSeat->m_flags & checkMask) != checkMask)
            return SPELL_FAILED_DONT_REPORT;
    }

    // check spell cast conditions from database
    {
        ConditionSourceInfo condInfo = ConditionSourceInfo(m_caster);
        condInfo.mConditionTargets[1] = m_targets.GetObjectTarget();
        ConditionList conditions = sConditionMgr->GetConditionsForNotGroupedEntry(CONDITION_SOURCE_TYPE_SPELL, m_spellInfo->Id);
        if (!conditions.empty() && !sConditionMgr->IsObjectMeetToConditions(condInfo, conditions))
        {
            // send error msg to player if condition failed and text message available
            // TODO: using WorldSession::SendNotification is not blizzlike
            if (Player* playerCaster = m_caster->ToPlayer())
            {
                // mLastFailedCondition can be NULL if there was an error processing the condition in Condition::Meets (i.e. wrong data for ConditionTarget or others)
                if (playerCaster->GetSession() && condInfo.mLastFailedCondition
                    && condInfo.mLastFailedCondition->ErrorTextId)
                {
                    playerCaster->GetSession()->SendNotification(condInfo.mLastFailedCondition->ErrorTextId);
                    return SPELL_FAILED_DONT_REPORT;
                }
            }
            if (!condInfo.mLastFailedCondition || !condInfo.mLastFailedCondition->ConditionTarget)
                return SPELL_FAILED_CASTER_AURASTATE;
            return SPELL_FAILED_BAD_TARGETS;
        }
    }

    // Don't check explicit target for passive spells (workaround) (check should be skipped only for learn case)
    // those spells may have incorrect target entries or not filled at all (for example 15332)
    // such spells when learned are not targeting anyone using targeting system, they should apply directly to caster instead
    // also, such casts shouldn't be sent to client
    if (!((AttributesCustom & SPELL_ATTR0_PASSIVE) && (!m_targets.GetUnitTarget() || m_targets.GetUnitTarget() == m_caster)))
    {
        bool targetCheck = false;
        // Check explicit target for m_originalCaster - todo: get rid of such workarounds
        if ((m_spellInfo->GetExplicitTargetMask() & TARGET_FLAG_CORPSE_MASK) && !m_targets.GetObjectTarget())
        {
            if (Unit* owner = m_caster->GetAnyOwner())
            {
                if (Player* playerCaster = owner->ToPlayer())
                    if (Unit* selectedUnit = ObjectAccessor::GetUnit(*m_caster, playerCaster->GetSelection()))
                    {
                        SpellCastResult castResult = m_spellInfo->CheckExplicitTarget(m_originalCaster ? m_originalCaster : m_caster, selectedUnit, m_targets.GetItemTarget());
                        if (castResult != SPELL_CAST_OK)
                            return castResult;
                        m_targets.SetUnitTarget(selectedUnit);
                        targetCheck = true;
                    }
            }
        }
        if(!targetCheck)
        {
            SpellCastResult castResult = m_spellInfo->CheckExplicitTarget(m_originalCaster ? m_originalCaster : m_caster, m_targets.GetObjectTarget(), m_targets.GetItemTarget());
            if (castResult != SPELL_CAST_OK)
                return castResult;
        }
    }

    if (Unit* target = m_targets.GetUnitTarget())
    {
        SpellCastResult castResult = m_spellInfo->CheckTarget(m_caster, target, false);
        if (castResult != SPELL_CAST_OK)
            return castResult;

        if (target != m_caster)
        {
            // Must be behind the target
            if ((AttributesCustomCu & SPELL_ATTR0_CU_REQ_CASTER_BEHIND_TARGET) && target->HasInArc(static_cast<float>(M_PI), m_caster))
                return SPELL_FAILED_NOT_BEHIND;

            // Target must be facing you
            if ((AttributesCustomCu & SPELL_ATTR0_CU_REQ_TARGET_FACING_CASTER) && !target->HasInArc(static_cast<float>(M_PI), m_caster) && !m_spellInfo->CanNonFacing(m_caster))
                return SPELL_FAILED_NOT_INFRONT;

            if (!IsTriggered())
            {
                // Hackfix for Raigonn
                if (m_caster->GetEntry() != WORLD_TRIGGER && target->GetEntry() != 56895) // Ignore LOS for gameobjects casts (wrongly casted by a trigger)
                    if (!(AttributesCustomEx2 & SPELL_ATTR2_CAN_TARGET_NOT_IN_LOS) && VMAP::VMapFactory::checkSpellForLoS(m_spellInfo->Id) && !m_caster->IsWithinLOSInMap(target))
                        return SPELL_FAILED_LINE_OF_SIGHT;

                if (m_spellInfo->RangeEntry->ID != 2 /*Combat Range*/ && m_caster->IsVisionObscured(target))
                {
                    if (m_caster->ToCreature() && m_caster->GetEntry() == 71529) //fix exploit on Thok Bloodthirsty
                        m_caster->ToCreature()->AI()->EnterEvadeMode();
                    return SPELL_FAILED_VISION_OBSCURED; // smoke bomb, camouflage...
                }
            }
        }
    }

    // Check for line of sight for spells with dest
    if (m_targets.HasDst())
    {
        float x, y, z;
        m_targets.GetDstPos()->GetPosition(x, y, z);

        if (!(AttributesCustomEx2 & SPELL_ATTR2_CAN_TARGET_NOT_IN_LOS) && !VMAP::VMapFactory::checkSpellForLoS(m_spellInfo->Id) && !m_caster->IsWithinLOS(x, y, z))
            return SPELL_FAILED_LINE_OF_SIGHT;
    }

    // check pet presence
    for (int j = 0; j < MAX_SPELL_EFFECTS; ++j)
    {
        if (m_spellInfo->Effects[j].TargetA.GetTarget() == TARGET_UNIT_PET)
        {
            if (!m_caster->GetGuardianPet() && !m_caster->isPet())
            {
                if (m_triggeredByAuraSpell)              // not report pet not existence for triggered spells
                    return SPELL_FAILED_DONT_REPORT;
                else
                    return SPELL_FAILED_NO_PET;
            }
            break;
        }
    }

    // Spell casted only on battleground
    if ((AttributesCustomEx3 & SPELL_ATTR3_BATTLEGROUND) &&  m_caster->GetTypeId() == TYPEID_PLAYER)
        if (!m_caster->ToPlayer()->InBattleground())
            return SPELL_FAILED_ONLY_BATTLEGROUNDS;

    // do not allow spells to be cast in arenas or rated battlegrounds
    if (Player * player = m_caster->ToPlayer())
        if (player->InArena()/* || player->InRatedBattleGround() NYI*/)
        {
            SpellCastResult castResult = CheckArenaAndRatedBattlegroundCastRules();
            if (castResult != SPELL_CAST_OK)
                return castResult;
        }

    // zone check
    if (m_caster->GetTypeId() == TYPEID_UNIT || !m_caster->ToPlayer()->isGameMaster())
    {
        SpellCastResult locRes= m_spellInfo->CheckLocation(m_caster->GetMapId(), m_caster->GetZoneId(), m_caster->GetAreaId(),
            m_caster->GetTypeId() == TYPEID_PLAYER ? m_caster->ToPlayer() : NULL);
        if (locRes != SPELL_CAST_OK)
            return locRes;
    }

    // not let players cast spells at mount (and let do it to creatures)
    if (m_caster->IsMounted() && m_caster->GetTypeId() == TYPEID_PLAYER && !(_triggeredCastFlags & TRIGGERED_IGNORE_CASTER_MOUNTED_OR_ON_VEHICLE) &&
        !m_spellInfo->IsPassive() && !(AttributesCustom & SPELL_ATTR0_CASTABLE_WHILE_MOUNTED))
    {
        if (m_caster->isInFlight())
            return SPELL_FAILED_NOT_ON_TAXI;
        else
            return SPELL_FAILED_NOT_MOUNTED;
    }

    SpellCastResult castResult = SPELL_CAST_OK;

    // always (except passive spells) check items (focus object can be required for any type casts)
    if (!m_spellInfo->IsPassive())
    {
        castResult = CheckItems();
        if (castResult != SPELL_CAST_OK)
            return castResult;
    }

    // Triggered spells also have range check
    // TODO: determine if there is some flag to enable/disable the check
    castResult = CheckRange(strict);
    if (castResult != SPELL_CAST_OK)
        return castResult;

    if (!(_triggeredCastFlags & TRIGGERED_IGNORE_POWER_AND_REAGENT_COST))
    {
        castResult = CheckPower();
        if (castResult != SPELL_CAST_OK)
            return castResult;
    }

    if (!(_triggeredCastFlags & TRIGGERED_IGNORE_CASTER_AURAS))
    {
        castResult = CheckCasterAuras();
        if (castResult != SPELL_CAST_OK)
            return castResult;
    }

    // script hook
    castResult = CallScriptCheckCastHandlers();
    if (castResult != SPELL_CAST_OK)
        return castResult;

    bool hasDispellableAura = false;
    bool hasNonDispelEffect = false;
    uint32 dispelMask = 0;
    for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
    {
        if (m_spellInfo->GetEffect(i, m_diffMode)->Effect == SPELL_EFFECT_DISPEL)
        {
            if (m_spellInfo->GetEffect(i, m_diffMode)->IsTargetingArea() || AttributesCustomEx & SPELL_ATTR1_MELEE_COMBAT_START)
            {
                hasDispellableAura = true;
                break;
            }

            dispelMask |= SpellInfo::GetDispelMask(DispelType(m_spellInfo->GetEffect(i, m_diffMode)->MiscValue));
        }
        else if (m_spellInfo->GetEffect(i, m_diffMode)->IsEffect())
        {
            hasNonDispelEffect = true;
            break;
        }
    }

    /*if (!hasNonDispelEffect && !hasDispellableAura && dispelMask && !IsTriggered())
    {
        if (Unit* target = m_targets.GetUnitTarget())
        {
            DispelChargesList dispelList;
            target->GetDispellableAuraList(m_caster, dispelMask, dispelList);
            if (dispelList.empty())
                return SPELL_FAILED_NOTHING_TO_DISPEL;
        }
    }*/

    for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
    {
        // for effects of spells that have only one target
        switch (m_spellInfo->GetEffect(i, m_diffMode)->Effect)
        {
            case SPELL_EFFECT_TRIGGER_SPELL:
            {
                switch (m_spellInfo->Id)
                {
                    case 36554:     // Shadowstep
                    {
                        if (Unit * target = m_targets.GetUnitTarget())
                            if (target == m_caster)
                                return SPELL_FAILED_BAD_TARGETS;
                        break;
                    }
                    default:
                        break;
                }
                break;
            }
            case SPELL_EFFECT_APPLY_AURA:
            {
                if (m_spellInfo->HasAura(SPELL_AURA_CLONE_CASTER))
                    if (m_caster->HasAuraType(SPELL_AURA_CLONE_CASTER))
                        return SPELL_FAILED_BAD_TARGETS;

                switch (m_spellInfo->Id)
                {
                    case 5217:   // Tiger's Fury
                    {
                        if (m_caster->HasAura(106951))
                            return SPELL_FAILED_CASTER_AURASTATE;
                        break;
                    }
                    case 5171:  // Slice and Dice
                    case 73651: // Recuperate
                    {
                        if (Aura * aura = m_caster->GetAura(m_spellInfo->Id))
                        {
                            int32 bonusDuration = m_spellInfo->Id == 5171 ? 6000: 0;
                        
                            if (Player * player = m_caster->ToPlayer())
                            {
                                if (aura->GetDuration() > player->GetComboPointsForDuration() * 6000 + bonusDuration)
                                    return SPELL_FAILED_TRY_AGAIN;
                            }
                        }
                        break;
                    }
                    case 143333:    // Water Strider Water Walking
                    {
                        if (Map* map = m_caster->GetMap())
                            if (map->IsBattlegroundOrArena())
                                return (_triggeredCastFlags & TRIGGERED_DONT_REPORT_CAST_ERROR) ? SPELL_FAILED_DONT_REPORT : SPELL_FAILED_INCORRECT_AREA;
                    }
                    default:
                        break;
                }
                break;
            }
            case SPELL_EFFECT_DUMMY:
            {
                // Death Coil and Death Coil (Symbiosis)
                if (m_spellInfo->SpellFamilyName == SPELLFAMILY_DEATHKNIGHT && m_spellInfo->SpellFamilyFlags[0] == 0x2000 ||
                    m_spellInfo->Id == 122282)
                {
                    Unit* target = m_targets.GetUnitTarget();
                    if (!target || (target == m_caster && !m_caster->HasAura(49039)) || (target->IsFriendlyTo(m_caster) && target->GetCreatureType() != CREATURE_TYPE_UNDEAD && !m_caster->HasAura(63333)))
                        return SPELL_FAILED_BAD_TARGETS;
                    if (!target->IsFriendlyTo(m_caster) && !m_caster->HasInArc(static_cast<float>(M_PI), target))
                        return SPELL_FAILED_UNIT_NOT_INFRONT;
                }
                else if (m_spellInfo->Id == 19938)          // Awaken Peon
                {
                    Unit* unit = m_targets.GetUnitTarget();
                    if (!unit || !unit->HasAura(17743))
                        return SPELL_FAILED_BAD_TARGETS;
                }
                else if (m_spellInfo->Id == 31789)          // Righteous Defense
                {
                    if (m_caster->GetTypeId() != TYPEID_PLAYER)
                        return SPELL_FAILED_DONT_REPORT;

                    Unit* target = m_targets.GetUnitTarget();
                    if (!target || !target->IsFriendlyTo(m_caster) || target->getAttackers().empty())
                        return SPELL_FAILED_BAD_TARGETS;

                }
                
                switch (m_spellInfo->Id)
                {
                    case 104316: // Glyph of Imp Swarm
                    {
                        if (Player* plr = m_caster->ToPlayer())
                            if (plr->GetSpecializationId(plr->GetActiveSpec()) != SPEC_WARLOCK_DEMONOLOGY)
                                return SPELL_FAILED_NO_SPEC;
                        break;
                    }
                    case 86121:  // Soul Swap
                    {
                        if (Unit* target = m_targets.GetUnitTarget())
                        {
                            if (!target->GetAuraEffect(SPELL_AURA_PERIODIC_DAMAGE, SPELLFAMILY_WARLOCK, 0x00000402, 0x00000112, 0, m_caster->GetGUID()))
                            {
                                return SPELL_FAILED_BAD_TARGETS;
                            }
                        }
                        break;
                    }
                    case 86213:  // Soul Swap Exhale
                    {
                        if (Unit * target = m_targets.GetUnitTarget())
                        {
                            if (m_caster->m_SpecialTarget == target->GetGUID())
                            {
                                return SPELL_FAILED_BAD_TARGETS;
                            }
                        }
                        break;
                    }
                    case 51640: // Taunt Flag Targeting
                    {
                        if (Unit* target = m_targets.GetUnitTarget())
                            if (Player* plr = m_caster->ToPlayer())
                            {
                                if (Player* targetplr = target->ToPlayer())
                                {
                                    if (targetplr->isAlive())
                                        return SPELL_FAILED_BAD_TARGETS;

                                    if (targetplr->GetTeam() == HORDE && plr->GetTeam() == HORDE)
                                        return SPELL_FAILED_BAD_TARGETS;

                                    if (targetplr->GetTeam() == ALLIANCE && plr->GetTeam() == ALLIANCE)
                                        return SPELL_FAILED_BAD_TARGETS;
                                }
                                else
                                    return SPELL_FAILED_BAD_TARGETS;
                            }
                    }
                    default:
                        break;
                }
                break;
            }
            case SPELL_EFFECT_LEARN_SPELL:
            {
                if (m_caster->GetTypeId() != TYPEID_PLAYER)
                    return SPELL_FAILED_BAD_TARGETS;

                if (m_spellInfo->GetEffect(i, m_diffMode)->TargetA.GetTarget() != TARGET_UNIT_PET)
                    break;

                Pet* pet = m_caster->ToPlayer()->GetPet();

                if (!pet)
                    return SPELL_FAILED_NO_PET;

                SpellInfo const* learn_spellproto = sSpellMgr->GetSpellInfo(m_spellInfo->GetEffect(i, m_diffMode)->TriggerSpell);

                if (!learn_spellproto)
                    return SPELL_FAILED_NOT_KNOWN;

                if (m_spellInfo->SpellLevel > pet->getLevel())
                    return SPELL_FAILED_LOWLEVEL;

                break;
            }
            case SPELL_EFFECT_LEARN_PET_SPELL:
            {
                // check target only for unit target case
                if (Unit* unitTarget = m_targets.GetUnitTarget())
                {
                    if (m_caster->GetTypeId() != TYPEID_PLAYER)
                        return SPELL_FAILED_BAD_TARGETS;

                    Pet* pet = unitTarget->ToPet();
                    if (!pet || pet->GetOwner() != m_caster)
                        return SPELL_FAILED_BAD_TARGETS;

                    SpellInfo const* learn_spellproto = sSpellMgr->GetSpellInfo(m_spellInfo->GetEffect(i, m_diffMode)->TriggerSpell);

                    if (!learn_spellproto)
                        return SPELL_FAILED_NOT_KNOWN;

                    if (m_spellInfo->SpellLevel > pet->getLevel())
                        return SPELL_FAILED_LOWLEVEL;
                }
                break;
            }
            case SPELL_EFFECT_APPLY_GLYPH:
            {
                uint32 glyphId = m_spellInfo->GetEffect(i, m_diffMode)->MiscValue;
                if (GlyphPropertiesEntry const* gp = sGlyphPropertiesStore.LookupEntry(glyphId))
                    if (m_caster->HasAura(gp->SpellId))
                        return SPELL_FAILED_UNIQUE_GLYPH;

                switch (m_spellInfo->Id)
                {
                    case 56292: // Glyph of Imp Swarm
                    {
                        if (Player* plr = m_caster->ToPlayer())
                            if (plr->GetSpecializationId(plr->GetActiveSpec()) != SPEC_WARLOCK_DEMONOLOGY)
                                return SPELL_FAILED_NO_SPEC;
                        break;
                    }
                    default:
                        break;
                }
                break;
            }
            case SPELL_EFFECT_FEED_PET:
            {
                if (m_caster->GetTypeId() != TYPEID_PLAYER)
                    return SPELL_FAILED_BAD_TARGETS;

                Item* foodItem = m_targets.GetItemTarget();
                if (!foodItem)
                    return SPELL_FAILED_BAD_TARGETS;

                Pet* pet = m_caster->ToPlayer()->GetPet();

                if (!pet)
                    return SPELL_FAILED_NO_PET;

                if (!pet->HaveInDiet(foodItem->GetTemplate()))
                    return SPELL_FAILED_WRONG_PET_FOOD;

                if (!pet->GetCurrentFoodBenefitLevel(foodItem->GetTemplate()->ItemLevel))
                    return SPELL_FAILED_FOOD_LOWLEVEL;

                if (m_caster->isInCombat() || pet->isInCombat())
                    return SPELL_FAILED_AFFECTING_COMBAT;

                break;
            }
            case SPELL_EFFECT_POWER_BURN:
            case SPELL_EFFECT_POWER_DRAIN:
            {
                // Can be area effect, Check only for players and not check if target - caster (spell can have multiply drain/burn effects)
                if (m_caster->GetTypeId() == TYPEID_PLAYER)
                    if (Unit* target = m_targets.GetUnitTarget())
                        if (target != m_caster && target->getPowerType() != Powers(m_spellInfo->GetEffect(i, m_diffMode)->MiscValue))
                            return SPELL_FAILED_BAD_TARGETS;
                break;
            }
            case SPELL_EFFECT_CHARGE:
            {
                if (m_spellInfo->SpellFamilyName == SPELLFAMILY_WARRIOR)
                {
                    // Warbringer - can't be handled in proc system - should be done before checkcast root check and charge effect process
                    if (strict && m_caster->IsScriptOverriden(m_spellInfo, 6953))
                        m_caster->RemoveMovementImpairingAuras();
                        
                    // Safeguard
                    if (m_spellInfo->Id == 114029)
                        m_caster->RemoveMovementImpairingAuras();
                }
                if (m_caster->HasUnitState(UNIT_STATE_ROOT))
                    return SPELL_FAILED_ROOTED;
                if (m_caster->GetTypeId() == TYPEID_PLAYER)
                    if (Unit* target = m_targets.GetUnitTarget())
                        if (!target->isAlive())
                            return SPELL_FAILED_BAD_TARGETS;
                break;
            }
            case SPELL_EFFECT_JUMP_DEST:
            {
                if (m_caster->HasUnitState(UNIT_STATE_ROOT))
                    return SPELL_FAILED_ROOTED;
                break;
            }
            case SPELL_EFFECT_SKINNING:
            {
                if (m_caster->GetTypeId() != TYPEID_PLAYER || !m_targets.GetUnitTarget() || m_targets.GetUnitTarget()->GetTypeId() != TYPEID_UNIT)
                    return SPELL_FAILED_BAD_TARGETS;

                if (!(m_targets.GetUnitTarget()->GetUInt32Value(UNIT_FIELD_FLAGS) & UNIT_FLAG_SKINNABLE))
                    return SPELL_FAILED_TARGET_UNSKINNABLE;

                Creature* creature = m_targets.GetUnitTarget()->ToCreature();
                if (creature->GetCreatureType() != CREATURE_TYPE_CRITTER && !creature->loot.isLooted())
                    return SPELL_FAILED_TARGET_NOT_LOOTED;

                uint32 skill = creature->GetCreatureTemplate()->GetRequiredLootSkill();

                int32 skillValue = m_caster->ToPlayer()->GetSkillValue(skill);
                int32 TargetLevel = m_targets.GetUnitTarget()->getLevel();
                int32 ReqValue = (skillValue < 100 ? (TargetLevel-10) * 10 : TargetLevel * 5);
                if (ReqValue > skillValue)
                    return SPELL_FAILED_LOW_CASTLEVEL;

                // chance for fail at orange skinning attempt
                if ((m_selfContainer && (*m_selfContainer) == this) &&
                    skillValue < sWorld->GetConfigMaxSkillValue() &&
                    (ReqValue < 0 ? 0 : ReqValue) > irand(skillValue - 25, skillValue + 37))
                    return SPELL_FAILED_TRY_AGAIN;

                break;
            }
            case SPELL_EFFECT_OPEN_LOCK:
            {
                if (m_spellInfo->GetEffect(i, m_diffMode)->TargetA.GetTarget() != TARGET_GAMEOBJECT_TARGET &&
                    m_spellInfo->GetEffect(i, m_diffMode)->TargetA.GetTarget() != TARGET_GAMEOBJECT_ITEM_TARGET)
                    break;

                if (m_caster->GetTypeId() != TYPEID_PLAYER  // only players can open locks, gather etc.
                    // we need a go target in case of TARGET_GAMEOBJECT_TARGET
                    || (m_spellInfo->GetEffect(i, m_diffMode)->TargetA.GetTarget() == TARGET_GAMEOBJECT_TARGET && !m_targets.GetGOTarget()))
                    return SPELL_FAILED_BAD_TARGETS;

                Item* pTempItem = NULL;
                if (m_targets.GetTargetMask() & TARGET_FLAG_TRADE_ITEM)
                {
                    if (TradeData* pTrade = m_caster->ToPlayer()->GetTradeData())
                        pTempItem = pTrade->GetTraderData()->GetItem(TradeSlots(m_targets.GetItemTargetGUID()));
                }
                else if (m_targets.GetTargetMask() & TARGET_FLAG_ITEM)
                    pTempItem = m_caster->ToPlayer()->GetItemByGuid(m_targets.GetItemTargetGUID());

                // we need a go target, or an openable item target in case of TARGET_GAMEOBJECT_ITEM_TARGET
                if (m_spellInfo->GetEffect(i, m_diffMode)->TargetA.GetTarget() == TARGET_GAMEOBJECT_ITEM_TARGET &&
                    !m_targets.GetGOTarget() &&
                    (!pTempItem || !pTempItem->GetTemplate()->LockID || !pTempItem->IsLocked()))
                    return SPELL_FAILED_BAD_TARGETS;

                if (m_spellInfo->Id != 1842 || (m_targets.GetGOTarget() &&
                    m_targets.GetGOTarget()->GetGOInfo()->type != GAMEOBJECT_TYPE_TRAP))
                    if (m_caster->ToPlayer()->InBattleground() && // In Battleground players can use only flags and banners
                        !m_caster->ToPlayer()->CanUseBattlegroundObject())
                        return SPELL_FAILED_TRY_AGAIN;

                // get the lock entry
                uint32 lockId = 0;
                if (GameObject* go = m_targets.GetGOTarget())
                {
                    lockId = go->GetGOInfo()->GetLockId();
                    if (!lockId)
                        return SPELL_FAILED_BAD_TARGETS;
                }
                else if (Item* itm = m_targets.GetItemTarget())
                    lockId = itm->GetTemplate()->LockID;

                SkillType skillId = SKILL_NONE;
                int32 reqSkillValue = 0;
                int32 skillValue = 0;

                // check lock compatibility
                SpellCastResult res = CanOpenLock(i, lockId, skillId, reqSkillValue, skillValue);
                if (res != SPELL_CAST_OK)
                    return res;

                // chance for fail at orange mining/herb/LockPicking gathering attempt
                // second check prevent fail at rechecks
                if (skillId != SKILL_NONE && (!m_selfContainer || ((*m_selfContainer) != this)))
                {
                    bool canFailAtMax = skillId != SKILL_HERBALISM && skillId != SKILL_MINING;

                    // chance for failure in orange gather / lockpick (gathering skill can't fail at maxskill)
                    if ((canFailAtMax || skillValue < sWorld->GetConfigMaxSkillValue()) && reqSkillValue > irand(skillValue - 25, skillValue + 37))
                        return SPELL_FAILED_TRY_AGAIN;
                }
                break;
            }
            case SPELL_EFFECT_SUMMON_DEAD_PET:
            {
                Creature* pet = m_caster->GetGuardianPet();

                if (pet && pet->isAlive())
                    return SPELL_FAILED_ALREADY_HAVE_SUMMON;

                break;
            }
            // This is generic summon effect
            case SPELL_EFFECT_SUMMON:
            {
                SummonPropertiesEntry const* SummonProperties = sSummonPropertiesStore.LookupEntry(m_spellInfo->GetEffect(i, m_diffMode)->MiscValueB);
                if (!SummonProperties)
                    break;
                switch (SummonProperties->Category)
                {
                    case SUMMON_CATEGORY_PET:
                        if (m_caster->GetPetGUID())
                            return SPELL_FAILED_ALREADY_HAVE_SUMMON;
                    case SUMMON_CATEGORY_PUPPET:
                        if (m_caster->GetCharmGUID())
                            return SPELL_FAILED_ALREADY_HAVE_CHARM;
                        break;
                }
                break;
            }
            case SPELL_EFFECT_CREATE_TAMED_PET:
            {
                if (m_targets.GetUnitTarget())
                {
                    if (m_targets.GetUnitTarget()->GetTypeId() != TYPEID_PLAYER)
                        return SPELL_FAILED_BAD_TARGETS;
                    if (m_targets.GetUnitTarget()->GetPetGUID())
                        return SPELL_FAILED_ALREADY_HAVE_SUMMON;
                }
                break;
            }
            case SPELL_EFFECT_SUMMON_PET:
            {
                if (m_caster->GetPetGUID())                  //let warlock do a replacement summon
                {
                    if (m_caster->GetTypeId() == TYPEID_PLAYER && m_caster->getClass() == CLASS_WARLOCK)
                    {
                        if (strict)                         //starting cast, trigger pet stun (cast by pet so it doesn't attack player)
                            if (Pet* pet = m_caster->ToPlayer()->GetPet())
                                pet->CastSpell(pet, 32752, true, NULL, NULL, pet->GetGUID());
                    }
                    else
                        return SPELL_FAILED_ALREADY_HAVE_SUMMON;
                }

                if (m_caster->GetCharmGUID())
                    return SPELL_FAILED_ALREADY_HAVE_CHARM;
                break;
            }
            case SPELL_EFFECT_SUMMON_PLAYER:
            {
                if (m_caster->GetTypeId() != TYPEID_PLAYER)
                    return SPELL_FAILED_BAD_TARGETS;
                if (!m_caster->ToPlayer()->GetSelection())
                    return SPELL_FAILED_BAD_TARGETS;

                Player* target = ObjectAccessor::FindPlayer(m_caster->ToPlayer()->GetSelection());
                if (!target || m_caster->ToPlayer() == target || (!target->IsInSameRaidWith(m_caster->ToPlayer()) && m_spellInfo->Id != 48955)) // refer-a-friend spell
                    return SPELL_FAILED_BAD_TARGETS;

                // check if our map is dungeon
                MapEntry const* map = sMapStore.LookupEntry(m_caster->GetMapId());
                if (map->IsDungeon())
                {
                    uint32 mapId = m_caster->GetMap()->GetId();
                    Difficulty difficulty = m_caster->GetMap()->GetDifficulty();
                    if (map->IsRaid())
                        if (InstancePlayerBind* targetBind = target->GetBoundInstance(mapId, difficulty))
                            if (InstancePlayerBind* casterBind = m_caster->ToPlayer()->GetBoundInstance(mapId, difficulty))
                                if (targetBind->perm && targetBind->save != casterBind->save)
                                    return SPELL_FAILED_TARGET_LOCKED_TO_RAID_INSTANCE;

                    InstanceTemplate const* instance = sObjectMgr->GetInstanceTemplate(mapId);
                    if (!instance)
                        return SPELL_FAILED_TARGET_NOT_IN_INSTANCE;
                    if (!target->Satisfy(sObjectMgr->GetAccessRequirement(mapId, difficulty), mapId))
                        return SPELL_FAILED_BAD_TARGETS;
                }
                break;
            }
            // RETURN HERE
            case SPELL_EFFECT_SUMMON_RAF_FRIEND:
            {
                if (m_caster->GetTypeId() != TYPEID_PLAYER)
                    return SPELL_FAILED_BAD_TARGETS;

                Player* playerCaster = m_caster->ToPlayer();
                    //
                if (!(playerCaster->GetSelection()))
                    return SPELL_FAILED_BAD_TARGETS;

                Player* target = ObjectAccessor::FindPlayer(playerCaster->GetSelection());

                if (!target ||
                    !(target->GetSession()->GetRecruiterId() == playerCaster->GetSession()->GetAccountId() || target->GetSession()->GetAccountId() == playerCaster->GetSession()->GetRecruiterId()))
                    return SPELL_FAILED_BAD_TARGETS;

                break;
            }
            case SPELL_EFFECT_LEAP:
            case SPELL_EFFECT_TELEPORT_UNITS_FACE_CASTER:
            {
              //Do not allow to cast it before BG starts.
                if (m_caster->GetTypeId() == TYPEID_PLAYER)
                    if (Battleground const* bg = m_caster->ToPlayer()->GetBattleground())
                        if (bg->GetStatus() != STATUS_IN_PROGRESS)
                            return SPELL_FAILED_TRY_AGAIN;
                break;
            }
            case SPELL_EFFECT_STEAL_BENEFICIAL_BUFF:
            {
                if (m_targets.GetUnitTarget() == m_caster)
                    return SPELL_FAILED_BAD_TARGETS;
                break;
            }
            case SPELL_EFFECT_LEAP_BACK:
            {
                if (m_caster->HasUnitState(UNIT_STATE_ROOT) && !m_caster->HasAura(109215))
                {
                    if (m_caster->GetTypeId() == TYPEID_PLAYER)
                        return SPELL_FAILED_ROOTED;
                    else
                        return SPELL_FAILED_DONT_REPORT;
                }
                // Wild Charge
                if (m_spellInfo->Id == 102383 && !m_caster->isInCombat())
                    return SPELL_FAILED_CANT_DO_THAT_RIGHT_NOW;
                break;
            }
            case SPELL_EFFECT_TALENT_SPEC_SELECT:
            case SPELL_EFFECT_UNLEARN_TALENT:
                // can't change during already started arena/battleground
                if (Player* player = m_caster->ToPlayer())
                    if (Battleground* bg = player->GetBattleground())
                        if (bg->GetStatus() == STATUS_IN_PROGRESS)
                            return SPELL_FAILED_NOT_IN_BATTLEGROUND;
                break;
            case SPELL_EFFECT_UNLOCK_GUILD_VAULT_TAB:
            {
                Player* player = m_caster->ToPlayer();

                if (!player)
                    return SPELL_FAILED_DONT_REPORT;

                Guild* guild = sGuildMgr->GetGuildById(player->GetGuildId());
                
                if (!guild)
                    return SPELL_FAILED_CANT_DO_THAT_RIGHT_NOW;

                if (guild->GetPurchasedTabsSize() >= GetSpellInfo()->Effects[0].BasePoints)
                    return SPELL_FAILED_CANT_DO_THAT_RIGHT_NOW;

                if (guild->GetPurchasedTabsSize() < GetSpellInfo()->Effects[0].BasePoints - 1)
                    return SPELL_FAILED_CANT_DO_THAT_RIGHT_NOW;

                break;
            }
            case SPELL_EFFECT_SCRIPT_EFFECT:
            {
                Player* player = m_caster->ToPlayer();
                if (!player)
                    break;

                switch (m_spellInfo->Id)
                {
                    case 143626:                                // Celestial Cloth and Its Uses
                        if (player->HasSpell(143011))
                            return SPELL_FAILED_SPELL_LEARNED;
                        break;
                    case 143644:                                // Hardened Magnificent Hide and Its Uses
                        if (player->HasSpell(142976))
                            return SPELL_FAILED_SPELL_LEARNED;
                        break;
                    case 143646:                                // Balanced Trillium Ingot and Its Uses
                        if (player->HasSpell(143255))
                            return SPELL_FAILED_SPELL_LEARNED;
                        break;
                    case 146428:                                // Timeless Essence of the Black Dragonflight
                    {
                        Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_BACK);
                        if (!item)
                            return SPELL_FAILED_EQUIPPED_ITEM;

                        switch (item->GetEntry())
                        {
                            case 98149:
                            case 98147:
                            case 98146:
                            case 98335:
                            case 98148:
                            case 98150:
                                break;
                            default:
                                return SPELL_FAILED_EQUIPPED_ITEM;
                        }
                    }
                }
                break;
            }
            case SPELL_EFFECT_SUMMON_RAID_MARKER:
            {
                Player* player = m_caster->ToPlayer();
                if (!player)
                    return SPELL_FAILED_CANT_DO_THAT_RIGHT_NOW;

                Group* group = player->GetGroup();
                if (!group)
                    return SPELL_FAILED_CANT_DO_THAT_RIGHT_NOW;
                break;
            }
            case SPELL_EFFECT_RESURRECT:
            {
                if (AttributesCustomEx8 & SPELL_ATTR8_BATTLE_RESURRECTION)
                {
                    if (InstanceScript* pInstance = m_caster->GetInstanceScript())
                        if (!pInstance->GetResurectSpell())
                            return SPELL_FAILED_IN_COMBAT_RES_LIMIT_REACHED;
                }
                break;
            }
            default:
                break;
        }
    }

    for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
    {
        switch (m_spellInfo->GetEffect(i, m_diffMode)->ApplyAuraName)
        {
            case SPELL_AURA_MOD_POSSESS_PET:
            {
                if (m_caster->GetTypeId() != TYPEID_PLAYER)
                    break;

                Pet* pet = m_caster->ToPlayer()->GetPet();
                if (!pet)
                    return SPELL_FAILED_NO_PET;

                if (pet->GetCharmerGUID())
                    return SPELL_FAILED_CHARMED;
                break;
            }
            case SPELL_AURA_MOD_POSSESS:
            case SPELL_AURA_MOD_CHARM:
            case SPELL_AURA_AOE_CHARM:
            {
                if (m_caster->GetCharmerGUID())
                    return SPELL_FAILED_CHARMED;

                if (m_spellInfo->GetEffect(i, m_diffMode)->ApplyAuraName == SPELL_AURA_MOD_CHARM
                    || m_spellInfo->GetEffect(i, m_diffMode)->ApplyAuraName == SPELL_AURA_MOD_POSSESS)
                {
                    if (m_caster->GetPetGUID())
                        return SPELL_FAILED_ALREADY_HAVE_SUMMON;

                    if (m_caster->GetCharmGUID())
                        return SPELL_FAILED_ALREADY_HAVE_CHARM;
                }

                if (Unit* target = m_targets.GetUnitTarget())
                {
                    if (target->GetTypeId() == TYPEID_UNIT && target->ToCreature()->IsVehicle())
                        return SPELL_FAILED_BAD_IMPLICIT_TARGETS;

                    if (target->IsMounted())
                        return SPELL_FAILED_CANT_BE_CHARMED;

                    if (target->GetCharmerGUID())
                        return SPELL_FAILED_CHARMED;

                    int32 damage = CalculateDamage(i, target);
                    if (damage && int32(target->getLevel()) > damage)
                        return SPELL_FAILED_HIGHLEVEL;
                }

                break;
            }
            case SPELL_AURA_MOUNTED:
            {
                //if (m_caster->IsInWater())
                    //return SPELL_FAILED_ONLY_ABOVEWATER;

                // Ignore map check if spell have AreaId. AreaId already checked and this prevent special mount spells
                bool allowMount = !m_caster->GetMap()->IsDungeon() || m_caster->GetMap()->IsBattlegroundOrArena();
                InstanceTemplate const* it = sObjectMgr->GetInstanceTemplate(m_caster->GetMapId());
                if (it)
                    allowMount = it->AllowMount;
                if ((m_caster->GetTypeId() == TYPEID_PLAYER && !allowMount && !m_spellInfo->AreaGroupId) || (m_caster->GetMapId() == 530 && m_caster->GetZoneId() == 0)) //530 - uwow event map
                    return SPELL_FAILED_NO_MOUNTS_ALLOWED;

                if (m_caster->IsInDisallowedMountForm())
                    return SPELL_FAILED_NOT_SHAPESHIFT;

                break;
            }
            case SPELL_AURA_RANGED_ATTACK_POWER_ATTACKER_BONUS:
            {
                if (!m_targets.GetUnitTarget())
                    return SPELL_FAILED_BAD_IMPLICIT_TARGETS;

                // can be casted at non-friendly unit or own pet/charm
                if (m_caster->IsFriendlyTo(m_targets.GetUnitTarget()))
                    return SPELL_FAILED_TARGET_FRIENDLY;

                break;
            }
            case SPELL_AURA_FLY:
            case SPELL_AURA_MOD_INCREASE_MOUNTED_FLIGHT_SPEED:
            {
                // not allow cast fly spells if not have req. skills  (all spells is self target)
                // allow always ghost flight spells
                if (m_originalCaster && m_originalCaster->GetTypeId() == TYPEID_PLAYER && m_originalCaster->isAlive())
                {
                    Battlefield* Bf = sBattlefieldMgr->GetBattlefieldToZoneId(m_originalCaster->GetZoneId());
                    if (m_originalCaster->vmapData->HasAreaTableFlags(AREA_FLAG_NO_FLY_ZONE) || (Bf && !Bf->CanFlyIn()))
                        return (_triggeredCastFlags & TRIGGERED_DONT_REPORT_CAST_ERROR) ? SPELL_FAILED_DONT_REPORT : SPELL_FAILED_NOT_HERE;
                }
                break;
            }
            case SPELL_AURA_PERIODIC_MANA_LEECH:
            {
                if (m_spellInfo->GetEffect(i, m_diffMode)->IsTargetingArea())
                    break;

                if (!m_targets.GetUnitTarget())
                    return SPELL_FAILED_BAD_IMPLICIT_TARGETS;

                if (m_caster->GetTypeId() != TYPEID_PLAYER || m_CastItem)
                    break;

                if (m_targets.GetUnitTarget()->getPowerType() != POWER_MANA)
                    return SPELL_FAILED_BAD_TARGETS;

                break;
            }
            default:
                break;
        }
    }

    // check trade slot case (last, for allow catch any another cast problems)
    if (m_targets.GetTargetMask() & TARGET_FLAG_TRADE_ITEM)
    {
        if (m_CastItem)
            return SPELL_FAILED_ITEM_ENCHANT_TRADE_WINDOW;

        if (m_caster->GetTypeId() != TYPEID_PLAYER)
            return SPELL_FAILED_NOT_TRADING;

        TradeData* my_trade = m_caster->ToPlayer()->GetTradeData();

        if (!my_trade)
            return SPELL_FAILED_NOT_TRADING;

        TradeSlots slot = TradeSlots(m_targets.GetItemTargetGUID());
        if (slot != TRADE_SLOT_NONTRADED)
            return SPELL_FAILED_BAD_TARGETS;

        if (!IsTriggered())
            if (my_trade->GetSpell())
                return SPELL_FAILED_ITEM_ALREADY_ENCHANTED;
    }

    if (AttributesCustomEx7 & SPELL_ATTR7_CONSOLIDATED_RAID_BUFF)
    {
        if (m_originalTarget)
            if (m_originalTarget->GetTypeId() != TYPEID_PLAYER)
                if (m_originalTarget->GetOwner() && m_originalTarget->GetOwner()->GetTypeId() == TYPEID_PLAYER)
                    return SPELL_FAILED_TARGET_IS_PLAYER_CONTROLLED;
    }

    // check if caster has at least 1 combo point for spells that require combo points
    if (m_needComboPoints)
        if (Player* plrCaster = m_caster->ToPlayer())
            if (!plrCaster->GetComboPoints())
                return SPELL_FAILED_NO_COMBO_POINTS;

    // all ok
    return SPELL_CAST_OK;
}

SpellCastResult Spell::CheckPetCast(Unit* target)
{
    if (m_caster->HasUnitState(UNIT_STATE_CASTING) && !(_triggeredCastFlags & TRIGGERED_IGNORE_CAST_IN_PROGRESS))              //prevent spellcast interruption by another spellcast
        return SPELL_FAILED_SPELL_IN_PROGRESS;

    // dead owner (pets still alive when owners ressed?)
    if (Unit* owner = m_caster->GetCharmerOrOwner())
        if (!owner->isAlive())
            return SPELL_FAILED_CASTER_DEAD;

    if (!target && m_targets.GetUnitTarget())
        target = m_targets.GetUnitTarget();

    if (m_spellInfo->NeedsExplicitUnitTarget())
    {
        if (!target)
            return SPELL_FAILED_BAD_IMPLICIT_TARGETS;
        m_targets.SetUnitTarget(target);
    }

    // cooldown
    if (Creature const* creatureCaster = m_caster->ToCreature())
        if (creatureCaster->HasSpellCooldown(m_spellInfo->Id))
            if(!m_caster->HasAuraTypeWithAffectMask(SPELL_AURA_IGNORE_CD, m_spellInfo))
                return SPELL_FAILED_NOT_READY;

    if (m_caster->HasAuraType(SPELL_AURA_371))
        return SPELL_FAILED_CANT_DO_THAT_RIGHT_NOW;

    return CheckCast(true);
}

SpellCastResult Spell::CheckCasterAuras() const
{
    // spells totally immuned to caster auras (wsg flag drop, give marks etc)
    if (AttributesCustomEx6 & SPELL_ATTR6_IGNORE_CASTER_AURAS)
        return SPELL_CAST_OK;

    uint8 school_immune = 0;
    uint32 mechanic_immune = 0;
    uint32 dispel_immune = 0;

    // Check if the spell grants school or mechanic immunity.
    // We use bitmasks so the loop is done only once and not on every aura check below.
    if (AttributesCustomEx & SPELL_ATTR1_DISPEL_AURAS_ON_IMMUNITY)
    {
        for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
        {
            if (m_spellInfo->GetEffect(i, m_diffMode)->ApplyAuraName == SPELL_AURA_SCHOOL_IMMUNITY)
                school_immune |= uint32(m_spellInfo->GetEffect(i, m_diffMode)->MiscValue);
            else if (m_spellInfo->GetEffect(i, m_diffMode)->ApplyAuraName == SPELL_AURA_MECHANIC_IMMUNITY)
                mechanic_immune |= 1 << uint32(m_spellInfo->GetEffect(i, m_diffMode)->MiscValue);
            else if (m_spellInfo->GetEffect(i, m_diffMode)->ApplyAuraName == SPELL_AURA_DISPEL_IMMUNITY)
                dispel_immune |= SpellInfo::GetDispelMask(DispelType(m_spellInfo->GetEffect(i, m_diffMode)->MiscValue));
        }
        // immune movement impairment and loss of control
        if (m_spellInfo->Id == 42292 || m_spellInfo->Id == 59752)
            mechanic_immune = IMMUNE_TO_MOVEMENT_IMPAIRMENT_AND_LOSS_CONTROL_MASK;
    }

    bool usableInStun = AttributesCustomEx5 & SPELL_ATTR5_USABLE_WHILE_STUNNED;

    // Check whether the cast should be prevented by any state you might have.
    SpellCastResult prevented_reason = SPELL_CAST_OK;
    // Have to check if there is a stun aura. Otherwise will have problems with ghost aura apply while logging out
    uint32 unitflag = m_caster->GetUInt32Value(UNIT_FIELD_FLAGS);     // Get unit state
    if (unitflag & UNIT_FLAG_STUNNED)
    {
        // spell is usable while stunned, check if caster has only mechanic stun auras, another stun types must prevent cast spell
        if (usableInStun)
        {
            bool foundNotStun = false;
            Unit::AuraEffectList const& stunAuras = m_caster->GetAuraEffectsByType(SPELL_AURA_MOD_STUN);
            for (Unit::AuraEffectList::const_iterator i = stunAuras.begin(); i != stunAuras.end(); ++i)
            {
                if (!((*i)->GetSpellInfo()->GetAllEffectsMechanicMask() & (1<<MECHANIC_STUN)))
                {
                    foundNotStun = true;
                    break;
                }
            }
            if (foundNotStun)
                prevented_reason = SPELL_FAILED_STUNNED;
        }
        else
            prevented_reason = SPELL_FAILED_STUNNED;
    }
    else if ((unitflag & UNIT_FLAG_CONFUSED || m_caster->HasAuraType(SPELL_AURA_MOD_CONFUSE)) && !(AttributesCustomEx5 & SPELL_ATTR5_USABLE_WHILE_CONFUSED))
        prevented_reason = SPELL_FAILED_CONFUSED;
    else if (unitflag & UNIT_FLAG_FLEEING && !(AttributesCustomEx5 & SPELL_ATTR5_USABLE_WHILE_FEARED))
        prevented_reason = SPELL_FAILED_FLEEING;
    else if (unitflag & UNIT_FLAG_SILENCED && m_spellInfo->PreventionType == SPELL_PREVENTION_TYPE_SILENCE && !(AttributesCustomEx8 & SPELL_ATTR8_USABLE_WHILE_SILENCED))
        prevented_reason = SPELL_FAILED_SILENCED;
    else if (unitflag & UNIT_FLAG_PACIFIED)
    {
        if (m_spellInfo->PreventionType == SPELL_PREVENTION_TYPE_PACIFY)
            prevented_reason = SPELL_FAILED_PACIFIED;

        Unit::AuraEffectList const& pacifAuras = m_caster->GetAuraEffectsByType(SPELL_AURA_MOD_PACIFY_SILENCE);
        for (auto i : pacifAuras)
            if ((i)->GetSpellInfo()->GetAllEffectsMechanicMask() & ((1 << MECHANIC_POLYMORPH) | (1 << MECHANIC_SILENCE) | (1 << MECHANIC_BANISH)))
                prevented_reason = SPELL_FAILED_PACIFIED;
    }
        

    // Attr must make flag drop spell totally immune from all effects
    if (prevented_reason != SPELL_CAST_OK)
    {
        if (school_immune || mechanic_immune || dispel_immune)
        {
            //Checking auras is needed now, because you are prevented by some state but the spell grants immunity.
            Unit::AuraApplicationMap const& auras = m_caster->GetAppliedAuras();
            for (Unit::AuraApplicationMap::const_iterator itr = auras.begin(); itr != auras.end(); ++itr)
            {
                Aura const* aura = itr->second->GetBase();
                SpellInfo const* auraInfo = aura->GetSpellInfo();
                if (auraInfo->GetAllEffectsMechanicMask() & mechanic_immune)
                    continue;
                if (auraInfo->GetSchoolMask() & school_immune && !(auraInfo->AttributesEx & SPELL_ATTR1_UNAFFECTED_BY_SCHOOL_IMMUNE))
                    continue;
                if (auraInfo->GetDispelMask() & dispel_immune)
                    continue;

                //Make a second check for spell failed so the right SPELL_FAILED message is returned.
                //That is needed when your casting is prevented by multiple states and you are only immune to some of them.
                for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
                {
                    if (AuraEffect* part = aura->GetEffect(i))
                    {
                        switch (part->GetAuraType())
                        {
                            case SPELL_AURA_MOD_STUN:
                                if (!usableInStun || !(auraInfo->GetAllEffectsMechanicMask() & (1<<MECHANIC_STUN)))
                                    return SPELL_FAILED_STUNNED;
                                break;
                            case SPELL_AURA_MOD_CONFUSE:
                                if (!(AttributesCustomEx5 & SPELL_ATTR5_USABLE_WHILE_CONFUSED))
                                    return SPELL_FAILED_CONFUSED;
                                break;
                            case SPELL_AURA_MOD_FEAR:
                            case SPELL_AURA_MOD_FEAR_2:
                                if (!(AttributesCustomEx5 & SPELL_ATTR5_USABLE_WHILE_FEARED))
                                    return SPELL_FAILED_FLEEING;
                                break;
                            case SPELL_AURA_MOD_SILENCE:
                            case SPELL_AURA_MOD_PACIFY:
                            case SPELL_AURA_MOD_PACIFY_SILENCE:
                                if (m_spellInfo->PreventionType == SPELL_PREVENTION_TYPE_PACIFY)
                                    return SPELL_FAILED_PACIFIED;
                                else if (m_spellInfo->PreventionType == SPELL_PREVENTION_TYPE_SILENCE && !(AttributesCustomEx8 & SPELL_ATTR8_USABLE_WHILE_SILENCED))
                                    return SPELL_FAILED_SILENCED;
                                break;
                            default: break;
                        }
                    }
                }
            }
        }
        // You are prevented from casting and the spell casted does not grant immunity. Return a failed error.
        else
            return prevented_reason;
    }
    return SPELL_CAST_OK;
}

SpellCastResult Spell::CheckArenaAndRatedBattlegroundCastRules()
{
    bool isRatedBattleground = false; // NYI
    bool isArena = !isRatedBattleground;

    // check USABLE attributes
    // USABLE takes precedence over NOT_USABLE
    if (isRatedBattleground && AttributesCustomEx9 & SPELL_ATTR9_USABLE_IN_RATED_BATTLEGROUNDS)
        return SPELL_CAST_OK;

    if (isArena && AttributesCustomEx4 & SPELL_ATTR4_USABLE_IN_ARENA)
        return SPELL_CAST_OK;

    // check NOT_USABLE attributes
    if (AttributesCustomEx4 & SPELL_ATTR4_NOT_USABLE_IN_ARENA_OR_RATED_BG)
        return isArena ? SPELL_FAILED_NOT_IN_ARENA : SPELL_FAILED_NOT_IN_RATED_BATTLEGROUND;

    if (isArena && AttributesCustomEx9 & SPELL_ATTR9_NOT_USABLE_IN_ARENA)
            return SPELL_FAILED_NOT_IN_ARENA;

    // check cooldowns
    uint32 spellCooldown = m_spellInfo->GetRecoveryTime();
    if (isArena && spellCooldown > 10 * MINUTE * IN_MILLISECONDS) // not sure if still needed
        return SPELL_FAILED_NOT_IN_ARENA;

    if (isRatedBattleground && spellCooldown > 15 * MINUTE * IN_MILLISECONDS)
        return SPELL_FAILED_NOT_IN_RATED_BATTLEGROUND;

    return SPELL_CAST_OK;
}

bool Spell::CanAutoCast(Unit* target)
{
    uint64 targetguid = target->GetGUID();

    for (uint32 j = 0; j < MAX_SPELL_EFFECTS; ++j)
    {
        switch (m_spellInfo->Effects[j].Effect)
        {
            case SPELL_EFFECT_APPLY_AURA:
            {
                if (m_spellInfo->StackAmount <= 1)
                {
                    if (target->HasAuraEffect(m_spellInfo->Id, j))
                        return false;
                }
                else
                {
                    if (AuraEffect* aureff = target->GetAuraEffect(m_spellInfo->Id, j))
                        if (aureff->GetBase()->GetStackAmount() >= m_spellInfo->StackAmount)
                            return false;
                }
                switch (m_spellInfo->Effects[j].ApplyAuraName)
                {
                    case SPELL_AURA_MOD_CONFUSE:
                    case SPELL_AURA_MOD_FEAR:
                    case SPELL_AURA_MOD_STUN:
                    {
                        bool find = false;
                        for (uint32 i = CURRENT_FIRST_NON_MELEE_SPELL; i < CURRENT_MAX_SPELL; i++)
                            if (target->GetCurrentSpell(CurrentSpellTypes(i)))
                                find = true;
                        if(!find)
                            return false;
                        break;
                    }
                    case SPELL_AURA_MOD_PET_STATS_MODIFIER:
                    {
                        if(target->GetHealthPct() >= 20.0f)
                            return false;
                        break;
                    }
                }
                break;
            }
            case SPELL_EFFECT_DISPEL:
            case SPELL_EFFECT_STEAL_BENEFICIAL_BUFF:
            {
                DispelChargesList dispelList;
                uint32 dispelMask = SpellInfo::GetDispelMask(DispelType(m_spellInfo->GetEffect(j, m_diffMode)->MiscValue));
                target->GetDispellableAuraList(m_caster, dispelMask, dispelList);

                if (dispelList.empty())
                    return false;
                break;
            }
            case SPELL_EFFECT_ATTACK_ME:
            {
                if (target->getVictim() == m_caster)
                    return false;
                break;
            }
            case SPELL_EFFECT_JUMP:
            case SPELL_EFFECT_CHARGE:
            case SPELL_EFFECT_PULL_TOWARDS:
            {
                if(m_spellInfo->GetMinRange(!m_caster->IsHostileTo(target)))
                    break;
                if (m_caster->IsWithinMeleeRange(target))
                    return false;
                break;
            }
            case SPELL_EFFECT_INTERRUPT_CAST:
            {
                bool find = false;
                for (uint32 i = CURRENT_FIRST_NON_MELEE_SPELL; i < CURRENT_MAX_SPELL; i++)
                    if (target->GetCurrentSpell(CurrentSpellTypes(i)))
                        find = true;
                if(!find)
                    return false;
                break;
            }
        }
        if (m_spellInfo->Effects[j].IsAreaAuraEffect())
        {
            if (target->HasAuraEffect(m_spellInfo->Id, j))
                return false;
        }
    }

    SpellCastResult result = CheckPetCast(target);

    if (result == SPELL_CAST_OK || result == SPELL_FAILED_UNIT_NOT_INFRONT)
    {
        SelectSpellTargets();
        //check if among target units, our WANTED target is as well (->only self cast spells return false)
        for (std::list<TargetInfo>::iterator ihit = m_UniqueTargetInfo.begin(); ihit != m_UniqueTargetInfo.end(); ++ihit)
            if (ihit->targetGUID == targetguid)
                return true;
    }
    if(m_spellInfo->Id == 128997) //Don't have target in spell
        return true;

    return false;                                           //target invalid
}

SpellCastResult Spell::CheckRange(bool strict)
{
    // Don't check for instant cast spells
    if (!strict && m_casttime == 0)
        return SPELL_CAST_OK;

    uint32 range_type = 0;

    if (m_spellInfo->RangeEntry)
    {
        // check needed by 68766 51693 - both spells are cast on enemies and have 0 max range
        // these are triggered by other spells - possibly we should omit range check in that case?
        if (m_spellInfo->RangeEntry->ID == 1)
            return SPELL_CAST_OK;

        range_type = m_spellInfo->RangeEntry->type;
    }

    Unit* target = m_targets.GetUnitTarget() ? m_targets.GetUnitTarget() : m_originalTarget;
    float max_range = m_caster->GetSpellMaxRangeForTarget(target, m_spellInfo);
    float min_range = m_caster->GetSpellMinRangeForTarget(target, m_spellInfo);

    if (Player* modOwner = m_caster->GetSpellModOwner())
        modOwner->ApplySpellMod(m_spellInfo->Id, SPELLMOD_RANGE, max_range, this);

    if (target && target != m_caster)
    {
        if (range_type == SPELL_RANGE_MELEE)
        {
            // Because of lag, we can not check too strictly here.
            if (!m_caster->IsWithinMeleeRange(target, max_range)) //Fix range hack
                return !(_triggeredCastFlags & TRIGGERED_DONT_REPORT_CAST_ERROR) ? SPELL_FAILED_OUT_OF_RANGE : SPELL_FAILED_DONT_REPORT;
        }
        else if (!m_caster->IsWithinCombatRange(target, max_range))
            return !(_triggeredCastFlags & TRIGGERED_DONT_REPORT_CAST_ERROR) ? SPELL_FAILED_OUT_OF_RANGE : SPELL_FAILED_DONT_REPORT; //0x5A;

        if (range_type == SPELL_RANGE_RANGED)
        {
            if (m_caster->IsWithinMeleeRange(target))
                return !(_triggeredCastFlags & TRIGGERED_DONT_REPORT_CAST_ERROR) ? SPELL_FAILED_TOO_CLOSE : SPELL_FAILED_DONT_REPORT;
        }
        else if (min_range && m_caster->IsWithinCombatRange(target, min_range)) // skip this check if min_range = 0
            return !(_triggeredCastFlags & TRIGGERED_DONT_REPORT_CAST_ERROR) ? SPELL_FAILED_TOO_CLOSE : SPELL_FAILED_DONT_REPORT;

        if (m_caster->GetTypeId() == TYPEID_PLAYER &&
            (m_spellInfo->FacingCasterFlags & SPELL_FACING_FLAG_INFRONT) && !m_caster->HasInArc(static_cast<float>(M_PI), target))
            return !(_triggeredCastFlags & TRIGGERED_DONT_REPORT_CAST_ERROR) ? SPELL_FAILED_UNIT_NOT_INFRONT : SPELL_FAILED_DONT_REPORT;
    }

    if (!target && m_targets.HasDst() && !m_targets.HasTraj())
    {
        if (!m_caster->IsWithinDist3d(m_targets.GetDstPos(), max_range))
            return SPELL_FAILED_OUT_OF_RANGE;
        if (min_range && m_caster->IsWithinDist3d(m_targets.GetDstPos(), min_range))
            return SPELL_FAILED_TOO_CLOSE;
    }

    return SPELL_CAST_OK;
}

SpellCastResult Spell::CheckPower()
{
    // item cast not used power
    if (m_CastItem)
        return SPELL_CAST_OK;

    for (uint8 i = 0; i < MAX_POWERS_FOR_SPELL; ++i)
    {
        if (!GetSpellInfo()->IsPowerActive(i))
            continue;

        SpellPowerEntry power = GetSpellInfo()->GetPowerInfo(i);
        // health as power used - need check health amount
        if (power.powerType == POWER_HEALTH)
        {
            if (int32(m_caster->GetHealth()) <= m_powerCost)
                return SPELL_FAILED_CASTER_AURASTATE;
            return SPELL_CAST_OK;
        }
        // Check valid power type
        if (power.powerType >= MAX_POWERS)
        {
            sLog->outError(LOG_FILTER_SPELLS_AURAS, "Spell::CheckPower: Unknown power type '%d'", power.powerType);
            return SPELL_FAILED_UNKNOWN;
        }
    }

    SpellPowerEntry power;
    if (GetSpellInfo()->GetSpellPowerByCasterPower(m_caster, power))
    {
        //check rune cost only if a spell has PowerType == POWER_RUNES
        if (power.powerType == POWER_RUNES)
        {
            SpellCastResult failReason = CheckRuneCost(m_spellInfo->RuneCostID);
            if (failReason != SPELL_CAST_OK)
                return failReason;
        }

        // Check power amount
        Powers powerType = Powers(power.powerType);
        if (int32(m_caster->GetPower(powerType)) < m_powerCost)
            return SPELL_FAILED_NO_POWER;

        if (powerType == POWER_HOLY_POWER)
        {
            m_powerCost = m_caster->HandleHolyPowerCost(m_powerCost, m_spellInfo->PowerCost);
            if(m_powerCost)
                if (Player* modOwner = m_caster->GetSpellModOwner())
                    modOwner->ApplySpellMod(m_spellInfo->Id, SPELLMOD_COST, m_powerCost);
        }
    }
    else if (!GetSpellInfo()->NoPower())
    {
        if (m_powerCost)
            return SPELL_FAILED_NO_POWER;
    }

    return SPELL_CAST_OK;
}

SpellCastResult Spell::CheckItems()
{
    if (m_caster->GetTypeId() != TYPEID_PLAYER)
        return SPELL_CAST_OK;

    Player* p_caster = (Player*)m_caster;

    if (!m_CastItem)
    {
        if (m_castItemGUID)
            return SPELL_FAILED_ITEM_NOT_READY;
    }
    else
    {
        uint32 itemid = m_CastItem->GetEntry();
        if (!p_caster->HasItemCount(itemid))
            return SPELL_FAILED_ITEM_NOT_READY;

        ItemTemplate const* proto = m_CastItem->GetTemplate();
        if (!proto)
            return SPELL_FAILED_ITEM_NOT_READY;

        for (int i = 0; i < MAX_ITEM_SPELLS; ++i)
            if (proto->Spells[i].SpellCharges)
                if (m_CastItem->GetSpellCharges(i) == 0)
                    return SPELL_FAILED_NO_CHARGES_REMAIN;

        // consumable cast item checks
        if (proto->Class == ITEM_CLASS_CONSUMABLE && m_targets.GetUnitTarget())
        {
            // such items should only fail if there is no suitable effect at all - see Rejuvenation Potions for example
            SpellCastResult failReason = SPELL_CAST_OK;
            for (int i = 0; i < MAX_SPELL_EFFECTS; i++)
            {
                    // skip check, pet not required like checks, and for TARGET_UNIT_PET m_targets.GetUnitTarget() is not the real target but the caster
                    if (m_spellInfo->GetEffect(i, m_diffMode)->TargetA.GetTarget() == TARGET_UNIT_PET)
                    continue;

                if (m_spellInfo->GetEffect(i, m_diffMode)->Effect == SPELL_EFFECT_HEAL)
                {
                    if (m_targets.GetUnitTarget()->IsFullHealth())
                    {
                        failReason = SPELL_FAILED_ALREADY_AT_FULL_HEALTH;
                        continue;
                    }
                    else
                    {
                        failReason = SPELL_CAST_OK;
                        break;
                    }
                }

                // Mana Potion, Rage Potion, Thistle Tea(Rogue), ...
                if (m_spellInfo->GetEffect(i, m_diffMode)->Effect == SPELL_EFFECT_ENERGIZE)
                {
                    if (m_spellInfo->GetEffect(i, m_diffMode)->MiscValue < 0 || m_spellInfo->GetEffect(i, m_diffMode)->MiscValue >= int8(MAX_POWERS))
                    {
                        failReason = SPELL_FAILED_ALREADY_AT_FULL_POWER;
                        continue;
                    }

                    Powers power = Powers(m_spellInfo->GetEffect(i, m_diffMode)->MiscValue);
                    if (m_targets.GetUnitTarget()->GetPower(power) == m_targets.GetUnitTarget()->GetMaxPower(power))
                    {
                        failReason = SPELL_FAILED_ALREADY_AT_FULL_POWER;
                        continue;
                    }
                    else
                    {
                        failReason = SPELL_CAST_OK;
                        break;
                    }
                }
            }
            if (failReason != SPELL_CAST_OK)
                return failReason;
        }
    }

    // check target item
    if (m_targets.GetItemTargetGUID())
    {
        if (m_caster->GetTypeId() != TYPEID_PLAYER)
            return SPELL_FAILED_BAD_TARGETS;

        if (!m_targets.GetItemTarget())
            return SPELL_FAILED_ITEM_GONE;

        if (!m_targets.GetItemTarget()->IsFitToSpellRequirements(m_spellInfo))
            return SPELL_FAILED_EQUIPPED_ITEM_CLASS;
    }
    // if not item target then required item must be equipped
    else
    {
        if (m_caster->GetTypeId() == TYPEID_PLAYER && !m_caster->ToPlayer()->HasItemFitToSpellRequirements(m_spellInfo))
            return SPELL_FAILED_EQUIPPED_ITEM_CLASS;
    }

    // check spell focus object
    if (m_spellInfo->RequiresSpellFocus)
    {
        CellCoord p(Trinity::ComputeCellCoord(m_caster->GetPositionX(), m_caster->GetPositionY()));
        Cell cell(p);

        GameObject* ok = NULL;
        Trinity::GameObjectFocusCheck go_check(m_caster, m_spellInfo->RequiresSpellFocus);
        Trinity::GameObjectSearcher<Trinity::GameObjectFocusCheck> checker(m_caster, ok, go_check);

        TypeContainerVisitor<Trinity::GameObjectSearcher<Trinity::GameObjectFocusCheck>, GridTypeMapContainer > object_checker(checker);
        Map& map = *m_caster->GetMap();
        cell.Visit(p, object_checker, map, *m_caster, m_caster->GetVisibilityRange());

        if (!ok)
            return SPELL_FAILED_REQUIRES_SPELL_FOCUS;

        focusObject = ok;                                   // game object found in range
    }

    // do not take reagents for these item casts
    if (!(m_CastItem && m_CastItem->GetTemplate()->Flags & ITEM_PROTO_FLAG_TRIGGERED_CAST))
    {
        bool checkReagents = !(_triggeredCastFlags & TRIGGERED_IGNORE_POWER_AND_REAGENT_COST) && !p_caster->CanNoReagentCast(m_spellInfo);
        // Not own traded item (in trader trade slot) requires reagents even if triggered spell
        if (!checkReagents)
            if (Item* targetItem = m_targets.GetItemTarget())
                if (targetItem->GetOwnerGUID() != m_caster->GetGUID())
                    checkReagents = true;

        // check reagents (ignore triggered spells with reagents processed by original spell) and special reagent ignore case.
        if (checkReagents)
        {
            for (uint32 i = 0; i < MAX_SPELL_REAGENTS; i++)
            {
                if (m_spellInfo->Reagent[i] <= 0)
                    continue;

                uint32 itemid    = m_spellInfo->Reagent[i];
                uint32 itemcount = m_spellInfo->ReagentCount[i];

                // if CastItem is also spell reagent
                if (m_CastItem && m_CastItem->GetEntry() == itemid)
                {
                    ItemTemplate const* proto = m_CastItem->GetTemplate();
                    if (!proto)
                        return SPELL_FAILED_ITEM_NOT_READY;
                    for (int s=0; s < MAX_ITEM_PROTO_SPELLS; ++s)
                    {
                        // CastItem will be used up and does not count as reagent
                        int32 charges = m_CastItem->GetSpellCharges(s);
                        if (proto->Spells[s].SpellCharges < 0 && abs(charges) < 2)
                        {
                            ++itemcount;
                            break;
                        }
                    }
                }
                if (!p_caster->HasItemCount(itemid, itemcount))
                    return SPELL_FAILED_REAGENTS;
            }
            if(m_spellInfo->ReagentCurrency != 0)
            {
                uint32 currencyId    = m_spellInfo->ReagentCurrency;
                uint32 currencyCount = m_spellInfo->ReagentCurrencyCount;

                if (!p_caster->HasCurrency(currencyId, currencyCount))
                    return SPELL_FAILED_REAGENTS;
            }
        }

        // check totem-item requirements (items presence in inventory)
        uint32 totems = 2;
        for (int i = 0; i < 2; ++i)
        {
            if (m_spellInfo->Totem[i] != 0)
            {
                if (p_caster->HasItemCount(m_spellInfo->Totem[i]))
                {
                    totems -= 1;
                    continue;
                }
            }else
            totems -= 1;
        }
        if (totems != 0)
            return SPELL_FAILED_TOTEMS;
    }

    // special checks for spell effects
    for (int i = 0; i < MAX_SPELL_EFFECTS; i++)
    {
        switch (m_spellInfo->GetEffect(i, m_diffMode)->Effect)
        {
            case SPELL_EFFECT_CREATE_ITEM:
            case SPELL_EFFECT_CREATE_ITEM_2:
            {
                if (!IsTriggered() && m_spellInfo->GetEffect(i, m_diffMode)->ItemType)
                {
                    ItemPosCountVec dest;
                    InventoryResult msg = p_caster->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, m_spellInfo->GetEffect(i, m_diffMode)->ItemType, 1);
                    if (msg != EQUIP_ERR_OK)
                    {
                        ItemTemplate const* pProto = sObjectMgr->GetItemTemplate(m_spellInfo->GetEffect(i, m_diffMode)->ItemType);
                        // TODO: Needs review
                        if (pProto && !(pProto->ItemLimitCategory))
                        {
                            p_caster->SendEquipError(msg, NULL, NULL, m_spellInfo->GetEffect(i, m_diffMode)->ItemType);
                            return SPELL_FAILED_DONT_REPORT;
                        }
                        else
                        {
                            if (!(m_spellInfo->SpellFamilyName == SPELLFAMILY_MAGE && (m_spellInfo->SpellFamilyFlags[0] & 0x40000000)))
                                return SPELL_FAILED_TOO_MANY_OF_ITEM;
                            else if (!(p_caster->HasItemCount(m_spellInfo->GetEffect(i, m_diffMode)->ItemType)))
                                return SPELL_FAILED_TOO_MANY_OF_ITEM;
                            // Conjure Mana Gem
                            else if (m_spellInfo->Id == 759)
                                p_caster->CastSpell(m_caster, 54408, false);
                            // Conjure Mana Gem
                            else if (m_spellInfo->Id == 119316)
                                p_caster->CastSpell(m_caster, 119318, false);
                            // Conjure Healthstone
                            else if (m_spellInfo->Id == 23517)
                                p_caster->CastSpell(m_caster, 120038, false);
                            return SPELL_FAILED_DONT_REPORT;
                        }
                    }
                }
                break;
            }
            case SPELL_EFFECT_CREATE_ITEM_3:
            {
                if(ItemTemplate const* itemProto = m_targets.GetItemTarget()->GetTemplate())
                {
                    if(p_caster->CanUseItem(itemProto) != EQUIP_ERR_OK)
                        return SPELL_FAILED_BAD_TARGETS;
                    if (!(itemProto->Flags & ITEM_PROTO_FLAG_UNK4) || itemProto->ItemLevel != 496 || itemProto->ItemId == 104347) //Timeless Curio cannot be target
                        return SPELL_FAILED_BAD_TARGETS;
                }
                else
                    return SPELL_FAILED_BAD_TARGETS;
                break;
            }
            case SPELL_EFFECT_ENCHANT_ITEM:
                if (m_spellInfo->GetEffect(i, m_diffMode)->ItemType && m_targets.GetItemTarget()
                    && (m_targets.GetItemTarget()->IsVellum()))
                {
                    // cannot enchant vellum for other player
                    if (m_targets.GetItemTarget()->GetOwner() != m_caster)
                        return SPELL_FAILED_NOT_TRADEABLE;
                    // do not allow to enchant vellum from scroll made by vellum-prevent exploit
                    if (m_CastItem && m_CastItem->GetTemplate()->Flags & ITEM_PROTO_FLAG_TRIGGERED_CAST)
                        return SPELL_FAILED_TOTEM_CATEGORY;
                    ItemPosCountVec dest;
                    InventoryResult msg = p_caster->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, m_spellInfo->GetEffect(i, m_diffMode)->ItemType, 1);
                    if (msg != EQUIP_ERR_OK)
                    {
                        p_caster->SendEquipError(msg, NULL, NULL, m_spellInfo->GetEffect(i, m_diffMode)->ItemType);
                        return SPELL_FAILED_DONT_REPORT;
                    }
                }
            case SPELL_EFFECT_ENCHANT_ITEM_PRISMATIC:
            {
                Item* targetItem = m_targets.GetItemTarget();
                if (!targetItem)
                    return SPELL_FAILED_ITEM_NOT_FOUND;

                if (targetItem->GetTemplate()->ItemLevel < m_spellInfo->BaseLevel)
                    return SPELL_FAILED_LOWLEVEL;

                bool isItemUsable = false;
                for (uint8 e = 0; e < MAX_ITEM_PROTO_SPELLS; ++e)
                {
                    ItemTemplate const* proto = targetItem->GetTemplate();
                    if (proto->Spells[e].SpellId && (
                        proto->Spells[e].SpellTrigger == ITEM_SPELLTRIGGER_ON_USE ||
                        proto->Spells[e].SpellTrigger == ITEM_SPELLTRIGGER_ON_NO_DELAY_USE))
                    {
                        isItemUsable = true;
                        break;
                    }
                }

                SpellItemEnchantmentEntry const* pEnchant = sSpellItemEnchantmentStore.LookupEntry(m_spellInfo->GetEffect(i, m_diffMode)->MiscValue);
                // do not allow adding usable enchantments to items that have use effect already
                if (pEnchant && isItemUsable)
                    for (uint8 s = 0; s < MAX_ITEM_ENCHANTMENT_EFFECTS; ++s)
						if (pEnchant->Effect[s] == ITEM_ENCHANTMENT_TYPE_USE_SPELL)
                            return SPELL_FAILED_ON_USE_ENCHANT;

                // Not allow enchant in trade slot for some enchant type
                if (targetItem->GetOwner() != m_caster)
                {
                    if (!pEnchant)
                        return SPELL_FAILED_ERROR;
					if (pEnchant->Flags & ENCHANTMENT_CAN_SOULBOUND)
                        return SPELL_FAILED_NOT_TRADEABLE;
                }
                break;
            }
            case SPELL_EFFECT_ENCHANT_ITEM_TEMPORARY:
            {
                Item* item = m_targets.GetItemTarget();
                if (!item)
                    return SPELL_FAILED_ITEM_NOT_FOUND;
                // Not allow enchant in trade slot for some enchant type
                if (item->GetOwner() != m_caster)
                {
                    uint32 enchant_id = m_spellInfo->GetEffect(i, m_diffMode)->MiscValue;
                    SpellItemEnchantmentEntry const* pEnchant = sSpellItemEnchantmentStore.LookupEntry(enchant_id);
                    if (!pEnchant)
                        return SPELL_FAILED_ERROR;
					if (pEnchant->Flags & ENCHANTMENT_CAN_SOULBOUND)
                        return SPELL_FAILED_NOT_TRADEABLE;
                }
                break;
            }
            case SPELL_EFFECT_ENCHANT_HELD_ITEM:
                // check item existence in effect code (not output errors at offhand hold item effect to main hand for example
                break;
            case SPELL_EFFECT_DISENCHANT:
            {
                if (!m_targets.GetItemTarget())
                    return SPELL_FAILED_CANT_BE_DISENCHANTED;

                // prevent disenchanting in trade slot
                if (m_targets.GetItemTarget()->GetOwnerGUID() != m_caster->GetGUID())
                    return SPELL_FAILED_CANT_BE_DISENCHANTED;

                ItemTemplate const* itemProto = m_targets.GetItemTarget()->GetTemplate();
                if (!itemProto)
                    return SPELL_FAILED_CANT_BE_DISENCHANTED;

                uint32 item_quality = itemProto->Quality;
                // 2.0.x addon: Check player enchanting level against the item disenchanting requirements
                uint32 item_disenchantskilllevel = itemProto->RequiredDisenchantSkill;
                if (item_disenchantskilllevel == uint32(-1))
                    return SPELL_FAILED_CANT_BE_DISENCHANTED;
                if (item_disenchantskilllevel > p_caster->GetSkillValue(SKILL_ENCHANTING))
                    return SPELL_FAILED_LOW_CASTLEVEL;
                if (item_quality > 4 || item_quality < 2)
                    return SPELL_FAILED_CANT_BE_DISENCHANTED;
                if (itemProto->Class != ITEM_CLASS_WEAPON && itemProto->Class != ITEM_CLASS_ARMOR)
                    return SPELL_FAILED_CANT_BE_DISENCHANTED;
                if (!itemProto->DisenchantID)
                    return SPELL_FAILED_CANT_BE_DISENCHANTED;
                break;
            }
            case SPELL_EFFECT_PROSPECTING:
            {
                if (!m_targets.GetItemTarget())
                    return SPELL_FAILED_CANT_BE_PROSPECTED;
                //ensure item is a prospectable ore
                if (!(m_targets.GetItemTarget()->GetTemplate()->Flags & ITEM_PROTO_FLAG_PROSPECTABLE))
                    return SPELL_FAILED_CANT_BE_PROSPECTED;
                //prevent prospecting in trade slot
                if (m_targets.GetItemTarget()->GetOwnerGUID() != m_caster->GetGUID())
                    return SPELL_FAILED_CANT_BE_PROSPECTED;
                //Check for enough skill in jewelcrafting
                uint32 item_prospectingskilllevel = m_targets.GetItemTarget()->GetTemplate()->RequiredSkillRank;
                if (item_prospectingskilllevel >p_caster->GetSkillValue(SKILL_JEWELCRAFTING))
                    return SPELL_FAILED_LOW_CASTLEVEL;
                //make sure the player has the required ores in inventory
                if (m_targets.GetItemTarget()->GetCount() < 5)
                    return SPELL_FAILED_NEED_MORE_ITEMS;

                if (!LootTemplates_Prospecting.HaveLootFor(m_targets.GetItemTargetEntry()))
                    return SPELL_FAILED_CANT_BE_PROSPECTED;

                break;
            }
            case SPELL_EFFECT_MILLING:
            {
                if (!m_targets.GetItemTarget())
                    return SPELL_FAILED_CANT_BE_MILLED;
                //ensure item is a millable herb
                if (!(m_targets.GetItemTarget()->GetTemplate()->Flags & ITEM_PROTO_FLAG_MILLABLE))
                    return SPELL_FAILED_CANT_BE_MILLED;
                //prevent milling in trade slot
                if (m_targets.GetItemTarget()->GetOwnerGUID() != m_caster->GetGUID())
                    return SPELL_FAILED_CANT_BE_MILLED;
                //Check for enough skill in inscription
                uint32 item_millingskilllevel = m_targets.GetItemTarget()->GetTemplate()->RequiredSkillRank;
                if (item_millingskilllevel >p_caster->GetSkillValue(SKILL_INSCRIPTION))
                    return SPELL_FAILED_LOW_CASTLEVEL;
                //make sure the player has the required herbs in inventory
                if (m_targets.GetItemTarget()->GetCount() < 5)
                    return SPELL_FAILED_NEED_MORE_ITEMS;

                if (!LootTemplates_Milling.HaveLootFor(m_targets.GetItemTargetEntry()))
                    return SPELL_FAILED_CANT_BE_MILLED;

                break;
            }
            case SPELL_EFFECT_WEAPON_DAMAGE:
            case SPELL_EFFECT_WEAPON_DAMAGE_NOSCHOOL:
            {
                if (m_caster->GetTypeId() != TYPEID_PLAYER)
                    return SPELL_FAILED_TARGET_NOT_PLAYER;

                if (m_attackType != RANGED_ATTACK)
                    break;

                Item* pItem = m_caster->ToPlayer()->GetWeaponForAttack(m_attackType);
                if (!pItem || pItem->IsBroken())
                    return SPELL_FAILED_EQUIPPED_ITEM;

                switch (pItem->GetTemplate()->SubClass)
                {
                    case ITEM_SUBCLASS_WEAPON_THROWN:
                    {
                        uint32 ammo = pItem->GetEntry();
                        if (!m_caster->ToPlayer()->HasItemCount(ammo))
                            return SPELL_FAILED_NO_AMMO;
                    };
                    break;
                    case ITEM_SUBCLASS_WEAPON_GUN:
                    case ITEM_SUBCLASS_WEAPON_BOW:
                    case ITEM_SUBCLASS_WEAPON_CROSSBOW:
                    case ITEM_SUBCLASS_WEAPON_WAND:
                        break;
                    default:
                        break;
                }
                break;
            }
            case SPELL_EFFECT_CREATE_MANA_GEM:
            {
                 uint32 item_id = m_spellInfo->GetEffect(i, m_diffMode)->ItemType;
                 ItemTemplate const* pProto = sObjectMgr->GetItemTemplate(item_id);

                 if (!pProto)
                     return SPELL_FAILED_ITEM_AT_MAX_CHARGES;

                 if (Item* pitem = p_caster->GetItemByEntry(item_id))
                 {
                     for (int x = 0; x < MAX_ITEM_PROTO_SPELLS; ++x)
                         if (pProto->Spells[x].SpellCharges != 0 && pitem->GetSpellCharges(x) == pProto->Spells[x].SpellCharges)
                             return SPELL_FAILED_ITEM_AT_MAX_CHARGES;
                 }
                 break;
            }
            default:
                break;
        }
    }

    // check weapon presence in slots for main/offhand weapons
    if (m_spellInfo->EquippedItemClass >=0)
    {
        // main hand weapon required
        if (AttributesCustomEx3 & SPELL_ATTR3_MAIN_HAND)
        {
            Item* item = m_caster->ToPlayer()->GetWeaponForAttack(BASE_ATTACK);

            // skip spell if no weapon in slot or broken
            if (!item || item->IsBroken())
                return (_triggeredCastFlags & TRIGGERED_DONT_REPORT_CAST_ERROR) ? SPELL_FAILED_DONT_REPORT : SPELL_FAILED_EQUIPPED_ITEM_CLASS;

            // skip spell if weapon not fit to triggered spell
            if (!item->IsFitToSpellRequirements(m_spellInfo))
                return (_triggeredCastFlags & TRIGGERED_DONT_REPORT_CAST_ERROR) ? SPELL_FAILED_DONT_REPORT : SPELL_FAILED_EQUIPPED_ITEM_CLASS;
        }

        // offhand hand weapon required
        if (AttributesCustomEx3 & SPELL_ATTR3_REQ_OFFHAND)
        {
            Item* item = m_caster->ToPlayer()->GetWeaponForAttack(OFF_ATTACK);

            // skip spell if no weapon in slot or broken
            if (!item || item->IsBroken())
                return (_triggeredCastFlags & TRIGGERED_DONT_REPORT_CAST_ERROR) ? SPELL_FAILED_DONT_REPORT : SPELL_FAILED_EQUIPPED_ITEM_CLASS;

            // skip spell if weapon not fit to triggered spell
            if (!item->IsFitToSpellRequirements(m_spellInfo))
                return (_triggeredCastFlags & TRIGGERED_DONT_REPORT_CAST_ERROR) ? SPELL_FAILED_DONT_REPORT : SPELL_FAILED_EQUIPPED_ITEM_CLASS;
        }
    }

    return SPELL_CAST_OK;
}

void Spell::Delayed() // only called in DealDamage()
{
    if (!m_caster)// || m_caster->GetTypeId() != TYPEID_PLAYER)
        return;

    //if (m_spellState == SPELL_STATE_DELAYED)
    //    return;                                             // spell is active and can't be time-backed

    if (isDelayableNoMore())                                 // Spells may only be delayed twice
        return;

    // spells not loosing casting time (slam, dynamites, bombs..)
    //if (!(m_spellInfo->InterruptFlags & SPELL_INTERRUPT_FLAG_DAMAGE))
    //    return;

    //check pushback reduce
    int32 delaytime = 500;                                  // spellcasting delay is normally 500ms
    int32 delayReduce = 100;                                // must be initialized to 100 for percent modifiers
    m_caster->ToPlayer()->ApplySpellMod(m_spellInfo->Id, SPELLMOD_NOT_LOSE_CASTING_TIME, delayReduce, this);
    delayReduce += m_caster->GetTotalAuraModifier(SPELL_AURA_REDUCE_PUSHBACK) - 100;
    if (delayReduce >= 100)
        return;

    AddPct(delaytime, -delayReduce);

    if (m_timer + delaytime > m_casttime)
    {
        delaytime = m_casttime - m_timer;
        m_timer = m_casttime;
    }
    else
        m_timer += delaytime;

    sLog->outInfo(LOG_FILTER_SPELLS_AURAS, "Spell %u partially interrupted for (%d) ms at damage", m_spellInfo->Id, delaytime);

    ObjectGuid guid = m_caster->GetGUID();

    //! 5.4.1
    WorldPacket data(SMSG_SPELL_DELAYED, 8+4);
    data << uint32(delaytime);
    data.WriteGuidMask<3, 2, 5, 4, 0, 7, 1, 6>(guid);
    data.WriteGuidBytes<1, 5, 0, 2, 4, 6, 3, 7>(guid);

    m_caster->SendMessageToSet(&data, true);
}

void Spell::DelayedChannel()
{
    if (!m_caster || m_caster->GetTypeId() != TYPEID_PLAYER || getState() != SPELL_STATE_CASTING)
        return;

    if (isDelayableNoMore())                                    // Spells may only be delayed twice
        return;

    //check pushback reduce
    int32 delaytime = CalculatePct(m_spellInfo->GetDuration(), 25); // channeling delay is normally 25% of its time per hit
    int32 delayReduce = 100;                                    // must be initialized to 100 for percent modifiers
    m_caster->ToPlayer()->ApplySpellMod(m_spellInfo->Id, SPELLMOD_NOT_LOSE_CASTING_TIME, delayReduce, this);
    delayReduce += m_caster->GetTotalAuraModifier(SPELL_AURA_REDUCE_PUSHBACK) - 100;
    if (delayReduce >= 100)
        return;

    AddPct(delaytime, -delayReduce);

    if (m_timer <= delaytime)
    {
        delaytime = m_timer;
        m_timer = 0;
    }
    else
        m_timer -= delaytime;

    sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "Spell %u partially interrupted for %i ms, new duration: %u ms", m_spellInfo->Id, delaytime, m_timer);

    for (std::list<TargetInfo>::const_iterator ihit = m_UniqueTargetInfo.begin(); ihit != m_UniqueTargetInfo.end(); ++ihit)
        if ((*ihit).missCondition == SPELL_MISS_NONE)
            if (Unit* unit = (m_caster->GetGUID() == ihit->targetGUID) ? m_caster : ObjectAccessor::GetUnit(*m_caster, ihit->targetGUID))
                unit->DelayOwnedAuras(m_spellInfo->Id, m_originalCasterGUID, delaytime);

    // partially interrupt persistent area auras
    if (DynamicObject* dynObj = m_caster->GetDynObject(m_spellInfo->Id))
        dynObj->Delay(delaytime);

    SendChannelUpdate(m_timer);
}

void Spell::UpdatePointers()
{
    if (m_originalCasterGUID == m_caster->GetGUID())
        m_originalCaster = m_caster;
    else
    {
        m_originalCaster = ObjectAccessor::GetUnit(*m_caster, m_originalCasterGUID);
        if (m_originalCaster && !m_originalCaster->IsInWorld())
            m_originalCaster = NULL;
    }

    if (m_castItemGUID && m_caster->GetTypeId() == TYPEID_PLAYER)
        m_CastItem = m_caster->ToPlayer()->GetItemByGuid(m_castItemGUID);

    m_targets.Update(m_caster);

    // further actions done only for dest targets
    if (!m_targets.HasDst())
        return;

    // cache last transport
    WorldObject* transport = NULL;

    // update effect destinations (in case of moved transport dest target)
    for (uint8 effIndex = 0; effIndex < MAX_SPELL_EFFECTS; ++effIndex)
    {
        SpellDestination& dest = m_destTargets[effIndex];
        if (!dest._transportGUID)
            continue;

        if (!transport || transport->GetGUID() != dest._transportGUID)
            transport = ObjectAccessor::GetWorldObject(*m_caster, dest._transportGUID);

        if (transport)
        {
            dest._position.Relocate(transport);
            dest._position.RelocateOffset(dest._transportOffset);
        }
    }
}

CurrentSpellTypes Spell::GetCurrentContainer() const
{
    if (IsNextMeleeSwingSpell())
        return(CURRENT_MELEE_SPELL);
    else if (IsAutoRepeat())
        return(CURRENT_AUTOREPEAT_SPELL);
    else if (m_spellInfo->IsChanneled())
        return(CURRENT_CHANNELED_SPELL);
    else
        return(CURRENT_GENERIC_SPELL);
}

bool Spell::CheckEffectTarget(Unit const* target, uint32 eff) const
{
    switch (m_spellInfo->Effects[eff].ApplyAuraName)
    {
        case SPELL_AURA_MOD_POSSESS:
        case SPELL_AURA_MOD_CHARM:
        case SPELL_AURA_MOD_POSSESS_PET:
        case SPELL_AURA_AOE_CHARM:
            if (m_caster->GetTypeId() == TYPEID_PLAYER && target->GetTypeId() == TYPEID_UNIT && target->IsVehicle())
                return false;
            if (target->IsMounted())
                return false;
            if (target->GetCharmerGUID())
                return false;
            if (int32 damage = CalculateDamage(eff, target))
                if ((int32)target->getLevel() > damage)
                    return false;
            break;
        default:
            break;
    }
    
    // Hour of Twilight
    if (m_spellInfo->Id == 103327)
    {
        // Deterrence
        if (target->HasAura(19263))
            return false;
    }
    if (canHitTargetInLOS) // Fists of Fury can't target not in LOS
        return true;

    // todo: shit below shouldn't be here, but it's temporary
    //Check targets for LOS visibility (except spells without range limitations)
    switch (m_spellInfo->Effects[eff].Effect)
    {
        case SPELL_EFFECT_RESURRECT_NEW:
            // player far away, maybe his corpse near?
            if (target != m_caster && !target->IsWithinLOSInMap(m_caster))
            {
                if (!m_targets.GetCorpseTargetGUID())
                    return false;

                Corpse* corpse = ObjectAccessor::GetCorpse(*m_caster, m_targets.GetCorpseTargetGUID());
                if (!corpse)
                    return false;

                if (target->GetGUID() != corpse->GetOwnerGUID())
                    return false;

                if (!corpse->IsWithinLOSInMap(m_caster))
                    return false;
            }

            // all ok by some way or another, skip normal check
            break;
        default:                                            // normal case
            // Get GO cast coordinates if original caster -> GO
            WorldObject* caster = NULL;
            if (IS_GAMEOBJECT_GUID(m_originalCasterGUID))
                caster = m_caster->GetMap()->GetGameObject(m_originalCasterGUID);
            if (!caster)
                caster = m_caster;

            if (m_targets.HasDst())
            {
                if (SpellDestination const* dest = m_targets.GetDst())
                {
                    float x = dest->_position.GetPositionX();
                    float y = dest->_position.GetPositionY();
                    float z = dest->_position.GetPositionZ();

                    if (!target->IsWithinLOS(x, y, z))
                        return false;
                }
            }
            else if (target != m_caster && !target->IsWithinLOSInMap(caster))
                return false;
            break;
    }

    return true;
}

bool Spell::IsNextMeleeSwingSpell() const
{
    return AttributesCustom & SPELL_ATTR0_ON_NEXT_SWING;
}

bool Spell::IsAutoActionResetSpell() const
{
    // TODO: changed SPELL_INTERRUPT_FLAG_AUTOATTACK -> SPELL_INTERRUPT_FLAG_INTERRUPT to fix compile - is this check correct at all?
    return !IsTriggered() && (m_spellInfo->InterruptFlags & SPELL_INTERRUPT_FLAG_INTERRUPT);
}

bool Spell::IsNeedSendToClient() const
{
    return m_spellInfo->SpellVisual[0] || m_spellInfo->SpellVisual[1] || m_spellInfo->IsChanneled() ||
        (AttributesCustomEx8 & SPELL_ATTR8_AURA_SEND_AMOUNT) || m_spellInfo->Speed > 0.0f || (!m_triggeredByAuraSpell && !IsTriggered());
}

bool Spell::HaveTargetsForEffect(uint8 effect) const
{
    for (std::list<TargetInfo>::const_iterator itr = m_UniqueTargetInfo.begin(); itr != m_UniqueTargetInfo.end(); ++itr)
        if (itr->effectMask & (1 << effect))
            return true;

    for (std::list<GOTargetInfo>::const_iterator itr = m_UniqueGOTargetInfo.begin(); itr != m_UniqueGOTargetInfo.end(); ++itr)
        if (itr->effectMask & (1 << effect))
            return true;

    for (std::list<ItemTargetInfo>::const_iterator itr = m_UniqueItemInfo.begin(); itr != m_UniqueItemInfo.end(); ++itr)
        if (itr->effectMask & (1 << effect))
            return true;

    return false;
}

SpellEvent::SpellEvent(Spell* spell) : BasicEvent()
{
    m_Spell = spell;
}

SpellEvent::~SpellEvent()
{
    if (m_Spell->getState() != SPELL_STATE_FINISHED)
        m_Spell->cancel();

    if (m_Spell->IsDeletable())
    {
        delete m_Spell;
    }
    else
    {
        sLog->outError(LOG_FILTER_SPELLS_AURAS, "~SpellEvent: %s %u tried to delete non-deletable spell %u. Was not deleted, causes memory leak.",
            (m_Spell->GetCaster()->GetTypeId() == TYPEID_PLAYER ? "Player" : "Creature"), m_Spell->GetCaster()->GetGUIDLow(), m_Spell->m_spellInfo->Id);
        //ASSERT(false);
    }
}

bool SpellEvent::Execute(uint64 e_time, uint32 p_time)
{
    // update spell if it is not finished
    if (m_Spell->getState() != SPELL_STATE_FINISHED)
        m_Spell->update(p_time);

    // check spell state to process
    switch (m_Spell->getState())
    {
        case SPELL_STATE_FINISHED:
        {
            // spell was finished, check deletable state
            if (m_Spell->IsDeletable())
            {
                // check, if we do have unfinished triggered spells
                return true;                                // spell is deletable, finish event
            }
            // event will be re-added automatically at the end of routine)
        } break;

        case SPELL_STATE_DELAYED:
        {
            // first, check, if we have just started
            if (m_Spell->GetDelayStart() != 0)
            {
                // no, we aren't, do the typical update
                // check, if we have channeled spell on our hands
                /*
                if (m_Spell->m_spellInfo->IsChanneled())
                {
                    // evented channeled spell is processed separately, casted once after delay, and not destroyed till finish
                    // check, if we have casting anything else except this channeled spell and autorepeat
                    if (m_Spell->GetCaster()->IsNonMeleeSpellCasted(false, true, true))
                    {
                        // another non-melee non-delayed spell is casted now, abort
                        m_Spell->cancel();
                    }
                    else
                    {
                        // Set last not triggered spell for apply spellmods
                        ((Player*)m_Spell->GetCaster())->SetSpellModTakingSpell(m_Spell, true);
                        // do the action (pass spell to channeling state)
                        m_Spell->handle_immediate();

                        // And remove after effect handling
                        ((Player*)m_Spell->GetCaster())->SetSpellModTakingSpell(m_Spell, false);
                    }
                    // event will be re-added automatically at the end of routine)
                }
                else
                */
                {
                    // run the spell handler and think about what we can do next
                    uint64 t_offset = e_time - m_Spell->GetDelayStart();
                    uint64 n_offset = m_Spell->handle_delayed(t_offset);
                    if (n_offset)
                    {
                        // re-add us to the queue
                        m_Spell->GetCaster()->m_Events.AddEvent(this, m_Spell->GetDelayStart() + n_offset, false);
                        return false;                       // event not complete
                    }
                    // event complete
                    // finish update event will be re-added automatically at the end of routine)
                }
            }
            else
            {
                // delaying had just started, record the moment
                m_Spell->SetDelayStart(e_time);
                // re-plan the event for the delay moment
                m_Spell->GetCaster()->m_Events.AddEvent(this, e_time + m_Spell->GetDelayMoment(), false);
                return false;                               // event not complete
            }
        } break;

        default:
        {
            // all other states
            // event will be re-added automatically at the end of routine)
        } break;
    }

    // spell processing not complete, plan event on the next update interval
    m_Spell->GetCaster()->m_Events.AddEvent(this, e_time + 1, false);
    return false;                                           // event not complete
}

void SpellEvent::Abort(uint64 /*e_time*/)
{
    // oops, the spell we try to do is aborted
    if (m_Spell->getState() != SPELL_STATE_FINISHED)
        m_Spell->cancel();
}

bool SpellEvent::IsDeletable() const
{
    return m_Spell->IsDeletable();
}

bool Spell::IsValidDeadOrAliveTarget(Unit const* target) const
{
    if (target->isAlive())
        return !m_spellInfo->IsRequiringDeadTarget();
    if (m_spellInfo->IsAllowingDeadTarget())
        return true;
    if (AttributesCustomEx2 & SPELL_ATTR2_CAN_TARGET_DEAD)
        return true;
    return false;
}

void Spell::HandleLaunchPhase()
{
    // handle effects with SPELL_EFFECT_HANDLE_LAUNCH mode
    for (uint32 i = 0; i < MAX_SPELL_EFFECTS; ++i)
    {
        // don't do anything for empty effect
        if (!m_spellInfo->GetEffect(i, m_diffMode)->IsEffect())
            continue;

        if (m_spellMissMask & ((1 << SPELL_MISS_MISS) | (1 << SPELL_MISS_IMMUNE)))
            if (m_spellInfo->GetEffect(i, m_diffMode)->TargetA.GetTarget() == TARGET_UNIT_CASTER)
                continue;

        HandleEffects(NULL, NULL, NULL, i, SPELL_EFFECT_HANDLE_LAUNCH);
    }

    float multiplier[MAX_SPELL_EFFECTS];
    for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
        if (m_applyMultiplierMask & (1 << i))
            multiplier[i] = m_spellInfo->GetEffect(i, m_diffMode)->CalcDamageMultiplier(m_originalCaster, this);

    bool usesAmmo = AttributesCustomCu & SPELL_ATTR0_CU_DIRECT_DAMAGE;

    for (std::list<TargetInfo>::iterator ihit= m_UniqueTargetInfo.begin(); ihit != m_UniqueTargetInfo.end(); ++ihit)
    {
        TargetInfo& target = *ihit;

        uint32 mask = target.effectMask;
        if (!mask)
            continue;

        // do not consume ammo anymore for Hunter's volley spell
        if (IsTriggered() && m_spellInfo->SpellFamilyName == SPELLFAMILY_HUNTER && m_spellInfo->IsTargetingArea())
            usesAmmo = false;

        if (usesAmmo)
        {
            bool ammoTaken = false;
            for (uint8 i = 0; i < MAX_SPELL_EFFECTS; i++)
            {
                if (!(mask & 1<<i))
                    continue;
                switch (m_spellInfo->GetEffect(i, m_diffMode)->Effect)
                {
                    case SPELL_EFFECT_SCHOOL_DAMAGE:
                    case SPELL_EFFECT_WEAPON_DAMAGE:
                    case SPELL_EFFECT_WEAPON_DAMAGE_NOSCHOOL:
                    case SPELL_EFFECT_NORMALIZED_WEAPON_DMG:
                    case SPELL_EFFECT_WEAPON_PERCENT_DAMAGE:
                    ammoTaken=true;
                    TakeAmmo();
                }
                if (ammoTaken)
                    break;
            }
        }
        DoAllEffectOnLaunchTarget(target, multiplier);
    }
}

void Spell::DoAllEffectOnLaunchTarget(TargetInfo& targetInfo, float* multiplier)
{
    Unit* unit = NULL;
    TargetInfo sourceTargetInfo = targetInfo;
    // In case spell hit target, do all effect on that target
    if (targetInfo.missCondition == SPELL_MISS_NONE || targetInfo.missCondition == SPELL_MISS_PARRY || targetInfo.missCondition == SPELL_MISS_DODGE)
        unit = m_caster->GetGUID() == targetInfo.targetGUID ? m_caster : ObjectAccessor::GetUnit(*m_caster, targetInfo.targetGUID);
    // In case spell reflect from target, do all effect on caster (if hit)
    else if (targetInfo.missCondition == SPELL_MISS_REFLECT && targetInfo.reflectResult == SPELL_MISS_NONE)
        unit = m_caster;
    if (!unit)
        return;

    for (uint32 i = 0; i < MAX_SPELL_EFFECTS; ++i)
    {
        if (targetInfo.effectMask & (1<<i))
        {
            m_damage = 0;
            m_healing = 0;

            HandleEffects(unit, NULL, NULL, i, SPELL_EFFECT_HANDLE_LAUNCH_TARGET);

            if (m_damage > 0)
            {
                if (m_spellInfo->GetEffect(i, m_diffMode)->IsTargetingArea())
                {
                    m_damage = int32(float(m_damage) * unit->GetTotalAuraMultiplierByMiscMask(SPELL_AURA_MOD_AOE_DAMAGE_AVOIDANCE, m_spellInfo->SchoolMask));
                    if (m_caster->GetTypeId() == TYPEID_UNIT)
                        m_damage = int32(float(m_damage) * unit->GetTotalAuraMultiplierByMiscMask(SPELL_AURA_MOD_CREATURE_AOE_DAMAGE_AVOIDANCE, m_spellInfo->SchoolMask));

                    if (m_caster->GetTypeId() == TYPEID_PLAYER)
                    {
                        uint32 targetAmount = m_UniqueTargetInfo.size();
                        if (targetAmount > 10)
                            m_damage = m_damage * 10/targetAmount;

                        // Hack Fix Frost Bomb : Doesn't add AoE damage to main target
                        if (m_spellInfo->Id == 113092)
                            if (targetInfo.targetGUID == (*m_UniqueTargetInfo.begin()).targetGUID)
                                continue;
                    }
                }

                if (m_spellInfo->Effects[i].Effect == SPELL_EFFECT_WEAPON_PERCENT_DAMAGE)
                {
                    targetInfo.damageBeforeHit += unit->MeleeDamageBonusForDamageBeforeHit(m_caster, m_damage, m_attackType, m_spellInfo);
                    m_damage = unit->MeleeDamageBonusTaken(m_caster, m_damage, m_attackType, m_spellInfo);
                }
                else
                {
                    targetInfo.damageBeforeHit += unit->SpellDamageBonusForDamageBeforeHit(m_caster, m_spellInfo, m_damage);
                    m_damage = unit->SpellDamageBonusTaken(m_caster, m_spellInfo, m_damage);
                }
            }
            else if (m_damage < 0)
            {
                if (m_spellInfo->Id == 88686 || // Holy Word: Sanctuary
                    m_spellInfo->Id == 73921)   // Healing Rain
                {
                    if (m_caster->GetTypeId() == TYPEID_PLAYER)
                    {
                        uint32 targetAmount = m_UniqueTargetInfo.size();
                        if (targetAmount > 6)
                            m_damage = (m_damage * 6) / int32(targetAmount);
                    }
                }

                else if (m_spellInfo->Id == 82327)
                {
                    if (targetInfo.targetGUID != m_targets.GetUnitTargetGUID())
                    {
                        m_damage = CalculatePct(m_damage, m_spellInfo->Effects[0].BasePoints);
                    }
                }
            }

            if (m_applyMultiplierMask & (1 << i))
            {
                m_damage = int32(m_damage * m_damageMultipliers[i]);
                m_damageMultipliers[i] *= multiplier[i];
            }

            targetInfo.damage += m_damage;
        }
    }

    float critChance = 0.0f;

    if (targetInfo.damage)
        targetInfo.crit = (m_originalCaster ? m_originalCaster : m_caster)->isSpellCrit(unit, m_spellInfo, m_spellSchoolMask, m_attackType, critChance);

    if (targetInfo.crit)
        targetInfo.damageBeforeHit += targetInfo.damageBeforeHit;

    sourceTargetInfo.damageBeforeHit = targetInfo.damageBeforeHit;

    if (targetInfo.missCondition == SPELL_MISS_PARRY || targetInfo.missCondition == SPELL_MISS_DODGE)
    {
        m_damage = 0;
        targetInfo = sourceTargetInfo;
    }
}

SpellCastResult Spell::CanOpenLock(uint32 effIndex, uint32 lockId, SkillType& skillId, int32& reqSkillValue, int32& skillValue)
{
    if (!lockId)                                             // possible case for GO and maybe for items.
        return SPELL_CAST_OK;

    // Get LockInfo
    LockEntry const* lockInfo = sLockStore.LookupEntry(lockId);

    if (!lockInfo)
        return SPELL_FAILED_BAD_TARGETS;

    bool reqKey = false;                                    // some locks not have reqs

    for (int j = 0; j < MAX_LOCK_CASE; ++j)
    {
        switch (lockInfo->Type[j])
        {
            // check key item (many fit cases can be)
            case LOCK_KEY_ITEM:
                if (lockInfo->Index[j] && m_CastItem && m_CastItem->GetEntry() == lockInfo->Index[j])
                    return SPELL_CAST_OK;
                reqKey = true;
                break;
                // check key skill (only single first fit case can be)
            case LOCK_KEY_SKILL:
            {
                reqKey = true;

                // wrong locktype, skip
                if (uint32(m_spellInfo->GetEffect(effIndex, m_diffMode)->MiscValue) != lockInfo->Index[j])
                    continue;

                skillId = SkillByLockType(LockType(lockInfo->Index[j]));

                if (skillId != SKILL_NONE)
                {
                    reqSkillValue = lockInfo->Skill[j];

                    // castitem check: rogue using skeleton keys. the skill values should not be added in this case.
                    skillValue = m_CastItem || m_caster->GetTypeId()!= TYPEID_PLAYER ?
                        0 : m_caster->ToPlayer()->GetSkillValue(skillId);

                    // skill bonus provided by casting spell (mostly item spells)
                    // add the effect base points modifier from the spell casted (cheat lock / skeleton key etc.)
                    if (m_spellInfo->GetEffect(effIndex, m_diffMode)->TargetA.GetTarget() == TARGET_GAMEOBJECT_ITEM_TARGET || m_spellInfo->GetEffect(effIndex, m_diffMode)->TargetB.GetTarget() == TARGET_GAMEOBJECT_ITEM_TARGET)
                        skillValue += m_spellInfo->GetEffect(effIndex, m_diffMode)->CalcValue(m_caster, 0);

                    if (skillValue < reqSkillValue)
                        return SPELL_FAILED_LOW_CASTLEVEL;
                }
                return SPELL_CAST_OK;
            }
            case LOCK_KEY_SPELL:
                if ((lockInfo->Index[j] == 143917 && m_caster->HasAura(146589)) || lockInfo->Index[j] == 144229)
                    return SPELL_CAST_OK;
                reqKey = true;
                break;
        }
    }

    if (reqKey)
        return SPELL_FAILED_BAD_TARGETS;

    return SPELL_CAST_OK;
}

void Spell::SetSpellValue(SpellValueMod mod, int32 value, bool lockValue)
{
    switch (mod)
    {
        case SPELLVALUE_BASE_POINT0:
            m_spellValue->EffectBasePoints[0] = m_spellInfo->GetEffect(EFFECT_0, m_diffMode)->CalcBaseValue(value);
            m_spellValue->LockBasePoints[0] = lockValue;
            break;
        case SPELLVALUE_BASE_POINT1:
            m_spellValue->EffectBasePoints[1] = m_spellInfo->GetEffect(EFFECT_1, m_diffMode)->CalcBaseValue(value);
            m_spellValue->LockBasePoints[1] = lockValue;
            break;
        case SPELLVALUE_BASE_POINT2:
            m_spellValue->EffectBasePoints[2] = m_spellInfo->GetEffect(EFFECT_2, m_diffMode)->CalcBaseValue(value);
            m_spellValue->LockBasePoints[2] = lockValue;
            break;
        case SPELLVALUE_BASE_POINT3:
            m_spellValue->EffectBasePoints[3] = m_spellInfo->GetEffect(EFFECT_3, m_diffMode)->CalcBaseValue(value);
            m_spellValue->LockBasePoints[3] = lockValue;
            break;
        case SPELLVALUE_BASE_POINT4:
            m_spellValue->EffectBasePoints[4] = m_spellInfo->GetEffect(EFFECT_4, m_diffMode)->CalcBaseValue(value);
            m_spellValue->LockBasePoints[4] = lockValue;
            break;
        case SPELLVALUE_BASE_POINT5:
            m_spellValue->EffectBasePoints[5] = m_spellInfo->GetEffect(EFFECT_5, m_diffMode)->CalcBaseValue(value);
            m_spellValue->LockBasePoints[5] = lockValue;
            break;
        case SPELLVALUE_RADIUS_MOD:
            m_spellValue->RadiusMod = (float)value / 10000;
            break;
        case SPELLVALUE_MAX_TARGETS:
            m_spellValue->MaxAffectedTargets = (uint32)value;
            break;
        case SPELLVALUE_AURA_STACK:
            m_spellValue->AuraStackAmount = uint8(value);
            break;
    }
}

void Spell::PrepareTargetProcessing()
{
    CheckEffectExecuteData();
}

void Spell::FinishTargetProcessing()
{
    SendLogExecute();
}

void Spell::InitEffectExecuteData(uint8 effIndex)
{
    ASSERT(effIndex < MAX_SPELL_EFFECTS);
    if (!m_effectExecuteData[effIndex])
        m_effectExecuteData[effIndex] = new EffectExecuteData();
}

void Spell::CheckEffectExecuteData()
{
    for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
        ASSERT(!m_effectExecuteData[i]);
}

void Spell::LoadScripts()
{
    sScriptMgr->CreateSpellScripts(m_spellInfo->Id, m_loadedScripts);
    for (std::list<SpellScript*>::iterator itr = m_loadedScripts.begin(); itr != m_loadedScripts.end();)
    {
        if (!(*itr)->_Load(this))
        {
            std::list<SpellScript*>::iterator bitr = itr;
            ++itr;
            delete (*bitr);
            m_loadedScripts.erase(bitr);
            continue;
        }
        sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "Spell::LoadScripts: Script `%s` for spell `%u` is loaded now", (*itr)->_GetScriptName()->c_str(), m_spellInfo->Id);
        (*itr)->Register();
        ++itr;
    }
}

void Spell::CallScriptBeforeCastHandlers()
{
    for (std::list<SpellScript*>::iterator scritr = m_loadedScripts.begin(); scritr != m_loadedScripts.end(); ++scritr)
    {
        (*scritr)->_PrepareScriptCall(SPELL_SCRIPT_HOOK_BEFORE_CAST);
        std::list<SpellScript::CastHandler>::iterator hookItrEnd = (*scritr)->BeforeCast.end(), hookItr = (*scritr)->BeforeCast.begin();
        for (; hookItr != hookItrEnd; ++hookItr)
            (*hookItr).Call(*scritr);

        (*scritr)->_FinishScriptCall();
    }
}

void Spell::CallScriptBeforeStartCastHandlers()
{
    for (std::list<SpellScript*>::iterator scritr = m_loadedScripts.begin(); scritr != m_loadedScripts.end(); ++scritr)
    {
        (*scritr)->_PrepareScriptCall(SPELL_SCRIPT_HOOK_BEFORE_START_CAST);
        std::list<SpellScript::CastHandler>::iterator hookItrEnd = (*scritr)->BeforeStartCast.end(), hookItr = (*scritr)->BeforeStartCast.begin();
        for (; hookItr != hookItrEnd; ++hookItr)
            (*hookItr).Call(*scritr);

        (*scritr)->_FinishScriptCall();
    }
}

void Spell::CallScriptOnCastHandlers()
{
    for (std::list<SpellScript*>::iterator scritr = m_loadedScripts.begin(); scritr != m_loadedScripts.end(); ++scritr)
    {
        (*scritr)->_PrepareScriptCall(SPELL_SCRIPT_HOOK_ON_CAST);
        std::list<SpellScript::CastHandler>::iterator hookItrEnd = (*scritr)->OnCast.end(), hookItr = (*scritr)->OnCast.begin();
        for (; hookItr != hookItrEnd; ++hookItr)
            (*hookItr).Call(*scritr);

        (*scritr)->_FinishScriptCall();
    }
}

void Spell::CallScriptAfterCastHandlers()
{
    for (std::list<SpellScript*>::iterator scritr = m_loadedScripts.begin(); scritr != m_loadedScripts.end(); ++scritr)
    {
        (*scritr)->_PrepareScriptCall(SPELL_SCRIPT_HOOK_AFTER_CAST);
        std::list<SpellScript::CastHandler>::iterator hookItrEnd = (*scritr)->AfterCast.end(), hookItr = (*scritr)->AfterCast.begin();
        for (; hookItr != hookItrEnd; ++hookItr)
            (*hookItr).Call(*scritr);

        (*scritr)->_FinishScriptCall();
    }
}

SpellCastResult Spell::CallScriptCheckCastHandlers()
{
    SpellCastResult retVal = SPELL_CAST_OK;
    for (std::list<SpellScript*>::iterator scritr = m_loadedScripts.begin(); scritr != m_loadedScripts.end(); ++scritr)
    {
        (*scritr)->_PrepareScriptCall(SPELL_SCRIPT_HOOK_CHECK_CAST);
        std::list<SpellScript::CheckCastHandler>::iterator hookItrEnd = (*scritr)->OnCheckCast.end(), hookItr = (*scritr)->OnCheckCast.begin();
        for (; hookItr != hookItrEnd; ++hookItr)
        {
            SpellCastResult tempResult = (*hookItr).Call(*scritr);
            if (retVal == SPELL_CAST_OK)
                retVal = tempResult;
        }

        (*scritr)->_FinishScriptCall();
    }
    return retVal;
}

void Spell::CallScriptTakePowerHandlers(Powers p, int32 &amount)
{
    for (std::list<SpellScript*>::iterator scritr = m_loadedScripts.begin(); scritr != m_loadedScripts.end(); ++scritr)
    {
        (*scritr)->_PrepareScriptCall(SPELL_SCRIPT_HOOK_TAKE_POWER);
        std::list<SpellScript::TakePowerHandler>::iterator hookItrEnd = (*scritr)->OnTakePower.end(), hookItr = (*scritr)->OnTakePower.begin();
        for (; hookItr != hookItrEnd; ++hookItr)
            (*hookItr).Call(*scritr, p, amount);

        (*scritr)->_FinishScriptCall();
    }
}

void Spell::PrepareScriptHitHandlers()
{
    for (std::list<SpellScript*>::iterator scritr = m_loadedScripts.begin(); scritr != m_loadedScripts.end(); ++scritr)
        (*scritr)->_InitHit();
}

bool Spell::CallScriptEffectHandlers(SpellEffIndex effIndex, SpellEffectHandleMode mode)
{
    // execute script effect handler hooks and check if effects was prevented
    bool preventDefault = false;
    for (std::list<SpellScript*>::iterator scritr = m_loadedScripts.begin(); scritr != m_loadedScripts.end(); ++scritr)
    {
        std::list<SpellScript::EffectHandler>::iterator effItr, effEndItr;
        SpellScriptHookType hookType;
        switch (mode)
        {
            case SPELL_EFFECT_HANDLE_LAUNCH:
                effItr = (*scritr)->OnEffectLaunch.begin();
                effEndItr = (*scritr)->OnEffectLaunch.end();
                hookType = SPELL_SCRIPT_HOOK_EFFECT_LAUNCH;
                break;
            case SPELL_EFFECT_HANDLE_LAUNCH_TARGET:
                effItr = (*scritr)->OnEffectLaunchTarget.begin();
                effEndItr = (*scritr)->OnEffectLaunchTarget.end();
                hookType = SPELL_SCRIPT_HOOK_EFFECT_LAUNCH_TARGET;
                break;
            case SPELL_EFFECT_HANDLE_HIT:
                effItr = (*scritr)->OnEffectHit.begin();
                effEndItr = (*scritr)->OnEffectHit.end();
                hookType = SPELL_SCRIPT_HOOK_EFFECT_HIT;
                break;
            case SPELL_EFFECT_HANDLE_HIT_TARGET:
                effItr = (*scritr)->OnEffectHitTarget.begin();
                effEndItr = (*scritr)->OnEffectHitTarget.end();
                hookType = SPELL_SCRIPT_HOOK_EFFECT_HIT_TARGET;
                break;
            default:
                ASSERT(false);
                return false;
        }
        (*scritr)->_PrepareScriptCall(hookType);
        for (; effItr != effEndItr; ++effItr)
            // effect execution can be prevented
            if (!(*scritr)->_IsEffectPrevented(effIndex) && (*effItr).IsEffectAffected(m_spellInfo, effIndex))
                (*effItr).Call(*scritr, effIndex);

        if (!preventDefault)
            preventDefault = (*scritr)->_IsDefaultEffectPrevented(effIndex);

        (*scritr)->_FinishScriptCall();
    }
    return preventDefault;
}

void Spell::CallScriptBeforeHitHandlers()
{
    for (std::list<SpellScript*>::iterator scritr = m_loadedScripts.begin(); scritr != m_loadedScripts.end(); ++scritr)
    {
        (*scritr)->_PrepareScriptCall(SPELL_SCRIPT_HOOK_BEFORE_HIT);
        std::list<SpellScript::HitHandler>::iterator hookItrEnd = (*scritr)->BeforeHit.end(), hookItr = (*scritr)->BeforeHit.begin();
        for (; hookItr != hookItrEnd; ++hookItr)
            (*hookItr).Call(*scritr);

        (*scritr)->_FinishScriptCall();
    }
}

void Spell::CallScriptOnHitHandlers()
{
    for (std::list<SpellScript*>::iterator scritr = m_loadedScripts.begin(); scritr != m_loadedScripts.end(); ++scritr)
    {
        (*scritr)->_PrepareScriptCall(SPELL_SCRIPT_HOOK_HIT);
        std::list<SpellScript::HitHandler>::iterator hookItrEnd = (*scritr)->OnHit.end(), hookItr = (*scritr)->OnHit.begin();
        for (; hookItr != hookItrEnd; ++hookItr)
            (*hookItr).Call(*scritr);

        (*scritr)->_FinishScriptCall();
    }
}

void Spell::CallScriptAfterHitHandlers()
{
    for (std::list<SpellScript*>::iterator scritr = m_loadedScripts.begin(); scritr != m_loadedScripts.end(); ++scritr)
    {
        (*scritr)->_PrepareScriptCall(SPELL_SCRIPT_HOOK_AFTER_HIT);
        std::list<SpellScript::HitHandler>::iterator hookItrEnd = (*scritr)->AfterHit.end(), hookItr = (*scritr)->AfterHit.begin();
        for (; hookItr != hookItrEnd; ++hookItr)
            (*hookItr).Call(*scritr);

        (*scritr)->_FinishScriptCall();
    }
}

void Spell::CallScriptObjectAreaTargetSelectHandlers(std::list<WorldObject*>& targets, SpellEffIndex effIndex, Targets targetId)
{
    for (std::list<SpellScript*>::iterator scritr = m_loadedScripts.begin(); scritr != m_loadedScripts.end(); ++scritr)
    {
        (*scritr)->_PrepareScriptCall(SPELL_SCRIPT_HOOK_OBJECT_AREA_TARGET_SELECT);
        std::list<SpellScript::ObjectAreaTargetSelectHandler>::iterator hookItrEnd = (*scritr)->OnObjectAreaTargetSelect.end(), hookItr = (*scritr)->OnObjectAreaTargetSelect.begin();
        for (; hookItr != hookItrEnd; ++hookItr)
            if ((*hookItr).IsEffectAffected(m_spellInfo, effIndex))
                (*hookItr).Call(*scritr, targets);

        (*scritr)->_FinishScriptCall();
    }
    CustomTargetSelector(targets, effIndex, targetId);
}

void Spell::CustomTargetSelector(std::list<WorldObject*>& targets, SpellEffIndex effIndex, Targets targetId)
{
    if (std::vector<SpellTargetFilter> const* spellTargetFilter = sSpellMgr->GetSpellTargetFilter(m_spellInfo->Id))
    {
        uint32 targetCount = 0;
        uint32 resizeType = 0;
        Unit* _caster = m_originalCaster ? m_originalCaster : m_caster;
        if(_caster->isTotem())
            if (Unit* owner = _caster->GetOwner())
                _caster = owner;

        Unit* _target = m_targets.GetUnitTarget();
        if(!_target && m_caster->ToPlayer())
            _target = m_caster->ToPlayer()->GetSelectedUnit();
        else if(!_target)
            _target = m_caster->getVictim();

        for (std::vector<SpellTargetFilter>::const_iterator itr = spellTargetFilter->begin(); itr != spellTargetFilter->end(); ++itr)
        {
            if (!(itr->effectMask & (1<<effIndex)))
                continue;

            if (itr->targetId != targetId)
                continue;

            if (targets.empty())
            {
                if(itr->addcaster == 2)
                    targets.push_back(GetCaster());
                return;
            }

            if(itr->count && !targetCount && !itr->maxcount)
                targetCount = itr->count;
            else if(itr->maxcount && !targetCount)
            {
                if(m_caster->GetMap()->Is25ManRaid())
                    targetCount = itr->maxcount;
                else
                    targetCount = itr->count;
            }

            if(itr->resizeType && !resizeType)
                resizeType = itr->resizeType;

            if(itr->aura > 0 && _caster->HasAura(itr->aura))
                targetCount += itr->addcount;
            if(itr->aura < 0 && !_caster->HasAura(abs(itr->aura)))
                targetCount -= itr->addcount;

            if(itr->addcaster < 0)
                targets.remove(GetCaster());
            if(itr->addcaster == 2)
                targets.remove(GetCaster());

            switch (itr->option)
            {
                case SPELL_FILTER_SORT_BY_HEALT: //0
                {
                    if(itr->param1 < 0.0f)
                        targets.sort(Trinity::UnitHealthState(false));
                    else
                        targets.sort(Trinity::UnitHealthState(true));
                    break;
                }
                case SPELL_FILTER_BY_AURA: //1
                {
                    targets.remove_if(Trinity::UnitAuraAndCheck(itr->param1, itr->param2, itr->param3));
                    break;
                }
                case SPELL_FILTER_BY_DISTANCE: //2
                {
                    if(itr->param1 < 0.0f)
                        targets.remove_if(Trinity::UnitDistanceCheck(false, _caster, itr->param2));
                    else
                        targets.remove_if(Trinity::UnitDistanceCheck(true, _caster, itr->param2));
                    break;
                }
                case SPELL_FILTER_TARGET_TYPE: //3
                {
                    if (itr->param1)  // can`t remove players
                    {
                        if (itr->param1 < 0.0f)
                            targets.remove_if(Trinity::UnitTypeMaskCheck(true, uint32(abs(itr->param1))));
                        else
                            targets.remove_if(Trinity::UnitTypeMaskCheck(false, uint32(itr->param1)));
                    }

                    if (itr->param2)
                    {
                        if (itr->param2 < 0.0f)
                            targets.remove_if(Trinity::UnitTypeIdCheck(true, uint32(abs(itr->param2))));
                        else
                            targets.remove_if(Trinity::UnitTypeIdCheck(false, uint32(itr->param2)));
                    }
                    break;
                }
                case SPELL_FILTER_SORT_BY_DISTANCE: //4
                {
                    if(itr->param1 < 0.0f)
                        targets.sort(Trinity::UnitSortDistance(false, _caster));
                    else
                        targets.sort(Trinity::UnitSortDistance(true, _caster));
                    break;
                }
                case SPELL_FILTER_TARGET_FRIENDLY: //5
                {
                    if(itr->param1 < 0.0f)
                        targets.remove_if(Trinity::UnitFriendlyCheck(true, _caster));
                    else
                        targets.remove_if(Trinity::UnitFriendlyCheck(false, _caster));
                    break;
                }
                case SPELL_FILTER_TARGET_IN_RAID: //6
                {
                    if(itr->param1 < 0.0f)
                        targets.remove_if(Trinity::UnitRaidCheck(true, _caster));
                    else
                        targets.remove_if(Trinity::UnitRaidCheck(false, _caster));
                    break;
                }
                case SPELL_FILTER_TARGET_IN_PARTY: //7
                {
                    if(itr->param1 < 0.0f)
                        targets.remove_if(Trinity::UnitPartyCheck(true, _caster));
                    else
                        targets.remove_if(Trinity::UnitPartyCheck(false, _caster));
                    break;
                }
                case SPELL_FILTER_TARGET_EXPL_TARGET: //8
                {
                    targets.clear();
                    if(_target)
                        targets.push_back(_target);
                    break;
                }
                case SPELL_FILTER_TARGET_EXPL_TARGET_REMOVE: //9
                {
                    if(_target)
                        targets.remove(_target);
                    break;
                }
                case SPELL_FILTER_TARGET_IN_LOS: //10
                {
                    if(itr->param1 < 0.0f)
                        targets.remove_if(Trinity::UnitCheckInLos(true, _caster));
                    else
                        targets.remove_if(Trinity::UnitCheckInLos(false, _caster));
                    break;
                }
                case SPELL_FILTER_TARGET_IS_IN_BETWEEN: //11
                {
                    if(_target)
                        targets.remove_if(Trinity::UnitCheckInBetween(false, _caster, _target, itr->param1));
                    break;
                }
                case SPELL_FILTER_TARGET_IS_IN_BETWEEN_SHIFT: //12
                {
                    if(_target)
                        targets.remove_if(Trinity::UnitCheckInBetweenShift(false, _caster, _target, itr->param1, itr->param2, itr->param3));
                    break;
                }
                case SPELL_FILTER_BY_AURA_OR: //13
                {
                    targets.remove_if(Trinity::UnitAuraOrCheck(itr->param1, itr->param2, itr->param3));
                    break;
                }
                case SPELL_FILTER_BY_ENTRY: //14
                {
                    targets.remove_if(Trinity::UnitEntryCheck(itr->param1, itr->param2, itr->param3));
                    break;
                }
                case SPELL_FILTER_TARGET_ATTACKABLE: // 15
                {
                    if(itr->param1 < 0.0f)
                        targets.remove_if(Trinity::UnitAttackableCheck(true, _caster));
                    else
                        targets.remove_if(Trinity::UnitAttackableCheck(false, _caster));
                    break;
                }
                case SPELL_FILTER_REMOVE_HEALTHY_TARGET: // 16
                {
                    std::list<Unit*> removeList;

                    for (auto i : targets)
                        if (Unit* target = i->ToUnit())
                            if (target->IsFullHealth())
                                removeList.push_back(target);

                    if (!removeList.empty())
                        for (auto i : removeList)
                            targets.remove(i);
                    break;
                }
            }
            switch(itr->addcaster)
            {
                case 1:
                    if (!targets.empty())
                        targets.remove(GetCaster());
                    targets.push_back(GetCaster());
                    break;
                case 2:
                    if (targets.empty())
                        targets.push_back(GetCaster());
                    break;
            }
        }
        switch(resizeType)
        {
            case 1:
                if (targets.size() > targetCount)
                    targets.resize(targetCount);
                break;
            case 2:
                if (targets.size() > targetCount)
                    Trinity::Containers::RandomResizeList(targets, targetCount);
                break;
        }
    }
}

void Spell::CallScriptObjectTargetSelectHandlers(WorldObject*& target, SpellEffIndex effIndex)
{
    for (std::list<SpellScript*>::iterator scritr = m_loadedScripts.begin(); scritr != m_loadedScripts.end(); ++scritr)
    {
        (*scritr)->_PrepareScriptCall(SPELL_SCRIPT_HOOK_OBJECT_TARGET_SELECT);
        std::list<SpellScript::ObjectTargetSelectHandler>::iterator hookItrEnd = (*scritr)->OnObjectTargetSelect.end(), hookItr = (*scritr)->OnObjectTargetSelect.begin();
        for (; hookItr != hookItrEnd; ++hookItr)
            if ((*hookItr).IsEffectAffected(m_spellInfo, effIndex))
                (*hookItr).Call(*scritr, target);

        (*scritr)->_FinishScriptCall();
    }
}

bool Spell::CanExecuteTriggersOnHit(uint32 effMask, SpellInfo const* triggeredByAura) const
{
    bool only_on_caster = (triggeredByAura && (triggeredByAura->AttributesEx4 & SPELL_ATTR4_PROC_ONLY_ON_CASTER));
    // If triggeredByAura has SPELL_ATTR4_PROC_ONLY_ON_CASTER then it can only proc on a casted spell with TARGET_UNIT_CASTER
    for (uint8 i = 0;i < MAX_SPELL_EFFECTS; ++i)
    {
        if ((effMask & (1 << i)) && (!only_on_caster || (m_spellInfo->GetEffect(i, m_diffMode)->TargetA.GetTarget() == TARGET_UNIT_CASTER)))
            return true;
    }
    return false;
}

void Spell::PrepareTriggersExecutedOnHit()
{
    // todo: move this to scripts
    if (m_spellInfo->SpellFamilyName)
    {
        SpellInfo const* excludeTargetSpellInfo = sSpellMgr->GetSpellInfo(m_spellInfo->ExcludeTargetAuraSpell);
        if (excludeTargetSpellInfo && !excludeTargetSpellInfo->IsPositive())
            m_preCastSpell = m_spellInfo->ExcludeTargetAuraSpell;
    }

    // handle SPELL_AURA_ADD_TARGET_TRIGGER auras:
    // save auras which were present on spell caster on cast, to prevent triggered auras from affecting caster
    // and to correctly calculate proc chance when combopoints are present
    Unit::AuraEffectList const& targetTriggers = m_caster->GetAuraEffectsByType(SPELL_AURA_ADD_TARGET_TRIGGER);
    for (Unit::AuraEffectList::const_iterator i = targetTriggers.begin(); i != targetTriggers.end(); ++i)
    {
        if (!(*i)->IsAffectingSpell(m_spellInfo))
            continue;
        SpellInfo const* auraSpellInfo = (*i)->GetSpellInfo();
        uint32 auraSpellIdx = (*i)->GetEffIndex();
        if (SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(auraSpellInfo->Effects[auraSpellIdx].TriggerSpell))
        {
            // calculate the chance using spell base amount, because aura amount is not updated on combo-points change
            // this possibly needs fixing
            float auraBaseAmount = (*i)->GetBaseAmount();
            // proc chance is stored in effect amount
            int32 chance = m_caster->CalculateSpellDamage(NULL, auraSpellInfo, auraSpellIdx, &auraBaseAmount);
            // build trigger and add to the list
            HitTriggerSpell spellTriggerInfo;
            spellTriggerInfo.triggeredSpell = spellInfo;
            spellTriggerInfo.triggeredByAura = auraSpellInfo;
            spellTriggerInfo.chance = chance * (*i)->GetBase()->GetStackAmount();
            m_hitTriggerSpells.push_back(spellTriggerInfo);
        }
    }
}

// Global cooldowns management
enum GCDLimits
{
    MIN_GCD = 1000,
    MAX_GCD = 1500
};

bool Spell::HasGlobalCooldown()
{
    // Only player or controlled units have global cooldown
    if (m_caster->GetCharmInfo())
        return m_caster->GetCharmInfo()->GetGlobalCooldownMgr().HasGlobalCooldown(m_spellInfo);
    else if (m_caster->GetTypeId() == TYPEID_PLAYER && GetGlobalCooldown() > 0)
        return m_caster->ToPlayer()->GetGlobalCooldownMgr().HasGlobalCooldown(m_spellInfo);
    else
        return false;
}

int32 Spell::GetGlobalCooldown()
{
    int32 gcd = m_spellInfo->StartRecoveryTime;
    if (!gcd)
        return 0;

     if (m_caster->GetTypeId() == TYPEID_PLAYER)
          if (m_caster->ToPlayer()->GetCommandStatus(CHEAT_COOLDOWN))
               return 0;

    // Global cooldown can't leave range 1..1.5 secs
    // There are some spells (mostly not casted directly by player) that have < 1 sec and > 1.5 sec global cooldowns
    // but as tests show are not affected by any spell mods.
    if (m_spellInfo->StartRecoveryTime >= MIN_GCD && m_spellInfo->StartRecoveryTime <= MAX_GCD)
    {
        // gcd modifier auras are applied only to own spells and only players have such mods
        if (m_caster->GetTypeId() == TYPEID_PLAYER)
            m_caster->ToPlayer()->ApplySpellMod(m_spellInfo->Id, SPELLMOD_GLOBAL_COOLDOWN, gcd, this);

        return gcd;
    }
    return 0;
}

void Spell::TriggerGlobalCooldown()
{
    int32 gcd = m_spellInfo->StartRecoveryTime;
    if (!gcd)
        return;

     if (m_caster->GetTypeId() == TYPEID_PLAYER)
          if (m_caster->ToPlayer()->GetCommandStatus(CHEAT_COOLDOWN))
               return;

    // Global cooldown can't leave range 1..1.5 secs
    // There are some spells (mostly not casted directly by player) that have < 1 sec and > 1.5 sec global cooldowns
    // but as tests show are not affected by any spell mods.
    if (m_spellInfo->StartRecoveryTime >= MIN_GCD && m_spellInfo->StartRecoveryTime <= MAX_GCD)
    {
        // Apply haste rating
        gcd = int32(float(gcd) * m_caster->GetFloatValue(UNIT_MOD_CAST_SPEED));
        if (gcd < MIN_GCD)
            gcd = MIN_GCD;
        else if (gcd > MAX_GCD)
            gcd = MAX_GCD;

        // gcd modifier auras are applied only to own spells and only players have such mods
        if (m_caster->GetTypeId() == TYPEID_PLAYER)
            m_caster->ToPlayer()->ApplySpellMod(m_spellInfo->Id, SPELLMOD_GLOBAL_COOLDOWN, gcd, this);

        if(gcd <= 0)
            return;
    }

    // Only players or controlled units have global cooldown
    if (m_caster->GetCharmInfo())
        m_caster->GetCharmInfo()->GetGlobalCooldownMgr().AddGlobalCooldown(m_spellInfo, gcd);
    else if (m_caster->GetTypeId() == TYPEID_PLAYER)
        m_caster->ToPlayer()->GetGlobalCooldownMgr().AddGlobalCooldown(m_spellInfo, gcd);
}

void Spell::CancelGlobalCooldown()
{
    if (!m_spellInfo->StartRecoveryTime)
        return;

    // Cancel global cooldown when interrupting current cast
    if (m_caster->GetCurrentSpell(CURRENT_GENERIC_SPELL) != this)
        return;

    // Only players or controlled units have global cooldown
    if (m_caster->GetCharmInfo())
        m_caster->GetCharmInfo()->GetGlobalCooldownMgr().CancelGlobalCooldown(m_spellInfo);
    else if (m_caster->GetTypeId() == TYPEID_PLAYER)
        m_caster->ToPlayer()->GetGlobalCooldownMgr().CancelGlobalCooldown(m_spellInfo);
}

bool Spell::IsCritForTarget(Unit* target) const
{
    if (!target)
        return false;

    for (std::list<TargetInfo>::const_iterator itr = m_UniqueTargetInfo.begin(); itr != m_UniqueTargetInfo.end(); ++itr)
        if (itr->targetGUID == target->GetGUID() && itr->crit)
            return true;

    return false;
}

void Spell::LoadAttrDummy()
{
    AttributesCustom = m_spellInfo->Attributes;
    AttributesCustomEx = m_spellInfo->AttributesEx;
    AttributesCustomEx2 = m_spellInfo->AttributesEx2;
    AttributesCustomEx3 = m_spellInfo->AttributesEx3;
    AttributesCustomEx4 = m_spellInfo->AttributesEx4;
    AttributesCustomEx5 = m_spellInfo->AttributesEx5;
    AttributesCustomEx6 = m_spellInfo->AttributesEx6;
    AttributesCustomEx7 = m_spellInfo->AttributesEx7;
    AttributesCustomEx8 = m_spellInfo->AttributesEx8;
    AttributesCustomEx9 = m_spellInfo->AttributesEx9;
    AttributesCustomEx10 = m_spellInfo->AttributesEx10;
    AttributesCustomEx11 = m_spellInfo->AttributesEx11;
    AttributesCustomEx12 = m_spellInfo->AttributesEx12;
    AttributesCustomEx13 = m_spellInfo->AttributesEx13;
    AttributesCustomCu = m_spellInfo->AttributesCu;

    if (std::vector<SpellAuraDummy> const* spellAuraDummy = sSpellMgr->GetSpellAuraDummy(m_spellInfo->Id))
    {
        for (std::vector<SpellAuraDummy>::const_iterator itr = spellAuraDummy->begin(); itr != spellAuraDummy->end(); ++itr)
        {
            Unit* _caster = m_caster;
            bool check = false;

            switch (itr->option)
            {
                case SPELL_DUMMY_ADD_ATTRIBUTE: //3
                {
                    if(itr->attrValue != 0)
                    {
                        int32 saveAttrValue = itr->attrValue;
                        if(itr->spellDummyId > 0 && !_caster->HasAura(itr->spellDummyId))
                            saveAttrValue *= -1;
                        if(itr->spellDummyId < 0 && _caster->HasAura(abs(itr->spellDummyId)))
                            saveAttrValue *= -1;

                        switch (itr->attr)
                        {
                            case -1:
                                if(saveAttrValue > 0)
                                    AttributesCustomCu |= saveAttrValue;
                                else
                                    AttributesCustomCu &= ~saveAttrValue;
                                break;
                            case 0:
                                if(saveAttrValue > 0)
                                    AttributesCustom |= saveAttrValue;
                                else
                                    AttributesCustom &= ~saveAttrValue;
                                break;
                            case 1:
                                if(saveAttrValue > 0)
                                    AttributesCustomEx |= saveAttrValue;
                                else
                                    AttributesCustomEx &= ~saveAttrValue;
                                break;
                            case 2:
                                if(saveAttrValue > 0)
                                    AttributesCustomEx2 |= saveAttrValue;
                                else
                                    AttributesCustomEx2 &= ~saveAttrValue;
                                break;
                            case 3:
                                if(saveAttrValue > 0)
                                    AttributesCustomEx3 |= saveAttrValue;
                                else
                                    AttributesCustomEx3 &= ~saveAttrValue;
                                break;
                            case 4:
                                if(saveAttrValue > 0)
                                    AttributesCustomEx4 |= saveAttrValue;
                                else
                                    AttributesCustomEx4 &= ~saveAttrValue;
                                break;
                            case 5:
                                if(saveAttrValue > 0)
                                    AttributesCustomEx5 |= saveAttrValue;
                                else
                                    AttributesCustomEx5 &= ~saveAttrValue;
                                break;
                            case 6:
                                if(saveAttrValue > 0)
                                    AttributesCustomEx6 |= saveAttrValue;
                                else
                                    AttributesCustomEx6 &= ~saveAttrValue;
                                break;
                            case 7:
                                if(saveAttrValue > 0)
                                    AttributesCustomEx7 |= saveAttrValue;
                                else
                                    AttributesCustomEx7 &= ~saveAttrValue;
                                break;
                            case 8:
                                if(saveAttrValue > 0)
                                    AttributesCustomEx8 |= saveAttrValue;
                                else
                                    AttributesCustomEx8 &= ~saveAttrValue;
                                break;
                            case 9:
                                if(saveAttrValue > 0)
                                    AttributesCustomEx9 |= saveAttrValue;
                                else
                                    AttributesCustomEx9 &= ~saveAttrValue;
                                break;
                            case 10:
                                if(saveAttrValue > 0)
                                    AttributesCustomEx10 |= saveAttrValue;
                                else
                                    AttributesCustomEx10 &= ~saveAttrValue;
                                break;
                            case 11:
                                if(saveAttrValue > 0)
                                    AttributesCustomEx11 |= saveAttrValue;
                                else
                                    AttributesCustomEx11 &= ~saveAttrValue;
                                break;
                            case 12:
                                if(saveAttrValue > 0)
                                    AttributesCustomEx12 |= saveAttrValue;
                                else
                                    AttributesCustomEx12 &= ~saveAttrValue;
                                break;
                            case 13:
                                if(saveAttrValue > 0)
                                    AttributesCustomEx13 |= saveAttrValue;
                                else
                                    AttributesCustomEx13 &= ~saveAttrValue;
                                break;
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

// was WriteAmmoToPacket. Not used only for creature range casts.
// ToDo: create new field on creature_equip_template and get data from it, as arrows are different.
void Spell::WriteProjectile(uint8 &ammoInventoryType, uint32 &ammoDisplayID)
{
    ASSERT(m_caster->GetTypeId() != TYPEID_PLAYER);

    for (uint8 i = 0; i < 3; ++i)
    {
        if (uint32 item_id = m_caster->GetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_ID + i))
       {
            if (ItemEntry const* itemEntry = sItemStore.LookupEntry(item_id))
            {
                if (itemEntry->Class == ITEM_CLASS_WEAPON)
                {
                    switch (itemEntry->SubClass)
                    {
                        case ITEM_SUBCLASS_WEAPON_THROWN:
                            ammoDisplayID = itemEntry->DisplayId;
                            ammoInventoryType = itemEntry->InventoryType;
                            break;
                        case ITEM_SUBCLASS_WEAPON_BOW:
                        case ITEM_SUBCLASS_WEAPON_CROSSBOW:
                            ammoDisplayID = 5996;       // is this need fixing?
                            ammoInventoryType = INVTYPE_AMMO;
                            break;
                        case ITEM_SUBCLASS_WEAPON_GUN:
                            ammoDisplayID = 5998;       // is this need fixing?
                            ammoInventoryType = INVTYPE_AMMO;
                            break;
                    }

                    if (ammoDisplayID)
                        break;
                }
            }
        }
    }
}

namespace Trinity
{

WorldObjectSpellTargetCheck::WorldObjectSpellTargetCheck(Unit* caster, Unit* referer, SpellInfo const* spellInfo,
    SpellTargetCheckTypes selectionType, ConditionList* condList, SpellEffIndex effIndex) : _caster(caster), _referer(referer), _spellInfo(spellInfo),
    _targetSelectionType(selectionType), _condList(condList), _effIndex(effIndex)
{
    if (condList)
        _condSrcInfo = new ConditionSourceInfo(NULL, caster);
    else
        _condSrcInfo = NULL;
}

WorldObjectSpellTargetCheck::~WorldObjectSpellTargetCheck()
{
    if (_condSrcInfo)
        delete _condSrcInfo;
}

bool WorldObjectSpellTargetCheck::operator()(WorldObject* target)
{
    if (_condSrcInfo)
    {
        _condSrcInfo->mConditionTargets[0] = target;
        bool check = sConditionMgr->IsObjectMeetToConditions(*_condSrcInfo, *_condList);
        //if(check)
            //sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "Spell::WorldObjectSpellTargetCheck spell id %u caster %u target %u entry %i", _spellInfo->Id,  _caster->GetGUID(), target->GetGUID(), target->GetEntry());
        return check;
    }

    uint8 res = _spellInfo->CheckTarget(_caster, target, true);
    if (res != SPELL_CAST_OK)
    {
        #ifdef WIN32
        sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "Spell::WorldObjectSpellTargetCheck::checkcast fail. spell id %u res %u caster %s target %s ", _spellInfo->Id, res, _caster->GetString().c_str(), target->GetString().c_str());
        #endif
        return false;
    }

    Unit* unitTarget = target->ToUnit();
    if (Corpse* corpseTarget = target->ToCorpse())
    {
        // use ofter for party/assistance checks
        if (Player* owner = ObjectAccessor::FindPlayer(corpseTarget->GetOwnerGUID()))
            unitTarget = owner;
        else
            return false;
    }
    if (unitTarget)
    {
        switch (_targetSelectionType)
        {
            case TARGET_CHECK_ENEMY:
                if (unitTarget->isTotem())
                    return false;
                if (!_caster->_IsValidAttackTarget(unitTarget, _spellInfo))
                    return false;
                break;
            case TARGET_CHECK_SUMMON:
                if (unitTarget->isTotem())
                    return false;
                if (!_caster->_IsValidAssistTarget(unitTarget, _spellInfo))
                    return false;
                if (!unitTarget->IsOwnerOrSelf(_caster))
                    return false;
                break;
            case TARGET_CHECK_ALLY:
                if (unitTarget->isTotem())
                    return false;
                if (!_caster->_IsValidAssistTarget(unitTarget, _spellInfo))
                    return false;
                break;
            case TARGET_CHECK_PARTY:
                if (unitTarget->isTotem())
                    return false;
                if (!_caster->_IsValidAssistTarget(unitTarget, _spellInfo))
                    return false;
                if (!_referer->IsInPartyWith(unitTarget))
                    return false;
                break;
            case TARGET_CHECK_RAID_CLASS:
                if (_referer->getClass() != unitTarget->getClass())
                    return false;
                // nobreak;
            case TARGET_CHECK_RAID:
                if (unitTarget->isTotem())
                    return false;
                if (!_caster->_IsValidAssistTarget(unitTarget, _spellInfo))
                {
                    if (unitTarget->m_ControlledByPlayer)
                    {
                        if (_spellInfo->Effects[_effIndex].Effect != SPELL_EFFECT_DISPEL_MECHANIC || _spellInfo->Effects[_effIndex].MiscValue != 1)
                            return false;
                    }
                    else
                        return false;
                }
                if (!_referer->IsInRaidWith(unitTarget))
                    return false;
                break;
            default:
                break;
        }
    }
    return true;
}

WorldObjectSpellNearbyTargetCheck::WorldObjectSpellNearbyTargetCheck(float range, Unit* caster, SpellInfo const* spellInfo,
    SpellTargetCheckTypes selectionType, ConditionList* condList, SpellEffIndex effIndex)
    : WorldObjectSpellTargetCheck(caster, caster, spellInfo, selectionType, condList, effIndex), _range(range), _position(caster)
{
}

bool WorldObjectSpellNearbyTargetCheck::operator()(WorldObject* target)
{
    float dist = target->GetDistance(*_position);
    if (dist < _range && WorldObjectSpellTargetCheck::operator ()(target))
    {
        _range = dist;
        return true;
    }
    return false;
}

WorldObjectSpellAreaTargetCheck::WorldObjectSpellAreaTargetCheck(float range, Position const* position, Unit* caster,
    Unit* referer, SpellInfo const* spellInfo, SpellTargetCheckTypes selectionType, ConditionList* condList, SpellEffIndex effIndex)
    : WorldObjectSpellTargetCheck(caster, referer, spellInfo, selectionType, condList, effIndex), _range(range), _position(position)
{
}

bool WorldObjectSpellAreaTargetCheck::operator()(WorldObject* target)
{
    if (!target->IsWithinDist3d(_position, _range))
        return false;
    return WorldObjectSpellTargetCheck::operator ()(target);
}

WorldObjectSpellBetweenTargetCheck::WorldObjectSpellBetweenTargetCheck(float width, float range, Unit* caster, Position const* position, Unit* referer,
    SpellInfo const* spellInfo, SpellTargetCheckTypes selectionType, ConditionList* condList, SpellEffIndex effIndex)
    : WorldObjectSpellAreaTargetCheck(range, caster, caster, referer, spellInfo, selectionType, condList, effIndex), _width(width), _range(range), _position(position)
{
}

bool WorldObjectSpellBetweenTargetCheck::operator()(WorldObject* target)
{
    if (!target->IsInBetween(_caster, _position->GetPositionX(), _position->GetPositionY(), _width))
        return false;

    return WorldObjectSpellAreaTargetCheck::operator ()(target);
}

WorldObjectSpellConeTargetCheck::WorldObjectSpellConeTargetCheck(float coneAngle, float range, Unit* caster,
    SpellInfo const* spellInfo, SpellTargetCheckTypes selectionType, ConditionList* condList, SpellEffIndex effIndex)
    : WorldObjectSpellAreaTargetCheck(range, caster, caster, caster, spellInfo, selectionType, condList, effIndex), _coneAngle(coneAngle)
{
}

bool WorldObjectSpellConeTargetCheck::operator()(WorldObject* target)
{
    if (_spellInfo->AttributesCu & SPELL_ATTR0_CU_CONE_BACK)
    {
        if (!_caster->isInBack(target, _coneAngle))
            return false;
    }
    else if (_spellInfo->AttributesCu & SPELL_ATTR0_CU_CONE_LINE)
    {
        if (!_caster->HasInLine(target, _caster->GetObjectSize()))
            return false;
    }
    else
    {
        if (!_caster->isInFront(target, _coneAngle))
            return false;
    }
    return WorldObjectSpellAreaTargetCheck::operator ()(target);
}

WorldObjectSpellTrajTargetCheck::WorldObjectSpellTrajTargetCheck(float range, Position const* position, Unit* caster, SpellInfo const* spellInfo)
    : WorldObjectSpellAreaTargetCheck(range, position, caster, caster, spellInfo, TARGET_CHECK_DEFAULT, NULL, MAX_SPELL_EFFECTS)
{
}

bool WorldObjectSpellTrajTargetCheck::operator()(WorldObject* target)
{
    // return all targets on missile trajectory (0 - size of a missile)
    if (!_caster->HasInLine(target, 0))
        return false;
    return WorldObjectSpellAreaTargetCheck::operator ()(target);
}

} //namespace Trinity
