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

#ifndef SC_SCRIPTMGR_H
#define SC_SCRIPTMGR_H

#include "Common.h"
#include <ace/Singleton.h>
#include <ace/Atomic_Op.h>

#include "DBCStores.h"
#include "Player.h"
#include "SharedDefines.h"
#include "World.h"
#include "Weather.h"

class AuctionHouseObject;
class AuraScript;
class Battleground;
class BattlegroundMap;
class Channel;
class ChatCommand;
class Creature;
class CreatureAI;
class DynamicObject;
class GameObject;
class GameObjectAI;
class Guild;
class GridMap;
class Group;
class InstanceMap;
class InstanceScript;
class Item;
class Map;
class OutdoorPvP;
class Player;
class Quest;
class ScriptMgr;
class Spell;
class SpellScript;
class SpellCastTargets;
class Transport;
class Unit;
class Vehicle;
class WorldPacket;
class WorldSocket;
class WorldObject;

struct AchievementCriteriaData;
struct AuctionEntry;
struct ConditionSourceInfo;
struct Condition;
struct ItemTemplate;
struct OutdoorPvPData;

#define VISIBLE_RANGE       166.0f                          //MAX visible range (size of grid)

// Generic scripting text function.
void DoScriptText(int32 textEntry, WorldObjectPtr pSource, UnitPtr target = NULL);

/*
    TODO: Add more script type classes.

    MailScript
    SessionScript
    CollisionScript
    ArenaTeamScript

*/

/*
    Standard procedure when adding new script type classes:

    First of all, define the actual class, and have it inherit from ScriptObject, like so:

    class MyScriptType : public ScriptObject
    {
        uint32 _someId;

        private:

            void RegisterSelf();

        protected:

            MyScriptType(const char* name, uint32 someId)
                : ScriptObject(name), _someId(someId)
            {
                ScriptRegistry<MyScriptType>::AddScript(this);
            }

        public:

            // If a virtual function in your script type class is not necessarily
            // required to be overridden, just declare it virtual with an empty
            // body. If, on the other hand, it's logical only to override it (i.e.
            // if it's the only method in the class), make it pure virtual, by adding
            // = 0 to it.
            virtual void OnSomeEvent(uint32 someArg1, std::string& someArg2) { }

            // This is a pure virtual function:
            virtual void OnAnotherEvent(uint32 someArg) = 0;
    }

    Next, you need to add a specialization for ScriptRegistry. Put this in the bottom of
    ScriptMgr.cpp:

    template class ScriptRegistry<MyScriptType>;

    Now, add a cleanup routine in ScriptMgr::~ScriptMgr:

    SCR_CLEAR(MyScriptType);

    Now your script type is good to go with the script system. What you need to do now
    is add functions to ScriptMgr that can be called from the core to actually trigger
    certain events. For example, in ScriptMgr.h:

    void OnSomeEvent(uint32 someArg1, std::string& someArg2);
    void OnAnotherEvent(uint32 someArg);

    In ScriptMgr.cpp:

    void ScriptMgr::OnSomeEvent(uint32 someArg1, std::string& someArg2)
    {
        FOREACH_SCRIPT(MyScriptType)->OnSomeEvent(someArg1, someArg2);
    }

    void ScriptMgr::OnAnotherEvent(uint32 someArg)
    {
        FOREACH_SCRIPT(MyScriptType)->OnAnotherEvent(someArg1, someArg2);
    }

    Now you simply call these two functions from anywhere in the core to trigger the
    event on all registered scripts of that type.
*/

class ScriptObject
{
    friend class ScriptMgr;

    public:

        // Do not override this in scripts; it should be overridden by the various script type classes. It indicates
        // whether or not this script type must be assigned in the database.
        virtual bool IsDatabaseBound() const { return false; }

        const std::string& GetName() const { return _name; }

    protected:

        ScriptObject(const char* name)
            : _name(name)
        {
        }

        virtual ~ScriptObject()
        {
        }

    private:

        const std::string _name;
};

template<class TObject> class UpdatableScript
{
    protected:

        UpdatableScript()
        {
        }

    public:

        virtual void OnUpdate(std::shared_ptr<TObject> /*obj*/, uint32 /*diff*/) { }
};

class SpellScriptLoader : public ScriptObject
{
    protected:

        SpellScriptLoader(const char* name);

    public:

        bool IsDatabaseBound() const { return true; }

        // Should return a fully valid SpellScript pointer.
        virtual SpellScript* GetSpellScript() const { return NULL; }

        // Should return a fully valid AuraScript pointer.
        virtual AuraScript* GetAuraScript() const { return NULL; }
};

class ServerScript : public ScriptObject
{
    protected:

        ServerScript(const char* name);

    public:

        // Called when reactive socket I/O is started (WorldSocketMgr).
        virtual void OnNetworkStart() { }

        // Called when reactive I/O is stopped.
        virtual void OnNetworkStop() { }

        // Called when a remote socket establishes a connection to the server. Do not store the socket object.
        virtual void OnSocketOpen(WorldSocket* /*socket*/) { }

        // Called when a socket is closed. Do not store the socket object, and do not rely on the connection
        // being open; it is not.
        virtual void OnSocketClose(WorldSocket* /*socket*/, bool /*wasNew*/) { }

        // Called when a packet is sent to a client. The packet object is a copy of the original packet, so reading
        // and modifying it is safe.
        virtual void OnPacketSend(WorldSocket* /*socket*/, WorldPacket& /*packet*/) { }

        // Called when a (valid) packet is received by a client. The packet object is a copy of the original packet, so
        // reading and modifying it is safe.
        virtual void OnPacketReceive(WorldSocket* /*socket*/, WorldPacket& /*packet*/) { }

        // Called when an invalid (unknown opcode) packet is received by a client. The packet is a reference to the orignal
        // packet; not a copy. This allows you to actually handle unknown packets (for whatever purpose).
        virtual void OnUnknownPacketReceive(WorldSocket* /*socket*/, WorldPacket& /*packet*/) { }
};

class WorldScript : public ScriptObject
{
    protected:

        WorldScript(const char* name);

    public:

        // Called when the open/closed state of the world changes.
        virtual void OnOpenStateChange(bool /*open*/) { }

        // Called after the world configuration is (re)loaded.
        virtual void OnConfigLoad(bool /*reload*/) { }

        // Called before the message of the day is changed.
        virtual void OnMotdChange(std::string& /*newMotd*/) { }

        // Called when a world shutdown is initiated.
        virtual void OnShutdownInitiate(ShutdownExitCode /*code*/, ShutdownMask /*mask*/) { }

        // Called when a world shutdown is cancelled.
        virtual void OnShutdownCancel() { }

        // Called on every world tick (don't execute too heavy code here).
        virtual void OnUpdate(uint32 /*diff*/) { }

        // Called when the world is started.
        virtual void OnStartup() { }

        // Called when the world is actually shut down.
        virtual void OnShutdown() { }
};

class FormulaScript : public ScriptObject
{
    protected:

        FormulaScript(const char* name);

    public:

        // Called after calculating honor.
        virtual void OnHonorCalculation(float& /*honor*/, uint8 /*level*/, float /*multiplier*/) { }

        // Called after gray level calculation.
        virtual void OnGrayLevelCalculation(uint8& /*grayLevel*/, uint8 /*playerLevel*/) { }

        // Called after calculating experience color.
        virtual void OnColorCodeCalculation(XPColorChar& /*color*/, uint8 /*playerLevel*/, uint8 /*mobLevel*/) { }

        // Called after calculating zero difference.
        virtual void OnZeroDifferenceCalculation(uint8& /*diff*/, uint8 /*playerLevel*/) { }

        // Called after calculating base experience gain.
        virtual void OnBaseGainCalculation(uint32& /*gain*/, uint8 /*playerLevel*/, uint8 /*mobLevel*/, ContentLevels /*content*/) { }

        // Called after calculating experience gain.
        virtual void OnGainCalculation(uint32& /*gain*/, PlayerPtr /*Player*/, UnitPtr /*Unit*/) { }

        // Called when calculating the experience rate for group experience.
        virtual void OnGroupRateCalculation(float& /*rate*/, uint32 /*count*/, bool /*isRaid*/) { }
};

template<class TMap> class MapScript : public UpdatableScript<TMap>
{
    MapEntry const* _mapEntry;

    protected:

        MapScript(uint32 mapId)
            : _mapEntry(sMapStore.LookupEntry(mapId))
        {
            if (!_mapEntry)
                sLog->outError(LOG_FILTER_TSCR, "Invalid MapScript for %u; no such map ID.", mapId);
        }

    public:

        // Gets the MapEntry structure associated with this script. Can return NULL.
        MapEntry const* GetEntry() { return _mapEntry; }

        // Called when the map is created.
        virtual void OnCreate(constMapPtr /*Map*/) { }

        // Called just before the map is destroyed.
        virtual void OnDestroy(constMapPtr /*Map*/) { }

        // Called when a grid map is loaded.
        virtual void OnLoadGridMap(constMapPtr /*Map*/, GridMap* /*gMap*/, uint32 /*gx*/, uint32 /*gy*/) { }

        // Called when a grid map is unloaded.
        virtual void OnUnloadGridMap(constMapPtr /*Map*/, GridMap* /*gMap*/, uint32 /*gx*/, uint32 /*gy*/)  { }

        // Called when a player enters the map.
        virtual void OnPlayerEnter(constMapPtr /*Map*/, PlayerPtr /*Player*/) { }

        // Called when a player leaves the map.
        virtual void OnPlayerLeave(constMapPtr /*Map*/, PlayerPtr /*Player*/) { }

        // Called on every map update tick.
        virtual void OnUpdate(constMapPtr /*Map*/, uint32 /*diff*/) { }
};

class WorldMapScript : public ScriptObject, public MapScript<Map>
{
    protected:

        WorldMapScript(const char* name, uint32 mapId);
};

class InstanceMapScript : public ScriptObject, public MapScript<InstanceMap>
{
    protected:

        InstanceMapScript(const char* name, uint32 mapId);

    public:

        bool IsDatabaseBound() const { return true; }

        // Gets an InstanceScript object for this instance.
        virtual InstanceScript* GetInstanceScript(InstanceMapPtr /*Map*/) const { return NULL; }
};

class BattlegroundMapScript : public ScriptObject, public MapScript<BattlegroundMap>
{
    protected:

        BattlegroundMapScript(const char* name, uint32 mapId);
};

class ItemScript : public ScriptObject
{
    protected:

        ItemScript(const char* name);

    public:

        bool IsDatabaseBound() const { return true; }

        // Called when a dummy spell effect is triggered on the item.
        virtual bool OnDummyEffect(UnitPtr /*caster*/, uint32 /*spellId*/, SpellEffIndex /*effIndex*/, ItemPtr /*target*/) { return false; }

        // Called when a player accepts a quest from the item.
        virtual bool OnQuestAccept(PlayerPtr /*Player*/, ItemPtr /*Item*/, Quest const* /*quest*/) { return false; }

        // Called when a player uses the item.
        virtual bool OnUse(PlayerPtr /*Player*/, ItemPtr /*Item*/, SpellCastTargets const& /*targets*/) { return false; }

        // Called when the item expires (is destroyed).
        virtual bool OnExpire(PlayerPtr /*Player*/, ItemTemplate const* /*proto*/) { return false; }
};

class CreatureScript : public ScriptObject, public UpdatableScript<Creature>
{
    protected:

        CreatureScript(const char* name);

    public:

        bool IsDatabaseBound() const { return true; }

        // Called when a dummy spell effect is triggered on the creature.
        virtual bool OnDummyEffect(UnitPtr /*caster*/, uint32 /*spellId*/, SpellEffIndex /*effIndex*/, CreaturePtr /*target*/) { return false; }

        // Called when a player opens a gossip dialog with the creature.
        virtual bool OnGossipHello(PlayerPtr /*Player*/, CreaturePtr /*Creature*/) { return false; }

        // Called when a player selects a gossip item in the creature's gossip menu.
        virtual bool OnGossipSelect(PlayerPtr /*Player*/, CreaturePtr /*Creature*/, uint32 /*sender*/, uint32 /*action*/) { return false; }

        // Called when a player selects a gossip with a code in the creature's gossip menu.
        virtual bool OnGossipSelectCode(PlayerPtr /*Player*/, CreaturePtr /*Creature*/, uint32 /*sender*/, uint32 /*action*/, const char* /*code*/) { return false; }

        // Called when a player accepts a quest from the creature.
        virtual bool OnQuestAccept(PlayerPtr /*Player*/, CreaturePtr /*Creature*/, Quest const* /*quest*/) { return false; }

        // Called when a player selects a quest in the creature's quest menu.
        virtual bool OnQuestSelect(PlayerPtr /*Player*/, CreaturePtr /*Creature*/, Quest const* /*quest*/) { return false; }

        // Called when a player completes a quest with the creature.
        virtual bool OnQuestComplete(PlayerPtr /*Player*/, CreaturePtr /*Creature*/, Quest const* /*quest*/) { return false; }

        // Called when a player selects a quest reward.
        virtual bool OnQuestReward(PlayerPtr /*Player*/, CreaturePtr /*Creature*/, Quest const* /*quest*/, uint32 /*opt*/) { return false; }

        // Called when the dialog status between a player and the creature is requested.
        virtual uint32 GetDialogStatus(PlayerPtr /*Player*/, CreaturePtr /*Creature*/) { return 100; }

        // Called when a CreatureAI object is needed for the creature.
        virtual CreatureAI* GetAI(CreaturePtr /*Creature*/) const { return NULL; }
};

class GameObjectScript : public ScriptObject, public UpdatableScript<GameObject>
{
    protected:

        GameObjectScript(const char* name);

    public:

        bool IsDatabaseBound() const { return true; }

        // Called when a dummy spell effect is triggered on the gameobject.
        virtual bool OnDummyEffect(UnitPtr /*caster*/, uint32 /*spellId*/, SpellEffIndex /*effIndex*/, GameObjectPtr /*target*/) { return false; }

        // Called when a player opens a gossip dialog with the gameobject.
        virtual bool OnGossipHello(PlayerPtr /*Player*/, GameObjectPtr /*go*/) { return false; }

        // Called when a player selects a gossip item in the gameobject's gossip menu.
        virtual bool OnGossipSelect(PlayerPtr /*Player*/, GameObjectPtr /*go*/, uint32 /*sender*/, uint32 /*action*/) { return false; }

        // Called when a player selects a gossip with a code in the gameobject's gossip menu.
        virtual bool OnGossipSelectCode(PlayerPtr /*Player*/, GameObjectPtr /*go*/, uint32 /*sender*/, uint32 /*action*/, const char* /*code*/) { return false; }

        // Called when a player accepts a quest from the gameobject.
        virtual bool OnQuestAccept(PlayerPtr /*Player*/, GameObjectPtr /*go*/, Quest const* /*quest*/) { return false; }

        // Called when a player selects a quest reward.
        virtual bool OnQuestReward(PlayerPtr /*Player*/, GameObjectPtr /*go*/, Quest const* /*quest*/, uint32 /*opt*/) { return false; }

        // Called when the dialog status between a player and the gameobject is requested.
        virtual uint32 GetDialogStatus(PlayerPtr /*Player*/, GameObjectPtr /*go*/) { return 100; }

        // Called when the game object is destroyed (destructible buildings only).
        virtual void OnDestroyed(GameObjectPtr /*go*/, PlayerPtr /*Player*/) { }

        // Called when the game object is damaged (destructible buildings only).
        virtual void OnDamaged(GameObjectPtr /*go*/, PlayerPtr /*Player*/) { }

        // Called when the game object loot state is changed.
        virtual void OnLootStateChanged(GameObjectPtr /*go*/, uint32 /*state*/, UnitPtr /*Unit*/) { }

        // Called when the game object state is changed.
        virtual void OnGameObjectStateChanged(GameObjectPtr /*go*/, uint32 /*state*/) { }

        // Called when a GameObjectAI object is needed for the gameobject.
        virtual GameObjectAI* GetAI(GameObjectPtr /*go*/) const { return NULL; }
};

class AreaTriggerScript : public ScriptObject
{
    protected:

        AreaTriggerScript(const char* name);

    public:

        bool IsDatabaseBound() const { return true; }

        // Called when the area trigger is activated by a player.
        virtual bool OnTrigger(PlayerPtr /*Player*/, AreaTriggerEntry const* /*trigger*/) { return false; }
};

class BattlegroundScript : public ScriptObject
{
    protected:

        BattlegroundScript(const char* name);

    public:

        bool IsDatabaseBound() const { return true; }

        // Should return a fully valid Battleground object for the type ID.
        virtual Battleground* GetBattleground() const = 0;
};

class OutdoorPvPScript : public ScriptObject
{
    protected:

        OutdoorPvPScript(const char* name);

    public:

        bool IsDatabaseBound() const { return true; }

        // Should return a fully valid OutdoorPvP object for the type ID.
        virtual OutdoorPvP* GetOutdoorPvP() const = 0;
};

class CommandScript : public ScriptObject
{
    protected:

        CommandScript(const char* name);

    public:

        // Should return a pointer to a valid command table (ChatCommand array) to be used by ChatHandler.
        virtual ChatCommand* GetCommands() const = 0;
};

class WeatherScript : public ScriptObject, public UpdatableScript<Weather>
{
    protected:

        WeatherScript(const char* name);

    public:

        bool IsDatabaseBound() const { return true; }

        // Called when the weather changes in the zone this script is associated with.
        virtual void OnChange(WeatherPtr /*Weather*/, WeatherState /*state*/, float /*grade*/) { }
};

class AuctionHouseScript : public ScriptObject
{
    protected:

        AuctionHouseScript(const char* name);

    public:

        // Called when an auction is added to an auction house.
        virtual void OnAuctionAdd(AuctionHouseObject* /*ah*/, AuctionEntry* /*entry*/) { }

        // Called when an auction is removed from an auction house.
        virtual void OnAuctionRemove(AuctionHouseObject* /*ah*/, AuctionEntry* /*entry*/) { }

        // Called when an auction was succesfully completed.
        virtual void OnAuctionSuccessful(AuctionHouseObject* /*ah*/, AuctionEntry* /*entry*/) { }

        // Called when an auction expires.
        virtual void OnAuctionExpire(AuctionHouseObject* /*ah*/, AuctionEntry* /*entry*/) { }
};

class ConditionScript : public ScriptObject
{
    protected:

        ConditionScript(const char* name);

    public:

        bool IsDatabaseBound() const { return true; }

        // Called when a single condition is checked for a player.
        virtual bool OnConditionCheck(Condition* /*condition*/, ConditionSourceInfo& /*sourceInfo*/) { return true; }
};

class VehicleScript : public ScriptObject
{
    protected:

        VehicleScript(const char* name);

    public:

        // Called after a vehicle is installed.
        virtual void OnInstall(VehiclePtr /*veh*/) { }

        // Called after a vehicle is uninstalled.
        virtual void OnUninstall(VehiclePtr /*veh*/) { }

        // Called when a vehicle resets.
        virtual void OnReset(VehiclePtr /*veh*/) { }

        // Called after an accessory is installed in a vehicle.
        virtual void OnInstallAccessory(VehiclePtr /*veh*/, CreaturePtr /*accessory*/) { }

        // Called after a passenger is added to a vehicle.
        virtual void OnAddPassenger(VehiclePtr /*veh*/, UnitPtr /*passenger*/, int8 /*seatId*/) { }

        // Called after a passenger is removed from a vehicle.
        virtual void OnRemovePassenger(VehiclePtr /*veh*/, UnitPtr /*passenger*/) { }
};

class DynamicObjectScript : public ScriptObject, public UpdatableScript<DynamicObject>
{
    protected:

        DynamicObjectScript(const char* name);
};

class TransportScript : public ScriptObject, public UpdatableScript<Transport>
{
    protected:

        TransportScript(const char* name);

    public:

        bool IsDatabaseBound() const { return true; }

        // Called when a player boards the transport.
        virtual void OnAddPassenger(TransportPtr /*Transport*/, PlayerPtr /*Player*/) { }

        // Called when a creature boards the transport.
        virtual void OnAddCreaturePassenger(TransportPtr /*Transport*/, CreaturePtr /*Creature*/) { }

        // Called when a player exits the transport.
        virtual void OnRemovePassenger(TransportPtr /*Transport*/, PlayerPtr /*Player*/) { }

        // Called when a transport moves.
        virtual void OnRelocate(TransportPtr /*Transport*/, uint32 /*waypointId*/, uint32 /*mapId*/, float /*x*/, float /*y*/, float /*z*/) { }
};

class AchievementCriteriaScript : public ScriptObject
{
    protected:

        AchievementCriteriaScript(const char* name);

    public:

        bool IsDatabaseBound() const { return true; }

        // Called when an additional criteria is checked.
        virtual bool OnCheck(PlayerPtr source, UnitPtr target) = 0;
};

class PlayerScript : public ScriptObject
{
    protected:

        PlayerScript(const char* name);

    public:

        // Called when a player kills another player
        virtual void OnPVPKill(PlayerPtr /*killer*/, PlayerPtr /*killed*/) { }

        // Called when a player kills a creature
        virtual void OnCreatureKill(PlayerPtr /*killer*/, CreaturePtr /*killed*/) { }

        // Called when a player is killed by a creature
        virtual void OnPlayerKilledByCreature(CreaturePtr /*killer*/, PlayerPtr /*killed*/) { }

        // Called when a player's level changes (right before the level is applied)
        virtual void OnLevelChanged(PlayerPtr /*Player*/, uint8 /*newLevel*/) { }

        // Called when a player's free talent points change (right before the change is applied)
        virtual void OnFreeTalentPointsChanged(PlayerPtr /*Player*/, uint32 /*points*/) { }

        // Called when a player's talent points are reset (right before the reset is done)
        virtual void OnTalentsReset(PlayerPtr /*Player*/, bool /*noCost*/) { }

        // Called when a player's money is modified (before the modification is done)
        virtual void OnMoneyChanged(PlayerPtr /*Player*/, int64& /*amount*/) { }

        // Called when a player gains XP (before anything is given)
        virtual void OnGiveXP(PlayerPtr /*Player*/, uint32& /*amount*/, UnitPtr /*victim*/) { }

        // Called when a player's reputation changes (before it is actually changed)
        virtual void OnReputationChange(PlayerPtr /*Player*/, uint32 /*factionId*/, int32& /*standing*/, bool /*incremental*/) { }

        // Called when a duel is requested
        virtual void OnDuelRequest(PlayerPtr /*target*/, PlayerPtr /*challenger*/) { }

        // Called when a duel starts (after 3s countdown)
        virtual void OnDuelStart(PlayerPtr /*player1*/, PlayerPtr /*player2*/) { }

        // Called when a duel ends
        virtual void OnDuelEnd(PlayerPtr /*winner*/, PlayerPtr /*loser*/, DuelCompleteType /*type*/) { }

        // The following methods are called when a player sends a chat message.
        virtual void OnChat(PlayerPtr /*Player*/, uint32 /*type*/, uint32 /*lang*/, std::string& /*msg*/) { }

        virtual void OnChat(PlayerPtr /*Player*/, uint32 /*type*/, uint32 /*lang*/, std::string& /*msg*/, PlayerPtr /*receiver*/) { }

        virtual void OnChat(PlayerPtr /*Player*/, uint32 /*type*/, uint32 /*lang*/, std::string& /*msg*/, GroupPtr /*Group*/) { }

        virtual void OnChat(PlayerPtr /*Player*/, uint32 /*type*/, uint32 /*lang*/, std::string& /*msg*/, GuildPtr /*Guild*/) { }

        virtual void OnChat(PlayerPtr /*Player*/, uint32 /*type*/, uint32 /*lang*/, std::string& /*msg*/, Channel* /*channel*/) { }

        // Both of the below are called on emote opcodes.
        virtual void OnEmote(PlayerPtr /*Player*/, uint32 /*emote*/) { }

        virtual void OnTextEmote(PlayerPtr /*Player*/, uint32 /*textEmote*/, uint32 /*emoteNum*/, uint64 /*guid*/) { }

        // Called in Spell::Cast.
        virtual void OnSpellCast(PlayerPtr /*Player*/, Spell* /*spell*/, bool /*skipCheck*/) { }

        // Called when a player logs in.
        virtual void OnLogin(PlayerPtr /*Player*/) { }

        // Called when a player logs out.
        virtual void OnLogout(PlayerPtr /*Player*/) { }

        // Called when a player is created.
        virtual void OnCreate(PlayerPtr /*Player*/) { }

        // Called when a player is deleted.
        virtual void OnDelete(uint64 /*guid*/) { }

        // Called when a player is bound to an instance
        virtual void OnBindToInstance(PlayerPtr /*Player*/, Difficulty /*difficulty*/, uint32 /*mapId*/, bool /*permanent*/) { }

        // Called when a player switches to a new zone
        virtual void OnUpdateZone(PlayerPtr /*Player*/, uint32 /*newZone*/, uint32 /*newArea*/) { }
};

class GuildScript : public ScriptObject
{
    protected:

        GuildScript(const char* name);

    public:

        bool IsDatabaseBound() const { return false; }

        // Called when a member is added to the guild.
        virtual void OnAddMember(GuildPtr /*Guild*/, PlayerPtr /*Player*/, uint8& /*plRank*/) { }

        // Called when a member is removed from the guild.
        virtual void OnRemoveMember(GuildPtr /*Guild*/, PlayerPtr /*Player*/, bool /*isDisbanding*/, bool /*isKicked*/) { }

        // Called when the guild MOTD (message of the day) changes.
        virtual void OnMOTDChanged(GuildPtr /*Guild*/, const std::string& /*newMotd*/) { }

        // Called when the guild info is altered.
        virtual void OnInfoChanged(GuildPtr /*Guild*/, const std::string& /*newInfo*/) { }

        // Called when a guild is created.
        virtual void OnCreate(GuildPtr /*Guild*/, PlayerPtr /*leader*/, const std::string& /*name*/) { }

        // Called when a guild is disbanded.
        virtual void OnDisband(GuildPtr /*Guild*/) { }

        // Called when a guild member withdraws money from a guild bank.
        virtual void OnMemberWitdrawMoney(GuildPtr /*Guild*/, PlayerPtr /*Player*/, uint32& /*amount*/, bool /*isRepair*/) { }

        // Called when a guild member deposits money in a guild bank.
        virtual void OnMemberDepositMoney(GuildPtr /*Guild*/, PlayerPtr /*Player*/, uint32& /*amount*/) { }

        // Called when a guild member moves an item in a guild bank.
        virtual void OnItemMove(GuildPtr /*Guild*/, PlayerPtr /*Player*/, ItemPtr /*pItem*/, bool /*isSrcBank*/, uint8 /*srcContainer*/, uint8 /*srcSlotId*/,
            bool /*isDestBank*/, uint8 /*destContainer*/, uint8 /*destSlotId*/) { }

        virtual void OnEvent(GuildPtr /*Guild*/, uint8 /*eventType*/, uint32 /*playerGuid1*/, uint32 /*playerGuid2*/, uint8 /*newRank*/) { }

        virtual void OnBankEvent(GuildPtr /*Guild*/, uint8 /*eventType*/, uint8 /*tabId*/, uint32 /*playerGuid*/, uint32 /*itemOrMoney*/, uint16 /*itemStackCount*/, uint8 /*destTabId*/) { }
};

class GroupScript : public ScriptObject
{
    protected:

        GroupScript(const char* name);

    public:

        bool IsDatabaseBound() const { return false; }

        // Called when a member is added to a group.
        virtual void OnAddMember(GroupPtr /*Group*/, uint64 /*guid*/) { }

        // Called when a member is invited to join a group.
        virtual void OnInviteMember(GroupPtr /*Group*/, uint64 /*guid*/) { }

        // Called when a member is removed from a group.
        virtual void OnRemoveMember(GroupPtr /*Group*/, uint64 /*guid*/, RemoveMethod /*method*/, uint64 /*kicker*/, const char* /*reason*/) { }

        // Called when the leader of a group is changed.
        virtual void OnChangeLeader(GroupPtr /*Group*/, uint64 /*newLeaderGuid*/, uint64 /*oldLeaderGuid*/) { }

        // Called when a group is disbanded.
        virtual void OnDisband(GroupPtr /*Group*/) { }
};

// Placed here due to ScriptRegistry::AddScript dependency.
#define sScriptMgr ACE_Singleton<ScriptMgr, ACE_Null_Mutex>::instance()

// Manages registration, loading, and execution of scripts.
class ScriptMgr
{
    friend class ACE_Singleton<ScriptMgr, ACE_Null_Mutex>;
    friend class ScriptObject;

    private:

        ScriptMgr();
        virtual ~ScriptMgr();

    public: /* Initialization */

        void Initialize();
        void LoadDatabase();
        void FillSpellSummary();

        const char* ScriptsVersion() const { return "Integrated Trinity Scripts"; }

        void IncrementScriptCount() { ++_scriptCount; }
        uint32 GetScriptCount() const { return _scriptCount; }

    public: /* Unloading */

        void Unload();

    public: /* SpellScriptLoader */

        void CreateSpellScripts(uint32 spellId, std::list<SpellScript*>& scriptVector);
        void CreateAuraScripts(uint32 spellId, std::list<AuraScript*>& scriptVector);
        void CreateSpellScriptLoaders(uint32 spellId, std::vector<std::pair<SpellScriptLoader*, std::multimap<uint32, uint32>::iterator> >& scriptVector);

    public: /* ServerScript */

        void OnNetworkStart();
        void OnNetworkStop();
        void OnSocketOpen(WorldSocket* socket);
        void OnSocketClose(WorldSocket* socket, bool wasNew);
        void OnPacketReceive(WorldSocket* socket, WorldPacket packet);
        void OnPacketSend(WorldSocket* socket, WorldPacket packet);
        void OnUnknownPacketReceive(WorldSocket* socket, WorldPacket packet);

    public: /* WorldScript */

        void OnOpenStateChange(bool open);
        void OnConfigLoad(bool reload);
        void OnMotdChange(std::string& newMotd);
        void OnShutdownInitiate(ShutdownExitCode code, ShutdownMask mask);
        void OnShutdownCancel();
        void OnWorldUpdate(uint32 diff);
        void OnStartup();
        void OnShutdown();

    public: /* FormulaScript */

        void OnHonorCalculation(float& honor, uint8 level, float multiplier);
        void OnGrayLevelCalculation(uint8& grayLevel, uint8 playerLevel);
        void OnColorCodeCalculation(XPColorChar& color, uint8 playerLevel, uint8 mobLevel);
        void OnZeroDifferenceCalculation(uint8& diff, uint8 playerLevel);
        void OnBaseGainCalculation(uint32& gain, uint8 playerLevel, uint8 mobLevel, ContentLevels content);
        void OnGainCalculation(uint32& gain, PlayerPtr player, UnitPtr unit);
        void OnGroupRateCalculation(float& rate, uint32 count, bool isRaid);

    public: /* MapScript */

        void OnCreateMap(MapPtr map);
        void OnDestroyMap(MapPtr map);
        void OnLoadGridMap(MapPtr map, GridMap* gmap, uint32 gx, uint32 gy);
        void OnUnloadGridMap(MapPtr map, GridMap* gmap, uint32 gx, uint32 gy);
        void OnPlayerEnterMap(MapPtr map, PlayerPtr player);
        void OnPlayerLeaveMap(MapPtr map, PlayerPtr player);
        void OnMapUpdate(MapPtr map, uint32 diff);

    public: /* InstanceMapScript */

        InstanceScript* CreateInstanceData(InstanceMapPtr map);

    public: /* ItemScript */

        bool OnDummyEffect(UnitPtr caster, uint32 spellId, SpellEffIndex effIndex, ItemPtr target);
        bool OnQuestAccept(PlayerPtr player, ItemPtr item, Quest const* quest);
        bool OnItemUse(PlayerPtr player, ItemPtr item, SpellCastTargets const& targets);
        bool OnItemExpire(PlayerPtr player, ItemTemplate const* proto);

    public: /* CreatureScript */

        bool OnDummyEffect(UnitPtr caster, uint32 spellId, SpellEffIndex effIndex, CreaturePtr target);
        bool OnGossipHello(PlayerPtr player, CreaturePtr creature);
        bool OnGossipSelect(PlayerPtr player, CreaturePtr creature, uint32 sender, uint32 action);
        bool OnGossipSelectCode(PlayerPtr player, CreaturePtr creature, uint32 sender, uint32 action, const char* code);
        bool OnQuestAccept(PlayerPtr player, CreaturePtr creature, Quest const* quest);
        bool OnQuestSelect(PlayerPtr player, CreaturePtr creature, Quest const* quest);
        bool OnQuestComplete(PlayerPtr player, CreaturePtr creature, Quest const* quest);
        bool OnQuestReward(PlayerPtr player, CreaturePtr creature, Quest const* quest, uint32 opt);
        uint32 GetDialogStatus(PlayerPtr player, CreaturePtr creature);
        CreatureAI* GetCreatureAI(CreaturePtr creature);
        void OnCreatureUpdate(CreaturePtr creature, uint32 diff);

    public: /* GameObjectScript */

        bool OnDummyEffect(UnitPtr caster, uint32 spellId, SpellEffIndex effIndex, GameObjectPtr target);
        bool OnGossipHello(PlayerPtr player, GameObjectPtr go);
        bool OnGossipSelect(PlayerPtr player, GameObjectPtr go, uint32 sender, uint32 action);
        bool OnGossipSelectCode(PlayerPtr player, GameObjectPtr go, uint32 sender, uint32 action, const char* code);
        bool OnQuestAccept(PlayerPtr player, GameObjectPtr go, Quest const* quest);
        bool OnQuestReward(PlayerPtr player, GameObjectPtr go, Quest const* quest, uint32 opt);
        uint32 GetDialogStatus(PlayerPtr player, GameObjectPtr go);
        void OnGameObjectDestroyed(GameObjectPtr go, PlayerPtr player);
        void OnGameObjectDamaged(GameObjectPtr go, PlayerPtr player);
        void OnGameObjectLootStateChanged(GameObjectPtr go, uint32 state, UnitPtr unit);
        void OnGameObjectStateChanged(GameObjectPtr go, uint32 state);
        void OnGameObjectUpdate(GameObjectPtr go, uint32 diff);
        GameObjectAI* GetGameObjectAI(GameObjectPtr go);

    public: /* AreaTriggerScript */

        bool OnAreaTrigger(PlayerPtr player, AreaTriggerEntry const* trigger);

    public: /* BattlegroundScript */

        Battleground* CreateBattleground(BattlegroundTypeId typeId);

    public: /* OutdoorPvPScript */

        OutdoorPvP* CreateOutdoorPvP(OutdoorPvPData const* data);

    public: /* CommandScript */

        std::vector<ChatCommand*> GetChatCommands();

    public: /* WeatherScript */

        void OnWeatherChange(WeatherPtr weather, WeatherState state, float grade);
        void OnWeatherUpdate(WeatherPtr weather, uint32 diff);

    public: /* AuctionHouseScript */

        void OnAuctionAdd(AuctionHouseObject* ah, AuctionEntry* entry);
        void OnAuctionRemove(AuctionHouseObject* ah, AuctionEntry* entry);
        void OnAuctionSuccessful(AuctionHouseObject* ah, AuctionEntry* entry);
        void OnAuctionExpire(AuctionHouseObject* ah, AuctionEntry* entry);

    public: /* ConditionScript */

        bool OnConditionCheck(Condition* condition, ConditionSourceInfo& sourceInfo);

    public: /* VehicleScript */

        void OnInstall(VehiclePtr veh);
        void OnUninstall(VehiclePtr veh);
        void OnReset(VehiclePtr veh);
        void OnInstallAccessory(VehiclePtr veh, CreaturePtr accessory);
        void OnAddPassenger(VehiclePtr veh, UnitPtr passenger, int8 seatId);
        void OnRemovePassenger(VehiclePtr veh, UnitPtr passenger);

    public: /* DynamicObjectScript */

        void OnDynamicObjectUpdate(DynamicObjectPtr dynobj, uint32 diff);

    public: /* TransportScript */

        void OnAddPassenger(TransportPtr transport, PlayerPtr player);
        void OnAddCreaturePassenger(TransportPtr transport, CreaturePtr creature);
        void OnRemovePassenger(TransportPtr transport, PlayerPtr player);
        void OnTransportUpdate(TransportPtr transport, uint32 diff);
        void OnRelocate(TransportPtr transport, uint32 waypointId, uint32 mapId, float x, float y, float z);

    public: /* AchievementCriteriaScript */

        bool OnCriteriaCheck(AchievementCriteriaData const* data, PlayerPtr source, UnitPtr target);

    public: /* PlayerScript */

        void OnPVPKill(PlayerPtr killer, PlayerPtr killed);
        void OnCreatureKill(PlayerPtr killer, CreaturePtr killed);
        void OnPlayerKilledByCreature(CreaturePtr killer, PlayerPtr killed);
        void OnPlayerLevelChanged(PlayerPtr player, uint8 oldLevel);
        void OnPlayerFreeTalentPointsChanged(PlayerPtr player, uint32 newPoints);
        void OnPlayerTalentsReset(PlayerPtr player, bool noCost);
        void OnPlayerMoneyChanged(PlayerPtr player, int64& amount);
        void OnGivePlayerXP(PlayerPtr player, uint32& amount, UnitPtr victim);
        void OnPlayerReputationChange(PlayerPtr player, uint32 factionID, int32& standing, bool incremental);
        void OnPlayerDuelRequest(PlayerPtr target, PlayerPtr challenger);
        void OnPlayerDuelStart(PlayerPtr player1, PlayerPtr player2);
        void OnPlayerDuelEnd(PlayerPtr winner, PlayerPtr loser, DuelCompleteType type);
        void OnPlayerChat(PlayerPtr player, uint32 type, uint32 lang, std::string& msg);
        void OnPlayerChat(PlayerPtr player, uint32 type, uint32 lang, std::string& msg, PlayerPtr receiver);
        void OnPlayerChat(PlayerPtr player, uint32 type, uint32 lang, std::string& msg, GroupPtr group);
        void OnPlayerChat(PlayerPtr player, uint32 type, uint32 lang, std::string& msg, GuildPtr guild);
        void OnPlayerChat(PlayerPtr player, uint32 type, uint32 lang, std::string& msg, Channel* channel);
        void OnPlayerEmote(PlayerPtr player, uint32 emote);
        void OnPlayerTextEmote(PlayerPtr player, uint32 textEmote, uint32 emoteNum, uint64 guid);
        void OnPlayerSpellCast(PlayerPtr player, Spell* spell, bool skipCheck);
        void OnPlayerLogin(PlayerPtr player);
        void OnPlayerLogout(PlayerPtr player);
        void OnPlayerCreate(PlayerPtr player);
        void OnPlayerDelete(uint64 guid);
        void OnPlayerBindToInstance(PlayerPtr player, Difficulty difficulty, uint32 mapid, bool permanent);
        void OnPlayerUpdateZone(PlayerPtr player, uint32 newZone, uint32 newArea);

    public: /* GuildScript */

        void OnGuildAddMember(GuildPtr guild, PlayerPtr player, uint8& plRank);
        void OnGuildRemoveMember(GuildPtr guild, PlayerPtr player, bool isDisbanding, bool isKicked);
        void OnGuildMOTDChanged(GuildPtr guild, const std::string& newMotd);
        void OnGuildInfoChanged(GuildPtr guild, const std::string& newInfo);
        void OnGuildCreate(GuildPtr guild, PlayerPtr leader, const std::string& name);
        void OnGuildDisband(GuildPtr guild);
        void OnGuildMemberWitdrawMoney(GuildPtr guild, PlayerPtr player, uint32 &amount, bool isRepair);
        void OnGuildMemberDepositMoney(GuildPtr guild, PlayerPtr player, uint32 &amount);
        void OnGuildItemMove(GuildPtr guild, PlayerPtr player, ItemPtr pItem, bool isSrcBank, uint8 srcContainer, uint8 srcSlotId,
            bool isDestBank, uint8 destContainer, uint8 destSlotId);
        void OnGuildEvent(GuildPtr guild, uint8 eventType, uint32 playerGuid1, uint32 playerGuid2, uint8 newRank);
        void OnGuildBankEvent(GuildPtr guild, uint8 eventType, uint8 tabId, uint32 playerGuid, uint32 itemOrMoney, uint16 itemStackCount, uint8 destTabId);

    public: /* GroupScript */

        void OnGroupAddMember(GroupPtr group, uint64 guid);
        void OnGroupInviteMember(GroupPtr group, uint64 guid);
        void OnGroupRemoveMember(GroupPtr group, uint64 guid, RemoveMethod method, uint64 kicker, const char* reason);
        void OnGroupChangeLeader(GroupPtr group, uint64 newLeaderGuid, uint64 oldLeaderGuid);
        void OnGroupDisband(GroupPtr group);

    public: /* Scheduled scripts */

        uint32 IncreaseScheduledScriptsCount() { return ++_scheduledScripts; }
        uint32 DecreaseScheduledScriptCount() { return --_scheduledScripts; }
        uint32 DecreaseScheduledScriptCount(size_t count) { return _scheduledScripts -= count; }
        bool IsScriptScheduled() const { return _scheduledScripts > 0; }

    private:

        uint32 _scriptCount;

        //atomic op counter for active scripts amount
        ACE_Atomic_Op<ACE_Thread_Mutex, long> _scheduledScripts;
};

#endif
