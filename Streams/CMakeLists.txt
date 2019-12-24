set(CAFE_IO_STREAMS_USE_CONCEPTS ON CACHE BOOL "Use concepts in Cafe.Io.Streams")
set(CAFE_IO_STREAMS_INCLUDE_FILE_STREAM ON CACHE BOOL "Include FileStream in Cafe.Io.Streams")
set(CAFE_IO_STREAMS_FILE_STREAM_ENABLE_FILE_MAPPING ON CACHE BOOL "Enable file mapping in FileStreams")

configure_file(cmake/StreamConfig.h.in Cafe/Io/Streams/Config/StreamConfig.h)

set(SOURCE_FILES
    src/Cafe/Io/Streams/BufferedStream.cpp
    src/Cafe/Io/Streams/MemoryStream.cpp
    src/Cafe/Io/Streams/StlStream.cpp
    src/Cafe/Io/Streams/StreamBase.cpp)

set(HEADERS
    src/Cafe/Io/Streams/BufferedStream.h
    src/Cafe/Io/Streams/MemoryStream.h
    src/Cafe/Io/Streams/StlStream.h
    src/Cafe/Io/Streams/StreamBase.h)

if(CAFE_IO_STREAMS_INCLUDE_FILE_STREAM)
    list(APPEND SOURCE_FILES src/Cafe/Io/Streams/FileStream.cpp)
    list(APPEND HEADERS src/Cafe/Io/Streams/FileStream.h)
endif()

add_library(Cafe.Io.Streams ${SOURCE_FILES} ${HEADERS}
    ${CMAKE_CURRENT_BINARY_DIR}/Cafe/Io/Streams/Config/StreamConfig.h)

target_include_directories(Cafe.Io.Streams PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:include>)

target_link_libraries(Cafe.Io.Streams PUBLIC
    Cafe.ErrorHandling)

if(CAFE_IO_STREAMS_USE_CONCEPTS)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        target_compile_options(Cafe.Io.Streams PUBLIC -fconcepts)
    else()
        message(FATAL "Compiler ${CMAKE_CXX_COMPILER_ID} does not support concept for now")
    endif()
endif()

if(CAFE_IO_STREAMS_INCLUDE_FILE_STREAM)
    if(CMAKE_SYSTEM_NAME MATCHES "Windows")
        target_link_libraries(Cafe.Io.Streams PUBLIC
            Cafe.Encoding.UTF-16)
    endif()
    target_link_libraries(Cafe.Io.Streams PUBLIC
        Cafe.TextUtils.Misc)
endif()

install(TARGETS Cafe.Io.Streams
    EXPORT Io.Streams)

install(DIRECTORY src/
    TYPE INCLUDE
    FILES_MATCHING PATTERN "*.h")

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/Cafe/Io/Streams/Config/StreamConfig.h
    DESTINATION include/Cafe/Io/Streams/Config)

install(EXPORT Io.Streams
    DESTINATION cmake/Io.Streams)
