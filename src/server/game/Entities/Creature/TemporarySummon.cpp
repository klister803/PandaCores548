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

#include "Log.h"
#include "ObjectAccessor.h"
#include "CreatureAI.h"
#include "ObjectMgr.h"
#include "TemporarySummon.h"

TempSummon::TempSummon(SummonPropertiesEntry const* properties, Unit* owner, bool isWorldObject) :
Creature(isWorldObject), m_Properties(properties), m_type(TEMPSUMMON_MANUAL_DESPAWN),
m_timer(0), m_lifetime(0), onUnload(false)
{
    m_Stampeded = false;
    m_summonerGUID = owner ? owner->GetGUID() : 0;
    m_unitTypeMask |= UNIT_MASK_SUMMON;
}

Unit* TempSummon::GetSummoner() const
{
    return m_summonerGUID ? ObjectAccessor::GetUnit(*this, m_summonerGUID) : NULL;
}

void TempSummon::Update(uint32 diff)
{
    Creature::Update(diff);

    if (m_deathState == DEAD)
    {
        UnSummon();
        return;
    }
    switch (m_type)
    {
        case TEMPSUMMON_MANUAL_DESPAWN:
            break;
        case TEMPSUMMON_TIMED_DESPAWN:
        {
            if (m_timer <= diff)
            {
                UnSummon();
                return;
            }

            m_timer -= diff;
            break;
        }
        case TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT:
        {
            if (!isInCombat())
            {
                if (m_timer <= diff)
                {
                    UnSummon();
                    return;
                }

                m_timer -= diff;
            }
            else if (m_timer != m_lifetime)
                m_timer = m_lifetime;

            break;
        }

        case TEMPSUMMON_CORPSE_TIMED_DESPAWN:
        {
            if (m_deathState == CORPSE)
            {
                if (m_timer <= diff)
                {
                    UnSummon();
                    return;
                }

                m_timer -= diff;
            }
            break;
        }
        case TEMPSUMMON_CORPSE_DESPAWN:
        {
            // if m_deathState is DEAD, CORPSE was skipped
            if (m_deathState == CORPSE || m_deathState == DEAD)
            {
                UnSummon();
                return;
            }

            break;
        }
        case TEMPSUMMON_DEAD_DESPAWN:
        {
            if (m_deathState == DEAD)
            {
                UnSummon();
                return;
            }
            break;
        }
        case TEMPSUMMON_TIMED_OR_CORPSE_DESPAWN:
        {
            // if m_deathState is DEAD, CORPSE was skipped
            if (m_deathState == CORPSE || m_deathState == DEAD)
            {
                UnSummon();
                return;
            }

            if (!isInCombat())
            {
                if (m_timer <= diff)
                {
                    UnSummon();
                    return;
                }
                else
                    m_timer -= diff;
            }
            else if (m_timer != m_lifetime)
                m_timer = m_lifetime;
            break;
        }
        case TEMPSUMMON_TIMED_OR_DEAD_DESPAWN:
        {
            // if m_deathState is DEAD, CORPSE was skipped
            if (m_deathState == DEAD)
            {
                UnSummon();
                return;
            }

            if (!isInCombat() && isAlive())
            {
                if (m_timer <= diff)
                {
                    UnSummon();
                    return;
                }
                else
                    m_timer -= diff;
            }
            else if (m_timer != m_lifetime)
                m_timer = m_lifetime;
            break;
        }
        default:
            UnSummon();
            sLog->outError(LOG_FILTER_UNITS, "Temporary summoned creature (entry: %u) have unknown type %u of ", GetEntry(), m_type);
            break;
    }
}

void TempSummon::InitStats(uint32 duration)
{
    ASSERT(!isPet());

    m_timer = duration;
    m_lifetime = duration;
    uint32 spellid = GetUInt32Value(UNIT_CREATED_BY_SPELL);

    if (m_type == TEMPSUMMON_MANUAL_DESPAWN)
        m_type = (duration == 0) ? TEMPSUMMON_DEAD_DESPAWN : TEMPSUMMON_TIMED_DESPAWN;

    Unit* owner = GetSummoner();

    if (owner && isTrigger() && m_spells[0])
    {
        setFaction(owner->getFaction());
        SetLevel(owner->getLevel());
        if (owner->GetTypeId() == TYPEID_PLAYER)
            m_ControlledByPlayer = true;
    }

    if (!m_Properties)
        return;

    if (owner)
    {
        int32 slot = m_Properties->Slot;
        if(m_Properties->Type == 17) //hack for spirit copy
            slot = 17;

        if (slot > MAX_SUMMON_SLOT)
            slot = 0;

        switch(GetEntry())
        {
            case 59271:     //Warlock purge gateway
                slot = MAX_SUMMON_SLOT - 1;
                break;
            case 59262:
                slot = MAX_SUMMON_SLOT - 2;
                break;
            default:
                break;
        }

        if (slot)
        {
            if(slot > 0)
            {
                if (owner->m_SummonSlot[slot] && owner->m_SummonSlot[slot] != GetGUID())
                {
                    Creature* oldSummon = GetMap()->GetCreature(owner->m_SummonSlot[slot]);
                    if (oldSummon && oldSummon->isSummon())
                        oldSummon->ToTempSummon()->UnSummon();
                }
                owner->m_SummonSlot[slot] = GetGUID();
            }
            else if(slot < 0)
            {
                int32 count = 1;
                if (SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellid))
                {
                    for (uint32 j = 0; j < MAX_SPELL_EFFECTS; j++)
                    {
                        if (spellInfo->Effects[j].Effect == SPELL_EFFECT_SUMMON && spellInfo->Effects[j].BasePoints > count)
                        {
                            count = spellInfo->Effects[j].BasePoints;
                            break;
                        }
                    }
                }
                if(count > 1)
                {
                    //Auto get free slot or slot for replace summon
                    int32 saveSlot = SUMMON_SLOT_TOTEM;
                    for (int32 i = SUMMON_SLOT_TOTEM; i < SUMMON_SLOT_TOTEM + count; ++i)
                    {
                        if(!owner->m_SummonSlot[i])
                        {
                            saveSlot = i;
                            break;
                        }
                        else if (owner->m_SummonSlot[i] < owner->m_SummonSlot[i + 1] && (i + 1 < SUMMON_SLOT_TOTEM + count))
                        {
                            if(owner->m_SummonSlot[i] <= owner->m_SummonSlot[saveSlot])
                                saveSlot = i;
                        }
                        else if (owner->m_SummonSlot[i] > owner->m_SummonSlot[i + 1] && (i + 1 < SUMMON_SLOT_TOTEM + count))
                        {
                            if(owner->m_SummonSlot[i + 1] <= owner->m_SummonSlot[saveSlot])
                                saveSlot = i + 1;
                        }
                    }
                    slot = saveSlot;
                    if (owner->m_SummonSlot[slot] && owner->m_SummonSlot[slot] != GetGUID())
                    {
                        Creature* oldSummon = GetMap()->GetCreature(owner->m_SummonSlot[slot]);
                        if (oldSummon && oldSummon->isSummon())
                            oldSummon->ToTempSummon()->UnSummon();
                    }
                    owner->m_SummonSlot[slot] = GetGUID();
                }
                else
                {
                    slot = SUMMON_SLOT_TOTEM;
                    if (owner->m_SummonSlot[slot] && owner->m_SummonSlot[slot] != GetGUID())
                    {
                        Creature* oldSummon = GetMap()->GetCreature(owner->m_SummonSlot[slot]);
                        if (oldSummon && oldSummon->isSummon())
                            oldSummon->ToTempSummon()->UnSummon();
                    }
                    owner->m_SummonSlot[slot] = GetGUID();
                }
            }

            if(slot >= SUMMON_SLOT_TOTEM && slot < MAX_TOTEM_SLOT)
            {
                if(Player* player = owner->ToPlayer())
                {
                    ObjectGuid guid = GetGUID();
                    //! 5.4.1
                    WorldPacket data(SMSG_TOTEM_CREATED, 1 + 8 + 4 + 4);
                    data << uint32(duration);
                    data << uint32(spellid);
                    data << uint8(slot - 1);
                    data.WriteGuidMask<6, 0, 5, 2, 1, 3, 7, 4>(guid);
                    data.WriteGuidBytes<0, 2, 1, 3, 5, 4, 6, 7>(guid);
                    player->SendDirectMessage(&data);
                }
            }
        }
    }

    if (m_Properties->Faction)
        setFaction(m_Properties->Faction);
    else if (IsVehicle() && owner) // properties should be vehicle
        setFaction(owner->getFaction());
}

void TempSummon::InitSummon()
{
    Unit* owner = GetSummoner();
    if (owner)
    {
        if (owner->GetTypeId() == TYPEID_UNIT && owner->ToCreature()->IsAIEnabled)
            owner->ToCreature()->AI()->JustSummoned(this);
        if (IsAIEnabled)
            AI()->IsSummonedBy(owner);
    }
}

bool TempSummon::InitBaseStat(uint32 creatureId, bool& damageSet)
{
    CreatureTemplate const* cinfo = GetCreatureTemplate();
    CreatureBaseStats const* stats = sObjectMgr->GetCreatureBaseStats(getLevel(), cinfo->unit_class);
    Unit* owner = GetAnyOwner();

    PetStats const* pStats = sObjectMgr->GetPetStats(creatureId);
    if (pStats)                                      // exist in DB
    {
        if(pStats->hp && owner)
        {
            SetCreateHealth(int32(owner->GetMaxHealth() * pStats->hp));
            SetMaxHealth(GetCreateHealth());
            SetHealth(GetCreateHealth());
        }
        else
        {
            SetCreateHealth(stats->BaseHealth[cinfo->expansion]);
            SetMaxHealth(int32(owner->GetMaxHealth() * pStats->hp));
            SetHealth(GetCreateHealth());
        }

        if (getPowerType() != pStats->energy_type)
            setPowerType(Powers(pStats->energy_type));

        if(pStats->energy_type)
        {
            SetMaxPower(Powers(pStats->energy_type), pStats->energy);
            SetPower(Powers(pStats->energy_type), pStats->energy);
        }
        else if(!pStats->energy_type && pStats->energy == 1)
        {
            SetMaxPower(Powers(pStats->energy_type), 0);
            SetPower(Powers(pStats->energy_type), 0);
        }
        else
        {
            if(pStats->energy && owner)
            {
                int32 manaMax = int32(owner->GetMaxPower(Powers(pStats->energy_type)) * float(pStats->energy / 100.0f));
                if(!pStats->energy_type)
                    SetCreateMana(manaMax);
                SetMaxPower(Powers(pStats->energy_type), manaMax);
                SetPower(Powers(pStats->energy_type), GetMaxPower(Powers(pStats->energy_type)));
            }
            else
            {
                if(!pStats->energy_type)
                    SetCreateMana(stats->BaseMana);
                SetPower(Powers(pStats->energy_type), GetMaxPower(Powers(pStats->energy_type)));
            }
        }
        if(pStats->damage && owner)
        {
            damageSet = true;
            SetBaseWeaponDamage(BASE_ATTACK, MINDAMAGE, float(owner->GetFloatValue(UNIT_FIELD_MINDAMAGE) * pStats->damage));
            SetBaseWeaponDamage(BASE_ATTACK, MAXDAMAGE, float(owner->GetFloatValue(UNIT_FIELD_MINDAMAGE) * pStats->damage));
        }
        if(pStats->type)
            SetCasterPet(true);

        return true;
    }
    else
        return false;
}

void TempSummon::SetTempSummonType(TempSummonType type)
{
    m_type = type;
}

void TempSummon::UnSummon(uint32 msTime)
{
    if (msTime)
    {
        m_Events.KillAllEvents(false);
        ForcedUnsummonDelayEvent* pEvent = new ForcedUnsummonDelayEvent(*this);

        m_Events.AddEvent(pEvent, m_Events.CalculateTime(msTime));
        return;
    }

    if (onUnload)
        return;
    onUnload = true;

    CastPetAuras(false);
    //ASSERT(!isPet());
    if (isPet())
    {
        if (((Pet*)this)->getPetType() == HUNTER_PET)
            ((Pet*)this)->Remove(PET_SLOT_ACTUAL_PET_SLOT, false);
        else
            ((Pet*)this)->Remove(PET_SLOT_OTHER_PET);
        ASSERT(!IsInWorld());
        return;
    }

    Unit* owner = GetSummoner();
    if (isMinion() && owner && owner->GetTypeId() == TYPEID_PLAYER)
    {
        if (sBattlePetSpeciesBySpellId.find(GetEntry()) != sBattlePetSpeciesBySpellId.end())
        {
            owner->SetUInt64Value(PLAYER_FIELD_SUMMONED_BATTLE_PET_GUID, 0);
            owner->SetUInt32Value(PLAYER_CURRENT_BATTLE_PET_BREED_QUALITY, 0);
        }
    }
    if (owner && owner->GetTypeId() == TYPEID_UNIT && owner->ToCreature()->IsAIEnabled)
        owner->ToCreature()->AI()->SummonedCreatureDespawn(this);

    AddObjectToRemoveList();
}

bool ForcedUnsummonDelayEvent::Execute(uint64 /*e_time*/, uint32 /*p_time*/)
{
    if (&m_owner)
        m_owner.UnSummon();
    return true;
}

void TempSummon::RemoveFromWorld()
{
    if (!IsInWorld())
        return;

    if (m_Properties)
        if (uint32 slot = m_Properties->Slot)
        {
            if (slot > MAX_SUMMON_SLOT)
                slot = 0;

            if (Unit* owner = GetSummoner())
                if (owner->m_SummonSlot[slot] == GetGUID())
                    owner->m_SummonSlot[slot] = 0;
        }

    //if (GetOwnerGUID())
    //    sLog->outError(LOG_FILTER_UNITS, "Unit %u has owner guid when removed from world", GetEntry());

    Creature::RemoveFromWorld();
}

Minion::Minion(SummonPropertiesEntry const* properties, Unit* owner, bool isWorldObject) : TempSummon(properties, owner, isWorldObject)
, m_owner(owner)
{
    ASSERT(m_owner);
    m_unitTypeMask |= UNIT_MASK_MINION;
    m_followAngle = frand(0, 2*M_PI);
}

void Minion::InitStats(uint32 duration)
{
    TempSummon::InitStats(duration);

    SetReactState(REACT_PASSIVE);

    SetCreatorGUID(m_owner->GetGUID());
    setFaction(m_owner->getFaction());

    m_owner->SetMinion(this, true, PET_SLOT_UNK_SLOT);
}

void Minion::RemoveFromWorld()
{
    if (!IsInWorld() || !m_owner || !this)
        return;

    m_owner->SetMinion(this, false, PET_SLOT_UNK_SLOT);
    TempSummon::RemoveFromWorld();
}

bool Minion::IsGuardianPet() const
{
    return isPet() || (m_Properties && m_Properties->Category == SUMMON_CATEGORY_PET);
}

bool Minion::IsWarlockPet() const
{
    if(isPet())
    {
        if(m_owner && m_owner->getClass() == CLASS_WARLOCK)
            return true;
    }

    return false;
}

Guardian::Guardian(SummonPropertiesEntry const* properties, Unit* owner, bool isWorldObject) : Minion(properties, owner, isWorldObject)
, m_bonusSpellDamage(0)
{
    bool controlable = true;
    memset(m_statFromOwner, 0, sizeof(float)*MAX_STATS);
    m_unitTypeMask |= UNIT_MASK_GUARDIAN;

    if (properties)
    {
        switch (properties->Id)
        {
            case 3459:
            case 3097:
                controlable = false;
                break;
        }

        if ((properties->Type == SUMMON_TYPE_PET || properties->Category == SUMMON_CATEGORY_PET) && controlable)
        {
            m_unitTypeMask |= UNIT_MASK_CONTROLABLE_GUARDIAN;
            InitCharmInfo();
        }
    }
}

void Guardian::InitStats(uint32 duration)
{
    Minion::InitStats(duration);

    InitStatsForLevel(m_owner->getLevel());

    if (m_owner->GetTypeId() == TYPEID_PLAYER && HasUnitTypeMask(UNIT_MASK_CONTROLABLE_GUARDIAN))
        m_charmInfo->InitCharmCreateSpells();

    SetReactState(REACT_AGGRESSIVE);
}

void Guardian::InitSummon()
{
    TempSummon::InitSummon();

    if (m_owner->GetTypeId() == TYPEID_PLAYER
        && m_owner->GetMinionGUID() == GetGUID()
        && !m_owner->GetCharmGUID())
        m_owner->ToPlayer()->CharmSpellInitialize();
}

Puppet::Puppet(SummonPropertiesEntry const* properties, Unit* owner) : Minion(properties, owner, false) //maybe true?
{
    ASSERT(owner->GetTypeId() == TYPEID_PLAYER);
    m_owner = (Player*)owner;
    m_unitTypeMask |= UNIT_MASK_PUPPET;
}

void Puppet::InitStats(uint32 duration)
{
    Minion::InitStats(duration);
    SetLevel(m_owner->getLevel());
    SetReactState(REACT_PASSIVE);
}

void Puppet::InitSummon()
{
    Minion::InitSummon();
    SetCharmedBy(m_owner, CHARM_TYPE_POSSESS);
    //if (!SetCharmedBy(m_owner, CHARM_TYPE_POSSESS))
        //ASSERT(false);
}

void Puppet::Update(uint32 time)
{
    Minion::Update(time);
    //check if caster is channelling?
    if (IsInWorld())
    {
        if (!isAlive())
        {
            UnSummon();
            // TODO: why long distance .die does not remove it
        }
    }
}

void Puppet::RemoveFromWorld()
{
    if (!IsInWorld())
        return;

    RemoveCharmedBy(NULL);
    Minion::RemoveFromWorld();
}
