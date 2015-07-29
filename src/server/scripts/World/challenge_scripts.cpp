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
#include "ChallengeMgr.h"

class go_challenge : public GameObjectScript
{
public:
    go_challenge() : GameObjectScript("go_challenge") { }

    bool OnGossipSelect(Player* player, GameObject* go, uint32 /*sender*/, uint32 /*action*/)
    {
        if (InstanceScript* instance = player->GetInstanceScript())
        {
            //WorldPacket data(SMSG_CHALLENGE_UNK);
            //player->GetSession()->SendPacket(&data);
            instance->StartChallenge();
            //sChallengeMgr->GroupReward(instance->instance, 100, ChallengeMode(1));
        }

        player->PlayerTalkClass->ClearMenus();
        player->CLOSE_GOSSIP_MENU();
        return false;
    }
};

//@ Challenge Conqueror: Gold (6378)
//Golden Chest of the Golden King
enum GoldenChest
{
    GOLDEN_CHEST_OF_THE_GOLDEN_KING_WARRIOR     = 90155,
    GOLDEN_CHEST_OF_THE_GOLDEN_KING_PALADIN     = 90161,
    GOLDEN_CHEST_OF_THE_GOLDEN_KING_HUNTER      = 90163,
    GOLDEN_CHEST_OF_THE_GOLDEN_KING_ROGUE       = 90159,
    GOLDEN_CHEST_OF_THE_GOLDEN_KING_PRIEST      = 90160,
    GOLDEN_CHEST_OF_THE_GOLDEN_KING_DEATH_KNIGHT= 90165,
    GOLDEN_CHEST_OF_THE_GOLDEN_KING_SHAMAN      = 90157,
    GOLDEN_CHEST_OF_THE_GOLDEN_KING_MAGE        = 90158,
    GOLDEN_CHEST_OF_THE_GOLDEN_KING_WARLOCK     = 90156,
    GOLDEN_CHEST_OF_THE_GOLDEN_KING_MONK        = 90162,
    GOLDEN_CHEST_OF_THE_GOLDEN_KING_DRUID       = 90164,
};

class challenge_achieve_reward : public AchievementRewardScript
{
public:
    challenge_achieve_reward() : AchievementRewardScript("challenge_achieve_reward") {}

    uint32 SelectItem(Player* source, AchievementReward const* data)
    {
        switch(source->getClass())
        {
            case CLASS_WARRIOR:
                return GOLDEN_CHEST_OF_THE_GOLDEN_KING_WARRIOR;
            case CLASS_PALADIN:
                return GOLDEN_CHEST_OF_THE_GOLDEN_KING_PALADIN;
            case CLASS_HUNTER:
                return GOLDEN_CHEST_OF_THE_GOLDEN_KING_HUNTER;
            case CLASS_ROGUE:
                return GOLDEN_CHEST_OF_THE_GOLDEN_KING_ROGUE;
            case CLASS_PRIEST:
                return GOLDEN_CHEST_OF_THE_GOLDEN_KING_PRIEST;
            case CLASS_DEATH_KNIGHT:
                return GOLDEN_CHEST_OF_THE_GOLDEN_KING_DEATH_KNIGHT;
            case CLASS_SHAMAN:
                return GOLDEN_CHEST_OF_THE_GOLDEN_KING_SHAMAN;
            case CLASS_MAGE:
                return GOLDEN_CHEST_OF_THE_GOLDEN_KING_MAGE;
            case CLASS_WARLOCK:
                return GOLDEN_CHEST_OF_THE_GOLDEN_KING_WARLOCK;
            case CLASS_MONK:
                return GOLDEN_CHEST_OF_THE_GOLDEN_KING_MONK;
            case CLASS_DRUID:
                return GOLDEN_CHEST_OF_THE_GOLDEN_KING_DRUID;
        }
        return 0;
    }
};

void AddSC_challenge_scripts()
{
    new go_challenge();
    new challenge_achieve_reward();
}
