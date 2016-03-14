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
    LOAD_PLAYER_DATA                = 0,
    LOAD_ACCOUNT_DATA               = 1,
    LOAD_PLAYER_LOGIN               = 2,
    LOAD_PLAYER_CRITERIA            = 3,
    LOAD_ACCOUNT_CRITERIA           = 4,
    LOAD_PLAYER_ITEMS               = 5,
};

#endif

