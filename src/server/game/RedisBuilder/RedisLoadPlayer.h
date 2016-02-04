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

#ifndef __SavePlayer_H
#define __SavePlayer_H

enum LoadingStep
{
    LOAD_PLAYER_HOMEBIND            = 0,
    LOAD_PLAYER_DATA                = 1,
    LOAD_PLAYER_GROUP               = 2,
    LOAD_PLAYER_LOOTCOOLDOWN        = 3,
    LOAD_PLAYER_CURRENCY            = 4,
    LOAD_PLAYER_BOUNDINSTANCES      = 5,
    LOAD_PLAYER_BGDATA              = 6,
    LOAD_PLAYER_BATTLEPETS          = 7,
    LOAD_PLAYER_BATTLEPETSLOTS      = 8,
    LOAD_PLAYER_SKILLS              = 9,
    LOAD_PLAYER_ARCHAEOLOGY         = 10,
    LOAD_PLAYER_TALENTS             = 11,
    LOAD_PLAYER_SPELLS              = 12,
    LOAD_PLAYER_MOUNTS              = 13,
    LOAD_PLAYER_GLYPHS              = 14,
    LOAD_PLAYER_AURAS               = 15,
    LOAD_PLAYER_QUESTSTATUS         = 16,
    LOAD_ACCOUNT_QUESTSTATUS        = 17,
    LOAD_PLAYER_QUESTREWARDED       = 18,
    LOAD_ACCOUNT_QUESTREWARDED      = 19,
    LOAD_PLAYER_QUESTDAILY          = 20,
    LOAD_ACCOUNT_QUESTDAILY         = 21,
    LOAD_PLAYER_QUESTWEEKLY         = 22,
    LOAD_ACCOUNT_QUESTWEEKLY        = 23,
    LOAD_PLAYER_QUESTSEASONAL       = 24,
    LOAD_ACCOUNT_QUESTSEASONAL      = 25,
    LOAD_PLAYER_REPUTATION          = 26,
    LOAD_PLAYER_ITEMS               = 27,
    LOAD_PLAYER_VOIDSTORAGE         = 28,
    LOAD_PLAYER_ACTIONS             = 29,
    LOAD_PLAYER_SOCIAL              = 30,
    LOAD_PLAYER_SPELLCOOLDOWNS      = 31,
    LOAD_PLAYER_KILLS               = 32,
    LOAD_PLAYER_INIT_THIRD          = 33,
    LOAD_PLAYER_DECLINEDNAME        = 34,
    LOAD_PLAYER_EQUIPMENTSETS       = 35,
    LOAD_PLAYER_CUFPROFILES         = 36,
    LOAD_PLAYER_VISUALS             = 37,
    LOAD_PLAYER_PLAYERACCOUNTDATA   = 38,
    LOAD_PLAYER_LOGIN               = 39,
    LOAD_PLAYER_ACHIEVEMENT         = 40,
    LOAD_ACCOUNT_ACHIEVEMENT        = 41,
    LOAD_PLAYER_CRITERIA            = 42,
    LOAD_ACCOUNT_CRITERIA           = 43,
    LOAD_PLAYER_NEXT                = 44,
    LOAD_PLAYER_GOLD                = 45,
    LOAD_PLAYER_MAILS               = 46,
    LOAD_PLAYER_MAIL_ITEMS          = 47,
};

#endif

