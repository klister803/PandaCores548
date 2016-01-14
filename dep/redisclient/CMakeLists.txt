
set(SRC_REDIS_DIR src/redisclient)

SET(SOURCES
    ${SRC_REDIS_DIR}/impl/redisvalue.cpp
    ${SRC_REDIS_DIR}/impl/redissyncclient.cpp
    ${SRC_REDIS_DIR}/impl/redisparser.cpp
    ${SRC_REDIS_DIR}/impl/redisclientimpl.cpp
    ${SRC_REDIS_DIR}/impl/redisasyncclient.cpp
)

SET(HEADERS
    ${SRC_REDIS_DIR}/config.h
    ${SRC_REDIS_DIR}/version.h
    ${SRC_REDIS_DIR}/redisvalue.h
    ${SRC_REDIS_DIR}/redissyncclient.h
    ${SRC_REDIS_DIR}/redisparser.h
    ${SRC_REDIS_DIR}/redisclient.h
    ${SRC_REDIS_DIR}/redisbuffer.h
    ${SRC_REDIS_DIR}/redisasyncclient.h
    ${SRC_REDIS_DIR}/impl/redisclientimpl.h
)

INCLUDE_DIRECTORIES(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${Boost_INCLUDE_DIRS}
)

ADD_LIBRARY(redisclient STATIC ${HEADERS} ${SOURCES})

TARGET_LINK_LIBRARIES(redisclient
    ${Boost_LIBRARIES}
)

set_target_properties(redisclient PROPERTIES LINKER_LANGUAGE CXX)
