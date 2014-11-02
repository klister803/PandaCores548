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
	Custom/event_april.cpp
	Custom/Loskutik.cpp
	Custom/midsummer_fire_festival.cpp
	Custom/multi_vendor.cpp
	Custom/bracket.cpp
	Custom/traffic_log.cpp
	Custom/custom_reward.cpp
	Custom/command_arena.cpp
	Custom/npc_profession.cpp
	Custom/command_donate.cpp
)

message("  -> Prepared: Custom")
