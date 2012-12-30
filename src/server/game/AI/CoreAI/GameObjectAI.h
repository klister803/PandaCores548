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

#ifndef TRINITY_GAMEOBJECTAI_H
#define TRINITY_GAMEOBJECTAI_H

#include "Define.h"
#include <list>
#include "Object.h"
#include "GameObject.h"
#include "CreatureAI.h"

class GameObjectAI
{
    protected:
        GameObjectPtr const go;
    public:
        explicit GameObjectAI(GameObjectPtr g) : go(g) {}
        virtual ~GameObjectAI() {}

        virtual void UpdateAI(uint32 /*diff*/) {}

        virtual void InitializeAI() { Reset(); }

        virtual void Reset() {};

        // Pass parameters between AI
        virtual void DoAction(const int32 /*param = 0 */) {}
        virtual void SetGUID(const uint64& /*guid*/, int32 /*id = 0 */) {}
        virtual uint64 GetGUID(int32 /*id = 0 */) { return 0; }

        static int Permissible(constGameObjectPtr go);

        virtual bool GossipHello(PlayerPtr /*Player*/) { return false; }
        virtual bool GossipSelect(PlayerPtr /*Player*/, uint32 /*sender*/, uint32 /*action*/) { return false; }
        virtual bool GossipSelectCode(PlayerPtr /*Player*/, uint32 /*sender*/, uint32 /*action*/, char const* /*code*/) { return false; }
        virtual bool QuestAccept(PlayerPtr /*Player*/, Quest const* /*quest*/) { return false; }
        virtual bool QuestReward(PlayerPtr /*Player*/, Quest const* /*quest*/, uint32 /*opt*/) { return false; }
        virtual uint32 GetDialogStatus(PlayerPtr /*Player*/) { return 100; }
        virtual void Destroyed(PlayerPtr /*Player*/, uint32 /*eventId*/) {}
        virtual uint32 GetData(uint32 /*id*/) { return 0; }
        virtual void SetData64(uint32 /*id*/, uint64 /*value*/) {}
        virtual uint64 GetData64(uint32 /*id*/) { return 0; }
        virtual void SetData(uint32 /*id*/, uint32 /*value*/) {}
        virtual void OnGameEvent(bool /*start*/, uint16 /*eventId*/) {}
        virtual void OnStateChanged(uint32 /*state*/, UnitPtr /*Unit*/) {}
        virtual void EventInform(uint32 /*eventId*/) {}
};

class NullGameObjectAI : public GameObjectAI
{
    public:
        explicit NullGameObjectAI(GameObjectPtr g);

        void UpdateAI(uint32 /*diff*/) {}

        static int Permissible(constGameObjectPtr /*go*/) { return PERMIT_BASE_IDLE; }
};
#endif
