cmake_minimum_required(VERSION 3.15)

project(Cafe.Io CXX)

set(CAFE_IO_INCLUDE_STREAMS ON CACHE BOOL "Include Cafe.Io.Streams")
set(CAFE_IO_INCLUDE_STREAM_HELPERS ON CACHE BOOL "Include Cafe.Io.StreamHelpers")

list(APPEND CAFE_OPTIONS
    CAFE_IO_INCLUDE_STREAMS
    CAFE_IO_INCLUDE_STREAM_HELPERS
)

include(${CMAKE_CURRENT_SOURCE_DIR}/CafeCommon/cmake/CafeCommon.cmake)

if(CAFE_IO_INCLUDE_STREAMS)
    add_subdirectory(Streams)
endif()

if(CAFE_IO_INCLUDE_STREAM_HELPERS)
    add_subdirectory(StreamHelpers)
endif()

if(CAFE_INCLUDE_TESTS)
    add_subdirectory(Test)
endif()
