/*
 * Copyright (C) 2005-2016 Uwow <http://uwow.biz/>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "Containers.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "Item.h"
#include "Group.h"
#include "GroupMgr.h"
#include "Spell.h"
#include "Util.h"
#include "World.h"
#include "WorldSession.h"
#include "InstanceSaveMgr.h"
#include "InstanceScript.h"
#include "BattlePetMgr.h"
#include "ReputationMgr.h"
#include "Battleground.h"
#include "BattlegroundMgr.h"
#include "AuctionHouseMgr.h"
#include "GameEventMgr.h"
#include "SocialMgr.h"
#include "AccountMgr.h"
#include "TicketMgr.h"
#include "RedisLoadPlayer.h"
#include "AchievementMgr.h"
#include "RedisBuilderMgr.h"

bool Pet::LoadPetFromRedis(Player* owner, uint32 petentry, uint32 petnumber, bool stampeded)
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
            sLog->outDebug(LOG_FILTER_PETS, "Pet::LoadPetFromRedis error m_currentSummonedSlot < 0");
            m_loading = false;
            return false;
        }
        else
            petnumber = owner->getPetIdBySlot(owner->m_currentSummonedSlot);
    }

    uint32 pet_number = 0;

    if (petnumber)
    {
        std::string petId = std::to_string(petnumber);
        PetData = owner->PlayerPetsJson[petId.c_str()];
        pet_number = petnumber;
    }
    else if (petentry)
    {
        std::string petId = std::to_string(petnumber);
        for (auto itr = owner->PlayerPetsJson.begin(); itr != owner->PlayerPetsJson.end(); ++itr)
        {
            auto petValue = (*itr);
            if (petValue["entry"].asUInt() == petentry)
            {
                pet_number = atoi(itr.memberName());
                PetData = petValue;
                break;
            }
        }
    }

    if (PetData.isNull())
    {
        sLog->outDebug(LOG_FILTER_PETS, "Pet::LoadPetFromRedis error result");
        m_loading = false;
        return false;
    }

    // update for case of current pet "slot = 0"
    petentry = PetData["entry"].asUInt();
    if (!petentry)
    {
        sLog->outDebug(LOG_FILTER_PETS, "Pet::LoadPetFromRedis error petentry");
        m_loading = false;
        return false;
    }

    // Double call if use Player::SummonPet
    // But this option for check through Player::LoadPet
    if (!owner->CanSummonPet(petentry))
    {
        sLog->outDebug(LOG_FILTER_PETS, "Pet::LoadPetFromRedis error CanSummonPet");
        m_loading = false;
        return false;
    }

    uint32 summon_spell_id = PetData["CreatedBySpell"].asUInt();
    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(summon_spell_id);

    bool is_temporary_summoned = spellInfo && spellInfo->GetDuration() > 0;

    // check temporary summoned pets like mage water elemental
    if (is_temporary_summoned && !stampeded)
    {
        m_loading = false;
        sLog->outDebug(LOG_FILTER_PETS, "Pet::LoadPetFromRedis error is_temporary_summoned %i", is_temporary_summoned);
        return false;
    }

    PetType pet_type = PetType(PetData["PetType"].asUInt());
    setPetType(pet_type);

    if (owner->GetTypeId() == TYPEID_PLAYER && isControlled() && !isTemporarySummoned())
        owner->SetLastPetEntry(petentry);

    if (pet_type == HUNTER_PET)
    {
        CreatureTemplate const* creatureInfo = sObjectMgr->GetCreatureTemplate(petentry);
        if (!creatureInfo || !creatureInfo->isTameable(owner->CanTameExoticPets()))
            return false;
    }

    if (owner->IsPetNeedBeTemporaryUnsummoned())
    {
        sLog->outDebug(LOG_FILTER_PETS, "Pet::LoadPetFromRedis error IsPetNeedBeTemporaryUnsummoned");
        owner->SetTemporaryUnsummonedPetNumber(pet_number);
        return false;
    }

    Map* map = owner->GetMap();
    uint32 guid = sObjectMgr->GenerateLowGuid(HIGHGUID_PET);
    if (!Create(guid, map, owner->GetPhaseMask(), petentry, pet_number))
    {
        sLog->outDebug(LOG_FILTER_PETS, "Pet::LoadPetFromRedis error Create");
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

    SetDisplayId(PetData["modelid"].asUInt());
    SetNativeDisplayId(PetData["modelid"].asUInt());
    uint32 petlevel = PetData["level"].asUInt();
    SetUInt32Value(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_NONE);
    SetName(PetData["name"].asString());

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

            SetByteFlag(UNIT_FIELD_BYTES_2, 2, PetData["renamed"].asBool() ? UNIT_CAN_BE_ABANDONED : UNIT_CAN_BE_RENAMED | UNIT_CAN_BE_ABANDONED);
            SetUInt32Value(UNIT_FIELD_FLAGS, UNIT_FLAG_PVP_ATTACKABLE);
                                                            // this enables popup window (pet abandon, cancel)
            SetSpecializationId(PetData["specialization"].asUInt());
            break;
        }
        default:
            if (!IsPetGhoul())
                sLog->outError(LOG_FILTER_PETS, "Pet have incorrect type (%u) for pet loading.", getPetType());
            break;
    }

    SetUInt32Value(UNIT_FIELD_PET_NAME_TIMESTAMP, uint32(time(NULL))); // cast can't be helped here
    SetCreatorGUID(owner->GetGUID());

    SetReactState(ReactStates(PetData["Reactstate"].asUInt()));
    if (GetReactState() == REACT_AGGRESSIVE)
        SetReactState(REACT_DEFENSIVE);

    InitStatsForLevel(petlevel);
    SetUInt32Value(UNIT_FIELD_PETEXPERIENCE, PetData["exp"].asUInt());

    SynchronizeLevelWithOwner();
    SetCanModifyStats(true);

    if (!stampeded)
    {
        uint32 savedhealth = PetData["curhealth"].asUInt();
        uint32 savedmana = PetData["curmana"].asUInt();

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

    sLog->outDebug(LOG_FILTER_PETS, "New Pet has guid %u", GetGUIDLow());

    if (owner->GetGroup())
        owner->SetGroupUpdateFlag(GROUP_UPDATE_PET);

    if (getPetType() == HUNTER_PET)
    {
        if (!PetData["declinedname"].empty())
        {
            delete m_declinedname;
            m_declinedname = new DeclinedName;
            for (uint8 i = 0; i < MAX_DECLINED_NAME_CASES; ++i)
            {
                std::string index = std::to_string(i);
                m_declinedname->name[i] = PetData["declinedname"][index.c_str()].asString();
            }
        }
    }

    //set last used pet number (for use in BG's)
    if (owner->GetTypeId() == TYPEID_PLAYER && isControlled() && !isTemporarySummoned() && getPetType() == HUNTER_PET && !m_Stampeded)
        owner->ToPlayer()->SetLastPetNumber(pet_number);

    std::string petId = std::to_string(pet_number);
    RedisDatabase.AsyncExecuteH("HGET", sObjectMgr->GetPetKey(), petId.c_str(), GetGUID(), [&](const RedisValue &v, uint64 guid) {
        if (Pet* pet = ObjectAccessor::GetObjectInWorld(guid, (Pet*)NULL))
            pet->LoadPetSpellRedis(&v);
    });
    return true;
}

void Pet::LoadPetSpellRedis(const RedisValue* v)
{
    bool _load = true;
    if (!sRedisBuilder->LoadFromRedis(v, PetDataSpell))
    {
        sLog->outInfo(LOG_FILTER_REDIS, "Pet::LoadPetSpellRedis data is empty");
        _load = false;
    }

    Player* owner = (Player*)m_owner;
    if (!owner)
        return;

    uint32 timediff = uint32(time(NULL) - PetData["savetime"].asUInt());
    if (_load)
        _LoadAurasRedis(timediff);

    if (owner->GetTypeId() == TYPEID_PLAYER && owner->ToPlayer()->InArena())
        RemoveArenaAuras();

    // load action bar, if data broken will fill later by default spells.
    m_charmInfo->LoadPetActionBar(PetData["abdata"].asString());

    if (_load)
    {
        _LoadSpellsRedis();
        _LoadSpellCooldownsRedis();
    }
    if(GetSpecializationId())
        LearnSpecializationSpell();

    LearnPetPassives();

    if(owner->HasSpell(108415)) // active talent Soul Link
        CastSpell(this, 108446, true);

    if(!m_Stampeded)
    {
        CleanupActionBar();                                     // remove unknown spells from action bar after load
        owner->PetSpellInitialize();
    }

    owner->SendTalentsInfoData(true);

    CastPetAuras(true);

    if (!_load)
        SavePetToDB();

    m_loading = false;
}

void Pet::_LoadSpellCooldownsRedis()
{
    m_CreatureSpellCooldowns.clear();
    m_CreatureCategoryCooldowns.clear();

    if (PetDataSpell["spellcooldowns"].empty())
        return;

    ObjectGuid guid = GetGUID();
    time_t curTime = time(NULL);
    uint32 count = 0;

    //! 5.4.1
    WorldPacket data(SMSG_SPELL_COOLDOWN, size_t(8 + 1 + PetDataSpell["spellcooldowns"].size() * 8));

    data.WriteGuidMask<4, 7, 6>(guid);
    size_t count_pos = data.bitwpos();
    data.WriteBits(0, 21);
    data.WriteGuidMask<2, 3, 1, 0>(guid);
    data.WriteBit(1);
    data.WriteGuidMask<5>(guid);

    data.WriteGuidBytes<7, 2, 1, 6, 5, 4, 3, 0>(guid);

    for (auto iter = PetDataSpell["spellcooldowns"].begin(); iter != PetDataSpell["spellcooldowns"].end(); ++iter)
    {
        auto cooldownValue = *iter;
        uint32 spell_id = atoi(iter.memberName());
        time_t db_time = time_t(cooldownValue.asInt64());

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

    data.FlushBits();
    data.PutBits(count_pos, count, 21);

    if (!m_CreatureSpellCooldowns.empty() && GetOwner())
        ((Player*)GetOwner())->GetSession()->SendPacket(&data);
}

void Pet::_LoadSpellsRedis()
{
    if (PetDataSpell["spell"].empty())
        return;

    for (auto itr = PetDataSpell["spell"].begin(); itr != PetDataSpell["spell"].end(); ++itr)
    {
        uint32 spellId = atoi(itr.memberName());
        auto value = (*itr);
        uint8 state = value.asInt();

        addSpell(spellId, ActiveStates(state), PETSPELL_UNCHANGED);
    }
}

void Pet::_LoadAurasRedis(uint32 time_diff)
{
    if (PetDataSpell["auras"].empty())
        return;

    std::list<auraEffectData> auraEffectList;
    for (auto itr = PetDataSpell["effect"].begin(); itr != PetDataSpell["effect"].end(); ++itr)
    {
        auto auraEffect = (*itr);
        uint8 slot = atoi(itr.memberName());
        uint8 effect = auraEffect["effect"].asInt();
        uint32 baseamount = auraEffect["baseamount"].asInt();
        uint32 amount = auraEffect["amount"].asInt();

        auraEffectList.push_back(auraEffectData(slot, effect, amount, baseamount));
    }

    for (auto itr = PetDataSpell["auras"].begin(); itr != PetDataSpell["auras"].end(); ++itr)
    {
        auto auraValue = (*itr);
        uint8 slot = atoi(itr.memberName());

        int32 damage[32];
        int32 baseDamage[32];
        uint64 caster_guid = auraValue["caster_guid"].asUInt64();
        uint32 spellid = auraValue["spell"].asInt();
        uint32 effmask = auraValue["effect_mask"].asInt();
        uint32 recalculatemask = auraValue["recalculate_mask"].asInt();
        uint8 stackcount = auraValue["stackcount"].asInt();
        int32 maxduration = auraValue["maxduration"].asInt();
        int32 remaintime = auraValue["remaintime"].asInt();
        uint8 remaincharges = auraValue["remaincharges"].asInt();

        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellid);
        if (!spellInfo)
        {
            sLog->outError(LOG_FILTER_PLAYER, "Unknown aura (spellid %u), ignore.", spellid);
            continue;
        }

        // negative effects should continue counting down after logout
        if (remaintime != -1 && !spellInfo->IsPositive())
        {
            if (remaintime / IN_MILLISECONDS <= int32(time_diff))
                continue;

            remaintime -= time_diff*IN_MILLISECONDS;
        }

        // prevent wrong values of remaincharges
        if (spellInfo->ProcCharges)
        {
            // we have no control over the order of applying auras and modifiers allow auras
            // to have more charges than value in SpellInfo
            if (remaincharges <= 0/* || remaincharges > spellproto->procCharges*/)
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
            sLog->outInfo(LOG_FILTER_PLAYER, "Added aura spellid %u, effectmask %u", spellInfo->Id, effmask);
        }
    }
}

void Pet::_SaveSpellCooldownsRedis()
{
    time_t curTime = time(NULL);

    // remove oudated and save active
    for (CreatureSpellCooldowns::iterator itr = m_CreatureSpellCooldowns.begin(); itr != m_CreatureSpellCooldowns.end();)
    {
        if (itr->second <= curTime)
            m_CreatureSpellCooldowns.erase(itr++);
        else
        {
            std::string index = std::to_string(itr->first);
            PetDataSpell["spellcooldowns"][index.c_str()] = uint32(itr->second);

            ++itr;
        }
    }
}

void Pet::_SaveSpellsRedis()
{
    PetDataSpell["spell"].clear();
    for (PetSpellMap::iterator itr = m_spells.begin(), next = m_spells.begin(); itr != m_spells.end(); itr = next)
    {
        ++next;

        // prevent saving family passives to DB
        if (itr->second.type == PETSPELL_FAMILY)
            continue;
        if (itr->second.state == PETSPELL_REMOVED)
        {
            m_spells.erase(itr);
            continue;
        }

        std::string index = std::to_string(itr->first);
        PetDataSpell["spell"][index.c_str()] = itr->second.active;

        itr->second.state = PETSPELL_UNCHANGED;
    }
}

void Pet::_SaveAurasRedis()
{
    PetDataSpell["auras"].clear();
    PetDataSpell["effect"].clear();

    for (AuraMap::const_iterator itr = m_ownedAuras.begin(); itr != m_ownedAuras.end(); ++itr)
    {
        if (!itr->second->CanBeSaved() || IsPetAura(itr->second))
            continue;

        Aura* aura = itr->second;
        AuraApplication * foundAura = GetAuraApplication(aura->GetId(), aura->GetCasterGUID(), aura->GetCastItemGUID());

        if(!foundAura)
            continue;

        std::string slot = std::to_string(foundAura->GetSlot());
        uint32 effMask = 0;
        uint32 recalculateMask = 0;

        for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
        {
            if (AuraEffect const* effect = aura->GetEffect(i))
            {
                PetDataSpell["effect"][slot.c_str()]["effect"] = i;
                PetDataSpell["effect"][slot.c_str()]["baseamount"] = effect->GetBaseAmount();
                PetDataSpell["effect"][slot.c_str()]["amount"] = effect->GetAmount();
                effMask |= 1 << i;
                if (effect->CanBeRecalculated())
                    recalculateMask |= 1 << i;
            }
        }

        // don't save guid of caster in case we are caster of the spell - guid for pet is generated every pet load, so it won't match saved guid anyways
        uint64 casterGUID = (itr->second->GetCasterGUID() == GetGUID()) ? 0 : itr->second->GetCasterGUID();

        PetDataSpell["auras"][slot.c_str()]["caster_guid"] = casterGUID;
        PetDataSpell["auras"][slot.c_str()]["item_guid"] = itr->second->GetCastItemGUID();
        PetDataSpell["auras"][slot.c_str()]["spell"] = itr->second->GetId();
        PetDataSpell["auras"][slot.c_str()]["effect_mask"] = effMask;
        PetDataSpell["auras"][slot.c_str()]["recalculate_mask"] = recalculateMask;
        PetDataSpell["auras"][slot.c_str()]["stackcount"] = itr->second->GetStackAmount();
        PetDataSpell["auras"][slot.c_str()]["maxduration"] = itr->second->GetMaxDuration();
        PetDataSpell["auras"][slot.c_str()]["remaintime"] = itr->second->GetDuration();
        PetDataSpell["auras"][slot.c_str()]["remaincharges"] = itr->second->GetCharges();
    }
}
