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

#ifndef DEF_PINNACLE_H
#define DEF_PINNACLE_H

enum Data
{
    DATA_SVALA_SORROWGRAVE_EVENT,
    DATA_GORTOK_PALEHOOF_EVENT,
    DATA_SKADI_THE_RUTHLESS_EVENT,
    DATA_KING_YMIRON_EVENT
};
enum Data64
{
    DATA_SVALA,
    DATA_SVALA_SORROWGRAVE,
    DATA_GORTOK_PALEHOOF,
    DATA_SKADI_THE_RUTHLESS,
    DATA_MOB_GRAUF,
    DATA_KING_YMIRON,
    DATA_MOB_FRENZIED_WORGEN,
    DATA_MOB_RAVENOUS_FURBOLG,
    DATA_MOB_MASSIVE_JORMUNGAR,
    DATA_MOB_FEROCIOUS_RHINO,
    DATA_MOB_ORB,
    DATA_GORTOK_PALEHOOF_SPHERE,
    DATA_SACRIFICED_PLAYER,
    DATA_SVALA_INTRO,
    DATA_GET_SACRIFICE_TARGET,
    DATA_KILL_RITUAL_CHANNELER,
};

enum eCreatures
{
    BOSS_SVALA_SORROWGRAVE     = 26668,
    BOSS_GORTOK_PALEHOOF       = 26687,
    BOSS_SKADI_RUTHLESS        = 26693,
    BOSS_KING_YMIRON           = 26861,
    MOB_FRENZIED_WORGEN        = 26683,
    MOB_RAVENOUS_FURBOLG       = 26684,
    MOB_MASSIVE_JORMUNGAR      = 26685,
    MOB_FEROCIOUS_RHINO        = 26686,
    MOB_SVALA                  = 29281,
    MOB_PALEHOOF_ORB           = 26688,
    NPC_RITUAL_TARGET          = 27327,
    NPC_RITUAL_CHANNELER       = 27281,
    CREATURE_ARTHAS            = 29280, // Image of Arthas
    CREATURE_SVALA_SORROWGRAVE = 26668, // Svala after transformation
    CREATURE_SVALA             = 29281, // Svala before transformation
    CREATURE_RITUAL_CHANNELER  = 27281,
    CREATURE_SPECTATOR         = 26667,
    CREATURE_RITUAL_TARGET     = 27327,
    CREATURE_FLAME_BRAZIER     = 27273,
    CREATURE_SCOURGE_HULK      = 26555,
};

enum SomeData
{
    SVALA_CALL_FLAMES = 17841,
    SPELL_BALL_OF_FLAME = 48246,
    ACTION_RE_ATTACK = 120,
};

#endif
