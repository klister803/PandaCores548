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

#ifndef _THREATMANAGER
#define _THREATMANAGER

#include "Common.h"
#include "SharedDefines.h"
#include "LinkedReference/Reference.h"
#include "UnitEvents.h"

#include <list>

//==============================================================

class Unit;
class Creature;
class ThreatManager;
class SpellInfo;

#define THREAT_UPDATE_INTERVAL 1 * IN_MILLISECONDS    // Server should send threat update to client periodically each second

//==============================================================
// Class to calculate the real threat based

struct ThreatCalcHelper
{
    static float calcThreat(UnitPtr hatedUnit, UnitPtr hatingUnit, float threat, SpellSchoolMask schoolMask = SPELL_SCHOOL_MASK_NORMAL, SpellInfo const* threatSpell = nullptr);
    static bool isValidProcess(UnitPtr hatedUnit, UnitPtr hatingUnit, SpellInfo const* threatSpell = nullptr);
};

//==============================================================
class HostileReference : public Reference<Unit, ThreatManager>
{
    public:
        HostileReference(UnitPtr refUnit, ThreatManager* threatManager, float threat);

        //=================================================
        void addThreat(float modThreat);

        void setThreat(float threat) { addThreat(threat - getThreat()); }

        void addThreatPercent(int32 percent);

        float getThreat() const { return iThreat; }

        bool isOnline() const { return iOnline; }

        // The Unit might be in water and the creature can not enter the water, but has range attack
        // in this case online = true, but accessible = false
        bool isAccessible() const { return iAccessible; }

        // used for temporary setting a threat and reducting it later again.
        // the threat modification is stored
        void setTempThreat(float threat)
        {
            addTempThreat(threat - getThreat());
        }

        void addTempThreat(float threat)
        {
            iTempThreatModifier = threat;
            if (iTempThreatModifier != 0.0f)
                addThreat(iTempThreatModifier);
        }

        void resetTempThreat()
        {
            if (iTempThreatModifier != 0.0f)
            {
                addThreat(-iTempThreatModifier);
                iTempThreatModifier = 0.0f;
            }
        }

        float getTempThreatModifier() { return iTempThreatModifier; }

        //=================================================
        // check, if source can reach target and set the status
        void updateOnlineStatus();

        void setOnlineOfflineState(bool isOnline);

        void setAccessibleState(bool isAccessible);
        //=================================================

        bool operator == (const HostileReference& hostileRef) const { return hostileRef.getUnitGuid() == getUnitGuid(); }

        //=================================================

        uint64 getUnitGuid() const { return iUnitGuid; }

        //=================================================
        // reference is not needed anymore. realy delete it !

        void removeReference();

        //=================================================

        std::shared_ptr<HostileReference> next() { return CAST(HostileReference, (Reference<Unit, ThreatManager>::next())); }

        //=================================================

        // Tell our refTo (target) object that we have a link
        void targetObjectBuildLink();

        // Tell our refTo (taget) object, that the link is cut
        void targetObjectDestroyLink();

        // Tell our refFrom (source) object, that the link is cut (Target destroyed)
        void sourceObjectDestroyLink();
    private:
        // Inform the source, that the status of that reference was changed
        void fireStatusChanged(ThreatRefStatusChangeEvent& threatRefStatusChangeEvent);

        UnitPtr getSourceUnit();
    private:
        float iThreat;
        float iTempThreatModifier;                          // used for taunt
        uint64 iUnitGuid;
        bool iOnline;
        bool iAccessible;
};

//==============================================================
class ThreatManager;

class ThreatContainer
{
    private:
        std::list<HostileReferencePtr> iThreatList;
        bool iDirty;
    protected:
        friend class ThreatManager;

        void remove(HostileReferencePtr hostileRef) { iThreatList.remove(hostileRef); }
        void addReference(HostileReferencePtr hostileRef) { iThreatList.push_back(hostileRef); }
        void clearReferences();

        // Sort the list if necessary
        void update();
    public:
        ThreatContainer() { iDirty = false; }
        ~ThreatContainer() { clearReferences(); }

        HostileReferencePtr addThreat(UnitPtr victim, float threat);

        void modifyThreatPercent(UnitPtr victim, int32 percent);

        HostileReferencePtr selectNextVictim(CreaturePtr attacker, HostileReferencePtr currentVictim);

        void setDirty(bool isDirty) { iDirty = isDirty; }

        bool isDirty() const { return iDirty; }

        bool empty() const { return iThreatList.empty(); }

        HostileReferencePtr getMostHated() { return iThreatList.empty() ? nullptr : iThreatList.front(); }

        HostileReferencePtr getReferenceByTarget(UnitPtr victim);

        std::list<HostileReferencePtr>& getThreatList() { return iThreatList; }
};

//=================================================

class ThreatManager
{
    public:
        friend class HostileReference;

        explicit ThreatManager(UnitPtr owner);

        ~ThreatManager() { clearReferences(); }

        void clearReferences();

        void addThreat(UnitPtr victim, float threat, SpellSchoolMask schoolMask = SPELL_SCHOOL_MASK_NORMAL, SpellInfo const* threatSpell = nullptr);

        void doAddThreat(UnitPtr victim, float threat);

        void modifyThreatPercent(UnitPtr victim, int32 percent);

        float getThreat(UnitPtr victim, bool alsoSearchOfflineList = false);

        bool isThreatListEmpty() { return iThreatContainer.empty(); }

        void processThreatEvent(ThreatRefStatusChangeEvent* threatRefStatusChangeEvent);

        bool isNeedUpdateToClient(uint32 time);

        HostileReferencePtr getCurrentVictim() { return iCurrentVictim; }

        UnitPtr getOwner() { return iOwner; }

        UnitPtr getHostilTarget();

        void tauntApply(UnitPtr taunter);
        void tauntFadeOut(UnitPtr taunter);

        void setCurrentVictim(HostileReferencePtr hostileRef);

        void setDirty(bool isDirty) { iThreatContainer.setDirty(isDirty); }

        // Reset all aggro without modifying the threadlist.
        void resetAllAggro();

        // Reset all aggro of unit in threadlist satisfying the predicate.
        template<class PREDICATE> void resetAggro(PREDICATE predicate)
        {
            std::list<HostileReferencePtr> &threatList = getThreatList();
            if (threatList.empty())
                return;

            for (std::list<HostileReferencePtr>::iterator itr = threatList.begin(); itr != threatList.end(); ++itr)
            {
                HostileReferencePtr ref = (*itr);

                if (predicate(ref->getTarget()))
                {
                    ref->setThreat(0);
                    setDirty(true);
                }
            }
        }

        // methods to access the lists from the outside to do some dirty manipulation (scriping and such)
        // I hope they are used as little as possible.
        std::list<HostileReferencePtr>& getThreatList() { return iThreatContainer.getThreatList(); }
        std::list<HostileReferencePtr>& getOfflineThreatList() { return iThreatOfflineContainer.getThreatList(); }
        ThreatContainer& getOnlineContainer() { return iThreatContainer; }
        ThreatContainer& getOfflineContainer() { return iThreatOfflineContainer; }
    private:
        void _addThreat(UnitPtr victim, float threat);

        HostileReferencePtr iCurrentVictim;
        UnitPtr iOwner;
        uint32 iUpdateTimer;
        ThreatContainer iThreatContainer;
        ThreatContainer iThreatOfflineContainer;
};

//=================================================

namespace Trinity
{
    // Binary predicate for sorting HostileReferences based on threat value
    class ThreatOrderPred
    {
        public:
            ThreatOrderPred(bool ascending = false) : m_ascending(ascending) {}
            bool operator() (constHostileReferencePtr a, constHostileReferencePtr b) const
            {
                return m_ascending ? a->getThreat() < b->getThreat() : a->getThreat() > b->getThreat();
            }
        private:
            const bool m_ascending;
    };
}
#endif

