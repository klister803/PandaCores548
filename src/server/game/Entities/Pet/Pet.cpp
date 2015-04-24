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
#include "Log.h"
#include "WorldPacket.h"
#include "ObjectMgr.h"
#include "SpellMgr.h"
#include "Pet.h"
#include "Formulas.h"
#include "SpellAuras.h"
#include "SpellAuraEffects.h"
#include "CreatureAI.h"
#include "Unit.h"
#include "Util.h"
#include "Group.h"

#define PET_XP_FACTOR 0.05f

Pet::Pet(Player* owner, PetType type) : Guardian(NULL, owner, true),
m_removed(false), m_duration(0), m_specialization(0),
m_auraRaidUpdateMask(0), m_declinedname(NULL)
{
    m_slot = PET_SLOT_UNK_SLOT;
    m_owner = (Unit*)owner;
    m_loading = false;
    if(m_owner && m_owner->getClass() == CLASS_HUNTER)
    {
        type = HUNTER_PET;
        setPetType(type);
    }
    m_unitTypeMask &= ~UNIT_MASK_MINION;

    m_unitTypeMask |= UNIT_MASK_PET;
    if (type == HUNTER_PET)
        m_unitTypeMask |= UNIT_MASK_HUNTER_PET;

    if (!(m_unitTypeMask & UNIT_MASK_CONTROLABLE_GUARDIAN))
    {
        m_unitTypeMask |= UNIT_MASK_CONTROLABLE_GUARDIAN;
        InitCharmInfo();
    }

    m_name = "Pet";
    m_Update    = false;
}

Pet::~Pet()
{
    delete m_declinedname;
}

void Pet::AddToWorld()
{
    ///- Register the pet for guid lookup
    if (!IsInWorld())
    {
        ///- Register the pet for guid lookup
        sObjectAccessor->AddObject(this);
        Unit::AddToWorld();
        AIM_Initialize();
    }

    // Prevent stuck pets when zoning. Pets default to "follow" when added to world
    // so we'll reset flags and let the AI handle things
    if (GetCharmInfo() && GetCharmInfo()->HasCommandState(COMMAND_FOLLOW))
    {
        GetCharmInfo()->SetIsCommandAttack(false);
        GetCharmInfo()->SetIsAtStay(false);
        GetCharmInfo()->SetIsFollowing(false);
        GetCharmInfo()->SetIsReturning(false);
    }

}

void Pet::RemoveFromWorld()
{
    ///- Remove the pet from the accessor
    if (IsInWorld())
    {
        ///- Don't call the function for Creature, normal mobs + totems go in a different storage
        Unit::RemoveFromWorld();
        sObjectAccessor->RemoveObject(this);
    }
}

bool Pet::LoadPetFromDB(Player* owner, uint32 petentry, uint32 petnumber, bool stampeded)
{
    if (owner->getClass() == CLASS_WARLOCK)
        if (owner->HasAura(108503))
            owner->RemoveAura(108503);

    if (owner->GetTypeId() == TYPEID_PLAYER && isControlled() && !isTemporarySummoned())
        owner->SetLastPetEntry(petentry);

    m_loading = true;
    m_Stampeded = stampeded;
    uint32 ownerid = owner->GetGUIDLow();

    // find number
    if (!petnumber && !petentry)
    {
        if(owner->m_currentSummonedSlot < PET_SLOT_HUNTER_FIRST)
        {
            sLog->outDebug(LOG_FILTER_PETS, "Pet::LoadPetFromDB error m_currentSummonedSlot < 0");
            m_loading = false;
            return false;
        }
        else
            petnumber = owner->getPetIdBySlot(owner->m_currentSummonedSlot);
    }

    //sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "LoadPetFromDB petentry %i, petnumber %i, ownerid %i slot %i stampeded %i m_currentPet %i", petentry, petnumber, ownerid, owner->m_currentSummonedSlot, stampeded, owner->m_currentPetNumber);

    QueryResult result;
    if (petnumber)                          //     0    1       2       3       4   5       6           7   8           9       10         11       12          13          14          15
        result = CharacterDatabase.PQuery("SELECT id, entry, owner, modelid, level, exp, Reactstate, name, renamed, curhealth, curmana, abdata, savetime, CreatedBySpell, PetType, specialization FROM character_pet WHERE owner = '%u' AND id = '%u' LIMIT 1", ownerid, petnumber);
    else if (petentry)  //non hunter pets
        result = CharacterDatabase.PQuery("SELECT id, entry, owner, modelid, level, exp, Reactstate, name, renamed, curhealth, curmana, abdata, savetime, CreatedBySpell, PetType, specialization FROM character_pet WHERE owner = '%u' AND id > 0 AND entry = '%u' LIMIT 1", ownerid, petentry);

    if (!result)
    {
        sLog->outDebug(LOG_FILTER_PETS, "Pet::LoadPetFromDB error result");
        m_loading = false;
        return false;
    }

    Field* fields = result->Fetch();

    // update for case of current pet "slot = 0"
    petentry = fields[1].GetUInt32();
    if (!petentry)
    {
        sLog->outDebug(LOG_FILTER_PETS, "Pet::LoadPetFromDB error petentry");
        m_loading = false;
        return false;
    }

    // Double call if use Player::SummonPet
    // But this option for check through Player::LoadPet
    if (!owner->CanSummonPet(petentry))
    {
        sLog->outDebug(LOG_FILTER_PETS, "Pet::LoadPetFromDB error CanSummonPet");
        m_loading = false;
        return false;
    }

    uint32 summon_spell_id = fields[13].GetUInt32();
    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(summon_spell_id);

    bool is_temporary_summoned = spellInfo && spellInfo->GetDuration() > 0;

    // check temporary summoned pets like mage water elemental
    if (is_temporary_summoned && !stampeded)
    {
        m_loading = false;
        sLog->outDebug(LOG_FILTER_PETS, "Pet::LoadPetFromDB error is_temporary_summoned %i", is_temporary_summoned);
        return false;
    }

    PetType pet_type = PetType(fields[14].GetUInt8());
    setPetType(pet_type);

    if (owner->GetTypeId() == TYPEID_PLAYER && isControlled() && !isTemporarySummoned())
        owner->SetLastPetEntry(petentry);

    if (pet_type == HUNTER_PET)
    {
        CreatureTemplate const* creatureInfo = sObjectMgr->GetCreatureTemplate(petentry);
        if (!creatureInfo || !creatureInfo->isTameable(owner->CanTameExoticPets()))
            return false;
    }

    uint32 pet_number = fields[0].GetUInt32();

    if (owner->IsPetNeedBeTemporaryUnsummoned())
    {
        sLog->outDebug(LOG_FILTER_PETS, "Pet::LoadPetFromDB error IsPetNeedBeTemporaryUnsummoned");
        owner->SetTemporaryUnsummonedPetNumber(pet_number);
        return false;
    }

    Map* map = owner->GetMap();
    uint32 guid = sObjectMgr->GenerateLowGuid(HIGHGUID_PET);
    if (!Create(guid, map, owner->GetPhaseMask(), petentry, pet_number))
    {
        sLog->outDebug(LOG_FILTER_PETS, "Pet::LoadPetFromDB error Create");
        return false;
    }

    if(petnumber && !stampeded)
    {
        Position pos;
        owner->GetFirstCollisionPosition(pos, PET_FOLLOW_DIST, PET_FOLLOW_ANGLE);
        Relocate(pos.m_positionX, pos.m_positionY, pos.m_positionZ, owner->GetOrientation());
    }

    if (!IsPositionValid())
    {
        sLog->outError(LOG_FILTER_PETS, "Pet (guidlow %d, entry %d) not loaded. Suggested coordinates isn't valid (X: %f Y: %f)",
            GetGUIDLow(), GetEntry(), GetPositionX(), GetPositionY());
        return false;
    }

    setFaction(owner->getFaction());
    SetUInt32Value(UNIT_CREATED_BY_SPELL, summon_spell_id);

    CreatureTemplate const* cinfo = GetCreatureTemplate();
    if (cinfo->type == CREATURE_TYPE_CRITTER)
    {
        map->AddToMap(this->ToCreature());
        return true;
    }

    m_charmInfo->SetPetNumber(pet_number, IsPermanentPetFor(owner));

    SetDisplayId(fields[3].GetUInt32());
    SetNativeDisplayId(fields[3].GetUInt32());
    uint32 petlevel = fields[4].GetUInt16();
    SetUInt32Value(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_NONE);
    SetName(fields[7].GetString());

    // ignore model info data for 
    SetFloatValue(UNIT_FIELD_BOUNDINGRADIUS, DEFAULT_WORLD_OBJECT_SIZE);
    SetFloatValue(UNIT_FIELD_COMBATREACH, ATTACK_DISTANCE);

    switch (getPetType())
    {
        case SUMMON_PET:
        {
            petlevel = owner->getLevel();
            SetUInt32Value(UNIT_FIELD_FLAGS, UNIT_FLAG_PVP_ATTACKABLE);
                                                            // this enables popup window (pet dismiss, cancel)
            if (owner->getClass() == CLASS_WARLOCK)
                SetClass(CLASS_ROGUE);
            break;
        }
        case HUNTER_PET:
        {
            SetSheath(SHEATH_STATE_MELEE);
            SetClass(CLASS_HUNTER);
            SetGender(GENDER_NONE);

            SetByteFlag(UNIT_FIELD_BYTES_2, 2, fields[8].GetBool() ? UNIT_CAN_BE_ABANDONED : UNIT_CAN_BE_RENAMED | UNIT_CAN_BE_ABANDONED);
            SetUInt32Value(UNIT_FIELD_FLAGS, UNIT_FLAG_PVP_ATTACKABLE);
                                                            // this enables popup window (pet abandon, cancel)
            SetSpecializationId(fields[15].GetUInt32());
            break;
        }
        default:
            if (!IsPetGhoul())
                sLog->outError(LOG_FILTER_PETS, "Pet have incorrect type (%u) for pet loading.", getPetType());
            break;
    }

    SetUInt32Value(UNIT_FIELD_PET_NAME_TIMESTAMP, uint32(time(NULL))); // cast can't be helped here
    SetCreatorGUID(owner->GetGUID());

    SetReactState(ReactStates(fields[6].GetUInt8()));
    if (GetReactState() == REACT_AGGRESSIVE)
        SetReactState(REACT_DEFENSIVE);

    InitStatsForLevel(petlevel);
    SetUInt32Value(UNIT_FIELD_PETEXPERIENCE, fields[5].GetUInt32());

    SynchronizeLevelWithOwner();
    SetCanModifyStats(true);

    if (!stampeded)
    {
        uint32 savedhealth = fields[9].GetUInt32();
        uint32 savedmana = fields[10].GetUInt32();

        if (!savedhealth && getPetType() == HUNTER_PET)
            setDeathState(JUST_DIED);
        else if (getPetType() == HUNTER_PET)
        {
            SetHealth(savedhealth > GetMaxHealth() ? GetMaxHealth() : savedhealth);
            SetPower(getPowerType(), savedmana > uint32(GetMaxPower(getPowerType())) ? GetMaxPower(getPowerType()) : savedmana);
        }
    }

    owner->SetMinion(this, true, stampeded);
    map->AddToMap(this->ToCreature());
    setActive(true);

    SetSlot(owner->m_currentSummonedSlot);

    uint32 timediff = uint32(time(NULL) - fields[12].GetUInt32());
    _LoadAuras(timediff);

    if (owner->GetTypeId() == TYPEID_PLAYER && owner->ToPlayer()->InArena())
        RemoveArenaAuras();

    // load action bar, if data broken will fill later by default spells.
    m_charmInfo->LoadPetActionBar(fields[11].GetString());

    _LoadSpells();
    _LoadSpellCooldowns();
    LearnPetPassives();

    if(owner->HasSpell(108415)) // active talent Soul Link
        CastSpell(this, 108446, true);

    if(!stampeded)
    {
        CleanupActionBar();                                     // remove unknown spells from action bar after load
        owner->PetSpellInitialize();
    }

    sLog->outDebug(LOG_FILTER_PETS, "New Pet has guid %u", GetGUIDLow());

    if (owner->GetGroup())
        owner->SetGroupUpdateFlag(GROUP_UPDATE_PET);

    owner->SendTalentsInfoData(true);

    if (getPetType() == HUNTER_PET)
    {
        PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_PET_DECLINED_NAME);
        stmt->setUInt32(0, owner->GetGUIDLow());
        stmt->setUInt32(1, GetCharmInfo()->GetPetNumber());
        PreparedQueryResult result = CharacterDatabase.Query(stmt);

        if (result)
        {
            delete m_declinedname;
            m_declinedname = new DeclinedName;
            Field* fields2 = result->Fetch();
            for (uint8 i = 0; i < MAX_DECLINED_NAME_CASES; ++i)
            {
                m_declinedname->name[i] = fields2[i].GetString();
            }
        }
    }

    //set last used pet number (for use in BG's)
    if (owner->GetTypeId() == TYPEID_PLAYER && isControlled() && !isTemporarySummoned() && getPetType() == HUNTER_PET && !m_Stampeded)
        owner->ToPlayer()->SetLastPetNumber(pet_number);

    CastPetAuras(true);
    m_loading = false;
    return true;
}

void Pet::SavePetToDB(bool isDelete)
{
    //sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "Pet SavePetToDB");
    if (!GetEntry() || m_Stampeded)
        return;

    // save only fully controlled creature
    if (!isControlled())
        return;

    //Don`t save temporary pets
    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(GetUInt32Value(UNIT_CREATED_BY_SPELL));
    bool is_temporary_summoned = spellInfo && spellInfo->GetDuration() > 0;
    if (is_temporary_summoned)
        return;

    Player* owner = (Player*)GetOwner();
    // not save not player pets
    if (!IS_PLAYER_GUID(GetOwnerGUID()) || !owner)
        return;

    // In some cases pet could get 0 id (stamped case)
    if (!m_charmInfo->GetPetNumber())
        return;

    PetSlot curentSlot = GetSlot();

    //not delete, just remove from curent slot
    if((m_owner->getClass() == CLASS_WARLOCK || m_owner->getClass() == CLASS_DEATH_KNIGHT) && isDelete)
        owner->m_currentPetNumber = 0;

    // not save pet as current if another pet temporary unsummoned
    if (owner->GetTemporaryUnsummonedPetNumber() && owner->GetTemporaryUnsummonedPetNumber() != m_charmInfo->GetPetNumber())
    {
        // pet will lost anyway at restore temporary unsummoned
        if (getPetType() == HUNTER_PET)
            return;
    }

    uint32 curhealth = GetHealth();
    uint32 curmana = GetPower(POWER_MANA);

    SQLTransaction trans = CharacterDatabase.BeginTransaction();
    // save auras before possibly removing them
    if (getPetType() == HUNTER_PET)
        _SaveAuras(trans);
    _SaveSpells(trans);
    _SaveSpellCooldowns(trans);
    CharacterDatabase.CommitTransaction(trans);

    //sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "SavePetToDB petentry %i, petnumber %i, slotID %i ownerid %i", GetEntry(), m_charmInfo->GetPetNumber(), curentSlot, GetOwnerGUID());

    // current/stable/not_in_slot
    if (!isDelete)
    {
        uint32 ownerLowGUID = GUID_LOPART(GetOwnerGUID());

        // save pet
        uint8 index = 0;
        std::ostringstream ss;

        PreparedStatement *stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_PET);
        stmt->setUInt32(index++, m_charmInfo->GetPetNumber());
        stmt->setUInt32(index++, GetEntry());
        stmt->setUInt32(index++, ownerLowGUID);
        stmt->setUInt32(index++, GetNativeDisplayId());
        stmt->setUInt32(index++, getLevel());
        stmt->setUInt32(index++, GetUInt32Value(UNIT_FIELD_PETEXPERIENCE));
        stmt->setUInt8(index++, GetReactState());
        stmt->setString(index++, GetName());
        stmt->setUInt8(index++, HasByteFlag(UNIT_FIELD_BYTES_2, 2, UNIT_CAN_BE_RENAMED) ? 0 : 1);
        stmt->setUInt32(index++, curhealth);
        stmt->setUInt32(index++, curmana);

        ss.str("");
        for (uint32 i = ACTION_BAR_INDEX_START; i < ACTION_BAR_INDEX_END; ++i)
        {
            ss << uint32(m_charmInfo->GetActionBarEntry(i)->GetType()) << ' '
                << uint32(m_charmInfo->GetActionBarEntry(i)->GetAction()) << ' ';
        };

        stmt->setString(index++, ss.str());

        stmt->setUInt32(index++, time(NULL));
        stmt->setUInt32(index++, GetUInt32Value(UNIT_CREATED_BY_SPELL));
        stmt->setUInt8(index++, getPetType());
        stmt->setUInt32(index++, GetSpecializationId());

        CharacterDatabase.Execute(stmt);
    }
    // delete
    else
    {
        if((curentSlot >= PET_SLOT_HUNTER_FIRST && curentSlot <= owner->GetMaxCurentPetSlot()))
            owner->cleanPetSlotForMove(curentSlot, m_charmInfo->GetPetNumber());     //could be already remove by early call this function
        RemoveAllAuras();
        DeleteFromDB(m_charmInfo->GetPetNumber());
    }
}

void Pet::DeleteFromDB(uint32 guidlow)
{
    SQLTransaction trans = CharacterDatabase.BeginTransaction();

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CHAR_PET_BY_ID);
    stmt->setUInt32(0, guidlow);
    trans->Append(stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CHAR_PET_DECLINED_BY_ID);
    stmt->setUInt32(0, guidlow);
    trans->Append(stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CHAR_PET_AURA_BY_ID);
    stmt->setUInt32(0, guidlow);
    trans->Append(stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CHAR_PET_SPELL_BY_ID);
    stmt->setUInt32(0, guidlow);
    trans->Append(stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CHAR_PET_SPELL_COOLDOWN_BY_ID);
    stmt->setUInt32(0, guidlow);
    trans->Append(stmt);

    CharacterDatabase.CommitTransaction(trans);
}

void Pet::setDeathState(DeathState s)                       // overwrite virtual Creature::setDeathState and Unit::setDeathState
{
    Creature::setDeathState(s);
    if (getDeathState() == CORPSE)
    {
        if (getPetType() == HUNTER_PET)
        {
            // pet corpse non lootable and non skinnable
            SetUInt32Value(OBJECT_FIELD_DYNAMIC_FLAGS, UNIT_DYNFLAG_NONE);
            RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_SKINNABLE);
            //SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_STUNNED);
        }
    }
    else if (getDeathState() == ALIVE)
    {
        if (getPetType() == HUNTER_PET)
        {
            if (Unit* owner = GetOwner())
                if (Player* player = owner->ToPlayer())
                    player->StopCastingCharm();
        }
    }
    if (s == ALIVE)
        CastPetAuras(true);
    else if (s == JUST_DIED)
        CastPetAuras(false);
}

void Pet::Update(uint32 diff)
{
    if (m_removed)                                           // pet already removed, just wait in remove queue, no updates
        return;

    if (m_loading)
        return;
    
    Unit* owner = GetOwner();
    if (!owner || owner->GetTypeId() != TYPEID_PLAYER || m_Update)
        return;

    m_Update = true;

    // Glyph of Animal Bond
    if (owner->HasAura(20895) && !owner->HasAura(24529))
        AddAura(24529, this);
    
    if (!owner->HasAura(20895))
        RemoveAurasDueToSpell(24529);
    
    switch (m_deathState)
    {
        case CORPSE:
        {
            if (getPetType() != HUNTER_PET || m_corpseRemoveTime <= time(NULL))
            {
                Remove();               //hunters' pets never get removed because of death, NEVER!
                m_Update = false;
                return;
            }
            break;
        }
        case ALIVE:
        {
            // unsummon pet that lost owner
            Unit* owner = GetOwner();
            if (!owner || (!IsWithinDistInMap(owner, GetMap()->GetVisibilityRange()) && !isPossessed()) || (isControlled() && !owner->GetPetGUID() && !m_Stampeded))
            {
                //sLog->outDebug(LOG_FILTER_PETS, "Pet::Update GetPetGUID %i", owner ? owner->GetPetGUID() : 0);
                Remove();
                m_Update = false;
                return;
            }

            if (isControlled())
            {
                if (owner->GetPetGUID() != GetGUID() && !m_Stampeded) // Stampede
                {
                    sLog->outError(LOG_FILTER_PETS, "Pet %u is not pet of owner %s, removed", GetEntry(), m_owner->GetName());
                    Remove();
                    m_Update = false;
                    return;
                }
            }

            if (m_duration > 0)
            {
                if (uint32(m_duration) > diff)
                    m_duration -= diff;
                else
                {
                    Remove();
                    m_Update = false;
                    return;
                }
            }
            break;
        }
        default:
            break;
    }
    Creature::Update(diff);
    m_Update = false;
}

void Creature::Regenerate(Powers power)
{
    int32 maxValue = GetMaxPower(power);

    uint32 powerIndex = GetPowerIndexByClass(power, getClass());
    if (powerIndex == MAX_POWERS)
        return;

    int32 saveCur = GetPower(power);
    int32 curValue = saveCur;

    if (!maxValue || curValue == maxValue)
        return;

    float addvalue = 0.0f;

    switch (power)
    {
        case POWER_MANA:
        {
            // Combat and any controlled creature
            if (isInCombat() || GetCharmerOrOwnerGUID())
            {
                float ManaIncreaseRate = sWorld->getRate(RATE_POWER_MANA);
                float Spirit = GetStat(STAT_SPIRIT);
                addvalue = uint32((Spirit / 5.0f + 17.0f) * ManaIncreaseRate);
            }
            else
                addvalue = maxValue / 3;
            break;
        }
        case POWER_FOCUS:
        {
            float defaultreg = 0.005f * m_petregenTimer;
            addvalue += defaultreg * m_baseRHastRatingPct * sWorld->getRate(RATE_POWER_FOCUS);
            break;
        }
        case POWER_ENERGY:
        {
            float defaultreg = 0.01f * m_petregenTimer;
            addvalue += defaultreg * m_baseMHastRatingPct * sWorld->getRate(RATE_POWER_ENERGY);
            break;
        }
        default:
            return;
    }

    // Apply modifiers (if any).
    addvalue += GetFloatValue(UNIT_FIELD_POWER_REGEN_FLAT_MODIFIER + powerIndex) * (0.001f * m_petregenTimer);
    addvalue += GetTotalAuraModifierByMiscValue(SPELL_AURA_MOD_POWER_REGEN, power) * ((power != POWER_ENERGY) ? m_regenTimerCount : m_petregenTimer) / (5 * IN_MILLISECONDS);

    if (addvalue <= 0.0f)
    {
        if (curValue == 0)
            return;
    }
    else if (addvalue > 0.0f)
    {
        if (saveCur == maxValue)
            return;
    }
    else
        return;

    if (this->IsAIEnabled)
        this->AI()->RegeneratePower(power, addvalue);

    int32 integerValue = uint32(fabs(addvalue));

    if (addvalue < 0.0f)
    {
        if (curValue > integerValue)
            curValue -= integerValue;
        else
            curValue = 0;
    }
    else
    {
        curValue += integerValue;
        if (curValue > maxValue)
            curValue = maxValue;
    }

    if ((saveCur != maxValue && curValue == maxValue) || m_regenTimerCount >= uint32(isAnySummons() ? PET_FOCUS_REGEN_INTERVAL : CREATURE_REGEN_INTERVAL))
    {
        SetInt32Value(UNIT_FIELD_POWER1 + powerIndex, curValue);
        m_regenTimerCount -= (isAnySummons() ? PET_FOCUS_REGEN_INTERVAL : CREATURE_REGEN_INTERVAL);
    }
    else
        UpdateInt32Value(UNIT_FIELD_POWER1 + powerIndex, curValue);

    m_petregenTimer = 0;
}

void Pet::Remove()
{
    if (Player* plr = m_owner->ToPlayer())
        plr->RemovePet(this);
}

void Pet::GivePetXP(uint32 xp)
{
    if (getPetType() != HUNTER_PET)
        return;

    if (xp < 1)
        return;

    if (!isAlive())
        return;

    uint8 maxlevel = std::min((uint8)sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL), GetOwner()->getLevel());
    uint8 petlevel = getLevel();

    // If pet is detected to be at, or above(?) the players level, don't hand out XP
    if (petlevel >= maxlevel)
       return;

    uint32 nextLvlXP = GetUInt32Value(UNIT_FIELD_PETNEXTLEVELEXP);
    uint32 curXP = GetUInt32Value(UNIT_FIELD_PETEXPERIENCE);
    uint32 newXP = curXP + xp;

    // Check how much XP the pet should receive, and hand off have any left from previous levelups
    while (newXP >= nextLvlXP && petlevel < maxlevel)
    {
        // Subtract newXP from amount needed for nextlevel, and give pet the level
        newXP -= nextLvlXP;
        ++petlevel;

        GivePetLevel(petlevel);

        nextLvlXP = GetUInt32Value(UNIT_FIELD_PETNEXTLEVELEXP);
    }
    // Not affected by special conditions - give it new XP
    SetUInt32Value(UNIT_FIELD_PETEXPERIENCE, petlevel < maxlevel ? newXP : 0);
}

void Pet::GivePetLevel(uint8 level)
{
    if (!level || level == getLevel())
        return;

    if (getPetType()==HUNTER_PET)
    {
        SetUInt32Value(UNIT_FIELD_PETEXPERIENCE, 0);
        SetUInt32Value(UNIT_FIELD_PETNEXTLEVELEXP, uint32(sObjectMgr->GetXPForLevel(level)*PET_XP_FACTOR));
    }

    InitStatsForLevel(level);
}

bool Pet::CreateBaseAtCreature(Creature* creature)
{
    ASSERT(creature);

    if (!CreateBaseAtTamed(creature->GetCreatureTemplate(), creature->GetMap(), creature->GetPhaseMask()))
        return false;

    Relocate(creature->GetPositionX(), creature->GetPositionY(), creature->GetPositionZ(), creature->GetOrientation());

    if (!IsPositionValid())
    {
        sLog->outError(LOG_FILTER_PETS, "Pet (guidlow %d, entry %d) not created base at creature. Suggested coordinates isn't valid (X: %f Y: %f)",
            GetGUIDLow(), GetEntry(), GetPositionX(), GetPositionY());
        return false;
    }

    CreatureTemplate const* cinfo = GetCreatureTemplate();
    if (!cinfo)
    {
        sLog->outError(LOG_FILTER_PETS, "CreateBaseAtCreature() failed, creatureInfo is missing!");
        return false;
    }

    SetDisplayId(creature->GetDisplayId());

    if (CreatureFamilyEntry const* cFamily = sCreatureFamilyStore.LookupEntry(cinfo->family))
        SetName(cFamily->Name);
    else
        SetName(creature->GetNameForLocaleIdx(sObjectMgr->GetDBCLocaleIndex()));

    return true;
}

bool Pet::CreateBaseAtCreatureInfo(CreatureTemplate const* cinfo, Unit* owner)
{
    if (!CreateBaseAtTamed(cinfo, owner->GetMap(), owner->GetPhaseMask()))
        return false;

    if (CreatureFamilyEntry const* cFamily = sCreatureFamilyStore.LookupEntry(cinfo->family))
        SetName(cFamily->Name);

    Relocate(owner->GetPositionX(), owner->GetPositionY(), owner->GetPositionZ(), owner->GetOrientation());

    return true;
}

bool Pet::CreateBaseAtTamed(CreatureTemplate const* cinfo, Map* map, uint32 phaseMask)
{
    sLog->outDebug(LOG_FILTER_PETS, "Pet::CreateBaseForTamed");
    uint32 guid=sObjectMgr->GenerateLowGuid(HIGHGUID_PET);
    uint32 pet_number = sObjectMgr->GeneratePetNumber();
    if (!Create(guid, map, phaseMask, cinfo->Entry, pet_number))
        return false;

    setPowerType(POWER_FOCUS);
    SetUInt32Value(UNIT_FIELD_PET_NAME_TIMESTAMP, 0);
    SetUInt32Value(UNIT_FIELD_PETEXPERIENCE, 0);
    SetUInt32Value(UNIT_FIELD_PETNEXTLEVELEXP, uint32(sObjectMgr->GetXPForLevel(getLevel()+1)*PET_XP_FACTOR));
    SetUInt32Value(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_NONE);

    if (cinfo->type == CREATURE_TYPE_BEAST)
    {
        SetClass(CLASS_WARRIOR);
        SetGender(GENDER_NONE);
        SetFieldPowerType(POWER_FOCUS);
        SetUInt32Value(UNIT_FIELD_DISPLAY_POWER, 0x2);
        SetSheath(SHEATH_STATE_MELEE);
        SetByteFlag(UNIT_FIELD_BYTES_2, 2, UNIT_CAN_BE_RENAMED | UNIT_CAN_BE_ABANDONED);
    }

    return true;
}

// TODO: Move stat mods code to pet passive auras
bool Guardian::InitStatsForLevel(uint8 petlevel)
{
    CreatureTemplate const* cinfo = GetCreatureTemplate();
    ASSERT(cinfo);

    SetLevel(petlevel);
    Unit* owner = GetOwner();
    bool damageSet = false;

    //Determine pet type
    PetType petType = getPetType();

    uint32 creature_ID = (petType == HUNTER_PET) ? 1 : cinfo->Entry;

    //sLog->outDebug(LOG_FILTER_PETS, "Guardian::InitStatsForLevel owner %u creature_ID %i petlevel %i", owner ? owner->GetGUID() : 0, creature_ID, petlevel);

    SetMeleeDamageSchool(SpellSchools(cinfo->dmgschool));

    SetModifierValue(UNIT_MOD_ARMOR, BASE_VALUE, float(petlevel*50));

    UpdateMeleeHastMod();
    UpdateHastMod();
    UpdateRangeHastMod();

    SetUInt32Value(UNIT_FIELD_FLAGS_2, cinfo->unit_flags2);

    // Resistance
    for (uint8 i = SPELL_SCHOOL_HOLY; i < MAX_SPELL_SCHOOL; ++i)
        SetModifierValue(UnitMods(UNIT_MOD_RESISTANCE_START + i), BASE_VALUE, float(cinfo->resistance[i]));

    CreatureBaseStats const* stats = sObjectMgr->GetCreatureBaseStats(petlevel, cinfo->unit_class);

    //health, mana, armor and resistance
    if(!InitBaseStat(creature_ID, damageSet))
    {
        SetCreateHealth(stats->BaseHealth[cinfo->expansion]);
        SetCreateMana(stats->BaseMana);
        SetPower(POWER_MANA, GetMaxPower(POWER_MANA));
    }

    if(owner && (owner->getClass() == CLASS_HUNTER || owner->getClass() == CLASS_WARLOCK))
    {
        SetCreateStat(STAT_STRENGTH, 22 + (petlevel * 6.2));
        SetCreateStat(STAT_AGILITY, 23 + (petlevel * 13.3));
        SetCreateStat(STAT_STAMINA, 22 + (petlevel * 4.4));
        SetCreateStat(STAT_INTELLECT, 20 + (petlevel * 0.6));
        SetCreateStat(STAT_SPIRIT, 20 + (petlevel * 1.17));
    }
    else
    {
        SetCreateStat(STAT_STRENGTH, 21 + (petlevel * 5.566));
        SetCreateStat(STAT_AGILITY, 17 + (petlevel * 29.92));
        SetCreateStat(STAT_STAMINA, 21 + (petlevel * 1.277));
        SetCreateStat(STAT_INTELLECT, 14 + (petlevel * 4.45));
        SetCreateStat(STAT_SPIRIT, 10 + (petlevel * 4.6));
    }

    SetBonusDamage(0);
    UpdateAllStats();

    if(!damageSet)
    {
        SetBaseWeaponDamage(BASE_ATTACK, MINDAMAGE, float(petlevel - (petlevel / 4) + cinfo->mindmg));
        SetBaseWeaponDamage(BASE_ATTACK, MAXDAMAGE, float(petlevel + (petlevel / 4) + cinfo->maxdmg));
    }

    if (petType == HUNTER_PET)
    {
        SetUInt32Value(UNIT_FIELD_PETNEXTLEVELEXP, uint32(sObjectMgr->GetXPForLevel(petlevel)*PET_XP_FACTOR));
        if (Player * plr = m_owner->ToPlayer())
        {
            float ratingBonusVal = plr->GetRatingBonusValue(CR_HASTE_RANGED);
            float val = 1.0f;

            ApplyPercentModFloatVar(val, ratingBonusVal, false);
            CalcAttackTimePercentMod(BASE_ATTACK, val);
        }
    }
    else
        SetUInt32Value(UNIT_FIELD_PETNEXTLEVELEXP, 1000);

    InitLevelupSpellsForLevel();

    SetFullHealth();
    return true;
}

void TempSummon::ToggleAutocast(SpellInfo const* spellInfo, bool apply)
{
    if (!spellInfo->IsAutocastable())
        return;

    uint32 spellid = spellInfo->Id;

    PetSpellMap::iterator itr = m_spells.find(spellid);
    if (itr == m_spells.end())
        return;

    uint32 i;

    if (apply)
    {
        for (i = 0; i < m_autospells.size() && m_autospells[i] != spellid; ++i)
            ;                                               // just search

        if (i == m_autospells.size())
        {
            m_autospells.push_back(spellid);

            if (itr->second.active != ACT_ENABLED)
            {
                itr->second.active = ACT_ENABLED;
                if (itr->second.state != PETSPELL_NEW)
                    itr->second.state = PETSPELL_CHANGED;
            }
        }
    }
    else
    {
        AutoSpellList::iterator itr2 = m_autospells.begin();
        for (i = 0; i < m_autospells.size() && m_autospells[i] != spellid; ++i, ++itr2)
            ;                                               // just search

        if (i < m_autospells.size())
        {
            m_autospells.erase(itr2);
            if (itr->second.active != ACT_DISABLED)
            {
                itr->second.active = ACT_DISABLED;
                if (itr->second.state != PETSPELL_NEW)
                    itr->second.state = PETSPELL_CHANGED;
            }
        }
    }
}

bool Pet::HaveInDiet(ItemTemplate const* item) const
{
    if (item->SubClass != ITEM_SUBCLASS_FOOD_DRINK)
        return false;

    CreatureTemplate const* cInfo = GetCreatureTemplate();
    if (!cInfo)
        return false;

    CreatureFamilyEntry const* cFamily = sCreatureFamilyStore.LookupEntry(cInfo->family);
    if (!cFamily)
        return false;

    uint32 diet = cFamily->petFoodMask;
    uint32 FoodMask = 1 << (item->FoodType-1);
    return diet & FoodMask;
}

uint32 Pet::GetCurrentFoodBenefitLevel(uint32 itemlevel)
{
    // -5 or greater food level
    if (getLevel() <= itemlevel + 5)                         //possible to feed level 60 pet with level 55 level food for full effect
        return 35000;
    // -10..-6
    else if (getLevel() <= itemlevel + 10)                   //pure guess, but sounds good
        return 17000;
    // -14..-11
    else if (getLevel() <= itemlevel + 14)                   //level 55 food gets green on 70, makes sense to me
        return 8000;
    // -15 or less
    else
        return 0;                                           //food too low level
}

void Pet::_LoadSpellCooldowns()
{
    m_CreatureSpellCooldowns.clear();
    m_CreatureCategoryCooldowns.clear();

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_PET_SPELL_COOLDOWN);
    stmt->setUInt32(0, m_charmInfo->GetPetNumber());
    PreparedQueryResult result = CharacterDatabase.Query(stmt);

    if (result)
    {
        ObjectGuid guid = GetGUID();
        time_t curTime = time(NULL);
        uint32 count = 0;

        //! 5.4.1
        WorldPacket data(SMSG_SPELL_COOLDOWN, size_t(8+1+result->GetRowCount()*8));

        data.WriteGuidMask<4, 7, 6>(guid);
        size_t count_pos = data.bitwpos();
        data.WriteBits(0, 21);
        data.WriteGuidMask<2, 3, 1, 0>(guid);
        data.WriteBit(1);
        data.WriteGuidMask<5>(guid);

        data.WriteGuidBytes<7, 2, 1, 6, 5, 4, 3, 0>(guid);

        do
        {
            Field* fields = result->Fetch();

            uint32 spell_id = fields[0].GetUInt32();
            time_t db_time  = time_t(fields[1].GetUInt32());

            if (!sSpellMgr->GetSpellInfo(spell_id))
            {
                sLog->outError(LOG_FILTER_PETS, "Pet %u have unknown spell %u in `pet_spell_cooldown`, skipping.", m_charmInfo->GetPetNumber(), spell_id);
                continue;
            }

            // skip outdated cooldown
            if (db_time <= curTime)
                continue;

            data << uint32(uint32(db_time-curTime)*IN_MILLISECONDS);
            data << uint32(spell_id);

            _AddCreatureSpellCooldown(spell_id, db_time);

            sLog->outDebug(LOG_FILTER_PETS, "Pet (Number: %u) spell %u cooldown loaded (%u secs).", m_charmInfo->GetPetNumber(), spell_id, uint32(db_time-curTime));
        }
        while (result->NextRow());

        data.FlushBits();
        data.PutBits(count_pos, count, 21);

        if (!m_CreatureSpellCooldowns.empty() && GetOwner())
            ((Player*)GetOwner())->GetSession()->SendPacket(&data);
    }
}

void Pet::_SaveSpellCooldowns(SQLTransaction& trans)
{
    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_PET_SPELL_COOLDOWNS);
    stmt->setUInt32(0, m_charmInfo->GetPetNumber());
    trans->Append(stmt);

    time_t curTime = time(NULL);

    // remove oudated and save active
    for (CreatureSpellCooldowns::iterator itr = m_CreatureSpellCooldowns.begin(); itr != m_CreatureSpellCooldowns.end();)
    {
        if (itr->second <= curTime)
            m_CreatureSpellCooldowns.erase(itr++);
        else
        {
            stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_PET_SPELL_COOLDOWN);
            stmt->setUInt32(0, m_charmInfo->GetPetNumber());
            stmt->setUInt32(1, itr->first);
            stmt->setUInt32(2, uint32(itr->second));
            trans->Append(stmt);

            ++itr;
        }
    }
}

void Pet::_LoadSpells()
{
    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_PET_SPELL);
    stmt->setUInt32(0, m_charmInfo->GetPetNumber());
    PreparedQueryResult result = CharacterDatabase.Query(stmt);

    if (result)
    {
        do
        {
            Field* fields = result->Fetch();

            addSpell(fields[0].GetUInt32(), ActiveStates(fields[1].GetUInt8()), PETSPELL_UNCHANGED);
        }
        while (result->NextRow());
    }
}

void Pet::_SaveSpells(SQLTransaction& trans)
{
    for (PetSpellMap::iterator itr = m_spells.begin(), next = m_spells.begin(); itr != m_spells.end(); itr = next)
    {
        ++next;

        // prevent saving family passives to DB
        if (itr->second.type == PETSPELL_FAMILY)
            continue;

        PreparedStatement* stmt;

        switch (itr->second.state)
        {
            case PETSPELL_REMOVED:
                stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_PET_SPELL_BY_SPELL);
                stmt->setUInt32(0, m_charmInfo->GetPetNumber());
                stmt->setUInt32(1, itr->first);
                trans->Append(stmt);

                m_spells.erase(itr);
                continue;
            case PETSPELL_CHANGED:
                stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_PET_SPELL_BY_SPELL);
                stmt->setUInt32(0, m_charmInfo->GetPetNumber());
                stmt->setUInt32(1, itr->first);
                trans->Append(stmt);

                stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_PET_SPELL);
                stmt->setUInt32(0, m_charmInfo->GetPetNumber());
                stmt->setUInt32(1, itr->first);
                stmt->setUInt8(2, itr->second.active);
                trans->Append(stmt);

                break;
            case PETSPELL_NEW:
                stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_PET_SPELL);
                stmt->setUInt32(0, m_charmInfo->GetPetNumber());
                stmt->setUInt32(1, itr->first);
                stmt->setUInt8(2, itr->second.active);
                trans->Append(stmt);
                break;
            case PETSPELL_UNCHANGED:
                continue;
        }
        itr->second.state = PETSPELL_UNCHANGED;
    }
}

void Pet::_LoadAuras(uint32 timediff)
{
    sLog->outDebug(LOG_FILTER_PETS, "Loading auras for pet %u", GetGUIDLow());

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_PET_AURA);
    stmt->setUInt32(0, m_charmInfo->GetPetNumber());
    PreparedQueryResult result = CharacterDatabase.Query(stmt);
    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_PET_AURA_EFFECT);
    stmt->setUInt32(0, m_charmInfo->GetPetNumber());
    PreparedQueryResult resultEffect = CharacterDatabase.Query(stmt);

    std::list<auraEffectData> auraEffectList;
    if(resultEffect)
    {
        do
        {
            Field* fields = resultEffect->Fetch();
            uint8 slot = fields[0].GetUInt8();
            uint8 effect = fields[1].GetUInt8();
            uint32 amount = fields[2].GetUInt32();
            uint32 baseamount = fields[3].GetUInt32();

            auraEffectList.push_back(auraEffectData(slot, effect, amount, baseamount));
        }
        while (resultEffect->NextRow());
    }

    if (result)
    {
        do
        {
            int32 damage[32];
            int32 baseDamage[32];
            Field* fields = result->Fetch();
            uint8 slot = fields[0].GetUInt8();
            uint64 caster_guid = fields[1].GetUInt64();
            // NULL guid stored - pet is the caster of the spell - see Pet::_SaveAuras
            if (!caster_guid)
                caster_guid = GetGUID();
            uint32 spellid = fields[2].GetUInt32();
            uint32 effmask = fields[3].GetUInt32();
            uint32 recalculatemask = fields[4].GetUInt32();
            uint8 stackcount = fields[5].GetUInt8();
            int32 maxduration = fields[6].GetInt32();
            int32 remaintime = fields[7].GetInt32();
            uint8 remaincharges = fields[8].GetUInt8();

            SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellid);
            if (!spellInfo)
            {
                sLog->outError(LOG_FILTER_PETS, "Unknown aura (spellid %u), ignore.", spellid);
                continue;
            }

            // negative effects should continue counting down after logout
            if (remaintime != -1 && !spellInfo->IsPositive())
            {
                if (remaintime/IN_MILLISECONDS <= int32(timediff))
                    continue;

                remaintime -= timediff*IN_MILLISECONDS;
            }

            // prevent wrong values of remaincharges
            if (spellInfo->ProcCharges)
            {
                if (remaincharges <= 0 || remaincharges > spellInfo->ProcCharges)
                    remaincharges = spellInfo->ProcCharges;
            }
            else
                remaincharges = 0;

            for(std::list<auraEffectData>::iterator itr = auraEffectList.begin(); itr != auraEffectList.end(); ++itr)
            {
                if(itr->_slot == slot)
                {
                    damage[itr->_effect] = itr->_amount;
                    baseDamage[itr->_effect] = itr->_baseamount;
                }
            }

            Aura* aura = Aura::TryCreate(spellInfo, effmask, this, NULL, &baseDamage[0], NULL, caster_guid);
            if (aura != NULL)
            {
                if (!aura->CanBeSaved())
                {
                    aura->Remove();
                    continue;
                }
                aura->SetLoadedState(maxduration, remaintime, remaincharges, stackcount, recalculatemask, &damage[0]);
                aura->ApplyForTargets();
                sLog->outInfo(LOG_FILTER_PETS, "Added aura spellid %u, effectmask %u", spellInfo->Id, effmask);
            }
        }
        while (result->NextRow());
    }
}

void Pet::_SaveAuras(SQLTransaction& trans)
{
    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_PET_AURAS);
    stmt->setUInt32(0, m_charmInfo->GetPetNumber());
    trans->Append(stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_PET_AURAS_EFFECTS);
    stmt->setUInt32(0, m_charmInfo->GetPetNumber());
    trans->Append(stmt);

    for (AuraMap::const_iterator itr = m_ownedAuras.begin(); itr != m_ownedAuras.end(); ++itr)
    {
        // check if the aura has to be saved
        if (!itr->second->CanBeSaved() || IsPetAura(itr->second))
            continue;

        Aura* aura = itr->second;
        AuraApplication * foundAura = GetAuraApplication(aura->GetId(), aura->GetCasterGUID(), aura->GetCastItemGUID());

        if(!foundAura)
            continue;

        int32 damage[MAX_SPELL_EFFECTS];
        int32 baseDamage[MAX_SPELL_EFFECTS];
        uint32 effMask = 0;
        uint32 recalculateMask = 0;
        uint8 index = 0;
        for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
        {
            if (aura->GetEffect(i))
            {
                index = 0;
                stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_PET_AURA_EFFECT);
                stmt->setUInt32(index++, m_charmInfo->GetPetNumber());
                stmt->setUInt8(index++, foundAura->GetSlot());
                stmt->setUInt8(index++, i);
                stmt->setInt32(index++, aura->GetEffect(i)->GetBaseAmount());
                stmt->setInt32(index++, aura->GetEffect(i)->GetAmount());

                trans->Append(stmt);

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

        // don't save guid of caster in case we are caster of the spell - guid for pet is generated every pet load, so it won't match saved guid anyways
        uint64 casterGUID = (itr->second->GetCasterGUID() == GetGUID()) ? 0 : itr->second->GetCasterGUID();

       
        index = 0;
        PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_PET_AURA);
        stmt->setUInt32(index++, m_charmInfo->GetPetNumber());
        stmt->setUInt8(index++, foundAura->GetSlot());
        stmt->setUInt64(index++, casterGUID);
        stmt->setUInt32(index++, itr->second->GetId());
        stmt->setUInt8(index++, effMask);
        stmt->setUInt8(index++, recalculateMask);
        stmt->setUInt8(index++, itr->second->GetStackAmount());
        stmt->setInt32(index++, itr->second->GetMaxDuration());
        stmt->setInt32(index++, itr->second->GetDuration());
        stmt->setUInt8(index++, itr->second->GetCharges());

        trans->Append(stmt);
    }
}

bool TempSummon::addSpell(uint32 spellId, ActiveStates active /*= ACT_DECIDE*/, PetSpellState state /*= PETSPELL_NEW*/, PetSpellType type /*= PETSPELL_NORMAL*/)
{
    //sLog->outError(LOG_FILTER_PETS, "TempSummon::addSpell spellId %i Entry %i", spellId, GetEntry());

    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId);
    if (!spellInfo)
    {
        // do pet spell book cleanup
        if (state == PETSPELL_UNCHANGED)                    // spell load case
        {
            sLog->outDebug(LOG_FILTER_PETS, "Pet::addSpell: Non-existed in SpellStore spell #%u request, deleting for all pets in `pet_spell`.", spellId);
            PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_INVALID_PET_SPELL);
            stmt->setUInt32(0, spellId);
            CharacterDatabase.Execute(stmt);
        }
        else
            sLog->outDebug(LOG_FILTER_PETS, "Pet::addSpell: Non-existed in SpellStore spell #%u request.", spellId);

        return false;
    }

    PetSpellMap::iterator itr = m_spells.find(spellId);
    if (itr != m_spells.end())
    {
        if (itr->second.state == PETSPELL_REMOVED)
        {
            m_spells.erase(itr);
            state = PETSPELL_CHANGED;
        }
        else if (state == PETSPELL_UNCHANGED && itr->second.state != PETSPELL_UNCHANGED)
        {
            // can be in case spell loading but learned at some previous spell loading
            itr->second.state = PETSPELL_UNCHANGED;

            if (active == ACT_ENABLED && !m_Stampeded)
                ToggleAutocast(spellInfo, true);
            else if (active == ACT_DISABLED)
                ToggleAutocast(spellInfo, false);

            return false;
        }
        else
            return false;
    }

    PetSpell newspell;
    newspell.state = state;
    newspell.type = type;

    if (active == ACT_DECIDE)                               // active was not used before, so we save it's autocast/passive state here
    {
        SpellInfo const* spellInfoDur = sSpellMgr->GetSpellInfo(GetUInt32Value(UNIT_CREATED_BY_SPELL));
        if(spellInfoDur && spellInfoDur->GetDuration() > 0 && spellInfo->GetMaxRange(false))
            newspell.active = ACT_ENABLED;
        else if (spellInfo->IsAutocastable())
            newspell.active = ACT_DISABLED;
        else
            newspell.active = ACT_PASSIVE;
    }
    else
        newspell.active = active;

    m_spells[spellId] = newspell;

    if(m_charmInfo)
    {
        if (spellInfo->IsPassive() && (!spellInfo->CasterAuraState || HasAuraState(AuraStateType(spellInfo->CasterAuraState))))
            CastSpell(this, spellId, true);
        else
            m_charmInfo->AddSpellToActionBar(spellInfo);
    }
    else
    {
        if (spellInfo->IsPassive() && (!spellInfo->CasterAuraState || HasAuraState(AuraStateType(spellInfo->CasterAuraState))))
            CastSpell(this, spellId, true);
        else
            AddPetCastSpell(spellId);

        if(GetCasterPet() && spellInfo->GetMaxRange(false) > GetAttackDist() && (spellInfo->AttributesCu & SPELL_ATTR0_CU_DIRECT_DAMAGE) && !spellInfo->IsTargetingAreaCast())
            SetAttackDist(spellInfo->GetMaxRange(false));

        //sLog->outDebug(LOG_FILTER_PETS, "TempSummon::addSpell guard GetMaxRange %f GetAttackDist %f GetCasterPet %i", spellInfo->GetMaxRange(false), GetAttackDist(), GetCasterPet());

        return false; //No info in spell for guard pet
    }

    if (newspell.active == ACT_ENABLED)
        ToggleAutocast(spellInfo, true);

    if(GetCasterPet() && spellInfo->GetMaxRange(false) > GetAttackDist() && spellInfo->IsAutocastable() && (spellInfo->AttributesCu & SPELL_ATTR0_CU_DIRECT_DAMAGE) && !spellInfo->IsTargetingAreaCast())
        SetAttackDist(spellInfo->GetMaxRange(false));

    //sLog->outDebug(LOG_FILTER_PETS, "TempSummon::addSpell pet GetMaxRange %f, active %i GetAttackDist %f GetCasterPet %i", spellInfo->GetMaxRange(false), newspell.active, GetAttackDist(), GetCasterPet());

    return true;
}

bool TempSummon::learnSpell(uint32 spell_id)
{
    // prevent duplicated entires in spell book
    if (!addSpell(spell_id))
        return false;

    if (!m_loading && m_owner->ToPlayer())
    {
        WorldPacket data(SMSG_PET_LEARNED_SPELL, 4);
        data.WriteBits(1, 22);
        data << uint32(spell_id);
        m_owner->ToPlayer()->GetSession()->SendPacket(&data);
        m_owner->ToPlayer()->PetSpellInitialize();
    }
    return true;
}

void TempSummon::InitLevelupSpellsForLevel()
{
    uint8 level = getLevel();

    //sLog->outDebug(LOG_FILTER_PETS, "TempSummon::InitLevelupSpellsForLevel level %i", level);

    if(m_charmInfo)
    {
        if (PetLevelupSpellSet const* levelupSpells = GetCreatureTemplate()->family ? sSpellMgr->GetPetLevelupSpellList(GetCreatureTemplate()->family) : NULL)
        {
            // PetLevelupSpellSet ordered by levels, process in reversed order
            for (PetLevelupSpellSet::const_reverse_iterator itr = levelupSpells->rbegin(); itr != levelupSpells->rend(); ++itr)
            {
                // will called first if level down
                if (itr->first > level)
                    unlearnSpell(itr->second);                 // will learn prev rank if any
                // will called if level up
                else
                    learnSpell(itr->second);                        // will unlearn prev rank if any
            }
        }

        int32 petSpellsId = GetCreatureTemplate()->PetSpellDataId ? -(int32)GetCreatureTemplate()->PetSpellDataId : GetEntry();

        // default spells (can be not learned if pet level (as owner level decrease result for example) less first possible in normal game)
        if (PetDefaultSpellsEntry const* defSpells = sSpellMgr->GetPetDefaultSpellsEntry(petSpellsId))
        {
            for (uint8 i = 0; i < MAX_CREATURE_SPELL_DATA_SLOT; ++i)
            {
                SpellInfo const* spellEntry = sSpellMgr->GetSpellInfo(defSpells->spellid[i]);
                if (!spellEntry)
                    continue;

                // will called first if level down
                if (spellEntry->SpellLevel > level)
                    unlearnSpell(spellEntry->Id);
                // will called if level up
                else
                    learnSpell(spellEntry->Id);
            }
        }
    }
    else
    {
        for (uint8 i=0; i < CREATURE_MAX_SPELLS; ++i)
        {
            if (!m_temlate_spells[i])
                continue;
            SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(m_temlate_spells[i]);
            if (!spellInfo)
                continue;

            learnSpell(spellInfo->Id);
        }
    }
}

bool TempSummon::unlearnSpell(uint32 spell_id)
{
    if (removeSpell(spell_id))
    {
        if (!m_loading && m_owner->ToPlayer())
        {
            WorldPacket data(SMSG_PET_REMOVED_SPELL, 4);
            data.WriteBits(1, 22);
            data << uint32(spell_id);
            if(Player* player = m_owner->ToPlayer())
                player->GetSession()->SendPacket(&data);
        }
        return true;
    }
    return false;
}

bool TempSummon::removeSpell(uint32 spell_id)
{
    PetSpellMap::iterator itr = m_spells.find(spell_id);
    if (itr == m_spells.end())
        return false;

    if (itr->second.state == PETSPELL_REMOVED)
        return false;

    if (itr->second.state == PETSPELL_NEW)
        m_spells.erase(itr);
    else
        itr->second.state = PETSPELL_REMOVED;

    RemoveAurasDueToSpell(spell_id);

    // if remove last rank or non-ranked then update action bar at server and client if need
    if (m_charmInfo && m_charmInfo->RemoveSpellFromActionBar(spell_id))
    {
        if (!m_loading)
        {
            // need update action bar for last removed rank
            if (Unit* owner = GetOwner())
                if (owner->GetTypeId() == TYPEID_PLAYER)
                    owner->ToPlayer()->PetSpellInitialize();
        }
    }

    return true;
}

void Pet::CleanupActionBar()
{
    for (uint8 i = 0; i < MAX_UNIT_ACTION_BAR_INDEX; ++i)
        if (UnitActionBarEntry const* ab = m_charmInfo->GetActionBarEntry(i))
            if (ab->GetAction() && ab->IsActionBarForSpell())
            {
                if (!HasSpell(ab->GetAction()))
                    m_charmInfo->SetActionBar(i, 0, ACT_PASSIVE);
                else if (ab->GetType() == ACT_ENABLED)
                {
                    if (SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(ab->GetAction()))
                        ToggleAutocast(spellInfo, true);
                }
            }
}

void Pet::InitPetCreateSpells()
{
    //sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "Pet InitPetCreateSpells");
    m_charmInfo->InitPetActionBar();
    m_spells.clear();

    LearnPetPassives();
    InitLevelupSpellsForLevel();
}

bool Pet::IsPermanentPetFor(Player* owner)
{
    switch (getPetType())
    {
        case SUMMON_PET:
            switch (owner->getClass())
            {
                case CLASS_WARLOCK:
                    return GetCreatureTemplate()->type == CREATURE_TYPE_DEMON;
                case CLASS_DEATH_KNIGHT:
                    return GetCreatureTemplate()->type == CREATURE_TYPE_UNDEAD;
                case CLASS_MAGE:
                    return GetCreatureTemplate()->type == CREATURE_TYPE_ELEMENTAL;
                default:
                    return false;
            }
        case HUNTER_PET:
            return true;
        default:
            return false;
    }
}

bool Pet::Create(uint32 guidlow, Map* map, uint32 phaseMask, uint32 Entry, uint32 pet_number)
{
    ASSERT(map);
    SetMap(map);

    SetPhaseMask(phaseMask, false);
    Object::_Create(guidlow, pet_number, HIGHGUID_PET);

    m_DBTableGuid = guidlow;
    m_originalEntry = Entry;

    if (!InitEntry(Entry))
        return false;

    SetSheath(SHEATH_STATE_MELEE);

    return true;
}

bool Pet::HasSpell(uint32 spell) const
{
    PetSpellMap::const_iterator itr = m_spells.find(spell);
    return itr != m_spells.end() && itr->second.state != PETSPELL_REMOVED;
}

// Get all passive spells in our skill line
void TempSummon::LearnPetPassives()
{
    CreatureTemplate const* cInfo = GetCreatureTemplate();
    if (!cInfo)
        return;

    CreatureFamilyEntry const* cFamily = sCreatureFamilyStore.LookupEntry(cInfo->family);
    if (!cFamily)
        return;

    PetFamilySpellsStore::const_iterator petStore = sPetFamilySpellsStore.find(cFamily->ID);
    if (petStore != sPetFamilySpellsStore.end())
    {
        // For general hunter pets skill 270
        // Passive 01~10, Passive 00 (20782, not used), Ferocious Inspiration (34457)
        // Scale 01~03 (34902~34904, bonus from owner, not used)
        for (PetFamilySpellsSet::const_iterator petSet = petStore->second.begin(); petSet != petStore->second.end(); ++petSet)
            addSpell(*petSet, ACT_DECIDE, PETSPELL_NEW, PETSPELL_FAMILY);
    }
}

void TempSummon::CastPetAuras(bool apply, uint32 spellId)
{
    if(m_Stampeded)
        return;

    //sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "Pet::CastPetAuras guid %u, apply %u, GetEntry() %u", GetGUIDLow(), apply, GetEntry());

    Unit* owner = GetAnyOwner();
    if (!owner || owner->GetTypeId() != TYPEID_PLAYER)
        return;

    uint32 createdSpellId = GetUInt32Value(UNIT_CREATED_BY_SPELL);

    if (std::vector<PetAura> const* petSpell = sSpellMgr->GetPetAura(GetEntry()))
    {
        for (std::vector<PetAura>::const_iterator itr = petSpell->begin(); itr != petSpell->end(); ++itr)
        {
            Unit* _target = this;
            Unit* _caster = this;
            Unit* _targetaura = this;

            if(itr->target == 1 || itr->target == 4) //get target owner
                _target = owner;

            if(itr->target == 2 || itr->target == 4) //set caster owner
                _caster = owner;

            if(itr->target == 3) //get target from spell chain
                _target = _target->GetTargetUnit();

            if(itr->targetaura == 1) //get target for aura owner
                _targetaura = owner;

            if(_target == NULL)
                _target = this;
            if(_caster == NULL)
                _caster = this;

            if(itr->aura > 0 && !_targetaura->HasAura(itr->aura))
                continue;
            if(itr->aura < 0 && _targetaura->HasAura(abs(itr->aura)))
                continue;
            if(itr->casteraura > 0 && !_caster->HasAura(itr->casteraura))
                continue;
            if(itr->casteraura < 0 && _caster->HasAura(abs(itr->casteraura)))
                continue;
            if(spellId != 0 && spellId != abs(itr->fromspell))
                continue;
            if(itr->createdspell != 0 && itr->createdspell != createdSpellId)
                continue;

            int32 bp0 = int32(itr->bp0);
            int32 bp1 = int32(itr->bp1);
            int32 bp2 = int32(itr->bp2);

            //sLog->outDebug(LOG_FILTER_PETS, "Pet::CastPetAuras PetAura bp0 %i, bp1 %i, bp2 %i, target %i", bp0, bp1, bp2, itr->target);

            if(itr->spellId > 0)
            {
                if (!apply)
                {
                    switch (itr->option)
                    {
                        case 4: //learn spell
                        {
                            if(Player* _lplayer = _target->ToPlayer())
                                _lplayer->removeSpell(itr->spellId);
                            else
                                removeSpell(itr->spellId);
                            break;
                        }
                        default:
                            break;
                    }
                    continue;
                }
                else
                {
                    switch (itr->option)
                    {
                        case 0: //cast spell without option
                            _caster->CastSpell(_target, itr->spellId, true);
                            break;
                        case 1: //cast custom spell option
                            _caster->CastCustomSpell(_target, itr->spellId, &bp0, &bp1, &bp2, true);
                            break;
                        case 2: //add aura
                            if (Aura* aura = _caster->AddAura(itr->spellId, _target))
                            {
                                if (bp0 && aura->GetEffect(0))
                                    aura->GetEffect(0)->SetAmount(bp0);
                                if (bp1 && aura->GetEffect(1))
                                    aura->GetEffect(1)->SetAmount(bp1);
                                if (bp2 && aura->GetEffect(2))
                                    aura->GetEffect(2)->SetAmount(bp2);
                            }
                            break;
                        case 3: //cast spell not triggered
                            _caster->CastSpell(_target, itr->spellId, false);
                            break;
                        case 4: //learn spell
                        {
                            if (Player* _lplayer = _target->ToPlayer())
                                _lplayer->learnSpell(itr->spellId, false);
                            else
                                learnSpell(itr->spellId);
                            break;
                        }
                    }
                }
            }
            else
            {
                if (apply)
                {
                    switch (itr->option)
                    {
                        case 4: //learn spell
                        {
                            if(Player* _lplayer = _target->ToPlayer())
                                _lplayer->removeSpell(abs(itr->spellId));
                            else
                                removeSpell(abs(itr->spellId));
                            break;
                        }
                        default:
                            _target->RemoveAurasDueToSpell(abs(itr->spellId));
                            break;
                    }
                    continue;
                }
                switch (itr->option)
                {
                    case 0: //cast spell without option
                        _caster->CastSpell(_target, abs(itr->spellId), true);
                        break;
                    case 1: //cast custom spell option
                        _caster->CastCustomSpell(_target, abs(itr->spellId), &bp0, &bp1, &bp2, true);
                        break;
                    case 2: //add aura
                        if(Aura* aura = _caster->AddAura(abs(itr->spellId), _target))
                        {
                            if(bp0 && aura->GetEffect(0))
                                aura->GetEffect(0)->SetAmount(bp0);
                            if(bp1 && aura->GetEffect(1))
                                aura->GetEffect(1)->SetAmount(bp1);
                            if(bp2 && aura->GetEffect(2))
                                aura->GetEffect(2)->SetAmount(bp2);
                        }
                        break;
                    case 3: //cast spell not triggered
                        _caster->CastSpell(_target, abs(itr->spellId), false);
                        break;
                    case 4: //learn spell
                    {
                        if(Player* _lplayer = _target->ToPlayer())
                            _lplayer->learnSpell(abs(itr->spellId), false);
                        else
                            learnSpell(abs(itr->spellId));
                        break;
                    }
                }
            }
        }
    }

    //for all hunters pets
    if(createdSpellId == 13481 || createdSpellId == 83245 || createdSpellId == 83244 || createdSpellId == 83243 || createdSpellId == 83242 || createdSpellId == 883)
    if (std::vector<PetAura> const* petSpell = sSpellMgr->GetPetAura(-1))
    {
        for (std::vector<PetAura>::const_iterator itr = petSpell->begin(); itr != petSpell->end(); ++itr)
        {
            Unit* _target = this;
            Unit* _caster = this;
            Unit* _targetaura = this;

            sLog->outDebug(LOG_FILTER_PETS, "CastPetAuras spellId %i", itr->spellId);

            if(itr->target == 1 || itr->target == 4) //get target owner
                _target = owner;

            if(itr->target == 2 || itr->target == 4) //set caster owner
                _caster = owner;

            if(itr->target == 3) //get target from spell chain
                _target = _target->GetTargetUnit();

            if(itr->targetaura == 1) //get target for aura owner
                _targetaura = owner;

            if(_target == NULL)
                _target = this;
            if(_caster == NULL)
                _caster = this;

            if(itr->aura > 0 && !_targetaura->HasAura(itr->aura))
                continue;
            if(itr->aura < 0 && _targetaura->HasAura(abs(itr->aura)))
                continue;
            if(itr->casteraura > 0 && !_caster->HasAura(itr->casteraura))
                continue;
            if(itr->casteraura < 0 && _caster->HasAura(abs(itr->casteraura)))
                continue;
            if(itr->createdspell != 0 && itr->createdspell != createdSpellId)
                continue;

            int32 bp0 = int32(itr->bp0);
            int32 bp1 = int32(itr->bp1);
            int32 bp2 = int32(itr->bp2);

            //sLog->outDebug(LOG_FILTER_PETS, "Pet::CastPetAuras PetAura bp0 %i, bp1 %i, bp2 %i, target %i", bp0, bp1, bp2, itr->target);

            if(itr->spellId > 0)
            {
                if (!apply)
                {
                    switch (itr->option)
                    {
                        case 4: //learn spell
                        {
                            if(Player* _lplayer = _target->ToPlayer())
                                _lplayer->removeSpell(itr->spellId);
                            else
                                removeSpell(itr->spellId);
                            break;
                        }
                        default:
                            _target->RemoveAurasDueToSpell(itr->spellId);
                            break;
                    }
                    continue;
                }
                if(spellId != 0 && spellId != abs(itr->fromspell))
                    continue;
                switch (itr->option)
                {
                    case 0: //cast spell without option
                        _caster->CastSpell(_target, itr->spellId, true);
                        break;
                    case 1: //cast custom spell option
                        _caster->CastCustomSpell(_target, itr->spellId, &bp0, &bp1, &bp2, true);
                        break;
                    case 2: //add aura
                        _caster->AddAura(itr->spellId, _target);
                        break;
                    case 3: //cast spell not triggered
                        _caster->CastSpell(_target, itr->spellId, false);
                        break;
                    case 4: //learn spell
                    {
                        if(Player* _lplayer = _target->ToPlayer())
                            _lplayer->learnSpell(itr->spellId, false);
                        else
                            learnSpell(itr->spellId);
                        break;
                    }
                }
            }
            else
            {
                if (apply)
                {
                    switch (itr->option)
                    {
                        case 4: //learn spell
                        {
                            if(Player* _lplayer = _target->ToPlayer())
                                _lplayer->removeSpell(abs(itr->spellId));
                            else
                                removeSpell(abs(itr->spellId));
                            break;
                        }
                        default:
                            _target->RemoveAurasDueToSpell(abs(itr->spellId));
                            break;
                    }
                    continue;
                }
                if(spellId != 0 && spellId != abs(itr->fromspell))
                    continue;
                switch (itr->option)
                {
                    case 0: //cast spell without option
                        _caster->CastSpell(_target, abs(itr->spellId), true);
                        break;
                    case 1: //cast custom spell option
                        _caster->CastCustomSpell(_target, abs(itr->spellId), &bp0, &bp1, &bp2, true);
                        break;
                    case 2: //add aura
                        _caster->AddAura(abs(itr->spellId), _target);
                        break;
                    case 3: //cast spell not triggered
                        _caster->CastSpell(_target, abs(itr->spellId), false);
                        break;
                    case 4: //learn spell
                    {
                        if(Player* _lplayer = _target->ToPlayer())
                            _lplayer->learnSpell(abs(itr->spellId), false);
                        else
                            learnSpell(abs(itr->spellId));
                        break;
                    }
                }
            }
        }
    }
}

bool Pet::IsPetAura(Aura const* aura)
{
    // if the owner has that pet aura, return true
    // pet auras
    if (std::vector<PetAura> const* petSpell = sSpellMgr->GetPetAura(GetEntry()))
    {
        for (std::vector<PetAura>::const_iterator itr = petSpell->begin(); itr != petSpell->end(); ++itr)
            if (itr->spellId == aura->GetId())
                return true;
    }
    return false;
}

void Pet::SynchronizeLevelWithOwner()
{
    Unit* owner = GetOwner();
    if (!owner || owner->GetTypeId() != TYPEID_PLAYER)
        return;

    switch (getPetType())
    {
        // always same level
        case SUMMON_PET:
            GivePetLevel(owner->getLevel());
            break;
        // always same level since 4.1.0
        case HUNTER_PET:
            GivePetLevel(owner->getLevel());
            break;
        default:
            break;
    }
}

void Pet::LearnSpecializationSpell()
{
    for (uint32 i = 0; i < sSpecializationSpellStore.GetNumRows(); i++)
    {
        SpecializationSpellEntry const* specializationEntry = sSpecializationSpellStore.LookupEntry(i);
        if (!specializationEntry)
            continue;

        if (specializationEntry->SpecializationEntry != GetSpecializationId())
            continue;

        learnSpell(specializationEntry->LearnSpell);
    }
}

void Pet::UnlearnSpecializationSpell()
{
    for (uint32 i = 0; i < sSpecializationSpellStore.GetNumRows(); i++)
    {
        SpecializationSpellEntry const* specializationEntry = sSpecializationSpellStore.LookupEntry(i);
        if (!specializationEntry)
            continue;

        if (specializationEntry->SpecializationEntry != GetSpecializationId())
            continue;

        unlearnSpell(specializationEntry->LearnSpell);
    }
}