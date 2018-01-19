# Copyright (C) 2008-2016 TrinityCore <http://www.trinitycore.org/>
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

# Enable precompiled headers when using the GCC compiler.

message(STATUS "SCRIPT PREPARATIONS")

macro(PrepareScripts name out)
  file(GLOB_RECURSE found
    ${name}/*.h
    ${name}/*.cpp
  )
  list(APPEND ${out} ${found})
  message(STATUS "  -> Prepared: ${name}")
endmacro(PrepareScripts)

if(SCRIPTS)
  PrepareScripts(Spells PRIVATE_SOURCES)
  PrepareScripts(Commands PRIVATE_SOURCES)
  PrepareScripts(Custom PRIVATE_SOURCES)
  PrepareScripts(World PRIVATE_SOURCES)
  PrepareScripts(OutdoorPvP PRIVATE_SOURCES)
  PrepareScripts(Kalimdor PRIVATE_SOURCES)
  PrepareScripts(Outland PRIVATE_SOURCES)
  PrepareScripts(Northrend PRIVATE_SOURCES)
  PrepareScripts(Maelstrom PRIVATE_SOURCES)
endif()

message(STATUS "SCRIPT PREPARATION COMPLETE")
message("")

if (USE_SCRIPTPCH)
  set(PRIVATE_PCH_HEADER ScriptPCH.h)
endif ()

GroupSources(${CMAKE_CURRENT_SOURCE_DIR})

add_library(scripts STATIC
  ${PRIVATE_SOURCES}
  ${PRIVATE_PCH_SOURCE}
)

target_include_directories(scripts
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
  PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(scripts
  PUBLIC
    game)

set_target_properties(scripts
    PROPERTIES
      FOLDER
        "server")

# Generate precompiled header
if (USE_SCRIPTPCH)
  add_cxx_pch(scripts ${PRIVATE_PCH_HEADER})
endif()
