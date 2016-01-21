

SET( JSONCPP_INCLUDE_DIR json )
SET( JSONCPP_SRC_DIR src )

SET( PUBLIC_HEADERS
    ${JSONCPP_INCLUDE_DIR}/config.h
    ${JSONCPP_INCLUDE_DIR}/forwards.h
    ${JSONCPP_INCLUDE_DIR}/features.h
    ${JSONCPP_INCLUDE_DIR}/value.h
    ${JSONCPP_INCLUDE_DIR}/reader.h
    ${JSONCPP_INCLUDE_DIR}/writer.h
    ${JSONCPP_INCLUDE_DIR}/assertions.h
    ${JSONCPP_INCLUDE_DIR}/version.h
)

SET(jsoncpp_sources
    ${JSONCPP_SRC_DIR}/json_tool.h
    ${JSONCPP_SRC_DIR}/json_reader.cpp
    ${JSONCPP_SRC_DIR}/json_valueiterator.inl
    ${JSONCPP_SRC_DIR}/json_value.cpp
    ${JSONCPP_SRC_DIR}/json_writer.cpp
    ${JSONCPP_SRC_DIR}/version.h.in
)

ADD_LIBRARY(jsoncpp STATIC ${PUBLIC_HEADERS} ${jsoncpp_sources})
set_target_properties(jsoncpp PROPERTIES LINKER_LANGUAGE CXX)

