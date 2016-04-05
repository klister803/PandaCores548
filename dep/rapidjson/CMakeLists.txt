

SET( RAPIDJSON_ERROR_DIR error )
SET( RAPIDJSON_INTERNAL_DIR internal )
SET( RAPIDJSON_MSINTTYPES_DIR msinttypes )

SET( PUBLIC_HEADERS
    document.h
    encodings.h
    filereadstream.h
    fwd.h
    memorybuffer.h
    msinttypes
    prettywriter.h
    reader.h
    stream.h
    writer.h
    allocators.h
    encodedstream.h
    filewritestream.h
    memorystream.h
    pointer.h
    rapidjson.h
    schema.h
    stringbuffer.h
    tutorial.cpp
)

SET(OTHER_HEADERS
    ${RAPIDJSON_ERROR_DIR}/en.h
    ${RAPIDJSON_ERROR_DIR}/error.h
    ${RAPIDJSON_INTERNAL_DIR}/biginteger.h
    ${RAPIDJSON_INTERNAL_DIR}/diyfp.h
    ${RAPIDJSON_INTERNAL_DIR}/dtoa.h
    ${RAPIDJSON_INTERNAL_DIR}/ieee754.h
    ${RAPIDJSON_INTERNAL_DIR}/itoa.h
    ${RAPIDJSON_INTERNAL_DIR}/meta.h
    ${RAPIDJSON_INTERNAL_DIR}/pow10.h
    ${RAPIDJSON_INTERNAL_DIR}/regex.h
    ${RAPIDJSON_INTERNAL_DIR}/stack.h
    ${RAPIDJSON_INTERNAL_DIR}/strfunc.h
    ${RAPIDJSON_INTERNAL_DIR}/strtod.h
    ${RAPIDJSON_INTERNAL_DIR}/swap.h
    ${RAPIDJSON_MSINTTYPES_DIR}/inttypes.h
    ${RAPIDJSON_MSINTTYPES_DIR}/stdint.h
)

ADD_LIBRARY(rapidjson STATIC ${PUBLIC_HEADERS} ${OTHER_HEADERS})
set_target_properties(rapidjson PROPERTIES LINKER_LANGUAGE CXX)

