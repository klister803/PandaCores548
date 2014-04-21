/*
 * Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
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

#include "ScriptMgr.h"

class go_challenge : public GameObjectScript
{
public:
    go_challenge() : GameObjectScript("go_challenge") { }

    bool OnGossipSelect(Player* player, GameObject* go, uint32 /*sender*/, uint32 /*action*/)
    {

        WorldPacket data(SMSG_CHALLENGE_UNK);
        player->GetSession()->SendPacket(&data);

        data.Initialize(SMSG_START_TIMER, 12);
        data << uint32(1);
        data << uint32(5);
        data << uint32(5);
        player->GetSession()->SendPacket(&data);

        data.Initialize(SMSG_WORLD_STATE_TIMER_START, 8);
        data << uint32(0);
        data << uint32(1);
        player->GetSession()->SendPacket(&data);

        player->PlayerTalkClass->ClearMenus();
        return true;
    }
};

void AddSC_challenge_scripts()
{
    new go_challenge();
}
