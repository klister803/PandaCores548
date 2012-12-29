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
# include "Object.h"
# include "PointMovementGenerator.h"
# include "TargetedMovementGenerator.h"
# include "ConfusedMovementGenerator.h"
# include "HomeMovementGenerator.h"
# include "RandomMovementGenerator.h"
# include "FleeingMovementGenerator.h"
# include "IdleMovementGenerator.h"

class ClassFactory
{
public:
    static CreaturePtr ConstructCreature(bool isWorldObject = false)
    {
        CreaturePtr creature(new Creature(isWorldObject));
        if (!creature)
            return nullptr;
        
        creature->i_motionMaster = MotionMaster(creature);
        creature->m_ThreatManager = ThreatManagerPtr(new ThreatManager(creature));
        creature->m_HostileRefManager.setOwner(creature);
        creature->_creature = CAST(GridObject<Creature>,creature);
        
        ObjectPtr unit = creature->SharedFromObject();
        return creature;
    }

    static PlayerPtr ConstructPlayer(WorldSession* session)
    {
        PlayerPtr player(new Player(session));
        if (!player)
            return nullptr;
        player->i_motionMaster = MotionMaster(player);
        player->m_ThreatManager = ThreatManagerPtr(new ThreatManager(player));
        player->m_HostileRefManager = HostileRefManager(player);
        player->m_achievementMgr = AchievementMgr<Player>(player);
        player->m_reputationMgr = ReputationMgr(player);
        player->m_mover = player;
        player->m_movedPlayer = player;
        player->m_seer = player;
        player->m_mapRef = MapReferencePtr(new MapReference());
        return player;
    }

    static PetPtr ConstructPet(PlayerPtr player, PetType type = MAX_PET_TYPE)
    {
        PetPtr pet(new Pet(player, type));
        if (!pet)
            return nullptr;
        pet->i_motionMaster = MotionMaster(pet);
        pet->m_ThreatManager = ThreatManagerPtr(new ThreatManager(pet));
        pet->m_HostileRefManager = HostileRefManager(pet);
        pet->_creature = CAST(GridObject<Creature>,pet);
        
        if (!(pet->m_unitTypeMask & UNIT_MASK_CONTROLABLE_GUARDIAN))
        {
            pet->m_unitTypeMask |= UNIT_MASK_CONTROLABLE_GUARDIAN;
            pet->InitCharmInfo();
        }

        pet->m_name = "Pet";
        pet->m_regenTimer = PET_FOCUS_REGEN_INTERVAL;
        return pet;
    }

    static TempSummonPtr ConstructTempSummon(SummonPropertiesEntry const* properties, UnitPtr owner, bool isWorldObject)
    {
        TempSummonPtr temp(new TempSummon(properties, owner, isWorldObject));
        if (!temp)
            return nullptr;
        temp->i_motionMaster = MotionMaster(temp);
        temp->m_ThreatManager = ThreatManagerPtr(new ThreatManager(temp));
        temp->m_HostileRefManager = HostileRefManager(temp);
        temp->_creature = CAST(GridObject<Creature>,temp);
        return temp;
    }

    static GuardianPtr ConstructGuardian(SummonPropertiesEntry const* properties, UnitPtr owner, bool isWorldObject)
    {
        GuardianPtr guardian(new Guardian(properties, owner, isWorldObject));
        if (!guardian)
            return nullptr;
        guardian->i_motionMaster = MotionMaster(guardian);
        guardian->m_ThreatManager = ThreatManagerPtr(new ThreatManager(guardian));
        guardian->m_HostileRefManager = HostileRefManager(guardian);
        guardian->_creature = CAST(GridObject<Creature>,guardian);
        return guardian;
    }

    static PuppetPtr ConstructPuppet(SummonPropertiesEntry const* properties, UnitPtr owner)
    {
        PuppetPtr puppet(new Puppet(properties, owner));
        if (!puppet)
            return nullptr;
        puppet->i_motionMaster = MotionMaster(puppet);
        puppet->m_ThreatManager = ThreatManagerPtr(new ThreatManager(puppet));
        puppet->m_HostileRefManager = HostileRefManager(puppet);
        puppet->_creature = CAST(GridObject<Creature>,puppet);
        return puppet;
    }

    static TotemPtr ConstructTotem(SummonPropertiesEntry const* properties, UnitPtr owner)
    {
        TotemPtr totem(new Totem(properties, owner));
        if (!totem)
            return nullptr;
        totem->i_motionMaster = MotionMaster(totem);
        totem->m_ThreatManager = ThreatManagerPtr(new ThreatManager(totem));
        totem->m_HostileRefManager = HostileRefManager(totem);
        totem->_creature = CAST(GridObject<Creature>,totem);
        return totem;
    }

    static MinionPtr ConstructMinion(SummonPropertiesEntry const* properties, UnitPtr owner, bool isWorldObject)
    {
        MinionPtr minion(new Minion(properties, owner, isWorldObject));
        if (!minion)
            return nullptr;
        minion->i_motionMaster = MotionMaster(minion);
        minion->m_ThreatManager = ThreatManagerPtr(new ThreatManager(minion));
        minion->m_HostileRefManager = HostileRefManager(minion);
        minion->_creature = CAST(GridObject<Creature>,minion);
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
        guild->m_achievementMgr = AchievementMgr<Guild>(guild);
        guild->_newsLog = Guild::GuildNewsLog(guild);
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

    static HostileReferencePtr ConstructHostileReference(UnitPtr refUnit, ThreatManagerPtr threatManager, float threat)
    {
        HostileReferencePtr ref(new HostileReference(refUnit, threatManager, threat));
        if (!ref)
            return nullptr;

        ref->iThreat = threat;
        ref->iTempThreatModifier = 0.0f;
        ref->link(refUnit, threatManager); //Dirty FIXME
        ref->iUnitGuid = refUnit->GetGUID();
        ref->iOnline = true;
        ref->iAccessible = true;

        return ref;
    }

    template<class T>
    static std::shared_ptr<FollowMovementGenerator<T>> ConstructFollowMovementGenerator(UnitPtr &target, float offset, float angle)
    {
        std::shared_ptr<FollowMovementGenerator<T>> gen(new FollowMovementGenerator<T>(target, offset, angle));
        if (!gen)
            return nullptr;
        gen->i_target->link(target, gen);
        return gen;
    }

    template<class T>
    static std::shared_ptr<FollowMovementGenerator<T>> ConstructFollowMovementGenerator(UnitPtr &target)
    {
        std::shared_ptr<FollowMovementGenerator<T>> gen(new FollowMovementGenerator<T>(target));
        if (!gen)
            return nullptr;
        gen->i_target.link(target, gen);
        return gen;
    }

    template<class T>
    static std::shared_ptr<PointMovementGenerator<T>> ConstructPointMovementGenerator(uint32 _id, float _x, float _y, float _z, float _speed = 0.0f)
    {
        std::shared_ptr<PointMovementGenerator<T>> gen(new PointMovementGenerator<T>(_id, _x, _y, _z, _speed));
        if (!gen)
            return nullptr;
        return gen;
    }

    template<class T>
    static std::shared_ptr<ChaseMovementGenerator<T>> ConstructChaseMovementGenerator(UnitPtr &target, float offset, float angle)
    {
        std::shared_ptr<ChaseMovementGenerator<T>> gen(new ChaseMovementGenerator<T>(target, offset, angle));
        if (!gen)
            return nullptr;
        gen->i_target->link(target, gen);
        return gen;
    }

    template<class T>
    static std::shared_ptr<ConfusedMovementGenerator<T>> ConstructConfusedMovementGenerator()
    {
        std::shared_ptr<ConfusedMovementGenerator<T>> gen(new ConfusedMovementGenerator<T>());
        if (!gen)
            return nullptr;
        return gen;
    }

    template<class T>
    static std::shared_ptr<HomeMovementGenerator<T>> ConstructHomeMovementGenerator()
    {
        //static_assert(false, "ClassFactory can't build HomeMovementGenerator for T <> Creature");
        return nullptr;
    }

    template<class T>
    static std::shared_ptr<RandomMovementGenerator<T>> ConstructRandomMovementGenerator(float spawn_dist = 0.0f)
    {
        std::shared_ptr<RandomMovementGenerator<T>> gen (new RandomMovementGenerator<T>());
        if (!gen)
            return nullptr;
        return gen;
    }

    static std::shared_ptr<EffectMovementGenerator> ConstructEffectMovementGenerator(uint32 Id)
    {
        std::shared_ptr<EffectMovementGenerator> gen(new EffectMovementGenerator(Id));
        if (!gen)
            return nullptr;
        return gen;
    }

    static std::shared_ptr<AssistanceMovementGenerator> ConstructAssistanceMovementGenerator(float _x, float _y, float _z)
    {
        std::shared_ptr<AssistanceMovementGenerator> gen ( new AssistanceMovementGenerator(_x, _y, _z));
        if (!gen)
            return nullptr;
        return gen;
    }

    static std::shared_ptr<AssistanceDistractMovementGenerator> ConstructAssistanceDistractMovementGenerator(uint32 timer)
    {
        std::shared_ptr<AssistanceDistractMovementGenerator> gen (new AssistanceDistractMovementGenerator(timer));
        if (!gen)
            return nullptr;
        return gen;
    }

    template<class T>
    static std::shared_ptr<FleeingMovementGenerator<T>> ConstructFleeingMovementGenerator(uint64 fright)
    {
        std::shared_ptr<FleeingMovementGenerator<T>> gen(new FleeingMovementGenerator<T>(fright));
        if (!gen)
            return nullptr;
        return gen;
    }

    static std::shared_ptr<TimedFleeingMovementGenerator> ConstructTimedFleeingMovementGenerator(uint64 fright, uint32 time)
    {
        std::shared_ptr<TimedFleeingMovementGenerator> gen(new TimedFleeingMovementGenerator(fright, time));
        if (!gen)
            return nullptr;
        return gen;
    }

    template<class T>
    static std::shared_ptr<T> ConstructClass()
    {
        //static_assert(false, "ConstructClass");
        std::shared_ptr<T> t(new T());
        return t;
    }
};

template<>
inline std::shared_ptr<HomeMovementGenerator<Creature>> ClassFactory::ConstructHomeMovementGenerator()
{
    std::shared_ptr<HomeMovementGenerator<Creature>> gen (new HomeMovementGenerator<Creature>());
    if (!gen)
        return nullptr;
    return gen;
}

template<>
inline std::shared_ptr<GameObject> ClassFactory::ConstructClass()
{
    std::shared_ptr<GameObject> t(new GameObject());
    return t;
}

template<>
inline std::shared_ptr<Creature> ClassFactory::ConstructClass()
{
    return ConstructCreature();
}

#endif /* !CLASS_FACTORY_H */
