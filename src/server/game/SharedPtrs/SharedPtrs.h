#ifndef SHARED_PTRS_H
# define SHARED_PTRS_H

class Aura;
class AuraApplication;
class UnitAura;
class AuraEffect;
class Object;
class WorldObject;
class Unit;
class TempSummon;
class Player;
class DynamicObject;
class Pet;
class GameObject;
class Group;
class Map;

# define CAST(x,y) std::dynamic_pointer_cast<x>(y)
# define STATIC_CAST(x,y) std::static_pointer_cast<x>(y)
# define CONST_CAST(x, y) std::const_pointer_cast<const x>(y)
# define NO_CONST(x,y) std::const_pointer_cast<x>(y)

# define GroupPtr std::shared_ptr<Group>
# define constGroupPtr std::shared_ptr<const Group>
# define MapPtr std::shared_ptr<Map>
# define constMapPtr std::shared_ptr<const Map>
# define BattlegroundMapPtr std::shared_ptr<BattlegroundMap>
# define constBattlegroundMapPtr std::shared_ptr<const BattlegroundMap>
# define InstanceMapPtr std::shared_ptr<InstanceMap>
# define constInstanceMapPtr std::shared_ptr<const InstanceMap>
# define MapInstancedPtr std::shared_ptr<MapInstanced>
# define constMapInstancedPtr std::shared_ptr<const MapInstanced>
# define LootPtr std::shared_ptr<Loot>
# define constLootPtr std::shared_ptr<const Loot>
# define GuildPtr std::shared_ptr<Guild>
# define constGuildPtr std::shared_ptr<const Guild>
# define RollPtr std::shared_ptr<Roll>
# define constRollPtr std::shared_ptr<const Roll>
# define LootValidatorRefPtr std::shared_ptr<LootValidatorRef>
# define constLootValidatorRefPtr std::shared_ptr<const LootValidatorRef>
# define HostileReferencePtr std::shared_ptr<HostileReference>
# define constHostileReferencePtr std::shared_ptr<const HostileReference>
# define FollowerReferencePtr std::shared_ptr<FollowerReference>
# define GroupReferencePtr std::shared_ptr<GroupReference>
# define constGroupReferencePtr std::shared_ptr<const GroupReference>
# define WeatherPtr std::shared_ptr<Weather>
# define constWeatherPtr std::shared_ptr<const Weather>

# define TO_GROUP(y) CAST(Group,y)
# define TO_CONST_GROUP(y) CAST(const Group,y)
# define TO_MAP(y) CAST(Map,y)
# define TO_CONST_MAP(y) CAST(const Map,y)
# define TO_INSTANCEMAP(y) CAST(InstanceMap,y)
# define TO_BATTLEGROUNDMAP(y) CAST(BattlegroundMap,y)
# define TO_CONST_INSTANCEMAP(y) CAST(const InstanceMap,y)
# define TO_CONST_BATTLEGROUNDMAP(y) CAST(const BattlegroundMap,y)
# define TO_MAPINSTANCED(y) CAST(MapInstanced,y)
# define TO_CONST_MAPINSTANCED(y) CAST(const MapInstanced,y)
# define TO_LOOT(y) CAST(Loot,y)
# define TO_CONST_LOOT(y) CAST(const Loot,y)
# define TO_GUILD(y) CAST(Guild,y)
# define TO_CONST_GUILD(y) CAST(const Guild,y)
# define TO_LOOTVALIDATORREF(y) CAST(LootValidatorRef,y)
# define TO_HOSTILEREFERENCE(y) CAST(HostileReference,y)
# define TO_WEATHER(y) CAST(Weather,y)

# define THIS_GROUP TO_GROUP(((std::enable_shared_from_this<Group>*)this)->shared_from_this())
# define THIS_MAP TO_MAP(((std::enable_shared_from_this<Map>*)this)->shared_from_this())
# define THIS_CONST_MAP TO_CONST_MAP(((std::enable_shared_from_this<Map>*)this)->shared_from_this())
# define THIS_INSTANCEMAP TO_INSTANCEMAP(((std::enable_shared_from_this<InstanceMap>*)this)->shared_from_this())
# define THIS_BATTLEGROUNDMAP TO_BATTLEGROUNDMAP(((std::enable_shared_from_this<BattlegroundMap>*)this)->shared_from_this())
# define THIS_CONST_INSTANCEMAP TO_CONST_INSTANCEMAP(((std::enable_shared_from_this<InstanceMap>*)this)->shared_from_this())
# define THIS_CONST_BATTLEGROUNDMAP TO_CONST_BATTLEGROUNDMAP(((std::enable_shared_from_this<BattlegroundMap>*)this)->shared_from_this())
# define THIS_MAPINSTANCED TO_MAPINSTANCED(((std::enable_shared_from_this<MapInstanced>*)this)->shared_from_this())
# define THIS_CONST_MAPINSTANCED TO_CONST_MAPINSTANCED(((std::enable_shared_from_this<MapInstanced>*)this)->shared_from_this())
# define THIS_LOOT TO_LOOT(((std::enable_shared_from_this<Loot>*)this)->shared_from_this())
# define THIS_CONST_LOOT TO_CONST_LOOT(((std::enable_shared_from_this<Loot>*)this)->shared_from_this())
# define THIS_GUILD TO_GUILD(((std::enable_shared_from_this<Guild>*)this)->shared_from_this())
# define THIS_CONST_GUILD TO_CONST_GUILD(((std::enable_shared_from_this<Guild>*)this)->shared_from_this())
# define THIS_LOOTVALIDATORREF TO_LOOTVALIDATORREF(((std::enable_shared_from_this<LootValidatorRef>*)this)->shared_from_this())
# define THIS_HOSTILEREFERENCE TO_HOSTILEREFERENCE(((std::enable_shared_from_this<HostileReference>*)this)->shared_from_this())
# define THIS_WEATHER TO_WEATHER(((std::enable_shared_from_this<Weather>*)this)->shared_from_this())

/*
** Objects, WorldObjects, Units, etc
*/
# define ObjectPtr std::shared_ptr<Object>
# define constObjectPtr std::shared_ptr<const Object>

# define WorldObjectPtr std::shared_ptr<WorldObject>
# define constWorldObjectPtr std::shared_ptr<const WorldObject>

# define UnitPtr std::shared_ptr<Unit>
# define constUnitPtr std::shared_ptr<const Unit>

# define CreaturePtr std::shared_ptr<Creature>
# define constCreaturePtr std::shared_ptr<const Creature>

# define TempSummonPtr std::shared_ptr<TempSummon>
# define constTempSummonPtr std::shared_ptr<const TempSummon>

# define PlayerPtr std::shared_ptr<Player>
# define constPlayerPtr std::shared_ptr<const Player>

# define DynamicObjectPtr std::shared_ptr<DynamicObject>
# define constDynamicObjectPtr std::shared_ptr<const DynamicObject>

# define PetPtr std::shared_ptr<Pet>
# define constPetPtr std::shared_ptr<const Pet>

# define GameObjectPtr std::shared_ptr<GameObject>
# define constGameObjectPtr std::shared_ptr<const GameObject>

# define GuardianPtr std::shared_ptr<Guardian>
# define constGuardianPtr std::shared_ptr<const Guardian>

# define MinionPtr std::shared_ptr<Minion>
# define constMinionPtr std::shared_ptr<const Minion>

# define PuppetPtr std::shared_ptr<Puppet>
# define constPuppetPtr std::shared_ptr<const Puppet>

# define CorpsePtr std::shared_ptr<Corpse>
# define constCorpsePtr std::shared_ptr<const Corpse>

# define TotemPtr std::shared_ptr<Totem>
# define constTotemPtr std::shared_ptr<const Totem>

# define ItemPtr std::shared_ptr<Item>
# define constItemPtr std::shared_ptr<const Item>

# define BagPtr std::shared_ptr<Bag>
# define constBagPtr std::shared_ptr<const Bag>

# define TransportPtr std::shared_ptr<Transport>
# define constTransportPtr std::shared_ptr<const Transport>

# define VehiclePtr std::shared_ptr<Vehicle>
# define constVehiclePtr std::shared_ptr<const Vehicle>

# define TO_WORLDOBJECT(y) CAST(WorldObject,y)
# define TO_CONST_WORLDOBJECT(y) CAST(const WorldObject,y)
# define TO_UNIT(y) CAST(Unit,y)
# define TO_CONST_UNIT(y) CAST(const Unit,y)
# define TO_CREATURE(y) CAST(Creature,y)
# define TO_CONST_CREATURE(y) CAST(const Creature,y)
# define TO_TEMPSUMMON(y) CAST(TempSummon,y)
# define TO_CONST_TEMPSUMMON(y) CAST(const TempSummon,y)
# define TO_PLAYER(y) CAST(Player,y)
# define TO_CONST_PLAYER(y) CAST(const Player,y)
# define TO_DYNAMICOBJECT(y) CAST(DynamicObject,y)
# define TO_CONST_DYNAMICOBJECT(y) CAST(const DynamicObject,y)
# define TO_PET(y) CAST(Pet,y)
# define TO_CONST_PET(y) CAST(const Pet,y)
# define TO_GAMEOBJECT(y) CAST(GameObject,y)
# define TO_CONST_GAMEOBJECT(y) CAST(const GameObject,y)
# define TO_GUARDIAN(y) CAST(Guardian,y)
# define TO_CONST_GUARDIAN(y) CAST(const Guardian,y)
# define TO_MINION(y) CAST(Minion,y)
# define TO_CONST_MINION(y) CAST(const Minion,y)
# define TO_PUPPET(y) CAST(Puppet,y)
# define TO_CONST_PUPPET(y) CAST(const Puppet,y)
# define TO_CORPSE(y) CAST(Corpse,y)
# define TO_CONST_CORPSE(y) CAST(const Corpse,y)
# define TO_TOTEM(y) CAST(Totem,y)
# define TO_CONST_TOTEM(y) CAST(const Totem,y)
# define TO_ITEM(y) CAST(Item,y)
# define TO_CONST_ITEM(y) CAST(const Item,y)
# define TO_BAG(y) CAST(Bag,y)
# define TO_CONST_BAG(y) CAST(const Bag,y)
# define TO_TRANSPORT(y) CAST(Transport,y)
# define TO_CONST_TRANSPORT(y) CAST(const Transport,y)
# define TO_VEHICLE(y) CAST(Vehicle,y)
# define TO_CONST_VEHICLE(y) CAST(Vehicle,y)

# define THIS_CREATURE TO_CREATURE(((std::enable_shared_from_this<Creature>*)this)->shared_from_this())
# define THIS_PLAYER TO_PLAYER(((std::enable_shared_from_this<Player>*)this)->shared_from_this())
# define THIS_WORLDOBJECT TO_WORLDOBJECT(((std::enable_shared_from_this<WorldObject>*)this)->shared_from_this())
# define THIS_UNIT TO_UNIT(((std::enable_shared_from_this<Unit>*)this)->shared_from_this())
# define THIS_GAMEOBJECT TO_GAMEOBJECT(((std::enable_shared_from_this<GameObject>*)this)->shared_from_this())
# define THIS_PET TO_PET(((std::enable_shared_from_this<Pet>*)this)->shared_from_this())
# define THIS_DYNAMICOBJECT TO_DYNAMICOBJECT(((std::enable_shared_from_this<DynamicObject>*)this)->shared_from_this())
# define THIS_TOTEM TO_TOTEM(((std::enable_shared_from_this<Totem>*)this)->shared_from_this())
# define THIS_TEMPSUMMON TO_TEMPSUMMON(((std::enable_shared_from_this<TempSummon>*)this)->shared_from_this())
# define THIS_CORPSE TO_CORPSE(((std::enable_shared_from_this<Corpse>*)this)->shared_from_this())
# define THIS_ITEM TO_ITEM(((std::enable_shared_from_this<Item>*)this)->shared_from_this())
# define THIS_MINION TO_MINION(((std::enable_shared_from_this<Minion>*)this)->shared_from_this())
# define THIS_BAG TO_BAG(((std::enable_shared_from_this<Bag>*)this)->shared_from_this())
# define THIS_TRANSPORT TO_TRANSPORT(((std::enable_shared_from_this<Transport>*)this)->shared_from_this())
# define THIS_VEHICLE TO_VEHICLE(((std::enable_shared_from_this<Vehicle>*)this)->shared_from_this())

# define THIS_CONST_WORLDOBJECT TO_CONST_WORLDOBJECT(((std::enable_shared_from_this<WorldObject>*)this)->shared_from_this())
# define THIS_CONST_PLAYER TO_CONST_PLAYER(((std::enable_shared_from_this<Player>*)this)->shared_from_this())
# define THIS_CONST_GAMEOBJECT TO_CONST_GAMEOBJECT(((std::enable_shared_from_this<GameObject>*)this)->shared_from_this())
# define THIS_CONST_CORPSE TO_CONST_CORPSE(((std::enable_shared_from_this<Corpse>*)this)->shared_from_this())
# define THIS_CONST_CREATURE TO_CONST_CREATURE(((std::enable_shared_from_this<Creature>*)this)->shared_from_this())
# define THIS_CONST_ITEM TO_CONST_ITEM(((std::enable_shared_from_this<Item>*)this)->shared_from_this())
# define THIS_CONST_DYNAMICOBJECT TO_CONST_DYNAMICOBJECT(((std::enable_shared_from_this<DynamicObject>*)this)->shared_from_this())
# define THIS_CONST_UNIT TO_CONST_UNIT(((std::enable_shared_from_this<Unit>*)this)->shared_from_this())
# define THIS_CONST_BAG TO_CONST_BAG(((std::enable_shared_from_this<Bag>*)this)->shared_from_this())
# define THIS_CONST_TRANSPORT TO_CONST_TRANSPORT(((std::enable_shared_from_this<Transport>*)this)->shared_from_this())
# define THIS_CONST_PET TO_CONST_PET(((std::enable_shared_from_this<Pet>*)this)->shared_from_this())
# define THIS_CONST_TOTEM TO_CONST_TOTEM(((std::enable_shared_from_this<const Totem>*)this)->shared_from_this())
# define THIS_CONST_TEMPSUMMON TO_CONST_TEMPSUMMON(((std::enable_shared_from_this<TempSummon>*)this)->shared_from_this())
/*
** End of Objects, WorldObjects, Units, etc
*/

/*
** Spells and Auras shared_ptr
*/
# define AuraPtr std::shared_ptr<Aura>
# define constAuraPtr std::shared_ptr<const Aura>
# define UnitAuraPtr std::shared_ptr<UnitAura>
# define AuraEffectPtr std::shared_ptr<AuraEffect>
# define constAuraEffectPtr std::shared_ptr<const AuraEffect>
# define AuraApplicationPtr std::shared_ptr<AuraApplication>
# define constAuraApplicationPtr std::shared_ptr<const AuraApplication>

# define TO_UNITAURA(y) CAST(UnitAura,y)
/*
** End of spells and auras shared_ptr
*/

#endif /* !SHARED_PTRS_H */