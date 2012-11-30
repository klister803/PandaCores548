# Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

set(scripts_STAT_SRCS
  ${scripts_STAT_SRCS}
  Pandaria/MogushanPalace/instance_mogu_shan_palace.cpp
  Pandaria/MogushanPalace/boss_trial_of_the_king.cpp
  Pandaria/MogushanPalace/boss_gekkan.cpp
  Pandaria/MogushanPalace/boss_xin_the_weaponmaster.cpp

  Pandaria/TempleJadeSerpent/instance_temple_of_jade_serpent.cpp
  Pandaria/TempleJadeSerpent/boss_wise_mari.cpp
  Pandaria/TempleJadeSerpent/boss_lorewalker_stonestep.cpp
  Pandaria/TempleJadeSerpent/boss_liu_flameheart.cpp
  Pandaria/TempleJadeSerpent/boss_sha_of_doubt.cpp

  Pandaria/StormstoutBrewery/instance_stormstout_brewery.cpp
  Pandaria/StormstoutBrewery/stormstout_brewery.h
)

message("  -> Prepared: Pandaria")
