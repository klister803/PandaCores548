# output generic information about the core and buildtype chosen
message("")
message("* UWOWCore rev. hash  : ${rev_hash}")
message("* UWOWCore rev. date  : ${rev_date}")
if( UNIX )
  message("* UWOWCore buildtype  : ${CMAKE_BUILD_TYPE}")
endif()
message("")

# output information about installation-directories and locations

message("* Install core to        : ${CMAKE_INSTALL_PREFIX}")
if( UNIX )
  message("* Install libraries to   : ${LIBSDIR}")
  message("* Install configs to     : ${CONF_DIR}")
endif()
message("")

# Show infomation about the options selected during configuration

if( SERVERS )
  message("* Build world/auth       : Yes (default)")
else()
  message("* Build world/authserver : No")
endif()

if( SCRIPTS )
  message("* Build with scripts     : Yes (default)")
  add_definitions(-DSCRIPTS)
else()
  message("* Build with scripts     : No")
  set(USE_SCRIPTPCH 0)
endif()

if( TOOLS )
  message("* Build map/vmap tools   : Yes")
else()
  message("* Build map/vmap tools   : No  (default)")
endif()

if( USE_COREPCH )
  message("* Build core w/PCH       : Yes (default)")
else()
  message("* Build core w/PCH       : No")
endif()

if( USE_SCRIPTPCH )
  message("* Build scripts w/PCH    : Yes (default)")
else()
  message("* Build scripts w/PCH    : No")
endif()

if( WITH_WARNINGS )
  message("* Show all warnings      : Yes")
else()
  message("* Show compile-warnings  : No  (default)")
endif()

if( WITH_COREDEBUG )
  message("* Use coreside debug     : Yes")
  add_definitions(-DTRINITY_DEBUG)
else()
  message("* Use coreside debug     : No  (default)")
endif()

if( WITH_SOURCE_TREE STREQUAL "flat" OR WITH_SOURCE_TREE STREQUAL "hierarchical" )
  # TODO: Remove this after Debian 8 is released and set general required version to 2.8.12
  #       Debian 7 is shipped with CMake 2.8.9 . But DIRECTORY flag of get_filename_component requires 2.8.12 .
  if (NOT CMAKE_VERSION VERSION_LESS 2.8.12)
    message("* Show source tree       : Yes - ${WITH_SOURCE_TREE}")
    set(_WITH_SOURCE_TREE ${WITH_SOURCE_TREE} CACHE INTERNAL "WITH_SOURCE_TREE support enabled.")
  else()
    message("* Show source tree       : No  (default)")

    message("")
    message(" *** WITH_SOURCE_TREE - WARNING!")
    message(" *** This functionality is ONLY supported on CMake 2.8.12 or higher.")
    message(" *** You are running ${CMAKE_VERSION}, which does not have the functions needed")
    message(" *** to create a sourcetree - this option is thus forced to disabled!")
    message("")

    set(_WITH_SOURCE_TREE "" CACHE INTERNAL "WITH_SOURCE_TREE support disabled.")
  endif()
else()
  message("* Show source tree       : No  (default)")
  set(_WITH_SOURCE_TREE "" CACHE INTERNAL "WITH_SOURCE_TREE support disabled.")
endif()

if( WIN32 )
  if( USE_MYSQL_SOURCES )
    message("* Use MySQL sourcetree   : Yes (default)")
  else()
    message("* Use MySQL sourcetree   : No")
  endif()
endif( WIN32 )

if ( NOJEM )
  message("")
  message("*** WARNING: jemalloc linking has been disabled!")
  message("*** Please note that this is for DEBUGGING WITH VALGRIND only!")
  message("*** DO NOT DISABLE IT UNLESS YOU KNOW WHAT YOU'RE DOING!")
endif()
message("")
