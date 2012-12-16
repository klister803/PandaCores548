#ifndef CLASS_FACTORY_H
# define CLASS_FACTORY_H

# include "Creature.h"
# include "Player.h"
# include "Pet.h"
# include "../Entities/Totem/Totem.h"
# include "SpellAuras.h"
# include "Guild.h"
# include "Map.h"
# include "MapInstanced.h"
# include "ScriptMgr.h"

class ClassFactory
{
public:
    static CreaturePtr ConstructCreature(bool isWorldObject = false)
    {
        CreaturePtr creature(new Creature(isWorldObject));
        if (!creature)
            return nullptr;
        creature->InitializeUnit();
        return creature;
    }

    static PlayerPtr ConstructPlayer(WorldSession* session)
    {
        PlayerPtr player(new Player(session));
        if (!player)
            return nullptr;
        player->InitializeUnit();
        return player;
    }

    static PetPtr ConstructPet(PlayerPtr player, PetType type = MAX_PET_TYPE)
    {
        PetPtr pet(new Pet(player, type));
        if (!pet)
            return nullptr;
        pet->InitializeUnit();
        return pet;
    }

    static TempSummonPtr ConstructTempSummon(SummonPropertiesEntry const* properties, UnitPtr owner, bool isWorldObject)
    {
        TempSummonPtr temp(new TempSummon(properties, owner, isWorldObject));
        if (!temp)
            return nullptr;
        temp->InitializeUnit();
        return temp;
    }

    static GuardianPtr ConstructGuardian(SummonPropertiesEntry const* properties, UnitPtr owner, bool isWorldObject)
    {
        GuardianPtr guardian(new Guardian(properties, owner, isWorldObject));
        if (!guardian)
            return nullptr;
        guardian->InitializeUnit();
        return guardian;
    }

    static PuppetPtr ConstructPuppet(SummonPropertiesEntry const* properties, UnitPtr owner)
    {
        PuppetPtr puppet(new Puppet(properties, owner));
        if (!puppet)
            return nullptr;
        puppet->InitializeUnit();
        return puppet;
    }

    static TotemPtr ConstructTotem(SummonPropertiesEntry const* properties, UnitPtr owner)
    {
        TotemPtr totem(new Totem(properties, owner));
        if (!totem)
            return nullptr;
        totem->InitializeUnit();
        return totem;
    }

    static MinionPtr ConstructMinion(SummonPropertiesEntry const* properties, UnitPtr owner, bool isWorldObject)
    {
        MinionPtr minion(new Minion(properties, owner, isWorldObject));
        if (!minion)
            return nullptr;
        minion->InitializeUnit();
        return minion;
    }

    static AuraApplicationPtr ConstructAuraApplication(UnitPtr target, UnitPtr caster, AuraPtr aura, uint32 effMask)
    {
        AuraApplicationPtr aura_app(new AuraApplication(target, caster, aura, effMask));
        if (!aura_app)
            return nullptr;

        ASSERT(aura_app->GetTarget() && aura_app->GetBase());

        if (aura_app->GetBase()->CanBeSentToClient())
        {
            // Try find slot for aura
            uint8 slot = MAX_AURAS;
            // Lookup for auras already applied from spell
            if (AuraApplicationPtr foundAura = aura_app->GetTarget()->GetAuraApplication(aura_app->GetBase()->GetId(), aura_app->GetBase()->GetCasterGUID(), aura_app->GetBase()->GetCastItemGUID()))
            {
                // allow use single slot only by auras from same caster
                slot = foundAura->GetSlot();
            }
            else
            {
                Unit::VisibleAuraMap const* visibleAuras = aura_app->GetTarget()->GetVisibleAuras();
                // lookup for free slots in units visibleAuras
                Unit::VisibleAuraMap::const_iterator itr = visibleAuras->find(0);
                for (uint32 freeSlot = 0; freeSlot < MAX_AURAS; ++itr, ++freeSlot)
                {
                    if (itr == visibleAuras->end() || itr->first != freeSlot)
                    {
                        slot = freeSlot;
                        break;
                    }
                }
            }

            // Register Visible Aura
            if (slot < MAX_AURAS)
            {
                aura_app->SetSlot(slot);
                aura_app->GetTarget()->SetVisibleAura(slot, aura_app);
                aura_app->SetNeedClientUpdate();
                sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "Aura: %u Effect: %d put to unit visible auras slot: %u", aura_app->GetBase()->GetId(), aura_app->GetEffectMask(), slot);
            }
            else
                sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "Aura: %u Effect: %d could not find empty unit visible slot", aura_app->GetBase()->GetId(), aura_app->GetEffectMask());
        }

        aura_app->_InitFlags(caster, effMask);
        return aura_app;
    }

    static AuraPtr ConstructUnitAura(SpellInfo const* spellproto, uint32 effMask, WorldObjectPtr owner, UnitPtr caster, SpellPowerEntry const* spellPowerData, int32 *baseAmount, ItemPtr castItem, uint64 casterGUID)
    {
        AuraPtr aura = AuraPtr(new UnitAura(spellproto, effMask, owner, caster, spellPowerData, baseAmount, castItem, casterGUID));
        if (!aura)
            return nullptr;

        aura->GetUnitOwner()->_AddAura(TO_UNITAURA(aura), caster);
        aura->LoadScripts();
        aura->_InitEffects(effMask, caster, baseAmount);

        return aura;
    }

    static AuraPtr ConstructDynAura(SpellInfo const* spellproto, uint32 effMask, WorldObjectPtr owner, UnitPtr caster, SpellPowerEntry const* spellPowerData, int32 *baseAmount, ItemPtr castItem, uint64 casterGUID)
    {
        AuraPtr aura = AuraPtr(new DynObjAura(spellproto, effMask, owner, caster, spellPowerData, baseAmount, castItem, casterGUID));
        if (!aura)
            return nullptr;

        aura->GetDynobjOwner()->SetAura(aura);
        aura->_InitEffects(effMask, caster, baseAmount);

        aura->LoadScripts();
        ASSERT(aura->GetDynobjOwner());
        ASSERT(aura->GetDynobjOwner()->IsInWorld());
        ASSERT(aura->GetDynobjOwner()->GetMap() == aura->GetCaster()->GetMap());

        return aura;
    }

    static GuildPtr ConstructGuild()
    {
        GuildPtr guild(new Guild());
        if (!guild)
            return nullptr;
        guild->InitializeGuild();
        return guild;
    }

    static MapPtr ConstructMapInstanced(uint32 id, time_t expiry)
    {
        MapPtr map(new MapInstanced(id, expiry));
        if (!map)
            return nullptr;

        map->m_parentMap = map;
        for (unsigned int idx=0; idx < MAX_NUMBER_OF_GRIDS; ++idx)
        {
            for (unsigned int j=0; j < MAX_NUMBER_OF_GRIDS; ++j)
            {
                //z code
                map->GridMaps[idx][j] = nullptr;
                map->setNGrid(nullptr, idx, j);
            }
        }

        //lets initialize visibility distance for map
        map->Map::InitVisibilityDistance();

        sScriptMgr->OnCreateMap(map);

        return map;
    }

    static MapPtr ConstructMap(uint32 id, time_t t, uint32 InstanceId, uint8 SpawnMode, MapPtr _parent = nullptr)
    {
        MapPtr map(new Map(id, t, InstanceId, SpawnMode, _parent));
        if (!map)
            return nullptr;

        map->m_parentMap = (_parent ? _parent : map);
        for (unsigned int idx=0; idx < MAX_NUMBER_OF_GRIDS; ++idx)
        {
            for (unsigned int j=0; j < MAX_NUMBER_OF_GRIDS; ++j)
            {
                //z code
                map->GridMaps[idx][j] = nullptr;
                map->setNGrid(nullptr, idx, j);
            }
        }

        //lets initialize visibility distance for map
        map->Map::InitVisibilityDistance();

        sScriptMgr->OnCreateMap(map);

        return map;
    }

    static InstanceMapPtr ConstructInstanceMap(uint32 id, time_t t, uint32 InstanceId, uint8 SpawnMode, MapPtr _parent)
    {
        InstanceMapPtr map(new InstanceMap(id, t, InstanceId, SpawnMode, _parent));
        if (!map)
            return nullptr;

        map->m_parentMap = (_parent ? _parent : map);
        for (unsigned int idx=0; idx < MAX_NUMBER_OF_GRIDS; ++idx)
        {
            for (unsigned int j=0; j < MAX_NUMBER_OF_GRIDS; ++j)
            {
                //z code
                map->GridMaps[idx][j] = nullptr;
                map->setNGrid(nullptr, idx, j);
            }
        }
        
        //lets initialize visibility distance for dungeons
        map->InstanceMap::InitVisibilityDistance();
        
        // the timer is started by default, and stopped when the first player joins
        // this make sure it gets unloaded if for some reason no player joins
        map->m_unloadTimer = std::max(sWorld->getIntConfig(CONFIG_INSTANCE_UNLOAD_DELAY), (uint32)MIN_UNLOAD_DELAY);

        sScriptMgr->OnCreateMap(map);

        return map;
    }

    static BattlegroundMapPtr ConstructBattlegroundMap(uint32 id, time_t t, uint32 InstanceId, MapPtr _parent, uint8 spawnMode)
    {
        BattlegroundMapPtr map(new BattlegroundMap(id, t, InstanceId, _parent, spawnMode));
        if (!map)
            return nullptr;

        map->m_parentMap = (_parent ? _parent : map);
        for (unsigned int idx=0; idx < MAX_NUMBER_OF_GRIDS; ++idx)
        {
            for (unsigned int j=0; j < MAX_NUMBER_OF_GRIDS; ++j)
            {
                //z code
                map->GridMaps[idx][j] = nullptr;
                map->setNGrid(nullptr, idx, j);
            }
        }
        
        //lets initialize visibility distance for dungeons
        map->BattlegroundMap::InitVisibilityDistance();

        sScriptMgr->OnCreateMap(map);

        return map;
    }

    template<class T>
    static std::shared_ptr<T> ConstructClass()
    {
        static_assert(true, "ConstructClass");
        std::shared_ptr<T> t(new T());
        return t;
    }

    /*template<>
    static std::shared_ptr<Creature> ConstructClass()
    {
        return ConstructCreature();
    }*/
};

#endif /* !CLASS_FACTORY_H */