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

  Scenario/ArenaofAnnihilation/instance_arena_of_annihilation.cpp
  Scenario/ArenaofAnnihilation/boss_scar_shell.cpp
  Scenario/ArenaofAnnihilation/boss_jolgrum.cpp
  Scenario/ArenaofAnnihilation/boss_little_liuyang.cpp
  Scenario/ArenaofAnnihilation/boss_chagan_firehoof.cpp
  Scenario/ArenaofAnnihilation/arena_of_annihilation.cpp
  Scenario/ArenaofAnnihilation/arena_of_annihilation.h
  
  Scenario/FallofShanBu/fall_of_shan_bu.cpp
  Scenario/FallofShanBu/instance_fall_of_shan_bu.cpp
  Scenario/FallofShanBu/fall_of_shan_bu.h
  
  Scenario/PursuingTheBlackHarvest/pursuing_the_black_harvest.cpp
  Scenario/PursuingTheBlackHarvest/instance_pursuing_the_black_harvest.cpp
  Scenario/PursuingTheBlackHarvest/pursuing_the_black_harvest.h
)

message("  -> Prepared: Scenario")
