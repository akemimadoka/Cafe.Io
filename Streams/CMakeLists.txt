set(CAFE_IO_STREAMS_INCLUDE_FILE_STREAM ON CACHE BOOL "Include FileStream in Cafe.Io.Streams")

configure_file(cmake/StreamConfig.h.in Cafe/Io/Streams/Config/StreamConfig.h)

set(SOURCE_FILES
    src/BufferedStream.cpp
    src/StreamBase.cpp)

set(HEADERS
    include/Cafe/Io/Streams/BufferedStream.h
    include/Cafe/Io/Streams/StreamBase.h)

if(CAFE_IO_STREAMS_INCLUDE_FILE_STREAM)
    list(APPEND SOURCE_FILES src/FileStream.cpp)
    list(APPEND HEADERS include/Cafe/Io/Streams/FileStream.h)
endif()

add_library(Cafe.Io.Streams ${SOURCE_FILES} ${HEADERS}
    ${CMAKE_CURRENT_BINARY_DIR}/Cafe/Io/Streams/Config/StreamConfig.h)

target_include_directories(Cafe.Io.Streams PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)

target_link_libraries(Cafe.Io.Streams PUBLIC
    Cafe.ErrorHandling)

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

install(DIRECTORY include/
    TYPE INCLUDE
    FILES_MATCHING PATTERN "*.h")

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/Cafe/Io/Streams/Config/StreamConfig.h
    DESTINATION include/Cafe/Io/Streams/Config)

install(EXPORT Io.Streams
    DESTINATION cmake/Io.Streams)
